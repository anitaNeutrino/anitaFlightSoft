/*! \file Archived.c
  \brief The Archived program that creates Event objects 
    
  Takes data from Acqd (waveforms), GPSd (gps time) and Calibd (calibration)
  and forms event objects. It passes these objects on to Prioritizerd.
  August 2004  rjn@mps.ohio-state.edu
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

#define MAX_FILES_PER_COMMAND 10


int readConfigFile();
void executeCommand(char *tarCommand);
void checkGpsSubTimes();

/*Global Variables*/
int gzipGpsSubTime=0;
int gpsSubTimeCollection=100; /* How many to accumulate before archiving */
int printToScreen=1;

/* Directories and gubbins */
char gpsdSubTimeDir[FILENAME_MAX];
char gpsdSubTimeArchiveLinkDir[FILENAME_MAX];
char archivedGpsSubTimeDir[FILENAME_MAX];


int main (int argc, char *argv[])
{
    int retVal;
    char *tempString;
    /* Config file thingies */
    int status=0;
    char* eString ;
    
    /* Log stuff */
    char *progName=basename(argv[0]);
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGINT, sigIntHandler);
    signal(SIGTERM,sigTermHandler);
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("gpsdSubTimeDir");
	if(tempString) 
	    strncpy(gpsdSubTimeDir,tempString,FILENAME_MAX-1);
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdSubTimeDir");
	    exit(0);
	}
	tempString=kvpGetString("archivedGpsSubTimeDir");
	if(tempString) 
	    strncpy(archivedGpsSubTimeDir,tempString,FILENAME_MAX-1);
	else {
	    syslog(LOG_ERR,"Couldn't get archivedGpsSubTimeDir");
	    exit(0);
	}
	tempString=kvpGetString("gpsdSubTimeArchiveLinkDir");
	if(tempString)
	    strncpy(gpsdSubTimeArchiveLinkDir,tempString,FILENAME_MAX-1);
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdSubTimeArchiveLinkDir");
	    exit(0);
	}

    }
    
    makeDirectories(archivedGpsSubTimeDir);

    retVal=0;
    /* Main event getting loop. */

    do {
	if(printToScreen) printf("Initalizing Archived\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading GPSd.config");
	    printf("Problem reading GPSd.config\n");
	    exit(1);
	}
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    checkGpsSubTimes();
	    sleep(1);
	}
    } while(currentState==PROG_STATE_INIT);    
    return 0;
}


void executeCommand(char *tarCommand) {
    if(printToScreen) printf("%s\n\n",tarCommand);
    int retVal=system(tarCommand);
    if(retVal<0) {
	syslog(LOG_ERR,"Archived problem tarring");
	fprintf(stderr,"Archived problem tarring\n");
    }

}


int readConfigFile() 
/* Load Archived config stuff */
{
    /* Config file thingies */
    int status=0;
    char* eString ;
    kvpReset();
    status = configLoad ("Archived.config","archived") ;
    status += configLoad ("Archived.config","output") ;

    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",0);
	gzipGpsSubTime=kvpGetInt("gzipGpsSubTime",0);
	gpsSubTimeCollection=kvpGetInt("gpsSubTimeNumber",100);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Archived.config: %s\n",eString);
    }
    
    return status;
}

void checkGpsSubTimes() 
{
    int count;
    /* Directory reading things */
    struct dirent **gpsSubTimeLinkList;
    int numGpsSubTimeLinks;

    long unixTime,subTime;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    char tarCommand[FILENAME_MAX*MAX_FILES_PER_COMMAND];
    char gzipCommand[FILENAME_MAX];
    char gzipFile[FILENAME_MAX];


    numGpsSubTimeLinks=getListofLinks(gpsdSubTimeArchiveLinkDir,&gpsSubTimeLinkList);
    if(printToScreen) printf("Found %d links\n",numGpsSubTimeLinks);
    if(numGpsSubTimeLinks>gpsSubTimeCollection) {
	//Do something
	sscanf(gpsSubTimeLinkList[0]->d_name,
	       "gps_%ld_%ld.dat",&unixTime,&subTime);
	sprintf(tarCommand,"cd %s ; tar -cf /tmp/subTime%ld_%ld.tar ",gpsdSubTimeDir,unixTime,subTime);
	
	for(count=0;count<numGpsSubTimeLinks;count++) {
	    if((((count)%MAX_FILES_PER_COMMAND)==0) && count) {
//		    printf("%s\n",tarCommand);
		executeCommand(tarCommand);
		sprintf(tarCommand,"cd %s ; tar -rf /tmp/subTime%ld_%ld.tar ",gpsdSubTimeDir,unixTime,subTime);
	    }	
	    strcat(tarCommand,gpsSubTimeLinkList[count]->d_name);
	    strcat(tarCommand," ");
		
	}
	executeCommand(tarCommand);	   
	if(gzipGpsSubTime) {
	    sprintf(gzipCommand,"gzip /tmp/subTime%ld_%ld.tar",unixTime,subTime);
	    executeCommand(gzipCommand);
	    sprintf(gzipFile,"/tmp/subTime%ld_%ld.tar.gz",unixTime,subTime);		
	}
	else sprintf(gzipFile,"/tmp/subTime%ld_%ld.tar",unixTime,subTime);
	moveFile(gzipFile,archivedGpsSubTimeDir); 
//	    exit(0);
	for(count=0;count<numGpsSubTimeLinks;count++) {
	    sprintf(currentFilename,"%s/%s",gpsdSubTimeDir,
		    gpsSubTimeLinkList[count]->d_name);
	    sprintf(currentLinkname,"%s/%s",gpsdSubTimeArchiveLinkDir,
		    gpsSubTimeLinkList[count]->d_name);
	    removeFile(currentLinkname);
	    removeFile(currentFilename);
	}
    }
	

    /* Free up the space used by dir queries */
    for(count=0;count<numGpsSubTimeLinks;count++)
	free(gpsSubTimeLinkList[count]);
    free(gpsSubTimeLinkList);
	

    
}
