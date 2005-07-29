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

#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"

void randomWaitForNextEvent();

int main (int argc, char *argv[])
{
    char filename[FILENAME_MAX];
    char touchCommand[FILENAME_MAX+5];
    char *acqdDir="/tmp/anita/trigAcqd";
    char *gpsdDir="/tmp/anita/trigGPSd";
    char *gpsdLinkDir="/tmp/anita/trigGPSd/link";
    FILE *pFile;
    struct timeval timeStruct;
    int retVal;

    makeDirectories(acqdDir);
    makeDirectories(gpsdLinkDir);
    makeDirectories(gpsdDir);

    while(1) {
	randomWaitForNextEvent();
	gettimeofday(&timeStruct,NULL);
/* 	printf("%d %d\n",timeStruct.tv_sec,timeStruct.tv_usec); */
	sprintf(filename,"%s/gps_%ld_%ld.dat",gpsdDir,timeStruct.tv_sec,timeStruct.tv_usec);
	pFile=fopen(filename,"w");
	if(pFile == NULL) {
	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	    exit(0);
	}	
	fprintf(pFile,"%ld %ld\n",timeStruct.tv_sec,timeStruct.tv_usec);
	fclose(pFile);
	makeLink(filename,gpsdLinkDir);
	sprintf(touchCommand,"touch %s/acqd_%ld_%ld.dat",acqdDir,timeStruct.tv_sec,timeStruct.tv_usec);
	retVal=system(touchCommand);
	if(retVal!=0) {
	    syslog(LOG_ERR,"%s returned %d",touchCommand,retVal);
	}
	

    }
  

}

void randomWaitForNextEvent() {
    /* Minimum gap between events will be 10 ms, maximum 10s */
    double randomNumber=drand48();
    int numChunks=(int)((randomNumber*100.0)+0.5); /* Should be *1000.0 */
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

