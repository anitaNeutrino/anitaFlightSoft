#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>

#include <sys/types.h>

#include <time.h>


#include "includes/anitaStructures.h"


void printSlowRateStuff(SlowRateFull_t *slowPtr);

int main (int argc, char ** argv)
{ 
    unsigned char *bigBuffer;
    int numBytes=0,count=0;
    FILE *slowFile;

    //data stuff
           
    if(argc!=2) {
	printf("Usage:\t%s <slowFile>\n",basename(argv[0]));
	return 0;
    }
    
    bigBuffer = (unsigned char*) malloc(1500000);
    slowFile=fopen(argv[1],"rb");
    if(!slowFile) {
	printf("Couldn't open: %s\n",argv[1]);
	return 0;
    }
    numBytes=fread(bigBuffer,1,1500000,slowFile);
    printf("Read %d bytes from %s\n",numBytes,argv[1]);

    while(count<numBytes) {
	printf("%x\n",bigBuffer[count]);
	printf("%d of %d\n",count,numBytes);
	unsigned char comm1or2=bigBuffer[count];
	if(comm1or2==0xc1 || comm1or2==0xc2) {

	    unsigned char seqNum=bigBuffer[count+1];
	    unsigned char numBytes=bigBuffer[count+2];
	    unsigned char *sciData=&bigBuffer[count+3];
	    count+=numBytes+3;
	    
//	    printf("count %d -- comm %#x, seqNum %d, numBytes %d\n",
//		   count,comm1or2,seqNum,numBytes);
	    if(numBytes==sizeof(SlowRateFull_t))
		printSlowRateStuff((SlowRateFull_t *)sciData);
	}
	else {
	    count++;
	}
    }

        
    free(bigBuffer);
    return 0;
}

void printSlowRateStuff(SlowRateFull_t *slowPtr)
{
    printf("unixTime: %lu\n",slowPtr->unixTime);
    printf("\n**************** RF Stuff ******************\n");
    printf("eventNumber: %lu\n",slowPtr->rf.eventNumber);
    printf("eventRate: 1min %f -- 10min %f\n",((float)slowPtr->rf.eventRate1Min)/8.,((float)slowPtr->rf.eventRate10Min)/8.);
    int surf,chan,ant,phi;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	printf("RF Power Average SURF %d: ",surf);    
	for(chan=0;chan<RFCHAN_PER_SURF;chan++) {
	    printf("%d ",slowPtr->rf.rfPwrAvg[surf][chan]);
	}
	printf("\n");
    }
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	printf("Avg Scaler SURF %d: ",surf);    
	for(ant=0;ant<ANTS_PER_SURF;ant++) {
	    printf("%d ",slowPtr->rf.avgScalerRates[surf][ant]);
	}
	printf("\n");
    }
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	printf("RMS Scaler SURF %d: ",surf);    
	for(ant=0;ant<ANTS_PER_SURF;ant++) {
	    printf("%d ",slowPtr->rf.rmsScalerRates[surf][ant]);
	}
	printf("\n");
    }
    

    printf("\n**************** Housekeeping ******************\n");
    printf("latitude %f, longitude %f, altitude %f\n",
	   slowPtr->hk.latitude,slowPtr->hk.longitude,slowPtr->hk.altitude);
    int i;
    for(i=0;i<8;i++) {
	printf("temp %d -- %d\n",i,(int)slowPtr->hk.temps[i]);
    }
    for(i=0;i<4;i++) {
	printf("power %d -- %d\n",i,(int)slowPtr->hk.powers[i]);
    }

}
