#include <stdio.h>
#include <stdlib.h>
#include "fibonacci.h"
#include "bytepack.h"
int main(int argc, char* argv[]){
     unsigned char nbytes;
     int packedbytes;
     unsigned char testbyte[255];
     unsigned short testcode[255];
     unsigned char packed[1024];
     int i,j,k;
     if (argc!=2){
	  fprintf(stderr,"Usage: packtest <number of bytes to code and pack>");
	  exit(-1);
     }
     sscanf(argv[1],"%u",&nbytes);
     for (i=0;i<nbytes;i++){
	  testbyte[i]=i+1; //fibonacci code cannot represent zero
	  testcode[i]=fibonacci(testbyte[i]);
     }
     packedbytes=bytepack(nbytes,1024,testcode,packed);
     printf("%i\n",packedbytes);
     for (i=0;i<packedbytes;i++){
	  printf("%02x\n",packed[i]);
     }
}
