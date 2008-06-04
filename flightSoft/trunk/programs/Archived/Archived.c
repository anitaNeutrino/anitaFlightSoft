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
float priorityFractionDelete[NUM_PRIORITIES];
int priDiskEventMask[NUM_PRIORITIES];
int alternateUsbs[NUM_PRIORITIES];
int currentUsb[NUM_PRIORITIES]; // 0 is int, 1 is ext;
int currentRun;

char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];


//Event Structures
AnitaEventHeader_t theHead;
AnitaEventBody_t theBody;
PedSubbedEventBody_t pedSubBody;

//Encoding Buffer
unsigned char outputBuffer[MAX_WAVE_BUFFER];

int printToScreen=0;
int verbosity=0;
int writeTelem=0;

int eventDiskBitMask;
int satabladeCloneMask;
int sataminiCloneMask;
int usbCloneMask;
int priorityPPS1;
int priorityPPS2;
float pps1FractionDelete=0;
float pps2FractionDelete=0;
AnitaEventWriterStruct_t eventWriter;
AnitaHkWriterStruct_t indexWriter;

char satabladeName[FILENAME_MAX];
char usbName[FILENAME_MAX];
char sataminiName[FILENAME_MAX];

int usbBitMasks[2]={0x4,0x8};

FILE *fpHead=NULL;
FILE *fpEvent=NULL;

int shouldWeThrowAway(int pri);
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);

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
    eString = configErrorString (status) ;
    if (status == CONFIG_E_OK) {
	eventDiskBitMask=kvpGetInt("eventDiskBitMask",1);
	satabladeCloneMask=kvpGetInt("satabladeCloneMask",0);
	sataminiCloneMask=kvpGetInt("sataminiCloneMask",0);
	usbCloneMask=kvpGetInt("usbCloneMask",0);
	tempString=kvpGetString("satabladeName");
	if(tempString) {
	    strncpy(satabladeName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get satabladeName");
	    fprintf(stderr,"Couldn't get satabladeName\n");
	}


	tempString=kvpGetString("usbName");
	if(tempString) {
	    strncpy(usbName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbName");
	    fprintf(stderr,"Couldn't get usbName\n");
	}



	tempString=kvpGetString("sataminiName");
	if(tempString) {
	    strncpy(sataminiName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get sataminiName");
	    fprintf(stderr,"Couldn't get sataminiName\n");
	}



    }
    
    //Fill event dir names
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
	currentUsb[pri]=0;
	sprintf(eventTelemDirs[pri],"%s/%s%d",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);
	sprintf(eventTelemLinkDirs[pri],"%s/link",eventTelemDirs[pri]);
	makeDirectories(eventTelemLinkDirs[pri]);
    }

    currentRun=getRunNumber();

    makeDirectories(ANITA_INDEX_DIR);
    makeDirectories(PRIORITIZERD_EVENT_LINK_DIR);
    retVal=0;
    /* Main event getting loop. */
    do {
	if(printToScreen) printf("Initalizing Archived\n");
	retVal=readConfigFile();
	prepWriterStructs();


	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading Archived.config");
	    printf("Problem reading Archived.config\n");
	}
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    checkEvents();
	    usleep(1000);
	}
    } while(currentState==PROG_STATE_INIT);    

    closeEventFilesAndTidy(&eventWriter);
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
	priorityPPS1=kvpGetInt("priorityPPS1",3);
	priorityPPS2=kvpGetInt("priorityPPS2",2);
	pps1FractionDelete=kvpGetFloat("pps1FractionDelete",0);
	pps2FractionDelete=kvpGetFloat("pps2FractionDelete",0);
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


	tempNum=10;
	kvpStatus = kvpGetFloatArray("priorityFractionDelete",
				     priorityFractionDelete,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetFloatArray(priorityFractionDelete): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetFloatArray(priorityFractionDelete): %s\n",
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


	tempNum=10;
	kvpStatus = kvpGetIntArray("alternateUsbs",
				   alternateUsbs,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(alternateUsbs): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(alternateUsbs): %s\n",
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
    char currentHeadname[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    char currentBodyname[FILENAME_MAX];
//    char currentPSBodyname[FILENAME_MAX];

    if(wd==0) {
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
    if(printToScreen && verbosity) printf("Found %d links\n",numLinks);
    eventWriter.justHeader=0;
    for(count=0;count<numLinks;count++) {
      tempString=getFirstLink(wd);
      
      
      sprintf(currentHeadname,"%s/%s",PRIORITIZERD_EVENT_DIR,tempString);
      sprintf(currentLinkname,"%s/%s",PRIORITIZERD_EVENT_LINK_DIR,tempString);
      retVal=fillHeader(&theHead,currentHeadname);

      //Switch to
      sprintf(currentBodyname,"%s/psev_%u.dat",PRIORITIZERD_EVENT_DIR,
	      theHead.eventNumber);
      if(!shouldWeThrowAway(theHead.priority&0xf) ) {
	    
	
	retVal=fillPedSubbedBody(&pedSubBody,currentBodyname);
	//	sprintf(currentPSBodyname,"%s/psev_%u.dat",PRIORITIZERD_EVENT_DIR,
	//		theHead.eventNumber);
	//	retVal=fillPedSubbedBody(&pedSubBody,currentPSBodyname);
	//	printf("Event %u, Body %u, PS Body %u\n",theHead.eventNumber,
	//	       theBody.eventNumber,pedSubBody.eventNumber);
	
	//Subtract Pedestals
	//	subtractCurrentPeds(&theBody,&pedSubBody);
	
	
	processEvent();
      }
      else {
	eventWriter.justHeader=1;
	processEvent();	
      }

      removeFile(currentLinkname);
      removeFile(currentBodyname);
      //	removeFile(currentPSBodyname);
      removeFile(currentHeadname);
    }
}

void processEvent() 
// This just fills the buffer with 9 (well ACTIVE_SURFS) EncodedSurfPacketHeader_t and their associated waveforms.
{

    CompressErrorCode_t retVal=0;
    EncodeControlStruct_t diskEncCntl;
    EncodeControlStruct_t telemEncCntl;
    int surf,chan,numBytes;

    //Stuff for satamini
    //    static int errorCounter=0;
    //    static int fileEpoch=0;
    //    static int fileNum=0;
    //    static int dirNum=0;
    //    static int otherDirNum=0;
    //    static char sataminiDirName[FILENAME_MAX];
    //    static char sataminiHeaderFileName[FILENAME_MAX];
    //    static char sataminiEventFileName[FILENAME_MAX];
    

    int priority=(theHead.priority&0xf);
    if(priority<0 || priority>9) priority=9;
    int thisBitMask=eventDiskBitMask&priDiskEventMask[priority];
    if(alternateUsbs[priority]) {
	int notMask= ~(usbBitMasks[currentUsb[priority]]);
	thisBitMask &= notMask;
	if(currentUsb[priority]==0) currentUsb[priority]=1;
	else currentUsb[priority]=0;
    }
    if((eventDiskBitMask&0x10) && (priDiskEventMask[priority]&0x10))
	thisBitMask |= 0x10;
    eventWriter.writeBitMask=thisBitMask;
    if(printToScreen) {
      printf("\nEvent %u, priority %d\n",theHead.eventNumber,
	     theHead.priority);
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
    if(!eventWriter.justHeader) {
	memset(outputBuffer,0,MAX_WAVE_BUFFER);
	switch(onboardStorageType) {
	case ARCHIVE_RAW:
	      //No longer an option
	      //		memcpy(outputBuffer,&theBody,sizeof(AnitaEventBody_t));
	      //		numBytes=sizeof(AnitaEventBody_t);
	      //		retVal=COMPRESS_E_OK;
	      //		break;
	case ARCHIVE_PEDSUBBED:
	  memcpy(outputBuffer,&pedSubBody,sizeof(PedSubbedEventBody_t));
	  numBytes=sizeof(PedSubbedEventBody_t);
	  retVal=COMPRESS_E_OK;
	  break;
	case ARCHIVE_ENCODED:	    
	  //Again no longer an option
	  //		retVal=packEvent(&theBody,&diskEncCntl,outputBuffer,&numBytes);
	  //		break;
	case ARCHIVE_PEDSUBBED_ENCODED:	    
	  retVal=packPedSubbedEvent(&pedSubBody,&diskEncCntl,outputBuffer,&numBytes);
	  break;
	default:
	  memcpy(outputBuffer,&pedSubBody,sizeof(PedSubbedEventBody_t));
	  numBytes=sizeof(PedSubbedEventBody_t);
	  retVal=COMPRESS_E_OK;
	  //memcpy(outputBuffer,&theBody,sizeof(AnitaEventBody_t));
	  //numBytes=sizeof(AnitaEventBody_t);
	  //retVal=COMPRESS_E_OK;
	  break;	    
	}
    }
    else numBytes=0;

    if(retVal==COMPRESS_E_OK || eventWriter.justHeader) {
/* 	if(eventWriter.writeBitMask & SATAMINI_DISK_MASK) { */
/* 	    //Now write to satamini */
/* 	    if(pedSubBody.eventNumber>fileEpoch) { */
/* 		if(fileEpoch) { */
/* 		    //Close file and zip */
/* 		    if(fpHead) { */
/* 			fclose(fpHead); */
/* 			zipFileInPlace(sataminiHeaderFileName); */
/* 			fpHead=NULL; */
/* 		    } */
/* 		    if(fpEvent) { */
/* 			fclose(fpEvent); */
/* 			zipFileInPlace(sataminiEventFileName); */
/* 			fpEvent=NULL; */
/* 		    }			 */
/* 		} */
/* 		//Need to make files */
/* 		dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR)*(pedSubBody.eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR)); */
/* 		//Make sub dir */
/* 		otherDirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR)*(pedSubBody.eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR)); */
/* 		sprintf(sataminiDirName,"%s/current/event/ev%d/ev%d",SATAMINI_DATA_MOUNT,dirNum,otherDirNum); */
/* 		makeDirectories(sataminiDirName); */
		
/* 		//Make files */
/* 		fileNum=(EVENTS_PER_FILE)*(pedSubBody.eventNumber/EVENTS_PER_FILE); */
/* 		sprintf(sataminiEventFileName,"%s/psev_%d.dat",sataminiDirName,fileNum); */
/* 		sprintf(sataminiHeaderFileName,"%s/hd_%d.dat",sataminiDirName,fileNum); */
/* 		fileEpoch=fileNum+EVENTS_PER_FILE; */
/* 		fpHead=fopen(sataminiHeaderFileName,"ab"); */
/* 		fpEvent=fopen(sataminiEventFileName,"ab"); */
/* 	    } */
	    
/* 	    if(fpHead) { */
/* 		retVal=fwrite(&theHead,sizeof(AnitaEventHeader_t),1,fpHead); */
/* 		if(retVal<0) { */
/* 		    errorCounter++; */
/* 		    printf("Error (%d of 100) writing to file -- %s (%d)\n", */
/* 			   errorCounter, */
/* 			   strerror(errno),retVal); */
/* 		} */
/* 		else  */
/* 		    fflush(fpHead); */
/* 	    }  */
/* 	    if(fpEvent) { */
/* 		retVal=fwrite(&pedSubBody,sizeof(PedSubbedEventBody_t),1,fpEvent); */
/* 		if(retVal<0) { */
/* 		    errorCounter++; */
/* 		    printf("Error (%d of 100) writing to file -- %s (%d)\n", */
/* 			   errorCounter, */
/* 			   strerror(errno),retVal); */
/* 		} */
/* 		else  */
/* 		    fflush(fpEvent); */
/* 	    }      */
	    

/* 	} */
/* 	eventWriter.writeBitMask &= ~SATAMINI_DISK_MASK; */
	writeOutputToDisk(numBytes);
    }
    else {
	syslog(LOG_ERR,"Error compressing event %u\n",pedSubBody.eventNumber);
	fprintf(stderr,"Error compressing event %u\n",pedSubBody.eventNumber);
    }

    //3) Pack Event For Telemetry
    //Again we have a range of options for event telemetry
    if(!eventWriter.justHeader) {
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
		memcpy(outputBuffer,&pedSubBody,sizeof(PedSubbedEventBody_t));
		numBytes=sizeof(PedSubbedEventBody_t);
		retVal=COMPRESS_E_OK;
		break;
		//	    case ARCHIVE_ENCODED:	    
		//		retVal=packEvent(&theBody,&telemEncCntl,outputBuffer,&numBytes);
		//		break;
		//	    case ARCHIVE_PEDSUBBED_ENCODED:	    
		retVal=packPedSubbedEvent(&pedSubBody,&telemEncCntl,outputBuffer,&numBytes);
		break;
	}
    }
    if(writeTelem) {
	if(retVal==COMPRESS_E_OK)
	    writeOutputForTelem(numBytes);
	else {
	    syslog(LOG_ERR,"Error compressing event %u for telemetry\n",pedSubBody.eventNumber);
	    fprintf(stderr,"Error compressing event %u for telemetry\n",pedSubBody.eventNumber);
	}
    }
}


void writeOutputToDisk(int numBytes) {
    IndexEntry_t indEnt;
    int retVal;

    retVal=cleverEventWrite((unsigned char*)outputBuffer,numBytes,&theHead,&eventWriter);
    if(printToScreen && verbosity>1) {
	printf("Event %u, (%d bytes)  %s \t%s\n",theHead.eventNumber,
	       numBytes,eventWriter.currentEventFileName[0],
	       eventWriter.currentHeaderFileName[0]);
    }

    //Now write the index
    indEnt.runNumber=currentRun;
    indEnt.eventNumber=theHead.eventNumber;
    indEnt.eventDiskBitMask=eventWriter.writeBitMask;
    strncpy(indEnt.satabladeLabel,satabladeName,11);
    strncpy(indEnt.usbLabel,usbName,11);
    strncpy(indEnt.sataminiLabel,sataminiName,11);
    cleverIndexWriter(&indEnt,&indexWriter);

 
}

void writeOutputForTelem(int numBytes) {
    int retVal;
    char headName[FILENAME_MAX];
    char bodyName[FILENAME_MAX];
    int pri=theHead.priority&0xf;
    if((theHead.turfio.trigType&0x2) && (priorityPPS1>=0 && priorityPPS1<=9))
	pri=priorityPPS1;
    if((theHead.turfio.trigType&0x4) && (priorityPPS2>=0 && priorityPPS2<=9))
	pri=priorityPPS2;
    if(pri<0 || pri>9) pri=9;
    //this step is now done in Prioritizerd
//    theHead.priority=(16*theHead.priority)+pri;
    if(!eventWriter.justHeader) {
	sprintf(bodyName,"%s/ev_%u.dat",eventTelemDirs[pri],theHead.eventNumber);
	sprintf(headName,"%s/hd_%u.dat",eventTelemDirs[pri],theHead.eventNumber);
	retVal=normalSingleWrite((unsigned char*)outputBuffer,bodyName,numBytes);
	if(retVal<0) {
	    printf("Something wrong while writing %s\n",bodyName);
	}
	else {
	  writeStruct(&theHead,headName,sizeof(AnitaEventHeader_t));
	    makeLink(headName,eventTelemLinkDirs[pri]);
	} 
    }
}



void prepWriterStructs() {
    int diskInd;
    if(printToScreen) 
	printf("Preparing Writer Structs\n");
    closeEventFilesAndTidy(&eventWriter); //Just to be safe
    //Event Writer
    strncpy(eventWriter.relBaseName,EVENT_ARCHIVE_DIR,FILENAME_MAX-1);
    strncpy(eventWriter.filePrefix,getFilePrefix(onboardStorageType),FILENAME_MAX-1);
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	eventWriter.currentHeaderFilePtr[diskInd]=0;
	eventWriter.currentEventFilePtr[diskInd]=0;
    }
    eventWriter.satabladeCloneMask=satabladeCloneMask;
    eventWriter.sataminiCloneMask=sataminiCloneMask;
    eventWriter.usbCloneMask=usbCloneMask;   
    eventWriter.gotData=0;
    eventWriter.writeBitMask=eventDiskBitMask;

    //Index Writer
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
    if(testVal<priorityFractionDelete[pri])
	return 1;
    return 0;
}

void handleBadSigs(int sig)
{
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
    closeEventFilesAndTidy(&eventWriter);
    closeHkFilesAndTidy(&indexWriter);
    unlink(ARCHIVED_PID_FILE);
    syslog(LOG_INFO,"Archived terminating");
    exit(0);
}




