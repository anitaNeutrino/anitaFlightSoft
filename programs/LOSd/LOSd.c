/* LOSd - Program that sends data ovwer the LOS link.
 *
 * Ryan Nichol, August '05
 * Started off as a modified version of Marty's driver program.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include <fcntl.h>
#include <zlib.h>
#include <sys/time.h>
#include <libgen.h> //For Mac OS X
#include <poll.h>

/* Flight soft includes */
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "linkWatchLib/linkWatchLib.h"
#include "includes/anitaStructures.h"
#include "losLib/telemwrap.h"

//#define SEND_TEST_PACKET 1
#define LOS_MAX_BYTES 8000
#define TIMEOUT_IN_MILLISECONDS 1000
#define REFRESH_LINKS_EVERY 2000

#ifdef FAKE_LOS
#define LOS_DEVICE "/tmp/dev_los"
#else
#define LOS_DEVICE "/dev/los"
#endif

#define NUM_HK_TELEM_DIRS 23 

typedef enum {
  LOS_TELEM_FIRST=0,
  LOS_TELEM_CMD_ECHO=0,
  LOS_TELEM_MONITOR,
  LOS_TELEM_HEADER,
  LOS_TELEM_HK,
  LOS_TELEM_ADU5A_SAT,
  LOS_TELEM_ADU5B_SAT,
  LOS_TELEM_G12_SAT,
  LOS_TELEM_ADU5A_PAT,
  LOS_TELEM_ADU5B_PAT,
  LOS_TELEM_G12_POS,
  LOS_TELEM_GPU,
  LOS_TELEM_ADU5A_VTG,
  LOS_TELEM_ADU5B_VTG,
  LOS_TELEM_G12_GGA,
  LOS_TELEM_ADU5A_GGA,
  LOS_TELEM_ADU5B_GGA,
  LOS_TELEM_SURFHK,
  LOS_TELEM_TURFRATE,
  LOS_TELEM_OTHER,
  LOS_TELEM_PEDESTAL,
  LOS_TELEM_REQUEST,
  LOS_TELEM_RTL, 
  LOS_TELEM_TUFF, 
  LOS_TELEM_NOT_A_TELEM
} LOSTelemType_t;

int initDevice();
void sendWakeUpBuffer();
 //void tryToSendData();
 //int bufferAndWrite(unsigned char *buf, short nbytes,int doWrite);
 //int fillBufferWithPackets();
 int doWrite();
 int readConfig();
 int checkLinkDir(int maxCopy, char *telemDir, char *linkDir, int fileSize);
int addToTelemetryBuffer(int maxCopy, int wd, char *telemDir, char *linkDir, int fileSize, int numToLeave);
 void fillBufferWithHk();
 void readAndSendEvent(char *headerFilename, unsigned int eventNumber);
 void readAndSendEventRamdisk(char *headerFilename);
 float getTimeDiff(struct timeval oldTime, struct timeval currentTime);
 unsigned int getLosNumber();
 int sendRawWaveformPackets(int bufSize);
 int sendPedSubbedWaveformPackets(int bufSize);
 int sendEncodedPedSubbedWavePackets(int bufSize);
 int sendEncodedPedSubbedSurfPackets(int bufSize);
 int sendEncodedSurfPackets(int bufSize);
 int sendRawSurfPackets(int bufSize);
 int sendPedSubbedSurfPackets(int bufSize);
 int sortOutPidFile(char *progName);
 int writeLosData(unsigned char *buffer, int numBytes);

 /* Signal Handle */
 void handleBadSigs(int sig);

 char fakeOutputDir[]="/tmp/fake/los";

// static struct timespec last_send; 


 //Packet Dirs
 char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];

 //Packet Link Dirs
 char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];

 int losBus;
 int losSlot;
 int verbosity;
 int printToScreen;
 int laptopDebug;
 int needToReload=0;

 // Bandwidth variables
 int eventBandwidth=80;
 int priorityBandwidths[NUM_PRIORITIES];
 int eventCounters[NUM_PRIORITIES]={0};
 int priorityOrder[NUM_PRIORITIES*100];
 int maxEventsBetweenLists;
 int numOrders=1000;
 int orderIndex=0;
 int currentPri=0;
 static float minTimeWait = 0.; 
 static float minTimeWait_b= 0.005; 
 static float minTimeWait_m = 1e-5; 

 /*Global Variables*/
 unsigned char eventBuffer[1000+MAX_EVENT_SIZE];
 unsigned char losBuffer[1000+LOS_MAX_BYTES];
 unsigned char wrappedBuffer[1000+LOS_MAX_BYTES];
 int numBytesInBuffer=0;
 int sendData=0;
 int sendWavePackets=0;
 int fdLos=0;



static  volatile unsigned long long nwrites_before = 0; 
static  volatile unsigned long long nwrites_after = 0; 

/** Watchdog for LOS, 
 *
 * Mostly it sleeps, but it tries to check the condition that we have something to write
 * but it takes too long 
 *
 * I'm not convinced there are no race conditions here, but worst case LOSd just restarts. 
 */ 
void * watchdogThread(void * unused) 
{
  while(currentState != PROG_STATE_TERMINATE )
  {

    //See what the number of writes is 
    //nwrites_before should only differ from n_writes_after during a write
    //

    int after = nwrites_after; 
    int before = nwrites_before; 
    
    if (after == before)
    {
      sleep(1); 
      continue; 
    }


    //wait a while
    sleep(1); 

    //Check if we're still in the same state after 1 second. 
    if (nwrites_before == before && after == nwrites_after) 
    {
      syslog(LOG_ERR, "Took too long to write, LOSd self-terminating\n"); 
      raise(SIGTERM);  // TODO: which signal should I send? 
    }
  }
  return 0; 
}


 #define MAX_ATTEMPTS 50

 int main(int argc, char *argv[])
 {
   int pri,retVal,firstTime=1;
   time_t lastRefresh=0;
   time_t lastUpdate=0;
   time_t currentTime=0;
   char *tempString=0;
   char *progName=basename(argv[0]);

//   last_send.tv_sec = 0; 
//   last_send.tv_nsec = 0; 

   //Sort out PID File
   retVal=sortOutPidFile(progName);
   if(retVal!=0) {
     return retVal;
   }


   //Directory listing tools
   //    char currentTouchname[FILENAME_MAX];
   char currentHeader[FILENAME_MAX];
   int numLinks[NUM_PRIORITIES]={0};
   int totalEventLinks=0;
   int wdEvents[NUM_PRIORITIES]={0};
   //   int sillyEvNum[NUM_PRIORITIES]={0};


   /* Set signal handlers */
   signal(SIGUSR1, sigUsr1Handler);
   signal(SIGUSR2, sigUsr2Handler);
   signal(SIGTERM, handleBadSigs);
   signal(SIGINT,handleBadSigs);
   signal(SIGSEGV,handleBadSigs);


   /* Setup log */
   setlogmask(LOG_UPTO(LOG_INFO));
   openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

   // Load Config 
   retVal=readConfig();
   if(!laptopDebug) {
     retVal=initDevice();
     if(retVal!=0) {
       return 1;
     }
   }
   else {
     retVal=0;
     printf("Running in debug mode not actually trying to talk to device\n");
     makeDirectories(fakeOutputDir);
   }

   makeDirectories(PRIORITIZERD_EVENT_LINK_DIR);
   makeDirectories(HEADER_TELEM_LINK_DIR);
   makeDirectories(SURFHK_TELEM_LINK_DIR);
   makeDirectories(TURFHK_TELEM_LINK_DIR);
   makeDirectories(PEDESTAL_TELEM_LINK_DIR);
   makeDirectories(HK_TELEM_LINK_DIR);
   makeDirectories(MONITOR_TELEM_LINK_DIR);
   makeDirectories(OTHER_MONITOR_TELEM_LINK_DIR);
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
   makeDirectories(GPU_TELEM_LINK_DIR);
   makeDirectories(RTL_TELEM_LINK_DIR);
   makeDirectories(TUFF_TELEM_LINK_DIR);
   makeDirectories(LOSD_CMD_ECHO_TELEM_LINK_DIR);
   makeDirectories(REQUEST_TELEM_LINK_DIR);


   //Fill event dir names
   for(pri=0;pri<NUM_PRIORITIES;pri++) {
     sprintf(eventTelemDirs[pri],"%s/%s%d",BASE_EVENT_TELEM_DIR,
	     EVENT_PRI_PREFIX,pri);
     sprintf(eventTelemLinkDirs[pri],"%s/link",eventTelemDirs[pri]);
     makeDirectories(eventTelemLinkDirs[pri]);


     if(sendData) {
	 //And setup inotify watches
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
   }
   time(&lastRefresh);

   pthread_t thread; 
   pthread_create(&thread, 0, watchdogThread,0); 

   do {
     if(verbosity) printf("Initializing LOSd\n");
     int lastSendData=sendData;
     retVal=readConfig();
     if(lastSendData!=sendData) {
	 break; //Lets restart LOSd
     }
     if(firstTime) {
       sendWakeUpBuffer();
       firstTime=0;
     }
     int hkCount=0;    	
     currentState=PROG_STATE_RUN;
     while(currentState==PROG_STATE_RUN) {
       //This is just if LOSd has been turned off when we're out of range
       if(!sendData) {
	 sleep(1);
	 continue;
       }
       
       //Check to see if we need to refresh the links
       time(&currentTime);
       if(currentTime>lastRefresh+REFRESH_LINKS_EVERY) {
	   refreshLinkDirs();
	   lastRefresh=currentTime;
       }

       //Work which priority we're doing
       currentPri=priorityOrder[orderIndex];

       //Will actually change this to almost work in the future
       if(hkCount%5==0) fillBufferWithHk();
       hkCount++;

       //Check the link dirs
       if(currentTime>lastUpdate+10 || totalEventLinks<1) {
	 if(totalEventLinks<1) 
	   checkLinkDirs(1,0);
	 else
	   checkLinkDirs(0,1); //Maybe I don't need to do this every time
	 lastUpdate=currentTime;
       }

       totalEventLinks=0;
       for(pri=0;pri<NUM_PRIORITIES;pri++) {
	 numLinks[pri]=getNumLinks(wdEvents[pri]);
	 totalEventLinks+=numLinks[pri];
       }

       if(printToScreen && verbosity>1) {
	 printf("Got %d links in %s\n",numLinks[currentPri],
		eventTelemLinkDirs[currentPri]);
       }
       //       sillyEvNum[currentPri]=0;

       if(numLinks[currentPri]>0) {
	 //Got an event

	 tempString=getLastLink(wdEvents[currentPri]);	
	 sprintf(currentHeader,"%s/%s",eventTelemLinkDirs[currentPri],
		 tempString);
	 if(printToScreen && verbosity)
	   printf("Starting %s\n",currentHeader);
	 readAndSendEventRamdisk(currentHeader); //Also deletes
	 numLinks[currentPri]--;
	 totalEventLinks--;	       
       }

       //Again I need to improve the bandwidth sharing between data and hk
       fillBufferWithHk();
//       if(totalEventLinks<1) usleep(100);
       //	    printf("totalEventLinks %d\n",totalEventLinks);
       orderIndex++;
       if(orderIndex>=numOrders) orderIndex=0;
     }
   } while(currentState==PROG_STATE_INIT);
   unlink(LOSD_PID_FILE);
   syslog(LOG_INFO,"LOSd terminating");
   //    fprintf(stderr, "Bye bye\n");
   return 0;
 } 


 int initDevice() {
   int retVal=0;
   if(fdLos==0) {
     //First time
     fprintf(stderr,"opening %s\n",LOS_DEVICE);
     fdLos = open(LOS_DEVICE, O_WRONLY );
     if(fdLos<=0) {
       fprintf(stderr,"Error opening %s (%s):\n",LOS_DEVICE,strerror(errno));
       syslog(LOG_ERR,"Error opening %s (%s):\n",LOS_DEVICE,strerror(errno));
       handleBadSigs(0);
     }
     telemwrap_init(TW_LOS);
   }
   return retVal;
 }

 int doWrite() {
   static unsigned int dataCounter=0;
   static int first=1;
   static struct timeval lastTime;
   struct timeval newTime;
   float timeDiff;
   int retVal;
   if(first) {
     gettimeofday(&lastTime,0);
     first=0;
   }
   //    if(numBytesInBuffer%2==1) numBytesInBuffer++;
   if(numBytesInBuffer>6000)
     printf("Trying to write %d bytes\ -- %d\n",numBytesInBuffer,laptopDebug);
  if(!laptopDebug) {
    retVal=writeLosData(losBuffer,numBytesInBuffer);
    if(retVal!=0) {
      syslog(LOG_ERR,"Error sending los buffer\n");
      fprintf(stderr,"Error sending los buffer\n");
    }
  }
  else {
    retVal=0;
    //	retVal=fake_los_write(losBuffer,numBytesInBuffer,fakeOutputDir);
  }	

  dataCounter+=numBytesInBuffer;
  //    printf("%d %d\n",numBytesInBuffer,dataCounter);
  if(dataCounter>100000) {
    gettimeofday(&newTime,0);
    timeDiff=getTimeDiff(lastTime,newTime);
    printf("Transferred %u bytes in %2.2f seconds (%3.4f bytes/sec)\n",dataCounter,timeDiff,((float)dataCounter)/timeDiff);
    dataCounter=0;
    lastTime=newTime;
  }

  numBytesInBuffer=0;
  return retVal;    
}

float getTimeDiff(struct timeval oldTime, struct timeval currentTime) {
  float timeDiff=currentTime.tv_sec-oldTime.tv_sec;
  timeDiff+=1e-6*(float)(currentTime.tv_usec-oldTime.tv_usec);
  return timeDiff;
}
	

int readConfig()
// Load LOSd config stuff
{
  // Config file thingies
  KvpErrorCode kvpStatus=0;
  int status=0,tempNum,count,maxVal,maxIndex;
  char* eString ;
  kvpReset();
  status = configLoad ("LOSd.config","output") ;
  if(status == CONFIG_E_OK) {
    printToScreen=kvpGetInt("printToScreen",-1);
    verbosity=kvpGetInt("verbosity",-1);
    if(printToScreen<0) {
      syslog(LOG_WARNING,
	     "Couldn't fetch printToScreen, defaulting to zero");
      printToScreen=0;
    }
  }
  else {
    eString=configErrorString (status) ;
    syslog(LOG_ERR,"Error reading LOSd.config: %s\n",eString);
    fprintf(stderr,"Error reading LOSd.config: %s\n",eString);
  }
  kvpReset();
  status = configLoad ("LOSd.config","losd");
  if(status == CONFIG_E_OK) {
    sendData=kvpGetInt("sendData",0);
    sendWavePackets=kvpGetInt("sendWavePackets",0);
    maxEventsBetweenLists=kvpGetInt("maxEventsBetweenLists",100);
    minTimeWait_b = kvpGetFloat("minTimeWait_b", 0.005); 
    minTimeWait_m = kvpGetFloat("minTimeWait_m", 1e-5); 
    laptopDebug=kvpGetInt("laptopDebug",0);
    losBus=kvpGetInt("losBus",1);
    losSlot=kvpGetInt("losSlot",1);
  }
  else {
    eString=configErrorString (status) ;
    syslog(LOG_ERR,"Error reading LOSd.config: %s\n",eString);
    fprintf(stderr,"Error reading LOSd.config: %s\n",eString);
  }
  kvpReset();
  status = configLoad ("LOSd.config","bandwidth") ;
  if(status == CONFIG_E_OK) {
    eventBandwidth=kvpGetInt("eventBandwidth",80);
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
    syslog(LOG_ERR,"Error reading LOSd.config: %s\n",eString);
    fprintf(stderr,"Error reading LOSd.config: %s\n",eString);
  }

  return status;
}

int checkLinkDir(int maxCopy, char *telemDir, char *linkDir, int fileSize)
/* Looks in the specified directroy and fill buffer upto maxCopy. */
/* fileSize is the maximum size of a packet in the directory */
{
  char currentFilename[FILENAME_MAX];
  char currentLinkname[FILENAME_MAX];
  char currentTouchname[FILENAME_MAX];
  char currentLOSTouchname[FILENAME_MAX];
  int retVal,numLinks,count,numBytes,totalBytes=0;//,checkVal=0;
  GenericHeader_t *gHdr;
  struct dirent **linkList;

  if((numBytesInBuffer+fileSize)>LOS_MAX_BYTES) return 0;


  numLinks=getListofLinks(linkDir,&linkList); 
  if(numLinks<=1) {
    return 0;
  }
        
  for(count=numLinks-1;count>=1;count--) {
    sprintf(currentFilename,"%s/%s",telemDir,
	    linkList[count]->d_name);
    sprintf(currentTouchname,"%s.sipd",currentFilename);
    sprintf(currentLOSTouchname,"%s.losd",currentFilename);
    sprintf(currentLinkname,"%s/%s",
	    linkDir,linkList[count]->d_name);

    if(checkFileExists(currentTouchname)) continue;
    touchFile(currentLOSTouchname);


    retVal=genericReadOfFile((unsigned char*)&(losBuffer[numBytesInBuffer]),
			     currentFilename,
			     LOS_MAX_BYTES-numBytesInBuffer);
    if(retVal<=0) {
      //	    syslog(LOG_ERR,"Error opening file, will delete: %s",
      //		   currentFilename);
      //	    fprintf(stderr,"Error reading file %s -- %d\n",currentFilename,retVal);
      removeFile(currentFilename);
      removeFile(currentLOSTouchname);
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
    gHdr = (GenericHeader_t*) (&(losBuffer[numBytesInBuffer]));
    //	checkVal=checkPacket(gHdr);
    //	if(checkVal!=0 ) {
    //	    printf("Bad packet %s == %d\n",currentFilename,checkVal);
    //	}
    gHdr->packetNumber=getLosNumber();
    numBytesInBuffer+=numBytes;
    totalBytes+=numBytes;

    if(!checkFileExists(currentTouchname)) {
      removeFile(currentLinkname);
      removeFile(currentFilename);
      removeFile(currentLOSTouchname);
    }
    else {
      printf("%s exists\n",currentTouchname);
    }

    if((totalBytes+fileSize)>maxCopy ||
       (numBytesInBuffer+fileSize)>LOS_MAX_BYTES) break;
  }
    
  for(count=0;count<numLinks;count++)
    free(linkList[count]);
  free(linkList);
    
  return totalBytes;
}



int addToTelemetryBuffer(int maxCopy, int wd, char *telemDir, char *linkDir, int fileSize,int numToLeave)
/* Uses the inotify watch specified by wd to look */
/* in the specified directroy and fill buffer upto maxCopy. */
/* fileSize is the maximum size of a packet in the directory */
{
  char currentFilename[FILENAME_MAX];
  char currentLinkname[FILENAME_MAX];
  char currentTouchname[FILENAME_MAX];
  char currentLOSTouchname[FILENAME_MAX];
  int retVal,numLinks,count,numBytes,totalBytes=0;//,checkVal=0;
  GenericHeader_t *gHdr;
  char *tempString;
  //  printf("%s %s == %d %d %d\n",telemDir,linkDir,fileSize,numBytesInBuffer,LOS_MAX_BYTES);

  //Both of these two checks should be uneccesary
  if((numBytesInBuffer+fileSize)>LOS_MAX_BYTES) return 0;
  numLinks=getNumLinks(wd);
  if(numLinks<=numToLeave) {
    return 0;
  }
  
  for(count=numLinks-1;count>=numToLeave;count--) {
    tempString=getLastLink(wd);
    sprintf(currentFilename,"%s/%s",telemDir,tempString);
    sprintf(currentTouchname,"%s.sipd",currentFilename);
    sprintf(currentLOSTouchname,"%s.losd",currentFilename);
    sprintf(currentLinkname,"%s/%s",linkDir,tempString);

    if(!checkFileExists(currentLinkname))
      continue;

    if(checkFileExists(currentTouchname)) continue;
    touchFile(currentLOSTouchname);


    retVal=genericReadOfFile((unsigned char*)&(losBuffer[numBytesInBuffer]),
			     currentFilename,
			     LOS_MAX_BYTES-numBytesInBuffer);
    if(retVal<=0) {
      //	    syslog(LOG_ERR,"Error opening file, will delete: %s",
      //		   currentFilename);
      //	    fprintf(stderr,"Error reading file %s -- %d\n",currentFilename,retVal);
      removeFile(currentFilename);
      removeFile(currentLOSTouchname);
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
    gHdr = (GenericHeader_t*) (&(losBuffer[numBytesInBuffer]));
    //	checkVal=checkPacket(gHdr);
    //	if(checkVal!=0 ) {
    //	    printf("Bad packet %s == %d\n",currentFilename,checkVal);
    //	}
    gHdr->packetNumber=getLosNumber();
    numBytesInBuffer+=numBytes;
    totalBytes+=numBytes;

    if(!checkFileExists(currentTouchname)) {
      removeFile(currentLinkname);
      removeFile(currentFilename);
      removeFile(currentLOSTouchname);
    }
    else {
      printf("%s exists\n",currentTouchname);
    }

    if((totalBytes+fileSize)>maxCopy ||
       (numBytesInBuffer+fileSize)>LOS_MAX_BYTES) break;
  }
        
  return totalBytes;
}


void sendWakeUpBuffer() 
{
  GenericHeader_t *gHdr = (GenericHeader_t*) &losBuffer[0];
  int count;
  for(count=sizeof(GenericHeader_t);count<WAKEUP_LOS_BUFFER_SIZE;count++) {
    losBuffer[count]=0xfe;
  }
  fillGenericHeader(gHdr,PACKET_WAKEUP_LOS,WAKEUP_LOS_BUFFER_SIZE);
  gHdr->packetNumber=getLosNumber();
  numBytesInBuffer=WAKEUP_LOS_BUFFER_SIZE;
  doWrite();
    
}

void fillBufferWithHk() 
{
  //  printf("fillBufferWithHk\n");
  //Setup inotify watches
  //Get the various number of links
  //Send some data down
    
  static int wdHks[NUM_HK_TELEM_DIRS]={0};
  static int numHkLinks[NUM_HK_TELEM_DIRS]={0};
  //May have to change this to be more robust and actually use a fixed string sizes, but will try this for now
  static char *telemLinkDirs[NUM_HK_TELEM_DIRS]=
    {LOSD_CMD_ECHO_TELEM_LINK_DIR,
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
     OTHER_MONITOR_TELEM_LINK_DIR,
     PEDESTAL_TELEM_LINK_DIR,
     REQUEST_TELEM_LINK_DIR, 
     RTL_TELEM_LINK_DIR, 
     TUFF_TELEM_LINK_DIR
    };
  static char *telemDirs[NUM_HK_TELEM_DIRS]=
    {LOSD_CMD_ECHO_TELEM_DIR,
     MONITOR_TELEM_DIR,
     HEADER_TELEM_DIR,
     HK_TELEM_DIR,
     ADU5A_SAT_TELEM_DIR,
     ADU5B_SAT_TELEM_DIR,
     G12_SAT_TELEM_DIR,
     ADU5A_PAT_TELEM_DIR,
     ADU5B_PAT_TELEM_DIR,
     G12_POS_TELEM_DIR,
     GPU_TELEM_DIR,
     ADU5A_VTG_TELEM_DIR,
     ADU5B_VTG_TELEM_DIR,
     G12_GGA_TELEM_DIR,
     ADU5A_GGA_TELEM_DIR,
     ADU5B_GGA_TELEM_DIR,
     SURFHK_TELEM_DIR,
     TURFHK_TELEM_DIR,
     OTHER_MONITOR_TELEM_DIR,
     PEDESTAL_TELEM_DIR,
     REQUEST_TELEM_DIR, 
     RTL_TELEM_DIR, 
     TUFF_TELEM_DIR
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
     sizeof(OtherMonitorStruct_t),
     sizeof(FullLabChipPedStruct_t),
     4000, //Who knows why, 
     sizeof(RtlSdrPowerSpectraStruct_t) , 
     sizeof(TuffNotchStatus_t)
    };
  int hkInd=0;

  
  //Setup inotify watches
  if(wdHks[0]==0) {    
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
  }

  //Update the link lists -- well don't for now
  //  checkLinkDirs(0,1);


  //Now try and send some hk data
  for(hkInd=0;hkInd<NUM_HK_TELEM_DIRS;hkInd++) {
    int numToLeave=1;
    if(hkInd==0) numToLeave=0;
    //Make sure we clear the buffer to leave room for requests
    if(hkInd==LOS_TELEM_REQUEST && numBytesInBuffer>1000)
      doWrite();
    
    numHkLinks[hkInd]=getNumLinks(wdHks[hkInd]);
    //    printf("numHkLinks[%d] %d\n",hkInd,numHkLinks[hkInd]);
    if(numHkLinks[hkInd]>0) {
      //Have stuff to send
      if((LOS_MAX_BYTES-numBytesInBuffer)>maxPacketSize[hkInd]) {
	addToTelemetryBuffer(LOS_MAX_BYTES-numBytesInBuffer,wdHks[hkInd],
			     telemDirs[hkInd],telemLinkDirs[hkInd],
			     maxPacketSize[hkInd],numToLeave);
      }      
    }
  }
   
  if(numBytesInBuffer>1000)
    doWrite();
  if(numBytesInBuffer<0) {
    printf("What the heck does %d bytes mean\n",numBytesInBuffer);
  }
}


void readAndSendEvent(char *headerFilename, unsigned int eventNumber) {
  AnitaEventHeader_t *theHeader;
  EncodedSurfPacketHeader_t *surfHdPtr;
  int numBytes,count=0,surf=0,retVal;//,useGzip=0;
  char waveFilename[FILENAME_MAX];

  if(!checkFileExists(headerFilename))
    return;

  //    FILE *eventFile;
  //    gzFile gzippedEventFile;

  //    if(printToScreen) 
    
  //Now remember these files actually contain upto 100 events or headers

  //First check if there is room for the header
  if((LOS_MAX_BYTES-numBytesInBuffer)<sizeof(AnitaEventHeader_t))
    doWrite();

  

  //     Next load header 
  theHeader=(AnitaEventHeader_t*) &losBuffer[numBytesInBuffer]; 
  retVal=fillHeaderWithThisEvent(theHeader,headerFilename,eventNumber); 
  theHeader->gHdr.packetNumber=getLosNumber();
  numBytesInBuffer+=sizeof(AnitaEventHeader_t);
     
  if(eventNumber!=theHeader->eventNumber) {
    printf("Trying event %u file %s\n",eventNumber,headerFilename);
    printf("Have event %u (wanted %u) -- retVal %d\n",
	   theHeader->eventNumber,
	   eventNumber,retVal);
  }
	 
	 
     
  if(retVal<0) {
    removeFile(headerFilename);
    sprintf(waveFilename,"%s/ev_%d.dat",eventTelemDirs[currentPri], 
	    theHeader->eventNumber);
    removeFile(waveFilename);
	 
    //Bollock
    return;
  }

  //Now get event file
  sprintf(waveFilename,"%s/ev_%d.dat",eventTelemDirs[currentPri], 
	  eventNumber);
  numBytes=readEncodedEventFromFile(eventBuffer,waveFilename,
				    eventNumber);

     
  //     printf("Read %d bytes of event %u from file %s\n",
  //	    numBytes,eventNumber,waveFilename);
	   
  //     exit(0);

  /*     //Now get event file */
  /*     sprintf(waveFilename,"%s/ev_%d.dat",eventTelemDirs[currentPri], */
  /* 	    theHeader->eventNumber); */
  /*     eventFile = fopen(waveFilename,"r"); */
  /*     if(!eventFile) { */
  /* 	sprintf(waveFilename,"%s/ev_%d.dat.gz",eventTelemDirs[currentPri], */
  /* 		theHeader->eventNumber); */
  /* 	gzippedEventFile = gzopen(waveFilename,"rb"); */
  /* 	if(!gzippedEventFile) { */
  /* 	    syslog(LOG_ERR,"Couldn't open %s -- %s",waveFilename,strerror(errno)); */
  /* 	    fprintf(stderr,"Couldn't open %s -- %s\n",waveFilename,strerror(errno));	 */
  /* 	    removeFile(headerFilename); */
  /* 	    removeFile(waveFilename); */
  /* 	    return; */
  /* 	} */
  /* 	useGzip=1; */
  /*     } */

  /*     if(useGzip) { */
  /* 	numBytes = gzread(gzippedEventFile,eventBuffer,MAX_EVENT_SIZE); */
  /* 	gzclose(gzippedEventFile);	 */
  /*     } */
  /*     else { */
  /* 	numBytes = fread(eventBuffer,1,MAX_EVENT_SIZE,eventFile); */
  /* 	fclose(eventFile); */
  /*     } */
    

  count+=sizeof(EncodedEventWrapper_t);
  // Remember what the file contains is actually 9 EncodedSurfPacketHeader_t's
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    surfHdPtr = (EncodedSurfPacketHeader_t*) &eventBuffer[count];
    surfHdPtr->gHdr.packetNumber=getLosNumber();
    numBytes = surfHdPtr->gHdr.numBytes;
    if(numBytes) {
      if((LOS_MAX_BYTES-numBytesInBuffer)<numBytes)
	fillBufferWithHk();
      memcpy(&losBuffer[numBytesInBuffer],surfHdPtr,numBytes);
      count+=numBytes;
      numBytesInBuffer+=numBytes;
    }
    else break;
  }
    
  if(printToScreen && verbosity>1) 
    printf("Removing files %s\t%s\n",headerFilename,waveFilename);
	        
  removeFile(headerFilename);
  removeFile(waveFilename);

}


void readAndSendEventRamdisk(char *headerLinkFilename) {
  static int errorCounter=0;
  AnitaEventHeader_t *theHeader;
  GenericHeader_t *gHdr;
  int retVal;
  char waveFilename[FILENAME_MAX];
  char headerFilename[FILENAME_MAX];
  char currentTouchname[FILENAME_MAX];
  char currentLOSTouchname[FILENAME_MAX];
  char justFile[FILENAME_MAX];
  unsigned int thisEventNumber;
    
  sprintf(justFile,"%s",basename(headerLinkFilename));
  sscanf(justFile,"hd_%u.dat",&thisEventNumber);
  sprintf(headerFilename,"%s/hd_%d.dat",eventTelemDirs[currentPri], 
	  thisEventNumber);
  sprintf(waveFilename,"%s/psev_%d.dat",eventTelemDirs[currentPri], 
	  thisEventNumber);
    
  sprintf(currentTouchname,"%s.sipd",headerFilename);
  sprintf(currentLOSTouchname,"%s.losd",headerFilename);

  if(checkFileExists(currentTouchname)) 
    return;
  touchFile(currentLOSTouchname);
  //Removing headerLinkFilename
  //  fprintf(stderr,"Removing %s\n",headerLinkFilename);
  removeFile(headerLinkFilename);



  //First check if there is room for the header
  if((LOS_MAX_BYTES-numBytesInBuffer)<sizeof(AnitaEventHeader_t))
    doWrite();

    
  //     Next load header 
  theHeader=(AnitaEventHeader_t*) &losBuffer[numBytesInBuffer]; 
  retVal=fillHeader(theHeader,headerFilename); 
  theHeader->gHdr.packetNumber=getLosNumber();
  numBytesInBuffer+=sizeof(AnitaEventHeader_t);

    
  if(retVal<0) {
    removeFile(headerFilename);
    removeFile(waveFilename);
    removeFile(currentLOSTouchname);
    //Bollocks
    return;
  }
    
  thisEventNumber=theHeader->eventNumber;
    

  //Now get event file
  sprintf(headerFilename,"%s/hd_%d.dat",eventTelemDirs[currentPri], 
	  thisEventNumber);
  sprintf(waveFilename,"%s/ev_%d.dat",eventTelemDirs[currentPri], 
	  thisEventNumber);


  retVal=genericReadOfFile(eventBuffer,waveFilename,MAX_EVENT_SIZE);
  if(retVal<0) {
    fprintf(stderr,"Problem reading %s\n",waveFilename);
    removeFile(headerFilename);
    removeFile(waveFilename);
    removeFile(currentLOSTouchname);
	
    //Bollocks
    return;
  }


  //Okay so now the buffer either contains EncodedSurfPacketHeader_t or
  // it contains EncodedPedSubbedSurfPacketHeader_t
  gHdr = (GenericHeader_t*) &eventBuffer[0];
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
    if(sendWavePackets)
      retVal=sendEncodedPedSubbedWavePackets(retVal);
    else 
      retVal=sendEncodedPedSubbedSurfPackets(retVal);
    break;
  default:
    if(errorCounter<100) {
      syslog(LOG_ERR,"Don't know what to do with packet %d -- %s (Message %d of 100)\n",gHdr->code,packetCodeAsString(gHdr->code),errorCounter);
      errorCounter++;
    }
    fprintf(stderr,"Don't know what to do with packet %d -- %s (file %s, size %d)\n",gHdr->code,packetCodeAsString(gHdr->code),waveFilename,retVal);
  }
	
  if(printToScreen && verbosity>1) 
    printf("Removing files %s\t%s\n",headerFilename,waveFilename);

  if(!checkFileExists(currentTouchname)) {
    removeFile(headerFilename);
    removeFile(waveFilename);
    removeFile(currentLOSTouchname);
  }
  else {
    printf("Not removing %s because checkFileExists == %d\n",
	   headerFilename,checkFileExists(currentTouchname));
  }
    
}


int sendEncodedSurfPackets(int bufSize)
{
  EncodedSurfPacketHeader_t *surfHdPtr;
  int numBytes,count=0,surf=0;

  // Remember what the file contains is actually 9 EncodedSurfPacketHeader_t's
  count+=sizeof(EncodedEventWrapper_t);


  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    surfHdPtr = (EncodedSurfPacketHeader_t*) &eventBuffer[count];
    surfHdPtr->gHdr.packetNumber=getLosNumber();
    numBytes = surfHdPtr->gHdr.numBytes;
    if(numBytes) {
      if((LOS_MAX_BYTES-numBytesInBuffer)<numBytes) {
	//		fillBufferWithHk();
	doWrite();
      }
      memcpy(&losBuffer[numBytesInBuffer],surfHdPtr,numBytes);
      count+=numBytes;
      numBytesInBuffer+=numBytes;
    }
    else break;
    if(count>bufSize) return -1;
  }
  return 0;
}


int sendEncodedPedSubbedSurfPackets(int bufSize)
{
  EncodedPedSubbedSurfPacketHeader_t *surfHdPtr;
  int numBytes,count=0,surf=0,chan,count2;;
  EncodedSurfChannelHeader_t *chanHdPtr;

  count+=sizeof(EncodedEventWrapper_t);

  // Remember what the file contains is actually 9 EncodedPedSubbedSurfPacketHeader_t's
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    surfHdPtr = (EncodedPedSubbedSurfPacketHeader_t*) &eventBuffer[count];
    surfHdPtr->gHdr.packetNumber=getLosNumber();
    numBytes = surfHdPtr->gHdr.numBytes;
    //	printf("surfHdPtr.eventNumber %u, numBytes %d (%d)\n",surfHdPtr->eventNumber,numBytes,numBytesInBuffer);
    count2=count+sizeof(EncodedPedSubbedSurfPacketHeader_t);
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
      chanHdPtr = (EncodedSurfChannelHeader_t*)&eventBuffer[count2];
		    
      //	    printf("surf %d, chan %d, encType %d, numBytes %d\n",
      //		   surf,chan,chanHdPtr->encType,chanHdPtr->numBytes);
      count2+=chanHdPtr->numBytes;
      count2+=sizeof(EncodedSurfChannelHeader_t);
    }


    if(numBytes) {
      if((LOS_MAX_BYTES-numBytesInBuffer)<numBytes) {
	//		fillBufferWithHk();
	doWrite();
      }
      memcpy(&losBuffer[numBytesInBuffer],surfHdPtr,numBytes);
      count+=numBytes;
      numBytesInBuffer+=numBytes;
    }
    else break;
    if(count>bufSize) return -1;
  }
  return 0;
}


int sendEncodedPedSubbedWavePackets(int bufSize)
{
  EncodedPedSubbedChannelPacketHeader_t *waveHdPtr;
  EncodedPedSubbedSurfPacketHeader_t *surfHdPtr;
  int numBytes,count=0,surf=0,chan,count2;;
  EncodedSurfChannelHeader_t *chanHdPtr;
  EncodedSurfChannelHeader_t *chanHdPtr2;

  count+=sizeof(EncodedEventWrapper_t);

  // Remember what the file contains is actually 9 EncodedPedSubbedSurfPacketHeader_t's
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    surfHdPtr = (EncodedPedSubbedSurfPacketHeader_t*) &eventBuffer[count];
    //	printf("surfHdPtr.eventNumber %u, numBytes %d (%d)\n",surfHdPtr->eventNumber,numBytes,numBytesInBuffer);
    count2=count+sizeof(EncodedPedSubbedSurfPacketHeader_t);
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
      chanHdPtr = (EncodedSurfChannelHeader_t*)&eventBuffer[count2];
		    
      //	    printf("surf %d, chan %d, encType %d, numBytes %d\n",
      //		   surf,chan,chanHdPtr->encType,chanHdPtr->numBytes);
      numBytes=chanHdPtr->numBytes;
      //	    printf("%d %d %d\n",numBytesInBuffer,LOS_MAX_BYTES,numBytes);
      if((numBytesInBuffer+numBytes+sizeof(EncodedPedSubbedSurfPacketHeader_t)+sizeof(EncodedSurfChannelHeader_t))>LOS_MAX_BYTES) {
	doWrite();
      }
      //	    if((LOS_MAX_BYTES-numBytesInBuffer)<(numBytes+sizeof(EncodedSurfChannelHeader_t))) {		
      //		doWrite();
      //	    }
      waveHdPtr=(EncodedPedSubbedChannelPacketHeader_t*)&losBuffer[numBytesInBuffer];
      waveHdPtr->gHdr.packetNumber=getLosNumber();
      waveHdPtr->eventNumber=surfHdPtr->eventNumber;
      waveHdPtr->whichPeds=surfHdPtr->whichPeds;
      numBytesInBuffer+=sizeof(EncodedPedSubbedChannelPacketHeader_t);
      chanHdPtr2= (EncodedSurfChannelHeader_t*)&losBuffer[numBytesInBuffer];
      (*chanHdPtr2)=(*chanHdPtr);
      numBytesInBuffer+=sizeof(EncodedSurfChannelHeader_t);
      count2+=sizeof(EncodedSurfChannelHeader_t);
      memcpy(&losBuffer[numBytesInBuffer],&eventBuffer[count2],numBytes);
      fillGenericHeader(waveHdPtr,PACKET_ENC_WV_PEDSUB,sizeof(EncodedPedSubbedChannelPacketHeader_t)+sizeof(EncodedSurfChannelHeader_t)+numBytes);
      count2+=chanHdPtr->numBytes;
      numBytesInBuffer+=numBytes;
    }
    if(surfHdPtr->gHdr.numBytes>0) {
      count+=surfHdPtr->gHdr.numBytes;
    }
    else break;
    if(count>bufSize) return -1;
  }
  return 0;
}

int sendRawWaveformPackets(int bufSize) 
{
  if(bufSize!=sizeof(AnitaEventBody_t)) {
    syslog(LOG_ERR,"Buffer size %d, expected %d -- Bailing\n",bufSize,(unsigned int)sizeof(AnitaEventBody_t));
    fprintf(stderr,"Buffer size %d, expected %d -- Bailing\n",bufSize,(unsigned int)sizeof(AnitaEventBody_t));
    return -1;
  }
  AnitaEventBody_t *bdPtr = (AnitaEventBody_t*) eventBuffer;
  int chan;
  RawWaveformPacket_t *wvPtr;
  int numBytes=sizeof(RawWaveformPacket_t);
  for(chan=0;chan<NUM_DIGITZED_CHANNELS;chan++) {
    if((LOS_MAX_BYTES-numBytesInBuffer)<numBytes) {
      doWrite();
    }
    wvPtr=(RawWaveformPacket_t*) &losBuffer[numBytesInBuffer];
    wvPtr->eventNumber=bdPtr->eventNumber;
    memcpy(&(wvPtr->waveform),&(bdPtr->channel[chan]),sizeof(SurfChannelFull_t));
    wvPtr->gHdr.packetNumber=getLosNumber();
    fillGenericHeader(wvPtr,PACKET_WV,sizeof(RawWaveformPacket_t));
    numBytesInBuffer+=sizeof(RawWaveformPacket_t);
  }
  return 0;
}


int sendRawSurfPackets(int bufSize) 
{
  if(bufSize!=sizeof(AnitaEventBody_t)) {
    syslog(LOG_ERR,"Buffer size %d, expected %d -- Bailing\n",bufSize,(unsigned int)sizeof(AnitaEventBody_t));
    fprintf(stderr,"Buffer size %d, expected %d -- Bailing\n",bufSize,(unsigned int)sizeof(AnitaEventBody_t));
    return -1;
  }
  AnitaEventBody_t *bdPtr = (AnitaEventBody_t*) eventBuffer;
  int surf;
  RawSurfPacket_t *surfPtr;
  int numBytes=sizeof(RawSurfPacket_t);
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    if((LOS_MAX_BYTES-numBytesInBuffer)<numBytes) {
      doWrite();
    }
    surfPtr=(RawSurfPacket_t*) &losBuffer[numBytesInBuffer];
    surfPtr->eventNumber=bdPtr->eventNumber;	
    memcpy(&(surfPtr->waveform[0]),&(bdPtr->channel[CHANNELS_PER_SURF*surf]),sizeof(SurfChannelFull_t)*CHANNELS_PER_SURF);
    surfPtr->gHdr.packetNumber=getLosNumber();
    fillGenericHeader(surfPtr,PACKET_SURF,sizeof(RawSurfPacket_t));
    numBytesInBuffer+=sizeof(RawSurfPacket_t);
  }
  return 0;
}



int sendPedSubbedWaveformPackets(int bufSize) 
{
  if(bufSize!=sizeof(PedSubbedEventBody_t)) {
    syslog(LOG_ERR,"Buffer size %d, expected %d -- Bailing\n",bufSize,(unsigned int)sizeof(PedSubbedEventBody_t));
    fprintf(stderr,"Buffer size %d, expected %d -- Bailing\n",bufSize,(unsigned int)sizeof(PedSubbedEventBody_t));
  }
  PedSubbedEventBody_t *bdPtr = (PedSubbedEventBody_t*) eventBuffer;
  int chan;
  PedSubbedWaveformPacket_t *wvPtr;
  int numBytes=sizeof(PedSubbedWaveformPacket_t);
  for(chan=0;chan<NUM_DIGITZED_CHANNELS;chan++) {
    if((LOS_MAX_BYTES-numBytesInBuffer)<numBytes) {
      doWrite();
    }
    wvPtr=(PedSubbedWaveformPacket_t*) &losBuffer[numBytesInBuffer];
    wvPtr->eventNumber=bdPtr->eventNumber;
    wvPtr->whichPeds=bdPtr->whichPeds;
    memcpy(&(wvPtr->waveform),&(bdPtr->channel[chan]),sizeof(SurfChannelPedSubbed_t));
    wvPtr->gHdr.packetNumber=getLosNumber();
    fillGenericHeader(wvPtr,PACKET_PEDSUB_WV,sizeof(PedSubbedWaveformPacket_t));
    //	printf("%#x %d\n",wvPtr->gHdr.code,wvPtr->gHdr.numBytes);
    numBytesInBuffer+=sizeof(PedSubbedWaveformPacket_t);
  }
  return 0;
}


int sendPedSubbedSurfPackets(int bufSize) 
{
  if(bufSize!=sizeof(PedSubbedEventBody_t)) {
    syslog(LOG_ERR,"Buffer size %d, expected %d -- Bailing\n",bufSize,(unsigned int)sizeof(PedSubbedEventBody_t));
    fprintf(stderr,"Buffer size %d, expected %d -- Bailing\n",bufSize,(unsigned int)sizeof(PedSubbedEventBody_t));
    return -1;
  }
  PedSubbedEventBody_t *bdPtr = (PedSubbedEventBody_t*) eventBuffer;
  int surf;
  PedSubbedSurfPacket_t *surfPtr;
  int numBytes=sizeof(PedSubbedSurfPacket_t);
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    if((LOS_MAX_BYTES-numBytesInBuffer)<numBytes) {
      doWrite();
    }
    surfPtr=(PedSubbedSurfPacket_t*) &losBuffer[numBytesInBuffer];
    surfPtr->eventNumber=bdPtr->eventNumber;	
    surfPtr->whichPeds=bdPtr->whichPeds;	
    memcpy(&(surfPtr->waveform[0]),&(bdPtr->channel[CHANNELS_PER_SURF*surf]),sizeof(SurfChannelPedSubbed_t)*CHANNELS_PER_SURF);
    surfPtr->gHdr.packetNumber=getLosNumber();
    fillGenericHeader(surfPtr,PACKET_PEDSUB_SURF,sizeof(PedSubbedSurfPacket_t));
    numBytesInBuffer+=sizeof(PedSubbedSurfPacket_t);
  }
  return 0;
}




unsigned int getLosNumber() {
  int retVal=0;
  static int firstTime=1;
  static int losNumber=0;
  /* This is just to get the lastLosNumber in case of program restart. */
  FILE *pFile;
  if(firstTime) {
    pFile = fopen (LAST_LOS_NUMBER_FILE, "r");
    if(pFile == NULL) {
      syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
	      LAST_LOS_NUMBER_FILE);
    }
    else {	    	    
      retVal=fscanf(pFile,"%d",&losNumber);
      if(retVal<0) {
	syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
		LAST_LOS_NUMBER_FILE);
      }
      fclose (pFile);
    }
    if(printToScreen) printf("The last los number is %d\n",losNumber);
    firstTime=0;
    //Only write out every 100th number, so always start from 100
    losNumber/=100;
    losNumber++;
    losNumber*=100;
  }  
  losNumber++;
  

  if(losNumber%100==0) {
    //    printf("LAST_LOS_NUMBER_FILE %s\n",LAST_LOS_NUMBER_FILE);
    pFile = fopen (LAST_LOS_NUMBER_FILE, "w");
    if(pFile == NULL) {
      syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
	      LAST_LOS_NUMBER_FILE);
      fprintf(stderr,"fopen: %s ---  %s\n",strerror(errno),
	      LAST_LOS_NUMBER_FILE);
    }
    else {
      retVal=fprintf(pFile,"%d\n",losNumber);
      if(retVal<0) {
	syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		LAST_LOS_NUMBER_FILE);
	fprintf(stderr,"fprintf: %s ---  %s\n",strerror(errno),
		LAST_LOS_NUMBER_FILE);
      }
      fclose (pFile);
    }
  }

  return losNumber;

}



void handleBadSigs(int sig)
{   
  fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
  syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
  unlink(LOSD_PID_FILE);
  syslog(LOG_INFO,"LOSd terminating");    
  exit(0);
}


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(LOSD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,LOSD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(LOSD_PID_FILE);
  return 0;
}






int writeLosData(unsigned char *buffer, int numBytesSci)
{



  int nbytes, retVal;
  int retVal2=0;
//  struct timespec now; 
//  double elapsed; 
#ifdef SEND_TEST_PACKET
  int i=0;
  numBytesSci=1024;
  for(i=0;i<numBytesSci;i++)
      buffer[i]=i;
#endif

//  clock_gettime(CLOCK_MONOTONIC_RAW, &now); 


//  elapsed = (double) (now.tv_sec - last_send.tv_sec) + 1e-9 *( now.tv_nsec - last_send.tv_nsec); 
//  if (elapsed < minTimeWait)
 // {
//    usleep((minTimeWait - elapsed) * 1e6); 
//  }

  usleep(minTimeWait * 1e6); 
  //  printf("Sending buffer length %d\n",numBytesSci);

  nbytes = telemwrap((unsigned short*)buffer,(unsigned short*)wrappedBuffer,numBytesSci);
  if (nbytes < 0) {
    syslog(LOG_ERR,"Error wrapping science buffer %d -- %d\n",numBytesSci,nbytes);
    fprintf(stderr,"Error wrapping science buffer %d -- %d\n",numBytesSci,nbytes);
    return -1;
  } else {
    struct pollfd writepoll;    
    writepoll.fd = fdLos;
    writepoll.events = POLLOUT;
    retVal = poll(&writepoll, 1, TIMEOUT_IN_MILLISECONDS);
   // fprintf(stderr,"retVal == %d\n",retVal);
    if (retVal > 0) {
      // write will now not block
      nwrites_before++; 
      retVal2=write(fdLos, wrappedBuffer, nbytes);
      nwrites_after++; 

      minTimeWait =  minTimeWait_b + nbytes * minTimeWait_m; 
//      clock_gettime(CLOCK_MONOTONIC_RAW, &last_send); 
    //  fprintf(stderr,"retVal2 == %d\n",retVal2);
     // int i=0;
    //  for(i=0;i<nbytes;i++) {
//	fprintf(stderr,"%x",wrappedBuffer[i]);
//	fprintf(stderr," ");
  //    }
    //  fprintf(stderr,"\n");
    } else if (retVal == 0) {
      // timeout, do something else, maybe check LOS status using
      // status = ioctl(fd, LOS_IOCSTATUS);
      //
      fprintf(stderr,"Here doing nothing\n");
      // probably put this into a loop above to guarantee that this
      // packet is sent
    } else { // retVal < 0
      // Probably caught a signal - if so, errno will be EINTR
      syslog(LOG_ERR,"Error sending los data -- %s\n",strerror(errno));
      fprintf(stderr,"Error sending los data -- %s\n",strerror(errno));
      return -1;
    }
  }
#ifdef FAKE_LOS
  //Need to sleep to mimic the actual writing speed
  int microseconds=nbytes*8;
  usleep(microseconds);
   printf("Sleeping %d us",microseconds);
#endif


  return 0;
}
