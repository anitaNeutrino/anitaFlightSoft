/*! \file Archived.c
  \brief The Archived program that writes Events to disk
    
  May 2006 rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <zlib.h>
#include <pthread.h>
#include <libgen.h> //For Mac OS X

#include "Archived.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "compressLib/compressLib.h"
#include "pedestalLib/pedestalLib.h"
#include "linkWatchLib/linkWatchLib.h"

#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"


// Directories and gubbins 

int onboardStorageType;
int telemType;
int priDiskEncodingType[NUM_PRIORITIES];
int priTelemEncodingType[NUM_PRIORITIES];
int priTelemClockEncodingType[NUM_PRIORITIES];
float priorityFractionGlobalDecimate[NUM_PRIORITIES];
float priorityFractionDeleteHelium1[NUM_PRIORITIES];
float priorityFractionDeleteHelium2[NUM_PRIORITIES];
float priorityFractionDeleteUsb[NUM_PRIORITIES];
float priorityFractionDeleteNeobrick[NUM_PRIORITIES];
float priorityFractionDeletePmc[NUM_PRIORITIES];
int priDiskEventMask[NUM_PRIORITIES];
int currentRun;

char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];



int printToScreen=0;
int verbosity=0;
int writeTelem=0;

int eventDiskBitMask;
int eventDiskBitMask=0;
int disableHelium1=0;
int disableHelium2=0;
int disableUsb=0;
//int disableNeobrick=0;
int helium1CloneMask;
int helium2CloneMask;
int usbCloneMask;
int priorityPPS1;
int priorityPPS2;
int prioritySoft;
float pps1FractionDelete=0;
float pps2FractionDelete=0;
float softFractionDelete=0;
float pps1FractionTelem=0;
float pps2FractionTelem=0;
float softFractionTelem=0;
AnitaHkWriterStruct_t indexWriter;


char usbName[FILENAME_MAX];


int usbBitMasks[2]={0x4,0x8};

FILE *fpHead=NULL;
FILE *fpEvent=NULL;

int shouldWeThrowAway(int pri);
int getDecimatedDiskMask(int pri, int diskMask);
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
int telemEvent(int trigType);



int diskBitMasks[DISK_TYPES]={HELIUM1_DISK_MASK,HELIUM2_DISK_MASK,USB_DISK_MASK,PMC_DISK_MASK,NTU_DISK_MASK};

#define MAX_EVENT_LINKS 100

typedef struct {
  int numLinks;
  char linkPath[MAX_EVENT_LINKS][FILENAME_MAX];
} EventLinkStruct_t;

void handleListOfEvents(EventLinkStruct_t *linkStructPtr) ;

//thread stuff
//pthread_mutex_t mutexindex





int main (int argc, char *argv[])
{
    int retVal,pri;
    char *tempString;
    /* Config file thingies */
    int status=0;
    char* eString ;

    
    /* Log stuff */
    char *progName=basename(argv[0]);
 
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, handleBadSigs);
    signal(SIGINT, handleBadSigs);
    signal(SIGSEGV, handleBadSigs);

    //Dont' wait for children
    signal(SIGCHLD, SIG_IGN); 

    //sortOutPidFile
    retVal=sortOutPidFile(progName);
    if(retVal!=0) {
      return retVal;
    }

   
    /* Load Config */
    readConfigFile();
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    if (status == CONFIG_E_OK) {
	eventDiskBitMask=kvpGetInt("eventDiskBitMask",1);
	disableHelium1=kvpGetInt("disableHelium1",0);
	if(disableHelium1) {
	  eventDiskBitMask&=(~HELIUM1_DISK_MASK);
	}
	disableHelium2=kvpGetInt("disableHelium2",0);
	if(disableHelium2) {
	  eventDiskBitMask&=(~HELIUM2_DISK_MASK);
	}
	disableUsb=kvpGetInt("disableUsb",0);
	if(disableUsb) {
	  eventDiskBitMask&=(~USB_DISK_MASK);
	}
	//	disableNeobrick=kvpGetInt("disableNeobrick",0);
	//	if(disableNeobrick) {
	//	  eventDiskBitMask&=(~NEOBRICK_DISK_MASK);
	//	}


	helium1CloneMask=kvpGetInt("helium1CloneMask",0);
	helium2CloneMask=kvpGetInt("helium2CloneMask",0);
	usbCloneMask=kvpGetInt("usbCloneMask",0);


	tempString=kvpGetString("usbName");
	if(tempString) {
	    strncpy(usbName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbName");
	    fprintf(stderr,"Couldn't get usbName\n");
	}


    }
    else {

      eString = configErrorString (status) ;
      syslog(LOG_ERR,"Error reading %s -- %s",GLOBAL_CONF_FILE,eString);
    }
    
    //Fill event dir names
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
	sprintf(eventTelemDirs[pri],"%s/%s%d",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);
	sprintf(eventTelemLinkDirs[pri],"%s/link",eventTelemDirs[pri]);
	makeDirectories(eventTelemLinkDirs[pri]);
    }

    currentRun=getRunNumber();

    makeDirectories(ANITA_INDEX_DIR);
    makeDirectories(PRIORITIZERD_EVENT_LINK_DIR);
    retVal=0;

    // pthread initialisation
    //   pthread_attr_t attr;
    //    pthread_mutex_init(&mutexindex, NULL);            
    //    pthread_attr_init(&attr);
    //    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);



    /* Main event getting loop. */
    do {
	if(printToScreen) printf("Initalizing Archived\n");
	retVal=readConfigFile();
	prepIndexWriter();


	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading Archived.config");
	    printf("Problem reading Archived.config\n");
	}
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    checkEvents();
	   // usleep(1000);
	}
    } while(currentState==PROG_STATE_INIT);    


    closeHkFilesAndTidy(&indexWriter);
    if(fpHead) fclose(fpHead);
    if(fpEvent) fclose(fpEvent);
    unlink(ARCHIVED_PID_FILE);
    syslog(LOG_INFO,"Archived terminating");
    return 0;
}



int readConfigFile() 
/* Load Archived config stuff */
{
    /* Config file thingies */
    int status=0,tempNum=0;
    char* eString ;
    KvpErrorCode kvpStatus;
    kvpReset();
    status = configLoad ("Archived.config","archived") ;
    status += configLoad ("Archived.config","output") ;

    if(status == CONFIG_E_OK) {
	writeTelem=kvpGetInt("writeTelem",0);
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	onboardStorageType=kvpGetInt("onboardStorageType",1);
	telemType=kvpGetInt("telemType",1);
	priorityPPS1=kvpGetInt("priorityPPS1",-1);
	priorityPPS2=kvpGetInt("priorityPPS2",-1);
	prioritySoft=kvpGetInt("prioritySoft",-1);
	pps1FractionDelete=kvpGetFloat("pps1FractionDelete",0);
	pps2FractionDelete=kvpGetFloat("pps2FractionDelete",0);
	softFractionDelete=kvpGetFloat("softFractionDelete",0);
	pps1FractionTelem=kvpGetFloat("pps1FractionTelem",0);
	pps2FractionTelem=kvpGetFloat("pps2FractionTelem",0);
	softFractionTelem=kvpGetFloat("softFractionTelem",0);
	tempNum=10;
	kvpStatus = kvpGetIntArray("priDiskEncodingType",
				   priDiskEncodingType,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priDiskEncodingType): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priDiskEncodingType): %s\n",
			kvpErrorString(kvpStatus));
	}

	tempNum=10;
	kvpStatus = kvpGetIntArray("priTelemEncodingType",
				   priTelemEncodingType,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priTelemEncodingType): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priTelemEncodingType): %s\n",
			kvpErrorString(kvpStatus));
	}


	tempNum=10;
	kvpStatus = kvpGetIntArray("priTelemClockEncodingType",
				   priTelemClockEncodingType,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priTelemClockEncodingType): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priTelemClockEncodingType): %s\n",
			kvpErrorString(kvpStatus));
	}

	tempNum=NUM_PRIORITIES;
	kvpStatus = kvpGetFloatArray("priorityFractionGlobalDecimate",
				     priorityFractionGlobalDecimate,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetFloatArray(priorityFractionGlobalDecimate): %s",
		   kvpErrorString(kvpStatus));
		fprintf(stderr,"kvpGetFloatArray(priorityFractionGlobalDecimate): %s\n",
			kvpErrorString(kvpStatus));
	}

	tempNum=NUM_PRIORITIES;
	kvpStatus = kvpGetFloatArray("priorityFractionDeleteHelium1",
				     priorityFractionDeleteHelium1,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetFloatArray(priorityFractionDeleteHelium1): %s",
		   kvpErrorString(kvpStatus));
		fprintf(stderr,"kvpGetFloatArray(priorityFractionDeleteHelium1): %s\n",
			kvpErrorString(kvpStatus));
	}

	tempNum=NUM_PRIORITIES;
	kvpStatus = kvpGetFloatArray("priorityFractionDeleteHelium2",
				     priorityFractionDeleteHelium2,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetFloatArray(priorityFractionDeleteHelium2): %s",
		   kvpErrorString(kvpStatus));
		fprintf(stderr,"kvpGetFloatArray(priorityFractionDeleteHelium2): %s\n",
			kvpErrorString(kvpStatus));
	}

	tempNum=NUM_PRIORITIES;
	kvpStatus = kvpGetFloatArray("priorityFractionDeleteUsb",
				     priorityFractionDeleteUsb,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetFloatArray(priorityFractionDeleteUsb): %s",
		   kvpErrorString(kvpStatus));
		fprintf(stderr,"kvpGetFloatArray(priorityFractionDeleteUsb): %s\n",
			kvpErrorString(kvpStatus));
	}

	tempNum=NUM_PRIORITIES;
	kvpStatus = kvpGetFloatArray("priorityFractionDeleteNeobrick",
				     priorityFractionDeleteNeobrick,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetFloatArray(priorityFractionDeleteNeobrick): %s",
		   kvpErrorString(kvpStatus));
		fprintf(stderr,"kvpGetFloatArray(priorityFractionDeleteNeobrick): %s\n",
			kvpErrorString(kvpStatus));
	}

	tempNum=NUM_PRIORITIES;
	kvpStatus = kvpGetFloatArray("priorityFractionDeletePmc",
				     priorityFractionDeletePmc,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetFloatArray(priorityFractionDeletePmc): %s",
		   kvpErrorString(kvpStatus));
		fprintf(stderr,"kvpGetFloatArray(priorityFractionDeletePmc): %s\n",
			kvpErrorString(kvpStatus));
	}


	tempNum=10;
	kvpStatus = kvpGetIntArray("priDiskEventMask",
				   priDiskEventMask,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priDiskEventMask): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priDiskEventMask): %s\n",
			kvpErrorString(kvpStatus));
	}
	


    }
    else {
	eString=configErrorString (status) ; 
	syslog(LOG_ERR,"Error reading Archived.config: %s\n",eString); 
    }
    
    return status;
}
 
void checkEvents() 
{

    int count,retVal;
    static int wd=0;
    /* Directory reading things */
    int numLinks=0;
    char *tempString;

    static EventLinkStruct_t linkStruct;   
    if(wd==0) {
      linkStruct.numLinks=0;    
      //First time need to prep the watcher directory
      wd=setupLinkWatchDir(PRIORITIZERD_EVENT_LINK_DIR);
      if(wd<=0) {
	fprintf(stderr,"Unable to watch %s\n",PRIORITIZERD_EVENT_LINK_DIR);
	syslog(LOG_ERR,"Unable to watch %s\n",PRIORITIZERD_EVENT_LINK_DIR);
	exit(0);
      }
      numLinks=getNumLinks(wd);
    }
    //Check for new inotify events
    retVal=checkLinkDirs(1,0); //Should probably check
    if(retVal || numLinks)
      numLinks=getNumLinks(wd); //Current events waiting
    if((printToScreen && verbosity) || 1) printf("Found %d links\n",numLinks);




    for(count=0;count<numLinks;count++) {
      //Need to add a check for when the event number is %MAX_EVENT_LINKS==0
      tempString=getFirstLink(wd);
      printf("%d %d %s\n",count,linkStruct.numLinks,tempString);
      if(linkStruct.numLinks<MAX_EVENT_LINKS) {
	strncpy(linkStruct.linkPath[linkStruct.numLinks],tempString,FILENAME_MAX);
	linkStruct.numLinks++;
      }
      else {
	handleListOfEvents(&linkStruct);
	memset(&linkStruct,0,sizeof(EventLinkStruct_t));
      }
 
    }
}


void handleListOfEvents(EventLinkStruct_t *linkStructPtr) {

  AnitaEventWriterStruct_t eventWriter;
  int count=0,retVal=0;
  char *tempString;
  char currentHeadname[FILENAME_MAX];
  char currentLinkname[FILENAME_MAX];
  char currentBodyname[FILENAME_MAX];
//    char currentPSBodyname[FILENAME_MAX];
  
  //Event Structures
  AnitaEventHeader_t theHead;
  //    AnitaEventBody_t theBody; //no longer used
  PedSubbedEventBody_t pedSubBody;
  IndexEntry_t indEnt[MAX_EVENT_LINKS];
 
  prepEventWriterStruct(&eventWriter);

  eventWriter.justHeader=0;
  for(count=0;count<linkStructPtr->numLinks;count++) {
    tempString=linkStructPtr->linkPath[count];
      
    sprintf(currentHeadname,"%s/%s",PRIORITIZERD_EVENT_DIR,tempString);
    sprintf(currentLinkname,"%s/%s",PRIORITIZERD_EVENT_LINK_DIR,tempString);
    retVal=fillHeader(&theHead,currentHeadname);
    
    unlink(currentHeadname);
    unlink(currentLinkname);
    
    
    if(retVal==0) {
      //Switch to
      sprintf(currentBodyname,"%s/psev_%u.dat",PRIORITIZERD_EVENT_DIR,
	      theHead.eventNumber);
      
      
      
      if(!shouldWeThrowAway(theHead.priority&0xf) ) {	    	
	retVal=fillPedSubbedBody(&pedSubBody,currentBodyname);
	
	if(retVal==0) {
	  processEvent(&theHead,&pedSubBody,indEnt[count],&eventWriter);
	}
	else {
	  syslog(LOG_ERR,"Error getting event from %s\n",currentBodyname);
	  fprintf(stderr,"Error getting event from %s\n",currentBodyname);
	  eventWriter.justHeader=1;
	  processEvent(&theHead,&pedSubBody,indEnt[count],&eventWriter);
	}	
      }
      else {
	eventWriter.justHeader=1;
	processEvent(&theHead,&pedSubBody,indEnt[count],&eventWriter);	
      }
      unlink(currentBodyname);
      
    }
    else {
      syslog(LOG_ERR,"Error getting header from %s\n",currentHeadname);
      fprintf(stderr,"Error getting header from %s\n",currentHeadname);
    }
  }


  closeEventFilesAndTidy(&eventWriter);

  //  pthread_mutex_lock (&mutexindex);
  for(count=0;count<linkStructPtr->numLinks;count++) {
    //RJN need to add ntu Label
    cleverIndexWriter(&indEnt[count],&indexWriter);
  }
  //  pthread_mutex_unlock (&mutexindex);
    
}




void processEvent(AnitaEventHeader_t *hdPtr, PedSubbedEventBody_t *psPtr,  IndexEntry_t *indEntPtr, AnitaEventWriterStruct_t *eventWriterPtr) 
// This just fills the buffer with 10 (well ACTIVE_SURFS) EncodedSurfPacketHeader_t and their associated waveforms.
{
    //Encoding Buffer
    unsigned char outputBuffer[MAX_WAVE_BUFFER];

    CompressErrorCode_t retVal=0;
    EncodeControlStruct_t diskEncCntl;
    EncodeControlStruct_t telemEncCntl;
    int surf,chan,numBytes;

    //Stuff for helium2
    //    static int errorCounter=0;
    //    static int fileEpoch=0;
    //    static int fileNum=0;
    //    static int dirNum=0;
    //    static int otherDirNum=0;
    //    static char helium2DirName[FILENAME_MAX];
    //    static char helium2HeaderFileName[FILENAME_MAX];
    //    static char helium2EventFileName[FILENAME_MAX];
    

    int priority=(hdPtr->priority&0xf);
    if(priority<0 || priority>9) priority=9;
    int thisBitMask=eventDiskBitMask&priDiskEventMask[priority];
    if((eventDiskBitMask&0x10) && (priDiskEventMask[priority]&0x10))
	thisBitMask |= 0x10;


//    printf("%#x %#x %#x -- %d \n",thisBitMask,priDiskEventMask[priority],eventDiskBitMask,priority);
    thisBitMask=getDecimatedDiskMask(priority,thisBitMask);

    eventWriterPtr->writeBitMask=thisBitMask;
    if(printToScreen && verbosity>1) {
      printf("\nEvent %u, priority %d\n",hdPtr->eventNumber,
	     hdPtr->priority);
      printf("Priority %d, thisBitMask %#x\n",priority,thisBitMask);

    }

    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    diskEncCntl.encTypes[surf][chan]=(ChannelEncodingType_t) priDiskEncodingType[priority];
	    telemEncCntl.encTypes[surf][chan]= (ChannelEncodingType_t) priTelemEncodingType[priority];
	    if(chan==8)
		telemEncCntl.encTypes[surf][chan]= (ChannelEncodingType_t) priTelemClockEncodingType[priority];
	}
    }
 
//Steps
    //2) Pack Event For On Board Storage
    //3) Pack Event For Telemetry


    //2) Pack Event For On Board Storage
    //Now depending on which option is selected we may either write out pedSubbed or not pedSubbed events to the hard disk (or just AnitaEventBody_t structs)
    if(!eventWriterPtr->justHeader) {
	/* memset(outputBuffer,0,MAX_WAVE_BUFFER); */
	/* switch(onboardStorageType) { */
	/* case ARCHIVE_RAW: */
	/*       //No longer an option */
	/*       //		memcpy(outputBuffer,&theBody,sizeof(AnitaEventBody_t)); */
	/*       //		numBytes=sizeof(AnitaEventBody_t); */
	/*       //		retVal=COMPRESS_E_OK; */
	/*       //		break; */
	/* case ARCHIVE_PEDSUBBED: */
	/*   memcpy(outputBuffer,psPtr,sizeof(PedSubbedEventBody_t)); */
	/*   numBytes=sizeof(PedSubbedEventBody_t); */
	/*   retVal=COMPRESS_E_OK; */
	/*   break; */
	/* case ARCHIVE_ENCODED:	     */
	/*   //Again no longer an option */
	/*   //		retVal=packEvent(&theBody,&diskEncCntl,outputBuffer,&numBytes); */
	/*   //		break; */
	/* case ARCHIVE_PEDSUBBED_ENCODED:	     */
	/*   retVal=packPedSubbedEvent(psPtr,&diskEncCntl,outputBuffer,&numBytes); */
	/*   break; */
	/* default: */
	/*   memcpy(outputBuffer,psPtr,sizeof(PedSubbedEventBody_t)); */
	/*   numBytes=sizeof(PedSubbedEventBody_t); */
	/*   retVal=COMPRESS_E_OK; */
	/*   //memcpy(outputBuffer,&theBody,sizeof(AnitaEventBody_t)); */
	/*   //numBytes=sizeof(AnitaEventBody_t); */
	/*   //retVal=COMPRESS_E_OK; */
	/*   break;	     */
	/* } */
      numBytes=sizeof(PedSubbedEventBody_t);
    }
    else numBytes=0;

    if(retVal==COMPRESS_E_OK || eventWriterPtr->justHeader) {
      writeOutputToDisk(numBytes,(unsigned char*)psPtr,hdPtr,eventWriterPtr);
      //Now write the index
      indEntPtr->runNumber=currentRun;
      indEntPtr->eventNumber=hdPtr->eventNumber;
      indEntPtr->eventDiskBitMask=eventWriterPtr->writeBitMask;
      strncpy(indEntPtr->usbLabel,usbName,11);
    }
    else {
	syslog(LOG_ERR,"Error compressing event %u\n",psPtr->eventNumber);
	fprintf(stderr,"Error compressing event %u\n",psPtr->eventNumber);
    }

    //3) Pack Event For Telemetry
    //Again we have a range of options for event telemetry

    if(printToScreen && verbosity>1) printf("Event %d, telemType %d\n",hdPtr->eventNumber,telemType);

    if(!eventWriterPtr->justHeader) {
	memset(outputBuffer,0,MAX_WAVE_BUFFER);
	switch(telemType) {
	    case ARCHIVE_RAW:
		//Need to add routine
	      //		memcpy(outputBuffer,&theBody,sizeof(AnitaEventBody_t));
	      //		numBytes=sizeof(AnitaEventBody_t);
	      //		retVal=COMPRESS_E_OK;
	      //		break;
	    case ARCHIVE_PEDSUBBED:
		//Need to add routine
		memcpy(outputBuffer,psPtr,sizeof(PedSubbedEventBody_t));
		numBytes=sizeof(PedSubbedEventBody_t);
		retVal=COMPRESS_E_OK;
		break;
		//	    case ARCHIVE_ENCODED:	    
		//		retVal=packEvent(&theBody,&telemEncCntl,outputBuffer,&numBytes);
		//		break;
	    case ARCHIVE_PEDSUBBED_ENCODED:	    
		retVal=packPedSubbedEvent(psPtr,&telemEncCntl,outputBuffer,&numBytes);
		break;
	}
    }
    if(writeTelem) {
	if(retVal==COMPRESS_E_OK)
	  writeOutputForTelem(numBytes,outputBuffer,hdPtr,eventWriterPtr);
	else {
	    syslog(LOG_ERR,"Error compressing event %u for telemetry\n",psPtr->eventNumber);
	    fprintf(stderr,"Error compressing event %u for telemetry\n",psPtr->eventNumber);
	}
    }
}


void writeOutputToDisk(int numBytes, unsigned char *outputBuffer, AnitaEventHeader_t *hdPtr, 
		       AnitaEventWriterStruct_t *eventWriterPtr) {
   
    int retVal;

    retVal=cleverEventWrite(outputBuffer,numBytes,hdPtr,eventWriterPtr);
    if(retVal!=0) {
      syslog(LOG_ERR,"Error writing event %s\n",strerror(errno));
    }

    if(printToScreen && verbosity>1) {
	printf("Event %u, (%d bytes)  %s \t%s\n",hdPtr->eventNumber,
	       numBytes,eventWriterPtr->currentEventFileName[0],
	       eventWriterPtr->currentHeaderFileName[0]);
    }


 
}

void writeOutputForTelem(int numBytes,unsigned char *outputBuffer, AnitaEventHeader_t *hdPtr, AnitaEventWriterStruct_t *eventWriterPtr) {
    int retVal;
    char headName[FILENAME_MAX];
    char bodyName[FILENAME_MAX];
    int pri=hdPtr->priority&0xf;
    if((hdPtr->turfio.trigType&0x2) && (priorityPPS1>=0 && priorityPPS1<=9))
	pri=priorityPPS1;
    if((hdPtr->turfio.trigType&0x4) && (priorityPPS2>=0 && priorityPPS2<=9))
	pri=priorityPPS2;
    if((hdPtr->turfio.trigType&0x8) && (prioritySoft>=0 && prioritySoft<=9))
	pri=prioritySoft;
    if(pri<0 || pri>9) pri=9;
    //this step is now done in Prioritizerd
//    hdPtr->priority=(16*hdPtr->priority)+pri;
    if(printToScreen && verbosity>1) {
	printf("Event %u, (%d bytes) \n",hdPtr->eventNumber,numBytes);
    }

    if(telemEvent(hdPtr->turfio.trigType)) {
      
      if(!eventWriterPtr->justHeader) {
	sprintf(bodyName,"%s/ev_%u.dat",eventTelemDirs[pri],hdPtr->eventNumber);
	sprintf(headName,"%s/hd_%u.dat",eventTelemDirs[pri],hdPtr->eventNumber);
	retVal=normalSingleWrite((unsigned char*)outputBuffer,bodyName,numBytes);
	if(retVal<0) {
	  printf("Something wrong while writing %s\n",bodyName);
	}
	else {
	  writeStruct(hdPtr,headName,sizeof(AnitaEventHeader_t));
	  makeLink(headName,eventTelemLinkDirs[pri]);
	} 
      }
    }
}



void prepEventWriterStruct(AnitaEventWriterStruct_t *eventWriterPtr) {
    int diskInd;
    closeEventFilesAndTidy(eventWriterPtr); //Just to be safe

    //Event Writer
    strncpy(eventWriterPtr->relBaseName,EVENT_ARCHIVE_DIR,FILENAME_MAX-1);
    strncpy(eventWriterPtr->filePrefix,getFilePrefix(onboardStorageType),FILENAME_MAX-1);
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	eventWriterPtr->currentHeaderFilePtr[diskInd]=0;
	eventWriterPtr->currentEventFilePtr[diskInd]=0;
    }
    eventWriterPtr->helium1CloneMask=helium1CloneMask;
    eventWriterPtr->helium2CloneMask=helium2CloneMask;
    eventWriterPtr->usbCloneMask=usbCloneMask;   
    eventWriterPtr->gotData=0;
    eventWriterPtr->writeBitMask=eventDiskBitMask;
}

void prepIndexWriter() {
    //Index Writer
    int diskInd;
    strncpy(indexWriter.relBaseName,ANITA_INDEX_DIR,FILENAME_MAX-1);
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	indexWriter.currentFilePtr[diskInd]=0;
    }
    indexWriter.writeBitMask=0x12;
}

int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(ARCHIVED_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,ARCHIVED_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(ARCHIVED_PID_FILE);
  return 0;
}


char *getFilePrefix(ArchivedDataType_t dataType)
{
    char *string;
    switch(dataType) {
	case ARCHIVE_RAW:
	    string="ev";
	    break;
	case ARCHIVE_PEDSUBBED:
	    string="psev";
	    break;
	case ARCHIVE_ENCODED:
	    string="encev";
	    break;
	case ARCHIVE_PEDSUBBED_ENCODED:
	    string="psencev";
	    break;
	default:
	    string="unknownev";
	    break;
    }
    return (string);

}

int shouldWeThrowAway(int pri)
{
  double testVal=drand48();
  if(testVal<priorityFractionGlobalDecimate[pri])
    return 1;
  return 0;
}

int getDecimatedDiskMask(int pri, int diskMask)
{
//    return diskMask;
  double testVal;
  int diskInd;
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(diskBitMasks[diskInd]&diskMask) {
      testVal=drand48();
      switch(diskInd) {
      case 0:
	if(testVal<priorityFractionDeleteHelium1[pri])
	  diskMask &= ~diskBitMasks[diskInd];
	break;
      case 1:
	if(testVal<priorityFractionDeleteHelium2[pri])
	  diskMask &= ~diskBitMasks[diskInd];
	break;
      case 2:
	if(testVal<priorityFractionDeleteUsb[pri])
	  diskMask &= ~diskBitMasks[diskInd];
	break;
      case 3:
	if(testVal<priorityFractionDeletePmc[pri])
	  diskMask &= ~diskBitMasks[diskInd];
	break;
      case 4:
	if(testVal<priorityFractionDeleteNeobrick[pri])
	  diskMask &= ~diskBitMasks[diskInd];
	break;
      default:
	break;
      }
    }
  }
  return diskMask;
    
}

int telemEvent(int trigType) 
{
  double testVal=drand48();
  if(trigType&0x1)
    return 1;
  if(trigType&0x2) {
    if(testVal<pps1FractionTelem)
      return 1;
    return 0;
  }
  if(trigType&0x4) {
    if(testVal<pps2FractionTelem)
      return 1;
    return 0;
  }
  if(trigType&0x8) {
    if(testVal<softFractionTelem)
      return 1;
    return 0;
  }
  return 1;
}


void handleBadSigs(int sig)
{
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
    //    closeEventFilesAndTidy(&eventWriter);
    closeHkFilesAndTidy(&indexWriter);
    unlink(ARCHIVED_PID_FILE);
    syslog(LOG_INFO,"Archived terminating");
    exit(0);
}




