#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>


#include <sys/types.h>

#include <time.h>

#include "sbsTempLib/sbsTempLib.h"

int main (int argc, char **argv)
{   
  int dt=10; //seconds
  int tempIndex=0;
  if(argc>1) {
    dt=atoi(argv[1]);
  }

  printf("unixTime\t");
  for(tempIndex=0;tempIndex<NUM_SBS_TEMPS;tempIndex++) {
    printf("%s\t",getSBSTemperatureLabel(tempIndex));
  }  
  printf("\n");
    
  while(1) {
    printf("%u\t",time(NULL));
    for(tempIndex=0;tempIndex<NUM_SBS_TEMPS;tempIndex++) {
      printf("%3.1f\t",convertAnitaToHuman(readSBSTemperature(tempIndex)));
    }  
    printf("\n");

    sleep(dt);
    
  }
  

  return 0;
}

