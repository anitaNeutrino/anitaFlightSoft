#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>

#include <sys/types.h>

#include <time.h>


#include "anitaStructures.h"


#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"

void handleScience(unsigned char *buffer,unsigned short numBytes);
void printSurfInfo(EncodedSurfPacketHeader_t *surfPtr);

int main (int argc, char ** argv)
{ 
    unsigned short *bigBuffer;
    int numBytes=0,count=0;
    FILE *losFile;

    //data stuff
    unsigned short numWords;
    unsigned short unused;
    unsigned short startHdr;
    unsigned short auxHdr;
    unsigned short idHdr;
    int losOrSip;
    int oddOrEven;
    unsigned long bufferCount;
    unsigned long *ulPtr;
    unsigned short numSciBytes;
    unsigned short checksum;
    unsigned short endHdr;
    unsigned short swEndHdr;
    unsigned short auxHdr2;
           
    if(argc!=2) {
	printf("Usage:\t%s <losFile>\n",basename(argv[0]));
	return 0;
    }
    
    bigBuffer = (unsigned short*) malloc(1500000);
    losFile=fopen(argv[1],"rb");
    if(!losFile) {
	printf("Couldn't open: %s\n",argv[1]);
	return 0;
    }
    numBytes=fread(bigBuffer,1,1500000,losFile);
    printf("Read %d bytes from %s\n",numBytes,argv[1]);

    while(count<numBytes) {
//	printf("%x\n",bigBuffer[count]);
	if(bigBuffer[count]==0xf00d) {
//	    printf("Got f00d\n");
	    //Maybe new los buffer
	    numWords=bigBuffer[count+1];
	    unused=bigBuffer[count+2];
	    startHdr=bigBuffer[count+3];
	    auxHdr=bigBuffer[count+4];
	    idHdr=bigBuffer[count+5];
//	    printf("startHdr -- %x\n",startHdr);
//	    printf("auxHdr -- %x\n",auxHdr);
//	    printf("idHdr -- %x\n",idHdr);
	    if(startHdr==0xf00d && auxHdr==0xd0cc && (idHdr&0xfff0)==0xae00) {
		//Got a los buffer
		losOrSip=idHdr&0x1;
		oddOrEven=idHdr&0x2>>1;
		ulPtr = (unsigned long*) &bigBuffer[count+6];
		bufferCount=*ulPtr;
		numSciBytes=bigBuffer[count+8];
//		printf("Buffer %lu -- %d bytes\n",bufferCount,numSciBytes);
		handleScience((unsigned char*)&bigBuffer[count+9],numSciBytes);
		checksum=bigBuffer[count+9+(numSciBytes/2)];		
		swEndHdr=bigBuffer[count+10+(numSciBytes/2)];
		endHdr=bigBuffer[count+11+(numSciBytes/2)];
		auxHdr2=bigBuffer[count+12+(numSciBytes/2)];
//		printf("swEndHdr -- %x\n",swEndHdr);
//		printf("endHdr -- %x\n",endHdr);
//		printf("auxHdr2 -- %x\n",auxHdr2);		
//		return 0;
		count+=12+(numSciBytes/2);
		continue;
	    }
	}
	count++;
    }

        
    free(bigBuffer);

}

void handleScience(unsigned char *buffer,unsigned short numBytes) {
    unsigned short count=0;
    GenericHeader_t *gHdr;
    int checkVal;
    
    while(count<numBytes) {
	if(buffer[count]==0xfe && buffer[count+1]==0xfe && buffer[numBytes-1]==0xfe) {	    
	    printf("Got 0xfe Wake Up Buffer\n");
	    return;
	}

	checkVal=checkPacket(&buffer[count]);
	if(checkVal==0) {
	    gHdr = (GenericHeader_t*) &buffer[count];	
	    printf("Got %s\n",packetCodeAsString(gHdr->code));
	    if(gHdr->code==PACKET_ENC_SURF) 
		printSurfInfo((EncodedSurfPacketHeader_t*) gHdr);
	    count+=gHdr->numBytes;
	}
	else {
	    printf("Problem with packet -- checkVal==%d\n",checkVal);
	    return;
//	    count+=gHdr->numBytes;
	}
    }
       
}

void printSurfInfo(EncodedSurfPacketHeader_t *surfPtr) 
{
    EncodedSurfChannelHeader_t *chan0= surfPtr + sizeof(EncodedSurfPacketHeader_t);
    printf("Event %d  Chan 0: numBytes %d crc %d\n",surfPtr->eventNumber,chan0->numBytes,chan0->crc);

}
