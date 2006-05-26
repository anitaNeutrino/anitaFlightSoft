/*  \file Acqd.c
    \brief The Acqd program 
    The first functional version of the acquisition software to work with SURF version 3. For now this guy will try to have the same functionality as crate_test, but will look less like a dead dog. 
    March 2006 rjn@mps.ohio-state.edu
*/

// Standard Includes
#define _GNU_SOURCE
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
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "Acqd.h"

#define TIME_DEBUG 1
//#define BAD_DEBUG 1


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
int turfRateTelemEvery=1,turfRateTelemInterval=60;
int surfHkPeriod=0;
int surfHkTelemEvery=0;
int lastSurfHk=0;
int lastTurfHk=0;
int turfHkCounter=0;
int surfHkCounter=0;
int surfMask;
int useUSBDisks=0;
int compressWavefile=1;
int useInterrupts=0;
unsigned short data_array[MAX_SURFS][N_CHAN][N_SAMP]; 
AnitaEventFull_t theEvent;
AnitaEventHeader_t *hdPtr;//=&(theEvent.header);
TurfioStruct_t *turfioPtr;//=&(hdPtr->turfio);
TurfRateStruct_t turfRates;


SimpleScalerStruct_t theScalers;
FullSurfHkStruct_t theSurfHk;

//Temporary Global Variables
unsigned int labData[MAX_SURFS][N_CHAN][N_SAMP];
unsigned int avgScalerData[MAX_SURFS][N_RFTRIG];
//unsigned short threshData[MAX_SURFS][N_RFTRIG];
//unsigned short rfpwData[MAX_SURFS][N_RFCHAN];



//Configurable watchamacallits
/* Ports and directories */
char acqdEventDir[FILENAME_MAX];
char altOutputdir[FILENAME_MAX];
char subAltOutputdir[FILENAME_MAX];
char acqdPidFile[FILENAME_MAX];
char acqdEventLinkDir[FILENAME_MAX];
char lastEventNumberFile[FILENAME_MAX];
char scalerOutputDir[FILENAME_MAX];
char hkOutputDir[FILENAME_MAX];
char surfHkTelemDir[FILENAME_MAX];
char turfHkTelemDir[FILENAME_MAX];
char surfHkTelemLinkDir[FILENAME_MAX];
char turfHkTelemLinkDir[FILENAME_MAX];
char surfHkArchiveDir[FILENAME_MAX];
char turfHkArchiveDir[FILENAME_MAX];
char surfHkUSBArchiveDir[FILENAME_MAX];
char turfHkUSBArchiveDir[FILENAME_MAX];


#define N_TMO 100    /* number of msec wait for Evt_f before timed out. */
#define N_TMO_INT 10000 //Millisec to wait for interrupt
#define N_FTMO 200    /* number of msec wait for Lab_f before timed out. */
//#define N_FTMO 2000    /* number of msec wait for Lab_f before timed out. */
//#define DAC_CHAN 4 /* might replace with N_CHIP at a later date */
#define EVTF_TIMEOUT 10000000 /* in microseconds */

int firmwareVersion=2;
int verbosity = 0 ; /* control debug print out. */
int addedVerbosity = 0 ; /* control debug print out. */
int oldStyleFiles = FALSE;
int useAltDir = FALSE;
int madeAltDir = FALSE;
int selftrig = FALSE ;/* self-trigger mode */
int surfonly = FALSE; /* Run in surf only mode */
int writeData = FALSE; /* Default is not to write data to a disk */
int writeScalers = FALSE;
int writeFullHk = FALSE;
int justWriteHeader = FALSE;
int doSlowDacCycle = FALSE; /* Do a cycle of the DAC values */
int doGlobalDacCycle = FALSE;
int printStatistics = FALSE;
int dontWaitForEvtF = FALSE;
int dontWaitForLabF = FALSE;
int sendSoftTrigger = FALSE;
int softTrigSleepPeriod = 0;
int writeOutC3p0Nums = FALSE;
int reprogramTurf = FALSE;
int tryToUseBarMap = FALSE;

//Trigger Modes
TriggerMode_t trigMode=TrigNone;
int pps1TrigFlag = 0;
int pps2TrigFlag = 0;

//Threshold Stuff
//int thresholdArray[ACTIVE_SURFS][DAC_CHAN][PHYS_DAC];
int surfAntTrigMasks[ACTIVE_SURFS][2];
int thresholdArray[ACTIVE_SURFS][N_RFTRIG];
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
int pidAverage;

//RFCM Mask
//1 is disable
//lowest bit of first word is antenna 1
//bit 8 in second word is 48th antenna
//We load in reverse order starting with the missing SURF 10 
//and working backwards to SURF 1
int rfcmMask[2] = {0x10001000, 0x00008000} ;


//Test of BarMap
//U32 barMapDataWord[ACTIVE_SURFS];
__volatile__ int *barMapAddr[ACTIVE_SURFS];
__volatile__ int *turfBarMap;





#ifdef TIME_DEBUG
    FILE *timeFile;
#endif    


int main(int argc, char **argv) {
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
    int numDevices, numEvents=0 ;
    int tmpGPIO,tmpINTR;
    unsigned short dacVal=2200;
    int surf;
    int gotSurfHk=0;
    
    unsigned short doingDacVal=1;//2100;
    struct timeval timeStruct;
    
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
    turfioPtr=&(hdPtr->turfio);

    //Initialize dacPid stuff
    memset(&thePids,0,sizeof(DacPidStruct_t)*ACTIVE_SURFS*N_RFTRIG);    
   
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);

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

    if(tryToUseBarMap) {	
	rc = setBarMap(surfHandles,turfioHandle);
	if(rc!=ApiSuccess) {
	    printf("Error setting bar map\n");
	}
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

    // Clear devices 
    clearDevices(surfHandles,turfioHandle);
    
    do {
	retVal=readConfigFile();

	if(retVal!=CONFIG_E_OK) {
	    syslog(LOG_ERR,"Error reading config file Acqd.config: bailing");
	    return -1;
	}

	// Set trigger modes
	//RF and Software Triggers enabled by default
	trigMode=TrigNone;
	if(pps1TrigFlag) trigMode|=TrigPPS1;
	if(pps2TrigFlag) trigMode|=TrigPPS2;
	// RJN debugging
//	setTurfControl(turfioHandle,SetTrigMode);

	//Write RFCM Mask
	writeRFCMMask(turfioHandle);
	
	theSurfHk.globalThreshold=0;
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
//	    fprintf(stderr,"while() { %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
	    fprintf(timeFile,"1 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    if(standAloneMode && (numEvents && numEvents<=doingEvent)) {
		currentState=PROG_STATE_TERMINATE;
		break;
	    }
	    //Fill theEvent with zeros 
	    bzero(&theEvent, sizeof(theEvent)) ;
	    memset(&theSurfHk,0,sizeof(FullSurfHkStruct_t));
	    if(setGlobalThreshold) 
		theSurfHk.globalThreshold=globalThreshold;

	    if(doGlobalDacCycle) {		
//		if(dacVal<=500) dacVal=4095;
//		else if(dacVal>=4000) dacVal=0;
		dacVal=doingDacVal;
		theScalers.threshold=dacVal;
		if(printToScreen) 
		    printf("Setting Local Threshold %d\n",dacVal);
		setGloablDACThreshold(surfHandles,dacVal);	
		theSurfHk.globalThreshold=dacVal;
		if(doingDacVal>4095) exit(0);//doingDacVal=0;
		doingDacVal++;
	    }

	    //Send software trigger if we want to
	    if(sendSoftTrigger) {
		sleep(softTrigSleepPeriod);
		setTurfControl(turfioHandle,SendSoftTrg);
	    }

	    if (selftrig){  /* --- send a trigger ---- */ 
		if(printToScreen) printf(" try to trigger it locally.\n") ;
		for(surf=0;surf<numSurfs;++surf) 
		    if (setSurfControl(surfHandles[surf], LTrig) != ApiSuccess) {
			status=ACQD_E_LTRIG;
			syslog(LOG_ERR,"Failed to set LTrig on SURF %d",surfIndex[surf]);
			if(printToScreen)
			    printf("Failed to set LTrig flag on SURF %d.\n",surfIndex[surf]);
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
		if(verbosity && printToScreen) {
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
	    
		if(printToScreen && verbosity) {		
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
			if(tmo%2 && tmo) //Might change tmo%2
			    usleep(1); 
			tmo++;
			if((tmo%1000)==0) {
			    if(currentState!=PROG_STATE_RUN) 
				break;
			    //Time out not in self trigger mode
			    if(tmo==EVTF_TIMEOUT) break;
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
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"Got for EVT_RDY { %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
	    fprintf(timeFile,"3 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif    

	    //Either have a trigger or are going ahead regardless
	    gettimeofday(&timeStruct,NULL);
#ifdef TIME_DEBUG
//	    fprintf(stderr,"readSurfEventData -- start %ld s, %ld ms\n",timeStruct.tv_sec,timeStruct.tv_usec);
	    fprintf(timeFile,"4 %ld %ld\n",timeStruct.tv_sec,timeStruct.tv_usec);  
#endif
	    if(firmwareVersion==1) {
		status+=readSurfEventData(surfHandles);
	    }
	    else {
		status+=readSurfEventDataVer2(surfHandles);
	    }
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

#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"6 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
//	    fprintf(stderr,"readSurfHkData -- start %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
#endif
	    if((timeStruct.tv_sec-lastSurfHk)>=surfHkPeriod) {
		//Record housekeeping data
		status+=readSurfHkData(surfHandles);
		gotSurfHk=1;
		lastSurfHk=timeStruct.tv_sec;
		theSurfHk.unixTime=timeStruct.tv_sec;
		theSurfHk.unixTimeUs=timeStruct.tv_usec;
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
	    }



#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readTurfEventData -- start %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"9 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    status+=readTurfEventData(turfioHandle);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readTurfEventData -- end %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"10 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

	    if(verbosity && printToScreen) printf("Done reading\n");
	    //Error checking
	    if(status!=ACQD_E_OK) {
		//We have an error
		if(printToScreen) fprintf(stderr,"Error detected %d, during event %d",status,doingEvent);
		
		status=ACQD_E_OK;
	    }
	    else {
		hdPtr->eventNumber=getEventNumber();
		hdPtr->surfMask=surfMask;
		hdPtr->rfcmMask[0]=(unsigned int)rfcmMask[0];
		hdPtr->rfcmMask[1]=(unsigned int)rfcmMask[1];
		

		/* 
		   Filled by Eventd
		hdPtr->gpsSubTime;
		hdPtr->calibStatus;
		*/

		if(printToScreen && verbosity) 
		    printf("Event:\t%lu\nSec:\t%ld\nMicrosec:\t%ld\nTrigTime:\t%lu\n",hdPtr->eventNumber,hdPtr->unixTime,hdPtr->unixTimeUs,turfioPtr->trigTime);


		// Save data
		if(writeData || writeScalers || writeFullHk){
		    if(useAltDir) {
			if(!madeAltDir || 
			   hdPtr->eventNumber%MAX_EVENTS_PER_DIR==0) 
			    makeSubAltDir();
			writeEventAndMakeLink(subAltOutputdir,acqdEventLinkDir,&theEvent);	
		    }
		    else 
			writeEventAndMakeLink(acqdEventDir,acqdEventLinkDir,&theEvent);


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
		    
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"readTurfEventData -- end %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"13 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
		    


		}
		//Insert stats call here
		if(printStatistics) calculateStatistics();
	    }
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"Wrote data %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"14 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif	    

	    // Clear boards
	    for(surf=0;surf<numSurfs;++surf)
		if (setSurfControl(surfHandles[surf], SurfClearEvent) != ApiSuccess)
		    printf("  failed to send clear event pulse on SURF %d.\n",surfIndex[surf]) ;
	    
	    if(!surfonly)
		if (setTurfControl(turfioHandle,TurfClearEvent) != ApiSuccess)
		    printf("  failed to send clear event pulse on TURFIO.\n") ;
	    
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
//	    fprintf(stderr,"Sent Clear -- start %ld s, %ld ms\n",timeStruct2.tv_sec,timeStruct2.tv_usec);
	    fprintf(timeFile,"15 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif	    
//	if(selftrig) sleep (2) ;
	}  /* closing master while loop. */
    } while(currentState==PROG_STATE_INIT);


    if(tryToUseBarMap) unsetBarMap(surfHandles,turfioHandle);
    // Clean up
    for(surf=0;surf<numSurfs;surf++)
	PlxPciDeviceClose(surfHandles[surf] );
    
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
	case LTrig :
	    string = "LTrig" ;
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
	case TurfClearEvent :
	    string = "TurfClearEvent" ;
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
    U32 locTrig    = 0000400000 ;  /* local software trigger for a SURF */
    U32 rdWrFlag   = 0000004000 ;  /* rd/_wr flag */
    U32 dataHkFlag = 0000040000 ;  /* dt/_hk flag */
    U32 dacLoad    = 0004000000 ;
    U32 intEnable    = 0040000000 ;
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
	case LTrig  : gpioVal = baseVal | locTrig  ; break ;
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
    if (verbosity && printToScreen) 
	printf(" setSurfControl: action %s, gpio= %o\n", 
	       surfControlActionAsString(action), gpioVal) ; 

    if ((rc = PlxRegisterWrite(surfHandle, PCI9030_GP_IO_CTRL, gpioVal ))
	!= ApiSuccess) {
	syslog(LOG_ERR,"setSurfControl: failed to set GPIO to %o.\n", gpioVal);
	if(printToScreen)
	    fprintf(stderr,"setSurfControl: failed to set GPIO to %o.\n", gpioVal) ; 
	return rc ;
    }
    if (LTrig < action && action < DACLoad) return rc ; 
    //Reset GPIO to base val
    return PlxRegisterWrite(surfHandle, PCI9030_GP_IO_CTRL, baseVal ) ;

}


PlxReturnCode_t setTurfControl(PlxHandle_t turfioHandle, TurfControlAction_t action) {

    //These numbers are octal
    U32 baseClr          = 0022222222 ;
    U32 rprgTurf         = 0000000004 ;
    U32 clearEvent       = 0000000040 ; 
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
	case TurfClearEvent : gpioVal = baseVal | clearEvent ; break ; 
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

    if (verbosity && printToScreen) 
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
    status += configLoad ("Acqd.config","debug") ;
    status += configLoad ("Acqd.config","trigger") ;
    status += configLoad ("Acqd.config","thresholds") ;
    status += configLoad ("Acqd.config","acqd") ;
//    printf("Debug rc1\n");
    if(status == CONFIG_E_OK) {
	useInterrupts=kvpGetInt("useInterrupts",0);
	firmwareVersion=kvpGetInt("firmwareVersion",2);
	tempString=kvpGetString("acqdPidFile");
	if(tempString) {
	    strncpy(acqdPidFile,tempString,FILENAME_MAX);
	    writePidFile(acqdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get acqdPidFile");
	    fprintf(stderr,"Couldn't get acqdPidFile\n");
	}
	tempString=kvpGetString ("acqdEventDir");
	if(tempString) {
	   strncpy(acqdEventDir,tempString,FILENAME_MAX-1);	   
	   strncpy(acqdEventLinkDir,tempString,FILENAME_MAX-1);	   
	   strcat(acqdEventLinkDir,"/link");
	   makeDirectories(acqdEventLinkDir);
	}
	else
	    fprintf(stderr,"Couldn't fetch acqdEventDir\n");
//	printf("%s\n",acqdEventDir);
	tempString=kvpGetString ("lastEventNumberFile");
	if(tempString)
	    strncpy(lastEventNumberFile,tempString,FILENAME_MAX-1);
	else
	    fprintf(stderr,"Coudn't fetch lastEventNumberFile\n");
	tempString=kvpGetString ("scalerOutputDir");
	if(tempString) {
	    strncpy(scalerOutputDir,tempString,FILENAME_MAX-1);
	    makeDirectories(scalerOutputDir);
	}
	else
	    fprintf(stderr,"Coudn't fetch scalerOutputDir\n");
	tempString=kvpGetString ("hkOutputDir");
	if(tempString) {
	    strncpy(hkOutputDir,tempString,FILENAME_MAX-1);
	    makeDirectories(hkOutputDir);
	}
	else
	    fprintf(stderr,"Coudn't fetch hkOutputDir\n");
//	printf("Debug rc2\n");
	printToScreen=kvpGetInt("printToScreen",0);
	if(addedVerbosity && printToScreen==0) {
	    printToScreen=1;
	    addedVerbosity--;
	}

	tempString=kvpGetString("baseHouseTelemDir");
	if(tempString) {
	    strncpy(surfHkTelemDir,tempString,FILENAME_MAX-1);
	    strncpy(turfHkTelemDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error baseHouseTelemDir");
	    fprintf(stderr,"Error baseHouseTelemDir\n");
	}
	tempString=kvpGetString("surfHkTelemSubDir");
	if(tempString) {
	    sprintf(surfHkTelemDir,"%s/%s",surfHkTelemDir,tempString);
	    sprintf(surfHkTelemLinkDir,"%s/link",surfHkTelemDir);
	    makeDirectories(surfHkTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting surfHkTelemSubDir");
	    fprintf(stderr,"Error getting surfHkTelemSubDir\n");
	}
	tempString=kvpGetString("turfHkTelemSubDir");
	if(tempString) {
	    sprintf(turfHkTelemDir,"%s/%s",turfHkTelemDir,tempString);
	    sprintf(turfHkTelemLinkDir,"%s/link",turfHkTelemDir);
	    makeDirectories(turfHkTelemLinkDir);
//	    printf("TURF Dir %s\n",turfHkTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting turfHkTelemSubDir");
	    fprintf(stderr,"Error getting turfHkTelemSubDir\n");
	}

	tempString=kvpGetString("mainDataDisk");
	if(tempString) {
	    strncpy(surfHkArchiveDir,tempString,FILENAME_MAX-1);
	    strncpy(turfHkArchiveDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error getting mainDataDisk");
	    fprintf(stderr,"Error getting mainDataDisk\n");
	}
	tempString=kvpGetString("usbDataDiskLink");
	if(tempString) {
	    strncpy(surfHkUSBArchiveDir,tempString,FILENAME_MAX-1);
	    strncpy(turfHkUSBArchiveDir,tempString,FILENAME_MAX-1);

	}
	else {
	    syslog(LOG_ERR,"Error getting usbDataDiskLink");
	    fprintf(stderr,"Error getting usbDataDiskLink\n");
	}
	    
	 
	useUSBDisks=kvpGetInt("useUSBDisks",0);
   
	tempString=kvpGetString("baseHouseArchiveDir");
	if(tempString) {
	    sprintf(surfHkArchiveDir,"%s/%s/",surfHkArchiveDir,tempString);
	    sprintf(turfHkArchiveDir,"%s/%s/",turfHkArchiveDir,tempString);
	    sprintf(surfHkUSBArchiveDir,"%s/%s/",surfHkUSBArchiveDir,tempString);
	    sprintf(turfHkUSBArchiveDir,"%s/%s/",turfHkUSBArchiveDir,tempString);
	}
	else {
	    syslog(LOG_ERR,"Error getting baseHouseArchiveDir");
	    fprintf(stderr,"Error getting baseHouseArchiveDir\n");
	}	    
	tempString=kvpGetString("surfHkArchiveSubDir");
	if(tempString) {
	    strcat(surfHkArchiveDir,tempString);
	    strcat(surfHkUSBArchiveDir,tempString);
	    makeDirectories(surfHkArchiveDir);
	    if(useUSBDisks) makeDirectories(surfHkUSBArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting surfHkArchiveSubDir");
	    fprintf(stderr,"Error getting surfHkArchiveSubDir");
	}	    
	tempString=kvpGetString("turfHkArchiveSubDir");
	if(tempString) {
	    strcat(turfHkArchiveDir,tempString);
	    strcat(turfHkUSBArchiveDir,tempString);
	    makeDirectories(turfHkArchiveDir);
	    if(useUSBDisks) makeDirectories(turfHkUSBArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting turfHkArchiveSubDir");
	    fprintf(stderr,"Error getting turfHkArchiveSubDir");
	}

	surfHkPeriod=kvpGetInt("surfHkPeriod",1);
	surfHkTelemEvery=kvpGetInt("surfHkTelemEvery",1);
	turfRateTelemEvery=kvpGetInt("turfRateTelemEvery",1);
	turfRateTelemInterval=kvpGetInt("turfRateTelemInterval",1);
//	printf("HK surfPeriod %d\nturfRateTelemEvery %d\nturfRateTelemInterval %d\n",surfHkPeriod,turfRateTelemEvery,turfRateTelemInterval);
	dontWaitForEvtF=kvpGetInt("dontWaitForEvtF",0);
	dontWaitForLabF=kvpGetInt("dontWaitForLabF",0);
	writeOutC3p0Nums=kvpGetInt("writeOutC3p0Nums",0);
	sendSoftTrigger=kvpGetInt("sendSoftTrigger",0);
	softTrigSleepPeriod=kvpGetInt("softTrigSleepPeriod",0);
	reprogramTurf=kvpGetInt("reprogramTurf",0);
	tryToUseBarMap=kvpGetInt("tryToUseBarMap",0);
//	printf("dontWaitForEvtF: %d\n",dontWaitForEvtF);
	standAloneMode=kvpGetInt("standAloneMode",0);
	if(!writeData)
	    writeData=kvpGetInt("writeData",1);
	writeScalers=kvpGetInt("writeScalers",1);
	writeFullHk=kvpGetInt("writeFullHk",1);
	justWriteHeader=kvpGetInt("justWriteHeader",0);
	if(!standAloneMode) 
	    verbosity=kvpGetInt("verbosity",0);
	else
	    verbosity=kvpGetInt("verbosity",0)+addedVerbosity;
	printStatistics=kvpGetInt("printStatistics",0);
	pps1TrigFlag=kvpGetInt("enablePPS1Trigger",1);
	pps2TrigFlag=kvpGetInt("enablePPS2Trigger",1);
	doSlowDacCycle=kvpGetInt("doSlowDacCycle",0);
	doGlobalDacCycle=kvpGetInt("doGlobalDacCycle",0);
	setGlobalThreshold=kvpGetInt("setGlobalThreshold",0);
	globalThreshold=kvpGetInt("globalThreshold",0);
	dacPGain=kvpGetFloat("dacPGain",0);
	dacIGain=kvpGetFloat("dacIGain",0);
	dacDGain=kvpGetFloat("dacDGain",0);
	dacIMin=kvpGetInt("dacIMin",0);
	dacIMax=kvpGetInt("dacIMax",0);
	enableChanServo=kvpGetInt("enableChanServo",1);
	if(doGlobalDacCycle || doSlowDacCycle)
	    enableChanServo=0;
	pidGoal=kvpGetInt("pidGoal",2000);
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
	kvpStatus=kvpGetIntArray("rfcmMask",rfcmMask,&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(rfcmMask): %s",
		   kvpErrorString(kvpStatus));
	    fprintf(stderr,"kvpGetIntArray(rfcmMask): %s\n",
			kvpErrorString(kvpStatus));
	}
	

	
	if(printToScreen) printf("Print to screen flag is %d\n",printToScreen);

	turfioPos.bus=kvpGetInt("turfioBus",3); //in seconds
	turfioPos.slot=kvpGetInt("turfioSlot",10); //in seconds
	tempNum=18;
	kvpStatus=kvpGetIntArray("surfAntTrigMasks",&surfAntTrigMasks[0][0],&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(surfAntTrigMasks): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(surfAntTrigMasks): %s\n",
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
	    }
	}
    	

	
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Acqd.config: %s\n",eString);
//	if(printToScreen)
	fprintf(stderr,"Error reading Acqd.config: %s\n",eString);
	    
    }

    if(printToScreen) {
	printf("Read Config File\n");
	printf("useInterrupts %d\n",useInterrupts);
	printf("writeData %d",writeData);
	if(writeData) {
	    if(useAltDir) 
		printf("\t--\t%s",altOutputdir);
	    else
		printf("\t--\t%s",acqdEventDir);
	}
	printf("\n");
	printf("writeFullHk %d\n",writeFullHk);
	if(writeFullHk) {
	    printf("\t surfHkArchiveDir -- %s\n",surfHkArchiveDir);
	    printf("\t turfHkArchiveDir -- %s\n",turfHkArchiveDir);
	}
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

// Prepare SURF boards
    for(i=0;i<numSurfs;++i){
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

    // Prepare TURFIO board
  
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
	syslog(LOG_ERR,"Failed to send clear pulse on TURF %d.\n",i) ;
	if(printToScreen)
	    printf("  failed to send clear pulse on TURF %d.\n",i) ;
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
		case 'c': doSlowDacCycle = TRUE; break;
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
	    buf = (dac<<16) + thresholdArray[surfIndex[surf]-1][dac] ;
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
		surfAntTrigMasks[surfIndex[surf]-1][dac-N_RFTRIG];	    
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



PlxReturnCode_t readPlxDataWord(PlxHandle_t handle, unsigned int *dataInt)
{
    return PlxBusIopRead(handle,IopSpace0,0x0,TRUE, dataInt, 4, BitSize32);
}


PlxReturnCode_t readPlxDataShort(PlxHandle_t handle, unsigned short *dataWord)
{
    return PlxBusIopRead(handle,IopSpace0,0x0,TRUE, dataWord, 2, BitSize16);
}



void calculateStatistics() {

//    static float mean_dt=0;
    static float mean_rms[MAX_SURFS][N_CHIP][N_CHAN];
    static double chanMean[MAX_SURFS][N_CHIP][N_CHAN];
    static double chanMeanSqd[MAX_SURFS][N_CHIP][N_CHAN];
    static unsigned long chanNumReads[MAX_SURFS][N_CHIP][N_CHAN];
    static int firstTime=1;
    static unsigned long numChipEvents[N_CHIP];

    int surf,chan,chip,samp;

    if(firstTime) {
	for(surf=0;surf<numSurfs;++surf)
	    for(chip=0;chip<N_CHIP;++chip) 
		for(chan=0;chan<N_CHAN;++chan) {
		    mean_rms[surf][chip][chan]=0;
		    chanMean[surf][chip][chan]=0;
		    chanMeanSqd[surf][chip][chan]=0;
		    chanNumReads[surf][chip][chan]=0;
		}
	firstTime=0;
    }
  


	// Calculate mean,rms,std dev
	for(surf=0;surf<numSurfs;++surf) {
	    for(chan=0;chan<N_CHAN;++chan){
		double sum[N_CHIP]={0};
		double sumSqd[N_CHIP]={0};
		int tempNum[N_CHIP]={0};
		for(samp=0;samp<N_SAMP;++samp) {
		    int tempChip=theEvent.body.channel[chan+surf*CHANNELS_PER_SURF].data[samp]>>14;
		    int tempData=theEvent.body.channel[chan+surf*CHANNELS_PER_SURF].data[samp] & DATA_MASK;
		    if(tempData) {
/* 	    printf("%d\t%d\n",tempChip,tempData); */
			sum[tempChip]+=tempData;
			sumSqd[tempChip]+=tempData*tempData;
			tempNum[tempChip]++;
		    }
		}

		for(chip=0;chip<N_CHIP;chip++) {	  
		    if(tempNum[chip]) {
			float tempRMS=0;
			tempRMS=sqrt(sumSqd[chip]/(float)tempNum[chip]);
			float temp1=mean_rms[surf][chip][chan]*((float)(numChipEvents[chip]))/(float)(numChipEvents[chip]+1);
			float temp2=tempRMS/(float)(numChipEvents[chip]+1);	  
			mean_rms[surf][chip][chan]=temp1+temp2;
			numChipEvents[chip]++;

			/* Ryan's statistics */	    
			double tempMean=sum[chip];///(double)tempNum[chip];
			double tempMeanSqd=sumSqd[chip];///(double)tempNum[chip];
/* 	    printf("%f\t%f\n",tempMean,tempMeanSqd); */
			chanMean[surf][chip][chan]=(chanMean[surf][chip][chan]*((double)chanNumReads[surf][chip][chan]/(double)(tempNum[chip]+chanNumReads[surf][chip][chan]))) +
			    (tempMean/(double)(tempNum[chip]+chanNumReads[surf][chip][chan]));
			chanMeanSqd[surf][chip][chan]=(chanMeanSqd[surf][chip][chan]*((double)chanNumReads[surf][chip][chan]/(double)(tempNum[chip]+chanNumReads[surf][chip][chan]))) +
			    (tempMeanSqd/(double)(tempNum[chip]+chanNumReads[surf][chip][chan]));
			chanNumReads[surf][chip][chan]+=tempNum[chip];	    
		    }
		}
	    }
	}
  
	// Display Mean
	if (verbosity && printStatistics) {
	    for(surf=0;surf<numSurfs;++surf){
		printf("Board %d Mean, After Event %d (Software)\n",surfIndex[surf],doingEvent);
		for(chip=0;chip<N_CHIP;++chip){
		    for(chan=0;chan<N_CHAN;++chan)
			printf("%.2f ",chanMean[surf][chip][chan]);
		    printf("\n");
		}
	    }

	    // Display Std Dev
	    for(surf=0;surf<numSurfs;++surf){
		printf("Board %d Std Dev., After Event %d (Software)\n",surfIndex[surf],doingEvent);
		for(chip=0;chip<N_CHIP;++chip){
		    for(chan=0;chan<N_CHAN;++chan)
			printf("%.2f ",sqrt(chanMeanSqd[surf][chip][chan]-chanMean[surf][chip][chan]*chanMean[surf][chip][chan]));
		    printf("\n");
		}
	    }
	}

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
	sprintf(theFilename,"%s/surfhk_%ld_%ld.dat.gz",surfHkArchiveDir,
		theSurfHk.unixTime,theSurfHk.unixTimeUs);
	retVal+=writeSurfHk(&theSurfHk,theFilename);
	if(useUSBDisks) {
	    sprintf(theFilename,"%s/surfhk_%ld_%ld.dat.gz",surfHkUSBArchiveDir,
		    theSurfHk.unixTime,theSurfHk.unixTimeUs);
	    retVal+=writeSurfHk(&theSurfHk,theFilename);
	}
    }
    if(dataOrTelem!=1) {
	// Write data for Telem
	sprintf(theFilename,"%s/surfhk_%ld_%ld.dat.gz",surfHkTelemDir,
		theSurfHk.unixTime,theSurfHk.unixTimeUs);
	retVal+=writeSurfHk(&theSurfHk,theFilename);
	makeLink(theFilename,surfHkTelemLinkDir);
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
//	sprintf(theFilename,"%s/turfhk_%ld_%ld.dat.gz",turfHkArchiveDir,
//		turfRates.unixTime,turfRates.unixTimeUs);
//	retVal+=writeTurfRate(&turfRates,theFilename);
	retVal+=bufferedTurfHkWrite(&turfRates,turfHkArchiveDir);
	if(useUSBDisks) {
	    sprintf(theFilename,"%s/turfhk_%ld_%ld.dat.gz",turfHkUSBArchiveDir,
		    turfRates.unixTime,turfRates.unixTimeUs);
	    retVal+=writeTurfRate(&turfRates,theFilename);
	}
    }
    if(dataOrTelem!=1) {
	// Write data for Telem
	sprintf(theFilename,"%s/turfhk_%ld_%ld.dat.gz",turfHkTelemDir,
		turfRates.unixTime,turfRates.unixTimeUs);
	writeTurfRate(&turfRates,theFilename);
	retVal+=makeLink(theFilename,turfHkTelemLinkDir);
    }

    return retVal;
}

int bufferedTurfHkWrite(TurfRateStruct_t *turfPtr, char *baseDir) 
{    
    int retVal=0;
    char *tempDir;
    static int firstTime=1;
    static int currentFileCount=0;
    static int currentDirCount=0;
    static char currentDir[FILENAME_MAX];
    static gzFile turfHkFile;
    if(firstTime) {
	tempDir=getCurrentHkDir(baseDir,turfPtr->unixTime);
	strncpy(currentDir,tempDir,FILENAME_MAX-1);
	free(tempDir);
	turfHkFile=getCurrentZippedHkFile(currentDir,"turfhk",turfPtr->unixTime);
	firstTime=0;
    }
    
//    printf("gzFile %d\n",(int)turfHkFile);
    //Check if we need a new file or directory
    if(currentFileCount>=HK_PER_FILE) {
	gzclose(turfHkFile);
	currentDirCount++;

	if(currentDirCount>=HK_FILES_PER_DIR) {
	    tempDir=getCurrentHkDir(baseDir,turfPtr->unixTime);
	    strncpy(currentDir,tempDir,FILENAME_MAX-1);
	    free(tempDir);
	    currentDirCount=0;
	}
	turfHkFile=getCurrentZippedHkFile(currentDir,"turfhk",turfPtr->unixTime);
	currentFileCount=0;
    }
    currentFileCount++;
    retVal=gzwrite(turfHkFile,turfPtr,sizeof(TurfRateStruct_t));
    if(retVal<0) {
	printf("Error writing to file (write %d) -- %s (%d)\n",currentFileCount,
	       gzerror(turfHkFile,&retVal),retVal);
    }
    //Should do some checking here
//    if(currentFileCount%100==0)
    gzflush(turfHkFile,Z_SYNC_FLUSH);    
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
    fillGenericHeader(hdPtr,PACKET_HD,sizeof(AnitaEventHeader_t));
    fillGenericHeader(theBody,PACKET_WV,sizeof(AnitaEventBody_t));


    if(justWriteHeader==0 && writeData) {
	sprintf(theFilename,"%s/ev_%ld.dat",theEventDir,
		theEventPtr->header.eventNumber);
	if(!oldStyleFiles) {
	    if(!compressWavefile) 
		retVal=writeBody(theBody,theFilename);  
	    else {
		strcat(theFilename,".gz");
		retVal=writeZippedBody(theBody,theFilename);
	    }
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

    if(writeScalers) {
	sprintf(theFilename,"%s/scale_3%ld.dat",scalerOutputDir,
		theEventPtr->header.eventNumber);
	FILE *scalerFile;
	int n;
	if ((scalerFile=fopen(theFilename, "wb")) == NULL) { 
	    printf("Failed to open scaler file, %s\n", theFilename) ;
	    return ;
	}
	    
	if ((n=fwrite(&theScalers, sizeof(SimpleScalerStruct_t),1,scalerFile))!=1)
	    printf("Failed to write all scaler data. wrote only %d.\n", n) ;
	    fclose(scalerFile);
    }

//    if(writeFullHk && standAloneMode) {
//	sprintf(theFilename,"%s/surfhk_%d.dat",hkOutputDir,hkNumber);
//	writeSurfHk(&theSurfHk,theFilename);
//    }

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
    if(verbosity && printToScreen) 
	printf("Triggered, event %d (by software counter).\n",doingEvent);

  	
    //Loop over SURFs and read out data
    for(surf=0;surf<numSurfs;surf++){  
//	sleep(1);
	if(printToScreen &&verbosity) {
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
	if(tryToUseBarMap) {
	    dataInt=*(barMapAddr[surf]);
	}
	else if ((rc=readPlxDataWord(surfHandles[surf],&dataInt))!= ApiSuccess) {
	    status=ACQD_E_PLXBUSREAD;
	    syslog(LOG_ERR,"Failed to prime pipe SURF %d (rc = %d)",surfIndex[surf],rc);
	    if(printToScreen) 
		printf("Failed to prime pipe SURF %d (rc = %d)\n", surfIndex[surf],rc) ;
	}

	
	for(chan=0;chan<N_CHAN; chan++) {
	    for (samp=0 ; samp<N_SAMP_EFF ; samp++) {		
		if(tryToUseBarMap) 
		    dataInt=*(barMapAddr[surf]);
		else if(readPlxDataWord(surfHandles[surf],&dataInt)
			!= ApiSuccess) {
		    status=ACQD_E_PLXBUSREAD;			
		    syslog(LOG_ERR,"Failed to read data: SURF %d, chan %d, samp %d",surfIndex[surf],chan,samp);
		    if(printToScreen) 
			printf("Failed to read IO. surf=%d, chn=%d sca=%d\n", surfIndex[surf], chan, samp) ;
		}
		if(printToScreen && verbosity>2) {
		    printf("SURF %d (%d), CHIP %d, CHN %d, SCA %d: %x (HITBUS=%d) %d\n",surfIndex[surf],((dataInt&0xf0000)>>16),((dataInt&0x00c00000)>> 22),chan,samp,dataInt,((dataInt& 0x1000)>>12),(dataInt&DATA_MASK));
		}
		labData[surf][chan][samp]=dataInt;
	    }
	}
	for(chan=0;chan<N_CHAN; chan++) {
	    for (samp=N_SAMP_EFF ; samp<N_SAMP ; samp++) {		
		if(tryToUseBarMap) 
		    dataInt=*(barMapAddr[surf]);
		else if(readPlxDataWord(surfHandles[surf],&dataInt)
			!= ApiSuccess) {
		    status=ACQD_E_PLXBUSREAD;			
		    syslog(LOG_ERR,"Failed to read data: SURF %d, chan %d, samp %d",surfIndex[surf],chan,samp);
		    if(printToScreen) 
			printf("Failed to read IO. surf=%d, chn=%d sca=%d\n", surfIndex[surf], chan, samp) ;
		}    
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
	if(printToScreen &&verbosity) {
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
	if(tryToUseBarMap) {
	    headerWord=*(barMapAddr[surf]);
	}
	else if ((rc=readPlxDataWord(surfHandles[surf],&headerWord))!= ApiSuccess) {
	    status=ACQD_E_PLXBUSREAD;
	    syslog(LOG_ERR,"Failed to read header word SURF %d (rc = %d)",surfIndex[surf],rc);
	    if(printToScreen) 
		printf("Failed to read header word %d (rc = %d)\n", surfIndex[surf],rc) ;
	}

	
	//Now read first 256 samples per SURF
	for(chan=0;chan<N_CHAN; chan++) {
	    for (readCount=samp=0 ; readCount<N_SAMP_EFF/2 ; readCount++) {		
		if(tryToUseBarMap) 
		    dataInt=*(barMapAddr[surf]);
		else if(readPlxDataWord(surfHandles[surf],&dataInt)
			!= ApiSuccess) {
		    status=ACQD_E_PLXBUSREAD;			
		    syslog(LOG_ERR,"Failed to read data: SURF %d, chan %d, readCount %d",surfIndex[surf],chan,readCount);
		    if(printToScreen) 
			printf("Failed to read IO. surf=%d, chn=%d read=%d\n", surfIndex[surf], chan, readCount) ;
		}
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
		if(tryToUseBarMap) 
		    dataInt=*(barMapAddr[surf]);
		else if(readPlxDataWord(surfHandles[surf],&dataInt)
			!= ApiSuccess) {
		    status=ACQD_E_PLXBUSREAD;			
		    syslog(LOG_ERR,"Failed to read data: SURF %d, chan %d, samp %d",surfIndex[surf],chan,samp);
		    if(printToScreen) 
			printf("Failed to read IO. surf=%d, chn=%d sca=%d\n", surfIndex[surf], chan, samp) ;
		} 
		
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
	
	if(tryToUseBarMap) 
	    dataInt=*(barMapAddr[surf]);
	else if(readPlxDataWord(surfHandles[surf],&dataInt)
		!= ApiSuccess) {
	    status=ACQD_E_PLXBUSREAD;			
	    syslog(LOG_ERR,"Failed to read last word: SURF %d",surfIndex[surf]);
	    if(printToScreen) 
		printf("Failed to read last word surf=%d\n", surfIndex[surf]) ;
	} 
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
        
    for(surf=0;surf<numSurfs;surf++){  

	//Set to read house keeping
	//Send DTRead first to have a transition on DT/HK line
	rc=setSurfControl(surfHandles[surf],DTRead);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Failed to set DTRead on SURF %d (rc = %d)",surfIndex[surf],rc);
	    if(printToScreen)
		fprintf(stderr,"Failed to set RDMode on SURF %d (rc = %d)\n",surfIndex[surf],rc);
	}
	rc=setSurfControl(surfHandles[surf],RDMode);
	if(rc!=ApiSuccess) {
	    syslog(LOG_ERR,"Failed to set RDMode on SURF %d",surf);
	    if(printToScreen)
		fprintf(stderr,"Failed to set RDMode on SURF %d\n",surf);
	}
	
	//First read the scaler data
	for(rfChan=0;rfChan<N_RFTRIG;rfChan++){
	    if(tryToUseBarMap) {
		dataInt=*(barMapAddr[surf]);
	    }	    
	    else if ((rc=readPlxDataWord(surfHandles[surf],&dataInt))!= ApiSuccess) {
		status=ACQD_E_PLXBUSREAD;
		syslog(LOG_ERR,"Failed to read SURF %d, Scaler %d (rc = %d)",surfIndex[surf],rfChan,rc);
		if(printToScreen) 
		    printf("Failed to read SURF %d, Scaler %d (rc = %d)\n",surfIndex[surf],rfChan,rc);
	    }
	    theSurfHk.scaler[surf][rfChan]=dataInt&0xffff;
	    if(printToScreen && verbosity>1) 
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
	    if(tryToUseBarMap) {
		dataInt=*(barMapAddr[surf]);
	    }	    
	    else if ((rc=readPlxDataWord(surfHandles[surf],&dataInt))!= ApiSuccess) {
		status=ACQD_E_PLXBUSREAD;
		syslog(LOG_ERR,"Failed to read SURF %d, Threshold %d (rc = %d)",surfIndex[surf],rfChan,rc);
		if(printToScreen) 
		    printf("Failed to read SURF %d, Threshold %d (rc = %d)\n",surfIndex[surf],rfChan,rc);
	    }
	    theSurfHk.threshold[surf][rfChan]=dataInt&0xffff;
	    if(printToScreen && verbosity>1) 
		printf("Surf %d, Threshold %d == %d\n",surfIndex[surf],rfChan,theSurfHk.threshold[surf][rfChan]);
	    //Should check if it is the same or not
	    if(theSurfHk.upperWords[surf]!=GetUpper16(dataInt)) {
		theSurfHk.errorFlag|=(1>>surf);
	    }
	}
	
	//Lastly read the RF Power Data
	for(rfChan=0;rfChan<N_RFCHAN;rfChan++){
	    if(tryToUseBarMap) {
		dataInt=*(barMapAddr[surf]);
	    }	    
	    else if ((rc=readPlxDataWord(surfHandles[surf],&dataInt))!= ApiSuccess) {
		status=ACQD_E_PLXBUSREAD;
		syslog(LOG_ERR,"Failed to read SURF %d, RF Power %d (rc = %d)",surfIndex[surf],rfChan,rc);
		if(printToScreen) 
		    printf("Failed to read SURF %d, RF Power %d (rc = %d)\n",surfIndex[surf],rfChan,rc);
	    }
	    theSurfHk.rfPower[surf][rfChan]=dataInt&0xfff;
	    if(printToScreen && verbosity>1) 
		printf("Surf %d, RF Power %d (%d) == %d\n",surfIndex[surf],rfChan,((dataInt&0xe000)>>13),(theSurfHk.rfPower[surf][rfChan]&0xfff));
	    if(theSurfHk.upperWords[surf]!=GetUpper16(dataInt)) {
		theSurfHk.errorFlag|=(1>>surf);
	    }

	}
    }
    return status;
}



AcqdErrorCode_t readTurfEventData(PlxHandle_t turfioHandle)
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
//    if(tryToUseBarMap) {
//	dataWord=*(turfBarMap);
//    }
//    else 
// Do we need prime read (rjn 20/05/06)

//    if (readPlxDataShort(turfioHandle,&dataWord)!= ApiSuccess) {
//	status=ACQD_E_PLXBUSREAD;
//	syslog(LOG_ERR,"Failed to prime pipe TURFIO");
//	if(printToScreen) 
//	    printf("Failed to prime pipe TURFIO\n") ;
//  }
    
    //Read out 144 words
    for(wordNum=0;wordNum<144;wordNum++) {
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
		    turfioPtr->trigType=dataChar; break;
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
		    turfioPtr->ppsNum=dataLong; break;
		case 17:
		    turfioPtr->ppsNum+=(dataLong<<8); break;
		case 18:
		    turfioPtr->ppsNum+=(dataLong<<16); break;
		case 19:
		    turfioPtr->ppsNum+=(dataLong<<24); break;
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
	else if(wordNum<144) {
	    endPat.test[wordNum-136]=dataChar; 
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
	       hdPtr->eventNumber,hdPtr->turfUpperWord,turfioPtr->trigNum,turfioPtr->trigType,turfioPtr->trigTime,turfioPtr->ppsNum,turfioPtr->c3poNum,turfioPtr->l3Type1Count);
    }
	
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

PlxReturnCode_t writeRFCMMask(PlxHandle_t turfioHandle) 
/*!
  Remember that 0 in the RFCM mask means on
*/
{
    PlxReturnCode_t rc;
    U32 gpio ;  
    int i=0,j=0,chanBit=1;

    rc=setTurfControl(turfioHandle,RFCBit);
    if(rc!=ApiSuccess) {		
	syslog(LOG_ERR,"Failed to set 1st RFCBit %d",rc);
	if(printToScreen)			    
	    printf("Failed to set 1st RFCBit %d.\n",rc) ;
    }	

    //Now send bit pattern to TURF using RFCBit and RFCClk
    while(i<40) {
	if (i==32) j=chanBit=1 ;
	
//	printf("Debug RFCM: i=%d j=%d chn_bit=%d mask[j]=%d\twith and %d\n",i,j,chanBit,rfcmMask[j],(rfcmMask[j]&chanBit));
	if(rfcmMask[j]&chanBit) rc=setTurfControl(turfioHandle,RFCBit);
	else rc=setTurfControl(turfioHandle,RFCClk);
	if (rc != ApiSuccess) {
	    syslog(LOG_ERR,"Failed to send RFC bit %d to TURF, %x %x",
		   i,rfcmMask[1],rfcmMask[0]);
	    if(printToScreen)
		fprintf(stderr,"Failed to send RFC bit %d to TURF, %x %x\n",
			i,rfcmMask[1],rfcmMask[0]);
	}	
	chanBit<<=1;
	i++;
    }

  /* check GPIO8 is set or not. */
    if((gpio=PlxRegisterRead(turfioHandle, PCI9030_GP_IO_CTRL, &rc )) 
       & 0400000000) {
	syslog(LOG_INFO,"writeRFCMMask: GPIO8 on, so mask should be set to %x%x.\n", rfcmMask[1],rfcmMask[0]) ;
	if(printToScreen) 
	    printf("writeRFCMMask: GPIO8 on, so mask should be set to %x%x.\n", rfcmMask[1], rfcmMask[0]) ;
    }   
    else {
	syslog(LOG_ERR,"writeRFCMMask: GPIO 8 not on: GPIO=%o",gpio);
	if(printToScreen) 
	    printf("writeRFCMMask: GPIO 8 not on: GPIO=%o\n",gpio);
    }
    return ApiSuccess;
    
}


int updateThresholdsUsingPID() {
    static int avgCount=0;
    int surf,dac,error,value,change;
    float pTerm, dTerm, iTerm;
    avgCount++;
    for(surf=0;surf<numSurfs;surf++) {
	for(dac=0;dac<N_RFTRIG;dac++) {
	    avgScalerData[surf][dac]+=theSurfHk.scaler[surf][dac];
	    if(avgCount==pidAverage) {
		value=avgScalerData[surf][dac]/avgCount;
		avgScalerData[surf][dac]=0;
		error=pidGoal-value;
//		printf("%d %d %d %d\n",thePids[surf][dac].iState,
//		       avgScalerData[surf][dac],value,error);
	    
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
    if(avgCount==10) {
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
