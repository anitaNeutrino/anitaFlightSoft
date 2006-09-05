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


/* Flight soft includes */
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "serialLib/serialLib.h"
#include "includes/anitaStructures.h"

/* Vendor includes */
#include "cr7.h"
#include "carrier/apc8620.h"
#include "ip320/ip320.h"

/*Config stuff*/
int readConfigFile();


/* SBS Temperature Reading Functions */
void printSBSTemps (void);
int readSBSTemps ();

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
int ip320Ranges[NUM_IP320_BOARDS];
int numIP320s;
int printToScreen;
int readoutPeriod;
int calibrationPeriod;

int hkDiskBitMask;
AnitaHkWriterStruct_t hkRawWriter;
AnitaHkWriterStruct_t hkCalWriter;


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
    signal(SIGTERM, sigUsr2Handler);
    signal(SIGINT, sigUsr2Handler);
    
    //Dont' wait for children
    signal(SIGCLD, SIG_IGN); 



    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    /* Get Port Numbers */
    if (status == CONFIG_E_OK) {
	hkDiskBitMask=kvpGetInt("hkDiskBitMask",1);
	tempString=kvpGetString("hkdPidFile");
	if(tempString) {
	    strncpy(hkdPidFile,tempString,FILENAME_MAX-1);
	    writePidFile(hkdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Error getting hkdPidFile");
	    fprintf(stderr,"Error getting hkdPidFile\n");
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
    acromagSetup();
    openMagnetometer();
    if(fdMag) setupMagnetometer();
    do {
	if(printToScreen) printf("Initializing Hkd\n");
	retVal=readConfigFile();
	
	//    milliWait.tv_nsec=100000000;
	milliWait.tv_nsec=10000000;
	readoutPeriod*=2;
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
	    if((millisecs % readoutPeriod)==0) {
		
		time(&rawTime);		
		if(fdMag) sendMagnetometerRequest();
		readSBSTemps();
		if(fdMag) retVal=checkMagnetometer(); 
//		printf("Mag Ret: %d\n",retVal);
		if((rawTime-lastCal)>calibrationPeriod) {
		    ip320Calibrate();
		    outputData(IP320_CAL);
		    outputData(IP320_AVZ);
		    lastCal=rawTime;
		}
		ip320Read(0);
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
    closeMagnetometer();
    closeHkFilesAndTidy(&hkRawWriter);
    closeHkFilesAndTidy(&hkCalWriter);
    unlink(hkdPidFile);
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


int readSBSTemps ()
{
    API_RESULT main_Api_Result;

    static int errorCounter=0;
    int gotError=0;

    int localTemp = 0;
    int remoteTemp = 0;
    main_Api_Result = 
	fn_ApiCr7ReadLocalTemperature(&localTemp);
    
    if (main_Api_Result != API_SUCCESS)
    {
	gotError+=1;
	if(errorCounter<100) {
	    syslog(LOG_WARNING,"Couldn't read (%d of 100) SBS Local Temp: %d\t%s",
		   errorCounter,main_Api_Result,fn_ApiGetErrorMsg(main_Api_Result));
	    errorCounter++;
	}
    }

    main_Api_Result = 
	fn_ApiCr7ReadRemoteTemperature(&remoteTemp);
    
    if (main_Api_Result != API_SUCCESS)
    {
	gotError+=2;
	if(errorCounter<100) {
	    syslog(LOG_WARNING,"Couldn't read (%d of 100) SBS Remote Temp: %d\t%s",
		   errorCounter,main_Api_Result,fn_ApiGetErrorMsg(main_Api_Result));
	    errorCounter++;
	}
    }
    sbsData.temp[0]=(localTemp);
    sbsData.temp[1]=(remoteTemp);
    return gotError;
}


void printSBSTemps (void)
{
    int retVal,unixTime;
    retVal=readSBSTemps();
    unixTime=time(NULL);
    if(!retVal) 
	printf("%d %d %d\n",unixTime,sbsData.temp[0],sbsData.temp[1]);
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
    if(CarrierOpen(0, &carrierHandle) != S_OK) {
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
	config320[count].average = 1;     /* number of samples to average */
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
    if(printToScreen) printf("%s\n",theFilename);

    fillGenericHeader(&theHkData,PACKET_HKD,sizeof(HkDataStruct_t));

    
    //Write file and make link for SIPd
    sprintf(fullFilename,"%s/%s",HK_TELEM_DIR,theFilename);
    retVal=writeHk(&theHkData,fullFilename);     
    retVal+=makeLink(fullFilename,HK_TELEM_LINK_DIR);      
    
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
	    printf("%lX ",corDataStruct[count].data[i]);
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
    usleep(1);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
	retVal=read(fdMag,tempData,256);
//	printf("%s\n",tempData);
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
    usleep(1);
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
    int retVal=1,i;
    float x,y,z;
    int checksum,otherChecksum;
    int countSpaces=0;
    magData.x=0;
    magData.y=0;
    magData.z=0;
    retVal=isThereDataNow(fdMag);
//    printf("We think there is %d in the way of data\n",retVal);
    if(retVal!=1) return 0;
//    sleep(1);
    retVal=read(fdMag,tempData,256);

    if(retVal>10) {
//	printf("Retval %d\n",retVal);
//	strcpy(tempData,"0.23456 0.78900 0.23997 4C");
	sscanf(tempData,"D\n%f %f %f %x",&x,&y,&z,&checksum);
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
	    syslog(LOG_WARNING,"Bad Magnetometer Checksum %s",tempData);
	    return -1;
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
