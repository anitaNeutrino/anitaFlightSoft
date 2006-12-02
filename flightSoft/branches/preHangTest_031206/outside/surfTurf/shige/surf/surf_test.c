/* 
     Test program for PLX 9030 chip that controls LABRADOR.
                                      started 27-Nov-04  SM.

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
#define N_SCA 256
#define N_CHN 8
#define N_CHP 4

#define N_TMO 100    /* number of msec wait for EoE before timed out. */

typedef enum __control_act
{  ClrBsy,
   ClrEvt,
   ClrBE,
   LTrig
} cntl_action ;

int verbose = FALSE ; /* control debug print out. */

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

RETURN_CODE cntl_set(HANDLE PlxHandle, cntl_action action, int ch) {

  U32 base_v =  0x02492082 ;
  U32 clr_evt = 0x00000004 ;
  U32 clr_bsy = 0x00800000 ; 
  U32 loc_trg = 0x00000100 ;

  U32           gpio_v ;
  RETURN_CODE   rc ;
  char *ch_act[] = {"ClrEvt", "ClrBsy", "ClrBE", "LTrig"} ;

  if (ch & 1) base_v |= 0x020000 ;
  if (ch & 2) base_v |= 0x100000 ;

  switch (action) {
  case ClrEvt : gpio_v = base_v | clr_evt ;  break ;
  case ClrBsy : gpio_v = base_v | clr_bsy ;  break ; 
  case ClrBE :  gpio_v = base_v | clr_bsy | clr_evt ; break ; 
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

  /* Jing's logic requires trigger bit to be kept high.  9-Dec SM */
  if (action == LTrig) return rc ;
  /* return to the main, after reseting GPIO to base value. */
  return PlxRegisterWrite(PlxHandle, PCI9030_GP_IO_CTRL, base_v ) ;

}

int init_device(HANDLE *PlxHandle, DEVICE_LOCATION *Device) {

  U32  DeviceNum ;
  U32  n=0 ;  /* this is the numebr of board from furthest from CPU. */

  Device->BusNumber       = (U8)-1;
  Device->SlotNumber      = (U8)-1;
  Device->VendorId        = (U16)-1;
  Device->DeviceId        = (U16)-1;
  Device->SerialNumber[0] = '\0';

  /* need to go through PlxPciDeviceFind twice.  1st for counting devices
     with "DeviceNum=FIND_AMOUNT_MATCHE" and 2nd to get device info. */

  DeviceNum = FIND_AMOUNT_MATCHED;
  if (PlxPciDeviceFind(Device, &DeviceNum) != ApiSuccess ||
      DeviceNum > 12) return -1 ;
  if (verbose) printf("init_device: found %d PLX devices.\n", DeviceNum) ;

  if (PlxPciDeviceFind(Device, &n) != ApiSuccess) 
    return -1 ;

  if (verbose) 
    printf("init_device: device found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
	 Device->DeviceId, Device->VendorId, Device->SerialNumber, 
	 Device->BusNumber, Device->SlotNumber) ;

  if (PlxPciDeviceOpen(Device, PlxHandle) != ApiSuccess)
    return -1 ;

  PlxPciBoardReset(*PlxHandle) ;

  return (int)DeviceNum ;
}

void write_data(char *directory, short *wv_data) {

  char f_name[128] ;
  FILE *wv_file ;
  int  n ;

  sprintf(f_name, "%s/surf_data.%d", directory, time(NULL)) ;
  printf(" write_data: save data to %s\n", f_name) ;

  if ((wv_file=fopen(f_name, "w")) == NULL) {
    printf(" write_data: failed to open wv file, %s\n", f_name) ;
    return ; 
  }

  if ((n=fwrite(wv_data, sizeof(short), N_SCA*N_CHN*N_CHP, wv_file))
      != N_SCA*N_CHN*N_CHP)
    printf(" write_data: failed to write all data. wrote only %d.\n", n) ;

  fclose (wv_file) ;
}

/* purse command arguments to initialize parameter(s). */
int init_param(int argn, char **argv, char **directory, int *n) {

  int ch=0 ;

  *directory = NULL ;   /* just make sure it points NULL. */

  while (--argn) {
    if (**(++argv) == '-') {
      switch (*(*argv+1)) {
      case 'v': verbose = TRUE ; break ;
      case 'd': *directory = *(++argv) ;
	--argn ; break ;
      case 'n': sscanf(*(++argv), "%d", n) ;
	--argn ; break ;
      case '3': ch++ ; 
      case '2': ch++ ; 
      case '1': ch++ ; 
      case '0': break ;
      default : printf(" init_param: undefined parameter %s\n", *argv) ;
      }
    }
  }
  
  return ch ;
}

/* ----------------===============----------------------- */
int main(int argnum, char **argval) {

  HANDLE           PlxHandle ;
  DEVICE_LOCATION  Device ;
  PLX_INTR         PlxIntr, Plxstate ;
  RETURN_CODE      rc ;
  U32              port, var[7] ;

  short  d_in=0, d_out ;
  short  data_array[N_CHP][N_CHN][N_SCA] ;
  int i, j=0, n=0 ; 
  int n_dev, chip, n_ev=0 ;

  char *dir_n ;

  printf(" Start test program !!\n") ;

  chip = init_param(argnum, argval, &dir_n, &n_ev) ;
  if (dir_n == NULL) dir_n = "./data" ;   /* this is default diretory name. */
/*   if (n_ev == 0) n_ev = 1000 ; */

  if (verbose) printf("  size of buffer is %d bytes.\n", sizeof(d_in)) ;

  if ((n_dev = init_device(&PlxHandle, &Device)) < 0)
    return 0 ;

  /* init. 9030 I/O descripter (to enable READY# input |= 0x02.) */
  if(PlxRegisterWrite(PlxHandle, PCI9030_DESC_SPACE0, 0x00800000) 
     != ApiSuccess)  printf("  failed to set Sp0 descriptor.\n" ) ;

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

  printf("\n Try to test interrupt control register.\n") ;

  printf(" Int reg contents = %x\n",
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
  while ((n_ev == 0) || (n++ < n_ev)) {  
  /* initialize data_array with 0, should be just inside the outmost loop.*/
  bzero(data_array, sizeof(data_array)) ;

  for (chip = 0 ; chip < N_CHP ; chip++) {

    
  /* first send Clear Event pulse. */
  if (cntl_set(PlxHandle, ClrEvt, chip) != ApiSuccess)
    printf("  failed to send clear event pulse.\n") ;

  if (verbose) printf(" Try to take data for chip #%d.\n\n", chip) ; 
  
  if (verbose) printf(" GPIO register contents = %x, return code = %x.\n",
		   PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc), rc) ; 

  /* --- send a trigger ---- */
  printf(" try to trigger it locally.\n") ;
  if (cntl_set(PlxHandle, LTrig, chip) != ApiSuccess)
      printf("  failed to set LTrig flag.\n") ;

  while (!(PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc) & 0x20) &&
	 j++<N_TMO) usleep(1000) ;
  if (j == N_TMO) 
    printf(" Timed out (%d ms) while waiting for EoE flag.\n", N_TMO) ;

  usleep (100) ;  /* I doubt EoE is issued in proper timing. */
  printf(" GPIO register contents = %x\n",
	 PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc)) ; 
  printf(" int reg contents = %x\n",
	 PlxRegisterRead(PlxHandle, PCI9030_INT_CTRL_STAT, &rc)) ; 

  /* test write/read local buss. */
/*   d_out = 0x0f0f ; */
/*   if (PlxBusIopWrite(PlxHandle, IopSpace0, 0x0, TRUE, &d_out, 2, BitSize16)  */
/*       != ApiSuccess) */
/*     printf("  failed to write IO port.\n") ; */
/*   if (PlxBusIopRead(PlxHandle, IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)  */
/*       != ApiSuccess) */
/*     printf("  failed to write IO port.\n") ; */
/*   printf(" data written is %x and data read is %x.\n", d_out, d_in) ; */

/*   if (PlxPciBarMap(PlxHandle, 2, &port) != ApiSuccess) */
/*     printf("  failed to map PCI BAR0\n") ; */
/*   if (verbose) printf("  BAR address = %x\n", port) ; */
/* moved from the i,j loop.  */
/*       data = *(U16 *)(port+0x110) ; */

  for (j=0 ; j<N_SCA ; j++) {
    for (i=0 ; i<N_CHN ; i++) {
      if (PlxBusIopRead(PlxHandle, IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
	  != ApiSuccess)
	printf("  failed to read IO. chn=%d sca=%d\n", i, j) ;

      /* put data into array.  1st line is for wird situation on SURF. */
      if (d_in > 0x1800) data_array[chip][i][j] = d_in - 0x1300 ;
      else if (d_in > 0x1000) data_array[chip][i][j] = d_in & 0xfff ;
      else data_array[chip][i][j] = d_in ;

      if (verbose) {
	if (i==0) printf(" data (%02d) = %x ", j, d_in) ;
	else printf("%x ", d_in) ;
      }
    }
    
    if (verbose) printf("\n") ;
  }

  /* to take care of firmware wirdness on the 1st data. */
  if (PlxBusIopRead(PlxHandle, IopSpace0, 0x0, TRUE, &d_in, 2, BitSize16)
      != ApiSuccess)
    printf("  failed to read IO.  @reread 1st data.\n") ;
  data_array[chip][0][0] = d_in ;
  if (verbose) printf(" data(0/0) = %x \n", d_in) ;

  }  /* this is closing for(chip) loop. */

  write_data(dir_n, data_array) ;

  sleep (10) ;
  }  /* closing while loop. */

  PlxPciDeviceClose(PlxHandle );

  return 1 ;
}
