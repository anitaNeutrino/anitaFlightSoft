/*! \file Eventd.c
    \brief The Eventd program that creates Event objects 
    
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


#define MAX_GPS_TIMES 1000
#define MAX_CALIB_TIMES 100


int writeHeaderAndMakeLink(const char *outputDir, const char *linkDir, 
		const char *sourceDir, AnitaEventHeader_t *theHeaderPtr);
int getCalibStatus(const char *calibLinkDir, int unixTime);


int main (int argc, char *argv[])
{
    int retVal,count,i,filledSubTime;
    char currentFilename[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
    char* eString ;

    /* Directories and gubbins */
    char acqdEventLinkDir[FILENAME_MAX];
    char gpsdSubTimeLinkDir[FILENAME_MAX];
    char acqdEventDir[FILENAME_MAX];
    char eventdEventDir[FILENAME_MAX];
    char eventdEventLinkDir[FILENAME_MAX];
    char calibdLinkDir[FILENAME_MAX];

    /* Directory reading things */
    struct dirent **eventLinkList;
    struct dirent **gpsSubTimeLinkList;
    int numEventLinks;
    int numGpsTimeLinks;
    
    /* Log stuff */
    char *progName=basename(argv[0]);

   /*Event object*/
    AnitaEventHeader_t theAcqdEventHeader;

    /*GPS subTime stuff*/
    GpsSubTime_t gpsArray[MAX_GPS_TIMES]; /*Will think about handling this better*/
    int numGpsStored=0;
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	strncpy(gpsdSubTimeLinkDir,kvpGetString("gpsdSubTimeLinkDir"),
		FILENAME_MAX-1);
	strncpy(acqdEventDir,kvpGetString("acqdEventDir"),
		FILENAME_MAX-1);
	strncpy(acqdEventLinkDir,kvpGetString("acqdEventLinkDir"),
		FILENAME_MAX-1);
	strncpy(eventdEventDir,kvpGetString("eventdEventDir"),
		FILENAME_MAX-1);
	strncpy(eventdEventLinkDir,kvpGetString("eventdEventLinkDir"),
		FILENAME_MAX-1);
	strncpy(calibdLinkDir,
		kvpGetString("calibdLinkDir"),FILENAME_MAX-1);
    }


    makeDirectories(eventdEventDir);
    makeDirectories(eventdEventLinkDir);


    
    retVal=0;
    /* Main event getting loop. */
    while(1) {
	numEventLinks=getListofLinks(acqdEventLinkDir,&eventLinkList);
	if(numEventLinks<1) {
	    usleep(10000);
	    continue;
	}
	numGpsTimeLinks=getListofLinks(gpsdSubTimeLinkDir,&gpsSubTimeLinkList);
	
/* 	printf("There are %d event links.\n",numEventLinks); */
/* 	printf("There are %d gps links.\n",numGpsTimeLinks); */


	/* need to do something if we ever have more 
	   than MAX_GPS_TIMES subTimes.*/
	for(count=0;count<numGpsTimeLinks;count++) {
	    sprintf(currentFilename,"%s/%s",gpsdSubTimeLinkDir,
		    gpsSubTimeLinkList[count]->d_name);
	    retVal=fillGpsStruct(&gpsArray[numGpsStored],currentFilename);
	    if(retVal) {
		/* Got some */
		numGpsStored+=retVal;
	    }
	    retVal=removeFile(currentFilename);
	}

	printf("There are %d events and %d times.\n",numEventLinks,numGpsStored);
	/* What to do with our events? */	
	for(count=0;count<numEventLinks;count++) {
	    filledSubTime=0;
/* 	    printf("%s\n",eventLinkList[count]->d_name); */
	    sprintf(currentFilename,"%s/%s",acqdEventLinkDir,
		    eventLinkList[count]->d_name);
	    retVal=fillHeader(&theAcqdEventHeader,currentFilename);
/* 	    printf("Event %d, time %d\n",theAcqdEventHeader.eventNumber, */
/* 		   theAcqdEventHeader.unixTime); */
	    for(i=0;i<numGpsStored;i++) {
		if(abs(theAcqdEventHeader.unixTime-gpsArray[i].unixTime)<2) {
/* 		    printf("Match %d\t%d\n",gpsArray[i].unixTime,gpsArray[i].subTime); */
		    theAcqdEventHeader.unixTime=gpsArray[i].unixTime;
		    theAcqdEventHeader.gpsSubTime=gpsArray[i].subTime;
		    /* As events come time sorted can delete all previous
		       elements of the gpsArray. */

		    numGpsStored-=(i+1);
		    if(numGpsStored>0) {
			memmove(gpsArray,&gpsArray[i+1],
				numGpsStored*sizeof(GpsSubTime_t));
		    }
		    filledSubTime=1;
		    break;
		}
		if(theAcqdEventHeader.unixTime<gpsArray[i].unixTime) break;
	    }
	    

	    /* Maybe we didn't get the GPS time, if currentTime is later than
	       three seconds after then we'll write theHeader anyhow. */
	    if((time(NULL)-theAcqdEventHeader.unixTime)>3 || filledSubTime) {
		if(!filledSubTime) {
		    theAcqdEventHeader.gpsSubTime=-1;
		    syslog (LOG_WARNING,"No GPS sub time for event %d",
			    theAcqdEventHeader.eventNumber);
		}
		theAcqdEventHeader.calibStatus
		    =getCalibStatus(calibdLinkDir,theAcqdEventHeader.unixTime);
		writeHeaderAndMakeLink(eventdEventDir,eventdEventLinkDir,
			    acqdEventDir,&theAcqdEventHeader);
		removeFile(currentFilename);
	    }
		
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
/* 	sleep(1); */

    }	
}





int writeHeaderAndMakeLink(const char *outputDir, const char *linkDir, 
		const char *sourceDir, AnitaEventHeader_t *theHeaderPtr)
{
    char theFilename[FILENAME_MAX];
    int retVal;

    /* Move ev_ file first */
    sprintf(theFilename,"%s/ev_%d.dat",sourceDir,
	    theHeaderPtr->eventNumber);
    retVal=moveFile(theFilename,outputDir);
    /* Should probably do something with retVal */
       
    sprintf(theFilename,"%s/hd_%d.dat",outputDir,
	    theHeaderPtr->eventNumber);
    retVal=writeHeader(theHeaderPtr,theFilename);

    /* Make links, not sure what to do with return value here */
    retVal=makeLink(theFilename,linkDir);
    
   /* Delete previous hd_ file */
    sprintf(theFilename,"%s/hd_%d.dat",sourceDir,
	    theHeaderPtr->eventNumber);
    retVal=removeFile(theFilename);
    
    
    return retVal;
}

int getCalibStatus(const char *calibLinkDir, int unixTime)
/* Checks calibLinkDir and gets current calibration status 
   Currently implemented options are on or off */
{
    static CalibStruct_t calibArray[MAX_CALIB_TIMES];
    static int numStored=0;
    
    struct dirent **calibList;
    int numCalibLinks=getListofLinks(calibLinkDir,&calibList);
    int count;
    char currentFilename[FILENAME_MAX];
    int currentStatus=0, gotStatus=0;

    if(numCalibLinks>1 || (numCalibLinks>0 && numStored==0)) {
	/* Read out calib status, and delete all but the most recent link */
	for(count=0;count<numCalibLinks;count++) {
	    sprintf(currentFilename,"%s/%s",calibLinkDir,
		    calibList[count]->d_name);	    
	    if(count==0) {
		if(numStored==0) {
		    fillCalibStruct(&calibArray[numStored],currentFilename); 
		    numStored++;
		}
		removeFile(currentFilename);
	    }
	    else {
		fillCalibStruct(&calibArray[numStored],currentFilename); 
		numStored++;
		if(count!=(numCalibLinks-1)) removeFile(currentFilename);
	    }
	}
    }
    for(count=numStored-1;count>=0;count--) {
	if(unixTime>calibArray[count].unixTime) {
	    currentStatus=calibArray[count].status;
	    /* As events come time sorted can delete all previous
	       elements of the gpsArray. */
	    numStored-=(count);
	    if(numStored>0) {
		memmove(calibArray,&calibArray[count],
			numStored*sizeof(CalibStruct_t));
	    }
	    gotStatus=1;
	}
    }

    if(numCalibLinks>0) {
	for(count=0;count<numCalibLinks;count++)
	    free(calibList[count]);
	free(calibList);
    }
    if(gotStatus) return currentStatus;
    return -1;
    
}
