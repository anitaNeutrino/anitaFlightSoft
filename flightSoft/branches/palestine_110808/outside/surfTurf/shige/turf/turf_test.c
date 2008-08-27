/* 
     Test program for PLX 9030 chip that controls LABRADOR.
                                      started 27-Nov-04  SM.

 */

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "PciRegs.h"
#include "PlxApi.h"
/*  #include "PlxInit.h" */
/*  #include "RegDefs.h" */
#include "Reg9030.h"

/* numbers for data array. */
#define N_SCA 256
#define N_CHN 16
#define N_BRD 4

typedef enum __control_act
  {  ClrBsy,
     ClrEvt,
     ClrTrg,
     LTrig
  } cntl_action ;

/* this should reflect the order in "cntl_action". */
char *ch_act[] = {"ClrBsy", "ClrEvt", "ClrTrg", "LTrig"} ;

int verbose = FALSE ; /* control debug print out. */

/********************************************************
*               Global Variables
********************************************************/
/*  BOOLEAN mgEventCompleted; */
U32     ChipTypeSelected;
U8      ChipRevision;


U32 device_find (DEVICE_LOCATION *pDevice) {

  U32 DeviceNum, n=0 ;

  pDevice->BusNumber       = (U8)-1;
  pDevice->SlotNumber      = (U8)-1;
  pDevice->VendorId        = (U16)-1;
  pDevice->DeviceId        = (U16)-1;
  pDevice->SerialNumber[0] = '\0';

  /* need to go through PlxPciDeviceFind twice.  1st for counting devices
     with "DeviceNum=FIND_AMOUNT_MATCHE" and 2nd to get device info. */

  DeviceNum = FIND_AMOUNT_MATCHED;
  if (PlxPciDeviceFind(pDevice, &DeviceNum) != ApiSuccess ||
      DeviceNum > 12) return -1 ;
  if (verbose) printf(" device_find: found %d PLX devices.\n", DeviceNum) ;

  n = DeviceNum-1 ; /* this is for turf closer to CPU than surf. */
  if (PlxPciDeviceFind(pDevice, &n) != ApiSuccess) 
    return -1 ;

  return DeviceNum ;
}

/* 
   send a pulse to a control line through 9030 GPIO.

*/

RETURN_CODE cntl_set(HANDLE PlxHandle, cntl_action action) {

  U32 base_v =  0x02492092 ;
  U32 clr_evt = 0x00000020 ;
  U32 clr_trg = 0x00000100 ; 
  U32 loc_trg = 0x00000004 ;

  U32           gpio_v ;
  RETURN_CODE   rc ;

  switch (action) {
  case ClrEvt : gpio_v = base_v | clr_evt ;  break ;
  case ClrTrg : gpio_v = base_v | clr_trg ; break ; 
  case LTrig :  gpio_v = base_v | loc_trg ; break ;
  default :
    printf(" cntl_set: undfined action, %d\n", action) ;
    return ApiFailed ;
  }
  if (verbose) printf(" cntl_set: action %s, gpio_v = %x\n", 
		      ch_act[(int)action], gpio_v) ; 

  if ((rc = PlxRegisterWrite(PlxHandle, PCI9030_GP_IO_CTRL, gpio_v ))
      != ApiSuccess) {
    printf(" cntl_set : failed to set GPIO to %x.\n", gpio_v) ;
    return rc ;
  }

  return PlxRegisterWrite(PlxHandle, PCI9030_GP_IO_CTRL, base_v ) ;

}

/* purse command arguments to initialize parameter(s). */
void init_param(int argn, char **argv) {

  while (--argn) {
    if (**(++argv) == '-') {
      switch (*(*argv+1)) {
      case 'v': verbose = TRUE ; break ;
      default : printf(" init_param: undefined parameter %s\n", *argv) ;
      }
    }
  }
}

int main(int argnum, char **argval) {

  U8               count ;
  U32              DeviceNum ;
  HANDLE           PlxHandle ;
  static HANDLE    p_lint ;
  DEVICE_LOCATION  Device ;
  PLX_INTR         PlxIntr, Plxstate ;
  RETURN_CODE      rc ;
  U32              port, var[7] ;
  BOOLEAN          ioormem ;

  time_t t_buf ;
  short  d_in=0, d_out ;
  int i, j, n ;

  printf(" Start test program !!\n") ;

  init_param(argnum, argval) ;

  if (verbose) printf("  size of buffer is %d bytes.\n", sizeof(d_in)) ;

  if ((DeviceNum = device_find(&Device)) < 1) {
    printf(" Failed to find device. %d\n", DeviceNum) ;
    return 0 ;
  }

  if (verbose) 
    printf("   device found, %.4x %.4x  [%s - bus %.2x  slot %.2x]\n",
	 Device.DeviceId, Device.VendorId, Device.SerialNumber, 
	 Device.BusNumber, Device.SlotNumber) ;

  if (PlxPciDeviceOpen(&Device, &PlxHandle) != ApiSuccess) {
    printf(" Failed to open Plx device.\n") ;
    return 0 ;
  }

  PlxPciBoardReset(PlxHandle) ;

/*  Get PLX Chip Type */
/*   PlxChipTypeGet(PlxHandle, &ChipTypeSelected, &ChipRevision ); */
/*    if (ChipTypeSelected == 0x0) { */
/*      printf("\n   ERROR: Unable to determine PLX chip type\n"); */
/*      PlxPciDeviceClose(PlxHandle ); */
/*      exit(-1); */
/*    } */
/*    printf(" Chip type:  %04x,  Revision :    %02X\n", */
/*          ChipTypeSelected, ChipRevision );  */

  printf(" GPIO register contents = %x\n",
	 PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc)) ; 

  printf(" Try to write 9030 GPIO.\n") ;

/*    printf(" Try to read 9030 GPIO.  %x\n", */
/*  	 PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc)) ; */
/*    if (rc != ApiSuccess) printf("  failed to read GPIO.\n") ; */

  printf("\n Try to test interrupt control register.\n") ;

  printf(" int reg contents = %x\n",
	 PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ; 

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

  if (PlxIntrDisable(PlxHandle, &PlxIntr) != ApiSuccess)
    printf("  failed to disable interrupt.\n") ;
  printf(" int reg contents = %x, after disable.\n",
	 PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ; 
  PlxIntr.PciMainInt = 0 ;
  if (PlxIntrEnable(PlxHandle, &PlxIntr) != ApiSuccess)
    printf("  failed to enable interrupt.\n") ;
  printf(" int reg contents = %x, after enable Lints.\n",
	 PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ; 
  PlxIntr.PciMainInt = 1 ;
  if (PlxIntrEnable(PlxHandle, &PlxIntr) != ApiSuccess)
    printf("  failed to enable interrupt.\n") ;
/*    printf(" int reg contents = %x, after enable Main.\n", */
/*  	 PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ;  */

  if (PlxIntrStatusGet(PlxHandle, &Plxstate) != ApiSuccess)
    printf("  fialed to get interrupt status.\n") ;
  printf("  Intr(Main/Lint1/Lint2) = %01d/%01d/%01d\n",
	 Plxstate.PciMainInt, Plxstate.IopToPciInt, Plxstate.IopToPciInt_2) ;

  /* -- initialize PCI board ---- */
  if (cntl_set(PlxHandle, ClrTrg) != ApiSuccess)
    printf("  failed to send clear event pulse.\n") ;

  printf(" GPIO register contents = %x\n",
	 PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc)) ; 

/*   printf(" try to trigger it locally.\n") ; */
/*   if (cntl_set(PlxHandle, LTrig) != ApiSuccess) */
/*       printf("  failed to set LTrig flag.\n") ; */

/*   for (j=0 ; j++<10 ; usleep(100)) ; */

  printf(" GPIO register contents = %x\n",
	 PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc)) ; 
  printf(" int reg contents = %x\n",
	 PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ; 

  /* check I/O port. */
/*    printf(" test I/O port of 9030.\n") ; */
/*    port = PlxPciConfigRegisterRead */
/*      (Device.BusNumber, Device.SlotNumber, CFG_BAR1, &rc) ; */
/*    printf(" port = %x, rc = %x (ApiSuccess = %x).\n", port, rc, ApiSuccess) ; */

/*    port &= ~(1<<0) ; */

  /* this is to enable READY# input. */
  if(PlxRegisterWrite(PlxHandle, PCI9030_DESC_SPACE0, 0x00800000)
     != ApiSuccess)  printf("  failed to set Sp0 descriptor.\n" ) ;

  /* test write/read local buss. */
  d_out = 0x0f0f ;
  if (PlxBusIopWrite(PlxHandle, IopSpace0, 0x0, TRUE, &d_out, 2, BitSize16)
      != ApiSuccess)
    printf("  failed to write IO port.\n") ;
/*   if (PlxBusIopRead(PlxHandle, IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16) */
/*       != ApiSuccess) */
/*     printf("  failed to write IO port.\n") ; */
/*   printf(" data written is %x and data read is %x.\n", d_out, d_in) ; */

  while (cntl_set(PlxHandle, ClrTrg) == ApiSuccess) {
    time(&t_buf) ;
    printf("\n time : %s\n", asctime(localtime(&t_buf))) ;

/*     if (cntl_set(PlxHandle, LTrig) != ApiSuccess) */
/*       printf("  failed to set LTrig flag.\n") ; */

  for (j=0 ; j<16 ; j++) {
    for (i=0, n=FALSE ; i<16 ; i++) {
      if (PlxBusIopRead(PlxHandle, IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	  != ApiSuccess)
	printf("  failed to read IO port.\n") ;
      if (i==0) printf(" data (%02d) = %02x ", j, d_in) ;
      else printf("%02x ", d_in) ;
      if (cntl_set(PlxHandle, LTrig) != ApiSuccess)
	printf("  failed to send local trigger pulse.\n") ;
      if (cntl_set(PlxHandle, ClrEvt) != ApiSuccess)
	printf("  failed to send clear event pulse.\n") ;
/*       usleep(2) ; */
      if (d_in != (16*j+i)) n = TRUE ;
    }
    if (n) printf(" <--\a") ;
    printf("\n") ;
  }

  fflush(stdout) ;
  sleep (5) ;
  }

  PlxPciDeviceClose(PlxHandle );

  return 1 ;
}
