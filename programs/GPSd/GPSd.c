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
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "serialLib/serialLib.h"
#include "utilLib/utilLib.h"
#include "socketLib/socketLib.h"
#include "includes/anitaStructures.h"

// Forward declarations
int readConfigFile();
int openDevices();
int setupG12();
int setupAdu5();
int checkG12();
int checkAdu5A();
void processG12Output(char *tempBuffer, int length,int latestData);
void processAdu5Output(char *tempBuffer, int length);
int updateClockFromG12(time_t gpsRawTime);
void prepWriterStructs();
//int breakdownG12TimeString(char *subString,int *hour,int *minute,int *second);
//int writeG12TimeFile(time_t compTime, time_t gpsTime, char *g12Output);
int checkChecksum(char *gpsString,int gpsLength);
void processGppatString(char *gpsString, int length);
void processTttString(char *gpsString, int gpsLength,int fromAdu5);
void processG12SatString(char *gpsString, int gpsLength);
void processGpzdaString(char *gpsString, int gpsLength, int latestData);
void processGpvtgString(char *gpsString, int gpsLength);
void processPosString(char *gpsString, int gpsLength);
void processAdu5Sa4String(char *gpsString, int gpsLength);
int tryToStartNtpd();
void handleBadSigs(int sig);

// Definitions
#define LEAP_SECONDS 14 //Need to check
#define G12_DATA_SIZE 1024
#define ADU5_DATA_SIZE 1024


// File desciptors for GPS serial ports
int fdG12,fdAdu5A;//,fdMag

//Output config stuff
int printToScreen=0;
int verbosity=0;

// Config stuff for G12
float g12PpsPeriod=1;
float g12PpsOffset=50;
int g12PpsRisingOrFalling=1; //1 is R, 2 is F
int g12DataPort=1;
int g12NtpPort=2;
int g12ZdaPeriod=5;
float g12PosPeriod=10;
int g12SatPeriod=600;
int g12StartNtp=0;
int g12UpdateClock=0;
int g12ClockSkew=0;
int g12EnableTtt=0;
int g12TttEpochRate=20;

// Config stuff for ADU5
int adu5EnableTtt=0;
int adu5SatPeriod=600;
float adu5PatPeriod=10;
float adu5VtgPeriod=10;
float adu5RelV12[3]={0};
float adu5RelV13[3]={0};
float adu5RelV14[3]={0};

//Output stuff
int hkDiskBitMask;

AnitaHkWriterStruct_t adu5PatWriter;
AnitaHkWriterStruct_t adu5SatWriter;
AnitaHkWriterStruct_t adu5TttWriter;
AnitaHkWriterStruct_t adu5VtgWriter;
AnitaHkWriterStruct_t g12PosWriter;
AnitaHkWriterStruct_t g12SatWriter;

int startedNtpd=0;

// GPSd config stuff
char gpsdPidFile[FILENAME_MAX];


int main (int argc, char *argv[])
{
    int retVal;


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
    signal(SIGINT, handleBadSigs);
    signal(SIGTERM, handleBadSigs);
    signal(SIGSEGV, handleBadSigs);
    
    
    //Dont' wait for children
    signal(SIGCLD, SIG_IGN); 

   
    /* Load Global Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    /* Get Device Names and config stuff */
    if (status == CONFIG_E_OK) {
	hkDiskBitMask=kvpGetInt("hkDiskBitMask",0);
	tempString=kvpGetString("gpsdPidFile");
	if(tempString) {
	    strncpy(gpsdPidFile,tempString,FILENAME_MAX);
	    retVal=checkPidFile(gpsdPidFile);
	    if(retVal) {
		fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,gpsdPidFile);
		syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
		return -1;
	    }
	    writePidFile(gpsdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdPidFile");
	    fprintf(stderr,"Couldn't get gpsdPidFile\n");
	}

    }
    makeDirectories(GPSD_SUBTIME_LINK_DIR);
    makeDirectories(ADU5_PAT_TELEM_LINK_DIR);
    makeDirectories(ADU5_VTG_TELEM_LINK_DIR);
    makeDirectories(ADU5_SAT_TELEM_LINK_DIR);
    makeDirectories(G12_POS_TELEM_LINK_DIR);
    makeDirectories(G12_SAT_TELEM_LINK_DIR);
    
    printf("hkDiskBitMask %d\n",hkDiskBitMask);

    retVal=readConfigFile();
    if(retVal<0) {
	syslog(LOG_ERR,"Problem reading GPSd.config");
	printf("Problem reading GPSd.config\n");
	handleBadSigs(1000);
    }
    retVal=openDevices();
    prepWriterStructs();

    //Main event loop
    do {
	if(printToScreen) printf("Initalizing GPSd\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading GPSd.config");
	    printf("Problem reading GPSd.config\n");
	    handleBadSigs(1000);
	}
	if(printToScreen)  
	    printf("Device fd's: %d %d\nDevices:\t%s\t%s\n",fdG12,fdAdu5A,G12A_DEV_NAME,ADU5A_DEV_NAME);
	retVal=setupG12();
	retVal=setupAdu5();
	
//	printf("Here\n");
	currentState=PROG_STATE_RUN;
//	currentState=PROG_STATE_TERMINATE;
	unsigned long loopCounter=0;
	while(currentState==PROG_STATE_RUN) {
	    checkG12();
	    checkAdu5A();
	    if(!startedNtpd && g12StartNtp && loopCounter>60) startedNtpd=tryToStartNtpd();
	    loopCounter++;
	    usleep(1);
	}
	printf("currentState == %d\n",currentState);

    } while(currentState==PROG_STATE_INIT);

    if(printToScreen) printf("Terminating GPSd\n");
    if(fdG12) close(fdG12);
    if(fdAdu5A) close(fdAdu5A);

    closeHkFilesAndTidy(&adu5PatWriter);
    closeHkFilesAndTidy(&adu5SatWriter);
    closeHkFilesAndTidy(&adu5TttWriter);
    closeHkFilesAndTidy(&adu5VtgWriter);
    closeHkFilesAndTidy(&g12PosWriter);
    closeHkFilesAndTidy(&g12SatWriter);
    unlink(gpsdPidFile);

    syslog(LOG_INFO,"GPSd terminating");
//    removeFile(gpsdPidFile);
    
	
    return 0;
}


int readConfigFile() 
/* Load GPSd config stuff */
{
    /* Config file thingies */
    int status=0;
    char* eString ;
    kvpReset();
    status = configLoad ("GPSd.config","output") ;
    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",-1);
	verbosity=kvpGetInt("verbosity",-1);
	if(printToScreen<0) {
	    syslog(LOG_WARNING,
		   "Couldn't fetch printToScreen, defaulting to zero");
	    printToScreen=0;	    
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
    }
    kvpReset();
    status = configLoad ("GPSd.config","g12");
    if(status == CONFIG_E_OK) {
	g12EnableTtt=kvpGetInt("enableTtt",1);
	g12TttEpochRate=kvpGetInt("tttEpochRate",20);
	if(g12TttEpochRate!=20 && g12TttEpochRate!=10 &&
	   g12TttEpochRate!=5 && g12TttEpochRate!=2 && g12TttEpochRate!=1) {
	    syslog(LOG_WARNING,"Incorrect TTT Epoch Rate %d, reverting to 20Hz",
		   g12TttEpochRate);
	    g12TttEpochRate=20;
	}
	g12PpsPeriod=kvpGetFloat("ppsPeriod",1); //in seconds
	g12PpsOffset=kvpGetFloat("ppsOffset",50); //in milliseconds 
	g12PpsRisingOrFalling=kvpGetInt("ppsRorF",1); //1 is 'R', 2 is 'F'
	g12DataPort=kvpGetInt("dataPort",1); //1 is 'A', 2 is 'B'
	g12NtpPort=kvpGetInt("ntpPort",2); //1 is 'A', 2 is 'B'
	g12ZdaPeriod=kvpGetInt("zdaPeriod",10); // in seconds
	g12PosPeriod=kvpGetFloat("posPeriod",10); // in seconds
	g12SatPeriod=kvpGetInt("satPeriod",600); // in seconds
	g12UpdateClock=kvpGetInt("updateClock",0); // 1 is yes, 0 is no
	g12StartNtp=kvpGetInt("startNtp",0); // 1 is yes, 0 is no
	g12ClockSkew=kvpGetInt("clockSkew",0); // Time difference in seconds
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
    }
    kvpReset();
    status = configLoad ("GPSd.config","adu5") ;    
    if(status == CONFIG_E_OK) {
	adu5EnableTtt=kvpGetInt("enableTtt",1);
	adu5SatPeriod=kvpGetInt("satPeriod",600); // in seconds
	adu5PatPeriod=kvpGetFloat("patPeriod",10); // in seconds
	adu5VtgPeriod=kvpGetFloat("vtgPeriod",10); // in seconds
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
    return status;
}

int openDevices()
/*!
  Open connections to the GPS devices
*/
{
    int retVal;
// Initialize the various devices    
    retVal=openGpsDevice(G12A_DEV_NAME);
    if(retVal<=0) {
	syslog(LOG_ERR,"Couldn't open: %s\n",G12A_DEV_NAME);
	if(printToScreen) printf("Couldn't open: %s\n",G12A_DEV_NAME);
	handleBadSigs(1001);
    }
    else fdG12=retVal;
 

    retVal=openGpsDevice(ADU5A_DEV_NAME);	
    if(retVal<=0) {
	syslog(LOG_ERR,"Couldn't open: %s\n",ADU5A_DEV_NAME);
	if(printToScreen) printf("Couldn't open: %s\n",ADU5A_DEV_NAME);
	handleBadSigs(1);
    }
    else fdAdu5A=retVal;

//    printf("%s %s %s\n",G12A_DEV_NAME,ADU5A_DEV_NAME,ADU5A_DEV_NAME);
    return 0;
}


int setupG12()
/*! Initializes the G12 with the correct PPS and ZDA settings */
{
    char portNames[2]={'A','B'};
    char edgeNames[2]={'R','F'};
    char g12Command[1024]="";
    char tempCommand[128]="";
    char dataPort='A';
    char ntpPort='B';
    char ppsEdge='R';
    int retVal;

    if(g12NtpPort==1 || g12NtpPort==2) ntpPort=portNames[g12NtpPort-1];
    if(g12DataPort==1 || g12DataPort==2) dataPort=portNames[g12DataPort-1];
    if(g12PpsRisingOrFalling==1 || g12PpsRisingOrFalling==2)
	ppsEdge=edgeNames[g12PpsRisingOrFalling-1];

//    strcat(g12Command,"$PASHQ,RID\n");
    if(g12EnableTtt) {
	sprintf(g12Command,"$PASHS,POP,%d\n",g12TttEpochRate);
	sprintf(tempCommand,"$PASHS,RCI,%1.2f\n",1.0/((float)g12TttEpochRate));
	strcat(g12Command,tempCommand);
    }
    else {
	sprintf(g12Command,"$PASHS,POP,10\n");
    }
//    strcat(g12Command,"$PASHQ,PRT,A\n");
//    strcat(g12Command,"$PASHQ,PRT,B\n");
    strcat(g12Command,"$PASHS,ELM,0\n");

    sprintf(tempCommand,"$PASHS,NME,ALL,%c,OFF\n",dataPort);
    strcat(g12Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,ALL,%c,OFF\n",ntpPort);
    strcat(g12Command,tempCommand);
    strcat(g12Command, "$PASHS,LTZ,0,0\n");
    strcat(g12Command, "$PASHS,UTS,ON\n");
    sprintf(tempCommand,"$PASHS,NME,ZDA,%c,ON,%d\n",dataPort,g12ZdaPeriod);
    strcat(g12Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,POS,%c,ON,%2.2f\n",dataPort,g12PosPeriod);
    strcat(g12Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,SAT,%c,ON,%d\n",dataPort,g12SatPeriod);
    strcat(g12Command,tempCommand);
 
    sprintf(tempCommand,"$PASHS,SPD,%c,4\n",ntpPort);
    strcat(g12Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,RMC,%c,ON,1\n",ntpPort);
    strcat(g12Command,tempCommand);
    sprintf(tempCommand, "$PASHS,PPS,%2.2f,%3.2f,%c\n",g12PpsPeriod,
	    g12PpsOffset,ppsEdge);
    strcat(g12Command,tempCommand);    
    if(g12EnableTtt) {
	sprintf(tempCommand,"$PASHS,NME,TTT,%c,ON,%1.2f\n",dataPort,1.0/((float)g12TttEpochRate));
	strcat(g12Command,tempCommand);        
    }


    if(printToScreen) 
	fprintf(stderr,"G12:\n%s\n%s\nLength: %d\n",G12A_DEV_NAME,g12Command,strlen(g12Command));
    retVal=write(fdG12, g12Command, strlen(g12Command));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to G12 Serial port\n, write: %s",
	       strerror(errno));
	fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	       strerror(errno));
	
    }
    else {
	syslog(LOG_INFO,"Sent %d bytes to G12 serial port",retVal);
	printf("Sent %d bytes to G12 serial port\n",retVal);
	
    }
    return retVal;
}


int checkG12()
/*! Try to read G12 */
{
//    printf("Checking G12\n");
    // Working variables
    char tempData[G12_DATA_SIZE];
    int retVal,i;
// Data stuff for G12
    static char g12Output[G12_DATA_SIZE]="";
    static int g12OutputLength=0;
    static int lastStar=-10;
    retVal=isThereDataNow(fdG12);
    usleep(5);
//    usleep(1);
//    printf("Check G12 got retVal %d\n",retVal);
    if(retVal!=1) return 0;
    retVal=read(fdG12, tempData, G12_DATA_SIZE);
    
    if(retVal>0) {
	for(i=0; i < retVal; i++) {
	    if(tempData[i]=='*') {
		lastStar=g12OutputLength;
	    }
	    //    printf("%c %d %d\n",tempData[i],g12OutputLength,lastStar);
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


int checkAdu5A()
/*! Try to read ADU5 */
{
//    printf("Here\n");
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
		    processAdu5Output(adu5Output,adu5OutputLength);
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


void processG12Output(char *tempBuffer, int length,int latestData)
/* Processes each G12 output string and acts accordingly */
{
    char gpsString[ADU5_DATA_SIZE];
    char gpsCopy[ADU5_DATA_SIZE];
    int gpsLength=0;
    int count=0;
    char *subString; 
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
    if(printToScreen) printf("G12:\t%s\t%d\n",gpsString,gpsLength);
    count=0;
    subString = strtok (gpsCopy,",");
    if(!strcmp(subString,"$GPZDA")) {
//	printf("Got $GPZDA\n");
	processGpzdaString(gpsString,gpsLength,latestData);
    }
    else if(!strcmp(subString,"$PASHR")) {
	//Maybe have POS,TTT or SAT
	subString = strtok (NULL, " ,.*");
	if(!strcmp(subString,"TTT")) {
//	    printf("Got TTT\n");
	    processTttString(gpsString,gpsLength,0);
	}
	if(!strcmp(subString,"POS")) {
//	    printf("Got POS\n");
	    processPosString(gpsString,gpsLength);
	}
	if(!strcmp(subString,"SAT")) {
//	    printf("And got Sat\n");
	    processG12SatString(gpsString,gpsLength);
	}
	
    }


}


void processGpzdaString(char *gpsString, int gpsLength, int latestData) 
/* Processes each GPZDA output string, writes it to disk and, if so inclined, sets the clock. */
{    
    char gpsCopy[G12_DATA_SIZE];
    char *subString;
    strncpy(gpsCopy,gpsString,gpsLength);
//    sprintf(gpsCopy,"$GPZDA,222835.10,21,07,1999,-07,00*4D");
    int hour,minute,second,subSecond;
    int day=-1,month=-1,year=-1;
    int tzHour,tzMin;
    time_t rawtime,gpsRawTime;
    struct tm timeinfo;    
    char unixString[128],otherString[128];


    subString = strtok (gpsCopy,"*");
    sscanf(subString,"$GPZDA,%02d%02d%02d.%02d,%02d,%02d,%04d,%02d,%02d",
	   &hour,&minute,&second,&subSecond,&day,&month,&year,&tzHour,&tzMin);
//    printf("%02d:%02d:%02d.%02d %02d %02d %02d\n%s\n",hour,minute,second,subSecond,day,month,year,ctime(&rawtime)); 
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
    if(abs(gpsRawTime-rawtime)>g12ClockSkew && g12UpdateClock &&latestData) {
	if(!g12StartNtp || (g12StartNtp && !startedNtpd))
	    updateClockFromG12(gpsRawTime);
    }

    //Need to output data to file somehow

}


void processGpvtgString(char *gpsString, int gpsLength) {
    char gpsCopy[ADU5_DATA_SIZE];
    char *subString;   
    char theFilename[FILENAME_MAX];
    int retVal;
    GpsAdu5VtgStruct_t theVtg;
    time_t rawtime;

    strncpy(gpsCopy,gpsString,gpsLength);
//    sprintf(gpsCopy,"$GPVTG,179.0,T,193.00,M,000.11,N,000.20,K*3E");

    //Get unix Timestamp
    time ( &rawtime );
    theVtg.unixTime=rawtime;

    //Process string
    subString = strtok (gpsCopy,"*");
    sscanf(subString,"$GPVTG,%f,T,%f,M,%f,N,%f,K",
	   &(theVtg.trueCourse),&(theVtg.magneticCourse),
	   &(theVtg.speedInKnots),&(theVtg.speedInKPH));

    fillGenericHeader(&theVtg,PACKET_GPS_ADU5_VTG,sizeof(GpsAdu5VtgStruct_t));
           
    //Write file and link for sipd
    sprintf(theFilename,"%s/vtg_%ld.dat",ADU5_VTG_TELEM_DIR,theVtg.unixTime);
    retVal=writeGpsVtg(&theVtg,theFilename);  
    retVal=makeLink(theFilename,ADU5_VTG_TELEM_LINK_DIR);  

    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theVtg,sizeof(GpsAdu5VtgStruct_t),
			 theVtg.unixTime,&adu5VtgWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }


//    if(useUsbDisks) {
//	sprintf(theFilename,"%s/vtg_%ld.dat",
//		gpsAdu5VtgUsbArchiveDir,theVtg.unixTime);
//	retVal=writeGpsVtg(&theVtg,theFilename);  
//    }


}


void processPosString(char *gpsString, int gpsLength) {
    char gpsCopy[ADU5_DATA_SIZE];
    char *subString;   
    char theFilename[FILENAME_MAX];
    int retVal;

    int hour,minute,second,subSecond;
    int posType;
    char eastOrWest=0,northOrSouth=0;
    int latDeg=0,longDeg=0;
    float latMin=0,longMin=0;
    float reserved;
    char firmware[5];
    struct timeval timeStruct;
	
    //Initialize structure
    GpsG12PosStruct_t thePos;
    bzero(&thePos,sizeof(GpsG12PosStruct_t));

    
    //Copy string
    strncpy(gpsCopy,gpsString,gpsLength);
//    sprintf(gpsCopy,"$PASHR,POS,0,03,003821.00,7751.70766,S,16703.78078,E,-00039.33,,007.64,000.17,+000.00,03.6,03.6,00.0,01.4,YM06*38");
    //Get unix Timestamp
    gettimeofday(&timeStruct,NULL);
    thePos.unixTime=timeStruct.tv_sec;
    thePos.unixTimeUs=timeStruct.tv_usec;

    //Scan string
    subString = strtok (gpsCopy,"*");
/*    printf("%s\n",subString); */
    sscanf(subString,"$PASHR,POS,%d,%lu,%02d%02d%02d.%02d,%02d%f,%c,%03d%f,%c,%f,,%f,%f,%f,%f,%f,%f,%f,%s",
	   &posType,&(thePos.numSats),&hour,&minute,&second,&subSecond,
	   &latDeg,&latMin,&northOrSouth,&longDeg,&longMin,&eastOrWest,
	   &(thePos.altitude),&(thePos.trueCourse),
	   &(thePos.speedInKnots),&(thePos.verticalVelocity),
	   &(thePos.pdop),&(thePos.hdop),&(thePos.vdop),&(thePos.tdop),
	   firmware);
          
    //Process Position
    thePos.latitude=(float)latDeg;		
    thePos.latitude+=(latMin/60.);
    if(northOrSouth=='S') thePos.latitude*=-1;
    thePos.longitude=(float)longDeg;		
    thePos.longitude+=(longMin/60.);
    if(eastOrWest=='W') thePos.longitude*=-1;
    
    //Process Time
//    printf("Time:\n%d\n%d\n%d\n%d\n",hour,minute,second,subSecond);
    thePos.timeOfDay=(float)(10*subSecond); 
    thePos.timeOfDay+=(float)(1000*second);
    thePos.timeOfDay+=(float)(1000*60*minute);
    thePos.timeOfDay+=(float)(1000*60*60*hour);  

    //Debugging
  /*   printf("Time:\t %ld\t%ld\t%ld\nNum Sats:\t%d\nPosition:\t%f\t%f\t%f\nCourse/Speed:\t%f\t%f\t%f\nDOP:\t\t%f\t%f\t%f\t%f\n", */
/* 	   thePos.unixTime,thePos.unixTimeUs,thePos.timeOfDay, */
/* 	   thePos.numSats, */
/* 	   thePos.latitude,thePos.longitude,thePos.altitude, */
/* 	   thePos.trueCourse,thePos.speedInKnots,thePos.verticalVelocity, */
/* 	   thePos.pdop,thePos.hdop,thePos.vdop,thePos.tdop); */
/*     exit(0); */


    fillGenericHeader(&thePos,PACKET_GPS_G12_POS,sizeof(GpsG12PosStruct_t));
    //Write file and link for sipd
    sprintf(theFilename,"%s/pos_%ld.dat",G12_POS_TELEM_DIR,thePos.unixTime);
    retVal=writeGpsPos(&thePos,theFilename);  
    retVal=makeLink(theFilename,G12_POS_TELEM_LINK_DIR);  

    //Write file to main disk

    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&thePos,sizeof(GpsG12PosStruct_t),
			 thePos.unixTime,&g12PosWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }

//    if(useUsbDisks) {
//	sprintf(theFilename,"%s/pos_%ld.dat",
//		gpsG12PosUsbArchiveDir,thePos.unixTime);
//	retVal=writeGpsPos(&thePos,theFilename);  
//    }


}


void processAdu5Output(char *tempBuffer, int length)
/* Processes each ADU5 output string and acts accordingly */
{
    char gpsString[ADU5_DATA_SIZE];
    char gpsCopy[ADU5_DATA_SIZE];
    int gpsLength=0;
    int count=0;
    char *subString; 
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
	processGppatString(gpsString,gpsLength);
    }
    else if(!strcmp(subString,"$GPVTG")) {
//	printf("Got $GPZDA\n");
	processGpvtgString(gpsString,gpsLength);
    }
    else if(!strcmp(subString,"$PASHR")) {
	//Maybe have TTT
	subString = strtok (NULL, " ,.*");
	if(!strcmp(subString,"TTT")) {
//	    printf("Got %s\t",subString);
//	    printf("Got TTT\n");
	    processTttString(gpsString,gpsLength,1);
	}
	if(!strcmp(subString,"SA4")) {
//	    printf("And got Sat\n");
	    processAdu5Sa4String(gpsString,gpsLength);
	}
	
    }


}

void processTttString(char *gpsString, int gpsLength, int fromAdu5) {
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
    theTTT.fromAdu5=fromAdu5;

    //Write file for eventd
    sprintf(filename,"%s/gps_%lu_%lu.dat",GPSD_SUBTIME_DIR,theTTT.unixTime,theTTT.subTime);
    writeGpsTtt(&theTTT,filename);
    retVal=makeLink(filename,GPSD_SUBTIME_LINK_DIR);  

    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theTTT,sizeof(GpsSubTime_t),
			 theTTT.unixTime,&adu5TttWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }
    
//    if(useUsbDisks) {
//	//Write file to usb disk
//	sprintf(filename,"%s/gps_%lu_%lu.dat",gpsAdu5TttUsbArchiveDir,theTTT.unixTime,theTTT.subTime);
//	writeGpsTtt(&theTTT,filename);
//    }
    
    

}



void processG12SatString(char *gpsString, int gpsLength) {
    char gpsCopy[ADU5_DATA_SIZE];
    char *subString;
    char theFilename[FILENAME_MAX];
    int retVal;
    int count=0;
    int doingSat=0;
    strncpy(gpsCopy,&gpsString[11],gpsLength-11);
//    printf("%s\n",gpsCopy);
    GpsG12SatStruct_t theSat;
    bzero(&theSat,sizeof(GpsG12SatStruct_t));
    theSat.gHdr.code=PACKET_GPS_G12_SAT;
    
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
		theSat.sat[doingSat].flag=subString[0];
//		if(subString[0]=='U') theSat.sat[doingSat].flag=1;
//		if(subString[0]=='-') theSat.sat[doingSat].flag=0;
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

    fillGenericHeader(&theSat,PACKET_GPS_G12_SAT,sizeof(GpsG12SatStruct_t));

    //Write file and link for sipd
    sprintf(theFilename,"%s/sat_%ld.dat",G12_SAT_TELEM_DIR,theSat.unixTime);
    retVal=writeGpsG12Sat(&theSat,theFilename);  
    retVal=makeLink(theFilename,G12_SAT_TELEM_LINK_DIR);  

    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theSat,sizeof(GpsG12SatStruct_t),
			 theSat.unixTime,&g12SatWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }
  
}


void processAdu5Sa4String(char *gpsString, int gpsLength) {
    char gpsCopy[ADU5_DATA_SIZE];
    char *subString;
    char theFilename[FILENAME_MAX];
    int retVal;
    int count=0;
    int doingSat=0;
    int doingAnt=-1;
    strncpy(gpsCopy,&gpsString[11],gpsLength-11);
//    printf("%s\n",gpsCopy);
    static GpsAdu5SatStruct_t theSat;
    static int reZero=1;
    static int whichDone[4];
    if(reZero) {
//	printf("\n\nRe-zeroing\n\n");
	bzero(&theSat,sizeof(GpsAdu5SatStruct_t));
	theSat.gHdr.code=PACKET_GPS_ADU5_SAT;
	reZero=0;	
	time_t rawtime;
	time ( &rawtime );
	theSat.unixTime=rawtime;
	for(doingAnt=0;doingAnt<4;doingAnt++) whichDone[doingAnt]=0;
	doingAnt=-1;
    }
    
    subString = strtok (gpsCopy,",");    
    while (subString != NULL) {
//	printf("%d\t%s\n",count,subString);	
	switch(count) {
	    case 0:		
		doingAnt=atoi(subString)-1;
		break;
	    case 1:		
		theSat.numSats[doingAnt]=atoi(subString);
		break;
	    case 2:
		theSat.sat[doingAnt][doingSat].prn=atoi(subString);
		break;
	    case 3:
//		theSat.sat[doingAnt][doingSat].azimuth=atoi(subString);
//		break;
//	    case 4:
//		theSat.sat[doingAnt][doingSat].elevation=atoi(subString);
//		break;
//	    case 5:
		theSat.sat[doingAnt][doingSat].snr=atoi(subString);
		doingSat++;
		count=1;
		break;
	    default:
		break;
	}
	if(doingAnt<0 || doingAnt>3) break;
	if(subString[1]=='*') break;
	count++;
	subString = strtok (NULL, ",");	
    }
    
/*     for(count=0;count<theSat.numSats;count++) { */
/* 	printf("Number:\t%d\nPRN:\t%d\nAzimuth:\t%d\nElevation:\t%d\nSNR:\t%d\nFlag:\t%d\n\n",count+1,theSat.sat[count].prn,theSat.sat[count].azimuth,theSat.sat[count].elevation,theSat.sat[count].snr,theSat.sat[count].flag); */
/*     } */
    if(doingAnt>=0 && doingAnt<=3) whichDone[doingAnt]=1;

    if(doingAnt==3) {
	reZero=1;
	count=0;
	for(doingAnt=0;doingAnt<4;doingAnt++) count+=whichDone[doingAnt];
	if(count!=4) {
	    syslog(LOG_WARNING,"Incomplete SA4 data from ADU5");
	    if(printToScreen) fprintf(stderr,"Incomplete SA4 data from ADU5\n");
	}
	else {
	    fillGenericHeader(&theSat,PACKET_GPS_ADU5_SAT,sizeof(GpsAdu5SatStruct_t));
	    //Write file and link for sipd
	    sprintf(theFilename,"%s/sat_adu5_%ld.dat",ADU5_SAT_TELEM_DIR,theSat.unixTime);
	    retVal=writeGpsAdu5Sat(&theSat,theFilename);  
	    retVal=makeLink(theFilename,ADU5_SAT_TELEM_LINK_DIR);  

	    //Write file to main disk
	    retVal=cleverHkWrite((unsigned char*)&theSat,sizeof(GpsAdu5SatStruct_t),
				 theSat.unixTime,&adu5SatWriter);
	    if(retVal<0) {
		//Had an error
		//Do something
	    }	    
	}
    }

}



void processGppatString(char *gpsString, int gpsLength) {
    char gpsCopy[ADU5_DATA_SIZE];
    char *subString;
    
    int hour,minute,second,subSecond;
    char eastOrWest=0,northOrSouth=0;
    int latDeg=0,longDeg=0;
    float latMin=0,longMin=0;
    struct timeval timeStruct;
    
    char theFilename[FILENAME_MAX];
    int retVal;
    
    //Initialize structure
    GpsAdu5PatStruct_t thePat;
    bzero(&thePat,sizeof(GpsAdu5PatStruct_t));
    
    //Copy String
    strncpy(gpsCopy,gpsString,gpsLength);
//    sprintf(gpsCopy,"$GPPAT,021140.00,3338.51561,N,11750.82575,W,-00288.42,666.0000,000.00,000.00,666.0000,666.0000,1*4F");

    //Set PacketCode
    thePat.gHdr.code=PACKET_GPS_ADU5_PAT;

    //Get unix Timestamp
    gettimeofday(&timeStruct,NULL);
    thePat.unixTime=timeStruct.tv_sec;
    thePat.unixTimeUs=timeStruct.tv_usec;

     //Scan string
    subString = strtok (gpsCopy,"*");
//    printf("%s\n",subString);
    sscanf(subString,"$GPPAT,%02d%02d%02d.%02d,%02d%f,%c,%03d%f,%c,%f,%f,%f,%f,%f,%f,%lu",
	   &hour,&minute,&second,&subSecond,
	   &latDeg,&latMin,&northOrSouth,&longDeg,&longMin,&eastOrWest,
	   &(thePat.altitude),&(thePat.heading),&(thePat.pitch),
	   &(thePat.roll),&(thePat.mrms),&(thePat.brms),&(thePat.attFlag));

	            
    //Process Position
    thePat.latitude=(float)latDeg;		
    thePat.latitude+=(latMin/60.);
    if(northOrSouth=='S') thePat.latitude*=-1;
    thePat.longitude=(float)longDeg;		
    thePat.longitude+=(longMin/60.);
    if(eastOrWest=='W') thePat.longitude*=-1;
    

    //Process Time
//    printf("Time:\n%d\n%d\n%d\n%d\n",hour,minute,second,subSecond);
    thePat.timeOfDay=(float)(10*subSecond);
    thePat.timeOfDay+=(float)(1000*second);
    thePat.timeOfDay+=(float)(1000*60*minute);
    thePat.timeOfDay+=(float)(1000*60*60*hour);  

/*     printf("%s\n",gpsTest); */
/*     printf("unixTime: %ld\ntimeOfDay: %ld\nheading: %lf\npitch: %lf\nroll: %lf\nmrms: %lf\nbrms: %lf\nattFlag: %d\nlatitude: %lf\nlongitude: %lf\naltitute: %lf\n",  */
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
    

    fillGenericHeader(&thePat,PACKET_GPS_ADU5_PAT,sizeof(GpsAdu5PatStruct_t));

    //Write file and link for sipd
    sprintf(theFilename,"%s/pat_%ld.dat",ADU5_PAT_TELEM_DIR,thePat.unixTime);
    retVal=writeGpsPat(&thePat,theFilename);  
    retVal=makeLink(theFilename,ADU5_PAT_TELEM_LINK_DIR);  


    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&thePat,sizeof(GpsAdu5PatStruct_t),
			 thePat.unixTime,&adu5PatWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }	    
    
}



int updateClockFromG12(time_t gpsRawTime)
/*!Just updates the clock, using sudo date */
{
    int retVal=0;//,len;
    pid_t childPid;
//    char writeString[180];
    char dateCommand[180];
    struct tm *ptm;
    gpsRawTime++; /*Silly*/	
    ptm = gmtime(&gpsRawTime);
    sprintf(dateCommand,"%02d%02d%02d%02d%04d.%02d",
	    ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,
	    ptm->tm_year+1900,ptm->tm_sec);

    char *setDateArg[]={"/usr/bin/sudo","/bin/date",dateCommand,(char*)0};
//    retVal=system(dateCommand);
    printf("Trying to set clock using date to %s\n",dateCommand);
    childPid=fork();
    if(childPid==0) {
	//Child
//          awsPtr->currentFileName;
	execv("/usr/bin/sudo",setDateArg);
	exit(0);
    }
    else if(childPid<0) {
	//Something wrong
	syslog(LOG_ERR,"Problem updating clock, %ld: %s ",gpsRawTime,strerror(errno));
	fprintf(stderr,"Problem updating clock, %ld: %s ",gpsRawTime,strerror(errno));
	return 0;
    }

    return retVal;
    
}


/* int breakdownG12TimeString(char *subString, int *hour, int *minute, int *second)  */
/* /\*!Splits up the time string into hours, minutes and seconds*\/ */
/* { */

/*     char hourStr[3],minuteStr[3],secondStr[3]; */
    
/*     hourStr[0]=subString[0]; */
/*     hourStr[1]=subString[1]; */
/*     hourStr[2]='\0'; */
/*     minuteStr[0]=subString[2]; */
/*     minuteStr[1]=subString[3]; */
/*     minuteStr[2]='\0'; */
/*     secondStr[0]=subString[4]; */
/*     secondStr[1]=subString[5]; */
/*     secondStr[2]='\0'; */
    
/*     *hour=atoi(hourStr); */
/*     *minute=atoi(minuteStr); */
/*     *second=atoi(secondStr);     */
/*     return 0; */
/* } */




int setupAdu5()
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
    sprintf(tempCommand,"$PASHS,NME,PAT,A,ON,%2.2f",adu5PatPeriod);
    strcat(tempCommand,"\r\n");
    strcat(adu5Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,SA4,A,ON,%d\r\n",adu5SatPeriod);
    strcat(adu5Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,VTG,A,ON,%2.2f\r\n",adu5VtgPeriod);
    strcat(adu5Command,tempCommand);
    strcat(adu5Command,"$PASHQ,PRT\r\n");
    if(adu5EnableTtt) strcat(adu5Command,"$PASHS,NME,TTT,A,ON\r\n");

    if(printToScreen) printf("ADU5:\n%s\n%s\n",ADU5A_DEV_NAME,adu5Command);
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

int tryToStartNtpd()
{
    static int donePort=0;
    int retVal=0;
    pid_t childPid;    
    time_t theTime=time(NULL);
    char *startNtpArg[]={"/bin/bash","/home/anita/flightSoft/bin/startNtp.sh",(char*)0};
    if(!donePort) {
	retVal=openGpsDevice(G12B_DEV_NAME);
	//May put something here to read message
	close(retVal);
	if(printToScreen) printf("Opened and closed: %s\n",G12B_DEV_NAME);
	donePort=1;
    }
    if(theTime>1e9) {
	childPid=fork();
	if(childPid==0) {
	    //Child
//	    awsPtr->currentFileName;
	    execv("/bin/bash",startNtpArg);
	    exit(0);
	}
	else if(childPid<0) {
	    //Something wrong
	    syslog(LOG_ERR,"Problem starting ntpd");
	    fprintf(stderr,"Problem starting ntpd\n");
	    return 0;
	}
	syslog(LOG_INFO,"Restarted ntpd");
	fprintf(stderr,"Restarted ntpd\n");
	return 1;
    }
    return 0;
}

void prepWriterStructs() {
    int diskInd;
    if(printToScreen) 
	printf("Preparing Writer Structs\n");

    //Adu5 Pat
    sprintf(adu5PatWriter.relBaseName,"%s/adu5/pat/",GPS_ARCHIVE_DIR);
    sprintf(adu5PatWriter.filePrefix,"pat");
    adu5PatWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5PatWriter.currentFilePtr[diskInd]=0;


    //Adu5 Sat
    sprintf(adu5SatWriter.relBaseName,"%s/adu5/sat/",GPS_ARCHIVE_DIR);
    sprintf(adu5SatWriter.filePrefix,"sat_adu5");
    adu5SatWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5SatWriter.currentFilePtr[diskInd]=0;

    //Adu5 Vtg
    sprintf(adu5VtgWriter.relBaseName,"%s/adu5/vtg/",GPS_ARCHIVE_DIR);
    sprintf(adu5VtgWriter.filePrefix,"vtg");
    adu5VtgWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5VtgWriter.currentFilePtr[diskInd]=0;

    //Adu5 Ttt
    sprintf(adu5TttWriter.relBaseName,"%s/adu5/ttt/",GPS_ARCHIVE_DIR);
    sprintf(adu5TttWriter.filePrefix,"ttt");
    adu5TttWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5TttWriter.currentFilePtr[diskInd]=0;


    //G12 Pos
    sprintf(g12PosWriter.relBaseName,"%s/g12/pos/",GPS_ARCHIVE_DIR);
    sprintf(g12PosWriter.filePrefix,"pos");
    g12PosWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	g12PosWriter.currentFilePtr[diskInd]=0;

    //G12 Sat
    sprintf(g12SatWriter.relBaseName,"%s/g12/sat/",GPS_ARCHIVE_DIR);
    sprintf(g12SatWriter.filePrefix,"sat");
    g12SatWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	g12SatWriter.currentFilePtr[diskInd]=0;
    

}    


void handleBadSigs(int sig)
{
   
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
    closeHkFilesAndTidy(&adu5PatWriter);
    closeHkFilesAndTidy(&adu5SatWriter);
    closeHkFilesAndTidy(&adu5TttWriter);
    closeHkFilesAndTidy(&adu5VtgWriter);
    closeHkFilesAndTidy(&g12PosWriter);
    closeHkFilesAndTidy(&g12SatWriter);
    unlink(gpsdPidFile);
    syslog(LOG_INFO,"GPSd terminating");    
    exit(0);
}
