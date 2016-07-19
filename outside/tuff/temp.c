#include "tuffLib/tuffLib.h" 
#include <stdio.h>
#include <stdlib.h>



int main(int nargs, char ** args) 
{
  tuff_dev_t * dev; 
  int irfcm; 
  
  if (nargs < 2) 
  {

    printf("Usage: temp serial_device irfcm \n"); 
    return 1; 

  }

  dev = tuff_open(args[1]); 
  irfcm = atoi(args[2]); 
  printf("Temperature reported by TUFF Master in  IRFCM %d is %f\n", irfcm, tuff_getTemperature(dev, irfcm)); 

  tuff_close(dev); 

  return 0; 



}
