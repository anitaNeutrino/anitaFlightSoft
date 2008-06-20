/*  \file Acqd.c
    \brief The Acqd program 
    The first version of Acqd that will work (hopefully) with Patrick's new 
    surfDriver.
    April 2008 rjn@hep.ucl.ac.uk
*/

// Standard Includes
#define _GNU_SOURCE

//Flight software includes
#include "Acqd.h"
#include "surfDriver_ioctl.h"
#include "turfioDriver_ioctl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <libgen.h> //For Mac OS X
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/pci.h>



// Anita includes
#include "includes/anitaCommand.h"
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "pedestalLib/pedestalLib.h"
#include "slowLib/slowLib.h"
#include "includes/anitaStructures.h"

//Definitions
#define HK_DEBUG 0


void servoOnRate(unsigned int eventNumber, unsigned int lastRateCalcEvent, struct timeval *currentTime, struct timeval *lastRateCalcTime);

inline unsigned short byteSwapUnsignedShort(unsigned short a){
  return (a>>8)+(a<<8);
}
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);

//The SURF and TURFIO File Descriptors
int surfFds[MAX_SURFS]; 
int turfioFd;

//Global Variables
BoardLocStruct_t turfioPos;
BoardLocStruct_t surfPos[MAX_SURFS];
int dacSurfs[MAX_SURFS];
int surfIndex[MAX_SURFS];
int printToScreen=0,standAloneMode=0;
int printPidStuff=1;
int numSurfs=0,doingEvent=0,hkNumber=0;
int slipCounter=0;
int turfRateTelemEvery=1,turfRateTelemInterval=60;
int turfRateAverage=60;
int surfHkPeriod=0;
int surfHkAverage=10;
int surfHkTelemEvery=0;
int writeRawScalers=0; //period in seconds
int lastSurfHk=0;
int lastTurfHk=0;
int turfHkCounter=0;
int surfHkCounter=0;
int surfMask;
int hkDiskBitMask=0;
int useInterrupts=0;
int niceValue=-20;
unsigned short data_array[MAX_SURFS][N_CHAN][N_SAMP]; 
AnitaEventFull_t theEvent;
AnitaEventHeader_t *hdPtr;//=&(theEvent.header);
AnitaEventBody_t *bdPtr;//=&(theEvent.body)
PedSubbedEventBody_t pedSubBody;
TurfioStruct_t *turfioPtr;//=&(hdPtr->turfio);
TurfRateStruct_t turfRates;
AveragedSurfHkStruct_t avgSurfHk;
int newTurfRateData=0;


TurfRegisterContents_t theTurfReg;
SimpleScalerStruct_t theScalers;
FullSurfHkStruct_t theSurfHk;

int eventsBeforeClear=1000;


//Pedestal stuff
int pedestalMode=0;
int pedUsePPS2Trig=0;
int pedSoftTrigSleepMs=100;
int writeDebugData=0;
int numPedEvents=4000;
int pedSwitchConfigAtEnd=1; //Need to work out exactly what this does

//Start test stuff
int enableStartTest=1;
int startSoftTrigs=10;



//Temporary Global Variables
unsigned int labData[MAX_SURFS][N_CHAN][N_SAMP];
unsigned int avgScalerData[MAX_SURFS][SCALERS_PER_SURF];
//unsigned short threshData[MAX_SURFS][SCALERS_PER_SURF];
//unsigned short rfpwData[MAX_SURFS][N_RFCHAN];

//Configurable watchamacallits
/* Ports and directories */
char altOutputdir[FILENAME_MAX];
char subAltOutputdir[FILENAME_MAX];

#define EVTF_TIMEOUT 10000000 /* in microseconds */

int surfFirmwareVersion=2;
int turfFirmwareVersion=3;
int verbosity = 0 ; /* control debug print out. */
int addedVerbosity = 0 ; /* control debug print out. */
int oldStyleFiles = FALSE;
int useAltDir = FALSE;
int madeAltDir = FALSE;
int writeData = FALSE; /* Default is not to write data to a disk */
int writeFullHk = FALSE;
int justWriteHeader = FALSE;

//Threshold Scan Things
int doThresholdScan = FALSE;
int doSingleChannelScan = FALSE;
int surfForSingle=0;
int dacForSingle=0;
int thresholdScanStepSize =1;
int thresholdScanPointsPerStep =1;
int threshSwitchConfigAtEnd=1;


int dontWaitForEvtF = FALSE;
int dontWaitForLabF = FALSE;
int sendSoftTrigger = FALSE;
int softTrigPeriod = 0;
int reprogramTurf = FALSE;

//Trigger Modes
TriggerMode_t trigMode=TrigNone;
int pps1TrigFlag = 0;
int pps2TrigFlag = 0;

//Threshold Stuff
//int thresholdArray[ACTIVE_SURFS][DAC_CHAN][PHYS_DAC];
int surfTrigBandMasks[ACTIVE_SURFS];
int thresholdArray[ACTIVE_SURFS][SCALERS_PER_SURF];
float pidGoalScaleFactors[ACTIVE_SURFS][SCALERS_PER_SURF];
int setGlobalThreshold=0; 
int globalThreshold=2200;
DacPidStruct_t thePids[ACTIVE_SURFS][SCALERS_PER_SURF];
float dacIGain;  // integral gain
float dacPGain;  // proportional gain
float dacDGain;  // derivative gain
int dacIMax;  // maximum intergrator state
int dacIMin; // minimum integrator state
int enableChanServo = FALSE; //Turn on the individual chanel servo
int pidGoals[BANDS_PER_ANT];
int pidPanicVal;
int pidAverage;
int numEvents=0;
//time_t lastRateTime;
int calculateRateAfter=5;

//Rate servo stuff
int enableRateServo=1;
int servoRateCalcPeriod=60;
float rateGoal=5;
float ratePGain=100;
float rateIGain=1;
float rateDGain=0;
float rateIMax=100;
float rateIMin=-100;



//RFCM Mask
//1 is disable
//lowest bit of first word is antenna 1
//bit 8 in second word is 48th antenna
//We load in reverse order starting with the missing SURF 10 
//and working backwards to SURF 1
unsigned int antTrigMask=0; //antennas 1-32
unsigned int nadirAntTrigMask=0; //antennas 33-40

//File writing structures
AnitaHkWriterStruct_t turfHkWriter;
AnitaHkWriterStruct_t surfHkWriter;
AnitaHkWriterStruct_t avgSurfHkWriter;
AnitaHkWriterStruct_t sumTurfRateWriter;
AnitaHkWriterStruct_t rawScalerWriter;
AnitaEventWriterStruct_t eventWriter;

//Deadtime monitoring
float totalDeadtime; //In Secs
float totalTime; //In Secs
float intervalDeadtime; //In secs

//Surf Hk Readout
struct timeval lastSurfHkRead;


//TURF Rate Readout
unsigned short lastPPSNum=0;

//RJN -- Nasty hack as SURF 2 is the only one we monitor busy on
int surfBusyWatch=1; //SURF 2 is the only one we monitor busy on
int surfClearIndex[9]={0,2,3,4,5,6,7,8,1};


int main(int argc, char **argv) {
  AcqdErrorCode_t status=ACQD_E_OK;
  int retVal=0;
  int i=0,tmo=0,surf; 
  int doneStartTest=0;
  int numDevices; 
  int eventReadyFlag=0;
  unsigned short dacVal=2200;
  unsigned int lastEvNum=0;
  struct timeval timeStruct;

 
  int reInitNeeded=0;

  //Initialize some of the timing variables
  lastSurfHkRead.tv_sec=0;
  lastSurfHkRead.tv_usec=0;

  /* Log stuff */
  char *progName=basename(argv[0]);
  retVal=sortOutPidFile(progName);
  if(retVal<0)
    return -1;

  //Initialize handy pointers
  hdPtr=&(theEvent.header);
  bdPtr=&(theEvent.body);
  turfioPtr=&(hdPtr->turfio);

  //Initialize dacPid stuff
  memset(&thePids,0,sizeof(DacPidStruct_t)*ACTIVE_SURFS*SCALERS_PER_SURF);    
   
  // Setup log 
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

  // Set signal handlers
  signal(SIGUSR1, sigUsr1Handler);
  signal(SIGUSR2, sigUsr2Handler);
  signal(SIGTERM, sigUsr2Handler);
  signal(SIGINT, sigUsr2Handler);
  signal(SIGSEGV,handleBadSigs);

  //Dont' wait for children
  //RJN hack will have to fix this somehow
  signal(SIGCLD, SIG_IGN); 

  //Setup output directories
  makeDirectories(HEADER_TELEM_LINK_DIR);
  makeDirectories(PEDESTAL_DIR);
  makeDirectories(ACQD_EVENT_LINK_DIR);
  makeDirectories(SURFHK_TELEM_LINK_DIR);
  makeDirectories(TURFHK_TELEM_LINK_DIR);
  makeDirectories(CMDD_COMMAND_LINK_DIR);
  makeDirectories(REQUEST_TELEM_LINK_DIR);

  retVal=readConfigFile();
  if(retVal!=CONFIG_E_OK) {
    syslog(LOG_ERR,"Error reading config file Acqd.config: bailing");
    fprintf(stderr,"Error reading config file Acqd.config: bailing\n");
    handleBadSigs(1002);
  }    

  if(standAloneMode) {
    //Read the command line variables
    init_param(argc, argv,  &numEvents, &dacVal) ;
  }

  // Initialize devices
    
  status=initializeDevices(&numDevices);
  if (status!=ACQD_E_OK || numDevices < 0) {
    fprintf(stderr,"Problem initializing devices\n");
    syslog(LOG_ERR,"Error initializing devices");
    handleBadSigs(1002);
  }

  //Reprogram TURF if desired
  if (reprogramTurf) {
    printf("Reprogramming Turf\n");
    setTurfControl(RprgTurf) ;
    for(i=9 ; i<100 ; i++) 
      usleep(1000);
  }

  //Setup the output structures
  if(doingEvent==0) prepWriterStructs();

  //No idea why we want to sleep here but hey there we are
  if(sendSoftTrigger) sleep(2);

 //Now we'll do our startUpTest
  if(enableStartTest && !doneStartTest) {
    clearDevices();
    doStartTest();
    doneStartTest=1;    
  }

  //Main program loop
  do { //while( currentState==PROG_STATE_INIT
    lastSurfHkRead.tv_sec=0;
    lastSurfHkRead.tv_usec=0;
    unsigned int tempTrigMask=antTrigMask;
    unsigned int tempNadirTrigMask=nadirAntTrigMask;
    // Clear devices 
    clearDevices();

    antTrigMask=tempTrigMask;
    nadirAntTrigMask=tempNadirTrigMask;

    if(!reInitNeeded) {
      retVal=readConfigFile();
      if(retVal!=CONFIG_E_OK) {
	syslog(LOG_ERR,"Error reading config file Acqd.config: bailing");
	handleBadSigs(1002);
	//		return -1;
      }
    }
    reInitNeeded=0;
    slipCounter=0;

    //Send software trigger
    //Take a forced trigger to get rid of any junk
    //    setTurfControl(SendSoftTrg);
    
   



      
    // Set trigger modes
    //RF and Software Triggers enabled by default
    trigMode=TrigNone;
    if(pps1TrigFlag) trigMode|=TrigPPS1;
    if(pps2TrigFlag) trigMode|=TrigPPS2;
    // RJN debugging -- what the heck there's some subtlety I can't
    // quite remember that explains why the line below is commented out
    //	setTurfControl(SetTrigMode);

    if(pedestalMode && pedUsePPS2Trig) {
      trigMode=TrigPPS2;
      sendSoftTrigger=0;
    }


    theSurfHk.globalThreshold=0;

    //Just some debugging statements
    if(printToScreen) {
      if(pps1TrigFlag) 
	printf("PPS 1 Trig Enabled\n");
      else 
	printf("PPS 1 Trig Disabled\n");		
      if(pps2TrigFlag) 
	printf("PPS 2 Trig Enabled\n");
      else 
	printf("PPS 2 Trig Disabled\n");		
      if(sendSoftTrigger) 
	printf("Sending software triggers every %d seconds\n",softTrigPeriod);
    }

    if(doThresholdScan || pedestalMode) {
      //If we're doing a threshold or a pedestal scan disable all antennas
      antTrigMask=0xffffffff;
      nadirAntTrigMask=0xff;

      //Also disable surfTrigBandMasks
      for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfTrigBandMasks[surf]=0;
      }
      
    }

    //Write antenna mask to exclude certain antennas from events
    writeAntTrigMask();
	
    //Set Thresholds
    if(setGlobalThreshold) {
      setGloablDACThreshold(globalThreshold);
      theSurfHk.globalThreshold=globalThreshold;
      if(printToScreen) 
	printf("Setting Global Threshold %d\n",globalThreshold);
    }
    else {
      setDACThresholds();
    }

    //Now we move into standard running mode
    currentState=PROG_STATE_RUN;
	
	

    //Now we either breakout in to threshold scan or pedestal mode
    //or we stay the main event loop
    if(doThresholdScan) {
      printf("About to enter threshold scan function\n");
      status=doGlobalThresholdScan();
      if(status!=ACQD_E_OK) {
	syslog(LOG_ERR,"Error running threshold scan\n");
	fprintf(stderr,"Error running threshold scan\n");
      }
      currentState=PROG_STATE_TERMINATE;
    }
	
    //Pedestal Mode
    if(pedestalMode) {
      printf("About to enter pedestal function\n");
      status=runPedestalMode();
      if(status!=ACQD_E_OK) {
	syslog(LOG_ERR,"Error running pedestal mode\n");
	fprintf(stderr,"Error running pedestal mode\n");
      }
      currentState=PROG_STATE_TERMINATE;
    }
	

    while (currentState==PROG_STATE_RUN) {
      //This is the start of the main event loop
      if(numEvents && numEvents<doingEvent) {
	currentState=PROG_STATE_TERMINATE;
	break;
	//This is one way out of the main event loop
      }

      //Fill theEvent with zeros 
      memset(&theEvent,0, sizeof(theEvent)) ;
      memset(&theSurfHk,0,sizeof(FullSurfHkStruct_t));
      if(setGlobalThreshold) 
	theSurfHk.globalThreshold=globalThreshold;

      gettimeofday(&timeStruct,NULL);
     //Send software trigger if we want to
      if(sendSoftTrigger)
	intersperseSoftTrig(&timeStruct);


      // Wait for ready to read (EVT_RDY) 
      tmo=0;	    
      if(!dontWaitForEvtF) {
	if(verbosity && printToScreen) 
	  printf("Wait for EVT_RDY on SURF %d\n",surfIndex[0]);
	do {
	  //Need to make the SURF that we read the GPIO value from
	  //somehow changeable
	  eventReadyFlag=0;
	  status=getSurfStatusFlag(0,SurfEventReady,&eventReadyFlag);
	  //	  printf("eventReadyFlag %d -- %d\n",eventReadyFlag,checkSurfFdsForData());
	  if(status!=ACQD_E_OK) {
	    fprintf(stderr,"Error reading GPIO value from SURF 0\n");
	    syslog(LOG_ERR,"Error reading GPIO value from SURF 0\n");
	    //Do what??
	  }		
	  if(verbosity>3 && printToScreen) 
	    printf("SURF %d Event Ready Flag %d\n",
		   surfIndex[0],eventReadyFlag);
		
	  //
	  //Have to change this at some point
	  if(tmo) //Might change tmo%2
	    myUsleep(1); 

	  tmo++;
		       		
	  //This time getting is quite important as the two bits below rely on having a recent time.
	  gettimeofday(&timeStruct,NULL);

	  //Now we have a couple of functions that work out rates and such
	  //Writes slow rate if approrpriate
	  handleSlowRate(&timeStruct,lastEvNum); 
	  //Calculates rate and servos on it if we wish
	  rateCalcAndServo(&timeStruct,lastEvNum); 
	  //Gives us a chance to read hk and servo thresholds
	  intersperseSurfHk(&timeStruct);
	  //Also gives a chance to send software tiggers
	  if(sendSoftTrigger)
	    intersperseSoftTrig(&timeStruct);

	  if(currentState!=PROG_STATE_RUN) 
	    break;
	  
	  if(tmo==EVTF_TIMEOUT)
	    break;
	
	} while(!(eventReadyFlag));
	      
	//Check if we are trying to leave the event loop
	if(currentState!=PROG_STATE_RUN) 
	  continue;
	      
	if (tmo==EVTF_TIMEOUT){
	  syslog(LOG_WARNING,"Timed out waiting for EVT_RDY flag");
	  if(printToScreen)
	    printf(" Timed out (%d ms) while waiting for EVT_RDY flag in self trigger mode.\n", tmo) ;
	  continue;		
	}
      }
      else {
	printf("Didn't wait for EVT_RDY\n");
	sleep(1);
      }
	    
      //Now load event into ram
      if (setTurfControl(TurfLoadRam) != ACQD_E_OK) {
	fprintf(stderr,"Failed to send load TURF event to TURFIO.\n") ;
	syslog(LOG_ERR,"Failed to send load TURF event to TURFIO.\n") ;
      }


      //Either have a trigger or are going ahead regardless
      // so we should work out when now is
      gettimeofday(&timeStruct,NULL);
	    
      //Now actually read the event data
      status+=readSurfEventData();
      if(status!=ACQD_E_OK) {
	fprintf(stderr,"Error reading SURF event data %d.\n",doingEvent);
	syslog(LOG_ERR,"Error reading SURF event data %d.\n",doingEvent);
      }
	      
      hdPtr->unixTime=timeStruct.tv_sec;
      hdPtr->unixTimeUs=timeStruct.tv_usec;
      if(verbosity && printToScreen) printf("Read SURF Labrador Data\n");
      
      if(turfFirmwareVersion==3)
	status=readTurfEventDataVer3();
      else
	status=readTurfEventDataVer5();

      turfRates.unixTime=timeStruct.tv_sec;
      if(status!=ACQD_E_OK) {
	fprintf(stderr,"Problem reading TURF event data\n");
	syslog(LOG_ERR,"Problem reading TURF event data\n");
      }
      if(slipCounter!=hdPtr->turfio.trigNum) {
	printf("Read TURF data -- doingEvent %d -- slipCounter %d -- trigNum %d\n", doingEvent,slipCounter,hdPtr->turfio.trigNum); 
	reInitNeeded=1;
      }
      if(reInitNeeded && (slipCounter>eventsBeforeClear)) {
	fprintf(stderr,"Acqd reinit required -- slipCounter %d -- trigNum %d",slipCounter,hdPtr->turfio.trigNum);
	syslog(LOG_INFO,"Acqd reinit required -- slipCounter %d -- trigNum %d",slipCounter,hdPtr->turfio.trigNum);
	currentState=PROG_STATE_INIT;
      }
      hdPtr->errorFlag=0;
      if(reInitNeeded) {
	hdPtr->errorFlag=0x1;
      }
            
      //Switched for timing test
      gettimeofday(&timeStruct,NULL);
      intersperseSurfHk(&timeStruct);
      intersperseTurfRate(&timeStruct);

	    
      if(verbosity && printToScreen) printf("Done reading\n");
	    
      //Add TURF Rate Info to slow file
      if(newTurfRateData) {
	addTurfRateToAverage(&turfRates);
      }
	    
      //Error checking
      if(status!=ACQD_E_OK) {
	//We have an error
	fprintf(stderr,"Error detected %d, during event %d",status,doingEvent);
	syslog(LOG_ERR,"Error detected %d, during event %d",status,doingEvent);	      
	status=ACQD_E_OK;
      }
      else if(doingEvent>=0) { //RJN changed 24/11/06 for test
	hdPtr->eventNumber=getEventNumber();
	bdPtr->eventNumber=hdPtr->eventNumber;
	lastEvNum=hdPtr->eventNumber;
	//	hdPtr->surfMask=surfMask;
	hdPtr->antTrigMask=(unsigned int)antTrigMask;	       
	hdPtr->nadirAntTrigMask=(unsigned char)nadirAntTrigMask&0xff;	       
		
	//Filled by Eventd
	//hdPtr->gpsSubTime;
	//hdPtr->calibStatus;

	if(printToScreen && verbosity) 
	  printf("Event:\t%u\nSec:\t%d\nMicrosec:\t%d\nTrigTime:\t%u\n",hdPtr->eventNumber,hdPtr->unixTime,hdPtr->unixTimeUs,turfioPtr->trigTime);

	// Save data if we want to
	outputEventData();
	if(newTurfRateData) outputTurfRateData();		    	

      }
	    
      // Clear boards
      sendClearEvent();     
	    
    }  //while(currentState==PROG_STATE_RUN)

	

  } while(currentState==PROG_STATE_INIT);

  // Clean up
  status=closeDevices();
  if(status!=ACQD_E_OK) {
    fprintf(stderr,"Error closing devices\n");
    syslog(LOG_ERR,"Error closing devices\n");
  }

  closeHkFilesAndTidy(&rawScalerWriter);
  closeHkFilesAndTidy(&avgSurfHkWriter);
  closeHkFilesAndTidy(&sumTurfRateWriter);
  closeHkFilesAndTidy(&surfHkWriter);
  closeHkFilesAndTidy(&turfHkWriter);
  unlink(ACQD_PID_FILE);
  syslog(LOG_INFO,"Acqd terminating");
  return 1 ;
}

//Reading and Writing GPIO Values
AcqdErrorCode_t getSurfStatusFlag(int surfId, SurfStatusFlag_t flag, int *value)
{
  int surfFd=surfFds[surfId];
  unsigned short svalue;
  int retVal=0;
  *value=0;
  if(surfFd<=0) {
    fprintf(stderr,"SURF FD %d is not a valid file descriptor\n",surfFd);
    syslog(LOG_ERR,"SURF FD %d is not a valid file descriptor\n",surfFd);
    return ACQD_E_BADFD;
  }
  switch(flag) {
  case SurfEventReady : 
    retVal=ioctl(surfFd, SURF_IOCEVENTREADY);
    *value=retVal;
    break ; 
  case SurfIntState:
    retVal=ioctl(surfFd,SURF_IOCQUERYINT);
    *value=retVal;
    break;
  case SurfBusInfo:
    retVal=ioctl(surfFd,SURF_IOCGETSLOT,&svalue);
    *value=svalue;
    break;
  default:
    if(printToScreen)
      fprintf(stderr,"getSurfStatusFlag: undefined status flag, %d\n", flag) ;
    return ACQD_E_UNKNOWN_CMD ;
  } 
  if(retVal<0) {
    fprintf(stderr,"getSurfStatusFlag %s (%d) had error: %s\n",surfStatusFlagAsString(flag),flag,strerror(errno));
    syslog(LOG_ERR,"getSurfStatusFlag %s (%d) had error: %s\n",surfStatusFlagAsString(flag),flag,strerror(errno));
    return ACQD_E_SURF_GPIO;
  }        
  return ACQD_E_OK;

}

AcqdErrorCode_t readTurfGPIOValue(unsigned int *gpioVal) 
{
  AcqdErrorCode_t status=ACQD_E_OK;
  if(turfioFd<=0) {
    return ACQD_E_NO_TURFIO;
  }
  int retVal=ioctl(turfioFd, TURFIO_IOCGETGPIO, gpioVal);
  if(retVal<0) {
    syslog(LOG_ERR,"Error reading TURFIO GPIO -- %s\n",strerror(errno));
    fprintf(stderr,"Error reading TURFIO GPIO -- %s\n",strerror(errno));
    status=ACQD_E_TURFIO_GPIO;
  }
  return status;
}

AcqdErrorCode_t setTurfGPIOValue(unsigned int gpioVal) 
{
  AcqdErrorCode_t status=ACQD_E_OK;
  if(turfioFd<=0) {
    return ACQD_E_NO_TURFIO;
  }
  int retVal=ioctl(turfioFd, TURFIO_IOCSETGPIO, &gpioVal);
  if(retVal<0) {
    syslog(LOG_ERR,"Error setting TURFIO GPIO -- %s\n",strerror(errno));
    fprintf(stderr,"Error setting TURFIO GPIO -- %s\n",strerror(errno));
    status=ACQD_E_TURFIO_GPIO;
  }
  return status;
}

char *surfStatusFlagAsString(SurfStatusFlag_t flag) {
  char* string ;
  switch(flag) {
  case SurfEventReady :
    string = "SurfEventReady" ;
    break ;
  case SurfIntState :
    string = "SurfIntState" ;
    break ;
  case SurfBusInfo :
    string = "SurfBusInfo" ;
    break ;
  default :
    string = "Unknown" ;
    break ;
  }
  return string;
}

char *surfControlActionAsString(SurfControlAction_t action) {
  char* string ;
  switch(action) {
  case SurfClearAll :
    string = "SurfClearAll" ;
    break ;
  case SurfClearEvent :
    string = "SurfClearEvent" ;
    break ;
  case SurfClearHk :
    string = "SurfClearHk" ;
    break ;
  case SurfDataMode :
    string = "SurfDataMode" ;
    break ;
  case SurfHkMode :
    string = "SurfHkMode" ;
    break ;
  case SurfDisableInt :
    string = "SurfDisableInt" ;
    break ;
  case SurfEnableInt :
    string = "SurfEnableInt" ;
    break ;
  default :
    string = "Unknown" ;
    break ;
  }
  return string;
}


char *turfControlActionAsString(TurfControlAction_t action) {
  char* string ;
  switch(action) {
  case SetTrigMode :
    string = "SetTrigMode" ;
    break ;
  case RprgTurf :
    string = "RprgTurf" ;
    break ;
  case TurfLoadRam :
    string = "TurfLoadRam" ;
    break ;
  case SendSoftTrg : 
    string = "SendSoftTrg" ;
    break;
  case RFCBit : 
    string = "RFCBit" ;
    break;
  case RFCClk : 
    string = "RFCClk" ;
    break;	  	  	    
  case EnablePPS1Trig : 
    string = "EnablePPS1Trig";
    break;
  case EnablePPS2Trig : 
    string = "EnablePPS2Trig";
    break;
  case TurfClearAll : 
    string = "TurfClearAll";
    break;
  default :
    string = "Unknown" ;
    break ;
  }
  return string;
}


AcqdErrorCode_t setSurfControl(int surfId, SurfControlAction_t action)
{
  int retVal=0;  
  if(surfId>=numSurfs) {
    fprintf(stderr,"SURF Id: %d is out of range (0 - %d)\n",surfId,numSurfs-1);
    syslog(LOG_ERR,"SURF Id: %d is out of range (0 - %d)\n",surfId,numSurfs-1);
    return ACQD_E_CANTCOUNT;
  }  
  int surfFd=surfFds[surfId];
  if(surfFd<=0) {
    fprintf(stderr,"SURF FD %d is not a valid file descriptor\n",surfFd);
    syslog(LOG_ERR,"SURF FD %d is not a valid file descriptor\n",surfFd);
    return ACQD_E_BADFD;
  }
  
  switch (action) {
  case SurfClearAll : 
    retVal=ioctl(surfFd, SURF_IOCCLEARALL);
    break ; 
  case SurfClearEvent:
    retVal=ioctl(surfFd,SURF_IOCCLEAREVENT);
    break;
  case SurfClearHk:
    retVal=ioctl(surfFd,SURF_IOCCLEARHK);
    break;
  case SurfHkMode:
    retVal=ioctl(surfFd,SURF_IOCHK);
    break;
  case SurfDataMode:
    retVal=ioctl(surfFd,SURF_IOCDATA);
    break;
  case SurfEnableInt:
    retVal=ioctl(surfFd,SURF_IOCENABLEINT);
    break;
  case SurfDisableInt:
    retVal=ioctl(surfFd,SURF_IOCDISABLEINT);
    break;
  default :
    syslog(LOG_WARNING,"setSurfControl: undefined action %d",action);
    fprintf(stderr,"setSurfControl: undefined action, %d\n", action) ;
    return ACQD_E_UNKNOWN_CMD ;
  }
  if (verbosity>1 && printToScreen) 
    printf("setSurfControl: surf %d -- action %s\n",surfId,
	   surfControlActionAsString(action)) ; 
  if(retVal!=0) {
    fprintf(stderr,"setSurfControl: surf %d -- action %s resulted in error %s\n:",surfId,surfControlActionAsString(action),strerror(errno));
    syslog(LOG_ERR,"setSurfControl: surf %d -- action %s resulted in error %s\n:",surfId,surfControlActionAsString(action),strerror(errno));
    return ACQD_E_SET_ERR;
  }
  return ACQD_E_OK;
}


AcqdErrorCode_t setTurfControl(TurfControlAction_t action) {
  //These numbers are octal
  unsigned int baseClr          = 0022222222 ;
  unsigned int rprgTurf         = 0000000004 ;
  unsigned int loadRam          = 0000000040 ;
  unsigned int sendSoftTrig     = 0000000400 ;
  unsigned int rfcDbit          = 0004000000 ;
  unsigned int rfcClk           = 0000004000 ;
  unsigned int enablePPS1Trig   = 0000040000 ; //Real PPS from ADU5
  unsigned int enablePPS2Trig   = 0000400000 ; //Burst PPS up to 20Hz from G123
  unsigned int clearAll         = 0040000000 ;

  unsigned int           gpioVal,baseVal ; 
  AcqdErrorCode_t status=ACQD_E_OK;
  int i ;

  static int first=1;
  //    printf("Dec: %d %d %d\nHex: %x %x %x\nOct: %o %o %o\n",base_clr,rprg_turf,clr_trg,base_clr,rprg_turf,clr_trg,base_clr,rprg_turf,clr_trg);
  //    handleBadSigs(1000);
  if(first) {
    gpioVal=baseClr;
    status=setTurfGPIOValue(gpioVal);
    if(status!=ACQD_E_OK) {
      syslog(LOG_ERR,"Error setting TURFIO gpio to %o --%s",gpioVal,strerror(errno));
      fprintf(stderr,"Error setting TURFIO gpio to %o --%s",gpioVal,strerror(errno));
      return ACQD_E_TURFIO_GPIO;
    }
    if(printToScreen) {
      status=readTurfGPIOValue(&gpioVal);
      printf("First read of TURF GPIO: %o\n", status);
    }	
    first=0;
  }
  // Read current GPIO state
  status=readTurfGPIOValue(&baseVal);
  if(status!=ACQD_E_OK) {
    syslog(LOG_ERR,"Error reading TURFIO gpio -- %s",strerror(errno));
    fprintf(stderr,"Error reading TURFIO gpio -- %s",strerror(errno));
  }
        
  if(trigMode&TrigPPS1) baseVal |= enablePPS1Trig;
  if(trigMode&TrigPPS2) baseVal |= enablePPS2Trig;

  switch (action) {
  case SetTrigMode : gpioVal =baseVal; break;
  case RprgTurf : gpioVal = baseVal | rprgTurf ; break ;
  case TurfLoadRam : gpioVal = baseVal | loadRam ; break ;
  case SendSoftTrg : gpioVal = baseVal | sendSoftTrig ; break ;
  case RFCBit : gpioVal = (baseVal |= rfcDbit) | rfcClk ; break ;
  case RFCClk : gpioVal = (baseVal &= ~rfcDbit) | rfcClk ; break ;
  case EnablePPS1Trig : gpioVal = baseVal | enablePPS1Trig ; break ;
  case EnablePPS2Trig : gpioVal = baseVal | enablePPS2Trig ; break ;
  case TurfClearAll : gpioVal = baseVal | clearAll ; break ;
	    
  default :
    syslog(LOG_WARNING,"setTurfControl: undefined action %d",action);
    fprintf(stderr,"setTurfControl: undfined action, %d\n", action) ;
    return ACQD_E_UNKNOWN_CMD;
  }
    
  if (verbosity && printToScreen)
    printf("setTurfControl: action %s, gpioVal = %o\n",
	   turfControlActionAsString(action), gpioVal) ;
    
  if ((status=setTurfGPIOValue(gpioVal))!= ACQD_E_OK) {
    syslog(LOG_ERR,"setTurfControl: failed to set GPIO to %o\t(%s).\n", gpioVal,strerror(errno));
    fprintf(stderr,"setTurfControl: failed to set GPIO to %o\t(%s).\n", gpioVal,strerror(errno));
    return ACQD_E_TURFIO_GPIO ;
  }
    
  if (action > TurfClearAll) return ACQD_E_OK ;
  if (action == RprgTurf)  /* wait a while in case of RprgTurf. */
    for(i=0 ; i++<10 ; usleep(1)) ;
              
  return setTurfGPIOValue(baseVal);

}

AcqdErrorCode_t closeDevices() 
/*!
  Closes the SURF and TURFIO devices
*/
{
  AcqdErrorCode_t status=ACQD_E_OK;
  int surf=0,retVal=0;
  for(surf=0;surf<numSurfs;surf++) {
    retVal=close(surfFds[surf]);
    if(retVal<0) {
      syslog(LOG_ERR,"Error closing SURF %d -- %s\n",surfIndex[surf],strerror(errno));
      fprintf(stderr,"Error closing SURF %d -- %s\n",surfIndex[surf],strerror(errno));
      status=ACQD_E_CLOSE;
    }
  }
  retVal=close(turfioFd);
  if(retVal<0) {
    syslog(LOG_ERR,"Error closing TURFIO -- %s\n",strerror(errno));
    fprintf(stderr,"Error closing TURFIO -- %s\n",strerror(errno));
    status=ACQD_E_CLOSE;
  }
  return status;
}


AcqdErrorCode_t initializeDevices(int *numDevPtr)
/*! 
  Initializes the SURFs and TURFIO, returns the number of devices initialized.
*/
{
  int surf,countSurfs=0;
  int countDevices=0;
  int testFd=0;
  char devName[FILENAME_MAX];
  int errorCounter=0;

  //Reset surfMask
  surfMask=0;
  
  for(surf=0;surf<MAX_SURFS;surf++) {
    //Do we wnat this SURF??
    if(surfPos[surf].bus<0 || surfPos[surf].slot<0) continue;
    sprintf(devName,"/dev/%02d:%02d",surfPos[surf].bus,surfPos[surf].slot);
    testFd = open(devName, O_RDWR | O_NONBLOCK);
    if (testFd < 0) {
      fprintf(stderr,"Couldn't open SURF %d -- Device %s (%s)\n ",surf+1,devName,strerror(errno));
      syslog(LOG_ERR,"Couldn't open SURF %d -- Device %s (%s)\n",surf+1,devName,strerror(errno));
      errorCounter++;
    }
    else {
      if(printToScreen && verbosity) 
	printf("Opened SURF %d -- Device %s\n",surf+1,devName);
      surfFds[countSurfs]=testFd;
      surfIndex[countSurfs]=surf+1;
      if(hdPtr) {
	surfMask|=(1<<surf);
      }
      countSurfs++;
    }
  }
  countDevices=countSurfs;
  
  //Now try and open TURFIO
  if(turfioPos.bus<0 || turfioPos.slot<0) 
    return ACQD_E_NO_TURFIO;
  sprintf(devName,"/dev/turfio");
  testFd = open(devName, O_RDWR);
  if (testFd < 0) {
    fprintf(stderr,"Couldn't open TURFIO -- Device %s (%s)\n ",devName,strerror(errno));
    syslog(LOG_ERR,"Couldn't open TURFIO -- Device %s (%s)\n",devName,strerror(errno));
    errorCounter++;
    return ACQD_E_NO_TURFIO;
  }
  else {
    turfioFd=testFd;
    countDevices++;
  }
  *numDevPtr=countDevices ;
  if(errorCounter==0)
    return ACQD_E_OK;
  return ACQD_E_MISSING_SURF;
}

int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(ACQD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,ACQD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(ACQD_PID_FILE);
  return 0;
}




int readConfigFile() 
/* Load Acqd config stuff */
{
  /* Config file thingies */
  int status=0,count,surf,dac;
  char keyName[180];
  int tempNum=12;
  int surfSlots[MAX_SURFS];
  int surfBuses[MAX_SURFS];
  int surfSlotCount=0;
  int surfBusCount=0;
  KvpErrorCode kvpStatus=0;
  char* eString ;

  kvpReset();
  status = configLoad (GLOBAL_CONF_FILE,"global") ;
  status += configLoad ("Acqd.config","output") ;
  status += configLoad ("Acqd.config","locations") ;
  if(status != CONFIG_E_OK)
    fprintf(stderr,"Problem Reading Acqd.config -- locations\n");
  status += configLoad ("Acqd.config","debug") ;
  if(status != CONFIG_E_OK)
    fprintf(stderr,"Problem Reading Acqd.config -- debug\n");
  status += configLoad ("Acqd.config","trigger") ;
  if(status != CONFIG_E_OK)
    fprintf(stderr,"Problem Reading Acqd.config -- trigger\n");
  status += configLoad ("Acqd.config","thresholds") ;
  if(status != CONFIG_E_OK)
    fprintf(stderr,"Problem Reading Acqd.config -- thresholds\n");
  status += configLoad ("Acqd.config","thresholdScan") ;
  if(status != CONFIG_E_OK)
    fprintf(stderr,"Problem Reading Acqd.config -- thresholdScan\n");
  status += configLoad ("Acqd.config","acqd") ;
  if(status != CONFIG_E_OK)
    fprintf(stderr,"Problem Reading Acqd.config -- acq\n");
  status += configLoad ("Acqd.config","pedestal") ;
  if(status != CONFIG_E_OK)
    fprintf(stderr,"Problem Reading Acqd.config -- pedestal\n");
  //    printf("Debug rc1\n");
  if(status == CONFIG_E_OK) {
    eventsBeforeClear=kvpGetInt("eventsBeforeClear",1000);
    numEvents=kvpGetInt("numEvents",1);
    hkDiskBitMask=kvpGetInt("hkDiskBitMask",1);
    niceValue=kvpGetInt("niceValue",-20);
    pedestalMode=kvpGetInt("pedestalMode",0);
    pedUsePPS2Trig=kvpGetInt("pedUsePPS2Trig",0);
    pedSoftTrigSleepMs=kvpGetInt("pedSoftTrigSleepMs",0);
    useInterrupts=kvpGetInt("useInterrupts",0);
    surfFirmwareVersion=kvpGetInt("surfFirmwareVersion",2);
    turfFirmwareVersion=kvpGetInt("turfFirmwareVersion",2);
    writeRawScalers=kvpGetInt("writeRawScalers",1);
    //	printf("%s\n",acqdEventDir);


    //	printf("Debug rc2\n");
    printToScreen=kvpGetInt("printToScreen",0);
    if(addedVerbosity && printToScreen==0) {
      printToScreen=1;
      addedVerbosity--;
    }
    printPidStuff=kvpGetInt("printPidStuff",1);
    surfHkPeriod=kvpGetInt("surfHkPeriod",1);
    surfHkTelemEvery=kvpGetInt("surfHkTelemEvery",1);
    surfHkAverage=kvpGetInt("surfHkAverage",1);
    turfRateAverage=kvpGetInt("turfRateAverage",60);
    if(printToScreen) 
      printf("surfHkAverage %d\nturfRateAverage %d\n",surfHkAverage,turfRateAverage);
    turfRateTelemEvery=kvpGetInt("turfRateTelemEvery",1);
    turfRateTelemInterval=kvpGetInt("turfRateTelemInterval",1);
    //	printf("HK surfPeriod %d\nturfRateTelemEvery %d\nturfRateTelemInterval %d\n",surfHkPeriod,turfRateTelemEvery,turfRateTelemInterval);
    dontWaitForEvtF=kvpGetInt("dontWaitForEvtF",0);
    dontWaitForLabF=kvpGetInt("dontWaitForLabF",0);
    sendSoftTrigger=kvpGetInt("sendSoftTrigger",0);
    softTrigPeriod=kvpGetInt("softTrigPeriod",0);
    reprogramTurf=kvpGetInt("reprogramTurf",0);
    //	printf("dontWaitForEvtF: %d\n",dontWaitForEvtF);
    standAloneMode=kvpGetInt("standAloneMode",0);
    if(!writeData)
      writeData=kvpGetInt("writeData",1);
    writeFullHk=kvpGetInt("writeFullHk",1);
    justWriteHeader=kvpGetInt("justWriteHeader",0);
    if(!standAloneMode) 
      verbosity=kvpGetInt("verbosity",0);
    else
      verbosity=kvpGetInt("verbosity",0)+addedVerbosity;
    pps1TrigFlag=kvpGetInt("enablePPS1Trigger",1);
    pps2TrigFlag=kvpGetInt("enablePPS2Trigger",1);
    doThresholdScan=kvpGetInt("doThresholdScan",0);
    doSingleChannelScan=kvpGetInt("doSingleChannelScan",0);
    surfForSingle=kvpGetInt("surfForSingle",0);
    dacForSingle=kvpGetInt("dacForSingle",0);
    thresholdScanStepSize=kvpGetInt("thresholdScanStepSize",0);
    thresholdScanPointsPerStep=kvpGetInt("thresholdScanPointsPerStep",0);
    threshSwitchConfigAtEnd=kvpGetInt("threshSwitchConfigAtEnd",1);
    setGlobalThreshold=kvpGetInt("setGlobalThreshold",0);
    globalThreshold=kvpGetInt("globalThreshold",0);
    dacPGain=kvpGetFloat("dacPGain",0);
    dacIGain=kvpGetFloat("dacIGain",0);
    dacDGain=kvpGetFloat("dacDGain",0);
    dacIMin=kvpGetInt("dacIMin",0);
    dacIMax=kvpGetInt("dacIMax",0);
    enableChanServo=kvpGetInt("enableChanServo",1);
    if(doThresholdScan)
      enableChanServo=0;
    pidPanicVal=kvpGetInt("pidPanicVal",200);
    pidAverage=kvpGetInt("pidAverage",1);
    if(pidAverage<1) pidAverage=1;
    if(printToScreen<0) {
      syslog(LOG_WARNING,"Couldn't fetch printToScreen, defaulting to zero");
      printToScreen=0;	    
    }
    if(standAloneMode<0) {
      syslog(LOG_WARNING,"Couldn't fetch standAloneMode, defaulting to zero");
      standAloneMode=0;	    
    }
    tempNum=2;
    antTrigMask=kvpGetUnsignedInt("antTrigMask",0);
    nadirAntTrigMask=kvpGetUnsignedInt("nadirAntTrigMask",0);
	
    if(printToScreen) printf("Print to screen flag is %d\n",printToScreen);

    turfioPos.bus=kvpGetInt("turfioBus",3); //in seconds
    turfioPos.slot=kvpGetInt("turfioSlot",10); //in seconds
    tempNum=BANDS_PER_ANT;
    kvpStatus=kvpGetIntArray("pidGoals",&pidGoals[0],&tempNum);
    if(kvpStatus!=KVP_E_OK) {
      syslog(LOG_WARNING,"kvpGetIntArray(pidGoals): %s",
	     kvpErrorString(kvpStatus));
      fprintf(stderr,"kvpGetIntArray(pidGoals): %s\n",
	      kvpErrorString(kvpStatus));
    }      
    tempNum=ACTIVE_SURFS;
    kvpStatus=kvpGetIntArray("surfTrigBandMasks",&surfTrigBandMasks[0],&tempNum);
    if(kvpStatus!=KVP_E_OK) {
      syslog(LOG_WARNING,"kvpGetIntArray(surfTrigBandMasks): %s",
	     kvpErrorString(kvpStatus));
      fprintf(stderr,"kvpGetIntArray(surfTrigBandMasks): %s\n",
		kvpErrorString(kvpStatus));
    }


    tempNum=12;
    kvpStatus = kvpGetIntArray("surfBus",surfBuses,&tempNum);
    if(kvpStatus!=KVP_E_OK) {
      syslog(LOG_WARNING,"kvpGetIntArray(surfBus): %s",
	     kvpErrorString(kvpStatus));
      if(printToScreen)
	fprintf(stderr,"kvpGetIntArray(surfBus): %s\n",
		kvpErrorString(kvpStatus));
    }
    else {
      for(count=0;count<tempNum;count++) {
	surfPos[count].bus=surfBuses[count];
	if(surfBuses[count]!=-1) {
	  surfBusCount++;
	}
      }
    }
    tempNum=12;
    kvpStatus = kvpGetIntArray("surfSlot",surfSlots,&tempNum);
    if(kvpStatus!=KVP_E_OK) {
      syslog(LOG_WARNING,"kvpGetIntArray(surfSlot): %s",
	     kvpErrorString(kvpStatus));
      if(printToScreen)
	fprintf(stderr,"kvpGetIntArray(surfSlot): %s\n",
		kvpErrorString(kvpStatus));
    }
    else {
      for(count=0;count<tempNum;count++) {
	surfPos[count].slot=surfSlots[count];
	if(surfSlots[count]!=-1) {
	  surfSlotCount++;
	}
      }
    }
    if(surfSlotCount==surfBusCount) numSurfs=surfBusCount;
    else {
      syslog(LOG_ERR,"Confused config file bus and slot count do not agree");
      if(printToScreen)
	fprintf(stderr,"Confused config file bus and slot count do not agree\n");
      handleBadSigs(1000);
    }
	
    tempNum=12;
    kvpStatus = kvpGetIntArray("dacSurfs",dacSurfs,&tempNum);
    if(kvpStatus!=KVP_E_OK) {
      syslog(LOG_WARNING,"kvpGetIntArray(dacSurfs): %s",
	     kvpErrorString(kvpStatus));
      if(printToScreen)
	fprintf(stderr,"kvpGetIntArray(dacSurfs): %s\n",
		kvpErrorString(kvpStatus));
    }
	
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
      for(dac=0;dac<SCALERS_PER_SURF;dac++) {
	thresholdArray[surf][dac]=0;
	pidGoalScaleFactors[surf][dac]=0;
      }
      if(dacSurfs[surf]) {		
	sprintf(keyName,"threshSurf%d",surf+1);
	tempNum=SCALERS_PER_SURF;	       
	kvpStatus = kvpGetIntArray(keyName,&(thresholdArray[surf][0]),&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	  syslog(LOG_WARNING,"kvpGetIntArray(%s): %s",keyName,
		 kvpErrorString(kvpStatus));
	  if(printToScreen)
	    fprintf(stderr,"kvpGetIntArray(%s): %s\n",keyName,
		    kvpErrorString(kvpStatus));
	}

	sprintf(keyName,"pidGoalScaleSurf%d",surf+1);
	tempNum=SCALERS_PER_SURF;	       
	kvpStatus = kvpGetFloatArray(keyName,&(pidGoalScaleFactors[surf][0]),&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	  syslog(LOG_WARNING,"kvpGetFloatArray(%s): %s",keyName,
		 kvpErrorString(kvpStatus));
	  if(printToScreen)
	    fprintf(stderr,"kvpGetFloatArray(%s): %s\n",keyName,
		    kvpErrorString(kvpStatus));
	}

      }
    }
    	
    //Pedestal things
    writeDebugData=kvpGetInt("writeDebugData",1);
    numPedEvents=kvpGetInt("numPedEvents",4000);
    pedSwitchConfigAtEnd=kvpGetInt("pedSwitchConfigAtEnd",1);
	
	
  }
  else {
    eString=configErrorString (status) ;
    syslog(LOG_ERR,"Error reading 1 Acqd.config: %s\n",eString);
    //	if(printToScreen)
    fprintf(stderr,"Error reading 1 Acqd.config: %s\n",eString);
	    
  }

    
  kvpReset();
  status = configLoad ("Acqd.config","rates");
  if(status == CONFIG_E_OK) {
    enableRateServo=kvpGetInt("enableRateServo",0);
    servoRateCalcPeriod=kvpGetInt("servoRateCalcPeriod",60);
    rateGoal=kvpGetFloat("rateGoal",5);
    ratePGain=kvpGetFloat("ratePGain",100);
    rateIGain=kvpGetFloat("rateIGain",1);
    rateDGain=kvpGetFloat("rateDGain",1);
    rateIMax=kvpGetFloat("rateIMax",100);
    rateIMin=kvpGetFloat("rateIMin",-100);
  }
  else {
    eString=configErrorString (status) ;
    syslog(LOG_ERR,"Error reading Acqd.config <rates>: %s\n",eString);
    fprintf(stderr,"Error reading Acqd.config <rates>: %s\n",eString);
  }

  kvpReset();
  status = configLoad ("Acqd.config","starttest");
  if(status == CONFIG_E_OK) {
    enableStartTest=kvpGetInt("enableStartTest",1);
    startSoftTrigs=kvpGetInt("startSoftTrigs",10);
  }
  else {
    eString=configErrorString(status);
    syslog(LOG_ERR,"Error reading Acqd.config <starttest>: %s\n",eString);
    fprintf(stderr,"Error reading Acqd.config <starttest>: %s\n",eString);
  }


  if(printToScreen) {
    printf("Read Config File\n");
    printf("doThresholdScan %d\n",doThresholdScan);
    printf("pedestalMode %d\n",pedestalMode);
    printf("enableStartTest %d\n",enableStartTest);
    printf("writeData %d",writeData);
    if(writeData) {
      if(useAltDir) 
	printf("\t--\t%s",altOutputdir);
      else
	printf("\t--\t%s",ACQD_EVENT_DIR);
    }
    printf("\n");
    printf("writeFullHk %d\n",writeFullHk);

  }   
	    
	    

    
  //    printf("Debug rc3\n");
  return status;
}

AcqdErrorCode_t sendClearEvent()
{
  //Sends SurfClearEvent to all SURFs but in the correct order
  AcqdErrorCode_t status=ACQD_E_OK;
  int surf;
  for(surf=0;surf<numSurfs;surf++) {
    //Send clear pulse to SURF 2 last
    if(surf!=surfBusyWatch) {
      if (setSurfControl(surf, SurfClearEvent) != ACQD_E_OK) {
	fprintf(stderr,"Failed to send clear event pulse on SURF %d.\n",surfIndex[surf]) ;
	syslog(LOG_ERR,"Failed to send clear event pulse on SURF %d.\n",surfIndex[surf]) ;
      }
    }
  }
  //Now send clear pulse to SURF 2
  surf=surfBusyWatch;
  if (setSurfControl(surf, SurfClearEvent) != ACQD_E_OK) {
    fprintf(stderr,"Failed to send clear event pulse on SURF %d.\n",surfIndex[surf]) ;
    syslog(LOG_ERR,"Failed to send clear event pulse on SURF %d.\n",surfIndex[surf]) ;
  }
  return status;
}



AcqdErrorCode_t clearDevices() 
// Clears boards for start of data taking 
{
  //    int testVal=0;
  int i;
  AcqdErrorCode_t status;

  if(verbosity>=0 && printToScreen) printf("*** Clearing devices ***\n");
    
  //Added RJN 24/11/06
  trigMode=TrigNone;
  if(verbosity && printToScreen)
    fprintf(stderr,"Setting trigMode to TrigNone\n");
  if (setTurfControl(SetTrigMode) != ACQD_E_OK) {
    syslog(LOG_ERR,"Failed to set trigger mode to none on TURF\n") ;
    fprintf(stderr,"Failed to set trigger mode to none on TURF\n");
  }

  antTrigMask=0xffffffff;    
  nadirAntTrigMask=0xff;
  //Mask off all antennas
  if(verbosity && printToScreen)
    fprintf(stderr,"Masking off antennas\n");
  status=writeAntTrigMask();
  if(status!=ACQD_E_OK) {
    syslog(LOG_ERR,"Failed to write antenna trigger mask\n");
    fprintf(stderr,"Failed to write antenna trigger mask\n");
  }

  //Added RJN 29/11/06
  if(verbosity && printToScreen)
    fprintf(stderr,"Setting surf thresholds to 1\n");
  status=setGloablDACThreshold(1); 
  if(status!=ACQD_E_OK) {
    syslog(LOG_ERR,"Failed to set global DAC thresholds\n");
    fprintf(stderr,"Failed to set global DAC thresholds\n");
  }   


  // Prepare TURF board
  if(verbosity && printToScreen)
    fprintf(stderr,"Sending Clear All pulse to TURF\n");
  if (setTurfControl(TurfClearAll) != ACQD_E_OK) {
    syslog(LOG_ERR,"Failed to send clear pulse on TURF \n") ;
    fprintf(stderr,"Failed to send clear pulse on TURF \n") ;
  }


  // Prepare SURF boards
  int tempInd;
  for(tempInd=0;tempInd<numSurfs;tempInd++) {
    //	i=surfClearIndex[tempInd];
    i=tempInd;
    if(i!=surfBusyWatch) {
      //Here was the strange SP0 thing do we still need this??
      if(verbosity && printToScreen)
	fprintf(stderr,"Sending clearAll to SURF %d\n",surfIndex[i]);
      if (setSurfControl(i, SurfClearAll) != ACQD_E_OK) {
	syslog(LOG_ERR,"Failed to send clear all event pulse on SURF %d.\n",surfIndex[i]) ;
	fprintf(stderr,"Failed to send clear all event pulse on SURF %d.\n",surfIndex[i]) ;
      }
    }
  }
  i=surfBusyWatch;
  if(verbosity && printToScreen)
    fprintf(stderr,"Sending clearAll to SURF %d\n",surfIndex[i]);
  if (setSurfControl(i, SurfClearAll) != ACQD_E_OK) {
    syslog(LOG_ERR,"Failed to send clear all event pulse on SURF %d.\n",surfIndex[i]) ;
    fprintf(stderr,"Failed to send clear all event pulse on SURF %d.\n",surfIndex[i]) ;
  }
  //Re mask all antennas
  if(verbosity && printToScreen)
    fprintf(stderr,"Masking off antennas\n");
  antTrigMask=0xffffffff;    
  nadirAntTrigMask=0xff;
  status=writeAntTrigMask();
  if(status!=ACQD_E_OK) {
    syslog(LOG_ERR,"Failed to write antenna trigger mask\n");
    fprintf(stderr,"Failed to write antenna trigger mask\n");
  }
  return ACQD_E_OK;
}



/* parse command arg
   uments to initialize parameter(s). */
int init_param(int argn, char **argv,  int *n, unsigned short *dacVal) {

  while (--argn) {
    if (**(++argv) == '-') {
      switch (*(*argv+1)) {
      case 'o': oldStyleFiles=TRUE ; break ;
      case 'v': addedVerbosity++ ; break ;
      case 'w': writeData = TRUE; break;
      case 'a': *dacVal=(unsigned short)atoi(*(++argv)) ; --argn ; break ;
      case 'r': reprogramTurf = TRUE ; break ;
      case 'd': 
	strncpy(altOutputdir,*(++argv),FILENAME_MAX);
	useAltDir=1;
	--argn ; break ;
      case 'n': sscanf(*(++argv), "%d", n) ;
	--argn ; break ;
      default : printf(" init_param: undefined parameter %s\n", *argv) ;
      }
    }
  }

  if (writeData && printToScreen) 
    printf(" data will be saved into disk.\n") ;  
  return 0;
}

void fillDacValBufferGlobal(unsigned int obuffer[MAX_SURFS][34], unsigned short val)
{
  memset(obuffer,0,sizeof(int)*MAX_SURFS*34);
  int dac,surf,index;
  unsigned int mask[2]={0};
  for(surf=0;surf<numSurfs;surf++) {
    mask[0]=0;
    mask[1]=0;
    for (dac=0;dac<RAW_SCALERS_PER_SURF;dac++) {
      index=rawScalerToLogicScaler[dac];
      //      printf("surf %d, surfTrigBandMasks %d, dac %d, index %d\n",
      //	     surf,surfTrigBandMasks[surfIndex[surf]-1],dac,index);
      if(index>=0) {
	obuffer[surf][dac] = (dac<<16) + val; //Actual channel
	if(dac<16) {
	  if(surfTrigBandMasks[surfIndex[surf]-1]&(1<<index))
	    mask[0] |= (1<<dac);
	}
	else {
	  if(surfTrigBandMasks[surfIndex[surf]-1]&(1<<index))
	    mask[1] |= (1<<(dac-16));
	}		      
      }
      else {
	obuffer[surf][dac] = (dac<<16) + 0; //Not an actual channel	
      }
    }    
    //    fprintf(stderr,"surf %d mask %#x\t%#x\n",surf,mask[0],mask[1]);
    obuffer[surf][32] = (32 << 16) + mask[0];
    obuffer[surf][33] = (33 << 16) + mask[1];
  }
}


void fillDacValBuffer(unsigned int obuffer[MAX_SURFS][34])
{
  memset(obuffer,0,sizeof(int)*MAX_SURFS*34);
  int dac,surf,index;
  unsigned int mask[2]={0};
  for(surf=0;surf<numSurfs;surf++) {
    mask[0]=0;
    mask[1]=0;
    for (dac=0;dac<RAW_SCALERS_PER_SURF;dac++) {
      index=rawScalerToLogicScaler[dac];
      if(index>=0) {
	obuffer[surf][dac] = (dac<<16) + (thresholdArray[surfIndex[surf]-1][index]&0xfff);
	if(dac<16) {
	  if(surfTrigBandMasks[surfIndex[surf]-1]&(1<<index))
	    mask[0] |= (1<<dac);
	}
	else {
	  if(surfTrigBandMasks[surfIndex[surf]-1]&(1<<index))
	    mask[1] |= (1<<(dac-16));

	}	
      }
      else {
	obuffer[surf][dac] = (dac<<16) + 0;	   
      }
    }

    obuffer[surf][32] = (32 << 16) + mask[0];
    obuffer[surf][33] = (33 << 16) + mask[1];
  }
}

AcqdErrorCode_t writeDacValBuffer(int surfId, unsigned int *obuffer) {
  if(surfId>=numSurfs) {
    fprintf(stderr,"SURF Id: %d is out of range (0 - %d)\n",surfId,numSurfs-1);
    syslog(LOG_ERR,"SURF Id: %d is out of range (0 - %d)\n",surfId,numSurfs-1);
    return ACQD_E_CANTCOUNT;
  }  
  int surfFd=surfFds[surfId];
  if(surfFd<=0) {
    fprintf(stderr,"SURF FD %d is not a valid file descriptor\n",surfFd);
    syslog(LOG_ERR,"SURF FD %d is not a valid file descriptor\n",surfFd);
    return ACQD_E_BADFD;
  }
  unsigned int count;
  count = write(surfFd, obuffer, 34*sizeof(int));
  if (count < 0) {
    syslog(LOG_ERR,"Error writing thresholds -- SURF %d  (%s)\n",surfId,strerror(errno));
    fprintf(stderr,"Error writing thresholds -- SURF %d  (%s)\n",surfId,strerror(errno));
  }
  ioctl(surfFd, SURF_IOCCLEARHK);
  return ACQD_E_OK;
}


AcqdErrorCode_t setGloablDACThreshold(unsigned short threshold) {
  unsigned int outBuffer[MAX_SURFS][RAW_SCALERS_PER_SURF+2];
  int surf ;
  if(verbosity && printToScreen) printf("Writing thresholds to SURFs\n");
  
  fillDacValBufferGlobal(outBuffer,threshold);  
  for(surf=0;surf<numSurfs;surf++) {
    writeDacValBuffer(surf,outBuffer[surf]);
  }
  return ACQD_E_OK;
}



AcqdErrorCode_t setDACThresholds() {
  unsigned int outBuffer[MAX_SURFS][RAW_SCALERS_PER_SURF+2];
  int surf ;
  if(verbosity && printToScreen) printf("Writing thresholds to SURFs\n");
  
  fillDacValBuffer(outBuffer);
  for(surf=0;surf<numSurfs;surf++) {
    writeDacValBuffer(surf,outBuffer[surf]);
  }
  return ACQD_E_OK;
}


AcqdErrorCode_t runPedestalMode()
{
  //This is the function that takes triggers using the software trigger
  //and uses the accumulated waveforms to write out a new pedestal file
  AcqdErrorCode_t status=ACQD_E_OK;
  unsigned int tmo=0;
  int eventReadyFlag=0;
  struct timeval timeStruct;
  doingEvent=0;
  writeData=writeDebugData;
  numEvents=numPedEvents;
  if(!pedUsePPS2Trig) {
    sendSoftTrigger=1;
    softTrigPeriod=0;
  }
  resetPedCalc();

  //Now have an event loop
  currentState=PROG_STATE_RUN;
  if(printToScreen)
    printf("Running pedestal mode with %d events\n",numEvents);

  while(currentState==PROG_STATE_RUN && doingEvent<numEvents) {
    //This is the start of the main event loop
    //Fill theEvent with zeros 
    bzero(&theEvent, sizeof(theEvent)) ;
    
    //Send software trigger 	
    if(!pedUsePPS2Trig) {
      setTurfControl(SendSoftTrg);
      if(pedSoftTrigSleepMs)
	myUsleep(1000*pedSoftTrigSleepMs);
    }
   
    // Wait for ready to read (EVT_RDY) 
    tmo=0;	    
    do {
      //Check if we wnat to break out
      if(currentState!=PROG_STATE_RUN) 
	break;
      //Need to make the SURF that we read the GPIO value from
      //somehow changeable
      eventReadyFlag=0;
      status=getSurfStatusFlag(0,SurfEventReady,&eventReadyFlag);
      if(status!=ACQD_E_OK) {
	fprintf(stderr,"Error reading GPIO value from SURF 0\n");
	syslog(LOG_ERR,"Error reading GPIO value from SURF 0\n");
	//Do what??
      }		      
      tmo++;      
    } while(!(eventReadyFlag) && tmo<EVTF_TIMEOUT);
    
    //Check if we are trying to leave the event loop
    if(currentState!=PROG_STATE_RUN) 
      continue;
    
    if (tmo==EVTF_TIMEOUT){
      syslog(LOG_WARNING,"Timed out waiting for EVT_RDY flag");
      fprintf(stderr,"Timed out (%d ms) while waiting for EVT_RDY flag in self trigger mode.\n", tmo) ;
      continue;		
    }

    //Now load event into ram
    if (setTurfControl(TurfLoadRam) != ACQD_E_OK) {
      fprintf(stderr,"Failed to send load TURF event to TURFIO.\n") ;
      syslog(LOG_ERR,"Failed to send load TURF event to TURFIO.\n") ;
    }

    gettimeofday(&timeStruct,NULL);
	    
    //Now actually read the event data
    status+=readSurfEventData();
    if(status!=ACQD_E_OK) {
      fprintf(stderr,"Error reading SURF event data %d.\n",doingEvent);
      syslog(LOG_ERR,"Error reading SURF event data %d.\n",doingEvent);
    }
	      
    hdPtr->unixTime=timeStruct.tv_sec;
    hdPtr->unixTimeUs=timeStruct.tv_usec;
    if(verbosity && printToScreen) printf("Read SURF Labrador Data\n");

    //Now read TURF event data (not strictly necessary for pedestal run
    if(turfFirmwareVersion==3)
      status=readTurfEventDataVer3();
    else
      status=readTurfEventDataVer5();
    turfRates.unixTime=timeStruct.tv_sec;
    if(status!=ACQD_E_OK) {
      fprintf(stderr,"Problem reading TURF event data\n");
      syslog(LOG_ERR,"Problem reading TURF event data\n");
    }
    if(verbosity && printToScreen) printf("Done reading\n");
	    	    
    //Error checking
    if(status!=ACQD_E_OK) {
      //We have an error
      fprintf(stderr,"Error detected %d, during event %d",status,doingEvent);
      syslog(LOG_ERR,"Error detected %d, during event %d",status,doingEvent);	      
      status=ACQD_E_OK;
    }
    else if(doingEvent>=0) { //RJN changed 24/11/06 for test
      hdPtr->eventNumber=getEventNumber();
      bdPtr->eventNumber=hdPtr->eventNumber;
      //      hdPtr->surfMask=surfMask;
      hdPtr->antTrigMask=(unsigned int)antTrigMask;	       
      hdPtr->nadirAntTrigMask=(unsigned char)nadirAntTrigMask;
      //Filled by Eventd
      //hdPtr->gpsSubTime;
      //hdPtr->calibStatus;
      
      if(printToScreen && verbosity) 
	printf("Event:\t%u\nSec:\t%d\nMicrosec:\t%d\nTrigTime:\t%u\n",hdPtr->eventNumber,hdPtr->unixTime,hdPtr->unixTimeUs,turfioPtr->trigTime);
      

      //Add the events to the pedestals
      addEventToPedestals(bdPtr);

      if(writeData) outputEventData();
		    
      if(doingEvent%100==0)
	printf("Done %d events\n",doingEvent);
    }		    
    // Clear boards
    sendClearEvent();
    
  }

  writePedestals();
  if(pedSwitchConfigAtEnd) {
    //Arrrghhh		
    CommandStruct_t theCmd;
    theCmd.numCmdBytes=3;
    theCmd.cmd[0]=LAST_CONFIG;
    theCmd.cmd[1]=1;
    theCmd.cmd[2]=0;
    writeCommandAndLink(&theCmd);	
    theCmd.numCmdBytes=1;
    theCmd.cmd[0]=CMD_START_NEW_RUN;
    writeCommandAndLink(&theCmd);
  }
  return status;
}

AcqdErrorCode_t doStartTest()
{
  AcqdErrorCode_t status=ACQD_E_OK;
  int dacVal=0,chanId,chan,surf,dac,retVal;
  int tInd=0,firstLoop=1,tmo=0,eventReadyFlag=0;
  struct timeval timeStruct;
  unsigned int tempTrigMask=antTrigMask;
  unsigned int tempNadirTrigMask=nadirAntTrigMask;
  AcqdStartStruct_t startStruct;
  float chanMean[ACTIVE_SURFS][CHANNELS_PER_SURF];
  float chanRMS[ACTIVE_SURFS][CHANNELS_PER_SURF]; 
  char theFilename[FILENAME_MAX];
  int tempGlobalThreshold=setGlobalThreshold;
  setGlobalThreshold=1;
  startStruct.numEvents=0;
  memset(chanMean,0,sizeof(float)*ACTIVE_SURFS*CHANNELS_PER_SURF);
  memset(chanRMS,0,sizeof(float)*ACTIVE_SURFS*CHANNELS_PER_SURF);

  //Disable triggers
  antTrigMask=0xffffffff;
  nadirAntTrigMask=0xff;
  writeAntTrigMask(); 
  antTrigMask=tempTrigMask;
  nadirAntTrigMask=tempNadirTrigMask;

  doingEvent=0;
  //Now have an event loop
  currentState=PROG_STATE_RUN;
  if(printToScreen)
    printf("Inside doStartTest\n");

  gettimeofday(&timeStruct,NULL);
  startStruct.unixTime=timeStruct.tv_sec;
  
  while(currentState==PROG_STATE_RUN && doingEvent<startSoftTrigs) {
    //Fill theEvent with zeros 
    bzero(&theEvent, sizeof(theEvent)) ;    
    //Send software trigger 	
    setTurfControl(SendSoftTrg);   
    // Wait for ready to read (EVT_RDY) 
    tmo=0;	    
    do {
      //Check if we want to break out
      if(currentState!=PROG_STATE_RUN) 
	break;
      //Need to make the SURF that we read the GPIO value from
      //somehow changeable
      eventReadyFlag=0;
      status=getSurfStatusFlag(0,SurfEventReady,&eventReadyFlag);
      if(status!=ACQD_E_OK) {
	fprintf(stderr,"Error reading GPIO value from SURF 0\n");
	syslog(LOG_ERR,"Error reading GPIO value from SURF 0\n");
	//Do what??
      }		      
      tmo++;      
    } while(!(eventReadyFlag) && tmo<EVTF_TIMEOUT);
    
    //Check if we are trying to leave the event loop
    if(currentState!=PROG_STATE_RUN) 
      continue;
    
    if (tmo==EVTF_TIMEOUT){
      syslog(LOG_WARNING,"Timed out waiting for EVT_RDY flag");
      fprintf(stderr,"Timed out (%d ms) while waiting for EVT_RDY flag in self trigger mode.\n", tmo) ;
      continue;		
    }

    //Now load event into ram
    if (setTurfControl(TurfLoadRam) != ACQD_E_OK) {
      fprintf(stderr,"Failed to send load TURF event to TURFIO.\n") ;
      syslog(LOG_ERR,"Failed to send load TURF event to TURFIO.\n") ;
    }
	    
    //Now actually read the event data
    status+=readSurfEventData();
    if(status!=ACQD_E_OK) {
      fprintf(stderr,"Error reading SURF event data %d.\n",doingEvent);
      syslog(LOG_ERR,"Error reading SURF event data %d.\n",doingEvent);
    }
    
    if(verbosity && printToScreen) printf("Read SURF Labrador Data\n");

    //Now read TURF event data (not strictly necessary for pedestal run
    if(turfFirmwareVersion==3)
      status=readTurfEventDataVer3();
    else
      status=readTurfEventDataVer5();
    if(status!=ACQD_E_OK) {
      fprintf(stderr,"Problem reading TURF event data\n");
      syslog(LOG_ERR,"Problem reading TURF event data\n");
    }
    if(verbosity && printToScreen) printf("Done reading\n");
	    	    
    //Error checking
    if(status!=ACQD_E_OK) {
      //We have an error
      fprintf(stderr,"Error detected %d, during event %d",status,doingEvent);
      syslog(LOG_ERR,"Error detected %d, during event %d",status,doingEvent);	      
      status=ACQD_E_OK;
    }
    else if(doingEvent>=0) { 
      //Actually add event to running means
      subtractCurrentPeds(bdPtr,&pedSubBody);
      startStruct.numEvents++;
      for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	  chanId=GetChanIndex(surf,chan);
	  //	  printf("%d %d -- %d %d\n",surf,chanId,bdPtr->channel[chanId].data[0],
	  //		 pedSubBody.channel[chanId].data[0]);
	  chanMean[surf][chan]+=pedSubBody.channel[chanId].mean;
	  chanRMS[surf][chan]+=pedSubBody.channel[chanId].rms;
	}
      }
      //      printf("Done %d events\n",doingEvent);
    }		    
    // Clear boards
    sendClearEvent();    
  }
  
  //Now have obtained our N events
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
      chanMean[surf][chan]/=startStruct.numEvents;
      chanRMS[surf][chan]/=startStruct.numEvents;
      startStruct.chanMean[surf][chan]=chanMean[surf][chan];
      startStruct.chanRMS[surf][chan]=chanRMS[surf][chan];
    }
  }

  //Now do out silly little threshold scan
  for(tInd=0;tInd<10;tInd++) {
    dacVal=2000 + (200*tInd);
    if(printToScreen) 
      printf("Setting Threshold -- %d\r",dacVal);
    setGloablDACThreshold(dacVal);
    if(firstLoop) {
      usleep(30000);
      //      firstLoop=0;
    }
    startStruct.threshVals[tInd]=dacVal;
    theSurfHk.globalThreshold=dacVal;
    status=readSurfHkData(); 
    theSurfHk.globalThreshold=0; //Just to avoid error message
    //Then send clear hk pulse
    for(surf=0;surf<numSurfs;++surf) {
      if (setSurfControl(surf,SurfClearHk) != ACQD_E_OK) {
	fprintf(stderr,"Failed to send clear hk pulse on SURF %d.\n",surfIndex[surf]) ;
	syslog(LOG_ERR,"Failed to send clear hk pulse on SURF %d.\n",surfIndex[surf]) ;
      }	
    }
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
      for(dac=0;dac<SCALERS_PER_SURF;dac++) {
	startStruct.scalerVals[surf][dac][tInd]=theSurfHk.scaler[surf][dac];
      }
    }
  }
  setGlobalThreshold=tempGlobalThreshold;
  fillGenericHeader(&startStruct,PACKET_ACQD_START,sizeof(AcqdStartStruct_t));
  sprintf(theFilename,"%s/acqd_%d.dat",REQUEST_TELEM_DIR,startStruct.unixTime);
  retVal=writeStruct(&startStruct,theFilename,sizeof(AcqdStartStruct_t));  
  retVal=makeLink(theFilename,REQUEST_TELEM_LINK_DIR); 
  retVal=simpleMultiDiskWrite(&startStruct,sizeof(AcqdStartStruct_t),startStruct.unixTime,STARTUP_ARCHIVE_DIR,"acqd",hkDiskBitMask);


  printf("\n\n\nAcqd Start Info\n\n");
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    printf("SURF %d Mean:",surfIndex[surf]);
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
      printf("\t%3.2f",startStruct.chanMean[surf][chan]);
    }
    printf("\n");
  }
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    printf("SURF %d RMS:",surfIndex[surf]);
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
      printf("\t%3.2f",startStruct.chanRMS[surf][chan]);
    }
    printf("\n");
  }

  printf("\nThreshold Stuff\n");
  printf("Thresholds: ");
  for(tInd=0;tInd<10;tInd++) {
    printf("%5d ",startStruct.threshVals[tInd]);
  }
  printf("\n");
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    for(dac=0;dac<SCALERS_PER_SURF;dac++) {
      printf("Chan %02d_%02d: ",surfIndex[surf],dac+1);
      for(tInd=0;tInd<10;tInd++) {
	printf("%5d ",startStruct.scalerVals[surf][dac][tInd]);
      }
      printf("\n");
    }
  }
  return ACQD_E_OK;
} 



AcqdErrorCode_t doGlobalThresholdScan() 
{		
  //Here the name of the game is simply to loop over all the thresholds and
  //write out the corresponding scaler values
  //should be fairly trivial
  //Ha!
  AcqdErrorCode_t status=ACQD_E_OK;
  int threshScanCounter=0;
  int dacVal=0,lastDacVal=-1;
  int done=0,surf=0;
  int firstLoop=1;
  struct timeval timeStruct;
  currentState=PROG_STATE_RUN;
  fprintf(stderr,"Inside doGlobalThresholdScan\n");
  do {
    //Check if we are trying to leave the event loop
    if(currentState!=PROG_STATE_RUN) 
      break;
    threshScanCounter++;
    //    theScalers.threshold=dacVal;

    if(dacVal!=lastDacVal) {
      if(printToScreen) 
	printf("Setting Threshold -- %d\r",dacVal);
      setGloablDACThreshold(dacVal);	
    }
    theSurfHk.globalThreshold=dacVal;
    lastDacVal=dacVal;
    if(firstLoop) {
      usleep(30000);
      firstLoop=0;
    }
    
    
    gettimeofday(&timeStruct,NULL);
    //Actually read and write surfHk here
    status=readSurfHkData();
    //check it
    //Then send clear hk pulse
    for(surf=0;surf<numSurfs;++surf) {
      if (setSurfControl(surf,SurfClearHk) != ACQD_E_OK) {
	fprintf(stderr,"Failed to send clear hk pulse on SURF %d.\n",surfIndex[surf]) ;
	syslog(LOG_ERR,"Failed to send clear hk pulse on SURF %d.\n",surfIndex[surf]) ;
      }	
    }
    theSurfHk.unixTime=timeStruct.tv_sec;
    theSurfHk.unixTimeUs=timeStruct.tv_usec;
    writeSurfHousekeeping(1);

    if(threshScanCounter>=thresholdScanPointsPerStep) {
      dacVal+=thresholdScanStepSize;
      threshScanCounter=0;
    }  
    if(dacVal>4095) {
      done=1;
      continue;
    }
  } while(!done);
  //This is single dacVal stuff
  //
  //    memset(&thresholdArray[0][0],0,sizeof(int)*ACTIVE_SURFS*SCALERS_PER_SURF);
  //  if(surfForSingle==100) {
  //    for(surf=0;surf<numSurfs;surf++)
  //      thresholdArray[surf][dacForSingle]=doingDacVal;
  //  }
  //  else  
  //    thresholdArray[surfForSingle][dacForSingle]=doingDacVal;
  //  setDACThresholds();
    

  if(threshSwitchConfigAtEnd) {
    //Arrrghhh		
    CommandStruct_t theCmd;
    theCmd.numCmdBytes=3;
    theCmd.cmd[0]=LAST_CONFIG;
    theCmd.cmd[1]=1;
    theCmd.cmd[2]=0;
    writeCommandAndLink(&theCmd);	
    theCmd.numCmdBytes=1;
    theCmd.cmd[0]=CMD_START_NEW_RUN;
    writeCommandAndLink(&theCmd);
  }	    

  return status;
}

int getEventNumber() {
  int retVal=0;
  static int firstTime=1;
  static int eventNumber=0;
  /* This is just to get the lastEventNumber in case of program restart. */
  FILE *pFile;
  if(firstTime && !standAloneMode) {
    pFile = fopen (LAST_EVENT_NUMBER_FILE, "r");
    if(pFile == NULL) {
      syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
	      LAST_EVENT_NUMBER_FILE);
    }
    else {	    	    
      retVal=fscanf(pFile,"%d",&eventNumber);
      if(retVal<0) {
	syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
		LAST_EVENT_NUMBER_FILE);
	fprintf(stderr,"fscanff: %s ---  %s\n",strerror(errno),
		LAST_EVENT_NUMBER_FILE);
      }
      fclose (pFile);
    }
    //Only write out every 100th number, so always start from a 100
    eventNumber/=100;
    eventNumber++;
    eventNumber*=100;
    if(printToScreen) printf("The last event number is %d\n",eventNumber);
    firstTime=0;
  }
  eventNumber++;
  if(!standAloneMode && eventNumber%100==0) {
    pFile = fopen (LAST_EVENT_NUMBER_FILE, "w");
    if(pFile == NULL) {
      syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
	      LAST_EVENT_NUMBER_FILE);
      fprintf(stderr,"fopen: %s ---  %s\n",strerror(errno),
	      LAST_EVENT_NUMBER_FILE);
    }
    else {
      retVal=fprintf(pFile,"%d\n",eventNumber);
      if(retVal<0) {
	syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		LAST_EVENT_NUMBER_FILE);
	fprintf(stderr,"fprintf: %s ---  %s\n",strerror(errno),
		LAST_EVENT_NUMBER_FILE);
      }
      fclose (pFile);
    }
  }
  return eventNumber;
}

void outputEventData() {
  if(writeData) {
    if(useAltDir) {
      //			if(!madeAltDir || 
      //			   hdPtr->eventNumber%MAX_EVENTS_PER_DIR==0) 
      //			    makeSubAltDir();
      writeEventAndMakeLink(altOutputdir,ACQD_EVENT_LINK_DIR,&theEvent);	
    }
    else 
      writeEventAndMakeLink(ACQD_EVENT_DIR,ACQD_EVENT_LINK_DIR,&theEvent);
  }
}

//Global variables for this annoying thing
float scalerMean[ACTIVE_SURFS][SCALERS_PER_SURF];
float scalerMeanSq[ACTIVE_SURFS][SCALERS_PER_SURF];
float threshMean[ACTIVE_SURFS][SCALERS_PER_SURF];
float threshMeanSq[ACTIVE_SURFS][SCALERS_PER_SURF];
float rfPowerMean[ACTIVE_SURFS][RFCHAN_PER_SURF];
float rfPowerMeanSq[ACTIVE_SURFS][RFCHAN_PER_SURF];
unsigned short numSurfHksInAvg=0;  


void outputSurfHkData() {
  //Do the housekeeping stuff
  
  
  int surf,dac,chan;//,ind;
  float tempVal;
  char theFilename[FILENAME_MAX];
  int retVal=0;
  static time_t lastRawScaler=0;
  if(surfHkAverage>0) {
    if(numSurfHksInAvg==0) {
      //      printf("Zeroing arrays, numSurfHksInAvg=%d\n",numSurfHksInAvg);
      //Zero arrays
      memset(&avgSurfHk,0,sizeof(AveragedSurfHkStruct_t));
      memset(scalerMean,0,sizeof(float)*ACTIVE_SURFS*SCALERS_PER_SURF);
      memset(scalerMeanSq,0,sizeof(float)*ACTIVE_SURFS*SCALERS_PER_SURF);
      memset(threshMean,0,sizeof(float)*ACTIVE_SURFS*SCALERS_PER_SURF);
      memset(threshMeanSq,0,sizeof(float)*ACTIVE_SURFS*SCALERS_PER_SURF);
      memset(rfPowerMean,0,sizeof(float)*ACTIVE_SURFS*RFCHAN_PER_SURF);
      memset(rfPowerMeanSq,0,sizeof(float)*ACTIVE_SURFS*RFCHAN_PER_SURF);
      //Set up initial values
      avgSurfHk.unixTime=theSurfHk.unixTime;
      avgSurfHk.globalThreshold=theSurfHk.globalThreshold;
      memcpy(avgSurfHk.scalerGoals,theSurfHk.scalerGoals,sizeof(short)*BANDS_PER_ANT);
      memcpy(avgSurfHk.surfTrigBandMask,theSurfHk.surfTrigBandMask,sizeof(short)*ACTIVE_SURFS);
    }
    //Now add stuff to the average
    numSurfHksInAvg++;
    if(theSurfHk.errorFlag & (1<<surf))
      avgSurfHk.hadError |= (1<<(surf+16));
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
      for(dac=0;dac<SCALERS_PER_SURF;dac++) {
	scalerMean[surf][dac]+=theSurfHk.scaler[surf][dac];
	scalerMeanSq[surf][dac]+=(theSurfHk.scaler[surf][dac]*theSurfHk.scaler[surf][dac]);
	threshMean[surf][dac]+=theSurfHk.threshold[surf][dac];
	threshMeanSq[surf][dac]+=(theSurfHk.threshold[surf][dac]*theSurfHk.threshold[surf][dac]);
	
	if(theSurfHk.threshold[surf][dac]!=theSurfHk.setThreshold[surf][dac]) {
	  avgSurfHk.hadError |= (1<<surf);
	}
      }
      for(chan=0;chan<RFCHAN_PER_SURF;chan++) {
	rfPowerMean[surf][chan]+=theSurfHk.rfPower[surf][chan];
	rfPowerMeanSq[surf][chan]+=(theSurfHk.rfPower[surf][chan]*theSurfHk.rfPower[surf][chan]);
      }      
    }
    //    printf("scalerMean[0][0]=%f (%d) numSurfHksInAvg=%d surfHkAverage=%d\n",scalerMean[0][0],numSurfHksInAvg,surfHkAverage,theSurfHk.scaler[0][0]);    
    if(numSurfHksInAvg==surfHkAverage) {

      //Time to output
      for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(dac=0;dac<SCALERS_PER_SURF;dac++) {
	  scalerMean[surf][dac]/=numSurfHksInAvg;
	  avgSurfHk.avgScaler[surf][dac]=(int)(scalerMean[surf][dac]+0.5);
	  scalerMeanSq[surf][dac]/=numSurfHksInAvg;
	  tempVal=scalerMeanSq[surf][dac]-(scalerMean[surf][dac]*scalerMean[surf][dac]);
	  if(tempVal>0)
	    avgSurfHk.rmsScaler[surf][dac]=(int)(sqrt(tempVal)+0.5);
	  else
	    avgSurfHk.rmsScaler[surf][dac]=0;

	  threshMean[surf][dac]/=numSurfHksInAvg;
	  avgSurfHk.avgThresh[surf][dac]=(int)(threshMean[surf][dac]+0.5);
	  threshMeanSq[surf][dac]/=numSurfHksInAvg;
	  tempVal=threshMeanSq[surf][dac]-(threshMean[surf][dac]*threshMean[surf][dac]);
	  if(tempVal>0)
	    avgSurfHk.rmsThresh[surf][dac]=(int)(sqrt(tempVal)+0.5);
	  else
	    avgSurfHk.rmsThresh[surf][dac]=0;
	}
	for(chan=0;chan<RFCHAN_PER_SURF;chan++) {
	  rfPowerMean[surf][chan]/=numSurfHksInAvg;
	  avgSurfHk.avgRFPower[surf][chan]=(int)(rfPowerMean[surf][chan]+0.5);
	  rfPowerMeanSq[surf][chan]/=numSurfHksInAvg;
	  tempVal=rfPowerMeanSq[surf][chan]-(rfPowerMean[surf][chan]*rfPowerMean[surf][chan]);
	  if(tempVal>0)
	    avgSurfHk.rmsRFPower[surf][chan]=(int)(sqrt(tempVal)+0.5);
	  else
	    avgSurfHk.rmsRFPower[surf][chan]=0;
	}
      }
      avgSurfHk.numHks=numSurfHksInAvg;
      avgSurfHk.deltaT=theSurfHk.unixTime-avgSurfHk.unixTime;
      fillGenericHeader(&avgSurfHk,PACKET_AVG_SURF_HK,sizeof(AveragedSurfHkStruct_t)); 
      sprintf(theFilename,"%s/avgsurfhk_%d.dat",SURFHK_TELEM_DIR,
	      avgSurfHk.unixTime);
      retVal+=writeStruct(&avgSurfHk,theFilename,sizeof(AveragedSurfHkStruct_t));
      makeLink(theFilename,SURFHK_TELEM_LINK_DIR);  
      
      //      if(printToScreen && verbosity>=0) {
      //	printf("Averaged Scaler [0][0]=%d\n",avgSurfHk.avgScaler[0][0]);
      //      }
      retVal=cleverHkWrite((unsigned char*)&avgSurfHk,
			   sizeof(AveragedSurfHkStruct_t),
			   avgSurfHk.unixTime,&avgSurfHkWriter);   
      numSurfHksInAvg=0;
    }      
  }

  if(writeFullHk) {
    if((theSurfHk.unixTime-lastSurfHk)>=surfHkPeriod) {
      //Record housekeeping data
      lastSurfHk=theSurfHk.unixTime;
      surfHkCounter++;
      if(surfHkCounter>=surfHkTelemEvery) {
	writeSurfHousekeeping(3);
	surfHkCounter=0;
      }
      else {
	//			    printf("\tSURF HK %d %d\n",theSurfHk.unixTime,lastSurfHk);
	writeSurfHousekeeping(1);
      }
    }
  }
  if(writeRawScalers) {
    if((theScalers.unixTime-lastRawScaler)>=writeRawScalers) {
      retVal=cleverHkWrite((unsigned char*)&theScalers,sizeof(SimpleScalerStruct_t),theScalers.unixTime,&rawScalerWriter);
      //      printf("Writing rawScalerWriter %u -- %u\n",theScalers.unixTime
      lastRawScaler=theScalers.unixTime;

    }
  }
}


void outputTurfRateData() {
  char theFilename[FILENAME_MAX];
  static SummedTurfRateStruct_t sumTurf;
  static int numRates=0;
  int retVal=0,phi,ring,nadirAnt;
  //  printf("outputTurfRateData -- numRates %d, ppsNum %d\n",numRates,turfRates.ppsNum);
  if(turfRateAverage>0) {
    if(numRates==0) {
      //Need to initialize sumTurf
      memset(&sumTurf,0,sizeof(SummedTurfRateStruct_t));
      sumTurf.unixTime=turfRates.unixTime;
      sumTurf.antTrigMask=turfRates.antTrigMask;
      sumTurf.nadirAntTrigMask=turfRates.nadirAntTrigMask;
      sumTurf.phiTrigMask=turfRates.phiTrigMask;
    }
    numRates++;
    sumTurf.errorFlag|=turfRates.errorFlag;
    for(nadirAnt=0;nadirAnt<NADIR_ANTS;nadirAnt++) {
      sumTurf.nadirL1Rates[nadirAnt]+=turfRates.nadirL1Rates[nadirAnt];
      sumTurf.nadirL2Rates[nadirAnt]+=turfRates.nadirL2Rates[nadirAnt];
    }
    for(phi=0;phi<PHI_SECTORS;phi++) {
      for(ring=0;ring<2;ring++) {
	sumTurf.l1Rates[phi][ring]+=turfRates.l1Rates[phi][ring];
      }
      sumTurf.upperL2Rates[phi]+=turfRates.upperL2Rates[phi];
      sumTurf.lowerL2Rates[phi]+=turfRates.lowerL2Rates[phi];
      sumTurf.l3Rates[phi]+=turfRates.l3Rates[phi];
    }
    if(numRates==turfRateAverage) {
      //Time to output it
      sumTurf.numRates=numRates;
      sumTurf.deltaT=turfRates.unixTime-sumTurf.unixTime;
      fillGenericHeader(&sumTurf,PACKET_SUM_TURF_RATE,sizeof(SummedTurfRateStruct_t)); 
      sprintf(theFilename,"%s/sumturfrate_%d.dat",TURFHK_TELEM_DIR,
	      sumTurf.unixTime);
      retVal+=writeStruct(&sumTurf,theFilename,sizeof(SummedTurfRateStruct_t));
      makeLink(theFilename,TURFHK_TELEM_LINK_DIR);  

      retVal=cleverHkWrite((unsigned char*)&sumTurf,
			   sizeof(SummedTurfRateStruct_t),
			   sumTurf.unixTime,&sumTurfRateWriter);      
      numRates=0;
    }
  }
  if(writeFullHk) {
    writeTurfHousekeeping(1);
    turfHkCounter++;
    if(turfHkCounter>=turfRateTelemEvery) {
      //			    printf("turfHk %d %d\n",turfHkCounter,turfRateTelemEvery);
      turfHkCounter=0;
      writeTurfHousekeeping(2);
      lastTurfHk=turfRates.unixTime;
    }
    else if(turfRateTelemInterval && 
	    (turfRateTelemInterval<=
	     (turfRates.unixTime-lastTurfHk))) {
      //			    printf("turfHk times %d %d -- %d\n",turfRates.unixTime,lastTurfHk,turfRateTelemInterval);
      turfHkCounter=0;
      writeTurfHousekeeping(2);
      lastTurfHk=turfRates.unixTime;
    }
  }
}


int writeSurfHousekeeping(int dataOrTelem) 
// dataOrTelem 
// 1 == data
// 2 == telem
// anything else == both
{
  if(printToScreen && verbosity>1)
    printf("writeSurfHousekeeping(%d) -- theSurfHk.setThreshold[0][0]=%d\n",dataOrTelem,theSurfHk.setThreshold[0][0]);
  char theFilename[FILENAME_MAX];
  int retVal=0;
  fillGenericHeader(&theSurfHk,PACKET_SURF_HK,sizeof(FullSurfHkStruct_t));
  //Write data to disk
  if(dataOrTelem!=2) {
    //	sprintf(theFilename,"%s/surfhk_%d_%d.dat.gz",surfHkArchiveDir,
    //		theSurfHk.unixTime,theSurfHk.unixTimeUs);
    //	retVal+=writeStruct(&theSurfHk,theFilename);
    retVal=cleverHkWrite((unsigned char*)&theSurfHk,sizeof(FullSurfHkStruct_t),
			 theSurfHk.unixTime,&surfHkWriter);
  }
  if(dataOrTelem!=1) {
    // Write data for Telem
    sprintf(theFilename,"%s/surfhk_%d_%d.dat.gz",SURFHK_TELEM_DIR,
	    theSurfHk.unixTime,theSurfHk.unixTimeUs);
    retVal+=writeStruct(&theSurfHk,theFilename,sizeof(FullSurfHkStruct_t));
    makeLink(theFilename,SURFHK_TELEM_LINK_DIR);
  }
  return retVal;

	
}


int writeTurfHousekeeping(int dataOrTelem) 
// dataOrTelem 
// 1 == data
// 2 == telem
// anything else == both
{
  char theFilename[FILENAME_MAX];
  int retVal=0;
    if(printToScreen &&verbosity>=2)
   	printf("writeTurfHousekeeping(%d)\n",dataOrTelem); 

  fillGenericHeader(&turfRates,PACKET_TURF_RATE,sizeof(TurfRateStruct_t));
  //Write data to disk
  if(dataOrTelem!=2) {
    retVal=cleverHkWrite((unsigned char*)&turfRates,sizeof(TurfRateStruct_t),
			 turfRates.unixTime,&turfHkWriter);
  }
  if(dataOrTelem!=1) {
    // Write data for Telem
    sprintf(theFilename,"%s/turfhk_%d_%d.dat.gz",TURFHK_TELEM_DIR,
	    turfRates.unixTime,turfRates.ppsNum);
    writeStruct(&turfRates,theFilename,sizeof(TurfRateStruct_t));
    retVal+=makeLink(theFilename,TURFHK_TELEM_LINK_DIR);
  }

  return retVal;
}

void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr)
{
  //    printf("writeEventAndMakeLink(%s,%s,%d)\n",theEventDir,theLinkDir,(int)theEventPtr);
  char theFilename[FILENAME_MAX];
  int retVal;
  AnitaEventHeader_t *theHeader=&(theEventPtr->header);
  AnitaEventBody_t *theBodyPtr=&(theEventPtr->body);


  //Fill Generic Header
  if(turfFirmwareVersion>=3) 
    fillGenericHeader(hdPtr,PACKET_HD,sizeof(AnitaEventHeader_t));
  else
    fillGenericHeader(hdPtr,PACKET_HD_SLAC,sizeof(AnitaEventHeader_t));
  fillGenericHeader(theBodyPtr,PACKET_BD,sizeof(AnitaEventBody_t));



  if(!useAltDir) {
    if(justWriteHeader==0 && writeData) {
      sprintf(theFilename,"%s/psev_%d.dat",theEventDir,
	      theEventPtr->header.eventNumber);
      //Now lets try and write a PedSubbedEventBody_t file
      	//Subtract Pedestals
      subtractCurrentPeds(theBodyPtr,&pedSubBody);
      retVal=writeStruct(&pedSubBody,theFilename,sizeof(PedSubbedEventBody_t));
    }
	
    if(writeData) {
      sprintf(theFilename,"%s/hd_%d.dat",theEventDir,
	      theEventPtr->header.eventNumber);
      retVal=writeStruct(theHeader,theFilename,sizeof(AnitaEventHeader_t));
    }
	
    /* Make links, not sure what to do with return value here */
    if(!standAloneMode && writeData) 
      retVal=makeLink(theFilename,theLinkDir);
  }
  else {
    if(printToScreen && verbosity) {
      printf("Trying to write event %u to dir %s\n",
	     theBodyPtr->eventNumber,theEventDir);
    }
	
    retVal=cleverEventWrite((unsigned char*)theBodyPtr,sizeof(AnitaEventBody_t),theHeader,&eventWriter);
    if(retVal<0) {
	
      //Do something
    }
  }

}


AcqdErrorCode_t readSurfEventData() 
/*! This is the code that is executed after we read the EVT_RDY flag */
/* It is the equivalent of read_LABdata in Shige's crate_test program */
{
  //Need to add a calls to
  // set read mode??
  //set data mode
  // read N bytes
  /* 0 -- Header Word
     1 -- Prime Word
     2:129 -- Chan 0 Samps 0-255 (2 x 16 bit words per 32 bit read)
     130:1153 -- Chans 1 through 8 128 reads per chan as above
     1154:1155 -- Chan 0 Samps 256-259 (2 x 16 bit words per 32 bit read)
     1156:1169 -- Chans 1 through 8 2 reads per chan as above
  */

  AcqdErrorCode_t status=ACQD_E_OK;
  unsigned int  dataInt=0;
  unsigned int eventBuf[1170];

  unsigned char tempVal;
  int count=0,i=0;
  int chanId=0,surf,chan=0,readCount,firstHitbus,lastHitbus,wrappedHitbus;
  int rcoSamp;
  unsigned int headerWord,samp=0;
  unsigned short upperWord=0;
  unsigned char rcoBit;
    

  doingEvent++;
  slipCounter++;
  if(verbosity && printToScreen) 
    printf("Triggered, event %d (by software counter).\n",doingEvent);

  	
  //Loop over SURFs and read out data
  for(surf=0;surf<numSurfs;surf++){
    count=0;
    //Set to Data mode
    status=setSurfControl(surf,SurfDataMode);
    if(status!=ACQD_E_OK) {
      //Persist anyhow
    }
      
    //In version X data is read as 32 bit words:
    /* 0 -- Header Word
       1 -- Event Id???
       2:129 -- Chan 0 Samps 0-255 (2 x 16 bit words per 32 bit read)
       130:1153 -- Chans 1 through 8 128 reads per chan as above
       1154:1155 -- Chan 0 Samps 256-259 (2 x 16 bit words per 32 bit read)
       1156:1169 -- Chans 1 through 8 2 reads per chan as above
    */
    //Read the Event data
    count = read(surfFds[surf], eventBuf, 1170*sizeof(int));
    if (count < 0) {
      syslog(LOG_ERR,"Error reading event data from SURF %d (%s)",surfIndex[surf],strerror(errno));
      fprintf(stderr,"Error reading event data from SURF %d (%s)",surfIndex[surf],strerror(errno));
    }
    if(printToScreen && verbosity>2) {	
      for(i=0;i<1170;i++) {
	printf("SURF %d, %d == 0x%x\n",surfIndex[surf],i,eventBuf[i]);
      }
    }
            
    //First is the header word
    headerWord=eventBuf[0];
    if(printToScreen && verbosity) {
      printf("SURF %d (%d), CHIP %d, CHN %d, Header: %x\n",surfIndex[surf],((headerWord&0xf0000)>>16),((headerWord&0x00c00000)>> 22),chan,headerWord);
    }      
    if(surf==0) {
      hdPtr->otherFlag=0;
      hdPtr->errorFlag=0;
      hdPtr->surfSlipFlag=0;
    }
	
    int surfEvNum=((headerWord&0xf000000)>>24);
    if(surf==0) {
      hdPtr->otherFlag=surfEvNum;
    }
    if(surf==1) {
      hdPtr->otherFlag|=(surfEvNum<<4);
    }
    if(printToScreen && verbosity>2)
      printf("surf %d\tsurfEvNum %d\theaderWord %#x\nhdPtr->otherFlag %d\n",surf,surfEvNum,headerWord,hdPtr->otherFlag);
    
      
    //Next is surf event id word 
    dataInt=eventBuf[1];
    if(printToScreen && verbosity>2) {
      printf("SURF %d (%d), CHIP %d, CHN %d, SURF Event Id: %x\n",surfIndex[surf],((headerWord&0xf0000)>>16),((headerWord&0x00c00000)>> 22),chan,dataInt);
    }
    bdPtr->surfEventId[surf]=dataInt;
    if(dataInt!=bdPtr->surfEventId[surf]) {
      //Have a slip
      if(surf<9) {
	hdPtr->surfSlipFlag|=(1<<(surf-1));
      }
      else {
	hdPtr->errorFlag|=(1<<2);
      }
    }

    //Now we get the first 256 samples per channel
    for(chan=0;chan<N_CHAN; chan++) {
      for (samp=readCount=0 ; readCount<N_SAMP_EFF/2 ; readCount++) {
	i=2+ chan*(N_SAMP_EFF/2) + readCount;
	dataInt=eventBuf[i];
	  
	if(printToScreen && verbosity>2) {
	  printf("SURF %d (%d), CHIP %d, CHN %d, Read %d  (i==%d): %#x %#x  (s %d %d) (sp %d %d) (hb %d %d) (low %d %d)\n",surfIndex[surf],((headerWord&0xf0000)>>16),((headerWord&0x00c00000)>> 22),chan,readCount,i,
		 GetUpper16(dataInt),
		 GetLower16(dataInt),
		 GetStartBit(GetUpper16(dataInt)),
		 GetStartBit(GetLower16(dataInt)),
		 GetStopBit(GetUpper16(dataInt)),
		 GetStopBit(GetLower16(dataInt)),
		 GetHitBus(GetUpper16(dataInt)),
		 GetHitBus(GetLower16(dataInt)),
		 GetUpper16(dataInt)&0x1,
		 GetLower16(dataInt)&0x1
		 );
	}
	  
	//Upper word
	labData[surf][chan][samp]=dataInt&0xffff;
	labData[surf][chan][samp++]|=headerWord&0xffff0000;
	  
	//Lower word
	labData[surf][chan][samp]=(dataInt>>16);
	labData[surf][chan][samp++]|=headerWord&0xffff0000;
	  
      }
    }

    /* 	Now read last 4 samples */
    for(chan=0;chan<N_CHAN; chan++) {
      samp=N_SAMP_EFF;
      for(readCount=N_SAMP_EFF/2;readCount<N_SAMP/2;readCount++) {
	dataInt=eventBuf[1154 + (chan*2) +(readCount-N_SAMP_EFF/2)];	  
	if(printToScreen && verbosity>2) {
	  printf("SURF %d (%d), CHIP %d, CHN %d, Read %d: %#x %#x  (s %d %d) (sp %d %d) (hb %d %d) (low %d %d)\n",surfIndex[surf],((headerWord&0xf0000)>>16),((headerWord&0x00c00000)>> 22),chan,readCount,
		 GetUpper16(dataInt),
		 GetLower16(dataInt),
		 GetStartBit(GetUpper16(dataInt)),
		 GetStartBit(GetLower16(dataInt)),
		 GetStopBit(GetUpper16(dataInt)),
		 GetStopBit(GetLower16(dataInt)),
		 GetHitBus(GetUpper16(dataInt)),
		 GetHitBus(GetLower16(dataInt)),
		 GetUpper16(dataInt)&0x1,
		 GetLower16(dataInt)&0x1
		 );
	    
	}
	//Store in array (will move to 16bit array when bothered

	//Lower word 
	labData[surf][chan][samp]=dataInt&0xffff;
	labData[surf][chan][samp++]|=headerWord&0xffff0000;

	//Upper word
	labData[surf][chan][samp]=(dataInt>>16);
	labData[surf][chan][samp++]|=headerWord&0xffff0000;	
      }
    }
    //Now we've stuffed the event data into 32bit arrays ordered by sample
    //it is time to put it into the format we actually want
    for (chan=0 ; chan<N_CHAN ; chan++) {
      chanId=chan+surf*CHANNELS_PER_SURF;
      firstHitbus=-1;
      lastHitbus=-1;
      wrappedHitbus=0;
      for (samp=0 ; samp<N_SAMP ; samp++) {
	dataInt=labData[surf][chan][samp];
	if(printToScreen && verbosity>2) {
	  printf("SURF %d, Chan %d, Samp %d -- %d (%x)\n",surfIndex[surf],chan,samp,dataInt,dataInt);
	}
	if(samp==0) upperWord=GetUpper16(dataInt);
	//Below is constructed to be true
	if(upperWord!=GetUpper16(dataInt)) {
	  //Will add an error flag to the channel header
	  syslog(LOG_WARNING,"Upper 16bits don't match: SURF %d Chan %d Samp %d (%x %x)",surfIndex[surf],chan,samp,upperWord,GetUpper16(dataInt));
	  fprintf(stderr,"Upper 16bits don't match: SURF %d Chan %d Samp %d (%x %x)",surfIndex[surf],chan,samp,upperWord,GetUpper16(dataInt));	    
	}
		    
	theEvent.body.channel[chanId].data[samp]=GetLower16(dataInt);
	  
	//Now check for HITBUS
	if(firstHitbus<0 && (GetLower16(dataInt)&HITBUS))
	  firstHitbus=samp;
	if(GetLower16(dataInt)&HITBUS) {
	  if(!wrappedHitbus) {
	    if(lastHitbus>-1 && (samp-lastHitbus)>20 && (260-samp)<20) {
	      //Wrapped hitbus
	      wrappedHitbus=1;
	      firstHitbus=lastHitbus;
	    }
	    lastHitbus=samp;
	  }
	}
      }
      //	End of sample loop
      //Now check which RCO is flagged
      rcoSamp=firstHitbus-1;
      if(wrappedHitbus)
	rcoSamp=lastHitbus-1;
      if(rcoSamp<0 || rcoSamp>255) rcoSamp=100;		
      rcoBit=((labData[surf][chan][rcoSamp]&0x2000)>>13);
      tempVal=chan+N_CHAN*surf;
      theEvent.body.channel[chanId].header.chanId=tempVal;
      theEvent.body.channel[chanId].header.firstHitbus=firstHitbus;
      tempVal=((upperWord&0xc0)>>6)+(rcoBit<<2)+(wrappedHitbus<<3);
      if(lastHitbus>255) {
	tempVal+=((lastHitbus-255)<<4);
	lastHitbus=255;
      }
      theEvent.body.channel[chanId].header.chipIdFlag=tempVal;
      theEvent.body.channel[chanId].header.lastHitbus=lastHitbus;
		
      if(printToScreen && verbosity>1) {
	printf("SURF %d, Chan %d, chanId %d\n\tFirst Hitbus %d\n\tLast Hitbus %d\n\tWrapped Hitbus %d\n\tUpper Word %x\n\tLabchip %d\n\tchipIdFlag %d\n",
	       surfIndex[surf],chan,
	       theEvent.body.channel[chanId].header.chanId,
	       theEvent.body.channel[chanId].header.firstHitbus,
	       theEvent.body.channel[chanId].header.lastHitbus,
	       wrappedHitbus,upperWord,((upperWord&0xc0)>>6),
	       theEvent.body.channel[chanId].header.chipIdFlag);
      }		
    }	 
  }  
	
  if(printToScreen && verbosity>1) {	
    for(surf=0;surf<numSurfs;surf++){
      for(chan=0;chan<N_CHAN;chan++) {
	for(samp=0;samp<5;samp++) {
	  printf("SURF %d, chan %d, samp %d, data %d\n",surf,chan,samp,labData[surf][chan][samp]&0xfff);
	}
      }
    }
  }
  return status; 
} 



AcqdErrorCode_t readSurfHkData() 
// Reads the scaler and RF power data from the SURF board
{
  AcqdErrorCode_t status=ACQD_E_OK;
  int surf,rfChan,index,band;
  unsigned int buffer[72];
  unsigned int count=0;
  unsigned int dataInt;

  hkNumber++;
  if(verbosity && printToScreen) 
    printf("Reading Surf HK %d.\n",hkNumber);
  
  if(enableChanServo) {
    for(band=0;band<ANTS_PER_SURF;band++) 
      theSurfHk.scalerGoals[band]=pidGoals[band];
  }
  else {
    theSurfHk.scalerGoals[0]=0;
    theSurfHk.scalerGoals[1]=0;
    theSurfHk.scalerGoals[2]=0;
    theSurfHk.scalerGoals[3]=0;
  }
  for(surf=0;surf<numSurfs;surf++){  
    count=0;
    //Set the SURF to Hk mode
    status=setSurfControl(surf,SurfHkMode);
    if(status!=ACQD_E_OK) {
      //Persist anyhow
    }
    //Read the Hk data
    if(writeRawScalers) {
	count = read(surfFds[surf], buffer, 128*sizeof(int));
    }
    else {
	count = read(surfFds[surf], buffer, 72*sizeof(int));
    }
    if (count < 0) {
      syslog(LOG_ERR,"Error reading housekeeping from SURF %d (%s)",surfIndex[surf],strerror(errno));
      fprintf(stderr,"Error reading housekeeping from SURF %d (%s)",surfIndex[surf],strerror(errno));
    }
    
    //Set it back to data mode -- maybe
    status=setSurfControl(surf,SurfDataMode);
    if(status!=ACQD_E_OK) {
      //Persist anyhow
    }

    //First just fill in antMask in SURF
    theSurfHk.surfTrigBandMask[surf]=surfTrigBandMasks[surf];
    

    if(printToScreen && verbosity>2) {
      for(index=0;index<72;index++) {
	printf("SURF %d, Word %d == %d  (%d)\n",surfIndex[surf],index,buffer[index],buffer[index]&0xffff);
      }
    }
    
    //Fill in the raw scaler data
    for(index=0;index<32;index++) {
      dataInt=buffer[index];
      theScalers.scaler[surf][index]=dataInt&0xffff;
    }
      


    //First stuff the scaler data
    for(rfChan=0;rfChan<SCALERS_PER_SURF;rfChan++){
      index=logicalScalerToRawScaler[rfChan];
      dataInt=buffer[index];
      theSurfHk.scaler[surf][rfChan]=dataInt&0xffff;
      if((printToScreen && verbosity>1) || HK_DEBUG) 
	printf("SURF %d, Scaler %d == %d\n",surfIndex[surf],rfChan,theSurfHk.scaler[surf][rfChan]);
      
      if(rfChan==0) {
	//Do something with upperWord
	theSurfHk.upperWords[surf]=GetUpper16(dataInt);
      }
      else if(theSurfHk.upperWords[surf]!=GetUpper16(dataInt)) {
	theSurfHk.errorFlag|=(1>>surf);
      }      
    }
    
    //Next comes the threshold (DAC val) data
    for(rfChan=0;rfChan<SCALERS_PER_SURF;rfChan++){
      index=logicalScalerToRawScaler[rfChan];
      dataInt=buffer[RAW_SCALERS_PER_SURF+index];
      if(printToScreen && verbosity>2) {
	printf("Surf %d, Threshold %d Word %d == %d\n",surfIndex[surf],rfChan,RAW_SCALERS_PER_SURF+index,dataInt&0xffff);
      }
      theSurfHk.threshold[surf][rfChan]=dataInt&0xffff;
      theSurfHk.setThreshold[surf][rfChan]=thresholdArray[surf][rfChan];
      if((printToScreen && verbosity>1) || HK_DEBUG) 
	printf("Surf %d, Threshold %d (Top bits %d) == %d\n",surfIndex[surf],rfChan,(theSurfHk.threshold[surf][rfChan]&0xf000)>>12,theSurfHk.threshold[surf][rfChan]&0xfff);
      //Should check if it is the same or not
      if(theSurfHk.upperWords[surf]!=GetUpper16(dataInt)) {
	theSurfHk.errorFlag|=(1>>surf);
      }
      if(!doThresholdScan) {
	if(!setGlobalThreshold) {
	  if((theSurfHk.threshold[surf][rfChan]&0xfff)!=thresholdArray[surf][rfChan])
	    {
	      if(surf<8) {
		printf("Surf %d, Threshold %d (Top bits %d) -- Is %d Should be %d\n",surfIndex[surf],rfChan,(theSurfHk.threshold[surf][rfChan]&0xf000)>>12,theSurfHk.threshold[surf][rfChan]&0xfff,thresholdArray[surf][rfChan]);
	      }
	    }
	}
      }
    }
    
    //Lastly read the RF Power Data
    for(rfChan=0;rfChan<N_RFCHAN;rfChan++){
      index=rfChan;
      dataInt=buffer[(2*RAW_SCALERS_PER_SURF)+index];
      theSurfHk.rfPower[surf][rfChan]=dataInt&0xfff;
      if((printToScreen && verbosity>1) || HK_DEBUG)
	printf("Surf %d, RF Power %d == %d\n",surfIndex[surf],rfChan,theSurfHk.rfPower[surf][rfChan]&0xfff);
      //Should check if it is the same or not
      if(theSurfHk.upperWords[surf]!=GetUpper16(dataInt)) {
	theSurfHk.errorFlag|=(1>>surf);
      }

    }

    if(writeRawScalers) {
	//Even more lastly get the extra scalers	 
	//Fill in the raw scaler data
	for(index=0;index<32;index++) {
	    dataInt=buffer[index+(3*32)];
	    theScalers.extraScaler[surf][index]=dataInt&0xffff;
	}
    }
  }
  return status;
}


AcqdErrorCode_t readTurfEventDataVer3()
/*! Reads out the TURF data via the TURFIO */
{
  newTurfRateData=0;
  AcqdErrorCode_t status=ACQD_E_OK;
  unsigned short  dataWord=0;
  unsigned char upperChar;
  unsigned char dataChar;
  unsigned short dataShort;
  unsigned int dataInt;


  int wordNum,phi,ring,errCount=0,count=0;
  TurfioTestPattern_t startPat;
  TurfioTestPattern_t endPat;
  unsigned short turfBuf[160];
    
  //Read out 160 words and shorts not ints

  //They'll be some read statement here    
  //Read the Hk data
  count = read(turfioFd, turfBuf, 160*sizeof(short));
  if (count < 0) {
    syslog(LOG_ERR,"Error reading TURF data from TURFIO (%s)",strerror(errno));
    fprintf(stderr,"Error reading TURF data from TURFIO (%s)",strerror(errno));
  }
    
  for(wordNum=0;wordNum<160;wordNum++) {
    dataWord=turfBuf[wordNum]&0xffff;
    dataChar=0;
    dataShort=0;
    dataInt=0;
    dataChar=dataWord&0xff;
    dataShort=dataWord&0xff;
    dataInt=dataWord&0xff;
    upperChar=(dataWord&0xff00)>>8;
    if(printToScreen && verbosity>1) {
      printf("TURFIO -- Word %d  -- %x -- %x + %x\n",wordNum,dataWord,
	     dataShort,upperChar);
    }
    if(wordNum==0) 
      hdPtr->turfUpperWord=upperChar;
      
    if(wordNum<7) {
      startPat.test[wordNum]=dataChar; 
    } 
    else if(wordNum<8) {
      startPat.test[wordNum]=dataChar; 
      turfioPtr->bufferDepth=((dataChar&CUR_BUF_MASK_READ)>>(CUR_BUF_SHIFT_READ-CUR_BUF_SHIFT_FINAL));
    }
      
    else if(wordNum<24) {
      switch(wordNum) {
      case 8:
	turfioPtr->trigType=dataChar; 
	turfioPtr->bufferDepth|=((dataChar&TRIG_BUF_MASK_READ)>>(TRIG_BUF_SHIFT_READ-TRIG_BUF_SHIFT_FINAL));
		    
	break;
      case 9:
	turfioPtr->l3Type1Count=dataChar; break;
      case 10:
	turfioPtr->trigNum=dataShort; break;
      case 11:
	turfioPtr->trigNum+=(dataShort<<8); break;
      case 12:
	turfioPtr->trigTime=dataInt; break;
      case 13:
	turfioPtr->trigTime+=(dataInt<<8); break;
      case 14:
	turfioPtr->trigTime+=(dataInt<<16); break;
      case 15:
	turfioPtr->trigTime+=(dataInt<<24); break;
      case 16:
	turfioPtr->ppsNum=dataShort; 
	turfRates.ppsNum=dataShort;break;
      case 17:
	turfRates.ppsNum+=(dataShort<<8);
	turfioPtr->ppsNum+=(dataShort<<8); break;
      case 18:
	turfioPtr->deadTime=(dataShort); break;
      case 19:
	turfioPtr->deadTime+=(dataShort<<8); break;
      case 20:
	turfioPtr->c3poNum=dataInt; break;
      case 21:
	turfioPtr->c3poNum+=(dataInt<<8); break;
      case 22:
	turfioPtr->c3poNum+=(dataInt<<16); break;
      case 23:
	turfioPtr->c3poNum+=(dataInt<<24); break;
      default:
	// Can't get here
	break;
      }
    }
    else if(wordNum<88) {
      //Surf Antenna counters
      phi=((wordNum-24)/2)%PHI_SECTORS;
      ring=((wordNum-24)/2)/PHI_SECTORS;
      if(wordNum%2==0) {
	//First word
	turfRates.l1Rates[phi][ring]=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfRates.l1Rates[phi][ring]+=(dataShort<<8);
      }	    
    }
    else if(wordNum<104) {
      turfRates.upperL2Rates[wordNum-88]=dataChar;
    }
    else if(wordNum<120) {
      turfRates.lowerL2Rates[wordNum-104]=dataChar;
    }
    else if(wordNum<136) {
      turfRates.l3Rates[wordNum-120]=dataChar;
    }
    else if(wordNum<138) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->upperL1TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->upperL1TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<140) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->lowerL1TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->lowerL1TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<142) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->upperL2TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->upperL2TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<144) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->lowerL2TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->lowerL2TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<146) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->l3TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->l3TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<148) {
      if(wordNum%2==0) {
	//First word
	//		turfioPtr->l3TrigPattern2=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	//		turfioPtr->l3TrigPattern2+=(dataShort<<8);
      }
    }	    	    	
    else if(wordNum<152) {
      //do nothing
    }
    else if(wordNum<160) {
      endPat.test[wordNum-152]=dataChar; 
    }
	    
	
  }
  for(wordNum=0;wordNum<8;wordNum++) {
    if(startPat.test[wordNum]!=endPat.test[wordNum]) 
      errCount++;
    if(verbosity && printToScreen) {
      printf("Test Pattern %d -- %x -- %x\n",wordNum,startPat.test[wordNum],
	     endPat.test[wordNum]);
    }
  }
  if(errCount) {
    //Flag for non matching TURF test patterns
    hdPtr->errorFlag|=(1<<3);
  }


  if(turfioPtr->deadTime>0 && lastPPSNum!=turfioPtr->ppsNum) {
    intervalDeadtime+=((float)turfioPtr->deadTime)/64400.;
    if(printToScreen && verbosity>1)
      printf("Deadtime %d counts %f fraction\n",turfioPtr->deadTime,
	     ((float)turfioPtr->deadTime)/64400.);
  }

    
  if(verbosity && printToScreen) {
    printf("TURF Data\n\tEvent (software):\t%u\n\tupperWord:\t0x%x\n\ttrigNum:\t%u\n\ttrigType:\t0x%x\n\ttrigTime:\t%u\n\tppsNum:\t\t%u\n\tc3p0Num:\t%u\n\tl3Type1#:\t%u\n\tdeadTime:\t\t%u\n",
	   hdPtr->eventNumber,hdPtr->turfUpperWord,turfioPtr->trigNum,turfioPtr->trigType,turfioPtr->trigTime,turfioPtr->ppsNum,turfioPtr->c3poNum,turfioPtr->l3Type1Count,turfioPtr->deadTime);
    printf("Trig Patterns:\nUpperL1:\t%x\nLowerL1:\t%x\nUpperL2:\t%x\nLowerL2:\t%x\nL31:\t%x\n",
	   turfioPtr->upperL1TrigPattern,
	   turfioPtr->lowerL2TrigPattern,
	   turfioPtr->upperL2TrigPattern,
	   turfioPtr->lowerL2TrigPattern,
	   turfioPtr->l3TrigPattern);
  }

  if(turfRates.ppsNum!=lastPPSNum) { //When the PPS isn't present won't get this
    newTurfRateData=1;
  }
  //  printf("turfioPtr->ppsNum=%d\n",turfioPtr->ppsNum);
  turfRates.antTrigMask=antTrigMask;
  turfRates.nadirAntTrigMask=nadirAntTrigMask&0xff;
  lastPPSNum=turfRates.ppsNum;
  return status;	
}


AcqdErrorCode_t readTurfEventDataVer5()
/*! Reads out the TURF data via the TURFIO */
{
  newTurfRateData=0;
  AcqdErrorCode_t status=ACQD_E_OK;
  unsigned short  dataWord=0;
  unsigned char upperChar;
  unsigned char dataChar;
  unsigned short dataShort;
  unsigned int dataInt;
  static unsigned short lastPPSNum=0;

  int wordNum,phi,ring,errCount=0,count=0,nadirAnt;
  TurfioTestPattern_t startPat;
  TurfioTestPattern_t endPat;
  unsigned short turfBuf[256];
    
  //Read out 160 words and shorts not ints

  //They'll be some read statement here    
  //Read the Hk data
  count = read(turfioFd, turfBuf, 160*sizeof(short));
  if (count < 0) {
    syslog(LOG_ERR,"Error reading TURF data from TURFIO (%s)",strerror(errno));
    fprintf(stderr,"Error reading TURF data from TURFIO (%s)",strerror(errno));
  }
    
  for(wordNum=0;wordNum<160;wordNum++) {
    dataWord=turfBuf[wordNum]&0xffff;
    dataChar=0;
    dataShort=0;
    dataInt=0;
    dataChar=dataWord&0xff;
    dataShort=dataWord&0xff;
    dataInt=dataWord&0xff;
    upperChar=(dataWord&0xff00)>>8;
    if(printToScreen && verbosity>1) {
      printf("TURFIO -- Word %d  -- %x -- %x + %x\n",wordNum,dataWord,
	     dataShort,upperChar);
    }
    if(wordNum==0) 
      hdPtr->turfUpperWord=upperChar;
      
    if(wordNum<7) {
      startPat.test[wordNum]=dataChar; 
    } 
    else if(wordNum<8) {
      startPat.test[wordNum]=dataChar; 
      turfioPtr->bufferDepth=((dataChar&CUR_BUF_MASK_READ)>>(CUR_BUF_SHIFT_READ-CUR_BUF_SHIFT_FINAL));
      turfRates.errorFlag=turfioPtr->bufferDepth;
    }
      
    else if(wordNum<24) {
      switch(wordNum) {
      case 8:
	turfioPtr->trigType=dataChar; 
	turfioPtr->bufferDepth|=((dataChar&TRIG_BUF_MASK_READ)>>(TRIG_BUF_SHIFT_READ-TRIG_BUF_SHIFT_FINAL));
		    
	break;
      case 9:
	turfioPtr->l3Type1Count=dataChar; break;
      case 10:
	turfioPtr->trigNum=dataShort; break;
      case 11:
	turfioPtr->trigNum+=(dataShort<<8); break;
      case 12:
	turfioPtr->trigTime=dataInt; break;
      case 13:
	turfioPtr->trigTime+=(dataInt<<8); break;
      case 14:
	turfioPtr->trigTime+=(dataInt<<16); break;
      case 15:
	turfioPtr->trigTime+=(dataInt<<24); break;
      case 16:
	turfioPtr->ppsNum=dataShort; 
	turfRates.ppsNum=dataShort;break;
      case 17:
	turfRates.ppsNum+=(dataShort<<8);
	turfioPtr->ppsNum+=(dataShort<<8); break;
      case 18:
	turfioPtr->deadTime=(dataShort); break;
	turfRates.deadTime=(dataShort); break;
	
      case 19:
	turfioPtr->deadTime+=(dataShort<<8); break;
	turfRates.deadTime+=(dataShort<<8); break;
      case 20:
	turfioPtr->c3poNum=dataInt; break;
      case 21:
	turfioPtr->c3poNum+=(dataInt<<8); break;
      case 22:
	turfioPtr->c3poNum+=(dataInt<<16); break;
      case 23:
	turfioPtr->c3poNum+=(dataInt<<24); break;
      default:
	// Can't get here
	break;
      }
    }
    else if(wordNum<88) {
      //Surf Antenna counters
      phi=((wordNum-24)/2)%PHI_SECTORS;
      ring=((wordNum-24)/2)/PHI_SECTORS;
      if(wordNum%2==0) {
	//First word
	turfRates.l1Rates[phi][ring]=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfRates.l1Rates[phi][ring]+=(dataShort<<8);
      }	    
    }
    else if(wordNum<104) {
      turfRates.upperL2Rates[wordNum-88]=dataChar;
    }
    else if(wordNum<120) {
      turfRates.lowerL2Rates[wordNum-104]=dataChar;
    }
    else if(wordNum<136) {
      turfRates.l3Rates[wordNum-120]=dataChar;
    }
    else if(wordNum<138) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->upperL1TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->upperL1TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<140) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->lowerL1TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->lowerL1TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<142) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->upperL2TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->upperL2TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<144) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->lowerL2TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->lowerL2TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<146) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->l3TrigPattern=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->l3TrigPattern+=(dataShort<<8);
      }
    }
    else if(wordNum<148) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->otherTrigPattern[0]=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->otherTrigPattern[0]+=(dataShort<<8);
      }
    }	  
    else if(wordNum<150) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->otherTrigPattern[1]=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->otherTrigPattern[1]+=(dataShort<<8);
      }
    }	  
    else if(wordNum<152) {
      if(wordNum%2==0) {
	//First word
	turfioPtr->otherTrigPattern[2]=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfioPtr->otherTrigPattern[2]+=(dataShort<<8);
      }
    }	    	    	
    else if(wordNum<153) {
      turfioPtr->nadirL1TrigPattern=dataChar;
    }    	    	
    else if(wordNum<154) {
      turfioPtr->nadirL2TrigPattern=dataChar;
    }
    else if(wordNum<158) {
      //TURF Event Id
      switch(wordNum) {
      case 154:
	hdPtr->turfEventId=dataInt; break;
      case 155:
	hdPtr->turfEventId+=(dataInt<<8); break;
      case 156:
	hdPtr->turfEventId+=(dataInt<<16); break;
      case 157:
	hdPtr->turfEventId+=(dataInt<<24); break;
      default:
	break;
      }
    }
    else if(wordNum<174) {
      nadirAnt=(wordNum-158)/2;
      if(wordNum%2==0) {
	//First word
	turfRates.nadirL1Rates[nadirAnt]=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	turfRates.nadirL1Rates[nadirAnt]+=(dataShort<<8);
      }	    
    }
    else if(wordNum<182) {
      turfRates.nadirL2Rates[wordNum-166]=dataChar;
    }
    else if(wordNum<186) {
      //Antenna Mask
      switch(wordNum) {
      case 182:
	hdPtr->antTrigMask=dataInt; break;
      case 183:
	hdPtr->antTrigMask+=(dataInt<<8); break;
      case 184:
	hdPtr->antTrigMask+=(dataInt<<16); break;
      case 185:
	hdPtr->antTrigMask+=(dataInt<<24); break;
      default:
	break;
      }
    }
    else if(wordNum<188) {
      if(wordNum==186)
	hdPtr->nadirAntTrigMask=dataChar;
      //Ignore word 187 for now
    }
    else if(wordNum<190) {
      if(wordNum%2==0) {
	//First word
	hdPtr->phiTrigMask=dataShort;
      }
      else if(wordNum%2==1) {
	//Second word
	hdPtr->phiTrigMask+=(dataShort<<8);
      }	    
    }
    else if(wordNum<248) {
      //Reserved 
      //Do Nothing
    }
    else if(wordNum<256) {
      endPat.test[wordNum-248]=dataChar; 
    }	    	
  }
  for(wordNum=0;wordNum<8;wordNum++) {
    if(startPat.test[wordNum]!=endPat.test[wordNum]) 
      errCount++;
    if(verbosity && printToScreen) {
      printf("Test Pattern %d -- %x -- %x\n",wordNum,startPat.test[wordNum],
	     endPat.test[wordNum]);
    }
  }
  if(errCount) {
    //Flag for non matching TURF test patterns
    hdPtr->errorFlag|=(1<<3);
  }

  if(turfioPtr->deadTime>0 && lastPPSNum!=turfioPtr->ppsNum) {
    intervalDeadtime+=((float)turfioPtr->deadTime)/64400.;
    if(printToScreen && verbosity>1)
      printf("Deadtime %d counts %f fraction\n",turfioPtr->deadTime,
	     ((float)turfioPtr->deadTime)/64400.);
  }

    
  if(verbosity && printToScreen) {
    printf("TURF Data\n\tEvent (software):\t%u\n\tupperWord:\t0x%x\n\ttrigNum:\t%u\n\ttrigType:\t0x%x\n\ttrigTime:\t%u\n\tppsNum:\t\t%u\n\tc3p0Num:\t%u\n\tl3Type1#:\t%u\n\tdeadTime:\t\t%u\n",
	   hdPtr->eventNumber,hdPtr->turfUpperWord,turfioPtr->trigNum,turfioPtr->trigType,turfioPtr->trigTime,turfioPtr->ppsNum,turfioPtr->c3poNum,turfioPtr->l3Type1Count,turfioPtr->deadTime);
    printf("Trig Patterns:\nUpperL1:\t%x\nLowerL1:\t%x\nUpperL2:\t%x\nLowerL2:\t%x\nL31:\t%x\n",
	   turfioPtr->upperL1TrigPattern,
	   turfioPtr->lowerL2TrigPattern,
	   turfioPtr->upperL2TrigPattern,
	   turfioPtr->lowerL2TrigPattern,
	   turfioPtr->l3TrigPattern);
  }

  if(turfRates.ppsNum!=lastPPSNum) { //When the PPS isn't present won't get this
    newTurfRateData=1;
  }
  //Make sure to copy relevant mask data to turfRate struct
  turfRates.antTrigMask=hdPtr->antTrigMask;
  turfRates.nadirAntTrigMask=hdPtr->nadirAntTrigMask;
  turfRates.phiTrigMask=hdPtr->phiTrigMask;
  lastPPSNum=turfRates.ppsNum;
  return status;	
}



AcqdErrorCode_t readTurfHkData()   
{
    //This fills a TurfRate structure
    unsigned int uvalue;
    unsigned short usvalue,usvalue2;
    unsigned char ucvalue,ucvalue2,ucvalue3,ucvalue4;
    int ring=0,phi=0;

    int i=0;

    memset(&theTurfReg,0,sizeof(TurfRegisterContents_t));    
    memset(&turfRates,0,sizeof(TurfRateStruct_t));
    newTurfRateData=0;
    AcqdErrorCode_t status=setTurfioReg(262, 3); // set TURF_BANK to Bank 3
    if(status!=ACQD_E_OK)
	return status;
    
    for(i=0;i<TURF_BANK_SIZE;i++) {
	status=readTurfioReg(i,&uvalue);
	theTurfReg.values[i]=uvalue;
	if(i<16) {
	    //L1 rates
	    usvalue=uvalue&0xffff;
	    usvalue2=(uvalue&0xffff0000)>>16;
	    ring=(i<8); //0 is upper, 1 is lower
	    phi=i*2;
	    if(phi>=16) phi-=16;
	    turfRates.l1Rates[phi][ring]=usvalue;
	    turfRates.l1Rates[phi+1][ring]=usvalue2;	
	}
	else if(i<20) {
	    ucvalue=uvalue&0xff;
	    ucvalue2=(uvalue&0xff00)>>8;
	    ucvalue3=(uvalue&0xff0000)>>16;
	    ucvalue4=(uvalue&0xff000000)>>24;
	    phi=(i-16)*4;
	    turfRates.upperL2Rates[phi]=ucvalue;
	    turfRates.upperL2Rates[phi+1]=ucvalue2;
	    turfRates.upperL2Rates[phi+2]=ucvalue3;
	    turfRates.upperL2Rates[phi+3]=ucvalue4;
	}
	else if(i<24) {
	    ucvalue=uvalue&0xff;
	    ucvalue2=(uvalue&0xff00)>>8;
	    ucvalue3=(uvalue&0xff0000)>>16;
	    ucvalue4=(uvalue&0xff000000)>>24;
	    phi=(i-20)*4;
	    turfRates.lowerL2Rates[phi]=ucvalue;
	    turfRates.lowerL2Rates[phi+1]=ucvalue2;
	    turfRates.lowerL2Rates[phi+2]=ucvalue3;
	    turfRates.lowerL2Rates[phi+3]=ucvalue4;
	}
	else if(i<28) {
	    ucvalue=uvalue&0xff;
	    ucvalue2=(uvalue&0xff00)>>8;
	    ucvalue3=(uvalue&0xff0000)>>16;
	    ucvalue4=(uvalue&0xff000000)>>24;
	    phi=(i-24)*4;
	    turfRates.l3Rates[phi]=ucvalue;
	    turfRates.l3Rates[phi+1]=ucvalue2;
	    turfRates.l3Rates[phi+2]=ucvalue3;
	    turfRates.l3Rates[phi+3]=ucvalue4;
	}
	else if(i<32) {
	    //Nadir L1 rates
	    usvalue=uvalue&0xffff;
	    usvalue2=(uvalue&0xffff0000)>>16;
	    phi=(i-28)*2; //Only counts to 8
	    turfRates.nadirL1Rates[phi]=usvalue;
	    turfRates.nadirL1Rates[phi+1]=usvalue2;	
	}
	else if(i<34) {
	    //Nadir L2 Rates
	    ucvalue=uvalue&0xff;
	    ucvalue2=(uvalue&0xff00)>>8;
	    ucvalue3=(uvalue&0xff0000)>>16;
	    ucvalue4=(uvalue&0xff000000)>>24;
	    phi=(i-32)*4; //Only counts to 8
	    turfRates.nadirL2Rates[phi]=ucvalue;
	    turfRates.nadirL2Rates[phi+1]=ucvalue2;	
	}
	else if(i<38) {
	    //Reserved for now
	}
	else if(i==38) {
	    //C3ponum 133
	}
	else if(i==39) {
	    //C3ponum 250
	}
	else if(i==40) {
	    //C3ponum 33
	}
	else if(i==41) {
	    usvalue=uvalue&0xffff;
	    usvalue2=(uvalue&0xffff0000)>>16;
	    turfRates.ppsNum=usvalue;	    
	    turfRates.deadTime=usvalue2;
	}
	else if(i==42) {
	    ucvalue=uvalue&0xff;
	    turfRates.errorFlag=ucvalue&0xf;
	}
	    
    }
    
    if(turfRates.ppsNum!=lastPPSNum) { //When the PPS isn't present won't get this
	newTurfRateData=1;
    }
    //Make sure to copy relevant mask data to turfRate struct
    status=setTurfioReg(262, 0); // set TURF_BANK to Bank 0
    if(status!=ACQD_E_OK)
	return status;

    //Ant Trig Mask
    status=readTurfioReg(4,&uvalue);   
    turfRates.antTrigMask=uvalue;
    
    status=readTurfioReg(5,&uvalue);   
    turfRates.nadirAntTrigMask=uvalue&0xff;

    status=readTurfioReg(6,&uvalue);   
    turfRates.phiTrigMask=uvalue;

    lastPPSNum=turfRates.ppsNum;
    return status;
}



AcqdErrorCode_t writeAntTrigMask() 
/*!
  Remember that 0 in the RFCM mask means on
*/
{
  AcqdErrorCode_t status;
  int i=0,j=0,chanBit=1;
  unsigned int gpioVal;
  int actualMask[2]={0,0};
  
  actualMask[0]|=(antTrigMask&0xffff); //Lowest 16-bits
  actualMask[0]|=((nadirAntTrigMask&0xff)<<16);
  actualMask[0]|=((antTrigMask&0xff0000)<<8);
  actualMask[1]=(antTrigMask&0xff000000)>>24;
  syslog(LOG_INFO,"antTrigMask: %#x  actualMask[1]:%#x   actualMask[0]:%#x\n",
	 antTrigMask,actualMask[1],actualMask[0]);
  
  status=setTurfControl(RFCBit);
  if(status!=ACQD_E_OK) {		
    syslog(LOG_ERR,"Failed to set 1st RFCBit %d",status);		    
    fprintf(stderr,"Failed to set 1st RFCBit %d.\n",status) ;
  }	
  
  //Now send bit pattern to TURF using RFCBit and RFCClk
  while(i<40) {
    if (i==32) j=chanBit=1 ;	
    //	printf("Debug RFCM: i=%d j=%d chn_bit=%d mask[j]=%d\twith and %d\n",i,j,chanBit,actualMask[j],(actualMask[j]&chanBit));
    if(actualMask[j]&chanBit) status=setTurfControl(RFCBit);
    else status=setTurfControl(RFCClk);
    if (status != ACQD_E_OK) {
      syslog(LOG_ERR,"Failed to send RFC bit %d to TURF, %x %x",
	     i,actualMask[1],actualMask[0]);
      fprintf(stderr,"Failed to send RFC bit %d to TURF, %x %x\n",
	      i,actualMask[1],actualMask[0]);
    }	
    chanBit<<=1;
    i++;
  }

  // check GPIO8 is set or not. 
  status=readTurfGPIOValue(&gpioVal);
  if(status!=ACQD_E_OK) {
    syslog(LOG_ERR,"Error reading TURFIO GPIO Val -- %s\n",strerror(errno));
    fprintf(stderr,"Error reading TURFIO GPIO Val -- %s\n",strerror(errno));
  }
  
  if(gpioVal & 0400000000) {
    syslog(LOG_INFO,"writeAntTrigMask: GPIO8 on, so mask should be set to %#04x %#010x.\n", actualMask[1],actualMask[0]) ;
    if(printToScreen) 
      printf("writeAntTrigMask: GPIO8 on, so mask should be set to %#04x %#010x.\n", actualMask[1], actualMask[0]) ;
  }   
  else {
    syslog(LOG_ERR,"writeAntTrigMask: GPIO 8 not on: GPIO=%o",gpioVal);
    fprintf(stderr,"writeAntTrigMask: GPIO 8 not on: GPIO=%o\n",gpioVal);
    return ACQD_E_TURFIO_GPIO;
  }
  return ACQD_E_OK;
    
}


int updateThresholdsUsingPID() {
  static int avgCount=0;
  int wayOffCount=0;
  int surf,dac,error,value,change,band;
  float pTerm, dTerm, iTerm;
  int chanGoal;
  avgCount++;
  for(surf=0;surf<numSurfs;surf++) {
    if(dacSurfs[surf]==1) {
      for(dac=0;dac<SCALERS_PER_SURF;dac++) {
	band=dac%4;
	chanGoal=(int)(pidGoals[band]*pidGoalScaleFactors[surf][dac]);
	if(chanGoal<0) chanGoal=0;
	if(chanGoal>16000) chanGoal=16000;
	if(abs(theSurfHk.scaler[surf][dac]-chanGoal)>pidPanicVal)
	  wayOffCount++;
	avgScalerData[surf][dac]+=theSurfHk.scaler[surf][dac];
      }
    }
  }
  //    if(wayOffCount>100) {
  //	printf("Way off count %d (avgCount %d)\n",wayOffCount,avgCount);
  //    }
 
  
  if(avgCount==pidAverage || wayOffCount>100) {
    if(printPidStuff) {
      printf("PID Status:Band Goals:");
      for(band=0;band<BANDS_PER_ANT;band++) 
	printf(" %d",pidGoals[band]);
      printf("\n");
    }
    for(surf=0;surf<numSurfs;surf++) {
      if(printPidStuff)
	printf("SURF %2d: ",surfIndex[surf]);
      for(dac=0;dac<SCALERS_PER_SURF;dac++) {		
	band=dac%4;
	chanGoal=(int)(pidGoals[band]*pidGoalScaleFactors[surf][dac]);
	if(chanGoal<0) chanGoal=0;
	if(chanGoal>16000) chanGoal=16000;
	if(dacSurfs[surf]==1) {
	  value=avgScalerData[surf][dac]/avgCount;
	  avgScalerData[surf][dac]=0;
	  error=chanGoal-value;
	  if(printPidStuff)
	    printf("%4d ",value);

	  //		    printf("%d %d %d %d\n",thePids[surf][dac].iState,
	  //			   avgScalerData[surf][dac],value,error);
	    
	  // Proportional term
	  pTerm = dacPGain * error;
		    
	  // Calculate integral with limiting factors
	  thePids[surf][dac].iState+=error;
	  //		printf("Here %d %d\n",thePids[surf][dac].iState,dacIMax);
	  if (thePids[surf][dac].iState > dacIMax) 
	    thePids[surf][dac].iState = dacIMax;
	  else if (thePids[surf][dac].iState < dacIMin) 
	    thePids[surf][dac].iState = dacIMin;
		    
	  // Integral and Derivative Terms
	  iTerm = dacIGain * (float)(thePids[surf][dac].iState);  
	  dTerm = dacDGain * (float)(value -thePids[surf][dac].dState);
	  thePids[surf][dac].dState = value;
		    
	  //Put them together		   
	  change = (int) (pTerm + iTerm - dTerm);
	  //		    if(surf==0 && dac==16) {
	  //			printf("thresh %d, change %d (scaler %d) (surf %d, dac %d)\n",
	  //			       thresholdArray[surfIndex[surf]-1][dac],change,value,surfIndex[surf]-1,dac);
	  //		    }
	  thresholdArray[surfIndex[surf]-1][dac]+=change;
	  if(thresholdArray[surfIndex[surf]-1][dac]>4095)
	    thresholdArray[surfIndex[surf]-1][dac]=4095;
	  if(thresholdArray[surfIndex[surf]-1][dac]<1)
	    thresholdArray[surfIndex[surf]-1][dac]=1;
	  //		printf("%d %d\n",thePids[surf][dac].dState,
	  //		       thePids[surf][dac].iState);
	  //		printf("%d %d %f %f %f\n",change,
	  //		       thresholdArray[surfIndex[surf]-1][dac],
	  //		       pTerm,iTerm,dTerm);
	}
      }
      if(printPidStuff)
	printf("\n");
    }
    avgCount=0;
    return 1;
  }
  return 0;
}
 

void makeSubAltDir() {
  int dirNum;
  madeAltDir=1;
  dirNum=MAX_EVENTS_PER_DIR*(int)(hdPtr->eventNumber/MAX_EVENTS_PER_DIR);
  sprintf(subAltOutputdir,"%s/ev%d",altOutputdir,dirNum);
  makeDirectories(subAltOutputdir);
}

void prepWriterStructs() {
  int diskInd=0;
  if(printToScreen) 
    printf("Preparing Writer Structs\n");

  //Turf Hk Writer
  strncpy(turfHkWriter.relBaseName,TURFHK_ARCHIVE_DIR,FILENAME_MAX-1);
  sprintf(turfHkWriter.filePrefix,"turfhk");
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
    turfHkWriter.currentFilePtr[diskInd]=0;
  turfHkWriter.writeBitMask=hkDiskBitMask;

  //Surf Hk Writer
  strncpy(surfHkWriter.relBaseName,SURFHK_ARCHIVE_DIR,FILENAME_MAX-1);
  sprintf(surfHkWriter.filePrefix,"surfhk");
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
    surfHkWriter.currentFilePtr[diskInd]=0;
  surfHkWriter.writeBitMask=hkDiskBitMask;

  //Averaged Surf Hk Writer
  strncpy(avgSurfHkWriter.relBaseName,SURFHK_ARCHIVE_DIR,FILENAME_MAX-1);
  sprintf(avgSurfHkWriter.filePrefix,"avgsurfhk");
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
    avgSurfHkWriter.currentFilePtr[diskInd]=0;
  avgSurfHkWriter.writeBitMask=hkDiskBitMask;

  //Raw Scaler Hk Writer
  strncpy(rawScalerWriter.relBaseName,SURFHK_ARCHIVE_DIR,FILENAME_MAX-1);
  sprintf(rawScalerWriter.filePrefix,"rawscalers");
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
    rawScalerWriter.currentFilePtr[diskInd]=0;
  rawScalerWriter.writeBitMask=hkDiskBitMask;


  //Summed Turf Rate Writer
  strncpy(sumTurfRateWriter.relBaseName,TURFHK_ARCHIVE_DIR,FILENAME_MAX-1);
  sprintf(sumTurfRateWriter.filePrefix,"sumturfhk");
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
    sumTurfRateWriter.currentFilePtr[diskInd]=0;
  sumTurfRateWriter.writeBitMask=hkDiskBitMask;

  //Event Writer
  if(useAltDir) 
    strncpy(eventWriter.relBaseName,altOutputdir,FILENAME_MAX-1);
  eventWriter.currentHeaderFilePtr[0]=0;
  eventWriter.currentEventFilePtr[0]=0;
  eventWriter.writeBitMask=1;
  sprintf(eventWriter.filePrefix,"ev");
}


int getChanIndex(int surf, int chan) {
  //Assumes chan 0-8 and surf 0-9
  return chan + CHANNELS_PER_SURF*surf;

}
	

void myUsleep(int usec)
{
  struct timespec waitTime;
  struct timespec retTime;
  waitTime.tv_sec=0;
  waitTime.tv_nsec=usec*1000;
    
  nanosleep(&waitTime,&retTime);
}


void servoOnRate(unsigned int eventNumber, unsigned int lastRateCalcEvent, struct timeval *currentTime, struct timeval *lastRateCalcTime)
{
  static float dState=0;  // Last position input
  static float iState=0;  // Integrator state
  float timespan=(currentTime->tv_sec-lastRateCalcTime->tv_sec) 
    + 1e-6*(float)(currentTime->tv_usec)
    -1e-6*(float)(lastRateCalcTime->tv_usec);
  float rate=((float)(eventNumber-lastRateCalcEvent))/timespan;    
  float error;
  int change,setting,band;
  float pTerm, dTerm, iTerm;
  


  if(!setGlobalThreshold) 
    setting=(pidGoals[0]+pidGoals[1]+pidGoals[2]+pidGoals[3])/4; 
  else
    setting=globalThreshold;
    
  error=rateGoal-rate;
    
  // Proportional term
  pTerm = ratePGain * error;
    
  // Calculate integral with limiting factors
  iState+=error;
  if (iState > rateIMax) 
    iState = rateIMax;
  else if (iState < rateIMin) 
    iState = rateIMin;
		    
  // Integral and Derivative Terms
  iTerm = rateIGain * iState;  
  dTerm = rateDGain * (rate-dState);
  dState = rate;
		    
  //Put them together		  
  printf("p %f\t + i %f\t -d %f\n",pTerm,iTerm,dTerm);
  change = (int) (pTerm + iTerm - dTerm);
  printf("Rate %2.2f Hz, Goal %2.2f Hz, Setting %d -- Change %d\n",
	 rate,rateGoal,setting,change);
    
  if(change!=0) {
    if(!setGlobalThreshold) {
      for(band=0;band<BANDS_PER_ANT;band++) {
	pidGoals[band]+=((int)((float)change*pidGoals[band])/((float)setting));
	if(pidGoals[band]<1) pidGoals[band]=1;
	if(pidGoals[band]>10000) pidGoals[band]=10000;
      }
      syslog(LOG_INFO,"Changing pidGoals to %d %d %d %d\n",
	     pidGoals[0],pidGoals[1],pidGoals[2],pidGoals[3]);
      if(printToScreen)
	fprintf(stdout,"Changing pidGoals to %d %d %d %d\n",
		pidGoals[0],pidGoals[1],pidGoals[2],pidGoals[3]);
    }
    else {
      globalThreshold=setting+change;
      if(globalThreshold<1) globalThreshold=1;
      if(globalThreshold>4095) globalThreshold=4095;
      syslog(LOG_INFO,"Changing globalThreshold to %d\n",globalThreshold);
      if(printToScreen)
	fprintf(stdout,"Changing globalThreshold to %d\n",globalThreshold);
    }    
  }
}


void handleBadSigs(int sig)
{
  static int numHere=0;
  fprintf(stderr,"Received sig %d -- will exit immediately\n",sig); 
  syslog(LOG_WARNING,"Received sig %d -- will exit immediately\n",sig); 
  if(numHere==0) {
    numHere++;    
    closeDevices();
    closeHkFilesAndTidy(&sumTurfRateWriter);
    closeHkFilesAndTidy(&avgSurfHkWriter);
    closeHkFilesAndTidy(&rawScalerWriter);
    closeHkFilesAndTidy(&surfHkWriter);
    closeHkFilesAndTidy(&turfHkWriter);
  }
  unlink(ACQD_PID_FILE);
  syslog(LOG_INFO,"Acqd terminating");
  exit(0);
}


void handleSlowRate(struct timeval *tvPtr, unsigned int lastEvNum)
{
  static struct timeval lastSlowRateCalc; //Every 60sec   
  static unsigned int lastSlowRateEvent=0; 
  static int firstTime=1;
  //    unsigned int lastSlowTurfRate=0;
  if(firstTime) {
    lastSlowRateCalc.tv_sec=time(NULL);
    lastSlowRateCalc.tv_usec=0;
    firstTime=0;
  }
  if((tvPtr->tv_sec-lastSlowRateCalc.tv_sec)>60) {
    if(doingEvent) {
      float slowRate=doingEvent-lastSlowRateEvent;
      float slowTime=tvPtr->tv_sec-lastSlowRateCalc.tv_sec;
      slowTime+=1e-6*((float)tvPtr->tv_usec);
      slowTime-=1e-6*((float)lastSlowRateCalc.tv_usec);
      slowRate/=slowTime;
      writeCurrentRFSlowRateObject(slowRate,lastEvNum);
      lastSlowRateCalc.tv_sec=tvPtr->tv_sec;
      lastSlowRateCalc.tv_usec=tvPtr->tv_usec;
      lastSlowRateEvent=doingEvent;
    }
    else {			      
      writeCurrentRFSlowRateObject(0,0);
      lastSlowRateCalc.tv_sec=tvPtr->tv_sec;
      lastSlowRateCalc.tv_usec=tvPtr->tv_usec;
    }
  }
}


void rateCalcAndServo(struct timeval *tvPtr, unsigned int lastEvNum)
//Calculates the rate and maybe servos thresholds
{
  static struct timeval lastRateCalc;
  static struct timeval lastServoRateCalc;
  static unsigned int lastRateCalcEvent=0; 
  static int firstTime=1;
  static int lastEventCounter=0;


  float rateCalcPeriod;
  
  if(firstTime) {
    lastRateCalc.tv_sec=tvPtr->tv_sec;
    lastRateCalc.tv_usec=0;
    lastServoRateCalc.tv_sec=0;
    lastServoRateCalc.tv_usec=0;
    lastEventCounter=doingEvent;
    totalDeadtime=0;
    intervalDeadtime=0;
    firstTime=0;
  }
  
  if((tvPtr->tv_sec-lastRateCalc.tv_sec)>calculateRateAfter) {
    
    //		  printf("Calculating rate %d %d %d\n",(int)tvPtr->tv_sec,(int)lastRateCalc.tv_sec,calculateRateAfter);
    
    if(enableRateServo) {
      if(((tvPtr->tv_sec-lastServoRateCalc.tv_sec)>servoRateCalcPeriod) || ((doingEvent-lastRateCalcEvent)>(servoRateCalcPeriod*rateGoal))) {
	//Time to servo on rate
	servoOnRate(doingEvent,lastRateCalcEvent,tvPtr,&lastServoRateCalc);
	lastRateCalcEvent=doingEvent;
	lastServoRateCalc.tv_sec=tvPtr->tv_sec;
	lastServoRateCalc.tv_usec=tvPtr->tv_usec;
	
      }
    }
    
    rateCalcPeriod=0;
    //Make rate calculation;
    if(lastRateCalc.tv_sec>0) {
      rateCalcPeriod=
	tvPtr->tv_sec-lastRateCalc.tv_sec;
      rateCalcPeriod+=((float)tvPtr->tv_usec-
		       lastRateCalc.tv_usec)/1e6;
      totalTime+=rateCalcPeriod;
		    
      //Deadtime Monitoring
      totalDeadtime+=intervalDeadtime;
    }
		  
    if(rateCalcPeriod) {
      if((doingEvent-lastEventCounter)>0 && rateCalcPeriod) {
	printf("Event %d -- Current Rate %3.2f Hz\n",lastEvNum,((float)(doingEvent-lastEventCounter))/rateCalcPeriod);
	//		    if(lastEventCounter<200)
	//			printf("\n");
      }
      else {
	printf("Event %d -- Current Rate 0 Hz\n",lastEvNum);
      }
		    
		    
      printf("\tTotal Time %3.1f (s)\t Total Deadtime %3.1f (s) (%3.2f %%)\n",totalTime,totalDeadtime,100.*(totalDeadtime/totalTime));
      printf("\tInterval Time %3.1f (s)\t Interval Deadtime %3.1f (s) (%3.2f %%)\n",rateCalcPeriod,intervalDeadtime,100.*(intervalDeadtime/rateCalcPeriod));
      intervalDeadtime=0;
    }			    
    lastRateCalc=(*tvPtr);
    lastEventCounter=doingEvent;		  
  }
}


void intersperseTurfRate(struct timeval *tvPtr)
{
    static time_t lastTurfRateRead=0;
    unsigned int timeDiff=tvPtr->tv_sec-lastTurfRateRead;
    if(turfFirmwareVersion>=5) {
	if(timeDiff>0) {
	    readTurfHkData();
	    lastTurfRateRead=tvPtr->tv_sec;
	    if(newTurfRateData) outputTurfRateData();		    	
	}
    }

}

void intersperseSurfHk(struct timeval *tvPtr)
{
  AcqdErrorCode_t status;
  int timeDiff=0;  
  int surf;
  static unsigned int lastSlowSurfHk=0;  
  timeDiff=tvPtr->tv_usec-lastSurfHkRead.tv_usec;  
  timeDiff+=1000000*(tvPtr->tv_sec-lastSurfHkRead.tv_sec);
		    		
  //Only read the housekeeping every 50ms, will change this to be configurable.
  //Maybe.
  if(timeDiff>50000 || lastSurfHkRead.tv_sec==0) {
    //printf("Time Diff %d, %d.%d and %d.%d\n",
    //       timeDiff,tvPtr->tv_sec,tvPtr->tv_usec,lastSurfHkRead.tv_sec,lastSurfHkRead.tv_usec);
    lastSurfHkRead.tv_sec=tvPtr->tv_sec;
    lastSurfHkRead.tv_usec=tvPtr->tv_usec;
    status=readSurfHkData();
    //Should check status
	
    for(surf=0;surf<numSurfs;++surf) {
      if (setSurfControl(surf,SurfClearHk) != ACQD_E_OK) {
	fprintf(stderr,"Failed to send clear hk pulse on SURF %d.\n",surfIndex[surf]) ;
	syslog(LOG_ERR,"Failed to send clear hk pulse on SURF %d.\n",surfIndex[surf]) ;
	//Do something??
      }
    }
    
    if(verbosity && printToScreen) 
      printf("Read SURF Housekeeping\n");
    
    if(tvPtr->tv_sec>lastSlowSurfHk) {
      addSurfHkToAverage(&theSurfHk);
      lastSlowSurfHk=tvPtr->tv_sec;
    }
    
    if(verbosity && printToScreen && enableChanServo) 
      printf("Will servo on channel scalers\n");

    //Here is the channel servo
    if(enableChanServo) {
      if(updateThresholdsUsingPID())
	setDACThresholds();
    }
				
		      
    //Now decide if we want to output the housekeeping data
    theScalers.unixTime=tvPtr->tv_sec;
    theScalers.unixTimeUs=tvPtr->tv_usec;
    theSurfHk.unixTime=tvPtr->tv_sec;
    theSurfHk.unixTimeUs=tvPtr->tv_usec;
    outputSurfHkData();

  }		    
}

void intersperseSoftTrig(struct timeval *tvPtr)
{          
  static int lastSoftTrigTime=0;  

  //Send software trigger if we want to
  if(sendSoftTrigger && doingEvent>=0) {
    if(!softTrigPeriod || (tvPtr->tv_sec-lastSoftTrigTime)>=softTrigPeriod) {		    		
      setTurfControl(SendSoftTrg);
      lastSoftTrigTime=tvPtr->tv_sec;
      if(printToScreen && verbosity)
	printf("Sent software trigger at %d: next at %d\n",
	       lastSoftTrigTime, 
	       lastSoftTrigTime+softTrigPeriod);
      
    }
  }
}

int checkSurfFdsForData()
{
  //Unused and unworkable?
  static struct timeval read_timeout;
  static struct timeval * read_timeout_ptr;
  static fd_set read_fds;
  int retVal=0;
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = 50000;
  read_timeout_ptr=&read_timeout;
  
  FD_ZERO(&read_fds);
  FD_SET(surfFds[0], &read_fds);
  retVal = select(surfFds[0]+1, &read_fds,NULL, NULL, read_timeout_ptr);
  
  printf("checkSurfFdsForData: retVal %d\n",retVal);
  return retVal;
}



AcqdErrorCode_t readTurfioReg(unsigned int address, unsigned int *valPtr)
{
  struct turfioDriverRead req;
  req.address = address;
  if (ioctl(turfioFd, TURFIO_IOCREAD, &req)) {
      syslog(LOG_ERR,"Error reading TURFIO register -- %s\n",strerror(errno));
      fprintf(stderr,"Error reading TURFIO register -- %s\n",strerror(errno));
      return ACQD_E_TURFIO_IOCTL;
  }
  *valPtr=req.value;
  return ACQD_E_OK;
}

AcqdErrorCode_t setTurfioReg(unsigned int address,unsigned int value)
{
  struct turfioDriverRead req;
  req.address = address; 
  req.value = value;
  if (ioctl(turfioFd, TURFIO_IOCWRITE, &req)) {
      syslog(LOG_ERR,"Error writing TURFIO register -- %s\n",strerror(errno));
      fprintf(stderr,"Error writing TURFIO register -- %s\n",strerror(errno));
      return ACQD_E_TURFIO_IOCTL;
  }
  return ACQD_E_OK;
}
