#include <stdio.h>
#include <stdlib.h>
#include "fibonacci.h"
#include "bifurcate.h"

// simple test code for bifurcate.c
int main(int argc,char* argv[])
{
     short input;
     if(argc!=2){
	  fprintf(stderr,"Usage: btest <number>");
	  exit(-1);
     }
     sscanf(argv[1],"%hi",&input);
     printf("%hi\n",input);
     printf("%#x\n",fibonacci(bifurcate(input)));
}
