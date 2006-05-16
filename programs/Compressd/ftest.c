#include <stdio.h>
#include <stdlib.h>
#include "fibonacci.h"

int main(int argc,char* argv[])
{
     unsigned char input;
     if(argc!=2){
	  fprintf(stderr,"Usage: fibonacci <number>");
	  exit(-1);
     }
     sscanf(argv[1],"%i",&input);
     printf("%i\n",input);
     printf("%#x\n",fibonacci(input));
}
