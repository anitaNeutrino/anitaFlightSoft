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
#include <math.h>


/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "serialLib/serialLib.h"
#include "anitaStructures.h"

/* Vendor includes */
#include "cr7.h"
#include "carrier/apc8620.h"
#include "ip320/ip320.h"

/*Config stuff*/
int readConfigFile();


/* SBS Temperature Reading Functions */
//void printSBSTemps (void);
int fakeSBSTemps ();

/* Acromag Functions */
//void acromagSetup();
//int ip320Setup();
int fakeIP320Calibrate();
int fakeIP320Read(int fillCor);
//void dumpValues();

/* Magnetometer Functions */
//int openMagnetometer();
//int setupMagnetometer();
//int sendMagnetometerRequest();
int fakeMagnetometer();
//int closeMagnetometer();

/* Output Function */
int outputData(AnalogueCode_t code);
void prepWriterStructs();


//Magnetometer Stuff
int fdMag; //Magnetometer


/* global variables for acromag control */
int carrierHandle;
struct conf_blk_320 config320[NUM_IP320_BOARDS];

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
int useUSBDisks=0;

char hkdArchiveDir[FILENAME_MAX];
char hkdUSBArchiveDir[FILENAME_MAX];
int ip320Ranges[NUM_IP320_BOARDS];
int numIP320s;
int printToScreen;
int readoutPeriod;
int calibrationPeriod;


AnitaWriterStruct_t hkRawWriter;
AnitaWriterStruct_t hkCalWriter;


int main (int argc, char *argv[])
{
    int retVal;

// Hkd config stuff
    char hkdPidFile[FILENAME_MAX];

/*     int localSBSTemp,remoteSBSTemp; */
    /* Config file thingies */
    int status=0;
    char* eString ;
    char *tempString;
    int millisecs=0;
    struct timespec milliWait;
    milliWait.tv_sec=0;


    long lastCal=0;
    time_t rawTime;
    /* Log stuff */
    char *progName=basename(argv[0]);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    
    //Dont' wait for children
    signal(SIGCLD, SIG_IGN); 


    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status &= configLoad (GLOBAL_CONF_FILE,"whiteheat") ;
    eString = configErrorString (status) ;

    /* Get Port Numbers */
    if (status == CONFIG_E_OK) {
	useUSBDisks=kvpGetInt("useUSBDisks",0);
	tempString=kvpGetString("hkdPidFile");
	if(tempString) {
	    strncpy(hkdPidFile,tempString,FILENAME_MAX-1);
	    writePidFile(hkdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Error getting hkdPidFile");
	    fprintf(stderr,"Error getting hkdPidFile\n");
	}
       
	tempString=kvpGetString("mainDataDisk");
	if(tempString) {
	    strncpy(hkdArchiveDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error getting mainDataDisk");
	    fprintf(stderr,"Error getting mainDataDisk\n");
	}
	tempString=kvpGetString("usbDataDiskLink");
	if(tempString) {
	    strncpy(hkdUSBArchiveDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error getting usbDataDiskLink");
	    fprintf(stderr,"Error getting usbDataDiskLink\n");
	}
	    
	    
	tempString=kvpGetString("baseHouseArchiveDir");
	if(tempString) {
	    sprintf(hkdArchiveDir,"%s/%s/",hkdArchiveDir,tempString);
	    sprintf(hkdUSBArchiveDir,"%s/%s/",hkdUSBArchiveDir,tempString);
	}
	else {
	    syslog(LOG_ERR,"Error getting baseHouseArchiveDir");
	    fprintf(stderr,"Error getting baseHouseArchiveDir\n");
	}	    
	tempString=kvpGetString("hkArchiveSubDir");
	if(tempString) {
	    strcat(hkdArchiveDir,tempString);
	    strcat(hkdUSBArchiveDir,tempString);
	    makeDirectories(hkdArchiveDir);
	    if(useUSBDisks) makeDirectories(hkdUSBArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting hkArchiveSubDir");
	    fprintf(stderr,"Error getting hkArchiveSubDir\n");
	}
	    
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
//    acromagSetup();
//    openMagnetometer();
//    if(fdMag) setupMagnetometer();
    do {
	if(printToScreen) printf("Initializing Hkd\n");
	retVal=readConfigFile();
	
	//    milliWait.tv_nsec=100000000;
	milliWait.tv_nsec=10000000;
	readoutPeriod*=2;
//	ip320Setup();
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
	    if((millisecs % readoutPeriod)==0) {
		
		time(&rawTime);		
//		if(fdMag) sendMagnetometerRequest();
		fakeSBSTemps();
		retVal=fakeMagnetometer(); 
//		printf("Mag Ret: %d\n",retVal);
		if((rawTime-lastCal)>calibrationPeriod) {
		    fakeIP320Calibrate();
		    outputData(IP320_CAL);
		    outputData(IP320_AVZ);
		    lastCal=rawTime;
		}
		fakeIP320Read(0);
		outputData(IP320_RAW);
//		dumpValues();
		//Send down data
		millisecs=1;
	    }
	    nanosleep(&milliWait,NULL);
//	    sleep(1);
//	    usleep(50000);
//	    usleep(1);
	    millisecs++;
//	    printf("%d\n",millisecs);
	}
    } while(currentState==PROG_STATE_INIT);
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
	numIP320s=kvpGetInt("numIP320s",3);
	printToScreen=kvpGetInt("printToScreen",-1);
	readoutPeriod=kvpGetInt("readoutPeriod",60);
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


int fakeSBSTemps ()
{
    float randomNum;
    int ind;
    if(sbsData.temp[0]==0) sbsData.temp[0]=25;
    if(sbsData.temp[1]==0) sbsData.temp[1]=27;
    
    for(ind=0;ind<2;ind++) {
	randomNum=rand();
	if(randomNum<0.25)
	   sbsData.temp[ind]--;
	else if(randomNum>0.75) sbsData.temp[ind]++;
    }
    return 0;
}


int fakeIP320Read(int fillCor)
{
    int count,counter2;
    float randomNum;
    unsigned short val;

    for(count=0;count<numIP320s;count++) {    
	for(counter2=0;counter2<40;counter2++) {
	    randomNum=rand();
	    val=(unsigned short)(4096.*randomNum);
	    rawDataStruct.board[count].data[counter2]=val;
	}
    }
    return 0;
}


int fakeIP320Calibrate()
{
    int count,counter2;
    float randomNum;
    unsigned short val;

    for(count=0;count<numIP320s;count++) {    
	for(counter2=0;counter2<40;counter2++) {
	    randomNum=rand();
	    val=4095-(unsigned short)(10.*randomNum);
	    calDataStruct.board[count].data[counter2]=val;
	    randomNum=rand();
	    val=2053-(unsigned short)(10.*randomNum);
	    autoZeroStruct.board[count].data[counter2]=val;
	}
    }
    return 0;


}

int outputData(AnalogueCode_t code) 
{
    int retVal;
    char theFilename[FILENAME_MAX];
    char fullFilename[FILENAME_MAX];
    HkDataStruct_t theHkData;
    AnitaWriterStruct_t *wrPtr;


    struct timeval timeStruct;
    gettimeofday(&timeStruct,NULL);
	    
    theHkData.unixTime=timeStruct.tv_sec;;
    theHkData.unixTimeUs=timeStruct.tv_usec;;
    switch(code) {
	case (IP320_RAW):
	    sprintf(theFilename,"hk_%ld_%ld.raw.dat",theHkData.unixTime,
		theHkData.unixTimeUs);
	    theHkData.ip320=rawDataStruct;
	    wrPtr=&hkRawWriter;
	    break;
	case(IP320_CAL):
	    sprintf(theFilename,"hk_%ld_%ld.cal.dat",theHkData.unixTime,
		    theHkData.unixTimeUs);
	    theHkData.ip320=calDataStruct;
	    wrPtr=&hkCalWriter;
	    break;
	case(IP320_AVZ):
	    sprintf(theFilename,"hk_%ld_%ld.avz.dat",theHkData.unixTime,
		    theHkData.unixTimeUs);
	    theHkData.ip320=autoZeroStruct;
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
    if(printToScreen) printf("%s\n",theFilename);

    fillGenericHeader(&theHkData,PACKET_HKD,sizeof(HkDataStruct_t));

    
    //Write file and make link for SIPd
    sprintf(fullFilename,"%s/%s",HK_TELEM_DIR,theFilename);
    retVal=writeHk(&theHkData,fullFilename);     
    retVal+=makeLink(fullFilename,HK_TELEM_LINK_DIR);      
    
    //Write file to main disk
    retVal=cleverHkWrite((char*)&theHkData,sizeof(HkDataStruct_t),
			 theHkData.unixTime,wrPtr);
    if(retVal<0) {
	//Had an error
	//Do something
    }	    
    

    return retVal;
}


int fakeMagnetometer()
{
    magData.x=5.*rand();
    magData.y=5.*rand();
    magData.z=5.*rand();
    return 0;
}



void prepWriterStructs() {
    if(printToScreen) 
	printf("Preparing Writer Structs\n");
    //Hk Writer
    
    sprintf(hkRawWriter.baseDirname,"%s/raw",hkdArchiveDir);
    sprintf(hkRawWriter.filePrefix,"hk_raw");
    hkRawWriter.currentFilePtr=0;
    hkRawWriter.maxSubDirsPerDir=HK_FILES_PER_DIR;
    hkRawWriter.maxFilesPerDir=HK_FILES_PER_DIR;
//    hkRawWriter.maxWritesPerFile=10;
    hkRawWriter.maxWritesPerFile=HK_PER_FILE;

    //Hk Cal Writer
    sprintf(hkCalWriter.baseDirname,"%s/cal",hkdArchiveDir);
    sprintf(hkCalWriter.filePrefix,"hk_cal_avz");
    hkCalWriter.currentFilePtr=0;
    hkCalWriter.maxSubDirsPerDir=HK_FILES_PER_DIR;
    hkCalWriter.maxFilesPerDir=HK_FILES_PER_DIR;
    hkCalWriter.maxWritesPerFile=HK_PER_FILE;


}
