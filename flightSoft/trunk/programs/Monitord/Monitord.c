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
void prepWriterStructs();

// Global Variables
int printToScreen=0;
int verbosity=0;
int monitorPeriod=60; //In seconds

//USB Disk Things
int useUSBDisks=0;
char mainDataDisk[FILENAME_MAX];
char usbDataDiskLink[FILENAME_MAX];
char usbDataDiskPrefix[FILENAME_MAX];

//Link Directories
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];


char monitordArchiveDir[FILENAME_MAX];
char monitordUSBArchiveDir[FILENAME_MAX];

AnitaWriterStruct_t monWriter;

int main (int argc, char *argv[])
{
    int retVal,pri;
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
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
	sprintf(eventTelemLinkDirs[pri],"%s/%s%d/link",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);
	makeDirectories(eventTelemLinkDirs[pri]);
    }
    
    retVal=readConfigFile();
    if(retVal<0) {
	syslog(LOG_ERR,"Problem reading Monitord.config");
	printf("Problem reading Monitord.config\n");
	exit(1);
    }
    prepWriterStructs();
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
	    memset(&monData,0,sizeof(MonitorStruct_t));
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
    
    fillGenericHeader(monitorPtr,PACKET_MONITOR,sizeof(MonitorStruct_t));
    
    sprintf(theFilename,"%s/mon_%ld.dat",
	    MONITOR_TELEM_DIR,monitorPtr->unixTime);
    retVal=writeMonitor(monitorPtr,theFilename);
    retVal=makeLink(theFilename,MONITOR_TELEM_LINK_DIR);

    retVal=cleverHkWrite((char*)monitorPtr,sizeof(MonitorStruct_t),
			 monitorPtr->unixTime,&monWriter);
    if(retVal<0) {
	//Had an error
    }
    
    return retVal;        
}


int readConfigFile() 
/* Load Monitord config stuff */
{
    /* Config file thingies */
    char *tempString;
    int status=0;
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
	    syslog(LOG_ERR,"Couldn't get monitorArchiveSubDir");
	    fprintf(stderr,"Couldn't get monitorArchiveSubDir\n");
	    exit(0);
	}	       	
	
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Monitord.config: %s\n",eString);
    }
    
    if(printToScreen && verbosity>1)
	printf("Dirs:\n\t%s\n\t%s\n\t%s\n\t%s\n",
	       MONITOR_TELEM_DIR,
	       MONITOR_TELEM_LINK_DIR,
	       monitordArchiveDir,
	       monitordUSBArchiveDir);
	       
    return status;
}

int checkDisks(DiskSpaceStruct_t *dsPtr) {
    int errFlag=0;
    int usbNum;
    unsigned short megaBytes=0;
    char usbDir[FILENAME_MAX];
    megaBytes=getDiskSpace("/var/log");
    dsPtr->mainDisk=megaBytes;
    if(printToScreen) printf("%s\t%u\n","/var/log",megaBytes);
    if(((short)megaBytes)==-1) errFlag--;
    

    megaBytes=getDiskSpace(OTHER_DISK1);
    dsPtr->otherDisks[0]=megaBytes;
    if(printToScreen) printf("%s\t%u\n",OTHER_DISK1,megaBytes);
    megaBytes=getDiskSpace(OTHER_DISK2);
    dsPtr->otherDisks[1]=megaBytes;
    if(printToScreen) printf("%s\t%u\n",OTHER_DISK2,megaBytes);
    megaBytes=getDiskSpace(OTHER_DISK3);
    dsPtr->otherDisks[2]=megaBytes;
    if(printToScreen) printf("%s\t%u\n",OTHER_DISK3,megaBytes);
    megaBytes=getDiskSpace(OTHER_DISK4);
    dsPtr->otherDisks[3]=megaBytes;
    if(printToScreen) printf("%s\t%u\n",OTHER_DISK4,megaBytes);
    megaBytes=getDiskSpace("/tmp");
    dsPtr->otherDisks[4]=megaBytes;
    if(printToScreen) printf("%s\t%u\n","/tmp",megaBytes);


    if(useUSBDisks) {
	for(usbNum=1;usbNum<=NUM_USBDISKS;usbNum++) {
	    sprintf(usbDir,"%s%d",usbDataDiskPrefix,usbNum);
	    megaBytes=getDiskSpace(usbDir);
	    dsPtr->usbDisk[usbNum-1]=megaBytes;
	    if(printToScreen) printf("%s\t%d\n",usbDir,megaBytes);
	    if(((short)megaBytes)==-1) errFlag--;
	    
	}
    }
    

//    printf("Err Flag %d\n",errFlag);
    return errFlag;
}



int checkQueues(QueueStruct_t *queuePtr) {
    int retVal=0,count;
    unsigned short numLinks=0;
    numLinks=countFilesInDir(CMD_ECHO_TELEM_LINK_DIR);
    queuePtr->cmdLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",CMD_ECHO_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(HK_TELEM_LINK_DIR);
    queuePtr->hkLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",HK_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(GPS_TELEM_LINK_DIR);
    queuePtr->gpsLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",GPS_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(MONITOR_TELEM_LINK_DIR);
    queuePtr->monitorLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",MONITOR_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(SURFHK_TELEM_LINK_DIR);
    queuePtr->surfHkLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",SURFHK_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(TURFHK_TELEM_LINK_DIR);
    queuePtr->turfHkLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",TURFHK_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(HEADER_TELEM_LINK_DIR);
    queuePtr->headLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",HEADER_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    
    for(count=0;count<NUM_PRIORITIES;count++) {
	numLinks=countFilesInDir(eventTelemLinkDirs[count]);
	queuePtr->eventLinks[count]=numLinks;
	if(printToScreen) printf("%s\t%u\n",eventTelemLinkDirs[count]
				 ,numLinks);
	if(((short)numLinks)==-1) retVal--;
    }
    return retVal;
}



void prepWriterStructs() {
    if(printToScreen) 
	printf("Preparing Writer Struct\n");

    strncpy(monWriter.baseDirname,monitordArchiveDir,FILENAME_MAX-1);
    sprintf(monWriter.filePrefix,"mon");
    monWriter.currentFilePtr=0;
    monWriter.maxSubDirsPerDir=HK_FILES_PER_DIR;
    monWriter.maxFilesPerDir=HK_FILES_PER_DIR;
    monWriter.maxWritesPerFile=HK_PER_FILE;
}
