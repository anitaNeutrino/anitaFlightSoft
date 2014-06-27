/*! \file Calibd.c
    \brief First version of the Calibd program for ANITA-3
    
    Calibd is the daemon that controls the digital acromag and so it's role is 
    to toggle relays.
    June 2014  r.nichol@ucl.ac.uk
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
#include <libgen.h> //For Mac OS X

// Flight Software includes
#include "Calibd.h"
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "includes/anitaStructures.h"

/* Vendor includes */
#include "cr7.h"
#include "carrier/apc8620.h"
#include "ip470/ip470.h"

// Acromag constants 
#define INTERRUPT_LEVEL 10

// Global variables for acromag control
int carrierHandle;
struct cblk470 cblk_470;

// Acromag constants 
#define INTERRUPT_LEVEL 10

void prepWriterStructs();
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
int setLevel(int port, int channel, int level);

// Global (config) variables
int digitalCarrierNum=1;


// Relay states
int stateAmplite1=0;
int stateAmplite2=0;
int stateBZAmpa1=0;
int stateBZAmpa2=0;
int stateNTUAmpa=0;
int stateSB=0;
int stateNTUSSD5V=0;
int stateNTUSSD12V=0;
int stateNTUSSDShutdown=0;


//Debug
int printToScreen=1;


int writePeriod=60;

int hkDiskBitMask;
int disableSatablade=0;
int disableSatamini=0;
int disableUsb=0;
int disableNeobrick=0;
AnitaHkWriterStruct_t calibWriter;

int main (int argc, char *argv[])
{
    int retVal=0;
    int relaysChanged=0;
    int firstTime=1;
    int writeSec=0;
       
    // Config file thingies 
    int status=0;
    char* eString;
    char *globalConfFile="anitaSoft.config";
    // Log stuff 
    char *progName=basename(argv[0]);
    
    // Setup log 
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    // Set signal handlers 
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, handleBadSigs);
    signal(SIGINT, handleBadSigs);
    signal(SIGSEGV, handleBadSigs);
    
    //Sort out PID File
    retVal=sortOutPidFile(progName);
    if(retVal!=0) {
      return retVal;
    }


    // Load Config 
    kvpReset () ;
    status = configLoad (globalConfFile,"global") ;


    // Get Calibd output dirs
    if (status == CONFIG_E_OK) {
	hkDiskBitMask=kvpGetInt("hkDiskBitMask",1);

	disableUsb=kvpGetInt("disableUsb",1);
	if(disableUsb)
	    hkDiskBitMask&=~USB_DISK_MASK;
	disableNeobrick=kvpGetInt("disableNeobrick",1);
	if(disableNeobrick)
	    hkDiskBitMask&=~NEOBRICK_DISK_MASK;
	disableSatamini=kvpGetInt("disableSatamini",1);
	if(disableSatamini)
	    hkDiskBitMask&=~SATAMINI_DISK_MASK;
	disableSatablade=kvpGetInt("disableSatablade",1);
	if(disableSatablade)
	    hkDiskBitMask&=~SATABLADE_DISK_MASK;
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading %s: %s\n",globalConfFile,eString);
    }

    makeDirectories(CALIBD_STATUS_LINK_DIR);

    //Need to set digital carrier num
    retVal=readConfigFile();
    //Setup acromag
    //    acromagSetup();
    //    ip470Setup();

    do {
	if(printToScreen) printf("Initializing Calibd\n");
	retVal=readConfigFile();
	relaysChanged=1;
	currentState=PROG_STATE_RUN;
	writeSec=0;
	while(currentState==PROG_STATE_RUN) {	  
	    if(printToScreen) {
	      printf("Amplite1 (%d)  Amplite2 (%d)  BZAmpa1 (%d)  BZAmpa2 (%d)  NTUAmpa (%d)\tSB (%d) SSD_5V (%d) SSD_12V (%d) SSD_Shutdown (%d)\n",stateAmplite1,stateAmplite2,stateBZAmpa1,stateBZAmpa2,stateNTUAmpa,stateSB,stateNTUSSD5V,stateNTUSSD12V,stateNTUSSDShutdown);
	    }
	    //Set Relays to correct state
	    //	    if(relaysChanged || firstTime) retVal=setRelays();

	    if(relaysChanged || writeSec>=writePeriod) {
		writeStatus();
		relaysChanged=0;
		writeSec=0;
	    }
	    

	    if(firstTime) firstTime=0;

	    sleep(1);
	    writeSec++;	
	}
    } while(currentState==PROG_STATE_INIT);
    closeHkFilesAndTidy(&calibWriter);
    unlink(CALIBD_PID_FILE);
    syslog(LOG_INFO,"Calibd Terminating");
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

    if(stateAmplite1) 
	theStatus.status|=AMPLITE1_MASK;
    if(stateAmplite2) 
	theStatus.status|=AMPLITE2_MASK;
    if(stateBZAmpa1) 
	theStatus.status|=BZAMPA1_MASK;
    if(stateBZAmpa2) 
	theStatus.status|=BZAMPA2_MASK;
    if(stateNTUAmpa) 
	theStatus.status|=NTUAMPA_MASK;
    if(stateSB) 
      theStatus.status|=SB_MASK;
    if(stateNTUSSD5V) 
      theStatus.status|=NTU_SSD_5V_MASK;
    if(stateNTUSSD12V) 
      theStatus.status|=NTU_SSD_12V_MASK;
    if(stateNTUSSDShutdown) 
      theStatus.status|=NTU_SSD_SHUTDOWN_MASK;
    
    
    sprintf(filename,"%s/calib_%u.dat",CALIBD_STATUS_DIR,theStatus.unixTime);
    writeStruct(&theStatus,filename,sizeof(CalibStruct_t));
    makeLink(filename,CALIBD_STATUS_LINK_DIR);

    cleverHkWrite((unsigned char*)&theStatus,sizeof(CalibStruct_t),
		  theStatus.unixTime,&calibWriter);
    
}


int readConfigFile() 
/* Load Calibd and Relay config stuff */
{
    /* Config file thingies */
    int status=0;
    //    int tempNum=12;
//    int tempNum=3,count=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;
    kvpReset();
    status = configLoad ("Calibd.config","output") ;
    status = configLoad ("Calibd.config","calibd") ;
    status |= configLoad ("Calibd.config","relays") ;

    if(status == CONFIG_E_OK) {
	//Which board is the digital acromag?
	digitalCarrierNum=kvpGetInt("digitalCarrierNum",1);
      	printf("digitalCarrierNum %d\n",digitalCarrierNum);
	//Debug
	printToScreen=kvpGetInt("printToScreen",-1);
	
	//Heartbeat
	writePeriod=kvpGetInt("writePeriod",60);

	//Relay States
	stateAmplite1=kvpGetInt("stateAmplite1",0);
	stateAmplite2=kvpGetInt("stateAmplite2",0);
	stateBZAmpa1=kvpGetInt("stateBZAmpa1",0);
	stateBZAmpa2=kvpGetInt("stateBZAmpa2",0);
	stateNTUAmpa=kvpGetInt("stateNTUAmpa",0);
	stateSB=kvpGetInt("stateSB",0);

	stateNTUSSD5V=kvpGetInt("stateNTUSSD5V",0);
	stateNTUSSD12V=kvpGetInt("stateNTUSSD12V",0);
	stateNTUSSDShutdown=kvpGetInt("stateNTUSSDShutdown",0);

    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Calibd.config: %s\n",eString);
    }

   


    return status;
   
}


void acromagSetup()
{
  long addr=0;
  
  /* Check for Carrier Library */
  if(InitCarrierLib() != S_OK) {
    printf("\nCarrier library failure");
    handleBadSigs(1000);
  }
  
  /* Connect to Carrier */
  if(CarrierOpen(digitalCarrierNum, &carrierHandle) != S_OK) {
    printf("\nUnable to Open instance of carrier.\n");
    handleBadSigs(1001);
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
      handleBadSigs(1002);
    }

  cblk_470.bCarrier=TRUE;

  /* GetCarrierAddress(carrierHandle, &addr);
     SetCarrierAddress(carrierHandle, addr); */

  if(GetIpackAddress(carrierHandle,cblk_470.slotLetter,
		     (long *) &cblk_470.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP470\n.");
      handleBadSigs(10003);
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
      handleBadSigs(1004);
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
  //  for(i = 0; i < 6; i++)
  //    cblk_470.event_status[i] = 0; /* initialize event status flags */
}


int setRelays() 
// Sets the relays to the state specified in Calibd.config
{
    int retVal=0;
    if(stateAmplite1)
	retVal+=toggleRelay(AMPLITE1_ON_LOGIC/8,AMPLITE1_ON_LOGIC%8);
    else
	retVal+=toggleRelay(AMPLITE1_OFF_LOGIC/8,AMPLITE1_OFF_LOGIC%8);

    if(stateAmplite2)
	retVal+=toggleRelay(AMPLITE2_ON_LOGIC/8,AMPLITE2_ON_LOGIC%8);
    else
	retVal+=toggleRelay(AMPLITE2_OFF_LOGIC/8,AMPLITE2_OFF_LOGIC%8);

    if(stateBZAmpa1)
	retVal+=toggleRelay(BZAMPA1_ON_LOGIC/8,BZAMPA1_ON_LOGIC%8);
    else
	retVal+=toggleRelay(BZAMPA1_OFF_LOGIC/8,BZAMPA1_OFF_LOGIC%8);

    if(stateBZAmpa2)
	retVal+=toggleRelay(BZAMPA2_ON_LOGIC/8,BZAMPA2_ON_LOGIC%8);
    else
	retVal+=toggleRelay(BZAMPA2_OFF_LOGIC/8,BZAMPA2_OFF_LOGIC%8);
   
    if(stateNTUAmpa)
	retVal+=toggleRelay(NTUAMPA_ON_LOGIC/8,NTUAMPA_ON_LOGIC%8);
    else
	retVal+=toggleRelay(NTUAMPA_OFF_LOGIC/8,NTUAMPA_OFF_LOGIC%8);
   

    //These ones operate on levels
    retVal=setLevel(SB_LOGIC/8,SB_LOGIC%8,stateSB);    
    retVal=setLevel(NTU_SSD_5V_LOGIC/8,NTU_SSD_5V_LOGIC%8,stateNTUSSD5V);
    retVal=setLevel(NTU_SSD_12V_LOGIC/8,NTU_SSD_12V_LOGIC%8,stateNTUSSD12V);
    retVal=setLevel(NTU_SSD_SHUTDOWN_LOGIC/8,NTU_SSD_SHUTDOWN_LOGIC%8,stateNTUSSDShutdown);

    return retVal;

}

int setLevel(int port, int channel, int level)
//Just sets one level
{
  return wpnt470(&cblk_470,port,channel,level);
}


int toggleRelay(int port, int chan) 
// Sends off, on, off to specified port to toggle relay
{
    int retVal=wpnt470(&cblk_470,port,chan,0);
//    usleep(1);
    retVal+=wpnt470(&cblk_470,port,chan,1);
//    usleep(1);
    retVal+=wpnt470(&cblk_470,port,chan,0);
    return retVal;
}

int setMultipleLevels(int basePort, int baseChan, int nbits, int value) {
    unsigned int current;
    unsigned int toWrite;
    unsigned int i;
    unsigned int mask=0;
    for (i=0;i<nbits;i++)
	mask |= 1<<i;
    current=rprt470(&cblk_470, basePort);
    toWrite = (current & ~(mask<<baseChan)) | ((value&mask)<<baseChan);
    if(printToScreen) {
	printf("Base Port %d -- Current %d\t New %d\n",
	       basePort,current,toWrite);
    }
    return wprt470(&cblk_470, basePort, toWrite);
}

void prepWriterStructs() {
    int diskInd;
    if(printToScreen) 
	printf("Preparing Writer Structs\n");
    //Hk Writer

    sprintf(calibWriter.relBaseName,"%s/",CALIB_ARCHIVE_DIR);
    sprintf(calibWriter.filePrefix,"calib");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	calibWriter.currentFilePtr[diskInd]=0;
    calibWriter.writeBitMask=hkDiskBitMask;




}



void handleBadSigs(int sig)
{
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
    closeHkFilesAndTidy(&calibWriter);
    unlink(CALIBD_PID_FILE);
    syslog(LOG_INFO,"Calibd terminating");
    exit(0);
}


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(CALIBD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,CALIBD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(CALIBD_PID_FILE);
  return 0;
}
