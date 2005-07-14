/* SIPd - Program that talks to sip.
 *
 * Ryan Nichol, July '05
 * Started off as a modified version of Marty's driver program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>

/* Flight soft includes */
#include "sipcom.h"
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "anitaStructures.h"



char *Progname;
pthread_t Hr_thread;

#define SLBUF_SIZE 240

// MAX_WRITE_RATE - maximum rate (bytes/sec) to write to SIP
#define MAX_WRITE_RATE	680L

int cmdLengths[256];

void handle_command(unsigned char *cmd);
void handle_slowrate_comm1();
void handle_slowrate_comm2();
void write_highrate(int *ignore);

int main(int argc, char *argv[])
{
    int ret,numCmds=256,count;

    Progname = argv[0];

    /* Config file thingies */
    int status=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    
    char *progName=basename(argv[0]);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

   /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status &= configLoad ("Cmdd.config","lengths") ;
    eString = configErrorString (status) ;


    if (status == CONFIG_E_OK) {
/* 	printf("Here\n"); */
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
	//printf("%d\t%s\n",kvpStatus,kvpErrorString(kvpStatus));
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
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
    for(count=0;count<numCmds;count++) {
	if(cmdLengths[count]) {
/* 	    printf("%d\t%d\n",count,cmdLengths[count]); */
	    sipcom_set_cmd_length(count,cmdLengths[count]);
	}
    }
    ret = sipcom_init(MAX_WRITE_RATE);
    if (ret) {
	char *s = sipcom_strerror();
	fprintf(stderr, "%s\n", s);
	exit(1);
    }
    
/*     // Start the high rate writer process. */
/*     pthread_create(&Hr_thread, NULL, (void *)write_highrate, NULL); */
/*     pthread_detach(Hr_thread); */


    sipcom_wait();
/*     pthread_cancel(Hr_thread); */
    fprintf(stderr, "Bye bye\n");
    return 0;
}


/* void write_highrate(int *ignore) */
/* { */
/*     long amtb; */
/* #define BSIZE 2048 */
/*     unsigned char buf[BSIZE]; */
/*     long bufno = 1; */
/*     int lim; */
/*     int bytes_avail; */
/*     int retval; */

/*     //memset(buf, 'a', BSIZE); */
/*     { */
/* 	int i; */
/* 	for (i=0; i<2048; i++) { */
/* 	    buf[i] = i % 256; */
/* 	} */
/*     } */
/*     rand_no_seed(getpid()); */
    
/*     lim = 2000; */

/*     { */
/* 	// We make this thread cancellable by any thread, at any time. This */
/* 	// should be okay since we don't have any state to undo or locks to */
/* 	// release. */
/* 	int oldtype; */
/* 	int oldstate; */
/* 	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype); */
/* 	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate); */
/*     } */

/*     while (1) { */

/* #ifdef NOTDEF */
/* 	// This bit demonstrates changing the high rate throttle mitten */
/* 	// drin. Every 10 updates, it switches from 688 to 300 bytes/sec */
/* 	// and vice-versa. */
/* 	static int cnt = 0; */
/* 	if (cnt % 10 == 0) { */
/* 	    static unsigned long rate = 300; */
/* 	    fprintf(stderr, "=== high rate ===> rate = %lu\n", rate); */
/* 	    sipcom_highrate_set_throttle(rate); */
/* 	    if (rate == 300) { */
/* 		rate = 688; */
/* 	    } else { */
/* 		rate = 300; */
/* 	    } */
/* 	} */
/* 	++cnt; */
/* #endif */

/* 	amtb = rand_no(lim); */
/* 	fprintf(stderr, "=== high rate ===> amtb = %ld\n", amtb); */
/* 	retval = sipcom_highrate_write(buf, amtb); */
/* 	if (retval != 0) { */
/* 	    fprintf(stderr, "Bad write\n"); */
/* 	} */
/*     } */

/* } */

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
    } else if (cmd[0] == 221) {
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
