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
int readAndWriteSubTimesFromGPS(const char *theGpsSubTimeDir, const char *theGpsSubTimeLinkDir);
void waitUntilNextSecond();

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
	strncpy(gpsSubTimeDir,kvpGetString ("gpsdSubTimeDir"),FILENAME_MAX-1);
	strncpy(gpsSubTimeLinkDir,kvpGetString ("gpsdSubTimeArchiveLinkDir"),FILENAME_MAX-1);
    }


    makeDirectories(gpsSubTimeDir);
    makeDirectories(gpsSubTimeLinkDir);
    printf("\nThe GPS Sub Time Dir is: %s\n\n",gpsSubTimeDir);


    /* Will open the GPS communications here.*/
  
/* Main gps getting loop */
    while(1) {
	waitUntilNextSecond();
	retVal=readAndWriteSubTimesFromGPS(gpsSubTimeDir,gpsSubTimeLinkDir);	
	if(retVal<0) {
	  printf("Bugger\n");
	  break;
	}
	sleep(1);
    }
    return 0;
}


int readAndWriteSubTimesFromGPS(const char *theGpsSubTimeDir, const char *theGpsSubTimeLinkDir)
{
    struct dirent **triggerList;
    int numTriggers,count;
    char filename[FILENAME_MAX];
    FILE *pFile;
    GpsSubTime_t *gpsArray;
    int retVal;
    numTriggers=getListofLinks("/tmp/anita/trigGPSd/link",&triggerList);
    if(numTriggers>0) {
	


	gpsArray=(GpsSubTime_t*) 
	    malloc(numTriggers*sizeof(GpsSubTime_t));
	for(count=0;count<numTriggers;count++) {
	    sprintf(filename,"/tmp/anita/trigGPSd/%s",
		    triggerList[count]->d_name);
/* 	    printf("%d %s\n",numTriggers,filename); */
	    pFile = fopen (filename, "r");
	    if(pFile == NULL) {
		syslog (LOG_ERR,"fopen: %s\t %s",strerror(errno),filename);
		return -1;
	    }
	    fscanf(pFile,"%ld %d",
		   &(gpsArray[count].unixTime),&(gpsArray[count].subTime));
	    fclose(pFile);
	    removeFile(filename);  
	    sprintf(filename,"/tmp/anita/trigGPSd/link/%s",
		    triggerList[count]->d_name);
	    removeFile(filename);    
	}
	sprintf(filename,"%s/gps_%ld_%d.dat",theGpsSubTimeDir,gpsArray[0].unixTime,gpsArray[0].subTime);
	pFile = fopen (filename, "w");
	if(pFile == NULL) {
	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	    exit(0);
	}	
	for(count=0;count<numTriggers;count++) {
	    retVal=fprintf(pFile,"%ld %d\n",gpsArray[count].unixTime,
			   gpsArray[count].subTime);
	    if(retVal<0) {
		syslog (LOG_ERR,"fprintf: %s ---  %s\n",
			strerror(errno),filename);
		exit(0);
	    }
	}
	fclose (pFile);

	/*Make link, ignore retVal for now.*/
	makeLink(filename,theGpsSubTimeLinkDir);
	free(gpsArray);
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
