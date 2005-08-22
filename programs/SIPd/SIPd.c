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
#include "utilLib/utilLib.h"
#include "anitaStructures.h"



pthread_t Hr_thread;

#define SLBUF_SIZE 240

// MAX_WRITE_RATE - maximum rate (bytes/sec) to write to SIP
#define MAX_WRITE_RATE	680L

int cmdLengths[256];

void handle_command(unsigned char *cmd);
void handle_slowrate_comm1();
void handle_slowrate_comm2();
void write_highrate(int *ignore);

char sipdPacketDir[FILENAME_MAX];
char sipdPidFile[FILENAME_MAX];
int numPacketDirs=0;

int main(int argc, char *argv[])
{
    int ret,numCmds=256,count,pk;
    char *tempString;
    char tempDir[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    
    char *progName=basename(argv[0]);
  
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

   /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("Cmdd.config","lengths") ;
    status += configLoad ("SIPd.config","global") ;
    eString = configErrorString (status) ;


    if (status == CONFIG_E_OK) {
//	printf("Here\n");
	numPacketDirs=kvpGetInt("numPacketDirs",9);
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
	tempString=kvpGetString("sipdPidFile");
	if(tempString) {
	    strncpy(sipdPidFile,tempString,FILENAME_MAX);
	    writePidFile(sipdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get sipdPidFile");
	    fprintf(stderr,"Couldn't get sipdPidFile\n");
	}
	tempString=kvpGetString("sipdPacketDir");
	if(tempString) {
	    strncpy(sipdPacketDir,tempString,FILENAME_MAX);
	    for(pk=0;pk<numPacketDirs;pk++) {
		sprintf(tempDir,"%s/pk%d/link",sipdPacketDir,pk);
		makeDirectories(tempDir);
	    }
		
	}
	else {
	    syslog(LOG_ERR,"Couldn't get sipdPacketDir");
	    fprintf(stderr,"Couldn't get sipdPacketDir\n");
	}
	//printf("%d\t%s\n",kvpStatus,kvpErrorString(kvpStatus));
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
	fprintf(stderr,"Error reading config file: %s\n",eString);
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
    if (ret) {
	char *s = sipcom_strerror();
	fprintf(stderr, "%s\n", s);
	exit(1);
    }
    for(count=0;count<numCmds;count++) {
	if(cmdLengths[count]) {
	    printf("%d\t%d\n",count,cmdLengths[count]);
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
    pthread_create(&Hr_thread, NULL, (void *)write_highrate, NULL);
    pthread_detach(Hr_thread);


    sipcom_wait();
    pthread_cancel(Hr_thread);
    fprintf(stderr, "Bye bye\n");
    return 0;
}


void write_highrate(int *ignore)
{
//    long amtb;
#define BSIZE 20000 //Silly hack until I packet up Events
    unsigned char buf[BSIZE];
//    long bufno = 1;
    int pk;
//    int bytes_avail;
    int retVal,count;
    int numLinks=0;
    long fileSize=0;
    char linkDir[FILENAME_MAX];
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int numItems=0;
    FILE *fp;
    struct dirent **linkList;

    //memset(buf, 'a', BSIZE);

//lim = 2000;    
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

//	amtb = rand_no(lim);

	for(pk=0;pk<numPacketDirs;pk++) {
	    sprintf(linkDir,"%s/pk%d/link",sipdPacketDir,pk);
	    numLinks=getListofLinks(linkDir,&linkList);
	    if(numLinks) break;
	}
	//Need to put something here so that it doesn't dick around 
	//forever in pk4, when there is data waiting in pk1, etc.
	
	for(count=0;count<numLinks;count++) {
	    sprintf(currentFilename,"%s/pk%d/%s",
		    sipdPacketDir,pk,linkList[count]->d_name);
	    sprintf(currentLinkname,"%s/pk%d/link/%s",
		    sipdPacketDir,pk,linkList[count]->d_name);
	    fp = fopen(currentFilename,"rb");
	    if(fp == NULL) {
		syslog(LOG_ERR,"Error opening file: %s",currentFilename);
		fprintf(stderr,"Error opening file: %s\n",currentFilename);
		//Mayhaps we should delete
		continue;
	    }
	    // Obtain file size
	    fseek(fp,0,SEEK_END);
	    fileSize=ftell(fp);
	    rewind(fp);
	    

	    numItems = fread(buf,1,fileSize,fp);
	    if(numItems!=1) {
		syslog(LOG_ERR,"Error reading file: %s",currentFilename);
		fprintf(stderr,"Error reading file: %s\n",currentFilename);
	    }	    
	    /*** the whole file is loaded in the buffer. ***/	    
	    fclose (fp);
	    fprintf(stderr, "=== high rate ===> amtb = %ld\n", fileSize);
	    retVal = sipcom_highrate_write(buf, fileSize);
	    if (retVal != 0) {
		syslog(LOG_ERR,"Couldn't write file: %s",currentFilename);
		fprintf(stderr, "Bad write\n");
	    }
	    else {
		removeFile(currentFilename);
		removeFile(currentLinkname);
	    }
	    break;
	    
	}
	
        for(count=0;count<numLinks;count++)
            free(linkList[count]);
        free(linkList);



    }

}

void
handle_command(unsigned char *cmd)
{
//    fprintf(stderr, "handle_command: cmd[0] = %02x (%d)\n", cmd[0], cmd[0]);
    char executeCommand[FILENAME_MAX];
    int byteNum=0;
    int retVal=0;
    if(cmdLengths[cmd[0]]) {
	printf("Got cmd: %d (length supposedly) %d\n",cmd[0],cmdLengths[cmd[0]]);
	sprintf(executeCommand,"Cmdd %d",cmd[0]);
	if(cmdLengths[cmd[0]]>1) {
	    for(byteNum=1;byteNum<cmdLengths[cmd[0]];byteNum++) 
		sprintf(executeCommand,"%s %d",executeCommand,cmd[byteNum]);
	}		
	syslog(LOG_INFO,"%s\n",executeCommand);
	retVal=system(executeCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error executing %s\n",executeCommand);
	    fprintf(stderr,"Error executing %s\n",executeCommand);
	}
    }
    else {
	printf("Weren't expecting cmd: %d\n",cmd[0]);
    }
	       

/*     if (cmd[0] == 129) { */
/* 	fprintf(stderr, "DISABLE_DATA_COLL\n"); */
/*     } else if (cmd[0] == 131) { */
/* 	// Use this command to quit. */
/* 	fprintf(stderr, "QUIT CMD\n"); */
/* 	sipcom_end(); */
/*     } else if (cmd[0] == 138) { */
/* 	fprintf(stderr, "HV_PWR_ON\n"); */
/*     } else if (cmd[0] == 221) { */
/* 	// Use the MARK command to change the throttle rate. Oops, need to */
/* 	// tell the highrate writer process to change the rate. */
/* 	unsigned short mark = (cmd[2] << 8) | cmd[1]; */
/* 	fprintf(stderr, "MARK %u\n", mark); */
/* 	sipcom_highrate_set_throttle(mark); */
/*     } else { */
/* 	fprintf(stderr, "Unknown command = 0x%02x (%d)\n", cmd[0], cmd[0]); */
/*     } */
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
