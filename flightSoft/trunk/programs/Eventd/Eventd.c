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
#include <libgen.h> //For Mac OS X

//Flightsoft Includes
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "linkWatchLib/linkWatchLib.h"
#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"

//#define TIME_DEBUG 1

#define MAX_GPS_TIMES 500
#define MAX_CALIB_TIMES 2000
#define EVENT_TIMEOUT 5

#define TIME_MATCH 0.005 //seconds
#define DEFAULT_C3PO 133000000

int writeHeaderAndMakeLink(AnitaEventHeader_t *theHeaderPtr);
int getCalibStatus(int unixTime);
int setGpsTime(AnitaEventHeader_t *theHeaderPtr);
int deleteGPSFiles(GpsSubTime_t *theGpsPtr);
void clearGpsDir();
int compareTimes(AnitaEventHeader_t *theHeaderPtr, GpsSubTime_t *theGpsPtr, int forceReset);
int readConfigFile();
int compareGpsTimes(const void *ptr1, const void *ptr2);
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);

/* Directories and gubbins */
int printToScreen=0;
int verbosity=0;
int tryToMatchGps=0;

AnitaEventHeader_t theAcqdEventHeader;

// Global Variables
GpsSubTime_t gpsArray[MAX_GPS_TIMES]; 
/*Will think about handling this better*/



int wdEvent=0;
int wdGps=0;
int wdCalib=0;

int main (int argc, char *argv[])
{
    int retVal,count,filledSubTime;
    unsigned int tempNum;
    char currentFilename[FILENAME_MAX];


    /* Directory reading things */

    int numEventLinks;
    char *tempString=0;

    /* Log stuff */
    char *progName=basename(argv[0]);

    // Sort out PID File
    retVal=sortOutPidFile(progName);
    if(retVal!=0) {
      return retVal;
    }
    
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, handleBadSigs);
    signal(SIGINT, handleBadSigs);
    signal(SIGSEGV, handleBadSigs);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    
    makeDirectories(ACQD_EVENT_LINK_DIR);
    makeDirectories(EVENTD_EVENT_LINK_DIR);
    makeDirectories(CALIBD_STATUS_LINK_DIR);
    makeDirectories(GPSD_SUBTIME_LINK_DIR);

    if(wdEvent==0) {
      wdEvent=setupLinkWatchDir(ACQD_EVENT_LINK_DIR);
      if(wdEvent<=0) {
	fprintf(stderr,"Unable to watch %s\n",ACQD_EVENT_LINK_DIR);
	syslog(LOG_ERR,"Unable to watch %s\n",ACQD_EVENT_LINK_DIR);
	exit(0);
      }
      numEventLinks=getNumLinks(wdEvent);
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
	  retVal=checkLinkDirs(1,0);
	  if(retVal || numEventLinks)
	    numEventLinks=getNumLinks(wdEvent);
	  /* 	printf("Got %d event links\n",numEventLinks); */
	  if(numEventLinks<1) {
	    usleep(1000);
	    continue;
	  }
	  if(verbosity>2 && printToScreen) 
	    fprintf(stderr,"There are %d event links.\n",numEventLinks);

	  //This is just to make sure the GPS sub times are up to date
	  //	    sleep(1);
	  
	  /* What to do with our events? */	
	  for(count=0;count<numEventLinks;count++) {
	    tempString=getFirstLink(wdEvent);
	    filledSubTime=0;
	    //		printf("%s\n",eventLinkList[count]->d_name);
	    sprintf(currentFilename,"%s/%s",ACQD_EVENT_LINK_DIR,tempString);

	    
	    retVal=fillHeader(&theAcqdEventHeader,currentFilename);
	    //		printf("%s\t%u\t%d\n",currentFilename,theAcqdEventHeader.eventNumber,retVal); 
	    //		exit(0);
	    //		printf("RetVal %d\n",retVal;
	    if(retVal<0) {
	      removeFile(currentFilename);
	      
	      sprintf(currentFilename,"%s/%s",ACQD_EVENT_DIR,tempString);
	      removeFile(currentFilename);	
	      
	      sscanf(tempString,"hd_%u.dat",&tempNum);
	      if(tempNum>0) {
		sprintf(currentFilename,"%s/ev_%u.dat",ACQD_EVENT_DIR,
			tempNum);
	      }		    
	      continue;
	    }	
	    
	    int tryCount=0;
	    do {
	      //		    printf("Here %u (filledSubTime %d)\n",theAcqdEventHeader.eventNumber,filledSubTime);
	      if(tryToMatchGps) {
		filledSubTime=setGpsTime(&theAcqdEventHeader);
	      }
	      else {
		filledSubTime=0;
		clearGpsDir();
		break;
	      }
		    if(!filledSubTime) {
		      if((time(NULL)-theAcqdEventHeader.unixTime)<EVENT_TIMEOUT) sleep(1);
		      else break;
		    }
		    else break;
		    tryCount++;
	    } while((time(NULL)-theAcqdEventHeader.unixTime)<EVENT_TIMEOUT
		    && !filledSubTime && tryCount<20);
	    
	    if(!filledSubTime) {
	      theAcqdEventHeader.gpsSubTime=-1;
	      //		    syslog (LOG_WARNING,"No GPS sub time for event %d",
	      //			    theAcqdEventHeader.eventNumber);
	      if(printToScreen)
		fprintf(stdout,"No GPS sub time for event %u\t%d\t%d\n",
			theAcqdEventHeader.eventNumber,
			theAcqdEventHeader.unixTime,
			theAcqdEventHeader.unixTimeUs);
	    }
	    else if(printToScreen) 
	      fprintf(stdout,"Match: Event %u\t Time:\t%d\t%d\n",theAcqdEventHeader.eventNumber,theAcqdEventHeader.unixTime,theAcqdEventHeader.gpsSubTime);
	    
	    theAcqdEventHeader.calibStatus
	      =getCalibStatus(theAcqdEventHeader.unixTime);
	    writeHeaderAndMakeLink(&theAcqdEventHeader);
	    numEvents++;
	    
	    
	  }
	  	    
	  //	usleep(10000);
	  /* 	break; */
	  //	sleep(2);
	    
	}
    } while(currentState==PROG_STATE_INIT); 
    unlink(EVENTD_PID_FILE);
    syslog(LOG_INFO,"Eventd terminating");
    return 0;
}


void clearGpsDir() 
{
    int  count;
    char *tempString=0;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    checkLinkDirs(0,1);
    int numGpsTimeLinks=getNumLinks(wdGps);
    for(count=0;count<numGpsTimeLinks;count++) {
      tempString=getFirstLink(wdGps);
      sprintf(currentFilename,"%s/%s",GPSD_SUBTIME_LINK_DIR,tempString);
      sprintf(currentLinkname,"%s/%s",GPSD_SUBTIME_DIR,tempString);
      removeFile(currentFilename);
      removeFile(currentLinkname);      
    }
}

int setGpsTime(AnitaEventHeader_t *theHeaderPtr)
{
    int retVal,count;
    char *tempString=0;
    char currentFilename[FILENAME_MAX];
/*     static float avgDiff=0; */
/*     static int numDiffs=0; */
/*     static float lastDiff=0; */ 
    int haveMatch=0;
//May implement a dynamic monitoring of the time offset
    int newGpsSubTime;
    static unsigned short lastPPSNum=0;
    static unsigned int turfPPSOffset=0;
    int numGpsTimeLinks=0;
    static int numGpsStored=0;
    float fracUnix,fracTurf,fracGps;


    if(wdGps==0) {
       //First time need to prep the watcher directory
      wdGps=setupLinkWatchDir(GPSD_SUBTIME_LINK_DIR);
      if(wdGps<=0) {
	fprintf(stderr,"Unable to watch %s\n",GPSD_SUBTIME_LINK_DIR);
	syslog(LOG_ERR,"Unable to watch %s\n",GPSD_SUBTIME_LINK_DIR);
	//	exit(0);
      }
      numGpsTimeLinks=getNumLinks(wdGps);
    }

    if(lastPPSNum>theHeaderPtr->turfio.ppsNum) {
	turfPPSOffset=0;
    }
    lastPPSNum=theHeaderPtr->turfio.ppsNum;
	

// GPS subTime stuff
    retVal=checkLinkDirs(0,1);
    if(retVal || numGpsTimeLinks)
      numGpsTimeLinks=getNumLinks(wdGps);
    int loadGpsLinks=numGpsTimeLinks;
    
    if(verbosity>2 && printToScreen) 	
      fprintf(stderr,"There are %d gps links.\n",numGpsTimeLinks);
        
    /* need to do something if we ever have more 
       than MAX_GPS_TIMES subTimes.*/
    if(numGpsStored+numGpsTimeLinks>MAX_GPS_TIMES-100) {
	syslog(LOG_ERR,"GPS overload junking %d times\n",numGpsStored);
	fprintf(stderr,"GPS overload junking %d times\n",numGpsStored);
	numGpsStored=0;
	bzero(gpsArray, sizeof(GpsSubTime_t)*MAX_GPS_TIMES) ;	
	for(count=0;count<numGpsTimeLinks;count++) {
	  tempString=getFirstLink(wdGps);
	  sprintf(currentFilename,"%s/%s",GPSD_SUBTIME_LINK_DIR,tempString);
	  removeFile(currentFilename);
	  sprintf(currentFilename,"%s/%s",GPSD_SUBTIME_DIR,tempString);
	  removeFile(currentFilename);
	}
	return 0;	
    }

    if(loadGpsLinks>MAX_GPS_TIMES)
      loadGpsLinks=MAX_GPS_TIMES-10;
    
    for(count=0;count<loadGpsLinks;count++) {
      tempString=getFirstLink(wdGps);
      sprintf(currentFilename,"%s/%s",GPSD_SUBTIME_LINK_DIR,tempString);
      retVal=fillGpsStruct(&gpsArray[numGpsStored],currentFilename);	
      if(retVal==0) {
	/* Got One */
	numGpsStored++;
      }
      deleteGPSFiles(&gpsArray[numGpsStored-1]);
    }    
    
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
	    printf("Event %u\tUnix: %d.%d\tTURF:%u (%u) %u\n\tGPS %d.%u\n", 
		   theHeaderPtr->eventNumber,theHeaderPtr->unixTime, 
		   theHeaderPtr->unixTimeUs,theHeaderPtr->turfio.ppsNum,
		   (unsigned int)(theHeaderPtr->turfio.ppsNum)+turfPPSOffset,
		   theHeaderPtr->turfio.trigTime, 
		   gpsArray[count].unixTime,gpsArray[count].subTime); 
	}
	if(printToScreen && verbosity>=0)
	    printf("Event %d\tUnix: %f\tTurf: %f\tGps: %f\n",theHeaderPtr->eventNumber,
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
	    newGpsSubTime=gpsArray[count].subTime;
	    if(theHeaderPtr->unixTime>gpsArray[count].unixTime)
		newGpsSubTime-=(int)(theHeaderPtr->unixTime-gpsArray[count].unixTime);
	    else if(theHeaderPtr->unixTime<gpsArray[count].unixTime)
		newGpsSubTime+=(int)(gpsArray[count].unixTime-theHeaderPtr->unixTime);
				      

//	    theHeaderPtr->unixTime=gpsArray[count].unixTime;    
//	    theHeaderPtr->gpsSubTime=gpsArray[count].subTime;
	    theHeaderPtr->gpsSubTime=newGpsSubTime;
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
	fprintf(timeFile,"5 %d %d\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
    
//    /* Move ev_ file first */
//    sprintf(theFilename,"%s/ev_%d.dat",ACQD_EVENT_DIR,
//	    theHeaderPtr->eventNumber);
//    testfp=fopen(theFilename,"rb");
//    if(testfp) {
//	fclose(testfp);
//	if(printToScreen && verbosity) printf("Moving %s\n",theFilename);
//	retVal=moveFile(theFilename,eventdEventDir);
//    }


		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"6 %d %d\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
    /* Should probably do something with retVal */
       
    sprintf(theFilename,"%s/hd_%d.dat",EVENTD_EVENT_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity) printf("Writing %s\n",theFilename);
    retVal=writeHeader(theHeaderPtr,theFilename);

		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"7 %d %d\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

    /* Make links, not sure what to do with return value here */
    retVal=makeLink(theFilename,EVENTD_EVENT_LINK_DIR);
    

		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"8 %d %d\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

    /* Delete previous hd_ file */
    sprintf(theFilename,"%s/hd_%d.dat",ACQD_EVENT_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity>2) printf("Deleting %s\n",theFilename);
    retVal=removeFile(theFilename);


		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"9 %d %d\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
   
    /* And the link */
    sprintf(theFilename,"%s/hd_%d.dat",ACQD_EVENT_LINK_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity>2) printf("Deleting %s\n",theFilename);
    retVal=removeFile(theFilename);

		 
#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"10 %d %d\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
    
    return retVal;
}

int getCalibStatus(int unixTime)
/* Checks calibLinkDir and gets current calibration status 
   Currently implemented options are on or off */
{
    static CalibStruct_t calibArray[MAX_CALIB_TIMES];
    static int numStored=0;
    char *tempString=0;
    int numCalibLinks=0;
    int count,retVal=0;
    char currentFilename[FILENAME_MAX];
    char otherFilename[FILENAME_MAX];
    int currentStatus=0, gotStatus=0;

    if(wdCalib==0) {
      wdCalib=setupLinkWatchDir(CALIBD_STATUS_LINK_DIR);
      if(wdCalib<=0) {	
	fprintf(stderr,"Unable to watch %s\n",CALIBD_STATUS_LINK_DIR);
	syslog(LOG_ERR,"Unable to watch %s\n",CALIBD_STATUS_LINK_DIR);
	exit(0);
      }
      numCalibLinks=getNumLinks(wdCalib);    
    }
    

    retVal=checkLinkDirs(0,1); //Will have to update to sue a smaller timeout
    if(retVal || numCalibLinks)
      numCalibLinks=getNumLinks(wdCalib);

    if(printToScreen && verbosity)
	fprintf(stderr,"There are %d calibs stored\n",numStored);

    if(numCalibLinks>1 || (numCalibLinks>0 && numStored==0)) {
	/* Read out calib status, and delete all but the most recent link */
	for(count=0;count<numCalibLinks;count++) {
	  tempString=getFirstLink(wdCalib);
	  sprintf(currentFilename,"%s/%s",CALIBD_STATUS_LINK_DIR,tempString);
	  sprintf(otherFilename,"%s/%s",CALIBD_STATUS_DIR,tempString);
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
	    //Not sure what's going on here may have to revisit,
	    //of course I might be doing the right thing
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

    if(gotStatus) return currentStatus;
    return -1;
    
}

int compareTimes(AnitaEventHeader_t *theHeaderPtr, GpsSubTime_t *theGpsPtr, int forceReset) 
{

    static unsigned int lastUnixTime=0;
    static unsigned int lastPPSNum=0;
    static unsigned int lastTrigTime=0;
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
	printf("Old:\t%u\t%u\t%u\n",lastUnixTime,lastTrigTime,lastPPSNum);
	printf("New:\t%d\t%u\t%u\n",theHeaderPtr->unixTime,
	       theHeaderPtr->turfio.trigTime,theHeaderPtr->turfio.ppsNum);
	
	printf("Raw:\t%d\t%d\n",theHeaderPtr->unixTime,theGpsPtr->unixTime);
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

    sprintf(theFilename,"%s/gps_%u_%u.dat",GPSD_SUBTIME_LINK_DIR,
	    theGpsPtr->unixTime,theGpsPtr->subTime);
    if(printToScreen && verbosity>2) 
	printf("Deleting: %s\n",theFilename);
    retVal=removeFile(theFilename);
    sprintf(theFilename,"%s/gps_%u_%u.dat",GPSD_SUBTIME_DIR,
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
    status = configLoad ("Eventd.config","eventd") ;
    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	tryToMatchGps=kvpGetInt("tryToMatchGps",1);

	   
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
//    printf("gps1 %u \t gps 2 %u\n",gps1->unixTime,gps2->unixTime);

    if(gps1->unixTime<gps2->unixTime) return -1;
    else if(gps1->unixTime>gps2->unixTime) return 1;

    if(gps1->subTime<gps2->subTime) return -1;
    else if(gps1->subTime>gps2->subTime) return 1;
    
    return 0;

}

void handleBadSigs(int sig)
{
    static int firstTime=1;
    if(firstTime) {
	clearGpsDir();
	firstTime=0;
    }	
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
    unlink(EVENTD_PID_FILE);
    syslog(LOG_INFO,"Eventd terminating");    
    exit(0);
}


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(EVENTD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,EVENTD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(EVENTD_PID_FILE);
  return 0;
}
