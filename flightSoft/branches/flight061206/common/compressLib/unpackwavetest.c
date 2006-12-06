#include <stdio.h>
#include <stdlib.h>
#include "compresslib.h"

int main(int argc, char* argv[])
{
     unsigned short nwords, width, nstrip, npack;
     unsigned short packedbytes;
     unsigned char packed[65536];
     unsigned short data[65536];
     int i;
     if (argc != 4){
	  printf("Usage: unpackwavetest <width> <nstrip> <npack>\n.");
	  exit(-1);
     }
     sscanf(argv[1],"%hu",&width);
     sscanf(argv[2],"%hu",&nstrip);
     sscanf(argv[3],"%hu",&npack);

     scanf("%hu",&packedbytes);
     for (i=0; i< packedbytes; i++){
	  scanf("%hhx",&packed[i]);
     }
     nwords=unpackwave(packedbytes,width,nstrip,npack,packed,data);
     printf("%hu\n",nwords);
     for (i=0;i<nwords;i++){
	  printf("%hu\n",data[i]);
     }
}
	  
