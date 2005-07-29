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


int main (int argc, char *argv[])
{
    int retVal,count,i;
    char currentFilename[FILENAME_MAX];
    char *tempString;

    /* Config file thingies */
    int status=0;
    char* eString ;

    /* Directories and gubbins */
    char gpsdSubTimeDir[FILENAME_MAX];
    char gpsdSubTimeArchiveLinkDir[FILENAME_MAX];

    /* Directory reading things */
    struct dirent **gpsSubTimeLinkList;
    int numGpsTimeLinks;
    
    /* Log stuff */
    char *progName=basename(argv[0]);

    int numGpsStored=0;
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	strncpy(gpsdSubTimeDir,kvpGetString("gpsdSubTimeDir"),
		FILENAME_MAX-1);
	strncpy(gpsdSubTimeArchiveLinkDir,kvpGetString("gpsdSubTimeArchiveLinkDir"),
		FILENAME_MAX-1);
    }



    
    retVal=0;
    /* Main event getting loop. */
    while(1) {

	numEventLinks=getListofLinks(acqdEventLinkDir,&eventLinkList);
/* 	printf("Got %d event links\n",numEventLinks); */
	if(numEventLinks<1) {
	    usleep(10000);
	    continue;
	}
	
	for(count=0;count<numEventLinks;count++) {
	    sprintf(currentFilename,"%s/%s",acqdEventLinkDir,
		    eventLinkList[count]->d_name);
	    retVal=fillHeader(&theAcqdEventHeader,currentFilename);

	
		
	}



      /* Free up the space used by dir queries */
        for(count=0;count<numEventLinks;count++)
            free(eventLinkList[count]);
        free(eventLinkList);
        for(count=0;count<numGpsTimeLinks;count++)
            free(gpsSubTimeLinkList[count]);
        free(gpsSubTimeLinkList);

	usleep(10000);
/* 	break; */
//	sleep(1);

    }	
}



