#include <stdlib.h>
#include <string.h>

#include "findmedian.h"

int uscompare(const void * e1, const void * e2)
{
     int result;
     if (*(unsigned short *)e1 < *(unsigned short *)e2) result=-1;
     else if (*(unsigned short*)e1 > *(unsigned short *)e2) result=1;
     else result=0;
     return result;
}
     

unsigned short findmedian(unsigned short nwords, unsigned short *in)
{
     unsigned short scratch[nwords];
     memcpy(scratch,in,nwords*(sizeof(unsigned short)));
     qsort(scratch,nwords,sizeof(unsigned short),uscompare);
     return scratch[nwords/2];
}
     
	   
