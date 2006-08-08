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

//#define TIME_DEBUG 1

#define MAX_GPS_TIMES 2000
#define MAX_CALIB_TIMES 2000
#define EVENT_TIMEOUT 5

#define TIME_MATCH 0.05 //seconds
#define DEFAULT_C3PO 133000000

int writeHeaderAndMakeLink(AnitaEventHeader_t *theHeaderPtr);
int getCalibStatus(int unixTime);
int setGpsTime(AnitaEventHeader_t *theHeaderPtr);
int deleteGPSFiles(GpsSubTime_t *theGpsPtr);
int compareTimes(AnitaEventHeader_t *theHeaderPtr, GpsSubTime_t *theGpsPtr, int forceReset);
int readConfigFile();
int compareGpsTimes(const void *ptr1, const void *ptr2);


/* Directories and gubbins */
char eventdPidFile[FILENAME_MAX];

int printToScreen=0;
int verbosity=0;

AnitaEventHeader_t theAcqdEventHeader;

// Global Variables
GpsSubTime_t gpsArray[MAX_GPS_TIMES]; 
/*Will think about handling this better*/


#ifdef TIME_DEBUG
FILE *timeFile;
struct timeval timeStruct2;
#endif    

int main (int argc, char *argv[])
{
    int retVal,count,filledSubTime;
    unsigned long tempNum;
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



#ifdef TIME_DEBUG

    timeFile = fopen("/tmp/evTimeLog.txt","w");
    if(!timeFile) {
	fprintf(stderr,"Couldn't open time file\n");
	exit(0);
    }
#endif

    
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

    }
    
    makeDirectories(ACQD_EVENT_LINK_DIR);
    makeDirectories(EVENTD_EVENT_LINK_DIR);
    makeDirectories(CALIBD_STATUS_LINK_DIR);
    makeDirectories(GPSD_SUBTIME_LINK_DIR);

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

#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"0 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    numEventLinks=getListofLinks(ACQD_EVENT_LINK_DIR,&eventLinkList);
/* 	printf("Got %d event links\n",numEventLinks); */
	    if(numEventLinks<1) {
		usleep(1000);
		continue;
	    }
	    if(verbosity>2 && printToScreen) 
		fprintf(stderr,"There are %d event links.\n",numEventLinks);

	    //This is just to make sure the GPS sub times are up to date
//	    sleep(1);
	    
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"1 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    /* What to do with our events? */	
	    for(count=0;count<numEventLinks;count++) {
 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"2 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
		filledSubTime=0;
//		printf("%s\n",eventLinkList[count]->d_name);
		sprintf(currentFilename,"%s/%s",ACQD_EVENT_LINK_DIR,
			eventLinkList[count]->d_name);

		
		retVal=fillHeader(&theAcqdEventHeader,currentFilename);
//		printf("%s\t%lu\t%d\n",currentFilename,theAcqdEventHeader.eventNumber,retVal); 
//		exit(0);
#ifdef TIME_DEBUG
		gettimeofday(&timeStruct2,NULL);
		fprintf(timeFile,"3 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
//		printf("RetVal %d\n",retVal;
		if(retVal<0) {
		    removeFile(currentFilename);

		    sprintf(currentFilename,"%s/%s",ACQD_EVENT_DIR,
			    eventLinkList[count]->d_name);
		    removeFile(currentFilename);	
	    
		    sscanf(eventLinkList[count]->d_name,
			   "hd_%lu.dat",&tempNum);
		    if(tempNum>0) {
			sprintf(currentFilename,"%s/ev_%lu.dat",ACQD_EVENT_DIR,
				tempNum);
		    }		    
		    continue;
		}	
	    
		do {
//		    printf("Here %lu (filledSubTime %d)\n",theAcqdEventHeader.eventNumber,filledSubTime);
		    filledSubTime=setGpsTime(&theAcqdEventHeader);
		    if(!filledSubTime) {
			if((time(NULL)-theAcqdEventHeader.unixTime)<EVENT_TIMEOUT) sleep(1);
			else break;
		    }
		    else break;
		} while((time(NULL)-theAcqdEventHeader.unixTime)<EVENT_TIMEOUT
			&& !filledSubTime);
				
#ifdef TIME_DEBUG
		gettimeofday(&timeStruct2,NULL);
		fprintf(timeFile,"4 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
		if(!filledSubTime) {
		    theAcqdEventHeader.gpsSubTime=-1;
//		    syslog (LOG_WARNING,"No GPS sub time for event %d",
//			    theAcqdEventHeader.eventNumber);
		    if(printToScreen)
			fprintf(stdout,"No GPS sub time for event %lu\t%ld\t%ld\n",
			       theAcqdEventHeader.eventNumber,
			       theAcqdEventHeader.unixTime,
			       theAcqdEventHeader.unixTimeUs);
		}
		else if(printToScreen) 
		    fprintf(stdout,"Match: Event %lu\t Time:\t%ld\t%ld\n",theAcqdEventHeader.eventNumber,theAcqdEventHeader.unixTime,theAcqdEventHeader.gpsSubTime);
		
		    theAcqdEventHeader.calibStatus
			=getCalibStatus(theAcqdEventHeader.unixTime);
		writeHeaderAndMakeLink(&theAcqdEventHeader);
		numEvents++;
		
		    
	    }
	    
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"11 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    
	    
	    
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
    int haveMatch=0;
//May implement a dynamic monitoring of the tim offset
    
    static unsigned short lastPPSNum=0;
    static unsigned long turfPPSOffset=0;


    if(lastPPSNum>theHeaderPtr->turfio.ppsNum) {
	turfPPSOffset=0;
    }
    lastPPSNum=theHeaderPtr->turfio.ppsNum;
	

// GPS subTime stuff
    static int numGpsStored=0;
    int numGpsTimeLinks;
    float fracUnix,fracTurf,fracGps;
    numGpsTimeLinks=getListofLinks(GPSD_SUBTIME_LINK_DIR,&gpsSubTimeLinkList);
    
    if(verbosity>2 && printToScreen) 	
	fprintf(stderr,"There are %d gps links.\n",numGpsTimeLinks);
    
    
    /* need to do something if we ever have more 
       than MAX_GPS_TIMES subTimes.*/
    if(numGpsStored>MAX_GPS_TIMES-100) {
	syslog(LOG_ERR,"GPS overload junking %d times\n",numGpsStored);
	fprintf(stderr,"GPS overload junking %d times\n",numGpsStored);
	numGpsStored=0;
	bzero(gpsArray, sizeof(GpsSubTime_t)*MAX_GPS_TIMES) ;	    
    }

    for(count=0;count<numGpsTimeLinks;count++) {
	sprintf(currentFilename,"%s/%s",GPSD_SUBTIME_LINK_DIR,
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
    
    // As we are reading out two GPS units there is a chance that the times
    // will come out of time order.
    if(numGpsStored) {
	qsort(gpsArray,numGpsStored,sizeof(GpsSubTime_t),compareGpsTimes);
    }


    if(printToScreen && verbosity)
	fprintf(stderr,"There are %d gps times stored\n",numGpsStored);
    for(count=0;count<numGpsStored;count++) {
	fracUnix=((float)theHeaderPtr->unixTimeUs)/1e6;
	fracTurf=((float)theHeaderPtr->turfio.trigTime)/((float)DEFAULT_C3PO);
	if(turfPPSOffset) {
	    fracTurf+=(((float)(turfPPSOffset+theHeaderPtr->turfio.ppsNum))-
		       ((float)theHeaderPtr->unixTime));
	}
	fracGps=((float)gpsArray[count].subTime)/1e7;
	fracGps+=(((float)gpsArray[count].unixTime)
		  -((float)theHeaderPtr->unixTime));
	if(printToScreen&&verbosity>=0) {
	    printf("Event %lu\tUnix: %ld.%ld\tTURF:%u (%lu) %lu\n\tGPS %ld.%lu\n", 
		   theHeaderPtr->eventNumber,theHeaderPtr->unixTime, 
		   theHeaderPtr->unixTimeUs,theHeaderPtr->turfio.ppsNum,
		   (unsigned long)(theHeaderPtr->turfio.ppsNum)+turfPPSOffset,
		   theHeaderPtr->turfio.trigTime, 
		   gpsArray[count].unixTime,gpsArray[count].subTime); 
	}
	if(printToScreen && verbosity>=0)
	    printf("Event %ld\tUnix: %f\tTurf: %f\tGps: %f\n",theHeaderPtr->eventNumber,
		   fracUnix,fracTurf,fracGps);

	if(fabsf(fracTurf-fracGps)<TIME_MATCH) {
	    haveMatch=1;
	}
	else if(turfPPSOffset==0) {
	    //Maybe we are offset by a second
	    if(fabsf((fracTurf+1)-fracGps)<TIME_MATCH) {
		haveMatch=1;
	    }
	    else if(fabsf((fracTurf-1)-fracGps)<TIME_MATCH) {
		haveMatch=1;
	    }
	}

	
	if(haveMatch==1) {
	    //We have a match
	    //Need to set gpsSubTime and delete all previous GPS times
	    if(turfPPSOffset==0)
		turfPPSOffset=gpsArray[count].unixTime-theHeaderPtr->turfio.ppsNum;	    

	    theHeaderPtr->unixTime=gpsArray[count].unixTime;    
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
//    FILE *testfp;

		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"5 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
    
//    /* Move ev_ file first */
//    sprintf(theFilename,"%s/ev_%ld.dat",ACQD_EVENT_DIR,
//	    theHeaderPtr->eventNumber);
//    testfp=fopen(theFilename,"rb");
//    if(testfp) {
//	fclose(testfp);
//	if(printToScreen && verbosity) printf("Moving %s\n",theFilename);
//	retVal=moveFile(theFilename,eventdEventDir);
//    }


		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"6 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
    /* Should probably do something with retVal */
       
    sprintf(theFilename,"%s/hd_%ld.dat",EVENTD_EVENT_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity) printf("Writing %s\n",theFilename);
    retVal=writeHeader(theHeaderPtr,theFilename);

		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"7 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

    /* Make links, not sure what to do with return value here */
    retVal=makeLink(theFilename,EVENTD_EVENT_LINK_DIR);
    

		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"8 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

    /* Delete previous hd_ file */
    sprintf(theFilename,"%s/hd_%ld.dat",ACQD_EVENT_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity>2) printf("Deleting %s\n",theFilename);
    retVal=removeFile(theFilename);


		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"9 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
   
    /* And the link */
    sprintf(theFilename,"%s/hd_%ld.dat",ACQD_EVENT_LINK_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity>2) printf("Deleting %s\n",theFilename);
    retVal=removeFile(theFilename);

		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"10 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
    
    return retVal;
}

int getCalibStatus(int unixTime)
/* Checks calibLinkDir and gets current calibration status 
   Currently implemented options are on or off */
{
    static CalibStruct_t calibArray[MAX_CALIB_TIMES];
    static int numStored=0;
    
    struct dirent **calibList;
    int numCalibLinks=getListofLinks(CALIBD_STATUS_LINK_DIR,&calibList);
    int count;
    char currentFilename[FILENAME_MAX];
    char otherFilename[FILENAME_MAX];
    int currentStatus=0, gotStatus=0;

    if(printToScreen && verbosity)
	fprintf(stderr,"There are %d calibs stored\n",numStored);

    if(numCalibLinks>1 || (numCalibLinks>0 && numStored==0)) {
	/* Read out calib status, and delete all but the most recent link */
	for(count=0;count<numCalibLinks;count++) {
	    sprintf(currentFilename,"%s/%s",CALIBD_STATUS_LINK_DIR,
		    calibList[count]->d_name);	
	    sprintf(otherFilename,"%s/%s",CALIBD_STATUS_DIR,
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
    if(verbosity>2 && printToScreen) { 
	printf("Old:\t%lu\t%lu\t%lu\n",lastUnixTime,lastTrigTime,lastPPSNum);
	printf("New:\t%ld\t%lu\t%u\n",theHeaderPtr->unixTime,
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

    sprintf(theFilename,"%s/gps_%lu_%lu.dat",GPSD_SUBTIME_LINK_DIR,
	    theGpsPtr->unixTime,theGpsPtr->subTime);
    if(printToScreen && verbosity>2) 
	printf("Deleting: %s\n",theFilename);
    retVal=removeFile(theFilename);
    sprintf(theFilename,"%s/gps_%lu_%lu.dat",GPSD_SUBTIME_DIR,
	    theGpsPtr->unixTime,theGpsPtr->subTime);
    if(printToScreen && verbosity>2) printf("Deleting: %s\n",theFilename);
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


int compareGpsTimes(const void *ptr1, const void *ptr2) {
    GpsSubTime_t *gps1 = (GpsSubTime_t*) ptr1;
    GpsSubTime_t *gps2 = (GpsSubTime_t*) ptr2;
//    printf("gps1 %lu \t gps 2 %lu\n",gps1->unixTime,gps2->unixTime);

    if(gps1->unixTime<gps2->unixTime) return -1;
    else if(gps1->unixTime>gps2->unixTime) return 1;

    if(gps1->subTime<gps2->subTime) return -1;
    else if(gps1->subTime>gps2->subTime) return 1;
    
    return 0;

}
