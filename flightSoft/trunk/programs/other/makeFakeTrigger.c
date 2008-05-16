/*! \file makeFakeTrigger.c
    \brief Program to trigger fakeAcqd and fakeGPSd
    
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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "includes/anitaStructures.h"

void randomWaitForNextEvent();
int writeGpsThisTime(int microSec);


int main (int argc, char *argv[])
{
    char filename[FILENAME_MAX];
    char *acqdDir="/tmp/anita/trigAcqd";
    char *acqdLinkDir="/tmp/anita/trigAcqd/link";
    FILE *pFile;
    struct timeval timeStruct;
    GpsSubTime_t theTTT;
    theTTT.fromAdu5=0;
    int retVal=0;

    makeDirectories(acqdDir);
    makeDirectories(acqdLinkDir);
    makeDirectories(GPSD_SUBTIME_LINK_DIR);

    while(1) {
	randomWaitForNextEvent();
	gettimeofday(&timeStruct,NULL);
	//	printf("%d %d\n",timeStruct.tv_sec,timeStruct.tv_usec);
	if(writeGpsThisTime(timeStruct.tv_usec)) {
	  theTTT.unixTime=timeStruct.tv_sec;
	  theTTT.subTime=timeStruct.tv_usec*10;
	  //	  printf("%u %u\n",theTTT.unixTime,theTTT.subTime);
	  sprintf(filename,"%s/gps_%u_%u.dat",GPSD_SUBTIME_DIR,theTTT.unixTime,theTTT.subTime);
	  writeStruct(&theTTT,filename,sizeof(GpsSubTime_t));
	  retVal=makeLink(filename,GPSD_SUBTIME_LINK_DIR);  
	  theTTT.fromAdu5++;
	  if(theTTT.fromAdu5>2)
	    theTTT.fromAdu5=0;
	}

	//Acqd file
	sprintf(filename,"%s/acqd_%d_%d.dat",acqdDir,(int)timeStruct.tv_sec,(int)timeStruct.tv_usec);
	pFile=fopen(filename,"w");
	if(pFile == NULL) {
	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	    exit(0);
	}	
	fprintf(pFile,"%d %d\n",(int)timeStruct.tv_sec,(int)timeStruct.tv_usec*10);
	fclose(pFile);
	makeLink(filename,acqdLinkDir);
	
    }
  

}

void randomWaitForNextEvent() {
    /* Minimum gap between events will be 10 ms, maximum 10s */
    double randomNumber=drand48();
    int numChunks=(int)((randomNumber*10.0)+0.5); /* Should be *1000.0 */
    int numMicrosecs,numSecs;
    if(!numChunks) numChunks++;
    numMicrosecs=10000*numChunks;
    numSecs=numMicrosecs/1e6;;
    if(numSecs) {
	numMicrosecs=numMicrosecs-(1e6*numSecs);
    }
/*    printf("%f %d %d %d\n",randomNumber,numChunks,numMicrosecs,numSecs);*/
    if(numSecs) sleep(numSecs);
    usleep(numMicrosecs);
}

int writeGpsThisTime(int microSec) {
    double randomNumber=drand48();
    if(randomNumber<0.95) return 1;
    return 0;

}
