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

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"
#include "includes/anitaCommand.h"

int readConfigFile();
int checkForRequests();


// Global Variables
int printToScreen=0;
int verbosity=0;

char playbackdPidFile[FILENAME_MAX];


//Link Directories
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];
char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];


int main (int argc, char *argv[])
{
    int retVal,pri;

    /* Log stuff */
    char *progName=basename(argv[0]);
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    
    //Dont' wait for children
    signal(SIGCLD, SIG_IGN); 
    
    retVal=0;
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
	sprintf(eventTelemLinkDirs[pri],"%s/%s%d/link",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);
	sprintf(eventTelemDirs[pri],"%s/%s%d",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);

	makeDirectories(eventTelemLinkDirs[pri]);
    }
    
    retVal=readConfigFile();
    if(retVal<0) {
	syslog(LOG_ERR,"Problem reading Playbackd.config");
	printf("Problem reading Playbackd.config\n");
	exit(1);
    }
  

    //Main Loop
    do {
	if(printToScreen) printf("Initalizing Playbackd\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading Playbackd.config");
	    printf("Problem reading Playbackd.config\n");
	    exit(1);
	}
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    retVal=checkForRequests();
	    if(!retVal)
		sleep(10);
	}
    } while(currentState==PROG_STATE_INIT);    
    unlink(playbackdPidFile);
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

    if(status == CONFIG_E_OK) {
	tempString=kvpGetString("playbackdPidFile");
	if(tempString) {
	    strncpy(playbackdPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get playbackdPidFile");
	    fprintf(stderr,"Couldn't get playbackdPidFile\n");
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
    unsigned long eventNumber;
    char fileName[FILENAME_MAX];
    char linkName[FILENAME_MAX];
    struct dirent **linkList;
    PlaybackRequest_t pReq;
    int numLinks=getListofLinks(PLAYBACK_LINK_DIR,&linkList);
    if(numLinks>0) {
	fprintf(stderr,"Playbackd has received request\n");
	for(count=0;count<numLinks-100;count++) {
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

    sprintf(indexName,"/mnt/data/index/ev%d/ev%d/index_%d.dat.gz",dirNum,subDirNum,fileNum);
    gzFile *indFile=gzopen(indexName,"rb");
    if(!indFile) return;
    
    IndexEntry_t indEntry;
    do {
	int test=gzread(indFile,&indEntry,sizeof(IndexEntry_t));
	if(test<0) {
	    gzclose(indFile);
	    return;
	}
    } while (indEntry.eventNumber!=pReq->eventNumber);

    //Now have index
    if(indEntry.eventDiskBitMask&PUCK_DISK_MASK) {
	//Read it from puck
	sprintf(headerFileName,"%s/run%d/event/ev%d/ev%d/hd_%d.dat.gz",
		PUCK_DATA_MOUNT,indEntry.runNumber,dirNum,subDirNum,fileNum);
	gzFile headFile = gzopen(headerFileName,"rb");
	if(!headFile) return;
	int retVal;
	do {
	    retVal=gzread(headFile,&theHead,sizeof(AnitaEventHeader_t));
	}
	while(retVal>0 && theHead.eventNumber!=pReq->eventNumber);
	gzclose(headFile);
	if(retVal<0) return;

	sprintf(eventFileName,"/run%d/event/ev%d/ev%d/psev_%d.dat.gz",
		PUCK_DATA_MOUNT,indEntry.runNumber,dirNum,subDirNum,fileNum);
	gzFile eventFile = gzopen(eventFileName,"rb");
	if(!eventFile) return;
	int retVal;
	do {
	    retVal=gzread(eventFile,&psBody,sizeof(PedSubbedEventBody_t));
	}
	while(retVal>0 && psBody.eventNumber!=pReq->eventNumber);	
	gzclose(eventFile);
	if(retVal<0) return;
    }
}


