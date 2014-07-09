/* rdstdin - read the data from stdin and send to sip high rate tdrss
 *
 * Marty Olevitch, Jul '05
 */

char Usage_msg[] = "[-p port]";

#include <sipcom.h>

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>

char *Progname;
int Sleepsec = -1;
pid_t Hwriter;

#define SLBUF_SIZE 240

// MAX_WRITE_RATE - maximum rate (bytes/sec) to write to SIP
#define MAX_WRITE_RATE	680L

#define CONFIG_DIR "/usr/local/anita/sipcom"

void
usage()
{
    fprintf(stderr, "%s %s\n", Progname, Usage_msg);
}

void handle_command(unsigned char *cmd);
void handle_slowrate_comm1();
void handle_slowrate_comm2();
void write_highrate();
void sleep_then_quit(int sec);

int
main(int argc, char *argv[])
{
    // for getopt
    int c;
    extern char *optarg;
    extern int optind;

    int ret;

    Progname = argv[0];

    while ((c = getopt(argc, argv, "p:s:")) != EOF) {
	switch (c) {
	    case 'p':
		fprintf(stderr, "Sorry, port option not implemented.\n");
		usage();
		exit(1);
		break;
	    case 's':
	    	Sleepsec = atoi(optarg);
		break;
	    default:
	    case '?':
		usage();
		exit(1);
		break;
	}
    }

    ret = sipcom_set_slowrate_callback(COMM1, handle_slowrate_comm1);
    if (ret) {
	char *s = sipcom_strerror();
	fprintf(stderr, "%s\n", s);
	exit(1);
    }

    ret = sipcom_set_slowrate_callback(COMM2, handle_slowrate_comm2);
    if (ret) {
	char *s = sipcom_strerror();
	fprintf(stderr, "%s\n", s);
	exit(1);
    }

    sipcom_set_cmd_callback(handle_command);
    sipcom_set_cmd_length(129, 1);
    sipcom_set_cmd_length(159, 3);
    sipcom_set_cmd_length(138, 1);
    ret = sipcom_init(MAX_WRITE_RATE, CONFIG_DIR, 0);
    if (ret) {
	char *s = sipcom_strerror();
	fprintf(stderr, "%s\n", s);
	exit(1);
    }

    // Start the high rate writer process.
    Hwriter = fork();
    if (Hwriter == 0) {
	// child
	fprintf(stderr, "Hwriter here: pid %d\n", getpid());
	write_highrate();
	exit(0);
    } else if (Hwriter < 0) {
	// parent
	fprintf(stderr, "Bad fork #1 (%s)\n", strerror(errno));
	exit(1);
    }

    // Parent continues here normally.
    fprintf(stderr, "I am pid %d\n", getpid());
    fprintf(stderr, "Hwriter is pid %d\n", Hwriter);

    {
	pid_t quitter = fork();
	if (quitter == 0) {
	    // child
	    sleep_then_quit(Sleepsec);
	    exit(0);
	} else if (quitter < 0) {
	    // parent
	    fprintf(stderr, "Bad fork #2 (%s)\n", strerror(errno));
	    exit(1);
	}

	// Parent continues here normally.

    }

    fprintf(stderr, "At sipcom_wait()\n");
    sipcom_wait();

    fprintf(stderr, "At waitpid()\n");
    waitpid(Hwriter, NULL, 0);	// wait for Hwriter

    exit(0);
}

void
write_highrate()
{
#define BSIZE 2048
    unsigned char buf[BSIZE];
    size_t got;
    int retval;

    while (1) {
	got = fread(buf, 1, BSIZE, stdin);
	if (got != BSIZE && ferror(stdin)) {
	    fprintf(stderr, "write_highrate: fread error (%s)\n",
	    	strerror(errno));
	    return;
	}

	fprintf(stderr, "=== high rate ===> got = %u\n", got);
	retval = sipcom_highrate_write(buf, got);
	if (retval != 0) {
	    fprintf(stderr, "Bad write\n");
	}

	if (got != BSIZE && feof(stdin)) {
	    fprintf(stderr, "write_highrate: end of input\n");
	    return;
	}
    }

}

void
handle_command(unsigned char *cmd)
{
    //fprintf(stderr, "handle_command: cmd[0] = %02x (%d)\n", cmd[0], cmd[0]);

    if (cmd[0] == 129) {
	fprintf(stderr, "DISABLE_DATA_COLL\n");
    } else if (cmd[0] == 138) {
	fprintf(stderr, "HV_PWR_ON\n");
    } else if (cmd[0] == 159) {
	// Use the MARK command to change the throttle rate. Oops, need to
	// tell the highrate writer process to change the rate.
	unsigned short mark = (cmd[2] << 8) | cmd[1];
	fprintf(stderr, "MARK %u\n", mark);
	sipcom_highrate_set_throttle(mark);
    } else {
	fprintf(stderr, "Unknown command = 0x%02x (%d)\n", cmd[0], cmd[0]);
    }
}

void
handle_slowrate_comm1()
{
    static unsigned char start = 0;
    unsigned char buf[SLBUF_SIZE + 6];
    int i;
    int ret;

    static unsigned char count = 0;

    buf[0] = 0xbe;
    buf[1] = 0xef;
    buf[2] = count;
    for (i=3; i<SLBUF_SIZE+3; i++) {
	buf[i] = start + i;
    }
    buf[SLBUF_SIZE+3] = 0xca;
    buf[SLBUF_SIZE+4] = 0xfe;
    buf[SLBUF_SIZE+5] = count;
    ++count;

    ++start;
    fprintf(stderr, "handle_slowrate_comm1 %02x %02x\n", count, start);

    ret = sipcom_slowrate_write(COMM1, buf, SLBUF_SIZE+6);
    if (ret) {
	fprintf(stderr, "handle_slowrate_comm1: %s\n", sipcom_strerror());
    }
}

void
handle_slowrate_comm2()
{
    static unsigned char start = 255;
    unsigned char buf[SLBUF_SIZE];
    int i;
    int ret;

    for (i=0; i<SLBUF_SIZE; i++) {
	buf[i] = start - i;
    }
    --start;
    fprintf(stderr, "handle_slowrate_comm2 %02x\n", start);

    ret = sipcom_slowrate_write(COMM2, buf, SLBUF_SIZE);
    if (ret) {
	fprintf(stderr, "handle_slowrate_comm1: %s\n", sipcom_strerror());
    }
}

void
sleep_then_quit(int sec)
{
    if (sec == -1) {
	fprintf(stderr, "======================= not sleeping\n");
	return;
    }
    fprintf(stderr, "======================= sleeping %d seconds\n", sec);
    sleep(sec);
    fprintf(stderr, "======================= done sleeping\n");

    sipcom_end();

    // Kill Hwriter while we're at it.
    kill(Hwriter, SIGKILL);
}
