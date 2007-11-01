/*! \file gpsSimulator.c
    \brief The program that opens up serial ports and simulates what the 
    GPS units would do
    
    October 2007  rjn@hep.ucl.ac.uk
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>


/* Flight soft includes */
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "serialLib/serialLib.h"
#include "utilLib/utilLib.h"
#include "socketLib/socketLib.h"
#include "includes/anitaStructures.h"

//Testing mode
//#define NO_SERIAL 1


// Forward declarations
int readConfigFile();
int openDevices();

//May reinstate these to send the acknoledgements
//int setupG12();
//int setupG12Ntp();
//int setupAdu5();

int serviceG12();
int serviceAdu5();
int addChecksum(char *gpsString,int gpsLength);
int checkChecksum(char *gpsString,int gpsLength);

//GPS Packet Sending Funcs
int sendG12GPZDA(unsigned int unixTime);
int sendG12Pos(unsigned int unixTime, float latitude, float longitude, float altitude);
int sendG12Sat();
int sendG12TTT(double tttTime);
int sendG12RMC(unsigned int unixTime);
int sendAdu5PAT(unsigned int unixTime, float latitude, float longitude, float altitude);
int sendAdu5Sat();
int sendAdu5TTT(double tttTime);
int sendAdu5VTG(unsigned int unixTime);
void sendLimitIsReached(int thisFd);

// Definitions
#define LEAP_SECONDS 14 //Need to check
#define G12_DATA_SIZE 1024
#define ADU5_DATA_SIZE 1024

// File desciptors for GPS serial ports
int fdG12,fdAdu5,fdG12Ntp;//,fdMag

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


//const char *G12_PORT="/dev/ttyUSB0";
//const char *G12_NTP_PORT="/dev/ttyUSB1";
//const char *ADU5_PORT="/dev/ttyUSB2";

char *G12_PORT="/tmp/ttyUSB0";
char *G12_NTP_PORT="/tmp/ttyUSB1";
char *ADU5_PORT="/tmp/ttyUSB2";


int justSendLimitIsReached=0;
float g12TttPeriod=0.5;
float adu5TttPeriod=0.5;

void printUsage(char *progName) {
  printf("Usage:\n\t%s\n",progName);
  printf("Command line options:\n");
  printf("\t-r <rate>\tSet event rate (i.e. TTT rate) in Hz\n");
  printf("\t-b\t\tEnable broken mode\n");
  printf("\t-s <rate>\tSet $LIMIT_IS_REACHED rate in Hz (enables broken mode)\n");
  printf("\t-h\t\tShow this message\n");
}

int main (int argc, char *argv[])
{
  float limitRate=0;
  float rate=4;
  char clswitch;
  if(argc>1) {
    //Got some command line args, we should do thingies
    while((clswitch = getopt(argc, argv, "hbr:s:")) != -1) {
      
      switch(clswitch) {
      case 'b':
	printf("Switching to broken mode\n");
	justSendLimitIsReached=1;
	break;	
      case 's':
	justSendLimitIsReached=1;
	limitRate=atof(optarg);
	printf("Switching to broken mode\n");
	printf("Sending $LIMIT_IS_REACHED messages at %f\n",limitRate);
	break;	
      case 'r':
	rate=atof(optarg);
	if(rate<0.01) {
	  printf("Rate to small defaulting to 1Hz\n");
	  rate=1;
	}
	g12TttPeriod=adu5TttPeriod=(2./rate);
        break;
      case 'h':
	printUsage(argv[0]);
	return 0;
	break;
      default:
	break;
      }
     }
  }
     

    /* Load Config */
    int retVal=readConfigFile();
    if(retVal<0) {
	//syslog(LOG_ERR,"Problem reading GPSd.config");
	printf("Problem reading GPSd.config\n");
	exit(1);
    }
#ifndef NO_SERIAL
    retVal=openDevices();
#endif
    printf("Initalizing gpsSimulator\n");    
    ///    retVal=setupG12();
    //    retVal=setupG12Ntp();
    //    retVal=setupAdu5();
    
    double usleepPeriod=0;
    if(limitRate>0) {
      usleepPeriod=1e6/limitRate;
    }

    while(1) {
      serviceAdu5();
      if(limitRate>0) {
	sendLimitIsReached(fdG12);
	sendLimitIsReached(fdG12Ntp);
	usleep(usleepPeriod);
      }
      else {
	serviceG12();
	usleep(1);
       } 
    }
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
	    //syslog(LOG_WARNING,
	  //		   "Couldn't fetch printToScreen, defaulting to zero");
	    printToScreen=0;	    
	}
    }
    else {
	eString=configErrorString (status) ;
	fprintf(stderr,"Error reading GPSd.config: %s\n",eString);
    }
    kvpReset();
    status = configLoad ("GPSd.config","g12");
    if(status == CONFIG_E_OK) {
	g12EnableTtt=kvpGetInt("enableTtt",1);
	g12TttEpochRate=kvpGetInt("tttEpochRate",20);
	if(g12TttEpochRate!=20 && g12TttEpochRate!=10 &&
	   g12TttEpochRate!=5 && g12TttEpochRate!=2 && g12TttEpochRate!=1) {
	    //syslog(LOG_WARNING,"Incorrect TTT Epoch Rate %d, reverting to 20Hz",
	  //		   g12TttEpochRate);
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
	//syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
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
	//syslog(LOG_ERR,"Error reading GPSd.config: %s\n",eString);
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
    retVal=openGpsDevice(G12_PORT);
    if(retVal<=0) {
	//syslog(LOG_ERR,"Couldn't open: %s\n",G12_PORT);
	if(1) printf("Couldn't open: %s\n",G12_PORT);
	exit(1);
    }
    else fdG12=retVal;

    retVal=openGpsDevice(G12_NTP_PORT);
    if(retVal<=0) {
	//syslog(LOG_ERR,"Couldn't open: %s\n",G12_NTP_PORT);
	if(1) printf("Couldn't open: %s\n",G12_NTP_PORT);
	exit(1);
    }
    else fdG12Ntp=retVal;
 

    retVal=openGpsDevice(ADU5_PORT);	
    if(retVal<=0) {
	//syslog(LOG_ERR,"Couldn't open: %s\n",ADU5_PORT);
	if(1) printf("Couldn't open: %s\n",ADU5_PORT);
	exit(1);
    }
    else fdAdu5=retVal;

//    printf("%s %s %s\n",G12A_DEV_NAME,ADU5A_DEV_NAME,ADU5A_DEV_NAME);
    return 0;
}


int addChecksum(char *gpsString,int gpsLength) 
{
    char theCheckSum=0;
    char temp;
    int count;
    for(count=1;count<gpsLength-3;count++) {
	memcpy(&temp,&gpsString[count],sizeof(char));
//	printf("%c ",gpsString[count]);
	theCheckSum^=temp;	
    }
    char hexChars[16]={'0','1','2','3','4','5','6','7','8','9','A',
		       'B','C','D','E','F'};
    gpsString[gpsLength-1]=hexChars[theCheckSum%16];
    gpsString[gpsLength-2]=hexChars[theCheckSum/16];
    //    int otherSum=0;
    //    for(count=0;count<16;count++) {
    //	if(gpsString[gpsLength-1]==hexChars[count]) otherSum+=count;
    //	if(gpsString[gpsLength-2]==hexChars[count]) otherSum+=count*16;
    //    }
    return theCheckSum;
}


int serviceG12()
{
  //Think about what to do
  static unsigned int lastGPZDA=0;
  static unsigned int lastRMC=0;
  static double lastPOS=0;
  static unsigned int lastSAT=0;
  static double lastTtt=0;

  time_t curTime;
  double fTime;
  //  time(&curTime);

  struct timeval timeStruct;
  gettimeofday(&timeStruct,NULL);
  curTime=timeStruct.tv_sec;
  fTime=curTime + 1e-6*timeStruct.tv_usec;

  //  printf("g12PosPeriod: %f\n",g12PosPeriod);
  //Now check if we're in the next second
    //Currently limited to 1Hz and lower
  if(curTime>=lastGPZDA+(int)g12ZdaPeriod) {
    lastGPZDA=curTime;
    if(justSendLimitIsReached)
      sendLimitIsReached(fdG12);
    else 
      sendG12GPZDA(lastGPZDA);
  }
  if(curTime>=lastRMC+(int)1) {
    lastRMC=curTime;
    if(justSendLimitIsReached)
      sendLimitIsReached(fdG12Ntp);
    else 
      sendG12RMC(lastRMC);
  }
  if(fTime>=lastPOS+g12PosPeriod) {
    //  printf("%d\t%d\t%f\n",curTime,lastPOS,g12PosPeriod);
    lastPOS=fTime;
    if(justSendLimitIsReached)
      sendLimitIsReached(fdG12);
    else 
      sendG12Pos(curTime,-70.68,175.0,30456.2);
  }
  if(curTime>=lastSAT+(int)g12SatPeriod) {
    lastSAT=curTime;
    if(justSendLimitIsReached)
      sendLimitIsReached(fdG12);
    else 
      sendG12Sat();
  }
  if(fTime>=lastTtt+g12TttPeriod) {
    lastTtt=fTime;
    if(justSendLimitIsReached)
      sendLimitIsReached(fdG12);
    else 
      sendG12TTT(fTime);
  }

  

  return 0;
}


int serviceAdu5()
{
  //Think about what to do
  static double lastGPPAT=0;
  static double lastGPVTG=0;
  static unsigned int lastSAT=0;
  static double lastTtt=0;

  time_t curTime;
  double fTime;
  //  time(&curTime);

  struct timeval timeStruct;
  gettimeofday(&timeStruct,NULL);
  curTime=timeStruct.tv_sec;
  fTime=curTime + 1e-6*timeStruct.tv_usec;

  //  printf("g12PosPeriod: %f\n",g12PosPeriod);
  //Now check if we're in the next second
    //Currently limited to 1Hz and lower
  
  if(fTime>=lastGPPAT+adu5PatPeriod) {
    //  printf("%d\t%d\t%f\n",curTime,lastPOS,g12PosPeriod);
    lastGPPAT=fTime;
    //    if(justSendLimitIsReached)
    //      sendLimitIsReached(fdAdu5);
    //    else 
    sendAdu5PAT(curTime,-70.68,175.0,30456.2);
  }
  if(fTime>=lastGPVTG+adu5VtgPeriod) {
    //  printf("%d\t%d\t%f\n",curTime,lastPOS,g12PosPeriod);
    lastGPVTG=fTime;
    //    if(justSendLimitIsReached)
    //      sendLimitIsReached(fdAdu5);
    //    else 
    sendAdu5VTG(curTime);
  }
  if(curTime>=lastSAT+(int)adu5SatPeriod) {
    lastSAT=curTime;
    //    if(justSendLimitIsReached)
    //      sendLimitIsReached(fdAdu5);
    //    else 
    sendAdu5Sat();
  }
  if(fTime>=lastTtt+adu5TttPeriod) {
    lastTtt=fTime;
    //    if(justSendLimitIsReached)
    //      sendLimitIsReached(fdAdu5);
    //    else 
    sendAdu5TTT(fTime);
  }

  

  return 0;
}



int sendG12GPZDA(unsigned int unixTime)
{
  int retVal=0;
  struct tm thisTm;
  time_t tempTime=unixTime;
  gmtime_r(&tempTime,&thisTm);
  char gpsString[180];
  sprintf(gpsString,"$GPZDA,%02d%02d%02d.%02d,%02d,%02d,%04d,%02d,%02d*00",
	  thisTm.tm_hour,thisTm.tm_min,thisTm.tm_sec,0,thisTm.tm_mday,thisTm.tm_mon+1,thisTm.tm_year+1900,0,0);
  addChecksum(gpsString,strlen(gpsString));
  //  printf("%d\n",strlen(gpsString));


#ifdef NO_SERIAL
  fprintf(stderr,"%s\n",gpsString);
#else
  retVal=write(fdG12,gpsString,strlen(gpsString));
  if(retVal<0) {
    fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	    strerror(errno));
  }    
#endif
  return retVal;
}



int sendG12Pos(unsigned int unixTime, float latitude, float longitude, float altitude)
{
  int posType=0;
  int numSats=6;
  float speedInKnots=10;
  float verticalVelocity=20;
  float trueCourse=90;
  float pdop=06.1;
  float hdop=04.2;
  float vdop=03.3;
  float tdop=01.4;
  char *firmware="YM06";

  int retVal=0;
  struct tm thisTm;
  time_t tempTime=unixTime;
  gmtime_r(&tempTime,&thisTm);
  char gpsString[180];

  char northOrSouth='N';
  if(latitude<0) {
    latitude*=-1;
    northOrSouth='S';
  }
  int latDeg=(int)latitude;
  float latMin=(float)60*(latitude-latDeg);

  char eastOrWest='E';
  if(longitude<0) {
    longitude*=-1;
    eastOrWest='W';
  }
  int longDeg=(int)longitude;
  float longMin=(float)60*(longitude-longDeg);


  sprintf(gpsString,"$PASHR,POS,%d,%02d,%02d%02d%02d.%02d,%02d%f,%c,%03d%f,%c,%+05.2f,,%03.2f,%03.2f,%+03.2f,%02.1f,%02.1f,%02.1f,%02.1f,%s*00",
	  posType,numSats,thisTm.tm_hour,thisTm.tm_min,thisTm.tm_sec,0,
	  latDeg,latMin,northOrSouth,longDeg,longMin,eastOrWest,
	  altitude,trueCourse,
	  speedInKnots,verticalVelocity,
	  pdop,hdop,vdop,tdop,
	  firmware);

  addChecksum(gpsString,strlen(gpsString));
  //  printf("%d\n",strlen(gpsString));


#ifdef NO_SERIAL
  fprintf(stderr,"%s\n",gpsString);
#else
  retVal=write(fdG12,gpsString,strlen(gpsString));
  if(retVal<0) {
    fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	    strerror(errno));
  }    
#endif
  return retVal;
}



int sendG12Sat()
{
  int retVal=0;
  char gpsString[180];
  sprintf(gpsString,"$PASHR,SAT,04,01,100,45,50,02,101,46,51,03,102,47,52,04,103,48,53*00");

  addChecksum(gpsString,strlen(gpsString));
  //  printf("%d\n",strlen(gpsString));


#ifdef NO_SERIAL
  fprintf(stderr,"%s\n",gpsString);
#else
  retVal=write(fdG12,gpsString,strlen(gpsString));
  if(retVal<0) {
    fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	    strerror(errno));
  }    
#endif
  return retVal;
}


int sendG12TTT(double tttTime)
{
  
  unsigned int unixTime= (unsigned int) tttTime;
  int subSecond=(int)((tttTime-unixTime)*1e7);
  int retVal=0;
  struct tm thisTm;
  time_t tempTime=unixTime;
  gmtime_r(&tempTime,&thisTm);
  char gpsString[180];
  sprintf(gpsString,"$PASHR,TTT,%d,%02d:%02d:%02d.%d*00",
	  thisTm.tm_wday+1,thisTm.tm_hour,thisTm.tm_min,thisTm.tm_sec,subSecond);
  addChecksum(gpsString,strlen(gpsString));
  //  printf("%d\n",strlen(gpsString));

#ifdef NO_SERIAL
  fprintf(stderr,"%s\n",gpsString);
#else
  retVal=write(fdG12,gpsString,strlen(gpsString));
  if(retVal<0) {
    fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	    strerror(errno));
  }    
#endif
  return retVal;
}


int sendG12RMC(unsigned int unixTime)
{
  int retVal=0;
  struct tm thisTm;
  time_t tempTime=unixTime;
  gmtime_r(&tempTime,&thisTm);
  char gpsString[180];
  sprintf(gpsString,"$GPRMC,%02d%02d%02d.%02d,,A,3722.410857,N,12159.773686,W,000.3,102.4,%02d%02d%02d,15.4,W*00",
	  thisTm.tm_hour,thisTm.tm_min,thisTm.tm_sec,0,thisTm.tm_mday,thisTm.tm_mon+1,thisTm.tm_year-100);
  addChecksum(gpsString,strlen(gpsString));
  //  printf("%d\n",strlen(gpsString));


#ifdef NO_SERIAL
  fprintf(stderr,"%s\n",gpsString);
#else
  retVal=write(fdG12Ntp,gpsString,strlen(gpsString));
  if(retVal<0) {
    fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	    strerror(errno));
  }    
#endif
  return retVal;
}


int sendAdu5PAT(unsigned int unixTime, float latitude, float longitude, float altitude)
{
  float heading=10;
  float pitch=20;
  float roll=90;
  float mrms=0.0031;
  float brms=0.0205;

  int retVal=0;
  struct tm thisTm;
  time_t tempTime=unixTime;
  gmtime_r(&tempTime,&thisTm);
  char gpsString[180];

  char northOrSouth='N';
  if(latitude<0) {
    latitude*=-1;
    northOrSouth='S';
  }
  int latDeg=(int)latitude;
  float latMin=(float)60*(latitude-latDeg);

  char eastOrWest='E';
  if(longitude<0) {
    longitude*=-1;
    eastOrWest='W';
  }
  int longDeg=(int)longitude;
  float longMin=(float)60*(longitude-longDeg);


  sprintf(gpsString,"$GPPAT,%02d%02d%02d.%02d,%02d%f,%c,%03d%f,%c,%+05.2f,%03.3f,%+03.2f,%+03.2f,%1.4f,%1.4f,0*00",
	  thisTm.tm_hour,thisTm.tm_min,thisTm.tm_sec,0,
	  latDeg,latMin,northOrSouth,longDeg,longMin,eastOrWest,
	  altitude,heading,pitch,roll,mrms,brms);

  addChecksum(gpsString,strlen(gpsString));
  //  printf("%d\n",strlen(gpsString));


#ifdef NO_SERIAL
  fprintf(stderr,"%s\n",gpsString);
#else
  retVal=write(fdAdu5,gpsString,strlen(gpsString));
  if(retVal<0) {
    fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	    strerror(errno));
  }    
#endif
  return retVal;
}

int sendAdu5VTG(unsigned int unixTime)
{
  float cog=100.33;
  float mog=99.23;
  float sog=0.07;
  float sog2=0.13;

  int retVal=0;
  struct tm thisTm;
  time_t tempTime=unixTime;
  gmtime_r(&tempTime,&thisTm);
  char gpsString[180];

  sprintf(gpsString,"$GPVTG,%3.2f,T,%3.2f,M,%3.2f,N,%3.2f,K",
	  cog,mog,sog,sog2);
	  
	 
  addChecksum(gpsString,strlen(gpsString));
  //  printf("%d\n",strlen(gpsString));


#ifdef NO_SERIAL
  fprintf(stderr,"%s\n",gpsString);
#else
  retVal=write(fdAdu5,gpsString,strlen(gpsString));
  if(retVal<0) {
    fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	    strerror(errno));
  }    
#endif
  return retVal;
}



int sendAdu5TTT(double tttTime)
{
  
  unsigned int unixTime= (unsigned int) tttTime;
  int subSecond=(int)((tttTime-unixTime)*1e7);
  int retVal=0;
  struct tm thisTm;
  time_t tempTime=unixTime;
  gmtime_r(&tempTime,&thisTm);
  char gpsString[180];
  sprintf(gpsString,"$PASHR,TTT,%d,%02d:%02d:%02d.%d*00",
	  thisTm.tm_wday+1,thisTm.tm_hour,thisTm.tm_min,thisTm.tm_sec,subSecond);
  addChecksum(gpsString,strlen(gpsString));
  //  printf("%d\n",strlen(gpsString));

#ifdef NO_SERIAL
  fprintf(stderr,"%s\n",gpsString);
#else
  retVal=write(fdAdu5,gpsString,strlen(gpsString));
  if(retVal<0) {
    fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	    strerror(errno));
  }    
#endif
  return retVal;
}


int sendAdu5Sat()
{
  int retVal=0;
  char gpsString[180];
  int sat;
  for(sat=1;sat<=4;sat++) {
    sprintf(gpsString,"$PASHR,SA4,%d,04,01,100,45,50,02,101,46,51,03,102,47,52,04,103,48,53*00",sat);
    
    addChecksum(gpsString,strlen(gpsString));
    //  printf("%d\n",strlen(gpsString));
    
#ifdef NO_SERIAL
    fprintf(stderr,"%s\n",gpsString);
#else
    retVal=write(fdAdu5,gpsString,strlen(gpsString));
    if(retVal<0) {
      fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	      strerror(errno));
    }    
#endif
  }
  return retVal;
}


void sendLimitIsReached(int thisFd)
{						
  char gpsString[180];
  int retVal=0;
  sprintf(gpsString,"$LIMIT IS REACHED*00");
  addChecksum(gpsString,strlen(gpsString));
  //  int retVal=checkChecksum(gpsString,strlen(gpsString));
#ifdef NO_SERIAL
    retVal=fprintf(stderr,"%s\n",gpsString);
#else
    retVal=write(thisFd,gpsString,strlen(gpsString));
    if(retVal<0) {
      fprintf(stderr,"Unable to write to G12 Serial port\n, write: %s",
	      strerror(errno));
    }    
#endif
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


    //    printf("%s %x %x\n",gpsString,theCheckSum,otherSum); 
//    printf("\n\n\n%d %x\n",theCheckSum,theCheckSum);
    
 /*    result=(theCheckSum==otherSum); */
/*     if(!result) { */
/* 	char checksum[3]; */
/* 	checksum[0]=gpsString[gpsLength-2]; */
/* 	checksum[1]=gpsString[gpsLength-1]; */
/* 	checksum[2]='\0'; */
/* 	printf("\n\nBugger:\t%s %x %x\n\n%s",checksum,theCheckSum,otherSum,gpsString); */
/*     } */
    return result;
}
