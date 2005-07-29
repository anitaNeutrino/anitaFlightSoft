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

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "anitaFlight.h"


/*Config stuff*/
int readConfigFile();


int main (int argc, char *argv[])
{
    int retVal,count,i;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    char *tempString;

    /* Config file thingies */
    int status=0;
    char* eString ;

    /* Directories and gubbins */
    char gpsdSubTimeDir[FILENAME_MAX];
    char gpsdSubTimeArchiveLinkDir[FILENAME_MAX];

    /* How many to accumulate before archiving */
    int gpsSubTimeCollection=100;

    /* Directory reading things */
    struct dirent **gpsSubTimeLinkList;
    int numGpsSubTimeLinks;
    
    /* Log stuff */
    char *progName=basename(argv[0]);
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("Archived.config","numbers") ;
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("gpsdSubTimeDir");
	if(tempString) 
	    strncpy(gpsdSubTimeDir,tempString,FILENAME_MAX-1);
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdSubTimeDir");
	    exit(0);
	}
	tempString=kvpGetString("gpsdSubTimeArchiveLinkDir");
	if(tempString)
	    strncpy(gpsdSubTimeArchiveLinkDir,tempString,FILENAME_MAX-1);
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdSubTimeArchiveLinkDir");
	    exit(0);
	}
	gpsSubTimeCollection=kvpGetInt("gpsSubTimeNumber",100);

    }

    int fool;
    retVal=0;
    /* Main event getting loop. */
    while(1) {

	numGpsSubTimeLinks=getListofLinks(gpsdSubTimeArchiveLinkDir,&gpsSubTimeLinkList);
	if(numGpsSubTimeLinks>gpsSubTimeLinkList) {
	    //Do something
	    for(count=0;count<numGpsSubTimeLinks;count++) {
		sprintf(currentFilename,"%s/%s",gpsdSubTimeDir,
			gpsSubTimeLinkList[count]->d_name);
		sprintf(currentLinkname,"%s/%s",gpsdSubTimeArchiveLinkDir,
			gpsSubTimeLinkList[count]->d_name);
		printf("%s\n%s\n",currentFilename,currentLinkname);
	    }
	}


      /* Free up the space used by dir queries */
        for(count=0;count<numGpsSubTimeLinks;count++)
            free(gpsSubTimeLinkList[count]);
        free(gpsSubTimeLinkList);
	
	sleep(1);
/* 	break; */
    }	
}



