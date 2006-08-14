#include <bzlib.h>
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
#include "compressLib/compressLib.h"


int main(int argc, char**argv) {
    if(argc<2) {
	printf("Usage: %s <event telem file>\n",basename(argv[0]));
	return 0;
    }
    char buffer[100000];
    char output[200000];
    unsigned int inLength=100000,outLength=200000;
    int count=0;
    int retVal=0;
    char filename[FILENAME_MAX];
    sprintf(filename,"%s",argv[1]);

    int numBytes=readSingleEncodedEvent((unsigned char*)buffer,filename);
    printf("Read %d bytes\n",numBytes);
    if(numBytes<0) return 0;


    retVal=BZ2_bzBuffToBuffCompress(output,&outLength,buffer,inLength,1,0,30);

    printf("In length %u, Out Length %u\n",inLength,outLength);

    return 1;
}
