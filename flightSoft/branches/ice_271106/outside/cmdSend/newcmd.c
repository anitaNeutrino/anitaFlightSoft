/* cmd - interface to NSBF for sending LDB commands
 *
 * USAGE: cmd [-d] [-l link] [-r route] [-g logfile]
 *
 * Marty Olevitch, June '01, editted KJP 7/05
 */

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

#include "newcmdfunc.h"	// generated from newcmdlist.h

WINDOW *Wuser;
WINDOW *Wmenu;

void catchsig(int sig);
static void quit_confirm(void);
int screen_confirm(char *fmt, ...);
void screen_dialog(char *response, int nbytes, char *fmt, ...);
void screen_mesg(char *fmt, ...);
void screen_printf(char *fmt, ...);
void screen_beep(void);

#define GETKEY wgetch(Wuser)

#define TIMEOUT	't'	/* set_timeout */
#define NEWCMD	'n'	/* new_cmd */
#define CURCMD	'c'	/* cur_cmd */
#define LINKSEL 'l'	/* select_link */
#define RTSEL	'r'	/* select_route */
#define SHOWCMD	's'	/* show_cmds */
#define QUIT	'Q'

static void set_timeout(void);
static void new_cmd(void);
static void cur_cmd(void);
static void show_cmds(void);

#define NMENULINES 2
char *progmenu[NMENULINES-1];
#define LINELEN	80
#define CTL(x)	(x & 0x0f)	/* control(x) */

static void initdisp(void);
static void menu(char **m, int nlines);
static void clear_screen(void);
static void generic_exit_routine(void);

char *menuformat[] = {
"%c=set_timeout  %c=new_cmd  %c=cur_cmd  %c=quit   %c=link   %c=route  %c=show_cmds",
};

#define CMDLEN		25	/* max length of command buffer */
#define LINK_LOS	0
#define LINK_TDRSS	1
#define LINK_HF		2
#define LINK_LOS	0
#define PORT		"/dev/ttyUSB0"
//#define PORT		"/dev/ttyS2"
#define PROMPT		": "
#define ROUTE_COMM1	0x09
#define ROUTE_COMM2	0x0C

static void accum_cmd(int key);
static void select_link(void);
static void select_route(void);
void sendcmd(int fd, unsigned char *s, int len);
int serial_end(void);
int serial_init(void);
void wait_for_ack(void);

unsigned char Curcmd[32];
int Curcmdlen = 0;
unsigned char Curlink = LINK_LOS;
unsigned char Curroute = ROUTE_COMM1;
int Fd;
struct termios Origopts;
long Timeout = 10L;

static short Dir_det =0;
static short Prog_det =0;
static short Config_det =0;
static short Priorit_det =0;
static short CalAdd =0;
static short SSGain =0;

static short Amp_led = 0;
static short Amplitude = 0;
static short Bak_led = 0;
static short Bak_led_onoff = 0;
static unsigned short C1_low_gain_thresh = 100;
static short Caltype = 2;	/* ped cal is type 2 */
static short Coin_det = 0;
static int Direct = 0;		/* nonzero for direct connection to flight */
static short Disc_level = 0;
static short Edac_val = 0;
static short Heater = 1;
static short Hsktype = 7;	/* scalars hsk is type 7 */
static short Hvps_det = 0;
static short Hvps_pair = 1;
static short Hvps_prim = 1;
static short Led = 0;
static short Led_width = 0;
static unsigned short Mark = 0;
static unsigned char Ncal_count = 50;
static unsigned char Ncal_delayamt = 0;
static unsigned char Ncal_type = 0;
static short Nlightcal = 20;
static short Npedcal = 20;
static short Pha_det = 0;
static unsigned short Pri_cutoff = 1800;
static short Scaler_interval_val = 0;
static short Selchan = 255;
static short Sip_rate = 4500;
static short Thresh_val = 0;
static short Voltage = 0;

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

int
main(int argc, char *argv[])
{
    int key = 0;
    char logname[LOGSTRSIZE] = "cmdlog";
    
    // for getopt
    int c;
    extern char *optarg;

    while ((c = getopt(argc, argv, "dl:r:g:")) != EOF) {
	switch (c) {
	    case 'd':
		// Using direct connect to flight system.
		Direct = 1;
		break;
	    case 'l':
		Curlink = atoi(optarg);
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
	    default:
	    case '?':
		fprintf(stderr, "USAGE: cmd [-l link] [-r route] [-g logfile]\n");
		exit(1);
		break;
	}
    }

    signal(SIGINT, catchsig);
    if (serial_init()) {
	exit(1);
    }
    sprintf(Curcmd, "Default comd");
    Curcmdlen = strlen(Curcmd);
    initdisp();

    if (log_init(logname, "program starting.")) {
	screen_printf(
	    "WARNING! Can't open log file '%s' (%s). NO COMMAND LOGGING!\n",
	    "cmdlog", strerror(errno));
    }
    log_out("link is %d, route is %x.", Curlink, Curroute);

    while(1) {
	wprintw(Wuser, PROMPT);
	wrefresh(Wuser);
	key = GETKEY;
	if (isdigit(key)) {
	    /* get the numerical value of the cmd to send to flight */
	    accum_cmd(key);
	    continue;
	}
	wprintw(Wuser, "%c\n", key);
	wrefresh(Wuser);
	switch(key) {
	case TIMEOUT:
	    set_timeout();
	    break;
	case NEWCMD:
	    new_cmd();
	    break;
	case CURCMD:
	    cur_cmd();
	    break;
	case LINKSEL:
	    select_link();
	    break;
	case RTSEL:
	    select_route();
	    break;
	case SHOWCMD:
	    show_cmds();
	    break;
	case CTL('L'):
	    clear_screen();
	    break;
	case CTL('C'):
	case QUIT:
	    quit_confirm();
	    break;
	default:
	    if(key != '\r' && key != '\n')
		wprintw(Wuser, "%c? What is that?\n", key);
		wrefresh(Wuser);
	    break;
	}
    }
    return 0;
}

static int Endmenu;	/* last line of menu */
static int Startuser;	/* first line of user area */

static void
initdisp()
{
    int i;

    (void)initscr();
    (void)cbreak();
    noecho();
    Endmenu = NMENULINES - 1;
    Wmenu = newwin(NMENULINES, 80, 0, 0);
    if (Wmenu == NULL) {
	fprintf(stderr,"Bad newwin [menu]\n");
	exit(1);
    }

    Startuser = Endmenu + 1;
    Wuser = newwin(LINES-NMENULINES, 80, Startuser, 0);
    if (Wuser == NULL) {
	fprintf(stderr,"Bad newwin [user]\n");
	exit(1);
    }
    scrollok(Wuser, 1);
    for(i=0; i<NMENULINES-1; i++) {
	if((progmenu[i] = malloc(strlen(menuformat[i] + 1))) == NULL) {
	    wprintw(Wuser, "Not enough memory\n");
	    exit(1);
	}
    }
    sprintf(progmenu[0], menuformat[0] , TIMEOUT, NEWCMD, CURCMD, QUIT,
	LINKSEL, RTSEL, SHOWCMD);

    menu(progmenu, NMENULINES);
}

static void
menu(char **m, int nlines)
{
    int i;
    wclear(Wmenu);
    for (i=0; i<nlines-1; i++) {
	wmove(Wmenu, i, 0);
	wprintw(Wmenu, "%s", progmenu[i]);
    }
    wmove(Wmenu, Endmenu, 0);
    whline(Wmenu, ACS_HLINE, 80);
    wclear(Wuser);
    wnoutrefresh(Wmenu);
    wnoutrefresh(Wuser);
    doupdate();
}

static void
set_timeout()
{
    char resp[32];
    screen_dialog(resp, 31, "New timeout value [%ld] ", Timeout);
    if (resp[0] == '\0') {
	screen_printf("value unchanged\n");
    } else {
	Timeout = atol(resp);
	screen_printf("new timeout is %ld seconds\n", Timeout);
	log_out("timeout set to %ld seconds", Timeout);
    }
}

static void
select_link(void)
{
    char resp[32];
    screen_dialog(resp, 31,
    	"Choose link 0=LOS, 1=TDRSS (COMM1), 2=HF (COMM2) [%d] ", Curlink);
    if (resp[0] == '\0') {
	screen_printf("value unchanged\n");
    } else {
	if (resp[0] < '0' || resp[0] > '2') {
	    screen_printf("Sorry, link must be 0, 1, or 2, not %c\n", resp[0]);
	    return;
	}
	Curlink = resp[0] - '0';
	screen_printf("New link is %d\n", Curlink);
	log_out("link set to '%d'", Curlink);
    }
}

static void
select_route(void)
{
    char resp[32];
    screen_dialog(resp, 31, "Choose route 9=COMM1, C=COMM2 [%x] ", Curroute);
    if (resp[0] == '\0') {
	screen_printf("value unchanged\n");
    } else {
	if (resp[0] == '9') {
	    Curroute = ROUTE_COMM1;
	} else if (resp[0] == 'C' || resp[0] == 'c') {
	    Curroute = ROUTE_COMM2;
	} else {
	    screen_printf("Sorry, route must be 9 or C, not %c\n", resp[0]);
	    return;
	}
	screen_printf("New route is %x\n", Curroute);
	log_out("route set to '%x'", Curroute);
    }
}

static void
new_cmd(void)
{
    unsigned char cmd[10];
    int i;
    char msg[1024];
    char *mp = msg;
    int n = 0;
    char resp[32];
    int val;

    screen_printf("Enter the value of each command byte.\n");
    screen_printf("Enter '.' (period) to finish.\n");
    memset(msg, '\0', 1024);
    while (1) {
	screen_dialog(resp, 32, "Command byte %d ", n);
	if (resp[0] == '.') {
	    break;
	}
	val = atoi(resp);
	if (n == 0 && (val < 128 || val > 255)) {
	    screen_printf("Sorry, val must be between 129 and 255, not %d\n",
		val);
	    screen_printf("Try again.\n");
	} else {
	    cmd[n] = (unsigned char)(val & 0x000000ff);
	    sprintf(mp, "%d ", cmd[n]);
	    mp += strlen(mp);
	    n++;
	    if (n >= 10) {
		break;
	    }
	}
    }

    set_cmd_log("new_cmd: %s", msg);

    for (i=0; i < n; i++) {
	Curcmd[i*2] = i;
	Curcmd[(i*2)+1] = cmd[i];
    }
    Curcmdlen = n*2;
    sendcmd(Fd, Curcmd, Curcmdlen);
}

#ifdef NOTDEF
static void
new_cmd(void)
{
    char resp[32];
    screen_dialog(resp, 31, "New command string? ");
    strncpy(Curcmd, resp, CMDLEN-5);
    sendcmd(Fd, Curcmd, strlen(Curcmd));
}
#endif

static void
cur_cmd()
{
    int i;
    screen_printf("Sending: ");
    for (i=0; i<Curcmdlen; i++) {
	if (i % 2) {
	    screen_printf("%d ", Curcmd[i]);
	}
    }
    screen_printf("\n");
    sendcmd(Fd, Curcmd, Curcmdlen);
}



static void
clear_screen(void)
{
    menu(progmenu, NMENULINES);
}

static void
quit_confirm(void)
{
    if (screen_confirm("Really quit")) {
	screen_printf("Bye bye...");
	endwin();
	generic_exit_routine();
    } else {
	screen_printf("\nNot quitting\n");
    }
}

static void
generic_exit_routine()
{
    log_close();
    if (serial_end()) {
	exit(1);
    }
    exit(0);
}

void
catchsig(int sig)
{
    signal(sig, catchsig);
    quit_confirm();
    wprintw(Wuser, PROMPT);
    wrefresh(Wuser);
}

int
screen_confirm(char *fmt, ...)
{
    char s[512];
    va_list ap;
    int key;
    if (fmt != NULL) {
	va_start(ap, fmt);
	vsprintf(s, fmt, ap);
	va_end(ap);
    	wprintw(Wuser, "%s [yn] ", s);
	wrefresh(Wuser);
	/* nodelay(Wuser, FALSE); */
	key = wgetch(Wuser);
	/* nodelay(Wuser, TRUE); */
	if (key == 'y' || key == 'Y' || key == '\n') {
	    return 1;
	} else {
	    return 0;
	}
    }
    return 0;
}

void
screen_dialog(char *response, int nbytes, char *fmt, ...)
{
    char prompt[512];
    va_list ap;
    if (fmt != NULL) {
	va_start(ap, fmt);
	vsprintf(prompt, fmt, ap);
	va_end(ap);
    } else {
	strcpy(prompt, "Well? ");
    }
    wprintw(Wuser, "%s ", prompt);
    wrefresh(Wuser);
    echo();
    /* nodelay(Wuser, FALSE); */
    wgetnstr(Wuser, response, nbytes);
    noecho();
    /* nodelay(Wuser, TRUE); */
}

void
screen_mesg(char *fmt, ...)
{
    char s[512];
    va_list ap;
    if (fmt != NULL) {
	va_start(ap, fmt);
	vsprintf(s, fmt, ap);
	va_end(ap);
	wprintw(Wuser, "%s ", s);
    }
    wprintw(Wuser, "[press any key to continue] ");
    wrefresh(Wuser);
    /* nodelay(Wuser, FALSE); */
    (void)wgetch(Wuser);
    /* nodelay(Wuser, TRUE); */
}

void
screen_printf(char *fmt, ...)
{
    char s[512];
    va_list ap;
    if (fmt != NULL) {
	va_start(ap, fmt);
	vsprintf(s, fmt, ap);
	va_end(ap);
	wprintw(Wuser, "%s", s);
	wrefresh(Wuser);
    }
}

void
screen_beep(void)
{
    beep();
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
sendcmd(int fd, unsigned char *s, int len)
{
    int n;
    char *bp;
    unsigned char buf[CMDLEN];
    int i;

    if (len > CMDLEN - 5) {
	len = CMDLEN - 5;
	screen_printf("Warning: command too long; truncating.\n");
    }

    if (len % 2) {
	screen_printf("Warning: command length is odd; truncating.\n");
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
	    write (fd, buf, 6);
#ifdef NOTDEF
	    {
		int n;
		for (n=0; n<6; n++) {
		    screen_printf("0x%02x ", buf[n]);
		}
		screen_printf("\n");
	    }
#endif // NOTDEF
	}
	screen_printf("Direct to science computer.\n");
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
	write (fd, buf, n);
	wait_for_ack();
    }

    cmd_log();

#ifdef PRINT_EXTRA_STUFF
    screen_printf("sending ");
#ifdef NOTDEF
    for (i=0; i<4; i++) {
	screen_printf("0x%02x ", buf[i]);
    }
    for ( ; i < n-1; i++) {
	screen_printf("%c ", buf[i]);
    }
    for ( ; i < n; i++) {
	screen_printf("0x%02x ", buf[i]);
    }
#else
    for (i=0; i<n; i++) {
	screen_printf("0x%02x ", buf[i]);
    }
#endif /* NOTDEF */
    screen_printf("\n");
#endif /* PRINT_EXTRA_STUFF */

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

    screen_printf("Awaiting NSBF response: ");

    t.tv_sec = Timeout;
    t.tv_usec = 0L;
    FD_ZERO(&f);
    FD_SET(Fd, &f);
    ret = select(Fd+1, &f, NULL, NULL, &t);
    if (ret == -1) {
	screen_printf("select() error (%s)\n", strerror(errno));
	log_out("select() error (%s)", strerror(errno));
	return;
    } else if (ret == 0 || !FD_ISSET(Fd, &f)) {
	screen_printf("no response after %ld seconds\n", Timeout);
	log_out("NSBF response; no response after %ld seconds", Timeout);
	return;
    }
    sleep(1);
    n = read(Fd, ibuf, IBUFSIZE);
    if (n > 0) {
	if (ibuf[0] != 0xFA || ibuf[1] != 0xF3) {
	    screen_printf("malformed response!\n%x\t\n",ibuf[0],ibuf[1]);
	    log_out("NSBF response; malformed response!");
	    return;
	}

	if (ibuf[2] == 0x00) {
	    screen_printf("OK\n");
	    log_out("NSBF response; OK");
	} else if (ibuf[2] == 0x0A) {
	    screen_printf("GSE operator disabled science commands.\n");
	    log_out("NSBF response; GSE operator disabled science commands.");
	} else if (ibuf[2] == 0x0B) {
	    screen_printf("routing address does not match selected link.\n");
	    log_out( "NSBF response; routing address does not match link.");
	} else if (ibuf[2] == 0x0C) {
	    screen_printf("selected link not enabled.\n");
	    log_out("NSBF response; selected link not enabled.");
	} else if (ibuf[2] == 0x0D) {
	    screen_printf("miscellaneous error.\n");
	    log_out("NSBF response; miscellaneous error.");
	} else {
	    screen_printf("unknown error code (0x%02x)", ibuf[2]);
	    log_out("NSBF response; unknown error code (0x%02x)", ibuf[2]);
	}
    } else {
	screen_printf("strange...no response read\n");
	log_out("NSBF response; strange...no response read");
    }
}

static void
accum_cmd(int key)
{
    char digits[4];
    int idx;
    int n;

    screen_printf( "\nEnter digits for flight cmd. Press ESC to cancel.\n");
    digits[3] = '\0';

    n = 0;
    do {
	if (isdigit(key)) {
	    digits[n] = key;
	    screen_printf("%c", key);
	    n++;
	} else if (key == 0x1b) {
	    // escape key
	    screen_printf(" cancelled\n");
	    return;
	} else {
	    screen_beep();
	}
    } while (n < 3 && (key = GETKEY));

    idx = atoi(digits);
    if (idx > Csize - 1) {
	screen_printf(" No such command '%d'\n", idx);
	return;
    }
    
    //screen_printf("       got '%d'\n", idx);
    //screen_printf("       got '%s'\n", digits);
    
    if (Cmdarray[idx].f == NULL) {
	screen_printf(" No such command '%d'\n", idx);
	return;
    }
    screen_printf(" %s\n", Cmdarray[idx].name);
    Cmdarray[idx].f(idx);
}

static void
show_cmds(void)
{
    int got = 0;
    int i;
    int val[2];

    for (i=0; i<256; i++) {
	if (Cmdarray[i].f != NULL) {
	    val[got] = i;
	    got++;
	}
	if (got == 3) { 
	    screen_printf("%d %s", val[0], Cmdarray[val[0]].name);
	    screen_printf("%d %s", val[1], Cmdarray[val[1]].name);
	    screen_printf("%d %s\n", val[2], Cmdarray[val[2]].name);
	    got = 0;
	}
    }
    if (got) {
	for (i=0; i<got; i++) {
	    screen_printf("%d %s", val[i], Cmdarray[val[i]].name);
	}
	screen_printf("\n");
    }
}

static void
SHUTDOWN_HALT(int idx)
{
    if (screen_confirm("Really shutdown the computer")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Shutdown and halt computer.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
REBOOT(int idx)
{
    if (screen_confirm("Really reboot the computer")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Reboot.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
KILL_PROGS(int idx)
{
    short det;
    int i;
    char resp[32];
    
    screen_printf("0. Acqd      3. GPSd\n");
    screen_printf("1. Calibd    4. Hkd\n");
    screen_printf("2. Eventd    5. Prioritizerd\n");
    screen_printf("6. All of the above\n");
    screen_dialog(resp, 31, "Kill which daemon? (-1 to cancel) [%d] ",
	Prog_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 6) {
	    Prog_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled\n");
	    return;
	} else {
	    screen_printf("Value must be 0-6, not %d.\n", det);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Prog_det;
    Curcmdlen = 4;
    set_cmd_log("%d; Program  %d killed.", idx, Prog_det);
    sendcmd(Fd, Curcmd, Curcmdlen);
}


static void
RESPAWN(int idx)
{
    short det;
    int i;
    char resp[32];
    
    screen_printf("0. Acqd      3. GPSd\n");
    screen_printf("1. Calibd    4. Hkd\n");
    screen_printf("2. Eventd    5. Prioritizerd\n");
    screen_printf("6. All of the above\n");
    screen_dialog(resp, 31, "Respawn which daemon? (-1 to cancel) [%d] ",
	Coin_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 6) {
	    Prog_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled\n");
	    return;
	} else {
	    screen_printf("Value must be 0-6, not %d.\n", det);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Prog_det;
    Curcmdlen = 4;
    set_cmd_log("%d; Program  %d respawned.", idx, Prog_det);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
TURN_GPS_ON(int idx)
{
    if (screen_confirm("Really turn on GPS/Magnetometer")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn on GPS/Magnetometer.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TURN_GPS_OFF(int idx)
{
    if (screen_confirm("Really turn off GPS/Magnetometer")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn off GPS/Magnetometer.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TURN_RFCM_ON(int idx)
{
    if (screen_confirm("Really turn on RFCM")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn on RFCM.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TURN_RFCM_OFF(int idx)
{
    if (screen_confirm("Really turn off RFCM")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn off RFCM.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TURN_CALPULSER_ON(int idx)
{
    if (screen_confirm("Really turn on CalPulser")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn on CalPulser.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TURN_CALPULSER_OFF(int idx)
{
    if (screen_confirm("Really turn off CalPulser")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn off CalPulser.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
TURN_ND_ON(int idx)
{
    if (screen_confirm("Really turn on Noise Diode")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn on Noise Diode.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TURN_ND_OFF(int idx)
{
    if (screen_confirm("Really turn off Noise Diode")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn off Noise Diode.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TURN_ALL_ON(int idx)
{
    if (screen_confirm("Really turn on GPS, RFCM, CalPulsr and ND")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn on GPS, RFCM, CalPulser, and ND.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TURN_ALL_OFF(int idx)
{
    if (screen_confirm("Really turn off GPS, RFCM, CalPulser and ND")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Turn off GPS, RFCM, CalPulser and ND.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TRIG_CALPULSER(int idx)
{
    if (screen_confirm("Really trigger the CalPulser")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Trigger the CalPulser.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
SET_CALPULSER(int idx)
{
    char resp[32];
    short det;
    short v;
     
    screen_dialog(resp, 31,
	"Set CalPulser to  (0-15, -1 to cancel) [%d] ",
	CalAdd);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 15) {
	    CalAdd = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Value must be 0-15, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = CalAdd;
    Curcmdlen = 4;
    set_cmd_log("%d; Set CalPulser to address? %d.", idx, CalAdd);
    sendcmd(Fd, Curcmd, Curcmdlen);
}



static void
SET_SS_GAIN(int idx)
{
    char resp[32];
    short det;
    short v;
     
    screen_dialog(resp, 31,
	"Set Sun Sensor gain to  (0-7, -1 to cancel) [%d] ",
	SSGain);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 7) {
	    SSGain = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Gain must be 0-7, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = SSGain;
    Curcmdlen = 4;
    set_cmd_log("%d; Set Sun Sensor Gain to %d.", idx, SSGain);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
GPS(int idx)
{
    if (screen_confirm("Really do something to the GPS")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Do something to the GPS.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
SIP_GPS(int idx)
{
    if (screen_confirm("Really talk to SIP GPS")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Talk to SIP GPS.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
SUN_SENSORS(int idx)
{
    if (screen_confirm("Really do something with the Sun Sensors")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Do something with the Sun Sensors.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
MAGNETOMETER(int idx)
{
    if (screen_confirm("Really do something for Magnetometer")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Do something for Magnetometer.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
ACCEL(int idx)
{
    if (screen_confirm("Really do something for accelerometer")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Do something for accelerometer.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
CLEAN_DIRS(int idx)
{
    short det;
    int i;
    char resp[32];
    
    screen_printf("0. Dir 0    3. Dir 3\n");
    screen_printf("1. Dir 1    4. Dir 4\n");
    screen_printf("2. Dir 2    5. Dir 5\n");
    screen_printf("6. All of the above directories\n");
    screen_dialog(resp, 31, "Clean which directory? (-1 to cancel) [%d] ",
	Dir_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 6) {
	    Dir_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled\n");
	    return;
	} else {
	    screen_printf("Value must be 0-6, not %d.\n", det);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Dir_det;
    Curcmdlen = 4;
    set_cmd_log("%d; Directory  %d cleaned.", idx, Dir_det);
    sendcmd(Fd, Curcmd, Curcmdlen);
}


static void
SEND_CONFIG(int idx)
{
    short det;
    int i;
    char resp[32];
    
    screen_printf("0. anitaSoft.config     1.Hkd.config\n");
    screen_printf("2. All of the above\n");
    screen_dialog(resp, 31, "Send which config file? (-1 to cancel) [%d] ",
	Config_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 2) {
	    Config_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled\n");
	    return;
	} else {
	    screen_printf("Value must be 0-2, not %d.\n", det);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Config_det;
    Curcmdlen = 4;
    set_cmd_log("%d; Configuration file  %d sent.", idx, Config_det);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
DEFAULT_CONFIG(int idx)
{
    if (screen_confirm("Really return to defaut configuration")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Return to default configuration.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
CHNG_PRIORIT(int idx)
{
    short det;
    int i;
    char resp[32];
    
    screen_printf("0. priority 0    1.priority 1\n");
    screen_dialog(resp, 31, "Set what priority? (-1 to cancel) [%d] ",
	Priorit_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 1) {
	    Priorit_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled\n");
	    return;
	} else {
	    screen_printf("Value must be 0-1, not %d.\n", det);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Priorit_det;
    Curcmdlen = 4;
    set_cmd_log("%d; Priority set to  %d .", idx, Priorit_det);
    sendcmd(Fd, Curcmd, Curcmdlen);
}


static void
SURF(int idx)
{
    if (screen_confirm("Really do something for SURF")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Do something for SURF.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
TURF(int idx)
{
    if (screen_confirm("Really do something for TURF")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Do something for TURF.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


static void
RFCM(int idx)
{
    if (screen_confirm("Really do something for RFCM")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Do something for RFCM.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}


/*static void
DISABLE_DATA_COLL(int idx)
{
    if (screen_confirm("Really disable data collection")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Disable data collection.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
HV_PWR_OFF(int idx)
{
    if (screen_confirm("Really power off hvps relay")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Power off hvps relay.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
KNOWN_STATE(int idx)
{
    if (screen_confirm("Really go into known (default) state?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Known state.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
HEATER_PWR_OFF(int idx)
{
    char resp[32];
    short htr;
    screen_dialog(resp, 31,
	"Which heater to turn OFF? (1, 2, 3 or -1 to cancel) [%d] ", Heater);
    if (resp[0] != '\0') {
	htr = atoi(resp);
	if (1 <= htr && htr <= 3) {
	    Heater = htr;
	} else if (htr == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("heater must be 1, 2, or 3, not %d.\n", htr);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Heater;
    Curcmdlen = 4;
    set_cmd_log("%d; Heater %d power off.", idx, Heater);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
PHA_PWR_OFF(int idx)
{
    if (screen_confirm("Really power off pha racks relay")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Pha power off.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
COIN_ENABLE(int idx)
{
    short det;
    int i;
    char resp[32];
    
    screen_printf("0. S1      6. C1C\n");
    screen_printf("1. S2      7. S1 OR S2\n");
    screen_printf("2. S3      8. S3 OR S4\n");
    screen_printf("3. S4      9. C1A OR C1B or C1C\n");
    screen_printf("4. C1A     10. All of the above\n");
    screen_printf("5. C1B\n");
    screen_dialog(resp, 31, "Which item to enable? (-1 to cancel) [%d] ",
	Coin_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 10) {
	    Coin_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled\n");
	    return;
	} else {
	    screen_printf("Value must be 0-10, not %d.\n", det);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Coin_det;
    Curcmdlen = 4;
    set_cmd_log("%d; Coin %d enable.", idx, Coin_det);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
HV_LEVEL(int idx)
{
    char resp[32];
    short det;
    short v;
    screen_printf("0. S2 hvps A                         18. C1 hvps A\n");
    screen_printf("1. S2 hvps B                         19. C1 hvps B\n");
    screen_printf("2. S3 hvps                           20. HTX hvps 1\n");
    screen_printf("3. S1 hvps A                         21. HTX hvps 2\n");
    screen_printf("4. EDAC 51        13. EDAC 40        22. HTY hvps 1\n");
    screen_printf("5. HBY hvps 1     14. S4 hvps        23. HTY hvps 2\n");
    screen_printf("6. HBY hvps 2     15. S1 hvps B      24. EDAC 48\n");
    screen_printf("7. HBX hvps 1     16. C0 hvps A      25. EDAC 49\n");
    screen_printf("8. HBX hvps 2     17. C0 hvps B      26. EDAC 50\n");
    screen_dialog(resp, 31, "Which hvps? (0-26, -1 to cancel) [%d] ", Hvps_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 26) {
	    Hvps_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Hvps must be 0-26, not %d.\n", det);
	    return;
	}
    }

    screen_dialog(resp, 31,
	    "Desired voltage? (<1200 for S&H, <1600 for C, -1 to cancel) [%d] ",
	    Voltage);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 1600) {
	    Voltage = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Voltage must be < 1600, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Hvps_det;
    Curcmd[4] = 2;
    Curcmd[5] = (Voltage & 0x00ff);
    Curcmd[6] = 3;
    Curcmd[7] = (Voltage & 0xff00) >> 8;
    Curcmdlen = 8;
    set_cmd_log("%d; HV %d level %d volts.", idx, Hvps_det, Voltage);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
SET_ONE_THRESH(int idx)
{
    char resp[32];
    short det;
    short v;
    screen_printf("0. S2            9. HBXF         18. C1 board 5\n");
    screen_printf("1. S4           10. HBXF/HBYC    19. C1 board 6\n");
    screen_printf("2. C0 board 1   11. HBYC         20. HTXC\n");
    screen_printf("3. C0 board 2   12. HBYC/HBYF    21. HTXC/HTXF\n");
    screen_printf("4. C1 board 1   13. HBYF         22. HTXF\n");
    screen_printf("5. C1 board 2   14. S1           23. HTXF/HTYC\n");
    screen_printf("6. C1 board 3   15. S3           24. HTYC\n");
    screen_printf("7. HBXC         16. C0 board 3   25. HTYC/HTYF\n");
    screen_printf("8. HBXC/HBXF    17. C1 board 4   26. HTYF\n");
    screen_dialog(resp, 31,
	"Which detector? (0-26, -1 to cancel) [%d] ", Pha_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 26) {
	    Pha_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Detector must be 0-26, not %d.\n", det);
	    return;
	}
    }

    screen_dialog(resp, 31,
	"Desired discrim level? (0-4095, -1 to cancel) [%d] ",
	Disc_level);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 4095) {
	    Disc_level = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Discrim level must be 0-4095, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Pha_det;
    Curcmd[4] = 2;
    Curcmd[5] = (Disc_level & 0x00ff);
    Curcmd[6] = 3;
    Curcmd[7] = (Disc_level & 0xff00) >> 8;
    Curcmdlen = 8;
    set_cmd_log("%d; Set thresh %d to %d.", idx, Pha_det, Disc_level);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
SEL_SCALER(int idx)
{
    char resp[32];
    short det;
    short v;
    screen_printf("0. S2            9. HBXF         18. C1 board 5\n");
    screen_printf("1. S4           10. HBXF/HBYC    19. C1 board 6\n");
    screen_printf("2. C0 board 1   11. HBYC         20. HTXC\n");
    screen_printf("3. C0 board 2   12. HBYC/HBYF    21. HTXC/HTXF\n");
    screen_printf("4. C1 board 1   13. HBYF         22. HTXF\n");
    screen_printf("5. C1 board 2   14. S1           23. HTXF/HTYC\n");
    screen_printf("6. C1 board 3   15. S3           24. HTYC\n");
    screen_printf("7. HBXC         16. C0 board 3   25. HTYC/HTYF\n");
    screen_printf("8. HBXC/HBXF    17. C1 board 4   26. HTYF\n");
    screen_dialog(resp, 31,
	"Which detector? (0-26, -1 to cancel) [%d] ", Pha_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 25) {
	    Pha_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Detector must be 0-25, not %d.\n", det);
	    return;
	}
    }

    screen_dialog(resp, 31,
	"Selectable scaler channel? (0-7, 255 to unfreeze, -1 to cancel) [%d] ",
	Selchan);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (v == 255 || (0 <= v && v <= 7)) {
	    Selchan = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Sel. channel must be 0-7 or 255, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Pha_det;
    Curcmd[4] = 2;
    Curcmd[5] = Selchan;
    Curcmdlen = 6;
    set_cmd_log("%d; Set det %d to selectable scaler %d.", idx, Pha_det, Selchan);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
HV_PWR_ON(int idx)
{
    if (screen_confirm("Really power on hvps relay")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; hv power on.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
HEATER_PWR_ON(int idx)
{
    char resp[32];
    short htr;
    screen_dialog(resp, 31,
	"Which heater to turn ON? (1, 2, 3 or -1 to cancel) [%d] ", Heater);
    if (resp[0] != '\0') {
	htr = atoi(resp);
	if (1 <= htr && htr <= 3) {
	    Heater = htr;
	} else if (htr == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("heater must be 1, 2, or 3, not %d.\n", htr);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Heater;
    Curcmdlen = 4;
    set_cmd_log("%d; heater %d on.", idx, Heater);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
PHA_PWR_ON(int idx)
{
    if (screen_confirm("Really power on pha racks relay")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; pha power on.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
LIGHT_NUM_PULSES(int idx)
{
    char resp[32];
    short v;
    screen_dialog(resp, 31,
	"Number of light cals per cycle? (0-255, -1 to cancel) [%d] ",
	Nlightcal);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 255) {
	    Nlightcal = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("value must be 0 - 255, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Nlightcal & 0x00ff;
    Curcmdlen = 4;
    set_cmd_log("%d; light cals/cycle = %d.", idx, Nlightcal);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
HV_SELECT(int idx)
{
    char resp[32];
    short det;
    short v;
    screen_dialog(resp, 31,
	"HVPS A top (1), B bottom (0), -1 to cancel? [%d] ", Hvps_pair);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 1) {
	    Hvps_pair = det;
	} else if (det == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("HVPS pair must be 1 or 0, not %d.\n", det);
	    return;
	}
    }

    screen_dialog(resp, 31,
	"Primary (1), backup (0), -1 to cancel?  [%d] ", Hvps_prim);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 1) {
	    Hvps_prim = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("HVPS primary must be 1 or 0, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Hvps_pair;
    Curcmd[4] = 2;
    Curcmd[5] = Hvps_prim;
    Curcmdlen = 6;
    set_cmd_log("%d; hv select %s %s", idx,
	    (Hvps_pair ? "top" : "bottom"), (Hvps_prim ? "primary" : "backup")); 
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
PED_NUM_PULSES(int idx)
{
    char resp[32];
    short v;
    screen_dialog(resp, 31,
	"Number of ped cals per cycle? (0-255, -1 to cancel) [%d] ",
	Npedcal);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 255) {
	    Npedcal = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("value must be 0 - 255, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Npedcal & 0x00ff;
    Curcmdlen = 4;
    set_cmd_log("%d; ped cal number of pulses = %d.", idx, Npedcal);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
CAL_BD_PWR_OFF(int idx)
{
    if (screen_confirm("Really power off cal board relay")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Cal board power off.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
CAL_BD_PWR_ON(int idx)
{
    if (screen_confirm("Really power on cal board relay")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; Cal board power on.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
PHA_INIT(int idx)
{
    if (screen_confirm("Really turn on and initialize phas")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; pha init.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
HVPS_INIT(int idx)
{
    if (screen_confirm("Really turn on hvps relay and set dacs")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; hvps init.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
REBOOT(int idx)
{
    if (screen_confirm("Really reboot the flight computer?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	set_cmd_log("%d; reboot.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
ENABLE_DATA_COLL(int idx)
{
    if (screen_confirm("Really enable data collection")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	set_cmd_log("%d; enable data collection.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
DO_CAL(int idx)
{
    char resp[32];
    short type;
    screen_dialog(resp, 31,
	"What type of calibration? (2=ped, 3=light, -1 to cancel) [%d] ",
	Caltype);
    if (resp[0] != '\0') {
	type = atoi(resp);
	if (type == 2 || type == 3) {
	    Caltype = type;
	    screen_printf("Doing cal type %u\n", Caltype);
	} else if (type == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("cal type must be 2 or 3, not %d.\n", type);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Caltype;
    Curcmdlen = 4;
    set_cmd_log("%d; do calibration type %d.", idx, Caltype);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
DO_HSK(int idx)
{
    char resp[32];
    short type;
    screen_dialog(resp, 31,
	"What type hsk? (5=sensor, 6=misc, 7=scalar, 11=params, -1=cancel) [%d] ",
	Hsktype);
    if (resp[0] != '\0') {
	type = atoi(resp);
	if ((5 <= type && type <= 7) || type == 11) {
	    Hsktype = type;
	    screen_printf("Doing hsk type %d\n", Hsktype);
	} else if (type == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("hsk type must be 5, 6, 7, or 11, not %d.\n", type);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Hsktype;
    Curcmdlen = 4;
    set_cmd_log("%d; do hsk type %d.", idx, Hsktype);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
MISC_CMD(int idx)
{
    screen_printf("MISC_CMD not yet implemented\n");
}

static void
CLR_BUF(int idx)
{
    if (screen_confirm("Really clear all xmit buffers?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; clear all xmit buffers", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
XMIT_ON(int idx)
{
    short type = 0;
    int i;
    char resp[32];
    
    screen_dialog(resp, 31,
	"Start transmitting SIP (0), line-of-sight (1), -1 to cancel? [%d] ",
	type);
    if (resp[0] != '\0') {
	type = atoi(resp);
	if (type == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else if (type < 0 || type > 1) {
	    screen_printf("Value must be 0 or 1, not %d.\n", type);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = type;
    Curcmdlen = 4;
    set_cmd_log("%d; start transmitting via %s.",
	    idx, (type == 0) ? "SIP" : "line of sight");
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
XMIT_OFF(int idx)
{
    short type = 0;
    int i;
    char resp[32];
    
    screen_dialog(resp, 31,
	"Stop transmitting SIP (0), line-of-sight (1), -1 to cancel? [%d] ",
	type);
    if (resp[0] != '\0') {
	type = atoi(resp);
	if (type == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else if (type < 0 || type > 1) {
	    screen_printf("Value must be 0 or 1, not %d.\n", type);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = type;
    Curcmdlen = 4;
    set_cmd_log("%d; stop transmitting via %s.",
	    idx, (type == 0) ? "SIP" : "line of sight");
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
MARK(int idx)
{
    char resp[32];
    unsigned short v;
    screen_dialog(resp, 31,
	"Mark value [%u] ", Mark);
    if (resp[0] != '\0') {
	Mark = atoi(resp);
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = (Mark & 0x00ff);
    Curcmd[4] = 2;
    Curcmd[5] = (Mark & 0xff00) >> 8;
    Curcmdlen = 6;
    set_cmd_log("%d; mark %u.", idx, Mark);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
C1_LOW_GAIN_THRESHOLD(int idx)
{
    char resp[32];
    short v;
    screen_dialog(resp, 31,
	"C1 low gain threshold value (-1 to cancel) [%u] ", C1_low_gain_thresh);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	}
	C1_low_gain_thresh = (unsigned short)v;
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = (C1_low_gain_thresh & 0x00ff);
    Curcmd[4] = 2;
    Curcmd[5] = (C1_low_gain_thresh & 0xff00) >> 8;
    Curcmdlen = 6;
    set_cmd_log("%d; C1 low gain threshold = %u.", idx, C1_low_gain_thresh);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
SIP_RATE_LIMIT(int idx)
{
    char resp[32];
    short v;
    screen_dialog(resp, 31,
	"New sip rate limit? (bits/sec) (-1 to cancel) [%d] ", Sip_rate);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	}
	Sip_rate = v;
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = (Sip_rate & 0x00ff);
    Curcmd[4] = 2;
    Curcmd[5] = (Sip_rate & 0xff00) >> 8;
    Curcmdlen = 6;
    set_cmd_log("%d; sip rate limit = %d.", idx, Sip_rate);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
SIP_RATE_REPORT(int idx)
{
    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmdlen = 2;
    set_cmd_log("%d; sip rate report.", idx);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
SET_ONE_EDAC(int idx)
{
    char resp[32];
    short det;
    short v;
    screen_printf("0. S2 hvps A       9. LED ctrl 1     18. C1 hvps A\n");
    screen_printf("1. S2 hvps B      10. LED ctrl 2     19. C1 hvps B\n");
    screen_printf("2. S3 hvps        11. LED ctrl 3     20. HTX hvps 1\n");
    screen_printf("3. S1 hvps A      12. LED ctrl 4     21. HTX hvps 2\n");
    screen_printf("4. EDAC 51        13. EDAC 40        22. HTY hvps 1\n");
    screen_printf("5. HBY hvps 1     14. S4 hvps        23. HTY hvps 2\n");
    screen_printf("6. HBY hvps 2     15. S1 hvps B      24. EDAC 48\n");
    screen_printf("7. HBX hvps 1     16. C0 hvps A      25. EDAC 49\n");
    screen_printf("8. HBX hvps 2     17. C0 hvps B      26. EDAC 50\n");
    screen_dialog(resp, 31, "Which hvps? (0-26, -1 to cancel) [%d] ", Hvps_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 26) {
	    Hvps_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Hvps must be 0-26, not %d.\n", det);
	    return;
	}
    }

    screen_dialog(resp, 31,
	    "Desired dac value? (0-4095, -1 to cancel) [%d] ", Edac_val);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 4095) {
	    Edac_val = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Edac_val must be 0-4095, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Hvps_det;
    Curcmd[4] = 2;
    Curcmd[5] = (Edac_val & 0x00ff);
    Curcmd[6] = 3;
    Curcmd[7] = (Edac_val & 0xff00) >> 8;
    Curcmdlen = 8;
    set_cmd_log("%d; set external dac %d to %d.", idx, Hvps_det, Edac_val);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
PART_AMTLEFT(int idx)
{
    if (screen_confirm("Really get amount left on partition?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; get amount left on disk partition.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
PART_NEXT(int idx)
{
    if (screen_confirm("Really increment the partition for saving data?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; increment partition.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
PART_SET(int idx)
{
    screen_printf("PART_SET not yet implemented\n");
}

static void
STOR_STATS(int idx)
{
    if (screen_confirm("Really get storage statistics?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; get stor statistics.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
HV_LEVEL_LIMIT(int idx)
{
    screen_printf("HV_LEVEL_LIMIT not yet implemented\n");
}

static void
RELAY_INIT(int idx)
{
    if (screen_confirm("Really turn off all relays")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; relay init.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
COIN_DISABLE(int idx)
{
    short det;
    int i;
    char resp[32];
    
    screen_printf("0. S1      6. C1C\n");
    screen_printf("1. S2      7. S1 OR S2\n");
    screen_printf("2. S3      8. S3 OR S4\n");
    screen_printf("3. S4      9. C1A OR C1B or C1C\n");
    screen_printf("4. C1A     10. All of the above\n");
    screen_printf("5. C1B\n");
    screen_dialog(resp, 31, "Which item to disable? (-1 to cancel) [%d] ",
	Coin_det);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 10) {
	    Coin_det = det;
	} else if (det == -1) {
	    screen_printf("Cancelled\n");
	    return;
	} else {
	    screen_printf("Value must be 0-10, not %d.\n", det);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Coin_det;
    Curcmdlen = 4;
    set_cmd_log("%d; Coin %d disable.", idx, Coin_det);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
LED_WID(int idx)
{
    char resp[32];
    short det;
    short v;
    screen_printf("0. S1     5. C1\n");
    screen_printf("1. S2     6. HA1\n");
    screen_printf("2. S3     7. HA2\n");
    screen_printf("3. S4     8. HB1\n");
    screen_printf("4. C0     9. HB2\n");
    screen_dialog(resp, 31, "Which led? (0-9, -1 to cancel) [%d] ", Led);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 9) {
	    Led = det;
	} else if (det == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Led must be 0-9, not %d.\n", det);
	    return;
	}
    }

    screen_printf("0. 20 ns   1. 32 ns   2. 44 ns   3. 48 ns\n");
    screen_dialog(resp, 31, "Desired pulse width? (0-3, -1 to cancel) [%d] ",
	Led_width);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 3) {
	    Led_width = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Width must be 0-3, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Led;
    Curcmd[4] = 2;
    Curcmd[5] = Led_width;
    Curcmdlen = 6;
    set_cmd_log("%d; set led %d width %d.", idx, Led, Led_width);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
LED_AMP(int idx)
{
    char resp[32];
    short det;
    short v;
    screen_printf("0. S   1. C0   2. C1   3. H\n");
    screen_dialog(resp, 31, "Which led? (0-3, -1 to cancel) [%d] ", Amp_led);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 3) {
	    Amp_led = det;
	} else if (det == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("led must be 0-3, not %d.\n", det);
	    return;
	}
    }

    screen_dialog(resp, 31,
	"Desired amplitude? (0-4095, -1 to cancel) [%d] ", Amplitude);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 4095) {
	    Amplitude = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Amplitude must be 0-4095, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Amp_led;
    Curcmd[4] = 2;
    Curcmd[5] = (Amplitude & 0x00ff);
    Curcmd[6] = 3;
    Curcmd[7] = (Amplitude & 0xff00) >> 8;
    Curcmdlen = 8;
    set_cmd_log("%d; set led %d amplitude %d.", idx, Amp_led, Amplitude);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
LED_INIT(int idx)
{
    if (screen_confirm("Really set leds to defaults?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	set_cmd_log("%d; led init.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
	screen_printf("Don't forget to turn on the cal board, if desired.\n");
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
LED_BACKUP(int idx)
{
    char resp[32];
    short det;
    short v;
    screen_printf("0. S1     3. S4\n");
    screen_printf("1. S2     4. C0\n");
    screen_printf("2. S3     5. C1\n");
    screen_dialog(resp, 31, "Which led? (0-5, -1 to cancel) [%d] ", Bak_led);
    if (resp[0] != '\0') {
	det = atoi(resp);
	if (0 <= det && det <= 5) {
	    Bak_led = det;
	} else if (det == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Led must be 0-5, not %d.\n", det);
	    return;
	}
    }

    screen_dialog(resp, 31,
	"Turn backup led %d on or off? (0=off, 1=on, -1=cancel) [%d] ",
	Bak_led, Bak_led_onoff);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 1) {
	    Bak_led_onoff = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("On or off must be 0 or 1, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Bak_led;
    Curcmd[4] = 2;
    Curcmd[5] = Bak_led_onoff;
    Curcmdlen = 6;
    set_cmd_log("%d; Led %d backup %s.", idx, Bak_led,
	    Bak_led_onoff ? "on" : "off");
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
FLASH_LED(int idx)
{
    if (screen_confirm("Really flash leds")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; flash leds.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
TRIP_COIN(int idx)
{
    if (screen_confirm("Really trip coincidence")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; trip coincidence.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
PRIO_CUTOFF(int idx)
{
    char resp[32];
    short v;
    screen_dialog(resp, 31,
	"New priority cutoff? (0-65534, -1 to cancel) [%u] ", Pri_cutoff);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	}
	Pri_cutoff = (unsigned short)v;
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = (Pri_cutoff & 0x00ff);
    Curcmd[4] = 2;
    Curcmd[5] = (Pri_cutoff & 0xff00) >> 8;
    Curcmdlen = 6;
    set_cmd_log("%d; priority cutoff = %d.", idx, Pri_cutoff);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
PRIO_DET(int idx)
{
    short type = 3;
    int i;
    char resp[32];
    
    screen_printf("0=fake   1=S1   2=S2   3=S1+S2   4=none, -1=cancel\n");
    screen_dialog(resp, 31, "Select priority type? [%d] ", type);
    if (resp[0] != '\0') {
	type = atoi(resp);
	if (type == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else if (type < 0 || type > 4) {
	    screen_printf("Value must be 0-4, not %d.\n", type);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = type;
    Curcmdlen = 4;
    set_cmd_log("%d; priority type = %d.", idx, type);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
SET_ALL_EDACS(int idx)
{
    char resp[32];
    short v;
    screen_dialog(resp, 31,
	"Value to set all external dacs to? (0-4095, -1=cancel) [%d] ",
	Edac_val);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 4095) {
	    Edac_val = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Edac val must be 0-4095, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = (Edac_val & 0x00ff);
    Curcmd[4] = 2;
    Curcmd[5] = (Edac_val & 0xff00) >> 8;
    Curcmdlen = 6;
    set_cmd_log("%d; set all external dacs to %d.", idx, Edac_val);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
SET_ALL_THRESH(int idx)
{
    char resp[32];
    short v;
    screen_dialog(resp, 31,
	"Value to set all threshold dacs to? (0-4095, -1=cancel) [%d] ",
	Thresh_val);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= 4095) {
	    Thresh_val = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Thresh val must be 0-4095, not %d.\n", v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = (Thresh_val & 0x00ff);
    Curcmd[4] = 2;
    Curcmd[5] = (Thresh_val & 0xff00) >> 8;
    Curcmdlen = 6;
    set_cmd_log("%d; set all thresholds to %d.", idx, Thresh_val);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
START_STOR(int idx)
{
    if (screen_confirm("Really start storing data on disk?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; start stor.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
ALL_RELAYS_OFF(int idx)
{
    if (screen_confirm("Really turn all relays off?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; all relays off.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
CLEAR_SCALERS(int idx)
{
    if (screen_confirm("Really clear all scalers?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; clear scalers.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
LAST_STATE(int idx)
{
    if (screen_confirm("Really go into previous state?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; last state.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
NCAL(int idx)
{
    char resp[32];
    int type;
    int count;
    int delayamt;
    screen_printf("0=flash, 2=pedcal, 3=lightcal, -1 to cancel\n");
    screen_dialog(resp, 31, "What type?  [%d] ", Ncal_type);
    if (resp[0] != '\0') {
	type = atoi(resp);
	if (type == 0 || type == 2 || type == 3) {
	    Ncal_type = type;
	} else if (type == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Sorry, type must be 0, 2, or 3, not %d\n", type);
	    return;
	}
    }

    screen_dialog(resp, 31, "Count? (0-255) [%d] ", Ncal_count);
    if (resp[0] != '\0') {
	count = atoi(resp);
	if (0 <= count && count <= 255) {
	    Ncal_count = count;
	} else if (count == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Sorry, 0 <= count <= 255, not %d\n", count);
	    return;
	}
    }

    screen_dialog(resp, 31, "Delay in ms between each thing? (0-255) [%d] ",
	Ncal_delayamt);
    if (resp[0] != '\0') {
	delayamt = atoi(resp);
	if (0 <= delayamt && delayamt <= 255) {
	    Ncal_delayamt = delayamt;
	} else if (delayamt == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Sorry, 0 <= delayamt <= 255, not %d\n", delayamt);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Ncal_type;
    Curcmd[4] = 2;
    Curcmd[5] = Ncal_count;
    Curcmd[6] = 3;
    Curcmd[7] = Ncal_delayamt;
    Curcmdlen = 8;
    set_cmd_log("%d; NCAL type=%d count=%d delay=%d.",
	    idx, Ncal_type, Ncal_count, Ncal_delayamt);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
STOP_NCAL(int idx)
{
    if (screen_confirm("Really stop NCAL mode?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; stop ncal mode.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
STOP_STOR(int idx)
{
    if (screen_confirm("Really stop storing data on disk?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; stop stor.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
GET_PRIO_DET(int idx)
{
    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmdlen = 2;
    set_cmd_log("%d; get priority detector.", idx);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
SCALER_INTERVAL(int idx)
{
    char resp[32];
    short v;
    int max = 10000;
    screen_dialog(resp, 31,
	"Number of seconds between each scalar hsk? (0-%d, -1=cancel) [%d] ",
	    max, Scaler_interval_val);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (0 <= v && v <= max) {
	    Scaler_interval_val = v;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Scaler interval val must be 0-%d, not %d.\n",
		max, v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = (Scaler_interval_val & 0x00ff);
    Curcmd[4] = 2;
    Curcmd[5] = (Scaler_interval_val & 0xff00) >> 8;
    Curcmdlen = 6;
    set_cmd_log("%d; scaler interval is now %d.", idx, Thresh_val);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

#define NUM_ANITA_CMD_BYTES	4

static void
ANITA_CMD(int idx)
{
    int i;
    char resp[32];
    int val;

    Curcmd[0] = 0;
    Curcmd[1] = idx;

    for (i=0; i < NUM_ANITA_CMD_BYTES; i++) {
	screen_dialog(resp, 32, "Command byte %d (-1 to cancel) ", i+1);
	val = atoi(resp);
	if (val == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	}
	if (val < 0 || val > 255) {
	    screen_printf("Sorry, value must be between 0 and 255, not %d\n",
		val);
	    screen_printf("Try again.\n");
	    i--;
	} else {
	    Curcmd[(i*2) + 2] = i + 1;
	    Curcmd[(i*2) + 3] =  (unsigned char)(val & 0x000000ff);
	}
    }
    Curcmdlen = 10;
    set_cmd_log("%d; anita cmd %d %d %d %d.", idx, Curcmd[3], Curcmd[5],
	Curcmd[7], Curcmd[9]);

    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
ANITA_DATA_REQ(int idx)
{
    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmdlen = 2;
    set_cmd_log("%d; anita data request.", idx);
    sendcmd(Fd, Curcmd, Curcmdlen);
}

static void
STOP_ANITA_DATA_REQ(int idx)
{
    if (screen_confirm("Really stop doing automatic anita data requests?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; stop anita auto data requests.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
START_ANITA_DATA_REQ(int idx)
{
    if (screen_confirm("Really start doing automatic anita data requests?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmdlen = 2;
	screen_printf("\n");
	set_cmd_log("%d; start anita auto data requests.", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	screen_printf("\nCancelled\n");
    }
}

static void
TIGER_ANITA_PREFERENCE(int idx)
{
    if (screen_confirm("Give ANITA preference over TIGER low-pri data?")) {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmd[2] = 1;
	Curcmd[3] = 0;
	Curcmdlen = 4;
	screen_printf("\n");
	set_cmd_log("%d; Give ANITA priority over TIGER low-pri", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    } else {
	Curcmd[0] = 0;
	Curcmd[1] = idx;
	Curcmd[2] = 1;
	Curcmd[3] = 1;
	Curcmdlen = 4;
	screen_printf("\n");
	set_cmd_log("%d; Give TIGER low-pri priority over ANITA", idx);
	sendcmd(Fd, Curcmd, Curcmdlen);
    }
}

static void
SET_NUM_ANITA_BYTES(int idx)
{
    static unsigned char Anita_bytes_to_process = 1;
    char resp[32];
    short v;
    screen_dialog(resp, 31,
    	"No. ANITA bytes to process each time through main loop? [%d]",
	    Anita_bytes_to_process);
    if (resp[0] != '\0') {
	v = atoi(resp);
	if (1 <= v) {
	    Anita_bytes_to_process = v & 0x00ff;
	} else if (v == -1) {
	    screen_printf("Cancelled.\n");
	    return;
	} else {
	    screen_printf("Scaler interval val must be non-negative, not %d.\n",
		v);
	    return;
	}
    }

    Curcmd[0] = 0;
    Curcmd[1] = idx;
    Curcmd[2] = 1;
    Curcmd[3] = Anita_bytes_to_process;
    Curcmdlen = 4;
    set_cmd_log("%d; Anita_bytes_to_process set to %d.",
    	idx, Anita_bytes_to_process);
    sendcmd(Fd, Curcmd, Curcmdlen);
}
*/

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
	sprintf(Logstr, "CMD ");
	vsnprintf(Logstr+4, LOGSTRSIZE-4, fmt, ap);
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
