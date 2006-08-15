#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <time.h>


#include "anitaStructures.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "pedestalLib/pedestalLib.h"

void dumpLabPeds(FullLabChipPedStruct_t *pedPtr);

int main(int argc, char **argv) {
    if(argc<2) {
	printf("Usage: %s <lab ped file>\n",basename(argv[0]));
	return 0;
    }
    char filename[FILENAME_MAX];
    sprintf(filename,"%s",argv[1]);
    FullLabChipPedStruct_t labPed;
    fillLabChipPedstruct(&labPed,filename);
    dumpLabPeds(&labPed);
    return 0;
}

void dumpLabPeds(FullLabChipPedStruct_t *pedPtr) 
{
    int chan,samp;
    printf("Times %lu\t%lu\n",pedPtr->unixTimeStart,pedPtr->unixTimeEnd);
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	printf("chanId: %d\tchipId: %d\tchipEntries: %d\n",
	       pedPtr->pedChan[chan].chanId,
	       pedPtr->pedChan[chan].chipId,
	       pedPtr->pedChan[chan].chipEntries);
	
	for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
	    printf("Samp: %d\tMean: %d\tRMS: %d\n",
		   samp,
		   pedPtr->pedChan[chan].pedMean[samp],
		   pedPtr->pedChan[chan].pedRMS[samp]);
	}
    }
	
}
