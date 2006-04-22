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
#include "anitaStructures.h"
#include "anitaFlight.h"
#include "anitaCommand.h"

int checkCommand(CommandStruct_t *theCmd);
int checkForNewCommand();
int executeCommand(CommandStruct_t *theCmd); //Returns unixTime or 0
int writeCommandEcho(CommandStruct_t *theCommand, int unixTime);
int sendSignal(ProgramId_t progId, int theSignal); 
int readConfig();
int cleanDirs();
int sendConfig();
int defaultConfig();
int killPrograms(int progMask);
int startPrograms(int progMask);
int respawnPrograms(int progMask);
char *getProgName(ProgramId_t prog);
int getIdMask(ProgramId_t prog);
char *getPidFile(ProgramId_t prog);

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

// Directories for commands and command echoes
char cmddCommandDir[FILENAME_MAX];
char cmddCommandLinkDir[FILENAME_MAX];
char cmdEchoDir[FILENAME_MAX];
char cmdEchoLinkDir[FILENAME_MAX];

//Debugging Output
int printToScreen=0;
int verbosity=0;

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
			
		    }
		}
		
	    }
	}
	
    } while(currentState==PROG_STATE_INIT);
    return 0;
}


int checkForNewCommand() {
    struct dirent **cmdLinkList;
    int numCmdLinks=getListofLinks(cmddCommandLinkDir,&cmdLinkList);
    int uptoCmd;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    if(numCmdLinks) {
	for(uptoCmd=0;uptoCmd<numCmdLinks;uptoCmd++) {
	    if(uptoCmd==MAX_COMMANNDS) break;
	    sprintf(currentFilename,"%s/%s",cmddCommandDir,
		    cmdLinkList[uptoCmd]->d_name);
	    sprintf(currentLinkname,"%s/%s",cmddCommandLinkDir,
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

	tempString=kvpGetString("baseHouseTelemDir");
	if(tempString) {
	    strncpy(cmdEchoDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error baseHouseTelemDir");
	    fprintf(stderr,"Error baseHouseTelemDir\n");
	}
	tempString=kvpGetString("cmdEchoTelemSubDir");
	if(tempString) {
	    sprintf(cmdEchoDir,"%s/%s",cmdEchoDir,tempString);
	    sprintf(cmdEchoLinkDir,"%s/link",cmdEchoDir);
	    makeDirectories(cmdEchoLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting cmdEchoTelemSubDir");
	    fprintf(stderr,"Error getting cmdEchoTelemSubDir\n");
	}
	tempString=kvpGetString("cmddCommandDir");
	if(tempString) {
	    strncpy(cmddCommandDir,tempString,FILENAME_MAX);
	    sprintf(cmddCommandLinkDir,"%s/link",cmddCommandDir);
	    makeDirectories(cmddCommandLinkDir);       		
	}
	else {
	    syslog(LOG_ERR,"Couldn't get cmddCommandDir");
	    fprintf(stderr,"Couldn't get cmddCommandDir\n");
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
	    killPrograms(ID_HKD);
	    killPrograms(ID_GPSD);
	    killPrograms(ID_ARCHIVED);
	    retVal=system("sudo /sbin/rmmod usb-uhci");
	    retVal=system("sudo /sbin/insmod usb-uhci");
	    sleep(10);
	    retVal=system("sudo mount -a");
	    startPrograms(ID_HKD);
	    startPrograms(ID_GPSD);
	    startPrograms(ID_ARCHIVED);
	    if(retVal==-1) return 0;	    
	    time(&rawtime);
	    return rawtime;
	case CMD_TURN_GPS_ON:
	    //turnGPSOn
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnGPSOn");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_TURN_GPS_OFF:
	    //turnGPSOff
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnGPSOff");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_TURN_RFCM_ON:
	    //turnRFCMOn
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnRFCMOn");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_TURN_RFCM_OFF:
	    //turnRFCMOff
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnRFCMOff");
	    retVal=system(theCommand);
	    return retVal;	    
	case CMD_TURN_CALPULSER_ON:
	    //turnCalPulserOn
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnCalPulserOn");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_TURN_CALPULSER_OFF:
	    //turnCalPulserOff
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnCalPulserOff");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_TURN_ND_ON:
	    //turnNDOn
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnNDOn");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_TURN_ND_OFF:
	    //turnNDOff
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnNDOff");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_TURN_ALL_ON:
	    //turnAllOn
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnAllOn");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_TURN_ALL_OFF:
	    //turnAllOff
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; turnAllOff");
	    retVal=system(theCommand);
	    return retVal;	    
	case SET_CALPULSER_SWITCH:
	    //setCalPulserSwitch
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; setCalPulserAddr %d",theCmd->cmd[1]);
	    retVal=system(theCommand);
	    return retVal;	    
	case SET_SUNSENSOR_GAIN:
	    //setSunSensorGain
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; setSSGain %d",theCmd->cmd[1]);
	    retVal=system(theCommand);	   
	    return retVal;
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
	    return sendConfig();	    
	case DEFAULT_CONFIG:
	    return defaultConfig();
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
	case ID_SIPD:
	    sprintf(fileName,"%s",sipdPidFile);
	    break;    
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

int sendConfig() 
{
    return 0;
}

int defaultConfig()
{
    return 0;
}

int getIdMask(ProgramId_t prog) {
    switch(prog) {
	case ID_ACQD: return ACQD_ID_MASK;
	case ID_CALIBD: return CALIBD_ID_MASK;
	case ID_CMDD: return CMDD_ID_MASK;
	case ID_EVENTD: return EVENTD_ID_MASK;
	case ID_GPSD: return GPSD_ID_MASK;
	case ID_HKD: return HKD_ID_MASK;
	case ID_LOSD: return LOSD_ID_MASK;
	case ID_PRIORITIZERD: return PRIORITIZERD_ID_MASK;
	case ID_SIPD: return SIPD_ID_MASK;
	default: break;
    }
    return 0;
}


char *getProgName(ProgramId_t prog) {
    char *string;
    switch(prog) {
	case ID_ACQD: string="Acqd"; break;
	case ID_CALIBD: string="Calibd"; break;
	case ID_CMDD: string="Cmdd"; break;
	case ID_EVENTD: string="Eventd"; break;
	case ID_GPSD: string="GPSd"; break;
	case ID_HKD: string="Hkd"; break;
	case ID_LOSD: string="LOSd"; break;
	case ID_PRIORITIZERD: string="Prioritizerd"; break;
	case ID_SIPD: string="SIPd"; break;
	default: string=NULL; break;
    }
    return string;
}

char *getPidFile(ProgramId_t prog) {
    switch(prog) {
	case ID_ACQD: return acqdPidFile;
	case ID_CALIBD: return calibdPidFile;
	case ID_CMDD: return cmddPidFile;
	case ID_EVENTD: return eventdPidFile;
	case ID_GPSD: return gpsdPidFile;
	case ID_HKD: return hkdPidFile;
	case ID_LOSD: return losdPidFile;
	case ID_PRIORITIZERD: return prioritizerdPidFile;
	case ID_SIPD: return sipdPidFile;
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
    for(prog=ID_FIRST;prog<ID_NOT_AN_ID;prog++) {
	testMask=getIdMask(prog);
	if(progMask&testMask) {
	    sprintf(daemonCommand,"daemon --stop -n %s",getProgName(prog));
	    retVal=system(daemonCommand);
	    if(retVal!=0) {
		errorCount++;
		syslog(LOG_ERR,"Error killing %s",getProgName(prog));
		//Maybe do something clever
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
	    sprintf(daemonCommand,"daemon -r %s -n %s",
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
    sprintf(filename,"%s/cmd_%ld.dat",cmdEchoDir,theEcho.unixTime);
    {
	FILE *pFile;
	int fileTag=1;
	pFile = fopen(filename,"rb");
	while(pFile!=NULL) {
	    fclose(pFile);
	    sprintf(filename,"%s/cmd_%ld_%d.dat",cmdEchoDir,theEcho.unixTime,fileTag);
	    pFile=fopen(filename,"rb");
	}
    }
    writeCmdEcho(&theEcho,filename);
    makeLink(filename,cmdEchoLinkDir);
    
    return retVal;
}

