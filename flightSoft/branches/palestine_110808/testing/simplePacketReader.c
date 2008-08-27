#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>

#include <sys/types.h>

#include <time.h>


#include "includes/anitaStructures.h"


#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"



int main(int argc, char** argv) {
    unsigned char bigBuffer[100000];
    int numBytes=0,count=0,checkVal;
    GenericHeader_t *gHdr;
    
    if(argc!=2) {
	printf("Usage:\t%s <packetFile>\n",basename(argv[0]));
	return 0;
    }

    numBytes=genericReadOfFile(bigBuffer,argv[1],100000);
    printf("Read %d bytes from %s\n",numBytes,argv[1]);
//    for(count =0; count<numBytes;count++) {
//	printf("count %d -- %x\n",count,(bigBuffer[count]&0xff));
//    }
    checkVal=checkPacket(bigBuffer);
    if(checkVal==0) {
	gHdr = (GenericHeader_t*) &bigBuffer[0];	
	printf("Got %s\n",packetCodeAsString(gHdr->code));
	count+=gHdr->numBytes;
    }
    else {
	printf("Problem with packet -- checkVal==%d\n",checkVal);
	return 1;
//	    count+=gHdr->numBytes;
    }
    
    return 0;
}


