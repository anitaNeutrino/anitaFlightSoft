/*! \file Prioritizerd.c
  \brief The Prioritizerd program that creates Event objects 
  
  Reads the event objects written by Eventd, assigns a priority to each event based on the likelihood that it is an interesting event. Events with the highest priority will be transmitted to the ground first.
  March 2005 rjn@mps.ohio-state.edu
*/

#include "prioritizerdFunctions.h"

int readConfig();
void prepWriterStructs();
int writeFileAndLink(GpuPhiSectorPowerSpectrumStruct_t* payloadPowSpec, int phi);

int hkDiskBitMask=0;
int panicQueueLength=5000;

//Global Control Variables
int printToScreen=1;
int verbosity=0;

int priorityPPS1=2;
int priorityPPS2=3;

int writePowSpecPeriodSeconds = 60;

/* NUM_EVENTS is defined in the imaginatively named myInterferometryConstants.h */
AnitaEventHeader_t theHeader[NUM_EVENTS];
PedSubbedEventBody_t pedSubBody[NUM_EVENTS];

/* Used to telemeter average power spectrum information to the ground */
GpuPhiSectorPowerSpectrumStruct_t payloadPowSpec[NUM_PHI_SECTORS];
AnitaHkWriterStruct_t gpuWriter;

// Two phi sector pulsing
/* #define NUM_RUNS 21 */
/* int runs[NUM_RUNS] = {11150,11151,11152,11153,11154,11155,11156,11157,11158,11159,11160,11161,11162,11163,11164,11165,11166,11167,11168,11169,11170}; */
/* int firstEvents[NUM_RUNS] = {48816101,48827301,48834201,48840101,48852501,48860201,48866501,48872701,48879301,48885501,48892101,48898101,48905301,48910701,48916801,48921601,48926801,48931401,48936401,48941701,48941701}; */
/* int lastEvents[NUM_RUNS] = {48827133,48833893,48840076,48852498,48860143,48866401,48872457,48879114,48885253,48892088,48897996,48905232,48910653,48916709,48921523,48926570,48931221,48936323,48941615,48941701,48943571}; */


// Three phi sector pulsing (-30 deg)
/* #define NUM_RUNS 15 */
/* int runs[NUM_RUNS] = {11795, 11796, 11797, 11798, 11799, 11800, 11801, 11802, 11803, 11804, 11805, 11806, 11807, 11808, 11809}; */
/* int firstEvents[NUM_RUNS] = {75504701, 75511101, 75525001, 75529501, 75534801, 75540601, 75545901, 75553201, 75561401, 75566701, 75571401, 75575401, 75580101, 75583901, 75587901}; */
/* int lastEvents[NUM_RUNS] = {75511090, 75524914, 75529463, 75534737, 75540599, 75545825, 75553100, 75561342, 75566645, 75571342, 75575363, 75580000, 75583839, 75587821, 75653732}; */


// Three phi sector pulsing (-10 deg)
#define NUM_RUNS 14
int runs[NUM_RUNS] = {11685, 11684, 11683, 11682, 11681, 11680, 11679, 11678, 11677, 11676, 11675, 11674, 11673, 11672};
int firstEvents[NUM_RUNS] = {71658301, 71653101, 71648601, 71644501, 71640501, 71636301, 71632301, 71627001, 71622701, 71618501, 71611901, 71607501, 71602801, 71598501};
int lastEvents[NUM_RUNS] = {71661640, 71658292, 71653082, 71648541, 71644406, 71640434, 71636252, 71632298, 71626963, 71622602, 71618494, 71611800, 71607424, 71602714};


/* #define NUM_RUNS 1 */
/* int runs[NUM_RUNS] = {11672}; */
/* int firstEvents[NUM_RUNS] = {71598501}; */
/* int lastEvents[NUM_RUNS] = {71602714}; */

/* #define NUM_RUNS 1 */
/* int runs[NUM_RUNS] = {11377}; */
/* int firstEvents[NUM_RUNS] = {58393701}; */
/* int lastEvents[NUM_RUNS] = {58417574}; */

/* #define NUM_RUNS 7 */
/* int runs[NUM_RUNS] = {11818, 11817, 11816, 11815, 11814, 11813, 11812}; */
/* int firstEvents[NUM_RUNS] = {75880701, 75855701, 75819201, 75798701, 75777201, 75756701, 75736601}; */
/* int lastEvents[NUM_RUNS] = {75944796, 75880604, 75855688, 75819110, 75798679, 75777149, 75756521};	 */


/* #define NUM_RUNS 1 */
/* int runs[NUM_RUNS] = {11843}; */
/* int firstEvents[NUM_RUNS] = {76425201}; */
/* int lastEvents[NUM_RUNS] = {76581611}; */

/* #define NUM_RUNS 1 */
/* int runs[NUM_RUNS] = {11251}; */
/* int firstEvents[NUM_RUNS] = {52357401}; */
/* int lastEvents[NUM_RUNS] = {52362784}; */

/* #define NUM_RUNS 1 */
/* int runs[NUM_RUNS] = {11255}; */
/* int firstEvents[NUM_RUNS] = {52408001}; */
/* int lastEvents[NUM_RUNS] = {52409331}; */

/* #define NUM_RUNS 1 */
/* int runs[NUM_RUNS] = {11845}; */
/* int firstEvents[NUM_RUNS] = {76697001}; */
/* int lastEvents[NUM_RUNS] = {76800500}; */

int main(int argc, char *argv[]){

  /* { */
  /*   int runInd=0; */
  /*   for(runInd=0; runInd<NUM_RUNS; runInd++){ */
  /*     lastEvents[runInd] = firstEvents[runInd]+5000; */
  /*   } */
  /* } */

  /* { */
  /*   int runInd=0; */
  /*   for(runInd=0; runInd<NUM_RUNS; runInd++){ */
  /*     lastEvents[runInd] = firstEvents[runInd]+10000; */
  /*   } */
  /* } */


  int retVal; //,count;
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

  makeDirectories(GPU_TELEM_DIR);
  makeDirectories(GPU_TELEM_LINK_DIR);

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
  /* Reset average */
  int phi=0;
  for(phi=0; phi<NUM_PHI_SECTORS; phi++){
    memset(&payloadPowSpec[phi], 0, sizeof(GpuPhiSectorPowerSpectrumStruct_t));
  }
  
  prepWriterStructs();

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
      /* if(run!=selectedRun) continue; */

      const char* baseDir = "/home/anita/anitaStorage/antarctica14/raw";
      int eventFileNum=0;
      int firstFile = 100*(firstEvents[runInd]/100) + 100;
      int lastFile = 100*(lastEvents[runInd]/100) - 100;
      for(eventFileNum=firstFile; eventFileNum<=lastFile; eventFileNum+=100){
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
	    /* printf("fakePrioritizer reads event %d with priority %d\n",theHeader[eventsReadIn].eventNumber,  theHeader[eventsReadIn].priority&0xf); */
	    doTimingCalibration(eventsReadIn, theHeader[eventsReadIn], pedSubBody[eventsReadIn], finalVolts);
	    addEventToGpuQueue(eventsReadIn, finalVolts, theHeader[eventsReadIn]);
	    int chanInd=0;
	    for(chanInd=0; chanInd<ACTIVE_SURFS*CHANNELS_PER_SURF; chanInd++){
	      free(finalVolts[chanInd]);
	    }
	    eventsReadIn++;
	  }
	  else{/* Panic! Write all header files to archived directory with priority 7! */
	    panicWriteAllLinks(wd, 7, panicQueueLength, priorityPPS1, priorityPPS2);
	  }
	}
	/* exit(-1); */

	printf("eventsReadIn = %d, max is %d.\n", eventsReadIn, NUM_EVENTS);

	/* Now use GPU to determine priority, send in arrays of length eventsReadIn... */
	if(eventsReadIn>0){
	  mainGpuLoop(eventsReadIn, theHeader, payloadPowSpec, writePowSpecPeriodSeconds);
	}

	if(payloadPowSpec[0].unixTimeLastEvent - payloadPowSpec[0].unixTimeFirstEvent >= writePowSpecPeriodSeconds
	   || currentState!=PROG_STATE_RUN){
	  int phi=0;
	  for(phi=0; phi<NUM_PHI_SECTORS; phi++){
	    printf("Trying to write and link for phi sector %d...\n", phi);
	    writeFileAndLink(&payloadPowSpec[phi], phi);
	  }
	  if(currentState==PROG_STATE_RUN){
	    /* Reset average */
	    for(phi=0; phi<NUM_PHI_SECTORS; phi++){
	      memset(&payloadPowSpec[phi], 0, sizeof(GpuPhiSectorPowerSpectrumStruct_t));
	    }
	  }
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

	  /* printf("eventNumber = %u, priority = %d\n", theHeader[count].eventNumber, theHeader[count].priority&0xf); */

	  //Now Fill Generic Header and calculate checksum
	  /* fillGenericHeader(&theHeader[count],theHeader[count].gHdr.code,sizeof(AnitaEventHeader_t)); */

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
      }
    }
  } while(currentState==PROG_STATE_INIT && 0); 
  unlink(PRIORITIZERD_PID_FILE);


  closeHkFilesAndTidy(&gpuWriter);

  tidyUpGpuThings();
  tidyUpTimingCalibThings();
 /* Close the file which contains the high level output information */

  printf("fakePrioritizerd exiting. Reached end of main\n.");
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
  status = configLoad (GLOBAL_CONF_FILE,"global") ;
  status = configLoad ("Prioritizerd.config","output") ;
  if(status == CONFIG_E_OK) {
    printToScreen=kvpGetInt("printToScreen",-1);
    verbosity=kvpGetInt("verbosity",-1);
    hkDiskBitMask=kvpGetInt("hkDiskBitMask",0);

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
      writePowSpecPeriodSeconds=kvpGetInt("writePowSpecPeriodSeconds",60);
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

int writeFileAndLink(GpuPhiSectorPowerSpectrumStruct_t* payloadPowSpec, int phi) {
    char theFilename[FILENAME_MAX];
    int retVal=0;
    
    fillGenericHeader(payloadPowSpec,PACKET_GPU_AVE_POW_SPEC,sizeof(GpuPhiSectorPowerSpectrumStruct_t));
    
    sprintf(theFilename,"%s/gpuPowSpec_%u_phi%d.dat",
	    GPU_TELEM_DIR,payloadPowSpec->unixTimeFirstEvent,phi);
    retVal=writeStruct(payloadPowSpec,theFilename,sizeof(GpuPhiSectorPowerSpectrumStruct_t));
    retVal=makeLink(theFilename,GPU_TELEM_LINK_DIR);

    retVal=cleverHkWrite((unsigned char*)payloadPowSpec,sizeof(GpuPhiSectorPowerSpectrumStruct_t),
			 payloadPowSpec->unixTimeFirstEvent,&gpuWriter);
    printf("cleverHkWrite for fileName %s returned %d\n", theFilename, retVal);

    if(retVal<0) {
	//Had an error
    }
    
    return retVal;
}

void prepWriterStructs() {
    int diskInd;
    if(printToScreen) 
	printf("Preparing Writer Structs\n");
    //Hk Writer

    sprintf(gpuWriter.relBaseName,"%s/",GPU_ARCHIVE_DIR);
    sprintf(gpuWriter.filePrefix,"gpu");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	gpuWriter.currentFilePtr[diskInd]=0;
    gpuWriter.writeBitMask=hkDiskBitMask;
}
