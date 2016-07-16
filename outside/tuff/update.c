#include <stdio.h>
#include "tuffLib/tuffLib.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */

int main(int argc, char **argv)
{
  unsigned int irfcm;
  unsigned int channel;
  unsigned int notch;
  unsigned int cap;
  argc--;
  argv++;

  // need port, irfcm, channel
  if (argc < 3) {
    exit(-1);
  }  

  tuff_dev_t *dev = tuff_open(argv[0]); 
  if (!dev) {perror(argv[0]); exit(-1); }

  irfcm = strtoul(argv[1], NULL, 0);
  channel = strtoul(argv[2], NULL, 0);
  
  if (channel > 23) {
    printf("channel must be from 0-23\n");
    exit(-1);
  }
  
  tuff_updateCaps(dev, irfcm, channel);
  // wait for cap ack
  tuff_waitForAck(dev, irfcm);
  printf("got ack; cap set complete\n");


  tuff_close(dev); 
  return 0; 
}



