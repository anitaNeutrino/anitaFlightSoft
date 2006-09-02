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
#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"

int readConfigFile();
int checkDisks(DiskSpaceStruct_t *dsPtr);
int checkQueues(QueueStruct_t *queuePtr);
int writeFileAndLink(MonitorStruct_t *monitorPtr);
void prepWriterStructs();


// Global Variables
int printToScreen=0;
int verbosity=0;
int monitorPeriod=60; //In seconds
char monitordPidFile[FILENAME_MAX];

//Link Directories
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];

char bladeName[FILENAME_MAX];
char usbIntName[FILENAME_MAX];
char usbExtName[FILENAME_MAX];


int hkDiskBitMask;
AnitaHkWriterStruct_t monWriter;

char *diskLocations[8]={"/tmp","/static","/home",SAFE_DATA_MOUNT,PUCK_DATA_MOUNT,BLADE_DATA_MOUNT,USBINT_DATA_MOUNT,USBEXT_DATA_MOUNT};


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

    retVal=cleverHkWrite((unsigned char*)monitorPtr,sizeof(MonitorStruct_t),
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
	tempString=kvpGetString("monitordPidFile");
	if(tempString) {
	    strncpy(monitordPidFile,tempString,FILENAME_MAX);
	    writePidFile(monitordPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get monitordPidFile");
	    fprintf(stderr,"Couldn't get monitordPidFile\n");
	}


	tempString=kvpGetString("bladeName");
	if(tempString) {
	    strncpy(bladeName,tempString,FILENAME_MAX);
	    writePidFile(bladeName);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get bladeName");
	    fprintf(stderr,"Couldn't get bladeName\n");
	}


	tempString=kvpGetString("usbIntName");
	if(tempString) {
	    strncpy(usbIntName,tempString,FILENAME_MAX);
	    writePidFile(usbIntName);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbIntName");
	    fprintf(stderr,"Couldn't get usbIntName\n");
	}



	tempString=kvpGetString("usbExtName");
	if(tempString) {
	    strncpy(usbExtName,tempString,FILENAME_MAX);
	    writePidFile(usbExtName);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbExtName");
	    fprintf(stderr,"Couldn't get usbExtName\n");
	}



	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	hkDiskBitMask=kvpGetInt("hkDiskBitMask",0);
	monitorPeriod=kvpGetInt("monitorPeriod",60);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Monitord.config: %s\n",eString);
    }
    
    if(printToScreen && verbosity>1)
	printf("Dirs:\n\t%s\n\t%s\n\t%s\n",
	       MONITOR_TELEM_DIR,
	       MONITOR_TELEM_LINK_DIR,
	       MONITOR_ARCHIVE_DIR
	       );
	       
    makeDirectories(MONITOR_TELEM_LINK_DIR);
    return status;
}

int checkDisks(DiskSpaceStruct_t *dsPtr) {
    int errFlag=0; 
    int diskNum;   
    unsigned long megaBytes=0;
    unsigned short megaBytes_short=0;
    for(diskNum=0;diskNum<8;diskNum++) {
	megaBytes=getDiskSpace(diskLocations[diskNum]);
//	printf("%lu\n",megaBytes);
	if(megaBytes>0) {
	    megaBytes/=10;
	    if(megaBytes<65535) megaBytes_short=megaBytes;
	    else megaBytes_short=65535;
	}
	else megaBytes_short=0;
	dsPtr->diskSpace[diskNum]=megaBytes_short;
	if(printToScreen) printf("%s\t%u\n",diskLocations[diskNum],megaBytes_short);
	if(((short)megaBytes)==-1) errFlag--;
    }    
    strncpy(dsPtr->bladeLabel,bladeName,9);
    strncpy(dsPtr->usbIntLabel,usbIntName,9);
    strncpy(dsPtr->usbExtLabel,usbExtName,9);
    if(printToScreen) {
	printf("usbInt -- %s\nusbExt -- %s\nblade -- %s\n",
	       usbIntName,usbExtName,bladeName);
    }
	       

//    printf("Err Flag %d\n",errFlag);
    return errFlag;
}



int checkQueues(QueueStruct_t *queuePtr) {
    int retVal=0,count;
    unsigned short numLinks=0;
    numLinks=countFilesInDir(LOSD_CMD_ECHO_TELEM_LINK_DIR);
    queuePtr->cmdLinksLOS=numLinks;
    if(printToScreen) printf("%s\t%u\n",LOSD_CMD_ECHO_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;

    numLinks=countFilesInDir(SIPD_CMD_ECHO_TELEM_LINK_DIR);
    queuePtr->cmdLinksSIP=numLinks;
    if(printToScreen) printf("%s\t%u\n",SIPD_CMD_ECHO_TELEM_LINK_DIR,numLinks);
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

    numLinks=countFilesInDir(PEDESTAL_TELEM_LINK_DIR);
    queuePtr->pedestalLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",PEDESTAL_TELEM_LINK_DIR,numLinks);
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
    int diskInd;
    if(printToScreen) 
	printf("Preparing Writer Structs\n");
    //Hk Writer

    sprintf(monWriter.relBaseName,"%s/",MONITOR_ARCHIVE_DIR);
    sprintf(monWriter.filePrefix,"mon");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	monWriter.currentFilePtr[diskInd]=0;
    monWriter.writeBitMask=hkDiskBitMask;
}

