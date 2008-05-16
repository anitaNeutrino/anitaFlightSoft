#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>

#include <sys/types.h>

#include <time.h>


#include "includes/anitaStructures.h"

#include "compressLib/compressLib.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"

void handleScience(unsigned char *buffer,unsigned short numBytes);
void printSurfInfo(EncodedSurfPacketHeader_t *surfPtr);
void printPedSubSurfInfo(EncodedPedSubbedSurfPacketHeader_t *surfPtr);

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
//	printf("%d of %d\n",count,numBytes);
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
		printf("Buffer %lu -- %d bytes\n",bufferCount,numSciBytes);
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
    return 0;
}

void handleScience(unsigned char *buffer,unsigned short numBytes) {
    unsigned short count=0;
    GenericHeader_t *gHdr;
    //    GenericHeader_t *gHdr2;
    int checkVal;
    //    char packetBuffer[10000];


    while(count<numBytes-1) {
	if(buffer[count]==0xfe && buffer[count+1]==0xfe && buffer[numBytes-1]==0xfe) {	    
	    printf("Got 0xfe Wake Up Buffer\n");
	    return;
	}
	printf("count %d\n",count);
	checkVal=checkPacket(&buffer[count]);
	gHdr = (GenericHeader_t*) &buffer[count];	

	if(checkVal==0) {
	    printf("Got %s (%#x) -- (%d bytes)\n",packetCodeAsString(gHdr->code),
		   gHdr->code,gHdr->numBytes);
/* 	    if(gHdr->code==PACKET_ENC_SURF)  */
/* 		printSurfInfo((EncodedSurfPacketHeader_t*) gHdr); */
/* 	    if(gHdr->code==PACKET_ENC_SURF_PEDSUB)  */
/* 		printPedSubSurfInfo((EncodedPedSubbedSurfPacketHeader_t*) gHdr);	     */
/* 	    if(gHdr->code==PACKET_ZIPPED_PACKET) { */
		
/* 		int retVal=unzipZippedPacket((ZippedPacket_t*)&buffer[count], */
/* 					     packetBuffer,10000); */
/* 		checkVal=checkPacket(packetBuffer); */
/* 		if(retVal==0 && checkVal==0) { */
/* 		    gHdr2=(GenericHeader_t*) packetBuffer; */
/* 		    printf("\tGot %s (%#x) -- (%d bytes)\n", */
/* 			   packetCodeAsString(gHdr2->code), */
/* 			   gHdr2->code,gHdr2->numBytes); */
/* 		} */
/* 		else { */
/* 		    printf("\tunzipZippedPacket retVal %d -- checkPacket == %d\n",retVal,checkVal); */
/* 		} */
/* 	    } */

	    count+=gHdr->numBytes;
	}
	else {
	    printf("Problem with packet -- checkVal==%d  (code? %#x)\n",checkVal,gHdr->code);
	    return;
	    count+=gHdr->numBytes;
	}
    }
       
}

void printSurfInfo(EncodedSurfPacketHeader_t *surfPtr) 
{
    EncodedSurfChannelHeader_t *chan0= (EncodedSurfChannelHeader_t*)(surfPtr + sizeof(EncodedSurfPacketHeader_t));
    printf("Event %u  Chan 0: numBytes %d crc %d\n",surfPtr->eventNumber,chan0->numBytes,chan0->crc);

}



void printPedSubSurfInfo(EncodedPedSubbedSurfPacketHeader_t *surfPtr) 
{
    SurfChannelPedSubbed_t psSurfChan;
    unsigned char *eventBuffer;
    int count=0,chan,i;
    EncodedSurfChannelHeader_t *chanHdPtr;
    printf("event %u, numBytes %d\n",surfPtr->eventNumber,surfPtr->gHdr.numBytes);
    eventBuffer = (unsigned char*) surfPtr;
    count=sizeof(EncodedPedSubbedSurfPacketHeader_t);
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	
	chanHdPtr = (EncodedSurfChannelHeader_t*)&eventBuffer[count];
	    
	printf("Event %u  Chan %d: encType %d, numBytes %d crc %d\n",surfPtr->eventNumber,chan,chanHdPtr->encType,chanHdPtr->numBytes,chanHdPtr->crc);
	count+=sizeof(EncodedSurfChannelHeader_t);
	CompressErrorCode_t retVal=decodePSChannel(chanHdPtr,&eventBuffer[count], &psSurfChan);
	if(retVal==COMPRESS_E_OK) {
	    if(chan<8) {
		if(psSurfChan.mean<-50 || psSurfChan.mean>50) {
		    for(i=0;i<100;i++) {
			printf("**********************************\n");
		    }
		}
	    }
	    printf("Chan mean %f, rms %f, xMin %d, xMax %d\n",
		   psSurfChan.mean,psSurfChan.rms,psSurfChan.xMin,
		   psSurfChan.xMax);
	}
	else {
	    printf("Problem decoding PS channel\n");
	}
	count+=chanHdPtr->numBytes;

    }

}
