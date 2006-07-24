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
	
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    int chanIndex=GetChanIndex(surf,chan);		
	    labChip=bdPtr->channel[chanIndex].header.chipIdFlag&0x3;
	    if(chan==0) {
		thePeds.chipEntries[surf][labChip]++;
	    }
//		cout << surf << "\t" << chan << "\t" << chanIndex
//		     << "\t" << theBody.channel[chanIndex].header.chipIdFlag
//		     << endl;

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
    memset(&thePeds,0,sizeof(PedCalcStruct_t));
    thePeds.unixTimeStart=time(NULL);
}


void writePedestals() {
    PedestalStruct_t usefulPeds;
    FullLabChipPedStruct_t labPeds;
    char filename[FILENAME_MAX];
//    char linkname[FILENAME_MAX];
    time_t rawtime;
    int surf,chip,chan,samp;
    float tempFloat;
    unsigned long avgSamples=0;
//    unsigned short tempShort;
//    unsigned char tempChar;
    
    time(&rawtime);
    thePeds.unixTimeEnd=rawtime;
    usefulPeds.unixTime=rawtime;

    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chip=0;chip<LABRADORS_PER_SURF;chip++) {
	    memset(&labPeds,0,sizeof(FullLabChipPedStruct_t));	   
	    labPeds.unixTimeStart=thePeds.unixTimeStart;
	    labPeds.unixTimeEnd=rawtime;
	    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
		labPeds.pedChan[chan].chipId=chip;
		labPeds.pedChan[chan].chanId=GetChanIndex(surf,chan);
		labPeds.pedChan[chan].chipEntries=thePeds.chipEntries[surf][chip];
		for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
		    avgSamples+=thePeds.entries[surf][chip][chan][samp];
		    if(thePeds.entries[surf][chip][chan][samp]) {
			tempFloat=
			    ((float)thePeds.mean[surf][chip][chan][samp])/
			    ((float)thePeds.entries[surf][chip][chan][samp]);
			    
			thePeds.fmean[surf][chip][chan][samp]=tempFloat;
						
			tempFloat=roundf(thePeds.fmean[surf][chip][chan][samp]);

			//Set pedestals
			if(tempFloat>65535) tempFloat=65535;
			if(tempFloat<0) tempFloat=0;
			labPeds.pedChan[chan].pedMean[samp]=
			    (unsigned short) tempFloat;
			usefulPeds.thePeds[surf][chip][chan][samp]=
			    (unsigned short) tempFloat;
			    
			
			//Calculate RMS
			tempFloat=
			    ((float)thePeds.meanSq[surf][chip][chan][samp])/
			    ((float)thePeds.entries[surf][chip][chan][samp]);
			    
			tempFloat=tempFloat-
			    (thePeds.fmean[surf][chip][chan][samp]*
			     thePeds.fmean[surf][chip][chan][samp]);
			if(tempFloat) {
			    thePeds.frms[surf][chip][chan][samp]=tempFloat;
			    tempFloat=roundf(10.*tempFloat);
			    if(tempFloat>255) tempFloat=255;
			    if(tempFloat<0) tempFloat=0;
			    labPeds.pedChan[chan].pedRMS[samp]=
				(unsigned char) tempFloat;
			    usefulPeds.pedsRMS[surf][chip][chan][samp]=
				(unsigned char) tempFloat;
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
    sprintf(filename,"%s/peds_%lu.dat",PEDESTAL_DIR,usefulPeds.unixTime);
    writeUsefulPedStruct(&usefulPeds,filename);

    //and link it
    symlink(filename,CURRENT_PEDESTALS);

}


int subtractCurrentPeds(AnitaEventBody_t *rawBdPtr,
			PedSubbedEventBody_t *pedSubBdPtr) {
    static int pedsLoaded=0;
    int surf,chan,samp,chip;
    
    int chanIndex=0;
    if(pedsLoaded) {
	if(!checkCurrentPedLink()) pedsLoaded=0;
    }
    
    if(!pedsLoaded) {
	loadCurrentPeds();
	pedsLoaded=1;
    }


    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chan=0;chan<ACTIVE_SURFS;chan++) {
	    chanIndex=GetChanIndex(surf,chan);
	    pedSubBdPtr->channel[chanIndex].header=
		rawBdPtr->channel[chanIndex].header;
	    chip=rawBdPtr->channel[chanIndex].header.chipIdFlag&0x3;
	    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {		
	    pedSubBdPtr->channel[chanIndex].data[samp]=
		((short)rawBdPtr->channel[chanIndex].data[samp])-
		((short)currentPeds.thePeds[surf][chip][chan][samp]);
	    }
	}
    }
		
    return 0;
    
}

int addCurrentPeds(PedSubbedEventBody_t *pedSubBdPtr,
		   AnitaEventBody_t *rawBdPtr);


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
    int surf,chip,chan,samp;
    int retVal=fillUsefulPedStruct(&currentPeds,CURRENT_PEDESTALS);
    if(!retVal) {
	//Can't load pedestals
	syslog(LOG_ERR,"Can't load current pedestals defaulting to %d\n",
	       PED_DEFAULT_VALUE);
	fprintf(stderr,"Can't load current pedestals defaulting to %d\n",
	       PED_DEFAULT_VALUE);
	currentPeds.unixTime=PED_DEFAULT_VALUE;
	currentPeds.nsamples=0;
	for(surf=0;surf<ACTIVE_SURFS;surf++) {
	    for(chip=0;chip<ACTIVE_SURFS;chip++) {
		for(chan=0;chan<ACTIVE_SURFS;chan++) {
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


