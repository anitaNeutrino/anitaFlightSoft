#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "surfDriver_ioctl.h"

inline unsigned long ReadTSC()
{
    unsigned long tsc;
    asm("rdtsc":"=A"(tsc));
    return tsc;
}

int main(int argc, char **argv) {
  unsigned long before, after;
  
  unsigned int ibuffer[72];
  unsigned int obuffer[34];
  int surfFD;
  int count;
  int i;
  int startval;
  argc--;
  argv++;
  if (!argc) exit(1);
  startval = atoi(*argv);
  printf("starting at %d\n", startval);

  surfFD = open("/dev/surf-0", O_RDWR);
  if (surfFD < 0) {
    printf("can't open surf\n");
    exit(1);
  }

  ioctl(surfFD, SURF_IOCCLEARALL);
  for (i=0;i<32;i++) {
    obuffer[i] = (i<<16) + (startval+i);
  }
  obuffer[32] = (32 << 16) + 0x0;
  obuffer[33] = (33 << 16) + 0xFFFF;
  count = write(surfFD, obuffer, 34*sizeof(int));

  usleep(30000);

  ioctl(surfFD, SURF_IOCCLEARHK);
  ioctl(surfFD, SURF_IOCHK);
  
  before = ReadTSC();
  count = read(surfFD, ibuffer, 72*sizeof(int));
  after = ReadTSC();
  printf("read %d bytes in %d cycles\n", count, after-before);
  for (i=0;i<72;i+=8) {
    printf("%8.8X ", ibuffer[i]);
    printf("%8.8X ", ibuffer[i+1]);
    printf("%8.8X ", ibuffer[i+2]);
    printf("%8.8X ", ibuffer[i+3]);
    printf("%8.8X ", ibuffer[i+4]);
    printf("%8.8X ", ibuffer[i+5]);
    printf("%8.8X ", ibuffer[i+6]);
    printf("%8.8X\n", ibuffer[i+7]);
  }

  close(surfFD);
  exit(0);
}
