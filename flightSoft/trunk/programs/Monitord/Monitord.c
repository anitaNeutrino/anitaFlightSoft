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
#include "linkWatchLib/linkWatchLib.h"
#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"
#include "includes/anitaCommand.h"

#define NUM_HK_TELEM_DIRS 21

typedef enum {
  MON_TELEM_FIRST=0,
  MON_TELEM_CMD_ECHO_LOS=0,
  MON_TELEM_CMD_ECHO_SIP,
  MON_TELEM_MONITOR,
  MON_TELEM_HEADER,
  MON_TELEM_HK,
  MON_TELEM_ADU5A_SAT,
  MON_TELEM_ADU5B_SAT,
  MON_TELEM_G12_SAT,
  MON_TELEM_ADU5A_PAT,
  MON_TELEM_ADU5B_PAT,
  MON_TELEM_G12_POS,
  MON_TELEM_ADU5A_VTG,
  MON_TELEM_ADU5B_VTG,
  MON_TELEM_G12_GGA,
  MON_TELEM_ADU5A_GGA,
  MON_TELEM_ADU5B_GGA,
  MON_TELEM_SURFHK,
  MON_TELEM_TURFRATE,
  MON_TELEM_OTHER,
  MON_TELEM_PEDESTAL,
  MON_TELEM_REQUEST,
  MON_TELEM_NOT_A_TELEM
} MONTelemType_t;


#ifndef SIGRTMIN
#define SIGRTMIN 32
#endif

int readConfigFile();
int checkDisks(DiskSpaceStruct_t *dsPtr);
int checkQueues(QueueStruct_t *queuePtr);
int writeFileAndLink(MonitorStruct_t *monitorPtr);
void prepWriterStructs();
void purgeDirectory(int priority);
void checkProcesses(int dontStart);
int getRamdiskInodes();
void fillOtherStruct(OtherMonitorStruct_t *otherPtr);
int writeOtherFileAndLink(OtherMonitorStruct_t *otherPtr);
void purgeHkDirectory(char *dirName,char *linkDirName, int deleteAll);
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
int getProcessInfo(ProgramId_t prog, int progPid);
void checkSatasAreWorking();



// Global Variables
int printToScreen=0;
int verbosity=0;
int monitorPeriod=60; //In seconds
int startProcesses=0;
int ramdiskKillAcqd=50;
int ramdiskDumpData=5;
int inodesKillAcqd=10000;
int inodesDumpData=2000;
int killedAcqd=0;
int acqdKillTime=0;
int maxAcqdWaitPeriod=180;
int maxEventQueueSize=300;
int maxHkQueueSize=200;



//
int usbSwitchMB=50;
int ntuSwitchMB=50;

//Link Directories
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];
char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];

char priorityPurgeDirs[NUM_PRIORITIES][FILENAME_MAX];

char usbName[FILENAME_MAX];
char ntuName[FILENAME_MAX];


int hkDiskBitMask=0;
int eventDiskBitMask=0;
int disableHelium1=0;
int disableHelium2=0;
int disableUsb=0;
//int disableNeobrick=0;
AnitaHkWriterStruct_t monWriter;
AnitaHkWriterStruct_t otherMonWriter;

int diskScaleFactors[8]={1,1,1,1,128,128,4,16};
char *diskLocations[7]={"/tmp","/var","/home","/",HELIUM1_DATA_MOUNT,HELIUM2_DATA_MOUNT,USB_DATA_MOUNT};
char *otherDirLoc[3]={ACQD_EVENT_DIR,EVENTD_EVENT_DIR,PRIORITIZERD_EVENT_DIR};
char *otherLinkLoc[3]={ACQD_EVENT_LINK_DIR,EVENTD_EVENT_LINK_DIR,PRIORITIZERD_EVENT_LINK_DIR};

//Laziness
MonitorStruct_t monData;
OtherMonitorStruct_t otherData;

int main (int argc, char *argv[])
{
    CommandStruct_t theCmd;
    theCmd.fromSipd=0;
    int retVal,pri;
//    DiskSpaceStruct_t diskSpace;


    /* Log stuff */
    char *progName=basename(argv[0]);
    unsigned int startTime=time(NULL);
    int usbMountCmdTime=0;

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
	    if((monData.unixTime-startTime)>60) {
	      checkProcesses(0);
	      checkSatasAreWorking();
	    }
	    else
	      checkProcesses(1);

	    fillOtherStruct(&otherData);

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
		if(monData.diskInfo.diskSpace[0]>ramdiskKillAcqd+200 || (monData.unixTime-acqdKillTime)>maxAcqdWaitPeriod) {	
		    theCmd.numCmdBytes=3;
		    theCmd.cmd[0]=CMD_START_PROGS;
		    theCmd.cmd[1]=ACQD_ID_MASK;
		    theCmd.cmd[2]=0;
		    writeCommandAndLink(&theCmd);
		    killedAcqd=0;
		}
	    }

	    //Now check if we need to change disks
	    //RJN Must add something for NTU disk

	    printf("usb %d\t%d\n",monData.diskInfo.diskSpace[6],usbSwitchMB);

	    if(monData.diskInfo.diskSpace[6]<usbSwitchMB) {
	      readConfigFile();
		//Change USB if in use
		if((hkDiskBitMask&USB_DISK_MASK || eventDiskBitMask&USB_DISK_MASK)) {
		    if((monData.unixTime-usbMountCmdTime)>300) {
			theCmd.numCmdBytes=3;
			theCmd.cmd[0]=CMD_MOUNT_NEXT_USB;
			theCmd.cmd[1]=1; //Now not used
			theCmd.cmd[2]=0;
			if(monData.diskInfo.diskSpace[7]<usbSwitchMB)
			    theCmd.cmd[1]=3;
			writeCommandAndLink(&theCmd);
			usbMountCmdTime=monData.unixTime;
		    }
		}
	    }
	    
	    writeFileAndLink(&monData);
	    writeOtherFileAndLink(&otherData);


//	    exit(0);
//	    usleep(1);
	    sleep(monitorPeriod);
	}
    } while(currentState==PROG_STATE_INIT);    
    closeHkFilesAndTidy(&monWriter);
    closeHkFilesAndTidy(&otherMonWriter);
    unlink(MONITORD_PID_FILE);
    return 0;
}

int writeFileAndLink(MonitorStruct_t *monitorPtr) {
    char theFilename[FILENAME_MAX];
    int retVal=0;
    
    fillGenericHeader(monitorPtr,PACKET_MONITOR,sizeof(MonitorStruct_t));
    
    sprintf(theFilename,"%s/mon_%d.dat",
	    MONITOR_TELEM_DIR,monitorPtr->unixTime);
    retVal=writeStruct(monitorPtr,theFilename,sizeof(MonitorStruct_t));
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
	startProcesses=kvpGetInt("startProcesses",0);
	maxEventQueueSize=kvpGetInt("maxEventQueueSize",600);
	maxHkQueueSize=kvpGetInt("maxHkQueueSize",1000);


	tempString=kvpGetString("usbName");
	if(tempString) {
	    strncpy(usbName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbName");
	    fprintf(stderr,"Couldn't get usbName\n");
	}


	
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	hkDiskBitMask=kvpGetInt("hkDiskBitMask",0);
	eventDiskBitMask=kvpGetInt("eventDiskBitMask",0);
	monitorPeriod=kvpGetInt("monitorPeriod",60);
	usbSwitchMB=kvpGetInt("usbSwitchMB",50);
	ntuSwitchMB=kvpGetInt("ntuSwitchMB",50);
	ramdiskKillAcqd=kvpGetInt("ramdiskKillAcqd",50);
	ramdiskDumpData=kvpGetInt("ramdiskDumpData",5);
	maxAcqdWaitPeriod=kvpGetInt("maxAcqdWaitPeriod",180);
	inodesKillAcqd=kvpGetInt("inodesKillAcqd",10000);
	inodesDumpData=kvpGetInt("inodesDumpData",2000);
	
	disableHelium1=kvpGetInt("disableHelium1",0);
	if(disableHelium1) {
	  hkDiskBitMask&=(~HELIUM1_DISK_MASK);
	  eventDiskBitMask&=(~HELIUM1_DISK_MASK);
	}
	disableHelium2=kvpGetInt("disableHelium2",0);
	if(disableHelium2) {
	  hkDiskBitMask&=(~HELIUM2_DISK_MASK);
	  eventDiskBitMask&=(~HELIUM2_DISK_MASK);
	}
	disableUsb=kvpGetInt("disableUsb",0);
	if(disableUsb) {
	  hkDiskBitMask&=(~USB_DISK_MASK);
	  eventDiskBitMask&=(~USB_DISK_MASK);
	}
	//	disableNeobrick=kvpGetInt("disableNeobrick",0);
	//	if(disableNeobrick) {
	//	  hkDiskBitMask&=(~NEOBRICK_DISK_MASK);
	//	  eventDiskBitMask&=(~NEOBRICK_DISK_MASK);
	//	}

    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Monitord.config: %s\n",eString);
    }
    
    if(printToScreen) 
      printf("hkDiskBitMask %#x -- eventDiskBitMask %#x\n",
	     hkDiskBitMask,eventDiskBitMask);
     

    if(printToScreen && verbosity>1)
	printf("Dirs:\n\t%s\n\t%s\n\t%s\n",
	       MONITOR_TELEM_DIR,
	       MONITOR_TELEM_LINK_DIR,
	       MONITOR_ARCHIVE_DIR
	       );

	printf("Dirs:\n\t%s\n\t%s\n\n",
	       OTHER_MONITOR_TELEM_DIR,
	       OTHER_MONITOR_TELEM_LINK_DIR
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
    FILE *neoFile=0;
    //    unsigned int neoTime;
    unsigned long long neoSpaceUsed;
    unsigned long long neoSpaceAvailable;
    static unsigned long long lastNeoSpaceAvailable=0;
    //    long long neoSpaceTotal;
    //    float neoTemp;
    //    float neoPress;
    for(diskNum=0;diskNum<7;diskNum++) {
	megaBytes=getDiskSpace(diskLocations[diskNum]);
	printf("%s -- %u\n",diskLocations[diskNum],megaBytes);
	if(megaBytes>0) {
	  megaBytes/=diskScaleFactors[diskNum];
	  if(megaBytes<65535) megaBytes_short=megaBytes;
	  else megaBytes_short=65535;
	}
	else megaBytes_short=0;
	dsPtr->diskSpace[diskNum]=megaBytes_short;
	if(printToScreen) printf("%s\t%u\n",diskLocations[diskNum],megaBytes_short);
	if(((short)megaBytes)==-1) errFlag--;
    }  

    neoFile=fopen("/tmp/lastNtuDiskSpace","r");
    if(neoFile) {
	fscanf(neoFile,"%s %llu %llu",ntuName,&neoSpaceUsed,&neoSpaceAvailable);
	printf("%s %llu %llu\n",ntuName,neoSpaceUsed,neoSpaceAvailable);

	if(neoSpaceAvailable>0) {
	    megaBytes=neoSpaceAvailable/(1024);  
	    lastNeoSpaceAvailable=neoSpaceAvailable;
	}
	else {
	    megaBytes=lastNeoSpaceAvailable/(1024);
	}
	megaBytes/=16;
	if(megaBytes<65535) megaBytes_short=megaBytes;
	else megaBytes_short=65535;
	dsPtr->diskSpace[7]=megaBytes_short;
	if(printToScreen) printf("Neobick\t%u\n",megaBytes_short);	
	fclose(neoFile);
    }
    else {
	dsPtr->diskSpace[7]=0;
    }
    strncpy(dsPtr->ntuLabel,ntuName,11);
    strncpy(dsPtr->usbLabel,usbName,11);
    if(printToScreen) {
	printf("usb -- %s\n",
	       usbName);
    }
    
    
//    printf("Err Flag %d\n",errFlag);
    return errFlag;
}



int checkQueues(QueueStruct_t *queuePtr) {
  int retVal=0,pri=0,hkInd=0;
  int purged=0;
  unsigned short numLinks=0;
  
  static char *telemLinkDirs[NUM_HK_TELEM_DIRS]=
    {LOSD_CMD_ECHO_TELEM_LINK_DIR,
     SIPD_CMD_ECHO_TELEM_LINK_DIR,
     MONITOR_TELEM_LINK_DIR,
     HEADER_TELEM_LINK_DIR,
     HK_TELEM_LINK_DIR,
     ADU5A_SAT_TELEM_LINK_DIR,
     ADU5B_SAT_TELEM_LINK_DIR,
     G12_SAT_TELEM_LINK_DIR,
     ADU5A_PAT_TELEM_LINK_DIR,
     ADU5B_PAT_TELEM_LINK_DIR,
     G12_POS_TELEM_LINK_DIR,
     ADU5A_VTG_TELEM_LINK_DIR,
     ADU5B_VTG_TELEM_LINK_DIR,
     G12_GGA_TELEM_LINK_DIR,
     ADU5A_GGA_TELEM_LINK_DIR,
     ADU5B_GGA_TELEM_LINK_DIR,   
     SURFHK_TELEM_LINK_DIR,
     TURFHK_TELEM_LINK_DIR,
     OTHER_MONITOR_TELEM_LINK_DIR,
     PEDESTAL_TELEM_LINK_DIR,
     REQUEST_TELEM_LINK_DIR};
  static char *telemDirs[NUM_HK_TELEM_DIRS]=
    {LOSD_CMD_ECHO_TELEM_DIR,
     SIPD_CMD_ECHO_TELEM_DIR,
     MONITOR_TELEM_DIR,
     HEADER_TELEM_DIR,
     HK_TELEM_DIR,
     ADU5A_SAT_TELEM_DIR,
     ADU5B_SAT_TELEM_DIR,
     G12_SAT_TELEM_DIR,
     ADU5A_PAT_TELEM_DIR,
     ADU5B_PAT_TELEM_DIR,
     G12_POS_TELEM_DIR,
     ADU5A_VTG_TELEM_DIR,
     ADU5B_VTG_TELEM_DIR,
     G12_GGA_TELEM_DIR,
     ADU5A_GGA_TELEM_DIR,
     ADU5B_GGA_TELEM_DIR,
     SURFHK_TELEM_DIR,
     TURFHK_TELEM_DIR,
     OTHER_MONITOR_TELEM_DIR,
     PEDESTAL_TELEM_DIR,
     REQUEST_TELEM_DIR};

  
  for(hkInd=0;hkInd<NUM_HK_TELEM_DIRS;hkInd++) {    
    numLinks=countFilesInDir(telemLinkDirs[hkInd]);
    queuePtr->hkLinks[hkInd]=numLinks;
    if(printToScreen) printf("%s\t%u\n",telemLinkDirs[hkInd],numLinks);
    if(((short)numLinks)==-1) retVal--;
    else if(numLinks>maxHkQueueSize) {
	purgeHkDirectory(telemDirs[hkInd],telemLinkDirs[hkInd],0);
      purged++;
    }
  }
  
  for(pri=0;pri<NUM_PRIORITIES;pri++) {
    numLinks=countFilesInDir(eventTelemLinkDirs[pri]);
    queuePtr->eventLinks[pri]=numLinks;
    if(printToScreen) printf("%s\t%u\n",eventTelemLinkDirs[pri]
			     ,numLinks);
    if(((short)numLinks)==-1) {
      retVal--;
    }
    else if(numLinks>maxEventQueueSize) {
      //purge directory
      purgeDirectory(pri);
      purged++;
    }
  }
  if(purged) {
    sendSignal(ID_SIPD,SIGRTMIN);

  }

  return retVal;
}

void purgeHkDirectory(char *dirName,char *linkDirName, int deleteAll) 
{
    int count=0;
    char currentLink[FILENAME_MAX];
    char currentFile[FILENAME_MAX];
    struct dirent **linkList;
    int numLinks=getListofLinks(linkDirName,
				&linkList);

    int linksToSave=maxHkQueueSize-100;
    if(deleteAll) linksToSave=0;
    if(linksToSave<0)
      linksToSave=100;
    if(numLinks>maxHkQueueSize) {
	syslog(LOG_INFO,"Telemetry not keeping up removing %d links from queue %s\n",numLinks-linksToSave,dirName);
	fprintf(stderr,"Telemetry not keeping up removing %d event links from queue %s\n",numLinks-linksToSave,dirName);
	for(count=0;count<numLinks-linksToSave;count++) {
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
    int linksToSave=maxEventQueueSize-100;
    if(linksToSave<0) 
      linksToSave=100;

    if(numLinks>maxEventQueueSize) {
	syslog(LOG_INFO,"Telemetry not keeping up removing %d event links from queue %s\n",numLinks-linksToSave,eventTelemLinkDirs[priority]);
	fprintf(stderr,"Telemetry not keeping up removing %d event links from queue %s\n",numLinks-linksToSave,eventTelemLinkDirs[priority]);
	
	
	sscanf(linkList[0]->d_name,
	       "hd_%u.dat",&eventNumber);
	sprintf(purgeFile,"%s/purged_%u.txt.gz",priorityPurgeDirs[priority],eventNumber);
	gzFile Purge=gzopen(purgeFile,"w");

	for(count=0;count<numLinks-linksToSave;
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


void checkProcesses(int dontStart)
{
  otherData.processBitMask=0;
    ProgramId_t prog;
    int testPid=0;
    int retVal=0;
    CommandStruct_t theCmd;    
    theCmd.fromSipd=0;
    theCmd.numCmdBytes=3;
    theCmd.cmd[0]=CMD_START_PROGS;
    unsigned int value=0;
    int sendCommand=0;
    int idMask=0;
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {  
      idMask=getIdMask(prog);
      otherData.processBitMask|=idMask;
	char *pidFile=getPidFile(prog);
//	syslog(LOG_INFO,"Trying prog: %d %s\n",prog,pidFile); 
	FILE *test = fopen(pidFile,"r");

	if(!test) {
	  otherData.processBitMask&=(~idMask);
	  fprintf(stderr,"%s is not present\n",pidFile);
	  syslog(LOG_INFO,"%s not present what will I do\n",pidFile);
	  if(startProcesses && !dontStart) {
	    if(prog==ID_CMDD) {
	      //Probably not much sense in sending a command
	      retVal=system("daemon --stop -n Cmdd");
	      retVal=system("daemon -r Cmdd -n Cmdd");
	      syslog(LOG_ERR,"Cmdd isn't running tried to restart it with daemon -r Cmdd -n Cmdd -- %d",retVal);
	    }
	    else {	      
	      if(prog!=ID_ACQD || (prog==ID_ACQD && !killedAcqd)) {
		fprintf(stderr,"%s not present will restart process\n",
			pidFile);
		syslog(LOG_WARNING,"%s not present will restart process\n",
		       pidFile);
		sendCommand=1;
		value|=idMask;	    
	      }
	    }
	  }
	}
	else {
	  fscanf(test,"%d", &testPid) ;
	  fclose(test);	  	  
	  retVal=getProcessInfo(prog,testPid);
	  if(retVal<0) {
	    otherData.processBitMask&=(~idMask);
	    fprintf(stderr,"%s present, but process %d isn't\n",pidFile,testPid);
	    syslog(LOG_INFO,"%s present, but process %d isn't\n",pidFile,testPid);
	    removeFile(pidFile);
	    if(startProcesses && !dontStart) {
	      if(prog==ID_CMDD) {
		//Probably not much sense in sending a command
		retVal=system("daemon --stop -n Cmdd");
		retVal=system("daemon -r Cmdd -n Cmdd");
		syslog(LOG_ERR,"Cmdd isn't running tried to restart it with daemon -r Cmdd -n Cmdd -- %d",retVal);
	      }
	      else {
		if(prog!=ID_ACQD || (prog==ID_ACQD && !killedAcqd)) {
		  syslog(LOG_WARNING,"%s not present will restart process\n",
			 pidFile);
		  sendCommand=1;
		  value|=idMask;	    
		}
	      }	 
	    }   
	  }
	}
    }
    if(sendCommand) {
	theCmd.cmd[1]=value&0xff;
	theCmd.cmd[2]=(value&0xff00)>>8;
	writeCommandAndLink(&theCmd);
    }
}

int getProcessInfo(ProgramId_t prog, int progPid)
{
  char statName[FILENAME_MAX];
  int pid,ppid,pgrp,session,tty_nr,tpgid;
  unsigned long flags,minflt,cminflt,majflt,cmajflt;
  unsigned long utime,stime;
  long cutime,cstime,priority,nice,hardCode,itrealvalue;
  unsigned long starttime, vsize;
  long rss;
  unsigned long rlim, startcode, endcode, startstack,kstkesp,kstkeip,signal;
  unsigned long blocked, sigignore,sigcatch,wchan,nswap,cnswap;
  int exit_signal,processor;
  unsigned long rt_priority, policy;
  
  char processName[180];
  char state;     
  sprintf(statName,"/proc/%d/stat",progPid);
  FILE *fpProc = fopen(statName,"r");
  if(!fpProc) {
    //Probably wasn't really a process
    return -1;
  }
  
  fscanf(fpProc,"%d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %lu %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d %lu %lu",&pid,processName,&state,&ppid,&pgrp,&session,&tty_nr,&tpgid,&flags,&minflt,&cminflt,&majflt,&cmajflt,&utime,&stime,&cutime,&cstime,&priority,&nice,&hardCode,&itrealvalue,&starttime,&vsize,&rss,&rlim,&startcode,&endcode,&startstack,&kstkesp,&kstkeip,&signal,&blocked,&sigignore,&sigcatch,&wchan,&nswap,&cnswap,&exit_signal,&processor,&rt_priority,&policy);
  fclose(fpProc);

  if(printToScreen) printf("Process %s (%d) -- utime %lu stime %lu starttime %lu\n",processName,pid,utime,stime,starttime);
  monData.procInfo.utime[prog-ID_FIRST]=utime;
  monData.procInfo.stime[prog-ID_FIRST]=stime;
  monData.procInfo.vsize[prog-ID_FIRST]=vsize;
  return 0;

}




void fillOtherStruct(OtherMonitorStruct_t *otherPtr)
{
    CommandStruct_t theCmd;
    static int archivedCheckCount=0;
    static int archivedLastVal=0;
    static int killedPrioritizerd=0;
    int retVal=0;
    theCmd.fromSipd=0;
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
	otherPtr->runNumber=getRunNumber();	
	struct stat buf;
	int retVal2=stat(LAST_RUN_NUMBER_FILE,&buf);
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

    //Add a check for number of cmds
    
    numLinks=countFilesInDir("/tmp/anita/cmdd/link");
    printf("Cmdd has %d commands to process\n",numLinks);
    if(numLinks>100) 
    {
	//Bad stuff is happening
	syslog(LOG_INFO,"Cmdd has %d links to process, bad things are probably happening\n",numLinks);

	retVal=system("daemon --stop -n Cmdd");
	sleep(3);
	retVal=system("killall -9 Cmmd");
	sleep(2);
	purgeHkDirectory("/tmp/anita/cmdd","/tmp/anita/cmdd/link",1);
	retVal=system("daemon -r Cmdd -n Cmdd");
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

#define TEST_PATTERN_SIZE 4096

void checkSatasAreWorking()
{   
    CommandStruct_t theCmd;
    theCmd.fromSipd=0;
    char testFilename[FILENAME_MAX];
    FILE *fp=0;
    int numObjs=0;
    int shouldDisableHelium2=0;
    int shouldDisableHelium1=0;
    static int bladeErrCounter=0;
    static int miniErrCounter=0;
    unsigned int increment;
    unsigned char testPattern[TEST_PATTERN_SIZE]={0};
    unsigned char readBackPattern[TEST_PATTERN_SIZE]={0};
    
    for(increment=0;increment<TEST_PATTERN_SIZE;increment++) {
	testPattern[increment]=increment%256;
    }

    if(!disableHelium2) {
	sprintf(testFilename,"%s/diskTestFile.dat",HELIUM2_DATA_MOUNT);
	fp=fopen(testFilename,"wb");
	if(fp==NULL) {

	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),testFilename);
	    fprintf(stderr,"fopen: %s -- %s\n",strerror(errno),testFilename);
	    shouldDisableHelium2=1;
	}
	else {
	    numObjs=fwrite(testPattern,sizeof(char),TEST_PATTERN_SIZE,fp);
	    if(numObjs!=TEST_PATTERN_SIZE) {	
		syslog (LOG_ERR,"fwrite: %s ---  %s\n",strerror(errno),testFilename);
		fprintf(stderr,"fwrite: %s -- %s\n",strerror(errno),testFilename);
		shouldDisableHelium2=1;
		fclose(fp);
	    }	
	    else {
		fclose(fp);
		//Time for the read test
		fp=fopen(testFilename,"rb");
		if(fp==NULL) {

		    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),testFilename);
		    fprintf(stderr,"fopen: %s -- %s\n",strerror(errno),testFilename);
		    shouldDisableHelium2=1;
		}
		else {
		    numObjs=fread(readBackPattern,sizeof(char),TEST_PATTERN_SIZE,fp);
		    if(numObjs!=TEST_PATTERN_SIZE) {	
			syslog (LOG_ERR,"fread: %s ---  %s\n",strerror(errno),testFilename);
			fprintf(stderr,"fread: %s -- %s\n",strerror(errno),testFilename);
			shouldDisableHelium2=1;
			fclose(fp);
		    }	
		    else {
			//Time to check pattern
			fclose(fp);
			for(increment=0;increment<TEST_PATTERN_SIZE;increment++) {
			    if(readBackPattern[increment]!=testPattern[increment]) {
				shouldDisableHelium2=1;
				break;
			    }
			}
		    }
		}	    					
	    }
	}	
    }

    if(!disableHelium1) {
	sprintf(testFilename,"%s/diskTestFile.dat",HELIUM1_DATA_MOUNT);
	fp=fopen(testFilename,"wb");
	if(fp==NULL) {

	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),testFilename);
	    fprintf(stderr,"fopen: %s -- %s\n",strerror(errno),testFilename);
	    shouldDisableHelium1=1;
	}
	else {
	    numObjs=fwrite(testPattern,sizeof(char),TEST_PATTERN_SIZE,fp);
	    if(numObjs!=TEST_PATTERN_SIZE) {	
		syslog (LOG_ERR,"fwrite: %s ---  %s\n",strerror(errno),testFilename);
		fprintf(stderr,"fwrite: %s -- %s\n",strerror(errno),testFilename);
		shouldDisableHelium1=1;
		fclose(fp);
	    }	
	    else {
		fclose(fp);
		//Time for the read test
		fp=fopen(testFilename,"rb");
		if(fp==NULL) {

		    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),testFilename);
		    fprintf(stderr,"fopen: %s -- %s\n",strerror(errno),testFilename);
		    shouldDisableHelium1=1;
		}
		else {
		    numObjs=fread(readBackPattern,sizeof(char),TEST_PATTERN_SIZE,fp);
		    if(numObjs!=TEST_PATTERN_SIZE) {	
			syslog (LOG_ERR,"fread: %s ---  %s\n",strerror(errno),testFilename);
			fprintf(stderr,"fread: %s -- %s\n",strerror(errno),testFilename);
			shouldDisableHelium1=1;
			fclose(fp);
		    }	
		    else {
			//Time to check pattern
			fclose(fp);
			for(increment=0;increment<TEST_PATTERN_SIZE;increment++) {
			    if(readBackPattern[increment]!=testPattern[increment]) {
				shouldDisableHelium1=1;
				break;
			    }
			}
		    }
		}	    					
	    }
	}

    }
   

    if(!disableHelium1) {
	if(shouldDisableHelium1==1) {
	    bladeErrCounter++;
	    if(bladeErrCounter>3) {
		syslog(LOG_INFO,"Problem with satabblade -- will disable\n");
		theCmd.cmd[0]=CMD_DISABLE_DISK;
		theCmd.cmd[1]=HELIUM1_DISK_MASK;
		theCmd.cmd[2]=1;
		writeCommandAndLink(&theCmd);
	    
	    }
	}
    }



    if(!disableHelium2) {
	if(shouldDisableHelium2==1) {
	    miniErrCounter++;
	    if(miniErrCounter>3) {
		syslog(LOG_INFO,"Problem with satabmini -- will disable\n");
		theCmd.cmd[0]=CMD_DISABLE_DISK;
		theCmd.cmd[1]=HELIUM2_DISK_MASK;
		theCmd.cmd[2]=1;
		writeCommandAndLink(&theCmd);
	    
	    }
	}
    }


    printf("Sata check Helium2 (%d %d) and Helium1 (%d %d)\n",
	   shouldDisableHelium2,miniErrCounter,shouldDisableHelium1,bladeErrCounter);
}
