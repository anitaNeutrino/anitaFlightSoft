/*! \file Hkd.c
  \brief The Hkd program that creates Hk objects 
    
  Reads in all the various different housekeeping quantities (temperatures, 
  voltages, etc.)

  Novemember 2004  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <libgen.h> //For Mac OS X

/* Flight soft includes */
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "serialLib/serialLib.h"
#include "includes/anitaStructures.h"

/* Vendor includes */
//#include "cr7.h"
#include "carrier/apc8620.h"
#include "ip320/ip320.h"

/*Config stuff*/
int readConfigFile();
int sortOutPidFile(char *progName);

/* SBS Temperature Reading Functions */
void printSBSTemps (void);
int readSBSTemps ();
int readSBSTemperatureFile(const char *tempFile);

/* Acromag Functions */
void acromagSetup();
int ip320Setup();
int ip320Calibrate();
int ip320Read(int fillCor);
void dumpValues();

/* Magnetometer Functions */
int openMagnetometer();
int setupMagnetometer();
int sendMagnetometerRequest();
int checkMagnetometer();
int closeMagnetometer();

/* Output Function */
int outputData(AnalogueCode_t code);
void prepWriterStructs();

/* Signal Handle */
void handleBadSigs(int sig);

//Neobrick Nonsense
void quickAddNeobrickPressAndTemp();

//Magnetometer Stuff
int fdMag; //Magnetometer
int sendMagRequests=1;

/* global variables for acromag control */
int carrierHandle;
struct cblk320 config320[NUM_IP320_BOARDS];

/* tables for ip320 data and calibration constants */
/* note sa_size is sort of arbitrary; 
   single channel reads only use first one */
struct scan_array scanArray[NUM_IP320_BOARDS][CHANS_PER_IP320]; 

FullAnalogueStruct_t autoZeroStruct;
FullAnalogueStruct_t rawDataStruct;
FullAnalogueStruct_t calDataStruct;
AnalogueCorrectedDataStruct_t corDataStruct[NUM_IP320_BOARDS];
MagnetometerDataStruct_t magData;
SBSTemperatureDataStruct_t sbsData;


/* Acromag constants */
#define INTERRUPT_LEVEL 10


/* Configurable thingummies */
//char carrierDevName[FILENAME_MAX];
int analogueCarrierNum=0;
int writeLookupFile=0;
int ip320Ranges[NUM_IP320_BOARDS];
int numIP320s;
int printToScreen=0;
int adcAverage=1;
int readoutPeriod; //in ms
int telemEvery;
int calibrationPeriod;

int hkDiskBitMask=0;
int disableUsb=0;
int disableNeobrick=0;
int disableSatablade=0;
int disableSatamini=0;
AnitaHkWriterStruct_t hkRawWriter;
AnitaHkWriterStruct_t hkCalWriter;

/* Advanced prettyprinting stuff */
#define MAX_COLUMN 5
#define MAX_ROW 20

//Pretty printing stuff for debugging
typedef struct {
  char description[20];
  float conversion;
  float offset;
  int active;
} ip320_desc; /* IP320 descriptor structure */


//Power stuff
ip320_desc descPower[CHANS_PER_IP320];
char headerPower[180];
int isTemp=0;
int prettyFormatPower = 0;
int numRowsPower = 0;
int rowaddrPower[MAX_ROW][MAX_COLUMN];

//Temp stuff
ip320_desc descTemp[CHANS_PER_IP320];
char headerTemp[180];
int prettyFormatTemp = 0;
int numRowsTemp = 0;
int rowaddrTemp[MAX_ROW][MAX_COLUMN];



void prettyPrintPowerLookupFile();
void prettyPrintTempLookupFile();
void readHkReadoutConfig();


int main (int argc, char *argv[])
{
    int retVal;



/*     int localSBSTemp,remoteSBSTemp; */
    /* Config file thingies */
    int status=0;
    char* eString ;
    int millisecs=0;
    struct timespec milliWait;
    milliWait.tv_sec=0;


    int lastCal=0;
    time_t rawTime;
    /* Log stuff */
    char *progName=basename(argv[0]);

    //Sort out PID File
    retVal=sortOutPidFile(progName);
    if(retVal!=0) {
      return retVal;
    }

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
    syslog(LOG_INFO,"Starting Hkd\n");

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, handleBadSigs);
    signal(SIGINT, handleBadSigs);
    signal(SIGSEGV, handleBadSigs);

    
    //Dont' wait for children
    signal(SIGCLD, SIG_IGN); 



    //Read some of HkReadout.config
    readHkReadoutConfig();

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    /* Get Port Numbers */
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
    autoZeroStruct.code=IP320_AVZ;
    rawDataStruct.code=IP320_RAW;
    calDataStruct.code=IP320_CAL;
    
    retVal=0;
    retVal=readConfigFile();

    makeDirectories(HK_TELEM_DIR);
    makeDirectories(HK_TELEM_LINK_DIR);

    prepWriterStructs();
	
//    printSBSTemps();
    acromagSetup();
    openMagnetometer();
    if(fdMag) setupMagnetometer();
    do {
	if(printToScreen) printf("Initializing Hkd\n");
	retVal=readConfigFile();
	
	//    milliWait.tv_nsec=100000000;

	ip320Setup();
	currentState=PROG_STATE_RUN;
	millisecs=0;
        while(currentState==PROG_STATE_RUN) {

/* 	    if((millisecs % calibrationPeriod)==0) { */
/* 		sendMagnetometerRequest(); */
/* 		ip320Calibrate(); */
/* 		readSBSTemps(); */
/* 		checkMagnetometer();		 */
/* 		outputData(IP320_CAL); */
/* 		outputData(IP320_AVZ); */
		
/* 		//Send down calibration info */
/* 	    } */
	    if(fdMag) retVal=checkMagnetometer(); 
	    if((millisecs % readoutPeriod)==0) {
		
		time(&rawTime);		
//		if(fdMag && sendMagRequests) sendMagnetometerRequest();
		readSBSTemps();

//		printf("Mag Ret: %d\n",retVal);
		if((rawTime-lastCal)>calibrationPeriod) {
		    ip320Calibrate();
		    outputData(IP320_CAL);
		    outputData(IP320_AVZ);
		    lastCal=rawTime;
		}
		ip320Read(writeLookupFile);
		quickAddNeobrickPressAndTemp();
		outputData(IP320_RAW);
		if(writeLookupFile) {
		  prettyPrintPowerLookupFile();
		  prettyPrintTempLookupFile();
		}
		
//		dumpValues();
		//Send down data
		millisecs=1;
	    }
	    //Just for safety we'll reset the sleep time
	    milliWait.tv_nsec=1000000;
	    nanosleep(&milliWait,NULL);
	    millisecs++;
//	    printf("%d\n",millisecs);
	}
    } while(currentState==PROG_STATE_INIT);
    closeMagnetometer();
    closeHkFilesAndTidy(&hkRawWriter);
    closeHkFilesAndTidy(&hkCalWriter);
    unlink(HKD_PID_FILE);
    syslog(LOG_INFO,"Hkd Terminating");
    return 0;
}

int readConfigFile() 
/* Load Hkd config stuff */
{
    /* Config file thingies */
    int status=0;
    int tempNum=3,count=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    kvpReset();
    status = configLoad ("Hkd.config","output") ;
    status &= configLoad ("Hkd.config","hkd") ;

    if(status == CONFIG_E_OK) {       
//	strncpy(carrierDevName,kvpGetString("ip320carrier"),FILENAME_MAX-1);
	analogueCarrierNum=kvpGetInt("analogueCarrierNum",0);
	numIP320s=kvpGetInt("numIP320s",3);
	printToScreen=kvpGetInt("printToScreen",-1);
	writeLookupFile=kvpGetInt("writeLookupFile",-1);
	readoutPeriod=kvpGetInt("readoutPeriod",60);
	telemEvery=kvpGetInt("telemEvery",60);
	adcAverage=kvpGetInt("adcAverage",5);
	calibrationPeriod=kvpGetInt("calibrationPeriod",1200);
       	kvpStatus = kvpGetIntArray("ip320Ranges",ip320Ranges,&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(ip320Ranges): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen) 
		fprintf(stderr,"kvpGetIntArray(ip320Ranges): %s",
			kvpErrorString(kvpStatus));
	}
	for(count=0;count<tempNum;count++) {
	    if(ip320Ranges[count]==5)  ip320Ranges[count]=RANGE_5TO5;
	    else if(ip320Ranges[count]==10) ip320Ranges[count]=RANGE_10TO10;
	}
    }

    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Hkd.config: %s\n",eString);
    }
    return status;
   
}


int readSBSTemperatureFile(const char *tempFile)
{
  static int errorCounter=0;
  int fd,retVal=0;
  char temp[6];
  int temp_con;
  fd = open(tempFile, O_RDONLY);
  if(fd<0) {
    if(errorCounter<100) {
      fprintf(stderr,"Error opening %s -- %s (Error %d of 100)",tempFile,strerror(errno),errorCounter);
      syslog(LOG_ERR,"Error opening %s -- %s (Error %d of 100)",tempFile,strerror(errno),errorCounter);
    }
  }
  else {    
    retVal=read(fd, temp, 6);   
    temp[5] = 0x0;
    temp_con = (atoi(temp)*4)/100;  // Reflects the accuracy of the temperature
    close(fd);
    return temp_con;
  }
  return 0;
}

int readSBSTemps ()
{  
  sbsData.temp[0]=readSBSTemperatureFile("/sys/class/hwmon/hwmon0/device/temp1_input");
  sbsData.temp[1]=readSBSTemperatureFile("/sys/class/hwmon/hwmon0/device/temp2_input");
  sbsData.temp[2]=readSBSTemperatureFile("/sys/class/hwmon/hwmon1/device/temp1_input");
  sbsData.temp[3]=readSBSTemperatureFile("/sys/class/hwmon/hwmon2/device/temp1_input");
  if(printToScreen) {
    printf("SBS Temps:\t%d %d %d %d\n",25*sbsData.temp[0],25*sbsData.temp[1],
	   25*sbsData.temp[2],25*sbsData.temp[3]);
  }
  return 0;
}


void printSBSTemps (void)
{
    int retVal,unixTime;
    retVal=readSBSTemps();
    unixTime=time(NULL);
    if(!retVal) 
      printf("%d %d %d %d %d\n",unixTime,sbsData.temp[0],sbsData.temp[1],
	     sbsData.temp[2],sbsData.temp[3]);
    /* Don't know what to do if it doesn't work */
}

 
void acromagSetup()
/* Seemingly does exactly what it says on the tin */
{
    int count=0;
    long addr=0;
  
    /* Check for Carrier Library */
    if(InitCarrierLib() != S_OK) {
	printf("\nCarrier library failure");
	exit(1);
    }
  
    /* Connect to Carrier */ 
    if(CarrierOpen(analogueCarrierNum, &carrierHandle) != S_OK) {
	printf("\nUnable to Open instance of carrier.\n");
	exit(2);
    }
    char letter[3]={'A','B','C'};
    for(count=0;count<numIP320s;count++) {
	config320[count].nHandle=carrierHandle;
	config320[count].slotLetter=letter[count];
    }


    if(CarrierInitialize(carrierHandle) == S_OK) 
    { /* Initialize Carrier */
	SetInterruptLevel(carrierHandle, INTERRUPT_LEVEL);
	/* Set carrier interrupt level */
    } 
    else 
    {
	syslog(LOG_ERR,"Unable initialize the carrier: %lX", addr);
	if(printToScreen) printf("\nUnable initialize the carrier: %lX", addr);
	exit(3);
    }

    for(count=0;count<numIP320s;count++) {
	config320[count].bCarrier=TRUE;
    }
  

    /* GetCarrierAddress(carrierHandle, &addr);
       SetCarrierAddress(carrierHandle, addr); */

    for(count=0;count<numIP320s;count++) {
	if(GetIpackAddress(carrierHandle,config320[count].slotLetter,
			   (long *) &config320[count].brd_ptr) != S_OK) 
	{
	    syslog(LOG_ERR,"Ipack address failure for IP320\n.");	  
	    if(printToScreen) printf("\nIpack address failure for IP320\n.");
	    exit(5);
	}
	config320[count].bInitialized=TRUE;
    }
}

int ip320Setup()
{
    int i,count;
    for(count=0;count<numIP320s;count++) {
//	printf("Doing ip320 setup %d\n",count);
	rsts320(&config320[count]);
	if((byte)config320[count].id_prom[5]!=0x32) {
	    if(printToScreen) {
		printf("Board ID Wrong\n\n");
		printf("Board ID Information\n");
		printf("\nIdentification:              ");
		for(i = 0; i < 4; i++)          /* identification */
		    printf("%c",config320[count].id_prom[i]);
		printf("\nManufacturer's ID: %X",(byte)config320[count].id_prom[4]);
		printf("\nIP Model Number: %X",(byte)config320[count].id_prom[5]);
		printf("\nRevision: %X",(byte)config320[count].id_prom[6]);
		printf("\nReserved: %X",(byte)config320[count].id_prom[7]);
		printf("\nDriver I.D. (low): %X",(byte)config320[count].id_prom[8]);
		printf("\nDriver I.D. (high): %X",(byte)config320[count].id_prom[9]);
		printf("\nTotal I.D. Bytes: %X",(byte)config320[count].id_prom[10]);
		printf("\nCRC: %X\n",(byte)config320[count].id_prom[11]);
		exit(0);
	    }
	}  
	else if(printToScreen)
	    printf("Board ID correct %X\n",(byte)config320[count].id_prom[5]);
      
	for( i = 0; i < CHANS_PER_IP320; i++ ) 
	{
	    scanArray[count][i].gain = GAIN_X1;      /*  gain=2 */
	    scanArray[count][i].chn = i;             /*  channel */
	    autoZeroStruct.board[count].data[i] = 0;                 /* clear auto zero buffer */
	    calDataStruct.board[count].data[i] = 0;                /* clear calibration buffer */
	    rawDataStruct.board[count].data[i] = 0;                /* clear raw input buffer */
	    corDataStruct[count].data[i] = 0;                /* clear corrected data buffer */
	}
	config320[count].range = ip320Ranges[count];
	config320[count].trigger = STRIG; /* 0 = software triggering */
	config320[count].mode = SEI;      /* differential input */
	config320[count].gain = GAIN_X1;  /* gain for analog input */
	config320[count].average = adcAverage;     /* number of samples to average */
	config320[count].channel = 0;     /* default channel */
	config320[count].data_mask = BIT12; /* A/D converter data mask */
	config320[count].bit_constant = CON12; /* constant for data correction */
  
	config320[count].s_raw_buf = &rawDataStruct.board[count].data[0];   /* raw buffer start */
	config320[count].s_az_buf = &autoZeroStruct.board[count].data[0]; /* auto zero buffer start */
	config320[count].s_cal_buf = &calDataStruct.board[count].data[0]; /* calibration buffer start */
	config320[count].s_cor_buf = &corDataStruct[count].data[0]; /* corrected buffer start */
	config320[count].sa_start = &scanArray[count][0]; /* address of start of scan array */
	config320[count].sa_end = &scanArray[count][CHANS_PER_IP320-1];  /* address of end of scan array*/

    }
    return 0;
}

int ip320Read(int fillCor)
{
    int count;
    for(count=0;count<numIP320s;count++) {    
	ainmc320(&config320[count]);
	if(fillCor) mccd320(&config320[count]);
    }
    return 0;
}


int ip320Calibrate()
{
    int count=0;
    byte temp_mode;
    for(count=0;count<numIP320s;count++) {
//	printf("Here\n");
	temp_mode=config320[count].mode;
	config320[count].mode= AZV; /* auto zero */	
	ainmc320(&config320[count]);
//	printf("Done auto zero\n");
	config320[count].mode= CAL; /* calibration */
	ainmc320(&config320[count]);
//	printf("Done calibration\n");
	config320[count].mode=temp_mode;
    }
    /* really should only do this every so often, */
    /* and send down the cal constants */
    return 0;
}

int outputData(AnalogueCode_t code) 
{
    int retVal;
    char theFilename[FILENAME_MAX];
    char fullFilename[FILENAME_MAX];
    HkDataStruct_t theHkData;
    AnitaHkWriterStruct_t *wrPtr;

    static int telemCount=0;


    struct timeval timeStruct;
    gettimeofday(&timeStruct,NULL);
	    
    theHkData.unixTime=timeStruct.tv_sec;;
    theHkData.unixTimeUs=timeStruct.tv_usec;;
    switch(code) {
	case (IP320_RAW):
	    sprintf(theFilename,"hk_%d_%d.raw.dat",theHkData.unixTime,
		theHkData.unixTimeUs);
	    theHkData.ip320=rawDataStruct;
	    wrPtr=&hkRawWriter;
	    break;
	case(IP320_CAL):
	    sprintf(theFilename,"hk_%d_%d.cal.dat",theHkData.unixTime,
		    theHkData.unixTimeUs);
	    theHkData.ip320=calDataStruct;
	    wrPtr=&hkCalWriter;
	    break;
	case(IP320_AVZ):
	    sprintf(theFilename,"hk_%d_%d.avz.dat",theHkData.unixTime,
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
	    
    theHkData.mag=magData;
    theHkData.sbs=sbsData;

//    if(printToScreen) printf("Mag here:\t%f %f %f\n",theHkData.mag.x,theHkData.mag.y,theHkData.mag.z);
//    if(printToScreen) printf("Mag there:\t%f %f %f\n",magData.x,magData.y,magData.z);
    if(printToScreen) printf("%s\n",theFilename);

    fillGenericHeader(&theHkData,PACKET_HKD,sizeof(HkDataStruct_t));


    telemCount++;
    if(telemCount>=telemEvery) {    
	//Write file and make link for SIPd
	sprintf(fullFilename,"%s/%s",HK_TELEM_DIR,theFilename);
	retVal=writeStruct(&theHkData,fullFilename,sizeof(HkDataStruct_t));     
	retVal+=makeLink(fullFilename,HK_TELEM_LINK_DIR);      
	
	telemCount=0;
    }
    
    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theHkData,sizeof(HkDataStruct_t),
			 theHkData.unixTime,wrPtr);
    if(retVal<0) {
	//Had an error
	//Do something
    }	    
    

    return retVal;
}


void dumpValues() {
    int i,count;
    for(count=0;count<numIP320s;count++) {
	printf("Data from %c\n\n",config320[count].slotLetter);	
	printf("Channel: ");
	for( i = 0; i < CHANS_PER_IP320; i++ ) 
	{
	    printf("%d ",scanArray[count][i].chn);
	}
	printf("\n\n");
	printf("Gain:    ");
	for( i = 0; i < CHANS_PER_IP320; i++ ) 
	{
	    printf("%d ",scanArray[count][i].gain);
	}
	printf("\n\n");
	printf("AVZ:     ");
	for( i = 0; i < CHANS_PER_IP320; i++ ) 
	{
	    printf("%X ",autoZeroStruct.board[count].data[i]);
	}
	printf("\n\n");
	printf("CAL:     ");
	for( i = 0; i < CHANS_PER_IP320; i++ ) 
	{
	    printf("%X ",calDataStruct.board[count].data[i]);
	}
	printf("\n\n");
	printf("RAW:     ");
	for( i = 0; i < CHANS_PER_IP320; i++ ) 
	{
	    printf("%X ",rawDataStruct.board[count].data[i]);
	}
	printf("\n\n");
	printf("COR:     ");
	for( i = 0; i < CHANS_PER_IP320; i++ ) 
	{
	    printf("%X ",corDataStruct[count].data[i]);
	}
	printf("\n\n");
    }
}


int openMagnetometer()
{

    int retVal;
// Initialize the various devices
    retVal=openMagnetometerDevice(MAGNETOMETER_DEV_NAME);
    fdMag=0;
    if(retVal<=0) {
	syslog(LOG_ERR,"Couldn't open: %s\n",MAGNETOMETER_DEV_NAME);
	if(printToScreen) printf("Couldn't open: %s\n",MAGNETOMETER_DEV_NAME);
//	exit(1);
    }
    else fdMag=retVal;

    printf("Opened %s %d\n",MAGNETOMETER_DEV_NAME,fdMag);
    return 0;
}

int setupMagnetometer() 
{
//    printf("Here\n");
    int retVal=0;
    char setupCommand[128];
    char tempData[256];

    sprintf(setupCommand,"*\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n");
	return -1;
    }
    else {
//	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//	if(printToScreen)
	    printf("Sent %d bytes to Magnetometer serial port:\t%s\n",retVal,MAGNETOMETER_DEV_NAME);
    }

sprintf(setupCommand,"M?\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n");
	return -1;
    }
    else {
//	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//	if(printToScreen)
	    printf("Sent %d bytes to Magnetometer serial port:\t%s\n",retVal,MAGNETOMETER_DEV_NAME);
    }

    usleep(1000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
	retVal=read(fdMag,tempData,256);
	printf("%s\n",tempData);
    }

    sprintf(setupCommand,"A\n"); //Start autosend data
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n");
	return -1;
    }
    else {
//	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//	if(printToScreen)
//	    printf("Sent %d bytes to Magnetometer serial port:\t%s\n",retVal,MAGNETOMETER_DEV_NAME);
    }
    usleep(1000);

   
    usleep(1000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
	retVal=read(fdMag,tempData,256);
	printf("%s\n",tempData);
    }
    sprintf(setupCommand,"M=T\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n");
	return -1;
    }
    else {
//	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//	if(printToScreen)
	    printf("Sent %d bytes to Magnetometer serial port:\t%s\n",retVal,MAGNETOMETER_DEV_NAME);
    }
    usleep(1000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
	retVal=read(fdMag,tempData,256);
	printf("%s\n",tempData);
    }
    sprintf(setupCommand,"M=C\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n");
	return -1;
    }
    else {
//	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//	if(printToScreen)
	    printf("Sent %d bytes to Magnetometer serial port:\t%s\n",retVal,MAGNETOMETER_DEV_NAME);
    }
    usleep(1000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
	retVal=read(fdMag,tempData,256);
	printf("%s\n",tempData);
    }

    sprintf(setupCommand,"M=E\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n");
	return -1;
    }
    else {
//	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//	if(printToScreen)
//	    printf("Sent %d bytes to Magnetometer serial port:\t%s\n",retVal,MAGNETOMETER_DEV_NAME);
    }
    usleep(1000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
	retVal=read(fdMag,tempData,256);
	printf("%s\n",tempData);
    }

    sprintf(setupCommand,"P=FFFF\n"); //Will need to play with this
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n");
	return -1;
    }
    else {
//	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//	if(printToScreen)
//	    printf("Sent %d bytes to Magnetometer serial port:\t%s\n",retVal,MAGNETOMETER_DEV_NAME);
    }
    usleep(1000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
	retVal=read(fdMag,tempData,256);
    }
    
     sprintf(setupCommand,"A\n"); //Start autosend data
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n");
	return -1;
    }
    else {
//	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//	if(printToScreen)
//	    printf("Sent %d bytes to Magnetometer serial port:\t%s\n",retVal,MAGNETOMETER_DEV_NAME);
    }
    usleep(1000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
	retVal=read(fdMag,tempData,256);
    }
    

/*     sprintf(setupCommand,"B=38400\n"); */
/*     retVal=write(fdMag, setupCommand, strlen(setupCommand)); */
/*     if(retVal<0) { */
/* 	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno)); */
/* 	if(printToScreen) */
/* 	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n"); */
/* 	return -1; */
/*     } */
/*     else { */
/* 	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal); */
/* 	if(printToScreen) */
/* 	    printf("Sent %d bytes to Magnetometer serial port:\t%s\n",retVal,MAGNETOMETER_DEV_NAME); */
/*     } */
/*     usleep(1); */
/*     retVal=isThereDataNow(fdMag); */
/*     if(retVal) { */
/* 	retVal=read(fdMag,tempData,256); */
/* 	printf("%s\n",tempData); */
/*     } */

//    M=E\nB=38400\n";
    return 0;
}


int sendMagnetometerRequest()
{
    int retVal=0;
    char *sendCommand = "D\n";
    retVal=write(fdMag, sendCommand, strlen(sendCommand));
//    printf("wrote buff -- %s\t%d\n",sendCommand,retVal);
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\n, write: %s", strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to Magnetometer Serial port\n");
	return -1;
    }
    else {
//	syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//	if(printToScreen)
//	    printf("Sent %d bytes to Magnetometer serial port\n",retVal);
    }
    return 0;
}


int checkMagnetometer()
{
    char tempData[256];
//    char tempData2[256];
    int retVal=1,i;
    float x,y,z;
    int checksum,otherChecksum;
    int countSpaces=0;
//    magData.x=0;
//    magData.y=0;
//    magData.z=0;
    retVal=isThereDataNow(fdMag);
//    printf("We think there is %d in the way of data\n",retVal);
    if(retVal!=1) return 0;
//    sleep(1);
    retVal=read(fdMag,tempData,256);

    if(retVal>10) {
//	printf("Retval %d\n",retVal);
//	strcpy(tempData,"0.23456 0.78900 0.23997 4C");
	sscanf(tempData,"%f %f %f %x",&x,&y,&z,&checksum);
//	printf("%s\n",tempData);
	if(printToScreen) printf("Mag:\t%f %f %f\t%d\n",x,y,z,checksum);
	magData.x=x;
	magData.y=y;
	magData.z=z;
	
	otherChecksum=0;
	for(i=0;i<strlen(tempData);i++) {
//	    otherChecksum+=tempData[i];
	    if(tempData[i]=='1') otherChecksum+=1;
	    if(tempData[i]=='2') otherChecksum+=2;
	    if(tempData[i]=='3') otherChecksum+=3;
	    if(tempData[i]=='4') otherChecksum+=4;
	    if(tempData[i]=='5') otherChecksum+=5;
	    if(tempData[i]=='6') otherChecksum+=6;
	    if(tempData[i]=='7') otherChecksum+=7;
	    if(tempData[i]=='8') otherChecksum+=8;
	    if(tempData[i]=='9') otherChecksum+=9;
	    if(otherChecksum && tempData[i]==' ') countSpaces++;
	    if(countSpaces==3) break;
//	    printf("%d %c %d\n",otherChecksum,tempData[i],countSpaces);
	}
	if(checksum!=otherChecksum) {
//	    syslog(LOG_WARNING,"Bad Magnetometer Checksum %s",tempData);
//	    return -1;
	}
	if(printToScreen) printf("Checksums:\t%d %d\n",checksum,otherChecksum);
    }
    return retVal;
}

int closeMagnetometer()
{
    return close(fdMag);   
}



void prepWriterStructs() {
    int diskInd;
    if(printToScreen) 
	printf("Preparing Writer Structs\n");
    //Hk Writer
    
    sprintf(hkRawWriter.relBaseName,"%s/raw",HK_ARCHIVE_DIR);
    sprintf(hkRawWriter.filePrefix,"hk_raw");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	hkRawWriter.currentFilePtr[diskInd]=0;
    hkRawWriter.writeBitMask=hkDiskBitMask;

    //Hk Cal Writer
    sprintf(hkCalWriter.relBaseName,"%s/cal",HK_ARCHIVE_DIR);
    sprintf(hkCalWriter.filePrefix,"hk_cal_avz");
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	hkCalWriter.currentFilePtr[diskInd]=0;
    hkCalWriter.writeBitMask=hkDiskBitMask;



}


void handleBadSigs(int sig)
{   
    fprintf(stderr,"Received sig %d -- will exit immediately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immediately\n",sig); 
    closeMagnetometer();
    closeHkFilesAndTidy(&hkRawWriter);
    closeHkFilesAndTidy(&hkCalWriter);
    unlink(HKD_PID_FILE);
    syslog(LOG_INFO,"Hkd terminating");    
    exit(0);
}

int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(HKD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,HKD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(HKD_PID_FILE);
  return 0;
}

void readHkReadoutConfig()
{
  //For now we'll only print out the power things
  int status,i=0;
  char *eString;
  char *temp;
  int readout=2;
  char hkreadout[9];

  //First up do the power
  readout=2;
  kvpReset();
  sprintf(hkreadout,"hkreadout%d",readout);
  status = configLoad("HkReadout.config",hkreadout);
  eString = configErrorString(status);
  if (status == CONFIG_E_OK) {
    prettyFormatPower = kvpGetInt("prettyformat",0);
    numRowsPower= kvpGetInt("numrows",0);
    if (numRowsPower > MAX_ROW) numRowsPower=MAX_ROW;
    if (numRowsPower)
      {
	char configstring[50];
	int i, nentries;
	memset(rowaddrPower, 0, sizeof(unsigned int)*MAX_ROW*MAX_COLUMN);
	for (i=0;i<numRowsPower;i++)
	  {
	    nentries=5;
	    sprintf(configstring,"row%dchannels", i+1);
	    kvpGetIntArray(configstring,rowaddrPower[i],&nentries);
	  }
	//	for (i=0;i<numRowsPower;i++)
	//	  printf("%d %d %d %d %d\n", rowaddrPower[i][0], rowaddrPower[i][1],
	//		 rowaddrPower[i][2], rowaddrPower[i][3], rowaddrPower[i][4]);
      }
    temp = kvpGetString("headername");
    if (temp != NULL) 
      strncpy(headerPower,temp,179);
    for (i=0;i<CHANS_PER_IP320;i++)
      {
	char configstring[50];
	sprintf(configstring, "chan%dname", i+1);
	temp = kvpGetString(configstring);
	if (temp != NULL) {
	  strncpy(descPower[i].description,temp,19);
	  //	  descPower[i].description = temp;
	  descPower[i].active = 1;
	  sprintf(configstring, "chan%dconvert", i+1);
	  descPower[i].conversion = kvpGetFloat(configstring, 1.0);
          sprintf(configstring, "chan%doffset", i+1);
          descPower[i].offset = kvpGetFloat(configstring, 0.0); 
	}
      }
  }
  else {
    printf("Unable to read config file.\n");
    printf("%s\n", eString);
    exit(1);
  }

  //Now the temps
  readout=3;
  kvpReset();
  sprintf(hkreadout,"hkreadout%d",readout);
  status = configLoad("HkReadout.config",hkreadout);
  eString = configErrorString(status);
  if (status == CONFIG_E_OK) {
    prettyFormatTemp = kvpGetInt("prettyformat",0);
    numRowsTemp= kvpGetInt("numrows",0);
    if (numRowsTemp > MAX_ROW) numRowsTemp=MAX_ROW;
    if (numRowsTemp)
      {
	char configstring[50];
	int i, nentries;
	memset(rowaddrTemp, 0, sizeof(unsigned int)*MAX_ROW*MAX_COLUMN);
	for (i=0;i<numRowsTemp;i++)
	  {
	    nentries=5;
	    sprintf(configstring,"row%dchannels", i+1);
	    kvpGetIntArray(configstring,rowaddrTemp[i],&nentries);
	  }
	//	for (i=0;i<numRowsTemp;i++)
	//	  printf("%d %d %d %d %d\n", rowaddrTemp[i][0], rowaddrTemp[i][1],
	//		 rowaddrTemp[i][2], rowaddrTemp[i][3], rowaddrTemp[i][4]);
      }
    temp = kvpGetString("headername");
    if (temp != NULL) 
      strncpy(headerTemp,temp,179);
    for (i=0;i<CHANS_PER_IP320;i++)
      {
	char configstring[50];
	sprintf(configstring, "chan%dname", i+1);
	temp = kvpGetString(configstring);
	if (temp != NULL) {
	  strncpy(descTemp[i].description,temp,19);
	  //	  descTemp[i].description = temp;
	  descTemp[i].active = 1;
	  sprintf(configstring, "chan%dconvert", i+1);
	  descTemp[i].conversion = kvpGetFloat(configstring, 1.0);
          sprintf(configstring, "chan%doffset", i+1);
          descTemp[i].offset = kvpGetFloat(configstring, 0.0); 
	}
      }
  }
  else {
    printf("Unable to read config file.\n");
    printf("%s\n", eString);
    exit(1);
  }
}

void prettyPrintPowerLookupFile()
{
  //For now just try and print power values
  time_t rawTime;
  time(&rawTime);
  int board=1;
  int useRange=ip320Ranges[board];
  int i, linecount=0;
  FILE *outFile=fopen(HK_POWER_LOOKUP,"w");

  fprintf(outFile,"Date: %s\n",ctime(&rawTime));  
  if (headerPower)
    fprintf(outFile,"%s:\n", headerPower);
  else
    fprintf(outFile,"No description available!\n");
  if (!numRowsPower || !prettyFormatPower) {
      for (i=0;i<CHANS_PER_IP320;i++)
      {
	  if (prettyFormatPower)
	  {
	      if (descPower[i].active)
	      {
		  /* RJN HACK */
		  fprintf(outFile,"%5.5s: %+7.3f ", descPower[i].description, (corDataStruct[board].data[i]*10.0/4095.-5.0*descPower[i].conversion+descPower[i].offset));
//	  fprintf(outFile,"%5.5s: %+7.2f ", descPower[i].description, (corDataStruct[board].data[i]*20.0/4095.-10.0));
//	  fprintf(outFile,"%5.5s: %+7.2f ", descPower[i].description,corDataStruct[board].data[i]);
		  linecount++;
		  if (!(linecount%5)) fprintf(outFile,"\n");
	      }
	  }
	  else
	  {
	      fprintf(outFile,"CH%2.2d: %5.3f ", i+1, (corDataStruct[board].data[i]*10.0/4095.-5.0));
	      if (!((i+1)%5)) fprintf(outFile,"\n");
	  }
      }
      fprintf(outFile,"\n");
  }
  else {
      for (i=0;i<numRowsPower;i++) {
      int j;
      for (j=0;j<MAX_COLUMN;j++) {
          if (rowaddrPower[i][j]) {
	      int chan = rowaddrPower[i][j]-1;  
	      /* RJN HACK */
	      if(isTemp) {
		  if(useRange==10)
		      fprintf(outFile,"%4.4s: %+4.2f\t", descPower[chan].description, (corDataStruct[board].data[chan]*20.0/4095.-10.0)*descPower[chan].conversion+descPower[chan].offset);
		  else 
		      fprintf(outFile,"%4.4s: %+4.2f\t", descPower[chan].description, (corDataStruct[board].data[chan]*10.0/4095.-5.0)*descPower[chan].conversion+descPower[chan].offset);
	      }
	      else {
		  if(useRange==10)
		      fprintf(outFile,"%7.7s: %+4.2f\t", descPower[chan].description, (corDataStruct[board].data[chan]*20.0/4095.-10.0)*descPower[chan].conversion+descPower[chan].offset);
		  else 
		      fprintf(outFile,"%7.7s: %+4.2f\t", descPower[chan].description, (corDataStruct[board].data[chan]*10.0/4095.-5.0)*descPower[chan].conversion+descPower[chan].offset);
	      }
	      	  
	  }
	  else fprintf(outFile,"               ");
      }
      fprintf(outFile,"\n");
      }
  }
  fclose(outFile);
  
}


void prettyPrintTempLookupFile()
{
  //For now just try and print temp values
  time_t rawTime;
  time(&rawTime);
  int board=2;
  int useRange=ip320Ranges[board];
  int i, linecount=0;
  FILE *outFile=fopen(HK_TEMP_LOOKUP,"w");

  fprintf(outFile,"Date: %s\n",ctime(&rawTime));  
  if (headerTemp)
    fprintf(outFile,"%s:\n", headerTemp);
  else
    fprintf(outFile,"No description available!\n");
  if (!numRowsTemp || !prettyFormatTemp) {
      for (i=0;i<CHANS_PER_IP320;i++)
      {
	  if (prettyFormatTemp)
	  {
	      if (descTemp[i].active)
	      {
		  /* RJN HACK */
		  fprintf(outFile,"%5.5s: %+7.3f ", descTemp[i].description, (corDataStruct[board].data[i]*10.0/4095.-5.0*descTemp[i].conversion+descTemp[i].offset));
//	  fprintf(outFile,"%5.5s: %+7.2f ", descTemp[i].description, (corDataStruct[board].data[i]*20.0/4095.-10.0));
//	  fprintf(outFile,"%5.5s: %+7.2f ", descTemp[i].description,corDataStruct[board].data[i]);
		  linecount++;
		  if (!(linecount%5)) fprintf(outFile,"\n");
	      }
	  }
	  else
	  {
	      fprintf(outFile,"CH%2.2d: %5.3f ", i+1, (corDataStruct[board].data[i]*10.0/4095.-5.0));
	      if (!((i+1)%5)) fprintf(outFile,"\n");
	  }
      }
      fprintf(outFile,"\n");
  }
  else {
      for (i=0;i<numRowsTemp;i++) {
      int j;
      for (j=0;j<MAX_COLUMN;j++) {
          if (rowaddrTemp[i][j]) {
	      int chan = rowaddrTemp[i][j]-1;  
	      /* RJN HACK */
	      if(1) {
		  if(useRange==10)
		      fprintf(outFile,"%4.4s: %+4.2f\t", descTemp[chan].description, (corDataStruct[board].data[chan]*20.0/4095.-10.0)*descTemp[chan].conversion+descTemp[chan].offset);
		  else 
		      fprintf(outFile,"%4.4s: %+4.2f\t", descTemp[chan].description, (corDataStruct[board].data[chan]*10.0/4095.-5.0)*descTemp[chan].conversion+descTemp[chan].offset);
	      }
	      else {
		  if(useRange==10)
		      fprintf(outFile,"%7.7s: %+4.2f\t", descTemp[chan].description, (corDataStruct[board].data[chan]*20.0/4095.-10.0)*descTemp[chan].conversion+descTemp[chan].offset);
		  else 
		      fprintf(outFile,"%7.7s: %+4.2f\t", descTemp[chan].description, (corDataStruct[board].data[chan]*10.0/4095.-5.0)*descTemp[chan].conversion+descTemp[chan].offset);
	      }
	      	  
	  }
	  else fprintf(outFile,"               ");
      }
      fprintf(outFile,"\n");
      }
  }
  int extraTemps[10]={39,19,38,18,37,17,36,16,35,15};
  for(i=0;i<2;i++) {				
      int j;
      for(j=0;j<5;j++) {
	  int ind=j +5*i;
	  if(ip320Ranges[0]==10 || 1) {
	      fprintf(outFile," T%d: %+4.2f\t", 41+ind , ((corDataStruct[0].data[extraTemps[ind]]*20.0/4095.-10.0)*100.)-273);
	  }
	  else {
	      fprintf(outFile," T%d: %+4.2f\t", 41+ind , ((corDataStruct[0].data[extraTemps[ind]]*10.0/4095.-5.0)*100.)-273);
	  }
      }
      fprintf(outFile,"\n");
  }

  

  fprintf(outFile,"\nSBS\t%4.2f\t%4.2f\t%4.2f\t%4.2f\n",
	  ((float)sbsData.temp[0])*25e-3,
	  ((float)sbsData.temp[1])*25e-3,
	  15+((float)sbsData.temp[2])*25e-3,
	  15+((float)sbsData.temp[3])*25e-3);


  fclose(outFile);
  
}

void quickAddNeobrickPressAndTemp()
{
    FILE *neoFile=0;
    unsigned int neoTime;
    unsigned long long neoSpaceUsed;
    unsigned long long neoSpaceAvailable;
    long long neoSpaceTotal;
    float neoTemp=0;
    float neoPress=0; 
    neoFile=fopen("/tmp/anita/neobrickSum.txt","r");
    if(neoFile) {
	fscanf(neoFile,"%u %llu %llu %llu %f %f",&neoTime,&neoSpaceUsed,&neoSpaceAvailable,&neoSpaceTotal,&neoTemp,&neoPress);
	fclose(neoFile);
    }
    float tempVolt=(neoTemp+273.15)/100.;
    float presVolt=(neoPress/400.);
    unsigned short tempAdc=((unsigned short)(tempVolt*(4055-2048)/4.9)+2048)<<4;
    unsigned short presAdc=((unsigned short)(presVolt*(4055-2048)/4.9)+2048)<<4;
    if(printToScreen) printf("Neobrick\t%f\t%f\t%u\t%u\n",neoTemp,neoPress,(tempAdc>>4),(presAdc>>4));	

    rawDataStruct.board[1].data[38]=tempAdc;
    rawDataStruct.board[1].data[39]=presAdc;
}

