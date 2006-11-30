/*  \file Acqd.c
    \brief The Acqd program 
    The first functional version of the acquisition software to work with SURF version 3. For now this guy will try to have the same functionality as crate_test, but will look less like a dead dog. 
    March 2006 rjn@mps.ohio-state.edu
*/

// Standard Includes
#define _GNU_SOURCE

#include "Acqd.h"


#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
//#include <iostream>



// Anita includes
#include "includes/anitaCommand.h"
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "pedestalLib/pedestalLib.h"
#include "slowLib/slowLib.h"
#include "includes/anitaStructures.h"



//#define TIME_DEBUG 1
//#define BAD_DEBUG 1
#define HK_DEBUG 0
void servoOnRate(unsigned long eventNumber, unsigned long lastRateCalcEvent, struct timeval *currentTime, struct timeval *lastRateCalcTime);

inline unsigned short byteSwapUnsignedShort(unsigned short a){
    return (a>>8)+(a<<8);
}


//Global Variables
BoardLocStruct_t turfioPos;
BoardLocStruct_t surfPos[MAX_SURFS];
int dacSurfs[MAX_SURFS];
int surfIndex[MAX_SURFS];
int dacChans[NUM_DAC_CHANS];
int printToScreen=0,standAloneMode=0;
int numSurfs=0,doingEvent=0,hkNumber=0;
int slipCounter=0;
int turfRateTelemEvery=1,turfRateTelemInterval=60;
int surfHkPeriod=0;
int surfHkTelemEvery=0;
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
AnitaEventBody_t *bdPtr;//=&(theEvent.header);
TurfioStruct_t *turfioPtr;//=&(hdPtr->turfio);
TurfRateStruct_t turfRates;
int newTurfRateData=0;


SimpleScalerStruct_t theScalers;
FullSurfHkStruct_t theSurfHk;

int eventsBeforeClear=1000;


//Pedestal stuff
int pedestalMode=0;
int writeDebugData=0;
int numPedEvents=4000;
int pedSwitchConfigAtEnd=1;


//Temporary Global Variables
unsigned int labData[MAX_SURFS][N_CHAN][N_SAMP];
unsigned int avgScalerData[MAX_SURFS][N_RFTRIG];
//unsigned short threshData[MAX_SURFS][N_RFTRIG];
//unsigned short rfpwData[MAX_SURFS][N_RFCHAN];



//Configurable watchamacallits
/* Ports and directories */
//char acqdEventDir[FILENAME_MAX];
char altOutputdir[FILENAME_MAX];
char subAltOutputdir[FILENAME_MAX];
char acqdPidFile[FILENAME_MAX];
//char acqdEventLinkDir[FILENAME_MAX];
char lastEventNumberFile[FILENAME_MAX];

#define N_TMO 100    /* number of msec wait for Evt_f before timed out. */
#define N_TMO_INT 10000 //Millisec to wait for interrupt
#define N_FTMO 200    /* number of msec wait for Lab_f before timed out. */
//#define N_FTMO 2000    /* number of msec wait for Lab_f before timed out. */
//#define DAC_CHAN 4 /* might replace with N_CHIP at a later date */
#define EVTF_TIMEOUT 10000000 /* in microseconds */

int firmwareVersion=2;
int turfFirmwareVersion=3;
int verbosity = 0 ; /* control debug print out. */
int addedVerbosity = 0 ; /* control debug print out. */
int oldStyleFiles = FALSE;
int useAltDir = FALSE;
int madeAltDir = FALSE;
int selftrig = FALSE ;/* self-trigger mode */
int surfonly = FALSE; /* Run in surf only mode */
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
int softTrigSleepPeriod = 0;
int reprogramTurf = FALSE;

//Trigger Modes
TriggerMode_t trigMode=TrigNone;
int pps1TrigFlag = 0;
int pps2TrigFlag = 0;

//Threshold Stuff
//int thresholdArray[ACTIVE_SURFS][DAC_CHAN][PHYS_DAC];
int surfTrigBandMasks[ACTIVE_SURFS][2];
int thresholdArray[ACTIVE_SURFS][N_RFTRIG];
float pidGoalScaleFactors[ACTIVE_SURFS][N_RFTRIG];
int setGlobalThreshold=0; 
int globalThreshold=2200;
DacPidStruct_t thePids[ACTIVE_SURFS][N_RFTRIG];
float dacIGain;  // integral gain
float dacPGain;  // proportional gain
float dacDGain;  // derivative gain
int dacIMax;  // maximum intergrator state
int dacIMin; // minimum integrator state
int enableChanServo = FALSE; //Turn on the individual chanel servo
int pidGoal;
int pidPanicVal;
int pidAverage;
int numEvents=0;
//time_t lastRateTime;
int lastEventCounter=0;
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
unsigned int antTrigMask=0;


//Test of BarMap
//U32 barMapDataWord[ACTIVE_SURFS];
__volatile__ int *barMapAddr[ACTIVE_SURFS];
__volatile__ int *turfBarMap;


//File writing structures
AnitaHkWriterStruct_t turfHkWriter;
AnitaHkWriterStruct_t surfHkWriter;
AnitaEventWriterStruct_t eventWriter;


#ifdef TIME_DEBUG
FILE *timeFile;
#endif    

//Deadtime monitoring
float totalDeadtime; //In Secs
float totalTime; //In Secs
float intervalDeadtime; //In secs



//Nasty fix as SURF 2 is the only one we monitor busy on
int surfClearIndex[9]={0,2,3,4,5,6,7,8,1};


int main(int argc, char **argv) {
    char reniceCommand[FILENAME_MAX];
    PlxHandle_t surfHandles[MAX_SURFS];
    PlxHandle_t turfioHandle;
    PlxHandle_t lint1Handle;
    PlxIntr_t plxIntr, plxState;
    PlxDevLocation_t surfLocation[MAX_SURFS];
    PlxDevLocation_t turfioLocation;
    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    int retVal=0;
//    unsigned short  dataWord=0;
// Read only one chip per trigger. PM

    int i=0,tmo=0; 
    int numDevices; 
    int tmpGPIO,tmpINTR;
    unsigned short dacVal=2200;
    int surf;
    int gotSurfHk=0;
    int threshScanCounter=0;
    unsigned short doingDacVal=1;//2100;
    unsigned long lastEvNum=0;
    struct timeval timeStruct;
    struct timeval lastRateCalc;
    struct timeval lastServoRateCalc;
    struct timeval lastSurfHkRead;
    int timeDiff=0;
    unsigned long lastRateCalcEvent=0;
    
    unsigned long lastSlowRateEvent=0; 
    unsigned long lastSlowSurfHk=0;
//    unsigned long lastSlowTurfRate=0;

    struct timeval lastSlowRateCalc; //Every 60sec

    lastSlowRateCalc.tv_sec=time(NULL);
    lastSlowRateCalc.tv_usec=0;

    lastRateCalc.tv_sec=time(NULL);
    lastRateCalc.tv_usec=0;
    lastServoRateCalc.tv_sec=0;
    lastServoRateCalc.tv_usec=0;
    lastSurfHkRead.tv_sec=0;
    lastSurfHkRead.tv_usec=0;
    float rateCalcPeriod;
    totalDeadtime=0;
    intervalDeadtime=0;
    int firstLoop=1;
    int reInitNeeded=0;
    int lastSofTrigTime=0;

#ifdef TIME_DEBUG
    struct timeval timeStruct2;
    timeFile = fopen("/tmp/testTimeLog.txt","w");
    if(!timeFile) {
	fprintf(stderr,"Couldn't open time file\n");
	exit(0);
    }
#endif

    /* Log stuff */
    char *progName=basename(argv[0]);
#ifdef BAD_DEBUG
    printf("Starting %s\n",progName);
#endif
    //Initialize handy pointers
    hdPtr=&(theEvent.header);
    bdPtr=&(theEvent.body);
    turfioPtr=&(hdPtr->turfio);

    //Initialize dacPid stuff
    memset(&thePids,0,sizeof(DacPidStruct_t)*ACTIVE_SURFS*N_RFTRIG);    
   
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, sigUsr2Handler);
    signal(SIGINT, sigUsr2Handler);

    //Dont' wait for children
    signal(SIGCLD, SIG_IGN); 

    makeDirectories(HEADER_TELEM_LINK_DIR);
    makeDirectories(PEDESTAL_DIR);
    makeDirectories(ACQD_EVENT_LINK_DIR);
    makeDirectories(SURFHK_TELEM_LINK_DIR);
    makeDirectories(TURFHK_TELEM_LINK_DIR);
    makeDirectories(CMDD_COMMAND_LINK_DIR);

    retVal=readConfigFile();
#ifdef BAD_DEBUG
    printf("Read config %d\n",retVal);
#endif

    if(retVal!=CONFIG_E_OK) {
	syslog(LOG_ERR,"Error reading config file Acqd.config: bailing");
	fprintf(stderr,"Error reading config file Acqd.config: bailing\n");
	return -1;
    }    

    if(standAloneMode) {
	init_param(argc, argv,  &numEvents, &dacVal) ;
//	if (dir_n == NULL) dir_n = "./data" ; /* this is default directory name. */
    }

    // Initialize devices
    if ((numDevices = initializeDevices(surfHandles,&turfioHandle,
					surfLocation,&turfioLocation)) < 0) {
	if(printToScreen) printf("Problem initializing devices\n");
	syslog(LOG_ERR,"Error initializing devices");
	return -1 ;
    }

    
    rc = setBarMap(surfHandles,turfioHandle);
    if(rc!=ApiSuccess) {
	printf("Error setting bar map\n");
    }

    
    // Initialize interrupt structure.
    memset(&plxIntr, 0, sizeof(PlxIntr_t)) ;
    memset(&plxState, 0, sizeof(PlxIntr_t)) ;
    
    plxIntr.IopToPciInt = 1 ;    // LINT1 interrupt line 
    plxIntr.IopToPciInt_2 = 1 ;  // LINT2 interrupt line 
    plxIntr.PciMainInt = 1 ;

    /*    if (PlxRegisterWrite(PlxHandle, PCI9030_INT_CTRL_STAT, 0x00300000 )  */
    /*        != ApiSuccess)  */
    /*      printf("  failed to reset interrupt control bits.\n") ;  */

    if(useInterrupts) {
	if (PlxIntrDisable(surfHandles[0], &plxIntr) != ApiSuccess)
	    printf(" Failed to disable interrupt.\n") ;
	plxIntr.IopToPciInt_2 = 0 ;  // LINT2 interrupt line 
    }


//Reprogram TURF if desired
    if (reprogramTurf) {
	printf("Reprogramming Turf\n");
	setTurfControl(turfioHandle,RprgTurf) ;
	for(i=9 ; i++<100 ; usleep(1000)) ;
    }
    //renice process
    sprintf(reniceCommand,"sudo renice %d -p `cat %s`",niceValue,
	    acqdPidFile);
    retVal=system(reniceCommand);

    if(doingEvent==0) prepWriterStructs();


    if(sendSoftTrigger) sleep(2);
    do {
	unsigned int tempTrigMask=antTrigMask;
	// Clear devices 
	clearDevices(surfHandles,turfioHandle);
	antTrigMask=tempTrigMask;

	if(!reInitNeeded) {
	    retVal=readConfigFile();
	    if(retVal!=CONFIG_E_OK) {
		syslog(LOG_ERR,"Error reading config file Acqd.config: bailing");
		return -1;
	    }
	}
	reInitNeeded=0;
	slipCounter=0;

	//Send software trigger
	setTurfControl(turfioHandle,SendSoftTrg);

	// Set trigger modes
	//RF and Software Triggers enabled by default
	trigMode=TrigNone;
	if(pps1TrigFlag) trigMode|=TrigPPS1;
	if(pps2TrigFlag) trigMode|=TrigPPS2;
	// RJN debugging
//	setTurfControl(turfioHandle,SetTrigMode);

	//Write RFCM Mask
	writeAntTrigMask(turfioHandle);
	

	theSurfHk.globalThreshold=0;

	//Pedestal Mode
	if(pedestalMode) {
	    doingEvent=0;
	    enableChanServo=0;
	    setGlobalThreshold=1;
	    globalThreshold=1;
	    doThresholdScan=0;
	    writeData=writeDebugData;
	    numEvents=numPedEvents;
	    sendSoftTrigger=1;
	    softTrigSleepPeriod=0;
	    writeFullHk=0;
	

	    resetPedCalc();
//	    memset(&thePeds,0,sizeof(PedCalcStruct_t));
//	    gettimeofday(&timeStruct,NULL);
//	    thePeds.unixTimeStart=timeStruct.tv_sec;

	}

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
		printf("Sending software triggers\n");
	}


	//Set Thresholds
	if(setGlobalThreshold) {
	    setGloablDACThreshold(surfHandles,globalThreshold);
	    theSurfHk.globalThreshold=globalThreshold;
	    if(printToScreen) 
		printf("Setting Global Threshold %d\n",globalThreshold);
	}
	else {
	    setDACThresholds(surfHandles);
	}

	
	currentState=PROG_STATE_RUN;
	while (currentState==PROG_STATE_RUN) {
	    gotSurfHk=0;
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"1 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    if((standAloneMode || pedestalMode || numEvents) && (numEvents && numEvents<doingEvent)) {
		currentState=PROG_STATE_TERMINATE;
		break;
	    }
	    //Fill theEvent with zeros 
	    bzero(&theEvent, sizeof(theEvent)) ;
	    memset(&theSurfHk,0,sizeof(FullSurfHkStruct_t));
	    if(setGlobalThreshold) 
		theSurfHk.globalThreshold=globalThreshold;

	THRESH_SCAN:
	    if(doThresholdScan || doSingleChannelScan) {		
		sendSoftTrigger=0;
		enableChanServo=0;
//		if(dacVal<=500) dacVal=4095;
//		else if(dacVal>=4000) dacVal=0;
		dacVal=doingDacVal;
		threshScanCounter++;
		theScalers.threshold=dacVal;
		if(printToScreen) 
		    printf("Setting Threshold -- %d\r",dacVal);
		if(!doSingleChannelScan) {
		    setGloablDACThreshold(surfHandles,dacVal);	
		    theSurfHk.globalThreshold=dacVal;
		}
		else {
		    memset(&thresholdArray[0][0],0,sizeof(int)*ACTIVE_SURFS*N_RFTRIG);
		    if(surfForSingle==100) {
			for(surf=0;surf<numSurfs;surf++)
			    thresholdArray[surf][dacForSingle]=doingDacVal;
		    }
		    else  
			thresholdArray[surfForSingle][dacForSingle]=doingDacVal;
		    setDACThresholds(surfHandles);
		}
		if(firstLoop) {
		    usleep(30000);
		    firstLoop=0;
		}

		if(doingDacVal>4095) {
		    currentState=PROG_STATE_TERMINATE;
		    continue;
		    exit(0);//doingDacVal=0;
		}
		if(threshScanCounter>=thresholdScanPointsPerStep) {
		    doingDacVal+=thresholdScanStepSize;
		    threshScanCounter=0;
		}
	    }

	    //Send software trigger if we want to
	    if(sendSoftTrigger && doingEvent>=0) {
		if(!softTrigSleepPeriod || (time(NULL)-lastSofTrigTime)>softTrigSleepPeriod) {		    		
		    setTurfControl(turfioHandle,SendSoftTrg);
		    lastSofTrigTime=time(NULL);
		}
	    }

	    
	    // attachment should be done while LINT1 is not active.  
	    // Otherwise, API disables LINT1 and waiting for it 
	    // will time out.   21-Jan-05, SM. 
	    if (useInterrupts) {
		if (PlxIntrAttach(surfHandles[0], plxIntr, &lint1Handle) 
		    != ApiSuccess)
		    printf(" failed to attatch interrupt. \n") ;
		
		if (PlxIntrEnable(surfHandles[0], &plxIntr) != ApiSuccess)
		    printf("  failed to enable interrupt.\n") ;
		if(verbosity>1 && printToScreen) {
		    tmpGPIO=PlxRegisterRead(surfHandles[0], 
					    PCI9030_GP_IO_CTRL, &rc);
		    printf("SURF %d GPIO (after INTR attachment) : 0x%o %d\n",
			   surfIndex[0],tmpGPIO,tmpGPIO);
		    
		    tmpINTR=PlxRegisterRead(surfHandles[0],
					    PCI9030_INT_CTRL_STAT, &rc); 
		    printf("INTR Reg (after INTR attachment) contents SURF %d = %x\n",
			   surfIndex[i],tmpINTR);
		}
		
		if (setSurfControl(surfHandles[0], IntEnb) != ApiSuccess)
		    printf("  failed to send IntEnb pulse to SURF %d.\n",i) ;
	    }


	    // Wait for ready to read (EVT_RDY) 
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"Waitf for EVT_RDY { %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
	    fprintf(timeFile,"2 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

	    if(useInterrupts) {
		if(printToScreen) 
		    printf("Using Interrupt Logic\n");
		//wait for an interrupt on LINT1 line.  worked, 21-Jan-05
		switch(PlxIntrWait(surfHandles[0],lint1Handle,N_TMO_INT)) {
		    case ApiSuccess:
			if(printToScreen)
			    printf("\tGot Interrput!\n");
			break;
		    case ApiWaitTimeout:
			// any setSurfControl call will reset IntEnb
			if(setSurfControl(surfHandles[0],RDMode) !=
			   ApiSuccess) {
			    printf("Failed to set RDMode (reset IntEnb) on SURF %d \n",surfIndex[0]);
			}
			printf("Timed out waiting for interrupt.\n");
			continue;
			
		    case ApiWaitCanceled : 
			printf(" interrupt wait was canceled ") ;
			printf("(likely indicates attach failure).\n") ;
		    case ApiFailed :
		    default: printf(" API failed for PlxIntrWait.\n") ;
			exit(1) ;
		}
	    
		if(printToScreen && verbosity>1) {		
		    tmpINTR=PlxRegisterRead(surfHandles[0],
					    PCI9030_INT_CTRL_STAT, &rc); 
		    printf("INTR Reg (after INTR wait) contents SURF %d = %x\n",
			   surfIndex[i],tmpINTR);
		    
		}
	    }
	    else {	    
		tmo=0;	    
		if(!dontWaitForEvtF) {
		    if(verbosity && printToScreen) 
			printf("Wait for EVT_RDY on SURF %d\n",surfIndex[0]);
		    do {
			tmpGPIO=PlxRegisterRead(surfHandles[0], 
						PCI9030_GP_IO_CTRL, &rc);
			if(verbosity>3 && printToScreen) 
			    printf("SURF %d GPIO: 0x%o %d\n",
				   surfIndex[0],tmpGPIO,tmpGPIO);
			if(tmo && (tmo%100)==0) //Might change tmo%2
			    myUsleep(1); 
			tmo++;

			

			if((timeStruct.tv_sec-lastSlowRateCalc.tv_sec)>60) {
			    if(doingEvent) {
				float slowRate=doingEvent-lastSlowRateEvent;
				float slowTime=timeStruct.tv_sec-lastSlowRateCalc.tv_sec;
				slowTime+=1e-6*((float)timeStruct.tv_usec);
				slowTime-=1e-6*((float)lastSlowRateCalc.tv_usec);
				slowRate/=slowTime;
				writeCurrentRFSlowRateObject(slowRate,lastEvNum);
				lastSlowRateCalc.tv_sec=timeStruct.tv_sec;
				lastSlowRateCalc.tv_usec=timeStruct.tv_usec;
				lastSlowRateEvent=doingEvent;
			    }
			    else 
				writeCurrentRFSlowRateObject(0,0);
			}




			if((timeStruct.tv_sec-lastRateCalc.tv_sec)>calculateRateAfter) {

			    if(enableRateServo) {
				if(((timeStruct.tv_sec-lastServoRateCalc.tv_sec)>servoRateCalcPeriod) || ((doingEvent-lastRateCalcEvent)>(servoRateCalcPeriod*rateGoal))) {
				    //Time to servo on rate
				    servoOnRate(doingEvent,lastRateCalcEvent,&timeStruct,&lastServoRateCalc);
				    lastRateCalcEvent=doingEvent;
				    lastServoRateCalc.tv_sec=timeStruct.tv_sec;
				    lastServoRateCalc.tv_usec=timeStruct.tv_usec;
				    
				}				    
			    }
			

			    rateCalcPeriod=0;
			    //Make rate calculation;
			    if(lastRateCalc.tv_sec>0) {
				rateCalcPeriod=
				    timeStruct.tv_sec-lastRateCalc.tv_sec;
				rateCalcPeriod+=((float)timeStruct.tv_usec-
						 lastRateCalc.tv_usec)/1e6;
				totalTime+=rateCalcPeriod;

				//Deadtime Monitoring
				totalDeadtime+=intervalDeadtime;
			    }			   

			    if(rateCalcPeriod) {
				if((doingEvent-lastEventCounter)>0 && rateCalcPeriod) {
				    printf("Event %d -- Current Rate %3.2f Hz\n",doingEvent,((float)(doingEvent-lastEventCounter))/rateCalcPeriod);
//		    if(lastEventCounter<200) 
//			printf("\n");
				}
				else {
				    printf("Event %d -- Current Rate 0 Hz\n",doingEvent);
				}
			    

				printf("\tTotal Time %3.1f (s)\t Total Deadtime %3.1f (s) (%3.2f %%)\n",totalTime,totalDeadtime,100.*(totalDeadtime/totalTime));
				printf("\tInterval Time %3.1f (s)\t Interval Deadtime %3.1f (s) (%3.2f %%)\n",rateCalcPeriod,intervalDeadtime,100.*(intervalDeadtime/rateCalcPeriod));
				intervalDeadtime=0;
			    }





			    lastRateCalc=timeStruct;
			    lastEventCounter=doingEvent;
			}
			if(tmo>500) {
			    if(currentState!=PROG_STATE_RUN) 
				break;
//			    //Time out not in self trigger mode
//			    if(tmo==EVTF_TIMEOUT) break;
		
//			    if(enableChanServo) {
			    tmo=0;
			    //Give us a chance to servo thresholds
			    if(firmwareVersion>=3) {
				gettimeofday(&timeStruct,NULL);
				timeDiff=timeStruct.tv_usec-lastSurfHkRead.tv_usec;
				timeDiff+=1000000*(timeStruct.tv_sec-lastSurfHkRead.tv_sec);


				if(timeDiff>50000 || lastSurfHkRead.tv_sec==0) {
//				    printf("Time Diff %d, %d.%d and %d.%d\n",
//					   timeDiff,timeStruct.tv_sec,timeStruct.tv_usec,lastSurfHkRead.tv_sec,lastSurfHkRead.tv_usec);
				    lastSurfHkRead=timeStruct;
				    status=readSurfHkData(surfHandles);
				    for(surf=0;surf<numSurfs;++surf)
					if (setSurfControl(surfHandles[surf], 
							   SurfClearHk) 
					    != ApiSuccess)
					    printf("  failed to send clear event pulse on SURF %d.\n",surfIndex[surf]) ;
				    
				    if(verbosity && printToScreen) 
					printf("Read SURF Housekeeping\n");
				    

				    if(timeStruct.tv_sec>lastSlowSurfHk) {
					addSurfHkToAverage(&theSurfHk);
					lastSlowSurfHk=timeStruct.tv_sec;
				    }


				    if(verbosity && printToScreen && enableChanServo) 
					printf("Will servo on channel scalers\n");
				    if(enableChanServo) {
					if(updateThresholdsUsingPID())
					    setDACThresholds(surfHandles);
				    }
				
				
				    
				    if((timeStruct.tv_sec-lastSurfHk)>=surfHkPeriod) {
					//Record housekeeping data
					lastSurfHk=timeStruct.tv_sec;
					theSurfHk.unixTime=timeStruct.tv_sec;
					theSurfHk.unixTimeUs=timeStruct.tv_usec;
					
					
					surfHkCounter++;
					if(surfHkCounter>=surfHkTelemEvery) {
					    writeSurfHousekeeping(3);
					    surfHkCounter=0;
					}
					else {
					    writeSurfHousekeeping(1);
					}

					
				    }
				    if(doThresholdScan || doSingleChannelScan)
					goto THRESH_SCAN;
				}

		    
			    }
			
						    

			}		    		    
		    } while(!(tmpGPIO & EVT_RDY) && (selftrig?(tmo<N_TMO):1));

		    if(currentState!=PROG_STATE_RUN) 
			continue;
		    
		    if (tmo == N_TMO || tmo==EVTF_TIMEOUT){
			syslog(LOG_WARNING,"Timed out waiting for EVT_RDY flag");
			if(printToScreen)
			    printf(" Timed out (%d ms) while waiting for EVT_RDY flag in self trigger mode.\n", N_TMO) ;
			continue;
			
		    }
		}
		else {
		    printf("Didn't wait for EVT_RDY\n");
		    sleep(1);
		}	
	    }
//	    printf("tmo was %d\n",tmo);

#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"Got for EVT_RDY { %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
	    fprintf(timeFile,"3 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif    


//Now load event into ram
	    if (setTurfControl(turfioHandle,TurfLoadRam) != ApiSuccess)
		printf("  failed to send clear event pulse on TURFIO.\n") ;
	    
//	    if(sendSoftTrigger) {
//		//RJN 13/06/06
//		// Insert sleep to cure weirdness of first few samples
//		usleep(200);
//	    }



	    //Either have a trigger or are going ahead regardless
	    gettimeofday(&timeStruct,NULL);
	    
	    


#ifdef TIME_DEBUG
//	    fprintf(stderr,"readSurfEventData -- start %ld s, %ld ms\n",timeStruct.tv_sec,timeStruct.tv_usec);
	    fprintf(timeFile,"4 %ld %ld\n",timeStruct.tv_sec,timeStruct.tv_usec);  
#endif
	    if(firmwareVersion==1) 
		status+=readSurfEventData(surfHandles);
	    else if(firmwareVersion==2)
		status+=readSurfEventDataVer2(surfHandles);
	    else if(firmwareVersion>=3)
		status+=readSurfEventDataVer3(surfHandles);

#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readSurfEventData -- end %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"5 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
//	    fprintf(stderr,"%ld %ld %ld %ld\n",timeStruct.tv_sec,timeStruct.tv_usec,timeStruct2.tv_sec,timeStruct2.tv_usec);
#endif
	    
	    hdPtr->unixTime=timeStruct.tv_sec;
	    hdPtr->unixTimeUs=timeStruct.tv_usec;
	    turfRates.unixTime=timeStruct.tv_sec;
	    turfRates.unixTimeUs=timeStruct.tv_usec;
	    if(verbosity && printToScreen) printf("Read SURF Labrador Data\n");

	    //For now I'll just read the HK data with the events.
	    //Later we will change this to do something more cleverer
	    if(pedestalMode) addEventToPedestals(bdPtr);
	    if(!pedestalMode) {
		//Switched for timing test
		gettimeofday(&timeStruct,NULL);
		timeDiff=timeStruct.tv_usec-lastSurfHkRead.tv_usec;
		timeDiff+=1000000*(timeStruct.tv_sec-lastSurfHkRead.tv_sec);
		
		if(timeDiff>50000 || lastSurfHkRead.tv_sec==0) {
//		    printf("Time Diff %d, %d.%d and %d.%d\n",
//			   timeDiff,timeStruct.tv_sec,timeStruct.tv_usec,lastSurfHkRead.tv_sec,lastSurfHkRead.tv_usec);
		    lastSurfHkRead=timeStruct;
		    status=readSurfHkData(surfHandles);
		
#ifdef TIME_DEBUG
		    gettimeofday(&timeStruct2,NULL);
		    fprintf(timeFile,"6 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
//	    fprintf(stderr,"readSurfHkData -- start %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
#endif
		
		    //Will change to SurfClearHk
		    for(surf=0;surf<numSurfs;++surf)
			if (setSurfControl(surfHandles[surf], 
					   SurfClearHk) 
			    != ApiSuccess)
			    printf("  failed to send clear event pulse on SURF %d.\n",surfIndex[surf]) ;
		    
		    if(verbosity && printToScreen) printf("Read SURF Housekeeping\n");
		    
		    
#ifdef TIME_DEBUG
		    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readSurfHkData -- end %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
		    fprintf(timeFile,"7 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
		    if(enableChanServo) {
			if(verbosity && printToScreen) 
			    printf("Will servo on channel scalers\n");
			
			if(updateThresholdsUsingPID())
			    setDACThresholds(surfHandles);
		    }    
		    
#ifdef TIME_DEBUG
		    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"Done channel servo -- start %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
		    fprintf(timeFile,"8 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
		    
		    if((timeStruct.tv_sec-lastSurfHk)>=surfHkPeriod) {
			//Record housekeeping data
// disabled by GSV
			gotSurfHk=1;  
			lastSurfHk=timeStruct.tv_sec;
			theSurfHk.unixTime=timeStruct.tv_sec;
			theSurfHk.unixTimeUs=timeStruct.tv_usec;
			
		    }
		}
	    }
	    

#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readTurfEventData -- start %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"9 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    if(turfFirmwareVersion<=2) 
		status+=readTurfEventDataVer2(turfioHandle);
	    else
		status+=readTurfEventDataVer3(turfioHandle);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readTurfEventData -- end %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"10 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

	    if(slipCounter!=hdPtr->turfio.trigNum) {
/* 		printf("Read TURF data -- doingEvent %d -- trigNum %d\n", */
/* 		       doingEvent,hdPtr->turfio.trigNum); */
		reInitNeeded=1;
	    }
	    if(reInitNeeded && (slipCounter>eventsBeforeClear)) {
		syslog(LOG_INFO,"Acqd reinit required -- slipCounter %d -- trigNum %d",slipCounter,hdPtr->turfio.trigNum);
		currentState=PROG_STATE_INIT;
	    }
	    hdPtr->otherFlag2=0;
	    if(reInitNeeded) {
		hdPtr->otherFlag2=0x1;
	    }
	    
	    if(verbosity && printToScreen) printf("Done reading\n");

	    //Add TURF Rate Info to slow file
	    if(newTurfRateData) {
		addTurfRateToAverage(&turfRates);
	    }


	    //Error checking
	    if(status!=ACQD_E_OK) {
		//We have an error
		if(printToScreen) fprintf(stderr,"Error detected %d, during event %d",status,doingEvent);
		
		status=ACQD_E_OK;
	    }
	    else if(doingEvent>=0) { //RJN changed 24/11/06 for test
		hdPtr->eventNumber=getEventNumber();
		bdPtr->eventNumber=hdPtr->eventNumber;
		lastEvNum=hdPtr->eventNumber;
		hdPtr->surfMask=surfMask;
		hdPtr->antTrigMask=(unsigned int)antTrigMask;
		

		/* 
		   Filled by Eventd
		   hdPtr->gpsSubTime;
		   hdPtr->calibStatus;
		*/

		if(printToScreen && verbosity) 
		    printf("Event:\t%lu\nSec:\t%ld\nMicrosec:\t%ld\nTrigTime:\t%lu\n",hdPtr->eventNumber,hdPtr->unixTime,hdPtr->unixTimeUs,turfioPtr->trigTime);


		// Save data
		if(writeData || writeFullHk){
		    if(useAltDir) {
//			if(!madeAltDir || 
//			   hdPtr->eventNumber%MAX_EVENTS_PER_DIR==0) 
//			    makeSubAltDir();
			writeEventAndMakeLink(altOutputdir,ACQD_EVENT_LINK_DIR,&theEvent);	
		    }
		    else 
			writeEventAndMakeLink(ACQD_EVENT_DIR,ACQD_EVENT_LINK_DIR,&theEvent);


#ifdef TIME_DEBUG
		    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readTurfEventData -- end %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
		    fprintf(timeFile,"11 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
		    
		    if(writeFullHk) {
			
			if(gotSurfHk) {
			    //Do the housekeeping stuff
			    surfHkCounter++;
			    if(surfHkCounter>=surfHkTelemEvery) {
				writeSurfHousekeeping(3);
				surfHkCounter=0;
			    }
			    else {
//			    printf("\tSURF HK %ld %ld\n",theSurfHk.unixTime,lastSurfHk);
				writeSurfHousekeeping(1);
			    }
			}
			

#ifdef TIME_DEBUG
			gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readTurfEventData -- end %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
			fprintf(timeFile,"12 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
			if(newTurfRateData) {
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
		    
#ifdef TIME_DEBUG
		    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readTurfEventData -- end %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
		    fprintf(timeFile,"13 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
		    


		}

	    }
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"Wrote data %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"14 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif	    

	    // Clear boards
	    int tempInd;
	    for(tempInd=0;tempInd<numSurfs;tempInd++) {
		surf=surfClearIndex[tempInd];
		if (setSurfControl(surfHandles[surf], SurfClearEvent) != ApiSuccess)
		    printf("  failed to send clear event pulse on SURF %d.\n",surfIndex[surf]) ;
	    }
//	    if(!surfonly)
//		if (setTurfControl(turfioHandle,TurfLoadRam) != ApiSuccess)
//		    printf("  failed to send clear event pulse on TURFIO.\n") ;
	    
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"Sent Clear -- start %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"15 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif	    
//	if(selftrig) sleep (2) ;
	}  /* closing master while loop. */
	if(pedestalMode) {
	    writePedestals();
	    if(pedSwitchConfigAtEnd) {
		//Arrrghhh		
		CommandStruct_t theCmd;
		theCmd.numCmdBytes=3;
		theCmd.cmd[0]=LAST_CONFIG;
		theCmd.cmd[1]=1;
		theCmd.cmd[2]=0;
		writeCommandAndLink(&theCmd);
	    }
	}
	if(doThresholdScan && threshSwitchConfigAtEnd) {
	    //Arrrghhh		
	    CommandStruct_t theCmd;
	    theCmd.numCmdBytes=3;
	    theCmd.cmd[0]=LAST_CONFIG;
	    theCmd.cmd[1]=1;
	    theCmd.cmd[2]=0;
	    writeCommandAndLink(&theCmd);
	}
	    


    } while(currentState==PROG_STATE_INIT);


    unsetBarMap(surfHandles,turfioHandle);
    // Clean up
    for(surf=0;surf<numSurfs;surf++)
	PlxPciDeviceClose(surfHandles[surf] );

    closeHkFilesAndTidy(&surfHkWriter);
    closeHkFilesAndTidy(&turfHkWriter);
    unlink(acqdPidFile);
    syslog(LOG_INFO,"Acqd terminating");
    return 1 ;
}


//Control Fucntions
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
	case RDMode :
	    string = "RDMode" ;
	    break ;
	case WTMode :
	    string = "WTMode" ;
	    break ;
	case DTRead :
	    string = "DTRead" ;
	    break ;
	case DACLoad :
	    string = "DACLoad" ;
	    break ;
	case CHGChp :
	    string = "CHGChp" ;
	    break ;
	case IntEnb :
	    string = "IntEnb" ;
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

PlxReturnCode_t setSurfControl(PlxHandle_t surfHandle, SurfControlAction_t action)
{
    //These numbers are octal
    
    U32 baseVal    = 0222222220 ;
    U32 clearEvent = 0000000040 ;
    U32 clearAll   = 0000000400 ;  
    U32 clearHk    = 0000400000 ;  // Clears hk
    U32 rdWrFlag   = 0000004000 ;  // rd/_wr flag 
    U32 dataHkFlag = 0000040000 ;  // dt/_hk flag 
    U32 dacLoad    = 0004000000 ;
    U32 intEnable  = 0040000000 ;
//    U32 changeChip = 0040000000 ;
    
 
    U32           gpioVal ;
    PlxReturnCode_t   rc ;

    // Read current GPIO state
//    U32 gpioVal=PlxRegisterRead(surfHandle, PCI9030_GP_IO_CTRL, &rc) ; 

    //Make RD mode the default
    baseVal|=rdWrFlag;
    switch (action) {
	case SurfClearAll : gpioVal = baseVal | clearAll      ; break ; 
	case SurfClearEvent : gpioVal = baseVal | clearEvent  ; break ; 
	case SurfClearHk  : gpioVal = baseVal | clearHk  ; break ;
	case RDMode  : gpioVal = baseVal | rdWrFlag ; break ; //Default 
	case WTMode   : gpioVal = baseVal & ~rdWrFlag ; break ;
	case DTRead   : gpioVal = baseVal | dataHkFlag ; break ;
	case DACLoad  : gpioVal = (baseVal&~rdWrFlag) | dacLoad ; break ;
	case IntEnb : gpioVal = baseVal | intEnable; break;
	default :
	    syslog(LOG_WARNING,"setSurfControl: undefined action %d",action);
	    if(printToScreen)
		fprintf(stderr,"setSurfControl: undefined action, %d\n", action) ;
	    return ApiFailed ;
    }
    if (verbosity>1 && printToScreen) 
	printf(" setSurfControl: action %s, gpio= %o\n", 
	       surfControlActionAsString(action), gpioVal) ; 

    if ((rc = PlxRegisterWrite(surfHandle, PCI9030_GP_IO_CTRL, gpioVal ))
	!= ApiSuccess) {
	syslog(LOG_ERR,"setSurfControl: failed to set GPIO to %o.\n", gpioVal);
	if(printToScreen)
	    fprintf(stderr,"setSurfControl: failed to set GPIO to %o.\n", gpioVal) ; 
	return rc ;
    }
    if (SurfClearHk < action && action < DACLoad) return rc ; 
    //Reset GPIO to base val
    return PlxRegisterWrite(surfHandle, PCI9030_GP_IO_CTRL, baseVal ) ;

}


PlxReturnCode_t setTurfControl(PlxHandle_t turfioHandle, TurfControlAction_t action) {

    //These numbers are octal
    U32 baseClr          = 0022222222 ;
    U32 rprgTurf         = 0000000004 ;
    U32 loadRam          = 0000000040 ; 
    U32 sendSoftTrig     = 0000000400 ;
    U32 rfcDbit          = 0004000000 ;
    U32 rfcClk           = 0000004000 ;
    U32 enablePPS1Trig   = 0000040000 ; //Real PPS from ADU5
    U32 enablePPS2Trig   = 0000400000 ; //Burst PPS up to 20Hz from G123
    U32 clearAll         = 0040000000 ;

    U32           gpioVal,baseVal ;
    PlxReturnCode_t   rc ;
    int i ;

    static int first=1;
//    printf("Dec: %d %d %d\nHex: %x %x %x\nOct: %o %o %o\n",base_clr,rprg_turf,clr_trg,base_clr,rprg_turf,clr_trg,base_clr,rprg_turf,clr_trg);
//    exit(0);
    if(first) {
	gpioVal=baseClr;    
	if ((rc = PlxRegisterWrite(turfioHandle, PCI9030_GP_IO_CTRL, gpioVal ))
	    != ApiSuccess) {
	    syslog(LOG_ERR,"setTurfControl: failed to set GPIO to %o\t(%d).\n", gpioVal,rc);
	    if(printToScreen)
		printf(" setTurfControl : failed to set GPIO to %o\t (%d).\n", gpioVal,rc) ;
	    return rc ;
	}
	if(printToScreen) printf("First read of TURF GPIO: %o\n", PlxRegisterRead(turfioHandle, PCI9030_GP_IO_CTRL, &rc));
	first=0;
    }
    // Read current GPIO state
    baseVal=PlxRegisterRead(turfioHandle, PCI9030_GP_IO_CTRL, &rc) ; 
    
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
	    if(printToScreen)
		printf(" setTurfControl: undfined action, %d\n", action) ;
	    return ApiFailed ;
    }

    if (verbosity>1 && printToScreen) 
	printf("setTurfControl: action %s, gpioVal = %o\n", 
	       turfControlActionAsString(action), gpioVal) ; 

    if ((rc = PlxRegisterWrite(turfioHandle, PCI9030_GP_IO_CTRL, gpioVal ))
	!= ApiSuccess) {
	syslog(LOG_ERR,"setTurfControl: failed to set GPIO to %o\t(%d).\n", gpioVal,rc);
	if(printToScreen)
	    printf(" setTurfControl : failed to set GPIO to %o\t (%d).\n", gpioVal,rc) ;
	return rc ;
    }
    if (action > TurfClearAll) return rc ;
    if (action == RprgTurf)  /* wait a while in case of RprgTurf. */
	for(i=0 ; i++<10 ; usleep(1)) ;

    return PlxRegisterWrite(turfioHandle, PCI9030_GP_IO_CTRL, baseVal ) ;

}

void copyPlxLocation(PlxDevLocation_t *dest, PlxDevLocation_t source)
{
    int count;
    dest->BusNumber=source.BusNumber;
    dest->SlotNumber=source.SlotNumber;
    dest->DeviceId=source.DeviceId;
    dest->VendorId=source.VendorId;
    for(count=0;count<12;count++)
	dest->SerialNumber[count]=source.SerialNumber[count];

}

int initializeDevices(PlxHandle_t *surfHandles, PlxHandle_t *turfioHandle, PlxDevLocation_t *surfLoc, PlxDevLocation_t *turfioLoc)
/*! 
  Initializes the SURFs and TURFIO, returns the number of devices initialized.
*/
{
    
    int surfNum,countSurfs=0;
    U32  numDevices,i ;
    PlxDevLocation_t tempLoc;
    PlxReturnCode_t rc;
    //  U32  n=0 ;  /* this is the numebr of board from furthest from CPU. */

    tempLoc.BusNumber       = (U8)-1;
    tempLoc.SlotNumber      = (U8)-1;
    tempLoc.VendorId        = (U16)-1;
    tempLoc.DeviceId        = (U16)-1;
    tempLoc.SerialNumber[0] = '\0';
   
    /* need to go through PlxPciDeviceFind twice.  1st for counting devices
       with "numDevices=FIND_AMOUNT_MATCHED" and 2nd to get device info. */

    numDevices = FIND_AMOUNT_MATCHED;
    if (PlxPciDeviceFind(&tempLoc, &numDevices) != ApiSuccess ||
	numDevices > 13) return -1 ;
    if (verbosity && printToScreen) printf("initializeDevices: found %d PLX devices.\n", numDevices) ;

    /* Initialize SURFs */    
    for(surfNum=0;surfNum<MAX_SURFS;surfNum++) {
	if(surfPos[surfNum].bus<0 || surfPos[surfNum].slot<0) continue;
	i=0;
	tempLoc.BusNumber       = surfPos[surfNum].bus;
	tempLoc.SlotNumber      = surfPos[surfNum].slot;
	tempLoc.VendorId        = (U16)-1;
	tempLoc.DeviceId        = (U16)-1;
	tempLoc.SerialNumber[0] = '\0';
	if (PlxPciDeviceFind(&tempLoc, &i) != ApiSuccess) {
	    //syslog
	    printf("Couldn't find SURF %d (Bus %d -- Slot %X)\n",
		   surfNum+1,surfPos[surfNum].bus,surfPos[surfNum].slot);
	}
	else {	    
	    // Got a SURF
/* 		printf("tempLoc[%d] %d %d \t surfPos[%d] %d %d\n", */
/* 		       i,tempLoc[i].BusNumber,tempLoc[i].SlotNumber,surfNum, */
/* 		       surfPos[surfNum].bus,surfPos[surfNum].slot); */		       	    	    
	    copyPlxLocation(&surfLoc[countSurfs],tempLoc);
	    if (verbosity && printToScreen)
		printf("SURF %d found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
		       surfNum+1,
		       surfLoc[countSurfs].DeviceId, 
		       surfLoc[countSurfs].VendorId, 
		       surfLoc[countSurfs].SerialNumber,
		       surfLoc[countSurfs].BusNumber, 
		       surfLoc[countSurfs].SlotNumber);
	    rc=PlxPciDeviceOpen(&surfLoc[countSurfs],&surfHandles[countSurfs]);
	    if ( rc!= ApiSuccess) {
		syslog(LOG_ERR,"Error opening SURF device %d",rc);
		if(printToScreen)
		    fprintf(stderr,"Error opening SURF device %d\n",rc);
		return -1 ;
	    }		
	    PlxPciBoardReset(surfHandles[countSurfs]) ;
	    surfIndex[countSurfs]=surfNum+1;
	    if(hdPtr) {
		surfMask|=(1<<surfNum);
	    }
	    countSurfs++;    
	}
    }
	
    //Initialize TURFIO    
    tempLoc.BusNumber       = turfioPos.bus;
    tempLoc.SlotNumber      = turfioPos.slot;
    tempLoc.VendorId        = (U16)-1;
    tempLoc.DeviceId        = (U16)-1;
    tempLoc.SerialNumber[0] = '\0';
    if (PlxPciDeviceFind(&tempLoc, &i) != ApiSuccess) {
	//syslog
	printf("Couldn't find TURFIO (Bus %d -- Slot %X)",
	       turfioPos.bus,turfioPos.slot);
    }
    else {
	copyPlxLocation(turfioLoc,tempLoc);
	
	if (verbosity && printToScreen)
	    printf("TURFIO found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
		   (*turfioLoc).DeviceId, (*turfioLoc).VendorId, 
		   (*turfioLoc).SerialNumber,
		   (*turfioLoc).BusNumber, (*turfioLoc).SlotNumber);
	if (PlxPciDeviceOpen(turfioLoc, turfioHandle) != ApiSuccess) {
	    syslog(LOG_ERR,"Error opening TURFIO device");
	    if(printToScreen)
		fprintf(stderr,"Error opening TURFIO device\n");
	    return -1 ;
	}
	PlxPciBoardReset(turfioHandle);
    }

    if(verbosity && printToScreen) {
	printf("initializeDevices: Initialized %d SURF board(s)\n",countSurfs);
    }

/*     for(i =0; i<countSurfs+1;i++) */
/* 	if (verbosity) */
/* 	    printf("Something found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n", */
/* 		   tempLoc[i].DeviceId,  */
/* 		   tempLoc[i].VendorId,  */
/* 		   tempLoc[i].SerialNumber, */
/* 		   tempLoc[i].BusNumber,  */
/* 		   tempLoc[i].SlotNumber); */
 


    if(countSurfs!=numSurfs) {
	syslog(LOG_WARNING,"Expected %d SURFs, but only found %d",numSurfs,countSurfs);
	if(printToScreen)
	    fprintf(stderr,"Expected %d SURFs, but only found %d\n",numSurfs,countSurfs);
	numSurfs=countSurfs;
	if(numSurfs==0) exit(0);
    }
//    exit(0);
    return (int)numDevices ;
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
    char *tempString;

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
	useInterrupts=kvpGetInt("useInterrupts",0);
	firmwareVersion=kvpGetInt("firmwareVersion",2);
	turfFirmwareVersion=kvpGetInt("turfFirmwareVersion",2);
	tempString=kvpGetString("acqdPidFile");
	if(tempString) {
	    strncpy(acqdPidFile,tempString,FILENAME_MAX);
	    writePidFile(acqdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get acqdPidFile");
	    fprintf(stderr,"Couldn't get acqdPidFile\n");
	}

//	printf("%s\n",acqdEventDir);
	tempString=kvpGetString ("lastEventNumberFile");
	if(tempString)
	    strncpy(lastEventNumberFile,tempString,FILENAME_MAX-1);
	else
	    fprintf(stderr,"Coudn't fetch lastEventNumberFile\n");

//	printf("Debug rc2\n");
	printToScreen=kvpGetInt("printToScreen",0);
	if(addedVerbosity && printToScreen==0) {
	    printToScreen=1;
	    addedVerbosity--;
	}

	surfHkPeriod=kvpGetInt("surfHkPeriod",1);
	surfHkTelemEvery=kvpGetInt("surfHkTelemEvery",1);
	turfRateTelemEvery=kvpGetInt("turfRateTelemEvery",1);
	turfRateTelemInterval=kvpGetInt("turfRateTelemInterval",1);
//	printf("HK surfPeriod %d\nturfRateTelemEvery %d\nturfRateTelemInterval %d\n",surfHkPeriod,turfRateTelemEvery,turfRateTelemInterval);
	dontWaitForEvtF=kvpGetInt("dontWaitForEvtF",0);
	dontWaitForLabF=kvpGetInt("dontWaitForLabF",0);
	sendSoftTrigger=kvpGetInt("sendSoftTrigger",0);
	softTrigSleepPeriod=kvpGetInt("softTrigSleepPeriod",0);
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
	pidGoal=kvpGetInt("pidGoal",2000);
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
	
	if(printToScreen) printf("Print to screen flag is %d\n",printToScreen);

	turfioPos.bus=kvpGetInt("turfioBus",3); //in seconds
	turfioPos.slot=kvpGetInt("turfioSlot",10); //in seconds
	tempNum=18;
	kvpStatus=kvpGetIntArray("surfTrigBandMasks",&surfTrigBandMasks[0][0],&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(surfTrigBandMasks): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
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
	    exit(0);
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
	    for(dac=0;dac<N_RFTRIG;dac++) {
		thresholdArray[surf][dac]=0;
		pidGoalScaleFactors[surf][dac]=0;
	    }
	    if(dacSurfs[surf]) {		
		sprintf(keyName,"threshSurf%d",surf+1);
		tempNum=32;	       
		kvpStatus = kvpGetIntArray(keyName,&(thresholdArray[surf][0]),&tempNum);
		if(kvpStatus!=KVP_E_OK) {
		    syslog(LOG_WARNING,"kvpGetIntArray(%s): %s",keyName,
			   kvpErrorString(kvpStatus));
		    if(printToScreen)
			fprintf(stderr,"kvpGetIntArray(%s): %s\n",keyName,
				kvpErrorString(kvpStatus));
		}

		sprintf(keyName,"pidGoalScaleSurf%d",surf+1);
		tempNum=32;	       
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
	rateIMin-kvpGetFloat("rateIMin",-100);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Acqd.config: %s\n",eString);
    }



    if(printToScreen) {
	printf("Read Config File\n");
	printf("doThresholdScan %d\n",doThresholdScan);
	printf("pedestalMode %d\n",pedestalMode);
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



void clearDevices(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle) 
// Clears boards for start of data taking 
{
    int testVal=0;
    int i;
    PlxReturnCode_t  rc ;

    if(verbosity &&printToScreen) printf("*** Clearing devices ***\n");
    

    //Added RJN 24/11/06
    trigMode=TrigNone;
    if (setTurfControl(turfioHandle, SetTrigMode) != ApiSuccess) {
	syslog(LOG_ERR,"Failed to set trigger mode to none on TURF\n") ;
	if(printToScreen)
	    printf("Failed to set trigger mode to none on SURF\n");
    }

    antTrigMask=0xffffffff;    
    //Mask off all antennas
    writeAntTrigMask(turfioHandle);


    //Added RJN 29/11/06
    setGloablDACThreshold(surfHandles,1);

    // Prepare TURF board
  
    /*  Get PLX Chip Type */
    /*   PlxChipTypeGet(surfHandles[i], &ChipTypeSelected, &ChipRevision ); */
    /*    if (ChipTypeSelected == 0x0) { */
    /*      printf("\n   ERROR: Unable to determine PLX chip type TURFIO\n"); */
    /*      PlxPciDeviceClose(surfHandles[i] ); */
    /*      exit(-1); */
    /*    } */
    /*    printf(" TURFIO Chip type:  %04x,  Revision :    %02X\n", */
    /*          ChipTypeSelected, ChipRevision );  */

    testVal=PlxRegisterRead(turfioHandle,PCI9030_GP_IO_CTRL,&rc) ; 
    if(printToScreen) printf(" GPIO register contents TURFIO = %o\n",testVal);

    
    if (setTurfControl(turfioHandle, TurfClearAll) != ApiSuccess) {
	syslog(LOG_ERR,"Failed to send clear pulse on TURF \n") ;
	if(printToScreen)
	    printf("  failed to send clear pulse on TURF \n") ;
    }


    testVal=PlxRegisterRead(turfioHandle, PCI9030_GP_IO_CTRL, &rc); 
    if(printToScreen) {	
        printf(" Try to read TURFIO GPIO.  %o\n", testVal);
    }
    if (rc != ApiSuccess) printf("  failed to read TURFIO GPIO .\n") ; 

    testVal=PlxRegisterRead(turfioHandle, PCI9030_INT_CTRL_STAT, &rc); 
    if(printToScreen){
	printf(" Try to test interrupt control register.\n") ;
	printf(" int reg contents TURFIO = %x\n",testVal);
	       
    }
    


// Prepare SURF boards
    int tempInd;
    for(tempInd=0;tempInd<numSurfs;tempInd++) {
	i=surfClearIndex[tempInd];
	/* init. 9030 I/O descripter (to enable READY# input |= 0x02.) */
	if(printToScreen) printf("Trying to set Sp0 on SURF %d\n",surfIndex[i]);
	if(PlxRegisterWrite(surfHandles[i], PCI9030_DESC_SPACE0, 0x00800000) 
	   != ApiSuccess)  {
	    syslog(LOG_ERR,"Failed to set SpO descriptor on SURF %d",surfIndex[i]);
	    if(printToScreen)
		printf("Failed to set Sp0 descriptor on SURF %d.\n",surfIndex[i] ) ;    
	}
	
	testVal=PlxRegisterRead(surfHandles[i], PCI9030_GP_IO_CTRL, &rc) ; 
	if(printToScreen)
	    printf(" GPIO register contents SURF %d = %o\n",surfIndex[i],testVal);
	
	
	if (setSurfControl(surfHandles[i], SurfClearAll) != ApiSuccess) {
	    syslog(LOG_ERR,"Failed to send clear all event pulse on SURF %d.\n",surfIndex[i]) ;
	    if(printToScreen)
		printf("Failed to send clear all event pulse on SURF %d.\n",surfIndex[i]);
	}
    }

    if(printToScreen){
	printf(" Try to test interrupt control register.\n") ;
	for(i=0;i<numSurfs;++i) {
	    testVal=
		PlxRegisterRead(surfHandles[i],PCI9030_INT_CTRL_STAT, &rc); 
	    printf(" Int reg contents SURF %d = %x\n",surfIndex[i],testVal);
	}
		   
    }
    

    antTrigMask=0xffffffff;    
    //Mask off all antennas
    writeAntTrigMask(turfioHandle);

    return;
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


void setGloablDACThreshold(PlxHandle_t *surfHandles,  unsigned short threshold) {

    PlxReturnCode_t rc;
    int dac,surf ;
    int buf ;
    if(verbosity && printToScreen) printf("Writing thresholds to SURFs\n");

    for(surf=0;surf<numSurfs;surf++) {
	for (dac=0 ; dac<N_RFTRIG ; dac++) {
	    buf = (dac<<16) + threshold ;
	    rc=PlxBusIopWrite(surfHandles[surf], IopSpace0, 0x0, TRUE, 
			      &buf, 4, BitSize32);
	    if(rc!=ApiSuccess) {
		syslog(LOG_ERR,"Error writing DAC Val, SURF %d - DAC %d (%d)",
		       surfIndex[surf],dac,rc);
		if(printToScreen)
		    fprintf(stderr,
			    "Error writing DAC Val, SURF %d - DAC %d (%d)\n",
			    surfIndex[surf],dac,rc);
	    }
	}


	//Insert SURF trigger channel masking here
	for (dac=N_RFTRIG ; dac<N_RFTRIG+2 ; dac++) {
	    buf = (dac<<16) + 
		surfTrigBandMasks[surfIndex[surf]-1][dac-N_RFTRIG];	    
	    rc=PlxBusIopWrite(surfHandles[surf], IopSpace0, 0x0, TRUE, 
			      &buf, 4, BitSize32);
	    if(rc!=ApiSuccess) {
		syslog(LOG_ERR,"Error writing Surf Ant Trig Mask, SURF %d - DAC %d (%d)",
		       surfIndex[surf],dac,rc);
		if(printToScreen)
		    fprintf(stderr,
			    "Error writing Surf Ant Trig Mask, SURF %d - DAC %d (%d)\n",
			    surfIndex[surf],dac,rc);
	    }

	}
	

	rc=setSurfControl(surfHandles[surf],DACLoad);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Error loading DAC Vals, SURF %d (%d)",surfIndex[surf],rc);
	    if(printToScreen)
		fprintf(stderr,"Error loading DAC Vals, SURF %d (%d)\n",
			surfIndex[surf],rc);
	}
	else if(printToScreen && verbosity)
	    printf("DAC Vals set on SURF %d\n",surfIndex[surf]);
    }
	
}



void setDACThresholds(PlxHandle_t *surfHandles) {
    PlxReturnCode_t rc;
    int dac,surf ;
    int buf ;
    if(verbosity && printToScreen) printf("Writing thresholds to SURFs\n");

    for(surf=0;surf<numSurfs;surf++) {
	for (dac=0 ; dac<N_RFTRIG ; dac++) {
//	    if(surf==0 && dac==16) {
//		printf("Trying to set threshold of %d\n",
//		       thresholdArray[surfIndex[surf]-1][dac]);
//	    }
	    buf = (dac<<16) + (thresholdArray[surfIndex[surf]-1][dac]&0xfff) ;
	    rc=PlxBusIopWrite(surfHandles[surf], IopSpace0, 0x0, TRUE, 
			      &buf, 4, BitSize32);
	    if(rc!=ApiSuccess) {
		syslog(LOG_ERR,"Error writing DAC Val, SURF %d - DAC %d (%d)",
		       surfIndex[surf],dac,rc);
		if(printToScreen)
		    fprintf(stderr,
			    "Error writing DAC Val, SURF %d - DAC %d (%d)\n",
			    surfIndex[surf],dac,rc);
	    }
	}

	//Insert SURF trigger channel masking here
	for (dac=N_RFTRIG ; dac<N_RFTRIG+2 ; dac++) {
	    buf = (dac<<16) + 
		surfTrigBandMasks[surfIndex[surf]-1][dac-N_RFTRIG];	    
	    rc=PlxBusIopWrite(surfHandles[surf], IopSpace0, 0x0, TRUE, 
			      &buf, 4, BitSize32);
	    if(rc!=ApiSuccess) {
		syslog(LOG_ERR,"Error writing Surf Ant Trig Mask, SURF %d - DAC %d (%d)",
		       surfIndex[surf],dac,rc);
		if(printToScreen)
		    fprintf(stderr,
			    "Error writing Surf Ant Trig Mask, SURF %d - DAC %d (%d)\n",
			    surfIndex[surf],dac,rc);
	    }

	}

	rc=setSurfControl(surfHandles[surf],DACLoad);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Error loading DAC Vals, SURF %d (%d)",surfIndex[surf],rc);
	    if(printToScreen)
		fprintf(stderr,"Error loading DAC Vals, SURF %d (%d)\n",
			surfIndex[surf],rc);
	}
	else if(printToScreen && verbosity>1)
	    printf("DAC Vals set on SURF %d\n",surfIndex[surf]);
    }
		    
	    
}



PlxReturnCode_t readPlxDataWord(PlxHandle_t handle, unsigned int *dataInt)
{
    return PlxBusIopRead(handle,IopSpace0,0x0,TRUE, dataInt, 4, BitSize32);
}


PlxReturnCode_t readPlxDataShort(PlxHandle_t handle, unsigned short *dataWord)
{
    return PlxBusIopRead(handle,IopSpace0,0x0,TRUE, dataWord, 2, BitSize16);
}



int getEventNumber() {
    int retVal=0;
    static int firstTime=1;
    static int eventNumber=0;
    /* This is just to get the lastEventNumber in case of program restart. */
    FILE *pFile;
    if(firstTime && !standAloneMode) {
	pFile = fopen (lastEventNumberFile, "r");
	if(pFile == NULL) {
	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		    lastEventNumberFile);
	}
	else {	    	    
	    retVal=fscanf(pFile,"%d",&eventNumber);
	    if(retVal<0) {
		syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
			lastEventNumberFile);
	    }
	    fclose (pFile);
	}
	if(printToScreen) printf("The last event number is %d\n",eventNumber);
	firstTime=0;
    }
    eventNumber++;
    if(!standAloneMode) {
	pFile = fopen (lastEventNumberFile, "w");
	if(pFile == NULL) {
	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		    lastEventNumberFile);
	}
	else {
	    retVal=fprintf(pFile,"%d\n",eventNumber);
	    if(retVal<0) {
		syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
			lastEventNumberFile);
	    }
	    fclose (pFile);
	}
    }
    return eventNumber;

}

int writeSurfHousekeeping(int dataOrTelem) 
// dataOrTelem 
// 1 == data
// 2 == telem
// anything else == both
{
    char theFilename[FILENAME_MAX];
    int retVal=0;
    fillGenericHeader(&theSurfHk,PACKET_SURF_HK,sizeof(FullSurfHkStruct_t));
    //Write data to disk
    if(dataOrTelem!=2) {
//	sprintf(theFilename,"%s/surfhk_%ld_%ld.dat.gz",surfHkArchiveDir,
//		theSurfHk.unixTime,theSurfHk.unixTimeUs);
//	retVal+=writeSurfHk(&theSurfHk,theFilename);
	retVal=cleverHkWrite((unsigned char*)&theSurfHk,sizeof(FullSurfHkStruct_t),
			     theSurfHk.unixTime,&surfHkWriter);
    }
    if(dataOrTelem!=1) {
	// Write data for Telem
	sprintf(theFilename,"%s/surfhk_%ld_%ld.dat.gz",SURFHK_TELEM_DIR,
		theSurfHk.unixTime,theSurfHk.unixTimeUs);
	retVal+=writeSurfHk(&theSurfHk,theFilename);
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


    fillGenericHeader(&turfRates,PACKET_TURF_RATE,sizeof(TurfRateStruct_t));
    //Write data to disk
    if(dataOrTelem!=2) {
	retVal=cleverHkWrite((unsigned char*)&turfRates,sizeof(TurfRateStruct_t),
			     turfRates.unixTime,&turfHkWriter);
    }
    if(dataOrTelem!=1) {
	// Write data for Telem
	sprintf(theFilename,"%s/turfhk_%ld_%ld.dat.gz",TURFHK_TELEM_DIR,
		turfRates.unixTime,turfRates.unixTimeUs);
	writeTurfRate(&turfRates,theFilename);
	retVal+=makeLink(theFilename,TURFHK_TELEM_LINK_DIR);
    }

    return retVal;
}

void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr)
{
//    printf("writeEventAndMakeLink(%s,%s,%d",theEventDir,theLinkDir,(int)theEventPtr);
    char theFilename[FILENAME_MAX];
    int retVal;
    AnitaEventHeader_t *theHeader=&(theEventPtr->header);
    AnitaEventBody_t *theBody=&(theEventPtr->body);


    //Fill Generic Header
    if(turfFirmwareVersion>=3) 
	fillGenericHeader(hdPtr,PACKET_HD,sizeof(AnitaEventHeader_t));
    else
	fillGenericHeader(hdPtr,PACKET_HD_SLAC,sizeof(AnitaEventHeader_t));
    fillGenericHeader(theBody,PACKET_BD,sizeof(AnitaEventBody_t));



    if(!useAltDir) {
	if(justWriteHeader==0 && writeData) {
	    sprintf(theFilename,"%s/ev_%ld.dat",theEventDir,
		    theEventPtr->header.eventNumber);
	    if(!oldStyleFiles) {
//		if(!compressWavefile) 
		retVal=writeBody(theBody,theFilename);  
//		else {
//		    strcat(theFilename,".gz");
//		    retVal=writeZippedBody(theBody,theFilename);
//		}
//		printf("%s\n",theFilename);
	    }
	    else {
		sprintf(theFilename,"%s/surf_data.%ld",theEventDir,
			theEventPtr->header.eventNumber);
		FILE *eventFile;
		int n;
		if ((eventFile=fopen(theFilename, "wb")) == NULL) { 
		    printf("Failed to open event file, %s\n", theFilename) ;
		    return ;
		}
		if ((n=fwrite(labData, sizeof(int), 
			      N_SAMP*N_CHAN*numSurfs,eventFile)) 
		    != N_SAMP*N_CHAN*numSurfs)  
		    printf("Failed to write all event data. wrote only %d.\n", n) ;
		fclose(eventFile);
	    }	    	    	    
	}
	
	if(writeData) {
	    sprintf(theFilename,"%s/hd_%ld.dat",theEventDir,
		    theEventPtr->header.eventNumber);
	    retVal=writeHeader(theHeader,theFilename);
	}
	
	/* Make links, not sure what to do with return value here */
	if(!standAloneMode && writeData) 
	    retVal=makeLink(theFilename,theLinkDir);
    }
    else {
	retVal=cleverEventWrite((unsigned char*)theBody,sizeof(AnitaEventBody_t),theHeader,&eventWriter);
	if(retVal<0) {
	    //Do something
	}
    }

}


AcqdErrorCode_t readSurfEventData(PlxHandle_t *surfHandles) 
/*! This is the code that is executed after we read the EVT_RDY flag */
/* It is the equivalent of read_LABdata in Shige's crate_test program */
{

    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    unsigned int  dataInt=0;

    unsigned char tempVal;
    int chanId=0,surf,chan=0,samp,firstHitbus,lastHitbus,wrappedHitbus;
    unsigned short upperWord=0;
    
    doingEvent++;
    slipCounter++;
    if(verbosity && printToScreen) 
	printf("Triggered, event %d (by software counter).\n",doingEvent);


  	
    //Loop over SURFs and read out data
    for(surf=0;surf<numSurfs;surf++){  
//	sleep(1);
	if(printToScreen &&verbosity>1) {
	    printf("GPIO register contents SURF %d = %o\n",surfIndex[surf],
		   PlxRegisterRead(surfHandles[surf], PCI9030_GP_IO_CTRL, &rc)) ;
	    printf("INT register contents SURF %d = %o\n",surfIndex[surf],
		   PlxRegisterRead(surfHandles[surf], PCI9030_INT_CTRL_STAT, &rc)) ;
	}

	//Set to read mode
	rc=setSurfControl(surfHandles[surf],RDMode);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Failed to set RDMode on SURF %d",surf);
	    if(printToScreen)
		fprintf(stderr,"Failed to set RDMode on SURF %d\n",surf);
	}
	rc=setSurfControl(surfHandles[surf],DTRead);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Failed to set DTRead on SURF %d (rc = %d)",surfIndex[surf],rc);
	    if(printToScreen)
		fprintf(stderr,"Failed to set RDMode on SURF %d (rc = %d)\n",surfIndex[surf],rc);
	}
	
	//First read to prime pipe
	dataInt=*(barMapAddr[surf]);
	
	for(chan=0;chan<N_CHAN; chan++) {
	    for (samp=0 ; samp<N_SAMP_EFF ; samp++) {		
		dataInt=*(barMapAddr[surf]);

		if(printToScreen && verbosity>2) {
		    printf("SURF %d (%d), CHIP %d, CHN %d, SCA %d: %x (HITBUS=%d) %d\n",surfIndex[surf],((dataInt&0xf0000)>>16),((dataInt&0x00c00000)>> 22),chan,samp,dataInt,((dataInt& 0x1000)>>12),(dataInt&DATA_MASK));
		}
		labData[surf][chan][samp]=dataInt;
	    }
	}
	for(chan=0;chan<N_CHAN; chan++) {
	    for (samp=N_SAMP_EFF ; samp<N_SAMP ; samp++) {		
		dataInt=*(barMapAddr[surf]);

		if(printToScreen && verbosity>2) {
		    printf("SURF %d (%d), CHIP %d,CHN %d, SCA %d: %x (HITBUS=%d) %d\n",surfIndex[surf],((dataInt&0xf0000)>>16),((dataInt&0x00c00000)>> 22),chan,samp,dataInt,dataInt & 0x1000,dataInt&DATA_MASK);
		}
		labData[surf][chan][samp]=dataInt;
	    }
	}
    }
    //Read out SURFs

    if(!oldStyleFiles) {
	for(surf=0;surf<numSurfs;surf++){  
	    for (chan=0 ; chan<N_CHAN ; chan++) {
		chanId=chan+surf*CHANNELS_PER_SURF;
		firstHitbus=-1;
		lastHitbus=-1;
		wrappedHitbus=0;
		for (samp=0 ; samp<N_SAMP ; samp++) {		
		    dataInt=labData[surf][chan][samp];
		    if(samp==0) upperWord=GetUpper16(dataInt);
/* 		    if(upperWord!=GetUpper16(dataInt)) { */
/* 			//Will add an error flag to the channel header */
/* 			syslog(LOG_WARNING,"Upper 16bits don't match: SURF %d Chan %d Samp %d (%x %x)",surfIndex[surf],chan,samp,upperWord,GetUpper16(dataInt)); */
/* 			if(printToScreen) */
/* 			    fprintf(stderr,"Upper word changed %x -- %x\n",upperWord,GetUpper16(dataInt));			 */
/* 		    } */
		    
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
		//End of sample loop
		tempVal=chan+N_CHAN*surf;
		theEvent.body.channel[chanId].header.chanId=tempVal;
		theEvent.body.channel[chanId].header.firstHitbus=firstHitbus;
		tempVal=((upperWord&0xc0)>>6)+(wrappedHitbus<<3);
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
    }   
    return status;
} 


AcqdErrorCode_t readSurfEventDataVer2(PlxHandle_t *surfHandles) 
/*! This is the code that is executed after we read the EVT_RDY flag */
/* It is the equivalent of read_LABdata in Shige's crate_test program */
/* And as the name suggests this is the code that expects version 2
   firmware (i.e. double word reads). */
{

    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    unsigned int  dataInt=0;

    unsigned char tempVal;
    int chanId=0,surf,chan=0,readCount,firstHitbus,lastHitbus,wrappedHitbus;
    unsigned int headerWord,samp=0;
    unsigned short upperWord=0;
    
    doingEvent++;
    if(verbosity && printToScreen) 
	printf("Triggered, event %d (by software counter).\n",doingEvent);

  	
    //Loop over SURFs and read out data
    for(surf=0;surf<numSurfs;surf++){  
//	sleep(1);
	if(printToScreen &&verbosity>1) {
	    printf("GPIO register contents SURF %d = %o\n",surfIndex[surf],
		   PlxRegisterRead(surfHandles[surf], PCI9030_GP_IO_CTRL, &rc)) ;
	    printf("INT register contents SURF %d = %o\n",surfIndex[surf],
		   PlxRegisterRead(surfHandles[surf], PCI9030_INT_CTRL_STAT, &rc)) ;
	}

	//Set to read mode
	rc=setSurfControl(surfHandles[surf],RDMode);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Failed to set RDMode on SURF %d",surf);
	    if(printToScreen)
		fprintf(stderr,"Failed to set RDMode on SURF %d\n",surf);
	}
	rc=setSurfControl(surfHandles[surf],DTRead);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Failed to set DTRead on SURF %d (rc = %d)",surfIndex[surf],rc);
	    if(printToScreen)
		fprintf(stderr,"Failed to set RDMode on SURF %d (rc = %d)\n",surfIndex[surf],rc);
	}
	
	//In version 2 data is read as 32 bit words:
	/* 0 -- Header Word
	   The next word only has 16 useful bits (upper)
	   1:128 -- Chan 0 Samps 0-255 (2 x 16 bit words per 32 bit read)
	   129:1152 -- Chans 1 through 8 128 reads per chan as above
	   1153:1154 -- Chan 0 Samps 256-259 (2 x 16 bit words per 32 bit read)
	   1155:1168 -- Chans 1 through 8 2 reads per chan as above
	   1169 last read; (only lower 16 bits)
	*/

	//First read header word
	headerWord=*(barMapAddr[surf]);
	

	//Now prime data stream
	dataInt=*(barMapAddr[surf]);
	
	//Now read first 256 samples per SURF
	for(chan=0;chan<N_CHAN; chan++) {
	    for (readCount=samp=0 ; readCount<N_SAMP_EFF/2 ; readCount++) {		
		dataInt=*(barMapAddr[surf]);

		if(printToScreen && verbosity>2) {
		    printf("SURF %d (%d), CHIP %d, CHN %d, Read %d: %x\n",surfIndex[surf],((headerWord&0xf0000)>>16),((headerWord&0x00c00000)>> 22),chan,readCount,dataInt);
		}
		
		//Store in array (will move to 16bit array when bothered
		if(readCount==0 ) {
		    //Funkiness
		    if(chan>0) {
			labData[surf][chan-1][N_SAMP_EFF-1]=dataInt&0xffff;
			labData[surf][chan-1][N_SAMP_EFF-1]|=headerWord&0xffff0000;
		    }
		    //if chan ==0 lower 16 bits are rubbish
			
		}
		else {
		    // Two 16 bit words
		    labData[surf][chan][samp]=dataInt&0xffff;
		    labData[surf][chan][samp++]|=headerWord&0xffff0000;
		}
		labData[surf][chan][samp]=(dataInt>>16);
		labData[surf][chan][samp++]|=headerWord&0xffff0000;
		
	    }
	}

	//Now read last 4 samples (+ last sample from above)
	for(chan=0;chan<N_CHAN; chan++) {
	    samp=N_SAMP_EFF;
	    for(readCount=N_SAMP_EFF/2;readCount<N_SAMP/2;readCount++) {
		dataInt=*(barMapAddr[surf]);
		
		if(printToScreen && verbosity>2) {
		    printf("SURF %d (%d), CHIP %d, CHN %d, Read %d: %x\n",surfIndex[surf],((headerWord&0xf0000)>>16),((headerWord&0x00c00000)>> 22),chan,readCount,dataInt);
		}
		//Store in array (will move to 16bit array when bothered
		if(readCount==N_SAMP_EFF/2) {
		    if(chan==0) {
			labData[surf][N_CHAN-1][N_SAMP_EFF-1]=dataInt&0xffff;
			labData[surf][N_CHAN-1][N_SAMP_EFF-1]|=headerWord&0xffff0000;
		    }
		    else {
			labData[surf][chan-1][N_SAMP-1]=dataInt&0xffff;
			labData[surf][chan-1][N_SAMP-1]|=headerWord&0xffff0000;
		    }
			
		}
		labData[surf][chan][samp]=(dataInt>>16);
		labData[surf][chan][samp++]|=headerWord&0xffff0000;
		labData[surf][chan][samp]=dataInt;
	    }
	}
	
	dataInt=*(barMapAddr[surf]);

	labData[surf][N_CHAN-1][N_SAMP-1]=dataInt&0xffff;
	labData[surf][N_CHAN-1][N_SAMP-1]|=headerWord&0xffff0000;
	
    }
    //Read out SURFs

    if(!oldStyleFiles) {
	for(surf=0;surf<numSurfs;surf++){  
	    for (chan=0 ; chan<N_CHAN ; chan++) {
		chanId=chan+surf*CHANNELS_PER_SURF;
		firstHitbus=-1;
		lastHitbus=-1;
		wrappedHitbus=0;
		for (samp=0 ; samp<N_SAMP ; samp++) {		
		    dataInt=labData[surf][chan][samp];
		    if(samp==0) upperWord=GetUpper16(dataInt);
/* 		    if(upperWord!=GetUpper16(dataInt)) { */
/* 			//Will add an error flag to the channel header */
/* 			syslog(LOG_WARNING,"Upper 16bits don't match: SURF %d Chan %d Samp %d (%x %x)",surfIndex[surf],chan,samp,upperWord,GetUpper16(dataInt)); */
/* 			if(printToScreen) */
/* 			    fprintf(stderr,"Upper word changed %x -- %x\n",upperWord,GetUpper16(dataInt));			 */
/* 		    } */
		    
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
		//End of sample loop
		tempVal=chan+N_CHAN*surf;
		theEvent.body.channel[chanId].header.chanId=tempVal;
		theEvent.body.channel[chanId].header.firstHitbus=firstHitbus;
		tempVal=((upperWord&0xc0)>>6)+(wrappedHitbus<<3);
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


AcqdErrorCode_t readSurfEventDataVer3(PlxHandle_t *surfHandles) 
/*! Updated to the latest SURF firmware verison, which I am calling 3 for now */
{

    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    unsigned int  dataInt=0;

    unsigned char tempVal;
    int chanId=0,surf,chan=0,readCount,firstHitbus,lastHitbus,wrappedHitbus;
    int rcoSamp;
    unsigned int headerWord,samp=0;
    unsigned short upperWord=0;
    unsigned char rcoBit;
    

    doingEvent++;
    if(verbosity && printToScreen) 
	printf("Triggered, event %d (by software counter).\n",doingEvent);

  	
    //Loop over SURFs and read out data
    for(surf=0;surf<numSurfs;surf++){  
//	sleep(1);
	if(printToScreen &&verbosity>1) {
	    printf("GPIO register contents SURF %d = %o\n",surfIndex[surf],
		   PlxRegisterRead(surfHandles[surf], PCI9030_GP_IO_CTRL, &rc)) ;
	    printf("INT register contents SURF %d = %o\n",surfIndex[surf],
		   PlxRegisterRead(surfHandles[surf], PCI9030_INT_CTRL_STAT, &rc)) ;
	}

	//Set to read mode
	rc=setSurfControl(surfHandles[surf],RDMode);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Failed to set RDMode on SURF %d",surf);
	    if(printToScreen)
		fprintf(stderr,"Failed to set RDMode on SURF %d\n",surf);
	}
	rc=setSurfControl(surfHandles[surf],DTRead);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Failed to set DTRead on SURF %d (rc = %d)",surfIndex[surf],rc);
	    if(printToScreen)
		fprintf(stderr,"Failed to set DTRead on SURF %d (rc = %d)\n",surfIndex[surf],rc);
	}
	
	//In version 3 data is read as 32 bit words:
	/* 0 -- Header Word
	   1 -- Prime Word
	   2:129 -- Chan 0 Samps 0-255 (2 x 16 bit words per 32 bit read)
	   130:1153 -- Chans 1 through 8 128 reads per chan as above
	   1154:1155 -- Chan 0 Samps 256-259 (2 x 16 bit words per 32 bit read)
	   1156:1169 -- Chans 1 through 8 2 reads per chan as above
	*/

	//First read header word
	headerWord=*(barMapAddr[surf]);
	if(printToScreen && verbosity>2) {
	    printf("SURF %d (%d), CHIP %d, CHN %d, Header: %x\n",surfIndex[surf],((headerWord&0xf0000)>>16),((headerWord&0x00c00000)>> 22),chan,headerWord);
	}
	
	if(surf==0) {
	    hdPtr->otherFlag=0;
	    hdPtr->otherFlag2=0;
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


	//Now prime data stream
	dataInt=*(barMapAddr[surf]);
	if(printToScreen && verbosity>2) {
	    printf("SURF %d (%d), CHIP %d, CHN %d, Prime: %x\n",surfIndex[surf],((headerWord&0xf0000)>>16),((headerWord&0x00c00000)>> 22),chan,dataInt);
	}

	//Now read first 256 samples per SURF
	for(chan=0;chan<N_CHAN; chan++) {
	    for (readCount=samp=0 ; readCount<N_SAMP_EFF/2 ; readCount++) {		
		dataInt=*(barMapAddr[surf]);

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

		//Upper word
		labData[surf][chan][samp]=dataInt&0xffff;
		labData[surf][chan][samp++]|=headerWord&0xffff0000;

		//Lower word
		labData[surf][chan][samp]=(dataInt>>16);
		labData[surf][chan][samp++]|=headerWord&0xffff0000;
		
	    }
	}

	//Now read last 4 samples
	for(chan=0;chan<N_CHAN; chan++) {
	    samp=N_SAMP_EFF;
	    for(readCount=N_SAMP_EFF/2;readCount<N_SAMP/2;readCount++) {
		dataInt=*(barMapAddr[surf]);
		
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
		
		labData[surf][chan][samp]=dataInt&0xffff;
		labData[surf][chan][samp++]|=headerWord&0xffff0000;
//		}
		labData[surf][chan][samp]=(dataInt>>16);
		labData[surf][chan][samp++]|=headerWord&0xffff0000;

	    }
	}
	
//	dataInt=*(barMapAddr[surf]);

//	labData[surf][N_CHAN-1][N_SAMP-1]=dataInt&0xffff;
//	labData[surf][N_CHAN-1][N_SAMP-1]|=headerWord&0xffff0000;
	
    }
    //Read out SURFs

    if(!oldStyleFiles) {
	for(surf=0;surf<numSurfs;surf++){  
	    for (chan=0 ; chan<N_CHAN ; chan++) {
		chanId=chan+surf*CHANNELS_PER_SURF;
		firstHitbus=-1;
		lastHitbus=-1;
		wrappedHitbus=0;
		for (samp=0 ; samp<N_SAMP ; samp++) {		
		    dataInt=labData[surf][chan][samp];
		    if(samp==0) upperWord=GetUpper16(dataInt);
/* 		    if(upperWord!=GetUpper16(dataInt)) { */
/* 			//Will add an error flag to the channel header */
/* 			syslog(LOG_WARNING,"Upper 16bits don't match: SURF %d Chan %d Samp %d (%x %x)",surfIndex[surf],chan,samp,upperWord,GetUpper16(dataInt)); */
/* 			if(printToScreen) */
/* 			    fprintf(stderr,"Upper word changed %x -- %x\n",upperWord,GetUpper16(dataInt));			 */
/* 		    } */
		    
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
		rcoSamp=firstHitbus-1;
		if(wrappedHitbus)
		    rcoSamp=lastHitbus-1;
		if(rcoSamp<0 || rcoSamp>255) rcoSamp=100;
		
		rcoBit=((labData[surf][chan][rcoSamp]&0x2000)>>13);
		//End of sample loop
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



AcqdErrorCode_t readSurfHkData(PlxHandle_t *surfHandles) 
// Reads the scaler and RF power data from the SURF board
{
   
    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    unsigned int  dataInt=0;
    int surf,rfChan;

    hkNumber++;
    if(verbosity && printToScreen) 
	printf("Reading Surf HK %d.\n",hkNumber);
        
    if(enableChanServo)
	theSurfHk.scalerGoal=pidGoal;
    else 
	theSurfHk.scalerGoal=0;

    for(surf=0;surf<numSurfs;surf++){  

	//Set to read house keeping
	//Send DTRead first to have a transition on DT/HK line
//	rc=setSurfControl(surfHandles[surf],DTRead);
//	if(rc!=ApiSuccess) {
//	    syslog(LOG_ERR,"Failed to set DTRead on SURF %d (rc = %d)",surfIndex[surf],rc);
//	    if(printToScreen)
//		fprintf(stderr,"Failed to set RDMode on SURF %d (rc = %d)\n",surfIndex[surf],rc);
//	}
	rc=setSurfControl(surfHandles[surf],RDMode);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Failed to set RDMode on SURF %d",surf);
	    if(printToScreen)
		fprintf(stderr,"Failed to set RDMode on SURF %d\n",surf);
	}


	//First just fill in antMask in SURF
	for(rfChan=0;rfChan<2;rfChan++) {
	    theSurfHk.surfTrigBandMask[surf][rfChan]=surfTrigBandMasks[surf][rfChan];
	}

	
	//First read the scaler data
	for(rfChan=0;rfChan<N_RFTRIG;rfChan++){
	    dataInt=*(barMapAddr[surf]);

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
	for(rfChan=0;rfChan<N_RFTRIG;rfChan++){
	    dataInt=*(barMapAddr[surf]);

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
	    dataInt=*(barMapAddr[surf]);
	    theSurfHk.rfPower[surf][rfChan]=dataInt&0xfff;
            if((printToScreen && verbosity>1) || HK_DEBUG)
                printf("Surf %d, RF Power %d == %d\n",surfIndex[surf],rfChan,theSurfHk.rfPower[surf][rfChan]&0xfff);
            //Should check if it is the same or not
            if(theSurfHk.upperWords[surf]!=GetUpper16(dataInt)) {
                theSurfHk.errorFlag|=(1>>surf);
            }

	}
    }
    return status;
}



AcqdErrorCode_t readTurfEventDataVer2(PlxHandle_t turfioHandle)
/*! Reads out the TURF data via the TURFIO */
{

//    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    unsigned short  dataWord=0;
    unsigned char upperChar;
    unsigned char dataChar;
    unsigned short dataShort;
    unsigned long dataLong;

    int wordNum,surf,ant,errCount=0;
    TurfioTestPattern_t startPat;
    TurfioTestPattern_t endPat;
    
    //First read to prime pipe
//	dataWord=*(turfBarMap);
// Do we need prime read (rjn 20/05/06)
    
    SlacTurfioStruct_t *oldTurfioPtr = (SlacTurfioStruct_t*) turfioPtr;
    
    //Read out 160 words
    for(wordNum=0;wordNum<160;wordNum++) {
	if (readPlxDataShort(turfioHandle,&dataWord)!= ApiSuccess) {
	    status=ACQD_E_PLXBUSREAD;
	    syslog(LOG_ERR,"Failed to read TURFIO word %d",wordNum);
	    if(printToScreen) 
		printf("Failed to read TURFIO word %d\n",wordNum) ;
	    continue;
	}
	dataChar=0;
	dataShort=0;
	dataLong=0;
	dataChar=dataWord&0xff;
	dataShort=dataWord&0xff;
	dataLong=dataWord&0xff;
	upperChar=(dataWord&0xff00)>>8;
	if(printToScreen && verbosity>1) {
	    printf("TURFIO -- Word %d  -- %x -- %x + %x\n",wordNum,dataWord,
		   dataShort,upperChar);
	}
	if(wordNum==0) 
	    hdPtr->turfUpperWord=upperChar;

	if(wordNum<8) {
	    startPat.test[wordNum]=dataChar; 
	} 
	else if(wordNum<24) {
	    switch(wordNum) {
		case 8:
		    oldTurfioPtr->trigType=dataChar; break;
		case 9:
		    oldTurfioPtr->l3Type1Count=dataChar; break;
		case 10:
		    oldTurfioPtr->trigNum=dataShort; break;
		case 11:
		    oldTurfioPtr->trigNum+=(dataShort<<8); break;
		case 12:
		    oldTurfioPtr->trigTime=dataLong; break;
		case 13:
		    oldTurfioPtr->trigTime+=(dataLong<<8); break;
		case 14:
		    oldTurfioPtr->trigTime+=(dataLong<<16); break;
		case 15:
		    oldTurfioPtr->trigTime+=(dataLong<<24); break;
		case 16:
		    oldTurfioPtr->ppsNum=dataLong; break;
		case 17:
		    oldTurfioPtr->ppsNum+=(dataLong<<8); break;
		case 18:
		    oldTurfioPtr->ppsNum+=(dataLong<<16); break;
		case 19:
		    oldTurfioPtr->ppsNum+=(dataLong<<24); break;
		case 20:
		    oldTurfioPtr->c3poNum=dataLong; break;
		case 21:
		    oldTurfioPtr->c3poNum+=(dataLong<<8); break;
		case 22:
		    oldTurfioPtr->c3poNum+=(dataLong<<16); break;
		case 23:
		    oldTurfioPtr->c3poNum+=(dataLong<<24); break;
		default:
		    // Can't get here
		    break;
	    }
	}
	else if(wordNum<88) {
	    //Surf Antenna counters
	    surf=(wordNum-24)/8;
	    ant=(wordNum%8)/2;
	    if(wordNum%2==0) {
		//First word
		turfRates.l1Rates[surf][ant]=dataShort;
	    }
	    else if(wordNum%2==1) {
		//Second word
		turfRates.l1Rates[surf][ant]+=(dataShort<<8);
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
		oldTurfioPtr->upperL1TrigPattern=dataShort;
	    }
	    else if(wordNum%2==1) {
		//Second word
		oldTurfioPtr->upperL1TrigPattern+=(dataShort<<8);
	    }
	}
	else if(wordNum<140) {
	    if(wordNum%2==0) {
		//First word
		oldTurfioPtr->lowerL1TrigPattern=dataShort;
	    }
	    else if(wordNum%2==1) {
		//Second word
		oldTurfioPtr->lowerL1TrigPattern+=(dataShort<<8);
	    }
	}
	else if(wordNum<142) {
	    if(wordNum%2==0) {
		//First word
		oldTurfioPtr->upperL2TrigPattern=dataShort;
	    }
	    else if(wordNum%2==1) {
		//Second word
		oldTurfioPtr->upperL2TrigPattern+=(dataShort<<8);
	    }
	}
	else if(wordNum<144) {
	    if(wordNum%2==0) {
		//First word
		oldTurfioPtr->lowerL2TrigPattern=dataShort;
	    }
	    else if(wordNum%2==1) {
		//Second word
		oldTurfioPtr->lowerL2TrigPattern+=(dataShort<<8);
	    }
	}
	else if(wordNum<146) {
	    if(wordNum%2==0) {
		//First word
		oldTurfioPtr->l3TrigPattern=dataShort;
	    }
	    else if(wordNum%2==1) {
		//Second word
		oldTurfioPtr->l3TrigPattern+=(dataShort<<8);
	    }
	}
	else if(wordNum<148) {
	    if(wordNum%2==0) {
		//First word
		oldTurfioPtr->l3TrigPattern2=dataShort;
	    }
	    else if(wordNum%2==1) {
		//Second word
		oldTurfioPtr->l3TrigPattern2+=(dataShort<<8);
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
    
    if(verbosity && printToScreen) {
	printf("TURF Data\n\tEvent (software):\t%lu\n\tupperWord:\t0x%x\n\ttrigNum:\t%u\n\ttrigType:\t0x%x\n\ttrigTime:\t%lu\n\tppsNum:\t\t%lu\n\tc3p0Num:\t%lu\n\tl3Type1#:\t%u\n",
	       hdPtr->eventNumber,hdPtr->turfUpperWord,oldTurfioPtr->trigNum,oldTurfioPtr->trigType,oldTurfioPtr->trigTime,oldTurfioPtr->ppsNum,oldTurfioPtr->c3poNum,oldTurfioPtr->l3Type1Count);
	printf("Trig Patterns:\nUpperL1:\t%x\nLowerL1:\t%x\nUpperL2:\t%x\nLowerL2:\t%x\nL31:\t%x\nL32:\t%x\n",
	       oldTurfioPtr->upperL1TrigPattern,
	       oldTurfioPtr->lowerL2TrigPattern,
	       oldTurfioPtr->upperL2TrigPattern,
	       oldTurfioPtr->lowerL2TrigPattern,
	       oldTurfioPtr->l3TrigPattern,
	       oldTurfioPtr->l3TrigPattern2);
    }
	
    return status;	
}



AcqdErrorCode_t readTurfEventDataVer3(PlxHandle_t turfioHandle)
/*! Reads out the TURF data via the TURFIO */
{
    newTurfRateData=0;
//    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    unsigned short  dataWord=0;
    unsigned char upperChar;
    unsigned char dataChar;
    unsigned short dataShort;
    unsigned long dataLong;
    static unsigned short lastPPSNum=0;

    int wordNum,surf,ant,errCount=0;
    TurfioTestPattern_t startPat;
    TurfioTestPattern_t endPat;
    
    //First read to prime pipe
//	dataWord=*(turfBarMap);
// Do we need prime read (rjn 20/05/06)
    
    
    //Read out 160 words
    for(wordNum=0;wordNum<160;wordNum++) {
	if (readPlxDataShort(turfioHandle,&dataWord)!= ApiSuccess) {
	    status=ACQD_E_PLXBUSREAD;
	    syslog(LOG_ERR,"Failed to read TURFIO word %d",wordNum);
	    if(printToScreen) 
		printf("Failed to read TURFIO word %d\n",wordNum) ;
	    continue;
	}
	dataChar=0;
	dataShort=0;
	dataLong=0;
	dataChar=dataWord&0xff;
	dataShort=dataWord&0xff;
	dataLong=dataWord&0xff;
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
		    turfioPtr->trigTime=dataLong; break;
		case 13:
		    turfioPtr->trigTime+=(dataLong<<8); break;
		case 14:
		    turfioPtr->trigTime+=(dataLong<<16); break;
		case 15:
		    turfioPtr->trigTime+=(dataLong<<24); break;
		case 16:
		    turfioPtr->ppsNum=dataShort; break;
		case 17:
		    turfioPtr->ppsNum+=(dataShort<<8); break;
		case 18:
		    turfioPtr->deadTime=(dataShort); break;
		case 19:
		    turfioPtr->deadTime+=(dataShort<<8); break;
		case 20:
		    turfioPtr->c3poNum=dataLong; break;
		case 21:
		    turfioPtr->c3poNum+=(dataLong<<8); break;
		case 22:
		    turfioPtr->c3poNum+=(dataLong<<16); break;
		case 23:
		    turfioPtr->c3poNum+=(dataLong<<24); break;
		default:
		    // Can't get here
		    break;
	    }
	}
	else if(wordNum<88) {
	    //Surf Antenna counters
	    surf=(wordNum-24)/8;
	    ant=(wordNum%8)/2;
	    if(wordNum%2==0) {
		//First word
		turfRates.l1Rates[surf][ant]=dataShort;
	    }
	    else if(wordNum%2==1) {
		//Second word
		turfRates.l1Rates[surf][ant]+=(dataShort<<8);
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

    if(turfioPtr->deadTime>0 && lastPPSNum!=turfioPtr->ppsNum) {
	intervalDeadtime+=((float)turfioPtr->deadTime)/64400.;
	if(printToScreen && verbosity>1)
	    printf("Deadtime %d counts %f fraction\n",turfioPtr->deadTime,
		   ((float)turfioPtr->deadTime)/64400.);
    }

    
    if(verbosity && printToScreen) {
	printf("TURF Data\n\tEvent (software):\t%lu\n\tupperWord:\t0x%x\n\ttrigNum:\t%u\n\ttrigType:\t0x%x\n\ttrigTime:\t%lu\n\tppsNum:\t\t%u\n\tc3p0Num:\t%lu\n\tl3Type1#:\t%u\n\tdeadTime:\t\t%u\n",
	       hdPtr->eventNumber,hdPtr->turfUpperWord,turfioPtr->trigNum,turfioPtr->trigType,turfioPtr->trigTime,turfioPtr->ppsNum,turfioPtr->c3poNum,turfioPtr->l3Type1Count,turfioPtr->deadTime);
	printf("Trig Patterns:\nUpperL1:\t%x\nLowerL1:\t%x\nUpperL2:\t%x\nLowerL2:\t%x\nL31:\t%x\n",
	       turfioPtr->upperL1TrigPattern,
	       turfioPtr->lowerL2TrigPattern,
	       turfioPtr->upperL2TrigPattern,
	       turfioPtr->lowerL2TrigPattern,
	       turfioPtr->l3TrigPattern);
    }
    if(turfioPtr->ppsNum!=lastPPSNum) {
	newTurfRateData=1;
    }
    lastPPSNum=turfioPtr->ppsNum;
    return status;	
}


PlxReturnCode_t setBarMap(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle) {
    PlxReturnCode_t rc=0;
    int surf,addrVal=0;

    for(surf=0;surf<numSurfs;surf++) {
	rc=PlxPciBarMap(surfHandles[surf],2,&addrVal);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Unable to map PCI bar 2 on SURF %d (%d)",surfIndex[surf],rc);
	}
	barMapAddr[surf]=(int*)addrVal;
	if(verbosity && printToScreen) {
	    printf("Bar Addr[%d] is %x\t%x\n",surfIndex[surf],(int)barMapAddr[surf],addrVal);
	}
    }
    rc=PlxPciBarMap(turfioHandle,2,&addrVal);
    if(rc!=ApiSuccess) {
	syslog(LOG_ERR,"Unable to map PCI bar 2 on TURFIO (%d)",rc);
    }
    turfBarMap=(int*)addrVal;
    if(verbosity && printToScreen) {
	printf("Turf Bar Addr is %x\t%x\n",(int)turfBarMap,addrVal);
    }
    return rc;
}


PlxReturnCode_t unsetBarMap(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle) {
  
    PlxReturnCode_t rc=0;
    int surf;
    unsigned int addrVal=0;
    for(surf=0;surf<numSurfs;surf++) {    
	addrVal=(unsigned int)(*barMapAddr[surf]);
	rc=PlxPciBarUnmap(surfHandles[surf],&addrVal);
    }
    addrVal=(unsigned int)(*turfBarMap);
    rc=PlxPciBarUnmap(turfioHandle,&addrVal);
    return rc;  
    
}

PlxReturnCode_t writeAntTrigMask(PlxHandle_t turfioHandle) 
/*!
  Remember that 0 in the RFCM mask means on
*/
{
    PlxReturnCode_t rc;
    U32 gpio ;  
    int i=0,j=0,chanBit=1;
    int actualMask[2]={255,0};
    
    actualMask[0]|=((antTrigMask&0xffffff)<<8);
    actualMask[1]=(antTrigMask&0xff000000)>>24;
    syslog(LOG_INFO,"antTrigMask: %#x  actualMask[1]:%#x   actualMask[0]:%#x\n",
	   antTrigMask,actualMask[1],actualMask[0]);

    rc=setTurfControl(turfioHandle,RFCBit);
    if(rc!=ApiSuccess) {		
	syslog(LOG_ERR,"Failed to set 1st RFCBit %d",rc);
	if(printToScreen)			    
	    printf("Failed to set 1st RFCBit %d.\n",rc) ;
    }	

    //Now send bit pattern to TURF using RFCBit and RFCClk
    while(i<40) {
	if (i==32) j=chanBit=1 ;
	
//	printf("Debug RFCM: i=%d j=%d chn_bit=%d mask[j]=%d\twith and %d\n",i,j,chanBit,actualMask[j],(actualMask[j]&chanBit));
	if(actualMask[j]&chanBit) rc=setTurfControl(turfioHandle,RFCBit);
	else rc=setTurfControl(turfioHandle,RFCClk);
	if (rc != ApiSuccess) {
	    syslog(LOG_ERR,"Failed to send RFC bit %d to TURF, %x %x",
		   i,actualMask[1],actualMask[0]);
	    if(printToScreen)
		fprintf(stderr,"Failed to send RFC bit %d to TURF, %x %x\n",
			i,actualMask[1],actualMask[0]);
	}	
	chanBit<<=1;
	i++;
    }

    /* check GPIO8 is set or not. */
    if((gpio=PlxRegisterRead(turfioHandle, PCI9030_GP_IO_CTRL, &rc )) 
       & 0400000000) {
	syslog(LOG_INFO,"writeAntTrigMask: GPIO8 on, so mask should be set to %#04x %#010x.\n", actualMask[1],actualMask[0]) ;
	if(printToScreen) 
	    printf("writeAntTrigMask: GPIO8 on, so mask should be set to %#04x %#010x.\n", actualMask[1], actualMask[0]) ;
    }   
    else {
	syslog(LOG_ERR,"writeAntTrigMask: GPIO 8 not on: GPIO=%o",gpio);
	if(printToScreen) 
	    printf("writeAntTrigMask: GPIO 8 not on: GPIO=%o\n",gpio);
    }
    return ApiSuccess;
    
}


int updateThresholdsUsingPID() {
    static int avgCount=0;
    int wayOffCount=0;
    int surf,dac,error,value,change;
    float pTerm, dTerm, iTerm;
    int chanGoal;
    avgCount++;
    for(surf=0;surf<numSurfs;surf++) {
	if(dacSurfs[surf]==1) {
	    for(dac=0;dac<N_RFTRIG;dac++) {
		chanGoal=(int)(pidGoal*pidGoalScaleFactors[surf][dac]);
		if(chanGoal<0) chanGoal=0;
		if(chanGoal>32000) chanGoal=32000;
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
	for(surf=0;surf<numSurfs;surf++) {
	    for(dac=0;dac<N_RFTRIG;dac++) {		
		chanGoal=(int)(pidGoal*pidGoalScaleFactors[surf][dac]);
		if(chanGoal<0) chanGoal=0;
		if(chanGoal>32000) chanGoal=32000;
		if(dacSurfs[surf]==1) {
		    value=avgScalerData[surf][dac]/avgCount;
		    avgScalerData[surf][dac]=0;
		    error=chanGoal-value;
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


void servoOnRate(unsigned long eventNumber, unsigned long lastRateCalcEvent, struct timeval *currentTime, struct timeval *lastRateCalcTime)
{
    static float dState=0;  // Last position input
    static float iState=0;  // Integrator state
    float timespan=(currentTime->tv_sec-lastRateCalcTime->tv_sec) 
	+ 1e-6*(float)(currentTime->tv_usec)
	-1e-6*(float)(lastRateCalcTime->tv_usec);
    float rate=((float)(eventNumber-lastRateCalcEvent))/timespan;    
    float error;
    int change,setting;
    float pTerm, dTerm, iTerm;

    if(!setGlobalThreshold) 
	setting=pidGoal;
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
	setting+=change;
	if(!setGlobalThreshold) {
	    pidGoal=setting;
	    if(pidGoal<1) pidGoal=1;
	    if(pidGoal>10000) pidGoal=10000;
	    syslog(LOG_INFO,"Changing pidGoal to %d\n",pidGoal);
	}
	else {
	    globalThreshold=setting;
	    if(globalThreshold<1) globalThreshold=1;
	    if(globalThreshold>4095) globalThreshold=4095;
	    syslog(LOG_INFO,"Changing globalThreshold to %d\n",pidGoal);
	}    
    }
}
