#include <stdio.h>
#include <stdlib.h>
#include "fibonacci.h"
#include "byteunpack.h"

int main(int argc, char* argv){
     int packedbytes, i, codewords;
     unsigned char packed[1024];
     unsigned short testcode[1024];
     scanf("%i",&packedbytes);
     printf("%i\n",packedbytes);
     for (i=0;i<packedbytes; i++){
	  scanf("%hhx",&(packed[i]));
     }
//     printf("%i\n",packedbytes);
//     for (i=0;i<packedbytes;i++){
//	  printf("%02x\n",packed[i]);
//     }
     codewords=byteunpack(packedbytes,1024,packed,testcode);
     printf("%i\n",codewords);
     for (i=0;i<codewords;i++){
	  printf("%hhu\n",unfibonacci(testcode[i]));
     }
}
	  
