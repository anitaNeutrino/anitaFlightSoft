#include <stdio.h>
#include <stdlib.h>

#include "bitpack.h"

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
		    scratch = (scratch & mask2) >> 8-bit;
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
