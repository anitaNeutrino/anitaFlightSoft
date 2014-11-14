/*! \file NTUd.c
  \brief The NTUd program that calls a script that 
  
  

  November 2014, r.nichol@ucl.ac.uk
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
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
void startCopyScript();
void stopCopyScript();

// Global Variables
int printToScreen=0;
int disableNtu=0;
pid_t copyScriptPid=-1;

int main (int argc, char *argv[])
{

    /* Log stuff */
    char *progName=basename(argv[0]);

    int retVal=sortOutPidFile(progName);
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
    
    
   
    retVal=readConfigFile();
    if(retVal<0) {
	syslog(LOG_ERR,"Problem reading NTUd.config");
	printf("Problem reading NTUd.config\n");
	exit(1);
    }
    do {
	if(printToScreen) printf("Initalizing NTUd\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading NTUd.config");
	    printf("Problem reading NTUd.config\n");
	    exit(1);
	}
	currentState=PROG_STATE_RUN;
	if(!disableNtu) startCopyScript();
	while(currentState==PROG_STATE_RUN) {
	  sleep(1);
	}	 
	if(copyScriptPid>0) stopCopyScript();
	copyScriptPid=-1;
	sleep(1);
    } while(currentState==PROG_STATE_INIT);   
    unlink(NTUD_PID_FILE);
    return 0;
}


int readConfigFile() 
/* Load NTUd config stuff */
{
  /* Config file thingies */
    int status=0;
    char* eString ;
    kvpReset();
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    if(status == CONFIG_E_OK) {
	disableNtu=kvpGetInt("disableNtu",0);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading NTUd.config: %s\n",eString);
    }
    

    return status;

}


void handleBadSigs(int sig)
{   
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig);  
    stopCopyScript();
    unlink(NTUD_PID_FILE);
    syslog(LOG_INFO,"NTUd terminating");    
    exit(0);
}

int sortOutPidFile(char *progName)
{
  int retVal=0;
  retVal=checkPidFile(NTUD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,NTUD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(NTUD_PID_FILE);  
  return 0;
}



void startCopyScript() {

  copyScriptPid = fork();
  if (copyScriptPid > 0) {
    
  }
  else if (copyScriptPid == 0){
    system("/home/anita/flightSoft/bin/ntuCopyScript.sh");
    printf("Finished copy script\n");
    exit(0);
  }

}

void stopCopyScript() {
  kill(copyScriptPid,SIGTERM);
}

