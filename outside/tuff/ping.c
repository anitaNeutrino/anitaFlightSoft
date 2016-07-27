#include "tuffLib/tuffLib.h" 
#include <stdio.h>
#include <stdlib.h>



int main(int nargs, char ** args) 
{
  tuff_dev_t * dev; 
  unsigned int irfcm; 
  int npongs =0; 
  
  if (nargs < 2) 
  {

    printf("Usage: ping serial_device irfcm \n"); 
    return 1; 

  }

  dev = tuff_open(args[1]); 
  irfcm = atoi(args[2]); 
  tuff_reset(dev,irfcm); 
  tuff_setQuietMode(dev,false); 
  npongs = tuff_pingiRFCM(dev, 10, 1, &irfcm); 
  printf("npongs: %d\n", npongs); 

  tuff_close(dev); 

  return 0; 



}
