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
//#include "Acqd.h"


typedef HANDLE PlxHandle_t;
typedef DEVICE_LOCATION PlxDevLocation_t;
typedef RETURN_CODE PlxReturnCode_t;


//Global Variables
static IOP_SPACE IopSpace = IopSpace0;	// for 9030 chip
U16 Data_buf[8100];
__volatile__ int *barAddr;
void readRegisters(PlxHandle_t surfHandle);
void printBinary(int number);

#define NUM_READS 9*260*12*100

int main(int argc, char **argv) {
    int regVal=0;
    PlxHandle_t surfHandle;
    PlxDevLocation_t surfLocation;
    int  dataWord=0,dataWord2;
    int dataArray[50];
    int j,countSurfs=0;
    int ret;
    U32  numDevices,i ;
    PlxDevLocation_t tempLoc;
    PlxReturnCode_t rc;
    PCI_MEMORY  commonBuffer;
    //  U32  n=0 ;  /* this is the numebr of board from furthest from CPU. */

    tempLoc.BusNumber       = (U8)-1;
    tempLoc.SlotNumber      = (U8)-1;
    tempLoc.VendorId        = (U16)-1;
    tempLoc.DeviceId        = (U16)-1;
    tempLoc.SerialNumber[0] = '\0';

    

    /* need to go through PlxPciDeviceFind twice.  1st for counting devices
       with "numDevices=FIND_AMOUNT_MATCHED" and 2nd to get device info. */

//    numDevices = FIND_AMOUNT_MATCHED;
//    if (PlxPciDeviceFind(&tempLoc, &numDevices) != ApiSuccess ||
//	numDevices > 12) return -1 ;
//    printf("init_device: found %d PLX devices.\n", numDevices) ;

//    exit(0);
    tempLoc.BusNumber=2;
    tempLoc.SlotNumber=0xe;
    i=0;
    
    if ((rc=PlxPciDeviceFind(&tempLoc, &i)) != ApiSuccess) {
	printf("Return code %d\n",rc);
	return -1 ;

    }
    printf("init_device: device found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
	   tempLoc.DeviceId, tempLoc.VendorId, tempLoc.SerialNumber, 
	   tempLoc.BusNumber, tempLoc.SlotNumber);
//    exit(0);
    surfLocation=tempLoc;
    rc=PlxPciDeviceOpen(&surfLocation,&surfHandle);
    if(rc!=ApiSuccess) {
	printf("Failed to Open PLX Device: %d\n",rc);
	return -1;
    }
       
    PlxPciBoardReset(surfHandle) ;
    
    U16 *sp;
    U32 total_bytes_read = 0;
    


    readRegisters(surfHandle);
    regVal=PlxRegisterRead(surfHandle,PCI9030_RANGE_SPACE0,&rc);
    regVal&=~0x4;
    regVal|=0x9;
    PlxRegisterWrite(surfHandle,PCI9030_RANGE_SPACE0,regVal);
    readRegisters(surfHandle);


    U32 locTrig=0022222062;
    PlxRegisterWrite(surfHandle,PCI9030_GP_IO_CTRL,locTrig);
    
    while(1) {
	regVal=PlxRegisterRead(surfHandle,PCI9030_GP_IO_CTRL,&rc);
	printf("GPIO %o\n",regVal);
	sleep(1);
    }
	
    for(i=0;i<NUM_READS;i++) {
      PlxBusIopRead(surfHandle,IopSpace0,0x0,TRUE, &dataWord, 2, BitSize16);
//        dataWord=*(barAddr);
//      dataWord2=*(barAddr+4);
        if(i<20)
            printf("%d\t%d\n",i,dataWord);
    }

}


void readRegisters(PlxHandle_t surfHandle) {

    PlxReturnCode_t rc;
    unsigned int val;
    printf(" GPIO register contents = %x\n",
	   PlxRegisterRead(surfHandle, PCI9030_GP_IO_CTRL, &rc)) ; 

    printf(" Int reg contents = %x\n",PlxRegisterRead(surfHandle, PCI9030_INT_CTRL_STAT, &rc)) ;


    printf(" EEPROM  Initialization Control register contents = %x\n",
	   PlxRegisterRead(surfHandle, PCI9030_EEPROM_CTRL, &rc)) ; 

    printf(" EEPROM  Initialization Control register contents = %x\n",
	   PlxRegisterRead(surfHandle, PCI9030_EEPROM_CTRL, &rc)) ; 


    printf(" PCI Base Address 2 register contents = %x\n",
	   PlxRegisterRead(surfHandle, PCI9030_PCI_BASE_2, &rc)) ; 

    printf(" Local Address Space 0 Range -- Register contents \n\t\t= %x\n",
	   PlxRegisterRead(surfHandle, PCI9030_RANGE_SPACE0, &rc)) ; 
    
    printf(" Local Bus Address Space 0 Local Base Address (Remap) -- Register contents \n\t\t= %x\n",
	   PlxRegisterRead(surfHandle, PCI9030_REMAP_SPACE0, &rc)) ; 


    printf(" Local Address Space 0 Bus Region Descriptor --  Register contents \n\t\t= %x\n",
	   PlxRegisterRead(surfHandle, PCI9030_DESC_SPACE0, &rc)) ;


    printf("\n");
    printf("\n");
    printf("PCICR\n");    
    printBinary(PlxRegisterRead(surfHandle, PCI9030_COMMAND, &rc)) ; 
    printf("PCIBAR2\n");    
    printBinary(PlxRegisterRead(surfHandle, PCI9030_PCI_BASE_2, &rc)) ; 
    printf("CNTL\n");
    printBinary(PlxRegisterRead(surfHandle, PCI9030_EEPROM_CTRL, &rc)) ;
    printf("\n");
    printf("LAS0RR -- %#x\n",PlxRegisterRead(surfHandle, PCI9030_RANGE_SPACE0, &rc)) ; 
    printBinary(PlxRegisterRead(surfHandle, PCI9030_RANGE_SPACE0, &rc)) ; 
    printf("\n");
    printf("LAS0BA\n");
    printBinary(PlxRegisterRead(surfHandle, PCI9030_REMAP_SPACE0, &rc)) ;
    printf("\n");
    printf("LAS0BRD\n");
    printBinary(PlxRegisterRead(surfHandle, PCI9030_DESC_SPACE0, &rc)) ;

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
 
