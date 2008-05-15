/*! \file Cmdd.c
    \brief The Cmdd program that receives commands from SIPd 
           and acts upon them 
    
    This is the updated Cmdd program which is actually a daemon.

    July 2005  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <libgen.h> //For Mac OS X

//Flightsoft includes
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "linkWatchLib/linkWatchLib.h"

#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"
#include "includes/anitaCommand.h"

int sameCommand(CommandStruct_t *theCmd, CommandStruct_t *lastCmd);
void copyCommand(CommandStruct_t *theCmd, CommandStruct_t *cpCmd);
int checkCommand(CommandStruct_t *theCmd);
int checkForNewCommand();
int executeCommand(CommandStruct_t *theCmd); //Returns unixTime or 0
int writeCommandEcho(CommandStruct_t *theCommand, int unixTime);
int sendSignal(ProgramId_t progId, int theSignal); 
int readConfig();
int cleanDirs();
int clearRamdisk();
int sendConfig(int progMask);
int defaultConfig(int progMask);
int lastConfig(int progMask);
int switchConfig(int progMask,unsigned char configId);
int saveConfig(int progMask,unsigned char configId);
int killPrograms(int progMask);
int reallyKillPrograms(int progMask);
int startPrograms(int progMask);
int respawnPrograms(int progMask);
int mountNextSata(int bladeOrMini, int whichNumber);
int mountNextSatablade(int whichSatablade);
int mountNextSatamini(int whichSatamini);
int mountNextUsb(int intExtFlag,int whichUsb);
void prepWriterStructs();
int requestFile(char *filename, int numLines);
int readAcqdConfig();
int readArchivedConfig();
int readLosdConfig();
int readSipdConfig();
int setEventDiskBitMask(int modOrSet, int bitMask);
int setHkDiskBitMask(int modOrSet, int bitMask);
int setPriDiskMask(int pri, int bitmask);
int setPriEncodingType(int pri, int encType);
int setAlternateUsb(int pri, int altUsb);
int setArchiveDefaultFrac(int pri, int frac);
int setArchiveDecimatePri(int pri, float frac);
int setTelemPriEncodingType(int pri, int encType, int encClockType);
int setPidGoalScaleFactor(int surf, int dac, float scaleFactor);
int setLosdPriorityBandwidth(int pri, int bw);
int setSipdPriorityBandwidth(int pri, int bw);
int executePrioritizerdCommand(int command, int value);
int executePlaybackCommand(int command, unsigned int value1, unsigned int value2);
int startNewRun();
int getNewRunNumber();
int getLatestEventNumber();
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);

#define MAX_COMMANNDS 100  //Hard to believe we can get to a hundred

/* Global variables */
int cmdLengths[256];

CommandStruct_t theCmds[MAX_COMMANNDS];

int numCmds=256;


//Debugging Output
int printToScreen=1;
int verbosity=1;

int disableSatablade;
int disableUsb;
int disableSatamini;
char satabladeName[FILENAME_MAX];
char usbName[FILENAME_MAX];
char sataminiName[FILENAME_MAX];

//Needed for commands
int surfTrigBandMasks[ACTIVE_SURFS][2];
int priDiskEncodingType[NUM_PRIORITIES];
int priTelemEncodingType[NUM_PRIORITIES];
int priTelemClockEncodingType[NUM_PRIORITIES];
float priorityFractionDefault[NUM_PRIORITIES];
float priorityFractionDelete[NUM_PRIORITIES];
int priDiskEventMask[NUM_PRIORITIES];
int alternateUsbs[NUM_PRIORITIES];
float pidGoalScaleFactors[ACTIVE_SURFS][SCALERS_PER_SURF];
int losdBandwidths[NUM_PRIORITIES];
int sipdBandwidths[NUM_PRIORITIES];


int hkDiskBitMask;
int eventDiskBitMask;
AnitaHkWriterStruct_t cmdWriter;

int diskBitMasks[DISK_TYPES]={SATABLADE_DISK_MASK,SATAMINI_DISK_MASK,USB_DISK_MASK,PMC_DISK_MASK};

char *diskNames[DISK_TYPES]={SATABLADE_DATA_MOUNT,SATAMINI_DATA_MOUNT,USB_DATA_MOUNT,SAFE_DATA_MOUNT};

int main (int argc, char *argv[])
{
    CommandStruct_t lastCmd;
    time_t lastCmdTime=0;
    time_t nowTime;
    int retVal;
    int count;
    int numCmds;

    /* Log stuff */
    char *progName=basename(argv[0]);
    retVal=sortOutPidFile(progName);
    if(retVal<0) {
	return -1; //Already have a Cmdd running
    }


     /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, handleBadSigs);
    signal(SIGINT, handleBadSigs);
    signal(SIGSEGV, handleBadSigs);


    makeDirectories(LOSD_CMD_ECHO_TELEM_LINK_DIR);
    makeDirectories(SIPD_CMD_ECHO_TELEM_LINK_DIR);
    makeDirectories(CMDD_COMMAND_LINK_DIR);
    makeDirectories(REQUEST_TELEM_LINK_DIR);
    makeDirectories(PLAYBACK_LINK_DIR);
    makeDirectories(LOGWATCH_LINK_DIR);

    do {
	retVal=readConfig();
	currentState=PROG_STATE_RUN;
        while(currentState==PROG_STATE_RUN) {
	    //Check for new commands
	    if((numCmds=checkForNewCommand())) {
		if(printToScreen) 
		    printf("Got %d commands\n",numCmds);		
		//Got at least one command
		for(count=0;count<numCmds;count++) {
		    if(printToScreen) 
			printf("Checking cmd %d, (%#x)\n",count,theCmds[count].cmd[0]);		
		    syslog(LOG_INFO,"Checking Cmd %d %d, numBytes %d\n",
			   theCmds[count].cmd[0],theCmds[count].cmd[1],
			   theCmds[count].numCmdBytes);
		    
		    if(checkCommand(&theCmds[count])) {
			if(printToScreen) 
			    printf("cmd %d good , (%#x)\n",count,theCmds[count].cmd[0]);		
			syslog(LOG_INFO,"Got Cmd %d, numBytes %d\n",
			       theCmds[count].cmd[0],theCmds[count].numCmdBytes);

			time(&nowTime);
			if((nowTime-lastCmdTime)>30 ||
			   !sameCommand(&theCmds[count],&lastCmd)) {
			    
			    retVal=executeCommand(&theCmds[count]);
			    lastCmdTime=retVal;
			    copyCommand(&theCmds[count],&lastCmd);
			}
			else {
			    printf("Same command: skipping\n");
			    retVal=lastCmdTime;
			}
			if(printToScreen)
			    printf("Writing command echo %d\n",count);
			writeCommandEcho(&theCmds[count],retVal);

		    }
		}
		
	    }
	    usleep(1000);
	}
	
    } while(currentState==PROG_STATE_INIT);
    closeHkFilesAndTidy(&cmdWriter);
    unlink(CMDD_PID_FILE);
    syslog(LOG_INFO,"Cmdd terminating");
    return 0;
}


int checkForNewCommand() {
  static int wd=0;
  int uptoCmd,retVal=0;
  char currentFilename[FILENAME_MAX];
  char currentLinkname[FILENAME_MAX];
  int numCmdLinks=0;
  char *tempString;

  if(wd==0) {
    //First time need to prep the watcher directory
    wd=setupLinkWatchDir(CMDD_COMMAND_LINK_DIR);
    if(wd<=0) {
      fprintf(stderr,"Unable to watch %s\n",CMDD_COMMAND_LINK_DIR);
      syslog(LOG_ERR,"Unable to watch %s\n",CMDD_COMMAND_LINK_DIR);
      exit(0);
    }
    numCmdLinks=getNumLinks(wd);
  }
    
  //Check for new inotify events
  retVal=checkLinkDirs(1,0);
  if(retVal || numCmdLinks)
    numCmdLinks=getNumLinks(wd);
  
  if(numCmdLinks) {
    //    printf("There are %d cmd links\n",numCmdLinks);
    for(uptoCmd=0;uptoCmd<numCmdLinks;uptoCmd++) {
      if(uptoCmd==MAX_COMMANNDS) break;
      tempString=getFirstLink(wd);
      sprintf(currentFilename,"%s/%s",CMDD_COMMAND_DIR,tempString);
      sprintf(currentLinkname,"%s/%s",CMDD_COMMAND_LINK_DIR,tempString);
      fillCommand(&theCmds[uptoCmd],currentFilename);
      removeFile(currentFilename);
      removeFile(currentLinkname);
    }
  }    
  return numCmdLinks;
}
    
int checkCommand(CommandStruct_t *theCmd) {
    if(theCmd->numCmdBytes!=cmdLengths[(int)theCmd->cmd[0]]) {
	syslog(LOG_ERR,"Wrong number of command bytes for %d: expected %d, got %d\n",theCmd->cmd[0],cmdLengths[(int)theCmd->cmd[0]],theCmd->numCmdBytes);	
	fprintf(stderr,"Wrong number of command bytes for %d: expected %d, got %d\n",theCmd->cmd[0],cmdLengths[(int)theCmd->cmd[0]],theCmd->numCmdBytes);
	return 0;
    }
    return 1;

}

int sortOutPidFile(char *progName)
{
  int retVal=checkPidFile(CMDD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,CMDD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(CMDD_PID_FILE);
  return 0;
}
    

int readConfig() {
    /* Config file thingies */
    int status=0;
    //    int retVal=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    char *tempString;

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status &= configLoad ("Cmdd.config","lengths") ;
    status &= configLoad ("Cmdd.config","output") ;
    eString = configErrorString (status) ;
    
    if (status == CONFIG_E_OK) {

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
	
	
	
	hkDiskBitMask=kvpGetInt("hkDiskBitMask",1);
	eventDiskBitMask=kvpGetInt("eventDiskBitMask",1);

	disableSatablade=kvpGetInt("disableSatablade",0);
	if(disableSatablade) {
	  hkDiskBitMask&=(~SATABLADE_DISK_MASK);
	  eventDiskBitMask&=(~SATABLADE_DISK_MASK);
	}
	disableSatamini=kvpGetInt("disableSatamini",0);
	if(disableSatamini) {
	  hkDiskBitMask&=(~SATAMINI_DISK_MASK);
	  eventDiskBitMask&=(~SATAMINI_DISK_MASK);
	}
	disableUsb=kvpGetInt("disableUsb",0);
	if(disableUsb) {
	  hkDiskBitMask&=(~USB_DISK_MASK);
	  eventDiskBitMask&=(~USB_DISK_MASK);
	}


	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_ERR,"Problem getting cmdLengths -- %s\n",
		    kvpErrorString(kvpStatus));
	    fprintf(stderr,"Problem getting cmdLengths -- %s\n",
		    kvpErrorString(kvpStatus));
	}
	
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }
    return status;

}

int executeCommand(CommandStruct_t *theCmd) 
//Returns 0 if there was a problem and unixTime otherwise
{
    printf("Have command %d\n",theCmd->cmd[0]);
    
    time_t rawtime;
    int retVal=0;
    int ivalue;
    int ivalue2;
    int ivalue3;
    short svalue;
    float fvalue;
    unsigned int uvalue,utemp;
    char theCommand[FILENAME_MAX];

    int index;
    int numDataProgs=8;
    int dataProgMasks[8]={HKD_ID_MASK,GPSD_ID_MASK,ARCHIVED_ID_MASK,
			  CALIBD_ID_MASK,MONITORD_ID_MASK,
			  PRIORITIZERD_ID_MASK,EVENTD_ID_MASK,ACQD_ID_MASK};
    
    char *dataPidFiles[8]={HKD_PID_FILE,GPSD_PID_FILE,ARCHIVED_PID_FILE,
			   CALIBD_PID_FILE,MONITORD_PID_FILE,PRIORITIZERD_PID_FILE,
			   EVENTD_PID_FILE,ACQD_PID_FILE};
    

    switch(theCmd->cmd[0]) {
	case CMD_MAKE_NEW_RUN_DIRS:
	    startNewRun();	    
	    time(&rawtime);
	    return rawtime;
	    break;
	case CMD_START_NEW_RUN:
    NEW_RUN:
	{
	    int sleepCount=0;
	    time(&rawtime);
	    for(index=0;index<numDataProgs;index++) {
		killPrograms(dataProgMasks[index]);
	    }
	    sleep(10); 
	    for(index=0;index<numDataProgs;index++) {
		int fileExists=0;
		do {
		    FILE *test =fopen(dataPidFiles[index],"r");
		    if(test==NULL) fileExists=0;
		    else {
			fclose(test);
			fileExists=1;
			sleepCount++;
			sleep(1);
		    }
		}
		while(fileExists && sleepCount<20);
	    }	   
	    sleep(10);
	    retVal=startNewRun();
	    if(retVal==-1) return 0;	    
	    time(&rawtime);
	    for(index=0;index<numDataProgs;index++) {
		startPrograms(dataProgMasks[index]);
	    }
	    return rawtime;
	    }
	case CMD_TAIL_VAR_LOG_MESSAGES:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return requestFile("/var/log/messages",ivalue);
	case CMD_TAIL_VAR_LOG_ANITA:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return requestFile("/var/log/anita.log",ivalue);
	case CMD_SHUTDOWN_HALT:
	    //Halt
	    time(&rawtime);
	    killPrograms(MONITORD_ID_MASK);
	    killPrograms(PLAYBACKD_ID_MASK);
	    killPrograms(HKD_ID_MASK);
	    killPrograms(GPSD_ID_MASK);
	    killPrograms(ARCHIVED_ID_MASK);
	    killPrograms(ACQD_ID_MASK);
	    killPrograms(CALIBD_ID_MASK);
	    killPrograms(PRIORITIZERD_ID_MASK);
	    killPrograms(EVENTD_ID_MASK);
	    sprintf(theCommand,"sudo halt");
	    retVal=system(theCommand);
	    if(retVal==-1) return 0;	    
	    time(&rawtime);
	    return rawtime;
	case CMD_REBOOT:
	    //Reboot
	    killPrograms(MONITORD_ID_MASK);
	    killPrograms(PLAYBACKD_ID_MASK);
	    killPrograms(HKD_ID_MASK);
	    killPrograms(GPSD_ID_MASK);
	    killPrograms(ARCHIVED_ID_MASK);
	    killPrograms(ACQD_ID_MASK);
	    killPrograms(CALIBD_ID_MASK);
	    killPrograms(PRIORITIZERD_ID_MASK);
	    killPrograms(EVENTD_ID_MASK);
	    sprintf(theCommand,"sudo reboot");
	    retVal=system(theCommand);
	    if(retVal==-1) return 0;	    
	    time(&rawtime);
	    return rawtime;
	case CMD_KILL_PROGS:
	    //Kill progs
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    retVal=killPrograms(ivalue);
	    return retVal;
	case CMD_REALLY_KILL_PROGS:
	    //Kill progs
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    retVal=reallyKillPrograms(ivalue);
	    return retVal;
	case CMD_RESPAWN_PROGS:
	    //Respawn progs
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    retVal=respawnPrograms(ivalue);	    
	    return retVal;
	case CMD_START_PROGS:
	    //Start progs
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    retVal=startPrograms(ivalue);	    
	    return retVal;
	case CMD_MOUNT:
	    //Mount -a
	  killPrograms(PLAYBACKD_ID_MASK);
	    killPrograms(MONITORD_ID_MASK);
	    killPrograms(HKD_ID_MASK);
	    killPrograms(GPSD_ID_MASK);
	    killPrograms(ARCHIVED_ID_MASK);
	    killPrograms(ACQD_ID_MASK);
	    killPrograms(CALIBD_ID_MASK);
	    killPrograms(PRIORITIZERD_ID_MASK);
	    killPrograms(EVENTD_ID_MASK);
	    retVal=system("sudo /sbin/rmmod usb-uhci");
	    retVal=system("sudo /sbin/insmod usb-uhci");
	    sleep(10);
	    retVal=system("sudo mount -a");
	    startPrograms(HKD_ID_MASK);
	    startPrograms(GPSD_ID_MASK);
	    startPrograms(ARCHIVED_ID_MASK);
	    startPrograms(ACQD_ID_MASK);
	    startPrograms(CALIBD_ID_MASK);
	    startPrograms(MONITORD_ID_MASK);
	    startPrograms(PRIORITIZERD_ID_MASK);
	    startPrograms(EVENTD_ID_MASK);
	    startPrograms(PLAYBACKD_ID_MASK);
	    if(retVal==-1) return 0;	    
	    time(&rawtime);
	    return rawtime;
	case CMD_MOUNT_NEXT_SATA:
	    ivalue=theCmd->cmd[1];	    
	    ivalue=theCmd->cmd[2];	    
	    return mountNextSata(ivalue,ivalue2);
	case CMD_MOUNT_NEXT_USB:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2];
	    return mountNextUsb(ivalue,ivalue2);
	case CMD_EVENT_DISKTYPE:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2]+(theCmd->cmd[3]<<8);
	    if(ivalue>=0 && ivalue<=2)
		return setEventDiskBitMask(ivalue,ivalue2);
	    return 0;
	case CMD_HK_DISKTYPE:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2]+(theCmd->cmd[3]<<8);
	    if(ivalue>=0 && ivalue<=2)
		return setHkDiskBitMask(ivalue,ivalue2);
	    return 0;
	case ARCHIVE_STORAGE_TYPE:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Archived.config","archived","onboardStorageType",ivalue,&rawtime);
	    respawnPrograms(ARCHIVED_ID_MASK);
	    return rawtime;
	case ARCHIVE_PRI_DISK:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2]+(theCmd->cmd[3]<<8);
	    return setPriDiskMask(ivalue,ivalue2);
	case ARCHIVE_PRI_ENC_TYPE:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2]+(theCmd->cmd[3]<<8);
	    return setPriEncodingType(ivalue,ivalue2);
	case ARCHIVE_ALTERNATE_USB:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2];
	    return setAlternateUsb(ivalue,ivalue2);
	case ARCHIVE_DEFAULT_FRAC:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2]+(theCmd->cmd[3]<<8);
	    return setArchiveDefaultFrac(ivalue,ivalue2);
	case ARCHIVE_DECIMATE_PRI:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2]+(theCmd->cmd[3]<<8);
	    fvalue=((float)ivalue2)/1000.;
	    return setArchiveDecimatePri(ivalue,fvalue);
	case TELEM_TYPE:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Archived.config","archived","telemType",ivalue,&rawtime);
	    respawnPrograms(ARCHIVED_ID_MASK);
	    return rawtime;
	case TELEM_PRI_ENC_TYPE:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2]+(theCmd->cmd[3]<<8);
	    ivalue3=theCmd->cmd[4]+(theCmd->cmd[5]<<8);
	    return setTelemPriEncodingType(ivalue,ivalue2,ivalue3);
	case ARCHIVE_PPS_PRIORITIES:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2];
	    if(ivalue<0 || ivalue>9) ivalue=-1;
	    if(ivalue2<0 || ivalue2>9) ivalue2=-1;
	    configModifyInt("Archived.config","archived","priorityPPS1",ivalue,NULL);
	    configModifyInt("Archived.config","archived","priorityPPS2",ivalue2,&rawtime);
	    retVal=sendSignal(ID_ARCHIVED,SIGUSR1);
	    return rawtime;
	case ARCHIVE_PPS_DECIMATE:
	    ivalue=theCmd->cmd[1];
	    if(ivalue<1 || ivalue>2) return -1;
	    ivalue2=theCmd->cmd[2]+(theCmd->cmd[3]<<8);
	    fvalue=((float)ivalue2)/1000.;
	    if(ivalue==1) {
		configModifyInt("Archived.config","archived","pps1FractionDelete",fvalue,&rawtime);
	    }
	    else if(ivalue==2) {
		configModifyInt("Archived.config","archived","pps2FractionDelete",fvalue,&rawtime);
	    }
	    retVal=sendSignal(ID_ARCHIVED,SIGUSR1);
	    return rawtime;
	    
	case CMD_TURN_GPS_ON:
	    //turnGPSOn
	    configModifyInt("Calibd.config","relays","stateGPS",1,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    sleep(5);
	    respawnPrograms(GPSD_ID_MASK);
	    if(retVal) return 0;
	    return rawtime;
	case CMD_TURN_GPS_OFF:
	    //turnGPSOff
	    configModifyInt("Calibd.config","relays","stateGPS",0,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case CMD_TURN_RFCM_ON:
	    ivalue=theCmd->cmd[1];
	    //turnRFCMOn
	    if(ivalue&0x1)
		configModifyInt("Calibd.config","relays","stateRFCM1",1,&rawtime);
	    if(ivalue&0x2)
		configModifyInt("Calibd.config","relays","stateRFCM2",1,&rawtime);
	    if(ivalue&0x4)
		configModifyInt("Calibd.config","relays","stateRFCM3",1,&rawtime);
	    if(ivalue&0x8)
		configModifyInt("Calibd.config","relays","stateRFCM4",1,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case CMD_TURN_RFCM_OFF:
	    ivalue=theCmd->cmd[1];
	    //turnRFCMOff
	    if(ivalue&0x1)
		configModifyInt("Calibd.config","relays","stateRFCM1",0,&rawtime);
	    
	    if(ivalue&0x2)
		configModifyInt("Calibd.config","relays","stateRFCM2",0,&rawtime);
	    
	    if(ivalue&0x4)
		configModifyInt("Calibd.config","relays","stateRFCM3",0,&rawtime);
	    
	    if(ivalue&0x8)
		configModifyInt("Calibd.config","relays","stateRFCM4",0,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	    
	case CMD_TURN_CALPULSER_ON:
	    //turnCalPulserOn
	    configModifyInt("Calibd.config","relays","stateCalPulser",1,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	    
	case CMD_TURN_CALPULSER_OFF:
	    //turnCalPulserOff
	    configModifyInt("Calibd.config","relays","stateCalPulser",0,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case CMD_TURN_VETO_ON:
	    //turnNDOn
	    configModifyInt("Calibd.config","relays","stateVeto",1,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case CMD_TURN_VETO_OFF:
	    //turnNDOff
	    configModifyInt("Calibd.config","relays","stateVeto",0,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	    return retVal;
	case CMD_TURN_ALL_ON:
	    //turnAllOn
	    configModifyInt("Calibd.config","relays","stateRFCM1",1,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM2",1,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM3",1,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM4",1,&rawtime);
	    configModifyInt("Calibd.config","relays","stateCalPulser",1,&rawtime);
	    configModifyInt("Calibd.config","relays","stateVeto",1,&rawtime);
	    configModifyInt("Calibd.config","relays","stateGPS",1,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;

	case CMD_TURN_ALL_OFF:
	    //turnAllOff
	    configModifyInt("Calibd.config","relays","stateRFCM1",0,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM2",0,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM3",0,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM4",0,&rawtime);
	    configModifyInt("Calibd.config","relays","stateCalPulser",0,&rawtime);
	    configModifyInt("Calibd.config","relays","stateVeto",0,&rawtime);
	    configModifyInt("Calibd.config","relays","stateGPS",0,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	    
	case SET_CALPULSER_SWITCH:
	    //setCalPulserSwitch
	    if(theCmd->cmd[1]>0 && theCmd->cmd[1]<5)
		configModifyInt("Calibd.config","rfSwitch","steadyState",theCmd->cmd[1],&rawtime);
	    else //Switch off steady state
		configModifyInt("Calibd.config","rfSwitch","steadyState",0,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	    
	case SET_CALPULSER_ATTEN:
	    //setCalPulserAtten
	    if(theCmd->cmd[1]<8) {
		configModifyInt("Calibd.config","attenuator","attenStartState",theCmd->cmd[1],&rawtime);
		configModifyInt("Calibd.config","attenuator","attenLoop",0,&rawtime);
	    }
	    else //Switch on looping
		configModifyInt("Calibd.config","attenuator","attenLoop",1,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_ATTEN_LOOP_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Calibd.config","attenuator","attenPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime; 
	case SET_SWITCH_LOOP_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Calibd.config","rfSwitch","switchPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime; 
	case SET_PULSER_OFF_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Calibd.config","rfSwitch","offPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime; 
	case SET_CALIB_WRITE_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Calibd.config","calibd","writePeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime; 

	    
	case SET_ADU5_PAT_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    fvalue=((float)ivalue)/10.;
//	    printf("ivalue: %d %x\t %x %x\n",ivalue,ivalue,theCmd->cmd[1],theCmd->cmd[2]);
	    configModifyFloat("GPSd.config","adu5","patPeriod",fvalue,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_ADU5_SAT_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("GPSd.config","adu5","satPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_ADU5_VTG_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","adu5","vtgPeriod",((float)ivalue),&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;	    
	case SET_G12_PPS_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","g12","ppsPeriod",((float)ivalue)/1000.,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;    
	case SET_G12_POS_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","g12","posPeriod",((float)ivalue)/1000.,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;	   	    	    
	case SET_G12_PPS_OFFSET: 
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    ivalue2=theCmd->cmd[3]+(theCmd->cmd[4]<<8);
	    ivalue3=theCmd->cmd[5];
	    fvalue=((float)ivalue) +((float)ivalue2)/10000.;
	    if(ivalue3==1) fvalue*=-1;	   

//	    printf("%d %d %d -- Offset %3.4f\n",ivalue, ivalue2, ivalue3,fvalue);
	    configModifyFloat("GPSd.config","g12","ppsOffset",fvalue,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;	    	    
	case SET_ADU5_CALIBRATION_12:
	    svalue=theCmd->cmd[1];
	    svalue|=(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV12_1",((float)svalue)/1000.,&rawtime);
	    svalue=theCmd->cmd[3];
	    svalue|=(theCmd->cmd[4]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV12_2",((float)svalue)/1000.,&rawtime);
	    svalue=theCmd->cmd[5];
	    svalue|=(theCmd->cmd[6]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV12_3",((float)svalue)/1000.,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_ADU5_CALIBRATION_13:
	    svalue=theCmd->cmd[1];
	    svalue|=(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV13_1",((float)svalue)/1000.,&rawtime);
	    svalue=theCmd->cmd[3];
	    svalue|=(theCmd->cmd[4]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV13_2",((float)svalue)/1000.,&rawtime);
	    svalue=theCmd->cmd[5];
	    svalue|=(theCmd->cmd[6]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV13_3",((float)svalue)/1000.,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_ADU5_CALIBRATION_14:
	    svalue=theCmd->cmd[1];
	    svalue|=(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV14_1",((float)svalue)/1000.,&rawtime);
	    svalue=theCmd->cmd[3];
	    svalue|=(theCmd->cmd[4]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV14_2",((float)svalue)/1000.,&rawtime);
	    svalue=theCmd->cmd[5];
	    svalue|=(theCmd->cmd[6]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV14_3",((float)svalue)/1000.,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_HK_PERIOD: 
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Hkd.config","hkd","readoutPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_HKD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_HK_CAL_PERIOD: 
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Hkd.config","hkd","calibrationPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_HKD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_HK_TELEM_EVERY: 
	    ivalue=theCmd->cmd[1];//+(theCmd->cmd[2]<<8);
	    configModifyInt("Hkd.config","hkd","telemEvery",ivalue,&rawtime);
	    retVal=sendSignal(ID_HKD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_ADU5_TRIG_FLAG:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","trigger","enablePPS1Trigger",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_G12_TRIG_FLAG: 
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","trigger","enablePPS2Trigger",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_SOFT_TRIG_FLAG: 	    
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","trigger","enableSoftTrigger",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_SOFT_TRIG_PERIOD:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","trigger","softTrigSleepPeriod",ivalue,NULL);
	    if(ivalue>0) 
		configModifyInt("Acqd.config","trigger","sendSoftTrig",1,&rawtime);
	    else
		configModifyInt("Acqd.config","trigger","sendSoftTrig",0,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_ENABLE_CHAN_SERVO: 	    
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","thresholds","enableChanServo",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);	    
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_SET_PID_GOAL: 	    
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Acqd.config","thresholds","pidGoal",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_PEDESTAL_RUN: 	    
	    ivalue=theCmd->cmd[1];
	    ivalue&=0x1;
	    configModifyInt("Acqd.config","acqd","pedestalMode",ivalue,&rawtime);
//	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    goto NEW_RUN;
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_THRESHOLD_SCAN: 	    
	    ivalue=theCmd->cmd[1];
	    ivalue&=0x1;
	    configModifyInt("Acqd.config","thresholdScan","doThresholdScan",ivalue,&rawtime);
//	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    goto NEW_RUN;
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_SET_ANT_TRIG_MASK: 	    
	    utemp=(theCmd->cmd[1]);	    
	    uvalue=utemp;
	    utemp=(theCmd->cmd[2]);
	    uvalue|=(utemp<<8);
	    utemp=(theCmd->cmd[3]);
	    uvalue|=(utemp<<16);
	    utemp=(theCmd->cmd[4]);
	    uvalue|=(utemp<<24);
	    syslog(LOG_INFO,"ACQD_SET_ANT_TRIG_MASK: %d %d %d %d -- %u -- %#x\n",theCmd->cmd[1],theCmd->cmd[2],
		   theCmd->cmd[3],theCmd->cmd[4],uvalue,(unsigned int)uvalue);

	    configModifyUnsignedInt("Acqd.config","thresholds","antTrigMask",uvalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_SET_SURF_BAND_TRIG_MASK: 	    
	    ivalue=(theCmd->cmd[1]); //Which Surf
	    ivalue2=(theCmd->cmd[2])+(theCmd->cmd[3]<<8);
	    ivalue3=(theCmd->cmd[4])+(theCmd->cmd[5]<<8);
	    readAcqdConfig();
	    if(ivalue>-1 && ivalue<9) {
		surfTrigBandMasks[ivalue][0]=ivalue2;
		surfTrigBandMasks[ivalue][1]=ivalue3;
		configModifyIntArray("Acqd.config","thresholds","surfTrigBandMasks",&surfTrigBandMasks[0][0],2*ACTIVE_SURFS,&rawtime);
	    }
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_SET_GLOBAL_THRESHOLD: 	    
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Acqd.config","thresholds","globalThreshold",ivalue,NULL);
	    if(ivalue>0) 
		configModifyInt("Acqd.config","thresholds","setGlobalThreshold",1,&rawtime);
	    else
		configModifyInt("Acqd.config","thresholds","setGlobalThreshold",0,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_REPROGRAM_TURF: 	    
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","acqd","reprogramTurf",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_SURFHK_PERIOD: 	    
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","acqd","surfHkPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_SURFHK_TELEM_EVERY: 	    
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","acqd","surfHkTelemEvery",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_TURFHK_TELEM_EVERY: 	    
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","acqd","turfRateTelemEvery",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_NUM_EVENTS_PEDESTAL: 	    
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Acqd.config","pedestal","numPedEvents",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_THRESH_SCAN_STEP_SIZE: 	    
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","thresholdScan","thresholdScanStepSize",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_THRESH_SCAN_POINTS_PER_STEP: 	    
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","thresholdScan","thresholdScanPointsPerStep",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_CHAN_PID_GOAL_SCALE:
	    ivalue=theCmd->cmd[1]; //surf 0:8
	    ivalue2=theCmd->cmd[2]; //dac 0:31
	    if(ivalue>8 || ivalue2>31) return 0;
	    ivalue3=theCmd->cmd[3]+(theCmd->cmd[4]<<8);
	    fvalue=((float)ivalue3)/1000.;
	    return setPidGoalScaleFactor(ivalue,ivalue2,fvalue);
	case ACQD_SET_RATE_SERVO:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    if(ivalue>0) {
		configModifyInt("Acqd.config","rates","enableRateServo",1,NULL);
		fvalue=((float)ivalue)/1000.;
		configModifyFloat("Acqd.config","rates","rateGoal",fvalue,&rawtime);
	    }
	    else {
		configModifyInt("Acqd.config","rates","enableRateServo",0,&rawtime);	    
	    }
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    return rawtime;
	case ACQD_SET_NICE_VALUE:
	    ivalue=theCmd->cmd[1]; //Between 0 and 255
	    ivalue-=20; //-20 is the lowest;
	    configModifyInt("Acqd.config","acqd","niceValue",ivalue,&rawtime);	
	    retVal=sendSignal(ID_ACQD,SIGUSR1);    
	    return rawtime;
	case SIPD_SEND_WAVE:
	    ivalue=theCmd->cmd[1];
	    if(ivalue<0 || ivalue>1) return -1;
	    configModifyInt("SIPd.config","sipd","sendWavePackets",ivalue,&rawtime);	
	    retVal=sendSignal(ID_SIPD,SIGUSR1);    
	    return rawtime;
	case SIPD_THROTTLE_RATE:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8); 
	    if(ivalue>680) return -1;
	    configModifyInt("SIPd.config","sipd","throttleRate",ivalue,&rawtime);	
	    retVal=sendSignal(ID_SIPD,SIGUSR1);    
	    return rawtime;
	case SIPD_PRIORITY_BANDWIDTH:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2];
	    if(ivalue<0 || ivalue>9) return -1;
	    if(ivalue2<0 || ivalue2>100) return -1;
	    return setSipdPriorityBandwidth(ivalue,ivalue2);
	case LOSD_PRIORITY_BANDWIDTH:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2];
	    if(ivalue<0 || ivalue>9) return -1;
	    if(ivalue2<0 || ivalue2>100) return -1;
	    return setLosdPriorityBandwidth(ivalue,ivalue2);
	case LOSD_SEND_DATA:
	    ivalue=theCmd->cmd[1];
	    if(ivalue>1 || ivalue<0) return -1;
	    configModifyInt("LOSd.config","losd","sendData",ivalue,&rawtime);	
	    retVal=sendSignal(ID_LOSD,SIGUSR1);    
	    return rawtime;	
	    
	case MONITORD_RAMDISK_KILL_ACQD:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Monitord.config","monitord","ramdiskKillAcqd",ivalue,&rawtime);	
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;		    
	case MONITORD_RAMDISK_DUMP_DATA:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Monitord.config","monitord","ramdiskDumpData",ivalue,&rawtime);	
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;	
	    
	    
	case MONITORD_MAX_ACQD_WAIT:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Monitord.config","monitord","maxAcqdWaitPeriod",ivalue,&rawtime);	
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;		    
	case MONITORD_PERIOD:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Monitord.config","monitord","monitorPeriod",ivalue,&rawtime);	
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;		    
	case MONITORD_USB_THRESH:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Monitord.config","monitord","usbSwitchMB",ivalue,&rawtime);	
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;		    
	case MONITORD_SATA_THRESH:
	    ivalue=theCmd->cmd[1];
	    ivalue2=theCmd->cmd[2];
	    if(ivalue==0) 
	      configModifyInt("Monitord.config","monitord","satabladeSwitchMB",ivalue2,&rawtime);	
	    else if(ivalue==1) 
	      configModifyInt("Monitord.config","monitord","sataminiSwitchMB",ivalue2,&rawtime);	
	    else
	      return -1;
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;
	case MONITORD_MAX_QUEUE:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Monitord.config","monitord","maxEventQueueSize",ivalue,&rawtime);	
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;
	case MONITORD_INODES_KILL_ACQD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Monitord.config","monitord","inodesKillAcqd",ivalue,&rawtime);	
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;
	case MONITORD_INODES_DUMP_DATA:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("Monitord.config","monitord","inodesDumpData",ivalue,&rawtime);	
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;
	case PRIORITIZERD_COMMAND:
	    ivalue=theCmd->cmd[1]; //Command num
	    ivalue2=theCmd->cmd[2]+(theCmd->cmd[3]<<8); //command value
	    return executePrioritizerdCommand(ivalue,ivalue2);
	case PLAYBACKD_COMMAND:
	    ivalue=theCmd->cmd[1]; //Command num	    
	    utemp=(theCmd->cmd[2]);	    
	    uvalue=utemp;
	    utemp=(theCmd->cmd[3]);
	    uvalue|=(utemp<<8);
	    utemp=(theCmd->cmd[4]);
	    uvalue|=(utemp<<16);
	    utemp=(theCmd->cmd[5]);
	    uvalue|=(utemp<<24);	    
	    ivalue2=theCmd->cmd[6];//+(theCmd->cmd[3]<<8); //command value
	    return executePlaybackCommand(ivalue,uvalue,ivalue2);		    
	case EVENTD_MATCH_GPS:
	    ivalue=theCmd->cmd[1];
	    if(ivalue<0 || ivalue>1) return -1;
	    configModifyInt("Eventd.config","eventd","tryToMatchGps",ivalue,&rawtime);	
	    retVal=sendSignal(ID_MONITORD,SIGUSR1);    
	    return rawtime;	
	case CLEAN_DIRS: 	    
	    cleanDirs();	    
	    return rawtime;
	case CLEAR_RAMDISK: 	    
	    clearRamdisk();
	    return rawtime;
	case SEND_CONFIG: 
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return sendConfig(ivalue);	    
	case DEFAULT_CONFIG:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return defaultConfig(ivalue);
	case SWITCH_CONFIG:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return switchConfig(ivalue,theCmd->cmd[3]);
	case SAVE_CONFIG:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return saveConfig(ivalue,theCmd->cmd[3]);
	case LAST_CONFIG:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return lastConfig(ivalue);
	default:
	    syslog(LOG_WARNING,"Unrecognised command %d\n",theCmd->cmd[0]);
	    return retVal;
    }
    return -1;
}

int sendSignal(ProgramId_t progId, int theSignal) 
{
    int retVal=0;
    FILE *fpPid ;
    char fileName[FILENAME_MAX];
    pid_t thePid;
    switch(progId) {
	case ID_ACQD:
	    sprintf(fileName,"%s",ACQD_PID_FILE);
	    break;
	case ID_ARCHIVED:
	    sprintf(fileName,"%s",ARCHIVED_PID_FILE);
	    break;
	case ID_EVENTD:
	    sprintf(fileName,"%s",EVENTD_PID_FILE);
	    break;
	case ID_CALIBD:
	    sprintf(fileName,"%s",CALIBD_PID_FILE);
	    break;
	case ID_GPSD:
	    sprintf(fileName,"%s",GPSD_PID_FILE);
	    break;
	case ID_HKD:
	    sprintf(fileName,"%s",HKD_PID_FILE);
	    break;
	case ID_LOSD:
	    sprintf(fileName,"%s",LOSD_PID_FILE);
	    break;
	case ID_PRIORITIZERD:
	    sprintf(fileName,"%s",PRIORITIZERD_PID_FILE);
	    break;
	case ID_MONITORD:
	    sprintf(fileName,"%s",MONITORD_PID_FILE);
	    break;
	case ID_SIPD:
	    sprintf(fileName,"%s",SIPD_PID_FILE);
	    break;    
//	case ID_MONITORD:
//	    sprintf(fileName,"%s",monitord);
//	    break;    
	default:
	    fprintf(stderr,"Unknown program id: %d\n",progId);
	    syslog(LOG_ERR,"Unknown program id: %d\n",progId);
    }
    if (!(fpPid=fopen(fileName, "r"))) {
	fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	return -1;
    }
    fscanf(fpPid,"%d", &thePid) ;
    fclose(fpPid) ;
    
    retVal=kill(thePid,theSignal);
    if(retVal!=0) {
	syslog(LOG_ERR,"Error sending %d to pid %d:\t%s",theSignal,thePid,strerror(errno));
	fprintf(stderr,"Error sending %d to pid %d:\t%s\n",theSignal,thePid,strerror(errno));
    }
    return retVal;
} 

int cleanDirs()
{
    

    return 0;
}

int sendConfig(int progMask) 
{
    time_t rawtime=0;
    int errorCount=0;
    ProgramId_t prog;
    char configFile[FILENAME_MAX];
    char *fullFilename;
    int testMask;	

    printf("sendConfig %d\n",progMask);
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
//	printf("%d %d\n",progMask,testMask);
	if(progMask&testMask) {
	    sprintf(configFile,"%s.config",getProgName(prog));
	    printf("Trying to send config file -- %s\n",configFile);
	    fullFilename=configFileSpec(configFile);
	    rawtime=requestFile(fullFilename,0);
	}
    }
    if(errorCount) return 0;
    return rawtime;

}

int defaultConfig(int progMask)
{
    return switchConfig(progMask,0);
}

int switchConfig(int progMask, unsigned char configId) 
{
    time_t rawtime;
//    time(&rawtime);
    int retVal;
    int errorCount=0;
    ProgramId_t prog;
    char configFile[FILENAME_MAX];
    
    int testMask;	

    printf("switchConfig %d %d\n",progMask,configId);
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
//	printf("%d %d\n",progMask,testMask);
	if(progMask&testMask) {
	    printf("Switching to config %s.config.%d\n",getProgName(prog),configId);
	    sprintf(configFile,"%s.config",getProgName(prog));
	    retVal=configSwitch(configFile,configId,&rawtime);
	    if(retVal!=CONFIG_E_OK) 
		errorCount++;
	    else {
		retVal=sendSignal(prog,SIGUSR1);
		if(retVal) 
		    errorCount++;
	    }

	}
    }
    if(errorCount) return 0;
    return rawtime;
}


int saveConfig(int progMask, unsigned char configId) 
{
//    if(configId<10) return -1;
    time_t rawtime=time(NULL);
//    time(&rawtime);
    int retVal;
    int errorCount=0;
    ProgramId_t prog;
    char configFile[FILENAME_MAX];
    char configFileNew[FILENAME_MAX];
    
    int testMask;	

    printf("saveConfig %d %d\n",progMask,configId);
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
//	printf("%d %d\n",progMask,testMask);
	if(progMask&testMask) {
	    sprintf(configFile,"/home/anita/flightSoft/config/%s.config",
		    getProgName(prog));
	    sprintf(configFileNew,"/home/anita/flightSoft/config/defaults/%s.config.%d",
		    getProgName(prog),configId);

	    retVal=copyFileToFile(configFile,configFileNew);

	}
    }
    if(errorCount) return 0;
    return rawtime;
}



int lastConfig(int progMask)
{
    time_t rawtime;
//    time(&rawtime);
    int retVal;
    int errorCount=0;
    ProgramId_t prog;
    char configFile[FILENAME_MAX];
    
    int testMask;	

    printf("lastConfig %x\n",progMask);
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
//	printf("%d %d\n",progMask,testMask);
	if(progMask&testMask) {
	    printf("Switching to last config %s.config\n",getProgName(prog));
	    sprintf(configFile,"%s.config",getProgName(prog));
	    retVal=configSwitchToLast(configFile,&rawtime);
	    if(retVal!=CONFIG_E_OK) 
		errorCount++;
	    else {
		retVal=sendSignal(prog,SIGUSR1);
		if(retVal) 
		    errorCount++;
	    }
	    
	}
    }
    if(errorCount) return 0;
    return 0;
}






int killPrograms(int progMask) 
{
    time_t rawtime;
    time(&rawtime);
    char daemonCommand[FILENAME_MAX];
    int retVal=0;
    int errorCount=0;
    ProgramId_t prog;
    
    int testMask;	
    syslog(LOG_INFO,"killPrograms %#x\n",progMask);
//    printf("Kill programs %d\n",progMask);
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
//	printf("%d %d\n",progMask,testMask);
	if(progMask&testMask) {
//	    printf("Killing prog %s\n",getProgName(prog));
	    sendSignal(prog,SIGUSR2);
	    sprintf(daemonCommand,"daemon --stop -n %s",getProgName(prog));
	    syslog(LOG_INFO,"Sending command: %s",daemonCommand);
	    retVal=system(daemonCommand);
	    if(retVal!=0) {
		errorCount++;
		syslog(LOG_ERR,"Error killing %s",getProgName(prog));
		//Maybe do something clever
	    }
	    else {
		syslog(LOG_INFO,"Killed %s\n",getProgName(prog));
	    }
	    if(prog==ID_EVENTD) {
		sleep(1);
		system("killall -9 Eventd");
	    }
	}
    }
    if(errorCount) return 0;
    return rawtime;
}


int reallyKillPrograms(int progMask) 
{
    time_t rawtime;
    time(&rawtime);
    char fileName[FILENAME_MAX];
    char systemCommand[FILENAME_MAX];
    int retVal=0;
    int errorCount=0;
    ProgramId_t prog;
    
    int testMask;	
    syslog(LOG_INFO,"reallyKillPrograms %#x\n",progMask);
//    printf("Kill programs %d\n",progMask);
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
//	printf("%d %d\n",progMask,testMask);
	if(progMask&testMask) {
	    sprintf(fileName,"%s",getPidFile(prog));
	    removeFile(fileName);
//	    printf("Killing prog %s\n",getProgName(prog));
	    
	    sprintf(systemCommand,"killall -9 %s",getProgName(prog));
	    retVal=system(systemCommand);
	    syslog(LOG_INFO,"killall -9 %s\n",getProgName(prog));
	}
    }
    if(errorCount) return 0;
    return rawtime;
}


int startPrograms(int progMask) 
{
    time_t rawtime;
    time(&rawtime);
    char daemonCommand[FILENAME_MAX];
    int retVal=0;
    ProgramId_t prog;
    int testMask;	
    int errorCount=0;
    syslog(LOG_INFO,"startPrograms %#x\n",progMask);
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
	if(progMask&testMask) {
	    //Match
	    if(prog==ID_PRIORITIZERD) {
		sprintf(daemonCommand,"nice -n 15 daemon -r %s -n %s ",
			getProgName(prog),getProgName(prog));
	    }
	    else {
		sprintf(daemonCommand,"nice -n 20 daemon -r %s -n %s ",
			getProgName(prog),getProgName(prog));
	    }

	    syslog(LOG_INFO,"Sending command: %s",daemonCommand);
	    retVal=system(daemonCommand);
	    if(retVal!=0) {
		errorCount++;
		syslog(LOG_ERR,"Error starting %s",getProgName(prog));
		//Maybe do something clever
	    }
	}
    }
    if(errorCount) return 0;
    return rawtime;
}

int respawnPrograms(int progMask)
{   
    time_t rawtime;
    time(&rawtime);
    FILE *fpPid ;
    char fileName[FILENAME_MAX];
    pid_t thePid;
    ProgramId_t prog;
    int retVal=0;
    int testMask;	
    int errorCount=0;
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
	if(progMask&testMask) {
	    sprintf(fileName,"%s",getPidFile(prog));
	    if (!(fpPid=fopen(fileName, "r"))) {
		errorCount++;
		fprintf(stderr," failed to open a file for PID, %s\n", fileName);
		syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
		startPrograms(testMask);
	    }
	    else {
		fscanf(fpPid,"%d", &thePid) ;
		fclose(fpPid) ;
		retVal=kill(thePid,SIGUSR2);
	    }
	} 
    }
    if(errorCount) return 0;
    return rawtime;
	
}

int setEventDiskBitMask(int modOrSet, int bitMask)
{
    int retVal;
    time_t rawtime;
    readConfig(); //Get Latest bit mask
    if(modOrSet==0) {
	//Then we are in disabling mode
	eventDiskBitMask&=(~bitMask);
    }
    else if(modOrSet==1) {
	//Then we are in enabling mode
	eventDiskBitMask|=bitMask;
    }
    else if(modOrSet==2) {
	//Then we are in setting mode
	eventDiskBitMask=bitMask;
    }
    else return 0;
    configModifyUnsignedInt("anitaSoft.config","global","eventDiskBitMask",eventDiskBitMask,&rawtime);
    retVal=respawnPrograms(ARCHIVED_ID_MASK);
    return rawtime;
}


int setHkDiskBitMask(int modOrSet, int bitMask)
{
    time_t rawtime;
    readConfig(); //Get Latest bit mask
    if(modOrSet==0) {
	//Then we are in disabling mode
	hkDiskBitMask&=(~bitMask);
    }
    else if(modOrSet==1) {
	//Then we are in enabling mode
	hkDiskBitMask|=bitMask;
    }
    else if(modOrSet==2) {
	//Then we are in setting mode
	hkDiskBitMask=bitMask;
    } 
    else return 0;
    configModifyUnsignedInt("anitaSoft.config","global","hkDiskBitMask",hkDiskBitMask,&rawtime);  
    closeHkFilesAndTidy(&cmdWriter); 
    prepWriterStructs();
    respawnPrograms(HKD_ID_MASK);
    respawnPrograms(GPSD_ID_MASK);
    respawnPrograms(ACQD_ID_MASK);
    respawnPrograms(CALIBD_ID_MASK);
    respawnPrograms(MONITORD_ID_MASK);
    return rawtime;
}

int setPriDiskMask(int pri, int bitmask)
{
    time_t rawtime;
    readArchivedConfig();
    priDiskEventMask[pri]=bitmask;
    configModifyIntArray("Archived.config","archived","priDiskEventMask",priDiskEventMask,NUM_PRIORITIES,&rawtime);
    respawnPrograms(ARCHIVED_ID_MASK);
    return rawtime;
}

int setPriEncodingType(int pri, int encType)
{
    time_t rawtime;
    readArchivedConfig();
    priDiskEncodingType[pri]=encType;
    configModifyIntArray("Archived.config","archived","priDiskEncodingType",priDiskEncodingType,NUM_PRIORITIES,&rawtime);
    sendSignal(ID_ARCHIVED,SIGUSR1);
    return rawtime;
}

int setAlternateUsb(int pri, int altUsb)
{
    time_t rawtime;
    readArchivedConfig();
    alternateUsbs[pri]=altUsb;
    configModifyIntArray("Archived.config","archived","alternateUsbs",alternateUsbs,NUM_PRIORITIES,&rawtime);
    sendSignal(ID_ARCHIVED,SIGUSR1);
    return rawtime;
}


int setArchiveDefaultFrac(int pri, int frac)
{
 /*    time_t rawtime; */
/*     readArchivedConfig(); */
/*     priorityFractionDefault[pri]=frac; */
/*     configModifyIntArray("Archived.config","archived","priorityFractionDefault",priorityFractionDefault,NUM_PRIORITIES,&rawtime); */
/*     sendSignal(ID_ARCHIVED,SIGUSR1); */
/*     return rawtime; */
    return 0;
}

int setArchiveDecimatePri(int pri, float frac)
{
    time_t rawtime;
    readArchivedConfig();
    priorityFractionDelete[pri]=frac;
    configModifyFloatArray("Archived.config","archived","priorityFractionDelete",priorityFractionDelete,NUM_PRIORITIES,&rawtime);
    sendSignal(ID_ARCHIVED,SIGUSR1);
    return rawtime;
    return 0;
}


int setLosdPriorityBandwidth(int pri, int bw)
{
    time_t rawtime;
    readLosdConfig();
    losdBandwidths[pri]=bw;
    configModifyIntArray("LOSd.config","bandwidth","priorityBandwidths",losdBandwidths,NUM_PRIORITIES,&rawtime);
    sendSignal(ID_LOSD,SIGUSR1);
    return rawtime;    
}

int setSipdPriorityBandwidth(int pri, int bw)
{
    time_t rawtime;
    readSipdConfig();
    sipdBandwidths[pri]=bw;
    configModifyIntArray("SIPd.config","bandwidth","priorityBandwidths",sipdBandwidths,NUM_PRIORITIES,&rawtime);
    sendSignal(ID_SIPD,SIGUSR1);
    return rawtime;    

}


int executePlaybackCommand(int command, unsigned int value1, unsigned int value2)
{
    
    PlaybackRequest_t pReq;
    time_t rawtime;    
    char filename[FILENAME_MAX];
    switch(command) {
	case PLAY_GET_EVENT:
	    pReq.eventNumber=value1;
	    pReq.pri=value2;
//	    printf("event %u, pri %d\n",value1,value2);
	    sprintf(filename,"%s/play_%u.dat",PLAYBACK_DIR,pReq.eventNumber);
	    normalSingleWrite((unsigned char*)&pReq,filename,sizeof(PlaybackRequest_t));
	    makeLink(filename,PLAYBACK_LINK_DIR);
	    rawtime=time(NULL);
	    break;
	case PLAY_START_PRI:

	    configModifyInt("Playbackd.config","playbackd","sendData",1,&rawtime);
	    system("killall Playbackd");
	    break;
	case PLAY_STOP_PRI:
	    configModifyInt("Playbackd.config","playbackd","sendData",0,&rawtime);	    
	    system("killall Playbackd");
	    break;
	default:
	    return -1;
    }
	      
    return rawtime;
}


int executePrioritizerdCommand(int command, int value)
{
    time_t rawtime;
    switch(command) {
	case PRI_HORN_THRESH:
	    configModifyFloat("Prioritizerd.config","prioritizerd","hornThresh",(float)value,&rawtime);
	    break;
	case PRI_HORN_DESC_WIDTH:
	    configModifyInt("Prioritizerd.config","prioritizerd","hornDiscWidth",value,&rawtime);
	    break;
	case PRI_HORN_SECTOR_WIDTH:
	    configModifyInt("Prioritizerd.config","prioritizerd","hornSectorWidth",value,&rawtime);
	    break;
	case PRI_CONE_THRESH:
	    configModifyFloat("Prioritizerd.config","prioritizerd","coneThresh",(float)value,&rawtime);
	    break;	    
	case PRI_CONE_DESC_WIDTH:
	    configModifyInt("Prioritizerd.config","prioritizerd","coneDiscWidth",value,&rawtime);
	    break;
	case PRI_HOLDOFF:
	    configModifyInt("Prioritizerd.config","prioritizerd","holdoff",value,&rawtime);
	    break;	    
	case PRI_DELAY:
	    configModifyInt("Prioritizerd.config","prioritizerd","delay",value,&rawtime);
	    break;	    
	case PRI_HORN_GUARD_OFFSET:
	    configModifyInt("Prioritizerd.config","prioritizerd","hornGuardOffset",value,&rawtime);
	    break;	    
	case PRI_HORN_GUARD_WIDTH:
	    configModifyInt("Prioritizerd.config","prioritizerd","hornGuardWidth",value,&rawtime);
	    break;	    	    
	case PRI_HORN_GUARD_THRESH:
	    configModifyInt("Prioritizerd.config","prioritizerd","hornGuardThresh",value,&rawtime);
	    break;	    
	case PRI_CONE_GUARD_OFFSET:
	    configModifyInt("Prioritizerd.config","prioritizerd","coneGuardOffset",value,&rawtime);
	    break;	    
	case PRI_CONE_GUARD_WIDTH:
	    configModifyInt("Prioritizerd.config","prioritizerd","coneGuardWidth",value,&rawtime);
	    break;	    
	case PRI_CONE_GUARD_THRESH:
	    configModifyInt("Prioritizerd.config","prioritizerd","coneGuardThresh",value,&rawtime);
	    break;	    	    
	case PRI_FFT_PEAK_MAX_A:
	    configModifyInt("Prioritizerd.config","prioritizerd","FFTPeakMaxA",value,&rawtime);
	    break;	    	    	    	    
	case PRI_FFT_PEAK_MAX_B:
	    configModifyInt("Prioritizerd.config","prioritizerd","FFTPeakMaxB",value,&rawtime);
	    break;	    	  	    
	case PRI_RMS_MAX:
	    configModifyInt("Prioritizerd.config","prioritizerd","RMSMax",value,&rawtime);
	    break;	    	  	    
	case PRI_RMS_EVENTS:
	    configModifyInt("Prioritizerd.config","prioritizerd","RMSevents",value,&rawtime);
	    break;	    	    
	case PRI_WINDOW_CUT:
	    configModifyInt("Prioritizerd.config","prioritizerd","RMSevents",value,&rawtime);
	    break;	    	    
	case PRI_BEGIN_WINDOW:
	    configModifyInt("Prioritizerd.config","prioritizerd","BeginWindow",value,&rawtime);
	    break;	    	    
	case PRI_END_WINDOW:
	    configModifyInt("Prioritizerd.config","prioritizerd","EndWindow",value,&rawtime);
	    break;	    	    
	case PRI_METHOD_MASK:
	    configModifyInt("Prioritizerd.config","prioritizerd","MethodMask",value,&rawtime);
	    break;	   	    
	case PRI_FFT_MAX_CHANNELS:
	    configModifyInt("Prioritizerd.config","prioritizerd","FFTMaxChannels",value,&rawtime);
	    break;
	case PRI_FFT_PEAK_WINDOW_L:
	    configModifyInt("Prioritizerd.config","prioritizerd","FFTPeakWindowL",value,&rawtime);
	    break;	    
	case PRI_FFT_PEAK_WINDOW_R:
	    configModifyInt("Prioritizerd.config","prioritizerd","FFTPeakWindowR",value,&rawtime);
	    break;	    
	case PRI_NU_CUT:
	    configModifyInt("Prioritizerd.config","prioritizerd","NuCut",value,&rawtime);
	    break;	    	   
	default:
	    return -1;
    }	    
    return rawtime;
}


int setTelemPriEncodingType(int pri, int encType,int encClockType)
{
    time_t rawtime;
    readArchivedConfig();
    priTelemEncodingType[pri]=encType;
    priTelemClockEncodingType[pri]=encClockType;
    configModifyIntArray("Archived.config","archived","priTelemEncodingType",priTelemEncodingType,NUM_PRIORITIES,&rawtime);
    configModifyIntArray("Archived.config","archived","priTelemClockEncodingType",priTelemClockEncodingType,NUM_PRIORITIES,&rawtime);
    sendSignal(ID_ARCHIVED,SIGUSR1);
    return rawtime;
}

int setPidGoalScaleFactor(int surf, int dac, float scaleFactor) 
{
    time_t rawtime;
    char keyName[180];
    readAcqdConfig();
    pidGoalScaleFactors[surf][dac]=scaleFactor;
    sprintf(keyName,"pidGoalScaleSurf%d",surf+1);
    configModifyFloatArray("Acqd.config","thresholds",keyName,&(pidGoalScaleFactors[surf][0]),SCALERS_PER_SURF,&rawtime);
    sendSignal(ID_ACQD,SIGUSR1);
    return rawtime;
}


int mountNextSata(int bladeOrMini,int whichNumber) {
  //0 is blade, 1 is mini
  if(bladeOrMini==0)
    return mountNextSatablade(whichNumber);
  else if(bladeOrMini==1)
    return mountNextSatamini(whichNumber);
  return -1;
}

int mountNextSatablade(int whichSatablade) {
    int retVal;
    time_t rawtime;
    int currentNum=0;
    readConfig(); // Get latest names and status
    //    if(disableSatablade) return -1;
    sscanf(satabladeName,"satablade%d",&currentNum);
    if(whichSatablade==0)
	currentNum++;
    else {
	currentNum=whichSatablade;
    }
       
    if(currentNum>NUM_SATABLADES) return -1;

    //Kill all programs that write to disk
    killPrograms(MONITORD_ID_MASK);
    killPrograms(PLAYBACKD_ID_MASK);
    killPrograms(HKD_ID_MASK);
    killPrograms(GPSD_ID_MASK);
    killPrograms(ARCHIVED_ID_MASK);
    killPrograms(ACQD_ID_MASK);
    killPrograms(CALIBD_ID_MASK);
    killPrograms(PRIORITIZERD_ID_MASK);
    killPrograms(EVENTD_ID_MASK);
    closeHkFilesAndTidy(&cmdWriter);    
    sleep(5); //Let them gzip their data


//    exit(0);
    
    //Change to new drive
    sprintf(satabladeName,"satablade%d",currentNum);
    configModifyString("anitaSoft.config","global","satabladeName",satabladeName,&rawtime);
    retVal=system("/home/anita/flightSoft/bin/mountCurrentSatablade.sh");
    sleep(5);
    startNewRun();
    sleep(10);
    prepWriterStructs();
    startPrograms(HKD_ID_MASK);
    startPrograms(GPSD_ID_MASK);
    startPrograms(ARCHIVED_ID_MASK);
    startPrograms(ACQD_ID_MASK);
    startPrograms(CALIBD_ID_MASK);
    startPrograms(MONITORD_ID_MASK);
    startPrograms(PLAYBACKD_ID_MASK);
    startPrograms(PRIORITIZERD_ID_MASK);
    startPrograms(EVENTD_ID_MASK);
    return rawtime;
}


int mountNextSatamini(int whichSatamini) {
    int retVal;
    time_t rawtime;
    int currentNum=0;
    readConfig(); // Get latest names and status
    //    if(disableSatamini) return -1;
    sscanf(sataminiName,"satamini%d",&currentNum);
    if(whichSatamini==0)
	currentNum++;
    else {
	currentNum=whichSatamini;
    }
       
    if(currentNum>NUM_SATAMINIS) return -1;

    //Kill all programs that write to disk
    killPrograms(MONITORD_ID_MASK);
    killPrograms(PLAYBACKD_ID_MASK);
    killPrograms(HKD_ID_MASK);
    killPrograms(GPSD_ID_MASK);
    killPrograms(ARCHIVED_ID_MASK);
    killPrograms(ACQD_ID_MASK);
    killPrograms(CALIBD_ID_MASK);
    killPrograms(PRIORITIZERD_ID_MASK);
    killPrograms(EVENTD_ID_MASK);
    closeHkFilesAndTidy(&cmdWriter);    
    sleep(5); //Let them gzip their data


//    exit(0);
    
    //Change to new drive
    sprintf(sataminiName,"satamini%d",currentNum);
    configModifyString("anitaSoft.config","global","sataminiName",sataminiName,&rawtime);
    retVal=system("/home/anita/flightSoft/bin/mountCurrentSatamini.sh");
    sleep(5);
    startNewRun();
    sleep(10);
    prepWriterStructs();
    startPrograms(HKD_ID_MASK);
    startPrograms(GPSD_ID_MASK);
    startPrograms(ARCHIVED_ID_MASK);
    startPrograms(ACQD_ID_MASK);
    startPrograms(CALIBD_ID_MASK);
    startPrograms(MONITORD_ID_MASK);
    startPrograms(PLAYBACKD_ID_MASK);
    startPrograms(PRIORITIZERD_ID_MASK);
    startPrograms(EVENTD_ID_MASK);
    return rawtime;
}

int mountNextUsb(int intExtFlag, int whichUsb) {
    int retVal;
    time_t rawtime;
    int currentNum[2]={0};
    intExtFlag=1; //No externals anymore
    readConfig(); // Get latest names and status

    //    if(hkDiskBitMask&USB_DISK_MASK) disableUsb=0;
    //    if(eventDiskBitMask&USB_DISK_MASK) disableUsb=0;

    //    if(hkDiskBitMask&USBEXT_DISK_MASK) disableUsbExt=0;
    //    if(eventDiskBitMask&USBEXT_DISK_MASK) disableUsbExt=0;

    //    if(intExtFlag<1 || intExtFlag>3) return 0;

    //    if(intExtFlag==1 && disableUsb) return 0;
    //    else if(intExtFlag==2 && disableUsbExt) return 0;
    //    else if(intExtFlag==3 && disableUsbExt && disableUsb) return 0;
    //    else if(intExtFlag==3 && disableUsb) intExtFlag=2;
    //    else if(intExtFlag==3 && disableUsbExt) intExtFlag=1;

    sscanf(usbName,"usbint%02d",&currentNum[0]);
    if(whichUsb==0)
	currentNum[0]++;
    else 
	currentNum[0]=whichUsb;
    if(currentNum[0]>NUM_USBS) {
	if(intExtFlag==1) return 0;
	else intExtFlag=2;
    }

    //    sscanf(usbExtName,"usbext%02d",&currentNum[1]);
    //    if(whichUsb==0)
    //	currentNum[1]++;
    //    else 
    //	currentNum[1]=whichUsb;
    //    if(currentNum[1]>NUM_USBEXTS && intExtFlag>1) return 0;

    //Kill all programs that write to disk
    killPrograms(MONITORD_ID_MASK);
    killPrograms(HKD_ID_MASK);
    killPrograms(GPSD_ID_MASK);
    killPrograms(ARCHIVED_ID_MASK);
    killPrograms(ACQD_ID_MASK);
    killPrograms(CALIBD_ID_MASK);
    killPrograms(PRIORITIZERD_ID_MASK);
    killPrograms(PLAYBACKD_ID_MASK);
    killPrograms(EVENTD_ID_MASK);
    closeHkFilesAndTidy(&cmdWriter);    
    sleep(5); //Let them gzip their data

//    exit(0);
    //Change to new drive
    if(intExtFlag&0x1) {
      sprintf(usbName,"usbint%02d",currentNum[0]);
      configModifyString("anitaSoft.config","global","usbName",usbName,&rawtime);
      retVal=system("/home/anita/flightSoft/bin/mountCurrentUsb.sh");
    }
    sleep(5);
    startNewRun();
    sleep(10);

    prepWriterStructs();
    startPrograms(HKD_ID_MASK);
    startPrograms(PLAYBACKD_ID_MASK);
    startPrograms(GPSD_ID_MASK);
    startPrograms(ARCHIVED_ID_MASK);
    startPrograms(ACQD_ID_MASK);
    startPrograms(CALIBD_ID_MASK);
    startPrograms(MONITORD_ID_MASK);
    startPrograms(PRIORITIZERD_ID_MASK);
    startPrograms(EVENTD_ID_MASK);
    return rawtime;

}

int requestFile(char *filename, int numLines) {
  static int counter=0;
  LogWatchRequest_t theRequest;
  char outName[FILENAME_MAX];
  time_t rawtime;
  time(&rawtime);
  theRequest.numLines=numLines;
  strncpy(theRequest.filename,filename,179);
  sprintf(outName,"%s/request_%d.dat",LOGWATCH_DIR,counter);
  counter++;
  writeLogWatchRequest(&theRequest,outName);
  makeLink(outName,LOGWATCH_LINK_DIR);
  return rawtime;
}




int writeCommandEcho(CommandStruct_t *theCmd, int unixTime) 
{
//    if(printToScreen)
//	printf("writeCommandEcho\n");
    CommandEcho_t theEcho;
    int count,retVal=0;
    char filename[FILENAME_MAX];
    char cmdString[100];
    theEcho.gHdr.code=PACKET_CMD_ECHO;
    sprintf(cmdString,"%d",theCmd->cmd[0]);
    theEcho.numCmdBytes=theCmd->numCmdBytes;
    theEcho.cmd[0]=theCmd->cmd[0];
//    if(printToScreen)
//	printf("copying command\n");

    if(theCmd->numCmdBytes>1) {
	for(count=1;count<theCmd->numCmdBytes;count++) {
	    sprintf(cmdString,"%s %d",cmdString,theCmd->cmd[count]);
	    theEcho.cmd[count]=theCmd->cmd[count];
	}
    }
//    if(printToScreen)
//	printf("Doing something\n");

    if(unixTime) {
	theEcho.unixTime=unixTime;
	theEcho.goodFlag=1;
    }
    else {
	time_t rawtime;
	time(&rawtime);
	theEcho.unixTime=rawtime;
	theEcho.goodFlag=0;
	syslog(LOG_ERR,"Error executing cmd: %s",cmdString);
	fprintf(stderr,"Error executing cmd: %s\n",cmdString);
    }


    //Write cmd echo for losd
    sprintf(filename,"%s/cmd_%d.dat",LOSD_CMD_ECHO_TELEM_DIR,theEcho.unixTime);
    {
	FILE *pFile;
	int fileTag=1;
	pFile = fopen(filename,"rb");
	while(pFile!=NULL) {
	    fclose(pFile);
	    sprintf(filename,"%s/cmd_%d_%d.dat",LOSD_CMD_ECHO_TELEM_DIR,theEcho.unixTime,fileTag);
	    pFile=fopen(filename,"rb");
	    fileTag++;
	}
    }
    printf("Writing to file %s\n",filename);
    fillGenericHeader(&theEcho,PACKET_CMD_ECHO,sizeof(CommandEcho_t));
    writeCmdEcho(&theEcho,filename);
    makeLink(filename,LOSD_CMD_ECHO_TELEM_LINK_DIR);


    //Write cmd echo for sipd
    sprintf(filename,"%s/cmd_%d.dat",SIPD_CMD_ECHO_TELEM_DIR,theEcho.unixTime);
    {
	FILE *pFile;
	int fileTag=1;
	pFile = fopen(filename,"rb");
	while(pFile!=NULL) {
	    fclose(pFile);
	    sprintf(filename,"%s/cmd_%d_%d.dat",SIPD_CMD_ECHO_TELEM_DIR,theEcho.unixTime,fileTag);
	    pFile=fopen(filename,"rb");
	    fileTag++;
	}
    }
    printf("Writing to file %s\n",filename);
    fillGenericHeader(&theEcho,PACKET_CMD_ECHO,sizeof(CommandEcho_t));
    writeCmdEcho(&theEcho,filename);
    makeLink(filename,SIPD_CMD_ECHO_TELEM_LINK_DIR);
    

    //Write Archive
    cleverHkWrite((unsigned char*)&theEcho,sizeof(CommandEcho_t),
		  theEcho.unixTime,&cmdWriter);

    return retVal;
}


void prepWriterStructs() {
    int diskInd;
    if(printToScreen) 
	printf("Preparing Writer Structs\n");
    //Hk Writer

    sprintf(cmdWriter.relBaseName,"%s/",CMD_ARCHIVE_DIR);
    sprintf(cmdWriter.filePrefix,"cmd");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	cmdWriter.currentFilePtr[diskInd]=0;
    cmdWriter.writeBitMask=hkDiskBitMask;

}


int readAcqdConfig()

/* Load Acqd config stuff */
{
    /* Config file thingies */
    int status=0,surf,dac;
    char keyName[180];
    int tempNum=12;
//    int surfSlots[MAX_SURFS];
//    int surfBuses[MAX_SURFS];
//    int surfSlotCount=0;
//    int surfBusCount=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
//    char *tempString;

    kvpReset();
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("Acqd.config","output") ;
    status += configLoad ("Acqd.config","locations") ;
    status += configLoad ("Acqd.config","debug") ;
    status += configLoad ("Acqd.config","trigger") ;
    status += configLoad ("Acqd.config","thresholds") ;
    status += configLoad ("Acqd.config","acqd") ;
    status += configLoad ("Acqd.config","pedestal") ;
//    printf("Debug rc1\n");
    if(status == CONFIG_E_OK) {
//	hkDiskBitMask=kvpGetInt("hkDiskBitMask",1);
//        niceValue=kvpGetInt("niceValue",-20);
//	pedestalMode=kvpGetInt("pedestalMode",0);
//	useInterrupts=kvpGetInt("useInterrupts",0);
//	firmwareVersion=kvpGetInt("firmwareVersion",2);
//	turfFirmwareVersion=kvpGetInt("turfFirmwareVersion",2);
//	surfHkPeriod=kvpGetInt("surfHkPeriod",1);
//	surfHkTelemEvery=kvpGetInt("surfHkTelemEvery",1);
//	turfRateTelemEvery=kvpGetInt("turfRateTelemEvery",1);
//	turfRateTelemInterval=kvpGetInt("turfRateTelemInterval",1);
//	sendSoftTrigger=kvpGetInt("sendSoftTrigger",0);
//	softTrigSleepPeriod=kvpGetInt("softTrigSleepPeriod",0);
//	reprogramTurf=kvpGetInt("reprogramTurf",0);
//	pps1TrigFlag=kvpGetInt("enablePPS1Trigger",1);
//	pps2TrigFlag=kvpGetInt("enablePPS2Trigger",1);
//	doThresholdScan=kvpGetInt("doThresholdScan",0);
//	doSingleChannelScan=kvpGetInt("doSingleChannelScan",0);
//	surfForSingle=kvpGetInt("surfForSingle",0);
//	dacForSingle=kvpGetInt("dacForSingle",0);
//	thresholdScanStepSize=kvpGetInt("thresholdScanStepSize",0);
//	thresholdScanPointsPerStep=kvpGetInt("thresholdScanPointsPerStep",0);
//	threshSwitchConfigAtEnd=kvpGetInt("threshSwitchConfigAtEnd",1);
//	setGlobalThreshold=kvpGetInt("setGlobalThreshold",0);
//	globalThreshold=kvpGetInt("globalThreshold",0);
//	dacPGain=kvpGetFloat("dacPGain",0);
//	dacIGain=kvpGetFloat("dacIGain",0);
//	dacDGain=kvpGetFloat("dacDGain",0);
//	dacIMin=kvpGetInt("dacIMin",0);
//	dacIMax=kvpGetInt("dacIMax",0);
//	enableChanServo=kvpGetInt("enableChanServo",1);
//	if(doThresholdScan)
//	    enableChanServo=0;
//	pidGoal=kvpGetInt("pidGoal",2000);
//	pidPanicVal=kvpGetInt("pidPanicVal",200);
//	pidAverage=kvpGetInt("pidAverage",1);
//	if(pidAverage<1) pidAverage=1;
//	tempNum=2;
//	antTrigMask=kvpGetUnsignedInt("antTrigMask",0);
//	turfioPos.bus=kvpGetInt("turfioBus",3); //in seconds
//	turfioPos.slot=kvpGetInt("turfioSlot",10); //in seconds

	tempNum=18;
	kvpStatus=kvpGetIntArray("surfTrigBandMasks",&surfTrigBandMasks[0][0],&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(surfTrigBandMasks): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(surfTrigBandMasks): %s\n",
			kvpErrorString(kvpStatus));
	}



	for(surf=0;surf<ACTIVE_SURFS;surf++) {
	    for(dac=0;dac<SCALERS_PER_SURF;dac++) {
		pidGoalScaleFactors[surf][dac]=0;
	    }
	    sprintf(keyName,"pidGoalScaleSurf%d",surf+1);
	    tempNum=32;	       
	    kvpStatus = kvpGetFloatArray(keyName,&(pidGoalScaleFactors[surf][0]),&tempNum);
	    if(kvpStatus!=KVP_E_OK) {
		syslog(LOG_WARNING,"kvpGetFloatArray(%s): %s",keyName,
		       kvpErrorString(kvpStatus));
		if(printToScreen)
		    fprintf(stderr,"kvpGetFloatArray(%s): %s\n",keyName,
			    kvpErrorString(kvpStatus));
	    }
	    
	}
	
    	


/* 	tempNum=12; */
/* 	kvpStatus = kvpGetIntArray("surfBus",surfBuses,&tempNum); */
/* 	if(kvpStatus!=KVP_E_OK) { */
/* 	    syslog(LOG_WARNING,"kvpGetIntArray(surfBus): %s", */
/* 		   kvpErrorString(kvpStatus)); */
/* 	    if(printToScreen) */
/* 		fprintf(stderr,"kvpGetIntArray(surfBus): %s\n", */
/* 			kvpErrorString(kvpStatus)); */
/* 	} */
/* 	else { */
/* 	    for(count=0;count<tempNum;count++) { */
/* 		surfPos[count].bus=surfBuses[count]; */
/* 		if(surfBuses[count]!=-1) { */
/* 		    surfBusCount++; */
/* 		} */
/* 	    } */
/* 	} */
/* 	tempNum=12; */
/* 	kvpStatus = kvpGetIntArray("surfSlot",surfSlots,&tempNum); */
/* 	if(kvpStatus!=KVP_E_OK) { */
/* 	    syslog(LOG_WARNING,"kvpGetIntArray(surfSlot): %s", */
/* 		   kvpErrorString(kvpStatus)); */
/* 	    if(printToScreen) */
/* 		fprintf(stderr,"kvpGetIntArray(surfSlot): %s\n", */
/* 			kvpErrorString(kvpStatus)); */
/* 	} */
/* 	else { */
/* 	    for(count=0;count<tempNum;count++) { */
/* 		surfPos[count].slot=surfSlots[count]; */
/* 		if(surfSlots[count]!=-1) { */
/* 		    surfSlotCount++; */
/* 		} */
/* 	    } */
/* 	} */
/* 	if(surfSlotCount==surfBusCount) numSurfs=surfBusCount; */
/* 	else { */
/* 	    syslog(LOG_ERR,"Confused config file bus and slot count do not agree"); */
/* 	    if(printToScreen) */
/* 		fprintf(stderr,"Confused config file bus and slot count do not agree\n"); */
/* 	    exit(0); */
/* 	} */
	
/* 	tempNum=12; */
/* 	kvpStatus = kvpGetIntArray("dacSurfs",dacSurfs,&tempNum); */
/* 	if(kvpStatus!=KVP_E_OK) { */
/* 	    syslog(LOG_WARNING,"kvpGetIntArray(dacSurfs): %s", */
/* 		   kvpErrorString(kvpStatus)); */
/* 	    if(printToScreen) */
/* 		fprintf(stderr,"kvpGetIntArray(dacSurfs): %s\n", */
/* 			kvpErrorString(kvpStatus)); */
/* 	} */
	
/* 	for(surf=0;surf<ACTIVE_SURFS;surf++) { */
/* 	    for(dac=0;dac<N_RFTRIG;dac++) { */
/* 		thresholdArray[surf][dac]=0; */
/* 	    } */
/* 	    if(dacSurfs[surf]) {		 */
/* 		sprintf(keyName,"threshSurf%d",surf+1); */
/* 		tempNum=32;	        */
/* 		kvpStatus = kvpGetIntArray(keyName,&(thresholdArray[surf][0]),&tempNum); */
/* 		if(kvpStatus!=KVP_E_OK) { */
/* 		    syslog(LOG_WARNING,"kvpGetIntArray(%s): %s",keyName, */
/* 			   kvpErrorString(kvpStatus)); */
/* 		    if(printToScreen) */
/* 			fprintf(stderr,"kvpGetIntArray(%s): %s\n",keyName, */
/* 				kvpErrorString(kvpStatus)); */
/* 		} */
/* 	    } */
/* 	} */
    	
/* 	//Pedestal things */
/* 	writeDebugData=kvpGetInt("writeDebugData",1); */
/* 	numPedEvents=kvpGetInt("numPedEvents",4000); */
/* 	pedSwitchConfigAtEnd=kvpGetInt("pedSwitchConfigAtEnd",1); */
	
	
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Acqd.config: %s\n",eString);
//	if(printToScreen)
	fprintf(stderr,"Error reading Acqd.config: %s\n",eString);
	    
    }

    return status;
}

void copyCommand(CommandStruct_t *theCmd, CommandStruct_t *cpCmd)
{
    int i;
    cpCmd->numCmdBytes=theCmd->numCmdBytes;
    for(i=0;i<theCmd->numCmdBytes;i++) {
	cpCmd->cmd[i]=theCmd->cmd[i];
    }
}

int sameCommand(CommandStruct_t *theCmd, CommandStruct_t *lastCmd) 
{
    int i;
    int different=0;
    different+=(theCmd->numCmdBytes!=lastCmd->numCmdBytes);
    for(i=0;i<theCmd->numCmdBytes;i++) {
	different+=(theCmd->cmd[i]!=lastCmd->cmd[i]);
    }
    if(different==0) return 1;
    return 0;
}



int readArchivedConfig() 
/* Load Archived config stuff */
{
    /* Config file thingies */
    int status=0,tempNum=0;
    char* eString ;
    KvpErrorCode kvpStatus;
    kvpReset();
    status = configLoad ("Archived.config","archived") ;
    if(status == CONFIG_E_OK) {
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
	kvpStatus = kvpGetFloatArray("priorityFractionDelete",
				     priorityFractionDelete,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priorityFractionDelete): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priorityFractionDelete): %s\n",
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


int readLosdConfig() 
/* Load Losd config stuff */
{
    /* Config file thingies */
    int status=0,tempNum=0;
    char* eString ;
    KvpErrorCode kvpStatus;
    kvpReset();
    status = configLoad ("LOSd.config","bandwidth") ;
    if(status == CONFIG_E_OK) {
	tempNum=10;
	kvpStatus = kvpGetIntArray("priorityBandwidths",
				   losdBandwidths,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priorityBandwidths): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priorityBandwidths): %s\n",
			kvpErrorString(kvpStatus));
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading LOSd.config: %s\n",eString);
    }
    
    return status;
}

int readSipdConfig() 
/* Load Sipd config stuff */
{
    /* Config file thingies */
    int status=0,tempNum=0;
    char* eString ;
    KvpErrorCode kvpStatus;
    kvpReset();
    status = configLoad ("SIPd.config","bandwidth") ;
    if(status == CONFIG_E_OK) {
	tempNum=10;
	kvpStatus = kvpGetIntArray("priorityBandwidths",
				   sipdBandwidths,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priorityBandwidths): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priorityBandwidths): %s\n",
			kvpErrorString(kvpStatus));
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading SIPd.config: %s\n",eString);
    }
    
    return status;
}


int startNewRun() {
    char filename[FILENAME_MAX];
    RunStart_t runStart;
    int diskInd=0;
    int retVal=0;
    int runNum=getNewRunNumber();
    char newRunDir[FILENAME_MAX];
    char currentDirLink[FILENAME_MAX];
    char mistakeDirName[FILENAME_MAX];
    runStart.runNumber=runNum;
    runStart.unixTime=time(NULL);
    runStart.eventNumber=getLatestEventNumber();
    runStart.eventNumber/=100;
    runStart.eventNumber++;
    runStart.eventNumber*=100; //Roughly

    //First make dirs
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	if(!(hkDiskBitMask&diskBitMasks[diskInd]) &&
	   !(eventDiskBitMask&diskBitMasks[diskInd]))
	    continue; // Disk not enabled
	
	sprintf(newRunDir,"%s/run%d",diskNames[diskInd],runNum);
	sprintf(currentDirLink,"%s/current",diskNames[diskInd]);
	
	makeDirectories(newRunDir);
	removeFile(currentDirLink);
	if(is_dir(currentDirLink)) {
	    sprintf(mistakeDirName,"%s/run%d.current",diskNames[diskInd],runNum-1);
	    rename(currentDirLink,mistakeDirName);	    
	}
	symlink(newRunDir,currentDirLink);       
//    char theCommand[180];
//    sprintf(theCommand,"/home/anita/flightSoft/bin/startNewRun.sh");
//    retVal=system(theCommand);
    }

    //Now delete dirs
    //For now with system command
    system("rm -rf /tmp/anita/acqd /tmp/anita/eventd /tmp/anita/gpsd /tmp/anita/prioritizerd /tmp/anita/calibd");
    
    fillGenericHeader(&runStart,PACKET_RUN_START,sizeof(RunStart_t));
    //Write file for Monitord
    normalSingleWrite((unsigned char*)&runStart,RUN_START_FILE,sizeof(RunStart_t));
    
    sprintf(filename,"%s/run_%u.dat",LOSD_CMD_ECHO_TELEM_DIR,runStart.runNumber);
    normalSingleWrite((unsigned char*)&runStart,filename,sizeof(RunStart_t));
    makeLink(filename,LOSD_CMD_ECHO_TELEM_LINK_DIR);
    
    sprintf(filename,"%s/run_%u.dat",SIPD_CMD_ECHO_TELEM_DIR,runStart.runNumber);
    normalSingleWrite((unsigned char*)&runStart,filename,sizeof(RunStart_t));
    makeLink(filename,SIPD_CMD_ECHO_TELEM_LINK_DIR);

    return retVal;
}


int getNewRunNumber() {
    static int firstTime=1;
    static int runNumber=0;
    int retVal=0;
    /* This is just to get the lastRunNumber in case of program restart. */
    if(firstTime) {
      runNumber=getRunNumber();
    }
    runNumber++;

    FILE *pFile = fopen (LAST_RUN_NUMBER_FILE, "w");
    if(pFile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		LAST_RUN_NUMBER_FILE);
	fprintf(stderr,"fopen: %s ---  %s\n",strerror(errno),
		LAST_RUN_NUMBER_FILE);
    }
    else {
	retVal=fprintf(pFile,"%d\n",runNumber);
	if(retVal<0) {
	    syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		    LAST_RUN_NUMBER_FILE);
	    fprintf(stderr,"fprintf: %s ---  %s\n",strerror(errno),
		    LAST_RUN_NUMBER_FILE);
	}
	fclose (pFile);
    }
    return runNumber;
    
}

int clearRamdisk()
{
    int progMask=0;
    progMask|=ACQD_ID_MASK;
    progMask|=ARCHIVED_ID_MASK;
    progMask|=CALIBD_ID_MASK;
//    progMask|=CMDD_ID_MASK;
    progMask|=EVENTD_ID_MASK;
    progMask|=GPSD_ID_MASK;
    progMask|=HKD_ID_MASK;
    progMask|=LOSD_ID_MASK;
    progMask|=PRIORITIZERD_ID_MASK;
    progMask|=PLAYBACKD_ID_MASK;
    progMask|=SIPD_ID_MASK;
    progMask|=MONITORD_ID_MASK;
    killPrograms(progMask);
    sleep(10);
    int retVal=system("rm -rf /tmp/anita");
    makeDirectories(LOSD_CMD_ECHO_TELEM_LINK_DIR);
    makeDirectories(SIPD_CMD_ECHO_TELEM_LINK_DIR);
    makeDirectories(CMDD_COMMAND_LINK_DIR);
    makeDirectories(REQUEST_TELEM_LINK_DIR);
    startNewRun();
    startPrograms(progMask);  
    return retVal;
}


int getLatestEventNumber() {
    int retVal=0;
    int eventNumber=0;

    FILE *pFile;
    pFile = fopen (LAST_EVENT_NUMBER_FILE, "r");
    if(pFile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		LAST_EVENT_NUMBER_FILE);
    }
    else {	    	    
	retVal=fscanf(pFile,"%d",&eventNumber);
	if(retVal<0) {
	    syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
		    LAST_EVENT_NUMBER_FILE);
	}
	fclose (pFile);
    }
    return eventNumber;
}


void handleBadSigs(int sig)
{
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
    unlink(CMDD_PID_FILE);
    syslog(LOG_INFO,"Cmdd terminating");
    exit(0);
}
