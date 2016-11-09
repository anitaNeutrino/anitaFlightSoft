#include <stdio.h>
#include "tuffLib/tuffLib.h"
#include <unistd.h>
#include <time.h>

#define _POSIX_SOURCE 1 /* POSIX compliant source */


#define DEFAULT_DEVICE "/dev/ttyTUFF" 
#define NUM_IRFCM 4 
#define NUM_ASSIGNMENTS 4

//16265  Expected 'correct' value for lower tuffs
//32649  Expected 'correct' value for bottom tuffs

const char * notch_assignments[NUM_ASSIGNMENTS] = {
"\r\n{\"set\":{\"irfcm\":%d,\"default\":[0,16265]}}\r\n" , 
"\r\n{\"set\":{\"irfcm\":%d,\"default\":[1,32649]}}\r\n",
"\r\n{\"set\":{\"irfcm\":%d,\"default\":[2,16265]}}\r\n", 
"\r\n{\"set\":{\"irfcm\":%d,\"default\":[3,32649]}}\r\n"
};

int main (int argc, char ** argv)
{

  int i; 
  const char * device = argc > 1 ? argv[1] : DEFAULT_DEVICE; 
  tuff_dev_t * dev = tuff_open(device); 

  int fd = tuff_getfd(dev); 

  for (i = 0; i < NUM_IRFCM; i++) 
  {
    char buf[512];
    int j; 
    for (j = 0; j < NUM_ASSIGNMENTS; j++)
    {
       sprintf(buf, notch_assignments[j], i); 
       printf("Sending %s to IRFCM %d\n", buf, i); 
       write(fd, buf, strlen(buf)+1); 
       tuff_waitForAck(dev,i,-1); 
       printf("Got ack!\n"); 
       sleep(2);
    }
  }


  tuff_close(dev); 

  return 0; 











}
