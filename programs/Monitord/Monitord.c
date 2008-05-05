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

int readConfigFile();
int checkDisks(DiskSpaceStruct_t *dsPtr);
int checkQueues(QueueStruct_t *queuePtr);
int writeFileAndLink(MonitorStruct_t *monitorPtr);
void prepWriterStructs();
void purgeDirectory(int priority);
void checkProcesses();
int getIdMask(ProgramId_t prog);
char *getPidFile(ProgramId_t prog);
char *getProgName(ProgramId_t prog);
int getRamdiskInodes();
void fillOtherStruct(OtherMonitorStruct_t *otherPtr);
int writeOtherFileAndLink(OtherMonitorStruct_t *otherPtr);
void purgeHkDirectory(char *dirName,char *linkDirName);
int getLatestRunNumber();
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);


// Global Variables
int printToScreen=0;
int verbosity=0;
int monitorPeriod=60; //In seconds
int watchProcesses=0;
int ramdiskKillAcqd=50;
int ramdiskDumpData=5;
int inodesKillAcqd=10000;
int inodesDumpData=2000;
int killedAcqd=0;
int acqdKillTime=0;
int maxAcqdWaitPeriod=180;
int maxEventQueueSize=300;
int maxHkQueueSize=200;

char lastRunNumberFile[FILENAME_MAX];


//
int usbSwitchMB=50;
int bladeSwitchMB=50;

//Link Directories
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];
char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];

char priorityPurgeDirs[NUM_PRIORITIES][FILENAME_MAX];

char bladeName[FILENAME_MAX];
char usbIntName[FILENAME_MAX];
char usbExtName[FILENAME_MAX];


int hkDiskBitMask;
int eventDiskBitMask;
AnitaHkWriterStruct_t monWriter;
AnitaHkWriterStruct_t otherMonWriter;


char *diskLocations[8]={"/tmp","/static","/home",SAFE_DATA_MOUNT,PUCK_DATA_MOUNT,BLADE_DATA_MOUNT,USBINT_DATA_MOUNT,USBEXT_DATA_MOUNT};
char *otherDirLoc[3]={ACQD_EVENT_DIR,EVENTD_EVENT_DIR,PRIORITIZERD_EVENT_DIR};
char *otherLinkLoc[3]={ACQD_EVENT_LINK_DIR,EVENTD_EVENT_LINK_DIR,PRIORITIZERD_EVENT_LINK_DIR};

int main (int argc, char *argv[])
{
    CommandStruct_t theCmd;
    int retVal,pri;
//    DiskSpaceStruct_t diskSpace;
    MonitorStruct_t monData;
    OtherMonitorStruct_t otherData;

    /* Log stuff */
    char *progName=basename(argv[0]);
    unsigned int startTime=time(NULL);
    int bladeMountCmdTime=0;
    int usbintMountCmdTime=0;
    int usbextMountCmdTime=0;

    retVal=sortOutPidFile(progName);
    if(retVal<0)
	return -1;

    	    
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
	    monData.unixTime=(int)time(NULL);
	    retVal=checkDisks(&(monData.diskInfo));	    
	    //Do something
	    retVal=checkQueues(&(monData.queueInfo));
	    fillOtherStruct(&otherData);
	    writeFileAndLink(&monData);
	    writeOtherFileAndLink(&otherData);



	    printf("ramDiskSpace: %d mb\n",monData.diskInfo.diskSpace[0]);
	    if(!killedAcqd) {
		//Need to put inode check here as well

		if(monData.diskInfo.diskSpace[0]<ramdiskDumpData || 
		   otherData.ramDiskInodes<inodesDumpData) {		
		    theCmd.numCmdBytes=1;
		    theCmd.cmd[0]=CLEAR_RAMDISK;
		    writeCommandAndLink(&theCmd);
		}
		else if(monData.diskInfo.diskSpace[0]<ramdiskKillAcqd || 
			otherData.ramDiskInodes<inodesKillAcqd) {
		    syslog(LOG_WARNING,"Killing Acqd -- ramDisk %d MB, %u inodes",monData.diskInfo.diskSpace[0],otherData.ramDiskInodes); 
		    theCmd.numCmdBytes=3;
		    theCmd.cmd[0]=CMD_KILL_PROGS;
		    theCmd.cmd[1]=ACQD_ID_MASK;
		    theCmd.cmd[2]=0;
		    writeCommandAndLink(&theCmd);
		    killedAcqd=1;
		    acqdKillTime=monData.unixTime;
		}
	    }
	    else {
		if(monData.diskInfo.diskSpace[0]>100 || (monData.unixTime-acqdKillTime)>maxAcqdWaitPeriod) {	
		    theCmd.numCmdBytes=3;
		    theCmd.cmd[0]=CMD_START_PROGS;
		    theCmd.cmd[1]=ACQD_ID_MASK;
		    theCmd.cmd[2]=0;
		    writeCommandAndLink(&theCmd);
		    killedAcqd=0;
		}
	    }

	    printf("blade %d\t%d\n",monData.diskInfo.diskSpace[5],bladeSwitchMB);
	    if(monData.diskInfo.diskSpace[5]<bladeSwitchMB) {
		readConfigFile();
		//Change blade if in use
		if((hkDiskBitMask&BLADE_DISK_MASK || eventDiskBitMask&BLADE_DISK_MASK)) {
		    if((monData.unixTime-bladeMountCmdTime)>300) {
			theCmd.numCmdBytes=1;
			theCmd.cmd[0]=CMD_MOUNT_NEXT_BLADE;
			writeCommandAndLink(&theCmd);
			bladeMountCmdTime=monData.unixTime;
		    }
		}
	    }
	    if(monData.diskInfo.diskSpace[6]<usbSwitchMB) {
		readConfigFile();
		//Change USB int if in use
		if((hkDiskBitMask&USBINT_DISK_MASK || eventDiskBitMask&USBINT_DISK_MASK)) {
		    if((monData.unixTime-usbintMountCmdTime)>300) {
			theCmd.numCmdBytes=3;
			theCmd.cmd[0]=CMD_MOUNT_NEXT_USB;
			theCmd.cmd[1]=1;
			theCmd.cmd[2]=0;
			if(monData.diskInfo.diskSpace[7]<usbSwitchMB)
			    theCmd.cmd[1]=3;
			writeCommandAndLink(&theCmd);
			usbintMountCmdTime=monData.unixTime;
		    }
		}
	    }
	    else if(monData.diskInfo.diskSpace[7]<usbSwitchMB) {
		readConfigFile();
		//Change USB ext if in use
		if((hkDiskBitMask&USBEXT_DISK_MASK || eventDiskBitMask&USBEXT_DISK_MASK)) {
		    if((monData.unixTime-usbextMountCmdTime)>300) {
			theCmd.numCmdBytes=3;
			theCmd.cmd[0]=CMD_MOUNT_NEXT_USB;
			theCmd.cmd[1]=2;
			theCmd.cmd[2]=0;
			writeCommandAndLink(&theCmd);
		    }
		}
	    }
	    
	    if((monData.unixTime-startTime)>60) {
		checkProcesses();
	    }





//	    exit(0);
//	    usleep(1);
	    sleep(monitorPeriod);
	}
    } while(currentState==PROG_STATE_INIT);    
    closeHkFilesAndTidy(&monWriter);
    unlink(MONITORD_PID_FILE);
    return 0;
}

int writeFileAndLink(MonitorStruct_t *monitorPtr) {
    char theFilename[FILENAME_MAX];
    int retVal=0;
    
    fillGenericHeader(monitorPtr,PACKET_MONITOR,sizeof(MonitorStruct_t));
    
    sprintf(theFilename,"%s/mon_%d.dat",
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
	watchProcesses=kvpGetInt("watchProcesses",0);
	maxEventQueueSize=kvpGetInt("maxEventQueueSize",300);
	maxHkQueueSize=kvpGetInt("maxHkQueueSize",200);

	tempString=kvpGetString("bladeName");
	if(tempString) {
	    strncpy(bladeName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get bladeName");
	    fprintf(stderr,"Couldn't get bladeName\n");
	}

	tempString=kvpGetString("usbIntName");
	if(tempString) {
	    strncpy(usbIntName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbIntName");
	    fprintf(stderr,"Couldn't get usbIntName\n");
	}

	tempString=kvpGetString("usbExtName");
	if(tempString) {
	    strncpy(usbExtName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbExtName");
	    fprintf(stderr,"Couldn't get usbExtName\n");
	}

	tempString=kvpGetString("lastRunNumberFile");
	if(tempString) {
	    strncpy(lastRunNumberFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get lastRunNumberFile");
	    fprintf(stderr,"Couldn't get lastRunNumberFile\n");
	}



	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	hkDiskBitMask=kvpGetInt("hkDiskBitMask",0);
	eventDiskBitMask=kvpGetInt("eventDiskBitMask",0);
	monitorPeriod=kvpGetInt("monitorPeriod",60);
	usbSwitchMB=kvpGetInt("usbSwitchMB",50);
	bladeSwitchMB=kvpGetInt("bladeSwitchMB",50);
	ramdiskKillAcqd=kvpGetInt("ramdiskKillAcqd",50);
	ramdiskDumpData=kvpGetInt("ramdiskDumpData",5);
	maxAcqdWaitPeriod=kvpGetInt("maxAcqdWaitPeriod",180);
	inodesKillAcqd=kvpGetInt("inodesKillAcqd",10000);
	inodesDumpData=kvpGetInt("inodesDumpData",2000);
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
    makeDirectories(OTHER_MONITOR_TELEM_LINK_DIR);
    makeDirectories(CMDD_COMMAND_LINK_DIR);
    return status;

}

int checkDisks(DiskSpaceStruct_t *dsPtr) {
    int errFlag=0; 
    int diskNum;   
    unsigned int megaBytes=0;
    unsigned short megaBytes_short=0;
    for(diskNum=0;diskNum<8;diskNum++) {
	megaBytes=getDiskSpace(diskLocations[diskNum]);
//	printf("%u\n",megaBytes);
	if(megaBytes>0) {
	    if(diskNum==4)
		megaBytes/=16;
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
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(LOSD_CMD_ECHO_TELEM_DIR,LOSD_CMD_ECHO_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(SIPD_CMD_ECHO_TELEM_LINK_DIR);
    queuePtr->cmdLinksSIP=numLinks;
    if(printToScreen) printf("%s\t%u\n",SIPD_CMD_ECHO_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(SIPD_CMD_ECHO_TELEM_DIR,SIPD_CMD_ECHO_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(HK_TELEM_LINK_DIR);
    queuePtr->hkLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",HK_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(HK_TELEM_DIR,HK_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(G12_POS_TELEM_LINK_DIR);
    queuePtr->gpsLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",G12_POS_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(G12_POS_TELEM_DIR,G12_POS_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(G12_SAT_TELEM_LINK_DIR);
    queuePtr->gpsLinks+=numLinks;
    if(printToScreen) printf("%s\t%u\n",G12_SAT_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(G12_SAT_TELEM_LINK_DIR,G12_SAT_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(ADU5_PAT_TELEM_LINK_DIR);
    queuePtr->gpsLinks+=numLinks;
    if(printToScreen) printf("%s\t%u\n",ADU5_PAT_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(ADU5_PAT_TELEM_DIR,ADU5_PAT_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(ADU5_VTG_TELEM_LINK_DIR);
    queuePtr->gpsLinks+=numLinks;
    if(printToScreen) printf("%s\t%u\n",ADU5_VTG_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(ADU5_VTG_TELEM_DIR,ADU5_VTG_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(ADU5_SAT_TELEM_LINK_DIR);
    queuePtr->gpsLinks+=numLinks;
    if(printToScreen) printf("%s\t%u\n",ADU5_SAT_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(ADU5_SAT_TELEM_DIR,ADU5_SAT_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(MONITOR_TELEM_LINK_DIR);
    queuePtr->monitorLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",MONITOR_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(MONITOR_TELEM_DIR,MONITOR_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(SURFHK_TELEM_LINK_DIR);
    queuePtr->surfHkLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",SURFHK_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(SURFHK_TELEM_DIR,SURFHK_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(TURFHK_TELEM_LINK_DIR);
    queuePtr->turfHkLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",TURFHK_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(TURFHK_TELEM_DIR,TURFHK_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(HEADER_TELEM_LINK_DIR);
    queuePtr->headLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",HEADER_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxEventQueueSize) {
	purgeHkDirectory(HEADER_TELEM_DIR,HEADER_TELEM_LINK_DIR);
    }

    numLinks=countFilesInDir(PEDESTAL_TELEM_LINK_DIR);
    queuePtr->pedestalLinks=numLinks;
    if(printToScreen) printf("%s\t%u\n",PEDESTAL_TELEM_LINK_DIR,numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(PEDESTAL_TELEM_DIR,PEDESTAL_TELEM_LINK_DIR);
    }
    
    for(count=0;count<NUM_PRIORITIES;count++) {
	numLinks=countFilesInDir(eventTelemLinkDirs[count]);
	queuePtr->eventLinks[count]=numLinks;
	if(printToScreen) printf("%s\t%u\n",eventTelemLinkDirs[count]
				 ,numLinks);
	if(((short)numLinks)==-1) {
	    retVal--;
	}
	else if(numLinks>maxEventQueueSize) {
	    //purge directory
	    purgeDirectory(count);
	}
    }
    return retVal;
}

void purgeHkDirectory(char *dirName,char *linkDirName) 
{
    int count=0;
    char currentLink[FILENAME_MAX];
    char currentFile[FILENAME_MAX];
    struct dirent **linkList;
    int numLinks=getListofLinks(linkDirName,
				&linkList);
    if(numLinks>maxHkQueueSize) {
	syslog(LOG_INFO,"Telemetry not keeping up removing %d links from queue %s\n",numLinks-50,dirName);
	fprintf(stderr,"Telemetry not keeping up removing %d event links from queue %s\n",numLinks-50,dirName);
	for(count=0;count<numLinks-50;count++) {
	    sprintf(currentLink,"%s/%s",linkDirName,linkList[count]->d_name);
	    sprintf(currentFile,"%s/%s",dirName,linkList[count]->d_name);
	    removeFile(currentFile);
	    removeFile(currentLink);			
	}
	for(count=0;count<numLinks;
	    count++) 
	    free(linkList[count]);		    
	free(linkList);		
    }

}


void purgeDirectory(int priority) {    
    int count;
    unsigned int eventNumber;
    char currentTouchname[FILENAME_MAX];
    char currentHeader[FILENAME_MAX];
    char purgeFile[FILENAME_MAX];
    char purgeString[60];
    struct dirent **linkList;
    int numLinks=getListofLinks(eventTelemLinkDirs[priority],
				&linkList);
    if(numLinks>maxEventQueueSize) {
	syslog(LOG_INFO,"Telemetry not keeping up removing %d event links from queue %s\n",numLinks-100,eventTelemLinkDirs[priority]);
	fprintf(stderr,"Telemetry not keeping up removing %d event links from queue %s\n",numLinks-100,eventTelemLinkDirs[priority]);
	
	
	sscanf(linkList[0]->d_name,
	       "hd_%u.dat",&eventNumber);
	sprintf(purgeFile,"%s/purged_%u.txt.gz",priorityPurgeDirs[priority],eventNumber);
	gzFile Purge=gzopen(purgeFile,"w");

	for(count=0;count<numLinks-100;
	    count++) {
	    
	    sscanf(linkList[count]->d_name,
		   "hd_%u.dat",&eventNumber);


	    sprintf(currentHeader,"%s/hd_%d.dat",eventTelemDirs[priority], 
		    eventNumber);	    
	    sprintf(currentTouchname,"%s.sipd",currentHeader);
	    if(checkFileExists(currentTouchname))
		continue;

	    sprintf(currentTouchname,"%s.losd",currentHeader);
	    if(checkFileExists(currentTouchname))
		continue;
	    
	    //Now need to add something here that archived the event numbers 
	    //to be deleted, so that Playbackd can resurrect them.
	    if(Purge) {
		sprintf(purgeString,"%u\n",eventNumber);
		gzputs(Purge,purgeString);
	    }


	    removeFile(currentHeader);			
	    sprintf(currentHeader,"%s/%s",eventTelemLinkDirs[priority],
		    linkList[count]->d_name);	    	    
	    removeFile(currentHeader);
	    
	    sprintf(currentHeader,"%s/ev_%d.dat",eventTelemDirs[priority], 
		    eventNumber);
	    removeFile(currentHeader);			
	}
	if(Purge) {
	    gzclose(Purge);
	}

	for(count=0;count<numLinks;
	    count++) 
	    free(linkList[count]);		    
	free(linkList);		
    }
    

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

    sprintf(otherMonWriter.relBaseName,"%s/",MONITOR_ARCHIVE_DIR);
    sprintf(otherMonWriter.filePrefix,"othermon");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	otherMonWriter.currentFilePtr[diskInd]=0;
    otherMonWriter.writeBitMask=hkDiskBitMask;
}


void checkProcesses()
{
    ProgramId_t prog;
    CommandStruct_t theCmd;    
    theCmd.numCmdBytes=3;
    theCmd.cmd[0]=CMD_START_PROGS;
    unsigned int value=0;
    int sendCommand=0;
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	char *pidFile=getPidFile(prog);
//	syslog(LOG_INFO,"Trying prog: %d %s\n",prog,pidFile); 
	FILE *test = fopen(pidFile,"r");

	if(!test) {
	    syslog(LOG_INFO,"%s not present what will I do\n",pidFile);
	    if(prog!=ID_ACQD || (prog==ID_ACQD && !killedAcqd)) {
		syslog(LOG_WARNING,"%s not present will restart process\n",
		       pidFile);
		sendCommand=1;
		value|=getIdMask(prog);	    
	    }
	}
	else fclose(test);
    }
    if(sendCommand) {
	theCmd.cmd[1]=value&0xff;
	theCmd.cmd[2]=(value&0xff00)>>8;
	writeCommandAndLink(&theCmd);
    }


}

int getIdMask(ProgramId_t prog) {
    switch(prog) {
	case ID_ACQD: return ACQD_ID_MASK;
	case ID_ARCHIVED: return ARCHIVED_ID_MASK;
	case ID_CALIBD: return CALIBD_ID_MASK;
	case ID_CMDD: return CMDD_ID_MASK;
	case ID_EVENTD: return EVENTD_ID_MASK;
	case ID_GPSD: return GPSD_ID_MASK;
	case ID_HKD: return HKD_ID_MASK;
	case ID_LOSD: return LOSD_ID_MASK;
	case ID_PRIORITIZERD: return PRIORITIZERD_ID_MASK;
	case ID_SIPD: return SIPD_ID_MASK;
	case ID_MONITORD: return MONITORD_ID_MASK;
	default: break;
    }
    return 0;
}



char *getProgName(ProgramId_t prog) {
    char *string;
    switch(prog) {
	case ID_ACQD: string="Acqd"; break;
	case ID_ARCHIVED: string="Archived"; break;
	case ID_CALIBD: string="Calibd"; break;
	case ID_CMDD: string="Cmdd"; break;
	case ID_EVENTD: string="Eventd"; break;
	case ID_GPSD: string="GPSd"; break;
	case ID_HKD: string="Hkd"; break;
	case ID_LOSD: string="LOSd"; break;
	case ID_PRIORITIZERD: string="Prioritizerd"; break;
	case ID_SIPD: string="SIPd"; break;
	case ID_MONITORD: string="Monitord"; break;
	default: string=NULL; break;
    }
    return string;
}

char *getPidFile(ProgramId_t prog) {
  switch(prog) {
  case ID_ACQD: return ACQD_PID_FILE;
  case ID_ARCHIVED: return ARCHIVED_PID_FILE;
  case ID_CALIBD: return CALIBD_PID_FILE;
  case ID_CMDD: return CMDD_PID_FILE;
  case ID_EVENTD: return EVENTD_PID_FILE;
  case ID_GPSD: return GPSD_PID_FILE;
  case ID_HKD: return HKD_PID_FILE;
  case ID_LOSD: return LOSD_PID_FILE;
  case ID_PRIORITIZERD: return PRIORITIZERD_PID_FILE;
  case ID_SIPD: return SIPD_PID_FILE;
  case ID_MONITORD: return MONITORD_PID_FILE;
  default: break;
  }
  return NULL;
}


void fillOtherStruct(OtherMonitorStruct_t *otherPtr)
{
    CommandStruct_t theCmd;
    static int archivedCheckCount=0;
    static int archivedLastVal=0;
    static int killedPrioritizerd=0;
    int retVal=0;
    otherPtr->unixTime=time(NULL);
    otherPtr->ramDiskInodes=getRamdiskInodes();
    if(printToScreen)
	printf("Ramdisk inodes: %u\n",otherPtr->ramDiskInodes);
    otherPtr->runStartTime=0;
    otherPtr->runStartEventNumber=0;
    otherPtr->runNumber=0;

    FILE *fp = fopen(RUN_START_FILE,"rb");
    if(fp) {
	RunStart_t runStart;
	retVal=fread(&runStart,sizeof(RunStart_t),1,fp);
	if(retVal>0) {
	    otherPtr->runStartTime=runStart.unixTime;
	    otherPtr->runStartEventNumber=runStart.eventNumber;
	    otherPtr->runNumber=runStart.runNumber;
	}
	fclose(fp);
    }	    
    else {
	otherPtr->runNumber=getLatestRunNumber();	
	struct stat buf;
	int retVal2=stat(lastRunNumberFile,&buf);
	if(retVal2==0)
	    otherPtr->runStartTime=buf.st_mtime;

    }
    int i;
    unsigned short numLinks=0;
    
    for(i=0;i<3;i++) {
	numLinks=countFilesInDir(otherDirLoc[i]);
	otherPtr->dirFiles[i]=numLinks;
	numLinks=countFilesInDir(otherLinkLoc[i]);
	otherPtr->dirLinks[i]=numLinks;
	if(i==2 && numLinks>1000) {
	    //Archived is a long way back
	    syslog(LOG_INFO,"Archived has %d links to process",numLinks);
	    if(archivedCheckCount>0) {
		if(numLinks>archivedLastVal) {
		    syslog(LOG_INFO,"Archived has %d links to process",numLinks);
		    if(archivedCheckCount>5) {
			if(!killedPrioritizerd) {
			    theCmd.numCmdBytes=3;
			    theCmd.cmd[0]=CMD_REALLY_KILL_PROGS;
			    theCmd.cmd[1]=0;
			    theCmd.cmd[2]=0x1; //Prioritizerd
			    writeCommandAndLink(&theCmd);
			    killedPrioritizerd=1;
			}
		    }			 
		}
	    }
	    archivedCheckCount++;
	    archivedLastVal=numLinks;		
	}
	else {
	    archivedCheckCount=0;
	}


    }	
}

int writeOtherFileAndLink(OtherMonitorStruct_t *otherPtr) {
    char theFilename[FILENAME_MAX];
    int retVal=0;
    
    fillGenericHeader(otherPtr,PACKET_OTHER_MONITOR,sizeof(OtherMonitorStruct_t));
//    syslog(LOG_INFO,"Other packet code: %d, numBytes %d, checkSum %d, feByte %d\n",
//	   otherPtr->gHdr.code,otherPtr->gHdr.numBytes,otherPtr->gHdr.checksum,otherPtr->gHdr.feByte);

    sprintf(theFilename,"%s/othermon_%d.dat",
	    OTHER_MONITOR_TELEM_DIR,otherPtr->unixTime);
    retVal=normalSingleWrite((unsigned char*)otherPtr,theFilename,sizeof(OtherMonitorStruct_t));
    retVal=makeLink(theFilename,OTHER_MONITOR_TELEM_LINK_DIR);

    retVal=cleverHkWrite((unsigned char*)otherPtr,sizeof(OtherMonitorStruct_t),
			 otherPtr->unixTime,&otherMonWriter);
    if(retVal<0) {
	//Had an error
    }
    
    return retVal;        
}


int getRamdiskInodes() {
    struct statvfs diskStat;
    static int errorCounter=0;
    int retVal=statvfs("/tmp",&diskStat); 
    if(retVal<0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Unable to statvfs /tmp: %s",strerror(errno));  
	    fprintf(stderr,"Unable to statvfs /tmp: %s\n",strerror(errno));            
	}
//	if(printToScreen) fprintf(stderr,"Unable to get disk space %s: %s\n",dirName,strerror(errno));       
	return 0;
    }    
    return diskStat.f_favail;
}

int getLatestRunNumber() {
    int retVal=0;
    int runNumber=0;

    FILE *pFile;
    pFile = fopen (lastRunNumberFile, "r");
    if(pFile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		lastRunNumberFile);
    }
    else {	    	    
	retVal=fscanf(pFile,"%d",&runNumber);
	if(retVal<0) {
	    syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
		    lastRunNumberFile);
	}
	fclose (pFile);
    }
    return runNumber;
}


void handleBadSigs(int sig)
{   
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig);  
    closeHkFilesAndTidy(&monWriter);
    unlink(MONITORD_PID_FILE);
    syslog(LOG_INFO,"Monitord terminating");    
    exit(0);
}

int sortOutPidFile(char *progName)
{
  int retVal=0;
  retVal=checkPidFile(MONITORD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,MONITORD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(MONITORD_PID_FILE);  
  return 0;
}
