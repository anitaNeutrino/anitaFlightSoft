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


#define MAX_COMMANNDS 100  //Hard to believe we can get to a hundred

/* Global variables */
int cmdLengths[256];

CommandStruct_t theCmds[MAX_COMMANNDS][MAX_CMD_LENGTH];

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


//Debugging Output
int printToScreen=1;
int verbosity=1;

int disableBlade;
int disableUsbInt;
int disableUsbExt;
char bladeName[FILENAME_MAX];
char usbIntName[FILENAME_MAX];
char usbExtName[FILENAME_MAX];



int hkDiskBitMask;
AnitaHkWriterStruct_t cmdWriter;

int main (int argc, char *argv[])
{
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
		    if(checkCommand(theCmds[count])) {
			retVal=executeCommand(theCmds[count]);
			writeCommandEcho(theCmds[count],retVal);
		    }
		}
		
	    }
	    usleep(100000);
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
	    fillCommand(theCmds[uptoCmd],currentFilename);
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

	disableBlade=kvpGetInt("disableBlade",0);
	disableUsbInt=kvpGetInt("disableUsbInt",0);
	disableUsbExt=kvpGetInt("disableUsbExt",0);

	hkDiskBitMask=kvpGetInt("hkDiskBitMask",1);
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_ERR,"Problem getting cmdLengths -- %s\n",
		    kvpErrorString(kvpStatus));
	    fprintf(stderr,"Problem getting cmdLengths -- %s\n",
		    kvpErrorString(kvpStatus));
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
    char theCommand[FILENAME_MAX];
    switch(theCmd->cmd[0]) {
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
	case CMD_TURN_GPS_ON:
	    //turnGPSOn
	    configModifyInt("Calibd.config","relays","stateGPS",1,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case CMD_TURN_GPS_OFF:
	    //turnGPSOff
	    configModifyInt("Calibd.config","relays","stateGPS",0,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case CMD_TURN_RFCM_ON:
	    //turnRFCMOn
	    configModifyInt("Calibd.config","relays","stateRFCM1",1,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM2",1,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM3",1,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM4",1,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case CMD_TURN_RFCM_OFF:
	    //turnRFCMOff
	    configModifyInt("Calibd.config","relays","stateRFCM1",0,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM2",0,&rawtime);
	    configModifyInt("Calibd.config","relays","stateRFCM3",0,&rawtime);
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
	    
	case SET_CALPULSER_SWITCH:
	    //setCalPulserSwitch
	    if(theCmd->cmd[1]>0 && theCmd->cmd[1]<5)
		configModifyInt("Calibd.config","rfswitch","steadyState",theCmd->cmd[1],&rawtime);
	    else //Switch off steady state
		configModifyInt("Calibd.config","rfswitch","steadyState",0,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	    
	case SET_CALPULSER_ATTEN:
	    //setCalPulserAtten
	    if(theCmd->cmd[1]>0 && theCmd->cmd[1]<8) {
		configModifyInt("Calibd.config","attenuator","attenStartState",theCmd->cmd[1],&rawtime);
		configModifyInt("Calibd.config","attenuator","attenLoop",0,&rawtime);
	    }
	    else //Switch on looping
		configModifyInt("Calibd.config","attenuator","attenLoop",1,&rawtime);
	    retVal=sendSignal(ID_CALIBD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	    
	case SET_ADU5_PAT_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
//	    printf("ivalue: %d %x\t %x %x\n",ivalue,ivalue,theCmd->cmd[1],theCmd->cmd[2]);
	    configModifyInt("GPSd.config","adu5","patPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_ADU5_SAT_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("GPSd.config","adu5","satPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;	    
	case SET_G12_PPS_PERIOD:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","g12","ppsPeriod",((float)ivalue)/1000.,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;	    	    
	case SET_G12_PPS_OFFSET: 
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyInt("GPSd.config","g12","ppsOffset",ivalue,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;	    	    
	case SET_ADU5_CALIBRATION_12:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV12_1",((float)ivalue)/1000.,&rawtime);
	    ivalue=theCmd->cmd[3]+(theCmd->cmd[4]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV12_2",((float)ivalue)/1000.,&rawtime);
	    ivalue=theCmd->cmd[5]+(theCmd->cmd[6]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV12_3",((float)ivalue)/1000.,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_ADU5_CALIBRATION_13:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV13_1",((float)ivalue)/1000.,&rawtime);
	    ivalue=theCmd->cmd[3]+(theCmd->cmd[4]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV13_2",((float)ivalue)/1000.,&rawtime);
	    ivalue=theCmd->cmd[5]+(theCmd->cmd[6]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV13_3",((float)ivalue)/1000.,&rawtime);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SET_ADU5_CALIBRATION_14:
	    ivalue=theCmd->cmd[1]+(theCmd->cmd[2]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV14_1",((float)ivalue)/1000.,&rawtime);
	    ivalue=theCmd->cmd[3]+(theCmd->cmd[4]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV14_2",((float)ivalue)/1000.,&rawtime);
	    ivalue=theCmd->cmd[5]+(theCmd->cmd[6]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV14_3",((float)ivalue)/1000.,&rawtime);
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
	    configModifyInt("Hkd.config","hkd","calPeriod",ivalue,&rawtime);
	    retVal=sendSignal(ID_HKD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SURF_ADU5_TRIG_FLAG:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","trigger","enablePPS1Trigger",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SURF_G12_TRIG_FLAG: 
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","trigger","enablePPS2Trigger",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SURF_RF_TRIG_FLAG: 
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","trigger","enableRFTrigger",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SURF_SOFT_TRIG_FLAG: 	    
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","trigger","enableSoftTrigger",ivalue,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
	case SURF_SOFT_TRIG_PERIOD:
	    ivalue=theCmd->cmd[1];
	    configModifyInt("Acqd.config","trigger","softTrigSleepPeriod",ivalue,&rawtime);
	    if(ivalue>0) 
		configModifyInt("Acqd.config","trigger","sendSoftTrig",1,&rawtime);
	    else
		configModifyInt("Acqd.config","trigger","sendSoftTrig",0,&rawtime);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    if(retVal) return 0;
	    return rawtime;
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
    ZippedFile_t *zipFilePtr;
    time_t rawtime;
    time(&rawtime);
    int retVal;
    int errorCount=0;
    ProgramId_t prog;
    char configFile[FILENAME_MAX];
    char outputName[FILENAME_MAX];
    char *fullFilename;
    char *inputBuffer;
    char *outputBuffer;
    unsigned long numBytesIn=0,numBytesOut=0;
    int testMask;	

    printf("sendConfig %d\n",progMask);
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
//	printf("%d %d\n",progMask,testMask);
	if(progMask&testMask) {
	    sprintf(configFile,"%s.config",getProgName(prog));
	    printf("Trying to send config file -- %s\n",configFile);
	    fullFilename=configFileSpec(configFile);
	    inputBuffer=readFile(fullFilename,&numBytesIn);
	    if(!inputBuffer || !numBytesIn) return 0;
	    numBytesOut=numBytesIn+1000 +sizeof(ZippedFile_t);
	    outputBuffer=malloc(numBytesOut);
	    zipFilePtr = (ZippedFile_t*) outputBuffer;
	    zipFilePtr->unixTime=rawtime;
	    zipFilePtr->numUncompressedBytes=numBytesIn;
	    strncpy(zipFilePtr->filename,configFile,60);
	    retVal=zipBuffer(inputBuffer,&outputBuffer[sizeof(ZippedFile_t)],numBytesIn,&numBytesOut);
	    if(retVal==0) {		
		fillGenericHeader(zipFilePtr,PACKET_ZIPPED_FILE,numBytesOut+sizeof(ZippedFile_t));
		sprintf(outputName,"%s/zipFile_%d_%lu.dat",REQUEST_TELEM_DIR,
			prog,zipFilePtr->unixTime);
		normalSingleWrite((unsigned char*)outputBuffer,
				  outputName,numBytesOut+sizeof(ZippedFile_t));
		makeLink(outputName,REQUEST_TELEM_LINK_DIR);		
	    }
	    free(inputBuffer);
	    free(outputBuffer);
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
    if(intExtFlag<1 || intExtFlag>3) return 0;

    readConfig(); // Get latest names and status
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

    prepWriterStructs();
    startPrograms(HKD_ID_MASK);
    startPrograms(GPSD_ID_MASK);
    startPrograms(ARCHIVED_ID_MASK);
    startPrograms(ACQD_ID_MASK);
    startPrograms(CALIBD_ID_MASK);
    startPrograms(MONITORD_ID_MASK);
    return rawtime;

}



int writeCommandEcho(CommandStruct_t *theCmd, int unixTime) {
    CommandEcho_t theEcho;
    int count,retVal=0;
    char filename[FILENAME_MAX];
    char cmdString[100];
    theEcho.gHdr.code=PACKET_CMD_ECHO;
    sprintf(cmdString,"%d",theCmd->cmd[0]);
    theEcho.numCmdBytes=theCmd->numCmdBytes;
    theEcho.cmd[0]=theCmd->cmd[0];
    if(theCmd->numCmdBytes>1) {
	for(count=1;count<theCmd->numCmdBytes;count++) {
	    sprintf(cmdString,"%s %d",cmdString,theCmd->cmd[count]);
	    theEcho.cmd[count]=theCmd->cmd[count];
	}
    }
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


