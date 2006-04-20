#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
//#include <zlib.h>
#include <math.h>

/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"

void fakeEvent(int trigType);
int rand_no(int lim);
void rand_no_seed(unsigned int seed);
float gaussianRand(float mean, float stdev);


/*Config Thingies*/
int useUsbDisks=0;
int useMainDisk=0;
int maxEventsPerDir=1000;
char eventArchiveDir[FILENAME_MAX];
char eventUSBArchiveDir[FILENAME_MAX];
char prioritizerdPidFile[FILENAME_MAX];


AnitaEventHeader_t theHeader;
AnitaEventBody_t theBody;


int main(int argc, char *argv[])
{
    char *tempString;
//    char tempDir[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
//    KvpErrorCode kvpStatus=0;
    char* eString;
    
    rand_no_seed(getpid());

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global");
    eString = configErrorString (status) ;


    if (status == CONFIG_E_OK) {
	useUsbDisks=kvpGetInt("useUsbDisks",0);
	useMainDisk=kvpGetInt("useMainDisk",0);
	printf("Disk Usage: %d %d\n",useUsbDisks,useMainDisk);
	maxEventsPerDir=kvpGetInt("maxEventsPerDir",1000);

	tempString=kvpGetString("mainDataDisk");
	if(tempString) {
	    strncpy(eventArchiveDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error getting mainDataDisk");
	    fprintf(stderr,"Error getting mainDataDisk\n");
	}
	tempString=kvpGetString("usbDataDiskLink");
	if(tempString) {
	    strncpy(eventUSBArchiveDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error getting usbDataDiskLink");
	    fprintf(stderr,"Error getting usbDataDiskLink\n");
	}
	    
	    
	tempString=kvpGetString("baseEventArchiveDir");
	if(tempString) {
	    sprintf(eventArchiveDir,"%s/%s/",eventArchiveDir,tempString);
	    sprintf(eventUSBArchiveDir,"%s/%s/",eventUSBArchiveDir,tempString);
	    if(useMainDisk) makeDirectories(eventArchiveDir);
	    if(useUsbDisks) makeDirectories(eventUSBArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting baseHouseArchiveDir");
	    fprintf(stderr,"Error getting baseHouseArchiveDir\n");
	}
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
	fprintf(stderr,"Error reading config file: %s\n",eString);
    }
    
    int evCounter=0;
    int trigType=34;
    while(1) {
//	time(&rawTime);
	fakeEvent(trigType);
	if(evCounter==0) {
	    usleep(50000);
	    trigType=38;
	}
	else {
	    usleep(180000);
	    trigType=34;
	}
	evCounter++;
	if(evCounter==6) evCounter=0;
	

    }
    return 0;
}

void fakeEvent(int trigType) 
{
//    printf("Here\n");
    int chan=0;
    int samp=0;
    int retVal=0;
    struct timeval timeStruct;
    static int evNum=1;    
    char archiveHdFilename[FILENAME_MAX];
    char archiveBodyFilename[FILENAME_MAX];

    if(evNum==1){
	//Fake some data
	for(chan=0;chan<NUM_DIGITZED_CHANNELS;chan++) {
	    for(samp=0;samp<256;samp++) {
		short value=(short)gaussianRand(2048,200);
		value+=((evNum%4)<<14);
		theBody.channel[chan].data[samp]=value;
//		printf("%d\t%d\n",samp,theBody.channel[chan].data[samp]);
	    }
	}
    }
    printf("Created Event: %d\n",evNum);
    gettimeofday(&timeStruct,NULL);
    theHeader.gHdr.code=PACKET_HD;
//    theHeader.turfio.trigType=trigType;
    theHeader.unixTime=timeStruct.tv_sec;
    theHeader.unixTimeUs=timeStruct.tv_usec;
    theHeader.numChannels=16;
    theHeader.eventNumber=evNum;
    if(useMainDisk) {
	sprintf(archiveHdFilename,"%s/hd_%d.dat",eventArchiveDir,
		evNum);
	sprintf(archiveBodyFilename,"%s/ev_%d.dat",eventArchiveDir,
		evNum);	    
	retVal=writeHeader(&theHeader,archiveHdFilename);
	retVal=writeBody(&theBody,archiveBodyFilename);
    }
    if(useUsbDisks) {
	sprintf(archiveHdFilename,"%s/hd_%d.dat",eventUSBArchiveDir,
		evNum);
	sprintf(archiveBodyFilename,"%s/ev_%d.dat",eventUSBArchiveDir,
		evNum);	    
//	printf("Writing %s\n",archiveHdFilename);
	retVal=writeHeader(&theHeader,archiveHdFilename);
	retVal=writeBody(&theBody,archiveBodyFilename);
    }     
    
    evNum++; 
       
}


int rand_no(int lim)
{
    float a;
    a = (float)rand() / RAND_MAX;
    a *= lim;
    return ((int) a);
}

void rand_no_seed(unsigned int seed)
{
    srand(seed);
}

float myRand() {
    float a =  (float)rand() / (float)(RAND_MAX);
    return a;
}

float gaussianRand(float mean, float stdev)
{
  static int cached = 0;
  static float extra;  	// the extra number from a 0 mean unit stdev gaussian

  if (cached) {
    cached = 0;
    return extra*stdev + mean;
  }
  else {
    // pick a random point in the unit circle (excluding the origin)
    float a,b,c;
    do {
      a = 2*myRand()-1.0;
      b = 2*myRand()-1.0;
      c = a*a + b*b;
    }
    while (c >= 1.0 || c == 0.0);
    // transform it into two values drawn from a gaussian
    float t = sqrt(-2*log(c)/c);
    extra = t*a;
    cached = 1;
    return t*b*stdev + mean;
  }
}

