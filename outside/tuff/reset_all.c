#include <stdio.h>
#include "tuffLib/tuffLib.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */


#define DEFAULT_DEVICE "/dev/ttyTUFF" 
#define NUM_IRFCM 4 

int main (int argc, char ** argv)
{

  int i; 
  const char * device = argc > 1 ? argv[1] : DEFAULT_DEVICE; 
  tuff_dev_t * dev = tuff_open(device); 

  for (i = 0; i < NUM_IRFCM; i++) 
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
