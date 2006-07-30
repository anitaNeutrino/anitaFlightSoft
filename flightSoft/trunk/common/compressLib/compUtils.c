/*! \file compUtils.c
    \brief Various packing routines 
    
    Some useful functions for
    June 2006  rjn@mps.ohio-state.edu (mainly written by JJB)
*/

#include "compressLib/compressLib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

unsigned char charbifurcate(char input)
{
     unsigned char output;
     if (input>=0){
	  output=(2*(unsigned char) input + 1);
     }
     else{
	 input*=-1;
	 output=(2* (unsigned char)input);
     }
     return output;
}

char charunbifurcate(unsigned char input)
{
     // note zero is not a valid input
     char output;
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
