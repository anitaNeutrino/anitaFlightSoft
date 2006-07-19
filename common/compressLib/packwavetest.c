#include <stdio.h>
#include <stdlib.h>
#include "compresslib.h"

int main(int argc, char* argv[]){
     unsigned short nwords, width, nstrip, npack;
     unsigned short packedbytes;
     unsigned char packed[65536];
     unsigned short data[65536];
     int i;
     if (argc != 4){
	  printf("Usage: packwavetest <width> <nstrip> <npack>\n.");
	  exit(-1);
     }
     sscanf(argv[1],"%hu",&width);
     sscanf(argv[2],"%hu",&nstrip);
     sscanf(argv[3],"%hu",&npack);
     scanf("%hu",&nwords);
     for (i=0; i< nwords; i++){
	  scanf("%hu",&data[i]);
     }
     packedbytes=packwave(nwords,width,nstrip,npack,data,packed);
     printf("%hu\n",packedbytes);
     for (i=0;i<packedbytes;i++){
	  printf("%#hhx\n",packed[i]);
     }
 }
	  
