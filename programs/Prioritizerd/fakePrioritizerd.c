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
AnitaEventHeader_t hackyHeaders[NUM_EVENTS];
PedSubbedEventBody_t pedSubBody[NUM_EVENTS];

/* Used to telemeter average power spectrum information to the ground */
GpuPhiSectorPowerSpectrumStruct_t payloadPowSpec[NUM_PHI_SECTORS];
AnitaHkWriterStruct_t gpuWriter;



int main(int argc, char *argv[]){


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

  //Dont' wait for children
  //RJN hack will have to fix this somehow
  signal(SIGCLD, SIG_IGN);


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
     This function mallocs at some global pointers
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

    /* const char* baseDir = "/home/anita/anitaStorage/antarctica14/raw"; */
    const char* baseDir = "/mnt/ben";

    // calibration badassery
    /* FILE* waisPulseEventNumbers = fopen("waisEventNumbers.txt", "r"); */
    FILE* waisPulseEventNumbers = fopen("waisEventNumbers352.txt", "r");
    int run = 0;
    int eventNumber;
    const int numWaisPulses = 5625; //118160; // from wc -l
    int numPulsesReadIn = 0; // counter


    fscanf(waisPulseEventNumbers, "%d\t%d", &run, &eventNumber);
    printf("I'm looking in run %d for eventNumber %d\n", run, eventNumber);
    numPulsesReadIn++;


    /* This one is the main while loop */
    int entry = 0;
    for(entry=0; entry < numWaisPulses; entry++){

      int numEventsInGpuQueue = 0;
      while(numEventsInGpuQueue < 100){

	numPulsesReadIn = 0;
	//	usleep(1);
	retVal = checkLinkDirs(1,0);
	if(retVal || numEventLinks) {
	  numEventLinks = getNumLinks(wd);
	}

	if(numEventLinks >= panicQueueLength) {
	  syslog(LOG_INFO,"Prioritizerd is getting behind (%d events in inbox), will have some prioritity 7 events",numEventLinks);
	}

	/* if(!numEventLinks){ */
	/* 	usleep(1000); */
	/* 	continue; */
	/* } */

	int eventFileNum = 100*(eventNumber/100);

	/* Read data into program memory */
	int eventDir1 = eventFileNum/1000000;
	eventDir1 *= 1000000;
	int eventDir2 = eventFileNum/10000;
	eventDir2 *= 10000;

	char dir[FILENAME_MAX];
	sprintf(dir, "%s/run%d/event/ev%d/ev%d", baseDir, run, eventDir1, eventDir2);

	/* char fileName1[FILENAME_MAX]; */
	/* sprintf(fileName1, "%s/psev_%d.dat.gz", dir, eventFileNum); */
	/* char fileName2[FILENAME_MAX]; */
	/* sprintf(fileName2, "%s/hd_%d.dat.gz", dir, eventFileNum); */

	char fileName1[FILENAME_MAX];
	sprintf(fileName1, "%s/psev_%d.dat", dir, eventFileNum);
	char fileName2[FILENAME_MAX];
	sprintf(fileName2, "%s/hd_%d.dat", dir, eventFileNum);


	/* printf("%s\n", fileName1); */

	readIn100UnzippedEvents(fileName1, pedSubBody, fileName2, theHeader);

	/* readInTextFile(pedSubBody, theHeader); */


	int eventsReadFromDisk = 0;
	while(eventsReadFromDisk<100 && currentState==PROG_STATE_RUN){

	  /* numEventLinks=getNumLinks(wd); */
	  /* tempString=getFirstLink(wd); */
	  /* if(numEventLinks==0) break; */
	  /* if(tempString==NULL) continue; */
	  /* //	printf("tempString = %s\n", tempString); */
	  /* //	printf("%s\n",eventLinkList[eventsReadFromDisk]->d_name);  */
	  /* sscanf(tempString,"hd_%d.dat",&doingEvent); */
	  /* if(lastEventNumber>0 && doingEvent!=lastEventNumber+1) { */
	  /*   syslog(LOG_INFO,"Non-sequential event numbers %d and %d\n", lastEventNumber, doingEvent); */
	  /* } */
	  /* lastEventNumber=doingEvent; */

	  /* sprintf(linkFilename, "%s/%s", EVENTD_EVENT_LINK_DIR, tempString); */
	  /* sprintf(hdFilename, "%s/hd_%d.dat", EVENTD_EVENT_DIR, doingEvent); */

	  /* //RJN 4th June 2008 */
	  /* //Switch to Acqd writing psev files */
	  /* sprintf(bodyFilename[eventsReadFromDisk],"%s/psev_%d.dat", ACQD_EVENT_DIR, doingEvent); */

	  /* retVal=fillHeader(&theHeader[eventsReadFromDisk],hdFilename); */

	  /* if(numEventLinks < panicQueueLength){ */
	  if(numEventLinks < panicQueueLength){

	    // OK so let's skip events that aren't in the read-in list.
	    if(theHeader[eventsReadFromDisk].eventNumber==eventNumber){

	      /* printf("I found eventNumber %d in run %d\n", eventNumber, run); */

	      /* Then we add to GPU queue... */
	      double* finalVolts[ACTIVE_SURFS*CHANNELS_PER_SURF];
	      doTimingCalibration(eventsReadFromDisk, &theHeader[eventsReadFromDisk], pedSubBody[eventsReadFromDisk], finalVolts);
	      /* addEventToGpuQueue(eventsReadFromDisk, finalVolts, theHeader[eventsReadFromDisk]); */
	      addEventToGpuQueue(numEventsInGpuQueue, finalVolts, theHeader[eventsReadFromDisk]);
	      /* addEventToGpuQueue(numPulsesReadIn, finalVolts, theHeader[eventsReadFromDisk]);	       */
	      /* hackyHeaders[numPulsesReadIn] = theHeader[eventsReadFromDisk]; */
	      hackyHeaders[numEventsInGpuQueue] = theHeader[eventsReadFromDisk];

	      int chanInd=0;
	      for(chanInd=0; chanInd<ACTIVE_SURFS*CHANNELS_PER_SURF; chanInd++){
		free(finalVolts[chanInd]);
	      }

	      fscanf(waisPulseEventNumbers, "%d\t%d", &run, &eventNumber);
	      /* printf("I'm looking in run %d for eventNumber %d\n", run, eventNumber); */
	      numPulsesReadIn++;
	      numEventsInGpuQueue++;
	    }
	    eventsReadFromDisk++;
	  }
	  else{/* Panic! Write all header files to archived directory with priority 7! */
	    panicWriteAllLinks(wd, 7, panicQueueLength, priorityPPS1, priorityPPS2);
	  }
	}
      }
      /* exit(-1); */


      /* printf("eventsReadFromDisk = %d, added %d to queue, max is %d.\n", eventsReadFromDisk, numPulsesReadIn, NUM_EVENTS); */
      printf("numEventsInGpuQueue = %d, max is %d.\n", numEventsInGpuQueue, NUM_EVENTS);
      /* printf("Doing main gpu loop...\n"); */

      /* Now use GPU to determine priority, send in arrays of length eventsReadFromDisk... */
      /* if(numPulsesReadIn > 0){ */
      /* 	/\* mainGpuLoop(eventsReadFromDisk, theHeader, payloadPowSpec, writePowSpecPeriodSeconds); *\/ */
      /* 	mainGpuLoop(numPulsesReadIn, hackyHeaders, payloadPowSpec, writePowSpecPeriodSeconds); */
      /* 	numPulsesReadIn = 0; */
      /* } */
      if(numEventsInGpuQueue > 0){
      	/* mainGpuLoop(eventsReadFromDisk, theHeader, payloadPowSpec, writePowSpecPeriodSeconds); */
      	mainGpuLoop(numEventsInGpuQueue, hackyHeaders, payloadPowSpec, writePowSpecPeriodSeconds);
      	numPulsesReadIn = 0;
      }

      /* if(payloadPowSpec[0].unixTimeLastEvent - payloadPowSpec[0].unixTimeFirstEvent >= writePowSpecPeriodSeconds */
      /* 	 || currentState!=PROG_STATE_RUN){ */
      /* 	int phi=0; */
      /* 	for(phi=0; phi<NUM_PHI_SECTORS; phi++){ */
      /* 	  printf("Trying to write and link for phi sector %d...\n", phi); */
      /* 	  writeFileAndLink(&payloadPowSpec[phi], phi); */
      /* 	} */
      /* 	if(currentState==PROG_STATE_RUN){ */
      /* 	  /\* Reset average *\/ */
      /* 	  for(phi=0; phi<NUM_PHI_SECTORS; phi++){ */
      /* 	    memset(&payloadPowSpec[phi], 0, sizeof(GpuPhiSectorPowerSpectrumStruct_t)); */
      /* 	  } */
      /* 	} */
      /* } */

      int count = 0;
      /* for(count=0;count<eventsReadFromDisk;count++) { */
      for(count=0;count<numPulsesReadIn;count++) {

	// handle queue forcing of PPS here
	int pri=theHeader[count].priority&0xf;
	if((theHeader[count].turfio.trigType&0x2) && (priorityPPS1>=0 && priorityPPS1<=9)){
	  pri = priorityPPS1;
	}
	if((theHeader[count].turfio.trigType&0x4) && (priorityPPS2>=0 && priorityPPS2<=9)){
	  pri = priorityPPS2;
	}
	if(pri<0 || pri>9) {
	  pri = 9;
	}
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
    syslog(LOG_ERR,"Error reading Prioritizerd.config: %s\n",eString);
    fprintf(stderr,"Error reading Prioritizerd.config: %s\n",eString);
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
