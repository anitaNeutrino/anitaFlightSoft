/*! \file pedestalLib.c
    \brief Pedestal library, full of (hopefully) useful stuff
    
    Some functions that do some useful things
    October 2004  rjn@mps.ohio-state.edu
*/
#define SLAC_DATA06
#include "pedestalLib/pedestalLib.h"


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
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "includes/anitaStructures.h"

PedCalcStruct_t thePeds;
PedestalStruct_t currentPeds;




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
		    dataVal=((short)(rawBdPtr->channel[chanIndex].data[samp]
				     &ELEVEN_BITMASK))-
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


int fillUsefulPedStruct(PedestalStruct_t *pedPtr, char *filename)
{
    int numBytes=genericReadOfFile((unsigned char*)pedPtr,filename,sizeof(PedestalStruct_t));
    if(numBytes==sizeof(PedestalStruct_t)) return 0;
    return numBytes;
}



int genericReadOfFile(unsigned char * buffer, char *filename, int maxBytes) 
/*!
  Reads all the bytes in a file and returns the number of bytes read
*/
{
    static int errorCounter=0;
    int numBytes;
#ifdef NO_ZLIB
    int fd;
    fd = open(filename,O_RDONLY);
    if(fd == 0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Error (%d of 100) opening file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    fprintf(stderr,"Error (%d of 100) opening file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    errorCounter++;
	}
	return -1;
    }
    numBytes=read(fd,buffer,maxBytes);
    if(numBytes<=0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Error (%d of 100) reading file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    fprintf(stderr,"Error (%d of 100) reading file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    errorCounter++;
	}
	close (fd);
	return -2;
    }
    close(fd);
    return numBytes;
#else
    gzFile infile = gzopen(filename,"rb");
    if(infile == NULL) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Error (%d of 100) opening file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    fprintf(stderr,"Error (%d of 100) opening file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    errorCounter++;
	}
	return -1;
    }       
    numBytes=gzread(infile,buffer,maxBytes);
    if(numBytes<=0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Error (%d of 100) reading file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    fprintf(stderr,"Error (%d of 100) reading file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    errorCounter++;
	}
	gzclose(infile);
	return -2;
    }
    gzclose(infile);
    return numBytes;
#endif
}


