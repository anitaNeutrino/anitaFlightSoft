/*! \file Monitord.c
  \brief The Monitord program monitors CPU status
    
  Monitord monitors the status of disk space, queue lengths and a few other
  resources. Will also be responsible for switching between the USB drives.
  December 2005  rjn@mps.ohio-state.edu
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
#include "anitaStructures.h"
#include "anitaFlight.h"

int readConfigFile();
int checkDisks(DiskSpaceStruct_t *dsPtr);
int checkQueues(QueueStruct_t *queuePtr);
int writeFileAndLink(MonitorStruct_t *monitorPtr);

// Global Variables
int printToScreen=0;
int verbosity=0;
int monitorPeriod=60; //In seconds

//USB Disk Things
int useUSBDisks=1;
char mainDataDisk[FILENAME_MAX];
char usbDataDiskLink[FILENAME_MAX];
char usbDataDiskPrefix[FILENAME_MAX];

//Link Directories
char cmdTelemLinkDir[FILENAME_MAX];
char hkTelemLinkDir[FILENAME_MAX];
char gpsTelemLinkDir[FILENAME_MAX];
//char monitordTelemLinkDir[FILENAME_MAX];
char eventTelemDirBase[FILENAME_MAX];

//Telemetry and Archiving Directories
char monitordTelemDir[FILENAME_MAX];
char monitordTelemLinkDir[FILENAME_MAX];
char monitordArchiveDir[FILENAME_MAX];
char monitordUSBArchiveDir[FILENAME_MAX];


int main (int argc, char *argv[])
{
    int retVal;
//    DiskSpaceStruct_t diskSpace;
    MonitorStruct_t monData;


    /* Log stuff */
    char *progName=basename(argv[0]);
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
   
    retVal=0;
    monData.gHdr.code=PACKET_MONITOR;
    monData.gHdr.numBytes=sizeof(MonitorStruct_t);

    /* Main monitor loop. */
    do {
	if(printToScreen) printf("Initalizing Monitord\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading Monitord.config");
	    printf("Problem reading Monitord.config\n");
	    exit(1);
	}
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    //Something
//	    checkDiskSpace("/home/rjn/flightSoft/programs/Monitord/");
	    
	    monData.unixTime=(long)time(NULL);
	    retVal=checkDisks(&(monData.diskInfo));	    
	    //Do something
	    retVal=checkQueues(&(monData.queueInfo));
	    writeFileAndLink(&monData);
//	    exit(0);
//	    usleep(1);
	    sleep(monitorPeriod);
	}
    } while(currentState==PROG_STATE_INIT);    
    return 0;
}

int writeFileAndLink(MonitorStruct_t *monitorPtr) {
    char theFilename[FILENAME_MAX];
    int retVal=0;
    sprintf(theFilename,"%s/mon_%ld.dat",
	    monitordTelemDir,monitorPtr->unixTime);
    retVal=writeMonitor(monitorPtr,theFilename);
    retVal=makeLink(theFilename,monitordTelemLinkDir);
    sprintf(theFilename,"%s/mon_%ld.dat",
	    monitordArchiveDir,monitorPtr->unixTime);
    retVal=writeMonitor(monitorPtr,theFilename);
    if(useUSBDisks) {
	sprintf(theFilename,"%s/mon_%ld.dat",
		monitordUSBArchiveDir,monitorPtr->unixTime);
	retVal=writeMonitor(monitorPtr,theFilename);
    }
    return retVal;        
}


int readConfigFile() 
/* Load Monitord config stuff */
{
    /* Config file thingies */
    char *tempString;
    char tempDir[FILENAME_MAX];
    int status=0,count=0;
    char* eString ;
    kvpReset();
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("Monitord.config","monitord") ;
    status += configLoad ("Monitord.config","output") ;

    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	useUSBDisks=kvpGetInt("useUSBDisks",0);
	monitorPeriod=kvpGetInt("monitorPeriod",60);
	//Disk locations
	tempString=kvpGetString("mainDataDisk");
	if(tempString) {
	    strncpy(mainDataDisk,tempString,FILENAME_MAX-1);	    
	}
	else {
	    syslog(LOG_ERR,"Couldn't get mainDataDisk");
	    fprintf(stderr,"Couldn't get mainDataDisk\n");
	    exit(0);
	}
	tempString=kvpGetString("usbDataDiskLink");
	if(tempString) {
	    strncpy(usbDataDiskLink,tempString,FILENAME_MAX-1);	    
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbDataDiskLink");
	    fprintf(stderr,"Couldn't get usbDataDiskLink\n");
	    exit(0);
	}
	tempString=kvpGetString("usbDataDiskPrefix");
	if(tempString) {
	    strncpy(usbDataDiskPrefix,tempString,FILENAME_MAX-1);	    
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbDataDiskPrefix");
	    fprintf(stderr,"Couldn't get usbDataDiskPrefix\n");
	    exit(0);
	}



	//Output and Link Directories
	tempString=kvpGetString("baseHouseTelemDir");
	if(tempString) {
	    strncpy(monitordTelemDir,tempString,FILENAME_MAX-1);
	    strncpy(cmdTelemLinkDir,tempString,FILENAME_MAX-1);
	    strncpy(hkTelemLinkDir,tempString,FILENAME_MAX-1);
	    strncpy(gpsTelemLinkDir,tempString,FILENAME_MAX-1);	    
	}
	else {
	    syslog(LOG_ERR,"Couldn't get baseHouseTelemDir");
	    fprintf(stderr,"Couldn't get baseHouseTelemDir\n");
	    exit(0);
	}
	tempString=kvpGetString("monitorTelemSubDir");
	if(tempString) {
	    strcat(monitordTelemDir,tempString);	    
	    sprintf(monitordTelemLinkDir,"%s/link",monitordTelemDir);
	    makeDirectories(monitordTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get monitorTelemSubDir");
	    fprintf(stderr,"Couldn't get monitorTelemSubDir\n");
	    exit(0);
	}
	tempString=kvpGetString("cmdEchoTelemSubDir");
	if(tempString) {
	    strcat(cmdTelemLinkDir,tempString);
	    strcat(cmdTelemLinkDir,"/link");
	    makeDirectories(cmdTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get cmdEchoTelemSubDir");
	    fprintf(stderr,"Couldn't get cmdEchoTelemSubDir\n");
	    exit(0);
	}	
	tempString=kvpGetString("hkTelemSubDir");
	if(tempString) {
	    strcat(hkTelemLinkDir,tempString);
	    strcat(hkTelemLinkDir,"/link");
	    makeDirectories(hkTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get hkTelemSubDir");
	    fprintf(stderr,"Couldn't get hkTelemSubDir\n");
	    exit(0);
	}	
	tempString=kvpGetString("gpsTelemSubDir");
	if(tempString) {
	    strcat(gpsTelemLinkDir,tempString);
	    strcat(gpsTelemLinkDir,"/link");
	    makeDirectories(gpsTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsTelemSubDir");
	    fprintf(stderr,"Couldn't get gpsTelemSubDir\n");
	    exit(0);
	}
	tempString=kvpGetString("baseEventTelemDir");
	if(tempString) {
	    strncpy(eventTelemDirBase,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get baseEventTelemDir");
	    fprintf(stderr,"Couldn't get baseEventTelemDir\n");
	    exit(0);
	}
	tempString=kvpGetString("eventTelemSubDirPrefix");
	if(tempString) {
	    strcat(eventTelemDirBase,"/");
	    strcat(eventTelemDirBase,tempString);
	    for(count=0;count<NUM_PRIORITIES;count++) {
		sprintf(tempDir,"%s%d/link",eventTelemDirBase,count);
		makeDirectories(tempDir);
	    }
	}
	else {
	    syslog(LOG_ERR,"Couldn't get eventTelemSubDirPrefix");
	    fprintf(stderr,"Couldn't get eventTelemSubDirPrefix\n");
	    exit(0);
	}
	

	tempString=kvpGetString("baseHouseArchiveDir");
	if(tempString) {
	    sprintf(monitordArchiveDir,"%s/%s/",mainDataDisk,tempString);
	    sprintf(monitordUSBArchiveDir,"%s/%s/",
		    usbDataDiskLink,tempString);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get baseHouseArchiveDir");
	    fprintf(stderr,"Couldn't get baseHouseArchiveDir\n");
	    exit(0);
	}
	tempString=kvpGetString("monitorArchiveSubDir");
	if(tempString) {
	    strcat(monitordArchiveDir,tempString);
	    strcat(monitordUSBArchiveDir,tempString);	    
	    makeDirectories(monitordArchiveDir);	    
	    makeDirectories(monitordUSBArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get monitorTelemSubDir");
	    fprintf(stderr,"Couldn't get monitorTelemSubDir\n");
	    exit(0);
	}	       	
	
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Monitord.config: %s\n",eString);
    }
    
    if(printToScreen && verbosity>1)
	printf("Dirs:\n\t%s\n\t%s\n\t%s\n\t%s\n",
	       monitordTelemDir,
	       monitordTelemLinkDir,
	       monitordArchiveDir,
	       monitordUSBArchiveDir);
	       
    return status;
}

int checkDisks(DiskSpaceStruct_t *dsPtr) {
    int errFlag=0;
    int usbNum;
    unsigned short megaBytes=0;
    char usbDir[FILENAME_MAX];
    megaBytes=getDiskSpace(mainDataDisk);
    dsPtr->mainDisk=megaBytes;
    if(printToScreen) printf("%s\t%u\n",mainDataDisk,megaBytes);
    if(((short)megaBytes)==-1) errFlag--;
    
    for(usbNum=1;usbNum<=NUM_USBDISKS;usbNum++) {
	sprintf(usbDir,"%s%d",usbDataDiskPrefix,usbNum);
	megaBytes=getDiskSpace(usbDir);
	dsPtr->usbDisk[usbNum-1]=megaBytes;
	if(printToScreen) printf("%s\t%d\n",usbDir,megaBytes);
	if(((short)megaBytes)==-1) errFlag--;

    }
//    printf("Err Flag %d\n",errFlag);
    return errFlag;
}



int checkQueues(QueueStruct_t *queuePtr) {
    int retVal=0,count;
    unsigned short numLinks=0;
    char tempDir[FILENAME_MAX];
    numLinks=countFilesInDir(cmdTelemLinkDir);
    queuePtr->cmdLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",cmdTelemLinkDir,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(hkTelemLinkDir);
    queuePtr->hkLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",hkTelemLinkDir,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(gpsTelemLinkDir);
    queuePtr->gpsLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",gpsTelemLinkDir,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(monitordTelemLinkDir);
    queuePtr->monitorLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",monitordTelemLinkDir,numLinks);
    if(((short)numLinks)==-1) retVal--;
    
    for(count=0;count<NUM_PRIORITIES;count++) {
	sprintf(tempDir,"%s%d/link",eventTelemDirBase,count);
	numLinks=countFilesInDir(tempDir);
	queuePtr->eventLinks[count]=numLinks;
    if(printToScreen) printf("%s\t%u\n",tempDir,numLinks);
	if(((short)numLinks)==-1) retVal--;
    }
    return retVal;
}
