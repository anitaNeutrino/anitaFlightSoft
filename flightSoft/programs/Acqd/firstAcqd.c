/*  \file firstAcqd.c
    \brief First version of the Acqd program 
    The first functional version of the acquisition software. For now this guy will try to have the same functionality as crate_test, but will look less like a dead dog.
    July 2005 rjn@mps.ohio-state.edu
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


inline unsigned short byteSwapUnsignedShort(unsigned short a){
    return (a>>8)+(a<<8);
}


//Global Variables
BoardLocStruct_t turfioPos;
BoardLocStruct_t surfPos[MAX_SURFS];
int dacSurfs[MAX_SURFS];
int dacChans[NUM_DAC_CHANS];
int printToScreen=0,standAloneMode=0;
int numSurfs=0,doingEvent=0;
unsigned short data_array[MAX_SURFS][N_CHAN][N_SAMP]; 
//TurfioStruct_t data_turf;
//AnitaEventHeader_t eventHeader;
//AnitaEventBody_t eventBody;
AnitaEventFull_t theEvent;

//Temporary Global Variables
unsigned short scalarData[MAX_SURFS][N_RFTRIG];
unsigned short rfpwData[MAX_SURFS][N_RFTRIG];


//Configurable watchamacallits
/* Ports and directories */
char acqdEventDir[FILENAME_MAX];
char acqdEventLinkDir[FILENAME_MAX];
char lastEventNumberFile[FILENAME_MAX];
char scalarOutputDir[FILENAME_MAX];

#define N_TMO 100    /* number of msec wait for Evt_f before timed out. */
#define N_FTMO 200    /* number of msec wait for Lab_f before timed out. */
//#define N_FTMO 2000    /* number of msec wait for Lab_f before timed out. */
#define DAC_CHAN 4 /* might replace with N_CHIP at a later date */


int verbosity = 0 ; /* control debug print out. */
int selftrig = FALSE ;/* self-trigger mode */
int surfonly = FALSE; /* Run in surf only mode */
int writeData = FALSE; /* Default is not to write data to a disk */
int writeScalars = FALSE;
int justWriteHeader = FALSE;
int doDacCycle = FALSE; /* Do a cycle of the DAC values */
int printStatistics = FALSE;
int dontWaitForEvtF = FALSE;
int dontWaitForLabF = FALSE;
int sendSoftTrigger = FALSE;
int writeOutC3p0Nums = FALSE;
int rprgturf = FALSE ;

//Trigger Modes
int rfTrigFlag = 0;
int pps1TrigFlag = 0;
int pps2TrigFlag = 0;
int softTrigFlag = 0;

void writeC3P0Nums();

int main(int argc, char **argv) {
    
    PlxHandle_t surfHandles[MAX_SURFS];
    PlxHandle_t turfioHandle;
    PlxDevLocation_t surfLocation[MAX_SURFS];
    PlxDevLocation_t turfioLocation;
    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    int retVal=0;
    unsigned short  dataWord=0;
    TriggerMode_t trigMode=TrigNone;
// Read only one chip per trigger. PM

    int i=0,tmo=0; 
    int numDevices, n_ev=0 ;
    int tmpGPIO;
    int dacVal=255;
    int dacValues[8];
    int surf,physDac;
    
    int dacChan=0;
    int doingSurfDac=0;
    int doingChanDac=0;
    struct timeval timeStruct;

   
    /* Set signal handlers */
    signal(SIGINT, sigIntHandler);
    signal(SIGTERM,sigTermHandler);

    retVal=readConfigFile();

    if(retVal!=CONFIG_E_OK) {
	syslog(LOG_ERR,"Error reading config file Acqd.config: bailing");
	return -1;
    }
//    printf("Debug3\n");
    if(printToScreen) printf(" Start test program %d %#x!!\n",LAB_F,LAB_F) ;
    makeDirectories(acqdEventDir);
    makeDirectories(acqdEventLinkDir);

//    printf("Debug4\n");
    if(standAloneMode) {
	init_param(argc, argv, acqdEventDir, &n_ev, &dacVal) ;
//	if (dir_n == NULL) dir_n = "./data" ; /* this is default directory name. */
    }

    if (verbosity && printToScreen) 
	printf("  size of buffer is %d bytes.\n", sizeof(dataWord)) ;

    // Initialize devices
    if ((numDevices = initializeDevices(surfHandles,&turfioHandle,
					surfLocation,&turfioLocation)) < 0) {
	if(printToScreen) printf("Problem initializing devices\n");
	syslog(LOG_ERR,"Error initializing devices");
	return -1 ;
    }

//    clearDevices(surfHandles,turfioHandle);
//    setTurfControl(turfioHandle,RprgTurf);
//    setTurfControl(turfioHandle,TurfClearEvent);
//    exit(0);
/* special initialization of TURF.  may not be necessary.  12-Jul-05 SM. */
    if (rprgturf) {
	setTurfControl(turfioHandle,TrigNone,RprgTurf) ;
	for(i=9 ; i++<100 ; usleep(1000)) ;
    }
    
    // Clear devices 
    clearDevices(surfHandles,turfioHandle);
//    exit(0);

    
    do {
	retVal=readConfigFile();
	/* set DAC value to a default value. */
//	setDACThresholds(surfHandles,dacVal);
	if(retVal!=CONFIG_E_OK) {
	    syslog(LOG_ERR,"Error reading config file Acqd.config: bailing");
	    return -1;
	}

	// Set trigger modes
	trigMode=TrigNone;
	if(rfTrigFlag) trigMode+=TrigRF;
	if(pps1TrigFlag) trigMode+=TrigPPS1;
	if(pps2TrigFlag) trigMode+=TrigPPS2;
	if(softTrigFlag) trigMode+=TrigSoft;
	
	//If doDacCycle only enable TrigPPS1;
	if(doDacCycle) trigMode=TrigPPS1;

	setTurfControl(turfioHandle,trigMode,SetTrigMode);



//    printf("Before Event Loop %d %d\n",doingEvent,n_ev);
	currentState=PROG_STATE_RUN;
	while (currentState==PROG_STATE_RUN) {
	/* initialize data_array with 0, 
	   should be just inside the outmost loop.*/
	    bzero(&theEvent, sizeof(theEvent)) ;
	    if(doDacCycle) {
		for(surf=doingSurfDac;i<numSurfs;++surf) {
		    if(dacSurfs[surf]) {
			for (dacChan=doingChanDac; dacChan<DAC_CHAN; dacChan++) {
			    if(dacChans[dacChan]) {
				for(dacVal=0;dacVal<4096;dacVal++) {
				    for(physDac=0;physDac<PHYS_DAC;physDac++) 
					dacValues[physDac]=dacVal;
				    
				    setDACThresholdsOnChan(surfHandles[surf],
							   dacChan,dacValues);
				    doingSurfDac=surf;
				    doingChanDac=dacChan; 
				    break;
				}
			    }
			}
		    }
		}
		exit(0);		
	    }

	    if(sendSoftTrigger) {
		sleep(4);
		setTurfControl(turfioHandle,trigMode,SendSoftTrg);
	    }
	    if (selftrig){  /* --- send a trigger ---- */ 
		if(printToScreen) printf(" try to trigger it locally.\n") ;
		for(surf=0;i<numSurfs;++surf) 
		    if (setSurfControl(surfHandles[surf], LTrig) != ApiSuccess) {
			status=ACQD_E_LTRIG;
			syslog(LOG_ERR,"Failed to set LTrig on SURF %d",surf);
			if(printToScreen)
			    printf("Failed to set LTrig flag on SURF %d.\n",surf);
		    }
	    }

//	    setTurfControl(turfioHandle,trigMode,SendSoftTrg);
	    /* Wait for ready to read (Evt_F) */
	    tmo=0;
	    
	    if(!dontWaitForEvtF) {

		if(verbosity && printToScreen) printf("Waiting for Evt_F on SURF 1-1\n");
		while (!((tmpGPIO=PlxRegisterRead(surfHandles[1], PCI9030_GP_IO_CTRL, &rc)) & EVT_F) && (selftrig?(++tmo<N_TMO):1)){
		    if(verbosity>3 && printToScreen) 
			printf("SURF 1 GPIO: 0x%x %d\n",tmpGPIO,tmpGPIO);
		    usleep(1); /*GV removed, RJN put it back in as it completely abuses the CPU*/
		}
		
		if (tmo == N_TMO){
		    syslog(LOG_WARNING,"Timed out waiting for Evt_F flag");
		    if(printToScreen)
			printf(" Timed out (%d ms) while waiting for Evt_F flag in self trigger mode.\n", N_TMO) ;
		    continue;
		}
	    }
	    else {
		printf("Didn't wait for Evt_F\n");
		sleep(1);
	    }
	    
	    gettimeofday(&timeStruct,NULL);
	    status+=readEvent(surfHandles,turfioHandle);
	    if(verbosity && printToScreen) printf("Done reading\n");
	    //Error checking
	    if(status!=ACQD_E_OK) {
		//We have an error
		if(printToScreen) fprintf(stderr,"Error detected %d, during event %d",status,doingEvent);
		
		status=ACQD_E_OK;
	    }
	    else {
		    theEvent.header.unixTime=timeStruct.tv_sec;
		    theEvent.header.unixTimeUs=timeStruct.tv_usec;
		    theEvent.header.eventNumber=getEventNumber();
		    if(printToScreen) 
			printf("Event:\t%d\nSec:\t%d\nMicrosec:\t%d\nTrigTime:\t%lu\n",theEvent.header.eventNumber,theEvent.header.unixTime,theEvent.header.unixTimeUs,theEvent.header.turfio.trigTime);
		// Save data
		if(writeData){
		    writeEventAndMakeLink(acqdEventDir,acqdEventLinkDir,&theEvent);
		}
		//Insert stats call here
		if(printStatistics) calculateStatistics();
	    }
	    
	    // Clear boards
	    for(surf=0;surf<numSurfs;++surf)
		if (setSurfControl(surfHandles[surf], SurfClearEvent) != ApiSuccess)
		    printf("  failed to send clear event pulse on SURF %d.\n",surf) ;
	    
	    if(!surfonly)
		if (setTurfControl(turfioHandle, trigMode,TurfClearEvent) != ApiSuccess)
		    printf("  failed to send clear event pulse on TURFIO.\n") ;
	    
	    
//	if(selftrig) sleep (2) ;
	}  /* closing master while loop. */
    } while(currentState==PROG_STATE_INIT);
    
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
	case EvNoD :
	    string = "EvNoD" ;
	    break ;
	case LabD :
	    string = "LabD" ;
	    break ;
	case SclD :
	    string = "SclD" ;
	    break ;
	case RFpwD :
	    string = "RFpwD" ;
	    break ;
	default :
	    string = "Unknown" ;
	    break ;
    }
    return string;
}

char *turfControlActionAsString(SurfControlAction_t action) {
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
	case EnableRfTrg : 
	    string = "EnableRfTrg" ;
	    break;
	case EnablePPS1Trig : 
	    string = "EnablePPS1Trig";
	    break;
	case EnablePPS2Trig : 
	    string = "EnablePPS2Trig";
	    break;
	case EnableSoftTrig : 
	    string = "EnableSoftTrig";
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
    U32 base_clr= 0222200222 ;
    U32 clr_evt = 0000000040 ;
    U32 clr_all = 0000000400 ;  
    U32 loc_trg = 0000000004 ;
    U32 evn_done= 0000400000 ;
    U32 lab_done= 0004000000 ;
    U32 scl_done= 0040000000 ;
    U32 rfp_done= 0400000000 ;

    U32           gpio_v ;
    PlxReturnCode_t   rc ;

    // Read current GPIO state
    //  base_v=PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc) ; 
    //  base_v=base_clr; // Gary's said this should be default PM 03-31-05

    switch (action) {
	case SurfClearAll : gpio_v = base_clr | clr_all  ; break ; /* RJN changed base_v */
	case SurfClearEvent : gpio_v = base_clr | clr_evt  ; break ; /* to base_clr */
	case LTrig  : gpio_v = base_clr | loc_trg  ; break ;
	case EvNoD  : gpio_v = base_clr | evn_done ; break ; 
	case LabD   : gpio_v = base_clr | lab_done ; break ;
	case SclD   : gpio_v = base_clr | scl_done ; break ;
	case RFpwD  : gpio_v = base_clr | rfp_done ; break ;
	default :
	    syslog(LOG_WARNING,"setSurfControl: undefined action %d",action);
	    if(printToScreen)
		fprintf(stderr,"setSurfControl: undefined action, %d\n", action) ;
	    return ApiFailed ;
    }
    if (verbosity && printToScreen) 
	printf(" setSurfControl: action %s, gpio_v = %x\n", 
	       surfControlActionAsString(action), gpio_v) ; 

    if ((rc = PlxRegisterWrite(surfHandle, PCI9030_GP_IO_CTRL, gpio_v ))
	!= ApiSuccess) {
	syslog(LOG_ERR,"setSurfControl: failed to set GPIO to %x.\n", gpio_v);
	if(printToScreen)
	    fprintf(stderr,"setSurfControl: failed to set GPIO to %x.\n", gpio_v) ; 
	return rc ;
    }

    /* Jing's logic requires trigger bit to be kept high.  9-Dec SM */
    //if (action == LTrig) return rc ;  // Self trigger disabled, PM 31-3-05
    /* return to the main, after reseting GPIO to base value. */
    return PlxRegisterWrite(surfHandle, PCI9030_GP_IO_CTRL, base_clr ) ;

}


PlxReturnCode_t setTurfControl(PlxHandle_t turfioHandle, TriggerMode_t trigMode, TurfControlAction_t action) {

    //These numbers are octal
    U32 base_clr         = 0022222222 ;
    U32 rprg_turf        = 0000000004 ;
    U32 clear_event      = 0000000040 ; 
    U32 send_soft_trig   = 0000000400 ;
    U32 enable_rf_trig   = 0000004000 ;
    U32 enable_pps1_trig = 0000040000 ; //Real PPS from ADU5
    U32 enable_pps2_trig = 0000400000 ; //Burst PPS up to 20Hz from G123
    U32 enable_soft_trig = 0004000000 ;
    U32 clear_all        = 0040000000 ;

    U32           gpio_v,base_v ;
    PlxReturnCode_t   rc ;
    int i ;

//    printf("Dec: %d %d %d\nHex: %x %x %x\nOct: %o %o %o\n",base_clr,rprg_turf,clr_trg,base_clr,rprg_turf,clr_trg,base_clr,rprg_turf,clr_trg);
//    exit(0);
    // Read current GPIO state
    //base_v=PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc) ; 
    //base_v=base_clr; // Gary's said this should be default PM 03-31-05
    
    base_v=base_clr;
    if(trigMode&TrigRF) base_v |= enable_rf_trig;
    if(trigMode&TrigPPS1) base_v |= enable_pps1_trig;
    if(trigMode&TrigPPS2) base_v |= enable_pps2_trig;
    if(trigMode&TrigSoft) base_v |= enable_soft_trig;

    switch (action) {
	case SetTrigMode : gpio_v =base_v; break;
	case RprgTurf : gpio_v = base_v | rprg_turf ; break ;
	case TurfClearEvent : gpio_v = base_v | clear_event ; break ; 
	case SendSoftTrg : gpio_v = base_v | send_soft_trig ; break ;
	case EnableRfTrg : gpio_v = base_v | enable_rf_trig ; break ;
	case EnablePPS1Trig : gpio_v = base_v | enable_pps1_trig ; break ;
	case EnablePPS2Trig : gpio_v = base_v | enable_pps2_trig ; break ;
	case EnableSoftTrig : gpio_v = base_v | enable_soft_trig ; break ;
	case TurfClearAll : gpio_v = base_v | clear_all ; break ;
	    
	default :
	    syslog(LOG_WARNING,"setTurfControl: undefined action %d",action);
	    if(printToScreen)
		printf(" setTurfControl: undfined action, %d\n", action) ;
	    return ApiFailed ;
    }

    if (verbosity && printToScreen) 
	printf("setTurfControl: action %s, gpio_v = %x (dec: %d, oct: %o)\n", 
	       turfControlActionAsString(action), gpio_v,gpio_v,gpio_v) ; 

    if ((rc = PlxRegisterWrite(turfioHandle, PCI9030_GP_IO_CTRL, gpio_v ))
	!= ApiSuccess) {
	syslog(LOG_ERR,"setTurfControl: failed to set GPIO to %x.\n", gpio_v);
	if(printToScreen)
	    printf(" setTurfControl : failed to set GPIO to %x.\n", gpio_v) ;
	return rc ;
    }

    if (action == RprgTurf)  /* wait a while in case of RprgTurf. */
	for(i=0 ; i++<10 ; usleep(1)) ;

    return PlxRegisterWrite(turfioHandle, PCI9030_GP_IO_CTRL, base_v ) ;

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

    int i,j,countSurfs=0;
    U32  numDevices ;
    PlxDevLocation_t tempLoc[MAX_SURFS+1];
    PlxReturnCode_t rc;
    //  U32  n=0 ;  /* this is the numebr of board from furthest from CPU. */

    for(i=0;i<MAX_SURFS+1;++i){
	tempLoc[i].BusNumber       = (U8)-1;
	tempLoc[i].SlotNumber      = (U8)-1;
	tempLoc[i].VendorId        = (U16)-1;
	tempLoc[i].DeviceId        = (U16)-1;
	tempLoc[i].SerialNumber[0] = '\0';
    }
    

    /* need to go through PlxPciDeviceFind twice.  1st for counting devices
       with "numDevices=FIND_AMOUNT_MATCHED" and 2nd to get device info. */

    numDevices = FIND_AMOUNT_MATCHED;
    if (PlxPciDeviceFind(&tempLoc[0], &numDevices) != ApiSuccess ||
	numDevices > 12) return -1 ;
    if (verbosity && printToScreen) printf("init_device: found %d PLX devices.\n", numDevices) ;

    /* Initialize SURFs */
    for(i=0;i<numDevices;++i){
	if (PlxPciDeviceFind(&tempLoc[i], &i) != ApiSuccess) 
	    return -1 ;

	if (verbosity>1 && printToScreen) 
	    printf("init_device: device found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
		   tempLoc[i].DeviceId, tempLoc[i].VendorId, tempLoc[i].SerialNumber, 
		   tempLoc[i].BusNumber, tempLoc[i].SlotNumber);
    
	

	if(tempLoc[i].BusNumber==turfioPos.bus && tempLoc[i].SlotNumber==turfioPos.slot) {
	    // Have got the TURFIO
	    copyPlxLocation(turfioLoc,tempLoc[i]);

	    
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
	else {
	    for(j=countSurfs;j<MAX_SURFS;j++) {
/* 		printf("tempLoc[%d] %d %d \t surfPos[%d] %d %d\n", */
/* 		       i,tempLoc[i].BusNumber,tempLoc[i].SlotNumber,j, */
/* 		       surfPos[j].bus,surfPos[j].slot); */
		       
		if(tempLoc[i].BusNumber==surfPos[j].bus && tempLoc[i].SlotNumber==surfPos[j].slot) {
		    // Got a SURF

		    copyPlxLocation(&surfLoc[countSurfs],tempLoc[i]);
//		    surfLoc[countSurfs]=tempLoc[i];   
		    if (verbosity && printToScreen)
			printf("SURF found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
			       surfLoc[countSurfs].DeviceId, 
			       surfLoc[countSurfs].VendorId, 
			       surfLoc[countSurfs].SerialNumber,
			       surfLoc[countSurfs].BusNumber, 
			       surfLoc[countSurfs].SlotNumber);
		    rc=PlxPciDeviceOpen(&surfLoc[countSurfs], 
					&surfHandles[countSurfs]);
		    if ( rc!= ApiSuccess) {
			syslog(LOG_ERR,"Error opening SURF device %d",rc);
			if(printToScreen)
			    fprintf(stderr,"Error opening SURF device %d\n",rc);
			return -1 ;
		    }		
		    PlxPciBoardReset(surfHandles[countSurfs]) ;
		    countSurfs++;    
		    break;
		}
	    }
	}
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
    int status=0,count;
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
//    printf("Debug rc1\n");
    if(status == CONFIG_E_OK) {
	tempString=kvpGetString ("acqdEventDir");
	if(tempString)
	   strncpy(acqdEventDir,tempString,FILENAME_MAX-1);
	else
	    fprintf(stderr,"Couldn't fetch acqdEventDir\n");
//	printf("%s\n",acqdEventDir);
	tempString=kvpGetString ("acqdEventLinkDir");
	if(tempString)
	    strncpy(acqdEventLinkDir,tempString,FILENAME_MAX-1);
	else
	    fprintf(stderr,"Coudn't fetch acqdEventLinkDir\n");
	tempString=kvpGetString ("lastEventNumberFile");
	if(tempString)
	    strncpy(lastEventNumberFile,tempString,FILENAME_MAX-1);
	else
	    fprintf(stderr,"Coudn't fetch lastEventNumberFile\n");
	tempString=kvpGetString ("scalarOutputDir");
	if(tempString)
	    strncpy(scalarOutputDir,tempString,FILENAME_MAX-1);
	else
	    fprintf(stderr,"Coudn't fetch scalarOutputDir\n");
//	printf("Debug rc2\n");
	printToScreen=kvpGetInt("printToScreen",0);
	dontWaitForEvtF=kvpGetInt("dontWaitForEvtF",0);
	dontWaitForLabF=kvpGetInt("dontWaitForLabF",0);
	writeOutC3p0Nums=kvpGetInt("writeOutC3p0Nums",0);
	sendSoftTrigger=kvpGetInt("sendSoftTrigger",0);
//	printf("dontWaitForEvtF: %d\n",dontWaitForEvtF);
	standAloneMode=kvpGetInt("standAloneMode",0);
	writeData=kvpGetInt("writeData",1);
	writeScalars=kvpGetInt("writeScalars",1);
	justWriteHeader=kvpGetInt("justWriteHeader",0);
	verbosity=kvpGetInt("verbosity",0);
	printStatistics=kvpGetInt("printStatistics",0);
	rfTrigFlag=kvpGetInt("enableRFTrigger",1);
	pps1TrigFlag=kvpGetInt("enablePPS1Trigger",1);
	pps2TrigFlag=kvpGetInt("enablePPS2Trigger",1);
	softTrigFlag=kvpGetInt("enableSoftTrigger",1);
	doDacCycle=kvpGetInt("doDacCycle",0);
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
		if(surfBuses[count]!=-1) {
		    surfPos[count].bus=surfBuses[count];
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
		if(surfSlots[count]!=-1) {
		    surfPos[count].slot=surfSlots[count];
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
	tempNum=4;
	kvpStatus = kvpGetIntArray("dacChans",dacChans,&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(dacChans): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(dacChans): %s\n",
			kvpErrorString(kvpStatus));
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
    int i;
    PlxReturnCode_t  rc ;

    if(verbosity &&printToScreen) printf("*** Clearing devices ***\n");

// Prepare SURF boards
    for(i=0;i<numSurfs;++i){
	/* init. 9030 I/O descripter (to enable READY# input |= 0x02.) */
	if(printToScreen) printf("Trying to set Sp0 on SURF %d\n",i);
	if(PlxRegisterWrite(surfHandles[i], PCI9030_DESC_SPACE0, 0x00800000) 
	   != ApiSuccess)  {
	    syslog(LOG_ERR,"Failed to set SpO descriptor on SURF %d",i);
	    if(printToScreen)
		printf("Failed to set Sp0 descriptor on SURF %d.\n",i ) ;    
	}
    }
    
    for(i=0;i<numSurfs;++i){
	if(printToScreen)
	    printf(" GPIO register contents SURF %d = %x\n",i,
		   PlxRegisterRead(surfHandles[i], PCI9030_GP_IO_CTRL, &rc)) ; 
	if (setSurfControl(surfHandles[i], SurfClearAll) != ApiSuccess) {
	    syslog(LOG_ERR,"Failed to send clear all event pulse on SURF %d.\n",i) ;
	    if(printToScreen)
		printf("Failed to send clear all event pulse on SURF %d.\n",i);
	}
    }

    if(verbosity && printToScreen){
	printf(" Try to test interrupt control register.\n") ;
	for(i=0;i<numSurfs;++i)
	    printf(" Int reg contents SURF %d = %x\n",i,
		   PlxRegisterRead(surfHandles[i], PCI9030_INT_CTRL_STAT, &rc)) ; 
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

    if(printToScreen) printf(" GPIO register contents TURFIO = %x\n",
			     PlxRegisterRead(turfioHandle, 
					     PCI9030_GP_IO_CTRL, &rc)) ; 

    if (setTurfControl(turfioHandle,TrigNone, TurfClearAll) != ApiSuccess) {
	syslog(LOG_ERR,"Failed to send clear pulse on TURF %d.\n",i) ;
	if(printToScreen)
	    printf("  failed to send clear pulse on TURF %d.\n",i) ;
    }

    /*    printf(" Try to read 9030 GPIO.  %x\n", */
    /*  	 PlxRegisterRead(turfioHandle, PCI9030_GP_IO_CTRL, &rc)) ; */
    /*    if (rc != ApiSuccess) printf("  failed to read TURFIO GPIO .\n") ; */

    if(verbosity && printToScreen){
	printf(" Try to test interrupt control register.\n") ;
	printf(" int reg contents TURFIO = %x\n",
	       PlxRegisterRead(turfioHandle, PCI9030_INT_CTRL_STAT, &rc)) ; 
    }
    
    return;
}



/* void writeSurfData(char *directory, unsigned short *wv_data,unsigned long evNum) { */

/*     char f_name[128] ; */
/*     FILE *wv_file ; */
/*     int  n ; */

/*     sprintf(f_name, "%s/surf_data.%lu", directory, evNum) ; */
/*     if(verbosity && printToScreen)  */
/* 	printf(" writeData: save data to %s\n", f_name) ; */

/*     if ((wv_file=fopen(f_name, "w")) == NULL) { */
/* 	printf(" writeData: failed to open wv file, %s\n", f_name) ; */
/* 	return ;  */
/*     } */

/*     if ((n=fwrite(wv_data, sizeof(unsigned short), N_SAMP*N_CHAN*numSurfs, wv_file)) */
/* 	!= N_SAMP*N_CHAN*numSurfs) */
/* 	printf(" writeData: failed to write all data. wrote only %d.\n", n) ; */

/*     fclose (wv_file) ; */
/* } */

/* void writeTurf(char *directory, TurfioStruct_t *theEvent.header.turfio,unsigned long evNum) { */

/*     char f_name[128] ; */
/*     FILE *wv_file ; */
/*     int  n ; */

/*     sprintf(f_name, "%s/turf_data.%lu", directory, evNum) ; */
/*     if(verbosity && printToScreen)  */
/* 	printf(" writeTurf: save data to %s\n", f_name) ; */

/*     if ((wv_file=fopen(f_name, "w")) == NULL) { */
/* 	printf(" writeData: failed to open wv file, %s\n", f_name) ; */
/* 	return ;  */
/*     } */

/*     if ((n=fwrite(theEvent.header.turfio, sizeof(TurfioStruct_t), 1, wv_file))!= 1) */
/* 	printf(" writeData: failed to write turf data.\n") ; */

/*     fclose (wv_file) ; */
/* } */


/* purse command arguments to initialize parameter(s). */
int init_param(int argn, char **argv, char *directory, int *n, int *dacVal) {

    while (--argn) {
	if (**(++argv) == '-') {
	    switch (*(*argv+1)) {
		case 'v': verbosity++ ; break ;
		case 't': selftrig = TRUE; break;
		case 's': surfonly = TRUE; break;
		case 'w': writeData = TRUE; break;
		case 'c': doDacCycle = TRUE; break;
		case 'a': *dacVal=atoi(*(++argv)) ; --argn ; break ;
		case 'r': rprgturf = TRUE ; break ;
		case 'd': strncpy(directory,*(++argv),FILENAME_MAX);
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

void setDACThresholdsOnChan(PlxHandle_t surfHandle, int chan, int values[8]) 
{
    
#define DAC_CNT 10000000
#define BIT_NO 16
#define BIT_EFF 12

    int bit_RS,bit;
    int phys_DAC;
    unsigned short dataByte;
    unsigned short DACVal[DAC_CHAN][PHYS_DAC],DAC_C_0[DAC_CHAN][BIT_NO]; 
    //  unsigned short DACVal_1[DAC_CHAN][PHYS_DAC];
    unsigned short DAC_C[DAC_CHAN][BIT_NO];


    //Initialize DAC values for the various channels
    //DAC1A..8A;
    DAC_C[0][15]=0; 	DAC_C_0[0][15]=0;
    DAC_C[0][14]=0; 	DAC_C_0[0][14]=0;
    DAC_C[0][13]=0xff;	DAC_C_0[0][13]=0xff;
    DAC_C[0][12]=0; 	DAC_C_0[0][12]=0;
    //DAC1A..8A;

    //DAC1B..8B;
    DAC_C[1][15]=0; 	DAC_C_0[1][15]=0;
    DAC_C[1][14]=0xff;	DAC_C_0[1][14]=0xff;
    DAC_C[1][13]=0xff;	DAC_C_0[1][13]=0xff;
    DAC_C[1][12]=0; 	DAC_C_0[1][12]=0;
    //DAC1B..8B;

    //DAC1C..8C;
    DAC_C[2][15]=0xff;	DAC_C_0[2][15]=0xff;
    DAC_C[2][14]=0; 	DAC_C_0[2][14]=0;
    DAC_C[2][13]=0xff;	DAC_C_0[2][13]=0xff;
    DAC_C[2][12]=0; 	DAC_C_0[2][12]=0;
    //DAC1C..8C;

    //DAC1D..8D;
    DAC_C[3][15]=0xff;	DAC_C_0[3][15]=0xff;
    DAC_C[3][14]=0xff;	DAC_C_0[3][14]=0xff;
    DAC_C[3][13]=0xff;	DAC_C_0[3][13]=0xff;
    DAC_C[3][12]=0; 	DAC_C_0[3][12]=0;
    //DAC1D..8D;

    for (phys_DAC=0; phys_DAC<PHYS_DAC; phys_DAC++)
    {
	DACVal[chan][phys_DAC]=values[phys_DAC]&0xffff;
	//  printf("DACVal[%d][%d]=%x    \n", chan, phys_DAC, DACVal[chan][phys_DAC]);
    }
    for (bit=0; bit<BIT_EFF; bit++)
    {
	DAC_C[chan][bit] =0;
	DAC_C_0[chan][bit] =0;
    }
    
    for (bit=0; bit<BIT_EFF; bit++) {
	bit_RS=bit;
	phys_DAC=0;
	while ((bit_RS>0) && (phys_DAC<PHYS_DAC)) {
	    DAC_C_0[chan][bit] |=(DACVal[chan][phys_DAC] & ((0x0001)<<bit)) >> bit_RS;
	    bit_RS--;
	    phys_DAC++;
	}
	while (phys_DAC<PHYS_DAC) {
	    DAC_C_0[chan][bit] |=(DACVal[chan][phys_DAC] & ((0x0001)<<bit)) << bit_RS;
	    bit_RS++;
	    phys_DAC++;
	}
	DAC_C[chan][bit]=(DAC_C_0[chan][bit]) & 0x00FF;
    }
    
    if(verbosity>1 && printToScreen)
	printf("  setDAC: Writing: ");
    for (bit=BIT_NO-1; bit>=0; bit--) {
	dataByte=DAC_C[chan][bit];
	if (PlxBusIopWrite(surfHandle, IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16) != ApiSuccess) {
	    syslog(LOG_ERR,"Failed to write DAC value\n");
	    if(printToScreen)
		fprintf(stderr,"Failed to write DAC value\n");
	}
	if(verbosity>1 && printToScreen)printf("%x ",dataByte);
	//else printf("Wrote %d to SURF %d; value: %d\n",dataByte,boardIndex,value);		    
    }
    if(verbosity>1 && printToScreen) printf("\n");
// tmpGPIO=PlxRegisterRead(surfHandles[boardIndex], PCI9030_GP_IO_CTRL, &rc);
// tmpGPIO=PlxRegisterRead(surfHandles[boardIndex], PCI9030_GP_IO_CTRL, &rc);
}


void setDACThresholds(PlxHandle_t *surfHandles,  int threshold) {
    //Only does one threshold at the moment
    
#define DAC_CNT 10000000
#define BIT_NO 16
#define BIT_EFF 12

    int bit_RS,bit;
    int chan,phys_DAC,boardIndex;
    unsigned short dataByte;
    unsigned short DACVal[DAC_CHAN][PHYS_DAC],DAC_C_0[DAC_CHAN][BIT_NO]; 
    //  unsigned short DACVal_1[DAC_CHAN][PHYS_DAC];
    unsigned short DAC_C[DAC_CHAN][BIT_NO];
    int tmpGPIO;
    PlxReturnCode_t      rc ;

    // Copy what Jing does at start of her program
    // Will probably remove this at some point
    /*     for(i=0;i<numDevices-1;++i) { */
    /* 	if (setSurfControl(surfHandles[i], SurfClearEvent) != ApiSuccess) */
    /* 	    printf("  failed to send clear event pulse on SURF %d.\n",i); */
    /* 	if (setSurfControl(surfHandles[i], SurfClearAll) != ApiSuccess) */
    /* 	    printf("  failed to send clear all pulse on SURF %d.\n",i); */
    /*     } */


    //DAC1A..8A;
    DAC_C[0][15]=0; 	DAC_C_0[0][15]=0;
    DAC_C[0][14]=0; 	DAC_C_0[0][14]=0;
    DAC_C[0][13]=0xff;	DAC_C_0[0][13]=0xff;
    DAC_C[0][12]=0; 	DAC_C_0[0][12]=0;
    //DAC1A..8A;

    //DAC1B..8B;
    DAC_C[1][15]=0; 	DAC_C_0[1][15]=0;
    DAC_C[1][14]=0xff;	DAC_C_0[1][14]=0xff;
    DAC_C[1][13]=0xff;	DAC_C_0[1][13]=0xff;
    DAC_C[1][12]=0; 	DAC_C_0[1][12]=0;
    //DAC1B..8B;

    //DAC1C..8C;
    DAC_C[2][15]=0xff;	DAC_C_0[2][15]=0xff;
    DAC_C[2][14]=0; 	DAC_C_0[2][14]=0;
    DAC_C[2][13]=0xff;	DAC_C_0[2][13]=0xff;
    DAC_C[2][12]=0; 	DAC_C_0[2][12]=0;
    //DAC1C..8C;

    //DAC1D..8D;
    DAC_C[3][15]=0xff;	DAC_C_0[3][15]=0xff;
    DAC_C[3][14]=0xff;	DAC_C_0[3][14]=0xff;
    DAC_C[3][13]=0xff;	DAC_C_0[3][13]=0xff;
    DAC_C[3][12]=0; 	DAC_C_0[3][12]=0;
    //DAC1D..8D;


    ///////////////////////////
    //Write in DAC thresholds--Start
    for (chan=0; chan<DAC_CHAN; chan++) 
    {
	for (phys_DAC=0; phys_DAC<PHYS_DAC; phys_DAC++)
	{
	    DACVal[chan][phys_DAC]=threshold&0xffff;
	    //  printf("DACVal[%d][%d]=%x    \n", chan, phys_DAC, DACVal[chan][phys_DAC]);
	}
	//	    printf("\n");
    }

    for (chan=0; chan<DAC_CHAN; chan++)
    {
	for (bit=0; bit<BIT_EFF; bit++)
	{
	    DAC_C[chan][bit] =0;
	    DAC_C_0[chan][bit] =0;
	}
    }


    for (chan=0; chan<DAC_CHAN; chan++) {
	for (bit=0; bit<BIT_EFF; bit++) {
	    bit_RS=bit;
	    phys_DAC=0;
	    while ((bit_RS>0) && (phys_DAC<PHYS_DAC)) {
		DAC_C_0[chan][bit] |=(DACVal[chan][phys_DAC] & ((0x0001)<<bit)) >> bit_RS;
		bit_RS--;
		phys_DAC++;
	    }
	    while (phys_DAC<PHYS_DAC) {
		DAC_C_0[chan][bit] |=(DACVal[chan][phys_DAC] & ((0x0001)<<bit)) << bit_RS;
		bit_RS++;
		phys_DAC++;
	    }
	    DAC_C[chan][bit]=(DAC_C_0[chan][bit]) & 0x00FF;
	}
    }

    for(boardIndex=0;boardIndex<numSurfs;boardIndex++) {	    
	for (chan=0; chan<DAC_CHAN; chan++) {
	    if(verbosity>1 && printToScreen)      printf("  setDAC: Writing: ");
	    for (bit=BIT_NO-1; bit>=0; bit--) {
		dataByte=DAC_C[chan][bit];
		if (PlxBusIopWrite(surfHandles[boardIndex], IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16) != ApiSuccess) {
		    syslog(LOG_ERR,"Failed to write DAC value\n");
		    if(printToScreen)
			fprintf(stderr,"Failed to write DAC value\n");
		}
		if(verbosity>1 && printToScreen)printf("%x ",dataByte);
		//else printf("Wrote %d to SURF %d; value: %d\n",dataByte,boardIndex,value);		    
	    }
	    if(verbosity>1 && printToScreen) printf("\n");
	    tmpGPIO=PlxRegisterRead(surfHandles[boardIndex], PCI9030_GP_IO_CTRL, &rc);
	    tmpGPIO=PlxRegisterRead(surfHandles[boardIndex], PCI9030_GP_IO_CTRL, &rc);
	}
    }
	
}

PlxReturnCode_t readPlxDataWord(PlxHandle_t handle, unsigned short *dataWord)
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
		printf("Board %d Mean, After Event %d (Software)\n",surf,doingEvent);
		for(chip=0;chip<N_CHIP;++chip){
		    for(chan=0;chan<N_CHAN;++chan)
			printf("%.2f ",chanMean[surf][chip][chan]);
		    printf("\n");
		}
	    }

	    // Display Std Dev
	    for(surf=0;surf<numSurfs;++surf){
		printf("Board %d Std Dev., After Event %d (Software)\n",surf,doingEvent);
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
    if(firstTime) {
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
    return eventNumber;
}


void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr)
{
    char theFilename[FILENAME_MAX];
    int retVal;
    AnitaEventHeader_t *theHeader=&(theEventPtr->header);
    AnitaEventBody_t *theBody=&(theEventPtr->body);



    if(justWriteHeader==0) {
	sprintf(theFilename,"%s/ev_%d.dat",theEventDir,
		theEventPtr->header.eventNumber);
	retVal=writeBody(theBody,theFilename);  
    }
      
    sprintf(theFilename,"%s/hd_%d.dat",theEventDir,
	    theEventPtr->header.eventNumber);
    retVal=writeHeader(theHeader,theFilename);

    /* Make links, not sure what to do with return value here */
    if(!standAloneMode) 
	retVal=makeLink(theFilename,theLinkDir);

    if(writeScalars) {
	sprintf(theFilename,"%s/scale_%d.dat",scalarOutputDir,
		theEventPtr->header.eventNumber);
	FILE *scalarFile;
	int n;
	if ((scalarFile=fopen(theFilename, "wb")) == NULL) { 
	    printf("Failed to open scalar file, %s\n", theFilename) ;
	    return ;
	}
	
	if ((n=fwrite(scalarData, sizeof(unsigned short),numSurfs*N_RFTRIG, scalarFile)) != N_RFTRIG*numSurfs)
	printf("Failed to write all scalar data. wrote only %d.\n", n) ;
	fclose(scalarFile);
    }

}


AcqdErrorCode_t readEvent(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle) 
/*! This is the code that is executed after we read the Evt_F flag */
{

    PlxReturnCode_t rc;
    AcqdErrorCode_t status=ACQD_E_OK;
    unsigned short  dataWord=0;
    unsigned char turfDataWord=0;
    unsigned char turfOtherWord=0;
    unsigned short  evNo=0;
// Read only one chip per trigger. PM

    int i=0, ftmo=0;
    int surf,chan,samp,rf;


    if(verbosity && printToScreen) 
	printf("Triggered, event %d (by software counter).\n",doingEvent);

	
    //Loop over SURFs and read out data
    for(surf=0;surf<numSurfs;surf++){
	if(verbosity && printToScreen)
	    printf(" GPIO register contents SURF %d = %x\n",surf,
		   PlxRegisterRead(surfHandles[surf], PCI9030_GP_IO_CTRL, &rc)) ; 
	if(verbosity && printToScreen)
	    printf(" int reg contents SURF %d = %x\n",surf,
		   PlxRegisterRead(surfHandles[surf], PCI9030_INT_CTRL_STAT, &rc)) ; 

	// Readout event number
	if (readPlxDataWord(surfHandles[surf],&dataWord)!= ApiSuccess) {
	    // What to do if you don't read out event number
	    status=ACQD_E_PLXBUSREAD;
	    syslog(LOG_ERR,"Failed to read event number: SURF %d",surf);
	    if(printToScreen) 
		printf("Failed to read event number:SURF %d\n", surf) ;
	}
	if(verbosity && printToScreen) 
	    printf("SURF %d, ID %d, event no %d\n",surf,
		   dataWord>>14,dataWord&DATA_MASK);
	    
	if(surf==0) evNo=dataWord&DATA_MASK;
	if(setSurfControl(surfHandles[surf],EvNoD)!=ApiSuccess) {
	    //Again what to do on failure?
	    status=ACQD_E_EVNOD;
	    syslog(LOG_ERR,"Failed to set EvNoD: SURF %d",surf);
	    if(printToScreen)
		printf("Failed to set EvNoD on SURF %d\n",surf);
	}

	// Wait for Lab_F
	if(!dontWaitForLabF) {
	    ftmo=0;
	    int testVal=0;
	    if(verbosity && printToScreen) printf("SURF %d, waiting for Lab_F\n",surf);
	    testVal=PlxRegisterRead(surfHandles[surf],PCI9030_GP_IO_CTRL, &rc);
	    while(!(testVal & LAB_F) && (++ftmo<N_FTMO)){ //RJN HACK
		usleep(1); // RJN/GV changed from usleep(1000)
		testVal=PlxRegisterRead(surfHandles[surf], PCI9030_GP_IO_CTRL, &rc);
	    }       
	    if (ftmo == N_FTMO){
		syslog(LOG_WARNING,"No Lab_F flag, bailing");
		if(printToScreen)
		    printf("No Lab_F flag for %d ms, bailing on the event\n",N_FTMO);
		continue;
	    }
	}
	else printf("Didn't wait for Lab_F\n");
      
	//      Read LABRADOR data
	//      int firstTime=1;
	for (samp=0 ; samp<N_SAMP ; samp++) {
	    for (chan=0 ; chan<N_CHAN ; chan++) {
		if (readPlxDataWord(surfHandles[surf],&dataWord)
		    != ApiSuccess) {
		    status=ACQD_E_PLXBUSREAD;			
		    syslog(LOG_ERR,"Failed to read data: SURF %d, chan %d, samp %d",surf,chan,samp);
		    if(printToScreen) 
			printf("Failed to read IO. surf=%d, chn=%d sca=%d\n", surf, chan, samp) ;
		}

		// Record which chip is being read out
//		    if( (((dataWord&DATA_MASK)-2750)>400) && !(dataWord & 0x1000) ) 
//			printf("SURF %d, chip#= %d Chan#= %d SCA#= %d: %d\n",surf,(dataWord>>14),chan,samp,((dataWord&DATA_MASK)-2750));
		if(verbosity>2 && printToScreen) 
		    printf("SURF %d, CHP %d,CHN %d, SCA %d: %d (HITBUS=%d) %d\n",surf,dataWord>>14,chan,samp,dataWord,dataWord & 0x1000,dataWord&DATA_MASK);		    
		// Check if 13 bit is zero
		if ((dataWord&0x2000) != 0) {
		    status=ACQD_E_TRANSFER;
		    syslog(LOG_ERR,"Bit 13 set on surf %d, chan %d, samp %d",surf,chan,samp);
		    if(printToScreen) 
			printf(" data xfer error; bit 13 != 0 on surf=%d, chn=%d sca=%d\n",surf,chan,samp);	  
		}
		theEvent.body.channel[chan+surf*CHANNELS_PER_SURF].data[samp]=dataWord;
//		    data_array[surf][chan][samp] = dataWord ;//& DATA_MASK ; 
	    }
	}
	    
	if(setSurfControl(surfHandles[surf],LabD)!=ApiSuccess) {
	    status=ACQD_E_LABD;
	    syslog(LOG_ERR,"Failed to set LabD: surf %d",surf);
	    if(printToScreen)
		printf("Failed to set LabD on SURF %d\n",surf);
	}
	// Read trigger scalars
	for(i=0;i<N_RFTRIG;++i){
	    if (readPlxDataWord(surfHandles[surf],&dataWord)
		!= ApiSuccess) {
		//Once more need a method of failure tracking
		status=ACQD_E_PLXBUSREAD;
		syslog(LOG_ERR,"Failed to read surf %d, scalar %d",surf,i);
		if(printToScreen)
		    printf("Failed to read IO. surf=%d, rf scl=%d\n", surf, i) ;
	    }
	    scalarData[surf][i]=dataWord;
	    if(verbosity>1 && printToScreen) {
		if(( (i % 8) ) == 0) {
		    printf("\n");
		    printf("SURF %d, SCL %d: %d",surf,i,dataWord);
		} 
		else printf(" %d",dataWord);
	    }
		
	}
	if(verbosity>1 && printToScreen) printf("\n");
      
	if(setSurfControl(surfHandles[surf],SclD)!=ApiSuccess) {
	    status=ACQD_E_SCLD;
	    syslog(LOG_ERR,"Failed to set SclD: surf %d",surf);
	    if(printToScreen)
		printf("Failed to set SclD on SURF %d\n",surf);
	}
	// Read RF power 
	for(rf=0;rf<N_CHAN;++rf){
	    if (readPlxDataWord(surfHandles[surf],&dataWord)
		!= ApiSuccess) {
		status=ACQD_E_PLXBUSREAD;
		//Once more need a method of failure tracking
		syslog(LOG_ERR,"Failed to read surf %d, rf %d",surf,rf);
		if(printToScreen)
		    printf("Failed to read IO. surf=%d, rf pw=%d\n", surf, rf) ;
	    }
	    rfpwData[surf][rf]=dataWord;
	    if(verbosity>1 && printToScreen) 
		printf("SURF %d, RF %d: %d\n",surf,rf,dataWord);
	}
      
	if(setSurfControl(surfHandles[surf],RFpwD)!=ApiSuccess) {
	    status=ACQD_E_RFPWD;
	    syslog(LOG_ERR,"Failed to set RFpwD: surf %d",surf);
	    if(printToScreen)		
		printf("Failed to set RFpwD on SURF %d\n",surf);
	}
	    
    }  //Close the numSurfs loop
	
    if(!surfonly){
	    
	if(verbosity && printToScreen) 
	    printf(" GPIO register contents TURFIO = %x\n",
		   PlxRegisterRead(turfioHandle,PCI9030_GP_IO_CTRL, &rc)); 
	if(verbosity && printToScreen) 
	    printf(" int reg contents TURFIO = %x\n", 
		   PlxRegisterRead(turfioHandle,PCI9030_INT_CTRL_STAT,&rc)) ; 
	    
	// Burn one TURFIO read
	if (readPlxDataWord(turfioHandle,&dataWord)!= ApiSuccess) {
	    status=ACQD_E_PLXBUSREAD;		
	    syslog(LOG_ERR,"Failed to read TURFIO burn read");
	    if(printToScreen)
		printf("Failed to read TURF IO port. Burn read.\n");
	}

	for(i=0;i<256;i++) {
	    if (readPlxDataWord(turfioHandle,&dataWord)
		    != ApiSuccess) {
		    //Once more need a method of failure tracking
		    status=ACQD_E_PLXBUSREAD;		
		    syslog(LOG_ERR,"Failed to read TURFIO word %d",i);
		    if(printToScreen)			    
			printf("Failed to read TURF IO port. word %d.\n",i) ;
	    }
	    turfDataWord=dataWord>>8;
	    turfOtherWord=dataWord&0xff;
	    if(i==10 && verbosity>1 && printToScreen) printf("Event %d\n",evNo);
	    if(i>=10 && i<=18 && verbosity>1 && printToScreen) printf("%d\t%#x\t%#x\t%#x\n",i,dataWord,turfDataWord,turfOtherWord); 
//	    if(verbosity) 
//	    if(verbosity) printf("%#x\t%#x\t%#x\n",turfDataWord,turfOtherWord); 
	    switch(i){
		case 0: 
		    theEvent.header.turfio.trigType=turfDataWord; 
		    break;
		case 1:
		    theEvent.header.turfio.trigNum=turfDataWord; 
		    break;
		case 2: 
		    theEvent.header.turfio.trigNum+=(turfDataWord<<8); 
		    break;
		case 3: 
		    theEvent.header.turfio.trigNum+=(turfDataWord<<16); 
		    break;
		case 4: 
		    theEvent.header.turfio.trigTime=turfDataWord; 
		    break;
		case 5: 
		    theEvent.header.turfio.trigTime+=(turfDataWord<<8); 
		    break;
		case 6: 
		    theEvent.header.turfio.trigTime+=(turfDataWord<<16); 
		    break;
		case 7: 
		    theEvent.header.turfio.trigTime+=(turfDataWord<<24); 
		    break;
		case 8: 
		    theEvent.header.turfio.ppsNum=(turfDataWord); 
		    break;
		case 9: 
		    theEvent.header.turfio.ppsNum+=(turfDataWord<<8); 
		    break;
		case 10: 
		    theEvent.header.turfio.ppsNum+=(turfDataWord<<16); 
		    break;
		case 11: 
		    theEvent.header.turfio.ppsNum+=(turfDataWord<<24); 
		    break;
		case 12: 
		    theEvent.header.turfio.c3poNum=(turfDataWord); 
		    break;
		case 13: 
		    theEvent.header.turfio.c3poNum+=(turfDataWord<<8); 
		    break;
		case 14: 
		    theEvent.header.turfio.c3poNum+=(turfDataWord<<16); 
		    break;
		case 15: 
		    theEvent.header.turfio.c3poNum+=(turfDataWord<<24); 
		    break;
		default: 
		    break;
	    }
	    
	}

	if(writeOutC3p0Nums && (evNo%4==0)) writeC3P0Nums();
	
	if(verbosity && printToScreen) {
/* 	    unsigned long fullTrigNum= */
/* 		(theEvent.header.turfio.trigNumByte1)+(theEvent.header.turfio.trigNumByte2<<8)+(theEvent.header.turfio.trigNumByte3<<16); */
/* 	    unsigned long fullTrigTime= */
/* 		(theEvent.header.turfio.trigTimeByte1)+(theEvent.header.turfio.trigTimeByte2<<8)+(theEvent.header.turfio.trigTimeByte3<<16)+(theEvent.header.turfio.trigTimeByte4<<24); */
/* 	    unsigned long fullPPSNum= */
/* 		(theEvent.header.turfio.ppsNumByte1)+(theEvent.header.turfio.ppsNumByte2<<8)+(theEvent.header.turfio.ppsNumByte3<<16)+(theEvent.header.turfio.ppsNumByte4<<24); */
	    
/* 	    unsigned long fullC3P0Num=theEvent.header.turfio.c3poByte1; */
/* 	    fullC3P0Num+=(theEvent.header.turfio.c3poByte2<<8); */
/* 	    fullC3P0Num+=(theEvent.header.turfio.c3poByte3<<16); */
/* 	    fullC3P0Num+=(theEvent.header.turfio.c3poByte4<<24); */
//		(theEvent.header.turfio.c3poByte1)+(theEvent.header.turfio.c3poByte2<<8)+(theEvent.header.turfio.c3poByte3<<16)+(theEvent.header.turfio.c3poByte4<<24);
	    double frequency=(double)theEvent.header.turfio.c3poNum;
	    frequency/=1e6;
	    double period=1e3/frequency;
	    
	    printf("trigType:\t%d\ntrigNum:\t%lu\ntrigTime:\t%lu\nppsNum:\t%lu\nc3poNum:\t%lu  (%3.1f Mhz or %3.1f ns)\n",
		   theEvent.header.turfio.trigType,
		   theEvent.header.turfio.trigNum,
		   theEvent.header.turfio.trigTime,
		   theEvent.header.turfio.ppsNum,
		   theEvent.header.turfio.c3poNum,
		   frequency,period);
		   
	}	    
/* 	if(verbosity && printToScreen){ */
/* 	    printf("raw: %hu\nL3A: %hu\n1PPS: %lu\ntime: %lu\nint: %lu\n", */
/* 		   theEvent.header.turfio.rawtrig,theEvent.header.turfio.L3Atrig,theEvent.header.turfio.pps1,theEvent.header.turfio.time, */
/* 		   theEvent.header.turfio.interval); */
/* 	    for(i=8;i<16;++i) */
/* 		printf("Rate[%d][%d]=%d\n",i/4-2,i%4,theEvent.header.turfio.rate[i/4-2][i%4]); */
/* 	} */
    }
    return status;
} 

void writeC3P0Nums() {

/*     FILE *fp=0; */
/*     static int firstTime=1; */
/*     if(firstTime) { */
/* 	fp = fopen ("c3p0File.txt", "wt"); */
/* 	firstTime=0; */
/*     } */
//    std::ofstream Output("testFile.txt", ios_base::app);
    
    unsigned long fullC3P0Num=theEvent.header.turfio.c3poNum;
//    fprintf(stderr,"%lu\t%lx\n",fullC3P0Num,fullC3P0Num);
//    Output << fullC3P0Num << endl;    
    fprintf(stderr,"%lu\n",fullC3P0Num);
//    Output.close();

}
