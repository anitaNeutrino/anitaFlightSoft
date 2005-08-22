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

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "anitaFlight.h"
#include "anitaCommand.h"

int executeCommand();

/* Global variables */
int cmdLengths[256];

int cmdBytes[20];
int numCmdBytes=0;

int main (int argc, char *argv[])
{
//    int retVal;
    int numCmds=256,count;

    /* Config file thingies */
    int status=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    
    /* Log stuff */
    char *progName=basename(argv[0]);

     /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status &= configLoad ("Cmdd.config","lengths") ;
    eString = configErrorString (status) ;
    
    if (status == CONFIG_E_OK) {
/* 	printf("Here\n"); */
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
	//printf("%d\t%s\n",kvpStatus,kvpErrorString(kvpStatus));
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }

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

int executeCommand() 
{
    int retVal=0;
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
	    return retVal;
	case CMD_KILL_PROGS:
	    //Kill all progs
	    return retVal;
	case CMD_RESPAWN_PROGS:
	    //Respawn progs
	    return retVal;
	case CMD_TURN_GPS_ON:
	    //turnGPSOn
	    return retVal;
	case CMD_TURN_GPS_OFF:
	    //turnGPSOff
	    return retVal;
	case CMD_TURN_RFCM_ON:
	    //turnRFCMOn
	    return retVal;
	case CMD_TURN_RFCM_OFF:
	    //turnRFCMOff
	    return retVal;	    
	case CMD_TURN_CALPULSER_ON:
	    //turnCalPulserOn
	    return retVal;
	case CMD_TURN_CALPULSER_OFF:
	    //turnCalPulserOff
	    return retVal;
	case CMD_TURN_ND_ON:
	    //turnNDOn
	    return retVal;
	case CMD_TURN_ND_OFF:
	    //turnNDOff
	    return retVal;
	case CMD_TURN_ALL_ON:
	    //turnAllOn
	    return retVal;
	case CMD_TURN_ALL_OFF:
	    //turnAllOff
	    return retVal;	    
	case SET_CALPULSER_SWITCH:
	    //setCalPulserSwitch
	    return retVal;	    
	case SET_SUNSENSOR_GAIN:
	    //setSunSensorGain
	    return retVal;
	case SET_ADU5_PAT_PERIOD:
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


