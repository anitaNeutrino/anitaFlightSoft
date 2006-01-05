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
void processG12Output(char *tempBuffer, int length,int setClock);
void processADU5Output(char *tempBuffer, int length);
int updateClockFromG12(time_t gpsRawTime);
int breakdownG12TimeString(char *subString,int *hour,int *minute,int *second);
int writeG12TimeFile(time_t compTime, time_t gpsTime, char *g12Output);
int checkChecksum(char *gpsString,int gpsLength);
void processGPPATString(char *gpsString, int length);
void processTTTString(char *gpsString, int gpsLength);
void processSATString(char *gpsString, int gpsLength);

// Definitions
#define LEAP_SECONDS 13
#define G12_DATA_SIZE 1024
#define ADU5_DATA_SIZE 1024


// Device names;
char g12ADevName[FILENAME_MAX];
//char g12BDevName[FILENAME_MAX];
char adu5ADevName[FILENAME_MAX];
//char adu5aDevName[FILENAME_MAX];

// File desciptors for GPS serial ports
int fdG12,fdAdu5A;//,fdMag
int printToScreen=0;

// Config stuff for G12
float g12PPSPeriod=1;
float g12PPSOffset=0.05;
int g12PPSRisingOrFalling=1; //1 is R, 2 is F
int g12DataPort=1;
int g12NTPPort=2;
int g12ZDAPeriod=5;
int g12POSPeriod=10;
int g12VTGPeriod=10;
int g12SATPeriod=60;
int g12UpdateClock=0;
int g12ClockSkew=0;


// Config stuff for ADU5
int adu5SatPeriod=3600;
int adu5PatPeriod=10;
float adu5RelV12[3]={0};
float adu5RelV13[3]={0};
float adu5RelV14[3]={0};

//Output stuff
int useUSBDisks=0;
char mainDataDisk[FILENAME_MAX];
char usbDataDiskLink[FILENAME_MAX];
char baseHouseTelemDir[FILENAME_MAX];
char baseHouseArchiveDir[FILENAME_MAX];
char gpsdG12LogDir[FILENAME_MAX];
char gpsTelemDir[FILENAME_MAX];
char gpsTelemLinkDir[FILENAME_MAX];
char gpsSubTimeDir[FILENAME_MAX];
char gpsSubTimeLinkDir[FILENAME_MAX];

char gpsAdu5TTTArchiveDir[FILENAME_MAX];
char gpsAdu5PATArchiveDir[FILENAME_MAX];
char gpsAdu5SATArchiveDir[FILENAME_MAX];
char gpsG12POSArchiveDir[FILENAME_MAX];
char gpsG12VTGArchiveDir[FILENAME_MAX];
char gpsAdu5TTTUSBArchiveDir[FILENAME_MAX];
char gpsAdu5PATUSBArchiveDir[FILENAME_MAX];
char gpsAdu5SATUSBArchiveDir[FILENAME_MAX];
char gpsG12POSUSBArchiveDir[FILENAME_MAX];
char gpsG12VTGUSBArchiveDir[FILENAME_MAX];


int main (int argc, char *argv[])
{
    int retVal;

// GPSd config stuff
    char gpsdPidFile[FILENAME_MAX];

    char *tempString;
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
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
   
    /* Load Global Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status &= configLoad (GLOBAL_CONF_FILE,"whiteheat") ;
    eString = configErrorString (status) ;

    /* Get Device Names and config stuff */
    if (status == CONFIG_E_OK) {
	useUSBDisks=kvpGetInt("useUSBDisks",0);
	tempString=kvpGetString("gpsdPidFile");
	if(tempString) {
	    strncpy(gpsdPidFile,tempString,FILENAME_MAX);
	    writePidFile(gpsdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdPidFile");
	    fprintf(stderr,"Couldn't get gpsdPidFile\n");
	}
	tempString=kvpGetString("g12ADevName");
	if(tempString) {
	    strncpy(g12ADevName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get g12ADevName");
	    fprintf(stderr,"Couldn't get g12ADevName\n");
	}
//	tempString=kvpGetString("g12BDevName");
//	if(tempString) {
//	    strncpy(g12BDevName,tempString,FILENAME_MAX);
//	}
//	else {
//	    syslog(LOG_ERR,"Couldn't get g12BDevName");
//	    fprintf(stderr,"Couldn't get g12BDevName\n");
//	}
	tempString=kvpGetString("adu5ADevName");
	if(tempString) {
	    strncpy(adu5ADevName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get adu5ADevName");
	    fprintf(stderr,"Couldn't get adu5ADevName\n");
	}
/* 	tempString=kvpGetString("adu5BDevName"); */
/* 	if(tempString) { */
/* 	    strncpy(adu5BDevName,tempString,FILENAME_MAX); */
/* 	} */
/* 	else { */
/* 	    syslog(LOG_ERR,"Couldn't get adu5BDevName"); */
/* 	    fprintf(stderr,"Couldn't get adu5BDevName\n"); */
/* 	} */
///////////////////////////////////////
	
	//Disk locations
	tempString=kvpGetString("mainDataDisk");
	if(tempString) {
	    strncpy(mainDataDisk,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get mainDataDisk");
	    fprintf(stderr,"Couldn't get mainDataDisk\n");
	}	
	tempString=kvpGetString("usbDataDiskLink");
	if(tempString) {
	    strncpy(usbDataDiskLink,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbDataDiskLink");
	    fprintf(stderr,"Couldn't get usbDataDiskLink\n");
	}	

	//Data locations
	tempString=kvpGetString("baseHouseTelemDir");
	if(tempString) {
	    strncpy(baseHouseTelemDir,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get baseHouseTelemDir");
	    fprintf(stderr,"Couldn't get baseHouseTelemDir\n");
	}	
	tempString=kvpGetString("baseHouseArchiveDir");
	if(tempString) {
	    strncpy(baseHouseArchiveDir,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get baseHouseArchiveDir");
	    fprintf(stderr,"Couldn't get baseHouseArchiveDir\n");
	}

	tempString=kvpGetString("gpsTelemSubDir");
	if(tempString) {
	    sprintf(gpsTelemDir,"%s/%s",baseHouseTelemDir,tempString);
	    sprintf(gpsTelemLinkDir,"%s/%s/link",baseHouseTelemDir,tempString);
	    makeDirectories(gpsTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsTelemDir");
	    fprintf(stderr,"Couldn't get gpsTelemDir\n");
	}
	tempString=kvpGetString("gpsSubTimeDir");
	if(tempString) {
	    strncpy(gpsSubTimeDir,tempString,FILENAME_MAX);
	    strncpy(gpsSubTimeLinkDir,tempString,FILENAME_MAX);
	    strcat(gpsSubTimeLinkDir,"/link");
	    makeDirectories(gpsSubTimeLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsSubTimeDir");
	    fprintf(stderr,"Couldn't get gpsSubTimeDir\n");
	}
	tempString=kvpGetString("gpsArchiveSubDir");
	if(tempString) {
	    sprintf(gpsAdu5PATArchiveDir,"%s/%s/%s/adu5/pat",
		    mainDataDisk,baseHouseArchiveDir,tempString);
	    sprintf(gpsAdu5SATArchiveDir,"%s/%s/%s/adu5/sat",
		    mainDataDisk,baseHouseArchiveDir,tempString);
	    sprintf(gpsAdu5TTTArchiveDir,"%s/%s/%s/adu5/ttt",
		    mainDataDisk,baseHouseArchiveDir,tempString);
	    sprintf(gpsG12POSArchiveDir,"%s/%s/%s/g12/pos",
		    mainDataDisk,baseHouseArchiveDir,tempString);
	    sprintf(gpsG12VTGArchiveDir,"%s/%s/%s/g12/vtg",
		    mainDataDisk,baseHouseArchiveDir,tempString);

	    makeDirectories(gpsAdu5PATArchiveDir);
	    makeDirectories(gpsAdu5SATArchiveDir);
	    makeDirectories(gpsAdu5TTTArchiveDir);
	    makeDirectories(gpsG12POSArchiveDir);
	    makeDirectories(gpsG12VTGArchiveDir);

	    if(useUSBDisks) {
		sprintf(gpsAdu5PATUSBArchiveDir,"%s/%s/%s/adu5/pat/link",
			usbDataDiskLink,baseHouseArchiveDir,tempString);
		sprintf(gpsAdu5SATUSBArchiveDir,"%s/%s/%s/adu5/sat/link",
		    usbDataDiskLink,baseHouseArchiveDir,tempString);
		sprintf(gpsAdu5TTTUSBArchiveDir,"%s/%s/%s/adu5/ttt/link",
			usbDataDiskLink,baseHouseArchiveDir,tempString);
		sprintf(gpsG12POSUSBArchiveDir,"%s/%s/%s/g12/pos/link",
			usbDataDiskLink,baseHouseArchiveDir,tempString);
		sprintf(gpsG12VTGUSBArchiveDir,"%s/%s/%s/g12/vtg/link",
			usbDataDiskLink,baseHouseArchiveDir,tempString);
		
		
		makeDirectories(gpsAdu5PATUSBArchiveDir);
		makeDirectories(gpsAdu5SATUSBArchiveDir);
		makeDirectories(gpsAdu5TTTUSBArchiveDir);
		makeDirectories(gpsG12POSUSBArchiveDir);
		makeDirectories(gpsG12VTGUSBArchiveDir);
	    }

	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsArchiveSubDir");
	    fprintf(stderr,"Couldn't get gpsArchiveSubDir\n");
	}

    }
    
    retVal=readConfigFile();
    if(retVal<0) {
	syslog(LOG_ERR,"Problem reading GPSd.config");
	printf("Problem reading GPSd.config\n");
	exit(1);
    }
    retVal=openDevices();
    

    //Main event loop
    do {
	if(printToScreen) printf("Initalizing GPSd\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading GPSd.config");
	    printf("Problem reading GPSd.config\n");
	    exit(1);
	}
	if(printToScreen)  
	    printf("Device fd's: %d %d\nDevices:\t%s\t%s\n",fdG12,fdAdu5A,g12ADevName,adu5ADevName);
	retVal=setupG12();
	retVal=setupADU5();

	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    checkG12();
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
//    int tempNum=3;;
//    KvpErrorCode kvpStatus=0;
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
	g12DataPort=kvpGetInt("dataPort",1); //1 is 'A', 2 is 'B'
	g12NTPPort=kvpGetInt("ntpPort",2); //1 is 'A', 2 is 'B'
	g12ZDAPeriod=kvpGetInt("zdaPeriod",5); // in seconds
	g12POSPeriod=kvpGetInt("posPeriod",5); // in seconds
	g12VTGPeriod=kvpGetInt("vtgPeriod",5); // in seconds
	g12SATPeriod=kvpGetInt("satPeriod",0); // in seconds
	g12UpdateClock=kvpGetInt("updateClock",0); // 1 is yes, 0 is no
	g12ClockSkew=kvpGetInt("clockSkew",0); // Time difference in seconds
	adu5SatPeriod=kvpGetInt("satPeriod",0);
	adu5PatPeriod=kvpGetInt("patPeriod",0);
	adu5RelV12[0]=kvpGetFloat("calibV12_1",0);
	adu5RelV12[1]=kvpGetFloat("calibV12_2",0);
	adu5RelV12[2]=kvpGetFloat("calibV12_3",0);
	adu5RelV13[0]=kvpGetFloat("calibV13_1",0);
	adu5RelV13[1]=kvpGetFloat("calibV13_2",0);
	adu5RelV13[2]=kvpGetFloat("calibV13_3",0);
	adu5RelV14[0]=kvpGetFloat("calibV14_1",0);
	adu5RelV14[1]=kvpGetFloat("calibV14_2",0);
	adu5RelV14[2]=kvpGetFloat("calibV14_3",0);
//	printf("v12 %f %f %f\n",adu5RelV12[0],adu5RelV12[1],adu5RelV12[2]);
//	printf("v13 %f %f %f\n",adu5RelV13[0],adu5RelV13[1],adu5RelV13[2]);
//	printf("v14 %f %f %f\n",adu5RelV14[0],adu5RelV14[1],adu5RelV14[2]);
    }
   else {
       eString=configErrorString (status) ;
       syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
   }

//    printf("%f %f %f\n",adu5RelV12[0],adu5RelV12[1],adu5RelV12[2]);
//    printf("%f %f %f\n",adu5RelV13[0],adu5RelV13[1],adu5RelV13[2]);
//    printf("%f %f %f\n",adu5RelV14[0],adu5RelV14[1],adu5RelV14[2]);

//    printf("%d %s %s %s\n",printToScreen,g12ADevName,adu5ADevName,adu5ADevName);
   return status;
}

int openDevices()
/*!
  Open connections to the GPS devices
*/
{
    int retVal;
// Initialize the various devices    
    retVal=openGPSDevice(g12ADevName);
    if(retVal<=0) {
	syslog(LOG_ERR,"Couldn't open: %s\n",g12ADevName);
	if(printToScreen) printf("Couldn't open: %s\n",g12ADevName);
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

//    printf("%s %s %s\n",g12ADevName,adu5ADevName,adu5ADevName);
    return 0;
}


int setupG12()
/*! Initializes the G12 with the correct PPS and ZDA settings */
{
    char portNames[2]={'A','B'};
    char edgeNames[2]={'R','F'};
    char g12Command[256]="";
    char tempCommand[128]="";
    char dataPort='A';
    char ntpPort='B';
    char ppsEdge='R';
    int retVal;

    if(g12NTPPort==1 || g12NTPPort==2) ntpPort=portNames[g12NTPPort-1];
    if(g12DataPort==1 || g12DataPort==2) dataPort=portNames[g12DataPort-1];
    if(g12PPSRisingOrFalling==1 || g12PPSRisingOrFalling==2)
	ppsEdge=edgeNames[g12PPSRisingOrFalling-1];

    strcat(g12Command,"$PASHS,ELM,0\n");
    sprintf(tempCommand,"$PASHS,NME,ALL,%c,OFF\n",dataPort);
    strcat(g12Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,ALL,%c,OFF\n",ntpPort);
    strcat(g12Command,tempCommand);
    strcat(g12Command, "$PASHS,LTZ,0,0\n");
    strcat(g12Command, "$PASHS,UTS,ON\n");
    sprintf(tempCommand,"$PASHS,NME,ZDA,%c,ON,%d\n",dataPort,g12ZDAPeriod);    
    strcat(g12Command,tempCommand); 
    sprintf(tempCommand,"$PASHS,NME,POS,%c,ON,%d\n",dataPort,g12POSPeriod);    
    strcat(g12Command,tempCommand); 
    sprintf(tempCommand,"$PASHS,NME,VTG,%c,ON,%d\n",dataPort,g12VTGPeriod);    
    strcat(g12Command,tempCommand); 
    sprintf(tempCommand,"$PASHS,NME,SAT,%c,ON,%d\n",dataPort,g12SATPeriod);    
    strcat(g12Command,tempCommand); 
 
    sprintf(tempCommand,"$PASHS,SPD,%c,4\n",ntpPort);    
    strcat(g12Command,tempCommand);   
    sprintf(tempCommand,"$PASHS,NME,RMC,%c,ON,1\n",ntpPort);    
    strcat(g12Command,tempCommand);     
    sprintf(tempCommand, "$PASHS,PPS,%2.2f,%d,%c\n",g12PPSPeriod,
	    (int)g12PPSOffset,ppsEdge);   
    strcat(g12Command,tempCommand); 
    
    if(printToScreen) 
	fprintf(stderr,"G12:\n%s\n",g12Command);
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
    static int lastStar=-10;
    retVal=isThereDataNow(fdG12);
//    usleep(1);
//    printf("Check G12 got retVal %d\n",retVal);
    if(retVal!=1) return 0;
    retVal=read(fdG12, tempData, G12_DATA_SIZE);

    if(retVal>0) {
	for(i=0; i < retVal; i++) {
	    if(tempData[i]=='*') {
		lastStar=g12OutputLength;
	    }
//	    printf("%c %d %d\n",tempData[i],g12OutputLength,lastStar);
	    g12Output[g12OutputLength++]=tempData[i];

	    if(g12OutputLength==lastStar+3) {
		if(g12OutputLength) {
		    if((retVal-i)<50)
			processG12Output(g12Output,g12OutputLength,1);
		    else
			processG12Output(g12Output,g12OutputLength,0);
		       
		}
		g12OutputLength=0;
		lastStar=-10;
	    }
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
    static int lastStar=-10;
    retVal=isThereDataNow(fdAdu5A);
    usleep(5);
//    printf("Check ADU5 got retVal %d\n",retVal);
    if(retVal!=1) return 0;
    retVal=read(fdAdu5A, tempData, ADU5_DATA_SIZE);
    if(retVal>0) {
	for(i=0; i < retVal; i++) {
	    if(tempData[i]=='*') {
		lastStar=adu5OutputLength;
	    }
//	    printf("%c %d %d\n",tempData[i],adu5OutputLength,lastStar);
	    adu5Output[adu5OutputLength++]=tempData[i];

	    if(adu5OutputLength==lastStar+3) {
		if(adu5OutputLength) {
		    processADU5Output(adu5Output,adu5OutputLength);
		}
		adu5OutputLength=0;
		lastStar=-10;
	    }
	    tempData[i]=0;
	}
	
    }
    else if(retVal<0) {
	syslog(LOG_ERR,"ADU5 read error: %s",strerror(errno));
    }
    return retVal;
}


void processG12Output(char *tempBuffer, int length,int setClock)
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
    char unixString[128],otherString[128];
//    char checksum[3];   
//    strncpy(gpsString,tempBuffer,length);
    
//    printf("%s\n",tempBuffer);
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
    if(printToScreen) printf("G12:\t%s\t%d\n",gpsString,gpsLength);

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
	strncpy(otherString,ctime(&gpsRawTime),179);	    
	strncpy(unixString,ctime(&rawtime),179);
//	printf("%s%s\n",unixString,otherString);
	if(abs(gpsRawTime-rawtime)>g12ClockSkew && g12UpdateClock &&setClock) {
	    updateClockFromG12(gpsRawTime);
	}
//	writeG12TimeFile(rawtime,gpsRawTime,gpsCopy);
	    
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
    char *subString; 
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

    strncpy(gpsCopy,gpsString,gpsLength);
    if(printToScreen) printf("ADU5:\t%s\t%d\n",gpsString,gpsLength);
    count=0;
    subString = strtok (gpsCopy,",");
    if(!strcmp(subString,"$GPPAT")) {
//	printf("Got $GPPAT\n");
	processGPPATString(gpsString,gpsLength);
    }
    else if(!strcmp(subString,"$PASHR")) {
	//Maybe have TTT
	subString = strtok (NULL, " ,.*");
	if(!strcmp(subString,"TTT")) {
//	    printf("Got %s\t",subString);
//	    printf("Got TTT\n");
	    processTTTString(gpsString,gpsLength);
	}
	if(!strcmp(subString,"SAT")) {
//	    printf("And got SAT\n");
	    processSATString(gpsString,gpsLength);
	}
	
    }


}

void processTTTString(char *gpsString, int gpsLength) {
    char gpsCopy[ADU5_DATA_SIZE];//="$PASHR,TTT,4,03:32:45.0111305*0E";   
    char *subString;
    char filename[FILENAME_MAX];
//    int count=0;
    strncpy(gpsCopy,gpsString,gpsLength);
    GpsSubTime_t theTTT;
    int day,hour,min,sec,subSecond,retVal;
    time_t rawtime,gpstime;  
//    char unixString[128],otherString[128];
    struct tm * timeinfo;
    time ( &rawtime );
    
    subString = strtok (gpsCopy,"*");
    sscanf(subString,"$PASHR,TTT,%d,%02d:%02d:%02d.%d",&day,&hour,&min,
	   &sec,&subSecond);
//    printf("%s\n%d\t%d\t%d\t%d\t%d\n",subString,day,hour,min,sec,subSecond);
//    .0111305;
    timeinfo = localtime ( &rawtime );
//    printf("Days: %d %d\n",timeinfo->tm_wday,day);
    if(timeinfo->tm_wday+1!=day) {
//	addDay(timeinfo);
    }
    timeinfo->tm_hour=hour;
    timeinfo->tm_min=min;	    
    timeinfo->tm_sec=sec;
    gpstime=mktime(timeinfo);
    gpstime-=LEAP_SECONDS;
//    strncpy(unixString,ctime(&rawtime),127);
//    strncpy(otherString,ctime(&gpstime),127);
//    printf("%ld %ld\n",rawtime,gpstime);
//    printf("Seconds: %d %d\n",oldSec,sec);
//    printf("***************************\n%s%s**********************\n",unixString,otherString);
    theTTT.unixTime=gpstime;
    theTTT.subTime=subSecond;
    
    //Write file for eventd
    sprintf(filename,"%s/gps_%ld_%d.dat",gpsSubTimeDir,theTTT.unixTime,theTTT.subTime);
    writeGPSTTT(&theTTT,filename);
    retVal=makeLink(filename,gpsSubTimeLinkDir);  

    //Write file to main disk
    sprintf(filename,"%s/gps_%ld_%d.dat",gpsAdu5TTTArchiveDir,theTTT.unixTime,theTTT.subTime);
    writeGPSTTT(&theTTT,filename);
    
    if(useUSBDisks) {
	//Write file to usb disk
	sprintf(filename,"%s/gps_%ld_%d.dat",gpsAdu5TTTUSBArchiveDir,theTTT.unixTime,theTTT.subTime);
	writeGPSTTT(&theTTT,filename);
    }
    
    

}



void processSATString(char *gpsString, int gpsLength) {
    char gpsCopy[ADU5_DATA_SIZE];
    char *subString;
    char theFilename[FILENAME_MAX];
    int retVal;
    int count=0;
    int doingSat=0;
    strncpy(gpsCopy,&gpsString[11],gpsLength-11);
//    printf("%s\n",gpsCopy);
    GpsSatStruct_t theSat;
    theSat.gHdr.code=PACKET_GPSD_SAT;
    
    time_t rawtime;
    time ( &rawtime );
    theSat.unixTime=rawtime;


    subString = strtok (gpsCopy,",");    
    while (subString != NULL) {
//	printf("%d\t%s\n",count,subString);
	
	switch(count) {
	    case 0:		
		theSat.numSats=atoi(subString);
		break;
	    case 1:
		theSat.sat[doingSat].prn=atoi(subString);
		break;
	    case 2:
		theSat.sat[doingSat].azimuth=atoi(subString);
		break;
	    case 3:
		theSat.sat[doingSat].elevation=atoi(subString);
		break;
	    case 4:
		theSat.sat[doingSat].snr=atoi(subString);
		break;
	    case 5:
		if(subString[0]=='U') theSat.sat[doingSat].flag=1;
		if(subString[0]=='-') theSat.sat[doingSat].flag=0;
		doingSat++;
		count=0;
		break;
	    default:
		break;
	}
	if(subString[1]=='*') break;
	count++;
	subString = strtok (NULL, ",");	
    }

/*     for(count=0;count<theSat.numSats;count++) { */
/* 	printf("Number:\t%d\nPRN:\t%d\nAzimuth:\t%d\nElevation:\t%d\nSNR:\t%d\nFlag:\t%d\n\n",count+1,theSat.sat[count].prn,theSat.sat[count].azimuth,theSat.sat[count].elevation,theSat.sat[count].snr,theSat.sat[count].flag); */
/*     } */


    //Write file and link for sipd
    sprintf(theFilename,"%s/sat_%ld.dat",gpsTelemDir,theSat.unixTime);
    retVal=writeGPSSat(&theSat,theFilename);  
    retVal=makeLink(theFilename,gpsTelemLinkDir);  

    //Write file to main disk
    sprintf(theFilename,"%s/sat_%ld.dat",gpsAdu5SATArchiveDir,theSat.unixTime);
    retVal=writeGPSSat(&theSat,theFilename);  
    
    if(useUSBDisks) {
	//Write file to usb disk
	sprintf(theFilename,"%s/sat_%ld.dat",
		gpsAdu5SATUSBArchiveDir,theSat.unixTime);
	retVal=writeGPSSat(&theSat,theFilename); 

    }


}



void processGPPATString(char *gpsString, int gpsLength) {
    char gpsCopy[ADU5_DATA_SIZE];//="$GPPAT,021140.00,3338.51561,N,11750.82575,W,-00288.42,666.0000,000.00,000.00,666.0000,666.0000,1*4F";    
    char *subString;
    int count=0;
//    char gpsTest[ADU5_DATA_SIZE];
//    sprintf(gpsTest,"$GPPAT,021140.00,3338.51561,N,11750.82575,W,-00288.42,666.0000,000.00,000.00,666.0000,666.0000,1*4F");

//    sprintf(gpsCopy,"%s",gpsTest);
    strncpy(gpsCopy,gpsString,gpsLength);

    GpsPatStruct_t thePat;
    thePat.gHdr.code=PACKET_GPSD_PAT;
    time_t rawtime;
    int hour,min,sec,msec,deg;//,subMin;
    float fmin;
    int noLock=0;
    time ( &rawtime );
    count=0;
//    printf("Here: %s\n",gpsCopy);
    subString = strtok (gpsCopy,",");    
    thePat.unixTime=rawtime;
    while (subString != NULL)
    {	
//	printf("Test %d %s\n",count,subString);
	switch(count) {
	    case 1:
		sscanf(subString,"%02d%02d%02d.%d",&hour,&min,&sec,&msec);
		thePat.timeOfDay=(float)msec;
		thePat.timeOfDay+=(float)(1000*sec);
		thePat.timeOfDay+=(float)(1000*60*min);
		thePat.timeOfDay+=(float)(1000*60*60*hour);
		break;
	    case 2:
		if(strlen(subString)==2) {
		    noLock=1;
		    break;
		}
//		sscanf(subString,"%02d%02d.%d",&deg,&min,&subMin);
//		fmin=((float)min)+((float)subMin/100000.);
//		printf("%d %d %d\t%f\n",deg,min,subMin,fmin);
//		thePat.latitude+=((float)min)/60.0;
//		thePat.latitude+=((float)subMin)/(60.0*10000.0);
//		printf("%s\t%f\n",subString,thePat.latitude);
//		printf("Length %d\n",strlen(subString));

		sscanf(subString,"%02d%f",&deg,&fmin);
		thePat.latitude=(float)deg;		
		thePat.latitude+=(fmin/60.);
		break;
	    case 3: 
		if(subString[0]=='S') thePat.latitude*=-1;
		break;
	    case 4:
//		sscanf(subString,"%03d%02d.%d",&deg,&min,&subMin);
		sscanf(subString,"%03d%f",&deg,&fmin);
		thePat.longitude=(float)deg;
//		thePat.longitude+=((float)min)/60.0;
//		thePat.longitude+=((float)subMin)/(60.0*10000.0);
		thePat.longitude+=(fmin/60.);
		break;
	    case 5:
		if(subString[0]=='W') thePat.longitude*=-1;
		break;
	    case 6:
		sscanf(subString,"%lf",&thePat.altitude);
		break;
	    case 7:
		sscanf(subString,"%lf",&thePat.heading);
		break;
	    case 8:
		sscanf(subString,"%lf",&thePat.pitch);
		break;
	    case 9:
		sscanf(subString,"%lf",&thePat.roll);
		break;
	    case 10:
		sscanf(subString,"%lf",&thePat.mrms);
		break;
	    case 11:
		sscanf(subString,"%lf",&thePat.brms);
		break;
	    case 12:
		if(subString[0]=='0') thePat.attFlag=0;
		if(subString[0]=='1') thePat.attFlag=1;
		
		break;		
	    default:
		break;

	}
	if(noLock) {
//	    printf("No lock\n");
	    thePat.heading=0;
	    thePat.pitch=0;
	    thePat.roll=0;
	    thePat.brms=0;
	    thePat.mrms=0;
	    thePat.attFlag=0;
	    thePat.latitude=0;
	    thePat.longitude=0;
	    thePat.altitude=0;
	    break;
	}			    
	count++;	
	subString = strtok (NULL, " ,*");
    }
/*     printf("%s\n",gpsTest); */
/*     printf("unixTime: %ld\ntimeOfDay: %ld\nheading: %lf\npitch: %lf\nroll: %lf\nmrms: %lf\nbrms: %lf\nattFlag: %d\nlatitude: %lf\nlongitude: %lf\naltitute: %lf\n", */
/* 	   thePat.unixTime, */
/* 	   thePat.timeOfDay, */
/* 	   thePat.heading, */
/* 	   thePat.pitch, */
/* 	   thePat.roll, */
/* 	   thePat.mrms, */
/* 	   thePat.brms, */
/* 	   thePat.attFlag, */
/* 	   thePat.latitude, */
/* 	   thePat.longitude, */
/* 	   thePat.altitude); */
/*     exit(0); */
    
    char theFilename[FILENAME_MAX];
    int retVal;

    //Write file and link for sipd
    sprintf(theFilename,"%s/pat_%ld.dat",gpsTelemDir,thePat.unixTime);
    retVal=writeGPSPat(&thePat,theFilename);  
    retVal=makeLink(theFilename,gpsTelemLinkDir);  

    //Write file to main disk
    sprintf(theFilename,"%s/pat_%ld.dat",gpsAdu5PATArchiveDir,thePat.unixTime);
    retVal=writeGPSPat(&thePat,theFilename);  

    if(useUSBDisks) {
	sprintf(theFilename,"%s/pat_%ld.dat",
		gpsAdu5PATUSBArchiveDir,thePat.unixTime);
	retVal=writeGPSPat(&thePat,theFilename);  
    }


    
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




int setupADU5()
/*! Initializes the ADU5 with the correct settings */
{
    char adu5Command[1024]="";
    char tempCommand[128]="";
    int retVal;

    strcat(adu5Command,"$PASHS,ELM,0\r\n"); 
    strcat(adu5Command,"$PASHS,KFP,ON\r\n");
    
    sprintf(tempCommand,"$PASHS,3DF,V12,%2.3f,%2.3f,%2.3f\r\n",
	    adu5RelV12[0],adu5RelV12[1],adu5RelV12[2]);
    strcat(adu5Command,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,V13,%2.3f,%2.3f,%2.3f\r\n",
	    adu5RelV13[0],adu5RelV13[1],adu5RelV13[2]);
    strcat(adu5Command,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,V14,%2.3f,%2.3f,%2.3f\r\n",
	    adu5RelV14[0],adu5RelV14[1],adu5RelV14[2]);
    strcat(adu5Command,tempCommand);
    strcat(adu5Command,"$PASHS,NME,ALL,A,OFF\r\n");
    strcat(adu5Command,"$PASHS,NME,ALL,B,OFF\r\n");
    strcat(adu5Command,"$PASHS,3DF,ANG,10\r\n");
    strcat(adu5Command,"$PASHS,3DF,FLT,Y\r\n");
    sprintf(tempCommand,"$PASHS,NME,PAT,A,ON,%d",adu5PatPeriod);
    strcat(tempCommand,"\r\n");
    strcat(adu5Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,SAT,A,ON,%d\r\n",adu5SatPeriod);
    strcat(adu5Command,tempCommand);
    strcat(adu5Command,"$PASHQ,PRT\r\n");
    strcat(adu5Command,"$PASHS,NME,TTT,A,ON\r\n");

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
    int result=0;
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


/*     printf("%s %x %x\n",checksum,theCheckSum,otherSum); */
//    printf("\n\n\n%d %x\n",theCheckSum,theCheckSum);
    
    result=(theCheckSum==otherSum);
/*     if(!result) { */
/* 	char checksum[3]; */
/* 	checksum[0]=gpsString[gpsLength-2]; */
/* 	checksum[1]=gpsString[gpsLength-1]; */
/* 	checksum[2]='\0'; */
/* 	printf("\n\nBugger:\t%s %x %x\n\n%s",checksum,theCheckSum,otherSum,gpsString); */
/*     } */
    return result;
}
