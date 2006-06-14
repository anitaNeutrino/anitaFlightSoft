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
void fakeAdu5Pat(struct timeval *currentTime);
void fakeAdu5Sat(struct timeval *currentTime);
void fakeAdu5Vtg(struct timeval *currentTime);
void fakeG12Sat(struct timeval *currentTime);
void fakeG12Pos(struct timeval *currentTime);
void fakeHkCal(struct timeval *currentTime);
void fakeHkRaw(struct timeval *currentTime);
void fakeMonitor(struct timeval *currentTime);
void fakeSurfHk(struct timeval *currentTime);
void fakeTurfRate(struct timeval *currentTime);


void writePackets(AnitaEventBody_t *bodyPtr, AnitaEventHeader_t *hdPtr);
int rand_no(int lim);
void rand_no_seed(unsigned int seed);
float gaussianRand(float mean, float stdev);
float getTimeDiff(struct timeval oldTime, struct timeval currentTime);



// Data rates
float g12PosPeriod=10;
int g12SatPeriod=600;
int adu5SatPeriod=600;
float adu5PatPeriod=10;
float adu5VtgPeriod=10;
int hkReadoutPeriod=5;
float actualHkPeriod=0.1;
int hkCalPeriod=600;
int monitorPeriod=60; //In seconds
int surfHkPeriod=10;
int turfRateTelemInterval=60;

// Event structs
AnitaEventHeader_t theHeader;
AnitaEventBody_t theBody;


//Max Events
int maxEvents=0;
int evNum=1;   


int main(int argc, char *argv[])
{
//    int pk;//,retVal;
    char *tempString;
//    char tempDir[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;
    
    if(argc==2)
	maxEvents=atoi(argv[1]);

    rand_no_seed(getpid());

    /* Load Config */
    kvpReset () ;
    

    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("Acqd.config","acqd") ;
    status += configLoad ("Hkd.config","hkd");
    status += configLoad ("Monitord.config","monitord") ;
    eString = configErrorString (status) ;


    if (status == CONFIG_E_OK) {
	hkReadoutPeriod=kvpGetInt("readoutPeriod",5);
	actualHkPeriod=0.1*((float)hkReadoutPeriod);
	hkCalPeriod=kvpGetInt("calibrationPeriod",600);
	monitorPeriod=kvpGetInt("monitorPeriod",60);
	surfHkPeriod=kvpGetInt("surfHkPeriod",10);
	turfRateTelemInterval=kvpGetInt("turfRateTelemInterval",60);

	//Output and Link Directories

    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
	fprintf(stderr,"Error reading config file: %s\n",eString);
    }


    kvpReset () ;
    status = configLoad ("GPSd.config","adu5");
    if (status == CONFIG_E_OK) {
//	printf("Here\n");
	adu5PatPeriod=kvpGetFloat("patPeriod",10);
	adu5SatPeriod=kvpGetInt("satPeriod",600);
	adu5VtgPeriod=kvpGetFloat("vtgPeriod",600);
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
	fprintf(stderr,"Error reading config file: %s\n",eString);
    }

    kvpReset () ;
    status = configLoad ("GPSd.config","g12");   
    if (status == CONFIG_E_OK) {
//	printf("Here\n");
	g12PosPeriod=kvpGetFloat("posPeriod",10);
	g12SatPeriod=kvpGetInt("satPeriod",600);

    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
	fprintf(stderr,"Error reading config file: %s\n",eString);
    }



    int evCounter=0;
    int trigType=34;
    
    struct timeval lastAdu5Pat;
    lastAdu5Pat.tv_sec=0;
    struct timeval lastAdu5Sat;
    lastAdu5Sat.tv_sec=0;
    struct timeval lastAdu5Vtg;
    lastAdu5Vtg.tv_sec=0;
    struct timeval lastG12Pos;
    lastG12Pos.tv_sec=0;
    struct timeval lastG12Sat;
    lastG12Sat.tv_sec=0;
    struct timeval lastHkCal;
    lastHkCal.tv_sec=0;
    struct timeval lastHkData;
    lastHkData.tv_sec=0;
    struct timeval lastMonitor;
    lastMonitor.tv_sec=0;
    struct timeval lastSurfHk;
    lastSurfHk.tv_sec=0;
    struct timeval lastTurfRate;
    lastTurfRate.tv_sec=0;
    struct timeval currentTime;

    printf("ADU5 PAT Period %f\n",adu5PatPeriod);
    printf("ADU5 SAT Period %d\n",adu5SatPeriod);
    printf("ADU5 VTG Period %f\n",adu5VtgPeriod);
    printf("G12 POS Period %f\n",g12PosPeriod);
    printf("G12 SAT Period %d\n",g12SatPeriod);
    printf("Monitor Period %d\n",monitorPeriod);
    printf("SURF Hk Period %d\n",surfHkPeriod);
    printf("TURF Rate Period %d\n",turfRateTelemInterval);
    printf("Acromag Cal Period %d\n",hkCalPeriod);
    printf("Acromag Data Period %f\n",actualHkPeriod);

    makeDirectories(PRIORITIZERD_EVENT_LINK_DIR);
    makeDirectories(HEADER_TELEM_LINK_DIR);
    makeDirectories(SURFHK_TELEM_LINK_DIR);
    makeDirectories(TURFHK_TELEM_LINK_DIR);
    makeDirectories(HK_TELEM_LINK_DIR);
    makeDirectories(MONITOR_TELEM_LINK_DIR);
    makeDirectories(GPS_TELEM_LINK_DIR);

    while(!maxEvents || (evNum<=maxEvents)) {
	gettimeofday(&currentTime,0);
//	time(&rawTime);

	
	//Check last PAT
//	printf("%d %d\n",lastAdu5Pat.tv_sec,lastAdu5Pat.tv_usec);
	if(getTimeDiff(lastAdu5Pat,currentTime)>adu5PatPeriod) {
	    fakeAdu5Pat(&currentTime);
	    lastAdu5Pat=currentTime;
	}
	//Check last Adu5 SAT
	if(getTimeDiff(lastAdu5Sat,currentTime)>adu5SatPeriod) {
	    fakeAdu5Sat(&currentTime);
	    lastAdu5Sat=currentTime;
	}
	//Check last Adu5 VTG
	if(getTimeDiff(lastAdu5Vtg,currentTime)>adu5VtgPeriod) {
	    fakeAdu5Vtg(&currentTime);
	    lastAdu5Vtg=currentTime;
	}
	//Check last G12 SAT
	if(getTimeDiff(lastG12Sat,currentTime)>g12SatPeriod) {
	    fakeG12Sat(&currentTime);
	    lastG12Sat=currentTime;
	}
	//Check last G12 SAT
	if(getTimeDiff(lastG12Pos,currentTime)>g12PosPeriod) {
	    fakeG12Pos(&currentTime);
	    lastG12Pos=currentTime;
	}
	//Check last CPU monitor
	if(getTimeDiff(lastMonitor,currentTime)>monitorPeriod) {
	    fakeMonitor(&currentTime);
	    lastMonitor=currentTime;
	}
	//Check last Surf Hk
	if(surfHkPeriod) {
	    //If not one per event
	    if(getTimeDiff(lastSurfHk,currentTime)>surfHkPeriod) {
		fakeSurfHk(&currentTime);
		lastSurfHk=currentTime;
	    }
	}
	//Check last Turf Rate
	if(turfRateTelemInterval) {
	    //If not one per event
	    if(getTimeDiff(lastTurfRate,currentTime)>turfRateTelemInterval) {
		fakeTurfRate(&currentTime);
		lastTurfRate=currentTime;
	    }
	}
	

	if(getTimeDiff(lastHkCal,currentTime)>hkCalPeriod) {
	    fakeHkCal(&currentTime);
	    lastHkCal=currentTime;
	}
	if(getTimeDiff(lastHkData,currentTime)>actualHkPeriod) {
	    fakeHkRaw(&currentTime);
	    lastHkData=currentTime;
	}
	
//	fakeEvent(trigType);
//	if(surfHkPeriod==0) fakeSurfHk(&currentTime);
	usleep(50000);
//	evCounter++;
//	if(evCounter==6) evCounter=0;
    }
    return 0;
}

void fakeEvent(int trigType) 
{
    int retVal=0;
    char sipdHdFilename[FILENAME_MAX];
    char archiveFilename[FILENAME_MAX];
    int chan=0;
    int samp=0;
    float mean=0;
    float meanSq=0;
    struct timeval timeStruct;
    static int thePriority=0;
    if(evNum==1){
	//Fake some data
	for(chan=0;chan<NUM_DIGITZED_CHANNELS;chan++) {
	    mean=0;
	    meanSq=0;
	    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
		short value=(short)gaussianRand(2048,200);
//		value+=((evNum%4)<<14);
		theBody.channel[chan].data[samp]=value;
		mean+=value;
		meanSq+=value*value;
//		printf("%d\t%d\n",samp,theBody.channel[chan].data[samp]);
	    }
	    meanSq/=(float)MAX_NUMBER_SAMPLES;
	    mean/=(float)MAX_NUMBER_SAMPLES;
	    theBody.channel[chan].header.mean=mean;
	    theBody.channel[chan].header.rms=sqrt(meanSq-mean*mean);
	}
    }
    gettimeofday(&timeStruct,NULL);
//    theHeader.gHdr.feByte=0xfe;
//    theHeader.gHdr.verId=VER_EVENT_HEADER;
//    theHeader.gHdr.code=PACKET_HD;
//    theHeader.gHdr.numBytes=sizeof(AnitaEventHeader_t);
//    theHeader.turfio.trigType=trigType;
    theHeader.unixTime=timeStruct.tv_sec;
    theHeader.unixTimeUs=timeStruct.tv_usec;
//    theHeader.numChannels=16;
    theHeader.eventNumber=evNum;
    theHeader.priority=thePriority;
    thePriority++;
    if(thePriority>9) thePriority=0;
    evNum++;      
    	    
    /* Write output for SIPd*/	
    fillGenericHeader(&theHeader,PACKET_HD,sizeof(AnitaEventHeader_t));    
    retVal=checkPacket(&theHeader);
    if(retVal) 
	printf("Problem with AnitaEventHeader_t %d\n",retVal);

//    writePackets(&theBody,&theHeader);
    
    //Write body and header for Archived
    sprintf(archiveFilename,"%s/ev_%lu.dat",PRIORITIZERD_EVENT_DIR,
	    theHeader.eventNumber);
    writeBody(&theBody,archiveFilename);
    sprintf(archiveFilename,"%s/hd_%lu.dat",PRIORITIZERD_EVENT_DIR,
	    theHeader.eventNumber);
    writeHeader(&theHeader,archiveFilename);
    makeLink(archiveFilename,PRIORITIZERD_EVENT_LINK_DIR);
    

    
    //Move and link header
    sprintf(sipdHdFilename,"%s/hd_%lu.dat",HEADER_TELEM_DIR,
	    theHeader.eventNumber);
    writeHeader(&theHeader,sipdHdFilename);
    makeLink(sipdHdFilename,HEADER_TELEM_LINK_DIR);


    if(evNum%100==0)
	printf("Event %d, sec %ld, millisec %ld\n",evNum,theHeader.unixTime,theHeader.unixTimeUs);
       
}

float getTimeDiff(struct timeval oldTime, struct timeval currentTime) {
    float timeDiff=currentTime.tv_sec-oldTime.tv_sec;
    timeDiff+=1e-6*(float)(currentTime.tv_usec-oldTime.tv_usec);
    return timeDiff;
}


void fakeSurfHk(struct timeval *currentTime) {
    
    FullSurfHkStruct_t theSurfHk;
    char theFilename[FILENAME_MAX];
    int retVal=0,i=0,j=0;


    theSurfHk.unixTime=currentTime->tv_sec;
    theSurfHk.unixTimeUs=currentTime->tv_usec;
    for(i=0;i<ACTIVE_SURFS;i++) {
	theSurfHk.upperWords[i]=i;
	for(j=0;j<SCALERS_PER_SURF;j++) {
	    theSurfHk.scaler[i][j]=200+i*j;
	    theSurfHk.threshold[i][j]=4000-i*j;
	}
	for(j=0;j<RFCHAN_PER_SURF;j++) {
	    theSurfHk.rfPower[i][j]=1000+i*j;

	}
    }
	    


    fillGenericHeader(&theSurfHk,PACKET_SURF_HK,sizeof(FullSurfHkStruct_t));
    retVal=checkPacket(&theSurfHk);
    if(retVal) 
	printf("Problem with FullSurfHkStruct_t %d\n",retVal);
    sprintf(theFilename,"%s/surfhk_%ld.dat",
	    SURFHK_TELEM_DIR,theSurfHk.unixTime);
    retVal=writeSurfHk(&theSurfHk,theFilename);
    retVal=makeLink(theFilename,SURFHK_TELEM_LINK_DIR);

}


void fakeTurfRate(struct timeval *currentTime) {
    
    TurfRateStruct_t theTurfRates;
    char theFilename[FILENAME_MAX];
    int retVal=0,i=0,j=0;
    theTurfRates.unixTime=currentTime->tv_sec;
    theTurfRates.unixTimeUs=currentTime->tv_usec;
    for(i=0;i<TRIGGER_SURFS;i++) {
	for(j=0;j<ANTS_PER_SURF;j++) {
	    theTurfRates.l1Rates[i][j]=i*j;
	}
    }
    for(i=0;i<PHI_SECTORS;i++) {
	theTurfRates.upperL2Rates[i]=255-i;
	theTurfRates.lowerL2Rates[i]=i;
	theTurfRates.l3Rates[i]=128+i;
    }

    fillGenericHeader(&theTurfRates,PACKET_TURF_RATE,sizeof(TurfRateStruct_t));
    retVal=checkPacket(&theTurfRates);
    if(retVal) 
	printf("Problem with TurfRateStruct_t %d\n",retVal);
    sprintf(theFilename,"%s/turfrate_%ld.dat",
	    TURFHK_TELEM_DIR,theTurfRates.unixTime);
    retVal=writeTurfRate(&theTurfRates,theFilename);
    retVal=makeLink(theFilename,TURFHK_TELEM_LINK_DIR);

}


void fakeMonitor(struct timeval *currentTime) {
// Monitord struct
    MonitorStruct_t theMon;
    char theFilename[FILENAME_MAX];
    int retVal=0,i=0;
//    theMon.gHdr.code=PACKET_MONITOR;
//    theMon.gHdr.numBytes=sizeof(MonitorStruct_t);
//    theMon.gHdr.feByte=0xfe;
//    theMon.gHdr.verId=VER_MONITOR;
    theMon.unixTime=currentTime->tv_sec;
    theMon.diskInfo.mainDisk=10;
    for(i=0;i<64;i++) {
	theMon.diskInfo.usbDisk[i]=100-i;
    }
    theMon.queueInfo.cmdLinks=0;
    theMon.queueInfo.hkLinks=10;
    theMon.queueInfo.gpsLinks=30;
    theMon.queueInfo.monitorLinks=20;
    for(i=0;i<10;i++) {
	theMon.queueInfo.eventLinks[i]=i*10;
    }

    fillGenericHeader(&theMon,PACKET_MONITOR,sizeof(MonitorStruct_t));
    retVal=checkPacket(&theMon);
    if(retVal) 
	printf("Problem with MonitorStruct_t %d\n",retVal);
    sprintf(theFilename,"%s/mon_%ld.dat",
	    MONITOR_TELEM_DIR,theMon.unixTime);
    retVal=writeMonitor(&theMon,theFilename);
    retVal=makeLink(theFilename,MONITOR_TELEM_LINK_DIR);

}
	    

void fakeAdu5Pat(struct timeval *currentTime) {

    GpsAdu5PatStruct_t thePat;
    char theFilename[FILENAME_MAX];
    int retVal;
//    thePat.gHdr.code=PACKET_GPS_ADU5_PAT;
//    thePat.gHdr.numBytes=sizeof(GpsAdu5PatStruct_t);
    thePat.unixTime=currentTime->tv_sec;
    thePat.unixTimeUs=currentTime->tv_usec;
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
    fillGenericHeader(&thePat,PACKET_GPS_ADU5_PAT,sizeof(GpsAdu5PatStruct_t));
    retVal=checkPacket(&thePat);
    if(retVal) 
	printf("Problem with GpsAdu5PatStruct_t %d\n",retVal);
    sprintf(theFilename,"%s/pat_%ld_%ld.dat",GPS_TELEM_DIR,thePat.unixTime,thePat.unixTimeUs);
//    printf("%s -- code %d -- numBytes %d\n",theFilename,thePat.gHdr.code,thePat.gHdr.numBytes);
    retVal=writeGpsPat(&thePat,theFilename);  
    retVal=makeLink(theFilename,GPS_TELEM_LINK_DIR); 
}


void fakeAdu5Vtg(struct timeval *currentTime) {

    GpsAdu5VtgStruct_t theVtg;
    char theFilename[FILENAME_MAX];
    int retVal;
//    theVtg.gHdr.code=PACKET_GPS_ADU5_VTG;
//    theVtg.gHdr.numBytes=sizeof(GpsAdu5VtgStruct_t);
    theVtg.unixTime=currentTime->tv_sec;
    theVtg.unixTimeUs=currentTime->tv_usec;
    theVtg.trueCourse=10.5;
    theVtg.magneticCourse=9.5;
    theVtg.speedInKnots=50;
    theVtg.speedInKPH=92.6;


    //Write file and link for sipd
    fillGenericHeader(&theVtg,PACKET_GPS_ADU5_VTG,sizeof(GpsAdu5VtgStruct_t));
    retVal=checkPacket(&theVtg);
    if(retVal) 
	printf("Problem with GpsAdu5VtgStruct_t %d\n",retVal);
    sprintf(theFilename,"%s/vtg_%ld_%ld.dat",GPS_TELEM_DIR,theVtg.unixTime,theVtg.unixTimeUs);
    retVal=writeGpsVtg(&theVtg,theFilename);  
    retVal=makeLink(theFilename,GPS_TELEM_LINK_DIR); 
}


void fakeAdu5Sat(struct timeval *currentTime) {
    GpsAdu5SatStruct_t theSat;
    char theFilename[FILENAME_MAX];
    int retVal,antNum;
    int satNum;


//    theSat.gHdr.code=PACKET_GPS_ADU5_SAT;
//    theSat.gHdr.numBytes=sizeof(GpsAdu5SatStruct_t);
    theSat.unixTime=currentTime->tv_sec;
    for(antNum=0;antNum<4;antNum++) {
	theSat.numSats[antNum]=3;
	for(satNum=0;satNum<(char)theSat.numSats[antNum];satNum++) {
	    theSat.sat[antNum][satNum].prn=satNum+10;
	    theSat.sat[antNum][satNum].azimuth=15;
	    theSat.sat[antNum][satNum].elevation=15;
	    theSat.sat[antNum][satNum].snr=99;
//	theSat.sat[satNum].flag=0;
	}
    }

    fillGenericHeader(&theSat,PACKET_GPS_ADU5_SAT,sizeof(GpsAdu5SatStruct_t));
    retVal=checkPacket(&theSat);
    if(retVal) 
	printf("Problem with GpsAdu5SatStruct_t %d\n",retVal);
    sprintf(theFilename,"%s/sat_adu5_%ld.dat",GPS_TELEM_DIR,theSat.unixTime);
    retVal=writeGpsAdu5Sat(&theSat,theFilename);  
    retVal=makeLink(theFilename,GPS_TELEM_LINK_DIR);
}


void fakeG12Pos(struct timeval *currentTime) {

    GpsG12PosStruct_t thePos;
    char theFilename[FILENAME_MAX];
    int retVal;
//    thePos.gHdr.code=PACKET_GPS_ADU5_PAT;
//    thePos.gHdr.numBytes=sizeof(GpsG12PosStruct_t);
    thePos.unixTime=currentTime->tv_sec;
    thePos.unixTimeUs=currentTime->tv_usec;
    thePos.numSats=9;
    thePos.latitude=34.489885;
    thePos.longitude=-1*104.2218668;
    thePos.altitude=1238.38;
    thePos.trueCourse=9.5;
    thePos.verticalVelocity=1.5;
    thePos.speedInKnots=50;
    thePos.pdop=0.5;
    thePos.hdop=0.5;
    thePos.vdop=0.5;
    thePos.tdop=0.5;

    fillGenericHeader(&thePos,PACKET_GPS_G12_POS,sizeof(GpsG12PosStruct_t));
    retVal=checkPacket(&thePos);
    if(retVal) 
	printf("Problem with GpsG12PosStruct_t %d\n",retVal);
    //Write file and link for sipd
    sprintf(theFilename,"%s/pos_%ld_%ld.dat",GPS_TELEM_DIR,thePos.unixTime,thePos.unixTimeUs);
    retVal=writeGpsPos(&thePos,theFilename);  
    retVal=makeLink(theFilename,GPS_TELEM_LINK_DIR); 
}


void fakeG12Sat(struct timeval *currentTime) {
    GpsG12SatStruct_t theSat;
    char theFilename[FILENAME_MAX];
    int retVal;
    int satNum;


//    theSat.gHdr.code=PACKET_GPS_G12_SAT;
//    theSat.gHdr.numBytes=sizeof(GpsG12SatStruct_t);
    theSat.unixTime=currentTime->tv_sec;
    theSat.numSats=3;
    for(satNum=0;satNum<(char)theSat.numSats;satNum++) {
	theSat.sat[satNum].prn=satNum+10;
	theSat.sat[satNum].azimuth=15;
	theSat.sat[satNum].elevation=15;
	theSat.sat[satNum].snr=99;
//	theSat.sat[satNum].flag=0;
    }
    

    fillGenericHeader(&theSat,PACKET_GPS_G12_SAT,sizeof(GpsG12SatStruct_t));
    retVal=checkPacket(&theSat);
    if(retVal) 
	printf("Problem with GpsG12SatStruct_t %d\n",retVal);
    sprintf(theFilename,"%s/sat_%ld.dat",GPS_TELEM_DIR,theSat.unixTime);
    retVal=writeGpsG12Sat(&theSat,theFilename);  
    retVal=makeLink(theFilename,GPS_TELEM_LINK_DIR);
}



void fakeHkCal(struct timeval *currentTime) 
{

    int retVal,chan,board;
    char theFilename[FILENAME_MAX];
    char fullFilename[FILENAME_MAX];
    HkDataStruct_t theHkData;
    float randomNum;
//    theHkData.gHdr.code=PACKET_HKD;
//    theHkData.gHdr.numBytes=sizeof(HkDataStruct_t);
    theHkData.unixTime=currentTime->tv_sec;
    theHkData.unixTimeUs=currentTime->tv_usec;
    theHkData.ip320.code=IP320_CAL;
    for(board=0;board<NUM_IP320_BOARDS;board++) {
	for(chan=0;chan<CHANS_PER_IP320;chan++) {
	    randomNum=rand();
	    theHkData.ip320.board[board].data[chan]=4095-(unsigned short)(10.*randomNum);
	}
    }
    theHkData.mag.x=2.3;
    theHkData.mag.y=2.3;
    theHkData.mag.z=2.3;
    theHkData.sbs.temp[0]=25.5;
    theHkData.sbs.temp[1]=25.5;   
     	
    sprintf(theFilename,"hk_%ld_%ld.cal.dat",theHkData.unixTime,theHkData.unixTimeUs);    
    //Write file and make link for SIPd
    fillGenericHeader(&theHkData,PACKET_HKD,sizeof(HkDataStruct_t));
    retVal=checkPacket(&theHkData);
    if(retVal) 
	printf("Problem with HkDataStruct_t %d\n",retVal);
    sprintf(fullFilename,"%s/%s",HK_TELEM_DIR,theFilename);
    retVal=writeHk(&theHkData,fullFilename);     
    retVal+=makeLink(fullFilename,HK_TELEM_LINK_DIR);

    sprintf(theFilename,"hk_%ld_%ld.avz.dat",theHkData.unixTime,theHkData.unixTimeUs);
    theHkData.ip320.code=IP320_AVZ;
    for(board=0;board<NUM_IP320_BOARDS;board++) {
	for(chan=0;chan<CHANS_PER_IP320;chan++) {
	    randomNum=rand();
	    theHkData.ip320.board[board].data[chan]=2053-(unsigned short)(10.*randomNum);
	}
    }    
    //Write file and make link for SIPd
    fillGenericHeader(&theHkData,PACKET_HKD,sizeof(HkDataStruct_t));
    retVal=checkPacket(&theHkData);
    if(retVal) 
	printf("Problem with HkDataStruct_t %d\n",retVal);
    sprintf(fullFilename,"%s/%s",HK_TELEM_DIR,theFilename);
    retVal=writeHk(&theHkData,fullFilename);     
    retVal+=makeLink(fullFilename,HK_TELEM_LINK_DIR);      

}

void fakeHkRaw(struct timeval *currentTime) 
{
    int retVal,chan,board;
    char theFilename[FILENAME_MAX];
    char fullFilename[FILENAME_MAX];
    HkDataStruct_t theHkData;
    float randomNum;
//    theHkData.gHdr.code=PACKET_HKD;
//    theHkData.gHdr.numBytes=sizeof(HkDataStruct_t);
    theHkData.unixTime=currentTime->tv_sec;
    theHkData.unixTimeUs=currentTime->tv_usec;
    theHkData.ip320.code=IP320_RAW;    
    for(board=0;board<NUM_IP320_BOARDS;board++) {
	for(chan=0;chan<CHANS_PER_IP320;chan++) {
	    theHkData.ip320.board[board].data[chan]=2048 + chan;
	    randomNum=rand();
	    randomNum-=0.5;
	    theHkData.ip320.board[board].data[chan]+=(int)(20*randomNum);
	}
    }
    theHkData.mag.x=2.3;
    theHkData.mag.y=2.3;
    theHkData.mag.z=2.3;
    theHkData.sbs.temp[0]=25;
    theHkData.sbs.temp[1]=27;   
    fillGenericHeader(&theHkData,PACKET_HKD,sizeof(HkDataStruct_t));
    retVal=checkPacket(&theHkData);
    if(retVal) 
	printf("Problem with HkDataStruct_t %d\n",retVal);
    sprintf(theFilename,"hk_%ld_%ld.raw.dat",theHkData.unixTime,theHkData.unixTimeUs);
    
    //Write file and make link for SIPd
    sprintf(fullFilename,"%s/%s",HK_TELEM_DIR,theFilename);
    retVal=writeHk(&theHkData,fullFilename);     
    retVal+=makeLink(fullFilename,HK_TELEM_LINK_DIR);    
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

