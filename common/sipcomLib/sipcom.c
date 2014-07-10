/* sipcom.c - communicate with the SIP
 *
 * Marty Olevitch, May '05
 */

#include "sipcom.h"
#include "sipcom_impl.h"
#include "serial.h"
#include "highrate.h"
#include "sipthrottle.h"
#include "telemwrap.h"
#include "crc_simple.h"
#include "conf_simple.h"

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>	// malloc

#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <assert.h>

#include <sys/types.h>
#include <signal.h>
#include <wait.h>

// Default port values
#define COMM1_PORT	"/dev/ttyS0"
#define COMM2_PORT	"/dev/ttyS1"
#define HIGHRATE_PORT	"/dev/ttyS2"
#define VERY_HIGHRATE_PORT	"/dev/ttyS3"

/* Sipmux_enable - a bitmask of which lines to enable.
 *    If a bit is set to 1, then that line will use the SIPMUX protocol for
 *    data transfer, else, if the bit is cleared to zero, it will not use
 *    SIPMUX on that line.
 *    
 *    The COMM1, COMM2, HIGHRATE (OMNI), and VERY_HIGHRATE (HGA) macros (in
 *    sipcom.h) should be used to access the bits.
 */
static unsigned char Sipmux_enable = 0;

// SLOWRATE_BAUD - line speed for the slow rate ports COMM1 and COMM2.
#define SLOWRATE_BAUD 1200

// HIGHRATE_BAUD - line speed for the high rate OMNI antenna port.
// Goes via TDRSS.
#define HIGHRATE_BAUD 115200

// VERY_HIGHRATE_BAUD - line speed for the high rate HGA antenna port.
// Goes via TDRSS.
#define VERY_HIGHRATE_BAUD 115200

// Fd - file descriptors for COMM1, COMM2, highrate OMNI, and highrate HGA.
static int Fd[4];

// Reader_thread - thread handles for the COMM1 and COMM2 reader threads.
static pthread_t *Reader_thread[2] = {NULL, NULL};

// Slowrate_callback - slowrate callback function pointers. Set by
// sipcom_set_slowrate_callback(). One for each of COMM1 and COMM2.
static slowrate_callback Slowrate_callback[2] = { NULL, NULL};

// Gps_callback - gps callback function pointer. Set by
// sipcom_set_gps_callback().
static gps_callback Gps_callback = NULL;

// Gpstime_callback - gpstime callback function pointer. Set by
// sipcom_set_gpstime_callback().
static gpstime_callback Gpstime_callback = NULL;

// Mks_press_alt_callback - mks pressure altitude callback function
// pointer. Set by sipcom_set_mks_press_alt_callback().
static mks_press_alt_callback Mks_press_alt_callback = NULL;

// Init - nonzero if we have already been initialized.
static int Init = 0;

static void cancel_thread(int comm);
static int get_mesg(int comm, unsigned char *buf, int n, char *mesgtype);
static int highrate_write_bytes(unsigned char *p, int bytes_to_write);
static int very_highrate_write_bytes(unsigned char *p, int bytes_to_write);

#ifndef PTHREAD_CREATE_SEEMS_TO_WORK
    /*
     * Kludge: I don't know why, but the last parameter to pthread_create()
     * is not getting passed correctly to the read_comm() function. Made a
     * intermediate function read_comm1() and read_comm2() to do it. The
     * original code, which worked under Redhat 9, is below in start_reader().
     * (MAO 2-21-08)
     *
     * It seems to be working again with Debian 5.0.
     * (MAO 7-7-10)
     */
    static void read_comm1(int *dummy);
    static void read_comm2(int *dummy);
#endif

static void read_comm(int *comm);  // parse the input from COMM1 or COMM2
static int readn(int fd, unsigned char *buf, int n);
//static void showbuf(unsigned char *buf, int size);
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

FILE *Very_highrate_stor_pipe;
char *Very_highrate_data_dir = "/data/very_highrate";
#endif

// Quit is non-zero until sipcom_end() sets it to 1. Quit_mutex guards the
// Quit variable. Quit_cv is signalled by sipcom_end() to let sipcom_wait()
// (which has been waiting on Quit_cv) know that it should stop waiting and
// cancel the two reader threads.
static int Quit = 0;
static pthread_mutex_t Quit_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t Quit_cv = PTHREAD_COND_INITIALIZER;

static int Conf_token = -1;

int
sipcom_init(unsigned long throttle_rate, char *config_dir,
                                                unsigned char _sipmux_enable)
{
    if (Init) {
	set_error_string(
	    "sipcom_init: already initialized. Run sipcom_reset() first.\n");
	return -3;
    }

    Init = 1;

    Conf_token = conf_set_dir(config_dir);

    // Initialize the serial ports.
    long slowrate_baud = SLOWRATE_BAUD;
    long highrate_baud = HIGHRATE_BAUD;
    long very_highrate_baud = VERY_HIGHRATE_BAUD;
    
    char Comm1_port[CONF_SIMPLE_VALUE_SIZE];
    char Comm2_port[CONF_SIMPLE_VALUE_SIZE];
    char Highrate_port[CONF_SIMPLE_VALUE_SIZE];
    char Very_highrate_port[CONF_SIMPLE_VALUE_SIZE];

    {
	int ret;
        char tmpstr[CONF_SIMPLE_VALUE_SIZE];

	ret = conf_read(Conf_token, "Comm1_port", Comm1_port);
	if (ret) {
	    // Use default.
	    strncpy(Comm1_port, COMM1_PORT, CONF_SIMPLE_VALUE_SIZE);
	}

	ret = conf_read(Conf_token, "Comm2_port", Comm2_port);
	if (ret) {
	    // Use default.
	    strncpy(Comm2_port, COMM2_PORT, CONF_SIMPLE_VALUE_SIZE);
	}

	ret = conf_read(Conf_token, "Highrate_port", Highrate_port);
	if (ret) {
	    // Use default.
	    strncpy(Highrate_port, HIGHRATE_PORT, CONF_SIMPLE_VALUE_SIZE);
	}

	ret = conf_read(Conf_token, "Very_highrate_port", Very_highrate_port);
	if (ret) {
	    // Use default.
	    strncpy(Very_highrate_port, VERY_HIGHRATE_PORT,
	    						CONF_SIMPLE_VALUE_SIZE);
	}

	ret = conf_read(Conf_token, "Slowrate_baud", tmpstr);
	if (!ret) {
	    // Use value from config file.
            slowrate_baud = atol(tmpstr);
	}
	ret = conf_read(Conf_token, "Highrate_baud", tmpstr);
	if (!ret) {
	    // Use value from config file.
            highrate_baud = atol(tmpstr);
	}
	ret = conf_read(Conf_token, "Very_highrate_baud", tmpstr);
	if (!ret) {
	    // Use value from config file.
            very_highrate_baud = atol(tmpstr);
	}

        //fprintf(stderr, "                   SIPCOM: s %ld  h %ld  vh %ld\n",
        //    slowrate_baud, highrate_baud, very_highrate_baud);

    }

    Sipmux_enable = _sipmux_enable;

    Fd[0] = serial_init(Comm1_port, slowrate_baud, Sipmux_enable & (1<<COMM1));
    if (Fd[0] < 0) {
	set_error_string("sipcom_init: Bad serial_init for comm1 (%s)",
								Comm1_port);
    }

    Fd[1] = serial_init(Comm2_port, slowrate_baud, Sipmux_enable & (1<<COMM2));
    if (Fd[1] < 0) {
	set_error_string("sipcom_init: Bad serial_init for comm2 (%s)",
								Comm2_port);
    }

    Fd[2] = serial_init(Highrate_port, highrate_baud,
                                                Sipmux_enable & (1<<HIGHRATE));
    if (Fd[2] < 0) {
	set_error_string("sipcom_init: Bad serial_init for highrate (%s)",
								Highrate_port);
    }

    Fd[3] = serial_init(Very_highrate_port, very_highrate_baud,
                                            Sipmux_enable & (1<<VERY_HIGHRATE));
    if (Fd[3] < 0) {
	set_error_string("sipcom_init: Bad serial_init for very highrate (%s)",
							    Very_highrate_port);
    }

    if (Fd[0] < 0 || Fd[1] < 0 || Fd[2] < 0 || Fd[3] < 0) {
	return -1;
    }

    if (telemwrap_init(TW_SIP)) {
	return -2;
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

	sprintf(cmdstr, "storstuff %s 200000 4096", Very_highrate_data_dir);
	Very_highrate_stor_pipe = popen(cmdstr, "w");
	if (Very_highrate_stor_pipe == NULL) {
	    set_error_string(
	    	"sipcom_init: Very_highrate_stor_pipe '%s' failed.\n", cmdstr);
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

void sipcom_set_gps_callback(gps_callback f)
{
    Gps_callback = f;
}

void sipcom_set_gpstime_callback(gpstime_callback f)
{
    Gpstime_callback = f;
}

void sipcom_set_mks_press_alt_callback(mks_press_alt_callback f)
{
    Mks_press_alt_callback = f;
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

    //assert(COMM1 == comm || COMM2 == comm);

    Reader_thread[comm] = (pthread_t *)malloc(sizeof(pthread_t));

    if (Reader_thread[comm] == NULL) {
	set_error_string("start_reader(%d): can't malloc (%s)\n",
	    comm, strerror(errno));
	return -1;
    }

#ifdef PTHREAD_CREATE_SEEMS_TO_WORK
    ret = pthread_create(Reader_thread[comm], NULL,
	(void *)read_comm, (void *)&comm);
#else
    if (COMM1 == comm) {
	ret = pthread_create(Reader_thread[comm], NULL,
	    (void *)read_comm1, NULL);
    } else {
	ret = pthread_create(Reader_thread[comm], NULL,
	    (void *)read_comm2, NULL);
    }
#endif
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

#ifndef PTHREAD_CREATE_SEEMS_TO_WORK
static void
read_comm1(int *dummy)
{
    int comm = COMM1;
    read_comm(&comm);
}

static void
read_comm2(int *dummy)
{
    int comm = COMM2;
    read_comm(&comm);
}
#endif // PTHREAD_CREATE_SEEMS_TO_WORK

static void
 swapw(unsigned short *valp)
{
    unsigned char *p = (unsigned char *)valp;
    unsigned char tmp = *p;
    *p = *(p+1);
    *(p+1) = tmp;
}

static void
read_comm(int *comm)
{
    ssize_t bytes_read;
    unsigned char ch;
    int state;	// automaton state
    int mycomm = *comm;
    //static unsigned int reqcount = 0;

#define CMD_MAX_LEN 32
    unsigned char cmd_so_far[CMD_MAX_LEN];
    int length_so_far = 0;

#define MESGBUFSIZE 32
    unsigned char mesgbuf[MESGBUFSIZE];

    //assert(COMM1 == mycomm || COMM2 == mycomm);

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
		if (Gps_callback != NULL) {
		    float longi, lat, alt;
		    //unsigned char satstat1, satstat2;
		    memcpy(&longi, mesgbuf,   4);
		    memcpy(&lat,   mesgbuf+4, 4);
		    memcpy(&alt,   mesgbuf+8, 4);
//		    printf("sipcom: GPS longi %.2f  lat %.2f  alt %.2f "
//		           "sat1 %u  sat2 %u\n"
//			   , longi, lat, alt, satstat1, satstat2);
		    Gps_callback(longi, lat, alt);
		}
		//fprintf(stderr, "GPS_POS (%d): \n", mycomm);
	    
	    } else if (ch == GPS_TIME) {
		if (get_mesg(mycomm,mesgbuf, 15, "GPS_TIME") == -1) {
		    continue;
		}
		if (Gpstime_callback != NULL) {
		    float time_of_week, utc_time_offset, cpu_time;
		    unsigned short week_number;
		    memcpy(&time_of_week,      mesgbuf,    4);
		    memcpy(&week_number,       mesgbuf+4,  2);
		    memcpy(&utc_time_offset,   mesgbuf+6,  4);
		    memcpy(&cpu_time,          mesgbuf+10, 4);
//		    printf(
//		       "sipcom: tow %.2f  week_no %u  offset %.2f  cpu %.2f\n",
//			 time_of_week, week_number, utc_time_offset, cpu_time);
		    Gpstime_callback( time_of_week, week_number,
		    				utc_time_offset, cpu_time);
		}
		//fprintf(stderr, "GPS_TIME (%d): \n", mycomm);

	    } else if (ch == MKS_ALT) {
		if (get_mesg(mycomm,mesgbuf, 7, "MKS_ALT") == -1) {
		    continue;
		}
                if (Mks_press_alt_callback != NULL) {
                    unsigned short hi, mid, lo;
                    memcpy(&hi,  mesgbuf,    2);
                    memcpy(&mid, mesgbuf+2,  2);
                    memcpy(&lo,  mesgbuf+4,  2);
		    // We hvae to swap the bytes of the three MKS words.
		    swapw(&hi);
		    swapw(&mid);
		    swapw(&lo);
                    Mks_press_alt_callback(hi, mid , lo);
                    //printf("sipcom: mks hi %u  mks mid %u  mks lo %u\n",
                    //    hi, mid, lo);
                }
		//fprintf(stderr, "MKS_ALT (%d): \n", mycomm);

	    } else if (ch == REQ_DATA) {
		if (get_mesg(mycomm,mesgbuf, 1, "REQ_DATA") == -1) {
		    continue;
		}
		if (Slowrate_callback[mycomm] != NULL) {
		    Slowrate_callback[mycomm]();
		}
//		fprintf(stderr, "REQ_DATA (COMM%d) [%u]: \n",
//						    mycomm+1, reqcount++);

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
  //RJN hack to lose warning
  //    int nrd;
    //assert(COMM1 == comm || COMM2 == comm);
    //    nrd = readn(Fd[comm], buf, n);
    readn(Fd[comm], buf, n);
    if (buf[n-1] != ETX) {
	//fprintf(stderr, "No ETX at end of %s\n", mesgtype);
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

static int
do_request(int req, char *caller_name)
{
    unsigned char xferbuf[3];
    xferbuf[0] = DLE;
    xferbuf[1] = req;
    xferbuf[2] = ETX;

    int comm = COMM2;
    int ret = 0;

    if (Sipmux_enable & (1 << comm)) {
        ret = serial_sipmux_begin(Fd[comm], 500000L);
        if (ret) {
            // serial_sipmux_begin() should have set the error string.
            return -5;
        }
    }

    //showbuf(xferbuf, nbytes + OVERHEAD_BYTES);
    ret = write(Fd[comm], xferbuf, 3);
    if (ret != 3) {
	set_error_string("%s: write error (%s)\n",
	    caller_name, strerror(errno));
	return -4;
    }

    if (Sipmux_enable & (1 << comm)) {
        ret = serial_sipmux_end(Fd[comm], 500000L);
        if (ret) {
            // serial_sipmux_end() should have set the error string.
            return -5;
        }
    }

    return 0;
}

int
sipcom_request_gps_pos(void)
{
    return do_request(REQ_GPS_POS, "sipcom_request_gps_pos");
}

int
sipcom_request_gps_time(void)
{
    return do_request(REQ_GPS_TIME, "sipcom_request_gps_time");
}

int sipcom_request_mks_pressure_altitude(void)
{
    return do_request(REQ_MKS_ALT, "sipcom_request_mks_pressure_altitude");
}

int
sipcom_slowrate_write(int comm, unsigned char *buf, unsigned char nbytes)
{
#define BUF_SIZE 255
    unsigned char xferbuf[BUF_SIZE];
    ssize_t ret;
    static unsigned char c1_bufseq = 0;
    static unsigned char c2_bufseq = 0;
    unsigned char bufseq = 0;

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
	bufseq = c1_bufseq;
	++c1_bufseq;
    } else {
	xferbuf[3] = SLOW_ID_COMM2;
	bufseq = c2_bufseq;
	++c2_bufseq;
    }
    
    xferbuf[4] = bufseq;

    xferbuf[5] = nbytes;

    memcpy(xferbuf + DATA_OFFSET, buf, nbytes);

    xferbuf[DATA_OFFSET + nbytes] = ETX;

    if (Sipmux_enable & (1 << comm)) {
        ret = serial_sipmux_begin(Fd[comm], 500000L);
        if (ret) {
            // serial_sipmux_begin() should have set the error string.
            return -5;
        }
    }

    //showbuf(xferbuf, nbytes + OVERHEAD_BYTES);
    ret = write(Fd[comm], xferbuf, OVERHEAD_BYTES + nbytes);
    if (ret != OVERHEAD_BYTES + nbytes) {
	set_error_string("%s: write error (%s)\n",
	    "sipcom_slowrate_write", strerror(errno));
	return -4;
    }

    if (Sipmux_enable & (1 << comm)) {
        ret = serial_sipmux_end(Fd[comm], 500000L);
        if (ret) {
            // serial_sipmux_end() should have set the error string.
            return -5;
        }
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

#ifdef NOTDEF
static void
showbuf(unsigned char *buf, int size)
{
    int i;
    for (i=0; i<size; i++) {
	fprintf(stderr, "%02x ", buf[i]);
    }
}
#endif

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

unsigned short *
sipcom_wrap_buffer(unsigned char *buf, unsigned short nbytes,
                                                short *wrapbytes)
{
    static const int extra_bytes = 128;
    static int nbytes_plus = 0;
    static unsigned short *wrapbuf = NULL;

    *wrapbytes = 0;

    if (nbytes == 0) {
	return NULL;
    }

    if (nbytes_plus < nbytes + extra_bytes) {
	// (Re)allocate the wrap buffer if it is not big enough.
	nbytes_plus = nbytes + extra_bytes;
	wrapbuf = (unsigned short *)realloc(wrapbuf, nbytes_plus);
	if (NULL == wrapbuf) {
	    set_error_string("sipcom_wrap_buffer: bad realloc (%s).\n",
	    	strerror(errno));
            *wrapbytes = -1;
	    return NULL;
	}
    }

    *wrapbytes = telemwrap((unsigned short *)buf, wrapbuf, nbytes,
                                                    SIPCOM_LOS, TW_LOS);

    return wrapbuf;
}

int
sipcom_highrate_write(unsigned char *buf, unsigned short nbytes)
{
    static const int extra_bytes = 128;
    static int nbytes_plus = 0;
    int retval = 0;
    static unsigned short *wrapbuf = NULL;
    int wrapbytes = 0;

    if (nbytes == 0) {
	return 0;
    }

    if (nbytes_plus < nbytes + extra_bytes) {
	// (Re)allocate the wrap buffer if it is not big enough.
	nbytes_plus = nbytes + extra_bytes;
	wrapbuf = (unsigned short *)realloc(wrapbuf, nbytes_plus);
	if (NULL == wrapbuf) {
	    set_error_string("sipcom_highrate_write: bad realloc (%s).\n",
	    	strerror(errno));
	    return -1;
	}
    }

    wrapbytes = telemwrap((unsigned short *)buf, wrapbuf, nbytes,
                                                    SIPCOM_OMNI, TW_SIP);

    if ((retval = highrate_write_bytes((unsigned char *)wrapbuf, wrapbytes))) {
	return retval;
    }

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

    if (Sipmux_enable & (1 << HIGHRATE)) {
        if (serial_sipmux_begin(Fd[HIGHRATE], 500000)) {
            // serial_sipmux_begin() should have set the error string.
            return -5;
        }
    }

    while (1) {
	bytes_avail = sipthrottle(bytes_to_write);

	if (bytes_avail == 0) {
	    // No room yet.
	    continue;

	} else if (bytes_avail > 0) {
try_write:
	    // write bytes_avail bytes
	    if (write(Fd[HIGHRATE], p + writeloc, bytes_avail) == -1) {
		if (errno == EAGAIN) {
		    goto try_write;
		}

		// other write error
		set_error_string("highrate_write_bytes: write error\n");
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
	    set_error_string("highrate_write_bytes: bytes_avail < 0\n");
	    break;
	}
    }

    if (Sipmux_enable & (1 << HIGHRATE)) {
        if (serial_sipmux_end(Fd[HIGHRATE], 500000L)) {
            // serial_sipmux_end() should have set the error string.
            return -5;
        }
    }

    //fprintf(stderr, "             sipcom: retval is %d\n", retval);
    return retval;
}

int
sipcom_very_highrate_write(unsigned char *buf, unsigned short nbytes)
{
    static const int extra_bytes = 128;
    static int nbytes_plus = 0;
    int retval = 0;
    static unsigned short *wrapbuf = NULL;
    int wrapbytes = 0;

    if (nbytes == 0) {
	return 0;
    }

    if (nbytes_plus < nbytes + extra_bytes) {
	// (Re)allocate the wrap buffer if it is not big enough.
	nbytes_plus = nbytes + extra_bytes;
	wrapbuf = (unsigned short *)realloc(wrapbuf, nbytes_plus);
	if (NULL == wrapbuf) {
	    set_error_string("sipcom_very_highrate_write: bad realloc (%s).\n",
	    	strerror(errno));
	    return -1;
	}
    }

    wrapbytes = telemwrap((unsigned short *)buf, wrapbuf, nbytes,
                                                        SIPCOM_HGA, TW_SIP);

    if ((retval = very_highrate_write_bytes((unsigned char *)wrapbuf,
								wrapbytes))) {
	return retval;
    }

    return 0;
}

static int
very_highrate_write_bytes(unsigned char *p, int bytes_to_write)
{
    if (Sipmux_enable & (1 << VERY_HIGHRATE)) {
        if (serial_sipmux_begin(Fd[VERY_HIGHRATE], 500000L)) {
            // serial_sipmux_begin() should have set the error string.
            return -5;
        }
    }

    if (write(Fd[VERY_HIGHRATE], p, bytes_to_write) == -1) {
	set_error_string("very_highrate_write_bytes: write error\n");
	return -1;
    }

#ifdef USE_STOR
    {
	FILE *pipe_ = Very_highrate_stor_pipe;
	int ret;
	int len;
	if (pipe_ != NULL) {
	    len = bytes_to_write;
	    ret = fwrite(p, 1, len, pipe_);
	    if (ret != len) {
		fprintf(stderr, "bad write to Very_highrate_stor_pipe\n");
	    }
	    fflush(pipe_);
	}
    }
#endif

    if (Sipmux_enable & (1 << VERY_HIGHRATE)) {
        if (serial_sipmux_end(Fd[VERY_HIGHRATE], 500000L)) {
            // serial_sipmux_end() should have set the error string.
            return -5;
        }
    }

    return 0;
}
