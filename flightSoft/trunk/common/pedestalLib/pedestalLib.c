/*! \file pedestalLib.c
    \brief Pedestal library, full of (hopefully) useful stuff
    
    Some functions that do some useful things
    October 2004  rjn@mps.ohio-state.edu
*/
#include "pedestalLib/pedestalLib.h"
#include "anitaStructures.h"

// Standard Includes
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
//#include <iostream>


// Anita includes
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"

PedCalcStruct_t thePeds;
PedestalStruct_t currentPeds;


void addEventToPedestals(AnitaEventBody_t *bdPtr) 
{
    int labChip=0,surf,chan,samp;
    int word;
//    printf("RJN -- Event %lu\n",bdPtr->eventNumber);
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    int chanIndex=GetChanIndex(surf,chan);		
	    labChip=bdPtr->channel[chanIndex].header.chipIdFlag&0x3;
	    if(chan==0) {
		thePeds.chipEntries[surf][labChip]++;
	    }
//	    fprintf(stderr,"RJN -- %d %d %d --- %d %d %d %d\n",surf, chan,chanIndex,
//		    bdPtr->channel[chanIndex].header.chanId,
//		    bdPtr->channel[chanIndex].header.chipIdFlag,
//		    bdPtr->channel[chanIndex].header.firstHitbus,
//		    bdPtr->channel[chanIndex].header.lastHitbus
//		    );

	    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
		word=bdPtr->channel[chanIndex].data[samp];
		if(word&0x1000) //HITBUS
		    continue;
		if(!(word&0xfff)) continue; //Zeros are junk
		if(samp>0) {
		    if(bdPtr->channel[chanIndex].data[samp-1] &0x1000)
			continue;
		}
		if(samp<255) {
		    if(bdPtr->channel[chanIndex].data[samp+1] &0x1000)
			continue;
		}
		
//		    if(surf==0 && chan==0 && samp==0) cout << labChip << endl;
		
		thePeds.mean[surf][labChip][chan][samp]+=word&SURF_BITMASK;
		thePeds.meanSq[surf][labChip][chan][samp]+=(word&SURF_BITMASK)*(word&SURF_BITMASK);
		thePeds.entries[surf][labChip][chan][samp]++;
	    }
	}
    }

}

void resetPedCalc() {
    resetPedCalcWithTime(time(NULL));
}


void resetPedCalcWithTime(unsigned long unixTime) {
    makeDirectories(HEADER_TELEM_LINK_DIR);
    makeDirectories(PEDESTAL_DIR);
    memset(&thePeds,0,sizeof(PedCalcStruct_t));
    thePeds.unixTimeStart=unixTime;
}


void writePedestals() {
    writePedestalsWithTime(time(NULL));
}

void writePedestalsWithTime(unsigned long unixTime) {
    PedestalStruct_t usefulPeds;
    FullLabChipPedStruct_t labPeds;
    char filename[FILENAME_MAX];
//    char linkname[FILENAME_MAX];
    int surf,chip,chan,samp;
    float tempFloat;
    unsigned long avgSamples=0;
//    unsigned short tempShort;
//    unsigned char tempChar;
    
    thePeds.unixTimeEnd=unixTime;
    usefulPeds.unixTime=unixTime;

    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chip=0;chip<LABRADORS_PER_SURF;chip++) {
	    memset(&labPeds,0,sizeof(FullLabChipPedStruct_t));	   
	    labPeds.unixTimeStart=thePeds.unixTimeStart;
	    labPeds.unixTimeEnd=unixTime;
	    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
		labPeds.pedChan[chan].chipId=chip;
		labPeds.pedChan[chan].chanId=GetChanIndex(surf,chan);
		labPeds.pedChan[chan].chipEntries=thePeds.chipEntries[surf][chip];
		for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
		    avgSamples+=thePeds.entries[surf][chip][chan][samp];
/* 		    printf("%d %d %d %d -- %lu %lu %lu\n",surf,chip,chan,samp, */
/* 			   thePeds.entries[surf][chip][chan][samp], */
/* 			   thePeds.mean[surf][chip][chan][samp], */
/* 			   thePeds.meanSq[surf][chip][chan][samp]); */
			   
		    if(thePeds.entries[surf][chip][chan][samp]>1) {
			tempFloat=
			    ((float)thePeds.mean[surf][chip][chan][samp])/
			    ((float)thePeds.entries[surf][chip][chan][samp]);
			    
			thePeds.fmean[surf][chip][chan][samp]=tempFloat;
						
			tempFloat=(int)(thePeds.fmean[surf][chip][chan][samp]);
			if((thePeds.fmean[surf][chip][chan][samp]-tempFloat)>=
			   0.5) tempFloat++;
			
			
			//Set pedestals
			if(tempFloat>65535) tempFloat=65535;
			if(tempFloat<0) tempFloat=0;
			labPeds.pedChan[chan].pedMean[samp]=
			    (unsigned short) tempFloat;
			usefulPeds.thePeds[surf][chip][chan][samp]=
			    (unsigned short) tempFloat;
			
/* 			printf("%lu\t%f\t%d\t%d\n", */
/* 			       thePeds.mean[surf][chip][chan][samp], */
/* 			       thePeds.fmean[surf][chip][chan][samp], */
/* 			       labPeds.pedChan[chan].pedMean[samp], */
/* 			       usefulPeds.thePeds[surf][chip][chan][samp]); */
			       
			
			//Calculate RMS
			tempFloat=
			    ((float)thePeds.meanSq[surf][chip][chan][samp])/
			    ((float)thePeds.entries[surf][chip][chan][samp]);
			    
			tempFloat=tempFloat-
			    (thePeds.fmean[surf][chip][chan][samp]*
			     thePeds.fmean[surf][chip][chan][samp]);
			if(tempFloat) {
			    thePeds.frms[surf][chip][chan][samp]=tempFloat;
			    tempFloat*=10;
			    int tempNum=(int)tempFloat;
			    if((tempFloat-tempNum)>=0.5)tempNum++;
			    if(tempNum>255) tempNum=255;
			    if(tempNum<0) tempNum=0;
			    labPeds.pedChan[chan].pedRMS[samp]=
				(unsigned char) tempNum;
			    usefulPeds.pedsRMS[surf][chip][chan][samp]=
				(unsigned char) tempNum;
			}
			else {
			    labPeds.pedChan[chan].pedRMS[samp]=0;
			}
			    
		    }
		    else {
			labPeds.pedChan[chan].pedMean[samp]=0;
			labPeds.pedChan[chan].pedRMS[samp]=0;
		    }					    
		}
	    }
	    
	    //Now write lab files
	    fillGenericHeader(&labPeds,PACKET_LAB_PED,sizeof(FullLabChipPedStruct_t));
	    sprintf(filename,"%s/labpeds_%lu_%d_%d.dat",PEDESTAL_DIR,labPeds.unixTimeEnd,surf,chip);
//	    printf("Writing lab pedestalFile: %s\n",filename);
	    writeLabChipPedStruct(&labPeds,filename);

	    //And for telemetery
	    sprintf(filename,"%s/labpeds_%lu_%d_%d.dat",HEADER_TELEM_DIR,labPeds.unixTimeEnd,surf,chip);
	    writeLabChipPedStruct(&labPeds,filename);

	    //and link it
	    makeLink(filename,HEADER_TELEM_LINK_DIR);
	    

	}
    }
    avgSamples/=(ACTIVE_SURFS*LABRADORS_PER_SURF*CHANNELS_PER_SURF*MAX_NUMBER_SAMPLES);
    usefulPeds.nsamples=avgSamples;
    //Now write ped calc file
    sprintf(filename,"%s/calcpeds_%lu.dat",PEDESTAL_DIR,thePeds.unixTimeEnd);
    writePedCalcStruct(&thePeds,filename);

    //Now write full ped file
//    dumpThesePeds(&usefulPeds);
    sprintf(filename,"%s/peds_%lu.dat",PEDESTAL_DIR,usefulPeds.unixTime);
    writeUsefulPedStruct(&usefulPeds,filename);

    //and link it
    symlink(filename,CURRENT_PEDESTALS);

}


int subtractCurrentPeds(AnitaEventBody_t *rawBdPtr,
			PedSubbedEventBody_t *pedSubBdPtr) {
    static int pedsLoaded=0;
    if(pedsLoaded) {
	if(!checkCurrentPedLink()) pedsLoaded=0;
    }
    
    if(!pedsLoaded) {
	loadCurrentPeds();
	pedsLoaded=1;
    }
    return doPedSubtraction(rawBdPtr,pedSubBdPtr);
}


int subtractThesePeds(AnitaEventBody_t *rawBdPtr,
		      PedSubbedEventBody_t *pedSubBdPtr,
		      unsigned long whichPeds) 
{
    loadThesePeds(whichPeds);
    return doPedSubtraction(rawBdPtr,pedSubBdPtr);
    
}


int subtractPedsFromFile(AnitaEventBody_t *rawBdPtr,
			 PedSubbedEventBody_t *pedSubBdPtr,
			 char *filename) {
    loadPedsFromFile(filename);
    return doPedSubtraction(rawBdPtr,pedSubBdPtr);
}


int doPedSubtraction(AnitaEventBody_t *rawBdPtr,
		     PedSubbedEventBody_t *pedSubBdPtr) {
    pedSubBdPtr->whichPeds=currentPeds.unixTime;
    int surf,chan,samp,chip;   
    int chanIndex=0;

    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    chanIndex=GetChanIndex(surf,chan);
	    pedSubBdPtr->channel[chanIndex].xMax=-4095;
	    pedSubBdPtr->channel[chanIndex].xMin=4095;

	    pedSubBdPtr->channel[chanIndex].header=
		rawBdPtr->channel[chanIndex].header;
	    chip=rawBdPtr->channel[chanIndex].header.chipIdFlag&0x3;
	    float nsamps=0;
	    float meanSq=0;
	    float mean=0;
	    short dataVal=0;
	    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {	
		if(rawBdPtr->channel[chanIndex].data[samp]&HITBUS_MASK) {
		    //HITBUS is on
		    pedSubBdPtr->channel[chanIndex].data[samp]=0;
		}
		else {
		    dataVal=((short)(rawBdPtr->channel[chanIndex].data[samp]&ELEVEN_BITMASK))-
			((short)currentPeds.thePeds[surf][chip][chan][samp]);
		    pedSubBdPtr->channel[chanIndex].data[samp]=dataVal;
			
		    if(dataVal>pedSubBdPtr->channel[chanIndex].xMax) {
			pedSubBdPtr->channel[chanIndex].xMax=dataVal;
		    }
		    if(dataVal<pedSubBdPtr->channel[chanIndex].xMin) {
			pedSubBdPtr->channel[chanIndex].xMin=dataVal;
		    }
		    mean+=dataVal;
		    meanSq+=(dataVal*dataVal);
		    nsamps++;		    
		}
	    }
	    pedSubBdPtr->channel[chanIndex].mean=0;
	    pedSubBdPtr->channel[chanIndex].rms=0;
	    if(nsamps) {
		mean/=nsamps;
		meanSq/=nsamps;
		pedSubBdPtr->channel[chanIndex].mean=mean;
		if(meanSq>mean*mean)
		    pedSubBdPtr->channel[chanIndex].rms=sqrt(meanSq-mean*mean);
	    }
	    
	}
    }		
    return 0;    
}

int addPeds(PedSubbedEventBody_t *pedSubBdPtr,
	    AnitaEventBody_t *rawBdPtr)
{

    loadThesePeds(pedSubBdPtr->whichPeds);
    return doPedAddition(pedSubBdPtr,rawBdPtr);
}

int addThesePeds(PedSubbedEventBody_t *pedSubBdPtr,
		 AnitaEventBody_t *rawBdPtr,
		 unsigned long whichPeds)
{

    loadThesePeds(whichPeds);
    return doPedAddition(pedSubBdPtr,rawBdPtr);
}

int addPedsFromFile(PedSubbedEventBody_t *pedSubBdPtr,
		    AnitaEventBody_t *rawBdPtr,
		    char *filename)
{

    loadPedsFromFile(filename);
    return doPedAddition(pedSubBdPtr,rawBdPtr);
}

int doPedAddition(PedSubbedEventBody_t *pedSubBdPtr,
		  AnitaEventBody_t *rawBdPtr)
{
    
    int surf,chan,samp,chip;    
    int chanIndex=0;

    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    chanIndex=GetChanIndex(surf,chan);
	    rawBdPtr->channel[chanIndex].header=
		pedSubBdPtr->channel[chanIndex].header;
	    chip=rawBdPtr->channel[chanIndex].header.chipIdFlag&0x3;
	    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {		
		rawBdPtr->channel[chanIndex].data[samp]=(unsigned short) 
		    (((short)pedSubBdPtr->channel[chanIndex].data[samp])+
		    ((short)currentPeds.thePeds[surf][chip][chan][samp]));
	    }
	}
    }
		
    return 0;

}


int checkCurrentPedLink() {
    char pedname[FILENAME_MAX];
    char crapBuf[FILENAME_MAX];
    unsigned long pedUnixTime;
    int retVal=readlink(CURRENT_PEDESTALS,pedname,FILENAME_MAX);
    if(retVal==-1) {
	syslog(LOG_ERR,"Error reading current pedestals link: %s\n",
	       strerror(errno));
	fprintf(stderr,"Error reading current pedestals link: %s\n",
	       strerror(errno));
	return 0;
    }
    sscanf(pedname,"%s/peds_%lu.dat",crapBuf,&pedUnixTime);
    if(pedUnixTime!=currentPeds.unixTime) return 0;
    return 1;
}


int loadCurrentPeds()
{
    int retVal;
    static int pedsLoaded=0;

    if(pedsLoaded) {
	if(!checkCurrentPedLink()) pedsLoaded=0;
    }
    if(pedsLoaded) return 0;           
    retVal=loadPedsFromFile(CURRENT_PEDESTALS);
    if(retVal==0) pedsLoaded=1;
    return retVal;
}


int loadThesePeds(unsigned long whichPeds)
{
    static int pedsLoaded=0;
    int retVal;
    char filename[180];

    if(pedsLoaded) {
	if(currentPeds.unixTime!=whichPeds) pedsLoaded=0;
    }
    if(pedsLoaded) return 0;           
    sprintf(filename,"%s/peds_%lu.dat",PEDESTAL_DIR,whichPeds);
    retVal=loadPedsFromFile(filename);
    if(retVal==0) pedsLoaded=1;
    return retVal;
}



int loadPedsFromFile(char *filename) {
    static int errorCounter=0;
    int surf,chip,chan,samp;
    int retVal;

    retVal=fillUsefulPedStruct(&currentPeds,filename);
    if(retVal) {
	//Can't load pedestals
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Can't load (retVal %d) pedestals from %s defaulting to %d\n",
		   retVal,filename,
		   PED_DEFAULT_VALUE);
	    fprintf(stderr,"Can't load (retVal %d) pedestals from %s defaulting to %d\n",
		    retVal,filename,
		    PED_DEFAULT_VALUE);
	    errorCounter++;
	}
	currentPeds.unixTime=PED_DEFAULT_VALUE;
	currentPeds.nsamples=0;
	for(surf=0;surf<ACTIVE_SURFS;surf++) {
	    for(chip=0;chip<LABRADORS_PER_SURF;chip++) {
		for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
		    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
			currentPeds.thePeds[surf][chip][chan][samp]=PED_DEFAULT_VALUE;
			currentPeds.pedsRMS[surf][chip][chan][samp]=0;
		    }
		}
	    }
	}
	return -1;
    }
    return 0;
       
}

void dumpPeds() {
    if(!currentPeds.unixTime) 
	loadCurrentPeds();
    int surf,chip,chan,samp;
    printf("SURF\tCHIP\tCHAN\tSAMP:\t\tPed\tRMS(x10)\n");
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chip=0;chip<LABRADORS_PER_SURF;chip++) {
	    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
		for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
		        printf("%d\t%d\t%d\t%d:\t\t%d\t%d\n",
			       surf,chip,chan,samp,
			       currentPeds.thePeds[surf][chip][chan][samp],
			       currentPeds.pedsRMS[surf][chip][chan][samp]);
		}
	    }
	}
    }
}

void dumpThesePeds(PedestalStruct_t *pedStruct) {
    int surf,chip,chan,samp;
    printf("SURF\tCHIP\tCHAN\tSAMP:\t\tPed\tRMS(x10)\n");
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chip=0;chip<LABRADORS_PER_SURF;chip++) {
	    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
		for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
		        printf("%d\t%d\t%d\t%d:\t\t%d\t%d\n",
			       surf,chip,chan,samp,
			       pedStruct->thePeds[surf][chip][chan][samp],
			       pedStruct->pedsRMS[surf][chip][chan][samp]);
		}
	    }
	}
    }
}
