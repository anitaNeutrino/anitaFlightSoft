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

// Plx includes
#include "PciRegs.h"
#include "PlxApi.h"
/*  #include "PlxInit.h" */
/*  #include "RegDefs.h" */
#include "Reg9030.h"

// Anita includes
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"


//Definitions
#define EVT_F 0x800
#define LAB_F 0x40000

#define MAX_SURFS 12

typedef HANDLE PlxHandle_t;
typedef DEVICE_LOCATION PlxDevLocation_t;
typedef RETURN_CODE PlxReturnCode_t;

typedef enum __SURF_control_act
{  ClrAll,
   ClrEvt,
   LTrig,
   EvNoD,
   LabD,
   SclD,
   RFpwD
} SurfControlAction ;

typedef enum __TURF_control_act
{ RstTurf,
  ClrTrg
} TurfControlAction ;

typedef struct {
    int bus;
    int slot;
} BoardLocStruct;
      

//Forward Declarations
int initializeDevices(PlxHandle_t *surfHandles, PlxHandle_t *turfioHandle, PlxDevLocation_t *surfLoc, PlxDevLocation_t *turfioLoc);
void clearDevices(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle);
void setDACThresholds(PlxHandle_t *surfHandles, int threshold); //Only does one threshold at the moment
PlxReturnCode_t setSurfControl(PlxHandle_t surfHandle, SurfControlAction action);
PlxReturnCode_t setTurfControl(PlxHandle_t turfioHandle, TurfControlAction action);
char *surfControlActionAsString(SurfControlAction action);
char *turfControlActionAsString(TurfControlAction action);
int readConfigFile();
int init_param(int argn, char **argv, char **directory, int *n, int *dacVal);
void writeData(char *directory, unsigned short *wv_data,unsigned long evNum);
void writeTurf(char *directory, TurfioStruct *data_turf,unsigned long evNum);
inline unsigned short byteSwapUnsignedShort(unsigned short a){
    return (a>>8)+(a<<8);
}


//Global Variables
BoardLocStruct turfioPos;
BoardLocStruct surfPos[MAX_SURFS];
int printToScreen=0;
int numSurfs=0;

/* numbers for data array. */
#define N_SCA 256
#define N_CHN 8
#define N_CHP 4
#define N_RF 32  // 32 RF trigger channels per SURF board

#define N_TMO 100    /* number of msec wait for Evt_f before timed out. */
#define N_FTMO 200    /* number of msec wait for Lab_f before timed out. */
//#define N_FTMO 2000    /* number of msec wait for Lab_f before timed out. */
#define DAC_CHAN 4 /* might replace with N_CHP at a later date */
#define PHYS_DAC 8
#define DAC_CNT 10000000
#define BIT_NO 16
#define BIT_EFF 12


int verbose = 0 ; /* control debug print out. */
int printout = FALSE ; /* control data printout for debugging. */
int selftrig = FALSE ;/* self-trigger mode */
int surfonly = FALSE; /* Run in surf only mode */
int writedata = FALSE; /* Default is not to write data to a disk */
int doDacCycle = TRUE; /* Do a cycle of the DAC values */
int rstturf = FALSE ;


/********************************************************
 *               Global Variables
 ********************************************************/
/*  BOOLEAN mgEventCompleted; */
//U32     ChipTypeSelected;
//U8      ChipRevision;



/* ----------------===============----------------------- */
int main(int argnum, char **argval) {

    PlxHandle_t surfHandles[numSurfs];
    PlxHandle_t turfioHandle;
    PlxDevLocation_t surfLocation[numSurfs];
    PlxDevLocation_t turfioLocation;
    PlxReturnCode_t      rc ;
    //  U32              port, var[7] ;
    int retVal=0;
    unsigned short  dataWord=0;
    unsigned short  evNo=0;
    unsigned short data_array[numSurfs][N_CHN][N_SCA]; 
// Read only one chip per trigger. PM
    unsigned short data_scl[N_RF];
    unsigned short data_rfpw[N_RF];
    unsigned long numChipEvents[N_CHP];
    TurfioStruct data_turf;
    int i=0, j, ftmo=0,tmo=0,doingEvent=0 ; 
    int numDevices, n_ev=0 ;
    float mean_dt=0;
    float mean_rms[numSurfs][N_CHP][N_CHN];
    double chanMean[numSurfs][N_CHP][N_CHN];
    double chanMeanSqd[numSurfs][N_CHP][N_CHN];
    unsigned long chanNumReads[numSurfs][N_CHP][N_CHN];
    char *dir_n ;
    int tmpGPIO;
    int dacVal=0;
    int surf,chan,chip,samp,rf;

  
    for(surf=0;surf<numSurfs;++surf)
	for(chip=0;chip<N_CHP;++chip) 
	    for(chan=0;chan<N_CHN;++chan) {
		mean_rms[surf][chip][chan]=0;
		chanMean[surf][chip][chan]=0;
		chanMeanSqd[surf][chip][chan]=0;
		chanNumReads[surf][chip][chan]=0;
	    }
  
    printf(" Start test program !!\n") ;

    retVal=readConfigFile();
    init_param(argnum, argval, &dir_n, &n_ev, &dacVal) ;
    if (dir_n == NULL) dir_n = "./data" ; /* this is default directory name. */
    /*   if (n_ev == 0) n_ev = 1000 ; */

    if (verbose) printf("  size of buffer is %d bytes.\n", sizeof(dataWord)) ;

    // Initialize devices
    if ((numDevices = initializeDevices(surfHandles,&turfioHandle,
					surfLocation,&turfioLocation)) < 0) {
	printf("Problem initializing devices\n");
	return 0 ;
    }

    // Clear devices 
    clearDevices(surfHandles,turfioHandle);

/* special initialization of TURF.  may not be necessary.  12-Jul-05 SM. */
    if (rstturf) {
	setTurfControl(turfioHandle,RstTurf) ;
	for(i=9 ; i++<100 ; usleep(1000)) ;
    }
  
    /* set DAC value to a default value. */
    setDACThresholds(surfHandles,dacVal);

    while ((doingEvent++ < n_ev) || (n_ev == 0)) {
	/* initialize data_array with 0, 
	   should be just inside the outmost loop.*/
	bzero(data_array, sizeof(data_array)) ;

	if (selftrig){  /* --- send a trigger ---- */ 
	    printf(" try to trigger it locally.\n") ;
	    for(surf=0;i<numSurfs;++surf) 
		if (setSurfControl(surfHandles[surf], LTrig) != ApiSuccess) 
		    printf("  failed to set LTrig flag on SURF %d.\n",surf) ;
	}
	/* Wait for ready to read (Evt_F) */
	tmo=0;
	if(verbose) printf("Waiting for Evt_F on SURF 1-1\n");
	while (!((tmpGPIO=PlxRegisterRead(surfHandles[0], PCI9030_GP_IO_CTRL, &rc)) & EVT_F) && (selftrig?(++tmo<N_TMO):1)){
	    if(verbose>3) printf("SURF 1 GPIO: 0x%x %d\n",tmpGPIO,tmpGPIO);
	    if(selftrig) usleep(100); /*RJN put it back in for selftrig */
	    usleep(100); /*GV removed, RJN put it back in as it completely abuses the CPU*/
	}

	if (tmo == N_TMO){
	    printf(" Timed out (%d ms) while waiting for Evt_F flag in self trigger mode.\n", N_TMO) ;
	    continue;
	}

	if(verbose) printf("Triggered, event %d (by software counter).\n",doingEvent);

	
	//Loop over SURFs and read out data
	for(surf=0;surf<numSurfs;surf++){
	    if(verbose)
		printf(" GPIO register contents SURF %d = %x\n",surf,
		       PlxRegisterRead(surfHandles[surf], PCI9030_GP_IO_CTRL, &rc)) ; 
	    if(verbose)
		printf(" int reg contents SURF %d = %x\n",surf,
		       PlxRegisterRead(surfHandles[surf], PCI9030_INT_CTRL_STAT, &rc)) ; 

	    // Readout event number
	    if (PlxBusIopRead(surfHandles[surf], IopSpace0, 0x0, TRUE, &dataWord, 2, BitSize16)
		!= ApiSuccess)
		printf("  failed to read event number on SURF %d\n", surf) ;
	    if(verbose) printf("SURF %d, ID %d, event no %d\n",surf,dataWord>>14,dataWord);
	    
	    if(surf==0) evNo=dataWord&0xfff;
	    if(setSurfControl(surfHandles[surf],EvNoD)!=ApiSuccess)
		printf("Failed to set EvNoD on SURF %d\n",surf);

	    // Wait for Lab_F
	    ftmo=0;
	    int testVal=0;
	    if(verbose) printf("SURF %d, waiting for Lab_F\n",surf);
	    testVal=PlxRegisterRead(surfHandles[surf],PCI9030_GP_IO_CTRL, &rc);
	    while(!(testVal & LAB_F) &&
		  (++ftmo<N_FTMO)){
		//	printf("testVal %x\n",testVal);
		usleep(1); // RJN/GV changed from usleep(1000)
		testVal=PlxRegisterRead(surfHandles[surf], PCI9030_GP_IO_CTRL, &rc);
	    }       
	    if (ftmo == N_FTMO){
		printf("No Lab_F flag for %d ms, bailing on the event\n",N_FTMO);
		continue;
	    }
    
  
	    //      Read LABRADOR data
	    //      int firstTime=1;
	    for (samp=0 ; samp<N_SCA ; samp++) {
		for (chan=0 ; chan<N_CHN ; chan++) {
		    int readOkay=-1;
		    if (PlxBusIopRead(surfHandles[surf], IopSpace0, 0x0, TRUE, &dataWord, 2, BitSize16)
			!= ApiSuccess)
			printf("  failed to read IO. surf=%d, chn=%d sca=%d\n", surf, chan, samp) ;
		    else readOkay=1;
		    // Record which chip is being read out
		    if( (((dataWord&0xfff)-2750)>400) && !(dataWord & 0x1000) ) 
			printf("SURF %d, chip#= %d Chan#= %d SCA#= %d: %d\n",surf,(dataWord>>14),chan,samp,((dataWord&0xfff)-2750));
		    if(verbose>2) 
			printf("SURF %d, CHP %d,CHN %d, SCA %d: %d (HITBUS=%d) %d\n",surf,dataWord>>14,chan,samp,dataWord,dataWord & 0x1000,dataWord&0xfff);
		    
		    // Check if 13 bit is zero
		    if ((dataWord&0x2000) != 0)
			printf(" data xfer error; bit 13 != 0 on surf=%d, chn=%d sca=%d\n",surf,i,samp);	  
		    data_array[surf][i][samp] = dataWord ;//& 0xfff ; 
		}
	    }
	    
	    if(setSurfControl(surfHandles[surf],LabD)!=ApiSuccess)
		printf("Failed to set LabD on SURF %d\n",surf);
      
	    // Read trigger scalers
	    for(i=0;i<N_RF;++i){
		if (PlxBusIopRead(surfHandles[surf], IopSpace0, 0x0, TRUE, &dataWord, 2, BitSize16)
		    != ApiSuccess)
		    printf("  failed to read IO. surf=%d, rf scl=%d\n", surf, i) ;
		data_scl[i]=dataWord;
		//	if(verbose>1) 
		//	if(( (i % 8) ) == 0) {
		//	    printf("\n");
		//	    printf("SURF %d, SCL %d: %d",surf,i,dataWord);
		//	} else 
		//	  printf(" %d",dataWord);
	    }
      
	    if(setSurfControl(surfHandles[surf],SclD)!=ApiSuccess)
		printf("Failed to set SclD on SURF %d\n",surf);

	    // Read RF power 
	    for(rf=0;rf<N_CHN;++rf){
		if (PlxBusIopRead(surfHandles[surf], IopSpace0, 0x0, TRUE, &dataWord, 2, BitSize16)
		    != ApiSuccess)
		    printf("  failed to read IO. surf=%d, rf pw=%d\n", surf, rf) ;
		data_rfpw[rf]=dataWord;
		if(verbose>1) 
		    printf("SURF %d, RF %d: %d\n",surf,rf,dataWord);
	    }
      
	    if(setSurfControl(surfHandles[surf],RFpwD)!=ApiSuccess)
		printf("Failed to set RFpwD on SURF %d\n",surf);

	}  //Close the numSurfs loop

	if(!surfonly){
	    
	    if(verbose) 
		printf(" GPIO register contents TURFIO = %x\n",
		       PlxRegisterRead(turfioHandle,PCI9030_GP_IO_CTRL, &rc)); 
	    if(verbose) 
		printf(" int reg contents TURFIO = %x\n", 
		       PlxRegisterRead(turfioHandle,PCI9030_INT_CTRL_STAT,&rc)) ; 
	    
	    // Burn one TURFIO read
	    if (PlxBusIopRead(turfioHandle,IopSpace0,0x0,
			      TRUE, &dataWord, 2, BitSize16)!= ApiSuccess)
		printf("  failed to read TURF IO port. Burn read.\n");

	    data_turf.interval=0;
	    data_turf.time=10;
	    data_turf.rawtrig=10;
	    if(verbose) printf("TURF Handle number: %d\n",surf);
	    for (j=0 ; j<16 ; j++) { // Loop 16 times over return structure
		for (i=0; i<16 ; i++) {
		    if (PlxBusIopRead(turfioHandle, IopSpace0, 0x0, TRUE, &dataWord, 2, BitSize16)
			!= ApiSuccess)
			printf("  failed to read TURF IO port. loop %d, word %d.\n",j,i) ;
		    // Need to byteswap TURFIO words. This might get changed in hardware, so beware. PM 04/01/05
		    dataWord=byteSwapUnsignedShort(dataWord);
		    if((verbose>2 && j==0) || verbose>3)
			printf("TURFIO %d (%d): %hu\n",i,j,dataWord);
		    if(j==0){ //Interpret on the first pass only
			switch(i){
			    case 0: data_turf.rawtrig=dataWord; break;
			    case 1: data_turf.L3Atrig=dataWord; break;
			    case 2: data_turf.pps1=dataWord; break;
			    case 3: data_turf.pps1+=dataWord*65536; break;
			    case 4: data_turf.time=dataWord; break;
			    case 5: data_turf.time+=dataWord*65536; break;
			    case 6: data_turf.interval=dataWord; break;
			    case 7: data_turf.interval+=dataWord*65536; break;
			    default: data_turf.rate[i/4-2][i%4]=dataWord; break;
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
	    if(!surfonly) writeTurf(dir_n,&data_turf,evNo);
	    writeData(dir_n,&data_array[0][0][0],evNo) ; 
/*RJN hack in case turf.time not working */
	}

	// Some Trigger performance indicators
	if(!surfonly){
	    mean_dt=mean_dt*(doingEvent-1)/doingEvent+(float)data_turf.interval/doingEvent;
	    //      printf("\n\n Trig no: %hu, trig time: %lu, 1PPS: %lu, dt: %lu\n",data_turf.rawtrig,
	    //	     data_turf.time,data_turf.pps1,data_turf.interval);
	    //      printf("mean dt: %f\n",mean_dt);
	}else{
	    //      printf("Event no: %d\n",evNo);
	}


	// Calculate mean,rms,std dev
	for(surf=0;surf<numSurfs;++surf) {
	    for(chan=0;chan<N_CHN;++chan){
		double sum[N_CHP]={0};
		double sumSqd[N_CHP]={0};
		int tempNum[N_CHP]={0};
		for(samp=0;samp<N_SCA;++samp) {
		    int tempChip=data_array[surf][chan][samp]>>14;
		    int tempData=data_array[surf][chan][samp] & 0xfff;
		    if(tempData) {
/* 	    printf("%d\t%d\n",tempChip,tempData); */
			sum[tempChip]+=tempData;
			sumSqd[tempChip]+=tempData*tempData;
			tempNum[tempChip]++;
		    }
		}

		for(chip=0;chip<N_CHP;chip++) {	  
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
	if (verbose) {
	    for(surf=0;surf<numSurfs;++surf){
		printf("Board %d Mean, After Event %d (Software)\n",surf,doingEvent);
		for(chip=0;chip<N_CHP;++chip){
		    for(chan=0;chan<N_CHN;++chan)
			printf("%.2f ",chanMean[surf][chip][chan]);
		    printf("\n");
		}
	    }

	    // Display Std Dev
	    for(surf=0;surf<numSurfs;++surf){
		printf("Board %d Std Dev., After Event %d (Software)\n",surf,doingEvent);
		for(chip=0;chip<N_CHP;++chip){
		    for(chan=0;chan<N_CHN;++chan)
			printf("%.2f ",sqrt(chanMeanSqd[surf][chip][chan]-chanMean[surf][chip][chan]*chanMean[surf][chip][chan]));
		    printf("\n");
		}
	    }
	}

	// Clear boards
	for(surf=0;surf<numSurfs;++surf)
	    if (setSurfControl(surfHandles[surf], ClrEvt) != ApiSuccess)
		printf("  failed to send clear event pulse on SURF %d.\n",surf) ;
    
	if(!surfonly)
	    if (setTurfControl(turfioHandle, ClrTrg) != ApiSuccess)
		printf("  failed to send clear event pulse on TURFIO.\n") ;
    

	//    if(selftrig) sleep (2) ;
    }  /* closing master while loop. */

    // Clean up
    for(surf=0;surf<numSurfs;surf++)  PlxPciDeviceClose(surfHandles[surf] );
  
    return 1 ;
}


//Control Fucntions
char *surfControlActionAsString(SurfControlAction action) {
    char* string ;
    switch(action) {
	case ClrAll :
	    string = "ClrAll" ;
	    break ;
	case ClrEvt :
	    string = "ClrEvt" ;
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

char *turfControlActionAsString(SurfControlAction action) {
    char* string ;
    switch(action) {
	case RstTurf :
	    string = "RstTurf" ;
	    break ;
	case ClrTrg :
	    string = "ClrTrg" ;
	    break ;
	default :
	    string = "Unknown" ;
	    break ;
    }
    return string;
}

PlxReturnCode_t setSurfControl(PlxHandle_t surfHandle, SurfControlAction action)
{
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
	case ClrAll : gpio_v = base_clr | clr_all  ; break ; /* RJN changed base_v */
	case ClrEvt : gpio_v = base_clr | clr_evt  ; break ; /* to base_clr */
	case LTrig  : gpio_v = base_clr | loc_trg  ; break ;
	case EvNoD  : gpio_v = base_clr | evn_done ; break ; 
	case LabD   : gpio_v = base_clr | lab_done ; break ;
	case SclD   : gpio_v = base_clr | scl_done ; break ;
	case RFpwD  : gpio_v = base_clr | rfp_done ; break ;
	default :
	    printf(" setSurfControl: undefined action, %d\n", action) ;
	    return ApiFailed ;
    }
    if (verbose) printf(" setSurfControl: action %s, gpio_v = %x\n", 
			surfControlActionAsString(action), gpio_v) ; 

    if ((rc = PlxRegisterWrite(surfHandle, PCI9030_GP_IO_CTRL, gpio_v ))
	!= ApiSuccess) {
	printf(" setSurfControl: failed to set GPIO to %x.\n", gpio_v) ;
	return rc ;
    }

    /* Jing's logic requires trigger bit to be kept high.  9-Dec SM */
    //if (action == LTrig) return rc ;  // Self trigger disabled, PM 31-3-05
    /* return to the main, after reseting GPIO to base value. */
    return PlxRegisterWrite(surfHandle, PCI9030_GP_IO_CTRL, base_clr ) ;

}


PlxReturnCode_t setTurfControl(PlxHandle_t turfioHandle, TurfControlAction action) {

    U32 base_clr = 000000022 ;
    U32 rst_turf = 000000004 ;
    U32 clr_trg  = 000000040 ; 

    U32           gpio_v ;
    PlxReturnCode_t   rc ;
    int i ;

    // Read current GPIO state
    //base_v=PlxRegisterRead(PlxHandle, PCI9030_GP_IO_CTRL, &rc) ; 
    //base_v=base_clr; // Gary's said this should be default PM 03-31-05

    switch (action) {
	case RstTurf : gpio_v = base_clr | rst_turf ; break ;
	case ClrTrg : gpio_v = base_clr | clr_trg ; break ; 
	default :
	    printf(" setTurfControl: undfined action, %d\n", action) ;
	    return ApiFailed ;
    }

    if (verbose) printf(" setTurfControl: action %s, gpio_v = %x\n", 
			turfControlActionAsString(action), gpio_v) ; 

    if ((rc = PlxRegisterWrite(turfioHandle, PCI9030_GP_IO_CTRL, gpio_v ))
	!= ApiSuccess) {
	printf(" setTurfControl : failed to set GPIO to %x.\n", gpio_v) ;
	return rc ;
    }

    if (action == RstTurf)  /* wait a while in case of RstTurf. */
	for(i=0 ; i++<10 ; usleep(1)) ;

    return PlxRegisterWrite(turfioHandle, PCI9030_GP_IO_CTRL, base_clr ) ;

}


int initializeDevices(PlxHandle_t *surfHandles, PlxHandle_t *turfioHandle, PlxDevLocation_t *surfLoc, PlxDevLocation_t *turfioLoc)
/*! 
  Initializes the SURFs and TURFIO, returns the number of devices initialized.
*/
{

    int i,j,countSurfs=0;
    U32  numDevices ;
    PlxDevLocation_t tempLoc[MAX_SURFS+1];
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
    if (verbose) printf("init_device: found %d PLX devices.\n", numDevices) ;

    /* Initialize SURFs */
    for(i=0;i<numDevices;++i){
	if (PlxPciDeviceFind(&tempLoc[i], &i) != ApiSuccess) 
	    return -1 ;

	if (verbose) 
	    printf("init_device: device found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
		   tempLoc[i].DeviceId, tempLoc[i].VendorId, tempLoc[i].SerialNumber, 
		   tempLoc[i].BusNumber, tempLoc[i].SlotNumber);
    
	if(tempLoc[i].BusNumber==turfioPos.bus && tempLoc[i].SlotNumber==turfioPos.slot) {
	    // Have got the TURFIO
	    (*turfioLoc)=tempLoc[i];
	    if (PlxPciDeviceOpen(turfioLoc, turfioHandle) != ApiSuccess) {
		syslog(LOG_ERR,"Error opening TURFIO device");
		fprintf(stderr,"Error opening TURFIO device\n");
		return -1 ;
	    }
	    PlxPciBoardReset(turfioHandle);
	}
	else {
	    for(j=0;j<MAX_SURFS;j++) {
		if(tempLoc[i].BusNumber==surfPos[j].bus && tempLoc[i].SlotNumber==surfPos[j].slot) {
		    // Got a SURF
		    surfLoc[countSurfs]=tempLoc[i];
		    if (PlxPciDeviceOpen(&surfLoc[countSurfs], 
					 &surfHandles[countSurfs]) != ApiSuccess) {
			syslog(LOG_ERR,"Error opening SURF device");
			fprintf(stderr,"Error opening SURF device\n");
			return -1 ;
		    }		
		    PlxPciBoardReset(surfHandles[countSurfs]) ;
		    countSurfs++;    
		}
	    }
	}
    }
    if(verbose) {
	printf("initializeDevices: Initialized %d SURF board(s)\n",countSurfs);
    }


    for(i=0;i<countSurfs;i++) {    
	if (verbose)
	    printf("SURF found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
		   surfLoc[i].DeviceId, surfLoc[i].VendorId, 
		   surfLoc[i].SerialNumber,
		   surfLoc[i].BusNumber, surfLoc[i].SlotNumber);
    }    
    if (verbose)
	printf("TURFIO found, %.4x %.4x [%s - bus %.2x  slot %.2x]\n",
	       (*turfioLoc).DeviceId, (*turfioLoc).VendorId, 
	       (*turfioLoc).SerialNumber,
	       (*turfioLoc).BusNumber, (*turfioLoc).SlotNumber);


    if(countSurfs!=numSurfs) {
	syslog(LOG_WARNING,"Expected %d SURFs, but only found %d",numSurfs,countSurfs);
	fprintf(stderr,"Expected %d SURFs, but only found %d\n",numSurfs,countSurfs);
	numSurfs=countSurfs;
    }
  
    return (int)numDevices ;
}


int readConfigFile() 
/* Load Acqd config stuff */
{
    /* Config file thingies */
    int status=0,count;
    int tempNum;
    int surfSlots[MAX_SURFS];
    int surfBuses[MAX_SURFS];
    int surfSlotCount=0;
    int surfBusCount=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    kvpReset();
    status = configLoad ("Acqd.config","output") ;
    status &= configLoad ("Acqd.config","locations") ;

    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",-1);
	if(printToScreen<0) {
	    syslog(LOG_WARNING,"Couldn't fetch printToScreen, defaulting to zero");
	    printToScreen=0;	    
	}

	turfioPos.bus=kvpGetInt("turfioBus",3); //in seconds
	turfioPos.slot=kvpGetFloat("turfioSlot",10); //in seconds 
	kvpStatus = kvpGetIntArray("surfBus",surfBuses,&tempNum);
	if(kvpStatus!=KVP_E_OK) 
	    syslog(LOG_WARNING,"kvpGetFloatArray(surfBus): %s",
		   kvpErrorString(kvpStatus));
	else {
	    for(count=0;count<tempNum;count++) {
		if(surfBuses[count]!=-1) {
		    surfPos[count].bus=surfBuses[count];
		    surfBusCount++;
		}
	    }
	} 
	kvpStatus = kvpGetIntArray("surfSlot",surfSlots,&tempNum);
	if(kvpStatus!=KVP_E_OK) 
	    syslog(LOG_WARNING,"kvpGetFloatArray(surfSlot): %s",
		   kvpErrorString(kvpStatus));
	else {
	    for(count=0;count<tempNum;count++) {
		if(surfSlots[count]!=-1) {
		    surfPos[count].bus=surfSlots[count];
		    surfSlotCount++;
		}
	    }
	}
	if(surfSlotCount==surfBusCount) numSurfs=surfBusCount;
	else {
	    syslog(LOG_ERR,"Confused config file bus and slot count do not agree");
	    fprintf(stderr,"Confused config file bus and slot count do not agree\n");
	    exit(0);
	}
	
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Acqd.config: %s\n",eString);
    }

    return status;
}

void clearDevices(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle) 
// Clears boards for start of data taking 
{
    int i;
    PlxReturnCode_t  rc ;

    if(verbose) printf("*** Clearing devices ***\n");

// Prepare SURF boards
    for(i=0;i<numSurfs;++i){
	/* init. 9030 I/O descripter (to enable READY# input |= 0x02.) */
	if(PlxRegisterWrite(surfHandles[i], PCI9030_DESC_SPACE0, 0x00800000) 
	   != ApiSuccess)  printf("  failed to set Sp0 descriptor on SURF %d.\n",i ) ;    
	/*  Get PLX Chip Type */
	/*   PlxChipTypeGet(surfHandles[i], &ChipTypeSelected, &ChipRevision ); */
	/*    if (ChipTypeSelected == 0x0) { */
	/*      printf("\n   ERROR: Unable to determine PLX chip type SURF %d\n",i); */
	/*      PlxPciDeviceClose(surfHandles[i] ); */
	/*      exit(-1); */
	/*    } */
	/*    printf(" SURF %d Chip type:  %04x,  Revision :    %02X\n", i,*/
	/*          ChipTypeSelected, ChipRevision );  */
    }
  
    for(i=0;i<numSurfs;++i){
	printf(" GPIO register contents SURF %d = %x\n",i,
	       PlxRegisterRead(surfHandles[i], PCI9030_GP_IO_CTRL, &rc)) ; 
	if (setSurfControl(surfHandles[i], ClrAll) != ApiSuccess)
	    printf("  failed to send clear all event pulse on SURF %d.\n",i) ;
    }

    if(verbose){
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

    printf(" GPIO register contents TURFIO = %x\n",
	   PlxRegisterRead(turfioHandle, PCI9030_GP_IO_CTRL, &rc)) ; 

    //printf(" Try to write 9030 GPIO.\n") ;
    if (setTurfControl(turfioHandle, ClrTrg) != ApiSuccess)
	printf("  failed to send clear pulse on TURF %d.\n",i) ;

    /*    printf(" Try to read 9030 GPIO.  %x\n", */
    /*  	 PlxRegisterRead(turfioHandle, PCI9030_GP_IO_CTRL, &rc)) ; */
    /*    if (rc != ApiSuccess) printf("  failed to read TURFIO GPIO .\n") ; */

    if(verbose){
	printf(" Try to test interrupt control register.\n") ;
	printf(" int reg contents TURFIO = %x\n",
	       PlxRegisterRead(turfioHandle, PCI9030_INT_CTRL_STAT, &rc)) ; 
    }

    return;
}



void writeData(char *directory, unsigned short *wv_data,unsigned long evNum) {

    char f_name[128] ;
    FILE *wv_file ;
    int  n ;

    sprintf(f_name, "%s/surf_data.%lu", directory, evNum) ;
    if(verbose) printf(" writeData: save data to %s\n", f_name) ;

    if ((wv_file=fopen(f_name, "w")) == NULL) {
	printf(" writeData: failed to open wv file, %s\n", f_name) ;
	return ; 
    }

    if ((n=fwrite(wv_data, sizeof(unsigned short), N_SCA*N_CHN*numSurfs, wv_file))
	!= N_SCA*N_CHN*numSurfs)
	printf(" writeData: failed to write all data. wrote only %d.\n", n) ;

    fclose (wv_file) ;
}

void writeTurf(char *directory, TurfioStruct *data_turf,unsigned long evNum) {

    char f_name[128] ;
    FILE *wv_file ;
    int  n ;

    sprintf(f_name, "%s/turf_data.%lu", directory, evNum) ;
    if(verbose) printf(" writeTurf: save data to %s\n", f_name) ;

    if ((wv_file=fopen(f_name, "w")) == NULL) {
	printf(" writeData: failed to open wv file, %s\n", f_name) ;
	return ; 
    }

    if ((n=fwrite(data_turf, sizeof(TurfioStruct), 1, wv_file))!= 1)
	printf(" writeData: failed to write turf data.\n") ;

    fclose (wv_file) ;
}


/* purse command arguments to initialize parameter(s). */
int init_param(int argn, char **argv, char **directory, int *n, int *dacVal) {

    *directory = NULL ;   /* just make sure it points NULL. */

    while (--argn) {
	if (**(++argv) == '-') {
	    switch (*(*argv+1)) {
		case 'v': verbose++ ; break ;
		case 'p': printout = TRUE ; break ;
		case 't': selftrig = TRUE; break;
		case 's': surfonly = TRUE; break;
		case 'w': writedata = TRUE; break;
		case 'c': doDacCycle = TRUE; break;
		case 'a': *dacVal=atoi(*(++argv)) ; --argn ; break ;
		case 'r': rstturf = TRUE ; break ;
		case 'd': *directory = *(++argv);
		    --argn ; break ;
		case 'n': sscanf(*(++argv), "%d", n) ;
		    --argn ; break ;
		default : printf(" init_param: undefined parameter %s\n", *argv) ;
	    }
	}
    }

    if (writedata) printf(" data will be saved into disk.\n") ;  
    return 0;
}

void setDACThresholds(PlxHandle_t *surfHandles,  int threshold) {
    //Only does one threshold at the moment
    
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
    /* 	if (setSurfControl(surfHandles[i], ClrEvt) != ApiSuccess) */
    /* 	    printf("  failed to send clear event pulse on SURF %d.\n",i); */
    /* 	if (setSurfControl(surfHandles[i], ClrAll) != ApiSuccess) */
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
	    //		printf("DAC_C[%d][%d] = %d\n",chan,bit,DAC_C[chan][bit]);
	}
    }

    for(boardIndex=0;boardIndex<numSurfs;boardIndex++) {	    
	for (chan=0; chan<DAC_CHAN; chan++) {
	    if(verbose>1)      printf("  setDAC: Writing: ");
	    for (bit=BIT_NO-1; bit>=0; bit--) {
		//		TESTAPP_WriteWord(hTESTAPP, 2, 0, DAC_C[chan][bit]);
		dataByte=DAC_C[chan][bit];
		if (PlxBusIopWrite(surfHandles[boardIndex], IopSpace0, 0x0, TRUE, &dataByte, 2, BitSize16) != ApiSuccess) {
		    printf("Failed to write DAC value\n");
		}
		if(verbose>1)printf("%x ",dataByte);
		//else printf("Wrote %d to SURF %d; value: %d\n",dataByte,boardIndex,value);		    
	    }
	    if(verbose>1)printf("\n");
	    tmpGPIO=PlxRegisterRead(surfHandles[boardIndex], PCI9030_GP_IO_CTRL, &rc);
	    tmpGPIO=PlxRegisterRead(surfHandles[boardIndex], PCI9030_GP_IO_CTRL, &rc);
	    //	    dwVal = TESTAPP_ReadDword(hTESTAPP, 0, dwOffset);
	    //	    dwVal = TESTAPP_ReadDword(hTESTAPP, 0, dwOffset);
	}
    }
	
}

