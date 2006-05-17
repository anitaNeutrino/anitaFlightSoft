#include <stdio.h>
#include <stdlib.h>
#include "byteunpack.h"

int byteunpack(int m, int nmax, unsigned char *in, unsigned int *out)
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
