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
#include "pedestalLib/pedestalLib.h"
#include "linkWatchLib/linkWatchLib.h"
#include "utilLib/utilLib.h"

/* Ben's GPU things */
#include "openCLwrapper.h"
#include "anitaTimingCalib.h"
#include "anitaGpu.h"
#include "tstamp.h"

void wasteTime(AnitaEventBody_t *bdPtr);
int readConfig();
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);

void readInEvent(PedSubbedEventBody_t *psev, AnitaEventHeader_t* head, const char* dir, int eventNumber);
void readIn100Events(const char* psevFileName, PedSubbedEventBody_t *theBody, const char* headFileName, AnitaEventHeader_t* theHeader);

/* void readInTextFile(PedSubbedEventBody_t *psev, AnitaEventHeader_t* head); */

void panicWriteAllLinks(int wd, int panicPri);

int panicQueueLength=5000;

//Global Control Variables
int printToScreen=1;
int verbosity=0;

int priorityPPS1=2;
int priorityPPS2=3;

/* NUM_EVENTS is defined in the imaginatively named myInterferometryConstants.h */
AnitaEventHeader_t theHeader[NUM_EVENTS];
PedSubbedEventBody_t pedSubBody[NUM_EVENTS];

/* #ifdef DEBUG_MODE */
#define NUM_RUNS 1

int runs[NUM_RUNS] = {11659}; //11617}; //11166}; //11165}; //11163};//11164}; //11150}; //11151}; //11152}; //11153}; //11154}; //11155}; //11156}; //11157}; //11159}; //11160}; //11161}; //11162};
int firstFile[NUM_RUNS] = {68448000}; //48926900}; //48921700}; //48910800};//48916900}; //48810500}; //48816200}; //48827400}; //48827400}; //48834300}; //48840200}; //48852600}; //48860300}; //48866600}; //48872800}; //48879400}; //48885600}; //48892200}; //48898200}; //48905400};
int lastFile[NUM_RUNS] = {68450400}; //48931100}; //48926400}; //48916600};//48921400}; //48815700}; //48827000}; //48833700}; //48833700}; //48839000}; //48852300}; //48860000}; //48866300}; //48872300}; //48879000}; //48885100};//48891900}; //48897800}; //48905100}; //48910500};


int main (int argc, char *argv[]){

  int retVal; //,count;
  /* int lastEventNumber=0; */
  /* char linkFilename[FILENAME_MAX]; */
  /* char hdFilename[FILENAME_MAX]; */
  /* char bodyFilename[NUM_EVENTS][FILENAME_MAX]; */
  /* char telemHdFilename[FILENAME_MAX]; */
  /* char archiveHdFilename[FILENAME_MAX]; */
  /* char archiveBodyFilename[FILENAME_MAX]; */

  /* Config file thingies */
  /* int doingEvent=0; */

  /* Directory reading things */
  int wd=0;
  /* char *tempString; */
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
    if(printToScreen) printf("Initalizing fakePrioritizerd\n");
    retVal=readConfig();
    if(retVal<0) {
      syslog(LOG_ERR,"Problem reading Prioritizerd.config");
      printf("Problem reading Prioritizerd.config\n");
      exit(1);
    }
    currentState=PROG_STATE_RUN;

    /* This one is the main while loop */
    int runInd=0;
    for(runInd=0; runInd<NUM_RUNS; runInd++) {
      int run = runs[runInd];

      const char* baseDir = "/home/anita/anitaStorage/antarctica14/raw";
      int eventFileNum=0;
      for(eventFileNum=firstFile[runInd]; eventFileNum<=lastFile[runInd]; eventFileNum+=100){
	//    while(currentState==PROG_STATE_RUN) {

	//	usleep(1);
	retVal=checkLinkDirs(1,0);
	if(retVal || numEventLinks) numEventLinks=getNumLinks(wd);

	if(numEventLinks>=panicQueueLength) {
	  syslog(LOG_INFO,"Prioritizerd is getting behind (%d events in inbox), will have some prioritity 7 events",numEventLinks);
	}

	/* if(!numEventLinks){ */
	/* 	usleep(1000); */
	/* 	continue; */
	/* } */

	/* Read data into program memory */
	int eventDir1 = eventFileNum/1000000;
	eventDir1 *= 1000000;
	int eventDir2 = eventFileNum/10000;
	eventDir2 *= 10000;
      
	char dir[FILENAME_MAX];
	sprintf(dir, "%s/run%d/event/ev%d/ev%d", baseDir, run, eventDir1, eventDir2);

	char fileName1[FILENAME_MAX];
	sprintf(fileName1, "%s/psev_%d.dat.gz", dir, eventFileNum);
	char fileName2[FILENAME_MAX];	
	sprintf(fileName2, "%s/hd_%d.dat.gz", dir, eventFileNum);

	printf("%s\n", fileName1);
	readIn100Events(fileName1, pedSubBody, fileName2, theHeader);
	/* readInTextFile(pedSubBody, theHeader); */


	int eventsReadIn = 0;
	while(eventsReadIn<100 && currentState==PROG_STATE_RUN){

	  /* numEventLinks=getNumLinks(wd); */
	  /* tempString=getFirstLink(wd); */
	  /* if(numEventLinks==0) break; */
	  /* if(tempString==NULL) continue; */
	  /* //	printf("tempString = %s\n", tempString); */
	  /* //	printf("%s\n",eventLinkList[eventsReadIn]->d_name);  */
	  /* sscanf(tempString,"hd_%d.dat",&doingEvent); */
	  /* if(lastEventNumber>0 && doingEvent!=lastEventNumber+1) { */
	  /*   syslog(LOG_INFO,"Non-sequential event numbers %d and %d\n", lastEventNumber, doingEvent); */
	  /* } */
	  /* lastEventNumber=doingEvent; */

	  /* sprintf(linkFilename, "%s/%s", EVENTD_EVENT_LINK_DIR, tempString); */
	  /* sprintf(hdFilename, "%s/hd_%d.dat", EVENTD_EVENT_DIR, doingEvent); */

	  /* //RJN 4th June 2008 */
	  /* //Switch to Acqd writing psev files */
	  /* sprintf(bodyFilename[eventsReadIn],"%s/psev_%d.dat", ACQD_EVENT_DIR, doingEvent); */

	  /* retVal=fillHeader(&theHeader[eventsReadIn],hdFilename);	 */

	  if(numEventLinks < panicQueueLength){
	    /* Then we add to GPU queue... */
	    /* retVal=fillPedSubbedBody(&pedSubBody,bodyFilename[eventsReadIn]); */
	    double* finalVolts[ACTIVE_SURFS*CHANNELS_PER_SURF];
	    printf("fakePrioritizer reads event %d with priority %d\n",theHeader[eventsReadIn].eventNumber,  theHeader[eventsReadIn].priority&0xf);
	    doTimingCalibration(eventsReadIn, theHeader[eventsReadIn], pedSubBody[eventsReadIn], finalVolts);
	    addEventToGpuQueue(eventsReadIn, finalVolts, theHeader[eventsReadIn]);
	    int chanInd=0;
	    for(chanInd=0; chanInd<ACTIVE_SURFS*CHANNELS_PER_SURF; chanInd++){
	      free(finalVolts[chanInd]);
	    }
	    eventsReadIn++;
	  }
	  else{/* Panic! Write all header files to archived directory with priority 7! */
	    panicWriteAllLinks(wd, 7);
	  }
	}
	/* exit(-1); */

	printf("eventsReadIn = %d, max is %d.\n", eventsReadIn, NUM_EVENTS);

	/* Now use GPU to determine priority, send in arrays of length eventsReadIn... */
	if(eventsReadIn>0){
	  mainGpuLoop(eventsReadIn, theHeader);
	}

	int count = 0;
	for(count=0;count<eventsReadIn;count++) {

	  // handle queue forcing of PPS here
	  int pri=theHeader[count].priority&0xf;
	  if((theHeader[count].turfio.trigType&0x2) && (priorityPPS1>=0 && priorityPPS1<=9))
	    pri=priorityPPS1;
	  if((theHeader[count].turfio.trigType&0x4) && (priorityPPS2>=0 && priorityPPS2<=9))
	    pri=priorityPPS2;
	  if(pri<0 || pri>9) pri=9;
	  theHeader[count].priority=(16*theHeader[count].priority)+pri;

	  printf("eventNumber = %u, priority = %d\n", theHeader[count].eventNumber, theHeader[count].priority&0xf);

	  //Now Fill Generic Header and calculate checksum
	  fillGenericHeader(&theHeader[count],theHeader[count].gHdr.code,sizeof(AnitaEventHeader_t));

	  /* //Rename body and write header for Archived */
	  /* sprintf(archiveBodyFilename,"%s/psev_%u.dat",PRIORITIZERD_EVENT_DIR, */
	  /* 	theHeader[count].eventNumber); */
	  /* if(rename(bodyFilename[count],archiveBodyFilename)==-1) */
	  /*   { */
	  /*     syslog(LOG_ERR,"Error moving file %s -- %s",archiveBodyFilename, */
	  /* 	   strerror(errno)); */
	  /*   } */

	  /* sprintf(archiveHdFilename,"%s/hd_%u.dat",PRIORITIZERD_EVENT_DIR, */
	  /* 	theHeader[count].eventNumber); */
	  /* writeStruct(&theHeader[count],archiveHdFilename,sizeof(AnitaEventHeader_t)); */

	  /* makeLink(archiveHdFilename,PRIORITIZERD_EVENT_LINK_DIR); */
    
	  /* //Write Header and make Link for telemetry */
	  /* sprintf(telemHdFilename,"%s/hd_%d.dat",HEADER_TELEM_DIR, */
	  /* 	theHeader[count].eventNumber); */
	  /* retVal=writeStruct(&theHeader[count],telemHdFilename,sizeof(AnitaEventHeader_t)); */
	  /* makeLink(telemHdFilename,HEADER_TELEM_LINK_DIR); */

	  /* /\* Delete input *\/ */
	  /* sprintf(linkFilename,"%s/hd_%d.dat",EVENTD_EVENT_LINK_DIR, */
	  /* 	theHeader[count].eventNumber); */
	  /* sprintf(hdFilename,"%s/hd_%d.dat",EVENTD_EVENT_DIR, */
	  /* 	theHeader[count].eventNumber); */
	  /* removeFile(linkFilename); */
	  /* removeFile(hdFilename); */

	}
	exit(0);
      }
    }
  } while(currentState==PROG_STATE_INIT && 0); 
  unlink(PRIORITIZERD_PID_FILE);


  tidyUpGpuThings();
  tidyUpTimingCalibThings();
 /* Close the file which contains the high level output information */


  return 0;
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

void panicWriteAllLinks(int wd, int panicPri){

  char linkFilename[FILENAME_MAX];
  char hdFilename[FILENAME_MAX];
  char bodyFilename[FILENAME_MAX];
  char telemHdFilename[FILENAME_MAX];
  char archiveHdFilename[FILENAME_MAX];
  char archiveBodyFilename[FILENAME_MAX];
  int numEventLinksAtPanic = getNumLinks(wd);
  syslog(LOG_WARNING,"Prioritizerd queue has reached panicQueueLength=%d!\n", panicQueueLength);
  syslog(LOG_WARNING, "Trying to recover by writing %d priority 7 events!\n", numEventLinksAtPanic);
  int doingEvent=0;
  int count=0;
  int retVal=0;
  char* tempString;
  int lastEventNumber=0;

  for(count=0; count<numEventLinksAtPanic; count++){
    int numEventLinks=getNumLinks(wd);
    tempString=getFirstLink(wd);
    if(numEventLinks==0) break;
    if(tempString==NULL) continue; /* Just to be safe */
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

    sprintf(bodyFilename,"%s/psev_%d.dat", ACQD_EVENT_DIR, doingEvent);

    AnitaEventHeader_t panicHeader;
    retVal=fillHeader(&panicHeader,hdFilename);
    panicHeader.priority = panicPri;

    int pri=panicHeader.priority&0xf;
    if((panicHeader.turfio.trigType&0x2) && (priorityPPS1>=0 && priorityPPS1<=9))
      pri=priorityPPS1;
    if((panicHeader.turfio.trigType&0x4) && (priorityPPS2>=0 && priorityPPS2<=9))
      pri=priorityPPS2;
    if(pri<0 || pri>9) pri=9;
    panicHeader.priority=(16*panicHeader.priority)+pri;

    //Now Fill Generic Header and calculate checksum
    fillGenericHeader(&panicHeader,panicHeader.gHdr.code,sizeof(AnitaEventHeader_t));
  
    //Rename body and write header for Archived
    sprintf(archiveBodyFilename,"%s/psev_%u.dat",PRIORITIZERD_EVENT_DIR,
	    panicHeader.eventNumber);
    if(rename(bodyFilename,archiveBodyFilename)==-1)
      {
	syslog(LOG_ERR,"Error moving file %s -- %s",archiveBodyFilename,
	       strerror(errno));
      }

    sprintf(archiveHdFilename,"%s/hd_%u.dat",PRIORITIZERD_EVENT_DIR,
	    panicHeader.eventNumber);
    writeStruct(&panicHeader,archiveHdFilename,sizeof(AnitaEventHeader_t));

    makeLink(archiveHdFilename,PRIORITIZERD_EVENT_LINK_DIR);
    
    //Write Header and make Link for telemetry
    sprintf(telemHdFilename,"%s/hd_%d.dat",HEADER_TELEM_DIR,
	    panicHeader.eventNumber);
    retVal=writeStruct(&panicHeader,telemHdFilename,sizeof(AnitaEventHeader_t));
    makeLink(telemHdFilename,HEADER_TELEM_LINK_DIR);

    /* Delete input */
    sprintf(linkFilename,"%s/hd_%d.dat",EVENTD_EVENT_LINK_DIR,
	    panicHeader.eventNumber);
    sprintf(hdFilename,"%s/hd_%d.dat",EVENTD_EVENT_DIR,
	    panicHeader.eventNumber);
    removeFile(linkFilename);
    removeFile(hdFilename);    
  }
}


void readInEvent(PedSubbedEventBody_t *psev, AnitaEventHeader_t* head, const char* dir, int eventNumber){
  
  char fileName[FILENAME_MAX];
  sprintf(fileName, "%s/psev_%d.dat.gz", dir, eventNumber);
  fillPedSubbedBody(psev, fileName);

  sprintf(fileName, "%s/hd_%d.dat.gz", dir, eventNumber);
  fillHeader(head, fileName);
  
  printf("Just read in event %d %d\n", head->eventNumber, psev->eventNumber);
}


void readIn100Events(const char* psevFileName, PedSubbedEventBody_t *theBody, const char* headFileName, AnitaEventHeader_t* theHeader){

  gzFile infile = gzopen (psevFileName, "rb");

  int numBytes;
  int i=0;
  int error = 0;
  for(i=0;i<100;i++) {
    numBytes=gzread(infile,&theBody[i],sizeof(PedSubbedEventBody_t));
    if(numBytes!=sizeof(PedSubbedEventBody_t)) {
      if(numBytes>0) {
	fprintf(stderr, "Read problem: %d\t%d of %lu\n", i, numBytes, sizeof(PedSubbedEventBody_t));
	printf("%s\n", psevFileName);
      }
      error=1;
      fprintf(stderr, "Balls\n");
      break;
    }

    //RJN hack for now
    /* if(runNumber>=10000 && runNumber<=10057) { */
    /*   if(counter>1 && i==1) { */
    /* 	if(theBody.eventNumber!=lastEventNumber+1) { */
    /* 	  gzrewind(infile); */
    /* 	  numBytes=gzread(infile,&theBody,sizeof(PedSubbedEventBody_t)); */
    /* 	  gzread(infile,&theBody,616); */
    /* 	  numBytes=gzread(infile,&theBody,sizeof(PedSubbedEventBody_t)); */
    /* 	} */
    /*   } */
    /* } */
    /* int lastEventNumber=theBody.eventNumber; */
    /* printf("i=%d\n", i); */
  }
  gzclose(infile);

  infile = gzopen(headFileName, "rb");

  int j=0;
  for(j=0;j<100;j++) {
    int numBytesExpected=sizeof(AnitaEventHeader_t);
    /* if(version==VER_EVENT_HEADER) { */
      numBytes=gzread(infile,&theHeader[j],sizeof(AnitaEventHeader_t));
    /* } */
    if(numBytes==-1) {
      int errorNum=0;
      printf("%s\t%d\n", gzerror(infile,&errorNum), errorNum);
    }
    if(numBytes!=numBytesExpected) {
      if(numBytes!=0) {
	error=1;
	break;
	fprintf(stderr, "Balls\n");
      }
      else break;
      fprintf(stderr, "Balls\n");
    }
    /* printf("j=%d\n", j); */
    /* processHeader(version); */
  }
  gzclose(infile);
}
