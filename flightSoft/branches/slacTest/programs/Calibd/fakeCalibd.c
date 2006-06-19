/*! \file fakeCalibd.c
    \brief Fake version of the Calibd program 
    
    Calibd is the daemon that controls the digital acromag and so it's role is 
    to toggle relays, set sunsensor gains and switch between the ports on the 
    RF switch. This version is the same as Calibd except it doesn't do anything
    to the Acromag.
    April 2006  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE

// System includes
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

// Flight Software includes
#include "Calibd.h"
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"

/* Vendor includes */
#include "cr7.h"
#include "carrier/apc8620.h"
#include "ip470/ip470.h"

// Acromag constants 
#define INTERRUPT_LEVEL 10


// Global variables for acromag control
int carrierHandle;
struct conf_blk_470 cblk_470;

// Acromag constants 
#define INTERRUPT_LEVEL 10


// Global (config) variables
// Relay states
int stateRFCM1=0;
int stateRFCM2=0;
int stateRFCM3=0;
int stateRFCM4=0;
int stateVeto=0;
int stateGPS=0;
int stateCalPulser=0;
// SS Gain
int ss1Gain=5;
// RF Switch
int steadyState=0;
int switchPeriod=60;
int offPeriod=60;
int switchState=1;

//Debug
int printToScreen=1;

//Output Dirs
char calibdStatusDir[FILENAME_MAX];
char calibdLinkDir[FILENAME_MAX];
char calibArchiveDir[FILENAME_MAX];
char calibArchiveLinkDir[FILENAME_MAX];


int main (int argc, char *argv[])
{
    int secs=0,retVal=0;
    int relaysChanged=0;
    // Config file thingies 
    int status=0;
    char* eString;
    char *tempString=0;
    char *globalConfFile="anitaSoft.config";
    // Log stuff 
    char *progName=basename(argv[0]);
    
    // Setup log 
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    // Set signal handlers 
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    
    // Load Config 
    kvpReset () ;
    status = configLoad (globalConfFile,"global") ;
    eString = configErrorString (status) ;

    // Get Calibd output dirs
    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("calibdStatusDir");
	if(tempString) {
	    strncpy(calibdStatusDir,tempString,FILENAME_MAX-1);
	    strncpy(calibdLinkDir,calibdStatusDir,FILENAME_MAX-1);
	    strcat(calibdLinkDir,"/link");
	    makeDirectories(calibdLinkDir);
	}
	else {	    
	    syslog(LOG_ERR,"Error getting calibdStatusDir");
	    fprintf(stderr,"Error getting calibdStatusDir\n");
	}
	
	tempString=kvpGetString("mainDataDisk");
	if(tempString) {
	    strncpy(calibArchiveDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error getting mainDataDisk");
	    fprintf(stderr,"Error getting mainDataDisk\n");
	}	    
	tempString=kvpGetString("baseHouseArchiveDir");
	if(tempString) {
	    sprintf(calibArchiveDir,"%s/%s/",calibArchiveDir,tempString);
	}
	else {
	    syslog(LOG_ERR,"Error getting baseHouseArchiveDir");
	    fprintf(stderr,"Error getting baseHouseArchiveDir\n");
	}	    
	tempString=kvpGetString("calibArchiveSubDir");
	if(tempString) {
	    strcat(calibArchiveDir,tempString);
	    makeDirectories(calibArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting calibArchiveSubDir");
	    fprintf(stderr,"Error getting calibArchiveSubDir\n");
	}
    }

    //Setup acromag
//    acromagSetup();
//    ip470Setup();

    do {
	if(printToScreen) printf("Initializing Calibd\n");
	retVal=readConfigFile();
	relaysChanged=1;
	currentState=PROG_STATE_RUN;
	secs=0;
	while(currentState==PROG_STATE_RUN) {	  
	    if(printToScreen)
		printf("Secs %d\tswitchState %d\tsteadyState %d\tstateCalPulse %d\trelaysChanged %d\n",secs,switchState,steadyState,stateCalPulser,relaysChanged);
	    if(secs==0) {		
		//Set Relays to correct state
		if(relaysChanged) retVal=setRelaysAndSSGain();
		setSwitchState();
		writeStatus();
		relaysChanged=0;
	    }
	    else if(stateCalPulser && secs==switchPeriod) {
		if(steadyState==0) {
		    switchState++;
		    if(switchState>4) {
			switchState=1;
			stateCalPulser=0;
			relaysChanged=1;
		    }
		}
		else {
		    stateCalPulser=0;
		    relaysChanged=1;
		}
		secs=-1;
	    }
	    else if(!stateCalPulser && secs==offPeriod && offPeriod) {
		stateCalPulser=1;
		relaysChanged=1;
		secs=-1;
	    }
	    sleep(1);
	    secs++;		
	}
    } while(currentState==PROG_STATE_INIT);
    return 0;
    
}

void writeStatus()
/*
  Write the current calibration state
*/
{
    // int retVal;
    char filename[FILENAME_MAX];
    CalibStruct_t theStatus;
    time_t unixTime;
    time(&unixTime);
    theStatus.unixTime=unixTime;
    theStatus.status=0;

    if(stateRFCM1) 
	theStatus.status|=RFCM1_MASK;
    if(stateRFCM2) 
	theStatus.status|=RFCM2_MASK;
    if(stateRFCM3) 
	theStatus.status|=RFCM3_MASK;
    if(stateRFCM4) 
	theStatus.status|=RFCM4_MASK;
    if(stateVeto) 
	theStatus.status|=VETO_MASK;
    if(stateGPS) 
	theStatus.status|=GPS_MASK;
    if(stateCalPulser) 
	theStatus.status|=CAL_PULSER_MASK;
    
    //Might change the below so that it only needs 2 bits
    theStatus.status |= (((switchState)&0xf)<<RFSWITCH_SHIFT); 
    
    //Might change the below when we have any idea what it is doing
    theStatus.status |= (((ss1Gain)&0xf)<<SS1_SHIFT); 
    
    sprintf(filename,"%s/calib_%ld.dat",calibdStatusDir,theStatus.unixTime);
    writeCalibStatus(&theStatus,filename);
    makeLink(filename,calibdLinkDir);
    sprintf(filename,"%s/calib_%ld.dat",calibArchiveDir,theStatus.unixTime);
    writeCalibStatus(&theStatus,filename);
    makeLink(filename,calibArchiveLinkDir);
    
}


int readConfigFile() 
/* Load Calibd and Relay config stuff */
{
    /* Config file thingies */
    int status=0;
//    int tempNum=3,count=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;
    kvpReset();
    status = configLoad ("Calibd.config","output") ;
    status |= configLoad ("Calibd.config","relays") ;
    status |= configLoad ("Calibd.config","sunsensor") ;
    status |= configLoad ("Calibd.config","rfSwitch") ;

    if(status == CONFIG_E_OK) {       
	//Debug
	printToScreen=kvpGetInt("printToScreen",-1);
	
	//Relay States
	stateRFCM1=kvpGetInt("stateRFCM1",0);
	stateRFCM2=kvpGetInt("stateRFCM2",0);
	stateRFCM3=kvpGetInt("stateRFCM3",0);
	stateRFCM4=kvpGetInt("stateRFCM4",0);
	stateVeto=kvpGetInt("stateVeto",0);
	stateGPS=kvpGetInt("stateGPS",0);
	stateCalPulser=kvpGetInt("stateCalPulser",0);
	
	//Sunsensor Gain
	ss1Gain=kvpGetInt("ss1Gain",0);
	
	//RF Switch
	steadyState=kvpGetInt("steadyState",0);
	switchPeriod=kvpGetInt("switchPeriod",60);
	offPeriod=kvpGetInt("offPeriod",60);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Calib.config: %s\n",eString);
    }
    return status;
   
}


void acromagSetup()
{
  long addr=0;
  
  /* Check for Carrier Library */
  if(InitCarrierLib() != S_OK) {
    printf("\nCarrier library failure");
    exit(1);
  }
  
  /* Connect to Carrier */
  if(CarrierOpenDev(0, &carrierHandle, IP470_CARRIER) != S_OK) {
    printf("\nUnable to Open instance of carrier.\n");
    exit(2);
  }

  cblk_470.nHandle = carrierHandle;
  cblk_470.slotLetter=IP470_SLOT;
  if(printToScreen)
      printf("Inititalizing IP470 in slot %c\n",cblk_470.slotLetter); 

  if(CarrierInitialize(carrierHandle) == S_OK) 
    { /* Initialize Carrier */
      SetInterruptLevel(carrierHandle, INTERRUPT_LEVEL);
      /* Set carrier interrupt level */
    } 
  else 
    {
      printf("\nUnable initialize the carrier %lX", addr);
      exit(3);
    }

  cblk_470.bCarrier=TRUE;

  /* GetCarrierAddress(carrierHandle, &addr);
     SetCarrierAddress(carrierHandle, addr); */

  if(GetIpackAddress(carrierHandle,cblk_470.slotLetter,
		     (long *) &cblk_470.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP470\n.");
      exit(6);
    }
  cblk_470.bInitialized = TRUE;
}


void ip470Setup()
{

  int i;
  /* channel assignments:
 ??
  */

  rmid470(&cblk_470);
  if((byte)cblk_470.id_prom[5]!=0x8) {
      printf("Board ID Wrong\n\n");
      printf("Board ID Information\n");
      printf("\nIdentification:              ");
      for(i = 0; i < 4; i++)          /* identification */
	  printf("%c",cblk_470.id_prom[i]);
      printf("\nManufacturer's ID:           %X",(byte)cblk_470.id_prom[4]);
      printf("\nIP Model Number:             %X",(byte)cblk_470.id_prom[5]);
      printf("\nRevision:                    %X",(byte)cblk_470.id_prom[6]);
      printf("\nReserved:                    %X",(byte)cblk_470.id_prom[7]);
      printf("\nDriver I.D. (low):           %X",(byte)cblk_470.id_prom[8]);
      printf("\nDriver I.D. (high):          %X",(byte)cblk_470.id_prom[9]);
      printf("\nTotal I.D. Bytes:            %X",(byte)cblk_470.id_prom[10]);
      printf("\nCRC:                         %X\n",(byte)cblk_470.id_prom[11]);
      exit(0);
  }
  else {
      printf("Board ID correct %d\n",(byte)cblk_470.id_prom[5]);
  }

  cblk_470.param=0;
  cblk_470.e_mode=0;
  cblk_470.mask_reg=0;
  cblk_470.deb_control=0;
  cblk_470.deb_clock=1;
  cblk_470.enable=0;
  cblk_470.vector=0;
  for(i = 0; i != 2; i++) /* Curious why != as oppposed to < */
    {
      cblk_470.ev_control[i] = 0;/* event control registers */
      cblk_470.deb_duration[i] = 0;/* debounce duration registers */
    }
  for(i = 0; i < 6; i++)
    cblk_470.event_status[i] = 0; /* initialize event status flags */
}


int setRelaysAndSSGain() 
// Sets the relays to the state specified in Calibd.config
{
    int retVal=0;
    if(stateRFCM1)
	retVal+=toggleRelay(RFCM1_ON_LOGIC/8,RFCM1_ON_LOGIC%8);
    else
	retVal+=toggleRelay(RFCM1_OFF_LOGIC/8,RFCM1_OFF_LOGIC%8);

    if(stateRFCM2)
	retVal+=toggleRelay(RFCM2_ON_LOGIC/8,RFCM2_ON_LOGIC%8);
    else
	retVal+=toggleRelay(RFCM2_OFF_LOGIC/8,RFCM2_OFF_LOGIC%8);

    if(stateRFCM3)
	retVal+=toggleRelay(RFCM3_ON_LOGIC/8,RFCM3_ON_LOGIC%8);
    else
	retVal+=toggleRelay(RFCM3_OFF_LOGIC/8,RFCM3_OFF_LOGIC%8);

    if(stateRFCM4)
	retVal+=toggleRelay(RFCM4_ON_LOGIC/8,RFCM4_ON_LOGIC%8);
    else
	retVal+=toggleRelay(RFCM4_OFF_LOGIC/8,RFCM4_OFF_LOGIC%8);
   
    if(stateVeto)
	retVal+=toggleRelay(VETO_ON_LOGIC/8,VETO_ON_LOGIC%8);
    else
	retVal+=toggleRelay(VETO_OFF_LOGIC/8,VETO_OFF_LOGIC%8);
   
    if(stateGPS)
	retVal+=toggleRelay(GPS_ON_LOGIC/8,GPS_ON_LOGIC%8);
    else
	retVal+=toggleRelay(GPS_OFF_LOGIC/8,GPS_OFF_LOGIC%8);
   
    if(stateCalPulser)
	retVal+=toggleRelay(CAL_PULSER_ON_LOGIC/8,CAL_PULSER_ON_LOGIC%8);
    else
	retVal+=toggleRelay(CAL_PULSER_OFF_LOGIC/8,CAL_PULSER_OFF_LOGIC%8);

    //Sun Sensor
    retVal+=setMultipleLevels(SUNSENSOR1_GAIN_LSB/8,SUNSENSOR1_GAIN_LSB%8,3,ss1Gain);
    return retVal;

}


int toggleRelay(int port, int chan) 
// Sends off, on, off to specified port to toggle relay
{
    int retVal=0;
//    wpnt470(&cblk_470,port,chan,0);
//    usleep(1);
//    retVal+=wpnt470(&cblk_470,port,chan,1);
//    usleep(1);
//    retVal+=wpnt470(&cblk_470,port,chan,0);
    return retVal;
}

int setMultipleLevels(int basePort, int baseChan, int nbits, int value) {
    if(printToScreen) 
	printf("basePort %d, baseChan %d, nbits %d, value %d\n",
	       basePort,baseChan,nbits,value);
    unsigned int current;
    unsigned int toWrite;
    unsigned int i;
    unsigned int mask=0;
    for (i=0;i<nbits;i++)
	mask |= 1<<i;
    current=0;
//    current=rprt470(&cblk_470, basePort);
    toWrite = (current & ~(mask<<baseChan)) | ((value&mask)<<baseChan);
    if(printToScreen) {
	printf("Base Port %d -- Current %d\t New %d\n",
	       basePort,current,toWrite);
    }
    return 0;
    //wprt470(&cblk_470, basePort, toWrite);
}

int setSwitchState() 
//Sets RF Switch state
{
    return setMultipleLevels(RFSWITCH_LSB/8,RFSWITCH_LSB%8,4,switchState);

}
