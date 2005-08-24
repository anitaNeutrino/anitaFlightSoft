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
int cleanDirs();
int sendConfig();
int defaultConfig();
int killPrograms(int progMask);
int startPrograms(int progMask);
int respawnPrograms(int progMask);

/* Global variables */
int cmdLengths[256];

int cmdBytes[20];
int numCmdBytes=0;
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

/*Echo Dirs*/
char cmdEchoDir[FILENAME_MAX];
char cmdEchoLinkDir[FILENAME_MAX];



int main (int argc, char *argv[])
{
    int retVal;
    int count;
    char cmdString[180];
    CommandEcho_t theEcho;
    time_t rawtime;
    char filename[180];

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
    retVal=executeCommand();
    time(&rawtime);
    theEcho.gHdr.code=PACKET_CMD_ECHO;
    theEcho.unixTime=rawtime;
    theEcho.goodFlag=1;
    sprintf(cmdString,"%d",cmdBytes[0]);
    theEcho.numCmdBytes=numCmdBytes;
    theEcho.cmd[0]=cmdBytes[0];
    if(numCmdBytes>1) {
	for(count=1;count<numCmdBytes;count++) {
	    sprintf(cmdString,"%s %d",cmdString,cmdBytes[count]);
	    theEcho.cmd[count]=cmdBytes[count];
	}
    }

    if(retVal!=0) {
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
	tempString=kvpGetString("cmdEchoDir");
	if(tempString) {
	    strncpy(cmdEchoDir,tempString,FILENAME_MAX);
//	    makeDirectories(cmdEchoLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get cmdEchoDir");
	    fprintf(stderr,"Couldn't get cmdEchoDir\n");
	}
	tempString=kvpGetString("cmdEchoLinkDir");
	if(tempString) {
	    strncpy(cmdEchoLinkDir,tempString,FILENAME_MAX);
	    makeDirectories(cmdEchoLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get cmdEchoLinkDir");
	    fprintf(stderr,"Couldn't get cmdEchoLinkDir\n");
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
    int ivalue;
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
	    //Kill progs
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    retVal=killPrograms(ivalue);
	    return retVal;
	case CMD_RESPAWN_PROGS:
	    //Respawn progs
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    retVal=respawnPrograms(ivalue);	    
	    return retVal;
	case CMD_START_PROGS:
	    //Start progs
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    retVal=startPrograms(ivalue);	    
	    return retVal;
	case CMD_MOUNT:
	    //Mount -a
	    retVal=system("sudo mount -a");
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
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
//	    printf("ivalue: %d %x\t %x %x\n",ivalue,ivalue,cmdBytes[1],cmdBytes[2]);
	    configModifyInt("GPSd.config","adu5","patPeriod",ivalue);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    return retVal;
	case SET_ADU5_SAT_PERIOD:
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    configModifyInt("GPSd.config","adu5","satPeriod",ivalue);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    return retVal;	    
	case SET_G12_PPS_PERIOD:
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    configModifyFloat("GPSd.config","g12","ppsPeriod",((float)ivalue)/1000.);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    return retVal;	    
	case SET_G12_PPS_OFFSET: 
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    configModifyInt("GPSd.config","g12","ppsOffset",ivalue);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    return retVal;
	case SET_ADU5_CALIBRATION_12:
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV12_1",((float)ivalue)/1000.);
	    ivalue=cmdBytes[3]+(cmdBytes[4]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV12_2",((float)ivalue)/1000.);
	    ivalue=cmdBytes[5]+(cmdBytes[6]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV12_3",((float)ivalue)/1000.);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    return retVal;
	case SET_ADU5_CALIBRATION_13:
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV13_1",((float)ivalue)/1000.);
	    ivalue=cmdBytes[3]+(cmdBytes[4]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV13_2",((float)ivalue)/1000.);
	    ivalue=cmdBytes[5]+(cmdBytes[6]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV13_3",((float)ivalue)/1000.);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    return retVal;
	case SET_ADU5_CALIBRATION_14:
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV14_1",((float)ivalue)/1000.);
	    ivalue=cmdBytes[3]+(cmdBytes[4]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV14_2",((float)ivalue)/1000.);
	    ivalue=cmdBytes[5]+(cmdBytes[6]<<8);
	    configModifyFloat("GPSd.config","adu5","calibV14_3",((float)ivalue)/1000.);
	    retVal=sendSignal(ID_GPSD,SIGUSR1);
	    return retVal;
	    
	case SET_HK_PERIOD: 
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    configModifyInt("Hkd.config","hkd","readoutPeriod",ivalue);
	    retVal=sendSignal(ID_HKD,SIGUSR1);
	    return retVal;
	case SET_HK_CAL_PERIOD: 
	    ivalue=cmdBytes[1]+(cmdBytes[2]<<8);
	    configModifyInt("Hkd.config","hkd","calPeriod",ivalue);
	    retVal=sendSignal(ID_HKD,SIGUSR1);
	    return retVal;
	case CLEAN_DIRS: 	    
	    return cleanDirs();
	case SEND_CONFIG: 
	    return sendConfig();	    
	case DEFAULT_CONFIG:
	    return defaultConfig();
	case SURF_ADU5_TRIG_FLAG:
	    ivalue=cmdBytes[1];
	    configModifyInt("Acqd.config","trigger","enablePPS1Trigger",ivalue);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    return retVal;
	case SURF_G12_TRIG_FLAG: 
	    ivalue=cmdBytes[1];
	    configModifyInt("Acqd.config","trigger","enablePPS2Trigger",ivalue);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    return retVal;
	case SURF_RF_TRIG_FLAG: 
	    ivalue=cmdBytes[1];
	    configModifyInt("Acqd.config","trigger","enableRFTrigger",ivalue);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    return retVal;	    
	case SURF_SOFT_TRIG_FLAG: 	    
	    ivalue=cmdBytes[1];
	    configModifyInt("Acqd.config","trigger","enableSoftTrigger",ivalue);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    return retVal;
	case SURF_SOFT_TRIG_PERIOD:
	    ivalue=cmdBytes[1];
	    configModifyInt("Acqd.config","trigger","softTrigSleepPeriod",ivalue);
	    if(ivalue>0) 
		configModifyInt("Acqd.config","trigger","sendSoftTrig",1);
	    else
		configModifyInt("Acqd.config","trigger","sendSoftTrig",0);
	    retVal=sendSignal(ID_ACQD,SIGUSR1);
	    return retVal;
	default:
	    syslog(LOG_WARNING,"Unrecognised command %d\n",cmdBytes[0]);
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

int killPrograms(int progMask) 
{
    char daemonCommand[FILENAME_MAX];
    int retVal=0;
    if(progMask&ACQD_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Acqd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Acqd");
	    //Maybe do something clever
	}
    }
    if(progMask&ARCHIVED_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Archived");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Archived");
	    //Maybe do something clever
	}
    }
    if(progMask&CALIBD_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Calibd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Calibd");
	    //Maybe do something clever
	}
    }
    if(progMask&CMDD_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Cmdd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Cmdd");
	    //Maybe do something clever
	}
    }    
    if(progMask&EVENTD_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Eventd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Eventd");
	    //Maybe do something clever
	}
    }
    if(progMask&GPSD_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Gpsd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Gpsd");
	    //Maybe do something clever
	}
    }
    if(progMask&HKD_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Hkd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Hkd");
	    //Maybe do something clever
	}
    }    
    if(progMask&LOSD_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Losd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Losd");
	    //Maybe do something clever
	}
    }
    if(progMask&PRIORITIZERD_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Prioritizerd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Prioritizerd");
	    //Maybe do something clever
	}
    }
    if(progMask&SIPD_ID_MASK) {
	sprintf(daemonCommand,"daemon --stop -n Sipd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error killing Sipd");
	    //Maybe do something clever
	}
    }
    return 0;
}


int startPrograms(int progMask) 
{
    char daemonCommand[FILENAME_MAX];
    int retVal=0;
    if(progMask&ACQD_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Acqd -n Acqd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Acqd");
	    //Maybe do something clever
	}
    }
    if(progMask&ARCHIVED_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Archived -n Archived");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Archived");
	    //Maybe do something clever
	}
    }
    if(progMask&CALIBD_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Calibd -n Calibd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Calibd");
	    //Maybe do something clever
	}
    }
    if(progMask&CMDD_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Cmdd -n Cmdd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Cmdd");
	    //Maybe do something clever
	}
    }    
    if(progMask&EVENTD_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Eventd -n Eventd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Eventd");
	    //Maybe do something clever
	}
    }
    if(progMask&GPSD_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Gpsd -n Gpsd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Gpsd");
	    //Maybe do something clever
	}
    }
    if(progMask&HKD_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Hkd -n Hkd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Hkd");
	    //Maybe do something clever
	}
    }    
    if(progMask&LOSD_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Losd -n Losd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Losd");
	    //Maybe do something clever
	}
    }
    if(progMask&PRIORITIZERD_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Prioritizerd -n Prioritizerd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Prioritizerd");
	    //Maybe do something clever
	}
    }
    if(progMask&SIPD_ID_MASK) {
	sprintf(daemonCommand,"daemon -r Sipd -n Sipd");
	retVal=system(daemonCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"Error starting Sipd");
	    //Maybe do something clever
	}
    }
    return 0;
}

int respawnPrograms(int progMask)
{
    FILE *fpPid ;
    char fileName[FILENAME_MAX];
    pid_t thePid;
    int retVal=0;
    if(progMask&ACQD_ID_MASK) {
	sprintf(fileName,"%s",acqdPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    } 
    if(progMask&ARCHIVED_ID_MASK) {
	sprintf(fileName,"%s",archivedPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    }
    if(progMask&CALIBD_ID_MASK) {
	sprintf(fileName,"%s",calibdPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    }
    if(progMask&CMDD_ID_MASK) {
	sprintf(fileName,"%s",cmddPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    }
    if(progMask&EVENTD_ID_MASK) {
	sprintf(fileName,"%s",eventdPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    }
    if(progMask&GPSD_ID_MASK) {
	sprintf(fileName,"%s",gpsdPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    }
    if(progMask&HKD_ID_MASK) {
	sprintf(fileName,"%s",hkdPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    }
    if(progMask&LOSD_ID_MASK) {
	sprintf(fileName,"%s",losdPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    }
    if(progMask&PRIORITIZERD_ID_MASK) {
	sprintf(fileName,"%s",prioritizerdPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    }
    if(progMask&SIPD_ID_MASK) {
	sprintf(fileName,"%s",sipdPidFile);
	if (!(fpPid=fopen(fileName, "r"))) {
	    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	    //return -1;
	}
	fscanf(fpPid,"%d", &thePid) ;
	fclose(fpPid) ;
	retVal=kill(thePid,SIGUSR2);
    }

    return 0;
}
