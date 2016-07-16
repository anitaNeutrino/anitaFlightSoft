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

  // need port, irfcm, channel, notch, cap
  if (argc < 5) {
    exit(-1);
  }  

  tuff_dev_t * dev = tuff_open(argv[0]); 
  if (!dev) { perror(argv[0]); return -1; }

  irfcm = strtoul(argv[1], NULL, 0);
  channel = strtoul(argv[2], NULL, 0);
  notch = strtoul(argv[3], NULL, 0);
  cap = strtoul(argv[4], NULL, 0);
  
  if (notch > 2) {
    printf("notch must be from 0-2\n");
    exit(-1);
  }
  if (channel > 23) {
    printf("channel must be from 0-23\n");
    exit(-1);
  }
  if (cap > 31) {
    printf("cap must be from 0-31\n");
    exit(-1);
  }
  
  tuff_setCap(dev, irfcm, channel, notch, cap);

  // wait for cap ack
  tuff_waitForAck(dev, irfcm);
  printf("got ack; cap set complete\n");


  tuff_close(dev); 

  return 0; 
}
