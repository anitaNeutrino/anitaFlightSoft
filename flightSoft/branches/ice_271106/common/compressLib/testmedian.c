#include <stdio.h>
#include <stdlib.h>
#include "compresslib.h"

int main(int argc, char* argv[]){
     unsigned short nwords;
     unsigned short testword[65536];
     unsigned short median;
     short i;
     if (argc!=2){
	  fprintf(stderr,"Usage: testmedian <1/2 number of words>");
	  exit(-1);
     }
     sscanf(argv[1],"%hu",&nwords);
     for (i=0;i<nwords;i++){
	  testword[i]=i;
	  testword[2*nwords-(i+1)]=i;
     }
     median=findmedian(2*nwords,testword);
     printf("%hu\n",median);
}
