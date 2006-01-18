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
#include <signal.h>
#include <math.h>

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "anitaFlight.h"


#define MAX_GPS_TIMES 1000
#define MAX_CALIB_TIMES 100

#define TIME_MATCH 0.05 //seconds
#define DEFAULT_C3PO 200453324

int writeHeaderAndMakeLink(AnitaEventHeader_t *theHeaderPtr);
int getCalibStatus(int unixTime);
int setGpsTime(AnitaEventHeader_t *theHeaderPtr);
int deleteGPSFiles(GpsSubTime_t *theGpsPtr);
int compareTimes(AnitaEventHeader_t *theHeaderPtr, GpsSubTime_t *theGpsPtr, int forceReset);
int readConfigFile();

/* Directories and gubbins */
char eventdPidFile[FILENAME_MAX];
char acqdEventLinkDir[FILENAME_MAX];
char acqdEventDir[FILENAME_MAX];
char eventdEventDir[FILENAME_MAX];
char eventdEventLinkDir[FILENAME_MAX];
char calibdLinkDir[FILENAME_MAX];
char calibdStatusDir[FILENAME_MAX];
char gpsdSubTimeLinkDir[FILENAME_MAX];
char gpsdSubTimeDir[FILENAME_MAX];

int printToScreen=0;
int verbosity=0;

// Global Variables
GpsSubTime_t gpsArray[MAX_GPS_TIMES]; 
/*Will think about handling this better*/


int main (int argc, char *argv[])
{
    int retVal,count,filledSubTime;
    char currentFilename[FILENAME_MAX];
    char *tempString;
    /* Config file thingies */
    int status=0;
    char* eString ;


    /* Directory reading things */
    struct dirent **eventLinkList;
    int numEventLinks;
    
    /* Log stuff */
    char *progName=basename(argv[0]);

    /*Event object*/
    AnitaEventHeader_t theAcqdEventHeader;



    
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("eventdPidFile");
	if(tempString) {
	    strncpy(eventdPidFile,tempString,FILENAME_MAX);
	    writePidFile(eventdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get eventdPidFile");
	    fprintf(stderr,"Couldn't get eventdPidFile\n");
	}
	tempString=kvpGetString("gpsSubTimeDir");
	if(tempString) {
	    strncpy(gpsdSubTimeDir,tempString,FILENAME_MAX-1);
	    strncpy(gpsdSubTimeLinkDir,tempString,FILENAME_MAX-1);
	    strcat(gpsdSubTimeLinkDir,"/link");
	    makeDirectories(gpsdSubTimeLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdSubTimeDir");
	}
	tempString=kvpGetString("acqdEventDir");
	if(tempString) {
	    strncpy(acqdEventDir,tempString,FILENAME_MAX-1);
	    strncpy(acqdEventLinkDir,tempString,FILENAME_MAX-1);
	    strcat(acqdEventLinkDir,"/link");
	    makeDirectories(acqdEventLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get acqdEventDir");
	}
	tempString=kvpGetString("eventdEventDir");
	if(tempString) {
	    strncpy(eventdEventDir,tempString,FILENAME_MAX-1);
	    strncpy(eventdEventLinkDir,tempString,FILENAME_MAX-1);
	    strcat(eventdEventLinkDir,"/link");
	    makeDirectories(eventdEventLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get eventdEventDir");
	}
	tempString=kvpGetString("calibdStatusDir");
	if(tempString) {
	    strncpy(calibdStatusDir,tempString,FILENAME_MAX-1);
	    strncpy(calibdLinkDir,tempString,FILENAME_MAX-1);
	    strcat(calibdLinkDir,"/link");
	}
	else {
	    syslog(LOG_ERR,"Couldn't get calibdStatusDir");
	}
    }
    

    do {
	retVal=readConfigFile();
	if(printToScreen) 
	    printf("Initializing Eventd\n");
	if(retVal!=CONFIG_E_OK) {
	    ///Arrgh
	}
	retVal=0;
	/* Main event getting loop. */
	int numEvents=0;
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    numEventLinks=getListofLinks(acqdEventLinkDir,&eventLinkList);
/* 	printf("Got %d event links\n",numEventLinks); */
	    if(numEventLinks<1) {
		usleep(1);
		continue;
	    }
	    if(verbosity>1 && printToScreen) 
		printf("There are %d event links.\n",numEventLinks);

	    //This is just tomake sure the GPS sub times are up to date
	    sleep(1);
	    
	    /* What to do with our events? */	
	    for(count=0;count<numEventLinks;count++) {
		filledSubTime=0;
/* 	    printf("%s\n",eventLinkList[count]->d_name); */
		sprintf(currentFilename,"%s/%s",acqdEventLinkDir,
			eventLinkList[count]->d_name);
		retVal=fillHeader(&theAcqdEventHeader,currentFilename);
		filledSubTime=setGpsTime(&theAcqdEventHeader);
		
		if((time(NULL)-theAcqdEventHeader.unixTime)>2 
		   || filledSubTime) {
		    if(!filledSubTime) {
			theAcqdEventHeader.gpsSubTime=-1;
//		    syslog (LOG_WARNING,"No GPS sub time for event %d",
//			    theAcqdEventHeader.eventNumber);
			if(printToScreen)
			    printf("No GPS sub time for event %d\t%ld\t%ld\n",
				   theAcqdEventHeader.eventNumber,
				   theAcqdEventHeader.unixTime,
				   theAcqdEventHeader.unixTimeUs);
		    }
		    else if(printToScreen) 
			 printf("Match: Event %d\t Time:\t%ld\t%d\n",theAcqdEventHeader.eventNumber,theAcqdEventHeader.unixTime,theAcqdEventHeader.gpsSubTime);

//		    theAcqdEventHeader.calibStatus
//			=getCalibStatus(theAcqdEventHeader.unixTime);
		    writeHeaderAndMakeLink(&theAcqdEventHeader);
		    numEvents++;
	
		    
		}
		
		
	    }
	    	    	    
	    /* Free up the space used by dir queries */
	    for(count=0;count<numEventLinks;count++)
		free(eventLinkList[count]);
	    free(eventLinkList);
	    
//	usleep(10000);
/* 	break; */
//	sleep(2);

	}
    } while(currentState==PROG_STATE_INIT); 
    return 0;
}

int setGpsTime(AnitaEventHeader_t *theHeaderPtr)
{
    int retVal,count;
    char currentFilename[FILENAME_MAX];
    struct dirent **gpsSubTimeLinkList;
/*     static float avgDiff=0; */
/*     static int numDiffs=0; */
/*     static float lastDiff=0; */ 

//May implement a dynamic monitoring of the tim offset
    
// GPS subTime stuff
    static int numGpsStored=0;
    int numGpsTimeLinks;
    float fracUnix,fracTurf,fracGps;
    numGpsTimeLinks=getListofLinks(gpsdSubTimeLinkDir,&gpsSubTimeLinkList);
    
    if(verbosity>1) 	
	printf("There are %d gps links.\n",numGpsTimeLinks);
   

    /* need to do something if we ever have more 
       than MAX_GPS_TIMES subTimes.*/
//    numGpsStored=0;
//    bzero(gpsArray, sizeof(GpsSubTime_t)*MAX_GPS_TIMES) ;	    
    for(count=0;count<numGpsTimeLinks;count++) {
	sprintf(currentFilename,"%s/%s",gpsdSubTimeLinkDir,
		gpsSubTimeLinkList[count]->d_name);     
	retVal=fillGpsStruct(&gpsArray[numGpsStored],currentFilename);	
	if(retVal==0) {
	    /* Got One */
	    numGpsStored++;
	}
	deleteGPSFiles(&gpsArray[numGpsStored-1]);
    }    
    for(count=0;count<numGpsTimeLinks;count++)
	free(gpsSubTimeLinkList[count]);
    free(gpsSubTimeLinkList);
    if(printToScreen && verbosity)
	printf("There are %d gps times stored\n",numGpsStored);
    for(count=0;count<numGpsStored;count++) {
	fracUnix=((float)theHeaderPtr->unixTimeUs)/1e6;
	fracTurf=((float)theHeaderPtr->turfio.trigTime)/((float)DEFAULT_C3PO);
	fracGps=((float)gpsArray[count].subTime)/1e7;
	fracGps+=(gpsArray[count].unixTime-theHeaderPtr->unixTime);
/* 	printf("Event %d\t%ld.%ld\t%lu\n\tGPS %ld.%d\n", */
/* 	       theHeaderPtr->eventNumber,theHeaderPtr->unixTime, */
/* 	       theHeaderPtr->unixTimeUs,theHeaderPtr->turfio.trigTime, */
	/* 	       gpsArray[count].unixTime,gpsArray[count].subTime); */
	if(printToScreen && (verbosity>1  ))
	    printf("Event %d\t%f\t%f\t%f\n",theHeaderPtr->eventNumber,
		   fracUnix,fracTurf,fracGps);
	if(fabsf(fracUnix-fracGps)<TIME_MATCH) {
	    //We have a match
	    //Need to set gpsSubTime and delete all previous GPS times
	    theHeaderPtr->gpsSubTime=gpsArray[count].subTime;
	    bzero(gpsArray,(count+1)*sizeof(GpsSubTime_t));
	    numGpsStored-=(count+1);
	    if(numGpsStored) 
		memmove(&gpsArray[0],&gpsArray[count+1],numGpsStored*sizeof(GpsSubTime_t));
	    return 1;
	}
    }
/*     for(count=0;count<numGpsStored;count++) { */
/* 	fracUnix=((float)theHeaderPtr->unixTimeUs)/1e6; */
/* 	fracTurf=((float)theHeaderPtr->turfio.trigTime)/((float)DEFAULT_C3PO); */
/* 	fracGps=((float)gpsArray[count].subTime)/1e7; */
/* 	fracGps+=(gpsArray[count].unixTime-theHeaderPtr->unixTime); */
/* 	 printf("Event %d\t%f\t%f\t%f\n",theHeaderPtr->eventNumber, */
/* 		   fracUnix,fracTurf,fracGps); */
/*     } */
    return 0;
}



int writeHeaderAndMakeLink(AnitaEventHeader_t *theHeaderPtr)
{
    char theFilename[FILENAME_MAX];
    int retVal;
    FILE *testfp;
    
    /* Move ev_ file first */
    sprintf(theFilename,"%s/ev_%d.dat",acqdEventDir,
	    theHeaderPtr->eventNumber);
    testfp=fopen(theFilename,"rb");
    if(testfp) {
	if(printToScreen && verbosity) printf("Moving %s\n",theFilename);
	retVal=moveFile(theFilename,eventdEventDir);
	fclose(testfp);
    }
    /* Should probably do something with retVal */
       
    sprintf(theFilename,"%s/hd_%d.dat",eventdEventDir,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity) printf("Writing %s\n",theFilename);
    retVal=writeHeader(theHeaderPtr,theFilename);

    /* Make links, not sure what to do with return value here */
    retVal=makeLink(theFilename,eventdEventLinkDir);
    
    /* Delete previous hd_ file */
    sprintf(theFilename,"%s/hd_%d.dat",acqdEventDir,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity) printf("Deleting %s\n",theFilename);
    retVal=removeFile(theFilename);
   
    /* And the link */
    sprintf(theFilename,"%s/hd_%d.dat",acqdEventLinkDir,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity) printf("Deleting %s\n",theFilename);
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
    char otherFilename[FILENAME_MAX];
    int currentStatus=0, gotStatus=0;

    if(numCalibLinks>1 || (numCalibLinks>0 && numStored==0)) {
	/* Read out calib status, and delete all but the most recent link */
	for(count=0;count<numCalibLinks;count++) {
	    sprintf(currentFilename,"%s/%s",calibdLinkDir,
		    calibList[count]->d_name);	
	    sprintf(otherFilename,"%s/%s",calibdStatusDir,
		    calibList[count]->d_name);	       
	    if(count==0) {
		if(numStored==0) {
		    fillCalibStruct(&calibArray[numStored],currentFilename); 
		    numStored++;
		}
		removeFile(currentFilename);
		removeFile(otherFilename);
	    }
	    else {
		fillCalibStruct(&calibArray[numStored],currentFilename); 
		numStored++;
		if(count!=(numCalibLinks-1)) {
		    removeFile(currentFilename);
		    removeFile(otherFilename);
		}
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

int compareTimes(AnitaEventHeader_t *theHeaderPtr, GpsSubTime_t *theGpsPtr, int forceReset) 
{

    static unsigned long lastUnixTime=0;
    static unsigned long lastPPSNum=0;
    static unsigned long lastTrigTime=0;
    static int addedOneLastTime=0;
    static int subtractedOneLastTime=0;
    double computerTime=(double)theHeaderPtr->unixTime;
    double fracTime=(double)theHeaderPtr->turfio.trigTime;
    fracTime/=DEFAULT_C3PO;
    computerTime+=fracTime;

    double gpsTime=(double)(theGpsPtr->unixTime);
    double gpsFracTime=1e-7*(double)(theGpsPtr->subTime);
    gpsTime+=gpsFracTime;


    
//    if(abs(theHeaderPtr->unixTime-theGpsPtr->unixTime)>2) return 0; 	


    if((theHeaderPtr->turfio.trigTime>(lastTrigTime+20e6)) &&
       (theHeaderPtr->unixTime>lastUnixTime) && lastUnixTime && 
       !addedOneLastTime)
    {
	if(theHeaderPtr->turfio.ppsNum==lastPPSNum) {
	    computerTime-=1;
	    subtractedOneLastTime=1;
	    addedOneLastTime=0;
	}
    }
    else if((theHeaderPtr->turfio.trigTime<lastTrigTime) &&
	    (theHeaderPtr->unixTime==lastUnixTime) && lastUnixTime)
    {
	if(theHeaderPtr->turfio.ppsNum==lastPPSNum+1) {	    
	    computerTime+=1;
	    addedOneLastTime=1;
	    subtractedOneLastTime=0;
	}
    }
    else if(addedOneLastTime && theHeaderPtr->unixTime==lastUnixTime
	    && theHeaderPtr->turfio.ppsNum>=lastPPSNum) {
	computerTime+=1;
	addedOneLastTime=1;
	subtractedOneLastTime=0;
    }
    else if(addedOneLastTime && (theHeaderPtr->unixTime==lastUnixTime+1)
	    && (theHeaderPtr->turfio.ppsNum==lastPPSNum+1) &&
	    (theHeaderPtr->turfio.trigTime<lastTrigTime)) {
	computerTime+=1;
	addedOneLastTime=1;
	subtractedOneLastTime=0;

    }
    else {
	subtractedOneLastTime=0;
	addedOneLastTime=0;
    }
	
    double diff=computerTime-gpsTime;
    if(verbosity>1 && printToScreen) { 
	printf("Old:\t%lu\t%lu\t%lu\n",lastUnixTime,lastTrigTime,lastPPSNum);
	printf("New:\t%ld\t%lu\t%lu\n",theHeaderPtr->unixTime,
	       theHeaderPtr->turfio.trigTime,theHeaderPtr->turfio.ppsNum);
	
	printf("Raw:\t%ld\t%ld\n",theHeaderPtr->unixTime,theGpsPtr->unixTime);
	printf("Here:\t%9.2lf\t%9.2lf\t%9.9lf\n",computerTime,gpsTime,diff);
    }
    lastUnixTime=theHeaderPtr->unixTime;
    lastTrigTime=theHeaderPtr->turfio.trigTime;
    lastPPSNum=theHeaderPtr->turfio.ppsNum;
    if(diff<0) diff*=-1;
    if(diff < TIME_MATCH) return 1;
    return 0;
    
}

int deleteGPSFiles(GpsSubTime_t *theGpsPtr) 
{
    char theFilename[FILENAME_MAX];
    int retVal;

    sprintf(theFilename,"%s/gps_%ld_%d.dat",gpsdSubTimeLinkDir,
	    theGpsPtr->unixTime,theGpsPtr->subTime);
    if(printToScreen && verbosity) 
	printf("Deleting: %s\n",theFilename);
    retVal=removeFile(theFilename);
    sprintf(theFilename,"%s/gps_%ld_%d.dat",gpsdSubTimeDir,
	    theGpsPtr->unixTime,theGpsPtr->subTime);
    if(printToScreen && verbosity) printf("Deleting: %s\n",theFilename);
    retVal=removeFile(theFilename);
    return retVal;
}

int readConfigFile() 
/* Load Eventd config stuff */
{
    /* Config file thingies */
    int status=0;
    char* eString ;

    kvpReset();
    status = configLoad ("Eventd.config","output") ;
    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);

	   
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Eventd.config: %s\n",eString);
//	if(printToScreen)
	    fprintf(stderr,"Error reading Eventd.config: %s\n",eString);
	    
    }
    
//    printf("Debug rc3\n");
    return status;
}
