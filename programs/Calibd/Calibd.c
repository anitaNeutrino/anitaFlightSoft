/*! \file Calibd.c
    \brief First version of the Calibd program for ANITA-3
    
    Calibd is the daemon that controls the digital acromag and so it's role is 
    to toggle relays.
    June 2014  r.nichol@ucl.ac.uk
*/
#define _GNU_SOURCE

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <libgen.h> //For Mac OS X

// Flight Software includes
#include "Calibd.h"
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "includes/anitaStructures.h"

/* Vendor includes */
#include "carrier/apc8620.h"
#include "ip470/ip470.h"
#include "ip320/ip320.h"


// Acromag constants 
#define INTERRUPT_LEVEL 10

// Global variables for acromag control
int carrierHandle;
struct cblk470 config470;
struct cblk320 config320;


/* tables for ip320 data and calibration constants */
/* note sa_size is sort of arbitrary; 
   single channel reads only use first one */
struct scan_array scanArray[CHANS_PER_IP320]; 

SingleAnalogueStruct_t autoZeroStruct;
SingleAnalogueStruct_t rawDataStruct;
SingleAnalogueStruct_t calDataStruct;
AnalogueCorrectedDataStruct_t corDataStruct;


// Acromag constants 
#define INTERRUPT_LEVEL 10

void prepWriterStructs();
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
int setLevel(int port, int channel, int level);
int outputData(AnalogueCode_t code) ;

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
int stateNTUSSDShutdown=1;


//Debug
int printToScreen=1;

int ip320Range=RANGE_10TO10;  //RJN Guess

int adcAverage=1;
int readoutPeriod; //in ms
int telemEvery;
int calibrationPeriod;

int relayWritePeriod=60;

int hkDiskBitMask;
int disableHelium1=0;
int disableHelium2=0;
int disableUsb=0;
//int disableNeobrick=0;
AnitaHkWriterStruct_t calibWriter;
AnitaHkWriterStruct_t hkRawWriter;
AnitaHkWriterStruct_t hkCalWriter;

int main (int argc, char *argv[])
{
    int retVal=0;
    int relaysChanged=0;
    int firstTime=1;
    int lastRelayWriteSec=0;
       
    // Config file thingies 
    int status=0;
    char* eString;
    char *globalConfFile="anitaSoft.config";
    // Log stuff 
    char *progName=basename(argv[0]);
    

    int millisecs=0;
    struct timespec milliWait;
    milliWait.tv_sec=0;


    int lastCal=0;
    time_t rawTime;

    // Setup log 
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    // Set signal handlers 
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, handleBadSigs);
    signal(SIGINT, handleBadSigs);
    signal(SIGSEGV, handleBadSigs);

    //Dont' wait for children
    signal(SIGCLD, SIG_IGN);     

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
	//	disableNeobrick=kvpGetInt("disableNeobrick",1);
	//	if(disableNeobrick)
	//	    hkDiskBitMask&=~NEOBRICK_DISK_MASK;
	disableHelium2=kvpGetInt("disableHelium2",1);
	if(disableHelium2)
	    hkDiskBitMask&=~HELIUM2_DISK_MASK;
	disableHelium1=kvpGetInt("disableHelium1",1);
	if(disableHelium1)
	    hkDiskBitMask&=~HELIUM1_DISK_MASK;
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading %s: %s\n",globalConfFile,eString);
    }

    makeDirectories(CALIBD_STATUS_LINK_DIR);
    makeDirectories(HK_TELEM_DIR);
    makeDirectories(HK_TELEM_LINK_DIR);

    prepWriterStructs();

    //Need to set digital carrier num
    retVal=readConfigFile();
    //Setup acromag
    acromagSetup();
    ip470Setup();

    //Set code values
    autoZeroStruct.code=IP320_AVZ;
    rawDataStruct.code=IP320_RAW;
    calDataStruct.code=IP320_CAL;



    do {
	if(printToScreen) printf("Initializing Calibd\n");
	retVal=readConfigFile();
	relaysChanged=1;
	currentState=PROG_STATE_RUN;
//RJN for Hkd
        ip320Setup();
	millisecs=0;
	lastRelayWriteSec=0;
	while(currentState==PROG_STATE_RUN) {	  
	  time(&rawTime);
	  
	  //Set Relays to correct state
	  if(relaysChanged || firstTime) retVal=setRelays();
	  
	  if(relaysChanged || (rawTime-lastRelayWriteSec)>=relayWritePeriod) {

	    if(printToScreen) {
	      printf("Amplite1 (%d)  Amplite2 (%d)  BZAmpa1 (%d)  BZAmpa2 (%d)  NTUAmpa (%d)\tSB (%d) SSD_5V (%d) SSD_12V (%d) SSD_Shutdown (%d)\n",stateAmplite1,stateAmplite2,stateBZAmpa1,stateBZAmpa2,stateNTUAmpa,stateSB,stateNTUSSD5V,stateNTUSSD12V,stateNTUSSDShutdown);
	    }
	    writeStatus();
	    relaysChanged=0;
	    lastRelayWriteSec=rawTime;
	  }
	    

	  if(firstTime) firstTime=0;
	  
	  

	  
	  if((millisecs % readoutPeriod)==0) {
	    
	    time(&rawTime);		
	    
	    //		printf("Mag Ret: %d\n",retVal);
	    if((rawTime-lastCal)>calibrationPeriod) {
	      ip320Calibrate();
	      outputData(IP320_CAL);
	      outputData(IP320_AVZ);
	      lastCal=rawTime;
	    }
	    ip320Read(0);
	    outputData(IP320_RAW);
	    
	    millisecs=1;
	  }
	    //Just for safety we'll reset the sleep time
	  milliWait.tv_nsec=1000000;
	  nanosleep(&milliWait,NULL);
	  millisecs++;	   	 

	} //End of run loop
    } while(currentState==PROG_STATE_INIT);
    closeHkFilesAndTidy(&hkRawWriter);
    closeHkFilesAndTidy(&hkCalWriter);
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
	relayWritePeriod=kvpGetInt("relayWritePeriod",60);
	readoutPeriod=kvpGetInt("readoutPeriod",60);
	telemEvery=kvpGetInt("telemEvery",60);
	adcAverage=kvpGetInt("adcAverage",5);
	calibrationPeriod=kvpGetInt("calibrationPeriod",1200);

	//Relay States
	stateAmplite1=kvpGetInt("stateAmplite1",0);
	stateAmplite2=kvpGetInt("stateAmplite2",0);
	stateBZAmpa1=kvpGetInt("stateBZAmpa1",0);
	stateBZAmpa2=kvpGetInt("stateBZAmpa2",0);
	stateNTUAmpa=kvpGetInt("stateNTUAmpa",0);
	stateSB=kvpGetInt("stateSB",0);

	stateNTUSSD5V=kvpGetInt("stateNTUSSD5V",0);
	stateNTUSSD12V=kvpGetInt("stateNTUSSD12V",0);
	//	stateNTUSSDShutdown=kvpGetInt("stateNTUSSDShutdown",0);

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

  config470.nHandle = carrierHandle;
  config470.slotLetter=IP470_SLOT;
  config320.nHandle = carrierHandle;
  config320.slotLetter=IP320_SLOT;



  if(printToScreen) {
      printf("Inititalizing IP470 in slot %c\n",config470.slotLetter); 
      printf("Inititalizing IP320 in slot %c\n",config320.slotLetter); 
  }

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

  config470.bCarrier=TRUE;
  config320.bCarrier=TRUE;

  /* GetCarrierAddress(carrierHandle, &addr);
     SetCarrierAddress(carrierHandle, addr); */

  if(GetIpackAddress(carrierHandle,config470.slotLetter,
		     (long *) &config470.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP470\n.");
      handleBadSigs(10003);
    }
  config470.bInitialized = TRUE;

  if(GetIpackAddress(carrierHandle,config320.slotLetter,
		     (long *) &config320.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP320\n.");
      handleBadSigs(10003);
    }
  config320.bInitialized = TRUE;
}


void ip470Setup()
{

  int i;
  /* channel assignments:
 ??
  */

  rmid470(&config470);
  if((byte)config470.id_prom[5]!=0x8) {
      printf("Board ID Wrong\n\n");
      printf("Board ID Information\n");
      printf("\nIdentification:              ");
      for(i = 0; i < 4; i++)          /* identification */
	  printf("%c",config470.id_prom[i]);
      printf("\nManufacturer's ID:           %X",(byte)config470.id_prom[4]);
      printf("\nIP Model Number:             %X",(byte)config470.id_prom[5]);
      printf("\nRevision:                    %X",(byte)config470.id_prom[6]);
      printf("\nReserved:                    %X",(byte)config470.id_prom[7]);
      printf("\nDriver I.D. (low):           %X",(byte)config470.id_prom[8]);
      printf("\nDriver I.D. (high):          %X",(byte)config470.id_prom[9]);
      printf("\nTotal I.D. Bytes:            %X",(byte)config470.id_prom[10]);
      printf("\nCRC:                         %X\n",(byte)config470.id_prom[11]);
      handleBadSigs(1004);
  }
  else {
      printf("Board ID correct %d\n",(byte)config470.id_prom[5]);
  }

  config470.param=0;
  config470.e_mode=0;
  config470.mask_reg=0;
  config470.deb_control=0;
  config470.deb_clock=1;
  config470.enable=0;
  config470.vector=0;
  for(i = 0; i != 2; i++) /* Curious why != as oppposed to < */
    {
      config470.ev_control[i] = 0;/* event control registers */
      config470.deb_duration[i] = 0;/* debounce duration registers */
    }
  //  for(i = 0; i < 6; i++)
  //    config470.event_status[i] = 0; /* initialize event status flags */
}


int setRelays() 
// Sets the relays to the state specified in Calibd.config
{
    int retVal=0;

    //These ones operate on levels, so will do them first
    retVal=setLevel(NTU_SSD_5V_LOGIC/8,NTU_SSD_5V_LOGIC%8,stateNTUSSD5V);
    retVal=setLevel(NTU_SSD_12V_LOGIC/8,NTU_SSD_12V_LOGIC%8,stateNTUSSD12V);
    retVal=setLevel(SB_LOGIC/8,SB_LOGIC%8,stateSB);    
    retVal=setLevel(NTU_SSD_SHUTDOWN_LOGIC/8,NTU_SSD_SHUTDOWN_LOGIC%8,stateNTUSSDShutdown);


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
   


    return retVal;

}

int setLevel(int port, int channel, int level)
//Just sets one level
{
  return wpnt470(&config470,port,channel,level);
}


int toggleRelay(int port, int chan) 
// Sends off, on, off to specified port to toggle relay
{
    int retVal=wpnt470(&config470,port,chan,0);
//    usleep(1);
    retVal+=wpnt470(&config470,port,chan,1);
//    usleep(1);
    retVal+=wpnt470(&config470,port,chan,0);
    return retVal;
}

int setMultipleLevels(int basePort, int baseChan, int nbits, int value) {
    unsigned int current;
    unsigned int toWrite;
    unsigned int i;
    unsigned int mask=0;
    for (i=0;i<nbits;i++)
	mask |= 1<<i;
    current=rprt470(&config470, basePort);
    toWrite = (current & ~(mask<<baseChan)) | ((value&mask)<<baseChan);
    if(printToScreen) {
	printf("Base Port %d -- Current %d\t New %d\n",
	       basePort,current,toWrite);
    }
    return wprt470(&config470, basePort, toWrite);
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

    //Hk Raw Writer
    sprintf(hkRawWriter.relBaseName,"%s/raw",HK_ARCHIVE_DIR);
    sprintf(hkRawWriter.filePrefix,"sshk_raw");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	hkRawWriter.currentFilePtr[diskInd]=0;
    hkRawWriter.writeBitMask=hkDiskBitMask;

    //Hk Cal Writer
    sprintf(hkCalWriter.relBaseName,"%s/cal",HK_ARCHIVE_DIR);
    sprintf(hkCalWriter.filePrefix,"sshk_cal_avz");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	hkCalWriter.currentFilePtr[diskInd]=0;
    hkCalWriter.writeBitMask=hkDiskBitMask;



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


int ip320Setup()
{
  int i;

  rsts320(&config320);
  if((byte)config320.id_prom[5]!=0x32) {
    if(printToScreen) {
      printf("Board ID Wrong\n\n");
      printf("Board ID Information\n");
      printf("\nIdentification:              ");
      for(i = 0; i < 4; i++)          /* identification */
	printf("%c",config320.id_prom[i]);
      printf("\nManufacturer's ID: %X",(byte)config320.id_prom[4]);
      printf("\nIP Model Number: %X",(byte)config320.id_prom[5]);
      printf("\nRevision: %X",(byte)config320.id_prom[6]);
      printf("\nReserved: %X",(byte)config320.id_prom[7]);
      printf("\nDriver I.D. (low): %X",(byte)config320.id_prom[8]);
      printf("\nDriver I.D. (high): %X",(byte)config320.id_prom[9]);
      printf("\nTotal I.D. Bytes: %X",(byte)config320.id_prom[10]);
      printf("\nCRC: %X\n",(byte)config320.id_prom[11]);
      exit(0);
    }
  }  
  else if(printToScreen)
    printf("Board ID correct %X\n",(byte)config320.id_prom[5]);
  
  for( i = 0; i < CHANS_PER_IP320; i++ ) 
    {
      scanArray[i].gain = GAIN_X1;      /*  gain=2 */
      scanArray[i].chn = i;             /*  channel */
      autoZeroStruct.board.data[i] = 0;                 /* clear auto zero buffer */
      calDataStruct.board.data[i] = 0;                /* clear calibration buffer */
      rawDataStruct.board.data[i] = 0;                /* clear raw input buffer */
      corDataStruct.data[i] = 0;                /* clear corrected data buffer */
    }
  config320.range = ip320Range;
  config320.trigger = STRIG; /* 0 = software triggering */
  config320.mode = SEI;      /* differential input */
  config320.gain = GAIN_X1;  /* gain for analog input */
  config320.average = adcAverage;     /* number of samples to average */
  config320.channel = 0;     /* default channel */
  config320.data_mask = BIT12; /* A/D converter data mask */
  config320.bit_constant = CON12; /* constant for data correction */
  
  config320.s_raw_buf = &rawDataStruct.board.data[0];   /* raw buffer start */
  config320.s_az_buf = &autoZeroStruct.board.data[0]; /* auto zero buffer start */
  config320.s_cal_buf = &calDataStruct.board.data[0]; /* calibration buffer start */
  config320.s_cor_buf = &corDataStruct.data[0]; /* corrected buffer start */
  config320.sa_start = &scanArray[0]; /* address of start of scan array */
  config320.sa_end = &scanArray[CHANS_PER_IP320-1];  /* address of end of scan array*/
  

  return 0;
}


int ip320Read(int fillCor)
{
    ainmc320(&config320);
    if(fillCor) mccd320(&config320);
    return 0;
}


int ip320Calibrate()
{
     byte temp_mode;
 //	printf("Here\n");
     temp_mode=config320.mode;
     config320.mode= AZV; /* auto zero */	
     ainmc320(&config320);
     //	printf("Done auto zero\n");
     config320.mode= CAL; /* calibration */
     ainmc320(&config320);
     //	printf("Done calibration\n");
     config320.mode=temp_mode;
     /* really should only do this every so often, */
     /* and send down the cal constants */
     return 0;
}

int outputData(AnalogueCode_t code) 
{
    int retVal;
    char theFilename[FILENAME_MAX];
    char fullFilename[FILENAME_MAX];
    SSHkDataStruct_t theHkData;
    AnitaHkWriterStruct_t *wrPtr;

    static int telemCount=0;


    struct timeval timeStruct;
    gettimeofday(&timeStruct,NULL);
	    
    theHkData.unixTime=timeStruct.tv_sec;;
    theHkData.unixTimeUs=timeStruct.tv_usec;;
    switch(code) {
	case (IP320_RAW):
	    sprintf(theFilename,"sshk_%d_%d.raw.dat",theHkData.unixTime,
		theHkData.unixTimeUs);
	    theHkData.ip320=rawDataStruct;
	    wrPtr=&hkRawWriter;
	    break;
	case(IP320_CAL):
	    sprintf(theFilename,"sshk_%d_%d.cal.dat",theHkData.unixTime,
		    theHkData.unixTimeUs);
	    theHkData.ip320=calDataStruct;
	    wrPtr=&hkCalWriter;
	    break;
	case(IP320_AVZ):
	    sprintf(theFilename,"sshk_%d_%d.avz.dat",theHkData.unixTime,
		    theHkData.unixTimeUs);
	    theHkData.ip320=autoZeroStruct;
//	    printf("First two %d %d\n",theHkData.ip320.board[0].data[0]&0xfff,
//		   theHkData.ip320.board[0].data[0]&0xfff);
	    wrPtr=&hkCalWriter;
	    break;
	default:
	    syslog(LOG_WARNING,"Unknown HK data code: %d",code);
	    if(printToScreen) 
		fprintf(stderr,"Unknown HK data code: %d\n",code);
	    return -1;
    }
	    


    fillGenericHeader(&theHkData,PACKET_HKD_SS,sizeof(SSHkDataStruct_t));

    if(code==IP320_RAW) {
      int chan=0;
      printf("Raw data: ");
      for(chan=0;chan<40;chan++) {
	printf("%d ",rawDataStruct.board.data[chan]);
      }
      printf("\n");
    }


    telemCount++;
    if(telemCount>=telemEvery) {    
	//Write file and make link for SIPd
      if(printToScreen) printf("%s\n",theFilename);
	sprintf(fullFilename,"%s/%s",HK_TELEM_DIR,theFilename);
	retVal=writeStruct(&theHkData,fullFilename,sizeof(SSHkDataStruct_t));     
	retVal+=makeLink(fullFilename,HK_TELEM_LINK_DIR);      
	
	telemCount=0;
    }
    
    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theHkData,sizeof(SSHkDataStruct_t),
			 theHkData.unixTime,wrPtr);
    if(retVal<0) {
	//Had an error
	//Do something
    }	    
    

    return retVal;
}
