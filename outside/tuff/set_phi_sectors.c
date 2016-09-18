#include <stdio.h>
#include "tuffLib/tuffLib.h"
#include <unistd.h>

#define _POSIX_SOURCE 1 /* POSIX compliant source */


#define DEFAULT_DEVICE "/dev/ttyTUFF" 
#define NUM_IRFCM 4 


const char * phi_sector_assignments[NUM_IRFCM] = {
"\r\n{\"set\":{\"irfcm\":0,\"phi\":[9,11,12,14,15,0,9,11,12,14,15,0,1,3,4,6,7,8,1,3,4,6,7,8]}}\r\n" , 
"\r\n{\"set\":{\"irfcm\":1,\"phi\":[9,10,11,12,13,15,9,10,11,12,13,15,1,2,3,5,6,7,1,2,3,5,6,7]}}\r\n", 
"\r\n{\"set\":{\"irfcm\":2,\"phi\":[10,11,13,14,15,0,10,11,13,14,15,0,2,3,4,5,6,8,2,3,4,5,6,8]}}\r\n", 
"\r\n{\"set\":{\"irfcm\":3,\"phi\":[9,10,12,13,14,0,9,10,12,13,14,0,1,2,4,5,7,8,1,2,4,5,7,8]}}\r\n"
};

int main (int argc, char ** argv)
{

  int i; 
  const char * device = argc > 1 ? argv[1] : DEFAULT_DEVICE; 
  tuff_dev_t * dev = tuff_open(device); 

  int fd = tuff_getfd(dev); 

  for (i = 0; i < NUM_IRFCM; i++) 
  {
    printf("Sending %s to IRFCM %d\n", phi_sector_assignments[i], i); 
    write(fd, phi_sector_assignments[i], strlen(phi_sector_assignments[i])+1); 
    tuff_waitForAck(dev,i,-1); 
    printf("Got ack!\n"); 
  }


  tuff_close(dev); 

  return 0; 











}
