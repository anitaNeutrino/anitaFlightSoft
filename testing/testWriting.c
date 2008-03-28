#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>


#include "includes/anitaStructures.h"

void fillEventGarbage(PedSubbedEventBody_t *psevPtr);

int main(int argc, char**argv) {
    int numEvents=1000;
    char outputDir[FILENAME_MAX];
    if(argc<3) { 
 	printf("Usage: %s <output dir> <num events\n",basename(argv[0])); 
	return 0; 
     } 
    printf("%s %s %s\n",argv[0],argv[1],argv[2]);
    sprintf(outputDir,"%s",argv[1]);
    numEvents=atoi(argv[2]);

    
    PedSubbedEventBody_t psBody;
    int event=0;
    FILE *fpEvent=0;
    char fileName[FILENAME_MAX];
    for(event=0;event<numEvents;event++) {
	if(event%EVENTS_PER_FILE==0) {
	    if(fpEvent) {
		fclose(fpEvent);
//		zipFileInPlace(fileName);
		fpEvent=0;
	    }
	}
	if(!fpEvent) {
	    //Need to make new file
	    sprintf(fileName,"%s/test_%d.dat",outputDir,event);
//	    printf("Trying to open: %s\n",fileName);
	    fpEvent=fopen(fileName,"wb");
//	    printf("%d\n",fpEvent);
	}
	fillEventGarbage(&psBody);
	fwrite(&psBody,sizeof(PedSubbedEventBody_t),1,fpEvent);
    }
    if(fpEvent)
	fclose(fpEvent);

}


void fillEventGarbage(PedSubbedEventBody_t *psevPtr) {
   int chan,samp,value;
   
/* Channel Stuff */
   for(chan=0;chan<NUM_DIGITZED_CHANNELS;chan++) {
       for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
	   value=-2094+(4096.0*rand())/RAND_MAX;
//	   printf("%d\n",value);
	   psevPtr->channel[chan].data[samp]=value;
       }
   }
   
}

