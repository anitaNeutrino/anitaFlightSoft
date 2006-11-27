/*! \file slowLib.c
    \brief Some useful functions for the calculation of the slow rate packets.
    
    Calculates things like average RF power and a couple fo other bits and pieces. 
   
   September 2006 rjn@mps.ohio-state.edu
    

*/

#include "slowLib/slowLib.h"
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

/* typedef struct { */
/*     unsigned long eventNumber; */
/*     unsigned char rfPwrAvg[ACTIVE_SURFS][RFCHAN_PER_SURF]; */
/*     unsigned char avgScalerRates[TRIGGER_SURFS][ANTS_PER_SURF]; // * 2^7 */
/*     unsigned char rmsScalerRates[TRIGGER_SURFS][ANTS_PER_SURF]; */
/*     unsigned char avgL1Rates[TRIGGER_SURFS]; // 3 of 8 counters */
/*     unsigned char avgUpperL2Rates[PHI_SECTORS];  */
/*     unsigned char avgLowerL2Rates[PHI_SECTORS]; */
/*     unsigned char avgL3Rates[PHI_SECTORS];     */
/*     unsigned char eventRate1Min; //Multiplied by 8 */
/*     unsigned char eventRate10Min; //Multiplied by 8 */
/* } SlowRateRFStruct_t; */


/* typedef struct { */
/*     float latitude; */
/*     float longitude; */
/*     float altitude; */
/*     char temps[8]; */
/*     char powers[4]; */
/* } SlowRateHkStruct_t; */



/* typedef struct {  */
/*     GenericHeader_t gHdr; */
/*     unsigned long unixTime; */
/*     unsigned long unixTimeUs; */
/*     unsigned short globalThreshold; //set to zero if there isn't one */
/*     unsigned short errorFlag; //Will define at some point     */
/*     unsigned short scalerGoal; //What are we aiming for with the scaler rate */
/*     unsigned short upperWords[ACTIVE_SURFS]; */
/*     unsigned short scaler[ACTIVE_SURFS][SCALERS_PER_SURF]; */
/*     unsigned short threshold[ACTIVE_SURFS][SCALERS_PER_SURF]; */
/*     unsigned short setThreshold[ACTIVE_SURFS][SCALERS_PER_SURF]; */
/*     unsigned short rfPower[ACTIVE_SURFS][RFCHAN_PER_SURF]; */
/*     unsigned short surfTrigBandMask[ACTIVE_SURFS][2]; */
/* } FullSurfHkStruct_t; */


/* typedef struct { */
/*     GenericHeader_t gHdr; */
/*     unsigned long unixTime; */
/*     unsigned long unixTimeUs;     */
/*     unsigned short l1Rates[TRIGGER_SURFS][ANTS_PER_SURF]; (X64) // 3 of 8 counters */
/*     unsigned char upperL2Rates[PHI_SECTORS]; (X16)*/
/*     unsigned char lowerL2Rates[PHI_SECTORS]; */
/*     unsigned char l3Rates[PHI_SECTORS]; */
/* } TurfRateStruct_t; */


typedef struct {
    float rfPwr[ACTIVE_SURFS][RFCHAN_PER_SURF];
    float scalerRates[TRIGGER_SURFS][ANTS_PER_SURF]; 
    float scalerRatesSq[TRIGGER_SURFS][ANTS_PER_SURF];
    float avgL1Rates[TRIGGER_SURFS]; // 3 of 8 counters
    float avgUpperL2Rates[PHI_SECTORS];
    float avgLowerL2Rates[PHI_SECTORS];
    float avgL3Rates[PHI_SECTORS]; 
} SlowRateRFCalcStruct_t;


FullSurfHkStruct_t theSurfHks[300];
TurfRateStruct_t theTurfRates[300];
int numSurfHks=0;
int numTurfRates=0;
int currentSurfHk=0;
int currentTurfRate=0;

float eventRates[10];
int numEventRates=0;


void writeCurrentRFSlowRateObject(float globalTriggerRate, unsigned long lastEventNumber) //to be called once a minute
{
    int i,surf,chan,ant,phi,scl;
    SlowRateRFCalcStruct_t slowCalc;
    SlowRateRFStruct_t slowRf;

//    time_t unixTime=time(NULL);
    
    if(numEventRates<10) {
	eventRates[numEventRates]=globalTriggerRate;
//	rateTimes[numEventRates]=unixTime;
	numEventRates++;
    }
    else {
	for(i=1;i<10;i++) {
	    eventRates[i-1]=eventRates[i];
//	    rateTimes[i-1]=rateTimes[i];
	}
	eventRates[9]=globalTriggerRate;
//	rateTimes[9]=unixTime;
	numEventRates=10;
    }
    float rate=(int)(8*globalTriggerRate);
    if(rate<0) rate=0;
    if(rate>255) rate=255;
    slowRf.eventNumber=lastEventNumber;
    slowRf.eventRate1Min=(unsigned char)rate;
	
    rate=0;
    for(i=0;i<numEventRates;i++) {
	rate+=eventRates[i];
    }
    rate/=((float)numEventRates);
    rate*=8;
    if(rate<0) rate=0;
    if(rate>255) rate=255;
    slowRf.eventRate10Min=(unsigned char)rate;
	    
    memset(&slowCalc,0,sizeof(SlowRateRFCalcStruct_t));
    if(numSurfHks>0) {
	for(i=0;i<numSurfHks;i++) {
	    for(surf=0;surf<ACTIVE_SURFS;surf++) {
		for(chan=0;chan<RFCHAN_PER_SURF;chan++) {
		slowCalc.rfPwr[surf][chan]+=theSurfHks[i].rfPower[surf][chan];
		}
	    }
	    for(surf=0;surf<TRIGGER_SURFS;surf++) {
		for(scl=0;scl<SCALERS_PER_SURF;scl++) {
		    float rate=theSurfHks[i].scaler[surf][scl];
		    slowCalc.scalerRates[surf][scl/8]+=rate;
		    slowCalc.scalerRatesSq[surf][scl/8]+=rate*rate;
		}
	    }
	}    
	for(surf=0;surf<ACTIVE_SURFS;surf++) {
	    for(chan=0;chan<RFCHAN_PER_SURF;chan++) {
		slowCalc.rfPwr[surf][chan]/=((float)numSurfHks);
		if(slowCalc.rfPwr[surf][chan]<0)
		    slowCalc.rfPwr[surf][chan]=0;
		if(slowCalc.rfPwr[surf][chan]>255)
		    slowCalc.rfPwr[surf][chan]=255;
		
		slowRf.rfPwrAvg[surf][chan]=slowCalc.rfPwr[surf][chan];
	    }
	}
	for(surf=0;surf<TRIGGER_SURFS;surf++) {
	    for(ant=0;ant<ANTS_PER_SURF;ant++) {
		slowCalc.scalerRates[surf][ant]/=((float)8*numSurfHks);
		slowCalc.scalerRatesSq[surf][ant]/=((float)8*numSurfHks);
		//Now scale
		slowCalc.scalerRates[surf][ant]/=((float)128);
		if(slowCalc.scalerRates[surf][ant]<0)
		    slowCalc.scalerRates[surf][ant]=0;
		if(slowCalc.scalerRates[surf][ant]>255)
		    slowCalc.scalerRates[surf][ant]=255;

		slowCalc.scalerRatesSq[surf][ant]/=((float)32);
		if(slowCalc.scalerRatesSq[surf][ant]<0)
		    slowCalc.scalerRatesSq[surf][ant]=0;
		if(slowCalc.scalerRatesSq[surf][ant]>255)
		    slowCalc.scalerRatesSq[surf][ant]=255;

	    }
	}
    }
    if(numTurfRates) {
	for(i=0;i<numTurfRates;i++) {
	    for(surf=0;surf<TRIGGER_SURFS;surf++) {
		for(ant=0;ant<ANTS_PER_SURF;ant++) {
		    slowCalc.avgL1Rates[surf]+=theTurfRates[i].l1Rates[surf][ant];
		}
	    }
	    
	    for(phi=0;phi<PHI_SECTORS;phi++) {
		slowCalc.avgUpperL2Rates[phi]+=theTurfRates[i].upperL2Rates[phi];
		slowCalc.avgLowerL2Rates[phi]+=theTurfRates[i].lowerL2Rates[phi];
		slowCalc.avgL3Rates[phi]+=theTurfRates[i].l3Rates[phi];
	    }
	}
	for(surf=0;surf<TRIGGER_SURFS;surf++) {
	    for(ant=0;ant<ANTS_PER_SURF;ant++) {
		slowCalc.avgL1Rates[surf]/=((float)4*numTurfRates);
		slowCalc.avgL1Rates[surf]/=((float)256);
		if(slowCalc.avgL1Rates[surf]<0)
		    slowCalc.avgL1Rates[surf]=0;
		if(slowCalc.avgL1Rates[surf]>255)
		    slowCalc.avgL1Rates[surf]=255;
	    }
	}
	
	for(phi=0;phi<PHI_SECTORS;phi++) {
	    slowCalc.avgUpperL2Rates[phi]/=((float)numTurfRates);
	    if(slowCalc.avgUpperL2Rates[phi]<0) 
		slowCalc.avgUpperL2Rates[phi]=0;
	    if(slowCalc.avgUpperL2Rates[phi]>255) 
		slowCalc.avgUpperL2Rates[phi]=255;
	    
	    slowCalc.avgLowerL2Rates[phi]/=((float)numTurfRates);
	    if(slowCalc.avgLowerL2Rates[phi]<0) 
		slowCalc.avgLowerL2Rates[phi]=0;
	    if(slowCalc.avgLowerL2Rates[phi]>255) 
		slowCalc.avgLowerL2Rates[phi]=255;

	    slowCalc.avgL3Rates[phi]/=((float)numTurfRates);
	    slowCalc.avgL3Rates[phi]*=((float)32);
	    if(slowCalc.avgL3Rates[phi]<0)
		slowCalc.avgL3Rates[phi]=0;
	    if(slowCalc.avgL3Rates[phi]>255)
		slowCalc.avgL3Rates[phi]=255;
	}
    }

    FILE *fp = fopen(SLOW_RF_FILE,"wb");
    if(!fp) {
	syslog(LOG_ERR,"Error opening %s: %s\n",SLOW_RF_FILE,strerror(errno));
	fprintf(stderr,"Error opening %s: %s\n",SLOW_RF_FILE,strerror(errno));
	return;
    }
    int numObjs=fwrite(&slowRf,sizeof(SlowRateRFStruct_t),1,fp);
    if(numObjs!=1) {
	syslog(LOG_ERR,"Error writing to %s: %s\n",SLOW_RF_FILE,strerror(errno));
	fprintf(stderr,"Error writing to %s: %s\n",SLOW_RF_FILE,strerror(errno));
    }
    fclose(fp);               

}



void addSurfHkToAverage(FullSurfHkStruct_t *hkStruct)
{
    if(numSurfHks) {
	int lastSurfHk=currentSurfHk-1;
	if(lastSurfHk<0) {
	    lastSurfHk=299;
	}
	if(hkStruct->unixTime<=theSurfHks[lastSurfHk].unixTime) 
	    return;
    }
    theSurfHks[currentSurfHk]=*hkStruct;
    currentSurfHk++;
    numSurfHks++;
    if(currentSurfHk>299) 
	currentSurfHk=0;
    if(numSurfHks>300)
	numSurfHks=300;    
}


void addTurfRateToAverage(TurfRateStruct_t *turfRate)
{

   if(numTurfRates) {
	int lastTurfRate=currentTurfRate-1;
	if(lastTurfRate<0) {
	    lastTurfRate=299;
	}
	if(turfRate->unixTime<=theTurfRates[lastTurfRate].unixTime) 
	    return;
    }
    theTurfRates[currentTurfRate]=*turfRate;
    currentTurfRate++;
    numTurfRates++;
    if(currentTurfRate>299) 
	currentTurfRate=0;
    if(numTurfRates>300)
	numTurfRates=300;    
}




