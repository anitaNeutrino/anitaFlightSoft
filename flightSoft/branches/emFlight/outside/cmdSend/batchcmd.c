/* batchcmd - interface to NSBF for sending LDB commands - reads command
 * from command line.
 *
 * Marty Olevitch, Oct '03
 */

char Usage_msg[] =
    "[-t][-d][-n nsbfwait][-l link][-r route][-g logfile] cmdbyte [cmdbyte ...]";

#include <stdio.h>
#include <errno.h>
#include <string.h>	/* strlen, strerror */
#include <stdlib.h>	/* malloc */
#include <curses.h>
#include <signal.h>

#include <fcntl.h>
#include <termios.h>

/* select() */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static void set_timeout(void);

#define CMDLEN		25	/* max length of command buffer */
#define LINK_LOS	0
#define LINK_TDRSS	1
#define LINK_HF		2
#define LINK_LOS	0
#define PORT		"/dev/ttyS0"
#define PROMPT		": "
#define ROUTE_COMM1	0x09
#define ROUTE_COMM2	0x0C

#define MAXCMDBYTES 10

static void select_link(void);
static void select_route(void);
void sendcmd(int fd, unsigned char *s, int len, char *msg);
int serial_end(void);
int serial_init(void);
void wait_for_ack(void);

unsigned char Curcmd[32];
int Curcmdlen = 0;
unsigned char Curlink = LINK_LOS;
unsigned char Curroute = ROUTE_COMM1;
static int Direct = 0;		/* nonzero for direct connection to flight */
int Fd;
struct termios Origopts;
char *Progname;
int Retcode = 0;
int Testing = 0;
long Timeout = 10L;

#define LOGSTRSIZE 2048
static char Logstr[LOGSTRSIZE];
static char Logfilename[LOGSTRSIZE];
static FILE *Logfp;

void clr_cmd_log(void);
void set_cmd_log(char *fmt, ...);
void cmd_log(void);
int log_init(char *filename, char *msg);
void log_out(char *fmt, ...);
void log_close(void);

void
usage()
{
    fprintf(stderr, "%s %s\n", Progname, Usage_msg);
}

int
main(int argc, char *argv[])
{
    char logname[LOGSTRSIZE] = "cmdlog";
    
    // for getopt
    int c;
    extern char *optarg;
    extern int optind;

    Progname = argv[0];

    while ((c = getopt(argc, argv, "dg:l:n:r:t")) != EOF) {
	switch (c) {
	    case 'd':
		// Using direct connect to flight system.
		Direct = 1;
		break;
	    case 'l':
		Curlink = atoi(optarg);
		break;
	    case 'n':
	    	Timeout = atoi(optarg);
		break;
	    case 'r':
		if (optarg[0] == '9') {
		    Curroute = ROUTE_COMM1;
		} else if (optarg[0] == 'C' || optarg[0] == 'c') {
		    Curroute = ROUTE_COMM2;
		}
		break;
	    case 'g':
		snprintf(logname, LOGSTRSIZE, optarg);
		break;
	    case 't':
	    	Testing = 1;
		break;
	    default:
	    case '?':
		usage();
		exit(1);
		break;
	}
    }

    if (serial_init()) {
	exit(1);
    }

    if (log_init(logname, "program starting.")) {
	printf(
	    "WARNING! Can't open log file '%s' (%s). NO COMMAND LOGGING!\n",
	    "cmdlog", strerror(errno));
    }
    log_out("link is %d, route is %x.", Curlink, Curroute);

    {
	unsigned char cmd[MAXCMDBYTES];
	int i;
	char msg[1024];
	char *msgend = msg;
	int n = 0;

	memset(msg, '\0', 1024);

	for (i=optind, n=0; i<argc; i++) {
	    int val = atoi(argv[i]);
	    if (val < 0 || val > 255) {
		fprintf(stderr,
		    "%s: cmdbytes must be between 0 and 255, not %d\n",
		    Progname, val);
		exit(1);
	    }

	    sprintf(msgend, "%s ", argv[i]);
	    msgend += (strlen(argv[i]) + 1);

	    cmd[n] = (unsigned char)(val & 0x000000ff);
	    n++;
	    if (n >= MAXCMDBYTES) {
		fprintf(stderr, "%s: too many cmd bytes. Max = %d\n",
		    Progname, MAXCMDBYTES);
		exit(1);
	    }
	}

	for (i=0; i < n; i++) {
	    Curcmd[i*2] = i;
	    Curcmd[(i*2)+1] = cmd[i];
	}
	Curcmdlen = n*2;
	sendcmd(Fd, Curcmd, Curcmdlen, msg);
    }

    return Retcode;
}

int
serial_init(void)
{
    struct termios newopts;
    Fd = open(PORT, O_RDWR | O_NDELAY);
    if (Fd == -1) {
	fprintf(stderr,"can't open '%s' (%s)\n",
	    PORT, strerror(errno));
	return -1;
    } else {
	//fcntl(Fd, F_SETFL, 0);	/* blocking reads */
	fcntl(Fd, F_SETFL, FNDELAY);	/* non-blocking reads */
    }

    tcgetattr(Fd, &Origopts);
    tcgetattr(Fd, &newopts);

    if (Direct) {
	// direct connection to flight system.
	cfsetispeed(&newopts, B1200);
	cfsetospeed(&newopts, B1200);
    } else {
	// connection via sip
	cfsetispeed(&newopts, B2400);
	cfsetospeed(&newopts, B2400);
    }
    
    newopts.c_cflag |= (CLOCAL | CREAD);   /* enable receiver, local mode */

    /* set 8N1 - 8 bits, no parity, 1 stop bit */
    newopts.c_cflag &= ~PARENB;
    newopts.c_cflag &= ~CSTOPB;
    newopts.c_cflag &= ~CSIZE;
    newopts.c_cflag |= CS8;

    newopts.c_iflag &= ~(INLCR | ICRNL);
    newopts.c_iflag &= ~(IXON | IXOFF | IXANY);	/* no XON/XOFF */

    newopts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	/* raw input */
    newopts.c_oflag &= ~OPOST;	/* raw output */

    tcsetattr(Fd, TCSANOW, &newopts);
    return 0;
}

int
serial_end(void)
{
    tcsetattr(Fd, TCSANOW, &Origopts);
    if (close(Fd)) {
	fprintf(stderr, "ERROR: bad close on %s (%s)\n",
	    PORT, strerror(errno));
	return -1;
    }
    return 0;
}

void
sendcmd(int fd, unsigned char *s, int len, char *msg)
{
    int n;
    char *bp;
    unsigned char buf[CMDLEN];
    int i;

    if (len > CMDLEN - 5) {
	len = CMDLEN - 5;
	printf("Warning: command too long; truncating.\n");
    }

    if (len % 2) {
	printf("Warning: command length is odd; truncating.\n");
	len--;
    }

    if (Direct) {
	// direct connection to flight system
	int i;
	for (i=0; i<len; i += 2) {
	    bp = buf;
#ifdef NOTDEF
	    *bp++ = 'p';
	    *bp++ = 'a';
	    *bp++ = 'u';
	    *bp++ = 'l';
	    *bp++ = ' ';
#endif
	    *bp++ = 0x10;
	    *bp++ = 0x14;
	    *bp++ = 2;
	    *bp++ = s[i];
	    *bp++ = s[i+1];
	    *bp++ = 0x03;
	    if (Testing) {
		int n;
		for (n=0; n<6; n++) {
		    printf("0x%02x ", buf[n]);
		}
		printf("\n");
	    } else {
		write (fd, buf, 6);
	    }
	}
	printf("Direct to science computer.\n");
    } else {
	// connection via sip
	bp = buf;
	*bp++ = 0x10;
	*bp++ = Curlink;
	*bp++ = Curroute;
	*bp++ = len;
	for (i=0; i < len; i++) {
	    *bp++ = *s++;
	}
	*bp = 0x03;
	n = len + 5;
	if (Testing) {
	    int i;
	    for (i=0; i<n; i++) {
		printf("%3d ", buf[i]);
	    }
	    printf("\n");
	} else {
	    write (fd, buf, n);
	    wait_for_ack();
	}

	set_cmd_log("%s", msg);
	cmd_log();
    }

}

void
wait_for_ack(void)
{
    fd_set f;
#define IBUFSIZE 32
    unsigned char ibuf[IBUFSIZE];
    int n;
    int ret;
    struct timeval t;

    printf("Awaiting NSBF response: ");

    t.tv_sec = Timeout;
    t.tv_usec = 0L;
    FD_ZERO(&f);
    FD_SET(Fd, &f);
    ret = select(Fd+1, &f, NULL, NULL, &t);
    if (ret == -1) {
	printf("select() error (%s)\n", strerror(errno));
	log_out("select() error (%s)", strerror(errno));
	Retcode = -2;
	return;
    } else if (ret == 0 || !FD_ISSET(Fd, &f)) {
	printf("no response after %ld seconds\n", Timeout);
	log_out("NSBF response; no response after %ld seconds", Timeout);
	Retcode = -3;
	return;
    }

    n = read(Fd, ibuf, IBUFSIZE);
    if (n > 0) {
	if (ibuf[0] != 0xFA || ibuf[1] != 0xF3) {
	    printf("malformed response!\n");
	    log_out("NSBF response; malformed response!");
	    Retcode = -4;
	    return;
	}

	if (ibuf[2] == 0x00) {
	    printf("OK\n");
	    log_out("NSBF response; OK");
	    Retcode = 0;
	} else if (ibuf[2] == 0x0A) {
	    printf("GSE operator disabled science commands.\n");
	    log_out("NSBF response; GSE operator disabled science commands.");
	    Retcode = -5;
	} else if (ibuf[2] == 0x0B) {
	    printf("routing address does not match selected link.\n");
	    log_out( "NSBF response; routing address does not match link.");
	    Retcode = -6;
	} else if (ibuf[2] == 0x0C) {
	    printf("selected link not enabled.\n");
	    log_out("NSBF response; selected link not enabled.");
	    Retcode = -7;
	} else if (ibuf[2] == 0x0D) {
	    printf("miscellaneous error.\n");
	    log_out("NSBF response; miscellaneous error.");
	    Retcode = -8;
	} else {
	    printf("unknown error code (0x%02x)", ibuf[2]);
	    log_out("NSBF response; unknown error code (0x%02x)", ibuf[2]);
	    Retcode = -9;
	}
    } else {
	printf("strange...no response read\n");
	log_out("NSBF response; strange...no response read");
	Retcode = -10;
    }
}

void
clr_cmd_log(void)
{
    Logstr[0] = '\0';
}

void
set_cmd_log(char *fmt, ...)
{
    va_list ap;
    if (fmt != NULL) {
	va_start(ap, fmt);
	sprintf(Logstr, "BATCHCMD ");
	vsnprintf(Logstr+9, LOGSTRSIZE-9, fmt, ap);
	va_end(ap);
    }
}

void
cmd_log(void)
{
    log_out(Logstr);
}

int
log_init(char *filename, char *msg)
{
    snprintf(Logfilename, LOGSTRSIZE, "%s", filename);
    Logfp = fopen(Logfilename, "a");
    if (Logfp == NULL) {
	return -1;
    }
    if (msg != NULL) {
	log_out(msg);
    } else {
	log_out("Starting up");
    }
    return 0;
}

void
log_out(char *fmt, ...)
{
    if (fmt != NULL) {
	va_list ap;
	char timestr[64];
	time_t t;

	/* output the date */
	t = time(NULL);
	sprintf(timestr, "%s", ctime(&t));
	timestr[strlen(timestr)-1] = '\0';	// delete \n
	fprintf(Logfp, "%s\t", timestr);

	/* output the message */
	va_start(ap, fmt);
	vfprintf(Logfp, fmt, ap);
	fprintf(Logfp, "\n");
	fflush(Logfp);
	va_end(ap);
    }
}

void
log_close(void)
{
    if (Logfp) {
	log_out("Finished");
	fclose(Logfp);
    }
}
