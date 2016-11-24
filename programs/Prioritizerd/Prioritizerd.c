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
int disableGpu=1;

//Global Control Variables
int printToScreen=0;
int verbosity=0;

int priorityPPS1=2;
int priorityPPS2=3;

int writePowSpecPeriodSeconds = 60;

/* NUM_EVENTS is defined in the imaginatively named myInterferometryConstants.h */
AnitaEventHeader_t theHeader[NUM_EVENTS];
PedSubbedEventBody_t pedSubBody;

/* Used to telemeter average power spectrum information to the ground */
GpuPhiSectorPowerSpectrumStruct_t payloadPowSpec[NUM_PHI_SECTORS];
AnitaHkWriterStruct_t gpuWriter;

int main (int argc, char *argv[])
{
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

  int retVal=sortOutPidFile(progName); //count
  if(retVal<0)
    return retVal;

#ifdef WRITE_DEBUG_FILE
  FILE *debugFile = fopen("/tmp/priDebugFile.txt","w");
  if(!debugFile) {
    fprintf(stderr,"Couldn't open debug file\n");
    exit(0);
  }
#endif


  //Dont' wait for children
  //RJN hack will have to fix this somehow
  signal(SIGCLD, SIG_IGN);

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
    ///    fprintf(stderr,"RJN numEventLinks_1=%d\n",numEventLinks);
  }

  retVal=readConfig();
  /* This function  mallocs at some global pointers */
  /* so needs to be outside any kind of loop. */

  if(!disableGpu) {
    prepareGpuThings();
    prepareTimingCalibThings();
  }
  /* Reset average */
  int phi=0;
  for(phi=0; phi<NUM_PHI_SECTORS; phi++){
    memset(&payloadPowSpec[phi], 0, sizeof(GpuPhiSectorPowerSpectrumStruct_t));
  }

  prepWriterStructs();

  do {
    if(printToScreen) printf("Initalizing Prioritizerd\n");
    retVal=readConfig();
    if(retVal<0) {
      syslog(LOG_ERR,"Problem reading Prioritizerd.config");
      printf("Problem reading Prioritizerd.config\n");
      exit(1);
    }
    currentState=PROG_STATE_RUN;

    /* This one is the main while loop */
    while(currentState==PROG_STATE_RUN) {

      retVal=checkLinkDirs(1,0);
      if(retVal || numEventLinks) numEventLinks=getNumLinks(wd);
      //      fprintf(stderr,"RJN numEventLinks_2=%d\n",numEventLinks);

      if(numEventLinks>=panicQueueLength) {
	syslog(LOG_INFO,"Prioritizerd is getting behind (%d events in inbox), will have some prioritity 7 events",numEventLinks);
      }

      if(!numEventLinks){
	usleep(1000);
	continue;
      }
      int eventsReadIn = 0;

      if(numEventLinks > panicQueueLength || disableGpu){
	/* Panic! Write all header files to archived directory with priority 7! */
	panicWriteAllLinks(wd, 7, panicQueueLength, priorityPPS1, priorityPPS2);
      }
      else {
	/* Read data into program memory */
	while(eventsReadIn<NUM_EVENTS && currentState==PROG_STATE_RUN){
	  numEventLinks=getNumLinks(wd);
	  //	  fprintf(stderr,"RJN numEventLinks_3=%d\n",numEventLinks);



	  tempString=getFirstLink(wd);
	  if(numEventLinks==0) break;
	  if(tempString==NULL) continue;
	  sscanf(tempString,"hd_%d.dat",&doingEvent);
	  if(lastEventNumber>0 && doingEvent!=lastEventNumber+1) {
	    syslog(LOG_INFO,"Non-sequential event numbers %d and %d\n", lastEventNumber, doingEvent);
	  }
	  lastEventNumber=doingEvent;

	  sprintf(linkFilename, "%s/%s", EVENTD_EVENT_LINK_DIR, tempString);
	  sprintf(hdFilename, "%s/hd_%d.dat", EVENTD_EVENT_DIR, doingEvent);

	  //RJN 4th June 2008
	  //Switch to Acqd writing psev files
	  sprintf(bodyFilename[eventsReadIn],"%s/psev_%d.dat", ACQD_EVENT_DIR, doingEvent);

	  retVal=fillHeader(&theHeader[eventsReadIn],hdFilename);



	  /* Then we add to GPU queue... */
	  retVal=fillPedSubbedBody(&pedSubBody,bodyFilename[eventsReadIn]);
	  double* finalVolts[ACTIVE_SURFS*CHANNELS_PER_SURF];
	  doTimingCalibration(eventsReadIn, &theHeader[eventsReadIn], pedSubBody, finalVolts);
	  addEventToGpuQueue(eventsReadIn, finalVolts, theHeader[eventsReadIn]);
	  int chanInd=0;
	  for(chanInd=0; chanInd<ACTIVE_SURFS*CHANNELS_PER_SURF; chanInd++){
	    free(finalVolts[chanInd]);
	  }
	  eventsReadIn++;
	}


	printf("eventsReadIn = %d, max is %d.\n", eventsReadIn, NUM_EVENTS);
      }

      /* Now use GPU to determine priority, send in arrays of length eventsReadIn... */
      if(eventsReadIn>0 && !disableGpu){
	mainGpuLoop(eventsReadIn, theHeader, payloadPowSpec, writePowSpecPeriodSeconds);
      }

      if((payloadPowSpec[0].unixTimeLastEvent > 0 &&
	  payloadPowSpec[0].unixTimeLastEvent - payloadPowSpec[0].unixTimeFirstEvent >= writePowSpecPeriodSeconds)
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

	printf("eventNumber = %u, priority = %d\n", theHeader[count].eventNumber, theHeader[count].priority&0xf);

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


	printf("just before writing... (theHeader[count].prioritizerStuff & 0x4000)>>14 = %hu\n", (theHeader[count].prioritizerStuff & 0x4000)>>14);

	sprintf(archiveHdFilename,"%s/hd_%u.dat",PRIORITIZERD_EVENT_DIR,
		theHeader[count].eventNumber);
	writeStruct(&theHeader[count],archiveHdFilename,sizeof(AnitaEventHeader_t));

	makeLink(archiveHdFilename,PRIORITIZERD_EVENT_LINK_DIR);

	//Write Header and make Link for telemetry
	sprintf(telemHdFilename,"%s/hd_%d.dat",HEADER_TELEM_DIR,
		theHeader[count].eventNumber);
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

  closeHkFilesAndTidy(&gpuWriter);

  tidyUpGpuThings();
  tidyUpTimingCalibThings();

  printf("fakePrioritizerd exiting. Reached end of main\n.");
  return 0;
}


int readConfig()
// Load Prioritizerd config stuff
{
  printf("readConfigFile\n");
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
      disableGpu=kvpGetInt("disableGpu",0);
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

    //write for telemetry
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
    //    if(printToScreen)
      printf("Preparing Writer Structs -- hkDiskBitMask %d\n",hkDiskBitMask);
    //Hk Writer

    sprintf(gpuWriter.relBaseName,"%s/",GPU_ARCHIVE_DIR);
    sprintf(gpuWriter.filePrefix,"gpu");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	gpuWriter.currentFilePtr[diskInd]=0;
    gpuWriter.writeBitMask=hkDiskBitMask;
}
