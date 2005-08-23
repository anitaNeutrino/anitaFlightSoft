#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"



void fakeEvent(int trigType);
void fakePat(long unixTime);
void fakeSat(long unixTime);
void fakeHkCal(long unixTime);
void fakeHkRaw(long unixTime);

void writePackets(AnitaEventBody_t *bodyPtr, AnitaEventHeader_t *hdPtr);

/*Config Thingies*/
char losdPacketDir[FILENAME_MAX];
char prioritizerdSipdHdDir[FILENAME_MAX];
char prioritizerdSipdHdLinkDir[FILENAME_MAX];
char prioritizerdSipdWvDir[FILENAME_MAX];
char prioritizerdSipdWvLinkDir[FILENAME_MAX];
char gpsdSipdDir[FILENAME_MAX];
char gpsdSipdLinkDir[FILENAME_MAX];
char hkdSipdDir[FILENAME_MAX];
char hkdSipdLinkDir[FILENAME_MAX];
int numPacketDirs=0;

float g12PPSPeriod=0.2;
int adu5PatPeriod=10;
int adu5SatPeriod=600;
int hkReadoutPeriod=5;
int hkCalPeriod=600;

AnitaEventHeader_t theHeader;
AnitaEventBody_t theBody;


int main(int argc, char *argv[])
{
    int pk;//,retVal;
    char *tempString;
    char tempDir[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;
    

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("SIPd.config","global") ;
    status += configLoad ("SIPd.config","losd") ;
    status += configLoad ("GPSd.config","g12");
    status += configLoad ("GPSd.config","adu5");
    status += configLoad ("Hkd.config","hkd");
    eString = configErrorString (status) ;


    if (status == CONFIG_E_OK) {
/* 	printf("Here\n"); */
	numPacketDirs=kvpGetInt("numPacketDirs",9);
	tempString=kvpGetString("losdPacketDir");
	if(tempString) {
	    strncpy(losdPacketDir,tempString,FILENAME_MAX);
	    for(pk=0;pk<numPacketDirs;pk++) {
		sprintf(tempDir,"%s/pk%d/link",losdPacketDir,pk);
		makeDirectories(tempDir);
	    }		
	}
	else {
	    syslog(LOG_ERR,"Couldn't get losdPacketDir");
	    fprintf(stderr,"Couldn't get losdPacketDir\n");
	}	
	g12PPSPeriod=kvpGetFloat("ppsPeriod",0.2);
	adu5PatPeriod=kvpGetInt("patPeriod",10);
	adu5SatPeriod=kvpGetInt("satPeriod",600);
	hkReadoutPeriod=kvpGetInt("readoutPeriod",5);
	hkCalPeriod=kvpGetInt("calibrationPeriod",600);

	tempString=kvpGetString("prioritizerdSipdHdDir");
	if(tempString) {
	    strncpy(prioritizerdSipdHdDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdSipdHdDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdSipdHdDir");
	    fprintf(stderr,"Error getting prioritizerdSipdHdDir\n");
	}
	tempString=kvpGetString("prioritizerdSipdHdLinkDir");
	if(tempString) {
	    strncpy(prioritizerdSipdHdLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdSipdHdLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdSipdHdLinkDir");
	    fprintf(stderr,"Error getting prioritizerdSipdHdLinkDir\n");
	}

	tempString=kvpGetString("prioritizerdSipdWvDir");
	if(tempString) {
	    strncpy(prioritizerdSipdWvDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdSipdWvDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdSipdWvDir");
	    fprintf(stderr,"Error getting prioritizerdSipdWvDir\n");
	}
	tempString=kvpGetString("prioritizerdSipdWvLinkDir");
	if(tempString) {
	    strncpy(prioritizerdSipdWvLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdSipdWvLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdSipdWvLinkDir");
	    fprintf(stderr,"Error getting prioritizerdSipdWvLinkDir\n");
	}


	tempString=kvpGetString("gpsdSipdDir");
	if(tempString) {
	    strncpy(gpsdSipdDir,tempString,FILENAME_MAX);
	    makeDirectories(gpsdSipdDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdSipdDir");
	    fprintf(stderr,"Couldn't get gpsdSipdDir\n");
	}
	tempString=kvpGetString("gpsdSipdLinkDir");
	if(tempString) {
	    strncpy(gpsdSipdLinkDir,tempString,FILENAME_MAX);
	    makeDirectories(gpsdSipdLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdSipdLinkDir");
	    fprintf(stderr,"Couldn't get gpsdSipdLinkDir\n");
	}

	tempString=kvpGetString("hkdSipdDir");
	if(tempString) {
	    strncpy(hkdSipdDir,tempString,FILENAME_MAX-1);	    
	    makeDirectories(hkdSipdDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting hkdSipdDir");
	    fprintf(stderr,"Error getting hkdSipdDir\n");
	}
	tempString=kvpGetString("hkdSipdLinkDir");
	if(tempString) {
	    strncpy(hkdSipdLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(hkdSipdLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting hkdSipdLinkDir");
	    fprintf(stderr,"Error getting hkdSipdLinkDir\n");
	}	    
	//printf("%d\t%s\n",kvpStatus,kvpErrorString(kvpStatus));
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }
    
    int evCounter=0;
    long lastAdu5Pat=0;
    long lastAdu5Sat=0;
    long lastHkCal=0;
    long lastHkData=0;
    time_t rawTime=0;
    int trigType=34;
    while(1) {
	time(&rawTime);
	
	//Check last PAT
	if((rawTime-lastAdu5Pat)>adu5PatPeriod) {
	    fakePat(rawTime);
	    lastAdu5Pat=rawTime;
	}
	//Check last SAT
	if((rawTime-lastAdu5Sat)>adu5SatPeriod) {
	    fakeSat(rawTime);
	    lastAdu5Sat=rawTime;
	}
	if((rawTime-lastHkCal)>hkCalPeriod) {
	    fakeHkCal(rawTime);
	    lastHkCal=rawTime;
	}
	if((rawTime-lastHkData)>hkReadoutPeriod) {
	    fakeHkRaw(rawTime);
	    lastHkData=rawTime;
	}
	
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
    char sipdHdFilename[FILENAME_MAX];
    int chan=0;
    int samp=0;
    struct timeval timeStruct;
    static int evNum=1;    
    if(evNum==1){
	//Fake some data
	for(chan=0;chan<16;chan++) {
	    for(samp=0;samp<256;samp++) {
		theBody.channel[chan].data[samp]=samp;
	    }
	}
    }
    gettimeofday(&timeStruct,NULL);
    theHeader.gHdr.code=PACKET_HD;
    theHeader.turfio.trigType=trigType;
    theHeader.unixTime=timeStruct.tv_sec;
    theHeader.unixTimeUs=timeStruct.tv_usec;
    theHeader.numChannels=16;
    theHeader.eventNumber=evNum;
    evNum++;      
    	    
    /* Write output for SIPd*/	    
    writePackets(&theBody,&theHeader);
    
    //Move and link header
    sprintf(sipdHdFilename,"%s/hd_%d.dat",prioritizerdSipdHdDir,
	    theHeader.eventNumber);
    writeHeader(&theHeader,sipdHdFilename);
    makeLink(sipdHdFilename,prioritizerdSipdHdLinkDir);
    if(evNum%100==0) 
	printf("Event %d, sec %ld\n",evNum,theHeader.unixTime);
       
}



void writePackets(AnitaEventBody_t *bodyPtr, AnitaEventHeader_t *hdPtr) 
{
//    int chan;
//    char packetName[FILENAME_MAX];
//    WaveformPacket_t wavePacket;
/*     wavePacket.gHdr.code=PACKET_WV; */
/*     for(chan=0;chan<hdPtr->numChannels;chan++) { */
/* 	sprintf(packetName,"%s/wvpk_%d_%d.dat",prioritizerdSipdWvDir,hdPtr->eventNumber,chan); */
/* //	printf("Packet: %s\n",packetName); */
/* 	wavePacket.eventNumber=hdPtr->eventNumber; */
/* 	wavePacket.packetNumber=chan; */
/* 	memcpy(&(wavePacket.waveform),&(bodyPtr->channel[chan]),sizeof(SurfChannelFull_t)); */
/* 	writeWaveformPacket(&wavePacket,packetName); */
/* 	makeLink(packetName,prioritizerdSipdWvLinkDir); */
/*     } */
    int surf;
    char packetName[FILENAME_MAX];    
    SurfPacket_t surfPacket;
    surfPacket.gHdr.code=PACKET_SURF;
    for(surf=0;surf<(hdPtr->numChannels)/CHANNELS_PER_SURF;surf++) {
	sprintf(packetName,"%s/surfpk_%d_%d.dat",prioritizerdSipdWvDir,hdPtr->eventNumber,surf); 
	surfPacket.eventNumber=hdPtr->eventNumber;
	surfPacket.packetNumber=surf;
	memcpy(&(surfPacket.waveform[0]),&(bodyPtr->channel[CHANNELS_PER_SURF*surf]),sizeof(SurfChannelFull_t)*CHANNELS_PER_SURF);
	writeSurfPacket(&surfPacket,packetName);
	makeLink(packetName,prioritizerdSipdWvLinkDir);
    }
}
	    

void fakePat(long unixTime) {

    GpsPatStruct_t thePat;
    char theFilename[FILENAME_MAX];
    int retVal;
    thePat.gHdr.code=PACKET_GPSD_PAT;
    thePat.unixTime=unixTime;
    thePat.heading=0;
    thePat.pitch=0;
    thePat.roll=0;
    thePat.brms=0;
    thePat.mrms=0;
    thePat.attFlag=0;
    thePat.latitude=34.489885;
    thePat.longitude=-1*104.2218668;
    thePat.altitude=1238.38;

    //Write file and link for sipd
    sprintf(theFilename,"%s/pat_%ld.dat",gpsdSipdDir,thePat.unixTime);
    retVal=writeGPSPat(&thePat,theFilename);  
    retVal=makeLink(theFilename,gpsdSipdLinkDir); 
}

void fakeSat(long unixTime) {
    GpsSatStruct_t theSat;
    char theFilename[FILENAME_MAX];
    int retVal,satNum;

    theSat.gHdr.code=PACKET_GPSD_SAT;
    theSat.unixTime=unixTime;
    theSat.numSats=3;
    for(satNum=0;satNum<theSat.numSats;satNum++) {
	theSat.sat[satNum].prn=satNum+10;
	theSat.sat[satNum].azimuth=15;
	theSat.sat[satNum].elevation=15;
	theSat.sat[satNum].snr=99;
	theSat.sat[satNum].flag=0;
    }

    sprintf(theFilename,"%s/pat_%ld.dat",gpsdSipdDir,theSat.unixTime);
    retVal=writeGPSSat(&theSat,theFilename);  
    retVal=makeLink(theFilename,gpsdSipdLinkDir);
}


void fakeHkCal(long unixTime) 
{

    int retVal,chan,board;
    char theFilename[FILENAME_MAX];
    char fullFilename[FILENAME_MAX];
    HkDataStruct_t theHkData;
    theHkData.gHdr.code=PACKET_HKD;
    theHkData.unixTime=unixTime;
    theHkData.ip320.code=IP320_CAL;
    for(board=0;board<NUM_IP320_BOARDS;board++) {
	for(chan=0;chan<CHANS_PER_IP320;chan++) {
	    theHkData.ip320.board[board].data[chan]=0xffe0;
	}
    }
    theHkData.mag.x=2.3;
    theHkData.mag.y=2.3;
    theHkData.mag.z=2.3;
    theHkData.sbs.temp[0]=25.5;
    theHkData.sbs.temp[1]=25.5;   
     	
    sprintf(theFilename,"hk_%ld.cal.dat",theHkData.unixTime);    
    //Write file and make link for SIPd
    sprintf(fullFilename,"%s/%s",hkdSipdDir,theFilename);
    retVal=writeHk(&theHkData,fullFilename);     
    retVal+=makeLink(fullFilename,hkdSipdLinkDir);

    sprintf(theFilename,"hk_%ld.avz.dat",theHkData.unixTime);
    theHkData.ip320.code=IP320_AVZ;
    for(board=0;board<NUM_IP320_BOARDS;board++) {
	for(chan=0;chan<CHANS_PER_IP320;chan++) {
	    theHkData.ip320.board[board].data[chan]=0x8fe0;
	}
    }    
    //Write file and make link for SIPd
    sprintf(fullFilename,"%s/%s",hkdSipdDir,theFilename);
    retVal=writeHk(&theHkData,fullFilename);     
    retVal+=makeLink(fullFilename,hkdSipdLinkDir);      

}

void fakeHkRaw(long unixTime) 
{
    int retVal,chan,board;
    char theFilename[FILENAME_MAX];
    char fullFilename[FILENAME_MAX];
    HkDataStruct_t theHkData;
    theHkData.gHdr.code=PACKET_HKD;
    theHkData.unixTime=unixTime;
    theHkData.ip320.code=IP320_RAW;    
    for(board=0;board<NUM_IP320_BOARDS;board++) {
	for(chan=0;chan<CHANS_PER_IP320;chan++) {
	    theHkData.ip320.board[board].data[chan]=0x8fe0 + chan;
	}
    }
    theHkData.mag.x=2.3;
    theHkData.mag.y=2.3;
    theHkData.mag.z=2.3;
    theHkData.sbs.temp[0]=25.5;
    theHkData.sbs.temp[1]=25.5;   
    sprintf(theFilename,"hk_%ld.raw.dat",theHkData.unixTime);
    
    //Write file and make link for SIPd
    sprintf(fullFilename,"%s/%s",hkdSipdDir,theFilename);
    retVal=writeHk(&theHkData,fullFilename);     
    retVal+=makeLink(fullFilename,hkdSipdLinkDir);    
}

