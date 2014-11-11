/*! \file Prioritizerd.c
  \brief The Prioritizerd program that creates Event objects 
  
  Reads the event objects written by Eventd, assigns a priority to each event based on the likelihood that it is an interesting event. Events with the highest priority will be transmitted to the ground first.
  March 2005 rjn@mps.ohio-state.edu
*/
#include <sys/time.h>
#include <signal.h>
#include <libgen.h> //For Mac OS X

//Flightsoft Includes
#include "Prioritizerd.h"
//#include "AnitaInstrument.h"
//#include "GenerateScore.h"
//#include "Filters.h"
#include "pedestalLib/pedestalLib.h"
#include "linkWatchLib/linkWatchLib.h"
#include "utilLib/utilLib.h"

/* Ben's GPU things */
#include "openCLwrapper.h"
#include "anitaTimingCalib.h"
#include "anitaGpu.h"
#include "tstamp.h"

//#define TIME_DEBUG 1
//#define WRITE_DEBUG_FILE

void wasteTime(AnitaEventBody_t *bdPtr);
int readConfig();
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);

int panicQueueLength=5000;

//Global Control Variables
int printToScreen=0;
int verbosity=0;
float hornThresh=0;
int hornDiscWidth=0;
int hornSectorWidth=0;
float coneThresh=0;
int coneDiscWidth=0;
int holdoff=0;
int delay=0;
int hornGuardOffset=30;
int hornGuardWidth=20;
int hornGuardThresh=50;
int coneGuardOffset=30;
int coneGuardWidth=20;
int coneGuardThresh=50;
int FFTPeakMaxA=0;
int FFTPeakMaxB=0;
int FFTPeakWindowL=0;
int FFTPeakWindowR=0;
int FFTMaxChannels=0;
int RMSMax=0;
int RMSevents=0;
int WindowCut=400;
int BeginWindow=0;
int EndWindow=0;
int MethodMask=0;
int NuCut=0;

//parameters for the quick cut on mid/early RMS for priority 7
int LowRMSChan=5;
int MidRMSChan=50;
int HighRMSChan=165;
int CutRMS=130;

int priorityPPS1=2;
int priorityPPS2=3;

/* NUM_EVENTS is defined in the imaginatively named myInterferometryConstants.h */
AnitaEventHeader_t theHeader[NUM_EVENTS];
PedSubbedEventBody_t pedSubBody[NUM_EVENTS];

int main (int argc, char *argv[])
{
  int retVal,count;
  int lastEventNumber=0;
  char linkFilename[FILENAME_MAX];
  char hdFilename[FILENAME_MAX];
  char bodyFilename[NUM_EVENTS][FILENAME_MAX];
  char telemHdFilename[FILENAME_MAX];
  char archiveHdFilename[FILENAME_MAX];
  char archiveBodyFilename[FILENAME_MAX];

  /* Config file thingies */
  int doingEvent=0;

  /* Directory reading things */
  int wd=0;
  char *tempString;
  int numEventLinks;
    
  /* Log stuff */
  char *progName=basename(argv[0]);

  retVal=sortOutPidFile(progName);
  if(retVal<0)
    return retVal;

#ifdef WRITE_DEBUG_FILE
  FILE *debugFile = fopen("/tmp/priDebugFile.txt","w");
  if(!debugFile) {
    fprintf(stderr,"Couldn't open debug file\n");
    exit(0);
  }
#endif
       

  /* Set signal handlers */
  signal(SIGUSR1, sigUsr1Handler);
  signal(SIGUSR2, sigUsr2Handler);
  signal(SIGTERM, handleBadSigs);
  signal(SIGINT, handleBadSigs);
  signal(SIGSEGV, handleBadSigs);
    
  /* Setup log */
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
  makeDirectories(HEADER_TELEM_LINK_DIR);
  makeDirectories(HEADER_TELEM_DIR);
  makeDirectories(EVENTD_EVENT_LINK_DIR);
  makeDirectories(PRIORITIZERD_EVENT_LINK_DIR);
  makeDirectories(ACQD_EVENT_LINK_DIR);

 
  retVal=0;
  /* Main event getting loop. */


  if(wd==0) {
    //First time need to prep the watcher directory
    wd=setupLinkWatchDir(EVENTD_EVENT_LINK_DIR);
    if(wd<=0) {
      fprintf(stderr,"Unable to watch %s\n",EVENTD_EVENT_LINK_DIR);
      syslog(LOG_ERR,"Unable to watch %s\n",EVENTD_EVENT_LINK_DIR);
      exit(0);
    }
    numEventLinks=getNumLinks(wd);
  }


  /* 
     This function  mallocs at some global pointers
     so needs to be outside any kind of loop.
  */
  prepareGpuThings();
  prepareTimingCalibThings();



  do {
    if(printToScreen) printf("Initalizing Prioritizerd\n");
    retVal=readConfig();
    if(retVal<0) {
      syslog(LOG_ERR,"Problem reading Prioritizerd.config");
      printf("Problem reading Prioritizerd.config\n");
      exit(1);
    }
    currentState=PROG_STATE_RUN;
    while(currentState==PROG_STATE_RUN) {

      //	usleep(1);
      retVal=checkLinkDirs(1,0);
      if(retVal || numEventLinks)
	numEventLinks=getNumLinks(wd);
	   
      if(numEventLinks>=panicQueueLength) {
	  syslog(LOG_INFO,"Prioritizerd is getting behind (%d events in inbox), will have some prioritity 7 events",numEventLinks);
      }


      printf("Prioritzerd is running and sees %d event links\n", numEventLinks);
    
      //      if(!numEventLinks) {
      /* if(numEventLinks < NUM_EVENTS) { */
      /* 	usleep(1000); */
      /* 	continue; */
      /* } */

      if(!numEventLinks){
	usleep(1000);
	continue;
      }

      /* Now read events in whenever I get them */
      while(count<NUM_EVENTS && currentState==PROG_STATE_RUN){
	tempString=getFirstLink(wd);
	printf("tempString = %s\n", tempString);
	//	printf("%s\n",eventLinkList[count]->d_name); 
	sscanf(tempString,"hd_%d.dat",&doingEvent);
	if(lastEventNumber>0 && doingEvent!=lastEventNumber+1) {
	    syslog(LOG_INFO,"Non-sequential event numbers %d and %d\n",
		   lastEventNumber,doingEvent);
	}
	lastEventNumber=doingEvent;

	sprintf(linkFilename,"%s/%s",EVENTD_EVENT_LINK_DIR,
		tempString);
	sprintf(hdFilename,"%s/hd_%d.dat",EVENTD_EVENT_DIR,
		doingEvent);

	//RJN 4th June 2008
	//Switch to Acqd writing psev files
	sprintf(bodyFilename[count],"%s/psev_%d.dat",ACQD_EVENT_DIR,
		doingEvent);

	retVal=fillPedSubbedBody(&pedSubBody[count],bodyFilename[count]);
	retVal=fillHeader(&theHeader[count],hdFilename);	

	if(numEventLinks<panicQueueLength){
	  double* finalVolts[ACTIVE_SURFS*CHANNELS_PER_SURF];
	  doTimingCalibration(count, theHeader[count], pedSubBody[count], finalVolts);
	  addEventToGpuQueue(count, finalVolts, theHeader[count]);
	  int chanInd=0;
	  for(chanInd=0; chanInd<ACTIVE_SURFS*CHANNELS_PER_SURF; chanInd++){
	    free(finalVolts[chanInd]);
	  }
	  count++;
	}
	else{
	  theHeader[count].priority=7;
	}
      }

      printf("count = %d, should be %d if program in normal state", count, NUM_EVENTS);

      /* Now use GPU to determine priority, send in arrays of length count... */
      mainGpuLoop(count, theHeader);      
      /* mainGpuLoop(count, pedSubBody, theHeader); */

      for(count=0;count<NUM_EVENTS;count++) {

	// handle queue forcing of PPS here
	int pri=theHeader[count].priority&0xf;
	if((theHeader[count].turfio.trigType&0x2) && (priorityPPS1>=0 && priorityPPS1<=9))
	  pri=priorityPPS1;
	if((theHeader[count].turfio.trigType&0x4) && (priorityPPS2>=0 && priorityPPS2<=9))
	  pri=priorityPPS2;
	if(pri<0 || pri>9) pri=9;
	theHeader[count].priority=(16*theHeader[count].priority)+pri;

	
	//Now Fill Generic Header and calculate checksum
	fillGenericHeader(&theHeader[count],theHeader[count].gHdr.code,sizeof(AnitaEventHeader_t));
  
	     
	//Rename body and write header for Archived
	sprintf(archiveBodyFilename,"%s/psev_%u.dat",PRIORITIZERD_EVENT_DIR,
		theHeader[count].eventNumber);
	if(rename(bodyFilename[count],archiveBodyFilename)==-1)
	  {
	    syslog(LOG_ERR,"Error moving file %s -- %s",archiveBodyFilename,
		   strerror(errno));
	  }

	sprintf(archiveHdFilename,"%s/hd_%u.dat",PRIORITIZERD_EVENT_DIR,
		theHeader[count].eventNumber);
	writeStruct(&theHeader[count],archiveHdFilename,sizeof(AnitaEventHeader_t));

	makeLink(archiveHdFilename,PRIORITIZERD_EVENT_LINK_DIR);
    
	//Write Header and make Link for telemetry 	    
	sprintf(telemHdFilename,"%s/hd_%d.dat",HEADER_TELEM_DIR,
		doingEvent);
	retVal=writeStruct(&theHeader[count],telemHdFilename,sizeof(AnitaEventHeader_t));
	makeLink(telemHdFilename,HEADER_TELEM_LINK_DIR);
	    
	/* Delete input */
	sprintf(linkFilename,"%s/hd_%d.dat",EVENTD_EVENT_LINK_DIR,
		theHeader[count].eventNumber);		 
	sprintf(hdFilename,"%s/hd_%d.dat",EVENTD_EVENT_DIR,
		theHeader[count].eventNumber);
	removeFile(linkFilename);
	removeFile(hdFilename);


      }
      
	
    }
  } while(currentState==PROG_STATE_INIT); 
  unlink(PRIORITIZERD_PID_FILE);



 /*
   I believe this gets done anyway as the program terminates...
   But we want to be good programming citizens.
   So let's free all CPU and GPU mallocs, close output files etc, etc...
 */
  tidyUpGpuThings();
  tidyUpTimingCalibThings();

  return 0;
}

void wasteTime(AnitaEventBody_t *bdPtr) {
  int chan,samp,loop;
  for(loop=0;loop<10;loop++) {       
    for(chan=0;chan<NUM_DIGITZED_CHANNELS;chan++) {
      for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
	bdPtr->channel[chan].data[samp]+=10*(-1+2*(loop%1));
      }
    }	
  }
}


int readConfig()
// Load Prioritizerd config stuff
{
  // Config file thingies
  //    KvpErrorCode kvpStatus=0;
  int status=0;
  char* eString ;
  kvpReset();
  status = configLoad ("Prioritizerd.config","output") ;
  if(status == CONFIG_E_OK) {
    printToScreen=kvpGetInt("printToScreen",-1);
    verbosity=kvpGetInt("verbosity",-1);
    if(printToScreen<0) {
      syslog(LOG_WARNING,
	     "Couldn't fetch printToScreen, defaulting to zero");
      printToScreen=0;
    }
  }
  else {
    eString=configErrorString (status) ;
    syslog(LOG_ERR,"Error reading LOSd.config: %s\n",eString);
    fprintf(stderr,"Error reading LOSd.config: %s\n",eString);
  }
  kvpReset();
  status = configLoad ("Prioritizerd.config","prioritizerd");
  if(status == CONFIG_E_OK) {
      panicQueueLength=kvpGetInt("panicQueueLength",5000);
    hornThresh=kvpGetFloat("hornThresh",250);
    coneThresh=kvpGetFloat("coneThresh",250);
    hornDiscWidth=kvpGetInt("hornDiscWidth",16);
    coneDiscWidth=kvpGetInt("coneDiscWidth",16);
    hornSectorWidth=kvpGetInt("hornSectorWidth",3);
    holdoff=kvpGetInt("holdoff",39);
    delay=kvpGetInt("delay",8);
    hornGuardOffset=kvpGetInt("hornGuardOffset",30);
    hornGuardWidth=kvpGetInt("hornGuardWidth",20);
    hornGuardThresh=kvpGetInt("hornGuardThresh",50);
    coneGuardOffset=kvpGetInt("coneGuardOffset",30);
    coneGuardWidth=kvpGetInt("coneGuardWidth",20);
    coneGuardThresh=kvpGetInt("coneGuardThresh",50);
    FFTPeakMaxA=kvpGetInt("FFTPeakMaxA",2500);
    FFTPeakMaxB=kvpGetInt("FFTPeakMaxB",2500);
    FFTPeakWindowL=kvpGetInt("FFTPeakWindowL",0);
    FFTPeakWindowR=kvpGetInt("FFTPeakWindowR",0);
    FFTMaxChannels=kvpGetInt("FFTMaxChannels",0);
    RMSMax=kvpGetInt("RMSMax",200);
    RMSevents=kvpGetInt("RMSevents",1000);
    WindowCut=kvpGetInt("WindowCut",400);
    BeginWindow=kvpGetInt("BeginWindow",100);
    EndWindow=kvpGetInt("EndWindow",100);
    MethodMask=kvpGetInt("MethodMask",0x0867);
    NuCut=kvpGetInt("NuCut",100);
    LowRMSChan=kvpGetInt("LowRMSChan",5);
    MidRMSChan=kvpGetInt("MidRMSChan",50);
    HighRMSChan=kvpGetInt("HighRMSChan",165);
    CutRMS=kvpGetInt("CutRMS",130);
  }
  else {
    eString=configErrorString (status) ;
    syslog(LOG_ERR,"Error reading Prioritizerd.config: %s\n",eString);
    fprintf(stderr,"Error reading Prioritizerd.config: %s\n",eString);
  }
  kvpReset();
  status = configLoad ("Archived.config","archived");
  if(status == CONFIG_E_OK) {
    priorityPPS1=kvpGetInt("priorityPPS1",3);
    priorityPPS2=kvpGetInt("priorityPPS2",2);
  }
  else {
    eString=configErrorString (status) ;
    syslog(LOG_ERR,"Error reading Prioritizerd.config: %s\n",eString);
    fprintf(stderr,"Error reading Prioritizerd.config: %s\n",eString);
  }
  return status;
}

void handleBadSigs(int sig)
{
  syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
  unlink(PRIORITIZERD_PID_FILE);
  syslog(LOG_INFO,"Prioritizerd terminating");
  exit(0);
}


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(PRIORITIZERD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,PRIORITIZERD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(PRIORITIZERD_PID_FILE);
  return 0;
}
