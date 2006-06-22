/* demo - 
 * generated by mkmenu (by Marty Olevitch) 
 */

#include "los.h"
#include "crc_simple.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>	/* strlen, strerror */
#include <stdlib.h>	/* malloc */
#include <curses.h>
#include <signal.h>

WINDOW *Wuser;
WINDOW *Wmenu;

void catchsig(int sig);
static void quit_confirm(void);
int screen_confirm(char *fmt, ...);
void screen_dialog(char *response, int nbytes, char *fmt, ...);
void screen_mesg(char *fmt, ...);
void screen_printf(char *fmt, ...);
void screen_beep(void);
int screen_keypress(int ms);

#define GETKEY wgetch(Wuser)

#define M1_quit		'Q'	/* quit */
#define M2_init		'i'	/* init */
#define M3_end		'E'	/* end */
#define M4_error_mesg	'e'	/* error_mesg */
#define M5_version	'v'	/* version */
#define M6_write_once	'W'	/* write_once */
#define M7_write_cont	'w'	/* write_cont */
#define M8_read_once	'R'	/* read_once */
#define M9_read_cont	'r'	/* read_cont */
#define M10_set_wr_amt	'a'	/* set_write_amt */
#define M11_check_data	'c'	/* check_data */

static void quit(void);
static void init(void);
static void end(void);
static void error_mesg(void);
static void version(void);
static void write_once(void);
static void write_cont(void);
static void read_once(void);
static void read_cont(void);
static void set_write_amt(void);
static void check_data(void);

static void initdisp(void);
static void menu(char **m, int nlines);
static void clear_screen(void);
static void generic_exit_routine(void);

char *menuformat[] = {
"%c=quit        %c=init        %c=end        %c=error_mesg  %c=version",
"%c=write_once  %c=write_cont  %c=read_once  %c=read_cont   %c=set_write_amt",
"%c=check_data"
};

#define NM ((sizeof(menuformat)/sizeof(menuformat[0])) + 1)
#define NMENULINES NM
char *progmenu[NMENULINES-1];
#define LINELEN	80
#define CTL(x)	(x & 0x0f)	/* control(x) */

#define PROMPT ": "
#define RESPLEN 32

static unsigned short Buf[LOS_MAX_WORDS];
static short Write_amt = 100;	// bytes to write
static int Check_data = 1;

int
main(void)
{
    int key = 0;

	signal(SIGINT, catchsig);
    initdisp();
    while(1) {
	wprintw(Wuser, PROMPT);
	wrefresh(Wuser);
	key = GETKEY;
	wprintw(Wuser, "%c\n", key);
	wrefresh(Wuser);
	switch(key) {
	case M1_quit:
	    quit();
	    break;
	case M2_init:
	    init();
	    break;
	case M3_end:
	    end();
	    break;
	case M4_error_mesg:
	    error_mesg();
	    break;
	case M5_version:
	    version();
	    break;
	case M6_write_once:
	    write_once();
	    break;
	case M7_write_cont:
	    write_cont();
	    break;
	case M8_read_once:
	    read_once();
	    break;
	case M9_read_cont:
	    read_cont();
	    break;
	case M10_set_wr_amt:
	    set_write_amt();
	    break;
	case M11_check_data:
	    check_data();
	    break;
	case CTL('L'):
	    clear_screen();
	    break;
	case CTL('C'):
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
    sprintf(progmenu[0], menuformat[0],
    	M1_quit, M2_init, M3_end, M4_error_mesg, M5_version);
    sprintf(progmenu[1], menuformat[1],
    	M6_write_once, M7_write_cont, M8_read_once, M9_read_cont,
	M10_set_wr_amt);
    sprintf(progmenu[2], menuformat[2],
    	M11_check_data);

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
quit()
{
    int ret;
    ret = los_end();
    if (ret) {
	screen_printf("Error: %s\n", los_strerror());
    }
    generic_exit_routine();
}

static void
init()
{
    static int bus = 0;
    static int intr = 0;
    static int ms = -1;
    char resp[RESPLEN];
    int ret;
    static int slot = 0x11;
    static int wr = 0;

    screen_dialog(resp, RESPLEN, "Bus ? [%d] ", bus);
    if ('\0' != resp[0]) {
	bus = atoi(resp);
    }
    screen_printf("Bus is %d\n", bus);

    screen_dialog(resp, RESPLEN, "Slot ? [0x%02x] ", slot);
    if ('\0' != resp[0]) {
	slot = (int)strtoul(resp, NULL, 16);
    }
    screen_printf("Slot is 0x%02x\n", slot);

    screen_dialog(resp, RESPLEN, "Use interrupts? [%d] ", intr);
    if ('\0' != resp[0]) {
	intr = atoi(resp);
    }
    screen_printf("%susing interrupts\n", intr ? "" : "Not " );

    screen_dialog(resp, RESPLEN, "Write mode? [%d] ", wr);
    if ('\0' != resp[0]) {
	wr = atoi(resp);
    }
    screen_printf("%s\n", wr ? "Writing" : "Reading" );

    screen_dialog(resp, RESPLEN, "Poll pause (ms)? [%d] ", ms);
    if ('\0' != resp[0]) {
	ms = atoi(resp);
    }

    ret = los_init(bus, slot, intr, wr, ms);
    if (ret) {
	screen_printf("Error: %s\n", los_strerror());
    }
}

static void
end()
{
    int ret;
    ret = los_end();
    if (ret) {
	screen_printf("Error: %s\n", los_strerror());
    }
}

static void
error_mesg()
{
    screen_printf("Error: %s\n", los_strerror());
}

static void
version()
{
    char *s = los_version();
    screen_printf("liblos version %s\n", s);
}

static void
write_once()
{
    short i;
    static unsigned char start = 0;
    unsigned char *cp;
    int ret;

    cp = (unsigned char *)Buf;
    for (i=0; i<Write_amt; i++) {
	*cp++ = start + i;
    }

    ret = los_write((unsigned char *)Buf, Write_amt);

    if (ret) {
	screen_printf("%s\n", los_strerror());
    } else {
	screen_printf("Wrote %d bytes\n", Write_amt);
    }
}

static void
write_cont()
{
    int wrcnt = 0;
    int MS = 30;  // ms to sleep between polls

    while (1) {
	screen_printf("  (%d)      PRESS ANY KEY TO STOP WRITING.\n", wrcnt++);

	// This screen_keypress waits up to MS milliseconds.
	if (screen_keypress(MS) != ERR) {
	    screen_printf("Finished write_cont\n");
	    return;
	}

	write_once();
    }
}

static void
set_write_amt()
{
    char resp[RESPLEN];

    screen_dialog(resp, RESPLEN, "Number of bytes to write? [%d] ", Write_amt);
    if ('\0' != resp[0]) {
	Write_amt = atoi(resp);
    }
    screen_printf("Write_amt is %d\n", Write_amt);
}

static void
check_data()
{
    if (Check_data) {
	Check_data = 0;
	screen_printf("NOT checking data\n");
    } else {
	Check_data = 1;
	screen_printf("Checking data\n");
    }
}

static void
read_once()
{
    short nbytes;
    int ret;
    unsigned short val;

    ret = los_read((unsigned char *)Buf, &nbytes);
    if (ret) {
	// Don't print the "timed out" errors.
	if (ret != -5) {
	    screen_printf("Error (%d): %s\n", ret, los_strerror());
	}
    } else {
	memcpy(&val, Buf+3, sizeof(unsigned short));
	screen_printf("Read %u bytes (buf no. %u)\n", nbytes, val);

#ifdef NOTDEF
	{
	    int i;
	    for (i=0; i<10; i++) {
		memcpy(&val, Buf+i, sizeof(unsigned short));
		screen_printf("\t%d. %04x %u\n", i, val, val);
	    }
	}
#endif
	
	if (Check_data) {
	    unsigned char *cp;
	    int i;
	    int offset = 10;
	    int ndata_bytes = Buf[4];
	    int want;

	    cp = (unsigned char *)Buf + offset;

	    for (want=0, i=0; i<ndata_bytes; want++, i++) {
		if (256 == want) {
		    want = 0;
		}
		if (*cp != want) {
		    screen_printf(
		    	"Error at data byte %d: expected %02x, got %02x\n",
			i, want, *cp);
		}
		cp++;
	    }

	    if (ndata_bytes % 2) {
		// odd ndata_bytes - check for FILL_BYTE
		if (FILL_BYTE != *cp) {
		    screen_printf("Didn't get FILL_BYTE (%02x)\n", FILL_BYTE);
		}
		i++;
	    }

	    // Do checksum.
	    {
		int i_wd = i / sizeof(short);
		unsigned short actual_chksum;
		unsigned short expected_chksum;

		int nwds = 5 + i_wd;

		actual_chksum = crc_short(Buf, nwds);
		memcpy(&expected_chksum, Buf + nwds, sizeof(short));
		if (expected_chksum == actual_chksum) {
		    screen_printf("Checksum okay\n");
		} else {
		    screen_printf("Bad checksum. Wanted %04x, got %04x\n",
		    	expected_chksum, actual_chksum);
		}
	    }
	}
    }
}

static void
read_cont()
{
    int MS = 30;  // ms to sleep between polls

    screen_printf("        PRESS ANY KEY TO STOP READING.\n");

    while (1) {
	// This screen_keypress waits up to MS milliseconds.
	if (screen_keypress(MS) != ERR) {
	    screen_printf("Finished read_cont\n");
	    return;
	}

	read_once();
    }
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

/* screen_keypress - return the key pressed, non-blocking; returns ERR if
 * no key pressed. */
int
screen_keypress(int ms)
{
    int key;
    wtimeout(Wuser, ms);
    key = wgetch(Wuser);
    wtimeout(Wuser, -1);
    return key;
}
