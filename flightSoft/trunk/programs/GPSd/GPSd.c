/*! \file GPSd.c
    \brief The GPSd program that creates GPS objects 
    
    Talks to the three GPS units and does it's funky stuff

    May 2008 rjn@hep.ucl.ac.uk
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
#include <libgen.h> //For Mac OS X

/* Flight soft includes */
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "serialLib/serialLib.h"
#include "utilLib/utilLib.h"
#include "mapLib/mapLib.h"
#include "includes/anitaStructures.h"

// Forward declarations
int readConfigFile();
int openDevices();
int setupG12();
int setupAdu5A();
int setupAdu5B();
int checkG12();
int checkAdu5A();
int checkAdu5B();
void processG12Output(char *tempBuffer, int length,int latestData);
void processAdu5Output(char *tempBuffer, int length,int latestData,int fromAdu5); //1 is old, 2 is new
int updateClockFromG12(time_t gpsRawTime);
void prepWriterStructs();
//int breakdownG12TimeString(char *subString,int *hour,int *minute,int *second);
//int writeG12TimeFile(time_t compTime, time_t gpsTime, char *g12Output);
int checkChecksum(char *gpsString,int gpsLength);
void processAckString(char *gpsString, int length, int fromAdu5);
void processNakString(char *gpsString, int length, int fromAdu5);
void processRioString(char *gpsString, int length, int fromAdu5);
void processBitString(char *gpsString, int length, int fromAdu5);
void processTstString(char *gpsString, int length, int fromAdu5);

void processGppatString(char *gpsString, int length, int fromAdu5);
void processTttString(char *gpsString, int gpsLength,int fromAdu5);
void processGpggaString(char *gpsString, int gpsLength, int fromAdu5);
void processGpvtgString(char *gpsString, int gpsLength, int fromAdu5);
void processAdu5aSatString(char *gpsString, int gpsLength, int isSat);
void processAdu5bSatString(char *gpsString, int gpsLength, int isSat);
void processG12SatString(char *gpsString, int gpsLength);
void processGpzdaString(char *gpsString, int gpsLength, int latestData, int fromAdu5);
void processPosString(char *gpsString, int gpsLength);
int tryToStartNtpd();
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
void writeStartTest(time_t startTime);
void checkPhiSources(GpsAdu5PatStruct_t *patPtr) ;

// Definitions
#define LEAP_SECONDS 14 //Need to check
#define G12_DATA_SIZE 1024
#define ADU5_DATA_SIZE 1024
#define ADU5_RIO "$PASHR,RIO,ADU5-0,____,YG01,D-P12-X--L-,800952(A)AD520033507*33"
#define G12_RIO "$PASHR,RIO,G12,XM06,,TTOPU--LEGM_C_,VC7200228022*49"


// File desciptors for GPS serial ports
int fdG12,fdAdu5A,fdAdu5B;//,fdMag
//Note here we are signifying that A = old ADU5, B = new ADU5


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
int g12GgaPeriod=1;
float g12PosPeriod=10;
int g12SatPeriod=600;
int g12PosTelemEvery=10;
int g12SatTelemEvery=1;
int g12GgaTelemEvery=1;
int g12StartNtp=0;
int g12UpdateClock=0;
int g12ClockSkew=0;
int g12EnableTtt=0;
int g12TttEpochRate=20;
int g12IniReset=0;
int g12ElevationMask=0;

// Config stuff for ADU5A
int adu5aEnableTtt=0;
int adu5aSatPeriod=600;
int adu5aZdaPeriod=1;
int adu5aGgaPeriod=1;
float adu5aPatPeriod=10;
float adu5aVtgPeriod=10;
int adu5aSatTelemEvery=1;
int adu5aPatTelemEvery=10;
int adu5aVtgTelemEvery=10;
int adu5aGgaTelemEvery=10;
float adu5aRelV12[3]={0};
float adu5aRelV13[3]={0};
float adu5aRelV14[3]={0};
int adu5aIniReset=0;
float adu5aCycPhaseErrorThreshold=0.2;
float adu5aMxbBaselineError=0.035;
float adu5aMxmPhaseError=0.005;
int adu5aElevationMask=0;

// Config stuff for ADU5B
int adu5bEnableTtt=0;
int adu5bSatPeriod=600;
int adu5bZdaPeriod=1;
int adu5bGgaPeriod=1;
float adu5bPatPeriod=10;
float adu5bVtgPeriod=10;
int adu5bSatTelemEvery=1;
int adu5bPatTelemEvery=10;
int adu5bVtgTelemEvery=10;
int adu5bGgaTelemEvery=10;
float adu5bRelV12[3]={0};
float adu5bRelV13[3]={0};
float adu5bRelV14[3]={0};
int adu5bIniReset=0;
float adu5bCycPhaseErrorThreshold=0.2;
float adu5bMxbBaselineError=0.035;
float adu5bMxmPhaseError=0.005;
int adu5bElevationMask=0;

//Stuff for GPS Phi Masking
int enableGpsPhiMasking=0;
int phiMaskingPeriod=0;
int numSources=1;
float sourceLats[MAX_PHI_SOURCES]={0};
float sourceLongs[MAX_PHI_SOURCES]={0};
float sourceAlts[MAX_PHI_SOURCES]={0};
float sourceDistKm[MAX_PHI_SOURCES]={0};
int sourcePhiWidth[MAX_PHI_SOURCES]={0};

//Output stuff
int hkDiskBitMask;
int disableUsb=0;
int disableNeobrick=0;
int disableHelium1=0;
int disableHelium2=0;


//Hk Writers
AnitaHkWriterStruct_t adu5aPatWriter;
AnitaHkWriterStruct_t adu5aSatWriter;
AnitaHkWriterStruct_t adu5aVtgWriter;
AnitaHkWriterStruct_t adu5aGgaWriter;

AnitaHkWriterStruct_t adu5bPatWriter;
AnitaHkWriterStruct_t adu5bSatWriter;
AnitaHkWriterStruct_t adu5bVtgWriter;
AnitaHkWriterStruct_t adu5bGgaWriter;

AnitaHkWriterStruct_t gpsTttWriter;


AnitaHkWriterStruct_t g12PosWriter;
AnitaHkWriterStruct_t g12SatWriter;
AnitaHkWriterStruct_t g12GgaWriter;

int startedNtpd=0;

GpsdStartStruct_t startStruct;

int main (int argc, char *argv[])
{
    int retVal;

    /* Config file thingies */
    int status=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;

    /* Log stuff */
    char *progName=basename(argv[0]);
    time_t startTime;
    time_t currentTime;
    
    bzero(&startStruct,sizeof(GpsdStartStruct_t));
    
    //Sort out pid File
    retVal=sortOutPidFile(progName);
    if(retVal!=0) {
      return retVal;
    }

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
    syslog(LOG_INFO,"GPSd Starting");    

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
	
	disableUsb=kvpGetInt("disableUsb",1);
	if(disableUsb)
	    hkDiskBitMask&=~USB_DISK_MASK;
	disableNeobrick=kvpGetInt("disableNeobrick",1);
	if(disableNeobrick)
	    hkDiskBitMask&=~NEOBRICK_DISK_MASK;
	disableHelium2=kvpGetInt("disableHelium2",1);
	if(disableHelium2)
	    hkDiskBitMask&=~HELIUM2_DISK_MASK;
	disableHelium1=kvpGetInt("disableHelium1",1);
	if(disableHelium1)
	    hkDiskBitMask&=~HELIUM1_DISK_MASK;

    }
    makeDirectories(GPSD_SUBTIME_LINK_DIR);
    makeDirectories(ADU5A_PAT_TELEM_LINK_DIR);
    makeDirectories(ADU5A_VTG_TELEM_LINK_DIR);
    makeDirectories(ADU5A_GGA_TELEM_LINK_DIR);
    makeDirectories(ADU5A_SAT_TELEM_LINK_DIR);
    makeDirectories(ADU5B_PAT_TELEM_LINK_DIR);
    makeDirectories(ADU5B_VTG_TELEM_LINK_DIR);
    makeDirectories(ADU5B_GGA_TELEM_LINK_DIR);
    makeDirectories(ADU5B_SAT_TELEM_LINK_DIR);
    makeDirectories(G12_POS_TELEM_LINK_DIR);
    makeDirectories(G12_GGA_TELEM_LINK_DIR);
    makeDirectories(G12_SAT_TELEM_LINK_DIR);
    makeDirectories(REQUEST_TELEM_LINK_DIR);
    
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
	  printf("Device fd's: %d %d %d\nDevices:\t%s\t%s\t%s\n",fdG12,fdAdu5A,fdAdu5B,G12A_DEV_NAME,ADU5A_DEV_NAME,ADU5B_DEV_NAME);
	retVal=setupG12();
	retVal=setupAdu5A();
	retVal=setupAdu5B();
	time(&startTime);
	
	if(g12IniReset) {	  
//	  sleep(10);
	  CommandStruct_t theCmd;
	  theCmd.fromSipd=0;
	  theCmd.numCmdBytes=4;
	  theCmd.cmd[0]=GPSD_EXTRA_COMMAND;
	  theCmd.cmd[1]=GPS_SET_INI_RESET_FLAG;
	  theCmd.cmd[2]=3;
	  theCmd.cmd[3]=0;
	  writeCommandAndLink(&theCmd);
	}

	if(adu5aIniReset) {	  
//	  sleep(10);
	  CommandStruct_t theCmd;
	  theCmd.fromSipd=0;
	  theCmd.numCmdBytes=4;
	  theCmd.cmd[0]=GPSD_EXTRA_COMMAND;
	  theCmd.cmd[1]=GPS_SET_INI_RESET_FLAG;
	  theCmd.cmd[2]=1;
	  theCmd.cmd[3]=0;
	  writeCommandAndLink(&theCmd);
	}


	if(adu5bIniReset) {	  
//	  sleep(10);
	  CommandStruct_t theCmd;
	  theCmd.fromSipd=0;
	  theCmd.numCmdBytes=4;
	  theCmd.cmd[0]=GPSD_EXTRA_COMMAND;
	  theCmd.cmd[1]=GPS_SET_INI_RESET_FLAG;
	  theCmd.cmd[2]=2;
	  theCmd.cmd[3]=0;
	  writeCommandAndLink(&theCmd);
	}


//	printf("Here\n");
	currentState=PROG_STATE_RUN;
//	currentState=PROG_STATE_TERMINATE;
	unsigned int loopCounter=0;
	unsigned int sentStartFile=0;
	while(currentState==PROG_STATE_RUN) {
	    checkG12();
	    checkAdu5A();
	    checkAdu5B();
	    if(!startedNtpd && g12StartNtp && loopCounter>60) startedNtpd=tryToStartNtpd();
	    loopCounter++;
	    usleep(1000);
	    if(!sentStartFile) {
	      time(&currentTime);
	      if(currentTime>startTime+10) {
		writeStartTest(startTime);
		sentStartFile=1;
	      }
	    }
	}
	printf("currentState == %d\n",currentState);

    } while(currentState==PROG_STATE_INIT);

    if(printToScreen) printf("Terminating GPSd\n");
    if(fdG12) close(fdG12);
    if(fdAdu5A) close(fdAdu5A);
    if(fdAdu5B) close(fdAdu5B);

    closeHkFilesAndTidy(&adu5aPatWriter);
    closeHkFilesAndTidy(&adu5aSatWriter);
    closeHkFilesAndTidy(&adu5aVtgWriter);
    closeHkFilesAndTidy(&adu5aGgaWriter);
    closeHkFilesAndTidy(&adu5bPatWriter);
    closeHkFilesAndTidy(&adu5bSatWriter);
    closeHkFilesAndTidy(&adu5bVtgWriter);
    closeHkFilesAndTidy(&adu5bGgaWriter);
    closeHkFilesAndTidy(&gpsTttWriter);
    closeHkFilesAndTidy(&g12PosWriter);
    closeHkFilesAndTidy(&g12SatWriter);
    closeHkFilesAndTidy(&g12GgaWriter);
    unlink(GPSD_PID_FILE);

    syslog(LOG_INFO,"GPSd terminating");
//    removeFile(GPSD_PID_FILE);
    
	
    return 0;
}


int readConfigFile() 
/* Load GPSd config stuff */
{
    /* Config file thingies */
    int kvpStatus=0;
    int tempNum=0;
    char* eString ;
    kvpReset();
    kvpStatus = configLoad ("GPSd.config","output") ;
    if(kvpStatus == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",-1);
	verbosity=kvpGetInt("verbosity",-1);
	if(printToScreen<0) {
	    syslog(LOG_WARNING,
		   "Couldn't fetch printToScreen, defaulting to zero");
	    printToScreen=0;	    
	}
    }
    else {
	eString=configErrorString (kvpStatus) ;
	syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
    }
    kvpReset();
    kvpStatus = configLoad ("GPSd.config","g12");
    if(kvpStatus == CONFIG_E_OK) {
	g12ElevationMask=kvpGetInt("elevationMask",0);
	g12IniReset=kvpGetInt("iniReset",0);
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
	g12GgaPeriod=kvpGetInt("ggaPeriod",10); // in seconds
	g12PosPeriod=kvpGetFloat("posPeriod",10); // in seconds
	g12SatPeriod=kvpGetInt("satPeriod",600); // in seconds
	g12PosTelemEvery=kvpGetInt("posTelemEvery",10); // send every nth one
	g12SatTelemEvery=kvpGetInt("satTelemEvery",1);// send every nth one
	g12GgaTelemEvery=kvpGetInt("ggaTelemEvery",1);// send every nth one
	g12UpdateClock=kvpGetInt("updateClock",0); // 1 is yes, 0 is no
	g12StartNtp=kvpGetInt("startNtp",0); // 1 is yes, 0 is no
	g12ClockSkew=kvpGetInt("clockSkew",0); // Time difference in seconds
    }
    else {
	eString=configErrorString (kvpStatus) ;
	syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
    }
    kvpReset();
    kvpStatus = configLoad ("GPSd.config","adu5a") ;    
    if(kvpStatus == CONFIG_E_OK) {
	adu5aElevationMask=kvpGetInt("elevationMask",0);
	adu5aCycPhaseErrorThreshold=kvpGetFloat("cycPhaseErrorThreshold",0.2);
	adu5aMxbBaselineError=kvpGetFloat("mxbBaselineError",0.035);
	adu5aMxmPhaseError=kvpGetFloat("mxmPhaseError",0.005);
      adu5aIniReset=kvpGetInt("iniReset",0);
      adu5aEnableTtt=kvpGetInt("enableTtt",1);
      adu5aSatPeriod=kvpGetInt("satPeriod",600); // in seconds
      adu5aZdaPeriod=kvpGetInt("zdaPeriod",600); // in seconds
      adu5aGgaPeriod=kvpGetInt("ggaPeriod",600); // in seconds
      adu5aPatPeriod=kvpGetFloat("patPeriod",10); // in seconds
      adu5aVtgPeriod=kvpGetFloat("vtgPeriod",10); // in seconds
      adu5aSatTelemEvery=kvpGetInt("satTelemEvery",1); // send every nth one
      adu5aPatTelemEvery=kvpGetInt("patTelemEvery",10); // send every nth one
      adu5aVtgTelemEvery=kvpGetInt("vtgTelemEvery",10);// send every nth one
      adu5aGgaTelemEvery=kvpGetInt("ggaTelemEvery",10);// send every nth one
      adu5aRelV12[0]=kvpGetFloat("calibV12_1",0);
      adu5aRelV12[1]=kvpGetFloat("calibV12_2",0);
      adu5aRelV12[2]=kvpGetFloat("calibV12_3",0);
      adu5aRelV13[0]=kvpGetFloat("calibV13_1",0);
      adu5aRelV13[1]=kvpGetFloat("calibV13_2",0);
      adu5aRelV13[2]=kvpGetFloat("calibV13_3",0);
      adu5aRelV14[0]=kvpGetFloat("calibV14_1",0);
      adu5aRelV14[1]=kvpGetFloat("calibV14_2",0);
      adu5aRelV14[2]=kvpGetFloat("calibV14_3",0);
      //	printf("v12 %f %f %f\n",adu5aRelV12[0],adu5aRelV12[1],adu5aRelV12[2]);
      //	printf("v13 %f %f %f\n",adu5aRelV13[0],adu5aRelV13[1],adu5aRelV13[2]);
      //	printf("v14 %f %f %f\n",adu5aRelV14[0],adu5aRelV14[1],adu5aRelV14[2]);
    }
    else {
	eString=configErrorString (kvpStatus) ;
	syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
    }
    kvpReset();
    kvpStatus = configLoad ("GPSd.config","adu5b") ;    
    if(kvpStatus == CONFIG_E_OK) {
	adu5aElevationMask=kvpGetInt("elevationMask",0);
	adu5aCycPhaseErrorThreshold=kvpGetFloat("cycPhaseErrorThreshold",0.2);
	adu5aMxbBaselineError=kvpGetFloat("mxbBaselineError",0.035);
	adu5aMxmPhaseError=kvpGetFloat("mxmPhaseError",0.005);
	adu5bIniReset=kvpGetInt("iniReset",0);
	adu5bEnableTtt=kvpGetInt("enableTtt",1);
	adu5bSatPeriod=kvpGetInt("satPeriod",600); // in seconds
	adu5bZdaPeriod=kvpGetInt("zdaPeriod",600); // in seconds
	adu5bGgaPeriod=kvpGetInt("ggaPeriod",600); // in seconds
	adu5bPatPeriod=kvpGetFloat("patPeriod",10); // in seconds
	adu5bVtgPeriod=kvpGetFloat("vtgPeriod",10); // in seconds
	adu5bSatTelemEvery=kvpGetInt("satTelemEvery",1); // send every nth one
	adu5bPatTelemEvery=kvpGetInt("patTelemEvery",10); // send every nth one
	adu5bVtgTelemEvery=kvpGetInt("vtgTelemEvery",10);// send every nth one
	adu5bGgaTelemEvery=kvpGetInt("ggaTelemEvery",10);// send every nth one
	adu5bRelV12[0]=kvpGetFloat("calibV12_1",0);
	adu5bRelV12[1]=kvpGetFloat("calibV12_2",0);
	adu5bRelV12[2]=kvpGetFloat("calibV12_3",0);
	adu5bRelV13[0]=kvpGetFloat("calibV13_1",0);
	adu5bRelV13[1]=kvpGetFloat("calibV13_2",0);
	adu5bRelV13[2]=kvpGetFloat("calibV13_3",0);
	adu5bRelV14[0]=kvpGetFloat("calibV14_1",0);
	adu5bRelV14[1]=kvpGetFloat("calibV14_2",0);
	adu5bRelV14[2]=kvpGetFloat("calibV14_3",0);
//	printf("v12 %f %f %f\n",adu5bRelV12[0],adu5bRelV12[1],adu5bRelV12[2]);
//	printf("v13 %f %f %f\n",adu5bRelV13[0],adu5bRelV13[1],adu5bRelV13[2]);
//	printf("v14 %f %f %f\n",adu5bRelV14[0],adu5bRelV14[1],adu5bRelV14[2]);
    }
    else {
	eString=configErrorString (kvpStatus) ;
	syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
    }
    kvpReset();
    kvpStatus = configLoad ("GPSd.config","sourceList") ;    
    if(kvpStatus == CONFIG_E_OK) {
      //Stuff for GPS Phi Masking
      enableGpsPhiMasking=kvpGetInt("enableGpsPhiMasking",0);
      phiMaskingPeriod=kvpGetInt("phiMaskingPeriod",0);
      numSources=kvpGetInt("numSources",1);
      
      tempNum=MAX_PHI_SOURCES;	       
      kvpStatus = kvpGetFloatArray("sourceLats",sourceLats,&tempNum);
      if(kvpStatus!=KVP_E_OK) {
	syslog(LOG_WARNING,"kvpGetFloatArray(%s): sourceLats",
	       kvpErrorString(kvpStatus));
	fprintf(stderr,"kvpGetFloatArray(%s): sourceLats\n",
		kvpErrorString(kvpStatus));
      }
      
      tempNum=MAX_PHI_SOURCES;	       
      kvpStatus = kvpGetFloatArray("sourceLongs",sourceLongs,&tempNum);
      if(kvpStatus!=KVP_E_OK) {
	syslog(LOG_WARNING,"kvpGetFloatArray(%s): sourceLongs",
	       kvpErrorString(kvpStatus));
	fprintf(stderr,"kvpGetFloatArray(%s): sourceLongs\n",
		kvpErrorString(kvpStatus));
      }
      
      tempNum=MAX_PHI_SOURCES;	       
      kvpStatus = kvpGetFloatArray("sourceAlts",sourceAlts,&tempNum);
      if(kvpStatus!=KVP_E_OK) {
	syslog(LOG_WARNING,"kvpGetFloatArray(%s): sourceAlts",
	       kvpErrorString(kvpStatus));
	fprintf(stderr,"kvpGetFloatArray(%s): sourceAlts\n",
		kvpErrorString(kvpStatus));
      }      
      
      tempNum=MAX_PHI_SOURCES;	       
      kvpStatus = kvpGetFloatArray("sourceDistKm",sourceDistKm,&tempNum);
      if(kvpStatus!=KVP_E_OK) {
	syslog(LOG_WARNING,"kvpGetFloatArray(%s): sourceDistKm",
	       kvpErrorString(kvpStatus));
	fprintf(stderr,"kvpGetFloatArray(%s): sourceDistKm\n",
		kvpErrorString(kvpStatus));
      }      
      
      tempNum=MAX_PHI_SOURCES;	       
      kvpStatus = kvpGetIntArray("sourcePhiWidth",sourcePhiWidth,&tempNum);
      if(kvpStatus!=KVP_E_OK) {
	syslog(LOG_WARNING,"kvpGetFloatArray(%s): sourcePhiWidth",
	       kvpErrorString(kvpStatus));
	fprintf(stderr,"kvpGetFloatArray(%s): sourcePhiWidth\n",
		kvpErrorString(kvpStatus));
      } 
    }
    else {
      eString=configErrorString (kvpStatus) ;
      syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
    } 
    return kvpStatus;
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

    retVal=openGpsDevice(ADU5B_DEV_NAME);	
    if(retVal<=0) {
	syslog(LOG_ERR,"Couldn't open: %s\n",ADU5B_DEV_NAME);
	if(printToScreen) printf("Couldn't open: %s\n",ADU5B_DEV_NAME);
	handleBadSigs(1);
    }
    else fdAdu5B=retVal;
    
    //    printf("%s %s %s\n",G12A_DEV_NAME,ADU5A_DEV_NAME,ADU5B_DEV_NAME);
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

    
    if(g12IniReset) {
      strcat(tempCommand,"$PASHS,INI,5,4,1\n");
      retVal=write(fdG12, tempCommand, strlen(tempCommand));
      if(retVal<0) {
	  syslog(LOG_ERR,"Unable to write to G12 Serial port\n, write: %s",
		 strerror(errno));
	  fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
		  strerror(errno));
	  
      }
      else {
	  syslog(LOG_INFO,"Sent $PASHS,INI to G12 serial port");
	  printf("Sent $PASHS,INI to G12 serial port\n");
	  
      }
      sleep(20);
    }


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


    strcat(g12Command,"$PASHQ,PRT\n");
    strcat(g12Command,"$PASHQ,RIO\n");
//    strcat(g12Command,"$PASHQ,PRT,B\n");
    sprintf(tempCommand,"$PASHS,ELM,%d\n",g12ElevationMask);
    strcat(g12Command,tempCommand);

    sprintf(tempCommand,"$PASHS,NME,ALL,%c,OFF\n",dataPort);
    strcat(g12Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,ALL,%c,OFF\n",ntpPort);
    strcat(g12Command,tempCommand);
    strcat(g12Command, "$PASHS,LTZ,0,0\n");
    strcat(g12Command, "$PASHS,UTS,ON\n");
    sprintf(tempCommand,"$PASHS,NME,ZDA,%c,ON,%d\n",dataPort,g12ZdaPeriod);
    strcat(g12Command,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,GGA,%c,ON,%d\r\n",dataPort,g12GgaPeriod);
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
      fprintf(stderr,"G12:\n%s\n%s\nLength: %d\n",G12A_DEV_NAME,g12Command,(int)strlen(g12Command));
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
/*! Try to read ADU5A */
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
//    printf("Check ADU5A got retVal %d\n",retVal);
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
		  if((retVal-i)<50) {
		    processAdu5Output(adu5Output,adu5OutputLength,1,1); //1 for ADU5A
		  }
		  else {
		    processAdu5Output(adu5Output,adu5OutputLength,0,1); //1 for ADU5A
		  }

		
		}
		adu5OutputLength=0;
		lastStar=-10;
	    }
	    tempData[i]=0;
	}
	
    }
    else if(retVal<0) {
	syslog(LOG_ERR,"ADU5A read error: %s",strerror(errno));
    }
    return retVal;
}


int checkAdu5B()
/*! Try to read ADU5B */
{
//    printf("Here\n");
    // Working variables
    char tempData[ADU5_DATA_SIZE];
    int retVal,i;
// Data stuff for ADU5
    static char adu5Output[ADU5_DATA_SIZE]="";
    static int adu5OutputLength=0;
    static int lastStar=-10;
    retVal=isThereDataNow(fdAdu5B);
    usleep(5);
//    printf("Check ADU5B got retVal %d\n",retVal);
    if(retVal!=1) return 0;
    retVal=read(fdAdu5B, tempData, ADU5_DATA_SIZE);
    if(retVal>0) {
	for(i=0; i < retVal; i++) {
	    if(tempData[i]=='*') {
		lastStar=adu5OutputLength;
	    }
//	    printf("%c %d %d\n",tempData[i],adu5OutputLength,lastStar);
	    adu5Output[adu5OutputLength++]=tempData[i];

	    if(adu5OutputLength==lastStar+3) {
		if(adu5OutputLength) {
		  if((retVal-i)<50) {
		    processAdu5Output(adu5Output,adu5OutputLength,1,2); //2 for ADU5B
		  }
		  else {
		    processAdu5Output(adu5Output,adu5OutputLength,0,2); //2 for ADU5B
		  }
		}
		adu5OutputLength=0;
		lastStar=-10;
	    }
	    tempData[i]=0;
	}
	
    }
    else if(retVal<0) {
	syslog(LOG_ERR,"ADU5B read error: %s",strerror(errno));
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
      processGpzdaString(gpsString,gpsLength,latestData,0);
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
	if(!strcmp(subString,"ACK")) {
	  processAckString(gpsString,gpsLength,0);
	}
	if(!strcmp(subString,"NAK")) {
	  processNakString(gpsString,gpsLength,0);
	}
	if(!strcmp(subString,"RIO")) {
	  processRioString(gpsString,gpsLength,0);
	}

	
    }


}


void processGpzdaString(char *gpsString, int gpsLength, int latestData, int fromAdu5) 
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


void processGpvtgString(char *gpsString, int gpsLength, int fromAdu5) {
  static int telemCount[3]={0};
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
  
  
  
  if(fromAdu5==1) {
      
      fillGenericHeader(&theVtg,PACKET_GPS_ADU5_VTG|PACKET_FROM_ADU5A,sizeof(GpsAdu5VtgStruct_t));
    //Write file and link for sipd
    telemCount[fromAdu5]++;
    if(telemCount[fromAdu5]>=adu5aVtgTelemEvery) {
      sprintf(theFilename,"%s/vtg_%d.dat",ADU5A_VTG_TELEM_DIR,theVtg.unixTime);
      retVal=writeStruct(&theVtg,theFilename,sizeof(GpsAdu5VtgStruct_t));  
      retVal=makeLink(theFilename,ADU5A_VTG_TELEM_LINK_DIR);  
      telemCount[fromAdu5]=0;
    }
    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theVtg,sizeof(GpsAdu5VtgStruct_t),
			 theVtg.unixTime,&adu5aVtgWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }
  }
  else if(fromAdu5==2) {
      fillGenericHeader(&theVtg,PACKET_GPS_ADU5_VTG|PACKET_FROM_ADU5B,sizeof(GpsAdu5VtgStruct_t));
    //Write file and link for sipd
    telemCount[fromAdu5]++;
    if(telemCount[fromAdu5]>=adu5bVtgTelemEvery) {
      sprintf(theFilename,"%s/vtg_%d.dat",ADU5B_VTG_TELEM_DIR,theVtg.unixTime);
      retVal=writeStruct(&theVtg,theFilename,sizeof(GpsAdu5VtgStruct_t));  
      retVal=makeLink(theFilename,ADU5B_VTG_TELEM_LINK_DIR);  
      telemCount[fromAdu5]=0;
    }
    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theVtg,sizeof(GpsAdu5VtgStruct_t),
			 theVtg.unixTime,&adu5bVtgWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }
  }
    
}



void processGpggaString(char *gpsString, int gpsLength, int fromAdu5) {
  static int telemCount[3]={0};
  char gpsCopy[ADU5_DATA_SIZE];
  char *subString;   
  char theFilename[FILENAME_MAX];
  int retVal;
  GpsGgaStruct_t theGga;
  struct timeval timeStruct;

  int hour,minute,second,subSecond;
  char eastOrWest=0,northOrSouth=0;
  int latDeg=0,longDeg=0;
  float latMin=0,longMin=0;
  int numSats=0,posFixType=0,baseStationId=0;

  //Zero array and copy string
  bzero(&theGga,sizeof(GpsGgaStruct_t));  
  strncpy(gpsCopy,gpsString,gpsLength);
  //    sprintf(gpsCopy,"$GPGGA,123456.12,1234.123456,N,12345.123456,W,2,12,12.1,12345.12,M,-12.1,M,123,1234*7F");
  
  //Get unix Timestamp
  gettimeofday(&timeStruct,NULL);
  theGga.unixTime=timeStruct.tv_sec;
  theGga.unixTimeUs=timeStruct.tv_usec;
  theGga.gHdr.code=PACKET_GPS_GGA;
  //Process string
  subString = strtok (gpsCopy,"*");
  sscanf(subString,"$GPGGA,%02d%02d%02d.%02d,%02d%f,%c,%03d%f,%c,%d,%d,%f,%f,M,%f,M,%f,%d",
	 &hour,&minute,&second,&subSecond,
	 &latDeg,&latMin,&northOrSouth,&longDeg,&longMin,&eastOrWest,
	 &posFixType,&numSats,&(theGga.hdop),
	 &(theGga.altitude),&(theGga.geoidSeparation),&(theGga.ageOfCalc),
	 &baseStationId);

  theGga.posFixType=posFixType;
  theGga.numSats=numSats;
  theGga.baseStationId=baseStationId;
  
  //Process Position
  theGga.latitude=(float)latDeg;		
  theGga.latitude+=(latMin/60.);
  if(northOrSouth=='S') theGga.latitude*=-1;
  theGga.longitude=(float)longDeg;		
  theGga.longitude+=(longMin/60.);
  if(eastOrWest=='W') theGga.longitude*=-1;
    
  //Process Time
  //    printf("Time:\n%d\n%d\n%d\n%d\n",hour,minute,second,subSecond);
  theGga.timeOfDay=(float)(10*subSecond);
  theGga.timeOfDay+=(float)(1000*second);
  theGga.timeOfDay+=(float)(1000*60*minute);
  theGga.timeOfDay+=(float)(1000*60*60*hour);  

  /*     printf("unixTime: %d\ntimeOfDay: %d\nnumSats: %d\nlatitude: %f\nlongitude: %f\naltitute: %f\ngeoidSep: %f\n",  */
/* 	   theGga.unixTime, */
/*         theGga.numSats, */
/* 	   theGga.timeOfDay, */
/* 	   theGga.latitude, */
/* 	   theGga.longitude, */
/* 	   theGga.altitude, */
/* 	   theGga.geoidSeparation); */
/*     exit(0); */
  
  if(fromAdu5==0) {
      
  fillGenericHeader(&theGga,PACKET_GPS_GGA|PACKET_FROM_G12,sizeof(GpsGgaStruct_t));
    //Write file and link for sipd
    telemCount[fromAdu5]++;
    if(telemCount[fromAdu5]>=g12GgaTelemEvery) {
      sprintf(theFilename,"%s/gga_%d.dat",G12_GGA_TELEM_DIR,theGga.unixTime);
      retVal=writeStruct(&theGga,theFilename,sizeof(GpsGgaStruct_t));  
      retVal=makeLink(theFilename,G12_GGA_TELEM_LINK_DIR);  
      telemCount[fromAdu5]=0;
    }
    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theGga,sizeof(GpsGgaStruct_t),
			 theGga.unixTime,&g12GgaWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }
  }
  else if(fromAdu5==1) {
      fillGenericHeader(&theGga,PACKET_GPS_GGA|PACKET_FROM_ADU5A,sizeof(GpsGgaStruct_t));
    //Write file and link for sipd
    telemCount[fromAdu5]++;
    if(telemCount[fromAdu5]>=adu5aGgaTelemEvery) {
      sprintf(theFilename,"%s/gga_%d.dat",ADU5A_GGA_TELEM_DIR,theGga.unixTime);
      retVal=writeStruct(&theGga,theFilename,sizeof(GpsGgaStruct_t));  
      retVal=makeLink(theFilename,ADU5A_GGA_TELEM_LINK_DIR);  
      telemCount[fromAdu5]=0;
    }
    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theGga,sizeof(GpsGgaStruct_t),
			 theGga.unixTime,&adu5aGgaWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }
  }
  else if(fromAdu5==2) {
      fillGenericHeader(&theGga,PACKET_GPS_GGA|PACKET_FROM_ADU5B,sizeof(GpsGgaStruct_t));
    //Write file and link for sipd
    telemCount[fromAdu5]++;
    if(telemCount[fromAdu5]>=adu5bGgaTelemEvery) {
      sprintf(theFilename,"%s/gga_%d.dat",ADU5B_GGA_TELEM_DIR,theGga.unixTime);
      retVal=writeStruct(&theGga,theFilename,sizeof(GpsGgaStruct_t));  
      retVal=makeLink(theFilename,ADU5B_GGA_TELEM_LINK_DIR);  
      telemCount[fromAdu5]=0;
    }
    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theGga,sizeof(GpsGgaStruct_t),
			 theGga.unixTime,&adu5bGgaWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }
  }
    
}


void processPosString(char *gpsString, int gpsLength) {
  static int telemCount=0;
    char gpsCopy[ADU5_DATA_SIZE];
    char *subString;   
    char theFilename[FILENAME_MAX];
    int retVal;

    int hour,minute,second,subSecond;
    int posType;
    char eastOrWest=0,northOrSouth=0;
    int latDeg=0,longDeg=0;
    float latMin=0,longMin=0;
    //    float reserved;
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
    sscanf(subString,"$PASHR,POS,%d,%u,%02d%02d%02d.%02d,%02d%f,%c,%03d%f,%c,%f,,%f,%f,%f,%f,%f,%f,%f,%s",
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
  /*   printf("Time:\t %d\t%d\t%d\nNum Sats:\t%d\nPosition:\t%f\t%f\t%f\nCourse/Speed:\t%f\t%f\t%f\nDOP:\t\t%f\t%f\t%f\t%f\n", */
/* 	   thePos.unixTime,thePos.unixTimeUs,thePos.timeOfDay, */
/* 	   thePos.numSats, */
/* 	   thePos.latitude,thePos.longitude,thePos.altitude, */
/* 	   thePos.trueCourse,thePos.speedInKnots,thePos.verticalVelocity, */
/* 	   thePos.pdop,thePos.hdop,thePos.vdop,thePos.tdop); */
/*     exit(0); */


    fillGenericHeader(&thePos,PACKET_GPS_G12_POS,sizeof(GpsG12PosStruct_t));

    telemCount++;
    if(telemCount>=g12PosTelemEvery) {
      //Write file and link for sipd
      sprintf(theFilename,"%s/pos_%d.dat",G12_POS_TELEM_DIR,thePos.unixTime);
      retVal=writeStruct(&thePos,theFilename,sizeof(GpsG12PosStruct_t));  
      retVal=makeLink(theFilename,G12_POS_TELEM_LINK_DIR);  
      telemCount=0;
    }

    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&thePos,sizeof(GpsG12PosStruct_t),
			 thePos.unixTime,&g12PosWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }


}


void processAdu5Output(char *tempBuffer, int length, int latestData, int fromAdu5)
/* Processes each ADU5 output string and acts accordingly */
{  
    char gpsString[ADU5_DATA_SIZE];
    char gpsCopy[ADU5_DATA_SIZE];
    int gpsLength=0;
    int count=0;
    char *subString;
    char gpsDes[3]={' ','A','B'};
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
    if(printToScreen) printf("ADU5%c:\t%s\t%d\n",gpsDes[fromAdu5],gpsString,gpsLength);
    count=0;
    subString = strtok (gpsCopy,",");
    if(!strcmp(subString,"$GPPAT")) {
//	printf("Got $GPPAT\n");
      processGppatString(gpsString,gpsLength,fromAdu5);
    }
    else if(!strcmp(subString,"$GPZDA")) {
      //	printf("Got $GPZDA\n");
      processGpzdaString(gpsString,gpsLength,latestData,fromAdu5);
    }
    else if(!strcmp(subString,"$GPVTG")) {
//	printf("Got $GPZDA\n");
      processGpvtgString(gpsString,gpsLength,fromAdu5);
    }
    else if(!strcmp(subString,"$PASHR")) {
	//Maybe have TTT
	subString = strtok (NULL, " ,.*");
	if(!strcmp(subString,"TTT")) {
//	    printf("Got %s\t",subString);
//	    printf("Got TTT\n");
	    processTttString(gpsString,gpsLength,fromAdu5);
	}
	if(!strcmp(subString,"SA4")) {
//	    printf("And got Sat\n");
	    if(fromAdu5==1)
		processAdu5aSatString(gpsString,gpsLength,0);
	    else if(fromAdu5==2)
		processAdu5bSatString(gpsString,gpsLength,0);
	}
	if(!strcmp(subString,"SAT")) {
//	    printf("And got Sat\n");
	    if(fromAdu5==1)
		processAdu5aSatString(gpsString,gpsLength,1);
	    else if(fromAdu5==2)
		processAdu5bSatString(gpsString,gpsLength,1);
	}
	if(!strcmp(subString,"ACK")) {
	  processAckString(gpsString,gpsLength,fromAdu5);
	}
	if(!strcmp(subString,"NAK")) {
	  processNakString(gpsString,gpsLength,fromAdu5);
	}
	if(!strcmp(subString,"RIO")) {
	  processRioString(gpsString,gpsLength,fromAdu5);
	}
	if(!strcmp(subString,"BIT")) {
	  processBitString(gpsString,gpsLength,fromAdu5);
	}
	if(!strcmp(subString,"TST")) {
	  processTstString(gpsString,gpsLength,fromAdu5);
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
//    printf("%d %d\n",rawtime,gpstime);
//    printf("Seconds: %d %d\n",oldSec,sec);
//    printf("***************************\n%s%s**********************\n",unixString,otherString);
    theTTT.unixTime=gpstime;
    theTTT.subTime=subSecond;
    theTTT.fromAdu5=fromAdu5;

    //Write file for eventd
    sprintf(filename,"%s/gps_%u_%u.dat",GPSD_SUBTIME_DIR,theTTT.unixTime,theTTT.subTime);
    writeStruct(&theTTT,filename,sizeof(GpsSubTime_t));
    retVal=makeLink(filename,GPSD_SUBTIME_LINK_DIR);  

    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theTTT,sizeof(GpsSubTime_t),
			 theTTT.unixTime,&gpsTttWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }        
}


void processAckString(char *gpsString, int length, int fromAdu5)
{
  //Assume it's fine
  startStruct.ackCount[fromAdu5]++;
}

void processNakString(char *gpsString, int length, int fromAdu5)
{
  //Assume it's fine
  startStruct.nakCount[fromAdu5]++;
}

void processRioString(char *gpsString, int length, int fromAdu5)
{
  int retVal=0;
  if(fromAdu5==0) {
    retVal=strncmp(gpsString,G12_RIO,length);
  }
  else {
    retVal=strncmp(gpsString,ADU5_RIO,length);
  }

  if (retVal==0) {
    startStruct.rioBitMask |= (1<<fromAdu5);
  }
}

void processBitString(char *gpsString, int length, int fromAdu5)
{
  char gpsCopy[ADU5_DATA_SIZE];//$PASHR,BIT,P,P,P,0,0.0*49	
  char *subString;
  strncpy(gpsCopy,gpsString,length);
  subString = strtok (gpsCopy,"*");
  char testChar[3];
  int sensCount=0;
  float percentBlock=0;
  sscanf(subString,"$PASHR,BIT,%c,%c,%c,%d,%f",
	 &testChar[0],&testChar[1],&testChar[2],&sensCount,&percentBlock);
  if(testChar[0] != 'P')
    startStruct.tstBitMask |= (1 << (4*(fromAdu5-1)));
  if(testChar[1] != 'P')
    startStruct.tstBitMask |= (1 << (1+(4*(fromAdu5-1))));
  if(testChar[2] != 'P')
    startStruct.tstBitMask |= (1 << (2+(4*(fromAdu5-1))));
  if(sensCount != 0) {
    startStruct.rioBitMask |= (1 << (4+fromAdu5));
  }

  //  printf("Testing %c %c %c %d\n",testChar[0],testChar[1],testChar[2],sensCount);
  //  printf("Testing %#x\n",sta

}

void processTstString(char *gpsString, int length, int fromAdu5)
{
  int retVal=strncmp(gpsString,"$PASHR,TST,000*3B",length);
  //  printf("Testing %d\n%s\n",retVal,gpsString);
  if (retVal!=0) {
    startStruct.tstBitMask |= (1<<(3 +(4*fromAdu5-1)));
  }
}

void processG12SatString(char *gpsString, int gpsLength) {
  static int telemCount=0;
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
    
    telemCount++;
    if(telemCount>=g12SatTelemEvery) {
      //Write file and link for sipd
      sprintf(theFilename,"%s/sat_%d.dat",G12_SAT_TELEM_DIR,theSat.unixTime);
      retVal=writeStruct(&theSat,theFilename,sizeof(GpsG12SatStruct_t));  
      retVal=makeLink(theFilename,G12_SAT_TELEM_LINK_DIR);  
      telemCount=0;
    }

    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&theSat,sizeof(GpsG12SatStruct_t),
			 theSat.unixTime,&g12SatWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }
  
}



void processAdu5aSatString(char *gpsString, int gpsLength,  int isSat) {
    int fromAdu5=1;
    static int telemCount[3]={0};
  char gpsCopy[ADU5_DATA_SIZE];
  char *subString;
  char theFilename[FILENAME_MAX];
  int retVal;
  int count=0;
  int doingSat=0;
  int doingAnt=-1;
  int satInd=0;
  static int satellitePrns[MAX_SATS]={0};
  static int satelliteAzimuths[MAX_SATS]={0};
  static int satelliteElevations[MAX_SATS]={0};
  static int totalSats=0;
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

  if(isSat) {
      memset(satellitePrns,0,sizeof(int)*MAX_SATS);
      memset(satelliteAzimuths,0,sizeof(int)*MAX_SATS);
      memset(satelliteElevations,0,sizeof(int)*MAX_SATS);
      totalSats=0;
//  $PASHR,SAT,12,25,319,47,47,U,16,041,32,43,U,20,201,20,36,U,06,087,42,43,U,23,151,88,50,U,03,110,49,45,U,19,147,33,37,U,27,297,25,44,U,07,299,32,45,U,13,322,54,48,U,32,181,09,36,U,51,201,51,46,U*18
    subString = strtok (gpsCopy,",");    
    while (subString != NULL) {
//	printf("%d\t%s\n",count,subString);
	
	switch(count) {
	    case 0:		
//		theSat.numSats=atoi(subString);
		break;
	    case 1:
		satellitePrns[doingSat]=atoi(subString);
		break;
	    case 2:
		satelliteAzimuths[doingSat]=atoi(subString);
		break;
	    case 3:
		satelliteElevations[doingSat]=atoi(subString);
		break;
	    case 4:
//		theSat.sat[doingSat].snr=atoi(subString);
		break;
	    case 5:
//		theSat.sat[doingSat].flag=subString[0];
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
    totalSats=doingSat;

  }
  else {
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
		  for(satInd=0;satInd<totalSats;satInd++) {
		      if(theSat.sat[doingAnt][doingSat].prn == satellitePrns[satInd]) {
			  
			  theSat.sat[doingAnt][doingSat].azimuth=satelliteAzimuths[satInd];
			  theSat.sat[doingAnt][doingSat].elevation=satelliteElevations[satInd];
			  break;
		      }
		  }

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

	      
	      if(fromAdu5==1) {
		  fillGenericHeader(&theSat,PACKET_GPS_ADU5_SAT|PACKET_FROM_ADU5A,sizeof(GpsAdu5SatStruct_t));
		  telemCount[fromAdu5]++;
		  if(telemCount[fromAdu5]>=adu5aSatTelemEvery) {
		      //Write file and link for sipd
		      sprintf(theFilename,"%s/sat_adu5_%d.dat",ADU5A_SAT_TELEM_DIR,theSat.unixTime);
		      retVal=writeStruct(&theSat,theFilename,sizeof(GpsAdu5SatStruct_t));  
		      retVal=makeLink(theFilename,ADU5A_SAT_TELEM_LINK_DIR);  
		      telemCount[fromAdu5]=0;
		  }
		  
		  //Write file to main disk
		  retVal=cleverHkWrite((unsigned char*)&theSat,sizeof(GpsAdu5SatStruct_t),
			     theSat.unixTime,&adu5aSatWriter);
		  if(retVal<0) {
		      //Had an error
		      //Do something
		  }	    
	      }
	      else if(fromAdu5==2) {
		  fillGenericHeader(&theSat,PACKET_GPS_ADU5_SAT|PACKET_FROM_ADU5B,sizeof(GpsAdu5SatStruct_t));
		  telemCount[fromAdu5]++;
		  if(telemCount[fromAdu5]>=adu5bSatTelemEvery) {
		      //Write file and link for sipd
		      sprintf(theFilename,"%s/sat_adu5_%d.dat",ADU5B_SAT_TELEM_DIR,theSat.unixTime);
		      retVal=writeStruct(&theSat,theFilename,sizeof(GpsAdu5SatStruct_t));  
		      retVal=makeLink(theFilename,ADU5B_SAT_TELEM_LINK_DIR);  
		      telemCount[fromAdu5]=0;
		  }
		  
		  //Write file to main disk
		  retVal=cleverHkWrite((unsigned char*)&theSat,sizeof(GpsAdu5SatStruct_t),
				       theSat.unixTime,&adu5bSatWriter);
		  if(retVal<0) {
		      //Had an error
		      //Do something
		  }	    
		  
	      }
	  }
      }
  }

}




void processAdu5bSatString(char *gpsString, int gpsLength,  int isSat) {
    int fromAdu5=2;
  static int telemCount[3]={0};
  char gpsCopy[ADU5_DATA_SIZE];
  char *subString;
  char theFilename[FILENAME_MAX];
  int retVal;
  int count=0;
  int doingSat=0;
  int doingAnt=-1;
  int satInd=0;
  static int satellitePrns[MAX_SATS]={0};
  static int satelliteAzimuths[MAX_SATS]={0};
  static int satelliteElevations[MAX_SATS]={0};
  static int totalSats=0;
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

  if(isSat) {
      memset(satellitePrns,0,sizeof(int)*MAX_SATS);
      memset(satelliteAzimuths,0,sizeof(int)*MAX_SATS);
      memset(satelliteElevations,0,sizeof(int)*MAX_SATS);
      totalSats=0;
//  $PASHR,SAT,12,25,319,47,47,U,16,041,32,43,U,20,201,20,36,U,06,087,42,43,U,23,151,88,50,U,03,110,49,45,U,19,147,33,37,U,27,297,25,44,U,07,299,32,45,U,13,322,54,48,U,32,181,09,36,U,51,201,51,46,U*18
    subString = strtok (gpsCopy,",");    
    while (subString != NULL) {
//	printf("%d\t%s\n",count,subString);
	
	switch(count) {
	    case 0:		
//		theSat.numSats=atoi(subString);
		break;
	    case 1:
		satellitePrns[doingSat]=atoi(subString);
		break;
	    case 2:
		satelliteAzimuths[doingSat]=atoi(subString);
		break;
	    case 3:
		satelliteElevations[doingSat]=atoi(subString);
		break;
	    case 4:
//		theSat.sat[doingSat].snr=atoi(subString);
		break;
	    case 5:
//		theSat.sat[doingSat].flag=subString[0];
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
    totalSats=doingSat;

  }
  else {
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
		  for(satInd=0;satInd<totalSats;satInd++) {
		      if(theSat.sat[doingAnt][doingSat].prn == satellitePrns[satInd]) {
			  
			  theSat.sat[doingAnt][doingSat].azimuth=satelliteAzimuths[satInd];
			  theSat.sat[doingAnt][doingSat].elevation=satelliteElevations[satInd];
			  break;
		      }
		  }

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

	      
	      if(fromAdu5==1) {
		  fillGenericHeader(&theSat,PACKET_GPS_ADU5_SAT|PACKET_FROM_ADU5A,sizeof(GpsAdu5SatStruct_t));
		  telemCount[fromAdu5]++;
		  if(telemCount[fromAdu5]>=adu5aSatTelemEvery) {
		      //Write file and link for sipd
		      sprintf(theFilename,"%s/sat_adu5_%d.dat",ADU5A_SAT_TELEM_DIR,theSat.unixTime);
		      retVal=writeStruct(&theSat,theFilename,sizeof(GpsAdu5SatStruct_t));  
		      retVal=makeLink(theFilename,ADU5A_SAT_TELEM_LINK_DIR);  
		      telemCount[fromAdu5]=0;
		  }
		  
		  //Write file to main disk
		  retVal=cleverHkWrite((unsigned char*)&theSat,sizeof(GpsAdu5SatStruct_t),
			     theSat.unixTime,&adu5aSatWriter);
		  if(retVal<0) {
		      //Had an error
		      //Do something
		  }	    
	      }
	      else if(fromAdu5==2) {
		  fillGenericHeader(&theSat,PACKET_GPS_ADU5_SAT|PACKET_FROM_ADU5B,sizeof(GpsAdu5SatStruct_t));
		  telemCount[fromAdu5]++;
		  if(telemCount[fromAdu5]>=adu5bSatTelemEvery) {
		      //Write file and link for sipd
		      sprintf(theFilename,"%s/sat_adu5_%d.dat",ADU5B_SAT_TELEM_DIR,theSat.unixTime);
		      retVal=writeStruct(&theSat,theFilename,sizeof(GpsAdu5SatStruct_t));  
		      retVal=makeLink(theFilename,ADU5B_SAT_TELEM_LINK_DIR);  
		      telemCount[fromAdu5]=0;
		  }
		  
		  //Write file to main disk
		  retVal=cleverHkWrite((unsigned char*)&theSat,sizeof(GpsAdu5SatStruct_t),
				       theSat.unixTime,&adu5bSatWriter);
		  if(retVal<0) {
		      //Had an error
		      //Do something
		  }	    
		  
	      }
	  }
      }
  }

}



void processGppatString(char *gpsString, int gpsLength, int fromAdu5) {
  static int telemCount[3]={0};
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
  sscanf(subString,"$GPPAT,%02d%02d%02d.%02d,%02d%f,%c,%03d%f,%c,%f,%f,%f,%f,%f,%f,%u",
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
/*     printf("unixTime: %d\ntimeOfDay: %d\nheading: %lf\npitch: %lf\nroll: %lf\nmrms: %lf\nbrms: %lf\nattFlag: %d\nlatitude: %lf\nlongitude: %lf\naltitute: %lf\n",  */
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
    



  if(fromAdu5==1) {
      fillGenericHeader(&thePat,PACKET_GPS_ADU5_PAT|PACKET_FROM_ADU5A,sizeof(GpsAdu5PatStruct_t));
    telemCount[fromAdu5]++;
    if(telemCount[fromAdu5]>=adu5aPatTelemEvery) {
      //Write file and link for sipd
      sprintf(theFilename,"%s/pat_%d.dat",ADU5A_PAT_TELEM_DIR,thePat.unixTime);
      retVal=writeStruct(&thePat,theFilename,sizeof(GpsAdu5PatStruct_t));  
      retVal=makeLink(theFilename,ADU5A_PAT_TELEM_LINK_DIR);  
      telemCount[fromAdu5]=0;
    }

    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&thePat,sizeof(GpsAdu5PatStruct_t),
			 thePat.unixTime,&adu5aPatWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }	 
  }
  else if(fromAdu5==2) {
      fillGenericHeader(&thePat,PACKET_GPS_ADU5_PAT|PACKET_FROM_ADU5B,sizeof(GpsAdu5PatStruct_t));
    telemCount[fromAdu5]++;
    if(telemCount[fromAdu5]>=adu5bPatTelemEvery) {
      //Write file and link for sipd
      sprintf(theFilename,"%s/pat_%d.dat",ADU5B_PAT_TELEM_DIR,thePat.unixTime);
      retVal=writeStruct(&thePat,theFilename,sizeof(GpsAdu5PatStruct_t));  
      retVal=makeLink(theFilename,ADU5B_PAT_TELEM_LINK_DIR);  
      telemCount[fromAdu5]=0;
    }

    //Write file to main disk
    retVal=cleverHkWrite((unsigned char*)&thePat,sizeof(GpsAdu5PatStruct_t),
			 thePat.unixTime,&adu5bPatWriter);
    if(retVal<0) {
	//Had an error
	//Do something
    }	 
  }
  checkPhiSources(&thePat);
    
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
      syslog(LOG_ERR,"Problem updating clock, %d: %s ",(int)gpsRawTime,strerror(errno));
	fprintf(stderr,"Problem updating clock, %d: %s ",(int)gpsRawTime,strerror(errno));
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




int setupAdu5A()
/*! Initializes the ADU5A with the correct settings */
{
    char adu5aCommand[2048]="";
    char tempCommand[128]="";
    int retVal;
    if(adu5aIniReset) {
	strcat(tempCommand,"$PASHS,INI\r\n");
	retVal=write(fdAdu5A, tempCommand, strlen(tempCommand));
	if(retVal<0) {
	    syslog(LOG_ERR,"Unable to write to ADU5A Serial port\n, write: %s",
		   strerror(errno));
	    if(printToScreen)
		fprintf(stderr,"Unable to write to ADU5A Serial port\n");
	}
	else {
	    syslog(LOG_INFO,"Sent $PASHS,INI to ADU5A serial port");
	    if(printToScreen)
		printf("Sent $PASHS,INI  to ADU5A serial port\n");
	    sleep(20);
	}
    }

    strcat(adu5aCommand,"$PASHQ,PRT\r\n");
    strcat(adu5aCommand,"$PASHQ,RIO\r\n");
    strcat(adu5aCommand,"$PASHQ,BIT\r\n");
    strcat(adu5aCommand,"$PASHQ,TST\r\n");
    sprintf(tempCommand,"$PASHS,ELM,%d\r\n",adu5aElevationMask); 
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,CYC,%1.3f\r\n",adu5aCycPhaseErrorThreshold);
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,MXM,%1.3f\r\n",adu5aMxmPhaseError);
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,MXB,%1.3f\r\n",adu5aMxbBaselineError);
    strcat(adu5aCommand,tempCommand);

    strcat(adu5aCommand,"$PASHS,KFP,ON\r\n");
    
    sprintf(tempCommand,"$PASHS,3DF,V12,%2.3f,%2.3f,%2.3f\r\n",
	    adu5aRelV12[0],adu5aRelV12[1],adu5aRelV12[2]);
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,V13,%2.3f,%2.3f,%2.3f\r\n",
	    adu5aRelV13[0],adu5aRelV13[1],adu5aRelV13[2]);
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,V14,%2.3f,%2.3f,%2.3f\r\n",
	    adu5aRelV14[0],adu5aRelV14[1],adu5aRelV14[2]);
    strcat(adu5aCommand,tempCommand);
    strcat(adu5aCommand,"$PASHS,NME,ALL,A,OFF\r\n");
    strcat(adu5aCommand,"$PASHS,NME,ALL,B,OFF\r\n");
    strcat(adu5aCommand,"$PASHS,3DF,ANG,10\r\n");
    strcat(adu5aCommand,"$PASHS,3DF,FLT,Y\r\n");
    sprintf(tempCommand,"$PASHS,NME,ZDA,A,ON,%d\r\n",adu5aZdaPeriod);
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,GGA,A,ON,%d\r\n",adu5aGgaPeriod);
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,PAT,A,ON,%2.2f",adu5aPatPeriod);
    strcat(tempCommand,"\r\n");
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,SA4,A,ON,%d\r\n",adu5aSatPeriod);
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,SAT,A,ON,%d\r\n",adu5aSatPeriod);
    strcat(adu5aCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,VTG,A,ON,%2.2f\r\n",adu5aVtgPeriod);
    strcat(adu5aCommand,tempCommand);
    strcat(adu5aCommand,"$PASHQ,PRT\r\n");
    if(adu5aEnableTtt) strcat(adu5aCommand,"$PASHS,NME,TTT,A,ON\r\n");

    if(printToScreen) printf("ADU5A:\n%s\n%s\n",ADU5A_DEV_NAME,adu5aCommand);
    retVal=write(fdAdu5A, adu5aCommand, strlen(adu5aCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to ADU5A Serial port\n, write: %s",
	       strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to ADU5A Serial port\n");
    }
    else {
	syslog(LOG_INFO,"Sent %d bytes to ADU5A serial port",retVal);
	if(printToScreen)
	    printf("Sent %d bytes to ADU5A serial port\n",retVal);
    }
    return retVal;
}


int setupAdu5B()
/*! Initializes the ADU5B with the correct settings */
{
    char adu5bCommand[2048]="";
    char tempCommand[128]="";
    int retVal;

    if(adu5bIniReset) {
	strcat(tempCommand,"$PASHS,INI\r\n");
	retVal=write(fdAdu5B, tempCommand, strlen(tempCommand));
	if(retVal<0) {
	    syslog(LOG_ERR,"Unable to write to ADU5B Serial port\n, write: %s",
		   strerror(errno));
	    if(printToScreen)
		fprintf(stderr,"Unable to write to ADU5B Serial port\n");
	}
	else {
	    syslog(LOG_INFO,"Sent $PASHS,INI to ADU5B serial port");
	    if(printToScreen)
		printf("Sent $PASHS,INI to ADU5B serial port\n");
	    sleep(20);
	}
    }

    strcat(adu5bCommand,"$PASHQ,PRT\r\n");
    strcat(adu5bCommand,"$PASHQ,RIO\r\n");
    strcat(adu5bCommand,"$PASHQ,BIT\r\n");
    strcat(adu5bCommand,"$PASHQ,TST\r\n");
    sprintf(tempCommand,"$PASHS,ELM,%d\r\n",adu5bElevationMask); 
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,CYC,%1.3f\r\n",adu5bCycPhaseErrorThreshold);
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,MXM,%1.3f\r\n",adu5bMxmPhaseError);
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,MXB,%1.3f\r\n",adu5bMxbBaselineError);
    strcat(adu5bCommand,tempCommand);
    strcat(adu5bCommand,"$PASHS,KFP,ON\r\n");    
    sprintf(tempCommand,"$PASHS,3DF,V12,%2.3f,%2.3f,%2.3f\r\n",
	    adu5bRelV12[0],adu5bRelV12[1],adu5bRelV12[2]);
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,V13,%2.3f,%2.3f,%2.3f\r\n",
	    adu5bRelV13[0],adu5bRelV13[1],adu5bRelV13[2]);
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,3DF,V14,%2.3f,%2.3f,%2.3f\r\n",
	    adu5bRelV14[0],adu5bRelV14[1],adu5bRelV14[2]);
    strcat(adu5bCommand,tempCommand);
    strcat(adu5bCommand,"$PASHS,NME,ALL,A,OFF\r\n");
    strcat(adu5bCommand,"$PASHS,NME,ALL,B,OFF\r\n");
    strcat(adu5bCommand,"$PASHS,3DF,ANG,10\r\n");
    strcat(adu5bCommand,"$PASHS,3DF,FLT,Y\r\n");
    sprintf(tempCommand,"$PASHS,NME,ZDA,A,ON,%d\r\n",adu5bZdaPeriod);
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,GGA,A,ON,%d\r\n",adu5bGgaPeriod);
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,PAT,A,ON,%2.2f",adu5bPatPeriod);
    strcat(tempCommand,"\r\n");
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,SA4,A,ON,%d\r\n",adu5bSatPeriod);
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,SAT,A,ON,%d\r\n",adu5bSatPeriod);
    strcat(adu5bCommand,tempCommand);
    sprintf(tempCommand,"$PASHS,NME,VTG,A,ON,%2.2f\r\n",adu5bVtgPeriod);
    strcat(adu5bCommand,tempCommand);
    strcat(adu5bCommand,"$PASHQ,PRT\r\n");
    if(adu5bEnableTtt) strcat(adu5bCommand,"$PASHS,NME,TTT,A,ON\r\n");

    if(printToScreen) printf("ADU5B:\n%s\n%s\n",ADU5B_DEV_NAME,adu5bCommand);
    retVal=write(fdAdu5B, adu5bCommand, strlen(adu5bCommand));
    if(retVal<0) {
	syslog(LOG_ERR,"Unable to write to ADU5B Serial port\n, write: %s",
	       strerror(errno));
	if(printToScreen)
	    fprintf(stderr,"Unable to write to ADU5B Serial port\n");
    }
    else {
	syslog(LOG_INFO,"Sent %d bytes to ADU5B serial port",retVal);
	if(printToScreen)
	    printf("Sent %d bytes to ADU5B serial port\n",retVal);
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

    //Adu5a Pat
    sprintf(adu5aPatWriter.relBaseName,"%s/adu5a/pat/",GPS_ARCHIVE_DIR);
    sprintf(adu5aPatWriter.filePrefix,"pat");
    adu5aPatWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5aPatWriter.currentFilePtr[diskInd]=0;

    //Adu5a Sat
    sprintf(adu5aSatWriter.relBaseName,"%s/adu5a/sat/",GPS_ARCHIVE_DIR);
    sprintf(adu5aSatWriter.filePrefix,"sat_adu5");
    adu5aSatWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5aSatWriter.currentFilePtr[diskInd]=0;

    //Adu5a Vtg
    sprintf(adu5aVtgWriter.relBaseName,"%s/adu5a/vtg/",GPS_ARCHIVE_DIR);
    sprintf(adu5aVtgWriter.filePrefix,"vtg");
    adu5aVtgWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5aVtgWriter.currentFilePtr[diskInd]=0;


    //Adu5a Gga
    sprintf(adu5aGgaWriter.relBaseName,"%s/adu5a/gga/",GPS_ARCHIVE_DIR);
    sprintf(adu5aGgaWriter.filePrefix,"gga");
    adu5aGgaWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5aGgaWriter.currentFilePtr[diskInd]=0;

    //Adu5b Pat
    sprintf(adu5bPatWriter.relBaseName,"%s/adu5b/pat/",GPS_ARCHIVE_DIR);
    sprintf(adu5bPatWriter.filePrefix,"pat");
    adu5bPatWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5bPatWriter.currentFilePtr[diskInd]=0;

    //Adu5b Sat
    sprintf(adu5bSatWriter.relBaseName,"%s/adu5b/sat/",GPS_ARCHIVE_DIR);
    sprintf(adu5bSatWriter.filePrefix,"sat_adu5");
    adu5bSatWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5bSatWriter.currentFilePtr[diskInd]=0;

    //Adu5b Vtg
    sprintf(adu5bVtgWriter.relBaseName,"%s/adu5b/vtg/",GPS_ARCHIVE_DIR);
    sprintf(adu5bVtgWriter.filePrefix,"vtg");
    adu5bVtgWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5bVtgWriter.currentFilePtr[diskInd]=0;

    //Adu5b Gga
    sprintf(adu5bGgaWriter.relBaseName,"%s/adu5b/gga/",GPS_ARCHIVE_DIR);
    sprintf(adu5bGgaWriter.filePrefix,"gga");
    adu5bGgaWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	adu5bGgaWriter.currentFilePtr[diskInd]=0;

    //GPS Ttt
    sprintf(gpsTttWriter.relBaseName,"%s/all/ttt/",GPS_ARCHIVE_DIR);
    sprintf(gpsTttWriter.filePrefix,"ttt");
    gpsTttWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	gpsTttWriter.currentFilePtr[diskInd]=0;

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


    //G12 Gga
    sprintf(g12GgaWriter.relBaseName,"%s/g12/gga/",GPS_ARCHIVE_DIR);
    sprintf(g12GgaWriter.filePrefix,"gga");
    g12GgaWriter.writeBitMask=hkDiskBitMask;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++)
	g12GgaWriter.currentFilePtr[diskInd]=0;
    

}    


void handleBadSigs(int sig)
{
   
    fprintf(stderr,"Received sig %d -- will exit immediately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immediately\n",sig); 
    closeHkFilesAndTidy(&adu5aPatWriter);
    closeHkFilesAndTidy(&adu5aSatWriter);
    closeHkFilesAndTidy(&adu5aVtgWriter); 
    closeHkFilesAndTidy(&adu5aGgaWriter); 
    closeHkFilesAndTidy(&adu5bPatWriter);
    closeHkFilesAndTidy(&adu5bSatWriter);
    closeHkFilesAndTidy(&adu5bVtgWriter);
    closeHkFilesAndTidy(&adu5bGgaWriter);
    closeHkFilesAndTidy(&gpsTttWriter);
    closeHkFilesAndTidy(&g12PosWriter);
    closeHkFilesAndTidy(&g12SatWriter);
    closeHkFilesAndTidy(&g12GgaWriter);
    unlink(GPSD_PID_FILE);
    syslog(LOG_INFO,"GPSd terminating");    
    exit(0);
}


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(GPSD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,GPSD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(GPSD_PID_FILE);
  return 0;
}

void writeStartTest(time_t startTime)
{
  int retVal;
  char theFilename[FILENAME_MAX];
  startStruct.unixTime=startTime;
  fillGenericHeader(&startStruct,PACKET_GPSD_START,sizeof(GpsdStartStruct_t));
  sprintf(theFilename,"%s/gps_%d.dat",REQUEST_TELEM_DIR,startStruct.unixTime);
  retVal=writeStruct(&startStruct,theFilename,sizeof(GpsdStartStruct_t));  
  retVal=makeLink(theFilename,REQUEST_TELEM_LINK_DIR);  
  retVal=simpleMultiDiskWrite(&startStruct,sizeof(GpsdStartStruct_t),startStruct.unixTime,STARTUP_ARCHIVE_DIR,"gpsd",hkDiskBitMask);

  printf("\n\nGPSd Start Test\n");
  printf("\tunixTime %u\n",startStruct.unixTime);
  printf("\tAcks:\t%d %d %d\n",startStruct.ackCount[0],startStruct.ackCount[1],startStruct.ackCount[2]);
  printf("\tNaks:\t%d %d %d\n",startStruct.nakCount[0],startStruct.nakCount[1],startStruct.nakCount[2]);
  printf("\tRIOmask:\t%#x\n",startStruct.rioBitMask);
  printf("\tTSTmask:\t%#x\n",startStruct.tstBitMask);
  
}


void checkPhiSources(GpsAdu5PatStruct_t *patPtr) {
  static time_t lastPhiUpdate=0;
  static unsigned short lastGpsPhiMask=0;
  unsigned short gpsPhiMask=0;
  int sourceIndex=0,phiIndex=0;
  int hotPhiSector[MAX_PHI_SOURCES]={0};
  double phiDist[MAX_PHI_SOURCES]={0};
  CommandStruct_t theCmd;
  theCmd.fromSipd=0;
    


  if(!enableGpsPhiMasking) return;
  if(patPtr->unixTime - lastPhiUpdate < phiMaskingPeriod) return;
  if(patPtr->heading<0 || patPtr->heading>360) return;
  if(patPtr->attFlag) return;
  double anitaLatLonAlt[3]={patPtr->latitude,patPtr->longitude,patPtr->altitude};
  double anitaHPR[3]={patPtr->heading,patPtr->pitch,patPtr->roll};
  double sourceLatLonAlt[3]={0};



  for(sourceIndex=0;sourceIndex<numSources;sourceIndex++) {
    sourceLatLonAlt[0]=sourceLats[sourceIndex];
    sourceLatLonAlt[1]=sourceLongs[sourceIndex];
    sourceLatLonAlt[2]=sourceAlts[sourceIndex];
    getSourcePhiAndDistance(anitaLatLonAlt,anitaHPR,sourceLatLonAlt,
			    &hotPhiSector[sourceIndex],&phiDist[sourceIndex]);
    
    if(phiDist[sourceIndex]<1e3*sourceDistKm[sourceIndex]) {
      //Got a source inside our horizon
      printf("Found source at %f %f %f --- Phi %d -- Dist %f\n",
	     sourceLatLonAlt[0],sourceLatLonAlt[1],sourceLatLonAlt[2],
	     hotPhiSector[sourceIndex],phiDist[sourceIndex]/1e3);
    }
    
    //Now need to add it to phiMask
    //It is either 1,3 (width=2),5 (width=3),7 (width=4),9 sectors wide
    for(phiIndex=0;phiIndex<PHI_SECTORS;phiIndex++) {
      if(getPhiSeparation(phiIndex,hotPhiSector[sourceIndex])<sourcePhiWidth[sourceIndex]) {
	gpsPhiMask |= (1<<phiIndex);
      }
    }       
  }

  if(gpsPhiMask|=lastGpsPhiMask) {
    //Wahey get to set a new mask
    syslog(LOG_INFO,"GPSd is going to update gpsPhiMask from %#x to %#x\n",
	   lastGpsPhiMask,gpsPhiMask);
    fprintf(stderr,"GPSd is going to update gpsPhiMask from %#x to %#x\n",
	    lastGpsPhiMask,gpsPhiMask);

    //How do we do it?

    memset(&theCmd,0,sizeof(CommandStruct_t));
    theCmd.numCmdBytes=10;
    theCmd.cmd[0]=ACQD_RATE_COMMAND;
    theCmd.cmd[1]=ACQD_RATE_SET_GPS_PHI_MASK;
    theCmd.cmd[2]=gpsPhiMask&0xff; //gps phi mask
    theCmd.cmd[3]=(gpsPhiMask&0xff00)>>8; //gps phi mask
    writeCommandAndLink(&theCmd);
    lastGpsPhiMask=gpsPhiMask;
  }
}
  
