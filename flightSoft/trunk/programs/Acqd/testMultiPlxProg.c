/*  \file barMapTest.c
    \brief Test program to try and play with the PLX PCI 9030 chip
   Feb 2006 rjn@mps.ohio-state.edu
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

// Plx includes
#include "PciRegs.h"
#include "PlxApi.h"
/*  #include "PlxInit.h" */
/*  #include "RegDefs.h" */
#include "Reg9030.h"

// Anita includes
//#include "includes/anitaFlight.h"



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

typedef PLX_DEVICE_OBJECT PlxDevObject_t;
typedef PLX_NOTIFY_OBJECT PlxNotifyObject_t;
typedef PLX_DEVICE_KEY PlxDevKey_t;
typedef PLX_STATUS PlxStatus_t;
typedef PLX_INTERRUPT PlxInterrupt_t;

typedef enum __SURF_control_act {  
    SurfClearAll,
    SurfClearEvent,
    SurfClearHk,
    RDMode,
    WTMode,
    DTRead,
    IntEnb,
    DACLoad,
    CHGChp
} SurfControlAction_t ;

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

      
//Global Variables
U16 Data_buf[8100];
__volatile__ int *barAddr;
void readRegisters(PlxDevObject_t *surfHandle);
void printBinary(int number);

#define NUM_READS 9*260*12*100

int printToScreen=1;
int verbosity=1;

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


PlxStatus_t setSurfControl(PlxDevObject_t *surfHandlePtr, SurfControlAction_t action)
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
    PlxStatus_t   rc ;

    // Read current GPIO state
//    U32 gpioVal=PlxPci_PlxRegisterRead(surfHandlePtr, PCI9030_GP_IO_CTRL, &rc) ; 

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
	    if(printToScreen)
		fprintf(stderr,"setSurfControl: undefined action, %d\n", action) ;
	    return ApiFailed ;
    }
    if (verbosity>1 && printToScreen) 
	printf(" setSurfControl: action %s, gpio= %o\n", 
	       surfControlActionAsString(action), gpioVal) ; 

    if ((rc = PlxPci_PlxRegisterWrite(surfHandlePtr, PCI9030_GP_IO_CTRL, gpioVal ))
	!= ApiSuccess) {
	if(printToScreen)
	    fprintf(stderr,"setSurfControl: failed to set GPIO to %o.\n", gpioVal) ; 
	return rc ;
    }
    if (SurfClearHk < action && action < DACLoad) return rc ; 
    //Reset GPIO to base val
    return PlxPci_PlxRegisterWrite(surfHandlePtr, PCI9030_GP_IO_CTRL, baseVal ) ;

}
__volatile__ int *barMapAddr[12];


PlxStatus_t setBarMap(PlxDevObject_t *surfHandle,int numSurfs) {
    PlxStatus_t rc=0;
    U32 *addrVal=0;
    int surf=0;

    for(surf=0;surf<numSurfs;surf++) {
      addrVal=0;
      rc=PlxPci_PciBarMap(&surfHandle[surf],2,(VOID**)&addrVal);
      if(rc!=ApiSuccess) {
	fprintf(stderr,"Unable to map PCI bar 2 on SURF  (%d)",rc);
      }
      barMapAddr[surf]=(int*)addrVal;
      printf("Bar Addr is %x\t%x\n",(int)barMapAddr[surf],*addrVal);
    }
    return rc;
}


PlxStatus_t unsetBarMap(PlxDevObject_t *surfHandle,int numSurfs) {
    PlxStatus_t rc=0;
    int surf=0;

    for(surf=0;surf<numSurfs;surf++) {
      U32 *addrVal=(U32*)barMapAddr[surf];
      rc=PlxPci_PciBarUnmap(&surfHandle[surf],(VOID**)&addrVal);
      if(rc!=ApiSuccess) {
	fprintf(stderr,"Unable to unmap PCI bar 2 on SURF  (%d)",rc);
      }
    }
    return rc;
}


PlxStatus_t setSingleBarMap(PlxDevObject_t *surfHandle,int surfNum) {
    PlxStatus_t rc=0;
    U32 *addrVal;
    
    rc=PlxPci_PciBarMap(&surfHandle[surfNum],2,(VOID**)&addrVal);
    if(rc!=ApiSuccess) {
      fprintf(stderr,"Unable to map PCI bar 2 on SURF %d  (%d)",rc,surfNum);
    }
    barMapAddr[surfNum]=(int*)addrVal;
    printf("Bar Addr %d is %x\t%x\n",surfNum,(int)barMapAddr[surfNum],*addrVal);    
    return rc;
}


PlxStatus_t unsetSingleBarMap(PlxDevObject_t *surfHandle,int surfNum) {
    PlxStatus_t rc=0;
    
    U32 *addrVal=(U32*)barMapAddr[surfNum];
    rc=PlxPci_PciBarUnmap(&surfHandle[surfNum],(VOID**)&addrVal);
    if(rc!=ApiSuccess) {
      fprintf(stderr,"Unable to unmap PCI bar 2 on SURF %d  (%d)",rc,surfNum);
    }
    return rc;
}


void setDACThresholds(PlxDevObject_t *surfHandle) {
    PlxStatus_t rc;
    int dac;
    int buf ;
    if(verbosity && printToScreen) printf("Writing thresholds to SURFs\n");
    int thresholdArray[32];
    for(dac=0;dac<32;dac++) 
	thresholdArray[dac]=1000+dac;



    for (dac=0 ; dac<N_RFTRIG ; dac++) {
	buf = (dac<<16) + (thresholdArray[dac]&0xfff) ;
	rc=PlxPci_PciBarSpaceWrite(surfHandle, 2, 0x0, 
				   &buf, 4, BitSize32,TRUE);
	if(rc!=ApiSuccess) {

	    if(printToScreen)
		fprintf(stderr,
			"Error writing DAC Val, SURF %d - DAC %d (%d)\n",
			1,dac,rc);
	}
    }
    
    //Insert SURF trigger channel masking here
    for (dac=N_RFTRIG ; dac<N_RFTRIG+2 ; dac++) {
	buf = (dac<<16) + 0;

	rc=PlxPci_PciBarSpaceWrite(surfHandle, 2, 0x0, 
				   &buf, 4, BitSize32, TRUE);
	if(rc!=ApiSuccess) {
	    
	    if(printToScreen)
		fprintf(stderr,
			"Error writing Surf Ant Trig Mask, SURF %d - DAC %d (%d)\n",
			1,dac,rc);
	}
	
    }
    
    rc=setSurfControl(surfHandle,DACLoad);
    if(rc!=ApiSuccess) {
	if(printToScreen)
	    fprintf(stderr,"Error loading DAC Vals, SURF %d (%d)\n",
		    1,rc);
    }
    else if(printToScreen && verbosity>1)
	printf("DAC Vals set on SURF %d\n",1);

		   	    
}

inline unsigned long readTSC();


int initializeDevices(PlxDevObject_t *surfHandles, int numSurfs) 
{
  int surfSlots[12]={11,13,14,8,9,11,14,15,8,-1,-1,-1};
  int surfBuses[12]={10,10,10,10,9,9,9,9,9,-1,-1,-1};

  int surfNum,countSurfs=0;
  U8  numDevices,i ;
  PlxDevKey_t tempKey;
  PlxStatus_t rc;
  numDevices = 0;
  for(i=0;i<20;i++) {
    
    tempKey.bus       = PCI_FIELD_IGNORE;
    tempKey.slot      = PCI_FIELD_IGNORE;
    tempKey.function      = PCI_FIELD_IGNORE;
    tempKey.VendorId        = PCI_FIELD_IGNORE;
    tempKey.DeviceId        = PCI_FIELD_IGNORE;
    tempKey.SubVendorId = PCI_FIELD_IGNORE;
    tempKey.SubDeviceId = PCI_FIELD_IGNORE;
    tempKey.Revision = PCI_FIELD_IGNORE;
    if (PlxPci_DeviceFind(&tempKey, i) != ApiSuccess)
      break;
    printf("Found device at (Bus %d -- Slot %X)\n",
	   tempKey.bus,tempKey.slot);  
    numDevices=i+1;
  }
  printf("initializeDevices: found %d PLX devices.\n", numDevices) ;

   /* Initialize SURFs */    
  for(surfNum=0;surfNum<numSurfs;surfNum++) {
    if(surfSlots[surfNum]<0 || surfBuses[surfNum]<0) continue;
    i=0;
    tempKey.bus       = surfBuses[surfNum];
    tempKey.slot      = surfSlots[surfNum];
    tempKey.function      = PCI_FIELD_IGNORE;
    tempKey.VendorId        = PCI_FIELD_IGNORE;
    tempKey.DeviceId        = PCI_FIELD_IGNORE;
    tempKey.SubVendorId = PCI_FIELD_IGNORE;
    tempKey.SubDeviceId = PCI_FIELD_IGNORE;
    tempKey.Revision = PCI_FIELD_IGNORE;
    if (PlxPci_DeviceFind(&tempKey, i) != ApiSuccess) {
      //syslog
      printf("Couldn't find SURF %d (Bus %d -- Slot %X)\n",
	     surfNum+1,surfBuses[surfNum],surfSlots[surfNum]);
    }
    else {	    
      // Got a SURF
      rc=PlxPci_DeviceOpen(&tempKey,&surfHandles[countSurfs]);
      if ( rc!= ApiSuccess) {
	fprintf(stderr,"Error opening SURF device %d\n",rc);
	return -1 ;
      }		
      PlxPci_DeviceReset(&surfHandles[countSurfs]) ;
      countSurfs++;    
    }
  }

}


int main(int argc, char **argv) {
    PlxDevObject_t surfHandle[12];
    U32  i ;
    PlxStatus_t rc;
    int surf=0;
    int numSurfs=1;
    unsigned long start,opened,beforeDac,afterDac,beforeRead,afterRead;
    start=readTSC();
//    exit(0);
    initializeDevices(surfHandle,1);
    opened=readTSC();
    

    
    setBarMap(surfHandle,numSurfs);
    for(surf=0;surf<numSurfs;surf++) {
      setSurfControl(&surfHandle[surf],SurfClearAll);
      beforeDac=readTSC();
      setDACThresholds(&surfHandle[surf]);
      afterDac=readTSC();
      setSurfControl(&surfHandle[surf],SurfClearHk);
      setSurfControl(&surfHandle[surf],SurfClearEvent);
      setSurfControl(&surfHandle[surf],SurfClearAll);
    }


    for(surf=0;surf<numSurfs;surf++) {
      //      setSingleBarMap(surfHandle,surf);
      unsigned int hkVals[72]={0};
      printf(" GPIO register contents = %o\n",
	     PlxPci_PlxRegisterRead(&surfHandle[surf], PCI9030_GP_IO_CTRL, &rc)) ; 	
      
      setSurfControl(&surfHandle[surf],RDMode);
      __volatile__ int *hkData=barMapAddr[surf];
      beforeRead=readTSC();
//    memcpy(hkData,hkVals,72*sizeof(int));
      for(i=0;i<72;i++) {	
	//	dataWord=;	
	//	PlxPci_PciBarSpaceRead(&surfHandle,2,0xi, &dataWord, 4, BitSize32, TRUE);
	hkVals[i]=*hkData++;
      }
      afterRead=readTSC();
      unsigned int upperWord=(hkVals[0]&0xffff0000)>>16;
      printf("Upper word %0x (SURF num %d)\n",upperWord,(upperWord&0xf));
      int j=0;
      for(i=0;i<72;i+=8) {
	printf("%d ",i);
	for(j=i;j<i+8;j++)
	  printf("%0x ",hkVals[j]);
	printf("\n");
      }
      printf("DAC Setting took %lu cycles\n",afterDac-beforeDac);
      printf("Hk Reading took %lu cycles\n",afterRead-beforeRead);
      //      unsetSingleBarMap(surfHandle,0);
    }

    unsetBarMap(surfHandle,numSurfs);

    return 0;
}


void readRegisters(PlxDevObject_t *surfHandle) {

    PlxStatus_t rc;
    printf(" GPIO register contents = %o\n",
	   PlxPci_PlxRegisterRead(surfHandle, PCI9030_GP_IO_CTRL, &rc)) ; 

    printf(" Int reg contents = %x\n",PlxPci_PlxRegisterRead(surfHandle, PCI9030_INT_CTRL_STAT, &rc)) ;


    printf(" EEPROM  Initialization Control register contents = %x\n",
	   PlxPci_PlxRegisterRead(surfHandle, PCI9030_EEPROM_CTRL, &rc)) ; 

    printf(" EEPROM  Initialization Control register contents = %x\n",
	   PlxPci_PlxRegisterRead(surfHandle, PCI9030_EEPROM_CTRL, &rc)) ; 


    printf(" PCI Base Address 2 register contents = %x\n",
	   PlxPci_PlxRegisterRead(surfHandle, PCI9030_PCI_BASE_2, &rc)) ; 

    printf(" Local Address Space 0 Range -- Register contents \n\t\t= %x\n",
	   PlxPci_PlxRegisterRead(surfHandle, PCI9030_RANGE_SPACE0, &rc)) ; 
    
    printf(" Local Bus Address Space 0 Local Base Address (Remap) -- Register contents \n\t\t= %x\n",
	   PlxPci_PlxRegisterRead(surfHandle, PCI9030_REMAP_SPACE0, &rc)) ; 


    printf(" Local Address Space 0 Bus Region Descriptor --  Register contents \n\t\t= %x\n",
	   PlxPci_PlxRegisterRead(surfHandle, PCI9030_DESC_SPACE0, &rc)) ;


    printf("\n");
    printf("\n");
    printf("PCICR\n");    
    printBinary(PlxPci_PlxRegisterRead(surfHandle, PCI9030_COMMAND, &rc)) ; 
    printf("PCIBAR2\n");    
    printBinary(PlxPci_PlxRegisterRead(surfHandle, PCI9030_PCI_BASE_2, &rc)) ; 
    printf("CNTL\n");
    printBinary(PlxPci_PlxRegisterRead(surfHandle, PCI9030_EEPROM_CTRL, &rc)) ;
    printf("\n");
    printf("LAS0RR -- %#x\n",PlxPci_PlxRegisterRead(surfHandle, PCI9030_RANGE_SPACE0, &rc)) ; 
    printBinary(PlxPci_PlxRegisterRead(surfHandle, PCI9030_RANGE_SPACE0, &rc)) ; 
    printf("\n");
    printf("LAS0BA\n");
    printBinary(PlxPci_PlxRegisterRead(surfHandle, PCI9030_REMAP_SPACE0, &rc)) ;
    printf("\n");
    printf("LAS0BRD\n");
    printBinary(PlxPci_PlxRegisterRead(surfHandle, PCI9030_DESC_SPACE0, &rc)) ;

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
