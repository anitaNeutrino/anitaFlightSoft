
/*  \file testSdProg.c
    \brief Test program to try and read data from the SURFs using Patrick's SURF driver.
   March 2008 rjn@hep.ucl.ac.uk
*/

// Standard Includes
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/pci.h>


// Anita includes
//#include "includes/anitaFlight.h"

//Surf Driver Include
#include "surfDriver_ioctl.h"

//Definitions
#define EVT_RDY  0004
#define LAB_F 0x4000
#define START_B 0x4000
#define STOP_B  0x8000
#define DATA_MASK 0xfff
#define LOWER16 0xffff
#define UPPER16 0xffff0000
#define HITBUS 0x1000
#define MAX_EVENTS_PER_DIR 1000




#define GetUpper16(A) (((A)&UPPER16)>>16)
#define GetLower16(A) ((A)&LOWER16)
#define GetHitBus(A) (((A)&HITBUS)>>12)
#define GetStartBit(A) (((A)&START_B)>>14)
#define GetStopBit(A) (((A)&STOP_B)>>15)

//Constants
#define N_SAMP 260
#define N_SAMP_EFF 256
#define N_CHAN 9
#define N_RFCHAN 8
#define N_CHIP 4
#define N_RFTRIG 32 // 32 RF trigger channels per SURF board


typedef enum __SURF_control_act {  
    SurfClearAll,
    SurfClearEvent,
    SurfClearHk,
    SurfHkMode,
    SurfDataMode,
    SurfEnableInt,
    SurfDisableInt
} SurfControlAction_t ;


typedef enum __SURF_status_flag {  
    SurfEventReady,
    SurfIntState,
    SurfBusInfo
} SurfStatusFlag_t ;


typedef enum __TURF_control_act { 
    SetTrigMode,
    RprgTurf,
    TurfLoadRam,
    SendSoftTrg,
    RFCBit,
    RFCClk,
//    EnableRfTrg,
    EnablePPS1Trig,
    EnablePPS2Trig,
//    EnableSoftTrig, //Always enabled
    TurfClearAll
} TurfControlAction_t ;

typedef enum __TURF_trigger_mode {
    TrigNone = 0,
    TrigPPS1 = 0x2,
    TrigPPS2 = 0x4
} TriggerMode_t;

typedef enum {
    ACQD_E_OK = 0,
    ACQD_E_LTRIG = 0x100,
    ACQD_E_EVNOD,
    ACQD_E_LABD,
    ACQD_E_SCLD,
    ACQD_E_RFPWD,
    ACQD_E_PLXBUSREAD,
    ACQD_E_TRANSFER,
    ACQD_E_UNNAMED
} AcqdErrorCode_t ;

typedef struct {
    int bus;
    int slot;
} BoardLocStruct_t;

typedef struct {
    int dState;  // Last position input
    int iState;  // Integrator state
} DacPidStruct_t;

//Function Declarations
void printBinary(int number);
char *surfStatusFlagAsString(SurfStatusFlag_t flag);
char *surfControlActionAsString(SurfControlAction_t action);
int getSurfStatusFlag(int surfFd, SurfStatusFlag_t flag) ;
int setSurfControl(int surfFd, SurfControlAction_t action);
int setDACThresholds(int surfFd, unsigned int thresholdArray[]);
int setGlobalDACThreshold(int surfFd, int thresVal);
void quickThresholdScan(int surfFd);
void otherThresholdScan(int surfFd);
int simpleReadHk(int surfFd, unsigned int hkVals[]);
void quickTimeTest(int surfFd);
void printSlotAndBus(int surfFd);
inline unsigned long readTSC();


//Global Variables
#define NUM_READS 9*260*12*100

int printToScreen=1;
int verbosity=0;


int main(int argc, char **argv) {
    int surfFd=0;
    int retVal=0;
    unsigned long start,opened;

    if(argc<3) {
      printf("Usage: %s <bus> <slot>\n",argv[0]);
      return -1;
    }
    int bus=atoi(argv[1]);
    int slot=atoi(argv[2]);
	
    char devName[180];
    sprintf(devName,"/dev/%d:%d",bus,slot);

    start=readTSC();
    
    surfFd = open(devName, O_RDWR);
    if (surfFd < 0) {
      fprintf(stderr,"Can't open %s\n",devName);
      exit(1);
    }
    opened=readTSC();
    
    //Send Clear All
    setSurfControl(surfFd,SurfClearAll);

    quickThresholdScan(surfFd);
//    otherThresholdScan(surfFd);
    printSlotAndBus(surfFd);
  
    retVal=close(surfFd);
    if(retVal<0) {
	fprintf(stderr,"Problem closing SURF -- %s\n",
		strerror(errno));
	//RJN Syslog here
    }
	

    return 0;
}


void quickTimeTest(int surfFd) {
    int i,j;    
    unsigned int hkVals[72]={0};
    unsigned long beforeDac,afterDac,beforeRead,afterRead;
    unsigned int thresholdArray[N_RFTRIG];

    for(i=0;i<32;i++) 
	thresholdArray[i]=100+i;
    //Set DAC Thresholds
    beforeDac=readTSC();
    setDACThresholds(surfFd,thresholdArray);
    afterDac=readTSC();

    //Send some clears
    setSurfControl(surfFd,SurfClearHk);
//    setSurfControl(surfFd,SurfClearEvent);
//    setSurfControl(surfFd,SurfClearEvent);

      //Read Hk
    beforeRead=readTSC(); 
    simpleReadHk(surfFd,hkVals);
    afterRead=readTSC();


    //Print some stuff
    for(i=0;i<72;i+=8) {
	printf("%d\t",i);
	for(j=0;j<8;j++) {
	    printf("%8.8X ", hkVals[i+j]);
	}
	printf("\n");
    }
    printf("DAC Setting took %lu %lu %lu cycles\n",afterDac,beforeDac,afterDac-beforeDac);
    printf("Hk Reading took %lu %lu %lu cycles\n",afterRead,beforeRead,afterRead-beforeRead);
}

void quickThresholdScan(int surfFd) {
    int globVal=0;
    int trigChan=0;
    unsigned int hkVals[72];
    for(globVal=0;globVal<4096;globVal+=1) {
	setGlobalDACThreshold(surfFd,globVal);
	usleep(30000);
	simpleReadHk(surfFd,hkVals);
	
	//Check thresholds are correct
	
	//Check scalers look OK.
	int readBackErrCount=0;
//	for(i=0;i<72;i++) 
//	    printf("%d %d 0x%x\t(%d)\n",globVal,i,hkVals[i],(hkVals[i]&0xffff));
	for(trigChan=0;trigChan<N_RFTRIG;trigChan++) {

	    int dacVal=hkVals[trigChan+N_RFTRIG]&0xffff;
	    if(dacVal!=globVal) {
		readBackErrCount++;
		printf("Error: Chan %d (%d != %d) -- hkVal 0x%x\n",trigChan,dacVal,globVal,hkVals[trigChan+N_RFTRIG]);
	    }
	    int scalerVal=hkVals[trigChan]&0xffff;
	    if(trigChan==0) {
		printf("Chan %d -- Threshold %d (%d) -- Scaler %d\n",
		       trigChan,dacVal,globVal,scalerVal);
	    }
	}
	

    }    
}


void otherThresholdScan(int surfFd) {
    unsigned int thresholdArray[32];
    int trigChanSet=0,trigChanTest,i;
    int setDacVal=0;
    unsigned int hkVals[72];
    for(trigChanSet=0;trigChanSet<N_RFTRIG;trigChanSet++) {
	memset(thresholdArray,0,sizeof(int)*32);
	for(setDacVal=0;setDacVal<4096;setDacVal+=64) {
	    thresholdArray[trigChanSet]=setDacVal;
	    setDACThresholds(surfFd,thresholdArray);
	    usleep(30000);
	    simpleReadHk(surfFd,hkVals);
	
	    //Check thresholds are correct
	    
	    //Check scalers look OK.
	    int readBackErrCount=0;
	for(i=0;i<72;i++) 
	  printf("%d %d 0x%x\t(%d)\n",globVal,i,hkVals[i],(hkVals[i]&0xffff));
	    /* for(trigChanTest=0;trigChanTest<N_RFTRIG;trigChanTest++) { */
	    /* 	int dacVal=hkVals[trigChanTest+N_RFTRIG]&0xffff; */
	    /* 	if(dacVal!=thresholdArray[trigChanTest]) { */
	    /* 	    readBackErrCount++; */
	    /* 	    printf("Error: Chan %d (%d != %d) -- hkVal 0x%x\n",trigChanTest,dacVal,thresholdArray[trigChanTest],hkVals[trigChanTest+N_RFTRIG]); */
	    /* 	} */
	    /* 	int scalerVal=hkVals[trigChanTest]&0xffff; */
	    /* 	if(trigChanTest==trigChanSet) { */
	    /* 	    printf("Chan %d -- Threshold %d (%d) -- Scaler %d\n", */
	    /* 		   trigChanTest,dacVal,thresholdArray[trigChanTest],scalerVal); */
	    /* 	} */
		
	    /* } */
	}	
    }        
}


int simpleReadHk(int surfFd, unsigned int hkVals[])
{
    int numBytes=0;
    //Set to Hk mode
    setSurfControl(surfFd,SurfHkMode);
    numBytes = read(surfFd, hkVals, 72*sizeof(int)); 
    setSurfControl(surfFd,SurfClearHk);   
    if(numBytes!=72*sizeof(int)) {
	fprintf(stderr,"Error reading SURF housekeeping %s\n",
		strerror(errno));
	//RJN -- Insert syslog here

	return -1;
    }
    return 0;    
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




//Status Fucntions
int getSurfStatusFlag(int surfFd, SurfStatusFlag_t flag)
{
    unsigned short value;
    int retVal=0,returnVal=0;
    switch(flag) {
	case SurfEventReady : 
	    retVal=ioctl(surfFd, SURF_IOCEVENTREADY);
	    returnVal=retVal;
	    break ; 
	case SurfIntState:
	    retVal=ioctl(surfFd,SURF_IOCQUERYINT);
	    returnVal=retVal;
	    break;
	case SurfBusInfo:
	    retVal=ioctl(surfFd,SURF_IOCGETSLOT,&value);
	    returnVal=value;
	    break;
	default:
	    if(printToScreen)
		fprintf(stderr,"getSurfStatusFlag: undefined status flag, %d\n", flag) ;
	    return -1 ;
    } 
    if(retVal<0) {
	fprintf(stderr,"getSurfStatusFlag %s (%d) had error: %s\n",surfStatusFlagAsString(flag),flag,strerror(errno));
	//RJN -- Insert syslog here
	return retVal;
    }        
    return returnVal;
}

//Control Fucntions
int setSurfControl(int surfFd, SurfControlAction_t action)
{
    //This just sets levels
    int retVal=0;
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
	    if(printToScreen)
		fprintf(stderr,"setSurfControl: undefined action, %d\n", action) ;
	    return -1 ;
    }
    if (verbosity>1 && printToScreen) 
	printf(" setSurfControl: action %s\n", 
	       surfControlActionAsString(action)) ; 
    if(retVal!=0) {
	//RJN insert syslog
	fprintf(stderr,"setSurfControl: action %s resulted in error %s\n:",surfControlActionAsString(action),strerror(errno));
    }
    return retVal;
}


void printBinary(int number) {
    int i,bit;
    for (i=31; i>=0; i--) {
	printf("%2d ",i);
    }
    printf("\n");
    for (i=31; i>=0; i--) {
	bit = ((number >> i) & 1);
	printf(" %d ",bit);
    }
    printf("\n");
}
 
inline unsigned long readTSC()
{
    unsigned long tsc;
    asm("rdtsc":"=A"(tsc));
    return tsc;
}

int setDACThresholds(int surfFd, unsigned int thresholdArray[]) {
    if(verbosity && printToScreen) printf("Writing thresholds to SURFs\n");
    int dac,retVal=0;
    unsigned int outputBuffer[N_RFTRIG+2];
    for(dac=0;dac<N_RFTRIG+2;dac++) {
	if(dac<N_RFTRIG) {
	    //This is the part that sets the dacs
	    outputBuffer[dac]=(dac<<16) + 
		(thresholdArray[dac]&0xffff);

	}
	else {
	    //This is the part that sets the SURF trigger channel mask
	    outputBuffer[dac]=(dac<<16) + 0;	
	}
    }
    retVal = write(surfFd, outputBuffer, (N_RFTRIG+2)*sizeof(unsigned int));
    if(retVal!=(N_RFTRIG+2)*sizeof(unsigned int)) {
	fprintf(stderr,"Error writing to SURF (fd = %d), %s",
		surfFd,strerror(errno));
	//RJN -- Insert syslog here
	return -1; //Maybe add an AcqdErrorCode_t
    }   
    return 0;
}


int setGlobalDACThreshold(int surfFd, int threshVal) {
    if(verbosity && printToScreen) printf("Writing thresholds to SURFs\n");
    int dac,retVal=0;
    unsigned int outputBuffer[N_RFTRIG+2];
    for(dac=0;dac<N_RFTRIG+2;dac++) {
	if(dac<N_RFTRIG) {
	    //This is the part that sets the dacs
	    outputBuffer[dac]=(dac<<16) + 
		(threshVal);

	}
	else {
	    //This is the part that sets the SURF trigger channel mask
	    outputBuffer[dac]=(dac<<16) + 0;	
	}
    }
    retVal = write(surfFd, outputBuffer, (N_RFTRIG+2)*sizeof(unsigned int));
    if(retVal!=(N_RFTRIG+2)*sizeof(unsigned int)) {
	fprintf(stderr,"Error writing to SURF (fd = %d), %s",
		surfFd,strerror(errno));
	//RJN -- Insert syslog here
	return -1; //Maybe add an AcqdErrorCode_t
    }   
    return 0;
}

void printSlotAndBus(int surfFd)
{
    int slot=getSurfStatusFlag(surfFd,SurfBusInfo);
     printf("Is at %d:%x.%d\n",
	    (slot&0xFF00)>>8,
	    PCI_SLOT(slot&0xFF),
	    PCI_FUNC(slot&0xFF));
}

