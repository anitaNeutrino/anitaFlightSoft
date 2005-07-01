/*! \file GPSd.c
    \brief The GPSd program that creates GPS objects 
    
    Talks to the GPS units and does it's funky stuff

    Novemember 2004  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <unistd.h>
#include <time.h>

/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "serialLib/serialLib.h"
#include "anitaStructures.h"


int fdG12,fdAdu5A,fdAdu5B,fdMag;

int main (int argc, char *argv[])
{
    int retVal,printToScreen=0;

    // Device names;
    char g12DevName[FILENAME_MAX];
    char adu5ADevName[FILENAME_MAX];
    char adu5BDevName[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
    char* eString ;

    /* Log stuff */
    char *progName=basename(argv[0]);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status &= configLoad (GLOBAL_CONF_FILE,"whiteheat") ;
    status &= configLoad ("GPSd.config","output") ;
    eString = configErrorString (status) ;

    /* Get Device Names and config stuff */
    if (status == CONFIG_E_OK) {
	strncpy(g12DevName,kvpGetString("g12DevName"),FILENAME_MAX-1);
	strncpy(adu5ADevName,kvpGetString("adu5PortADevName"),FILENAME_MAX-1);
	strncpy(adu5BDevName,kvpGetString("adu5PortBDevName"),FILENAME_MAX-1);
	printToScreen=kvpGetInt("printToScreen",-1);
	if(printToScreen<0) {
	    syslog(LOG_WARNING,"Couldn't fetch printToScreen, defaulting to zero");
	    printToScreen=0;	    
	}
    }

//    printf("%d %s %s %s\n",printToScreen,g12DevName,adu5ADevName,adu5BDevName);
    // Initialize the various devices    
    retVal=openGPSDevice(g12DevName);
    if(retVal<=0) {
	printf("Couldn't open: %s\n",g12DevName);
	exit(1);
    }
    else fdG12=retVal;

    return 0;
}
