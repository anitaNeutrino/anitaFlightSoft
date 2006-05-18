#include <stdio.h>
#include <stdlib.h>

#include "bytepack.h"

int bytepack(int n, unsigned int *in, unsigned char *out)
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


