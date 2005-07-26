/*! \file GPSd.c
    \brief The GPSd program that creates GPS objects 
    
    Talks to the GPS units and does it's funky stuff

    Novemember 2004  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>


/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "serialLib/serialLib.h"
#include "utilLib/utilLib.h"
#include "socketLib/socketLib.h"
#include "anitaStructures.h"

// Forward declarations
int readConfigFile();
int openDevices();
int setupG12();
int setupADU5();
int checkG12();
int checkADU5A();
void processG12Output(char *tempBuffer, int length);
void processADU5Output(char *tempBuffer, int length);
int updateClockFromG12(time_t gpsRawTime);
int breakdownG12TimeString(char *subString,int *hour,int *minute,int *second);
int writeG12TimeFile(time_t compTime, time_t gpsTime, char *g12Output);
int checkChecksum(char *gpsString,int gpsLength);


// Definitions
#define LEAP_SECONDS 13
#define G12_DATA_SIZE 1024
#define ADU5_DATA_SIZE 1024


// Device names;
char g12DevName[FILENAME_MAX];
char adu5ADevName[FILENAME_MAX];
char adu5BDevName[FILENAME_MAX];

// File desciptors for GPS serial ports
int fdG12,fdAdu5A,fdAdu5B;//,fdMag
int printToScreen=0;

// Config stuff for G12
float g12PPSPeriod=1;
float g12PPSOffset=0.05;
int g12PPSRisingOrFalling=1; //1 is R, 2 is F
int g12ZDAPort=1;
int g12ZDAPeriod=5;
int g12UpdateClock=0;
int g12ClockSkew=0;
char g12LogDir[FILENAME_MAX];

// Config stuff for ADU5
int adu5SatPeriod=3600;
int adu5PatPeriod=10;
float adu5RelV12[3]={0};
float adu5RelV13[3]={0};
float adu5RelV14[3]={0};


int main (int argc, char *argv[])
{
    int retVal;

// GPSd config stuff
    char gpsdPidFile[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;

    /* Log stuff */
    char *progName=basename(argv[0]);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGINT, sigIntHandler);
    signal(SIGTERM,sigTermHandler);
   
    /* Load Global Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status &= configLoad (GLOBAL_CONF_FILE,"whiteheat") ;
    eString = configErrorString (status) ;

    /* Get Device Names and config stuff */
    if (status == CONFIG_E_OK) {
	strncpy(gpsdPidFile,kvpGetString("gpsdPidFile"),FILENAME_MAX-1);
	writePidFile(gpsdPidFile);
	strncpy(g12DevName,kvpGetString("g12DevName"),FILENAME_MAX-1);
	strncpy(adu5ADevName,kvpGetString("adu5PortADevName"),FILENAME_MAX-1);
	strncpy(adu5BDevName,kvpGetString("adu5PortBDevName"),FILENAME_MAX-1);
	strncpy(g12LogDir,kvpGetString("gpsdG12LogDir"),FILENAME_MAX-1);
    }

    
    retVal=readConfigFile();
    if(retVal<0) {
	syslog(LOG_ERR,"Problem reading GPSd.config");
	printf("Problem reading GPSd.config\n");
	exit(1);
    }
    retVal=openDevices();
    
    do {
	if(printToScreen) printf("Initalizing GPSd\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading GPSd.config");
	    printf("Problem reading GPSd.config\n");
	    exit(1);
	}
	if(printToScreen) 
	    printf("Device fd's: %d %d %d\n",fdG12,fdAdu5A,fdAdu5B);
	retVal=setupG12();
	retVal=setupADU5();

	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
//	    checkG12();
	    checkADU5A();
	    usleep(1);
	}
    } while(currentState==PROG_STATE_INIT);

    if(printToScreen) printf("Terminating GPSd\n");
    removeFile(gpsdPidFile);
    
	
    return 0;
}


int readConfigFile() 
/* Load GPSd config stuff */
{
    /* Config file thingies */
    int status=0;
    int tempNum=3;;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    kvpReset();
    status = configLoad ("GPSd.config","output") ;
    status &= configLoad ("GPSd.config","g12") ;
    status &= configLoad ("GPSd.config","adu5") ;

   if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",-1);
	if(printToScreen<0) {
	    syslog(LOG_WARNING,"Couldn't fetch printToScreen, defaulting to zero");
	    printToScreen=0;	    
	}

	g12PPSPeriod=kvpGetFloat("ppsPeriod",1); //in seconds
	g12PPSOffset=kvpGetFloat("ppsOffset",0.05); //in seconds 
	g12PPSRisingOrFalling=kvpGetInt("ppsRorF",1); //1 is 'R', 2 is 'F'
	g12ZDAPort=kvpGetInt("zdaPort",1); //1 is 'A', 2 is 'B'
	g12ZDAPeriod=kvpGetInt("zdaPeriod",5); // in seconds
	g12UpdateClock=kvpGetInt("updateClock",0); // 1 is yes, 0 is no
	g12ClockSkew=kvpGetInt("clockSkew",0); // Time difference in seconds
	adu5SatPeriod=kvpGetInt("satPeriod",0);
	adu5PatPeriod=kvpGetInt("patPeriod",0);
	tempNum=3;
	kvpStatus = kvpGetFloatArray("calibV12",adu5RelV12,&tempNum);
	if(kvpStatus!=KVP_E_OK) 
	    syslog(LOG_WARNING,"kvpGetFloatArray(calibV12): %s",
		   kvpErrorString(kvpStatus));
	kvpStatus = kvpGetFloatArray("calibV13",adu5RelV13,&tempNum);
	if(kvpStatus!=KVP_E_OK) 
	    syslog(LOG_WARNING,"kvpGetFloatArray(calibV13): %s",
		   kvpErrorString(kvpStatus));
	kvpStatus = kvpGetFloatArray("calibV14",adu5RelV14,&tempNum);
	if(kvpStatus!=KVP_E_OK) 
	    syslog(LOG_WARNING,"kvpGetFloatArray(calibV14): %s",
		   kvpErrorString(kvpStatus));
    }
   else {
       eString=configErrorString (status) ;
       syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
   }

//    printf("%f %f %f\n",adu5RelV12[0],adu5RelV12[1],adu5RelV12[2]);
//    printf("%f %f %f\n",adu5RelV13[0],adu5RelV13[1],adu5RelV13[2]);
//    printf("%f %f %f\n",adu5RelV14[0],adu5RelV14[1],adu5RelV14[2]);

//    printf("%d %s %s %s\n",printToScreen,g12DevName,adu5ADevName,adu5BDevName);
   return status;
}

int openDevices()
/*!
  Open connections to the GPS devices
*/
{
    int retVal;
// Initialize the various devices    
    retVal=openGPSDevice(g12DevName);
    if(retVal<=0) {
	syslog(LOG_ERR,"Couldn't open: %s\n",g12DevName);
	if(printToScreen) printf("Couldn't open: %s\n",g12DevName);
	exit(1);
    }
    else fdG12=retVal;
      
    retVal=openGPSDevice(adu5ADevName);	
    if(retVal<=0) {
	syslog(LOG_ERR,"Couldn't open: %s\n",adu5ADevName);
	if(printToScreen) printf("Couldn't open: %s\n",adu5ADevName);
	exit(1);
    }
    else fdAdu5A=retVal;

    retVal=openGPSDevice(adu5BDevName);	
    if(retVal<=0) {
	syslog(LOG_ERR,"Couldn't open: %s\n",adu5BDevName);
	if(printToScreen) printf("Couldn't open: %s\n",adu5BDevName);
	exit(1);
    }
    else fdAdu5B=retVal;
    return 0;
}


int setupG12()
/*! Initializes the G12 with the correct PPS and ZDA settings */
{
    char portNames[2]={'A','B'};
    char edgeNames[2]={'R','F'};
    char g12Command[256]="";
    char tempCommand[128]="";
    char zdaPort='B';
    char ppsEdge='R';
    int retVal;

    if(g12ZDAPort==1 || g12ZDAPort==2) zdaPort=portNames[g12ZDAPort-1];
    if(g12PPSRisingOrFalling==1 || g12PPSRisingOrFalling==2)
	ppsEdge=edgeNames[g12PPSRisingOrFalling-1];

    strcat(g12Command,"$PASHS,ELM,0\n");
    sprintf(tempCommand,"$PASHS,NME,ALL,%c,OFF\n",zdaPort);
    strcat(g12Command,tempCommand);
    strcat(g12Command, "$PASHS,LTZ,0,0\n");
    strcat(g12Command, "$PASHS,UTS,ON\n");
    sprintf(tempCommand,"$PASHS,NME,ZDA,%c,ON,%d\n",zdaPort,g12ZDAPeriod);
    strcat(g12Command,tempCommand); 
    sprintf(tempCommand, "$PASHS,PPS,%f,%f,%c\n",g12PPSPeriod,
	    g12PPSOffset,ppsEdge);   
    strcat(g12Command,tempCommand); 
    

    retVal=write(fdG12, g12Command, strlen(g12Command));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to G12 Serial port\n, write: %s",
	       strerror(errno));
    }
    else {
	syslog(LOG_INFO,"Sent %d bytes to G12 serial port",retVal);
    }
    return retVal;
}


int checkG12()
/*! Try to read G12 */
{
    // Working variables
    char tempData[G12_DATA_SIZE];
    int retVal,i;
// Data stuff for G12
    static char g12Output[G12_DATA_SIZE]="";
    static int g12OutputLength=0;
    retVal=isThereDataNow(fdG12);
//    printf("Check G12 got retVal %d\n",retVal);
    if(retVal!=1) return 0;
    retVal=read(fdG12, tempData, G12_DATA_SIZE);
    if(retVal>0) {
	for(i=0; i < retVal; i++) {
/* 	 printf("%c", data[i]); */
	    if(tempData[i]=='*' && (retVal-i)>=2) {
		g12Output[g12OutputLength++]=tempData[i++];
		g12Output[g12OutputLength++]=tempData[i++];
		g12Output[g12OutputLength++]=tempData[i++];

/* 	 printf("%d\n",currentIndex); */
		if(g12OutputLength) {
		    processG12Output(g12Output,g12OutputLength);
/* 		 printf("%d\t%s\n",time(NULL),tempBuffer); */
/* 		 for(i=0; i < DATA_SIZE; i++) tempBuffer[i]=0; */
		}
		g12OutputLength=0;
	    }
	    g12Output[g12OutputLength++]=tempData[i];
/* 	 printf("%d\n",currentIndex); */
	    tempData[i]=0;
	}
	
    }
    else if(retVal<0) {
	syslog(LOG_ERR,"G12 read error: %s",strerror(errno));
    }
    return retVal;
}


int checkADU5A()
/*! Try to read ADU5 */
{
    // Working variables
    char tempData[ADU5_DATA_SIZE];
    int retVal,i;
// Data stuff for ADU5
    static char adu5Output[ADU5_DATA_SIZE]="";
    static int adu5OutputLength=0;
    retVal=isThereDataNow(fdAdu5B);
    usleep(5);
//    printf("Check ADU5 got retVal %d\n",retVal);
    if(retVal!=1) return 0;
    retVal=read(fdAdu5B, tempData, ADU5_DATA_SIZE);
    if(retVal>0) {
	for(i=0; i < retVal; i++) {
/* 	 printf("%c", data[i]); */
	    if(tempData[i]=='*' && (retVal-i)>2) {
		adu5Output[adu5OutputLength++]=tempData[i++];
		adu5Output[adu5OutputLength++]=tempData[i++];
		adu5Output[adu5OutputLength++]=tempData[i++];

/* 	 printf("%d\n",currentIndex); */
		if(adu5OutputLength) {
		    processADU5Output(adu5Output,adu5OutputLength);
/* 		 printf("%d\t%s\n",time(NULL),tempBuffer); */
/* 		 for(i=0; i < DATA_SIZE; i++) tempBuffer[i]=0; */
		}
		adu5OutputLength=0;
	    }
	    adu5Output[adu5OutputLength++]=tempData[i];
/* 	 printf("%d\n",currentIndex); */
	    tempData[i]=0;
	}
	
    }
    else if(retVal<0) {
	syslog(LOG_ERR,"ADU5 read error: %s",strerror(errno));
    }
    return retVal;
}


void processG12Output(char *tempBuffer, int length)
/* Processes each G12 output string, writes it to file and, if so inclined, sets the clock. */
{
    char gpsString[G12_DATA_SIZE];
    char gpsCopy[G12_DATA_SIZE];
    int gpsLength=0;
    int count=0;
//    char theCheckSum=0;
    char *subString;
    int hour,minute,second,subSecond;
    int day=-1,month=-1,year=-1;
    int tzHour,tzMin;
    time_t rawtime,gpsRawTime;
    struct tm timeinfo;    
//    char checksum[3];   
//    strncpy(gpsString,tempBuffer,length);

    for(count=0;count<length;count++) {
	if(gpsLength) gpsString[gpsLength++]=tempBuffer[count];
	else if(tempBuffer[count]=='$') 
	    gpsString[gpsLength++]=tempBuffer[count];
    }
    if(!gpsLength) {
	return; //Crap data
    }
    gpsString[gpsLength]='\0';
    if(checkChecksum(gpsString,gpsLength)!=1) {
	syslog(LOG_WARNING,"Checksum failed: %s",gpsString);
	if(printToScreen) printf("Checksum failed %s\n",gpsString);
	return;
    }

//    printf("%s\n",tempBuffer);
//    printf("GPS Length %d\n",gpsLength);

    strncpy(gpsCopy,gpsString,G12_DATA_SIZE);
    if(printToScreen) printf("%s\t%d\n",gpsString,gpsLength);

    /* Should do checksum */
    count=0;
    subString = strtok (gpsString,",");
    if(!strcmp(subString,"$GPZDA") && gpsLength==37) {
//	if(printToScreen) printf("Got GPZDA\n");
	while (subString != NULL)
	{
	    switch(count) {
		case 1: 
/* 		    printf("subString: %s\n",subString); */
		    breakdownG12TimeString(subString,&hour,&minute,&second);
//		    printf("Length %d\n",strlen(subString));
		    break;
		case 2:
		    subSecond=atoi(subString);
		    break;
		case 3:
		    day=atoi(subString);
		    break;
		case 4:
		    month=atoi(subString);
		    break;
		case 5:
		    year=atoi(subString);
		    break;
		case 6:
		    tzHour=atoi(subString);
		    break;
		case 7:
		    tzMin=atoi(subString);
		    break;
		case 8:
/* 		    checksum[0]=subString[0]; */
/* 		    checksum[1]=subString[1]; */
/* 		    checksum[2]='\0';		   */  
		    break;
		default: 
		    break;
	    }
// 	    printf ("%d\t%d\t%s\n",count,strlen(subString),subString); 
	    count++;
	    subString = strtok (NULL, " ,.*");
	}


	    
/* 	printf("%02d:%02d:%02d.%02d %02d %02d %02d\n%s\n",hour,minute,second,subSecond,day,month,year,ctime(&rawtime)); */
	timeinfo.tm_hour=hour;
	timeinfo.tm_mday=day;
	timeinfo.tm_min=minute;
	timeinfo.tm_mon=month-1;
	timeinfo.tm_sec=second;
	timeinfo.tm_year=year-1900;
	gpsRawTime=mktime(&timeinfo);
	time ( &rawtime );
// 	printf("%d\t%d\n",rawtime,gpsRawTime); 
//	strncpy(otherString,ctime(&gpsRawTime),179);	    
//	strncpy(unixString,ctime(&rawtime),179);
//	printf("%s%s\n",unixString,otherString);
	if(abs(gpsRawTime-rawtime)>g12ClockSkew && g12UpdateClock) {
	    updateClockFromG12(gpsRawTime);
	}
	writeG12TimeFile(rawtime,gpsRawTime,gpsCopy);
	    
    }

}


void processADU5Output(char *tempBuffer, int length)
/* Processes each ADU5 output string, writes it to file and, if so inclined, sets the clock. */
{
    char gpsString[ADU5_DATA_SIZE];
    char gpsCopy[ADU5_DATA_SIZE];
    int gpsLength=0;
    int count=0;
//    char theCheckSum=0;
/*     char *subString; */
/*     int hour,minute,second,subSecond; */
/*     int day=-1,month=-1,year=-1; */
/*     int tzHour,tzMin; */
/*     time_t rawtime,gpsRawTime; */
/*     struct tm timeinfo;     */
//    char checksum[3];   
//    strncpy(gpsString,tempBuffer,length);

    for(count=0;count<length;count++) {
	if(gpsLength) gpsString[gpsLength++]=tempBuffer[count];
	else if(tempBuffer[count]=='$') 
	    gpsString[gpsLength++]=tempBuffer[count];
    }
    if(!gpsLength) {
	return; //Crap data
    }
    gpsString[gpsLength]='\0';
    if(checkChecksum(gpsString,gpsLength)!=1) {
	syslog(LOG_WARNING,"Checksum failed: %s",gpsString);
	if(printToScreen) printf("Checksum failed %s\n",gpsString);
	return;
    }

//    printf("%s\n",tempBuffer);
//    printf("GPS Length %d\n",gpsLength);

    strncpy(gpsCopy,gpsString,ADU5_DATA_SIZE);
    if(printToScreen) printf("%s\t%d\n",gpsString,gpsLength);
}


int updateClockFromG12(time_t gpsRawTime)
/*!Just updates the clock, using sudo date */
{
    int retVal;
    char writeString[180];
    char dateCommand[180];
    gpsRawTime++; /*Silly*/	
    strncpy(writeString,ctime(&gpsRawTime),179);
    sprintf(dateCommand,"sudo date -s \"%s\"",writeString);
    retVal=system(dateCommand);
    if(retVal<0) {
	syslog(LOG_ERR,"Problem updating clock, %ld: %s ",gpsRawTime,strerror(errno));
    }
    return retVal;
    
}


int breakdownG12TimeString(char *subString, int *hour, int *minute, int *second) 
/*!Splits up the time string into hours, minutes and seconds*/
{

    char hourStr[3],minuteStr[3],secondStr[3];
    
    hourStr[0]=subString[0];
    hourStr[1]=subString[1];
    hourStr[2]='\0';
    minuteStr[0]=subString[2];
    minuteStr[1]=subString[3];
    minuteStr[2]='\0';
    secondStr[0]=subString[4];
    secondStr[1]=subString[5];
    secondStr[2]='\0';
    
    *hour=atoi(hourStr);
    *minute=atoi(minuteStr);
    *second=atoi(secondStr);    
    return 0;
}

int writeG12TimeFile(time_t compTime, time_t gpsTime, char *g12Output)
/*!
  Keeps a permanent on-board record of the time from the GPS unit
*/
{
    int retVal;
    static int firstTime=1;
    static char logFileName[FILENAME_MAX];
    FILE *fpG12;
    
//    printf("%ld\n",compTime);
//    printf("%ld\n",gpsTime);
//    printf("%s\n",g12Output);

    if(firstTime) {
	sprintf(logFileName,"%s/g12Times.%ld",g12LogDir,compTime);
	if(printToScreen) printf("G12 Log File%s\n",logFileName);
	firstTime=0;
    }

    fpG12 = fopen(logFileName,"wat");
    if(fpG12<0) {
	syslog(LOG_ERR,"Error writing to g12 log file: %s",strerror(errno));
	if(printToScreen) 
	    fprintf(stderr,"Error writing to g12 log file");
    }
//    printf("%ld %ld %s\n",compTime,gpsTime,g12Output);
    retVal=fprintf(fpG12,"%ld %ld %s\n",compTime,gpsTime,g12Output); 
    fclose(fpG12); 
    return retVal;
}


int setupADU5()
/*! Initializes the ADU5 with the correct settings */
{
    char adu5Command[1024]="";
    char tempCommand[128]="";
    int retVal;

    strcat(adu5Command,"$PASHS,ELM,0\n"); 
    
    sprintf(tempCommand,"$PASHS,3DF,V12,%2.3f,%2.3f,%2.3f\r\n",
	    adu5RelV12[0],adu5RelV12[1],adu5RelV12[2]);
    strcat(adu5Command,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,V13,%2.3f,%2.3f,%2.3f\r\n",
	    adu5RelV12[0],adu5RelV12[1],adu5RelV12[2]);
    strcat(adu5Command,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,V14,%2.3f,%2.3f,%2.3f\r\n",
	    adu5RelV12[0],adu5RelV12[1],adu5RelV12[2]);
    strcat(adu5Command,tempCommand);
    strcat(adu5Command,"$PASHS,NME,ALL,A,OFF\r\n");
    strcat(adu5Command,"$PASHS,NME,ALL,B,OFF\r\n");
    strcat(adu5Command,"$PASHS,NME,TTT,A,ON\r\n");
    sprintf(tempCommand,"$PASHS,NME,PAT,A,ON,%d\r\n",adu5PatPeriod);
    strcat(adu5Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,SAT,A,ON,%d\r\n",adu5SatPeriod);
    strcat(adu5Command,tempCommand);

    if(printToScreen) printf("%s\n",adu5Command);
    retVal=write(fdAdu5A, adu5Command, strlen(adu5Command));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to ADU5 Serial port\n, write: %s",
	       strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to ADU5 Serial port\n");
    }
    else {
	syslog(LOG_INFO,"Sent %d bytes to ADU5 serial port",retVal);
	if(printToScreen)
	    printf("Sent %d bytes to ADU5 serial port\n",retVal);
    }
    return retVal;
}

int checkChecksum(char *gpsString,int gpsLength) 
// Returns 1 if checksum checks out
{
    char theCheckSum=0;
    char temp;
    int count;
/*     char checksum[3]; */
    for(count=1;count<gpsLength-3;count++) {
	memcpy(&temp,&gpsString[count],sizeof(char));
//	printf("%c ",gpsString[count]);
	theCheckSum^=temp;	
    }
    char hexChars[16]={'0','1','2','3','4','5','6','7','8','9','A',
		       'B','C','D','E','F'};
    int otherSum=0;
    for(count=0;count<16;count++) {
	if(gpsString[gpsLength-1]==hexChars[count]) otherSum+=count;
	if(gpsString[gpsLength-2]==hexChars[count]) otherSum+=count*16;
    }

/*     checksum[0]=gpsString[gpsLength-2]; */
/*     checksum[1]=gpsString[gpsLength-1]; */
/*     checksum[2]='\0'; */
/*     printf("%s %x %x\n",checksum,theCheckSum,otherSum); */
//    printf("\n\n\n%d %x\n",theCheckSum,theCheckSum);
    
    return (theCheckSum==otherSum);
}
