/* LOSd - Program that sends data ovwer the LOS link.
 *
 * Ryan Nichol, August '05
 * Started off as a modified version of Marty's driver program.
 */

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

/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "losLib/los.h"

int initDevice();
void sendWakeUpBuffer();
//void tryToSendData();
//int bufferAndWrite(unsigned char *buf, short nbytes,int doWrite);
//int fillBufferWithPackets();
int doWrite();
int readConfig();
int checkLinkDir(int maxCopy, char *telemDir, char *linkDir, int fileSize);
void fillBufferWithHk();
void readAndSendEvent(char *headerFilename, unsigned long eventNumber);
void readAndSendEventRamdisk(char *headerFilename);
float getTimeDiff(struct timeval oldTime, struct timeval currentTime);
int getLosNumber();



char fakeOutputDir[]="/tmp/fake/los";

/*Config Thingies*/
char losdPidFile[FILENAME_MAX];
char lastLosNumberFile[FILENAME_MAX];

//Packet Dirs
char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];

//Packet Link Dirs
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];

int losBus;
int losSlot;
int verbosity;
int printToScreen;
int laptopDebug;

// Bandwidth variables
int eventBandwidth=80;
int priorityBandwidths[NUM_PRIORITIES];
int eventCounters[NUM_PRIORITIES]={0};
int priorityOrder[NUM_PRIORITIES*100];
int maxEventsBetweenLists;
int numOrders=1000;
int orderIndex=0;
int currentPri=0;

/*Global Variables*/
unsigned char eventBuffer[MAX_EVENT_SIZE];
unsigned char losBuffer[LOS_MAX_BYTES];
int numBytesInBuffer=0;

#define MAX_ATTEMPTS 50

int main(int argc, char *argv[])
{
    int pri,retVal,firstTime=1;
    char *tempString;
    unsigned long eventNumber=0;
    /* Config file thingies */
    int status=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;
    
    char *progName=basename(argv[0]);

    //Directory listing tools
    char currentHeader[FILENAME_MAX];
    int numLinks[NUM_PRIORITIES]={0};
    struct dirent **linkList[NUM_PRIORITIES];
    int sillyEvNum=0;
    int count=0;

   
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("losdPidFile");
	if(tempString) {
	    strncpy(losdPidFile,tempString,FILENAME_MAX);
	    writePidFile(losdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get losdPidFile");
	    fprintf(stderr,"Couldn't get losdPidFile\n");
	}
	tempString=kvpGetString("lastLosNumberFile");
	if(tempString) {
	    strncpy(lastLosNumberFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get lastLosNumberFile");
	    fprintf(stderr,"Couldn't get lastLosNumberFile\n");
	}

    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }
    retVal=readConfig();
    if(!laptopDebug) {
	retVal=initDevice();
	if(retVal!=0) {
	    return 1;
	}
    }
    else {
	printf("Running in debug mode not actually trying to talk to device\n");
	makeDirectories(fakeOutputDir);
    }

    makeDirectories(PRIORITIZERD_EVENT_LINK_DIR);
    makeDirectories(HEADER_TELEM_LINK_DIR);
    makeDirectories(SURFHK_TELEM_LINK_DIR);
    makeDirectories(TURFHK_TELEM_LINK_DIR);
    makeDirectories(HK_TELEM_LINK_DIR);
    makeDirectories(MONITOR_TELEM_LINK_DIR);
    makeDirectories(GPS_TELEM_LINK_DIR);
    makeDirectories(CMD_ECHO_TELEM_LINK_DIR);

    
    //Fill event dir names
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
	sprintf(eventTelemDirs[pri],"%s/%s%d",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);
	sprintf(eventTelemLinkDirs[pri],"%s/link",eventTelemDirs[pri]);
	makeDirectories(eventTelemLinkDirs[pri]);
    }
    
    do {
	if(verbosity) printf("Initializing LOSd\n");
	retVal=readConfig();
	if(firstTime) {
	    sendWakeUpBuffer();
	    firstTime=0;
	}
	    	
	currentState=PROG_STATE_RUN;
        while(currentState==PROG_STATE_RUN) {
	    currentPri=priorityOrder[orderIndex];	    
	    if(numLinks[currentPri]==0 || 
	       (sillyEvNum)>maxEventsBetweenLists) {
		if(numLinks[currentPri]>0) {
		    //Need to free memory
		    for(count=0;count<numLinks[currentPri];
			count++) 
			free(linkList[currentPri][count]);		    
		    free(linkList[currentPri]);
		}		
		numLinks[currentPri]=
		    getListofLinks(eventTelemLinkDirs[currentPri],
				   &linkList[currentPri]);
		
		//Quick and dirty hack
		if(numLinks[currentPri]>200) {
//		    printf("Here %d %d\n",currentPri,numLinks[currentPri]);
		    syslog(LOG_INFO,"LOSd not keeping up removing %d event links from queue %s\n",numLinks[currentPri]-100,eventTelemLinkDirs[currentPri]);
		    fprintf(stderr,"LOSd not keeping up removing %d event links from queue %s\n",numLinks[currentPri]-100,eventTelemLinkDirs[currentPri]);
		    for(count=0;count<numLinks[currentPri]-100;
			count++) {
			
			sscanf(linkList[currentPri][count]->d_name,
			       "hd_%lu.dat",&eventNumber);
									
			sprintf(currentHeader,"%s/%s",eventTelemLinkDirs[currentPri],
				linkList[currentPri][count]->d_name);
			removeFile(currentHeader);
			sprintf(currentHeader,"%s/ev_%ld.dat",eventTelemDirs[currentPri], 
				eventNumber);
			removeFile(currentHeader);			
#ifdef SPEED_UP_LOSD
			sprintf(currentHeader,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
				eventNumber);
			removeFile(currentHeader);			
#endif
		    }
		    for(count=0;count<numLinks[currentPri];
			count++) 
			free(linkList[currentPri][count]);		    
		    free(linkList[currentPri]);
			
		    numLinks[currentPri]=
			getListofLinks(eventTelemLinkDirs[currentPri],
				       &linkList[currentPri]);	
		    
		}

		
		if(printToScreen && verbosity>1) {
		    printf("Got %d links in %s\n",numLinks[currentPri],
			   eventTelemLinkDirs[currentPri]);
		}
		sillyEvNum=0;
	    }
	    if(numLinks[currentPri]>0) {
		//Got an event
#ifdef SPEED_UP_LOSD
		sprintf(currentHeader,"%s/%s",eventTelemLinkDirs[currentPri],
			linkList[currentPri][numLinks[currentPri]-1]->d_name);
		readAndSendEventRamdisk(currentHeader); //Also deletes
#else
		sscanf(linkList[currentPri][numLinks[currentPri]-1]->d_name,
		       "hd_%lu.dat",&eventNumber);
		sprintf(currentHeader,"%s/%s",eventTelemLinkDirs[currentPri],
			linkList[currentPri][numLinks[currentPri]-1]->d_name);
		readAndSendEvent(currentHeader,eventNumber); //Also deletes
#endif
		free(linkList[currentPri][numLinks[currentPri]-1]);
		numLinks[currentPri]--;
		sillyEvNum++;
	    }
	    fillBufferWithHk();
	    usleep(1);
	    orderIndex++;
	    if(orderIndex>=numOrders) orderIndex=0;
	}
    } while(currentState==PROG_STATE_INIT);

//    fprintf(stderr, "Bye bye\n");
    return 0;
} 


int initDevice() {
    int retVal=los_init((unsigned char)losBus,(unsigned char)losSlot,0,1,100);
    if(retVal!=0) {
	syslog(LOG_ERR,"Problem opening LOS board: %s",los_strerror());	
	fprintf(stderr,"Problem opening LOS board: %s\n",los_strerror()); 
    }
    else if(verbosity) {
	printf("Succesfully opened board: Bus %d, Slot %d\n",losBus,losSlot);
    }


    return retVal;
}

int doWrite() {
    static unsigned int dataCounter=0;
    static int first=1;
    static struct timeval lastTime;
    struct timeval newTime;
    float timeDiff;
    if(first) {
	gettimeofday(&lastTime,0);
	first=0;
    }

    int retVal;
    int attempts=0;
    if(!laptopDebug) {
	while(attempts<MAX_ATTEMPTS) {
	    retVal=los_write(losBuffer,numBytesInBuffer);
	    if(retVal==0) break;
	    attempts++;
	}
	if(attempts>30) {
	    printf("Took %d attempts to write %d bytes\n",attempts,numBytesInBuffer);
	    syslog(LOG_ERR,"Took %d attempts to write %d bytes\n",attempts,numBytesInBuffer);

	}
    }
    else {
	retVal=fake_los_write(losBuffer,numBytesInBuffer,fakeOutputDir);
    }	

    dataCounter+=numBytesInBuffer;
//    printf("%d %d\n",numBytesInBuffer,dataCounter);
    if(dataCounter>1000000) {
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
	maxEventsBetweenLists=kvpGetInt("maxEventsBetweenLists",100);
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
    int retVal,numLinks,count,numBytes,totalBytes=0,checkVal=0;
    GenericHeader_t *gHdr;
    struct dirent **linkList;

    if((numBytesInBuffer+fileSize)>LOS_MAX_BYTES) return 0;


    numLinks=getListofLinks(linkDir,&linkList); 
    if(numLinks<=0) {
	return 0;
    }
        
    for(count=numLinks-1;count>=0;count--) {
	sprintf(currentFilename,"%s/%s",telemDir,
		linkList[count]->d_name);
	sprintf(currentLinkname,"%s/%s",
		linkDir,linkList[count]->d_name);

	retVal=genericReadOfFile((char*)&(losBuffer[numBytesInBuffer]),
				 currentFilename,
				 LOS_MAX_BYTES-numBytesInBuffer);
	if(retVal<=0) {
	    syslog(LOG_ERR,"Error opening file, will delete: %s",
		   currentFilename);
	    fprintf(stderr,"Error reading file %s -- %d\n",currentFilename,retVal);
	    removeFile(currentFilename);

	    removeFile(currentLinkname);
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
	removeFile(currentLinkname);
	removeFile(currentFilename);

	if((totalBytes+fileSize)>maxCopy ||
	   (numBytesInBuffer+fileSize)>LOS_MAX_BYTES) break;
    }
    
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
    
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

    if((LOS_MAX_BYTES-numBytesInBuffer)>sizeof(CommandEcho_t)) {
	checkLinkDir(LOS_MAX_BYTES-numBytesInBuffer,
		     CMD_ECHO_TELEM_DIR,CMD_ECHO_TELEM_LINK_DIR,
		     sizeof(CommandEcho_t)); 
    }
    if((LOS_MAX_BYTES-numBytesInBuffer)>sizeof(MonitorStruct_t)) {
	checkLinkDir(LOS_MAX_BYTES-numBytesInBuffer,
		     MONITOR_TELEM_DIR,MONITOR_TELEM_LINK_DIR,
		     sizeof(MonitorStruct_t)); 
    }
    if((LOS_MAX_BYTES-numBytesInBuffer)>sizeof(GpsAdu5SatStruct_t)) {
	checkLinkDir(LOS_MAX_BYTES-numBytesInBuffer,GPS_TELEM_DIR,
		     GPS_TELEM_LINK_DIR,sizeof(GpsAdu5SatStruct_t)); 
    }
    if((LOS_MAX_BYTES-numBytesInBuffer)>sizeof(HkDataStruct_t)) {
	checkLinkDir(LOS_MAX_BYTES-numBytesInBuffer,HK_TELEM_DIR,
		     HK_TELEM_LINK_DIR,sizeof(HkDataStruct_t)); 
    }
    if((LOS_MAX_BYTES-numBytesInBuffer)>sizeof(FullSurfHkStruct_t)) {
	checkLinkDir(LOS_MAX_BYTES-numBytesInBuffer,SURFHK_TELEM_DIR,
		     SURFHK_TELEM_LINK_DIR,sizeof(FullSurfHkStruct_t)); 
    }
    if((LOS_MAX_BYTES-numBytesInBuffer)>sizeof(TurfRateStruct_t)) {	
	checkLinkDir(LOS_MAX_BYTES-numBytesInBuffer,TURFHK_TELEM_DIR,
		     TURFHK_TELEM_LINK_DIR,sizeof(TurfRateStruct_t)); 
    }
    if((LOS_MAX_BYTES-numBytesInBuffer)>sizeof(AnitaEventHeader_t)) {	
	checkLinkDir(LOS_MAX_BYTES-numBytesInBuffer,HEADER_TELEM_DIR,
		     HEADER_TELEM_LINK_DIR,sizeof(AnitaEventHeader_t));
    }        
    if(numBytesInBuffer>0)
	doWrite();
    if(numBytesInBuffer<0) {
	printf("What the heck does %d bytes mean\n",numBytesInBuffer);
    }
}


void readAndSendEvent(char *headerFilename, unsigned long eventNumber) {
    AnitaEventHeader_t *theHeader;
    EncodedSurfPacketHeader_t *surfHdPtr;
    int numBytes,count=0,surf=0,retVal;//,useGzip=0;
    char waveFilename[FILENAME_MAX];
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
	 printf("Trying event %lu file %s\n",eventNumber,headerFilename);
	 printf("Have event %lu (wanted %lu) -- retVal %d\n",
		theHeader->eventNumber,
		eventNumber,retVal);
     }
	 
	 
     
     if(retVal<0) {
	 removeFile(headerFilename);
	 sprintf(waveFilename,"%s/ev_%ld.dat",eventTelemDirs[currentPri], 
	     theHeader->eventNumber);
	 removeFile(waveFilename);
	 
	 //Bollock
	 return;
     }

     //Now get event file
     sprintf(waveFilename,"%s/ev_%ld.dat",eventTelemDirs[currentPri], 
 	    eventNumber);
     numBytes=readEncodedEventFromFile(eventBuffer,waveFilename,
				       eventNumber);

     
//     printf("Read %d bytes of event %lu from file %s\n",
//	    numBytes,eventNumber,waveFilename);
	   
//     exit(0);

/*     //Now get event file */
/*     sprintf(waveFilename,"%s/ev_%ld.dat",eventTelemDirs[currentPri], */
/* 	    theHeader->eventNumber); */
/*     eventFile = fopen(waveFilename,"r"); */
/*     if(!eventFile) { */
/* 	sprintf(waveFilename,"%s/ev_%ld.dat.gz",eventTelemDirs[currentPri], */
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
    AnitaEventHeader_t *theHeader;
    EncodedSurfPacketHeader_t *surfHdPtr;
    int numBytes,count=0,surf=0,retVal;
    char waveFilename[FILENAME_MAX];
    char headerFilename[FILENAME_MAX];
    char crapBuffer[FILENAME_MAX];
    unsigned long thisEventNumber;

    //First check if there is room for the header
    if((LOS_MAX_BYTES-numBytesInBuffer)<sizeof(AnitaEventHeader_t))
	doWrite();

//     Next load header 
    theHeader=(AnitaEventHeader_t*) &losBuffer[numBytesInBuffer]; 
    retVal=fillHeader(theHeader,headerLinkFilename); 
    theHeader->gHdr.packetNumber=getLosNumber();
    numBytesInBuffer+=sizeof(AnitaEventHeader_t);

    
    if(retVal<0) {
	removeFile(headerLinkFilename);
	sscanf(headerLinkFilename,"%s/hd_%lu.dat",crapBuffer,
	       &thisEventNumber);
	sprintf(headerFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
		thisEventNumber);
	removeFile(headerFilename);
	sprintf(waveFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
		thisEventNumber);
	removeFile(waveFilename);
	
	//Bollocks
	return;
    }
    
    thisEventNumber=theHeader->eventNumber;
    

    //Now get event file
    sprintf(headerFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
 	    thisEventNumber);
    sprintf(waveFilename,"%s/ev_%ld.dat",eventTelemDirs[currentPri], 
 	    thisEventNumber);


    retVal=genericReadOfFile(eventBuffer,waveFilename,MAX_EVENT_SIZE);
    if(retVal<0) {
	fprintf(stderr,"Problem reading %s\n",waveFilename);
	removeFile(headerLinkFilename);
	removeFile(headerFilename);
	removeFile(waveFilename);
	
	//Bollocks
	return;
    }

    // Remember what the file contains is actually 9 EncodedSurfPacketHeader_t's
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
    }
    
    if(printToScreen && verbosity>1) 
	printf("Removing files %s\t%s\n%s\n",headerFilename,waveFilename,
	       headerLinkFilename);
	        
    removeFile(headerFilename);
    removeFile(headerLinkFilename);
    removeFile(waveFilename);

}


int getLosNumber() {
    int retVal=0;
    static int firstTime=1;
    static int losNumber=0;
    /* This is just to get the lastLosNumber in case of program restart. */
    FILE *pFile;
    if(firstTime) {
	pFile = fopen (lastLosNumberFile, "r");
	if(pFile == NULL) {
	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		    lastLosNumberFile);
	}
	else {	    	    
	    retVal=fscanf(pFile,"%d",&losNumber);
	    if(retVal<0) {
		syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
			lastLosNumberFile);
	    }
	    fclose (pFile);
	}
	if(printToScreen) printf("The last los number is %d\n",losNumber);
	firstTime=0;
    }
    losNumber++;

    pFile = fopen (lastLosNumberFile, "w");
    if(pFile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		lastLosNumberFile);
    }
    else {
	retVal=fprintf(pFile,"%d\n",losNumber);
	if(retVal<0) {
	    syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		    lastLosNumberFile);
	    }
	fclose (pFile);
    }

    return losNumber;

}
