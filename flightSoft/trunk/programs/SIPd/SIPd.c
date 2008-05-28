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
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include <fcntl.h>
#include <zlib.h>
#include <libgen.h> //For Mac OS X

/* Flight soft includes */
#include "sipcomLib/sipcom.h"
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "linkWatchLib/linkWatchLib.h"
#include "includes/anitaStructures.h"
#include "includes/anitaCommand.h"
  
#define SLBUF_SIZE 240
// MAX_WRITE_RATE - maximum rate (bytes/sec) to write to SIP
#define MAX_WRITE_RATE	680L

//#define COMPLETELY_FAKE 1


#define SEND_REAL_SLOW_DATA 1
#define NUM_HK_TELEM_DIRS 20 

typedef enum {
  TDRSS_TELEM_FIRST=0,
  TDRSS_TELEM_CMD_ECHO=0,
  TDRSS_TELEM_MONITOR,
  TDRSS_TELEM_HEADER,
  TDRSS_TELEM_HK,
  TDRSS_TELEM_ADU5A_SAT,
  TDRSS_TELEM_ADU5B_SAT,
  TDRSS_TELEM_G12_SAT,
  TDRSS_TELEM_ADU5A_PAT,
  TDRSS_TELEM_ADU5B_PAT,
  TDRSS_TELEM_G12_POS,
  TDRSS_TELEM_ADU5A_VTG,
  TDRSS_TELEM_ADU5B_VTG,
  TDRSS_TELEM_G12_GGA,
  TDRSS_TELEM_ADU5A_GGA,
  TDRSS_TELEM_ADU5B_GGA,
  TDRSS_TELEM_SURFHK,
  TDRSS_TELEM_TURFRATE,
  TDRSS_TELEM_OTHER,
  TDRSS_TELEM_PEDESTAL,
  TDRSS_TELEM_REQUEST,
  TDRSS_TELEM_NOT_A_TELEM
} TDRSSTelemType_t;



int getTdrssNumber();
void commandHandler(unsigned char *cmd);
void comm1Handler();
void comm2Handler();
void highrateHandler(int *ignore);
int readConfig();
void readAndSendEventRamdisk(char *headerLinkFilename);
int checkLinkDirAndTdrss(int maxCopy, char *telemDir, char *linkDir, int fileSize);
int readHkAndTdrss(int wd,int maxCopy, char *telemDir, char *linkDir, int fileSize, int *numSent);
void sendWakeUpBuffer();
void sendSomeHk(int maxBytes);
int sendEncodedPedSubbedSurfPackets(int bufSize);
int sendEncodedPedSubbedWavePackets(int bufSize);
int sendEncodedSurfPackets(int bufSize);
int sendRawWaveformPackets(int bufSize); 
int sendPedSubbedWaveformPackets(int bufSize);
int sendRawSurfPackets(int bufSize);
int sendPedSubbedSurfPackets(int bufSize); 
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
int highRateWrite(unsigned char *buf, unsigned short nbytes, int isHk);
float getTimeDiff(struct timeval oldTime, struct timeval currentTime);

// Config Thingies
int cmdLengths[256];


//Packet Dirs
char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];

//Packet Link Dirs
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];


//Output variables
int verbosity;
int printToScreen;
int sendData;

// Bandwidth variables
int headersPerEvent=30;
int echoesPerEvent=100;
int hkPerEvent=1;
int monitorPerEvent=1;


int eventBandwidth=5;
int priorityBandwidths[NUM_PRIORITIES];
int priorityOrder[NUM_PRIORITIES*100];
int numOrders=1000;
int orderIndex=0;
int currentPri=0;
unsigned int eventDataSent=0;
unsigned int hkDataSent=0;

//Data buffer
unsigned char theBuffer[MAX_EVENT_SIZE*5];
unsigned char chanBuffer[MAX_EVENT_SIZE*5];


//High rate thread
pthread_t Hr_thread;
int throttleRate=MAX_WRITE_RATE;
int sendWavePackets=0;

//Low Rate Struct
SlowRateFull_t slowRateData;

static char *telemLinkDirs[NUM_HK_TELEM_DIRS]=
  {SIPD_CMD_ECHO_TELEM_LINK_DIR,
     MONITOR_TELEM_LINK_DIR,
     HEADER_TELEM_LINK_DIR,
     HK_TELEM_LINK_DIR,
     ADU5A_SAT_TELEM_LINK_DIR,
     ADU5B_SAT_TELEM_LINK_DIR,
     G12_SAT_TELEM_LINK_DIR,
     ADU5A_PAT_TELEM_LINK_DIR,
     ADU5B_PAT_TELEM_LINK_DIR,
     G12_POS_TELEM_LINK_DIR,
     ADU5A_VTG_TELEM_LINK_DIR,
     ADU5B_VTG_TELEM_LINK_DIR,
     G12_GGA_TELEM_LINK_DIR,
     ADU5A_GGA_TELEM_LINK_DIR,
     ADU5B_GGA_TELEM_LINK_DIR,   
     SURFHK_TELEM_LINK_DIR,
     TURFHK_TELEM_LINK_DIR,
     OTHER_MONITOR_TELEM_LINK_DIR,
     PEDESTAL_TELEM_LINK_DIR,
     REQUEST_TELEM_LINK_DIR};
  static char *telemDirs[NUM_HK_TELEM_DIRS]=
    {SIPD_CMD_ECHO_TELEM_DIR,
     MONITOR_TELEM_DIR,
     HEADER_TELEM_DIR,
     HK_TELEM_DIR,
     ADU5A_SAT_TELEM_DIR,
     ADU5B_SAT_TELEM_DIR,
     G12_SAT_TELEM_DIR,
     ADU5A_PAT_TELEM_DIR,
     ADU5B_PAT_TELEM_DIR,
     G12_POS_TELEM_DIR,
     ADU5A_VTG_TELEM_DIR,
     ADU5B_VTG_TELEM_DIR,
     G12_GGA_TELEM_DIR,
     ADU5A_GGA_TELEM_DIR,
     ADU5B_GGA_TELEM_DIR,
     SURFHK_TELEM_DIR,
     TURFHK_TELEM_DIR,
     OTHER_MONITOR_TELEM_DIR,
     PEDESTAL_TELEM_DIR,
     REQUEST_TELEM_DIR};
static int maxPacketSize[NUM_HK_TELEM_DIRS]=
  {sizeof(CommandEcho_t),
   sizeof(MonitorStruct_t),
   sizeof(AnitaEventHeader_t),
   sizeof(HkDataStruct_t),
   sizeof(GpsAdu5SatStruct_t),
   sizeof(GpsAdu5SatStruct_t),
   sizeof(GpsG12SatStruct_t),
   sizeof(GpsAdu5PatStruct_t),
   sizeof(GpsAdu5PatStruct_t),
   sizeof(GpsG12PosStruct_t),
   sizeof(GpsAdu5VtgStruct_t),
   sizeof(GpsAdu5VtgStruct_t),
   sizeof(GpsGgaStruct_t),
   sizeof(GpsGgaStruct_t),
   sizeof(GpsGgaStruct_t),     
   sizeof(FullSurfHkStruct_t),
   sizeof(TurfRateStruct_t),
   sizeof(OtherMonitorStruct_t),
   sizeof(FullLabChipPedStruct_t),
   2000 //Who knows why
  };

//Will make these configurable soon
int hkTelemOrder[NUM_HK_TELEM_DIRS]={0,19,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18};
int hkTelemMaxCopy[NUM_HK_TELEM_DIRS]={10,1,3,3,1,1,1,5,5,2,2,2,1,1,1,5,5,3,1,3};

//Lazinesss
int wdEvents[NUM_PRIORITIES]={0};
int wdHks[NUM_HK_TELEM_DIRS]={0};
int numEventsSent[NUM_PRIORITIES]={0};
int numHksSent[NUM_HK_TELEM_DIRS]={0};

int main(int argc, char *argv[])
{
    //Temporary variables
    int retVal,numCmds=256,count,pri;

    /* Config file thingies */
    int status=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    
   
    char *progName=basename(argv[0]);

    //
    retVal=sortOutPidFile(progName);
    if(retVal<0)
      return -1;

  
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, handleBadSigs);
    signal(SIGINT, handleBadSigs);
    signal(SIGSEGV, handleBadSigs);


    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;


    syslog(LOG_INFO,"Starting SIPd\n");

    //Zero low rate structs
    memset(&slowRateData,0,sizeof(SlowRateFull_t));

   /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("Cmdd.config","lengths") ;
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);	
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
	fprintf(stderr,"Error reading config file: %s\n",eString);
    }

    
    //Fill event dir names
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
	sprintf(eventTelemDirs[pri],"%s/%s%d",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);
	sprintf(eventTelemLinkDirs[pri],"%s/link",eventTelemDirs[pri]);
	makeDirectories(eventTelemLinkDirs[pri]);
    }
    makeDirectories(SIPD_CMD_ECHO_TELEM_LINK_DIR);
    makeDirectories(HEADER_TELEM_LINK_DIR);
    makeDirectories(ADU5A_SAT_TELEM_LINK_DIR);
    makeDirectories(ADU5A_PAT_TELEM_LINK_DIR);
    makeDirectories(ADU5A_VTG_TELEM_LINK_DIR);
    makeDirectories(ADU5B_SAT_TELEM_LINK_DIR);
    makeDirectories(ADU5B_PAT_TELEM_LINK_DIR);
    makeDirectories(ADU5B_VTG_TELEM_LINK_DIR);
    makeDirectories(G12_GGA_TELEM_LINK_DIR);
    makeDirectories(ADU5A_GGA_TELEM_LINK_DIR);
    makeDirectories(ADU5B_GGA_TELEM_LINK_DIR);
    makeDirectories(G12_SAT_TELEM_LINK_DIR);
    makeDirectories(G12_POS_TELEM_LINK_DIR);
    makeDirectories(PEDESTAL_TELEM_LINK_DIR);
    makeDirectories(SURFHK_TELEM_LINK_DIR);
    makeDirectories(TURFHK_TELEM_LINK_DIR);
    makeDirectories(HK_TELEM_LINK_DIR);
    makeDirectories(MONITOR_TELEM_LINK_DIR);
    makeDirectories(OTHER_MONITOR_TELEM_LINK_DIR);
    makeDirectories(REQUEST_TELEM_LINK_DIR);


    retVal=readConfig();
#ifndef COMPLETELY_FAKE
    retVal = sipcom_set_slowrate_callback(COMM1, comm1Handler);
    if (retVal) {
	char *s = sipcom_strerror();
	syslog(LOG_ERR,"Couldn't set COMM1 Handler -- %s\n",s);
	fprintf(stderr,"Couldn't set COMM1 Handler -- %s\n",s);
    }

    retVal = sipcom_set_slowrate_callback(COMM2, comm2Handler);
    if (retVal) {
	char *s = sipcom_strerror();
	syslog(LOG_ERR,"Couldn't set COMM2 Handler -- %s\n",s);
	fprintf(stderr,"Couldn't set COMM2 Handler -- %s\n",s);
    }

    sipcom_set_cmd_callback(commandHandler);
    if (retVal) {
	char *s = sipcom_strerror();
	syslog(LOG_ERR,"Couldn't set COMM2 Handler -- %s\n",s);
	fprintf(stderr,"Couldn't set COMM2 Handler -- %s\n",s);
    }
    for(count=0;count<numCmds;count++) {
	if(cmdLengths[count]) {
	    printf("%d\t%d\n",count,cmdLengths[count]);
	    sipcom_set_cmd_length(count,cmdLengths[count]);
	}
    }

    printf("Max Write Rate %ld\n",MAX_WRITE_RATE);
    retVal = sipcom_init(MAX_WRITE_RATE);
    if (retVal) {
	char *s = sipcom_strerror();
	fprintf(stderr, "%s\n", s);
	handleBadSigs(1);
    }
     // Start the high rate writer process. 
    pthread_create(&Hr_thread, NULL, (void *)highrateHandler, NULL);
    pthread_detach(Hr_thread);


    sipcom_wait();
    pthread_cancel(Hr_thread);
    fprintf(stderr, "Bye bye\n");
    unlink(SIPD_PID_FILE);
    syslog(LOG_INFO,"SIPd terminating");
#else
    highrateHandler(&retVal);
#endif

    return 0;
}


void highrateHandler(int *ignore)
{
    char currentHeader[FILENAME_MAX];
    static int numLinks[NUM_PRIORITIES]={0};
    static int numHkLinks[NUM_HK_TELEM_DIRS]={0};
    int totalEventLinks=0;
    int totalHkLinks=0;
    int totalEventsSent=0;

    int pri=0,hkInd=0;
    char *tempString=0;
    int retVal=0;
    int numSent=0;


    {
        // We make this thread cancellable by any thread, at any time. This
        // should be okay since we don't have any state to undo or locks to
        // release.
        int oldtype;
        int oldstate;
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    }
    
    //First let the world know we're alive
    sendWakeUpBuffer();
    
    //Setup inotify watches
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
      //Setup inotify watches
      if(wdEvents[pri]==0) {
       wdEvents[pri]=setupLinkWatchDir(eventTelemLinkDirs[pri]);
       if(wdEvents[pri]<=0) {
	 fprintf(stderr,"Unable to watch %s\n",eventTelemLinkDirs[pri]);
	 syslog(LOG_ERR,"Unable to watch %s\n",eventTelemLinkDirs[pri]);
	 handleBadSigs(0);
       }
       numLinks[pri]=getNumLinks(wdEvents[pri]);
      }
    }
        
    for(hkInd=0;hkInd<NUM_HK_TELEM_DIRS;hkInd++) {
      if(wdHks[hkInd]==0) {	
	wdHks[hkInd]=setupLinkWatchDir(telemLinkDirs[hkInd]);
	if(wdHks[hkInd]<=0) {
	  fprintf(stderr,"Unable to watch %s\n",telemLinkDirs[hkInd]);
	  syslog(LOG_ERR,"Unable to watch %s\n",telemLinkDirs[hkInd]);
	  //	handleBadSigs(0);
	}
	numHkLinks[hkInd]=getNumLinks(wdHks[hkInd]);
      }      
    }

    int hkCount=0;
   
    while(1) {
	if(!sendData) {
	    sleep(1);
	    continue;
	}
	if(totalEventsSent%10==0) {
	  printf("Data sent\nEvents:\t");
	  for(pri=0;pri<NUM_PRIORITIES;pri++)
	    printf("%d ",numEventsSent[pri]);
	  printf("\nHk:\t");
	  for(hkInd=0;hkInd<NUM_HK_TELEM_DIRS;hkInd++) 
	    printf("%d ",numHksSent[hkInd]);
	  printf("\n");
	}


	//Update the link lists
	//or some time has passed
	retVal=checkLinkDirs(0,1);
	totalEventLinks=0;
	for(pri=0;pri<NUM_PRIORITIES;pri++) {
	  numLinks[pri]=getNumLinks(wdEvents[pri]);
	  totalEventLinks+=numLinks[pri];
	}
       
	totalHkLinks=0;
	for(hkInd=0;hkInd<NUM_HK_TELEM_DIRS;hkInd++) {
	  numHkLinks[hkInd]=getNumLinks(wdHks[hkInd]);
	  totalHkLinks+=numHkLinks[hkInd];
	}
	if(!retVal && totalHkLinks==0 && totalEventLinks==0) {
	  //No data
	  sleep(1);
	  continue;
	}
	printf("%d %d %d\n",totalEventLinks,totalHkLinks,retVal);
	
	if(totalEventLinks) {
	  //Which priority are we sending
	  currentPri=priorityOrder[orderIndex];	
	  while(numLinks[currentPri]==0) {
	    orderIndex++;
	    if(orderIndex>=numOrders) orderIndex=0;
	    currentPri=priorityOrder[orderIndex];	
	  }
	  printf("Trying priority %d -- numLinks %d\n",currentPri,numLinks[currentPri]);
	  if(numLinks[currentPri]) {
	    //Got an event	    
	    tempString=getLastLink(wdEvents[currentPri]);
	    sprintf(currentHeader,"%s/%s",eventTelemLinkDirs[currentPri],
		    tempString);
	    //	    syslog(LOG_INFO,"Trying %s\n",currentHeader);
	    readAndSendEventRamdisk(currentHeader); //Also deletes
	    numEventsSent[currentPri]++;
	    totalEventsSent++;
	    numLinks[currentPri]--;
	    totalEventLinks--;
	    
	    //Now try to send some stuff I like
	    numSent=0;
	    if(numHkLinks[TDRSS_TELEM_HEADER])
	      readHkAndTdrss(wdHks[TDRSS_TELEM_HEADER],
			     headersPerEvent,HEADER_TELEM_DIR,
			     HEADER_TELEM_LINK_DIR,sizeof(AnitaEventHeader_t),&numSent);
	    numHksSent[TDRSS_TELEM_HEADER]+=numSent;

	    numSent=0;
	    if(numHkLinks[TDRSS_TELEM_CMD_ECHO])
	      readHkAndTdrss(wdHks[TDRSS_TELEM_CMD_ECHO],echoesPerEvent,
			     SIPD_CMD_ECHO_TELEM_DIR,
			     SIPD_CMD_ECHO_TELEM_LINK_DIR,
			     sizeof(CommandEcho_t),&numSent);
	    numHksSent[TDRSS_TELEM_CMD_ECHO]+=numSent;
	    
	    numSent=0;
	    if(numHkLinks[TDRSS_TELEM_HK])
	      readHkAndTdrss(wdHks[TDRSS_TELEM_HK],hkPerEvent,HK_TELEM_DIR,
			     HK_TELEM_LINK_DIR,sizeof(HkDataStruct_t),&numSent);
	    numHksSent[TDRSS_TELEM_HK]+=numSent;
	    
	    numSent=0;
	    if(numHkLinks[TDRSS_TELEM_MONITOR])
	      readHkAndTdrss(wdHks[TDRSS_TELEM_MONITOR],monitorPerEvent,
			     MONITOR_TELEM_DIR,MONITOR_TELEM_LINK_DIR,
			     sizeof(MonitorStruct_t),&numSent); 
	    numHksSent[TDRSS_TELEM_MONITOR]+=numSent;
	    
	  }
	  //	else if(totalEventLinks==0) {
	  //	  usleep(1000);
	  //	}
	  
	  orderIndex++;
	  if(orderIndex>=numOrders) orderIndex=0;
	}

	//Now we try and send some data
	//I'm going to try and modify this but for now
	if(hkCount%eventBandwidth==0)
	  sendSomeHk(10000);
	hkCount++;
    }	

}


void commandHandler(unsigned char *cmd)
{
    fprintf(stderr, "commandHandler: cmd[0] = %02x (%d)\n", cmd[0], cmd[0]);
    CommandStruct_t theCmd;
    int byteNum=0;
    int retVal=0;
    theCmd.numCmdBytes=cmdLengths[cmd[0]];
    for(byteNum=0;byteNum<cmdLengths[cmd[0]];byteNum++) 
	theCmd.cmd[byteNum]=cmd[byteNum];
    

    
    //Need to add here

    if (cmd[0] == CMD_SIPD_REBOOT) {
	syslog(LOG_INFO,"Received command %d -- SIPd reboot",cmd[0]);
	system("daemon --stop -n Monitord");
	system("daemon --stop -n Acqd");
	system("daemon --stop -n Archived");
	system("daemon --stop -n GPSd");
	system("daemon --stop -n Eventd");
	system("daemon --stop -n Prioritizerd");
	system("rm -rf /tmp/anita/acqd /tmp/anita/prioritizerd /tmp/anita/eventd");
	system("sudo reboot");	
    } else if (cmd[0] == CMD_RESPAWN_PROGS && cmd[2]&0x2) {
	// Use this command to quit.
	syslog(LOG_INFO,"SIPd received respawn cmd\n");  
	retVal=writeCommandAndLink(&theCmd);
	sipcom_end();
    } else if (cmd[0] == SIPD_THROTTLE_RATE) {
	// Use the MARK command to change the throttle rate. Oops, need to
	// tell the highrate writer process to change the rate.
	unsigned short mark = (cmd[2] << 8) | cmd[1];
	syslog(LOG_INFO,"SIPd changing throttle rate to %u\n", mark);
	sipcom_highrate_set_throttle(mark);
	retVal=writeCommandAndLink(&theCmd);
    }
    else {
	retVal=writeCommandAndLink(&theCmd);
    }
	
}

void comm1Handler()
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
    fprintf(stderr, "comm1Handler %02x %02x\n", count, start);


//    fprintf(stderr,"Last Temp %d\n",slowRateData.sbsTemp[0]);
    FILE *fp =fopen(SLOW_RF_FILE,"rb");
    if(fp) {
	int numObjs=fread(&(slowRateData.rf),sizeof(SlowRateRFStruct_t),1,fp);
	if(numObjs<0) {
	    syslog(LOG_ERR,"Error reading %s: %s\n",SLOW_RF_FILE,strerror(errno));
	}
	fclose(fp);
    }



    slowRateData.unixTime=time(NULL);
    fillGenericHeader(&slowRateData,PACKET_SLOW_FULL,sizeof(SlowRateFull_t));
    printf("SlowRateFull_t %u\n",slowRateData.unixTime);
    ret = sipcom_slowrate_write(COMM1, (unsigned char*)&slowRateData, sizeof(SlowRateFull_t));

    if (ret) {
	fprintf(stderr, "comm1Handler: %s\n", sipcom_strerror());
    }
}

void comm2Handler()
{
    static unsigned char start = 255;
    unsigned char buf[SLBUF_SIZE];
    int i;
    int ret;

    for (i=0; i<SLBUF_SIZE; i++) {
	buf[i] = start - i;
    }
    --start;
    fprintf(stderr, "comm2Handler %02x\n", start);
   
    FILE *fp =fopen(SLOW_RF_FILE,"rb");
    if(fp) {
	int numObjs=fread(&(slowRateData.rf),sizeof(SlowRateRFStruct_t),1,fp);
	if(numObjs<0) {
	    syslog(LOG_ERR,"Error reading %s: %s\n",SLOW_RF_FILE,strerror(errno));
	}
	fclose(fp);
    }
    slowRateData.unixTime=time(NULL);
    fillGenericHeader(&slowRateData,PACKET_SLOW_FULL,sizeof(SlowRateFull_t));
    ret = sipcom_slowrate_write(COMM2,(unsigned char*) &slowRateData, sizeof(SlowRateFull_t));

    if (ret) {
	fprintf(stderr, "comm2Handler: %s\n", sipcom_strerror());
    }
}


int readConfig()
// Load SIPd config stuff
{
    // Config file thingies
    KvpErrorCode kvpStatus=0;
    int status=0,tempNum,count,maxVal,maxIndex;
    char* eString ;
    
    //Load output settings
    kvpReset();
    status = configLoad ("SIPd.config","output") ;
    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",-1);
	verbosity=kvpGetInt("verbosity",-1);
	if(printToScreen<0) {
	    printf("Couldn't fetch printToScreen, defaulting to zero\n");
	    printToScreen=0;
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading SIPd.config: %s\n",eString);
	fprintf(stderr,"Error reading SIPd.config: %s\n",eString);
    }

    //Load SIPd specfic settings
    kvpReset();
    status = configLoad ("SIPd.config","sipd");
    if(status == CONFIG_E_OK) {
	sendData=kvpGetInt("sendData",0);
	throttleRate=kvpGetInt("throttleRate",680);
	sendWavePackets=kvpGetInt("sendWavePackets",0);
	syslog(LOG_INFO,"sendWavePackets %d\n",sendWavePackets);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading SIPd.config: %s\n",eString);
	fprintf(stderr,"Error reading SIPd.config: %s\n",eString);
    }

    //Load bandwidth settings
    kvpReset();
    status = configLoad ("SIPd.config","bandwidth") ;
    if(status == CONFIG_E_OK) {
//	maxEventsBetweenLists=kvpGetInt("maxEventsBetweenLists",100);
	headersPerEvent=kvpGetInt("headersPerEvent",30);
	eventBandwidth=kvpGetInt("eventBandwidth",5);
	tempNum=10;
	kvpStatus = kvpGetIntArray("priorityBandwidths",priorityBandwidths,&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priorityBandwidths): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priorityBandwidths): %s\n",
			kvpErrorString(kvpStatus));
	}
	else {
	    for(orderIndex=0;orderIndex<NUM_PRIORITIES*100;orderIndex++) {
		//Check for hundreds first
		for(count=0;count<tempNum;count++) {
		    if(priorityBandwidths[count]>=100) 
			priorityOrder[orderIndex++]=count;
		}
		
		//Next look for highest number
		maxVal=0;
		maxIndex=-1;
		for(count=0;count<tempNum;count++) {
		    if(priorityBandwidths[count]<100) {
			if(priorityBandwidths[count]>maxVal) {
			    maxVal=priorityBandwidths[count];
			    maxIndex=count;
			}
		    }
		}
		if(maxIndex>-1) {
		    priorityOrder[orderIndex]=maxIndex;
		    priorityBandwidths[maxIndex]--;
		}
		else break;
	    }			
	    numOrders=orderIndex;
	    if(printToScreen) {
		printf("Priority Order\n");
		for(orderIndex=0;orderIndex<numOrders;orderIndex++) {
		    printf("%d ",priorityOrder[orderIndex]);
		}
		printf("\n");
	    }
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading SIPd.config: %s\n",eString);
	fprintf(stderr,"Error reading SIPd.config: %s\n",eString);
    }

    return status;
}


void readAndSendEventRamdisk(char *headerLinkFilename) {
    static int errorCounter=0;
    AnitaEventHeader_t *theHeader;
    GenericHeader_t *gHdr;
    int retVal;
    char currentTouchname[FILENAME_MAX];
    char currentLOSTouchname[FILENAME_MAX];
    char waveFilename[FILENAME_MAX];
    char headerFilename[FILENAME_MAX];
//    char crapBuffer[FILENAME_MAX];
    char justFile[FILENAME_MAX];
    unsigned int thisEventNumber;

    sprintf(justFile,"%s",basename(headerLinkFilename));
    sscanf(justFile,"hd_%u.dat",&thisEventNumber);
    if(thisEventNumber==0) {
	printf("Why is this zero -- %s\n",headerLinkFilename);
    }
    sprintf(headerFilename,"%s/hd_%d.dat",eventTelemDirs[currentPri], 
	    thisEventNumber);
    sprintf(waveFilename,"%s/hd_%d.dat",eventTelemDirs[currentPri], 
	    thisEventNumber);
    sprintf(currentTouchname,"%s.sipd",headerFilename);
    sprintf(currentLOSTouchname,"%s.losd",headerFilename);

    
    if(checkFileExists(currentLOSTouchname)) 
	return;
    touchFile(currentTouchname);
//    printf("%s\n",headerLinkFilename);
    removeFile(headerLinkFilename);

//     Next load header 
    theHeader=(AnitaEventHeader_t*) &theBuffer[0]; 
    retVal=fillHeader(theHeader,headerFilename); 
        
    if(retVal<0) {
	syslog(LOG_ERR,"Problem with %s",headerFilename);
	sprintf(headerFilename,"%s/hd_%d.dat",eventTelemDirs[currentPri], 
		thisEventNumber);
	
	removeFile(headerFilename);
	removeFile(waveFilename);
	
	//Bollocks
	return;
    }


    theHeader->gHdr.packetNumber=getTdrssNumber();
    retVal = highRateWrite((unsigned char*)theHeader,sizeof(AnitaEventHeader_t),0);
    if(retVal<0) {
	//Problem sending data
	syslog(LOG_ERR,"Problem sending file %s over TDRSS high rate -- %s\n",headerFilename,sipcom_strerror());
	fprintf(stderr,"Problem sending file %s over TDRSS high rate -- %s\n",headerFilename,sipcom_strerror());	
    }
    eventDataSent+=sizeof(AnitaEventHeader_t);
    
    
    thisEventNumber=theHeader->eventNumber;
    

    //Now get event file
    sprintf(headerFilename,"%s/hd_%d.dat",eventTelemDirs[currentPri], 
 	    thisEventNumber);
    sprintf(waveFilename,"%s/ev_%d.dat",eventTelemDirs[currentPri], 
 	    thisEventNumber);


    retVal=genericReadOfFile((unsigned char*)theBuffer,waveFilename,MAX_EVENT_SIZE);
    if(retVal<0) {
	fprintf(stderr,"Problem reading %s\n",waveFilename);
	syslog(LOG_ERR,"Problem reading %s\n",waveFilename);

//	removeFile(headerLinkFilename);
	removeFile(headerFilename);
	removeFile(waveFilename);	
	//Bollocks
	return;
    }


    //Okay so now the buffer either contains EncodedSurfPacketHeader_t or
    // it contains EncodedPedSubbedSurfPacketHeader_t
    gHdr = (GenericHeader_t*) &theBuffer[0];
    switch(gHdr->code) {
	case PACKET_BD:
	    if(sendWavePackets)
		retVal=sendRawWaveformPackets(retVal);
	    else 
		retVal=sendRawSurfPackets(retVal);
	    break;
	case PACKET_PED_SUBBED_EVENT:
	    if(sendWavePackets)
		retVal=sendPedSubbedWaveformPackets(retVal);
	    else
		retVal=sendPedSubbedSurfPackets(retVal);
	    break;
	case PACKET_ENC_EVENT_WRAPPER:
	    retVal=sendEncodedSurfPackets(retVal);
	    break;
	case PACKET_ENC_PEDSUB_EVENT_WRAPPER:
	    if(!sendWavePackets) {
		retVal=sendEncodedPedSubbedSurfPackets(retVal);
	    }
	    else {
		retVal=sendEncodedPedSubbedWavePackets(retVal);
	    }
	    break;
	default:
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Don't know what to do with packet %d -- %s (Message %d of 100)\n",gHdr->code,packetCodeAsString(gHdr->code),errorCounter);
		errorCounter++;
	    }
	    fprintf(stderr,"Don't know what to do with packet %d -- %s\n",gHdr->code,packetCodeAsString(gHdr->code));
    }
    


    
    if(printToScreen && verbosity>1) 
	printf("Removing files %s\t%s\n%s\n",headerFilename,waveFilename,
	       headerLinkFilename);
	        
    if(!checkFileExists(currentLOSTouchname)) {
//	removeFile(headerLinkFilename);
	removeFile(headerFilename);
	removeFile(waveFilename);
	removeFile(currentTouchname);
    }
    else {
	sleep(1);
//	removeFile(headerLinkFilename);
	removeFile(headerFilename);
	removeFile(waveFilename);
	removeFile(currentTouchname);
	removeFile(currentLOSTouchname);
    }



}

int sendEncodedSurfPackets(int bufSize) 
{
    static int errorCounter=0;
    EncodedSurfPacketHeader_t *surfHdPtr;
    int numBytes,count=0,surf=0,retVal;

    // Remember what the file contains is actually 9 EncodedSurfPacketHeader_t's
    count=sizeof(EncodedEventWrapper_t);
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedSurfPacketHeader_t*) &theBuffer[count];
	surfHdPtr->gHdr.packetNumber=getTdrssNumber();
	numBytes = surfHdPtr->gHdr.numBytes;
	if(numBytes) {
	  retVal = highRateWrite(&theBuffer[count],numBytes,0);
	    if(retVal<0) {
		//Problem sending data
		if(errorCounter<100) {
		    syslog(LOG_ERR,"Problem sending event %u over TDRSS high rate -- %s\n",surfHdPtr->eventNumber,sipcom_strerror());
		    errorCounter++;
		}
		fprintf(stderr,"Problem sending event %u over TDRSS high rate -- %s\n",surfHdPtr->eventNumber,sipcom_strerror());	
	    }
	    count+=numBytes;
	    eventDataSent+=numBytes;
	}
	else break;
	if(count>bufSize) return -1;
    }
    return 0;
}


int sendEncodedPedSubbedSurfPackets(int bufSize) 
{
    static int errorCounter=0;
    EncodedPedSubbedSurfPacketHeader_t *surfHdPtr;
    int numBytes,count=0,surf=0,retVal;

    // Remember what the file contains is actually 9 EncodedPedSubbedSurfPacketHeader_t's
//    count=0;
    count=sizeof(EncodedEventWrapper_t);
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedPedSubbedSurfPacketHeader_t*) &theBuffer[count];
	surfHdPtr->gHdr.packetNumber=getTdrssNumber();
	numBytes = surfHdPtr->gHdr.numBytes;
	if(numBytes) {
	  retVal = highRateWrite(&theBuffer[count],numBytes,0);
	    if(retVal<0) {
		//Problem sending data
		if(errorCounter<100) {
		    syslog(LOG_ERR,"Problem sending event %u over TDRSS high rate -- %s\n",surfHdPtr->eventNumber,sipcom_strerror());
		    errorCounter++;
		}
		fprintf(stderr,"Problem sending event %u over TDRSS high rate -- %s\n",surfHdPtr->eventNumber,sipcom_strerror());
	    }
	    count+=numBytes;
	    eventDataSent+=numBytes;
	}
	else break;
	if(count>bufSize) return -1;
    }
    return 0;
}


int sendEncodedPedSubbedWavePackets(int bufSize) 
{
    static int errorCounter=0;
    EncodedPedSubbedChannelPacketHeader_t *waveHdPtr;    
    EncodedPedSubbedSurfPacketHeader_t *surfHdPtr;  
    EncodedSurfChannelHeader_t *chanHdPtr;
    EncodedSurfChannelHeader_t *chanHdPtr2;
    int numBytes,count=0,surf=0,retVal,chan,count2=0;
    int chanNumBytes=0;
    // Remember what the file contains is actually 9 EncodedPedSubbedSurfPacketHeader_t's
//    count=0;
    count=sizeof(EncodedEventWrapper_t);
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedPedSubbedSurfPacketHeader_t*) &theBuffer[count];
//	surfHdPtr->gHdr.packetNumber=getTdrssNumber();
	numBytes = surfHdPtr->gHdr.numBytes;
	count2=count+sizeof(EncodedPedSubbedSurfPacketHeader_t);
	if(numBytes) {	    
	    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
		chanNumBytes=0;
		chanHdPtr = (EncodedSurfChannelHeader_t*) &theBuffer[count2];
		waveHdPtr= (EncodedPedSubbedChannelPacketHeader_t*) &chanBuffer[0];
		waveHdPtr->gHdr.packetNumber=getTdrssNumber();
		waveHdPtr->eventNumber=surfHdPtr->eventNumber;
		waveHdPtr->whichPeds=surfHdPtr->whichPeds;
		chanNumBytes=sizeof(EncodedPedSubbedChannelPacketHeader_t);
		chanHdPtr2 = (EncodedSurfChannelHeader_t*) &chanBuffer[chanNumBytes];
		(*chanHdPtr2)=(*chanHdPtr);
		count2+=sizeof(EncodedSurfChannelHeader_t);
		chanNumBytes+=sizeof(EncodedSurfChannelHeader_t);
		memcpy(&chanBuffer[chanNumBytes],&theBuffer[count2],chanHdPtr->numBytes);
		chanNumBytes+=chanHdPtr->numBytes;
		fillGenericHeader(waveHdPtr,PACKET_ENC_WV_PEDSUB,chanNumBytes);
		retVal = highRateWrite(chanBuffer,chanNumBytes,0);
		eventDataSent+=chanNumBytes;
		if(retVal<0) {
		    //Problem sending data
		    if(errorCounter<100) {
			syslog(LOG_ERR,"Problem sending event %u over TDRSS high rate -- %s\n",surfHdPtr->eventNumber,sipcom_strerror());
			errorCounter++;
		    }
		    fprintf(stderr,"Problem sending event %u over TDRSS high rate -- %s\n",surfHdPtr->eventNumber,sipcom_strerror());
		}
	    }		
	    count+=numBytes;

	}
	else break;
	if(count>bufSize) return -1;
    }
    return 0;
}


int sendRawWaveformPackets(int bufSize) 
{
    static int errorCounter=0;
    int retVal;
    if(bufSize!=sizeof(AnitaEventBody_t)) {
	if(errorCounter<100) {
	  syslog(LOG_ERR,"Buffer size %d, expected %u -- Bailing\n",bufSize,(unsigned int)sizeof(AnitaEventBody_t));
	  fprintf(stderr,"Buffer size %d, expected %u -- Bailing\n",bufSize,(unsigned int)sizeof(AnitaEventBody_t));
	    errorCounter++;
	}
	return -1;
    }
    AnitaEventBody_t *bdPtr = (AnitaEventBody_t*) theBuffer;
    int chan;
    RawWaveformPacket_t *wvPtr;
    for(chan=0;chan<NUM_DIGITZED_CHANNELS;chan++) {
	wvPtr=(RawWaveformPacket_t*) chanBuffer;
	wvPtr->eventNumber=bdPtr->eventNumber;
	memcpy(&(wvPtr->waveform),&(bdPtr->channel[chan]),sizeof(SurfChannelFull_t));
	wvPtr->gHdr.packetNumber=getTdrssNumber();
	fillGenericHeader(wvPtr,PACKET_WV,sizeof(RawWaveformPacket_t));
	retVal = highRateWrite(chanBuffer,sizeof(RawWaveformPacket_t),0);
	eventDataSent+=sizeof(RawWaveformPacket_t);
	if(retVal<0) {
	    //Problem sending data
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Problem sending event %u over TDRSS high rate -- %s\n",bdPtr->eventNumber,sipcom_strerror());
		errorCounter++;
	    }
	    fprintf(stderr,"Problem sending event %u over TDRSS high rate -- %s\n",bdPtr->eventNumber,sipcom_strerror());
	}
    }
    return retVal;
}


int sendRawSurfPackets(int bufSize) 
{
    static int errorCounter=0;
    int retVal;
    if(bufSize!=sizeof(AnitaEventBody_t)) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Buffer size %d, expected %u -- Bailing\n",bufSize,(unsigned int)sizeof(AnitaEventBody_t));
	    fprintf(stderr,"Buffer size %d, expected %u -- Bailing\n",bufSize,(unsigned int)sizeof(AnitaEventBody_t));
	    errorCounter++;
	}
	return -1;
    }
    AnitaEventBody_t *bdPtr = (AnitaEventBody_t*) theBuffer;
    int surf;
    RawSurfPacket_t *surfPtr;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfPtr=(RawSurfPacket_t*) chanBuffer;
	surfPtr->eventNumber=bdPtr->eventNumber;	
	memcpy(&(surfPtr->waveform[0]),&(bdPtr->channel[CHANNELS_PER_SURF*surf]),sizeof(SurfChannelFull_t)*CHANNELS_PER_SURF);

	surfPtr->gHdr.packetNumber=getTdrssNumber();
	fillGenericHeader(surfPtr,PACKET_SURF,sizeof(RawSurfPacket_t));
	retVal = highRateWrite(chanBuffer,sizeof(RawSurfPacket_t),0);
	eventDataSent+=sizeof(RawSurfPacket_t);
	if(retVal<0) {
	    //Problem sending data
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Problem sending event %u over TDRSS high rate -- %s\n",bdPtr->eventNumber,sipcom_strerror());
		errorCounter++;
	    }
	    fprintf(stderr,"Problem sending event %u over TDRSS high rate -- %s\n",bdPtr->eventNumber,sipcom_strerror());
	}
    }
    return retVal;
}


int sendPedSubbedWaveformPackets(int bufSize) 
{
    static int errorCounter=0;
    int retVal;
    if(bufSize!=sizeof(PedSubbedEventBody_t)) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Buffer size %d, expected %u -- Bailing\n",bufSize,(unsigned int)sizeof(PedSubbedEventBody_t));
	    fprintf(stderr,"Buffer size %d, expected %u -- Bailing\n",bufSize,(unsigned int)sizeof(PedSubbedEventBody_t));
	    errorCounter++;
	}
	return -1;
    }
    PedSubbedEventBody_t *bdPtr = (PedSubbedEventBody_t*) theBuffer;
    int chan;
    PedSubbedWaveformPacket_t *wvPtr;
    for(chan=0;chan<NUM_DIGITZED_CHANNELS;chan++) {
	wvPtr=(PedSubbedWaveformPacket_t*) chanBuffer;
	wvPtr->eventNumber=bdPtr->eventNumber;
	memcpy(&(wvPtr->waveform),&(bdPtr->channel[chan]),sizeof(SurfChannelPedSubbed_t));
	wvPtr->gHdr.packetNumber=getTdrssNumber();
	wvPtr->whichPeds=bdPtr->whichPeds;
	fillGenericHeader(wvPtr,PACKET_PEDSUB_WV,sizeof(PedSubbedWaveformPacket_t));

	retVal = highRateWrite(chanBuffer,sizeof(PedSubbedWaveformPacket_t),0);
	eventDataSent+=sizeof(PedSubbedWaveformPacket_t);
	if(retVal<0) {
	    //Problem sending data
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Problem sending event %u over TDRSS high rate -- %s\n",bdPtr->eventNumber,sipcom_strerror());
		errorCounter++;
	    }
	    fprintf(stderr,"Problem sending event %u over TDRSS high rate -- %s\n",bdPtr->eventNumber,sipcom_strerror());
	}

    }
    return retVal;
}


int sendPedSubbedSurfPackets(int bufSize) 
{
    static int errorCounter=0;
    int retVal;
    if(bufSize!=sizeof(PedSubbedEventBody_t)) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Buffer size %d, expected %u -- Bailing\n",bufSize,(unsigned int)sizeof(PedSubbedEventBody_t));
	    fprintf(stderr,"Buffer size %d, expected %u -- Bailing\n",bufSize,(unsigned int)sizeof(PedSubbedEventBody_t));
	    errorCounter++;
	}
	return -1;
    }
    PedSubbedEventBody_t *bdPtr = (PedSubbedEventBody_t*) theBuffer;
    int surf;
    PedSubbedSurfPacket_t *surfPtr;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfPtr=(PedSubbedSurfPacket_t*) chanBuffer;
	surfPtr->eventNumber=bdPtr->eventNumber;	
	surfPtr->whichPeds=bdPtr->whichPeds;	
	memcpy(&(surfPtr->waveform[0]),&(bdPtr->channel[CHANNELS_PER_SURF*surf]),sizeof(SurfChannelPedSubbed_t)*CHANNELS_PER_SURF);

	surfPtr->gHdr.packetNumber=getTdrssNumber();
	fillGenericHeader(surfPtr,PACKET_SURF,sizeof(PedSubbedSurfPacket_t));
	retVal = highRateWrite(chanBuffer,sizeof(PedSubbedSurfPacket_t),0);
	eventDataSent+=sizeof(PedSubbedSurfPacket_t);
	if(retVal<0) {
	    //Problem sending data
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Problem sending event %u over TDRSS high rate -- %s\n",bdPtr->eventNumber,sipcom_strerror());
		errorCounter++;
	    }
	    fprintf(stderr,"Problem sending event %u over TDRSS high rate -- %s\n",bdPtr->eventNumber,sipcom_strerror());
	}
    }
    return retVal;
}




int getTdrssNumber() {
    int retVal=0;
    static int firstTime=1;
    static int tdrssNumber=0;
    /* This is just to get the lastTdrssNumber in case of program restart. */
    FILE *pFile;
    if(firstTime) {
	pFile = fopen (LAST_TDRSS_NUMBER_FILE, "r");
	if(pFile == NULL) {
	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		    LAST_TDRSS_NUMBER_FILE);
	}
	else {	    	    
	    retVal=fscanf(pFile,"%d",&tdrssNumber);
	    if(retVal<0) {
		syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
			LAST_TDRSS_NUMBER_FILE);
	    }
	    fclose (pFile);
	}
	if(printToScreen) printf("The last tdrss number is %d\n",tdrssNumber);
	firstTime=0;
	//Only write out every 100th number, so always start from a 100
	tdrssNumber/=100;
	tdrssNumber++;
	tdrssNumber*=100;
    }
    tdrssNumber++;

    
    if(tdrssNumber%100==0) {
      pFile = fopen (LAST_TDRSS_NUMBER_FILE, "w");
      if(pFile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		LAST_TDRSS_NUMBER_FILE);
      }
      else {
	retVal=fprintf(pFile,"%d\n",tdrssNumber);
	if(retVal<0) {
	  syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		    LAST_TDRSS_NUMBER_FILE);
	}
	fclose (pFile);
      }
    }

    return tdrssNumber;

}

void sendWakeUpBuffer() 
{
    GenericHeader_t *gHdr = (GenericHeader_t*) &theBuffer[0];
    int count,retVal=0;
    for(count=sizeof(GenericHeader_t);count<WAKEUP_TDRSS_BUFFER_SIZE-sizeof(GenericHeader_t);count++) {
	theBuffer[count]=0xfe;
    }
    fillGenericHeader(gHdr,PACKET_WAKEUP_HIGHRATE,WAKEUP_TDRSS_BUFFER_SIZE);
    gHdr->packetNumber=getTdrssNumber();
    retVal = highRateWrite(theBuffer, count,1);
    if(retVal<0) {
	//Problem sending data
	syslog(LOG_ERR,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());
	fprintf(stderr,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());	
    }
    
}

void sendSomeHk(int maxBytes) 
{
    int hkCount=0;
    int hkInd=0,i;
    int numHkLinks[NUM_HK_TELEM_DIRS];
    int numSent=0;

    for(i=0;i<NUM_HK_TELEM_DIRS;i++) {
      hkInd=hkTelemOrder[i];
      //      fprintf(stderr,"hkInd is %d (%d)\n",hkInd,i);
      numHkLinks[hkInd]=getNumLinks(wdHks[hkInd]);
      //      fprintf(stderr,"numLinks %d %d %d %d-- %s\n",maxBytes,hkCount,maxPacketSize[hkInd],numHkLinks[hkInd],telemLinkDirs[hkInd]);
      if(numHkLinks[hkInd] && (maxBytes-hkCount)>maxPacketSize[hkInd]) {
	//Can try and send it
	numSent=0;
	hkCount+=readHkAndTdrss(wdHks[hkInd],hkTelemMaxCopy[hkInd],
				telemDirs[hkInd],
				telemLinkDirs[hkInd],maxPacketSize[hkInd],
				&numSent);
	numHksSent[hkInd]+=numSent;
      }
    }   
    hkDataSent+=hkCount;   
}


int checkLinkDirAndTdrss(int maxCopy, char *telemDir, char *linkDir, int fileSize) 
/* Looks in the specified directroy and TDRSS's up to maxCopy bytes of data */
/* fileSize is the maximum size of a packet in the directory */
{
    char currentFilename[FILENAME_MAX];
    char currentTouchname[FILENAME_MAX];
    char currentLOSTouchname[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int retVal,numLinks,count,numBytes,totalBytes=0;//,checkVal=0;
    GenericHeader_t *gHdr;
    struct dirent **linkList;
    
    GpsAdu5PatStruct_t *patPtr;
    HkDataStruct_t *hkPtr;


    numLinks=getListofLinks(linkDir,&linkList); 
    if(numLinks<=0) {
	return 0;
    }
    int counter=0;
    for(count=numLinks-1;count>=0;count--) {
	sprintf(currentFilename,"%s/%s",telemDir,
		linkList[count]->d_name);
	sprintf(currentTouchname,"%s.sipd",currentFilename);
	sprintf(currentLOSTouchname,"%s.losd",currentFilename);
	sprintf(currentLinkname,"%s/%s",
		linkDir,linkList[count]->d_name);

	if(checkFileExists(currentLOSTouchname)) 
	    continue;
	touchFile(currentTouchname);

	retVal=genericReadOfFile((unsigned char*)theBuffer,
				 currentFilename,
				 MAX_EVENT_SIZE);
	syslog(LOG_DEBUG,"Trying %s",currentFilename);
	if(retVal<=0) {
//	    syslog(LOG_ERR,"Error opening file, will delete: %s",
//		   currentFilename);
//	    fprintf(stderr,"Error reading file %s -- %d\n",currentFilename,retVal);
	    removeFile(currentFilename);

	    removeFile(currentLinkname);
	    removeFile(currentTouchname);
	    continue;
	}
	numBytes=retVal;

	if(printToScreen && verbosity>1) {
	    printf("Read File: %s -- (%d bytes)\n",currentFilename,numBytes);
	}
	

//	printf("Read %d bytes from file\n",numBytes);
//	Maybe I'll add a packet check here
	gHdr = (GenericHeader_t*)theBuffer;
//	checkVal=checkPacket(gHdr);
//	if(checkVal!=0 ) {
//	    printf("Bad packet %s == %d\n",currentFilename,checkVal);
//	}
	gHdr->packetNumber=getTdrssNumber();
	retVal = highRateWrite(theBuffer, numBytes,1);
	if(retVal<0) {
	    //Problem sending data
	    syslog(LOG_ERR,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());
	    fprintf(stderr,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());	
	}
	
	totalBytes+=numBytes;

	if(!checkFileExists(currentLOSTouchname)) {
	    removeFile(currentLinkname);
	    removeFile(currentFilename);
	    removeFile(currentTouchname);
	}
	else {
	    sleep(1);
	    removeFile(currentLinkname);
	    removeFile(currentFilename);
	    removeFile(currentTouchname);
	    removeFile(currentLOSTouchname);
	}
	int j;
	int tempInds[7]={1,3,6,37,19,15,36};
	int powerInds[4]={37,36,15,16};
	float powerCal[4]={18.252,10.1377,20,20};
	float tempVal;
	//Now fill low rate structs
	switch (gHdr->code) {
	    case PACKET_GPS_ADU5_PAT:
		patPtr=(GpsAdu5PatStruct_t*)gHdr;
		slowRateData.hk.altitude=patPtr->altitude;
		slowRateData.hk.longitude=patPtr->longitude;
		slowRateData.hk.latitude=patPtr->latitude;
		break;
	    case PACKET_HKD:
		hkPtr=(HkDataStruct_t*)gHdr;
		slowRateData.hk.temps[0]=(char)hkPtr->sbs.temp[0];
		for(j=0;j<7;j++) {
		    tempVal=((hkPtr->ip320.board[2].data[tempInds[j]])>>8);
		    //tempVal*=10;
		    //tempVal/=4096;
		    //tempVal-=5;
		    //tempVal*=100;
		    //tempVal+=273.15;
		    //if(tempVal<-127) tempVal=-127;
		    //if(tempVal>127) tempVal=127;
		    slowRateData.hk.temps[j+1]=(char)tempVal;
		}
		for(j=0;j<4;j++) {		    
		    tempVal=((hkPtr->ip320.board[1].data[powerInds[j]])>>8);
		    //tempVal*=10;
		    //tempVal/=4096;
		    //tempVal-=5;
		    //tempVal*=powerCal[j];
		    //if(tempVal<-127) tempVal=-127;
		    //if(tempVal>127) tempVal=127;
		    slowRateData.hk.powers[j]=(char)tempVal;
		}
		
		
	    default:
		break;
	}

//	if((totalBytes+fileSize)>maxCopy) break;
	counter++;
	if(counter>=maxCopy) break;
//	break;
    }
    
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
    
    return totalBytes;
}


int readHkAndTdrss(int wd,int maxCopy, char *telemDir, char *linkDir, int fileSize, int *numSent) 
/* Looks in the specified directroy and TDRSS's up to maxCopy bytes of data */
/* fileSize is the maximum size of a packet in the directory */
{
    char currentFilename[FILENAME_MAX];
    char currentTouchname[FILENAME_MAX];
    char currentLOSTouchname[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int retVal,numLinks,count,numBytes,totalBytes=0;//,checkVal=0;
    char *tempString;
    GenericHeader_t *gHdr;    
    GpsAdu5PatStruct_t *patPtr;
    HkDataStruct_t *hkPtr;
    *numSent=0;
    
    numLinks=getNumLinks(wd);
    if(numLinks<=0) {
	return 0;
    }
    int counter=0;
    for(count=numLinks-1;count>=0;count--) {
      //Get last link name
      tempString=getLastLink(wd);
      sprintf(currentFilename,"%s/%s",telemDir,tempString);
      sprintf(currentTouchname,"%s.sipd",currentFilename);
      sprintf(currentLOSTouchname,"%s.losd",currentFilename);
      sprintf(currentLinkname,"%s/%s",linkDir,tempString);

      if(checkFileExists(currentLOSTouchname)) 
	continue;
      touchFile(currentTouchname);

      retVal=genericReadOfFile((unsigned char*)theBuffer,
			       currentFilename,
			       MAX_EVENT_SIZE);
      syslog(LOG_DEBUG,"Trying %s",currentFilename);
      if(retVal<=0) {
	//	    syslog(LOG_ERR,"Error opening file, will delete: %s",
	//		   currentFilename);
	//	    fprintf(stderr,"Error reading file %s -- %d\n",currentFilename,retVal);
	removeFile(currentFilename);
	
	removeFile(currentLinkname);
	removeFile(currentTouchname);
	continue;
      }
      numBytes=retVal;

      if(printToScreen && verbosity>1) {
	printf("Read File: %s -- (%d bytes)\n",currentFilename,numBytes);
      }
      

      //	printf("Read %d bytes from file\n",numBytes);
      //	Maybe I'll add a packet check here
      gHdr = (GenericHeader_t*)theBuffer;
      //	checkVal=checkPacket(gHdr);
      //	if(checkVal!=0 ) {
      //	    printf("Bad packet %s == %d\n",currentFilename,checkVal);
      //	}
      gHdr->packetNumber=getTdrssNumber();
      retVal = highRateWrite(theBuffer, numBytes,1);
      if(retVal<0) {
	//Problem sending data
	syslog(LOG_ERR,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());
	fprintf(stderr,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());	
      }
      
      totalBytes+=numBytes;
      
      if(!checkFileExists(currentLOSTouchname)) {
	removeFile(currentLinkname);
	removeFile(currentFilename);
	removeFile(currentTouchname);
      }
      else {
	sleep(1);
	removeFile(currentLinkname);
	removeFile(currentFilename);
	removeFile(currentTouchname);
	removeFile(currentLOSTouchname);
      }
      (*numSent)++;

      //This bit here is just for the slow rate data
      int j;
      int tempInds[7]={1,3,6,37,19,15,36};
      int powerInds[4]={37,36,15,16};
      float powerCal[4]={18.252,10.1377,20,20};
      float tempVal;
      //Now fill low rate structs
      switch (gHdr->code) {
      case PACKET_GPS_ADU5_PAT:
	patPtr=(GpsAdu5PatStruct_t*)gHdr;
	slowRateData.hk.altitude=patPtr->altitude;
	slowRateData.hk.longitude=patPtr->longitude;
	slowRateData.hk.latitude=patPtr->latitude;
	break;
      case PACKET_HKD:
	hkPtr=(HkDataStruct_t*)gHdr;
	slowRateData.hk.temps[0]=(char)hkPtr->sbs.temp[0];
	for(j=0;j<7;j++) {
	  tempVal=((hkPtr->ip320.board[2].data[tempInds[j]])>>8);
	  //tempVal*=10;
	  //tempVal/=4096;
	  //tempVal-=5;
	  //tempVal*=100;
	  //tempVal+=273.15;
	  //if(tempVal<-127) tempVal=-127;
	  //if(tempVal>127) tempVal=127;
	  slowRateData.hk.temps[j+1]=(char)tempVal;
	}
	for(j=0;j<4;j++) {		    
	  tempVal=((hkPtr->ip320.board[1].data[powerInds[j]])>>8);
	  //tempVal*=10;
	  //tempVal/=4096;
	  //tempVal-=5;
	  //tempVal*=powerCal[j];
	  //if(tempVal<-127) tempVal=-127;
	  //if(tempVal>127) tempVal=127;
	  slowRateData.hk.powers[j]=(char)tempVal;
	}
	
	
      default:
	break;
      }
      
      //	if((totalBytes+fileSize)>maxCopy) break;
      counter++;
      if(counter>=maxCopy) break;
      //	break;
    }
    
    return totalBytes;
}


void handleBadSigs(int sig)
{
    fprintf(stderr,"Received sig %d -- will exit immediately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immediately\n",sig); 
    unlink(SIPD_PID_FILE);
    syslog(LOG_INFO,"SIPd terminating");
    exit(0);
}



int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(SIPD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,SIPD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(SIPD_PID_FILE);
  return 0;
}

int highRateWrite(unsigned char *buf, unsigned short nbytes, int isHk)
{
  static unsigned int dataCounter=0;
  static unsigned int hkCounter=0;
  static unsigned int eventCounter=0;
  static int first=1;
  static struct timeval lastTime;
  struct timeval newTime;
  float timeDiff;
  int retVal=0;
  if(first) {
    gettimeofday(&lastTime,0);
    first=0;
  }


  //Now send the data
#ifndef COMPLETELY_FAKE
  retVal=sipcom_highrate_write(buf,nbytes);
#else
  int fd=open("/tmp/sipdDump", O_CREAT | O_WRONLY);
  write(fd,buf,nbytes);
  close(fd);
  retVal=0;
#endif
  //Now some accounting
  dataCounter+=nbytes;
  if(isHk) hkCounter+=nbytes;
  else eventCounter+=nbytes;
  if(dataCounter>10000) {
    
    gettimeofday(&newTime,0);
    timeDiff=getTimeDiff(lastTime,newTime);
    printf("Transferred %u bytes (%u hk, %u event) in %2.2f seconds (%3.4f bytes/sec)\n",dataCounter,hkCounter,eventCounter,timeDiff,((float)dataCounter)/timeDiff);
    dataCounter=0;
    hkCounter=0;
    eventCounter=0;
    lastTime=newTime;
  }

  return retVal;
}


float getTimeDiff(struct timeval oldTime, struct timeval currentTime) {
  float timeDiff=currentTime.tv_sec-oldTime.tv_sec;
  timeDiff+=1e-6*(float)(currentTime.tv_usec-oldTime.tv_usec);
  return timeDiff;
}
