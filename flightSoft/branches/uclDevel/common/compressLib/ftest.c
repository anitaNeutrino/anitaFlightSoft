#include <stdio.h>
#include <stdlib.h>
#include "compressLib/compressLib.h"


// simple test code for fibonacci.c
int main(int argc,char* argv[])
{
     unsigned short input;
     
     if(argc!=2){
	  fprintf(stderr,"Usage: fibonacci <number>");
	  exit(-1);
     }
     sscanf(argv[1],"%u",&input);
     printf("%i\n",input);
     printf("Jim %#x\n",fibonacci(input));
}
