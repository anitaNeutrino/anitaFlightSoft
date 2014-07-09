/* driver - test sipcom library
 *
 * Marty Olevitch, May '05
 */

char Usage_msg[] = "[-d] [-e enables] [-h] [-o] [-p port] [-r throttle_rate]";

#include <sipcom.h>

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include "rand_no.h"

char *Progname;
int Debug = 0;
pthread_t Hr_thread;
pthread_t Vhr_thread;

int Do_Vhr = 1;
int Do_Hr = 1;

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
void write_highrate(int *ignore);
void write_very_highrate(int *ignore);

int
main(int argc, char *argv[])
{
    // for getopt
    int c;
    extern char *optarg;
    extern int optind;
    int rate = MAX_WRITE_RATE;	// bytes/sec throttle rate
    unsigned char sipmux_enable = 0;

    int ret;

    Progname = argv[0];

    while ((c = getopt(argc, argv, "de:hop:r:")) != EOF) {
	switch (c) {
	    case 'd':
	    	Debug = 1;
		break;
	    case 'e':
                sipmux_enable = (unsigned char)atoi(optarg);
		break;
	    case 'h':
	    	Do_Vhr = 0;
		fprintf(stderr, "Not doing HGA (very high rate).\n");
		break;
	    case 'o':
	    	Do_Hr = 0;
		fprintf(stderr, "Not doing OMNI (high rate).\n");
		break;
	    case 'p':
		fprintf(stderr, "Sorry, port option not implemented.\n");
		usage();
		exit(1);
		break;
	    case 'r':
	    	rate = atoi(optarg);
		fprintf(stderr, "throttle rate = %d bytes/sec\n", rate);
		break;
	    default:
	    case '?':
		usage();
		exit(1);
		break;
	}
    }

    printf("sipmux_enable = %02x\n", sipmux_enable);

    if (!Debug) {
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
    }

    sipcom_set_cmd_callback(handle_command);
    sipcom_set_cmd_length(129, 1);
    sipcom_set_cmd_length(159, 3);
    sipcom_set_cmd_length(138, 1);
    ret = sipcom_init(rate, CONFIG_DIR, sipmux_enable);
    if (ret) {
	char *s = sipcom_strerror();
	fprintf(stderr, "%s\n", s);
	exit(1);
    }

    // Start the very high rate writer process.
    if (Do_Vhr) {
	pthread_create(&Vhr_thread, NULL, (void *)write_very_highrate, NULL);
	pthread_detach(Vhr_thread);
    }

    // Start the high rate writer process.
    if (Do_Hr) {
	pthread_create(&Hr_thread, NULL, (void *)write_highrate, NULL);
	pthread_detach(Hr_thread);
    }

    fprintf(stderr, "I am pid %d\n", getpid());
    sipcom_wait();

    if (Do_Hr) {
	pthread_cancel(Hr_thread);
    }

    if (Do_Vhr) {
	pthread_cancel(Vhr_thread);
    }

    fprintf(stderr, "Bye bye\n");
    exit(0);
}

void
write_highrate(int *ignore)
{
    unsigned short amtb;
#define BSIZE 2048
    unsigned char buf[BSIZE];
    int lim;
    int retval;

    //memset(buf, 'a', BSIZE);
    {
	int i;
	for (i=0; i<2048; i++) {
	    buf[i] = i % 256;
	}
    }
    if (Debug) {
	rand_no_seed(0);
    } else {
	rand_no_seed(getpid());
    }
    
    lim = 2000;

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

#ifdef NOTDEF
	// This bit demonstrates changing the high rate throttle mitten
	// drin. Every 10 updates, it switches from 688 to 300 bytes/sec
	// and vice-versa.
	static int cnt = 0;
	if (cnt % 10 == 0) {
	    static unsigned long rate = 300;
	    fprintf(stderr, "=== high rate ===> rate = %lu\n", rate);
	    sipcom_highrate_set_throttle(rate);
	    if (rate == 300) {
		rate = 688;
	    } else {
		rate = 300;
	    }
	}
	++cnt;
#endif

#ifdef NOTDEF
	{
	    int amti = rand_no(lim);
	    fprintf(stderr, "======================= ARGH! %d\n", amti);
	    amtb = amti;
	}
#endif
	amtb = rand_no(lim);
	fprintf(stderr, "=== high rate ===> amtb = %u\n", amtb);
	retval = sipcom_highrate_write(buf, amtb);
	if (retval != 0) {
	    fprintf(stderr, "Bad write: %s", sipcom_strerror());
	}
    }

}

void
write_very_highrate(int *ignore)
{
    unsigned short amtb;
#define BSIZE 2048
    unsigned char buf[BSIZE];
    int lim;
    int retval;

    //memset(buf, 'a', BSIZE);
    {
	int i;
	for (i=0; i<2048; i++) {
	    buf[i] = i % 256;
	}
    }
    if (Debug) {
	rand_no_seed(0);
    } else {
	rand_no_seed(getpid());
    }
    
    lim = 2000;

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
	amtb = rand_no(lim);
	fprintf(stderr, "=== very high rate ===> amtb = %u\n", amtb);
	retval = sipcom_very_highrate_write(buf, amtb);
	if (retval != 0) {
	    fprintf(stderr, "Bad write: %s", sipcom_strerror());
	}

	//usleep(100);
    }

}

void
handle_command(unsigned char *cmd)
{
    //fprintf(stderr, "handle_command: cmd[0] = %02x (%d)\n", cmd[0], cmd[0]);

    if (cmd[0] == 129) {
	fprintf(stderr, "DISABLE_DATA_COLL\n");
    } else if (cmd[0] == 131) {
	// Use this command to quit.
	fprintf(stderr, "QUIT CMD\n");
	sipcom_end();
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
