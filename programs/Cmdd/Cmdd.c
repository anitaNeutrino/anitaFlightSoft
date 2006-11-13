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


#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
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
int sendConfig(int progMask);
int defaultConfig(int progMask);
int lastConfig(int progMask);
int switchConfig(int progMask,unsigned char configId);
int killPrograms(int progMask);
int startPrograms(int progMask);
int respawnPrograms(int progMask);
char *getProgName(ProgramId_t prog);
int getIdMask(ProgramId_t prog);
char *getPidFile(ProgramId_t prog);
int mountNextBlade();
int mountNextUsb(int intExtFlag);
void prepWriterStructs();
int tailFile(char *filename, int numLines);
int makeZippedFilePacket(char *filename,int fileTag);
int readAcqdConfig();
int readArchivedConfig();
int setEventDiskBitMask(int modOrSet, int bitMask);
int setHkDiskBitMask(int modOrSet, int bitMask);
int setPriDiskMask(int pri, int bitmask);
int setPriEncodingType(int pri, int encType);
int setAlternateUsb(int pri, int altUsb);
int setArchiveDefaultFrac(int pri, int frac);
int setTelemPriEncodingType(int pri, int encType, int encClockType);
int setPidGoalScaleFactor(int surf, int dac, float scaleFactor);
int startNewRun();
int getRunNumber();

#define MAX_COMMANNDS 100  //Hard to believe we can get to a hundred

/* Global variables */
int cmdLengths[256];

CommandStruct_t theCmds[MAX_COMMANNDS];

int numCmds=256;

/* PID Files */
char acqdPidFile[FILENAME_MAX];
char archivedPidFile[FILENAME_MAX];
char calibdPidFile[FILENAME_MAX];
char cmddPidFile[FILENAME_MAX];
char eventdPidFile[FILENAME_MAX];
char gpsdPidFile[FILENAME_MAX];
char hkdPidFile[FILENAME_MAX];
char losdPidFile[FILENAME_MAX];
char prioritizerdPidFile[FILENAME_MAX];
char sipdPidFile[FILENAME_MAX];
char monitordPidFile[FILENAME_MAX];
char lastRunNumberFile[FILENAME_MAX];

//Debugging Output
int printToScreen=1;
int verbosity=1;

int disableBlade;
int disableUsbInt;
int disableUsbExt;
char bladeName[FILENAME_MAX];
char usbIntName[FILENAME_MAX];
char usbExtName[FILENAME_MAX];

//Needed for commands
int surfTrigBandMasks[ACTIVE_SURFS][2];
int priDiskEncodingType[NUM_PRIORITIES];
int priTelemEncodingType[NUM_PRIORITIES];
int priTelemClockEncodingType[NUM_PRIORITIES];
float priorityFractionDefault[NUM_PRIORITIES];
int priDiskEventMask[NUM_PRIORITIES];
int alternateUsbs[NUM_PRIORITIES];
float pidGoalScaleFactors[ACTIVE_SURFS][SCALERS_PER_SURF];



int hkDiskBitMask;
int eventDiskBitMask;
AnitaHkWriterStruct_t cmdWriter;

int diskBitMasks[DISK_TYPES]={BLADE_DISK_MASK,PUCK_DISK_MASK,USBINT_DISK_MASK,USBEXT_DISK_MASK,PMC_DISK_MASK};

char *diskNames[DISK_TYPES]={BLADE_DATA_MOUNT,PUCK_DATA_MOUNT,USBINT_DATA_MOUNT,USBEXT_DATA_MOUNT,SAFE_DATA_MOUNT};

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

     /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);

    makeDirectories(LOSD_CMD_ECHO_TELEM_LINK_DIR);
    makeDirectories(SIPD_CMD_ECHO_TELEM_LINK_DIR);
    makeDirectories(CMDD_COMMAND_LINK_DIR);
    makeDirectories(REQUEST_TELEM_LINK_DIR);

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
		    if(checkCommand(&theCmds[count])) {
			if(printToScreen) 
			    printf("cmd %d good , (%#x)\n",count,theCmds[count].cmd[0]);		
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
	    usleep(1);
	}
	
    } while(currentState==PROG_STATE_INIT);
    closeHkFilesAndTidy(&cmdWriter);
    unlink(cmddPidFile);
    syslog(LOG_INFO,"Cmdd terminating");
    return 0;
}


int checkForNewCommand() {
    struct dirent **cmdLinkList;
    int numCmdLinks=getListofLinks(CMDD_COMMAND_LINK_DIR,&cmdLinkList);
    int uptoCmd;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    if(numCmdLinks) {
	for(uptoCmd=0;uptoCmd<numCmdLinks;uptoCmd++) {
	    if(uptoCmd==MAX_COMMANNDS) break;
	    sprintf(currentFilename,"%s/%s",CMDD_COMMAND_DIR,
		    cmdLinkList[uptoCmd]->d_name);
	    sprintf(currentLinkname,"%s/%s",CMDD_COMMAND_LINK_DIR,
		    cmdLinkList[uptoCmd]->d_name);
	    fillCommand(&theCmds[uptoCmd],currentFilename);
	    removeFile(currentFilename);
	    removeFile(currentLinkname);
	}
    }    
    if(numCmdLinks>0) {
	for(uptoCmd=0;uptoCmd<numCmdLinks;uptoCmd++) {
	    free(cmdLinkList[uptoCmd]);
	}
	free(cmdLinkList);
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

int readConfig() {
    /* Config file thingies */
    int status=0;
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
	
	
	hkDiskBitMask=kvpGetInt("hkDiskBitMask",1);
	eventDiskBitMask=kvpGetInt("eventDiskBitMask",1);
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_ERR,"Problem getting cmdLengths -- %s\n",
		    kvpErrorString(kvpStatus));
	    fprintf(stderr,"Problem getting cmdLengths -- %s\n",
		    kvpErrorString(kvpStatus));
	}
	
	tempString=kvpGetString("lastRunNumberFile");
	if(tempString) {
	    strncpy(lastRunNumberFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get lastRunNumberFile");
	    fprintf(stderr,"Couldn't get lastRunNumberFile\n");
	}


	tempString=kvpGetString("acqdPidFile");
	if(tempString) {
	    strncpy(acqdPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get acqdPidFile");
	    fprintf(stderr,"Couldn't get acqdPidFile\n");
	}
	tempString=kvpGetString("calibdPidFile");
	if(tempString) {
	    strncpy(calibdPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get calibdPidFile");
	    fprintf(stderr,"Couldn't get calibdPidFile\n");
	}
	tempString=kvpGetString("archivedPidFile");
	if(tempString) {
	    strncpy(archivedPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedPidFile");
	    fprintf(stderr,"Couldn't get archivedPidFile\n");
	}
	tempString=kvpGetString("eventdPidFile");
	if(tempString) {
	    strncpy(eventdPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get eventdPidFile");
	    fprintf(stderr,"Couldn't get eventdPidFile\n");
	}
	tempString=kvpGetString("gpsdPidFile");
	if(tempString) {
	    strncpy(gpsdPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdPidFile");
	    fprintf(stderr,"Couldn't get gpsdPidFile\n");
	}
	tempString=kvpGetString("hkdPidFile");
	if(tempString) {
	    strncpy(hkdPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get hkdPidFile");
	    fprintf(stderr,"Couldn't get hkdPidFile\n");
	}
	tempString=kvpGetString("losdPidFile");
	if(tempString) {
	    strncpy(losdPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get losdPidFile");
	    fprintf(stderr,"Couldn't get losdPidFile\n");
	}
	tempString=kvpGetString("prioritizerdPidFile");
	if(tempString) {
	    strncpy(prioritizerdPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get prioritizerdPidFile");
	    fprintf(stderr,"Couldn't get prioritizerdPidFile\n");
	}
	tempString=kvpGetString("sipdPidFile");
	if(tempString) {
	    strncpy(sipdPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get sipdPidFile");
	    fprintf(stderr,"Couldn't get sipdPidFile\n");
	}
	tempString=kvpGetString("monitordPidFile");
	if(tempString) {
	    strncpy(monitordPidFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get monitordPidFile");
	    fprintf(stderr,"Couldn't get monitordPidFile\n");
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
    unsigned long ulvalue,ultemp;
    char theCommand[FILENAME_MAX];

    int index;
    int numDataProgs=6;
    int dataProgMasks[6]={HKD_ID_MASK,GPSD_ID_MASK,ARCHIVED_ID_MASK,
			  ACQD_ID_MASK,CALIBD_ID_MASK,MONITORD_ID_MASK};
    char *dataPidFiles[6]={hkdPidFile,gpsdPidFile,archivedPidFile,acqdPidFile,
			   calibdPidFile,monitordPidFile};
    

    switch(theCmd->cmd[0]) {
	case CMD_START_NEW_RUN:
	{
	    
	    int sleepCount=0;
	    time(&rawtime);
	    for(index=0;index<numDataProgs;index++) {
		killPrograms(dataProgMasks[index]);
	    }
	    
	    for(index=0;index<numDataProgs;index++) {
		int fileExists=0;
		do {
		    FILE *test =fopen(dataPidFiles[index],"r");
		    if(test==NULL) fileExists=0;
		    else {
			fclose(test);
			fileExists=1;
			sleepCount++;
		    }
		}
		while(fileExists && sleepCount<20);
	    }	   
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
	    return tailFile("/var/log/messages",ivalue);
	case CMD_TAIL_VAR_LOG_ANITA:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return tailFile("/var/log/anita.log",ivalue);
	case CMD_SHUTDOWN_HALT:
	    //Halt
	    time(&rawtime);
	    sprintf(theCommand,"sudo halt");
	    retVal=system(theCommand);
	    if(retVal==-1) return 0;	    
	    time(&rawtime);
	    return rawtime;
	case CMD_REBOOT:
	    //Reboot
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
	    killPrograms(HKD_ID_MASK);
	    killPrograms(GPSD_ID_MASK);
	    killPrograms(ARCHIVED_ID_MASK);
	    killPrograms(ACQD_ID_MASK);
	    killPrograms(CALIBD_ID_MASK);
	    killPrograms(MONITORD_ID_MASK);
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
	    if(retVal==-1) return 0;	    
	    time(&rawtime);
	    return rawtime;
	case CMD_MOUNT_NEXT_BLADE:
	    return mountNextBlade();
	case CMD_MOUNT_NEXT_USB:
	    ivalue=theCmd->cmd[1];
	    return mountNextUsb(ivalue);
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
	    configModifyInt("GPSd.config","adu5","vtgPeriod",ivalue,&rawtime);
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
	    fvalue=ivalue +((float)ivalue2)/10000.;
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
	    configModifyInt("Acqd.config","trigger","softTrigSleepPeriod",ivalue,&rawtime);
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
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_THRESHOLD_SCAN: 	    
	    ivalue=theCmd->cmd[1];
	    ivalue&=0x1;
	    configModifyInt("Acqd.config","thresholdScan","doThresholdScan",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case ACQD_SET_ANT_TRIG_MASK: 	    
	    ultemp=(theCmd->cmd[1]);	    
	    ulvalue=ultemp;
	    ultemp=(theCmd->cmd[2]);
	    ulvalue|=(ulvalue<<8);
	    ultemp=(theCmd->cmd[3]);
	    ulvalue|=(ulvalue<<16);
	    ultemp=(theCmd->cmd[4]);
	    ulvalue|=(ulvalue<<24);
//	    printf("%d %d %d %d -- %lu\n",theCmd->cmd[1],theCmd->cmd[2],
//		   theCmd->cmd[3],theCmd->cmd[4],ulvalue);

	    configModifyUnsignedInt("Acqd.config","thresholds","antTrigMask",ulvalue,&rawtime);
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
	    configModifyInt("Acqd.config","thresholds","globalThreshold",ivalue,&rawtime);
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
	case CLEAN_DIRS: 	    
	    return cleanDirs();
	case SEND_CONFIG: 
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return sendConfig(ivalue);	    
	case DEFAULT_CONFIG:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return defaultConfig(ivalue);
	case SWITCH_CONFIG:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    return switchConfig(ivalue,theCmd->cmd[3]);
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
	    sprintf(fileName,"%s",acqdPidFile);
	    break;
	case ID_ARCHIVED:
	    sprintf(fileName,"%s",archivedPidFile);
	    break;
	case ID_EVENTD:
	    sprintf(fileName,"%s",eventdPidFile);
	    break;
	case ID_CALIBD:
	    sprintf(fileName,"%s",calibdPidFile);
	    break;
	case ID_GPSD:
	    sprintf(fileName,"%s",gpsdPidFile);
	    break;
	case ID_HKD:
	    sprintf(fileName,"%s",hkdPidFile);
	    break;
	case ID_LOSD:
	    sprintf(fileName,"%s",losdPidFile);
	    break;
	case ID_PRIORITIZERD:
	    sprintf(fileName,"%s",prioritizerdPidFile);
	    break;
	case ID_MONITORD:
	    sprintf(fileName,"%s",monitordPidFile);
	    break;
	case ID_SIPD:
	    sprintf(fileName,"%s",sipdPidFile);
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
	    rawtime=makeZippedFilePacket(fullFilename,prog);
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
	case ID_ACQD: return acqdPidFile;
	case ID_ARCHIVED: return archivedPidFile;
	case ID_CALIBD: return calibdPidFile;
	case ID_CMDD: return cmddPidFile;
	case ID_EVENTD: return eventdPidFile;
	case ID_GPSD: return gpsdPidFile;
	case ID_HKD: return hkdPidFile;
	case ID_LOSD: return losdPidFile;
	case ID_PRIORITIZERD: return prioritizerdPidFile;
	case ID_SIPD: return sipdPidFile;
	case ID_MONITORD: return monitordPidFile;
	default: break;
    }
    return NULL;
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

//    printf("Kill programs %d\n",progMask);
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
//	printf("%d %d\n",progMask,testMask);
	if(progMask&testMask) {
//	    printf("Killing prog %s\n",getProgName(prog));
	    sendSignal(prog,SIGUSR2);
	    sprintf(daemonCommand,"daemon --stop -n %s",getProgName(prog));
	    retVal=system(daemonCommand);
	    if(retVal!=0) {
		errorCount++;
		syslog(LOG_ERR,"Error killing %s",getProgName(prog));
		//Maybe do something clever
	    }
	    else {
		syslog(LOG_INFO,"Killed %s\n",getProgName(prog));
	    }
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
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
	if(progMask&testMask) {
	    //Match
	    sprintf(daemonCommand,"daemon -r %s -n %s ",
		    getProgName(prog),getProgName(prog));
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


int mountNextBlade() {
    int retVal;
    time_t rawtime;
    int currentNum=0;
    readConfig(); // Get latest names and status
    if(disableBlade) return -1;
    sscanf(bladeName,"zeus%02d",&currentNum);
    currentNum++;
    if(currentNum>NUM_BLADES) return -1;

    //Kill all programs that write to disk
    killPrograms(HKD_ID_MASK);
    killPrograms(GPSD_ID_MASK);
    killPrograms(ARCHIVED_ID_MASK);
    killPrograms(ACQD_ID_MASK);
    killPrograms(CALIBD_ID_MASK);
    killPrograms(MONITORD_ID_MASK);
    closeHkFilesAndTidy(&cmdWriter);    
    sleep(5); //Let them gzip their data


//    exit(0);
    
    //Change to new drive
    sprintf(bladeName,"zeus%02d",currentNum);
    configModifyString("anitaSoft.config","global","bladeName",bladeName,&rawtime);
    retVal=system("/home/anita/flightSoft/bin/mountCurrentBlade.sh");
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
    return rawtime;

}

int mountNextUsb(int intExtFlag) {
    int retVal;
    time_t rawtime;
    int currentNum[2]={0};

    readConfig(); // Get latest names and status

    disableUsbInt=1;
    disableUsbExt=1;
    
    if(hkDiskBitMask&USBINT_DISK_MASK) disableUsbInt=0;
    if(eventDiskBitMask&USBINT_DISK_MASK) disableUsbInt=0;

    if(hkDiskBitMask&USBEXT_DISK_MASK) disableUsbExt=0;
    if(eventDiskBitMask&USBEXT_DISK_MASK) disableUsbExt=0;


    if(intExtFlag<1 || intExtFlag>3) return 0;

    if(intExtFlag==1 && disableUsbInt) return 0;
    else if(intExtFlag==2 && disableUsbExt) return 0;
    else if(intExtFlag==3 && disableUsbExt && disableUsbInt) return 0;
    else if(intExtFlag==3 && disableUsbInt) intExtFlag=2;
    else if(intExtFlag==3 && disableUsbExt) intExtFlag=1;

    sscanf(usbIntName,"usbint%02d",&currentNum[0]);
    currentNum[0]++;
    if(currentNum[0]>NUM_USBINTS) {
	if(intExtFlag==1) return 0;
	else intExtFlag=2;
    }

    sscanf(usbExtName,"usbext%02d",&currentNum[1]);
    currentNum[1]++;
    if(currentNum[1]>NUM_USBEXTS && intExtFlag>1) return 0;

    //Kill all programs that write to disk
    killPrograms(HKD_ID_MASK);
    killPrograms(GPSD_ID_MASK);
    killPrograms(ARCHIVED_ID_MASK);
    killPrograms(ACQD_ID_MASK);
    killPrograms(CALIBD_ID_MASK);
    killPrograms(MONITORD_ID_MASK);
    closeHkFilesAndTidy(&cmdWriter);    
    sleep(5); //Let them gzip their data

//    exit(0);
    //Change to new drive
    if(intExtFlag&0x1) {
	sprintf(usbIntName,"usbint%02d",currentNum[0]);
	configModifyString("anitaSoft.config","global","usbIntName",usbIntName,&rawtime);
	retVal=system("/home/anita/flightSoft/bin/mountCurrentUsbInt.sh");
    }
    sleep(2);
    if(intExtFlag&0x2) {
	sprintf(usbExtName,"usbext%02d",currentNum[1]);
	configModifyString("anitaSoft.config","global","usbExtName",usbExtName,&rawtime);
	retVal=system("/home/anita/flightSoft/bin/mountCurrentUsbExt.sh");
    }
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
    return rawtime;

}

int tailFile(char *filename, int numLines) {
    static int counter=0;
    time_t rawtime;
    time(&rawtime);
    char tailFilename[FILENAME_MAX];
    sprintf(tailFilename,"/tmp/tail_%s",basename(filename));
    char theCommand[FILENAME_MAX];
    sprintf(theCommand,"tail -n %d %s > %s",numLines,filename,tailFilename);
    system(theCommand);
    rawtime=makeZippedFilePacket(tailFilename,counter);
    counter++;    
    return rawtime;
}

int makeZippedFilePacket(char *filename,int fileTag) 
{
    int retVal;
    time_t rawtime;
    time(&rawtime);
    ZippedFile_t *zipFilePtr;
    char outputName[FILENAME_MAX];
    char *inputBuffer;
    char *outputBuffer;
    unsigned long numBytesIn=0,numBytesOut=0;
    
    inputBuffer=readFile(filename,&numBytesIn);
    if(!inputBuffer || !numBytesIn) return 0;
    numBytesOut=numBytesIn+1000 +sizeof(ZippedFile_t);
    outputBuffer=malloc(numBytesOut);
    zipFilePtr = (ZippedFile_t*) outputBuffer;
    zipFilePtr->unixTime=rawtime;
    zipFilePtr->numUncompressedBytes=numBytesIn;
    strncpy(zipFilePtr->filename,basename(filename),60);
    retVal=zipBuffer(inputBuffer,&outputBuffer[sizeof(ZippedFile_t)],numBytesIn,&numBytesOut);
    if(retVal==0) {		
	fillGenericHeader(zipFilePtr,PACKET_ZIPPED_FILE,numBytesOut+sizeof(ZippedFile_t));
	sprintf(outputName,"%s/zipFile_%d_%lu.dat",REQUEST_TELEM_DIR,
		fileTag,zipFilePtr->unixTime);
	normalSingleWrite((unsigned char*)outputBuffer,
			  outputName,numBytesOut+sizeof(ZippedFile_t));
	makeLink(outputName,REQUEST_TELEM_LINK_DIR);		
    }
    free(inputBuffer);
    free(outputBuffer);
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
    sprintf(filename,"%s/cmd_%ld.dat",LOSD_CMD_ECHO_TELEM_DIR,theEcho.unixTime);
    {
	FILE *pFile;
	int fileTag=1;
	pFile = fopen(filename,"rb");
	while(pFile!=NULL) {
	    fclose(pFile);
	    sprintf(filename,"%s/cmd_%ld_%d.dat",LOSD_CMD_ECHO_TELEM_DIR,theEcho.unixTime,fileTag);
	    pFile=fopen(filename,"rb");
	    fileTag++;
	}
    }
    printf("Writing to file %s\n",filename);
    fillGenericHeader(&theEcho,PACKET_CMD_ECHO,sizeof(CommandEcho_t));
    writeCmdEcho(&theEcho,filename);
    makeLink(filename,LOSD_CMD_ECHO_TELEM_LINK_DIR);


    //Write cmd echo for sipd
    sprintf(filename,"%s/cmd_%ld.dat",SIPD_CMD_ECHO_TELEM_DIR,theEcho.unixTime);
    {
	FILE *pFile;
	int fileTag=1;
	pFile = fopen(filename,"rb");
	while(pFile!=NULL) {
	    fclose(pFile);
	    sprintf(filename,"%s/cmd_%ld_%d.dat",SIPD_CMD_ECHO_TELEM_DIR,theEcho.unixTime,fileTag);
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
	kvpStatus = kvpGetFloatArray("priorityFractionDefault",
				     priorityFractionDefault,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priorityFractionDefault): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priorityFractionDefault): %s\n",
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

int startNewRun() {
    int diskInd=0;
    int retVal=0;
    int runNum=getRunNumber();
    char newRunDir[FILENAME_MAX];
    char currentDirLink[FILENAME_MAX];
    char mistakeDirName[FILENAME_MAX];

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
    return retVal;
}


int getRunNumber() {
	int retVal=0;
    static int firstTime=1;
    static int runNumber=0;
    /* This is just to get the lastRunNumber in case of program restart. */
    FILE *pFile;
    if(firstTime) {
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
	if(printToScreen) printf("The last run number is %d\n",runNumber);
	firstTime=0;
    }
    runNumber++;

    pFile = fopen (lastRunNumberFile, "w");
    if(pFile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		lastRunNumberFile);
    }
    else {
	retVal=fprintf(pFile,"%d\n",runNumber);
	if(retVal<0) {
	    syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		    lastRunNumberFile);
	    }
	fclose (pFile);
    }

    return runNumber;
    
}
