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

#define GPS_CLOCK_TICKS 1e7
#define MAX_GPS_TIMES 500
#define MAX_CALIB_TIMES 2000
#define EVENT_TIMEOUT 5

#define TIME_MATCH 0.005 //seconds
#define DEFAULT_C3PO 133000000

int writeHeaderAndMakeLink(AnitaEventHeader_t *theHeaderPtr);
int getCalibStatus(int unixTime);
int fillGpsTime(AnitaEventHeader_t *theHeaderPtr);
int deleteGPSFiles(GpsSubTime_t *theGpsPtr);
void clearGpsDir();
int compareTimes(AnitaEventHeader_t *theHeaderPtr, GpsSubTime_t *theGpsPtr, int forceReset);
int readConfigFile();
int compareGpsTimes(GpsSubTime_t *ptr1, GpsSubTime_t *ptr2);
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);

/* Directories and gubbins */
int printToScreen=0;
int verbosity=0;
int tryToMatchGps=0;
int g12Offset=0; //in 100ns TTT units
int adu52Offset=0; //in 100ns TTT units


AnitaEventHeader_t theAcqdEventHeader;

// Global Variables
GpsSubTime_t g12Array[MAX_GPS_TIMES]; 
GpsSubTime_t adu51Array[MAX_GPS_TIMES]; 
GpsSubTime_t adu52Array[MAX_GPS_TIMES]; 
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
	  //	  if(retVal || numEventLinks)
	  numEventLinks=getNumLinks(wdEvent);
	  /* 	printf("Got %d event links\n",numEventLinks); */
	  if(numEventLinks<1) {
	    //	    usleep(1000);
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
		//Do I need this here?
		removeFile(currentFilename);	
	      }		    
	      continue;
	    }	
	    
	    int tryCount=0;
	    do {
	      //printf("Here %u (filledSubTime %d)\n",theAcqdEventHeader.eventNumber,filledSubTime);
	      if(tryToMatchGps) {
		filledSubTime=fillGpsTime(&theAcqdEventHeader);
	      }
	      else {
		//Not trying to fill GPS sub times
		filledSubTime=0;
		clearGpsDir();
		break;
	      }

	      if(!filledSubTime) {
		if((time(NULL)-theAcqdEventHeader.unixTime)<EVENT_TIMEOUT) 
		  sleep(1);
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
	    
	    theAcqdEventHeader.calibStatus=getCalibStatus(theAcqdEventHeader.unixTime);
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
    struct dirent **gpsSubTimeLinkList;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int numGpsTimeLinks=getListofLinks(GPSD_SUBTIME_LINK_DIR,&gpsSubTimeLinkList);

    for(count=0;count<numGpsTimeLinks;count++) {
      sprintf(currentFilename,"%s/%s",GPSD_SUBTIME_LINK_DIR,
	      gpsSubTimeLinkList[count]->d_name);   
      sprintf(currentLinkname,"%s/%s",GPSD_SUBTIME_DIR,
	      gpsSubTimeLinkList[count]->d_name);     
      removeFile(currentFilename);
      removeFile(currentLinkname);      
    }


    /* Free up the space used by dir queries */
    for(count=0;count<numGpsTimeLinks;count++)
      free(gpsSubTimeLinkList[count]);
    free(gpsSubTimeLinkList);

}

int fillGpsTime(AnitaEventHeader_t *theHeaderPtr)
{
    int retVal,count;
    char *tempString=0;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int haveMatch=0;
//May implement a dynamic monitoring of the time offset
    int newGpsSubTime;
    static unsigned short lastPPSNum=0;
    static unsigned int turfPPSOffset=0;
    int numGpsTimeLinks=0;
    static int numG12Stored=0;
    static int numAdu51Stored=0;
    static int numAdu52Stored=0;
    static int eventsThisEpoch=0;
    static int matchesThisEpoch=0;
    static int lastEventNumber=0;
    float fracUnix,fracTurf,fracGps;
    GpsSubTime_t tempGps;
    int numGpsStored=numG12Stored+numAdu51Stored+numAdu52Stored;
    
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
    
    if(lastPPSNum>theHeaderPtr->turfio.ppsNum || theHeaderPtr->turfio.ppsNum==0) {
	turfPPSOffset=0;
	eventsThisEpoch=0;
	matchesThisEpoch=0;
    }
    //    printf("eventsThisEpoch %d, matchesThisEpoch %d\n",eventsThisEpoch,matchesThisEpoch);
    if(eventsThisEpoch>10) {
      if((((float)matchesThisEpoch)/((float)eventsThisEpoch))<0.5) {
	//Try and reset the offset
	turfPPSOffset=0;
	eventsThisEpoch=0;
	matchesThisEpoch=0;
      }
    }

    if(theHeaderPtr->eventNumber!=lastEventNumber)
      eventsThisEpoch++;
    lastPPSNum=theHeaderPtr->turfio.ppsNum;
    lastEventNumber=theHeaderPtr->eventNumber;
    
    // GPS subTime stuff
    retVal=checkLinkDirs(0,1);
    //    if(retVal || numGpsTimeLinks)
    numGpsTimeLinks=getNumLinks(wdGps);
    int loadGpsLinks=numGpsTimeLinks;
    
    if(verbosity>2 && printToScreen) 	
      fprintf(stderr,"There are %d gps links.\n",numGpsTimeLinks);
    
    /* need to do something if we ever have more 
       than MAX_GPS_TIMES subTimes.*/
    if(numGpsStored+numGpsTimeLinks>MAX_GPS_TIMES) {
      //Step one is to ditch the currently stored times
      syslog(LOG_ERR,"GPS overload junking %d times\n",numGpsStored);
      fprintf(stderr,"GPS overload junking %d times\n",numGpsStored);
      numG12Stored=0;
      numAdu51Stored=0;
      numAdu52Stored=0;
      bzero(g12Array, sizeof(GpsSubTime_t)*MAX_GPS_TIMES) ;	
      bzero(adu51Array, sizeof(GpsSubTime_t)*MAX_GPS_TIMES) ;	
      bzero(adu52Array, sizeof(GpsSubTime_t)*MAX_GPS_TIMES) ;	
    }

    if(numGpsTimeLinks>MAX_GPS_TIMES) {
      //If we have too many in the directory delete all but 50
      for(count=0;count<numGpsTimeLinks-50;count++) {
	tempString=getFirstLink(wdGps);
	sprintf(currentLinkname,"%s/%s",GPSD_SUBTIME_LINK_DIR,tempString);
	sprintf(currentFilename,"%s/%s",GPSD_SUBTIME_DIR,tempString);
	removeFile(currentFilename);
	removeFile(currentLinkname);
	loadGpsLinks--;
      }
    }
    
    //Should never be possible
    if(loadGpsLinks>MAX_GPS_TIMES)
      loadGpsLinks=MAX_GPS_TIMES-10;
    
    for(count=0;count<loadGpsLinks;count++) {
      tempString=getFirstLink(wdGps);
      sprintf(currentLinkname,"%s/%s",GPSD_SUBTIME_LINK_DIR,tempString);
      sprintf(currentFilename,"%s/%s",GPSD_SUBTIME_DIR,tempString);
      retVal=fillGpsStruct(&tempGps,currentFilename);	
      if(retVal==0) {
	/* Got One */
	switch(tempGps.fromAdu5) {
	case 0:
	  //G12
	  if((g12Offset>0 && tempGps.subTime>g12Offset) || 
	     (g12Offset<0 && tempGps.subTime<g12Offset+GPS_CLOCK_TICKS) ) {
	    tempGps.subTime-=g12Offset;
	  }
	  else if(g12Offset>0) {
	    tempGps.unixTime--;
	    tempGps.subTime+=GPS_CLOCK_TICKS-g12Offset;
	  }
	  else if(g12Offset<0) {
	    tempGps.unixTime++;
	    tempGps.subTime-=(GPS_CLOCK_TICKS+g12Offset);
	  }
	  g12Array[numG12Stored]=tempGps;
	  numG12Stored++;
	  break;
	case 1:
	  //ADU51
	  adu51Array[numAdu51Stored]=tempGps;
	  numAdu51Stored++;
	  break;
	case 2:
	  //ADU52
	  if((adu52Offset>0 && tempGps.subTime>adu52Offset) || 
	     (adu52Offset<0 && tempGps.subTime<adu52Offset+GPS_CLOCK_TICKS) ) {
	    tempGps.subTime-=adu52Offset;
	  }
	  else if(adu52Offset>0) {
	    tempGps.unixTime--;
	    tempGps.subTime+=GPS_CLOCK_TICKS-adu52Offset;
	  }
	  else if(adu52Offset<0) {
	    tempGps.unixTime++;
	    tempGps.subTime-=(GPS_CLOCK_TICKS+adu52Offset);
	  }
	  adu52Array[numAdu52Stored]=tempGps;
	  numAdu52Stored++;
	  break;
	default:
	  //don't know don't care
	  break;
	}
      }
      removeFile(currentFilename);
      removeFile(currentLinkname);
    }    
    

    numGpsStored=numG12Stored+numAdu51Stored+numAdu52Stored;
    if(printToScreen && verbosity>=1)
      fprintf(stderr,"There are %d gps times stored (G12 %d, Adu51 %d, Adu52 %d)\n",numGpsStored,numG12Stored,numAdu51Stored,numAdu52Stored);


    fracUnix=((float)theHeaderPtr->unixTimeUs)/1e6;
    fracTurf=((float)theHeaderPtr->turfio.trigTime)/((float)DEFAULT_C3PO);
    if(turfPPSOffset) {
      fracTurf+=(((float)(turfPPSOffset+theHeaderPtr->turfio.ppsNum))-
		 ((float)theHeaderPtr->unixTime));
    }

    int uptoG12=0;
    int uptoAdu51=0;
    int uptoAdu52=0;
    int comp1=0,comp2=0;
    for(count=0;count<numGpsStored;count++) {
      //First up find out which gps has next timestamp
      if(numG12Stored-uptoG12) {
	if(numAdu51Stored-uptoAdu51) {
	  comp1=compareGpsTimes(&g12Array[uptoG12],&adu51Array[uptoAdu51]);
	  if(numAdu52Stored-uptoAdu52) {	    
	    if(comp1<=0) {
	      //Between G12 and ADU52
	      comp2=compareGpsTimes(&g12Array[uptoG12],&adu52Array[uptoAdu52]);
	      if(comp2<=0) {
		//Use G12
		tempGps=g12Array[uptoG12];
		uptoG12++;
	      }
	      else {
		//Use ADU52
		tempGps=adu52Array[uptoAdu52];
		uptoAdu52++;
	      }	      
	    }
	    else {
	      //Between ADU51 and ADU52
	      comp2=compareGpsTimes(&adu51Array[uptoAdu51],&adu52Array[uptoAdu52]);
	      if(comp2<=0) {
		//Use ADU51
		tempGps=adu51Array[uptoAdu51];
		uptoAdu51++;
	      }
	      else {
		//Use ADU52
		tempGps=adu52Array[uptoAdu52];
		uptoAdu52++;
	      }
	    }
	  }
	  else {
	    //Between G12 and ADU51
	    if(comp1<=0) {
	      //Use G12
	      tempGps=g12Array[uptoG12];
	      uptoG12++;
	    }
	    else {
	      //Use ADU51
	      tempGps=adu51Array[uptoAdu51];
		uptoAdu51++;
	    }	      
	  }
	}
	else if(numAdu52Stored-uptoAdu52) {
	  //Between G12 and ADU52
	  comp2=compareGpsTimes(&g12Array[uptoAdu51],&adu52Array[uptoAdu52]);
	  if(comp2<=0) {
	    //Use G12
	    tempGps=g12Array[uptoG12];
	    uptoG12++;
	  }
	  else {
	    //Use ADU52
	    tempGps=adu52Array[uptoAdu52];
	    uptoAdu52++;
	  }
	}
	else {
	  //G12
	  tempGps=g12Array[uptoG12];
	  uptoG12++;
	}
      }            
      else if(numAdu51Stored-uptoAdu51) {
	//Between ADU51 and ADU52
	comp2=compareGpsTimes(&adu51Array[uptoAdu51],&adu52Array[uptoAdu52]);
	if(comp2<=0) {
	  //Use ADU51
	  tempGps=g12Array[uptoAdu51];
	  uptoAdu51++;
	}
	else {
	    //Use ADU52
	  tempGps=adu52Array[uptoAdu52];
	    uptoAdu52++;
	}
      }
      else if(numAdu52Stored-uptoAdu52) {
	//Must be ADU52
	tempGps=adu52Array[uptoAdu52];
	uptoAdu52++;
      }
	    

      if(printToScreen && verbosity>=1) {
	printf("uptoG12 %d, uptoAdu51 %d, uptoAdu52 %d\n",
	       uptoG12,uptoAdu51,uptoAdu52);
      }


      fracGps=((float)tempGps.subTime)/GPS_CLOCK_TICKS;
      fracGps+=(((float)tempGps.unixTime)
		-((float)theHeaderPtr->unixTime));
      if(printToScreen&&verbosity>=1) {
	printf("lastPPSNum %d, ppsNum %d\n",lastPPSNum,theHeaderPtr->turfio.ppsNum);
	printf("Event %u\tUnix: %d.%d\tTURF:%u (%u) %u\n\tGPS %d.%u\n", 
	       theHeaderPtr->eventNumber,theHeaderPtr->unixTime, 
	       theHeaderPtr->unixTimeUs,theHeaderPtr->turfio.ppsNum,
	       (unsigned int)(theHeaderPtr->turfio.ppsNum)+turfPPSOffset,
	       theHeaderPtr->turfio.trigTime, 
	       tempGps.unixTime,tempGps.subTime); 
	printf("Event %d\tUnix: %f\tTurf: %f\tGps: %f\n",theHeaderPtr->eventNumber,
	       fracUnix,fracTurf,fracGps);
      }
      
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
	matchesThisEpoch++;
	//We have a match
	//Need to set gpsSubTime and delete all previous GPS times
	if(turfPPSOffset==0)
	  turfPPSOffset=tempGps.unixTime-theHeaderPtr->turfio.ppsNum;	    
	newGpsSubTime=tempGps.subTime;
	if(theHeaderPtr->unixTime>tempGps.unixTime)
	  newGpsSubTime-=(int)(theHeaderPtr->unixTime-tempGps.unixTime);
	else if(theHeaderPtr->unixTime<tempGps.unixTime)
	  newGpsSubTime+=(int)(tempGps.unixTime-theHeaderPtr->unixTime);
	
	//	    theHeaderPtr->unixTime=tempGps.unixTime;    
	//	    theHeaderPtr->gpsSubTime=tempGps.subTime;
	theHeaderPtr->gpsSubTime=newGpsSubTime;

	//Now need to zero out all previous times
	if(printToScreen && verbosity>1) {
	  printf("uptoG12 %d \t numG12Stored %d\n",uptoG12,numG12Stored);
	  printf("uptoAdu51 %d \t numAdu51Stored %d\n",uptoAdu51,numAdu51Stored);
	  printf("uptoAdu52 %d \t numAdu52Stored %d\n",uptoAdu52,numAdu52Stored);
	}
	if(uptoG12) {
	  bzero(g12Array,(uptoG12)*sizeof(GpsSubTime_t));
	  numG12Stored-=(uptoG12);
	  if(numG12Stored>0) 
	    memmove(&g12Array[0],&g12Array[uptoG12],numG12Stored*sizeof(GpsSubTime_t));
	}
	if(uptoAdu51) {
	  bzero(adu51Array,(uptoAdu51)*sizeof(GpsSubTime_t));
	  numAdu51Stored-=(uptoAdu51);
	  if(numAdu51Stored>0) 
	    memmove(&adu51Array[0],&adu51Array[uptoAdu51],numAdu51Stored*sizeof(GpsSubTime_t));
	}
	if(uptoAdu52) {
	  bzero(adu52Array,(uptoAdu52)*sizeof(GpsSubTime_t));
	  numAdu52Stored-=(uptoAdu52);
	  if(numAdu52Stored>0) 
	    memmove(&adu52Array[0],&adu52Array[uptoAdu52],numAdu52Stored*sizeof(GpsSubTime_t));
	}		
	return 1;
      }	
    }
    return 0;
}



int writeHeaderAndMakeLink(AnitaEventHeader_t *theHeaderPtr)
{
    char theFilename[FILENAME_MAX];
    int retVal;
//    FILE *testfp;		     
//    /* Move ev_ file first */
//    sprintf(theFilename,"%s/ev_%d.dat",ACQD_EVENT_DIR,
//	    theHeaderPtr->eventNumber);
//    testfp=fopen(theFilename,"rb");
//    if(testfp) {
//	fclose(testfp);
//	if(printToScreen && verbosity) printf("Moving %s\n",theFilename);
//	retVal=moveFile(theFilename,eventdEventDir);
//    }


		
    /* Should probably do something with retVal */
       
    sprintf(theFilename,"%s/hd_%d.dat",EVENTD_EVENT_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity) printf("Writing %s\n",theFilename);
    retVal=writeHeader(theHeaderPtr,theFilename);

		 
    /* Make links, not sure what to do with return value here */
    retVal=makeLink(theFilename,EVENTD_EVENT_LINK_DIR);
    		
    /* Delete previous hd_ file */
    sprintf(theFilename,"%s/hd_%d.dat",ACQD_EVENT_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity>2) printf("Deleting %s\n",theFilename);
    retVal=removeFile(theFilename);
		   
    /* And the link */
    sprintf(theFilename,"%s/hd_%d.dat",ACQD_EVENT_LINK_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity>2) printf("Deleting %s\n",theFilename);
    retVal=removeFile(theFilename);
		     
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
	    if(count!=(numCalibLinks-1)) {
	      removeFile(currentFilename);
	      removeFile(otherFilename);
	    }
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
	       elements of the calibArray. */
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
	g12Offset=kvpGetInt("g12Offset",0);
	adu52Offset=kvpGetInt("adu52Offset",0);

	   
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


int compareGpsTimes(GpsSubTime_t *gps1, GpsSubTime_t *gps2) {
  //-1 means gps1 < gps2  , 0 is gps1==gps2 and 1 is gps1>gps2
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
