#include <stdio.h>
#include <stdlib.h>

#include "bitstrip.h"

// discard (in place) the last nbits bits of the data, shifting right
// number of words is nwords
void bitstrip(unsigned short nbits, unsigned short nwords, unsigned short *data)
{
     unsigned short i;
     for (i=0; i<nwords; i++){
	  data[i]=data[i]>>nbits;
     }
}
