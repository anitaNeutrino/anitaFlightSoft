#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "compressLib/compressLib.h"
#include "utilLib/utilLib.h"


unsigned int fn[24]={1,2,3,5,8,13,21,34,55,89,
		     144,233,377,610,987,1597,2584,4181,6765,10946,
		     17711,28657,46368,75025}; 
//these are enough to encode unsigned short

//convert a short to its fibonacci representation with a 11 prefix
unsigned int fibonacci(unsigned short input){
    int numBits;
    return encodeFibonacci(input,&numBits);
}

unsigned int encodeFibonacci(unsigned short input,int *numBits){
     unsigned int output;
     int i;
     i=0;
     // find the first fibonacci number greater than input
     while(input>=fn[i]){
	  i++;
     }
     // set the prefix bit
     (*numBits)=i+1;
     output=1<<(i);
     i--; // we are now at the largest fibonacci number less than input
     //now set the bits for the minimal fibonacci representation
     while(i>=0){
	  if (input>=fn[i]){
	       output += (1<<i);
	       input -= fn[i];
	  }
	  i--;
     }  
     return output;
}

// reverse the above operation
unsigned short unfibonacci(unsigned int input)
{
     unsigned int i;
     unsigned short output;
     unsigned int curbit, lastbit;
     output=0;
     lastbit=0;
     for (i=0; i<23; i++){
	  curbit=(input>>i) & 0x0001;
	  if (curbit==1 && lastbit==1) break;  // done--found the prefix bit
	  else if (curbit==1) output+=fn[i]; 
	  lastbit=curbit;
     }
     return output;
}

	  
// perform the bifurcation mapping from integers to the positive
// integers as a prelude to applying prefix codes (e.g. fibonacci)
//
// chosen mapping is 0,-1,1,-2,2... => 1,2,3,4,5
// 
// posi integers map to 2*n+1
// negative integers map 2*abs(n)
//

unsigned short bifurcate(short input)
{
     unsigned short output;
     if (input>=0){
	  output=(2*(unsigned short) input + 1);
     }
     else{
	  output=(2* (unsigned short)abs(input));
     }
     return output;
}

short unbifurcate(unsigned short input)
{
     // note zero is not a valid input
     short output;
     if ((input % 2) == 1){//odd => non-negative
	  output=((input-1)/2);
     }
     else{ //even => negative
	  output=-(input/2);
     }
     return output;
}
	  
	  
// discard (in place) the last nbits bits of the data, shifting right
// number of words is nwords
void bitstrip(unsigned short nbits, unsigned short nwords, unsigned short *data)
{
     unsigned short i;
     for (i=0; i<nwords; i++){
	  data[i]=data[i]>>nbits;
     }
}


// pack the nbits least-significant bits of the data
// into a byte stream.  Returns number of bytes of buffer used.
// The input data are left unchanged.
unsigned short bitpack(unsigned short nbits, unsigned short nwords,
	     unsigned short *in, unsigned char *out)
{
     unsigned short i;
     unsigned short scratch, mask1,mask2, byte, bit, nbitsleft;
     byte=0; //current output byte
     bit=0; //current output bit
     out[0]=0;
     mask1= ~(0xffff << (unsigned short) nbits); //mask to yield nbits lsbs
     for (i=0; i<nwords; i++){
	  scratch= in[i] & mask1;
	  nbitsleft=nbits;
	  while (nbitsleft>0){
	       if (nbitsleft>(8-bit)){ //won't all fit
		    mask2 = ~((0xffff)<<(8-bit));
		    out[byte] |= (scratch & mask2) << bit;
		    mask2 = ~mask2;
		    scratch = ((scratch & mask2) >> (8-bit));
		    nbitsleft -= (8-bit);
		    byte++; bit=0; out[byte]=0;
	       }
	       else { //everything fits in current byte
		    out[byte] |= scratch << bit;
		    bit = bit + nbitsleft;
		    if (bit==8){ byte++; bit=0; out[byte]=0;}
		    nbitsleft=0;
	       }
	  }
     }
     if (bit==0) return byte;
     else return byte+1;
}

//pack fibonacci code into bytes

int codepack(int n, unsigned int *in, unsigned char *out)
{
     //need to add checks on m!!!
     int byte,i;
     unsigned char bit, nbits;
     unsigned int sbit,scratch,mask;
     // in[n] is packed into out with leading zeros removed.
     // it must be filled with nonzero data in a prefix code
     byte=0;//current output byte
     bit=0;//next bit to be written
     out[0]=0;
     for (i=0; i<n; i++){ //i indexes input array
	  //how many bits in in[i]?
	  nbits=0;sbit=0x1;
	  while (sbit<=in[i]) {sbit=sbit<<1; nbits++;}
	  //there are nbit bits before there are leading zeros
	  //last 1 is in bit (nbit-1)
	  scratch=in[i];
	  while (nbits>0){
	       if (nbits>((unsigned char)8-bit)){
               //pack the least significant bits into the current byte
		    mask= ~((unsigned int)(0xffffffff)<<(8-bit));
		    out[byte] |= (scratch & mask) << bit;
		    mask = ~mask;
		    scratch=(scratch & mask) >> (8-bit);
		    nbits -= (8-bit);
		    byte++; bit=0; out[byte]=0;
	       }
	       else{//everything fits
		    out[byte] |= scratch << bit;
		    bit= bit + nbits;
		    if (bit==8){ byte++; bit=0; out[byte]=0;}
		    nbits=0;
	       }
	  }
     }
     if (bit==0) return byte; 
     else return byte+1;
}

//unpack fibonacci code 

int codeunpack(int m, unsigned char *in, unsigned int *out)
{
     int inbyte, inbit;
     int outword, outbit;
     unsigned int curbit, lastbit;
/* proceed one bit at a time.  When two ones are seen, advance
	to the next output word */
     inbyte=0; inbit=0;
     outword=0,outbit=0;
     lastbit=0;
     out[outword]=0;
     while (inbyte<m){
	  curbit=(in[inbyte]>>inbit) & 0x1;
	  out[outword] |= curbit << outbit;
	  inbit++; outbit++; 
	  if (inbit==8){ inbyte++; inbit=0;}
	  if ((curbit==1 && lastbit==1)|| outbit==32){
	       outword++; outbit=0; out[outword]=0;
	       //printf("%i %hx\n",outword,out[outword-1]);
	  }
	  if (outbit!=0) lastbit=curbit;
	  else lastbit=0;
     }
     return outword;
}

// utilty routing needed for sorting data in median finder

int uscompare(const void * e1, const void * e2)
{
     int result;
     if (*(unsigned short *)e1 < *(unsigned short *)e2) result=-1;
     else if (*(unsigned short*)e1 > *(unsigned short *)e2) result=1;
     else result=0;
     return result;
}

// find the median of an array of unsigned shorts     

unsigned short findmedian(unsigned short nwords, unsigned short *in)
{
     unsigned short scratch[nwords];
     memcpy(scratch,in,nwords*(sizeof(unsigned short)));
     qsort(scratch,nwords,sizeof(unsigned short),uscompare);
     return scratch[nwords/2];
}
     

// function packwave
//
//pack an array of nwords unsigned shorts
//width is the number of bits of data (12 for ANITA SURFs)
//(the caller is responsible for masking off the bits above width)
//nstrip is the number of bits that are stripped and thrown away
//npack is the number of bits that are packed verbatim, after stripping
//the input array will have the width-(npack+nstrip) bits remaining.
//the return value is the number of bytes in the output array. 

unsigned short packwave(unsigned short nwords, 
			unsigned short width, unsigned short nstrip, 
			unsigned short npack, unsigned short *in,
			unsigned char *out)
{
     unsigned short packbytes, codebytes=0, median=0;
     unsigned short i;
     unsigned int scratch[65536];
     if (nstrip!=0) bitstrip(nstrip,nwords,in);
     if (npack!=0) {
	  packbytes=bitpack(npack,nwords,in,out+6);
	  bitstrip(npack,nwords,in);
     }
     else{
	  packbytes=0;
     }
     if ((width-(nstrip+npack))!=0){
	  for (i=0; i<nwords; i++){
	       median=findmedian(nwords,in);
	       scratch[i]=fibonacci(bifurcate((int) in[i]- (int)median));
	       codebytes=codepack(nwords,scratch,out+6+packbytes);
	  }
     }
     else
     {
	  codebytes=0;
     }
     out[0]=(unsigned char) (packbytes>>8);
     out[1]=(unsigned char) (packbytes & 0xff);
     out[2]=(unsigned char) (codebytes>>8);
     out[3]=(unsigned char) (codebytes & 0xff);
     out[4]=(unsigned char) (median>>8);
     out[5]=(unsigned char) (median & 0xff);
     return (packbytes+codebytes+6);
}
	
unsigned short unpackwave(unsigned short nbytes, 
			  unsigned short width, unsigned short nstrip, 
			  unsigned short npack, unsigned char *in,
			  unsigned short *out)
{
     unsigned short median,codebytes,packbytes;
     unsigned int scratch[65536];
     unsigned short nwords;
     int i;
     median=in[4]*256+in[5];
     codebytes=in[2]*256+in[3];
     packbytes=in[0]*256+in[2];
     //data length only known from the fibonacci code part
     nwords=codeunpack(codebytes,&(in[6+packbytes]),scratch);
     for (i=0;i<nwords;i++){
	  out[i]=median+
	       (unbifurcate(unfibonacci(scratch[i]))<<(nstrip+npack));
     }
//handle packed bits here NEEDS WORK !!!
     return nwords;
}


CompressErrorCode_t packEvent(AnitaEventBody_t *bdPtr,
			      EncodeControlStruct_t *cntlPtr,
			      unsigned char *output,
			      int *numBytes) 
{
    int count=0,surfStart=0;
    int surf=0;
    int chan=0;  
    EncodedSurfPacketHeader_t *surfHdPtr;


    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedSurfPacketHeader_t*) &output[count];
	surfStart=count;
	surfHdPtr->eventNumber=bdPtr->eventNumber;
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

    return COMPRESS_E_OK;
    

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
    count+=sizeof(EncodedSurfChannelHeader_t);
    
    switch(encType) {
	case ENCODE_NONE:
	    encSize=encodeWaveNone(&buffer[count],chanPtr);
	    break;
	default:
	    encType=ENCODE_NONE;
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


int encodePSChannel(ChannelEncodingType_t encType, SurfChannelPedSubbed_t *chanPtr, unsigned char *buffer) 
{
//    printf("Event %lu, ChanId %d\n",theHead.eventNumber,
//	   chanPtr->header.chanId);



    EncodedSurfChannelHeader_t *chanHdPtr
	=(EncodedSurfChannelHeader_t*) &buffer[0];
    int count=0;    
    int encSize=0;
    chanHdPtr->rawHdr=chanPtr->header;
    count+=sizeof(EncodedSurfChannelHeader_t);
    
    switch(encType) {
	case ENCODE_NONE:
	    encSize=encodePSWaveNone(&buffer[count],chanPtr);
	    break;
	default:
	    encType=ENCODE_NONE;
	    encSize=encodePSWaveNone(&buffer[count],chanPtr);
	    break;	    
    }    
    chanHdPtr->numBytes=encSize;
    chanHdPtr->crc=simpleCrcShort((unsigned short*)&buffer[count],
				  encSize/2);
    return (count+encSize);        
}

int encodePSWaveNone(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr) {    
    int encSize=MAX_NUMBER_SAMPLES*sizeof(unsigned short);
    memcpy(buffer,chanPtr->data,encSize);
    return encSize;
}

int encodePSWaveLosslessBinary(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr) {    
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
    int bitSize=ceil(log2f((float)rangeTotal));
    mean=xMin+rangeTotal/2;
    printf("mean %d\txMin %d\txMax %d\tbitSize %d\n",mean,xMin,xMax,bitSize);
    
    char *meanPtr=(char*)currentChar;
    *meanPtr=(mean);
    currentChar++;
    *currentChar = (unsigned char) bitSize;
    currentChar++;

    for(wordNum=0;wordNum<MAX_NUMBER_SAMPLES;wordNum++) {
	if(input[wordNum]>=mean) {
	    newVal=((input[wordNum]-mean)<<1)+1;
	}
	else {
	    newVal=((mean-input[wordNum])<<1);
	}
 	numBitsLeft=bitSize;		
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


int encodePSWaveLosslessBinFibCombo(unsigned char *buffer,SurfChannelPedSubbed_t *chanPtr) {    
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
    float sigma=chanPtr->rms;
    short xMax=chanPtr->xMax; //Filled by pedestalLib
    short xMin=chanPtr->xMin; //Filled by pedestalLib   
    short *input = chanPtr->data;
    int rangeTotal=(int)(numSigma*sigma);
    int bitSize=ceil(log2f((float)rangeTotal));

    //Reset xMin and xMax to be the binary/fibonacci cut off point
    xMin=mean-(1<<(bitSize-1));
    xMax=mean+(1<<(bitSize-1));
    xMax-=1;
    printf("mean %d\txMin %d\txMax %d\tbitSize %d\n",mean,xMin,xMax,bitSize);
    
    char *meanPtr=(char*)currentChar;
    *meanPtr=(mean);
    currentChar++;
    *currentChar = (unsigned char) bitSize;
    currentChar++;


    overflowChar=currentChar+(MAX_NUMBER_SAMPLES*bitSize)/8;
    overflowBit=(MAX_NUMBER_SAMPLES*bitSize)%8;

    unsigned short flagVal=(1<<bitSize)-1;
    int countOverflows=0;

    for(wordNum=0;wordNum<MAX_NUMBER_SAMPLES;wordNum++) {
	//Check if value is in the range xMin to xMax
	if(input[wordNum]>xMin && input[wordNum]<xMax) {
	    if(input[wordNum]>=mean) {
		newVal=((input[wordNum]-mean)<<1)+1;
	    }
	    else {
		newVal=((mean-input[wordNum])<<1);
	    }
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
    

unsigned short simpleCrcShort(unsigned short *p, unsigned long n)
{

    unsigned short sum = 0;
    unsigned long i;
    for (i=0L; i<n; i++) {
	sum += *p++;
    }
    return ((0xffff - sum) + 1);
}
