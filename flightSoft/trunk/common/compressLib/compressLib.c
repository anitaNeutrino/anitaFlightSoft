/*! \file compressLib.c
    \brief Compression library, full of (hopefully) useful stuff
    
    Some functions that compress and uncompress ANITA data
    June 2006  rjn@mps.ohio-state.edu
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "compressLib/compressLib.h"
#include "pedestalLib/pedestalLib.h"
#include "compressLib/pack12bit.h"
#include "utilLib/utilLib.h"
#include "compressLib/mulaw.h"	  

CompressErrorCode_t packEvent(AnitaEventBody_t *bdPtr,
			      EncodeControlStruct_t *cntlPtr,
			      unsigned char *output,
			      int *numBytes) 
{
    int count=0,surfStart=0;
    int surf=0;
    int chan=0;  
    EncodedSurfPacketHeader_t *surfHdPtr;
//    printf("%lu\n",bdPtr->eventNumber);

    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedSurfPacketHeader_t*) &output[count];
	surfStart=count;
	surfHdPtr->eventNumber=bdPtr->eventNumber;
//	printf("Doing event %lu %lu, surf (%d)\n",surfHdPtr->eventNumber,bdPtr->eventNumber,surf);
	count+=sizeof(EncodedSurfPacketHeader_t);
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    count+=encodeChannel(cntlPtr->encTypes[surf][chan],
				 &(bdPtr->channel[GetChanIndex(surf,chan)]),
				 &output[count]);
	    if(count>MAX_WAVE_BUFFER)
		return COMPRESS_E_TOO_BIG;
	}
	//Fill Generic Header_t;
	fillGenericHeader(surfHdPtr,PACKET_ENC_SURF,(count-surfStart));
							 
    }
    *numBytes=count;
    return COMPRESS_E_OK;
}


CompressErrorCode_t packPedSubbedEvent(PedSubbedEventBody_t *bdPtr,
				       EncodeControlStruct_t *cntlPtr,
				       unsigned char *output,
				       int *numBytes)
{

    int count=0,surfStart=0;
    int surf=0;
    int chan=0;  
    EncodedPedSubbedSurfPacketHeader_t *surfHdPtr;
//    printf("Event %lu\n",bdPtr->eventNumber);
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedPedSubbedSurfPacketHeader_t*) &output[count];
	surfStart=count;
	surfHdPtr->whichPeds=bdPtr->whichPeds;
	surfHdPtr->eventNumber=bdPtr->eventNumber;
	count+=sizeof(EncodedPedSubbedSurfPacketHeader_t);
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    count+=encodePSChannel(cntlPtr->encTypes[surf][chan],
				   &(bdPtr->channel[GetChanIndex(surf,chan)]),
				   &output[count]);
	    if(count>MAX_WAVE_BUFFER)
		return COMPRESS_E_TOO_BIG;
	}
	//Fill Generic Header_t;
	fillGenericHeader(surfHdPtr,PACKET_ENC_SURF_PEDSUB,(count-surfStart));
							 
    }
    *numBytes=count;
    return COMPRESS_E_OK;
}



CompressErrorCode_t unpackEvent(AnitaEventBody_t *bdPtr,
				unsigned char *input,
				int numBytes)
{
    PedSubbedEventBody_t pedSubBd;
    CompressErrorCode_t retVal=
	unpackToPedSubbedEvent(&pedSubBd,input,numBytes);
    if(retVal!=COMPRESS_E_OK) return retVal;
    addPeds(&pedSubBd,bdPtr);
    return retVal;

}


CompressErrorCode_t unpackToPedSubbedEvent(PedSubbedEventBody_t *bdPtr,
					   unsigned char *input,
					   int numBytes)
{

    return unpackToPedSubbedEventWithStats(bdPtr,input,numBytes,0,0);
    
}


CompressErrorCode_t unpackOneSurfToPedSubbedEvent(PedSubbedEventBody_t *bdPtr,
						  unsigned char *input,
						  int numBytes)
{
    int count=0;
    int chan=0;  
    int chanIndex;
    CompressErrorCode_t retVal=0;
    EncodedPedSubbedSurfPacketHeader_t *surfHdPtr;
    EncodedSurfChannelHeader_t *chanHdPtr;
    
    
    surfHdPtr = (EncodedPedSubbedSurfPacketHeader_t*) &input[count];
    bdPtr->whichPeds=surfHdPtr->whichPeds;
    bdPtr->eventNumber=surfHdPtr->eventNumber;
    count+=sizeof(EncodedPedSubbedSurfPacketHeader_t);
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	
	chanHdPtr = (EncodedSurfChannelHeader_t*)&input[count];
	count+=sizeof(EncodedSurfChannelHeader_t);
//	    printf("surf %d, chan %d, encType %d\n",
//		   surf,chan,chanHdPtr->encType);
	chanIndex=chanHdPtr->rawHdr.chanId;
	if(chanIndex%ACTIVE_SURFS != chan) {
	    printf("Error reading Encoded SURF packet, have chanId %d and chan %d\n",chanIndex,chan);
	    return COMPRESS_E_UNPACK;
	}
	    	
	retVal=decodePSChannel(chanHdPtr,
			       &input[count],
			       &(bdPtr->channel[chanIndex]));
	if(retVal!=COMPRESS_E_OK) return retVal;
	count+=chanHdPtr->numBytes;
	if(count>numBytes)
	    return COMPRESS_E_BADSIZE;
    }
	//Fill Generic Header_t;
	
//    fillGenericHeader(bdPtr,PACKET_PED_SUBBED_EVENT,count);
    return COMPRESS_E_OK;

}


CompressErrorCode_t unpackToPedSubbedEventWithStats(PedSubbedEventBody_t *bdPtr,
					   unsigned char *input,
					   int numBytes,
					   EncodeControlStruct_t *cntlPtr,
					   int *sizeArray)
{

    int count=0;
    int surf=0;
    int chan=0;  
    CompressErrorCode_t retVal=0;
    EncodedPedSubbedSurfPacketHeader_t *surfHdPtr;
    EncodedSurfChannelHeader_t *chanHdPtr;

//    printf("unpackToPedSubbedEvent\n");
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedPedSubbedSurfPacketHeader_t*) &input[count];
	bdPtr->whichPeds=surfHdPtr->whichPeds;
	bdPtr->eventNumber=surfHdPtr->eventNumber;
	count+=sizeof(EncodedPedSubbedSurfPacketHeader_t);
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    
	    chanHdPtr = (EncodedSurfChannelHeader_t*)&input[count];
	    count+=sizeof(EncodedSurfChannelHeader_t);
//	    printf("surf %d, chan %d, encType %d\n",
//		   surf,chan,chanHdPtr->encType);


	    retVal=decodePSChannel(chanHdPtr,
				   &input[count],
				   &(bdPtr->channel[GetChanIndex(surf,chan)]));
	    if(retVal!=COMPRESS_E_OK) return retVal;
	    count+=chanHdPtr->numBytes;	
	    if(cntlPtr) {
		cntlPtr->encTypes[surf][chan]=chanHdPtr->encType;;
	    }
	    if(sizeArray) {
		sizeArray[GetChanIndex(surf,chan)]=chanHdPtr->numBytes;
	    }
	    if(count>numBytes)
		return COMPRESS_E_BADSIZE;
	}
	//Fill Generic Header_t;
    }
//    fillGenericHeader(bdPtr,PACKET_PED_SUBBED_EVENT,count);
    return COMPRESS_E_OK;

}


void fillMinMaxMeanRMS(SurfChannelPedSubbed_t *chanPtr) {
    int samp;
    int wrappedHitbus=chanPtr->header.chipIdFlag&WRAPPED_HITBUS;
    int firstSamp=0,lastSamp=MAX_NUMBER_SAMPLES;
    int index=0;
    float mean=0,meanSq=0,nsamps=0;
    chanPtr->xMax=-4095;
    chanPtr->xMin=4095;
	
    if(wrappedHitbus) {
	firstSamp=chanPtr->header.firstHitbus+1;
	lastSamp=chanPtr->header.lastHitbus;
    }
    else {
	firstSamp=chanPtr->header.lastHitbus+1;
	lastSamp=MAX_NUMBER_SAMPLES+chanPtr->header.firstHitbus;
    }    
    for(samp=firstSamp;samp<lastSamp;samp++) {
	index=samp;
	if(index>=MAX_NUMBER_SAMPLES) index-=MAX_NUMBER_SAMPLES;
	mean+=chanPtr->data[index];
	meanSq+=(chanPtr->data[index]*chanPtr->data[index]);
	nsamps++;

	if(chanPtr->data[index]>chanPtr->xMax)
	    chanPtr->xMax=chanPtr->data[index];
	if(chanPtr->data[index]<chanPtr->xMin)
	    chanPtr->xMin=chanPtr->data[index];

    }
    if(nsamps>0) {
	mean/=nsamps;
	meanSq/=nsamps;
	chanPtr->mean=mean;
	if(meanSq>mean*mean)
	    chanPtr->rms=sqrt(meanSq-mean*mean);
    }
    

}


int encodeChannel(ChannelEncodingType_t encType, SurfChannelFull_t *chanPtr, unsigned char *buffer) 
{
//    printf("Event %lu, ChanId %d\n",theHead.eventNumber,
//	   chanPtr->header.chanId);



    EncodedSurfChannelHeader_t *chanHdPtr
	=(EncodedSurfChannelHeader_t*) &buffer[0];
    int count=0;    
    int encSize=0;
    chanHdPtr->rawHdr=chanPtr->header;
    chanHdPtr->encType=encType;
    count+=sizeof(EncodedSurfChannelHeader_t);
    
    switch(encType) {
	case ENCODE_NONE:
	    encSize=encodeWaveNone(&buffer[count],chanPtr);
	    break;
	default:
	    chanHdPtr->encType=ENCODE_NONE;
	    encSize=encodeWaveNone(&buffer[count],chanPtr);
	    break;	    
    }    
    chanHdPtr->numBytes=encSize;
    chanHdPtr->crc=simpleCrcShort((unsigned short*)&buffer[count],
				  encSize/2);
    return (count+encSize);        
}

int encodeWaveNone(unsigned char *buffer,SurfChannelFull_t *chanPtr) {    
    int encSize=MAX_NUMBER_SAMPLES*sizeof(unsigned short);
    memcpy(buffer,chanPtr->data,encSize);
    return encSize;
}

CompressErrorCode_t decodeWaveNone(unsigned char *input,int numBytes,SurfChannelFull_t *chanPtr){    
    int encSize=MAX_NUMBER_SAMPLES*sizeof(unsigned short);
    //Check that numBytes is correct
    if(numBytes!=encSize) return COMPRESS_E_BADSIZE;
    memcpy(chanPtr->data,input,encSize);
    return COMPRESS_E_OK;
}


int encodePSChannel(ChannelEncodingType_t encType, SurfChannelPedSubbed_t *chanPtr, unsigned char *buffer) 
{
//    printf("Event %lu, ChanId %d\n",theHead.eventNumber,
//	   chanPtr->header.chanId);

    EncodedSurfChannelHeader_t *chanHdPtr
	=(EncodedSurfChannelHeader_t*) &buffer[0];
    int count=0;    
    int encSize=0;
    chanHdPtr->rawHdr=chanPtr->header;
    chanHdPtr->encType=encType;
    count+=sizeof(EncodedSurfChannelHeader_t);
    
    switch(encType) {
	case ENCODE_NONE:
	    encSize=encodePSWaveNone(&buffer[count],chanPtr);
	    break;
	case ENCODE_LOSSLESS_12BIT:
	    encSize=encodePSWave12bitBinary(&buffer[count],chanPtr);
	    break;
	case ENCODE_LOSSLESS_BINARY:
	    encSize=encodePSWaveLosslessBinary(&buffer[count],chanPtr,
					       &(chanHdPtr->encType));
	    break;
	case ENCODE_LOSSLESS_FIBONACCI:
	    encSize=encodePSWaveLosslessFibonacci(&buffer[count],chanPtr);
	    break;
	case ENCODE_LOSSLESS_BINFIB_COMBO:
	    encSize=encodePSWaveLosslessBinFibCombo(&buffer[count],chanPtr,
						    &(chanHdPtr->encType));
	    break;
	case ENCODE_LOSSY_MULAW:
	case ENCODE_LOSSY_MULAW_8BIT:
	    encSize=encodePSWaveLossyMulawOptimally(&buffer[count],chanPtr,8,
						    &(chanHdPtr->encType));
	    break;
	case ENCODE_LOSSY_MULAW_7BIT:
	    encSize=encodePSWaveLossyMulawOptimally(&buffer[count],chanPtr,7,
						    &(chanHdPtr->encType));
	    break;
	case ENCODE_LOSSY_MULAW_6BIT:
	    encSize=encodePSWaveLossyMulawOptimally(&buffer[count],chanPtr,6,
						    &(chanHdPtr->encType));
	    break;
	case ENCODE_LOSSY_MULAW_5BIT:
	    encSize=encodePSWaveLossyMulawOptimally(&buffer[count],chanPtr,5,
						    &(chanHdPtr->encType));
	    break;
	case ENCODE_LOSSY_MULAW_4BIT:
	    encSize=encodePSWaveLossyMulawOptimally(&buffer[count],chanPtr,4,
						    &(chanHdPtr->encType));
	    break;
	case ENCODE_LOSSY_MULAW_3BIT:
	    encSize=encodePSWaveLossyMulawOptimally(&buffer[count],chanPtr,3,
						    &(chanHdPtr->encType));
	    break;
	default:
	    chanHdPtr->encType=ENCODE_NONE;
	    encSize=encodePSWaveNone(&buffer[count],chanPtr);
	    break;	    
    }    
    chanHdPtr->numBytes=encSize;
    chanHdPtr->crc=simpleCrcShort((unsigned short*)&buffer[count],
				  encSize/2);
    return (count+encSize);        
}

CompressErrorCode_t decodePSChannel(EncodedSurfChannelHeader_t *encChanHdPtr,unsigned char *input, SurfChannelPedSubbed_t *chanPtr)
{
    CompressErrorCode_t retVal;
    int numBytes=encChanHdPtr->numBytes;
    int newCrc=simpleCrcShort((unsigned short*)input,numBytes/2);
    if(encChanHdPtr->crc!=newCrc) return COMPRESS_E_BAD_CRC;
    

//    printf("decodePSChannel encType==%d\n",encChanHdPtr->encType);
    chanPtr->header=encChanHdPtr->rawHdr;
    switch(encChanHdPtr->encType) {
	case ENCODE_NONE:
	    retVal=decodePSWaveNone(input,numBytes,chanPtr);
	    break;
	case ENCODE_LOSSLESS_12BIT:
	    retVal=decodePSWave12bitBinary(input,numBytes,chanPtr);
	    break;
	case ENCODE_LOSSLESS_11BIT:
	case ENCODE_LOSSLESS_10BIT:
	case ENCODE_LOSSLESS_9BIT:
	case ENCODE_LOSSLESS_8BIT:
	case ENCODE_LOSSLESS_7BIT:
	case ENCODE_LOSSLESS_6BIT:
	case ENCODE_LOSSLESS_5BIT:
	case ENCODE_LOSSLESS_4BIT:
	case ENCODE_LOSSLESS_3BIT:
	case ENCODE_LOSSLESS_2BIT:
	case ENCODE_LOSSLESS_1BIT:
	    retVal=decodePSWaveLosslessBinary(input,numBytes,chanPtr,encChanHdPtr->encType);
	    break;
	case ENCODE_LOSSLESS_FIBONACCI:
	    retVal=decodePSWaveLosslessFibonacci(input,numBytes,chanPtr);
	    break;
	case ENCODE_LOSSLESS_BINFIB_COMBO:
	case ENCODE_LOSSLESS_BINFIB_10BIT:
	case ENCODE_LOSSLESS_BINFIB_9BIT:
	case ENCODE_LOSSLESS_BINFIB_8BIT:
	case ENCODE_LOSSLESS_BINFIB_7BIT:
	case ENCODE_LOSSLESS_BINFIB_6BIT:
	case ENCODE_LOSSLESS_BINFIB_5BIT:
	case ENCODE_LOSSLESS_BINFIB_4BIT:
	case ENCODE_LOSSLESS_BINFIB_3BIT:
	case ENCODE_LOSSLESS_BINFIB_2BIT:
	case ENCODE_LOSSLESS_BINFIB_1BIT:
	    retVal=decodePSWaveLosslessBinFibCombo(input,numBytes,chanPtr,
						   encChanHdPtr->encType);
	    break;
	case ENCODE_LOSSY_MULAW_11_8:
	case ENCODE_LOSSY_MULAW_11_7:
	case ENCODE_LOSSY_MULAW_11_6:
	case ENCODE_LOSSY_MULAW_11_5:
	case ENCODE_LOSSY_MULAW_11_4:
	case ENCODE_LOSSY_MULAW_11_3:
	case ENCODE_LOSSY_MULAW_10_8:
	case ENCODE_LOSSY_MULAW_10_7:
	case ENCODE_LOSSY_MULAW_10_6:
	case ENCODE_LOSSY_MULAW_10_5:
	case ENCODE_LOSSY_MULAW_10_4:
	case ENCODE_LOSSY_MULAW_10_3:
	case ENCODE_LOSSY_MULAW_9_7:
	case ENCODE_LOSSY_MULAW_9_6:
	case ENCODE_LOSSY_MULAW_9_5:
	case ENCODE_LOSSY_MULAW_9_4:
	case ENCODE_LOSSY_MULAW_9_3:
	case ENCODE_LOSSY_MULAW_8_6:
	case ENCODE_LOSSY_MULAW_8_5:
	case ENCODE_LOSSY_MULAW_8_4:
	case ENCODE_LOSSY_MULAW_8_3:
	case ENCODE_LOSSY_MULAW_7_5:
	case ENCODE_LOSSY_MULAW_7_4:
	case ENCODE_LOSSY_MULAW_7_3:
	case ENCODE_LOSSY_MULAW_6_4:
	case ENCODE_LOSSY_MULAW_6_3:
	    retVal=decodePSWaveLossyMuLaw(input,numBytes,chanPtr,
					  encChanHdPtr->encType);
	    break;

	default:
	    //Try ENCODE_NONE
	    retVal=decodePSWaveNone(input,numBytes,chanPtr);
	    break;
    }
    if(retVal==COMPRESS_E_OK) fillMinMaxMeanRMS(chanPtr);
    return retVal;
}


int encodePSWaveNone(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr) {    
//    int samp;
    int encSize=MAX_NUMBER_SAMPLES*sizeof(short);
    memcpy(buffer,chanPtr->data,encSize);
//    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp+=4) {
//	printf("samp %d: value %d\n",samp,chanPtr->data[samp]);
//    }
    return encSize;
}

CompressErrorCode_t decodePSWaveNone(unsigned char *input,int numBytes,SurfChannelPedSubbed_t *chanPtr){    
//    int samp;
    int encSize=MAX_NUMBER_SAMPLES*sizeof(unsigned short);
    //Check that numBytes is correct
    if(numBytes!=encSize) return COMPRESS_E_BADSIZE;
//    printf("Here\n");
    memcpy(chanPtr->data,input,encSize);
//    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
//	printf("samp %d: value %d\n",samp,chanPtr->data[samp]);
//    }
    return COMPRESS_E_OK;
}

#define OFFSET_VALUE 2048

int encodePSWave12bitBinary(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr) {    
    int encSize=3*(MAX_NUMBER_SAMPLES/4)*sizeof(unsigned short);
    int samp=0,charNum=0,valNum=0;
    unsigned short value[4];
    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp+=4) {
	charNum=(samp*3)/2;
	for(valNum=0;valNum<4;valNum++)
	    value[valNum]=chanPtr->data[samp+valNum]+OFFSET_VALUE;
    
	pack12bit(value,&buffer[charNum]);
    }
    return encSize;
}

CompressErrorCode_t decodePSWave12bitBinary(unsigned char *input,int numBytes,SurfChannelPedSubbed_t *chanPtr){    
    int encSize=3*(MAX_NUMBER_SAMPLES/4)*sizeof(unsigned short);
    if(numBytes!=encSize) return COMPRESS_E_BADSIZE;
    unsigned short value[4];
    int samp=0,charNum=0,valNum=0;
    for(charNum=0;charNum<6*(MAX_NUMBER_SAMPLES/4);charNum+=6) {
	samp=2*(charNum/3);	
	unpack12bit(value,&input[charNum]);
	for(valNum=0;valNum<4;valNum++)
	    chanPtr->data[samp+valNum]=((short)value[valNum])-OFFSET_VALUE;
    }
    return COMPRESS_E_OK;
}

int encodePSWaveLosslessBinary(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr, ChannelEncodingType_t *encTypePtr) {    
    //Remember this function works an array of pedestal subtracted shorts
    //Which is what SurfChannelPedSubbed_t contains
    int wordNum=0;
    int numBitsLeft;
    int bitMask;
    int bitNum;
    unsigned char *currentChar=buffer;
    int currentBit=0;
    unsigned short newVal=0;
    int mean=0;
    short xMax=chanPtr->xMax; //Filled by pedestalLib
    short xMin=chanPtr->xMin; //Filled by pedestalLib   
    short *input = chanPtr->data;

    //Find min and max
//    for(wordNum=0;wordNum<numInputs;wordNum++) {
//	mean+=input[wordNum];
//	if(input[wordNum]>xMax) xMax=input[wordNum];
//	if(input[wordNum]<xMin) xMin=input[wordNum];
//    }
//    mean/=numInputs;
    //Don't actual use min we just use the median value

    int rangeTotal=xMax-xMin;
    float logValue=(float)(log(rangeTotal+1)/log(2));
    int bitSize=(int)logValue;
    if((logValue-bitSize)>0) bitSize++;
    
    *encTypePtr=getBinaryEncType(bitSize);

    mean=xMin+rangeTotal/2;
//    printf("mean %d\txMin %d\txMax %d\tbitSize %d\n",mean,xMin,xMax,bitSize);
    
    char *meanPtr=(char*)currentChar;
    (*meanPtr)=(char)(mean);
    currentChar++;
/*     *currentChar = (unsigned char) bitSize; */
/*     currentChar++; */

    for(wordNum=0;wordNum<MAX_NUMBER_SAMPLES;wordNum++) {
	newVal=bifurcate(input[wordNum]-mean);
 	numBitsLeft=bitSize;
//	printf("input: %d (%d)\t newVal: %#x\t: bitsize %d\n",input[wordNum],
//	       input[wordNum]-mean,newVal,bitSize);
//	printf("cc %u cbit %d:\n",(int)(currentChar-buffer),currentBit);
	while(numBitsLeft) {
//	    if(wordNum<5) cout << wordNum << "\t" << numBitsLeft << endl;
	    if(numBitsLeft>(8-currentBit)) {			
		bitMask=0;
		for(bitNum=0;bitNum<(8-currentBit);bitNum++)
		    bitMask|=(1<<bitNum);	   
		(*currentChar)|= (newVal&bitMask)<<currentBit;
		newVal=(newVal>>(8-currentBit));
		numBitsLeft-=(8-currentBit);
		currentChar++;
		currentBit=0;
	    }
	    else {			
		bitMask=0;
		for(bitNum=0;bitNum<numBitsLeft;bitNum++)
		    bitMask|=(1<<bitNum);
		(*currentChar)|= (newVal&bitMask)<<currentBit;
		currentBit+=numBitsLeft;
		if(currentBit==8) {
		    currentBit=0;
		    currentChar++;
		}
		numBitsLeft=0;
	    }
	}
    }
//    for(int i=0;i<int(currentChar-buffer);i++) {
//	cout << i << "\t"  << (int)buffer[i] << endl;
//    }
    if(currentBit) currentChar++;
    return (int)(currentChar-buffer);
}

CompressErrorCode_t decodePSWaveLosslessBinary(unsigned char *input,int numBytes,SurfChannelPedSubbed_t *chanPtr, ChannelEncodingType_t encType)
{
    int sampNum=0;
    char *meanPtr=(char*)input;
    short mean=(short)(*meanPtr);
//    unsigned char flag =  input[1];
    int bitSize=getBinaryBitSize(encType);
    int bitNum;

//    printf("mean %d\tbitSize %d\n",mean,bitSize);

    unsigned char *currentChar=&input[1];
    int currentBit=0;
    unsigned short tempNum;
    while(sampNum<MAX_NUMBER_SAMPLES) {
	int fred = *( (int*) currentChar);
//	printf("cc %u cbit %d: \t %#x\n",(int)(currentChar-input),currentBit,fred);
//	bin_prnt_int(fred);
	tempNum=0;	    
	for(bitNum=currentBit;bitNum<currentBit+bitSize;bitNum++) {
	    tempNum|= (((fred>>bitNum)&0x1)<<(bitNum-currentBit));
//	    bin_prnt_short(tempNum);
//	    cout << (((fred>>bitNum)&0x1)<<(bitNum-currentBit));
	}
//	cout << endl;
//	bin_prnt_short(tempNum);

//	cout << sampNum << "\t" << tempNum << endl;

	chanPtr->data[sampNum]=mean+unbifurcate(tempNum);
//	printf("samp %d:\t %#x \t %d\n",sampNum,tempNum,chanPtr->data[sampNum]);
	
	sampNum++;
	currentBit+=bitSize;
	while(currentBit>=8) {
	    currentBit-=8;
	    currentChar++;
	}
	if(((int)(currentChar-input))>numBytes) return COMPRESS_E_BADSIZE;
    }
    return COMPRESS_E_OK;

}

int encodePSWaveLosslessFibonacci(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr) {    
    //Remember this function works an array of pedestal subtracted shorts
    //Which is what SurfChannelPedSubbed_t contains
    int wordNum=0;
    int numBitsLeft;
    int bitMask;
    int bitNum;
    unsigned char *currentChar=buffer;
    int currentBit=0;
    unsigned short newVal=0;
    int mean=chanPtr->mean;
    if(mean>127) mean=127;
    if(mean<-127) mean=-127;
//    short xMax=chanPtr->xMax; //Filled by pedestalLib
//    short xMin=chanPtr->xMin; //Filled by pedestalLib   
    short *input = chanPtr->data;

//    int rangeTotal=xMax-xMin;
//    printf("mean %d\txMin %d\txMax %d\n",mean,xMin,xMax);
    
    char *meanPtr=(char*)currentChar;
    (*meanPtr)=(char)(mean);
    currentChar++;


    for(wordNum=0;wordNum<MAX_NUMBER_SAMPLES;wordNum++) {
	newVal=bifurcate(input[wordNum]-mean);
	newVal=encodeFibonacci(newVal,&numBitsLeft);
	while(numBitsLeft) {
//	    if(wordNum<5) cout << wordNum << "\t" << numBitsLeft << endl;
	    if(numBitsLeft>(8-currentBit)) {			
		bitMask=0;
		for(bitNum=0;bitNum<(8-currentBit);bitNum++)
		    bitMask|=(1<<bitNum);	   
		(*currentChar)|= (newVal&bitMask)<<currentBit;
		newVal=(newVal>>(8-currentBit));
		numBitsLeft-=(8-currentBit);
		currentChar++;
		currentBit=0;
	    }
	    else {			
		bitMask=0;
		for(bitNum=0;bitNum<numBitsLeft;bitNum++)
		    bitMask|=(1<<bitNum);
		(*currentChar)|= (newVal&bitMask)<<currentBit;
		currentBit+=numBitsLeft;
		if(currentBit==8) {
		    currentBit=0;
		    currentChar++;
		}
		numBitsLeft=0;
	    }
	}
    }
//    for(int i=0;i<int(currentChar-buffer);i++) {
//	cout << i << "\t"  << (int)buffer[i] << endl;
//    }
    if(currentBit) currentChar++;
    return (int)(currentChar-buffer);
}

CompressErrorCode_t decodePSWaveLosslessFibonacci(unsigned char *input,int numBytes,SurfChannelPedSubbed_t *chanPtr)
{
    int sampNum=0;
    char *meanPtr=(char*)input;
    short mean=(short)(*meanPtr);

    unsigned char *currentChar=&input[1];
    int currentBit=0;
    unsigned short fibNum;
    unsigned short unfibNum;
    int tempBit,bitNum;
    while(sampNum<MAX_NUMBER_SAMPLES) {
	tempBit=currentBit;
	int fred = *( (int*) currentChar);
	if(sampNum==259)
	    printf("samp %d, fred %#x\n",sampNum,fred);
	while(tempBit<32) {
	    if( ((fred>>tempBit)&0x1) && ((fred>>(tempBit+1))&0x1)) {
		//Got two ones;
		fibNum=0;
		for(bitNum=currentBit;bitNum<=tempBit+1;bitNum++) {
		    fibNum|= (((fred>>bitNum)&0x1)<<(bitNum-currentBit));
		}
		unfibNum=unfibonacci(fibNum);
		chanPtr->data[sampNum]=mean+unbifurcate(unfibNum);
	
		

		tempBit+=2;
		currentChar+=(tempBit/8);
		currentBit=(tempBit%8);
//		cout << tempBit << "\t" << currentChar << "\t" << currentBit << endl;
		break;
	    }
//	    cout << ((fred>>tempBit)&0x1) << " ";
	    tempBit++;
	}
	sampNum++;
	if(((int)(currentChar-input))>numBytes) return COMPRESS_E_BADSIZE;
    }

    return COMPRESS_E_OK;

}

int encodePSWaveLosslessBinFibCombo(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr, ChannelEncodingType_t *encTypePtr) {    
    //Remember this function works an array of pedestal subtracted shorts
    //Which is what SurfChannelPedSubbed_t contains
    float numSigma=3; //Will hard code for now
    int wordNum=0;
    int numBitsLeft;
    int bitMask;
    int bitNum;
    unsigned short fibVal=0;
    int asFib=0;
    int fibBits;
    unsigned char *currentChar=buffer;
    int currentBit=0;
    unsigned char *overflowChar;
    int overflowBit=0;
    unsigned short newVal=0;
    int mean=(int)chanPtr->mean;
    if(mean>127) mean=127;
    if(mean<-127) mean=-127;
    float sigma=chanPtr->rms;
    short xMax=chanPtr->xMax; //Filled by pedestalLib
    short xMin=chanPtr->xMin; //Filled by pedestalLib   
    short *input = chanPtr->data;
    int rangeTotal=(int)(numSigma*sigma);
    float logValue=(float)(log(rangeTotal+1)/log(2));
    int bitSize=(int)logValue;
    if((logValue-bitSize)>0) bitSize++;

    //Reset xMin and xMax to be the binary/fibonacci cut off point
    xMin=mean-(1<<(bitSize-1));
    xMax=mean+(1<<(bitSize-1));
    xMax-=1;
//    printf("mean %d\txMin %d\txMax %d\tbitSize %d\n",mean,xMin,xMax,bitSize);
    
    char *meanPtr=(char*)currentChar;
    (*meanPtr)=(char)(mean);
    currentChar++;
    *encTypePtr=getBinFibEncType(bitSize);
//    *currentChar = (unsigned char) bitSize;
    //   currentChar++;


    overflowChar=currentChar+(MAX_NUMBER_SAMPLES*bitSize)/8;
    overflowBit=(MAX_NUMBER_SAMPLES*bitSize)%8;

    unsigned short flagVal=(1<<bitSize)-1;
    int countOverflows=0;

    for(wordNum=0;wordNum<MAX_NUMBER_SAMPLES;wordNum++) {
	//Check if value is in the range xMin to xMax
	if(input[wordNum]>xMin && input[wordNum]<xMax) {
	    newVal=bifurcate(input[wordNum]-mean);
	}
	else {
	    //If not encode number as fibonacci in the overflow
	    countOverflows++;
	    newVal=flagVal;
	    if(input[wordNum]>=xMax) {
		fibVal=((input[wordNum]-xMax)<<1)+1; //Odd numbers
	    }
	    else {
		fibVal=((xMin-input[wordNum])<<1)+2; //Even numbers
	    }
	    asFib=encodeFibonacci(fibVal,&fibBits);

	    
//	    cout << "Fib:\t" << fibVal << "\t" << asFib << "\t" << fibBits << endl;
//	    bin_prnt_int(asFib);
//	    cout <<

	    numBitsLeft=fibBits;		
	    while(numBitsLeft) {
//	    if(wordNum<3) cout << wordNum << "\t" << numBitsLeft << endl;
		if(numBitsLeft>(8-overflowBit)) {			
		    bitMask=0;
		    for(bitNum=0;bitNum<(8-overflowBit);bitNum++)
			bitMask|=(1<<bitNum);	   
		    (*overflowChar)|= (asFib&bitMask)<<overflowBit;
		    asFib=(asFib>>(8-overflowBit));
		    numBitsLeft-=(8-overflowBit);
		    overflowChar++;
		    overflowBit=0;
		}
		else {			
		    bitMask=0;
		    for(bitNum=0;bitNum<numBitsLeft;bitNum++)
			bitMask|=(1<<bitNum);
		    (*overflowChar)|= (asFib&bitMask)<<overflowBit;
		    overflowBit+=numBitsLeft;
		    if(overflowBit==8) {
			overflowBit=0;
			overflowChar++;
		    }
		    numBitsLeft=0;
		}
	    }
	    
	}
//	cout << wordNum << "\t" << newVal << endl;
 	numBitsLeft=bitSize;		
	while(numBitsLeft) {
//	    if(wordNum<3) cout << wordNum << "\t" << numBitsLeft << endl;
	    if(numBitsLeft>(8-currentBit)) {			
		bitMask=0;
		for(bitNum=0;bitNum<(8-currentBit);bitNum++)
		    bitMask|=(1<<bitNum);	   
		(*currentChar)|= (newVal&bitMask)<<currentBit;
		newVal=(newVal>>(8-currentBit));
		numBitsLeft-=(8-currentBit);
		currentChar++;
		currentBit=0;
	    }
	    else {			
		bitMask=0;
		for(bitNum=0;bitNum<numBitsLeft;bitNum++)
		    bitMask|=(1<<bitNum);
		(*currentChar)|= (newVal&bitMask)<<currentBit;
		currentBit+=numBitsLeft;
		if(currentBit==8) {
		    currentBit=0;
		    currentChar++;
		}
		numBitsLeft=0;
	    }
	}
    }
//    cout << "countOverflows: " << countOverflows << endl;
    if(overflowBit) overflowChar++;
//    cout << int(overflowChar-buffer) << "\t" << overflowBit << endl;
    return (int)(overflowChar-buffer);

}
 
CompressErrorCode_t decodePSWaveLosslessBinFibCombo(unsigned char *input,int numBytes,SurfChannelPedSubbed_t *chanPtr, ChannelEncodingType_t encType) {

    char *meanPtr=(char*)input;
    short mean=(*meanPtr);
//    unsigned char flag =  input[1];
    int bitSize=getBinFibBitSize(encType);   
    short overFlowVals[256];
    short xMin=mean-(1<<(bitSize-1));
    short xMax=mean+(1<<(bitSize-1));
    xMax-=1;
    unsigned short flagVal=(1<<bitSize)-1;

    unsigned char *currentChar=&input[1];
    int currentBit=0;
    unsigned char *overflowChar;
    int overflowBit=0;
    overflowChar=currentChar+(MAX_NUMBER_SAMPLES*bitSize)/8;
    overflowBit=(MAX_NUMBER_SAMPLES*bitSize)%8;
    int numOverflows=numBytes-(int)(overflowChar-input);
//    cout << "numOverflows:  " << numOverflows << endl;

    int overflowNum=0;
    unsigned short tempNum;
    int fibNum=0;
    int sampNum=0;
    int tempBit,bitNum;

   //Loop through overflow
    while(overflowNum<260) {
	tempBit=overflowBit;
	int fred = *( (int*) overflowChar);	
//	bin_prnt_int(fred);
	while(tempBit<32) {
	    //    bin_prnt_int((fred>>tempBit));
	    if( ((fred>>tempBit)&0x1) && ((fred>>(tempBit+1))&0x1)) {
		//Got two ones;
		fibNum=0;
		for(bitNum=overflowBit;bitNum<=tempBit+1;bitNum++) {
		    fibNum|= (((fred>>bitNum)&0x1)<<(bitNum-overflowBit));
		}
//		bin_prnt_int(fibNum);
//		cout << fibNum << endl;
//		tempNum=decodeFibonacci(fibNum,tempBit+2-overflowBit);
		tempNum=unfibonacci(fibNum);
//		cout << fibNum << "\t" << tempBit+2-overflowBit 
//		     <<"\t" << tempNum << endl;
		if(tempNum&0x1) {
		    //Positive
//		    tempNum-=1;
		    overFlowVals[overflowNum]=xMax+((tempNum-1)>>1);
		}
		else {
		    //Negative
		    overFlowVals[overflowNum]=xMin-((tempNum-2)>>1);
		}
//		if(overflowNum<2) cout << buffer[overflowNum] << "\t" << tempNum << endl;
//		cout << overflowNum << "\t" <<   overFlowVals[overflowNum] << endl;
		tempBit+=2;
		overflowChar+=(tempBit/8);
		overflowBit=(tempBit%8);
//		cout << tempBit << "\t" << overflowChar << "\t" << overflowBit << endl;

		break;
	    }
//	    cout << ((fred>>tempBit)&0x1) << " ";
	    tempBit++;
	}
	if(tempBit==32) break;
//	cout << endl;
//	cout << overflowNum << "\t" << buffer[overflowNum] << endl;
	overflowNum++;
//	if(overflowNum>4) break;
    }
    numOverflows=overflowNum;
//    cout << "numOverflows:  " << numOverflows << endl;
    overflowNum=0;
    //Loop through binary

    while(sampNum<MAX_NUMBER_SAMPLES) {
	int fred = *( (int*) currentChar);
//	bin_prnt_int(fred);
	tempNum=0;	    
	for(bitNum=currentBit;bitNum<currentBit+bitSize;bitNum++) {
	    tempNum|= (((fred>>bitNum)&0x1)<<(bitNum-currentBit));
//	    bin_prnt_short(tempNum);
//	    cout << (((fred>>bitNum)&0x1)<<(bitNum-currentBit));
	}
//	cout << endl;
//	bin_prnt_short(tempNum);
//	cout << sampNum << "\t" << tempNum << endl;
	if(tempNum==flagVal) {
	    chanPtr->data[sampNum]=overFlowVals[overflowNum];
	    overflowNum++;
	}
	else {
	    chanPtr->data[sampNum]=mean+unbifurcate(tempNum);
	}
	sampNum++;
	currentBit+=bitSize;
	while(currentBit>=8) {
	    currentBit-=8;
	    currentChar++;
	}
    }
    return COMPRESS_E_OK;
}   



int encodePSWaveLossyMulawOptimally(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr,int mulawBits, ChannelEncodingType_t *encTypePtr) {
  short xMax=chanPtr->xMax; //Filled by pedestalLib
  short xMin=chanPtr->xMin; //Filled by pedestalLib
  short rangeTotal=xMax-xMin;
  int numBits=1;
  while(rangeTotal>(1<<numBits)) numBits++;
  int inputBits=numBits+1;
  if(inputBits<=mulawBits) return encodePSWaveLosslessBinary(buffer,chanPtr,encTypePtr);
  if(inputBits<6) inputBits=6;
  if(inputBits<mulawBits+2) inputBits=mulawBits+2;
  *encTypePtr=getEncodingTypeFromInputAndMuLawBits(inputBits,mulawBits);
  return encodePSWaveLossyMuLaw(buffer,chanPtr,*encTypePtr);
}

int encodePSWaveLossyMuLaw(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr,ChannelEncodingType_t encType)
{
    //Remember this function works an array of pedestal subtracted shorts
    //Which is what SurfChannelPedSubbed_t contains
    int wordNum=0;
    int numBitsLeft;
    int bitMask;
    int bitNum;
    unsigned char *currentChar=buffer;
    int currentBit=0;
    unsigned char newVal=0;
    int inputBits=0,mulawBits=0;
    getInputAndMuLawBits(encType,&inputBits,&mulawBits);
    short maxVal=(1<<(inputBits-1));
    short minVal=-1*maxVal;

    short xMax=chanPtr->xMax; //Filled by pedestalLib
    short xMin=chanPtr->xMin; //Filled by pedestalLib  
    short rangeTotal=xMax-xMin; 
    short *input = chanPtr->data;
    int mean=xMin+(int)rangeTotal/2;
    short testVal;
    int bitSize=mulawBits;

//    printf("mean %d\txMin %d\txMax %d\tbitSize %d\n",mean,xMin,xMax,bitSize);
//    printf("minVal %d, maxVal %d\n",minVal,maxVal);
    char *meanPtr=(char*)currentChar;
    (*meanPtr)=(char)(mean);
    currentChar++;

    for(wordNum=0;wordNum<MAX_NUMBER_SAMPLES;wordNum++) {
      testVal=input[wordNum]-mean;
      if(testVal>maxVal) testVal=maxVal;
      if(testVal<minVal) testVal=minVal;
      newVal=convertToMuLawUC(testVal,inputBits,mulawBits);
//      printf("input %d, testVal %d, newVal %d\n",input[wordNum],testVal,newVal);
      numBitsLeft=bitSize;		
      while(numBitsLeft) {
	if(numBitsLeft>(8-currentBit)) {			
	  bitMask=0;
	  for(bitNum=0;bitNum<(8-currentBit);bitNum++)
	    bitMask|=(1<<bitNum);	   
	  (*currentChar)|= (newVal&bitMask)<<currentBit;
	  newVal=(newVal>>(8-currentBit));
	  numBitsLeft-=(8-currentBit);
	  currentChar++;
	  currentBit=0;
	    }
	else {			
	  bitMask=0;
	  for(bitNum=0;bitNum<numBitsLeft;bitNum++)
	    bitMask|=(1<<bitNum);
	  (*currentChar)|= (newVal&bitMask)<<currentBit;
	  currentBit+=numBitsLeft;
	  if(currentBit==8) {
	    currentBit=0;
	    currentChar++;
	  }
	  numBitsLeft=0;
	}
      }
    }
    //    for(int i=0;i<int(currentChar-buffer);i++) {
    //	cout << i << "\t"  << (int)buffer[i] << endl;
    //    }
    if(currentBit) currentChar++;
    return (int)(currentChar-buffer);
    
    
}



CompressErrorCode_t decodePSWaveLossyMuLaw(unsigned char *input,int numBytes,SurfChannelPedSubbed_t *chanPtr,ChannelEncodingType_t encType)
{
    int sampNum=0;
    char *meanPtr=(char*)input;
    short mean=(short)(*meanPtr);
    int inputBits=0,mulawBits=0;
    getInputAndMuLawBits(encType,&inputBits,&mulawBits);
    int bitSize=mulawBits;
    int bitNum;

    unsigned char *currentChar=&input[1];
    int currentBit=0;
    unsigned char tempNum;
    short newVal;

//    printf("mean %d, mulawBits %d, inputBits %d\n",mean,mulawBits,inputBits);

    while(sampNum<MAX_NUMBER_SAMPLES) {
	int fred = *( (int*) currentChar);
//	bin_prnt_int(fred);
	tempNum=0;	    
	for(bitNum=currentBit;bitNum<currentBit+bitSize;bitNum++) {
	    tempNum|= (((fred>>bitNum)&0x1)<<(bitNum-currentBit));
//	    bin_prnt_short(tempNum);
//	    cout << (((fred>>bitNum)&0x1)<<(bitNum-currentBit));
	}
//	cout << endl;
//	bin_prnt_short(tempNum);
//	cout << sampNum << "\t" << tempNum << endl;
	newVal=convertFromMuLawUC(tempNum,inputBits,mulawBits);
	chanPtr->data[sampNum]=mean+newVal;
//	printf("output %d, newVal %d, tempNum %d\n",chanPtr->data[sampNum],
//	       newVal,tempNum);
	
	sampNum++;
	currentBit+=bitSize;
	while(currentBit>=8) {
	    currentBit-=8;
	    currentChar++;
	}
	if(((int)(currentChar-input))>numBytes) return COMPRESS_E_BADSIZE;
    }
    return COMPRESS_E_OK;


}

void getInputAndMuLawBits(ChannelEncodingType_t encType, int *inputPtr,
			  int *mulawPtr) 
{
  switch(encType) {    
    case ENCODE_LOSSY_MULAW_11_8:
      *inputPtr=11;
      *mulawPtr=8;
      return;
    case ENCODE_LOSSY_MULAW_11_7:
      *inputPtr=11;
      *mulawPtr=7;
      return;
    case ENCODE_LOSSY_MULAW_11_6:
      *inputPtr=11;
      *mulawPtr=6;
      return;
    case ENCODE_LOSSY_MULAW_11_5:
      *inputPtr=11;
      *mulawPtr=5;
      return;
    case ENCODE_LOSSY_MULAW_11_4:
      *inputPtr=11;
      *mulawPtr=4;
      return;
    case ENCODE_LOSSY_MULAW_11_3:
      *inputPtr=11;
      *mulawPtr=3;
      return;
    case ENCODE_LOSSY_MULAW_10_8:
      *inputPtr=10;
      *mulawPtr=8;
      return;
    case ENCODE_LOSSY_MULAW_10_7:
      *inputPtr=10;
      *mulawPtr=7;
      return;
    case ENCODE_LOSSY_MULAW_10_6:
      *inputPtr=10;
      *mulawPtr=6;
      return;
    case ENCODE_LOSSY_MULAW_10_5:
      *inputPtr=10;
      *mulawPtr=5;
      return;
    case ENCODE_LOSSY_MULAW_10_4:
      *inputPtr=10;
      *mulawPtr=4;
      return;
    case ENCODE_LOSSY_MULAW_10_3:
      *inputPtr=10;
      *mulawPtr=3;
      return;
    case ENCODE_LOSSY_MULAW_9_7:
      *inputPtr=9;
      *mulawPtr=7;
      return;
    case ENCODE_LOSSY_MULAW_9_6:
      *inputPtr=9;
      *mulawPtr=6;
      return;
    case ENCODE_LOSSY_MULAW_9_5:
      *inputPtr=9;
      *mulawPtr=5;
      return;
    case ENCODE_LOSSY_MULAW_9_4:
      *inputPtr=9;
      *mulawPtr=4;
      return;
    case ENCODE_LOSSY_MULAW_9_3:
      *inputPtr=9;
      *mulawPtr=3;
      return;
    case ENCODE_LOSSY_MULAW_8_6:
      *inputPtr=8;
      *mulawPtr=6;
      return;
    case ENCODE_LOSSY_MULAW_8_5:
      *inputPtr=8;
      *mulawPtr=5;
      return;
    case ENCODE_LOSSY_MULAW_8_4:
      *inputPtr=8;
      *mulawPtr=4;
      return;
    case ENCODE_LOSSY_MULAW_8_3:
      *inputPtr=8;
      *mulawPtr=3;
      return;
    case ENCODE_LOSSY_MULAW_7_5:
      *inputPtr=7;
      *mulawPtr=5;
      return;
    case ENCODE_LOSSY_MULAW_7_4:
      *inputPtr=7;
      *mulawPtr=4;
      return;
    case ENCODE_LOSSY_MULAW_7_3:
      *inputPtr=7;
      *mulawPtr=3;
      return;
    case ENCODE_LOSSY_MULAW_6_4:
      *inputPtr=6;
      *mulawPtr=4;
      return;
  case ENCODE_LOSSY_MULAW_6_3:
      *inputPtr=6;
      *mulawPtr=3;
      return;
  default:
      *inputPtr=11;
      *mulawPtr=8;
      return;
  }
  *inputPtr=11;
  *mulawPtr=11;
  return;
      
}

ChannelEncodingType_t getEncodingTypeFromInputAndMuLawBits(int inputBits,
							     int mulawBits)
{
  switch (inputBits) {
  case 11:
    switch (mulawBits) {
    case 8:
      return ENCODE_LOSSY_MULAW_11_8;
    case 7:
      return ENCODE_LOSSY_MULAW_11_7;
    case 6:
      return ENCODE_LOSSY_MULAW_11_6;
    case 5:
      return ENCODE_LOSSY_MULAW_11_5;
    case 4:
      return ENCODE_LOSSY_MULAW_11_4;
    case 3:
      return ENCODE_LOSSY_MULAW_11_3;
    default:
      return ENCODE_LOSSY_MULAW_11_8;
    }
  case 10:
    switch (mulawBits) {
    case 8:
      return ENCODE_LOSSY_MULAW_10_8;
    case 7:
      return ENCODE_LOSSY_MULAW_10_7;
    case 6:
      return ENCODE_LOSSY_MULAW_10_6;
    case 5:
      return ENCODE_LOSSY_MULAW_10_5;
    case 4:
      return ENCODE_LOSSY_MULAW_10_4;
    case 3:
      return ENCODE_LOSSY_MULAW_10_3;
    default:
      return ENCODE_LOSSY_MULAW_10_8;
    }
  case 9:
    switch (mulawBits) {
    case 7:
      return ENCODE_LOSSY_MULAW_9_7;
    case 6:
      return ENCODE_LOSSY_MULAW_9_6;
    case 5:
      return ENCODE_LOSSY_MULAW_9_5;
    case 4:
      return ENCODE_LOSSY_MULAW_9_4;
    case 3:
      return ENCODE_LOSSY_MULAW_9_3;
    default:
      return ENCODE_LOSSY_MULAW_9_7;
    }
  case 8:
    switch (mulawBits) {
    case 6:
      return ENCODE_LOSSY_MULAW_8_6;
    case 5:
      return ENCODE_LOSSY_MULAW_8_5;
    case 4:
      return ENCODE_LOSSY_MULAW_8_4;
    case 3:
      return ENCODE_LOSSY_MULAW_8_3;
    default:
      return ENCODE_LOSSY_MULAW_8_6;
    }
  case 7:
    switch (mulawBits) {
    case 5:
      return ENCODE_LOSSY_MULAW_7_5;
    case 4:
      return ENCODE_LOSSY_MULAW_7_4;
    case 3:
      return ENCODE_LOSSY_MULAW_7_3;
    default:
      return ENCODE_LOSSY_MULAW_7_5;
    }
  case 6:
    switch (mulawBits) {
    case 4:
      return ENCODE_LOSSY_MULAW_6_4;
    case 3:
      return ENCODE_LOSSY_MULAW_6_3;
    default:
      return ENCODE_LOSSY_MULAW_6_4;
    }
  default:
    switch (mulawBits) {
    case 8:
      return ENCODE_LOSSY_MULAW_11_8;
    case 7:
      return ENCODE_LOSSY_MULAW_11_7;
    case 6:
      return ENCODE_LOSSY_MULAW_11_6;
    case 5:
      return ENCODE_LOSSY_MULAW_11_5;
    case 4:
      return ENCODE_LOSSY_MULAW_11_4;
    case 3:
      return ENCODE_LOSSY_MULAW_11_3;
    default:
      return ENCODE_LOSSY_MULAW_11_8;
    }
  }
  return ENCODE_NONE;
}

ChannelEncodingType_t getBinaryEncType(int bitSize) {
    switch(bitSize) {
	case 12: return ENCODE_LOSSLESS_12BIT;
	case 11: return ENCODE_LOSSLESS_11BIT;
	case 10: return ENCODE_LOSSLESS_10BIT;
	case 9: return ENCODE_LOSSLESS_9BIT;
	case 8: return ENCODE_LOSSLESS_8BIT;
	case 7: return ENCODE_LOSSLESS_7BIT;
	case 6: return ENCODE_LOSSLESS_6BIT;
	case 5: return ENCODE_LOSSLESS_5BIT;
	case 4: return ENCODE_LOSSLESS_4BIT;
	case 3: return ENCODE_LOSSLESS_3BIT;
	case 2: return ENCODE_LOSSLESS_2BIT;
	case 1: return ENCODE_LOSSLESS_1BIT;
	default:
	    return ENCODE_LOSSLESS_12BIT;
    }
    return ENCODE_LOSSLESS_12BIT;
}
	    
    
int getBinaryBitSize(ChannelEncodingType_t encType) {
    switch(encType) {
	case ENCODE_LOSSLESS_12BIT: return 12;
	case ENCODE_LOSSLESS_11BIT: return 11;
	case ENCODE_LOSSLESS_10BIT: return 10;	   
	case ENCODE_LOSSLESS_9BIT: return 9;
	case ENCODE_LOSSLESS_8BIT: return 8;
	case ENCODE_LOSSLESS_7BIT: return 7;
	case ENCODE_LOSSLESS_6BIT: return 6;
	case ENCODE_LOSSLESS_5BIT: return 5;
	case ENCODE_LOSSLESS_4BIT: return 4;
	case ENCODE_LOSSLESS_3BIT: return 3;
	case ENCODE_LOSSLESS_2BIT: return 2;
	case ENCODE_LOSSLESS_1BIT: return 1;
	default:
	    return 12;
    }
    return 12;
}


ChannelEncodingType_t getBinFibEncType(int bitSize) {
    switch(bitSize) {
	case 10: return ENCODE_LOSSLESS_BINFIB_10BIT;
	case 9: return ENCODE_LOSSLESS_BINFIB_9BIT;
	case 8: return ENCODE_LOSSLESS_BINFIB_8BIT;
	case 7: return ENCODE_LOSSLESS_BINFIB_7BIT;
	case 6: return ENCODE_LOSSLESS_BINFIB_6BIT;
	case 5: return ENCODE_LOSSLESS_BINFIB_5BIT;
	case 4: return ENCODE_LOSSLESS_BINFIB_4BIT;
	case 3: return ENCODE_LOSSLESS_BINFIB_3BIT;
	case 2: return ENCODE_LOSSLESS_BINFIB_2BIT;
	case 1: return ENCODE_LOSSLESS_BINFIB_1BIT;
	default:
	    return ENCODE_LOSSLESS_BINFIB_10BIT;
    }
    return ENCODE_LOSSLESS_BINFIB_10BIT;
}
	    
    
int getBinFibBitSize(ChannelEncodingType_t encType) {
    switch(encType) {
	case ENCODE_LOSSLESS_BINFIB_10BIT: return 10;	   
	case ENCODE_LOSSLESS_BINFIB_9BIT: return 9;
	case ENCODE_LOSSLESS_BINFIB_8BIT: return 8;
	case ENCODE_LOSSLESS_BINFIB_7BIT: return 7;
	case ENCODE_LOSSLESS_BINFIB_6BIT: return 6;
	case ENCODE_LOSSLESS_BINFIB_5BIT: return 5;
	case ENCODE_LOSSLESS_BINFIB_4BIT: return 4;
	case ENCODE_LOSSLESS_BINFIB_3BIT: return 3;
	case ENCODE_LOSSLESS_BINFIB_2BIT: return 2;
	case ENCODE_LOSSLESS_BINFIB_1BIT: return 1;
	default:
	    return 10;
    }
    return 10;
}
    





unsigned short simpleCrcShort(unsigned short *p, unsigned long n)
{

    unsigned short sum = 0;
    unsigned long i;
    for (i=0L; i<n; i++) {
	sum += *p++;
    }
    return ((0xffff - sum) + 1);
}

char *compressErrorCodeAsString(CompressErrorCode_t code) 
{
    char *string;
    switch(code) {
	case COMPRESS_E_OK:
	    string = "Success";
	    break;
	case COMPRESS_E_PACK:
	    string = "Error packing data";
	    break;
	case COMPRESS_E_BADSIZE:
	    string = "Enocded data size wrong";
	    break;
	case COMPRESS_E_UNPACK:
	    string = "Error unpacking data";
	    break;
	case COMPRESS_E_NO_PEDS:
	    string = "Pedestals not available";
	    break;
	case COMPRESS_E_BAD_PEDS:
	    string = "Pedestals malformed";
	    break;
	case COMPRESS_E_TOO_BIG:
	    string = "Data overflow";
	    break;
	case COMPRESS_E_BAD_CRC:
	    string = "Checksum mismatch";
	    break;
	default:
      string = "Unknown error code" ;
   }

   return (string) ;
}
	    
