#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <time.h>


#include "includes/anitaStructures.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "pedestalLib/pedestalLib.h"
#include "compressLib/compressLib.h"

char eventBuffer[100000];



int main(int argc, char **argv) {
    if(argc<2) {
	printf("Usage: %s <event telem file>\n",basename(argv[0]));
	return 0;
    }
//    AnitaEventBody_t theBody;
    PedSubbedEventBody_t theBody;
    int surf,chan,samp;
    char filename[FILENAME_MAX];
    sprintf(filename,"%s",argv[1]);

//    int numBytes=genericReadOfFile(eventBuffer,filename,100000);
    int numBytes=readSingleEncodedEvent(eventBuffer,filename);
    printf("Read %d bytes\n",numBytes);
//    CompressErrorCode_t retVal=unpackEvent(&theBody,eventBuffer,numBytes);
    CompressErrorCode_t retVal=unpackToPedSubbedEvent(&theBody,eventBuffer,numBytes);
    printf("Compress retVal %d\n",retVal);
    
    for(surf=0;surf<9;surf++) {
	for(chan=0;chan<9;chan++) {
	    int chanIndex=GetChanIndex(surf,chan);
	    for(samp=0;samp<260;samp++) {
		printf("%d %d %d -- %d\n",surf,chan,samp,theBody.channel[chanIndex].data[samp]);
	    }
	}
    }

    return 0;
}
