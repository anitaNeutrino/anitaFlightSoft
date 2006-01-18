/*! \file fakeGPSd.c
    \brief Fake version of the GPSd program (fake as it makes up the data)
    
    Going to be used in the testing of the inter-process communication,
    and will form the basis of the real GPSd program, when we have some
    hardware.
    February 2005 rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"

/* int getAndWriteSubTimesFromGPS(const char *theGpsSubTimeDir); */
int checkForFakeTriggerAndWriteSubTime(const char *theGpsSubTimeDir, const char *theGpsSubTimeLinkDir);
//void waitUntilNextSecond();

/* Useful global thing-a-me-bobs */

int main (int argc, char *argv[])
{
  int retVal;

    /* Config file thingies */
    int status=0;
    char* eString ;
    
    /* Ports and directories */
    char gpsSubTimeDir[FILENAME_MAX];
    char gpsSubTimeLinkDir[FILENAME_MAX];

    /* Log stuff */
    char *progName=basename(argv[0]);
    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    /* Read config file*/
    if (status == CONFIG_E_OK) {
	strncpy(gpsSubTimeDir,kvpGetString ("gpsSubTimeDir"),FILENAME_MAX-1);
	sprintf(gpsSubTimeLinkDir,"%s/link",gpsSubTimeDir);
    }

    makeDirectories(gpsSubTimeLinkDir);
    printf("\nThe GPS Sub Time Dir is: %s\n\n",gpsSubTimeDir);

  
/* Main gps getting loop */
    while(1) {
//	waitUntilNextSecond();
	retVal=checkForFakeTriggerAndWriteSubTime(gpsSubTimeDir,gpsSubTimeLinkDir);	
	if(retVal<0) {
	  printf("Bugger\n");
	  break;
	}
	usleep(10);
    }
    return 0;
}


int checkForFakeTriggerAndWriteSubTime(const char *theGpsSubTimeDir, const char *theGpsSubTimeLinkDir)
{
    struct dirent **triggerList;
    int numTriggers,count;
    char filename[FILENAME_MAX];
    FILE *pFile;
    GpsSubTime_t tempGPS;
    int retVal;
    numTriggers=getListofLinks("/tmp/anita/trigGPSd/link",&triggerList);
    if(numTriggers>0) {	
	for(count=0;count<numTriggers;count++) {
	    //Read file
	    sprintf(filename,"/tmp/anita/trigGPSd/%s",
		    triggerList[count]->d_name);
	    pFile = fopen (filename, "r");
	    if(pFile == NULL) {
		syslog (LOG_ERR,"fopen: %s\t %s",strerror(errno),filename);
		return -1;
	    }
	    fscanf(pFile,"%ld %d",
		   &(tempGPS.unixTime),&(tempGPS.subTime));
	    fclose(pFile);
	    removeFile(filename);  
	    sprintf(filename,"/tmp/anita/trigGPSd/link/%s",
		    triggerList[count]->d_name);
	    removeFile(filename); 

	    //Write file for eventd
	    sprintf(filename,"%s/gps_%ld_%d.dat",theGpsSubTimeDir,tempGPS.unixTime,tempGPS.subTime);
	    writeGpsTtt(&tempGPS,filename);
	    retVal=makeLink(filename,theGpsSubTimeLinkDir); 
   
	}
	for(count=0;count<numTriggers;count++)
	    free(triggerList[count]);
	free(triggerList);	 
    }
    return 0;
}



void waitUntilNextSecond()
{
    time_t start;
    start=time(NULL);
/*     printf("Start time %d\n",start); */
    while(start==time(NULL)) usleep(1000);
    
}
