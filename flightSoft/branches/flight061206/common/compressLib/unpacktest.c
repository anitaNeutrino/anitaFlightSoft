#include <stdio.h>
#include <stdlib.h>
#include "compresslib.h"

int main(int argc, char* argv[]){
     int packedbytes, i, codewords;
     unsigned char packed[200000];
     unsigned int testcode[65536];
     scanf("%i",&packedbytes);
     printf("%i\n",packedbytes);
     for (i=0;i<packedbytes; i++){
	  scanf("%hhx",&(packed[i]));
     }
//     printf("%i\n",packedbytes);
//     for (i=0;i<packedbytes;i++){
//	  printf("%02x\n",packed[i]);
//     }
     codewords=codeunpack(packedbytes,packed,testcode);
     printf("%i\n",codewords);
     for (i=0;i<codewords;i++){
	  printf("%hi\n",unbifurcate(unfibonacci(testcode[i])));
     }
}
	  
