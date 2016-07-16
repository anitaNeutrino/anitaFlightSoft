#include <stdio.h>
#include "tuffLib/tuffLib.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */


int main(int argc, char **argv)
{
  unsigned int irfcm;
  unsigned int channel;
  unsigned int notch;
  argc--;
  argv++;

  // need port, irfcm, channel, notch
  if (argc < 4) {
    exit(-1);
  }  

  tuff_dev_t * dev  = tuff_open(argv[0]); 

  if (!dev) {perror(argv[0]); exit(-1); }

  irfcm = strtoul(argv[1], NULL, 0);
  channel = strtoul(argv[2], NULL, 0);
  notch = strtoul(argv[3], NULL, 0);

  if (notch > 7) {
    printf("notch is a 3-bit bitmask: range from 0-7\n\r");
    exit(-1);
  }

  tuff_setChannelNotches(dev, irfcm, channel, notch);
  // wait for on ack
  tuff_waitForAck(dev, irfcm);
  // wait for off ack
  tuff_waitForAck(dev, irfcm);

  tuff_close(dev); 
  return 0; 
}
