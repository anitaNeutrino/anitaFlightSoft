#include <stdio.h>
#include "tuffLib/tuffLib.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */


int main(int argc, char **argv)
{
  unsigned int notch;
  unsigned int start; 
  unsigned int stop;
  argc--;
  argv++;

  // need port, notch start stop 
  if (argc < 3) {
    exit(-1);
  }  

  tuff_dev_t * dev  = tuff_open(argv[0]); 

  if (!dev) {perror(argv[0]); exit(-1); }

  notch = strtoul(argv[1], NULL, 0);
  start = strtoul(argv[2], NULL,0); 
  stop = strtoul(argv[3], NULL,0); 

  if (notch > 2) {
    printf("notch range from 0-2\n\r");
    exit(-1);
  }

  tuff_setNotchRange(dev, notch, start, stop);

  tuff_close(dev); 
  return 0; 
}
