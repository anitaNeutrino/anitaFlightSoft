/* sipcom.c - communicate with the SIP
 *
 * Marty Olevitch, May '05
 */

#include "sipcom.h"
#include "sipcom_impl.h"
#include "serial.h"
#include "highrate.h"
#include "sipthrottle.h"
#include "crc_simple.h"

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>	// malloc

#include <pthread.h>
#include <unistd.h>

#include <assert.h>

#include <sys/types.h>
#include <signal.h>
#include <wait.h>

#define COMM1_PORT	"/dev/ttyS2"
#define COMM2_PORT	"/dev/ttyS1"
#define HIGHRATE_PORT	"/dev/ttyS0"

// SLOWRATE_BAUD - line speed for the slow rate ports COMM1 and COMM2.
#define SLOWRATE_BAUD 1200

// HIGHRATE_BAUD - line speed for the high rate TDRSS port.
#define HIGHRATE_BAUD 19200

// Fd - file descriptors for COMM1, COMM2, and highrate TDRSS.
static int Fd[3];

// Reader_thread - thread handles for the COMM1 and COMM2 reader threads.
static pthread_t *Reader_thread[2] = {NULL, NULL};

// Slowrate_callback - slowrate callback function pointers. Set by
// sipcom_set_slowrate_callback(). One for each of COMM1 and COMM2.
static slowrate_callback Slowrate_callback[2] = { NULL, NULL};

// Init - nonzero if we have already been initialized.
static int Init = 0;

static void cancel_thread(int comm);
static int get_mesg(int comm, unsigned char *buf, int n, char *mesgtype);
static int highrate_write_bytes(unsigned char *p, int bytes_to_write);
static void read_comm(int *comm);  // parse the input from COMM1 or COMM2
static int readn(int fd, unsigned char *buf, int n);
static void showbuf(unsigned char *buf, int size);
static int start_reader(int comm); // start threads to read COMM1 or COMM2
				   // slow rate lines.

#define ERROR_STRING_LEN 2048
static char Error_string[ERROR_STRING_LEN];

void
set_error_string(char *fmt, ...)
{
    va_list ap;
    if (fmt == NULL) {
        Error_string[0] = '\0';
    } else {
        va_start(ap, fmt);
        vsnprintf(Error_string, ERROR_STRING_LEN, fmt, ap);
        va_end(ap);
    }
}

/* SIP message IDs */
#define GPS_POS		0x10
#define GPS_TIME	0x11
#define MKS_ALT		0x12
#define REQ_DATA	0x13
#define SCI_CMD		0x14
#define ANITA_DATA	0xad

/* Requests to the SIP */
#define REQ_GPS_POS	0x50
#define REQ_GPS_TIME	0x51
#define REQ_MKS_ALT	0x52
#define SCI_DATA_ID	0x53

#define DLE 0x10
#define ETX 0x03

/* Finite state automaton */
enum {
    WANT_DLE = 0,
    WANT_ID,
    IN_GPS_POS,
    IN_GPS_TIME,
    IN_MKS_ALT,
    IN_REQ_DATA,
    IN_SCI_CMD,
};

// Cmdlen - array of command lengths. A command consists of a single-byte
// ID followed by 0 or more bytes. The array index is the ID and the value
// is the length of that byte. A value of -1 indicates that the command has
// not been defined.
#define NCMD 255
static int Cmdlen[NCMD];

static cmd_callback Cmdf = NULL; // User-supplied function for science commands.

static void cmd_accum(unsigned char *buf, unsigned char *cmd_so_far,
    int *length_so_far);

#ifdef USE_STOR
FILE *Comm_stor_pipe[2];
char *Comm_data_dir[2] = {
    "/data/comm1",
    "/data/comm2"
};

FILE *Highrate_stor_pipe;
char *Highrate_data_dir = "/data/highrate";
#endif

// Quit is non-zero until sipcom_end() sets it to 1. Quit_mutex guards the
// Quit variable. Quit_cv is signalled by sipcom_end() to let sipcom_wait()
// (which has been waiting on Quit_cv) know that it should stop waiting and
// cancel the two reader threads.
static int Quit = 0;
static pthread_mutex_t Quit_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t Quit_cv = PTHREAD_COND_INITIALIZER;

int
sipcom_init(unsigned long throttle_rate)
{
    if (Init) {
	set_error_string(
	    "sipcom_init: already initialized. Run sipcom_reset() first.\n");
	return -3;
    }

    Init = 1;

    // Initialize the serial ports.
    Fd[0] = serial_init(COMM1_PORT, SLOWRATE_BAUD);
    Fd[1] = serial_init(COMM2_PORT, SLOWRATE_BAUD);
    Fd[2] = serial_init(HIGHRATE_PORT, HIGHRATE_BAUD);
    if (Fd[0] < 0 || Fd[1] < 0 || Fd[2] < 0) {
	return -1;
    }
    
    sipthrottle_init(throttle_rate, 1.0);

#ifdef USE_STOR
    {
	char cmdstr[128];
	sprintf(cmdstr, "storstuff %s 200000 4096", Comm_data_dir[0]);
	Comm_stor_pipe[0] = popen(cmdstr, "w");
	if (Comm_stor_pipe[0] == NULL) {
	    set_error_string(
	    	"sipcom_init: Comm_stor_pipe[0] '%s' failed.\n", cmdstr);
	    return -4;
	}

	sprintf(cmdstr, "storstuff %s 200000 4096", Comm_data_dir[1]);
	Comm_stor_pipe[1] = popen(cmdstr, "w");
	if (Comm_stor_pipe[1] == NULL) {
	    set_error_string(
	    	"sipcom_init: Comm_stor_pipe[1] '%s' failed.\n", cmdstr);
	    return -4;
	}

	sprintf(cmdstr, "storstuff %s 200000 4096", Highrate_data_dir);
	Highrate_stor_pipe = popen(cmdstr, "w");
	if (Highrate_stor_pipe == NULL) {
	    set_error_string(
	    	"sipcom_init: Highrate_stor_pipe '%s' failed.\n", cmdstr);
	    return -4;
	}
    }
#endif

    // Start the SIP reader threads.
    {
	if (start_reader(COMM1)) {
	    return -2;
	}
	if (start_reader(COMM2)) {
	    return -2;
	}
    }

    return 0;
}

void
sipcom_end()
{
    pthread_mutex_lock(&Quit_mutex);
    Quit = 1;
    pthread_cond_signal(&Quit_cv);
    pthread_mutex_unlock(&Quit_mutex);
}

static void
cancel_thread(int comm)
{
    if (Reader_thread[comm] != NULL) {
	//fprintf(stderr, "cancel_thread %d\n", comm);
	pthread_cancel(*Reader_thread[comm]);
	free(Reader_thread[comm]);
    }
}

void
sipcom_wait()
{
    pthread_mutex_lock(&Quit_mutex);
    while (!Quit) {
	pthread_cond_wait(&Quit_cv, &Quit_mutex);
    }
    cancel_thread(COMM1);
    cancel_thread(COMM2);
}

void
sipcom_reset()
{
    Init = 0;
}

void
sipcom_set_cmd_callback(cmd_callback f)
{
    Cmdf = f;
}

int
sipcom_set_slowrate_callback(int comm, slowrate_callback f)
{
    if (COMM1 != comm && COMM2 != comm) {
	set_error_string(
	    "%s: comm must be either COMM1 or COMM2, not %d\n",
	    "sipcom_set_slowrate_callback", comm);
	return -1;
    }
    Slowrate_callback[comm] = f;
    return 0;
}

char *
sipcom_strerror()
{
    return Error_string;
}

static int
start_reader(int comm)
{
    int ret;

    assert(COMM1 == comm || COMM2 == comm);

    Reader_thread[comm] = (pthread_t *)malloc(sizeof(pthread_t));

    if (Reader_thread[comm] == NULL) {
	set_error_string("start_reader(%d): can't malloc (%s)\n",
	    comm, strerror(errno));
	return -1;
    }

    ret = pthread_create(Reader_thread[comm], NULL,
	(void *)read_comm, (void *)&comm);
    if (ret) {
	set_error_string("start_reader(%d): bad pthread_create (%s)\n",
	    comm, strerror(errno));
	return -1;
    }
    pthread_detach(*Reader_thread[comm]);

    return 0;
}

int
sipcom_set_cmd_length(unsigned char id, unsigned char len)
{
    static int cmdlen_init = 0;

    if (!cmdlen_init) {
	int i;
	for (i=0; i<NCMD; i++) {
	    Cmdlen[i] = -1;
	}
	cmdlen_init = 1;
    }

    Cmdlen[id] = len;
    return 0;
}

static void
read_comm(int *comm)
{
    ssize_t bytes_read;
    unsigned char ch;
    int state;	// automaton state
    int mycomm = *comm;

#define CMD_MAX_LEN 32
    unsigned char cmd_so_far[CMD_MAX_LEN];
    int length_so_far = 0;

#define MESGBUFSIZE 32
    unsigned char mesgbuf[MESGBUFSIZE];

    assert(COMM1 == mycomm || COMM2 == mycomm);
    memset(mesgbuf, '\0', MESGBUFSIZE);
    state = WANT_DLE;

    {
	// We make this thread cancellable by any thread, at any time. This
	// should be okay since we don't have any state to undo or locks to
	// release.
	int oldtype;
	int oldstate;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    }

    while (1) {
	bytes_read = read(Fd[mycomm], &ch, 1);
	if (bytes_read == 0) {
	    // EOF
	    break;
	} else if (bytes_read == -1) {
	    // error
	    set_error_string("read_comm(%d) bad read (%s)\n",
	    	mycomm, strerror(errno));
	    break;
	}

	if (state == WANT_DLE) {
	    // Start of message
	    if (ch == DLE) {
		state = WANT_ID;
	    }

	} else if (state == WANT_ID) {
	    // Kind of message
	    if (ch == GPS_POS) {
		if (get_mesg(mycomm,mesgbuf, 15, "GPS_POS") == -1) {
		    continue;
		}
		fprintf(stderr, "GPS_POS (%d): \n", mycomm);
	    
	    } else if (ch == GPS_TIME) {
		if (get_mesg(mycomm,mesgbuf, 15, "GPS_TIME") == -1) {
		    continue;
		}
		fprintf(stderr, "GPS_TIME (%d): \n", mycomm);

	    } else if (ch == MKS_ALT) {
		if (get_mesg(mycomm,mesgbuf, 6, "MKS_ALT") == -1) {
		    continue;
		}
		fprintf(stderr, "MKS_ALT (%d): \n", mycomm);

	    } else if (ch == REQ_DATA) {
		if (get_mesg(mycomm,mesgbuf, 1, "REQ_DATA") == -1) {
		    continue;
		}
		if (Slowrate_callback[mycomm] != NULL) {
		    Slowrate_callback[mycomm]();
		}
		//fprintf(stderr, "REQ_DATA (%d): \n", mycomm);

	    } else if (ch == SCI_CMD) {
		if (get_mesg(mycomm,mesgbuf, 4, "SCI_CMD") == -1) {
		    continue;
		}
		cmd_accum(mesgbuf, cmd_so_far, &length_so_far);

	    } else {
		// Unknown id.
		;
	    }

	    state = WANT_DLE;
	}
    }
}

static int
get_mesg(int comm, unsigned char *buf, int n, char *mesgtype)
{
    int nrd;
    assert(COMM1 == comm || COMM2 == comm);
    nrd = readn(Fd[comm], buf, n);
    if (buf[n-1] != ETX) {
	fprintf(stderr, "No ETX at end of %s\n", mesgtype);
	return -1;
    }
    return 0;
}

static int
readn(int fd, unsigned char *buf, int n)
{
    int i;
    int got;

    for (i=0; i<n; i++) {
	got = read(fd, buf+i, 1);
	if (got != 1) {
	    return got;
	}
    }

    return n;
}

int
sipcom_slowrate_write(int comm, unsigned char *buf, unsigned char nbytes)
{
#define BUF_SIZE 255
    unsigned char xferbuf[BUF_SIZE];
    ssize_t ret;
    static unsigned char bufseq = 0;

#define OVERHEAD_BYTES 7
#define DATA_OFFSET	(OVERHEAD_BYTES - 1)
#define EXTRA_DATA_BYTES (OVERHEAD_BYTES - 4)
#define MAX_USER_BYTES (BUF_SIZE - OVERHEAD_BYTES)
    if (nbytes > MAX_USER_BYTES) {
	set_error_string(
	    "sipcom_slowrate_write: nbytes must be < %d, not %d\n",
	    MAX_USER_BYTES, nbytes);
	return -1;
    }

    if (buf == NULL) {
	set_error_string("sipcom_slowrate_write: NULL buffer!\n");
	return -2;
    }

    if (COMM1 != comm && COMM2 != comm) {
	set_error_string(
	    "%s: comm must be either COMM1 or COMM2, not %d\n",
	    "sipcom_slowrate_write", comm);
	return -3;
    }

    xferbuf[0] = DLE;
    xferbuf[1] = SCI_DATA_ID;
    xferbuf[2] = nbytes + EXTRA_DATA_BYTES;

    // Insert our header: it consists of three bytes.
    //
    // Byte 1: SLOW_ID_COMM1 or SLOW_ID_COMM2.
    // 
    // Byte 2: sequence number.
    //     Each time through this subroutine increments the value.
    //     It resets to 0 after 255.
    // 
    // Byte 3: number of data bytes that follow.

#define SLOW_ID_COMM1 0xc1
#define SLOW_ID_COMM2 0xc2

    if (COMM1 == comm) {
	xferbuf[3] = SLOW_ID_COMM1;
    } else {
	xferbuf[3] = SLOW_ID_COMM2;
    }
    
    xferbuf[4] = bufseq;
    ++bufseq;
    if (bufseq > 255) {
	bufseq = 0;
    }

    xferbuf[5] = nbytes;

    memcpy(xferbuf + DATA_OFFSET, buf, nbytes);

    xferbuf[DATA_OFFSET + nbytes] = ETX;

    //showbuf(xferbuf, nbytes + OVERHEAD_BYTES);
    ret = write(Fd[comm], xferbuf, OVERHEAD_BYTES + nbytes);
    if (ret != OVERHEAD_BYTES + nbytes) {
	set_error_string("%s: write error (%s)\n",
	    "sipcom_slowrate_write", strerror(errno));
	return -4;
    }

#ifdef USE_STOR
    {
	FILE *pipe_ = Comm_stor_pipe[comm];
	if (pipe_ != NULL) {
	    int len = OVERHEAD_BYTES + nbytes;
	    ret = fwrite(xferbuf, 1, len, pipe_);
	    if (ret != len) {
		fprintf(stderr, "bad write to Comm_stor_pipe[%d]\n", comm);
	    }
	    fflush(pipe_);
	}
    }
#endif

    return 0;
}

static void
showbuf(unsigned char *buf, int size)
{
    int i;
    for (i=0; i<size; i++) {
	fprintf(stderr, "%02x ", buf[i]);
    }
}

static void
cmd_accum(unsigned char *buf, unsigned char *cmd_so_far, int *length_so_far)
{
    /* Note that each science command is broken into pieces by NSBF. Each
     * piece has the following format: DLE (0x10), ID (0x14), length
     * (always 2), cmd byte 1, cmd byte 2, and ETX (0x03). Cmd byte1 is a
     * sequence number and Cmd byte 2 is the actual command data. Thus we
     * get one of our command bytes in each of these packets. This function
     * takes care of restoring the original command and then passing to
     * back to the callback. We receive here as input only the last 4 bytes
     * of each command packet, ie: length, sequence, cmd, and etx bytes. */
    unsigned char id;

    unsigned char seq = buf[1];
    unsigned char cmdbyte = buf[2];

    if (seq != *length_so_far) {
	// Out of sequence packet. Reset everything.
	*length_so_far = 0;
	return;
    }

    cmd_so_far[*length_so_far] = cmdbyte;

    ++(*length_so_far);

    id = cmd_so_far[0];

    if (Cmdlen[id] == *length_so_far) {
	// We have a complete command. Execute the callback.
    	if (Cmdf != NULL) {
	    Cmdf(cmd_so_far);
	}
	*length_so_far = 0;
	return;

    } else if (Cmdlen[id] == -1) {
	// Unknown command. Reset everything. Let callback handle the cmd.
	if (Cmdf != NULL) {
	    Cmdf(cmd_so_far);
	}
	*length_so_far = 0;
	return;
    }
}

int
sipcom_highrate_write(unsigned char *buf, unsigned short nbytes)
{
    // aligned_bufp - used for calculating the checksum.
    static unsigned short *aligned_bufp = NULL;
    static int aligned_nbytes = 0;

    static unsigned long Bufcnt = 1L;
    int odd_number_of_science_bytes = 0;
    int retval = 0;

#define HBUFSIZE 6
    unsigned short header_buf[HBUFSIZE];

    /* Ender handling. Kludge. If the number of science data bytes is even,
     * then the ender consists of the following 16-bit words: checksum,
     * SW_END_HDR, END_HDR, AUX_HDR. However, if the number is odd, then we
     * prepend a single byte of 0x00 to the above.
     *
     * We put the checksum, SW_END_HDR, END_HDR, and AUX_HDR into the
     * ender_buf starting at offset 1 (NOT offset 0) and set enderp to
     * point at that location. If the number of bytes is even, then, we
     * simply write the ender_buf starting at enderp. However, if the
     * number is odd, then we take uchar *cp = ((uchar *)ender_buf) - 1;
     * and write the 0x00 to *cp. Then, we set enderp to cp, and write the
     * ender_buf from that location. */
    unsigned short ender_buf[HBUFSIZE];
    unsigned char *enderp = (unsigned char *)(ender_buf + 1);
    int endersize = 8;

    if (nbytes == 0) {
	return 0;
    }

    if (nbytes % 2) {
	odd_number_of_science_bytes = 1;
    }

    if (aligned_nbytes < nbytes) {
	// (Re)allocate the aligned buffer if it is not big enough.
	free(aligned_bufp);
	aligned_nbytes = nbytes;
	aligned_bufp = (unsigned short *)malloc(nbytes);
	if (NULL == aligned_bufp) {
	    set_error_string("sipcom_highrate_write: bad malloc (%s).\n",
	    	strerror(errno));
	    return -1;
	}
    }

    memcpy(aligned_bufp, buf, nbytes);

    header_buf[0] = START_HDR;
    header_buf[1] = AUX_HDR;
    
    header_buf[2] = ID_HDR;
    header_buf[2] |= 0x0001;	// SIP data

    if (odd_number_of_science_bytes) {
	header_buf[2] |= 0x0002;
	header_buf[5] = nbytes + 1;
	enderp--;
	*enderp = '\0';	// extra byte
	endersize++;
    } else {
	header_buf[5] = nbytes;
    }

    {
	unsigned short *sp;
	sp = (unsigned short *)&Bufcnt;
	header_buf[3] = *sp;
	header_buf[4] = *(sp+1);
    }

    ender_buf[1] = crc_short(aligned_bufp, nbytes / sizeof(short));
    ender_buf[2] = SW_END_HDR;
    ender_buf[3] = END_HDR;
    ender_buf[4] = AUX_HDR;

    // Write header.
    if (retval = highrate_write_bytes(
	    (unsigned char *)header_buf, HBUFSIZE * sizeof(short))) {
	return retval;
    }

    // Write data.
    if (retval = highrate_write_bytes(buf, nbytes)) {
	return retval;
    }

    // Write ender.
    if (retval = highrate_write_bytes(enderp, endersize)) {
	return retval;
    }

    Bufcnt++;

    return 0;
}

void
sipcom_highrate_set_throttle(unsigned long rate)
{
    sipthrottle_set_bytes_per_sec_limit(rate);
    sipthrottle_set_frac_sec(1.0);
}

static int
highrate_write_bytes(unsigned char *p, int bytes_to_write)
{
    int bytes_avail = 0;
    int retval = 0;
    int writeloc = 0;

    while (1) {
	bytes_avail = sipthrottle(bytes_to_write);

	if (bytes_avail == 0) {
	    // No room yet.
	    usleep(1000);
	    continue;

	} else if (bytes_avail > 0) {
try_write:
	    // write bytes_avail bytes
	    if (write(Fd[HIGHRATE], p + writeloc, bytes_avail) == -1) {
		if (errno == EAGAIN) {
		    goto try_write;
		}

		// other write error
		retval = -1;
		break;
	    }

#ifdef USE_STOR
	    {
		FILE *pipe_ = Highrate_stor_pipe;
		int ret;
		int len;
		if (pipe_ != NULL) {
		    len = bytes_avail;
		    ret = fwrite(p + writeloc, 1, len, pipe_);
		    if (ret != len) {
			fprintf(stderr, "bad write to Highrate_stor_pipe\n");
		    }
		    fflush(pipe_);
		}
	    }
#endif

	    bytes_to_write -= bytes_avail;
	    writeloc += bytes_avail;

	    if (bytes_to_write == 0) {
		// wrote all bytes
		retval = 0;
		break;
	    }

	} else {
	    // bytes_avail < 0
	    retval = -3;
	    break;
	}
    }

    return 0;
}
