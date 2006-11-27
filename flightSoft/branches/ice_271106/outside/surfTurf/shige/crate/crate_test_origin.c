/* 
     Test program for PLX 9030 chip that controls LABRADOR.
                                      started 27-Nov-04  SM.

     Merging of surf_test and turf_test into crate_test. 
                                      started 29-Mar_05 PM

 */

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

#include "PciRegs.h"
#include "PlxApi.h"
/*  #include "PlxInit.h" */
/*  #include "RegDefs.h" */
#include "Reg9030.h"

/* numbers for data array. */
#define N_SURF 3  /* Can handle up to 3 SURF + 1 TURFIO */
#define N_SCA 256
#define N_CHN 8
#define N_CHP 4
#define N_RF 32  // 32 RF trigger channels per SURF board

#define N_TMO 100    /* number of msec wait for EoE before timed out. */

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

  switch (action) {
  case ClrAll : base_v=base_clr; gpio_v = base_v | clr_all  ; break ; 
  case ClrEvt : base_v=base_clr; gpio_v = base_v | clr_evt  ; break ;
  case LTrig  : gpio_v = base_v | loc_trg  ; break ;
  case EvNoD  : gpio_v = base_v | evn_done ; break ;
  case LabD   : gpio_v = base_v | lab_done ; break ;
  case SclD   : gpio_v = base_v | scl_done ; break ;
  case RFpwD  : gpio_v = base_v | rfp_done ; break ;
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
  return PlxRegisterWrite(PlxHandle, PCI9030_GP_IO_CTRL, base_v ) ;

}

RETURN_CODE cntl_set_TURF(HANDLE PlxHandle, TURF_cntl_action action) {

  U32 base_clr = 0x00000010 ;
  U32 clr_trg  = 0x00000030 ; 

  U32           base_v,gpio_v ;
  RETURN_CODE   rc ;
  char *ch_act[] = {"ClrTrg"} ;

  // Read current GPIO state
  base_v=PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc) ; 

  switch (action) {
  case ClrTrg : base_v=base_clr; gpio_v = base_v | clr_trg ; break ; 
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

  int i;
  U32  DeviceNum ;
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
  for(i=0;i<DeviceNum-1;++i){
    if (PlxPciDeviceFind(&Device[i], &i) != ApiSuccess) 
      return -1 ;

    if (verbose) 
      printf("init_device: device found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
	     Device[i].DeviceId, Device[i].VendorId, Device[i].SerialNumber, 
	     Device[i].BusNumber, Device[i].SlotNumber);
    
    if (PlxPciDeviceOpen(&Device[i], &PlxHandle[i]) != ApiSuccess)
      return -1 ;
    
    PlxPciBoardReset(PlxHandle[i]) ;
  }
  if(verbose)
    printf("init_device: Initialized %d SURF board(s)\n",i-1);


  if(surfonly) return (int)DeviceNum ;

  /* Initialize TURFIO */
  i = DeviceNum-1 ; /* this is for turf closer to CPU than any surf. */
  if (PlxPciDeviceFind(&Device[i], &i) != ApiSuccess) 
    return -1 ;
  if (verbose) 
    printf("init_device: device found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
	   Device[i].DeviceId, Device[i].VendorId, Device[i].SerialNumber, 
	   Device[i].BusNumber, Device[i].SlotNumber);
  
  if (PlxPciDeviceOpen(&Device[i], &PlxHandle[i]) != ApiSuccess) 
    return -1;

  PlxPciBoardReset(&PlxHandle[i]) ;
  if(verbose)
    printf("init_device: Initialized TURFIO board\n");

  return (int)DeviceNum ;
}

// Clears boards for start of data taking 
void clear_device(HANDLE *PlxHandle,int n_dev){
  int i;
  RETURN_CODE      rc ;

  printf("*** Clearing devices ***\n");

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

  for(i=1;i<n_dev;++i){
    printf(" GPIO register contents SURF %d = %x\n",i,
	   PlxRegisterRead(PlxHandle[i], PCI9030_GP_IO_CTRL, &rc)) ; 
    if (cntl_set_SURF(PlxHandle[i], ClrAll) != ApiSuccess)
      printf("  failed to send clear all event pulse on SURF %d.\n",i) ;
  }
  printf("\n Try to test interrupt control register.\n") ;

  for(i=1;i<n_dev;++i)
      printf(" Int reg contents SURF %d = %x\n",i,
	     PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ; 
  
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

  printf(" Try to write 9030 GPIO.\n") ;
  if (cntl_set_TURF(PlxHandle[i], ClrTrg) != ApiSuccess)
    printf("  failed to send clear pulse on TURF %d.\n",i) ;

/*    printf(" Try to read 9030 GPIO.  %x\n", */
/*  	 PlxRegisterRead(PlxHandle[i], PCI9030_GP_IO_CTRL, &rc)) ; */
/*    if (rc != ApiSuccess) printf("  failed to read TURFIO GPIO .\n") ; */

  printf("\n Try to test interrupt control register.\n") ;
  
  printf(" int reg contents TURFIO = %x\n",
	 PlxRegisterRead(PlxHandle[i], PCI9030_INT_CTRL_STAT, &rc)) ; 

  return;
}


void write_data(char *directory, unsigned short wv_data[N_SURF][N_CHN][],unsigned long trigtime) {

  char f_name[128] ;
  FILE *wv_file ;
  int  n ;

  sprintf(f_name, "%s/surf_data.%lo", directory, trigtime) ;
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

void write_turf(char *directory, TURFIO_struct *data_turf) {

  char f_name[128] ;
  FILE *wv_file ;
  int  n ;

  sprintf(f_name, "%s/turf_data.%lo", directory, data_turf->time) ;
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
      case 'd': *directory = *(++argv) ;
	--argn ; break ;
      case 'n': sscanf(*(++argv), "%d", n) ;
	--argn ; break ;
      default : printf(" init_param: undefined parameter %s\n", *argv) ;
      }
    }
  }
  
  return 0;
}

/* ----------------===============----------------------- */
int main(int argnum, char **argval) {

  HANDLE           PlxHandle[N_SURF+1] ;
  DEVICE_LOCATION  Device[N_SURF+1] ;
  PLX_INTR         PlxIntr, Plxstate ;
  RETURN_CODE      rc ;
  //  U32              port, var[7] ;

  unsigned short  d_in=0;//,d_out;
  unsigned short  evNo;
  unsigned short data_array[N_SURF][N_CHN][N_SCA]; // Read only one chip per trigger. PM
  float data_calib[N_SURF][N_CHN][N_SCA]; 
  unsigned short data_scl[N_RF];
  unsigned short data_rfpw[N_RF];
  TURFIO_struct data_turf;
  int i, j, k, tmo=0,n=0 ; 
  int n_dev, chip[N_SURF], n_ev=0 ;

  float mean_dt=0;
  float mean_rms[N_SURF][N_CHP][N_CHN];

  char *dir_n ;
  int status_ok,tmpGPIO;
  
  for(k=0;k<N_SURF;++k)
    for(j=0;j<N_CHP;++j) 
      for(i=0;i<N_CHN;++i)
	mean_rms[k][j][i]=0;
  
  printf(" Start test program !!\n") ;

  init_param(argnum, argval, &dir_n, &n_ev) ;
  if (dir_n == NULL) dir_n = "./data" ;   /* this is default diretory name. */
/*   if (n_ev == 0) n_ev = 1000 ; */

  if (verbose) printf("  size of buffer is %d bytes.\n", sizeof(d_in)) ;

  // Initialize devices
  if ((n_dev = init_device(PlxHandle, Device)) < 0)
    return 0 ;

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


  while ((n++ < n_ev) || (n_ev == 0)) {  
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
    status_ok=TRUE;

    while (!((tmpGPIO=PlxRegisterRead(PlxHandle[0], PCI9030_GP_IO_CTRL, &rc)) & 0x800) &&
	   (selftrig?(++tmo<N_TMO):1)){
      if(verbose>3) printf("SURF 1 GPIO: 0x%x %d\n",tmpGPIO,tmpGPIO); 
      usleep(1000);
    }

    if (tmo == N_TMO){
      printf(" Timed out (%d ms) while waiting for Evt_F flag in self trigger mode.\n", N_TMO) ;
      continue;
    }

    if(verbose) printf("Triggered, event %d (by software counter).\n",n);

    usleep (100) ;  /* I doubt EoE is issued in proper timing. */
    
    /* Loop over SURF boards and read out */
    for(k=0;k<n_dev-1;++k){
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
      if(verbose) printf("SURF %d, event no %d\n",k,d_in);
      if(k==0) evNo=d_in;
      if(cntl_set_SURF(PlxHandle[k],EvNoD)!=ApiSuccess)
	printf("Failed to set EvNoD on SURF %d\n",k);

      usleep(1000000); // wait a bit according to Jing
      
      if(!(PlxRegisterRead(PlxHandle[k], PCI9030_GP_IO_CTRL, &rc) & 0x4000)){
	printf("No Lab_F flag, bailing on the event\n");
	status_ok=FALSE;
	continue;
      }
	  
      // Read LABRADOR data
      for (j=0 ; j<N_SCA ; j++) {
	for (i=0 ; i<N_CHN ; i++) {
	  if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	      != ApiSuccess)
	    printf("  failed to read IO. surf=%d, chn=%d sca=%d\n", k, i, j) ;
	  // Record which chip is being read out
	  if(verbose>2) 
	    printf("SURF %d, CHP %d,CHN %d, SCA %d: %d (HITBUS=%d)\n",k,i,j,d_in,d_in>>14,d_in & 0x1000);
	  if (i==0 && j==0)
	    chip[k-1] = d_in>>14;
	  else if((d_in>>14)!=chip[k-1])
	    printf(" data xfer error; chip %d (instead of %d) seen on surf=%d, chn=%d sca=%d\n",
		   d_in>>14,chip[k-1],k,i,j);
	  // Check if 13 bit is zero
	  if ((d_in&0x2000) != 0)
	    printf(" data xfer error; bit 13 != 0 on surf=%d, chn=%d sca=%d\n",k,i,j);
	  
	  /* put data into array */
	  if(d_in & 0x1000) data_array[k-1][i][j] = 0; // This is hitbus bin
	  else data_array[k-1][i][j] = d_in & 0xfff ;
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

    }  /* this is closing for(n_dev) loop. */

    if(!surfonly){

    /* Readout TURFIO */
    k=0;
    
    if(verbose) printf(" GPIO register contents TURFIO = %x\n",
		       PlxRegisterRead(PlxHandle[k], PCI9030_GP_IO_CTRL, &rc)) ; 
    if(verbose) printf(" int reg contents TURFIO = %x\n", 
		       PlxRegisterRead(PlxHandle[k], PCI9030_INT_CTRL_STAT, &rc)) ; 

    for (j=0 ; j<16 ; j++) { // Loop 16 times over return structure
      for (i=0; i<16 ; i++) {
	if (PlxBusIopRead(PlxHandle[k], IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	    != ApiSuccess)
	  printf("  failed to read TURF IO port. loop %d, word %d.\n",j,i) ;
	if((verbose>2 && j==0) || verbose>3)
	  printf("TURFIO %d (%d): %d\n",i,j,d_in);
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
      printf("raw: %d\nL3A: %d\n1PPS: %lo\ntime: %lo\nint: %lo\n",
	     data_turf.rawtrig,data_turf.L3Atrig,data_turf.pps1,data_turf.time,
	     data_turf.interval);
      for(i=8;i<16;++i)
	printf("Rate[%d][%d]=%d\n",i/4-2,i%4,data_turf.rate[i/4-2][i%4]);
    }
    }

    if(verbose) printf("Done reading\n");

    // Save data
    //write_turf(dir_n,&data_turf);
    //write_data(dir_n,data_array,data_turf.time) ;

    // Some performance indicators
    if(!surfonly){
      mean_dt=mean_dt*(n-1)/n+data_turf.interval/n;
      printf("Trig no: %d, trig time: %lo, 1PPS: %lo, dt: %lo\n",data_turf.rawtrig,
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
	  else data_calib[k][j][i]=(data_array[k][j][i]-1600)/1.6-1.3;
	}
    // Calculate mean rms
    for(k=0;k<n_dev-1;++k)
      for(j=0;j<N_CHN;++j){
	float sum=0;
	for(i=0;i<N_SCA;++i)
	  sum+=data_calib[k][j][i]*data_calib[k][j][i];
	mean_rms[k][chip[k]][j]=mean_rms[k][chip[k]][j]*(n-1)/n+(sum/(N_SCA-5))/n;
      }
    // Display mean rms
    for(k=0;k<n_dev-1;++k){
      printf("Board %d RMS\n",k);
      for(j=0;j<N_CHP;++j){
	for(i=0;i<N_CHN;++i)
	  printf("%.2f ",mean_rms[k][j][i]);
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
