/* Openportd - Program that sends data to the Openport.
 *
 * Ryan Nichol, November 2014 (argh)
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
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "sipcomLib/sipcom.h"
#include "linkWatchLib/linkWatchLib.h"
#include "includes/anitaStructures.h"
#include "includes/anitaCommand.h"
  

#ifndef SIGRTMIN
#define SIGRTMIN 32
#endif

#define NUM_HK_TELEM_DIRS 26
#define REFRESH_LINKS_EVERY 600

typedef enum {
  OPENPORT_TELEM_FIRST=0,
  OPENPORT_TELEM_CMD_ECHO=0,  //0
  OPENPORT_TELEM_MONITOR, //1
  OPENPORT_TELEM_HEADER, //2
  OPENPORT_TELEM_HK,  //3
  OPENPORT_TELEM_ADU5A_SAT, //4
  OPENPORT_TELEM_ADU5B_SAT, //5 
  OPENPORT_TELEM_G12_SAT, //6
  OPENPORT_TELEM_ADU5A_PAT,  //7
  OPENPORT_TELEM_ADU5B_PAT, //8
  OPENPORT_TELEM_G12_POS, //9
  OPENPORT_TELEM_GPU, //10
  OPENPORT_TELEM_ADU5A_VTG, //11
  OPENPORT_TELEM_ADU5B_VTG,  //12
  OPENPORT_TELEM_G12_GGA, //13
  OPENPORT_TELEM_ADU5A_GGA, //14
  OPENPORT_TELEM_ADU5B_GGA, //15
  OPENPORT_TELEM_SURFHK, //16
  OPENPORT_TELEM_TURFRATE, //17
  OPENPORT_TELEM_AVGSURFHK, //18
  OPENPORT_TELEM_SUMTURFRATE, //19
  OPENPORT_TELEM_SSHK, //20
  OPENPORT_TELEM_OTHER, //21
  OPENPORT_TELEM_PEDESTAL, //22
  OPENPORT_TELEM_REQUEST, //23
  OPENPORT_TELEM_RTL, //24
  OPENPORT_TELEM_TUFF, //25
  OPENPORT_TELEM_NOT_A_TELEM
} OPENPORTTelemType_t;


void stopCopyScript();
int getOpenportNumber();
int getOpenportFileNumber();
int getOpenportRunNumber();
int readConfig();
int readAndSendEventRamdisk(char *headerLinkFilename);
int checkLinkDirAndOpenport(int maxCopy, char *telemDir, char *linkDir, int fileSize);
int readHkAndOpenport(int wd,int maxCopy, char *telemDir, char *linkDir, int fileSize, int *numSent);
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
int openportWrite(unsigned char *buf, unsigned short nbytes, int isHk);
float getTimeDiff(struct timeval oldTime, struct timeval currentTime);
void refreshLinksHandler(int sig);
void startCopyScript();



//Packet Dirs
char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];

//Packet Link Dirs
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];


//Output variables
int verbosity;
int printToScreen;
int sendData;
int sendWavePackets;
int standAloneMode=0;
int maxFileSize;


// Bandwidth variables
int headersPerEvent=30;
int echoesPerEvent=100;
int hkPerEvent=1;
int monitorPerEvent=1;

//Refresh links, cause monitord told us to
int needToRefreshLinks=0;


int eventBandwidth=5;
int priorityBandwidths[NUM_PRIORITIES];
int priorityOrder[NUM_PRIORITIES*100];
int numOrders=1000;
int orderIndex=0;
int currentPri=0;
unsigned int eventDataSent=0;
unsigned int hkDataSent=0;

//Copy stuff
pid_t copyScriptPid=-1;


//Data buffer
unsigned char theBuffer[MAX_EVENT_SIZE*5];
unsigned char chanBuffer[MAX_EVENT_SIZE*5];
//unsigned short openportBuffer[MAX_EVENT_SIZE*5];




static char *telemLinkDirs[NUM_HK_TELEM_DIRS]=
  {OPENPORTD_CMD_ECHO_TELEM_LINK_DIR,
   MONITOR_TELEM_LINK_DIR,
   HEADER_TELEM_LINK_DIR,
   HK_TELEM_LINK_DIR,
   ADU5A_SAT_TELEM_LINK_DIR,
   ADU5B_SAT_TELEM_LINK_DIR,
   G12_SAT_TELEM_LINK_DIR,
   ADU5A_PAT_TELEM_LINK_DIR,
   ADU5B_PAT_TELEM_LINK_DIR,
   G12_POS_TELEM_LINK_DIR,
   GPU_TELEM_LINK_DIR,
   ADU5A_VTG_TELEM_LINK_DIR,
   ADU5B_VTG_TELEM_LINK_DIR,
   G12_GGA_TELEM_LINK_DIR,
   ADU5A_GGA_TELEM_LINK_DIR,
   ADU5B_GGA_TELEM_LINK_DIR,   
   SURFHK_TELEM_LINK_DIR,
   TURFHK_TELEM_LINK_DIR,
   AVGSURFHK_TELEM_LINK_DIR,
   SUMTURFHK_TELEM_LINK_DIR,
   SSHK_TELEM_LINK_DIR,
   OTHER_MONITOR_TELEM_LINK_DIR,
   PEDESTAL_TELEM_LINK_DIR,
   REQUEST_TELEM_LINK_DIR,
   RTL_TELEM_LINK_DIR,
   TUFF_TELEM_LINK_DIR  
  };
static char *telemDirs[NUM_HK_TELEM_DIRS]=
  {OPENPORTD_CMD_ECHO_TELEM_DIR,  //0
   MONITOR_TELEM_DIR, //1
   HEADER_TELEM_DIR, //2
   HK_TELEM_DIR, //3 
   ADU5A_SAT_TELEM_DIR, //4
   ADU5B_SAT_TELEM_DIR, //5
   G12_SAT_TELEM_DIR, //6
   ADU5A_PAT_TELEM_DIR, //7 
   ADU5B_PAT_TELEM_DIR, //8
   G12_POS_TELEM_DIR, //9
   GPU_TELEM_DIR, //10
   ADU5A_VTG_TELEM_DIR, //11
   ADU5B_VTG_TELEM_DIR, //12
   G12_GGA_TELEM_DIR, //13
   ADU5A_GGA_TELEM_DIR, //14
   ADU5B_GGA_TELEM_DIR, //15
   SURFHK_TELEM_DIR, //16
   TURFHK_TELEM_DIR,//17 
   AVGSURFHK_TELEM_DIR, //18
   SUMTURFHK_TELEM_DIR, //19
   SSHK_TELEM_DIR, //20
   OTHER_MONITOR_TELEM_DIR, //21
   PEDESTAL_TELEM_DIR, //22
   REQUEST_TELEM_DIR, //23
   RTL_TELEM_DIR, //24
   TUFF_TELEM_DIR //25
  }; 

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
   sizeof(GpuPhiSectorPowerSpectrumStruct_t),
   sizeof(GpsAdu5VtgStruct_t),
   sizeof(GpsAdu5VtgStruct_t),
   sizeof(GpsGgaStruct_t),
   sizeof(GpsGgaStruct_t),
   sizeof(GpsGgaStruct_t),     
   sizeof(FullSurfHkStruct_t),
   sizeof(TurfRateStruct_t),
   sizeof(AveragedSurfHkStruct_t),
   sizeof(SummedTurfRateStruct_t),
   sizeof(SSHkDataStruct_t),
   sizeof(OtherMonitorStruct_t),
   sizeof(FullLabChipPedStruct_t),
   2000, //Who knows why
   sizeof(RtlSdrPowerSpectraStruct_t), 
   sizeof(TuffNotchStatus_t) 
  };

//Will make these configurable soon
int hkTelemOrder[NUM_HK_TELEM_DIRS]={0,20,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,21,22};
int hkTelemMaxPackets[NUM_HK_TELEM_DIRS]={10,3,3,3,1,1,1,5,5,5,3,2,2,1,1,1,5,5,3,1,3,1,1};


//Lazinesss
int wdEvents[NUM_PRIORITIES]={0};
int wdHks[NUM_HK_TELEM_DIRS]={0};
int numEventsSent[NUM_PRIORITIES]={0};
int numHksSent[NUM_HK_TELEM_DIRS]={0};

char currentOpenportDir[FILENAME_MAX];


int main(int argc, char *argv[])
{
    //Temporary variables
  int retVal=0,pri=0,ind=0;
  
  /* Config file thingies */
  //  int status=0;
  //  char* eString ;
  sprintf(currentOpenportDir,"%s",OPENPORT_OUTPUT_DIR);
   
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
    signal(SIGRTMIN, refreshLinksHandler);


    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;


    syslog(LOG_INFO,"Starting Openportd\n");

   /* Load Config */
    kvpReset () ;
    //    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    //    eString = configErrorString (status) ;

    //Fill event dir names
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
      retVal=snprintf(eventTelemDirs[pri],sizeof(eventTelemDirs[pri]),"%s/%s%d",BASE_EVENT_TELEM_DIR,EVENT_PRI_PREFIX,pri);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      retVal=snprintf(eventTelemLinkDirs[pri],sizeof(eventTelemLinkDirs[pri]),"%s/link",eventTelemDirs[pri]);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      makeDirectories(eventTelemLinkDirs[pri]);
    }    
    makeDirectories(OPENPORT_OUTPUT_DIR);
    makeDirectories(OPENPORT_STAGE_DIR);
    makeDirectories(OPENPORTD_CMD_ECHO_TELEM_LINK_DIR);


    for(ind=0;ind<NUM_HK_TELEM_DIRS;ind++) {
      makeDirectories(telemLinkDirs[ind]);
    }


    retVal=readConfig();
    
    char currentHeader[FILENAME_MAX];
    static int numLinks[NUM_PRIORITIES]={0};
    static int numHkLinks[NUM_HK_TELEM_DIRS]={0};
    int totalEventLinks=0;
    int totalHkLinks=0;
    int totalEventsSent=0;
    time_t currentTime=0;
    time_t lastRefresh=0;

    int hkInd=0;
    char *tempString=0;
    int numSent=0;
    
    
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
    time(&lastRefresh);

    int hkCount=0;
    
    printf("Before while loop\n");
    startCopyScript();

    do {
	if(printToScreen) printf("Initalizing Openportd\n");
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	  printf("Inside while loop\n");
	  if(!sendData) {
	    sleep(1);
	    continue;
	  }
	  
	  //The idea is that we only write an output file if there is space for it
	  int numWaitingToTransfer=countFilesInDir(currentOpenportDir);
	  if(numWaitingToTransfer>2) {
	    fprintf(stderr,"There are %d files waiting to transer, will sleep now\n",numWaitingToTransfer);
	    
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
	  
	  //Check to see if we need to refresh the links
	  time(&currentTime);
	  if(currentTime>lastRefresh+REFRESH_LINKS_EVERY || needToRefreshLinks) {
	    refreshLinkDirs();
	    lastRefresh=currentTime;
	    needToRefreshLinks=0;
	  }
	  
	  //Update the link lists
	  //or some time has passed
	  if(totalEventLinks==0)
	    retVal=checkLinkDirs(1,0);
	  else
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
	    needToRefreshLinks=1;
	    usleep(1000);
	    continue;
	  }
	  printf("%d %d %d\n",totalEventLinks,totalHkLinks,retVal);
	  
	  if(totalEventLinks) {
	    //Which priority are we sending
	    currentPri=priorityOrder[orderIndex];	
	    int doneEvent=0;
	    while(!doneEvent && totalEventLinks>0) {
	    while(numLinks[currentPri]==0) {
	      orderIndex++;
	      if(orderIndex>=numOrders) orderIndex=0;
	      currentPri=priorityOrder[orderIndex];	
	    }
	    printf("Trying priority %d -- numLinks %d\n",currentPri,numLinks[currentPri]);
	    if(numLinks[currentPri]) {
	      //Got an event	    
	      tempString=getLastLink(wdEvents[currentPri]);
	      retVal=snprintf(currentHeader,sizeof(currentHeader),"%s/%s",eventTelemLinkDirs[currentPri],tempString);
	      if(retVal<0) {
		syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
	      }
	      //	    syslog(LOG_INFO,"Trying %s\n",currentHeader);
	      doneEvent=readAndSendEventRamdisk(currentHeader); //Also deletes
	      if(doneEvent) {
		numEventsSent[currentPri]++;
		totalEventsSent++;
	      }
	      numLinks[currentPri]--;
	      totalEventLinks--;
	    }
	    }
	    
	  if(currentTime>lastRefresh+REFRESH_LINKS_EVERY || 
	     needToRefreshLinks) {
	    refreshLinkDirs();
	    lastRefresh=currentTime;
	    needToRefreshLinks=0;
	    
	    totalHkLinks=0;
	    for(hkInd=0;hkInd<NUM_HK_TELEM_DIRS;hkInd++) {
	      numHkLinks[hkInd]=getNumLinks(wdHks[hkInd]);
	      totalHkLinks+=numHkLinks[hkInd];
	    }	    
	  }
	    

	  //Now try to send some stuff I like
	  numSent=0;
	  if(numHkLinks[OPENPORT_TELEM_HEADER])
	    readHkAndOpenport(wdHks[OPENPORT_TELEM_HEADER],
			   headersPerEvent,HEADER_TELEM_DIR,
			   HEADER_TELEM_LINK_DIR,sizeof(AnitaEventHeader_t),&numSent);
	  numHksSent[OPENPORT_TELEM_HEADER]+=numSent;
	  
	  numSent=0;
	  if(numHkLinks[OPENPORT_TELEM_CMD_ECHO])
	    readHkAndOpenport(wdHks[OPENPORT_TELEM_CMD_ECHO],echoesPerEvent,
			   OPENPORTD_CMD_ECHO_TELEM_DIR,
			   OPENPORTD_CMD_ECHO_TELEM_LINK_DIR,
			   sizeof(CommandEcho_t),&numSent);
	  numHksSent[OPENPORT_TELEM_CMD_ECHO]+=numSent;
	  
	  numSent=0;
	  if(numHkLinks[OPENPORT_TELEM_HK])
	    readHkAndOpenport(wdHks[OPENPORT_TELEM_HK],hkPerEvent,HK_TELEM_DIR,
			   HK_TELEM_LINK_DIR,sizeof(HkDataStruct_t),&numSent);
	  
	  numSent=0;
	  if(numHkLinks[OPENPORT_TELEM_ADU5A_PAT])
	    readHkAndOpenport(wdHks[OPENPORT_TELEM_ADU5A_PAT],1,
			   ADU5A_PAT_TELEM_DIR,
			   ADU5A_PAT_TELEM_LINK_DIR,
			   sizeof(GpsAdu5PatStruct_t),&numSent);
	  numHksSent[OPENPORT_TELEM_ADU5A_PAT]+=numSent;
	  
	  numSent=0;
	  if(numHkLinks[OPENPORT_TELEM_MONITOR])
	    readHkAndOpenport(wdHks[OPENPORT_TELEM_MONITOR],monitorPerEvent,
			   MONITOR_TELEM_DIR,MONITOR_TELEM_LINK_DIR,
			   sizeof(MonitorStruct_t),&numSent); 
	  numHksSent[OPENPORT_TELEM_MONITOR]+=numSent;
	  
	
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
    } while (currentState == PROG_STATE_INIT);
    stopCopyScript();
    unlink(OPENPORTD_PID_FILE);
    return 0;
}





int readConfig()
// Load Openportd config stuff
{
    // Config file thingies
    KvpErrorCode kvpStatus=0;
    int status=0,tempNum,count,maxVal,maxIndex;
    char* eString ;
    
    //Load output settings
    kvpReset();
    status = configLoad ("Openportd.config","output") ;
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
	syslog(LOG_ERR,"Error reading Openportd.config: %s\n",eString);
	fprintf(stderr,"Error reading Openportd.config: %s\n",eString);
    }

    //Load Openportd specfic settings
    kvpReset();
    status = configLoad ("Openportd.config","openportd");
    if(status == CONFIG_E_OK) {
	sendData=kvpGetInt("sendData",0);
	sendWavePackets=kvpGetInt("sendWavePackets",0);
	maxFileSize=kvpGetInt("maxFileSize",50000);
	syslog(LOG_INFO,"sendWavePackets %d\n",sendWavePackets);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Openportd.config: %s\n",eString);
	fprintf(stderr,"Error reading Openportd.config: %s\n",eString);
    }

    //Load bandwidth settings
    kvpReset();
    status = configLoad ("Openportd.config","bandwidth") ;
    if(status == CONFIG_E_OK) {
//	maxEventsBetweenLists=kvpGetInt("maxEventsBetweenLists",100);
	headersPerEvent=kvpGetInt("headersPerEvent",30);
	eventBandwidth=kvpGetInt("eventBandwidth",5);
	tempNum=NUM_HK_TELEM_DIRS;
	kvpStatus = kvpGetIntArray("hkTelemOrder",hkTelemOrder,&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(hkTelemOrder): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(hkTelemOrder): %s\n",
			kvpErrorString(kvpStatus));
	}
	tempNum=NUM_HK_TELEM_DIRS;
	kvpStatus = kvpGetIntArray("hkTelemMaxPackets",hkTelemMaxPackets,&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(hkTelemMaxPackets): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(hkTelemMaxPackets): %s\n",
			kvpErrorString(kvpStatus));
	}


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
	syslog(LOG_ERR,"Error reading Openportd.config: %s\n",eString);
	fprintf(stderr,"Error reading Openportd.config: %s\n",eString);
    }

    return status;
}


int readAndSendEventRamdisk(char *headerLinkFilename) {
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
    

    //First up we test if the link still exists
    if(!checkFileExists(headerLinkFilename))
      return 0;
    
    retVal=snprintf(justFile,sizeof(justFile),"%s",basename(headerLinkFilename));
    if(retVal<0) {
      syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
    }
    retVal=sscanf(justFile,"hd_%u.dat",&thisEventNumber);
    if(retVal<=0) {
      syslog(LOG_ERR,"Error reading eventNumber from %s",justFile);
      thisEventNumber=0;
    }

    if(thisEventNumber==0) {
      printf("Why is this zero -- %s\n",headerLinkFilename);
    }

    retVal=snprintf(headerFilename,sizeof(headerFilename),"%s/hd_%d.dat",eventTelemDirs[currentPri],thisEventNumber);
    if(retVal<0) {
      syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
    }

    retVal=snprintf(waveFilename,sizeof(waveFilename),"%s/hd_%d.dat",eventTelemDirs[currentPri], thisEventNumber);
    if(retVal<0) {
      syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
    }

    retVal=snprintf(currentTouchname,sizeof(currentTouchname),"%s.sipd",headerFilename);
    if(retVal<0) {
      syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
    }

    retVal=snprintf(currentLOSTouchname,sizeof(currentLOSTouchname),"%s.losd",headerFilename);
    if(retVal<0) {
      syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
    }
    
    
    if(checkFileExists(currentLOSTouchname)) 
      return 0;
    touchFile(currentTouchname);
//    printf("%s\n",headerLinkFilename);
    removeFile(headerLinkFilename);

//     Next load header 
    theHeader=(AnitaEventHeader_t*) &theBuffer[0]; 
    retVal=fillHeader(theHeader,headerFilename); 
        
    if(retVal<0) {
//	syslog(LOG_ERR,"Problem with %s",headerFilename);
      retVal=snprintf(headerFilename,sizeof(headerFilename),"%s/hd_%d.dat",eventTelemDirs[currentPri],thisEventNumber);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      
      removeFile(currentTouchname);
      removeFile(headerFilename);
      removeFile(waveFilename);
      
      //Bollocks
      return 0;
    }
    
    
    theHeader->gHdr.packetNumber=getOpenportNumber();
    retVal = openportWrite((unsigned char*)theHeader,sizeof(AnitaEventHeader_t),0);
    if(retVal<0) {
	//Problem sending data
	syslog(LOG_ERR,"Problem sending file %s over Openport\n",headerFilename);
	fprintf(stderr,"Problem sending file %s over Openport\n",headerFilename);	
    }
    eventDataSent+=sizeof(AnitaEventHeader_t);
    
    
    thisEventNumber=theHeader->eventNumber;
    

    //Now get event file
    retVal=snprintf(headerFilename,sizeof(headerFilename),"%s/hd_%d.dat",eventTelemDirs[currentPri], thisEventNumber);
    if(retVal<0) {
      syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
    }
    retVal=snprintf(waveFilename,sizeof(waveFilename),"%s/ev_%d.dat",eventTelemDirs[currentPri], thisEventNumber);
    if(retVal<0) {
      syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
    }


    retVal=genericReadOfFile((unsigned char*)theBuffer,waveFilename,MAX_EVENT_SIZE);
    if(retVal<0) {
	fprintf(stderr,"Problem reading %s\n",waveFilename);
	syslog(LOG_ERR,"Problem reading %s\n",waveFilename);

//	removeFile(headerLinkFilename);
	removeFile(headerFilename);
	removeFile(waveFilename);	
	removeFile(currentTouchname);
	//Bollocks
	return 0;
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

    return 1;

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
	surfHdPtr->gHdr.packetNumber=getOpenportNumber();
	numBytes = surfHdPtr->gHdr.numBytes;
	if(numBytes) {
	  retVal = openportWrite(&theBuffer[count],numBytes,0);
	    if(retVal<0) {
		//Problem sending data
		if(errorCounter<100) {
		    syslog(LOG_ERR,"Problem sending event %u over OPENPORT high rate\n",surfHdPtr->eventNumber);
		    errorCounter++;
		}
		fprintf(stderr,"Problem sending event %u over OPENPORT high rate\n",surfHdPtr->eventNumber);	
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
	surfHdPtr->gHdr.packetNumber=getOpenportNumber();
	numBytes = surfHdPtr->gHdr.numBytes;
	if(numBytes) {
	  retVal = openportWrite(&theBuffer[count],numBytes,0);
	    if(retVal<0) {
		//Problem sending data
		if(errorCounter<100) {
		    syslog(LOG_ERR,"Problem sending event %u over OPENPORT high rate \n",surfHdPtr->eventNumber);
		    errorCounter++;
		}
		fprintf(stderr,"Problem sending event %u over OPENPORT high rate \n",surfHdPtr->eventNumber);
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
    // Remember what the file contains is actually 10 EncodedPedSubbedSurfPacketHeader_t's
//    count=0;
    count=sizeof(EncodedEventWrapper_t);
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedPedSubbedSurfPacketHeader_t*) &theBuffer[count];
//	surfHdPtr->gHdr.packetNumber=getOpenportNumber();
	numBytes = surfHdPtr->gHdr.numBytes;
	count2=count+sizeof(EncodedPedSubbedSurfPacketHeader_t);
	if(numBytes) {	    
	    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
		chanNumBytes=0;
		chanHdPtr = (EncodedSurfChannelHeader_t*) &theBuffer[count2];
		waveHdPtr= (EncodedPedSubbedChannelPacketHeader_t*) &chanBuffer[0];
		waveHdPtr->gHdr.packetNumber=getOpenportNumber();
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
		retVal = openportWrite(chanBuffer,chanNumBytes,0);
		eventDataSent+=chanNumBytes;
		if(retVal<0) {
		    //Problem sending data
		    if(errorCounter<100) {
			syslog(LOG_ERR,"Problem sending event %u over OPENPORT high rate \n",surfHdPtr->eventNumber);
			errorCounter++;
		    }
		    fprintf(stderr,"Problem sending event %u over OPENPORT high rate \n",surfHdPtr->eventNumber);
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
	wvPtr->gHdr.packetNumber=getOpenportNumber();
	fillGenericHeader(wvPtr,PACKET_WV,sizeof(RawWaveformPacket_t));
	retVal = openportWrite(chanBuffer,sizeof(RawWaveformPacket_t),0);
	eventDataSent+=sizeof(RawWaveformPacket_t);
	if(retVal<0) {
	    //Problem sending data
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Problem sending event %u over OPENPORT high rate\n",bdPtr->eventNumber);
		errorCounter++;
	    }
	    fprintf(stderr,"Problem sending event %u over OPENPORT high rate \n",bdPtr->eventNumber);
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

	surfPtr->gHdr.packetNumber=getOpenportNumber();
	fillGenericHeader(surfPtr,PACKET_SURF,sizeof(RawSurfPacket_t));
	retVal = openportWrite(chanBuffer,sizeof(RawSurfPacket_t),0);
	eventDataSent+=sizeof(RawSurfPacket_t);
	if(retVal<0) {
	    //Problem sending data
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Problem sending event %u over Openport\n",bdPtr->eventNumber);
		errorCounter++;
	    }
	    fprintf(stderr,"Problem sending event %u over Openport\n",bdPtr->eventNumber);
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
	wvPtr->gHdr.packetNumber=getOpenportNumber();
	wvPtr->whichPeds=bdPtr->whichPeds;
	fillGenericHeader(wvPtr,PACKET_PEDSUB_WV,sizeof(PedSubbedWaveformPacket_t));

	retVal = openportWrite(chanBuffer,sizeof(PedSubbedWaveformPacket_t),0);
	eventDataSent+=sizeof(PedSubbedWaveformPacket_t);
	if(retVal<0) {
	    //Problem sending data
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Problem sending event %u over Openport\n",bdPtr->eventNumber);
		errorCounter++;
	    }
	    fprintf(stderr,"Problem sending event %u over Openport\n",bdPtr->eventNumber);
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

	surfPtr->gHdr.packetNumber=getOpenportNumber();
	fillGenericHeader(surfPtr,PACKET_SURF,sizeof(PedSubbedSurfPacket_t));
	retVal = openportWrite(chanBuffer,sizeof(PedSubbedSurfPacket_t),0);
	eventDataSent+=sizeof(PedSubbedSurfPacket_t);
	if(retVal<0) {
	    //Problem sending data
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Problem sending event %u over Openport\n",bdPtr->eventNumber);
		errorCounter++;
	    }
	    fprintf(stderr,"Problem sending event %u over Openport\n",bdPtr->eventNumber);
	}
    }
    return retVal;
}




int getOpenportNumber() {
    int retVal=0;
    static int firstTime=1;
    static int openportNumber=0;
    /* This is just to get the lastOpenportNumber in case of program restart. */
    FILE *pFile;
    if(firstTime) {
	pFile = fopen (LAST_OPENPORT_NUMBER_FILE, "r");
	if(pFile == NULL) {
	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		    LAST_OPENPORT_NUMBER_FILE);
	}
	else {	    	    
	    retVal=fscanf(pFile,"%d",&openportNumber);
	    if(retVal<0) {
		syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
			LAST_OPENPORT_NUMBER_FILE);
	    }
	    fclose (pFile);
	}
	if(printToScreen) printf("The last openport number is %d\n",openportNumber);
	firstTime=0;
	//Only write out every 100th number, so always start from a 100
	openportNumber/=100;
	openportNumber++;
	openportNumber*=100;
    }
    openportNumber++;

    
    if(openportNumber%100==0) {
      pFile = fopen (LAST_OPENPORT_NUMBER_FILE, "w");
      if(pFile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		LAST_OPENPORT_NUMBER_FILE);
      }
      else {
	retVal=fprintf(pFile,"%d\n",openportNumber);
	if(retVal<0) {
	  syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		    LAST_OPENPORT_NUMBER_FILE);
	}
	fclose (pFile);
      }
    }

    return openportNumber;

}


void sendSomeHk(int maxBytes) 
{
  int retVal=0; 
  int hkCount=0;
  int hkInd=0,i;
  int numHkLinks[NUM_HK_TELEM_DIRS];
  int numSent=0;

  for(i=0;i<NUM_HK_TELEM_DIRS;i++) {
    hkInd=hkTelemOrder[i];
    //    fprintf(stderr,"hkInd is %d (%d)\n",hkInd,i);
    numHkLinks[hkInd]=getNumLinks(wdHks[hkInd]);
    //    retVal=fprintf(stderr,"numLinks %d %d %d %d-- %s\n",maxBytes,hkCount,maxPacketSize[hkInd],numHkLinks[hkInd],telemLinkDirs[hkInd]);
    //    if(retVal<0) {
    //      syslog(LOG_ERR,"Error using fprintf -- %s",strerror(errno));
    //    }
    if(numHkLinks[hkInd]>0 && (maxBytes-hkCount)>maxPacketSize[hkInd]) {
      //Can try and send it
      numSent=0;
      hkCount+=readHkAndOpenport(wdHks[hkInd],hkTelemMaxPackets[hkInd],
			      telemDirs[hkInd],
			      telemLinkDirs[hkInd],maxPacketSize[hkInd],
			      &numSent);
      numHksSent[hkInd]+=numSent;
    }
  }   
  hkDataSent+=hkCount;   
}


int checkLinkDirAndOpenport(int maxCopy, char *telemDir, char *linkDir, int fileSize) 
/* Looks in the specified directroy and OPENPORT's up to maxCopy bytes of data */
/* fileSize is the maximum size of a packet in the directory */
{
    char currentFilename[FILENAME_MAX];
    char currentTouchname[FILENAME_MAX];
    char currentLOSTouchname[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int retVal,numLinks,count,numBytes,totalBytes=0;//,checkVal=0;
    GenericHeader_t *gHdr;
    struct dirent **linkList;
    


    numLinks=getListofLinks(linkDir,&linkList); 
    if(numLinks<=0) {
	return 0;
    }
    int counter=0;
    for(count=numLinks-1;count>=0;count--) {
      retVal=snprintf(currentFilename,sizeof(currentFilename),"%s/%s",telemDir,
	       linkList[count]->d_name);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      retVal=snprintf(currentTouchname,sizeof(currentTouchname),"%s.sipd",currentFilename);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      retVal=snprintf(currentLOSTouchname,sizeof(currentLOSTouchname),"%s.losd",currentFilename);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      retVal=snprintf(currentLinkname,sizeof(currentLinkname),"%s/%s",
		      linkDir,linkList[count]->d_name);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      
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
	

	printf("Read %d bytes from file\n",numBytes);
//	Maybe I'll add a packet check here
	gHdr = (GenericHeader_t*)theBuffer;
//	checkVal=checkPacket(gHdr);
//	if(checkVal!=0 ) {
//	    printf("Bad packet %s == %d\n",currentFilename,checkVal);
//	}
	gHdr->packetNumber=getOpenportNumber();
	printf("Openport number %d\n",gHdr->packetNumber);
	retVal = openportWrite(theBuffer, numBytes,1);
	if(retVal<0) {
	    //Problem sending data
	    syslog(LOG_ERR,"Problem sending Wake up Packet over Openprt\n");
	    fprintf(stderr,"Problem sending Wake up Packet over Openport\n");	
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
	printf("Got code: %#x %#x\n",gHdr->code&BASE_PACKET_MASK,PACKET_GPS_ADU5_PAT);

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


int readHkAndOpenport(int wd,int maxCopy, char *telemDir, char *linkDir, int fileSize, int *numSent) 
/* Looks in the specified directroy and OPENPORT's up to maxCopy bytes of data */
/* fileSize is the maximum size of a packet in the directory */
{
  //  fprintf(stderr,"readHkAndOpenport %s -- %d\n",linkDir,maxCopy);
  char currentFilename[FILENAME_MAX];
  char currentTouchname[FILENAME_MAX];
  char currentLOSTouchname[FILENAME_MAX];
  char currentLinkname[FILENAME_MAX];
  int retVal,numLinks,count,numBytes,totalBytes=0;//,checkVal=0;
  char *tempString;
  GenericHeader_t *gHdr; 
  *numSent=0;
    
  numLinks=getNumLinks(wd);
  if(numLinks<=0) {
	return 0;
  }

  int counter=0;
    for(count=numLinks-1;count>=0;count--) {
      //Get last link name
      tempString=getLastLink(wd);
      retVal=snprintf(currentFilename,sizeof(currentFilename),"%s/%s",telemDir,tempString);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      retVal=snprintf(currentTouchname,sizeof(currentTouchname),"%s.sipd",currentFilename);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      retVal=snprintf(currentLOSTouchname,sizeof(currentLOSTouchname),"%s.losd",currentFilename);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
      retVal=snprintf(currentLinkname,sizeof(currentLinkname),"%s/%s",linkDir,tempString);
      if(retVal<0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }

      if(!checkFileExists(currentLinkname))
	continue;

      if(checkFileExists(currentLOSTouchname)) 
	continue;
      
      touchFile(currentTouchname);
      
      if(checkFileExists(currentLOSTouchname)) {	  
	  removeFile(currentTouchname);	  
	  continue;
      }

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

      if(printToScreen && verbosity>=0) {
	printf("Read File: %s -- (%d bytes)\n",currentFilename,numBytes);
      }
      

      //	printf("Read %d bytes from file\n",numBytes);
      //	Maybe I'll add a packet check here
      gHdr = (GenericHeader_t*)theBuffer;
      //	checkVal=checkPacket(gHdr);
      //	if(checkVal!=0 ) {
      //	    printf("Bad packet %s == %d\n",currentFilename,checkVal);
      //	}
      gHdr->packetNumber=getOpenportNumber();
      printf("Openport number %d\n",gHdr->packetNumber);
      retVal = openportWrite(theBuffer, numBytes,1);
      if(retVal<0) {
	//Problem sending data
	syslog(LOG_ERR,"Problem sending Wake up Packet over OPENPORT\n");
	fprintf(stderr,"Problem sending Wake up Packet over OPENPORT\n");	
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

      
      //	if((totalBytes+fileSize)>maxCopy) break;
      counter++;
      if(counter>=maxCopy) break;
      //	break;
    }
    //    fprintf(stderr,"readHkAndOpenport %s -- %d\n",linkDir,*numSent);
    return totalBytes;
}


void stopCopyScript() {
  //  fprintf(stderr,"stopCopyScript:\t");
   FILE *fp = fopen("/tmp/openportCopyPid","r");
  if(fp) {
    fscanf(fp,"%d",&copyScriptPid);
    fprintf(stderr,"Got copy script pid: %d\n",copyScriptPid);
    kill(copyScriptPid,SIGTERM);
  }
}


void handleBadSigs(int sig)
{
  int retVal=0;
  fprintf(stderr,"Received sig %d -- will exit immediately\n",sig); 
  syslog(LOG_CRIT,"Received sig %d -- will exit immediately\n",sig); 


  if(copyScriptPid>0) stopCopyScript();

  retVal=unlink(OPENPORTD_PID_FILE);
  if(retVal<0) {
    syslog(LOG_ERR,"Error deleting %s -- %s",OPENPORTD_PID_FILE,strerror(errno));
    //No idea what to do here, if you can't delete it you are pretty much buggered
  }
  syslog(LOG_INFO,"Openportd terminating");
  exit(0);
}



int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(OPENPORTD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,OPENPORTD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(OPENPORTD_PID_FILE);
  return 0;
}

int openportWrite(unsigned char *buf, unsigned short nbytes, int isHk)
{
  static unsigned int dataCounter=0;
  static unsigned int hkCounter=0;
  static unsigned int eventCounter=0;
  static int first=1;
  static struct timeval lastTime;
  struct timeval newTime;
  static FILE *fp=NULL;
  static int openedFile=0;
  static int bytesToFile=0;
  static char fileName[FILENAME_MAX];

  static int openportRun=-1;
  if(openportRun<0) openportRun=getOpenportRunNumber();
  short numWrapBytes=0;
  float timeDiff;
  int retVal=0;
  if(first) {
    gettimeofday(&lastTime,0);
    first=0;
  }

  
  unsigned short *openportBuffer=openport_wrap_buffer(buf,nbytes,&numWrapBytes);

  //  const int maxBytesToFile=1000000;
  
  if(!openedFile) {
    sprintf(fileName,"%s/%05d",OPENPORT_STAGE_DIR,openportRun);
    makeDirectories(fileName);
    sprintf(currentOpenportDir,"%s/%05d",OPENPORT_OUTPUT_DIR,openportRun);
    makeDirectories(currentOpenportDir);
    sprintf(fileName,"%s/%05d/%06d",OPENPORT_STAGE_DIR,openportRun,getOpenportFileNumber());
    printf("Openport file: %s\n",fileName);
    fp=fopen(fileName,"wb");
    openedFile=1;
  }

  if(fp) {
    fwrite(openportBuffer,numWrapBytes,1,fp);
    bytesToFile+=numWrapBytes;


    //Now some accounting
    dataCounter+=nbytes;
    if(isHk) hkCounter+=nbytes;
    else eventCounter+=nbytes;
    printf("openportWrite %d bytes, file %d, hk %d, event %d\n",nbytes,bytesToFile,hkCounter,eventCounter);

    if(bytesToFile>maxFileSize) {
      printf("Finished file %s -- with %d bytes\n",fileName,bytesToFile);
      fclose(fp);
      //      zipFileInPlace(fileName);
      moveFile(fileName,currentOpenportDir);
      fp=NULL;
      openedFile=0;
      bytesToFile=0;

   
      gettimeofday(&newTime,0);
      timeDiff=getTimeDiff(lastTime,newTime);
      printf("Transferred %u bytes (%u hk, %u event) in %2.2f seconds (%3.4f bytes/sec)\n",dataCounter,hkCounter,eventCounter,timeDiff,((float)dataCounter)/timeDiff);
      //      sleep(5);
      dataCounter=0;
      hkCounter=0;
      eventCounter=0;
      lastTime=newTime;
    
    }
  }
  
  return retVal;
}


float getTimeDiff(struct timeval oldTime, struct timeval currentTime) {
  float timeDiff=currentTime.tv_sec-oldTime.tv_sec;
  timeDiff+=1e-6*(float)(currentTime.tv_usec-oldTime.tv_usec);
  return timeDiff;
}

void refreshLinksHandler(int sig)
{
  syslog(LOG_INFO,"Got signal %d, so will refresh links",sig);
  needToRefreshLinks=1;
}

int getOpenportFileNumber() {
  static int openportNumber=-1;
  openportNumber++;
  return openportNumber;
}

int getOpenportRunNumber() {
  int retVal=0;
  static int firstTime=1;
  static int openportNumber=0;
  /* This is just to get the lastOpenportNumber in case of program restart. */
  FILE *pFile;
  if(firstTime && !standAloneMode) {
    pFile = fopen (LAST_OPENPORT_RUN_NUMBER_FILE, "r");
    if(pFile == NULL) {
      syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
	      LAST_OPENPORT_RUN_NUMBER_FILE);
    }
    else {	    	    
      retVal=fscanf(pFile,"%d",&openportNumber);
      if(retVal<0) {
	syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
		LAST_OPENPORT_RUN_NUMBER_FILE);
	fprintf(stderr,"fscanff: %s ---  %s\n",strerror(errno),
		LAST_OPENPORT_RUN_NUMBER_FILE);
      }
      fclose (pFile);
    }
    //    openportNumber++;
    if(printToScreen) printf("The last openport run number is %d\n",openportNumber);
    firstTime=0;
  }
  openportNumber++;
  if(openportNumber>0) {
    pFile = fopen (LAST_OPENPORT_RUN_NUMBER_FILE, "w");
    if(pFile == NULL) {
      syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
	      LAST_OPENPORT_RUN_NUMBER_FILE);
      fprintf(stderr,"fopen: %s ---  %s\n",strerror(errno),
	      LAST_OPENPORT_RUN_NUMBER_FILE);
    }
    else {
      retVal=fprintf(pFile,"%d\n",openportNumber);
      if(retVal<0) {
	syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		LAST_OPENPORT_RUN_NUMBER_FILE);
	fprintf(stderr,"fprintf: %s ---  %s\n",strerror(errno),
		LAST_OPENPORT_RUN_NUMBER_FILE);
      }
      fclose (pFile);
    }
  }
  return openportNumber;
}

void startCopyScript() {

  copyScriptPid = fork();
  if (copyScriptPid > 0) {
    
  }
  else if (copyScriptPid == 0){
    system("/home/anita/flightSoft/bin/openportCopyScript.sh");
    printf("Finished copy script\n");
    exit(0);
  }

}
