#include <stdio.h>
#include <stdlib.h>

#include "packwave.h"
#include "bitstrip.h"
#include "bitpack.h"
#include "bifurcate.h"
#include "fibonacci.h"
#include "bytepack.h"
#include "findmedian.h"

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
     unsigned short packbytes, codebytes, median;
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
	       codebytes=bytepack(nwords,scratch,out+6+packbytes);
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
	
