/*! \file Playbackd.c
  \brief The Playbackd plays back events over telemetry
    
  Novemeber 2006  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <libgen.h> //For Mac OS X

//Flightsoft Includes
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"
#include "includes/anitaCommand.h"
#include "compressLib/compressLib.h"
#include "pedestalLib/pedestalLib.h"


int readConfigFile();
int checkForRequests();
void encodeAndWriteEvent(AnitaEventHeader_t *hdPtr,PedSubbedEventBody_t *psbPtr,int pri);
void sendEvent(PlaybackRequest_t *pReq);
void startPlayback();
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);

// Global Variables
int printToScreen=0;
int verbosity=0;
int sendData=0;


//Link Directories
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];
char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];

//Encoding Buffer
unsigned char outputBuffer[MAX_WAVE_BUFFER];

char priorityPurgeDirs[NUM_PRIORITIES][FILENAME_MAX];




int main (int argc, char *argv[])
{
    int retVal,pri;

    /* Log stuff */
    char *progName=basename(argv[0]);
    	    
    retVal=sortOutPidFile(progName);
    if(retVal<0)
	return -1;

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGINT,handleBadSigs);
    signal(SIGTERM,handleBadSigs);
    signal(SIGSEGV,handleBadSigs);


    //Dont' wait for children
    signal(SIGCLD, SIG_IGN); 
    
    retVal=0;
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
	sprintf(eventTelemLinkDirs[pri],"%s/%s%d/link",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);
	sprintf(eventTelemDirs[pri],"%s/%s%d",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);

	makeDirectories(eventTelemLinkDirs[pri]);
	
	sprintf(priorityPurgeDirs[pri],"%s/pri%d",BASE_PRIORITY_PURGE_DIR,
		pri);
	makeDirectories(priorityPurgeDirs[pri]);
    }
    
    retVal=readConfigFile();
    if(retVal<0) {
	syslog(LOG_ERR,"Problem reading Playbackd.config");
	printf("Problem reading Playbackd.config\n");
	exit(1);
    }
  

    //Main Loop
    do {
	printf("Initalizing Playbackd\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading Playbackd.config");
	    printf("Problem reading Playbackd.config\n");
	    exit(1);
	}
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    if(sendData) startPlayback();
	    retVal=checkForRequests();
	    if(!retVal)
		printf("sleep\n");sleep(10);
	}
    } while(currentState==PROG_STATE_INIT);    
    unlink(PLAYBACKD_PID_FILE);
    return 0;
}

int readConfigFile() 
/* Load Playbackd config stuff */
{
    /* Config file thingies */
    char *tempString;
    int status=0;
    char* eString ;
    kvpReset();
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status = configLoad ("Playbackd.config","playbackd") ;

    if(status == CONFIG_E_OK) {
	sendData=kvpGetInt("sendData",0);
	tempString=kvpGetString("PLAYBACKD_PID_FILE");
	if(tempString) {
	    strncpy(PLAYBACKD_PID_FILE,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get PLAYBACKD_PID_FILE");
	    fprintf(stderr,"Couldn't get PLAYBACKD_PID_FILE\n");
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Playbackd.config: %s\n",eString);
    }
           
    makeDirectories(PLAYBACK_LINK_DIR);
    return status;
}


int checkForRequests() {
    int count,retVal;
//    unsigned int eventNumber;
    char fileName[FILENAME_MAX];
    char linkName[FILENAME_MAX];
    struct dirent **linkList;
    PlaybackRequest_t pReq;
    int numLinks=getListofLinks(PLAYBACK_LINK_DIR,&linkList);
    if(numLinks>0) {
	fprintf(stderr,"Playbackd has received %d requests\n",numLinks);
	for(count=0;count<numLinks;count++) {
	    sprintf(linkName,"%s/%s",PLAYBACK_LINK_DIR,linkList[count]->d_name);
	    sprintf(fileName,"%s/%s",PLAYBACK_DIR,linkList[count]->d_name);

	    
	    FILE *fp=fopen(fileName,"rb");
	    if(!fp) {
		removeFile(linkName);
		removeFile(fileName);
		continue;
	    }
	    retVal=fread(&pReq,sizeof(PlaybackRequest_t),1,fp);
	    if(retVal<0) {
		removeFile(linkName);
		removeFile(fileName);
		continue;
	    }
//	    fprintf(stderr,"Here\n");
	    sendEvent(&pReq);
	    removeFile(linkName);
	    removeFile(fileName);
	}
	for(count=0;count<numLinks;
	    count++) 
	    free(linkList[count]);		    
	free(linkList);		
    }
    return 0;
}

void sendEvent(PlaybackRequest_t *pReq)
{
    AnitaEventHeader_t theHead;
    PedSubbedEventBody_t psBody;
    char indexName[FILENAME_MAX];
    char eventFileName[FILENAME_MAX];
    char headerFileName[FILENAME_MAX];
    int dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR)*(pReq->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR));
    int subDirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR)*(pReq->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR));
    int fileNum=(EVENTS_PER_FILE)*(pReq->eventNumber/EVENTS_PER_FILE);

    sprintf(indexName,"/mnt/data/anita/index/ev%d/ev%d/index_%d.dat.gz",subDirNum,dirNum,fileNum);
    
    syslog(LOG_INFO,"Trying to send event %u, with priority %d\n",
	   pReq->eventNumber,pReq->pri);
    fprintf(stderr,"Trying to send event %u, with priority %d\n",
	   pReq->eventNumber,pReq->pri);
    
    int numSeek=pReq->eventNumber-fileNum;
    numSeek--;
    gzFile *indFile=gzopen(indexName,"rb");
    if(!indFile) {
	fprintf(stderr,"Couldn't open: %s\n",indexName);
	return;
    }
    
/*     if(numSeek) { */
/* 	gzseek(indFile,numSeek*sizeof(IndexEntry_t),SEEK_SET); */
/*     } */


    IndexEntry_t indEntry;
    int count=0;
    do {
	int test=gzread(indFile,&indEntry,sizeof(IndexEntry_t));
	if(test<0) {
	    fprintf(stderr,"Couldn't read from: %s\n",indexName);
	    gzclose(indFile);
	    return;
	}
//	printf("count %d, index %u\n",count,indEntry.eventNumber);
	count++;
    } while (indEntry.eventNumber!=pReq->eventNumber);
    gzclose(indFile);
//    printf("index: %u -- %#x\n",indEntry.eventNumber,indEntry.eventDiskBitMask);

    //Now have index
    if(1) {
	//Read it from puck
	sprintf(headerFileName,"%s/run%u/event/ev%d/ev%d/hd_%d.dat.gz",
		PUCK_DATA_MOUNT,indEntry.runNumber,subDirNum,dirNum,fileNum);
	gzFile headFile = gzopen(headerFileName,"rb");
	if(!headFile) {
	    fprintf(stderr,"Couldn't open: %s\n",headerFileName);	    
	    return;
	}
	int retVal;

	
	if(numSeek) {
	    gzseek(headFile,numSeek*sizeof(AnitaEventHeader_t),SEEK_SET);
	}

	int test=0;
	do {
	    retVal=gzread(headFile,&theHead,sizeof(AnitaEventHeader_t));
	    printf("try %d, head %u\n",test,theHead.eventNumber);
	    test++;
	}
	while(retVal>0 && theHead.eventNumber!=pReq->eventNumber);
	gzclose(headFile);
	if(retVal<0) {
	    fprintf(stderr,"Couldn't find header %u\n",pReq->eventNumber);	    
	    return;
	}

	sprintf(eventFileName,"%s/run%u/event/ev%d/ev%d/psev_%d.dat.gz",
		PUCK_DATA_MOUNT,indEntry.runNumber,subDirNum,dirNum,fileNum);
	gzFile eventFile = gzopen(eventFileName,"rb");
	if(!eventFile) {
	    fprintf(stderr,"Couldn't open: %s\n",eventFileName);	    
	    return;
	}
//	int numSeek=pReq->eventNumber-fileNum;
//	numSeek--;
	if(numSeek) {
	    gzseek(eventFile,numSeek*sizeof(PedSubbedEventBody_t),SEEK_SET);
	}
	do {
	    retVal=gzread(eventFile,&psBody,sizeof(PedSubbedEventBody_t));
	}
	while(retVal>0 && psBody.eventNumber!=pReq->eventNumber);	
	gzclose(eventFile);
	if(retVal<0) {
	    fprintf(stderr,"Couldn't find event %u\n",pReq->eventNumber);	    
	    return;
	}
    }

    theHead.priority=((pReq->pri&0xf) | (theHead.priority&0xf0));
    fillGenericHeader(&theHead,PACKET_HD,sizeof(AnitaEventHeader_t));
    printf("head: %u, event: %u\n",theHead.eventNumber,psBody.eventNumber);
//    int samp=0;
//    for(samp=0;samp<260;samp++) {
//	printf("%d --%d\n",samp,psBody.channel[0].data[samp]);
//    }
    //In theory should have header and event now
    encodeAndWriteEvent(&theHead,&psBody, pReq->pri);

}

void encodeAndWriteEvent(AnitaEventHeader_t *hdPtr,
			 PedSubbedEventBody_t *psbPtr,
			 int pri)
{
    memset(outputBuffer,0,MAX_WAVE_BUFFER);
    EncodeControlStruct_t telemEncCntl;
    int surf,chan;
    int retVal,numBytes;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    telemEncCntl.encTypes[surf][chan]=ENCODE_LOSSLESS_BINFIB_COMBO;
	    if(chan==8)
		telemEncCntl.encTypes[surf][chan]=ENCODE_LOSSY_MULAW_6BIT;
	}
    }
    	    
    retVal=packPedSubbedEvent(psbPtr,&telemEncCntl,outputBuffer,&numBytes);
    printf("retVal %d, numBytes %d\n",retVal,numBytes);
    char headName[FILENAME_MAX];
    char bodyName[FILENAME_MAX];
    sprintf(bodyName,"%s/ev_%u.dat",eventTelemDirs[pri],hdPtr->eventNumber);
    sprintf(headName,"%s/hd_%u.dat",eventTelemDirs[pri],hdPtr->eventNumber);
    retVal=normalSingleWrite((unsigned char*)outputBuffer,bodyName,numBytes);
    if(retVal<0) {
	printf("Something wrong while writing %s\n",bodyName);
    }
    else {
	writeHeader(hdPtr,headName);
	makeLink(headName,eventTelemLinkDirs[pri]);
	fprintf(stderr,"Think I did it\n");
    } 
}


void startPlayback() 
{
    int priority;
    int eventCount=0;
    PlaybackRequest_t pReq;
    char purgeFile[FILENAME_MAX];
    char evNumAsString[180];
    struct dirent **linkList;
    syslog(LOG_INFO,"Playbackd Starting Playback");
    printf("Playbackd Starting Playback\n");
    
    while(currentState==PROG_STATE_RUN) {
	
	int numPri0Links=countFilesInDir(eventTelemLinkDirs[0]);
//	printf("Here\n");
	if(numPri0Links<100) {
//	    printf("Here\n");
	    for(priority=0;priority<=9;priority++) {		
		
		int numLinks=getListofPurgeFiles(priorityPurgeDirs[priority],
						 &linkList);
		int i;
//		printf("Here: %d -- %s\n",numLinks,priorityPurgeDirs[priority]);
		for(i=0;i<numLinks;i++) {
		    //Loop over files;
		    sprintf(purgeFile,"%s/%s",priorityPurgeDirs[priority],linkList[i]->d_name);
//		    printf("%s\n",purgeFile);
					
		    printf("Got purgeFile: %s\n",purgeFile);
		    gzFile Purge=gzopen(purgeFile,"r");
		    
		    if(Purge) {
			
			while(1) {
			    char *test=gzgets(Purge,evNumAsString,179);
//			    printf("evNumAsString: %s\n",evNumAsString);
			    if(test==Z_NULL) break;
//			    printf("%d %d\n",test,evNumAsString);
			    pReq.eventNumber=strtoul(evNumAsString,NULL,10);
			    printf("Got string: %s num: %u\n",
				   evNumAsString,pReq.eventNumber);
			    pReq.pri=0;
//			    printf("Freda\n");
			    sendEvent(&pReq);
			    eventCount++;
//			    printf("Fred: eventCount: %d\n",eventCount);
			    if(eventCount+numPri0Links>100) {
//				printf("sleep\n");sleep(60);
				numPri0Links=countFilesInDir(eventTelemLinkDirs[0]);
				eventCount=0;
			    }
//			    printf("Fredrick\n");
			}
//			printf("Here\n");
			gzclose(Purge);
			removeFile(purgeFile);
		    }




		    if(eventCount+numPri0Links>100) break;
		}


		for(i=0;i<numLinks;i++) {
		    free(linkList[i]);	
		}	    
		free(linkList);		
	    }
	    printf("sleep\n");sleep(1);

	}
	printf("sleep\n");sleep(1);
	eventCount=0;
    }


}



void handleBadSigs(int sig)
{   
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig);  
    unlink(PLAYBACKD_PID_FILE);
    syslog(LOG_INFO,"Playbackd terminating");    
    exit(0);
}


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(PLAYBACKD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,PLAYBACKD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(PLAYBACKD_PID_FILE);
  return 0;
}
