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
#include "linkWatchLib/linkWatchLib.h"

#define MAX_FILE_SIZE 10000
char unzippedBuffer[MAX_FILE_SIZE];

void processZippedFile(ZippedFile_t *zfPtr);

int main(int argc, char** argv) {
    unsigned char bigBuffer[100000];
    int numBytes=0,count=0,checkVal;
     char currentLink[FILENAME_MAX];
    char currentFile[FILENAME_MAX];
    struct dirent **linkList;
    int numLinks=getListofLinks(REQUEST_TELEM_LINK_DIR,&linkList);
    
    GenericHeader_t *gHdr;    
    printf("numLinks %d\n",numLinks);


    for(count=0;count<numLinks;count++) {
      sprintf(currentLink,"%s/%s",REQUEST_TELEM_LINK_DIR,linkList[count]->d_name);
      sprintf(currentFile,"%s/%s",REQUEST_TELEM_DIR,linkList[count]->d_name);
    
      
      numBytes=genericReadOfFile(bigBuffer,currentFile,100000);
      //      printf("Read %d bytes from %s\n",numBytes,currentFile);
      //    for(count =0; count<numBytes;count++) {
      //	printf("count %d -- %x\n",count,(bigBuffer[count]&0xff));
      //    }
      checkVal=checkPacket(bigBuffer);
      if(checkVal==0) {
	gHdr = (GenericHeader_t*) &bigBuffer[0];	
	//	printf("Got %s\n",packetCodeAsString(gHdr->code));
	if(gHdr->code==PACKET_ZIPPED_FILE)
	  processZippedFile((ZippedFile_t*)gHdr);
      }
      else {
	printf("Problem with packet -- checkVal==%d\n",checkVal);
	return 1;
	//	    count+=gHdr->numBytes;
      }
    }
    return 0;
}

void processZippedFile(ZippedFile_t *zfPtr)
{
  static int lastTime=0;
  static char lastFilename[FILENAME_MAX];
  static FILE *fpOut=0;

  char outputFilename[FILENAME_MAX];
  char *zippedBuf=(char*) zfPtr;
  unsigned int numInputBytes=zfPtr->gHdr.numBytes-sizeof(ZippedFile_t);
  unsigned int numOutputBytes=MAX_FILE_SIZE;
  int retVal=unzipBuffer(&zippedBuf[sizeof(ZippedFile_t)],unzippedBuffer,numInputBytes,&numOutputBytes);
  if(retVal<0) {
    fprintf(stderr,"Error processing zipped file\n");
    return;
  }
  if(numOutputBytes!=zfPtr->numUncompressedBytes) {
    fprintf(stderr,"Expected %d bytes but only got %d bytes\n",
	    zfPtr->numUncompressedBytes,numOutputBytes);
  }
  //  fprintf(stderr,"Got %s %u\n",zfPtr->filename,zfPtr->unixTime);
  if(lastTime!=zfPtr->unixTime || strcmp(lastFilename,zfPtr->filename)) {
    sprintf(outputFilename,"/tmp/%s_%u",zfPtr->filename,zfPtr->unixTime);
    printf("New file %s\n",outputFilename);
    strcpy(lastFilename,zfPtr->filename);
    lastTime=zfPtr->unixTime;

    if(fpOut) fclose(fpOut);
    fpOut = fopen(outputFilename,"w");
    if(fpOut<=0) {
      fprintf(stderr,"Couldn't open %s\n",outputFilename);
      return;
    }    
  }
  fwrite(unzippedBuffer,numOutputBytes,1,fpOut);
}
