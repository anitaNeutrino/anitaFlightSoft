#include <stdio.h>
#include <stdlib.h>
#include "tuffLib/tuffLib.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */


#define DEFAULT_DEVICE "/dev/ttyTUFF" 
#define NUM_IRFCM 4 

int main (int argc, char ** argv)
{

  int i; 
  const char * device = argc > 1 ? argv[1] : DEFAULT_DEVICE; 
  int which = argc > 2 ? atoi(argv[2]) : -1; 
  int start = which == -1 ? 0  : which; 
  int end = which == -1 ? NUM_IRFCM  : which+1; 
  tuff_dev_t * dev = tuff_open(device); 

  for (i = start; i < end; i++) 
  {
    printf("Sending reset to IRFCM %d\n", i); 
    tuff_setQuietMode(dev,false); 

    tuff_reset(dev, i); 

    tuff_setQuietMode(dev,false); 
    tuff_waitForAck(dev,i,-1); 
    printf("Got ack from %d!\n", i); 


  }

  return 0; 











}
