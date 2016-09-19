#include "tuffLib/tuffLib.h" 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



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
  /*
//  sleep(1); 
  tuff_setQuietMode(dev,true); 
  sleep(1); 
  tuff_reset(dev,irfcm); 
  printf("Reset!\n"); 
  sleep(1); 

  
  tuff_waitForAck(dev,irfcm,true); 
//  printf("Got ack!\n"); 

*/
  printf("Temperature reported by TUFF Master in  IRFCM %d is %f\n", irfcm, tuff_getTemperature(dev, irfcm,-1)); 

  /*
  tuff_setQuietMode(dev,true); 
  */


  tuff_close(dev); 

  return 0; 



}
