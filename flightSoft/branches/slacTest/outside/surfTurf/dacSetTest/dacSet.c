/* 
   dacSet.c //Program to set DAC values on SURFs

   started as a copy of crate_test.c on 20/4/2005

*/

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "PciRegs.h"
#include "PlxApi.h"
#include "Reg9030.h"

/* numbers for data array. */
#define N_SURF 3  /* Can handle up to 3 SURF + 1 TURFIO */
#define N_SCA 256
#define N_CHN 8
#define N_CHP 4
#define N_RF 32  // 32 RF trigger channels per SURF board
#define N_TMO 100    /* number of msec wait for Evt_f before timed out. */
#define N_FTMO 2000    /* number of msec wait for Lab_f before timed out. */
#define DAC_CHAN 4 /* might replace with N_CHP at a later date */
#define PHYS_DAC 8
#define DAC_CNT 10000000
#define BIT_NO 16
#define BIT_EFF 12

typedef enum __SURF_control_act
{  ClrAll,
   ClrEvt,
   LTrig,
   EvNoD,
   LabD,
   SclD,
   RFpwD
} SURF_cntl_action ;

typedef enum __TURF_control_act
{ ClrTrg
} TURF_cntl_action ;


// TURFIO data structure, TV test
typedef struct {
    unsigned short rawtrig; // raw trigger number
    unsigned short L3Atrig; // same as raw trig
    unsigned long pps1;     // 1PPS
    unsigned long time;     // trig time
    unsigned long interval; // interval since last trig
    unsigned int rate[2][4]; // Antenna trigger output rates (Hz)
} TURFIO_struct;
  
/********************************************************
 *               Global Variables
 ********************************************************/
/*  BOOLEAN mgEventCompleted; */
U32     ChipTypeSelected;
U8      ChipRevision;
int verbose=2;
/* Byteswap unsigned short (16 bit) word. uh == unsigned short, uh != University of Hawaii */
inline unsigned short byteswap_uh(unsigned short a){
    return (a>>8)+(a<<8);
}

RETURN_CODE cntl_set_SURF(HANDLE PlxHandle, SURF_cntl_action action) {

    U32 base_clr= 0x02490092 ;
    U32 clr_evt = 0x00000030 ;
    U32 clr_all = 0x00000180 ;  
    U32 loc_trg = 0x00000006 ;
    U32 evn_done= 0x00030000 ;
    U32 lab_done= 0x00180000 ;
    U32 scl_done= 0x00C00000 ;
    U32 rfp_done= 0x06000000 ;

    U32           base_v,gpio_v ;
    RETURN_CODE   rc ;
    char *ch_act[] = {"ClrAll", "ClrEvt", "LTrig","EvNoD","LabD","SclD","RFpwD"} ;

    // Read current GPIO state
    base_v=PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc) ; 
    base_v=base_clr; // Gary's said this should be default PM 03-31-05

    switch (action) {
	case ClrAll   : gpio_v = base_clr | clr_all  ; break ; /* RJN base_v */
	case ClrEvt   : gpio_v = base_clr | clr_evt  ; break ; /* to base_clr*/
	case LTrig    : gpio_v = base_clr | loc_trg  ; break ;
	case EvNoD    : gpio_v = base_clr | evn_done ; break ; 
	case LabD     : gpio_v = base_clr | lab_done ; break ;
	case SclD     : gpio_v = base_clr | scl_done ; break ;
	case RFpwD    : gpio_v = base_clr | rfp_done ; break ;
	default :
	    printf(" cntl_set_SURF: undefined action, %d\n", action) ;
	    return ApiFailed ;
    }
    if (verbose) printf(" cntl_set_SURF: action %s, gpio_v = %x\n", 
			ch_act[(int)action], gpio_v) ; 

    if ((rc = PlxRegisterWrite(PlxHandle, PCI9030_GP_IO_CTRL, gpio_v ))
	!= ApiSuccess) {
	printf(" cntl_set_SURF: failed to set GPIO to %x.\n", gpio_v) ;
	return rc ;
    }

    /* Jing's logic requires trigger bit to be kept high.  9-Dec SM */
    //if (action == LTrig) return rc ;  // Self trigger disabled, PM 31-3-05
    /* return to the main, after reseting GPIO to base value. */
    return PlxRegisterWrite(PlxHandle, PCI9030_GP_IO_CTRL, base_clr ) ;

}

RETURN_CODE cntl_set_TURF(HANDLE PlxHandle, TURF_cntl_action action) {

    U32 base_clr = 0x00000010 ;
    U32 clr_trg  = 0x00000030 ; 

    U32           base_v,gpio_v ;
    RETURN_CODE   rc ;
    char *ch_act[] = {"ClrTrg"} ;

    // Read current GPIO state
    //base_v=PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc) ; 
    base_v=base_clr; // Gary's said this should be default PM 03-31-05

    switch (action) {
	case ClrTrg : gpio_v = base_v | clr_trg ; break ; 
	default :
	    printf(" cntl_set_TURF: undfined action, %d\n", action) ;
	    return ApiFailed ;
    }
    if (verbose) printf(" cntl_set_TURF: action %s, gpio_v = %x\n", 
			ch_act[(int)action], gpio_v) ; 

    if ((rc = PlxRegisterWrite(PlxHandle, PCI9030_GP_IO_CTRL, gpio_v ))
	!= ApiSuccess) {
	printf(" cntl_set_TURF : failed to set GPIO to %x.\n", gpio_v) ;
	return rc ;
    }

    return PlxRegisterWrite(PlxHandle, PCI9030_GP_IO_CTRL, base_v ) ;

}

/* Now it initializes all SURF's and TURFIO. Assumption is that TURF is 
   closest to CPU. If not, warranty is void. PM 03-29-05 */
int init_device(HANDLE *PlxHandle, DEVICE_LOCATION *Device) {

    int i,countSurfs=0,retVal;
    U32  DeviceNum ;
    DEVICE_LOCATION dummyDevice;
    HANDLE dummyHandle;
    //  U32  n=0 ;  /* this is the numebr of board from furthest from CPU. */

    for(i=0;i<N_SURF+1;++i){
	Device[i].BusNumber       = (U8)-1;
	Device[i].SlotNumber      = (U8)-1;
	Device[i].VendorId        = (U16)-1;
	Device[i].DeviceId        = (U16)-1;
	Device[i].SerialNumber[0] = '\0';
    }

    /* need to go through PlxPciDeviceFind twice.  1st for counting devices
       with "DeviceNum=FIND_AMOUNT_MATCHED" and 2nd to get device info. */

    DeviceNum = FIND_AMOUNT_MATCHED;
    if (PlxPciDeviceFind(&Device[0], &DeviceNum) != ApiSuccess ||
	DeviceNum > 12) return -1 ;
    if (verbose) printf("init_device: found %d PLX devices.\n", DeviceNum) ;

    /* Initialize SURFs */
    /* RJN hack -- When we are only using 1 SURF need to reset DeviceNum*/
    if(N_SURF==1 && DeviceNum>2)
	DeviceNum=2;
    for(i=0;i<DeviceNum;++i){
	if (PlxPciDeviceFind(&Device[i], &i) != ApiSuccess) 
	    return -1 ;

	if (verbose) 
	    printf("init_device: device found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
		   Device[i].DeviceId, Device[i].VendorId, Device[i].SerialNumber, 
		   Device[i].BusNumber, Device[i].SlotNumber);
    
	if (PlxPciDeviceOpen(&Device[i], &PlxHandle[i]) != ApiSuccess)
	    return -1 ;
	else countSurfs++;
    
	PlxPciBoardReset(PlxHandle[i]) ;
    }
    if(verbose) {
	printf("init_device: Initialized %d (well actually %d) SURF board(s)\n",i-1,countSurfs);
    }

    dummyDevice=Device[3];
    Device[3]=Device[1];
    Device[1]=dummyDevice;
    dummyHandle=PlxHandle[3];
    PlxHandle[3]=PlxHandle[1];
    PlxHandle[1]=dummyHandle;

    for(i=0;i<4;i++) {
    
	if (verbose)
	    printf("RJN: device found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
		   Device[i].DeviceId, Device[i].VendorId, Device[i].SerialNumber,
		   Device[i].BusNumber, Device[i].SlotNumber);
    }

    return (int)DeviceNum ;
}

// Clears boards for start of data taking 
void clear_device(HANDLE *PlxHandle,int n_dev){
    int i;
    RETURN_CODE      rc ;

    if(verbose) printf("*** Clearing devices ***\n");

    // Prepare SURF boards
    for(i=0;i<n_dev-1;++i){
	/* init. 9030 I/O descripter (to enable READY# input |= 0x02.) */
	if(PlxRegisterWrite(PlxHandle[i], PCI9030_DESC_SPACE0, 0x00800000) 
	   != ApiSuccess)  printf("  failed to set Sp0 descriptor on SURF %d.\n",i ) ;
    
	/*  Get PLX Chip Type */
	/*   PlxChipTypeGet(PlxHandle[i], &ChipTypeSelected, &ChipRevision ); */
	/*    if (ChipTypeSelected == 0x0) { */
	/*      printf("\n   ERROR: Unable to determine PLX chip type SURF %d\n",i); */
	/*      PlxPciDeviceClose(PlxHandle[i] ); */
	/*      exit(-1); */
	/*    } */
	/*    printf(" SURF %d Chip type:  %04x,  Revision :    %02X\n", i,*/
	/*          ChipTypeSelected, ChipRevision );  */
    }

    for(i=0;i<n_dev-1;++i){
	printf(" GPIO register contents SURF %d = %x\n",i,
	       PlxRegisterRead(PlxHandle[i], PCI9030_GP_IO_CTRL, &rc)) ; 
	if (cntl_set_SURF(PlxHandle[i], ClrAll) != ApiSuccess)
	    printf("  failed to send clear all event pulse on SURF %d.\n",i) ;
    }

    if(verbose){
	printf(" Try to test interrupt control register.\n") ;
	for(i=0;i<n_dev-1;++i)
	    printf(" Int reg contents SURF %d = %x\n",i,
		   PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ; 
    }

    
    printf(" GPIO register contents TURFIO = %x\n",
	   PlxRegisterRead(PlxHandle[i], PCI9030_GP_IO_CTRL, &rc)) ; 
    
    //printf(" Try to write 9030 GPIO.\n") ;
    if (cntl_set_TURF(PlxHandle[i], ClrTrg) != ApiSuccess)
      printf("  failed to send clear pulse on TURF %d.\n",i) ;
    
    /*    printf(" Try to read 9030 GPIO.  %x\n", */
    /*  	 PlxRegisterRead(PlxHandle[i], PCI9030_GP_IO_CTRL, &rc)) ; */
    /*    if (rc != ApiSuccess) printf("  failed to read TURFIO GPIO .\n") ; */
    
    if(verbose){
      printf(" Try to test interrupt control register.\n") ;
      printf(" int reg contents TURFIO = %x\n",
	     PlxRegisterRead(PlxHandle[i], PCI9030_INT_CTRL_STAT, &rc)) ; 
    }
    return;
}


void setDAC(HANDLE *PlxHandle,int numDevices,int value) {
  int bit_RS,bit;
  int chan,chip,phys_DAC,boardIndex;
  unsigned short DACVal_A, DACVal_B, DACVal_C, DACVal_D,dataByte;
  unsigned short DACVal[DAC_CHAN][PHYS_DAC],DAC_C_0[DAC_CHAN][BIT_NO]; 
  unsigned short DACVal_1[DAC_CHAN][PHYS_DAC];
  unsigned short DAC_C[DAC_CHAN][BIT_NO];
  int tmpGPIO;
    RETURN_CODE      rc ;

  // Copy what Jing does at start of her program
  // Will probably remove this at some point
  /*     for(i=0;i<numDevices-1;++i) { */
  /* 	if (cntl_set_SURF(PlxHandle[i], ClrEvt) != ApiSuccess) */
  /* 	    printf("  failed to send clear event pulse on SURF %d.\n",i); */
  /* 	if (cntl_set_SURF(PlxHandle[i], ClrAll) != ApiSuccess) */
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
  //Write in DAC values--Start
  for (chan=0; chan<DAC_CHAN; chan++) 
    {
      for (phys_DAC=0; phys_DAC<PHYS_DAC; phys_DAC++)
	{
	  DACVal[chan][phys_DAC]=value&0xffff;
	  //		printf("DACVal[%d][%d]=%x    \n", chan, phys_DAC, DACVal[chan][phys_DAC]);
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
      //		printf("DAC_C[%d][%d] = %d\n",chan,bit,DAC_C[chan][bit]);
    }
  }

  for(boardIndex=0;boardIndex<numDevices-1;boardIndex++) {	    
    for (chan=0; chan<DAC_CHAN; chan++) {
      printf("Writing: ");
      for (bit=BIT_NO-1; bit>=0; bit--) {
	//		TESTAPP_WriteWord(hTESTAPP, 2, 0, DAC_C[chan][bit]);
	dataByte=DAC_C[chan][bit];
	if (PlxBusIopWrite(PlxHandle[boardIndex], IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16) != ApiSuccess) {
	  printf("Failed to write DAC value\n");
	}
	printf("%x ",dataByte);
	//else printf("Wrote %d to SURF %d; value: %d\n",dataByte,boardIndex,value);		    
      }
      printf("\n");
      tmpGPIO=PlxRegisterRead(PlxHandle[0], PCI9030_GP_IO_CTRL, &rc);
      tmpGPIO=PlxRegisterRead(PlxHandle[0], PCI9030_GP_IO_CTRL, &rc);
      //	    dwVal = TESTAPP_ReadDword(hTESTAPP, 0, dwOffset);
      //	    dwVal = TESTAPP_ReadDword(hTESTAPP, 0, dwOffset);
    }
  }
	
  value++;



}


/* ----------------===============----------------------- */
int main(int argnum, char **argval) {

    HANDLE           PlxHandle[N_SURF+1] ;
    DEVICE_LOCATION  Device[N_SURF+1] ;
    PLX_INTR         PlxIntr, Plxstate ;
    RETURN_CODE      rc ;

    unsigned short  dataByte=0;//,d_out;
    unsigned short  evNo=0;
    unsigned short rawDataArray[N_SURF][N_CHN][N_SCA]; // Read only one chip per trigger. PM
    float calibDataArray[N_SURF][N_CHN][N_SCA]; 
    unsigned short data_scl[N_RF];
    unsigned short data_rfpw[N_RF];
    unsigned long numChipEvents[N_CHP];
    TURFIO_struct data_turf;
    int i, j, boardIndex, ftmo=0,tmo=0,doingEvent=0 ; 
    int numDevices, n_ev=0 ;
    float mean_dt=0;
    int testVal;
    char *outputDir ;
    int tmpGPIO;
    int selftrig=0;
    int value=0;

  
    printf(" Start dacSet program !!\n") ;


    if (verbose) printf("  size of buffer is %d bytes.\n", sizeof(dataByte)) ;

    // Initialize devices
    if ((numDevices = init_device(PlxHandle, Device)) < 0) {
	printf("Problem initializing devices\n");
	return 0 ;
    }
    for(i=0;i<4;i++) {  
	if (verbose)
	    printf("RJN: now, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
		   Device[i].DeviceId, Device[i].VendorId, Device[i].SerialNumber,
		   Device[i].BusNumber, Device[i].SlotNumber);
    }

    // Clear devices 
    clear_device(PlxHandle,numDevices);

    /* initialize interrupt structure. */
    memset(&PlxIntr, 0, sizeof(PLX_INTR)) ;
    memset(&Plxstate, 0, sizeof(PLX_INTR)) ;

    /* -----------
       despite of "Prog.Ref." manual description, IopToPciInt did not change 
       LINT select enable (level/edge trigger sw.) bits.   10-Nov-04
       ----------- */
    PlxIntr.IopToPciInt = 1 ;    /* LINT1 interrupt line */
    PlxIntr.IopToPciInt_2 = 1 ;  /* LINT2 interrupt line */
    PlxIntr.PciMainInt = 1 ;




    while ((doingEvent++ < n_ev) || (n_ev == 0)) { 
	
	
      setDAC(PlxHandle,numDevices,value);
      value++;


	if (selftrig){  /* --- send a trigger ---- */ 
	    printf(" try to trigger it locally.\n") ;
	    for(i=0;i<numDevices-1;++i)
		if (cntl_set_SURF(PlxHandle[i], LTrig) != ApiSuccess) 
		    printf("  failed to set LTrig flag on SURF %d.\n",i) ;
	}
	/* Wait for ready to read (Evt_F) */
	tmo=0;
	if(verbose) printf("Sitting here about to wait\n");
	/* RJN change PlxHandle[numDevices-2] to PlxHandle[0] */
	while (!((tmpGPIO=PlxRegisterRead(PlxHandle[0], PCI9030_GP_IO_CTRL, &rc)) & 0x800) &&
	       (selftrig?(++tmo<N_TMO):1)){
	    if(verbose>3) printf("SURF 1 GPIO: 0x%x %d\n",tmpGPIO,tmpGPIO);
	    if(selftrig) usleep(1000); /*RJN put it back in for selftrig */
	    usleep(1000); /*GV removed, RJN put it back in as it completely abuses the CPU*/
	}

	if (tmo == N_TMO){
	    printf(" Timed out (%d ms) while waiting for Evt_F flag in self trigger mode.\n", N_TMO) ;
	    continue;
	}

	if(verbose) printf("Triggered, event %d (by software counter).\n",doingEvent);

	//usleep (100) ;  /* I doubt Evt_f is issued in proper timing. SM */
    
	/* Loop over SURF boards and read out */
	for(boardIndex=0;boardIndex<numDevices-1;boardIndex++){
	    printf("What is boardIndex %d\n",boardIndex);
	    if(verbose)
		printf(" GPIO register contents SURF %d = %x\n",boardIndex,
		       PlxRegisterRead(PlxHandle[boardIndex], PCI9030_GP_IO_CTRL, &rc)) ; 
	    if(verbose)
		printf(" int reg contents SURF %d = %x\n",boardIndex,
		       PlxRegisterRead(PlxHandle[boardIndex], PCI9030_INT_CTRL_STAT, &rc)) ; 

	    // Readout event number
	    if (PlxBusIopRead(PlxHandle[boardIndex], IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16)
		!= ApiSuccess)
		printf("  failed to read event number on SURF %d\n", boardIndex) ;
	    if(verbose) printf("SURF %d, ID %d, event no %d\n",boardIndex,dataByte>>14,dataByte);
	    
	    if(boardIndex==0) evNo=dataByte;
	    if(cntl_set_SURF(PlxHandle[boardIndex],EvNoD)!=ApiSuccess)
		printf("Failed to set EvNoD on SURF %d\n",boardIndex);

	    // still too short!?!      usleep(1000000000); // wait a bit according to Jing

	    // Wait for Lab_F
	    ftmo=0;
	    if(verbose) printf("SURF %d, waiting for Lab_F\n",boardIndex);
	    testVal=PlxRegisterRead(PlxHandle[boardIndex], PCI9030_GP_IO_CTRL, &rc); 
	    while(!(testVal & 0x4000) &&
		  (++ftmo<N_FTMO)){
		/* 	  printf("testVal %x\n",testVal); */
		usleep(1); /* RJN/GV changed from usleep(1000) */
		testVal=PlxRegisterRead(PlxHandle[boardIndex], PCI9030_GP_IO_CTRL, &rc);
	    }
	
	    if (ftmo == N_FTMO){
		printf("No Lab_F flag for %d ms, bailing on the event\n",N_FTMO);
		continue;
	    }

	    if(cntl_set_SURF(PlxHandle[boardIndex],LabD)!=ApiSuccess)
		printf("Failed to set LabD on SURF %d\n",boardIndex);

	    // Read trigger scalars
	    for(i=0;i<N_RF;++i){
		if (PlxBusIopRead(PlxHandle[boardIndex], IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16)
		    != ApiSuccess)
		    printf("  failed to read IO. surf=%d, rf scl=%d\n", boardIndex, i) ;
		data_scl[i]=dataByte;
		if(verbose>2) 
		    printf("SURF %d, SCL %d: %d\n",boardIndex,i,dataByte);
	    }
      
	    if(cntl_set_SURF(PlxHandle[boardIndex],SclD)!=ApiSuccess)
		printf("Failed to set SclD on SURF %d\n",boardIndex);

	    // Read RF power 
	    for(i=0;i<N_RF;++i){
		if (PlxBusIopRead(PlxHandle[boardIndex], IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16)
		    != ApiSuccess)
		    printf("  failed to read IO. surf=%d, rf pw=%d\n", boardIndex, i) ;
		data_rfpw[i]=dataByte;
		if(verbose>2) 
		    printf("SURF %d, RF %d: %d\n",boardIndex,i,dataByte);
	    }
      
	    if(cntl_set_SURF(PlxHandle[boardIndex],RFpwD)!=ApiSuccess)
		printf("Failed to set RFpwD on SURF %d\n",boardIndex);



	}  /* this is closing for(numDevices) loop. */
	
	/* Readout TURFIO */
	boardIndex=numDevices-1;
	/* RJN Hack */
/*       boardIndex=1; */
      
	if(verbose) printf(" GPIO register contents TURFIO = %x\n",
			   PlxRegisterRead(PlxHandle[boardIndex], PCI9030_GP_IO_CTRL, &rc)) ; 
	if(verbose) printf(" int reg contents TURFIO = %x\n", 
			   PlxRegisterRead(PlxHandle[boardIndex], PCI9030_INT_CTRL_STAT, &rc)) ; 
	
	// Burn one TURFIO read
	if (PlxBusIopRead(PlxHandle[boardIndex], IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16)
	    != ApiSuccess)
	  printf("  failed to read TURF IO port. Burn read.\n");
	
	data_turf.interval=0;
	data_turf.time=10;
	data_turf.rawtrig=10;
	if(verbose) printf("TURF Handle number: %d\n",boardIndex);
	for (j=0 ; j<16 ; j++) { // Loop 16 times over return structure
	  for (i=0; i<16 ; i++) {
	    if (PlxBusIopRead(PlxHandle[boardIndex], IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16)
		!= ApiSuccess)
	      printf("  failed to read TURF IO port. loop %d, word %d.\n",j,i) ;
	    // Need to byteswap TURFIO words. This might get changed in hardware, so beware. PM 04/01/05
	    dataByte=byteswap_uh(dataByte);
	    if((verbose>2 && j==0) || verbose>3)
	      printf("TURFIO %d (%d): %hu\n",i,j,dataByte);
	    if(j==0){ //Interpret on the first pass only
	      switch(i){
	      case 0: data_turf.rawtrig=dataByte; break;
	      case 1: data_turf.L3Atrig=dataByte; break;
	      case 2: data_turf.pps1=dataByte; break;
	      case 3: data_turf.pps1+=dataByte*65536; break;
	      case 4: data_turf.time=dataByte; break;
	      case 5: data_turf.time+=dataByte*65536; break;
	      case 6: data_turf.interval=dataByte; break;
	      case 7: data_turf.interval+=dataByte*65536; break;
	      default: data_turf.rate[i/4-2][i%4]=dataByte; break;
	      }
	    }
	  }
	}
	if(verbose>2){
	  printf("raw: %hu\nL3A: %hu\n1PPS: %lu\ntime: %lu\nint: %lu\n",
		 data_turf.rawtrig,data_turf.L3Atrig,data_turf.pps1,data_turf.time,
		 data_turf.interval);
	  for(i=8;i<16;++i)
	    printf("Rate[%d][%d]=%d\n",i/4-2,i%4,data_turf.rate[i/4-2][i%4]);
	}
	
    
    if(verbose) printf("Done reading\n");
          mean_dt=mean_dt*(doingEvent-1)/doingEvent+(float)data_turf.interval/doingEvent;
      printf("Trig no: %hu, trig time: %lu, 1PPS: %lu, dt: %lu\n",data_turf.rawtrig,
	     data_turf.time,data_turf.pps1,data_turf.interval);
      printf("mean dt: %f\n",mean_dt);



	// Clear boards
	for(i=0;i<numDevices-1;++i)
	    if (cntl_set_SURF(PlxHandle[i], ClrEvt) != ApiSuccess)
		printf("  failed to send clear event pulse on SURF %d.\n",i) ;
    


    }  /* closing master while loop. */

    // Clean up
    for(i=0;i<numDevices;++i)  PlxPciDeviceClose(PlxHandle[i] );
  
    return 1 ;
}

