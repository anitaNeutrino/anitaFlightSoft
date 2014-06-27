#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "turfioDriver_ioctl.h"
#include "AcqdDefs.h"

unsigned int readTurfioReg(unsigned int tiofd, unsigned int address);
void setTurfioReg(unsigned int tiofd, unsigned int address,
		  unsigned int value);

int main(int argc, char **argv) {
  unsigned int value;
  int tiofd;
  int i;
  argc--;argv++;
  tiofd = open("/dev/turfio", O_RDWR);

  setTurfioReg(tiofd,TurfioRegTurfBank, TurfBankControl); // set TURF_BANK to Bank 3

  value = readTurfioReg(tiofd,TurfRegControlFrut);
  printf("TURF: %c %c %c %c\n",value&0xFF,(value&0xFF00)>>8,(value&0xFF0000)>>16,(value&0xFF000000)>>24);
  value = readTurfioReg(tiofd,TurfRegControlVersion);
  printf("Version: %x\n",value);
  printf("Version: %d %x %d/%d\n",value&0xFF,(value&0xFF00)>>8,(value&0xFF0000)>>16,(value&0xFF000000)>>24);

  value = readTurfioReg(tiofd,TurfioRegId);
  printf("TURFIO: %c %c %c %c\n",value&0xFF,(value&0xFF00)>>8,(value&0xFF0000)>>16,(value&0xFF000000)>>24);
  value = readTurfioReg(tiofd,TurfioRegVersion);
  printf("Version: %x\n",value);



  close(tiofd);
}

unsigned int readTurfioReg(unsigned int tiofd, unsigned int address) {
  struct turfioDriverRead req;
  req.address = address;
  if (ioctl(tiofd, TURFIO_IOCREAD, &req)) {
    perror("ioctl");
    exit(1);
  }
  return req.value;
}
  
void setTurfioReg(unsigned int tiofd, unsigned int address,
		  unsigned int value) {
  struct turfioDriverRead req;
  req.address = address; 
  req.value = value;

  if (ioctl(tiofd, TURFIO_IOCWRITE, &req)) {
    perror("ioctl");
    exit(1);
  }
}
