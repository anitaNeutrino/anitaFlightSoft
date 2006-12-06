/* 
   Test program for PLX 9030 chip that controls LABRADOR.
   started 27-Nov-04  SM.

   Merging of surf_test and turf_test into crate_test. 
   started 29-Mar_05 PM

*/

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "PciRegs.h"
#include "PlxApi.h"
/*  #include "PlxInit.h" */
/*  #include "RegDefs.h" */
#include "Reg9030.h"

/* numbers for data array. */
/* RJN hack changing 3 to 1 and back again */
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

int verbose = 0 ; /* control debug print out. */
int selftrig = FALSE ;/* self-trigger mode */
int surfonly = FALSE; /* Run in surf only mode */
int writedata = FALSE; /* Default is not to write data to a disk */
int doDacCycle = FALSE; /* Do a cycle of the DAC values */


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

/* 
   send a pulse to a control line through 9030 GPIO.

   ClrEvt : ClrEvt on LABRADOR              (pos. logic)
   ClrBsy : ClrBsy on LABRADOR

*/

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
  case ClrAll : gpio_v = base_clr | clr_all  ; break ; /* RJN changed base_v */
  case ClrEvt : gpio_v = base_clr | clr_evt  ; break ; /* to base_clr */
  case LTrig  : gpio_v = base_clr | loc_trg  ; break ;
  case EvNoD  : gpio_v = base_clr | evn_done ; break ; 
  case LabD   : gpio_v = base_clr | lab_done ; break ;
  case SclD   : gpio_v = base_clr | scl_done ; break ;
  case RFpwD  : gpio_v = base_clr | rfp_done ; break ;
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
  /* RJN hack -- For only one surf uncomment below line*/
/*   DeviceNum=2;  */
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

  if(surfonly) return;

  // Prepare TURFIO board
  i=n_dev-1;
  
  /*  Get PLX Chip Type */
  /*   PlxChipTypeGet(PlxHandle[i], &ChipTypeSelected, &ChipRevision ); */
  /*    if (ChipTypeSelected == 0x0) { */
  /*      printf("\n   ERROR: Unable to determine PLX chip type TURFIO\n"); */
  /*      PlxPciDeviceClose(PlxHandle[i] ); */
  /*      exit(-1); */
  /*    } */
  /*    printf(" TURFIO Chip type:  %04x,  Revision :    %02X\n", */
  /*          ChipTypeSelected, ChipRevision );  */

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


void write_data(char *directory, unsigned short wv_data[N_SURF][N_CHN][],unsigned long evNum) {

  char f_name[128] ;
  FILE *wv_file ;
  int  n ;

  sprintf(f_name, "%s/surf_data.%lu", directory, evNum) ;
  if(verbose) printf(" write_data: save data to %s\n", f_name) ;

  if ((wv_file=fopen(f_name, "w")) == NULL) {
    printf(" write_data: failed to open wv file, %s\n", f_name) ;
    return ; 
  }

  if ((n=fwrite(wv_data, sizeof(unsigned short), N_SCA*N_CHN*N_SURF, wv_file))
      != N_SCA*N_CHN*N_SURF)
    printf(" write_data: failed to write all data. wrote only %d.\n", n) ;

  fclose (wv_file) ;
}

void write_turf(char *directory, TURFIO_struct *data_turf,unsigned long evNum) {

  char f_name[128] ;
  FILE *wv_file ;
  int  n ;

  sprintf(f_name, "%s/turf_data.%lu", directory, evNum) ;
  if(verbose) printf(" write_turf: save data to %s\n", f_name) ;

  if ((wv_file=fopen(f_name, "w")) == NULL) {
    printf(" write_data: failed to open wv file, %s\n", f_name) ;
    return ; 
  }

  if ((n=fwrite(data_turf, sizeof(TURFIO_struct), 1, wv_file))!= 1)
    printf(" write_data: failed to write turf data.\n") ;

  fclose (wv_file) ;
}


/* purse command arguments to initialize parameter(s). */
int init_param(int argn, char **argv, char **directory, int *n) {

  *directory = NULL ;   /* just make sure it points NULL. */

  while (--argn) {
    if (**(++argv) == '-') {
      switch (*(*argv+1)) {
      case 'v': verbose++ ; break ;
      case 't': selftrig = TRUE; break;
      case 's': surfonly = TRUE; break;
      case 'w': writedata = TRUE; break;
      case 'c': doDacCycle = TRUE; break;
      case 'd': *directory = *(++argv);
	--argn ; break ;
      case 'n': sscanf(*(++argv), "%d", n) ;
	--argn ; break ;
      default : printf(" init_param: undefined parameter %s\n", *argv) ;
      }
    }
  }
  
  return 0;
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
      if(verbose>1)      printf("Writing: ");
      for (bit=BIT_NO-1; bit>=0; bit--) {
	//		TESTAPP_WriteWord(hTESTAPP, 2, 0, DAC_C[chan][bit]);
	dataByte=DAC_C[chan][bit];
	if (PlxBusIopWrite(PlxHandle[boardIndex], IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16) != ApiSuccess) {
	  printf("Failed to write DAC value\n");
	}
	if(verbose>1)printf("%x ",dataByte);
	//else printf("Wrote %d to SURF %d; value: %d\n",dataByte,boardIndex,value);		    
      }
      if(verbose>1)printf("\n");
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
  //  U32              port, var[7] ;

  unsigned short  d_in=0;//,d_out;
  unsigned short  evNo=0;
  unsigned short data_array[N_SURF][N_CHN][N_SCA]; // Read only one chip per trigger. PM
  float data_calib[N_SURF][N_CHN][N_SCA]; 
  unsigned short data_scl[N_RF];
  unsigned short data_rfpw[N_RF];
  unsigned long numChipEvents[N_CHP];
  TURFIO_struct data_turf;
  int i, j, k, ftmo=0,tmo=0,doingEvent=0 ; 
  int n_dev, n_ev=0 ;
  float mean_dt=0;
  float mean_rms[N_SURF][N_CHP][N_CHN];
  double chanMean[N_SURF][N_CHP][N_CHN];
  double chanMeanSqd[N_SURF][N_CHP][N_CHN];
  unsigned long chanNumReads[N_SURF][N_CHP][N_CHN];
  /*   int fred, testVal; */
  int testVal;
  char *dir_n ;
  int tmpGPIO;
  int dacVal=0;
  
  for(k=0;k<N_SURF;++k)
    for(j=0;j<N_CHP;++j) 
      for(i=0;i<N_CHN;++i) {
	mean_rms[k][j][i]=0;
	chanMean[k][j][i]=0;
	chanMeanSqd[k][j][i]=0;
	chanNumReads[k][j][i]=0;
      }


  
  printf(" Start test program !!\n") ;

  init_param(argnum, argval, &dir_n, &n_ev) ;
  if (dir_n == NULL) dir_n = "./data" ;   /* this is default diretory name. */
  /*   if (n_ev == 0) n_ev = 1000 ; */

  if (verbose) printf("  size of buffer is %d bytes.\n", sizeof(d_in)) ;

  // Initialize devices
  if ((n_dev = init_device(PlxHandle, Device)) < 0) {
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
  clear_device(PlxHandle,n_dev);

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

  /*    if (PlxRegisterWrite(PlxHandle, PCI9030_INT_CTRL_STAT, 0x00300000 )  */
  /*        != ApiSuccess)  */
  /*      printf("  failed to reset interrupt control bits.\n") ;  */

  /*   if (PlxIntrDisable(PlxHandle, &PlxIntr) != ApiSuccess) */
  /*     printf("  failed to disable interrupt.\n") ; */
  /*   printf(" int reg contents = %x, after disable.\n", */
  /* 	 PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ;  */
  /*   PlxIntr.PciMainInt = 0 ; */
  /*   if (PlxIntrEnable(PlxHandle, &PlxIntr) != ApiSuccess) */
  /*     printf("  failed to enable interrupt.\n") ; */
  /*   printf(" int reg contents = %x, after enable Lints.\n", */
  /* 	 PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ;  */
  /*   PlxIntr.PciMainInt = 1 ; */
  /*   if (PlxIntrEnable(PlxHandle, &PlxIntr) != ApiSuccess) */
  /*     printf("  failed to enable interrupt.\n") ; */
  /*   if (PlxIntrStatusGet(PlxHandle, &Plxstate) != ApiSuccess) */
  /*     printf("  fialed to get interrupt status.\n") ; */
  /*   printf("  Intr(Main/Lint1/Lint2) = %01d/%01d/%01d\n", */
  /* 	 Plxstate.PciMainInt, Plxstate.IopToPciInt, Plxstate.IopToPciInt_2) ; */


  while ((doingEvent++ < n_ev) || (n_ev == 0)) {
    if(doDacCycle) {
      setDAC(PlxHandle,n_dev,dacVal);
      dacVal++;
    }
    /* initialize data_array with 0, should be just inside the outmost loop.*/
    bzero(data_array, sizeof(data_array)) ;

    if (selftrig){  /* --- send a trigger ---- */ 
      printf(" try to trigger it locally.\n") ;
      for(i=0;i<n_dev-1;++i)
	if (cntl_set_SURF(PlxHandle[i], LTrig) != ApiSuccess) 
	  printf("  failed to set LTrig flag on SURF %d.\n",i) ;
    }
    /* Wait for ready to read (Evt_F) */
    tmo=0;
    if(verbose) printf("Sitting here about to wait\n");
    /* RJN change PlxHandle[n_dev-2] to PlxHandle[0] */
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
    for(k=0;k<n_dev-1;k++){
/*       if(k==1) { */
/* 	k=3; /\*RJN hack to ignore middle surf*\/ */
/*       } */
      printf("What is k %d\n",k);
      if(verbose)
	printf(" GPIO register contents SURF %d = %x\n",k,
	       PlxRegisterRead(PlxHandle[k], PCI9030_GP_IO_CTRL, &rc)) ; 
      if(verbose)
	printf(" int reg contents SURF %d = %x\n",k,
	       PlxRegisterRead(PlxHandle[k], PCI9030_INT_CTRL_STAT, &rc)) ; 

      // Readout event number
      if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	  != ApiSuccess)
	printf("  failed to read event number on SURF %d\n", k) ;
      if(verbose) printf("SURF %d, ID %d, event no %d\n",k,d_in>>14,(d_in & 0xfff));
      /*       if(k==1 || k==0) { */
      /* 	for(fred=0;fred<10;fred++) { */
      /* 	 if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16) */
      /* 	  != ApiSuccess) */
      /* 	   printf("  failed to read event number on SURF %d\n", k) ; */

      /* 	   printf("SURF %d, event no %d\n",k,d_in); */

      /* 	} */
      /*       } */
      if(k==0) evNo=d_in & 0xfff;
      if(cntl_set_SURF(PlxHandle[k],EvNoD)!=ApiSuccess)
	printf("Failed to set EvNoD on SURF %d\n",k);

      // still too short!?!      usleep(1000000000); // wait a bit according to Jing

      // Wait for Lab_F
      ftmo=0;
      /* Ugly RJN hack */
      if(k!=1 || 1) {
	if(verbose) printf("SURF %d, waiting for Lab_F\n",k);
	testVal=PlxRegisterRead(PlxHandle[k], PCI9030_GP_IO_CTRL, &rc); 
	while(!(testVal & 0x4000) &&
	      (++ftmo<N_FTMO)){
	  /* 	  printf("testVal %x\n",testVal); */
	  usleep(1); /* RJN/GV changed from usleep(1000) */
	  testVal=PlxRegisterRead(PlxHandle[k], PCI9030_GP_IO_CTRL, &rc);
	}
	
	if (ftmo == N_FTMO){
	  printf("No Lab_F flag for %d ms, bailing on the event\n",N_FTMO);
	  continue;
	}
      }
      else {
      	printf("SURF %d, didn't look for Lab_F\n",k);
      }
      // Read LABRADOR data
      for (j=0 ; j<N_SCA ; j++) {
	for (i=0 ; i<N_CHN ; i++) {
	  if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	      != ApiSuccess)
	    printf("  failed to read IO. surf=%d, chn=%d sca=%d\n", k, i, j) ;
	  // Record which chip is being read out
	  if(verbose>2) 
	    printf("SURF %d, CHP %d,CHN %d, SCA %d: %d (HITBUS=%d) %d\n",k,d_in>>14,i,j,d_in,d_in & 0x1000,d_in&0xfff);
/* 	  if (i==0 && j==0) */
/* 	    chip[k] = d_in>>14; */
/* 	  else if((d_in>>14)!=chip[k]) */
/* 	    printf(" data xfer error; chip %d (instead of %d) seen on surf=%d, chn=%d sca=%d\n", */
/* 		   d_in>>14,chip[k],k,i,j); */
	  // Check if 13 bit is zero
	  if ((d_in&0x2000) != 0)
	    printf(" data xfer error; bit 13 != 0 on surf=%d, chn=%d sca=%d\n",k,i,j);
	  
	  /* put data into array */
	  if(d_in & 0x1000) data_array[k][i][j] = 0; // This is hitbus bin
	  else data_array[k][i][j] = d_in;// & 0xfff ; //RJN test
	}    
      }

      /* to take care of firmware weirdness on the 1st data. */
      /* if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16) */
      /* 	  != ApiSuccess) */
      /* 	printf("  failed to read IO.  @reread 1st data.\n") ; */
      /*       data_array[1][0][0] = d_in ; */
      /*       if (verbose) printf(" data(0/0) = %x \n", d_in) ; */

      if(cntl_set_SURF(PlxHandle[k],LabD)!=ApiSuccess)
	printf("Failed to set LabD on SURF %d\n",k);

      // Read trigger scalars
      for(i=0;i<N_RF;++i){
	if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	    != ApiSuccess)
	  printf("  failed to read IO. surf=%d, rf scl=%d\n", k, i) ;
	data_scl[i]=d_in;
	if(verbose>2) 
	  printf("SURF %d, SCL %d: %d\n",k,i,d_in);
      }
      
      if(cntl_set_SURF(PlxHandle[k],SclD)!=ApiSuccess)
	printf("Failed to set SclD on SURF %d\n",k);

      // Read RF power 
      for(i=0;i<N_RF;++i){
	if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	    != ApiSuccess)
	  printf("  failed to read IO. surf=%d, rf pw=%d\n", k, i) ;
	data_rfpw[i]=d_in;
	if(verbose>2) 
	  printf("SURF %d, RF %d: %d\n",k,i,d_in);
      }
      
      if(cntl_set_SURF(PlxHandle[k],RFpwD)!=ApiSuccess)
	printf("Failed to set RFpwD on SURF %d\n",k);

      /* RJN hack ugly ugly dangerous */
/*       if(k==3) k=1; */

    }  /* this is closing for(n_dev) loop. */

    if(!surfonly){

      /* Readout TURFIO */
      k=n_dev-1;
      /* RJN Hack */
/*       k=1; */
      
      if(verbose) printf(" GPIO register contents TURFIO = %x\n",
			 PlxRegisterRead(PlxHandle[k], PCI9030_GP_IO_CTRL, &rc)) ; 
      if(verbose) printf(" int reg contents TURFIO = %x\n", 
			 PlxRegisterRead(PlxHandle[k], PCI9030_INT_CTRL_STAT, &rc)) ; 

      // Burn one TURFIO read
      if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	  != ApiSuccess)
	printf("  failed to read TURF IO port. Burn read.\n");

      data_turf.interval=0;
      data_turf.time=10;
      data_turf.rawtrig=10;
      if(verbose) printf("TURF Handle number: %d\n",k);
      for (j=0 ; j<16 ; j++) { // Loop 16 times over return structure
	for (i=0; i<16 ; i++) {
	  if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	      != ApiSuccess)
	    printf("  failed to read TURF IO port. loop %d, word %d.\n",j,i) ;
	  // Need to byteswap TURFIO words. This might get changed in hardware, so beware. PM 04/01/05
	  d_in=byteswap_uh(d_in);
	  if((verbose>2 && j==0) || verbose>3)
	    printf("TURFIO %d (%d): %hu\n",i,j,d_in);
	  if(j==0){ //Interpret on the first pass only
	    switch(i){
	    case 0: data_turf.rawtrig=d_in; break;
	    case 1: data_turf.L3Atrig=d_in; break;
	    case 2: data_turf.pps1=d_in; break;
	    case 3: data_turf.pps1+=d_in*65536; break;
	    case 4: data_turf.time=d_in; break;
	    case 5: data_turf.time+=d_in*65536; break;
	    case 6: data_turf.interval=d_in; break;
	    case 7: data_turf.interval+=d_in*65536; break;
	    default: data_turf.rate[i/4-2][i%4]=d_in; break;
	    }
	  }
	}
      }
      if(verbose>1){
	printf("raw: %hu\nL3A: %hu\n1PPS: %lu\ntime: %lu\nint: %lu\n",
	       data_turf.rawtrig,data_turf.L3Atrig,data_turf.pps1,data_turf.time,
	       data_turf.interval);
	for(i=8;i<16;++i)
	  printf("Rate[%d][%d]=%d\n",i/4-2,i%4,data_turf.rate[i/4-2][i%4]);
      }
    }

    if(verbose) printf("Done reading\n");

    // Save data
    if(writedata){
/*       if(!surfonly) write_turf(dir_n,&data_turf); */
/*       write_data(dir_n,data_array,data_turf.time) ; */
      if(!surfonly) write_turf(dir_n,&data_turf,evNo);
      write_data(dir_n,data_array,evNo) ; /*RJN hack in case turf.time not working */

    }

    // Some performance indicators
    if(!surfonly){
      mean_dt=mean_dt*(doingEvent-1)/doingEvent+(float)data_turf.interval/doingEvent;
      printf("Trig no: %hu, trig time: %lu, 1PPS: %lu, dt: %lu\n",data_turf.rawtrig,
	     data_turf.time,data_turf.pps1,data_turf.interval);
      printf("mean dt: %f\n",mean_dt);
    }else{
      printf("Event no: %d\n",evNo);
    }

    // Calibrate waveform (roughly)
    for(k=0;k<n_dev-1;++k)
      for(j=0;j<N_CHN;++j)
	for(i=0;i<N_SCA;++i){
	  if(data_array[k][j][i]==0) data_calib[k][j][i]=0;
	  else data_calib[k][j][i]=(data_array[k][j][i]-2700)/1.6;
	  if(verbose>4)
	    printf("CALIB SURF %d, CHN %d, SCA %d: %f\n",k,j,i,data_calib[k][j][i]);
	}
    // Calculate mean,rms,std dev
    for(k=0;k<n_dev-1;++k) {
      for(j=0;j<N_CHN;++j){
	double sum[N_CHP]={0};
	double sumSqd[N_CHP]={0};
	int tempNum[N_CHP]={0};
	for(i=0;i<N_SCA;++i) {
	  int tempChip=data_array[k][j][i]>>14;
	  int tempData=data_array[k][j][i] & 0xfff;
	  if(tempData) {
/* 	    printf("%d\t%d\n",tempChip,tempData); */
	    sum[tempChip]+=tempData;
	    sumSqd[tempChip]+=tempData*tempData;
	    tempNum[tempChip]++;
	  }
	}
	int chipNum;
	for(chipNum=0;chipNum<N_CHP;chipNum++) {	  
	  if(tempNum[chipNum]) {
	    float tempRMS=0;
	    tempRMS=sqrt(sumSqd[chipNum]/(float)tempNum[chipNum]);
	    float temp1=mean_rms[k][chipNum][j]*((float)(numChipEvents[chipNum]))/(float)(numChipEvents[chipNum]+1);
	    float temp2=tempRMS/(float)(numChipEvents[chipNum]+1);	  
	    mean_rms[k][chipNum][j]=temp1+temp2;
	    numChipEvents[chipNum]++;

	    /* Ryan's statistics */	    
	    double tempMean=sum[chipNum];///(double)tempNum[chipNum];
	    double tempMeanSqd=sumSqd[chipNum];///(double)tempNum[chipNum];
/* 	    printf("%f\t%f\n",tempMean,tempMeanSqd); */
	    chanMean[k][chipNum][j]=(chanMean[k][chipNum][j]*((double)chanNumReads[k][chipNum][j]/(double)(tempNum[chipNum]+chanNumReads[k][chipNum][j]))) +
	      (tempMean/(double)(tempNum[chipNum]+chanNumReads[k][chipNum][j]));
	    chanMeanSqd[k][chipNum][j]=(chanMeanSqd[k][chipNum][j]*((double)chanNumReads[k][chipNum][j]/(double)(tempNum[chipNum]+chanNumReads[k][chipNum][j]))) +
	      (tempMeanSqd/(double)(tempNum[chipNum]+chanNumReads[k][chipNum][j]));
	    chanNumReads[k][chipNum][j]+=tempNum[chipNum];	    
	  }
	}
      }
    }
  
     
/*   // Display mean rms */
/*     for(k=0;k<n_dev-1;++k){ */
/*       printf("Board %d RMS, Event %d (Software)\n",k,doingEvent); */
/*       for(j=0;j<N_CHP;++j){ */
/* 	for(i=0;i<N_CHN;++i) */
/* 	  printf("%.2f ",mean_rms[k][j][i]); */
/* 	printf("\n"); */
/*       } */
/*     } */

  // Display Mean
    for(k=0;k<n_dev-1;++k){
      printf("Board %d Mean, After Event %d (Software)\n",k,doingEvent);
      for(j=0;j<N_CHP;++j){
	for(i=0;i<N_CHN;++i)
	  printf("%.2f ",chanMean[k][j][i]);
	printf("\n");
      }
    }

  // Display Std Dev
    for(k=0;k<n_dev-1;++k){
      printf("Board %d Std Dev., After Event %d (Software)\n",k,doingEvent);
      for(j=0;j<N_CHP;++j){
	for(i=0;i<N_CHN;++i)
	  printf("%.2f ",sqrt(chanMeanSqd[k][j][i]-chanMean[k][j][i]*chanMean[k][j][i]));
	printf("\n");
      }
    }

    // Clear boards
    for(i=0;i<n_dev-1;++i)
      if (cntl_set_SURF(PlxHandle[i], ClrEvt) != ApiSuccess)
	printf("  failed to send clear event pulse on SURF %d.\n",i) ;
    
    if(!surfonly)
      if (cntl_set_TURF(PlxHandle[i], ClrTrg) != ApiSuccess)
	printf("  failed to send clear event pulse on TURFIO.\n") ;
    

    if(selftrig) sleep (2) ;
  }  /* closing master while loop. */

  // Clean up
  for(i=0;i<n_dev;++i)  PlxPciDeviceClose(PlxHandle[i] );
  
  return 1 ;
}

