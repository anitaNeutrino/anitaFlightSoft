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

#define TIME_MATCH 1000000 //In 100ns counts

int writeHeaderAndMakeLink(AnitaEventHeader_t *theHeaderPtr);
int getCalibStatus(int unixTime);
int deleteGPSFiles(GpsSubTime_t *theGpsPtr);
int compareTimes(AnitaEventHeader_t *theHeaderPtr, GpsSubTime_t *theGpsPtr);


/* Directories and gubbins */
char acqdEventLinkDir[FILENAME_MAX];
char acqdEventDir[FILENAME_MAX];
char eventdEventDir[FILENAME_MAX];
char eventdEventLinkDir[FILENAME_MAX];
char calibdLinkDir[FILENAME_MAX];
char gpsdSubTimeLinkDir[FILENAME_MAX];
char gpsdSubTimeDir[FILENAME_MAX];

int main (int argc, char *argv[])
{
    int retVal,count,i,filledSubTime;
    char currentFilename[FILENAME_MAX];
    char *tempString;

    /* Config file thingies */
    int status=0;
    char* eString ;


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
    int whichGps=0;

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("gpsdSubTimeDir");
	if(tempString) {
	    strncpy(gpsdSubTimeDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdSubTimeDir");
	}
	tempString=kvpGetString("gpsdSubTimeLinkDir");
	if(tempString) {
	    strncpy(gpsdSubTimeLinkDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdSubTimeLinkDir");
	}
	tempString=kvpGetString("acqdEventDir");
	if(tempString) {
	    strncpy(acqdEventDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get acqdEventDir");
	}
	tempString=kvpGetString("acqdEventLinkDir");
	if(tempString) {
	    strncpy(acqdEventLinkDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get acqdEventLinkDir");
	}
	tempString=kvpGetString("eventdEventDir");
	if(tempString) {
	    strncpy(eventdEventDir,tempString,FILENAME_MAX-1);
	    makeDirectories(eventdEventDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get eventdEventDir");
	}
	tempString=kvpGetString("eventdEventLinkDir");
	if(tempString) {
	    strncpy(eventdEventLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(eventdEventLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get eventdEventLinkDir");
	}
	tempString=kvpGetString("calibdLinkDir");
	if(tempString) {
	    strncpy(calibdLinkDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get calibdLinkDir");
	}
    }
    
    retVal=0;
    /* Main event getting loop. */
    while(1) {
/* 	printf("Starting Loop\n"); */
	numEventLinks=getListofLinks(acqdEventLinkDir,&eventLinkList);
/* 	printf("Got %d event links\n",numEventLinks); */
	if(numEventLinks<1) {
	    usleep(10000);
	    continue;
	}
	numGpsTimeLinks=getListofLinks(gpsdSubTimeLinkDir,&gpsSubTimeLinkList);
	
//	printf("There are %d event links.\n",numEventLinks);
//	printf("There are %d gps links.\n",numGpsTimeLinks);


	/* need to do something if we ever have more 
	   than MAX_GPS_TIMES subTimes.*/
	numGpsStored=0;
	bzero(gpsArray, sizeof(GpsSubTime_t)*MAX_GPS_TIMES) ;

	for(count=0;count<numGpsTimeLinks;count++) {
	    sprintf(currentFilename,"%s/%s",gpsdSubTimeLinkDir,
		    gpsSubTimeLinkList[count]->d_name);
	    retVal=fillGpsStruct(&gpsArray[numGpsStored],currentFilename);
	    if(retVal==0) {
		/* Got One */
		numGpsStored++;
	    }
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
/* 		printf("Event %d, time %d\tGPS\t%d\n", */
/* 		       theAcqdEventHeader.eventNumber, */
/* 		       theAcqdEventHeader.unixTime, */
/* 		       gpsArray[i].unixTime); */
		if(compareTimes(&theAcqdEventHeader,&gpsArray[i])) {
//		if(theAcqdEventHeader.unixTime==gpsArray[i].unixTime) {
		    
		    printf("Match: Event %d\t Time:\t%d\t%d\n",theAcqdEventHeader.eventNumber,gpsArray[i].unixTime,gpsArray[i].subTime);
		    theAcqdEventHeader.unixTime=gpsArray[i].unixTime;
		    theAcqdEventHeader.gpsSubTime=gpsArray[i].subTime;
		    /* As events come time sorted can delete all previous
		       elements of the gpsArray. */
//		    printf("Diff: %d\t%d\t%d\n",
//			   theAcqdEventHeader.unixTimeUs*10,
//			   theAcqdEventHeader.gpsSubTime,
//			   (theAcqdEventHeader.unixTimeUs*10)-theAcqdEventHeader.gpsSubTime);

		    filledSubTime=1;
		    break;
		}
//		if(theAcqdEventHeader.unixTime<gpsArray[i].unixTime) break;
	    }
	    

	    /* Maybe we didn't get the GPS time, if currentTime is later than
	       three seconds after then we'll write theHeader anyhow. */
	    if((time(NULL)-theAcqdEventHeader.unixTime)>3 || filledSubTime) {
		if(!filledSubTime) {
		    theAcqdEventHeader.gpsSubTime=-1;
		    syslog (LOG_WARNING,"No GPS sub time for event %d",
			    theAcqdEventHeader.eventNumber);
		    printf("No GPS sub time for event %d\t%d\t%d\n",
			   theAcqdEventHeader.eventNumber,
			   theAcqdEventHeader.unixTime,
			   theAcqdEventHeader.unixTimeUs);
		}
		theAcqdEventHeader.calibStatus
		    =getCalibStatus(theAcqdEventHeader.unixTime);
		writeHeaderAndMakeLink(&theAcqdEventHeader);

		whichGps=0;
		for(i=0;i<numGpsStored;i++) {
		    printf("GPS:\t%d\tEvent:\t%d\n",gpsArray[i].unixTime,
			   theAcqdEventHeader.unixTime);
		    if(gpsArray[i].unixTime<(theAcqdEventHeader.unixTime-1)) {

			//Delete GPS files
			deleteGPSFiles(&gpsArray[count]);
			whichGps=i;
		    }
		    else break;
		    
		}
		if(whichGps) {
		    numGpsStored-=(whichGps+1);	    
		    if(numGpsStored>0) {
			memmove(gpsArray,&gpsArray[whichGps+1],
				numGpsStored*sizeof(GpsSubTime_t));
		    }
		    
		}
		
		
	    }
	    
	    
	}



      /* Free up the space used by dir queries */
        for(count=0;count<numEventLinks;count++)
            free(eventLinkList[count]);
        free(eventLinkList);
        for(count=0;count<numGpsTimeLinks;count++)
            free(gpsSubTimeLinkList[count]);
        free(gpsSubTimeLinkList);

//	usleep(10000);
/* 	break; */
	sleep(2);

    }	
}





int writeHeaderAndMakeLink(AnitaEventHeader_t *theHeaderPtr)
{
    char theFilename[FILENAME_MAX];
    int retVal;
   
    /* Move ev_ file first */
    sprintf(theFilename,"%s/ev_%d.dat",acqdEventDir,
	    theHeaderPtr->eventNumber);
    printf("Moving %s\n",theFilename);
    retVal=moveFile(theFilename,eventdEventDir);
    /* Should probably do something with retVal */
       
    sprintf(theFilename,"%s/hd_%d.dat",eventdEventDir,
	    theHeaderPtr->eventNumber);
    printf("Writing %s\n",theFilename);
    retVal=writeHeader(theHeaderPtr,theFilename);

    /* Make links, not sure what to do with return value here */
    retVal=makeLink(theFilename,eventdEventLinkDir);
    
   /* Delete previous hd_ file */
    sprintf(theFilename,"%s/hd_%d.dat",acqdEventDir,
	    theHeaderPtr->eventNumber);
    printf("Deleting %s\n",theFilename);
    retVal=removeFile(theFilename);
   
    /* And the link */
    sprintf(theFilename,"%s/hd_%d.dat",acqdEventLinkDir,
	    theHeaderPtr->eventNumber);
    printf("Deleting %s\n",theFilename);
    retVal=removeFile(theFilename);
    
    return retVal;
}

int getCalibStatus(int unixTime)
/* Checks calibLinkDir and gets current calibration status 
   Currently implemented options are on or off */
{
    static CalibStruct_t calibArray[MAX_CALIB_TIMES];
    static int numStored=0;
    
    struct dirent **calibList;
    int numCalibLinks=getListofLinks(calibdLinkDir,&calibList);
    int count;
    char currentFilename[FILENAME_MAX];
    int currentStatus=0, gotStatus=0;

    if(numCalibLinks>1 || (numCalibLinks>0 && numStored==0)) {
	/* Read out calib status, and delete all but the most recent link */
	for(count=0;count<numCalibLinks;count++) {
	    sprintf(currentFilename,"%s/%s",calibdLinkDir,
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

int compareTimes(AnitaEventHeader_t *theHeaderPtr, GpsSubTime_t *theGpsPtr) 
{
    long secDiff=theHeaderPtr->unixTime-theGpsPtr->unixTime;
    long tickDiff=(theHeaderPtr->unixTimeUs*10)-theGpsPtr->subTime;

    if(abs(secDiff)>1) return 0;
    long theDiff=tickDiff+10000000*secDiff;
//    printf("%ld %ld %ld\n",secDiff,tickDiff,theDiff);

    return (abs(theDiff)<TIME_MATCH);
}

int deleteGPSFiles(GpsSubTime_t *theGpsPtr) 
{
    char theFilename[FILENAME_MAX];
    int retVal;

    sprintf(theFilename,"%s/gps_%d_%d.dat",gpsdSubTimeLinkDir,
	    theGpsPtr->unixTime,theGpsPtr->subTime);
    printf("Deleting: %s\n",theFilename);
    retVal=removeFile(theFilename);
    sprintf(theFilename,"%s/gps_%d_%d.dat",gpsdSubTimeDir,
	    theGpsPtr->unixTime,theGpsPtr->subTime);
    printf("Deleting: %s\n",theFilename);
    retVal=removeFile(theFilename);
    return retVal;
}
