#include <stdio.h>
#include "tuffLib/tuffLib.h"
#include <unistd.h>

#define _POSIX_SOURCE 1 /* POSIX compliant source */


#define DEFAULT_DEVICE "/dev/ttyTUFF" 
#define NUM_IRFCM 4 


const char * phi_sector_assignments[NUM_IRFCM] = {
"\r\n{\"set\":{\"irfcm\":0,\"phi\":[10,12,13,15,16,1,10,12,13,15,16,1,2,4,5,7,8,9,2,4,5,7,8,9]}}\r\n" , 
"\r\n{\"set\":{\"irfcm\":1,\"phi\":[10,11,12,13,14,16,10,11,12,13,14,16,2,3,4,6,7,8,2,3,4,6,7,8]}}\r\n", 
"\r\n{\"set\":{\"irfcm\":2,\"phi\":[11,12,14,15,16,1,11,12,14,15,16,1,3,4,5,6,7,9,3,4,5,6,7,9]}}\r\n", 
"\r\n{\"set\":{\"irfcm\":3,\"phi\":[10,11,13,14,15,1,10,11,13,14,15,1,2,3,5,6,8,9,2,3,5,6,8,9]}}\r\n"
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
    tuff_waitForAck(dev,i); 
    printf("Got ack!\n"); 
  }


  tuff_close(dev); 

  return 0; 











}
