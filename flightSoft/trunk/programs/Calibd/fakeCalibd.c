/*! \file fakeCalibd.c
    \brief Fake version of the Calibd program 
    
    Going to be used in the testing of the inter-process communication,
    and will form the basis of the real Calibd program.
    August 2004  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"


void writeStatusChange(int unixTime, char calibStatus, const char *statusDir, const char *linkDir);
void waitUptoNSecs(int maxWait);

int main (int argc, char *argv[])
{
    /* Current Calibration Status */
    char currentStatus=0; /* Starts in off state */
      
    /* Config file thingies */
    int status=0;
    char* eString;
    char *globalConfFile="anitaSoft.config";

    /* Directory things */
    char calibdStatusDir[FILENAME_MAX];
    char calibdLinkDir[FILENAME_MAX];
    
    /* Log stuff */
    char *progName=basename(argv[0]);
    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Load Config */
    kvpReset () ;
    status = configLoad (globalConfFile,"global") ;
    eString = configErrorString (status) ;

    /* Get Port Numbers */
    if (status == CONFIG_E_OK) {
	strncpy(calibdStatusDir,
		kvpGetString("calibdStatusDir"),FILENAME_MAX-1);
	strncpy(calibdLinkDir,calibdStatusDir,FILENAME_MAX-1);
	strcat(calibdLinkDir,"/link");
    }

    makeDirectories(calibdLinkDir);

    while(1) {
	writeStatusChange(time(NULL),currentStatus,
			  calibdStatusDir,calibdLinkDir);
	waitUptoNSecs(100);
	currentStatus=1-currentStatus;
    }
    
}

void writeStatusChange(int unixTime, char calibStatus, const char *statusDir, const char *linkDir)
/*
  When the calibration status changes (on to off or off to on)
  write this in a file.
*/
{
    char filename[FILENAME_MAX];
    FILE *pFile;
    int retVal;

    /* Will name files with on or off in the title */
    if(calibStatus==1) 
	sprintf(filename,"%s/calib_%d_on.dat",statusDir,unixTime);
    else if(calibStatus==0)
	sprintf(filename,"%s/calib_%d_off.dat",statusDir,unixTime);
    else
	sprintf(filename,"%s/calib_%d_other.dat",statusDir,unixTime);

    pFile = fopen (filename, "w");
    if(pFile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	exit(0);
    }
    retVal=fprintf(pFile,"%d %d\n",unixTime,calibStatus);
    if(retVal<0) {
	syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),filename);
	exit(0);
    }
    fclose(pFile);
    makeLink(filename,linkDir);        
}

void waitUptoNSecs(int maxWait) 
{    
    double randomNumber=drand48();
    int numSecs=(int)((randomNumber*maxWait)+0.5);
    if(numSecs==0) numSecs+=2;
/*     printf("numSecs %d\n",numSecs); */
    sleep(numSecs);
    
}
