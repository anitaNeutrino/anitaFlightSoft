/*! \file Cmdd.c
    \brief The Cmdd program that receives commands from SIPd 
           and acts upon them 
    
    Receives command bytes from SIPd and then carries out the relevant action.

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

int executeCommand();
int sendSignal(ProgramId_t progId, int theSignal); 
int readConfig();


/* Global variables */
int cmdLengths[256];

int cmdBytes[20];
int numCmdBytes=0;
int numCmds=256;

/* PID Files */
char acqdPidFile[FILENAME_MAX];
char archivedPidFile[FILENAME_MAX];
char eventdPidFile[FILENAME_MAX];
char gpsdPidFile[FILENAME_MAX];
char hkdPidFile[FILENAME_MAX];
char losdPidFile[FILENAME_MAX];
char prioritizerdPidFile[FILENAME_MAX];
char sipdPidFile[FILENAME_MAX];


int main (int argc, char *argv[])
{
    int retVal;
    int count;


    
    /* Log stuff */
    char *progName=basename(argv[0]);

     /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    retVal=readConfig();

/*     for(count=0;count<numCmds;count++) { */
/* 	if(cmdLengths[count]) { */
/* 	    printf("%d\t%d\n",count,cmdLengths[count]); */
/* 	} */
/*     } */
    
    /* For now I'm just going to write this as a command line program, 
       at some point in the future it will metamorphose, into a proper
       daemon that either listens on a socket or looks in a directory for 
       commands. */
    if(argc>1) {
	for(count=1;count<argc;count++) {
	    cmdBytes[count-1]=atoi(argv[count]);
	    numCmdBytes++;
	}
    }
    else {
	fprintf(stderr,"Usage:\n\t%s <cmd byte 1> .... <cmd byte N> (N<=20)\n",
		progName);
	exit(0);
    }
//    printf("%d %d %d\n",cmdBytes[0],numCmdBytes,cmdLengths[cmdBytes[0]]);

    if(cmdLengths[cmdBytes[0]]!=numCmdBytes) {
	syslog(LOG_ERR,"Wrong number of command bytes for %d: expected %d, got %d\n",cmdBytes[0],cmdLengths[cmdBytes[0]],numCmdBytes);	
	fprintf(stderr,"Wrong number of command bytes for %d: expected %d, got %d\n",cmdBytes[0],cmdLengths[cmdBytes[0]],numCmdBytes);
	exit(0);
    }
    return executeCommand();
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
    eString = configErrorString (status) ;
    
    if (status == CONFIG_E_OK) {
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
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
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }
    return status;

}

int executeCommand() 
{
    int retVal=0;
    int period;
    char theCommand[FILENAME_MAX];
//    printf("%x %x %x %x\n",ACQD_ID_MASK,ARCHIVED_ID_MASK,CMDD_ID_MASK,
//	   ALL_ID_MASK);

//    configModifyInt("GPSd.config","output","printToScreen",0);
    switch(cmdBytes[0]) {
	case CMD_SHUTDOWN_HALT:
	    //Halt
	    sprintf(theCommand,"sudo halt");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_REBOOT:
	    //Reboot
	    sprintf(theCommand,"sudo reboot");
	    retVal=system(theCommand);
	    return retVal;
	case CMD_KILL_PROGS:
	    //Kill all progs
	    return retVal;
	case CMD_RESPAWN_PROGS:
	    //Respawn progs
	    return retVal;
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
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; setCalPulserAddr %d",cmdBytes[1]);
	    retVal=system(theCommand);
	    return retVal;	    
	case SET_SUNSENSOR_GAIN:
	    //setSunSensorGain
	    sprintf(theCommand,"source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; setSSGain %d",cmdBytes[1]);
	    retVal=system(theCommand);
	    return retVal;
	case SET_ADU5_PAT_PERIOD:
	    period=cmdBytes[1]+(cmdBytes[2]<<8);
//	    printf("period: %d %x\t %x %x\n",period,period,cmdBytes[1],cmdBytes[2]);
	    configModifyInt("GPSd.config","adu5","satPeriod",period);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    return retVal;
	case SET_ADU5_SAT_PERIOD:
	    return retVal;	    
	case SET_G12_PPS_PERIOD:
	    return retVal;	    
	case SET_G12_PPS_OFFSET: 
	    return retVal;
	case SET_HK_PERIOD: 
	    return retVal;
	case SET_HK_CAL_PERIOD: 
	    return retVal;
	case CLEAN_DIRS: 
	    return retVal;
	case SEND_CONFIG: 
	    return retVal;	    
	case DEFAULT_CONFIG:
	    return retVal;
	case SURF_ADU5_TRIG_FLAG: 
	    return retVal;
	case SURF_G12_TRIG_FLAG: 
	    return retVal;
	case SURF_RF_TRIG_FLAG: 
	    return retVal;	    
	case SURF_SELF_TRIG_FLAG: 
	    return retVal;
	case SURF_SELF_TRIG_PERIOD:
	    return retVal;
	default:
	    syslog(LOG_WARNING,"Unrecognised command %d\n",cmdBytes[0]);
	    return retVal;
    }
    return -1;
}

int sendSignal(ProgramId_t progId, int theSignal) 
{
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
    
    return kill(thePid,theSignal);
} 
