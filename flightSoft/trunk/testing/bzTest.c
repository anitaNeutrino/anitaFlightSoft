#include <bzlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <time.h>
#include <math.h>


#include "includes/anitaStructures.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "pedestalLib/pedestalLib.h"
#include "compressLib/compressLib.h"


int main(int argc, char**argv) {
/*     if(argc<2) { */
/* 	printf("Usage: %s <event telem file>\n",basename(argv[0])); */
/* 	return 0; */
/*     } */

    EncodedPedSubbedSurfPacketHeader_t *surfHdPtr;
    char buffer[100000];
    char output[200000];
    char reverse[200000];
    char *runTag="run110";
    int startEvent=285285;
    int attens[9]={0,2,3,4,5,6,7,8,9};
    unsigned int inLength=100000,outLength=200000,reverseLength=200000;
    int count=0,surf;
    int retVal=0;
    char filename[FILENAME_MAX];
    int atIndex,rms,event;
    
    float means[9][11];
    float meanSq[9][11];
    float entries[9][11];

    for(atIndex=0;atIndex<9;atIndex++) {
	for(rms=0;rms<=50;rms+=5) {
	    int rmsIndex=rms/5;
	    means[atIndex][rmsIndex]=0;
	    meanSq[atIndex][rmsIndex]=0;
	    entries[atIndex][rmsIndex]=0;
	    printf("Att %d, RMS %d\n",attens[atIndex],rms);
	    for(event=startEvent;;event++) {
		sprintf(filename,"/raid3/rjn/compression/telem/att%d_rms%d/%s_none/event/pri2/ev_%d.dat",attens[atIndex],rms,runTag,event);
		if(event%100==0) fprintf(stderr,"*");
//		printf("Filename %s\n",filename);		
		int numBytes=readSingleEncodedEvent((unsigned char*)buffer,filename);
//		printf("Read %d bytes\n",numBytes);
		if(numBytes<0) break;;
		count =0;
		for(surf=0;surf<ACTIVE_SURFS;surf++) {
		    surfHdPtr = (EncodedPedSubbedSurfPacketHeader_t*) &buffer[count];
		    inLength=surfHdPtr->gHdr.numBytes;
		    outLength=200000;
		    reverseLength=200000;
		    retVal=BZ2_bzBuffToBuffCompress(output,&outLength,buffer,
					inLength,1,0,100);
    
//		    retVal=BZ2_bzBuffToBuffDecompress(reverse,&reverseLength,output,
//					  outLength,0,0);
//		    printf("In length %u, Out Length %u\t, Reverse Length %u\n",inLength,outLength,reverseLength);
		    float tempVal=(outLength+16)-((12*9)+24);
		    tempVal/=9;
		    means[atIndex][rmsIndex]+=tempVal;
		    meanSq[atIndex][rmsIndex]+=tempVal*tempVal;
		    entries[atIndex][rmsIndex]++;
		    count+=inLength;
		}		
	    }
	    fprintf(stderr,"\n");
	}
    }


    for(atIndex=0;atIndex<9;atIndex++) {
	for(rms=0;rms<=50;rms+=5) {
	    int rmsIndex=rms/5;
	    means[atIndex][rmsIndex]/=entries[atIndex][rmsIndex];
	    meanSq[atIndex][rmsIndex]/=entries[atIndex][rmsIndex];
	    float newrms=sqrt(meanSq[atIndex][rmsIndex]-
			   (means[atIndex][rmsIndex]*means[atIndex][rmsIndex]));
	    printf("%d %d %f %f\n",attens[atIndex],rms,means[atIndex][rmsIndex],
		   newrms);
	}
    }
	    
    return 1;
}
