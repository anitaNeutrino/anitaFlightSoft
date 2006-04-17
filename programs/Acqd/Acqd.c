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


//inline unsigned short byteSwapUnsignedShort(unsigned short a){
//    return (a>>8)+(a<<8);
//}


//Global Variables
BoardLocStruct_t turfioPos;
BoardLocStruct_t surfPos[MAX_SURFS];
int dacSurfs[MAX_SURFS];
int surfIndex[MAX_SURFS];
int dacChans[NUM_DAC_CHANS];
int printToScreen=0,standAloneMode=0;
int numSurfs=0,doingEvent=0,hkNumber=0;
unsigned short data_array[MAX_SURFS][N_CHAN][N_SAMP]; 
AnitaEventFull_t theEvent;
AnitaEventHeader_t *hdPtr;//=&(theEvent.header);
TurfioStruct_t *turfioPtr;//=&(hdPtr->turfio);


SimpleScalerStruct_t theScalers;
FullSurfHkStruct_t theHk;

//Temporary Global Variables
unsigned int labData[MAX_SURFS][N_CHAN][N_SAMP];
unsigned int avgScalerData[MAX_SURFS][N_RFTRIG];
//unsigned short threshData[MAX_SURFS][N_RFTRIG];
//unsigned short rfpwData[MAX_SURFS][N_RFCHAN];



//Configurable watchamacallits
/* Ports and directories */
char acqdEventDir[FILENAME_MAX];
char altOutputdir[FILENAME_MAX];
char acqdPidFile[FILENAME_MAX];
char acqdEventLinkDir[FILENAME_MAX];
char lastEventNumberFile[FILENAME_MAX];
char scalerOutputDir[FILENAME_MAX];
char hkOutputDir[FILENAME_MAX];

#define N_TMO 100    /* number of msec wait for Evt_f before timed out. */
#define N_FTMO 200    /* number of msec wait for Lab_f before timed out. */
//#define N_FTMO 2000    /* number of msec wait for Lab_f before timed out. */
//#define DAC_CHAN 4 /* might replace with N_CHIP at a later date */
#define EVTF_TIMEOUT 10000000 /* in microseconds */

int verbosity = 0 ; /* control debug print out. */
int addedVerbosity = 0 ; /* control debug print out. */
int oldStyleFiles = FALSE;
int useAltDir = FALSE;
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
unsigned int rfcmMask[2] = {0x10001000, 0x00008000} ;


//Test of BarMap
//U32 barMapDataWord[ACTIVE_SURFS];
__volatile__ int *barMapAddr[ACTIVE_SURFS];
__volatile__ int *turfBarMap;

int main(int argc, char **argv) {
    
    PlxHandle_t surfHandles[MAX_SURFS];
    PlxHandle_t turfioHandle;
    PlxDevLocation_t surfLocation[MAX_SURFS];
    PlxDevLocation_t turfioLocation;
    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    int retVal=0;
//    unsigned short  dataWord=0;
// Read only one chip per trigger. PM

    int i=0,tmo=0; 
    int numDevices, numEvents=0 ;
    int tmpGPIO;
    unsigned short dacVal=2200;
    int surf;
    
    unsigned short doingDacVal=1;//2100;
    struct timeval timeStruct;

    //Initialize handy pointers
    hdPtr=&(theEvent.header);
    turfioPtr=&(hdPtr->turfio);
   
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);

    retVal=readConfigFile();

    if(retVal!=CONFIG_E_OK) {
	syslog(LOG_ERR,"Error reading config file Acqd.config: bailing");
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
	if(pps1TrigFlag) trigMode+=TrigPPS1;
	if(pps2TrigFlag) trigMode+=TrigPPS2;
	// RJN debugging
//	setTurfControl(turfioHandle,SetTrigMode);

	//Write RFCM Mask
	writeRFCMMask(turfioHandle);
	
	theHk.globalThreshold=0;
	//Set Thresholds
	if(setGlobalThreshold) {
	    setGloablDACThreshold(surfHandles,globalThreshold);
	    theHk.globalThreshold=globalThreshold;
	    if(printToScreen) 
		printf("Setting Global Threshold %d\n",globalThreshold);
	}
	else {
	    setDACThresholds(surfHandles);
	}
	
	currentState=PROG_STATE_RUN;
	while (currentState==PROG_STATE_RUN) {
	    if(standAloneMode && (numEvents && numEvents<doingEvent)) {
		currentState=PROG_STATE_TERMINATE;
		break;
	    }
	    //Fill theEvent with zeros 
	    bzero(&theEvent, sizeof(theEvent)) ;
	    memset(&theHk,0,sizeof(FullSurfHkStruct_t));
	    if(setGlobalThreshold) 
		theHk.globalThreshold=globalThreshold;

	    if(doGlobalDacCycle) {		
//		if(dacVal<=500) dacVal=4095;
//		else if(dacVal>=4000) dacVal=0;
		dacVal=doingDacVal;
		theScalers.threshold=dacVal;
		if(printToScreen) 
		    printf("Setting Local Threshold %d\n",dacVal);
		setGloablDACThreshold(surfHandles,dacVal);	
		theHk.globalThreshold=dacVal;
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

	    // Wait for ready to read (EVT_RDY) 
	    tmo=0;	    
	    if(!dontWaitForEvtF) {
		if(verbosity && printToScreen) 
		    printf("Waiting for EVT_RDY on SURF %d\n",surfIndex[0]);

		do {
		    tmpGPIO=PlxRegisterRead(surfHandles[0], PCI9030_GP_IO_CTRL, &rc);
		    if(verbosity>3 && printToScreen) 
			printf("SURF %d GPIO: 0x%o %d\n",surfIndex[0],tmpGPIO,tmpGPIO);
		    usleep(1); /*GV removed, RJN put it back in as it completely abuses the CPU*/
		    tmo++;
		    if((tmo%1000)==0) {
			if(currentState!=PROG_STATE_RUN) 
			    break;
			//Time out not in self trigger mode
			if(tmo==EVTF_TIMEOUT) break;
		    }
		    
		    
		} while(!(tmpGPIO & EVT_RDY) && (selftrig?(tmo<N_TMO):1) );

		if(currentState!=PROG_STATE_RUN) continue;
		
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
	    

	    //Either have a trigger or are going ahead regardless
	    gettimeofday(&timeStruct,NULL);
	    status+=readSurfEventData(surfHandles);
	    if(verbosity && printToScreen) printf("Read SURF Labrador Data\n");

	    //For now I'll just read the HK data with the events.
	    //Later we will change this to do something more cleverer
	    status+=readSurfHkData(surfHandles);
	    if(verbosity && printToScreen) printf("Read SURF Housekeeping\n");
	    if(enableChanServo) {
		if(verbosity && printToScreen) 
		    printf("Will servo on channel scalers\n");
		
		if(updateThresholdsUsingPID())
		    setDACThresholds(surfHandles);
		
	    }


	    status+=readTurfEventData(turfioHandle);

	    if(verbosity && printToScreen) printf("Done reading\n");
	    //Error checking
	    if(status!=ACQD_E_OK) {
		//We have an error
		if(printToScreen) fprintf(stderr,"Error detected %d, during event %d",status,doingEvent);
		
		status=ACQD_E_OK;
	    }
	    else {
		hdPtr->gHdr.code=PACKET_HD;
		hdPtr->unixTime=timeStruct.tv_sec;
		hdPtr->unixTimeUs=timeStruct.tv_usec;
		hdPtr->eventNumber=getEventNumber();
		hdPtr->numChannels=CHANNELS_PER_SURF*numSurfs;
		hdPtr->numSamples=N_SAMP;
		if(printToScreen && verbosity) 
		    printf("Event:\t%d\nSec:\t%ld\nMicrosec:\t%ld\nTrigTime:\t%lu\n",hdPtr->eventNumber,hdPtr->unixTime,hdPtr->unixTimeUs,turfioPtr->trigTime);
		// Save data
		if(writeData || writeScalers || writeFullHk){
		    if(useAltDir) 
			writeEventAndMakeLink(altOutputdir,acqdEventLinkDir,&theEvent);	
		    else 
			writeEventAndMakeLink(acqdEventDir,acqdEventLinkDir,&theEvent);
		}
		//Insert stats call here
		if(printStatistics) calculateStatistics();
	    }
	    
	    // Clear boards
	    for(surf=0;surf<numSurfs;++surf)
		if (setSurfControl(surfHandles[surf], SurfClearEvent) != ApiSuccess)
		    printf("  failed to send clear event pulse on SURF %d.\n",surfIndex[surf]) ;
	    
	    if(!surfonly)
		if (setTurfControl(turfioHandle,TurfClearEvent) != ApiSuccess)
		    printf("  failed to send clear event pulse on TURFIO.\n") ;
	    
	    
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
	case AdvLAB :
	    string = "AdvLAB" ;
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
		hdPtr->surfMask|=(1<<surfNum);
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
	dontWaitForEvtF=kvpGetInt("dontWaitForEvtF",0);
	dontWaitForLabF=kvpGetInt("dontWaitForLabF",0);
	writeOutC3p0Nums=kvpGetInt("writeOutC3p0Nums",0);
	sendSoftTrigger=kvpGetInt("sendSoftTrigger",0);
	softTrigSleepPeriod=kvpGetInt("softTrigSleepPeriod",0);
	reprogramTurf=kvpGetInt("reprogramTurf",0);
	tryToUseBarMap=kvpGetInt("tryToUseBarMap",0);
//	printf("dontWaitForEvtF: %d\n",dontWaitForEvtF);
	standAloneMode=kvpGetInt("standAloneMode",0);
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
	
	if(printToScreen) printf("Print to screen flag is %d\n",printToScreen);

	turfioPos.bus=kvpGetInt("turfioBus",3); //in seconds
	turfioPos.slot=kvpGetInt("turfioSlot",10); //in seconds
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
	if(printToScreen)
	fprintf(stderr,"Error reading Acqd.config: %s\n",eString);
	    
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


void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr)
{
//    printf("writeEventAndMakeLink(%s,%s,%d",theEventDir,theLinkDir,(int)theEventPtr);
    char theFilename[FILENAME_MAX];
    int retVal;
    AnitaEventHeader_t *theHeader=&(theEventPtr->header);
    AnitaEventBody_t *theBody=&(theEventPtr->body);



    if(justWriteHeader==0 && writeData) {
	sprintf(theFilename,"%s/ev_%d.dat",theEventDir,
		theEventPtr->header.eventNumber);
	if(!oldStyleFiles) 
	    retVal=writeBody(theBody,theFilename);  
	else {
	    sprintf(theFilename,"%s/surf_data.%d",theEventDir,
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
	sprintf(theFilename,"%s/hd_%d.dat",theEventDir,
		theEventPtr->header.eventNumber);
	retVal=writeHeader(theHeader,theFilename);
    }

    /* Make links, not sure what to do with return value here */
    if(!standAloneMode && writeData) 
	retVal=makeLink(theFilename,theLinkDir);

    if(writeScalers) {
	sprintf(theFilename,"%s/scale_3%d.dat",scalerOutputDir,
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

    if(writeFullHk) {

	sprintf(theFilename,"%s/hk_%d.dat",hkOutputDir,hkNumber);
	FILE *hkFile;
	int n;
	if ((hkFile=fopen(theFilename, "wb")) == NULL) { 
	    printf("Failed to open hk file, %s\n", theFilename) ;
	    return ;
	}
	
	if ((n=fwrite(&theHk, sizeof(FullSurfHkStruct_t),1,hkFile))!=1)
	    printf("Failed to write all hk data. wrote only %d.\n", n) ;
	    fclose(hkFile);
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
		theEvent.body.channel[chanId].header.lastHitbus=lastHitbus;
		tempVal=((upperWord&0xc0)>>6)+(wrappedHitbus<<3);
		theEvent.body.channel[chanId].header.chipIdFlag=tempVal;
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
	    theHk.scaler[surf][rfChan]=dataInt&0xffff;
	    if(printToScreen && verbosity>1) 
		printf("SURF %d, Scaler %d == %d\n",surfIndex[surf],rfChan,theHk.scaler[surf][rfChan]);
	    
	    if(rfChan==0) {
		//Do something with upperWord
		theHk.upperWords[surf]=GetUpper16(dataInt);
	    }
	    else if(theHk.upperWords[surf]!=GetUpper16(dataInt)) {
		theHk.errorFlag|=(1>>surf);
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
	    theHk.threshold[surf][rfChan]=dataInt&0xffff;
	    if(printToScreen && verbosity>1) 
		printf("Surf %d, Threshold %d == %d\n",surfIndex[surf],rfChan,theHk.threshold[surf][rfChan]);
	    //Should check if it is the same or not
	    if(theHk.upperWords[surf]!=GetUpper16(dataInt)) {
		theHk.errorFlag|=(1>>surf);
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
	    theHk.rfPower[surf][rfChan]=dataInt&0xffff;
	    if(printToScreen && verbosity>1) 
		printf("Surf %d, RF Power %d == %d\n",surfIndex[surf],rfChan,theHk.rfPower[surf][rfChan]);
	    if(theHk.upperWords[surf]!=GetUpper16(dataInt)) {
		theHk.errorFlag|=(1>>surf);
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

    int wordNum;
    
    //First read to prime pipe
//    if(tryToUseBarMap) {
//	dataWord=*(turfBarMap);
//    }
//    else 
    if (readPlxDataShort(turfioHandle,&dataWord)!= ApiSuccess) {
	status=ACQD_E_PLXBUSREAD;
	syslog(LOG_ERR,"Failed to prime pipe TURFIO");
	if(printToScreen) 
	    printf("Failed to prime pipe TURFIO\n") ;
    }
    
    //Read out 256 words
    for(wordNum=0;wordNum<256;wordNum++) {
	if (readPlxDataShort(turfioHandle,&dataWord)!= ApiSuccess) {
	    status=ACQD_E_PLXBUSREAD;
	    syslog(LOG_ERR,"Failed to read TURFIO word %d",wordNum);
	    if(printToScreen) 
		printf("Failed to read TURFIO word %d\n",wordNum) ;
	    continue;
	}
	if(wordNum<128) switch(wordNum) {
	    case 0: turfioPtr->rawTrig=dataWord; break;
	    case 1: turfioPtr->l3ATrig=dataWord; break;
	    case 2: turfioPtr->ppsNum=dataWord; break;
	    case 3: turfioPtr->ppsNum+=(dataWord<<16); break;
	    case 4: turfioPtr->trigTime=dataWord; break;
	    case 5: turfioPtr->trigTime+=(dataWord<<16); break;
	    case 6: turfioPtr->trigInterval=dataWord; break;
	    case 7: turfioPtr->trigInterval+=(dataWord<<16); break;
	    default: turfioPtr->l1Rate[(wordNum/4)-2][wordNum%4]=dataWord; break;
	}
	else if(wordNum<160) {
	    turfioPtr->l2Rate[(wordNum/16)-8][wordNum%16]=dataWord;
	}
	else if(wordNum<176) {
	    turfioPtr->vetoMon[wordNum-160]=dataWord;
	}
	else if(wordNum<240) {
	    turfioPtr->l3Rate[(wordNum/16)-176][wordNum%16]=dataWord;
	}
	
    }
    if(verbosity && printToScreen) {
	printf("raw: %hu\nL3A: %hu\n1PPS: %lu\ntime: %lu\nint: %lu\n",
	       turfioPtr->rawTrig,turfioPtr->l3ATrig,turfioPtr->ppsNum,
	       turfioPtr->trigTime,turfioPtr->trigInterval);
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

PlxReturnCode_t writeRFCMMask(PlxHandle_t turfioHandle) {
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
    while(i<48) {
	if (i==32) j=chanBit=1 ;
	
//	printf("Debug RFCM: i=%d j=%d chn_bit=%d mask[j]=%d\n",i,j,chanBit,rfcmMask[j]);
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
	    avgScalerData[surf][dac]+=theHk.scaler[surf][dac];
	    if(avgCount==pidAverage) {
		value=avgScalerData[surf][dac]/avgCount;
		avgScalerData[surf][dac]=0;
		error=pidGoal-value;
	    
		// Proportional term
		pTerm = dacPGain * error;
		
		// Calculate integral with limiting factors
		thePids[surf][dac].iState+=error;
		if (thePids[surf][dac].iState > dacIMax) 
		    thePids[surf][dac].iState = dacIMax;
		else if (thePids[surf][dac].iState < dacIMin) 
		    thePids[surf][dac].iState = dacIMin;
		
		// Integral and Derivative Terms
		iTerm = dacIGain * thePids[surf][dac].iState;  
		dTerm = dacDGain * (value -thePids[surf][dac].dState);
		thePids[surf][dac].dState = value;
		
		//Put them together
		change = (int) (pTerm + iTerm - dTerm);
		thresholdArray[surfIndex[surf]-1][dac]+=change;
		if(thresholdArray[surfIndex[surf]-1][dac]>4095)
		    thresholdArray[surfIndex[surf]-1][dac]=4095;
		if(thresholdArray[surfIndex[surf]-1][dac]<1)
		    thresholdArray[surfIndex[surf]-1][dac]=1;
	    }
	}
    }
    if(avgCount==10) {
	avgCount=0;
	return 1;
    }
    return 0;
}
