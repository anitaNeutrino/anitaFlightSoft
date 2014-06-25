#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "turfioDriver_ioctl.h"

unsigned int readTurfioReg(unsigned int tiofd, unsigned int address);
void setTurfioReg(unsigned int tiofd, unsigned int address,
		  unsigned int value);

int main(int argc, char **argv) {
  unsigned int value;
  int tiofd;
  int i;
  argc--;argv++;
  tiofd = open("/dev/turfio", O_RDWR);

  setTurfioReg(tiofd, 262, 1); // set TURF_BANK to Bank 1

  value = readTurfioReg(tiofd, 0x20);
  printf("L1TOP: 00: %5.5d 01: %5.5d ",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x21);
  printf("02: %5.5d 03: %5.5d\n",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x22);
  printf("L1TOP: 04: %5.5d 05: %5.5d ",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x23);
  printf("06: %5.5d 07: %5.5d\n",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x24);
  printf("L1TOP: 08: %5.5d 09: %5.5d ",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x25);
  printf("10: %5.5d 11: %5.5d\n",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x26);
  printf("L1TOP: 12: %5.5d 13: %5.5d ",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x27);
  printf("14: %5.5d 15: %5.5d\n",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);


  value = readTurfioReg(tiofd, 0x28);
  printf("L1BOT: 00: %5.5d 01: %5.5d ",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x29);
  printf("02: %5.5d 03: %5.5d\n",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x2A);
  printf("L1BOT: 04: %5.5d 05: %5.5d ",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x2B);
  printf("06: %5.5d 07: %5.5d\n",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x2C);
  printf("L1BOT: 08: %5.5d 09: %5.5d ",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x2D);
  printf("10: %5.5d 11: %5.5d\n",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x2E);
  printf("L1BOT: 12: %5.5d 13: %5.5d ",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  value = readTurfioReg(tiofd, 0x2F);
  printf("14: %5.5d 15: %5.5d\n",
	 (value & 0xFFFF), (value & 0xFFFF0000)>>16);

  setTurfioReg(tiofd, 262, 2); // set to bank 2
  value = readTurfioReg(tiofd, 0x3);
  printf("C3PO_250: %u\n", value);
  value = readTurfioReg(tiofd, 0x4);
  printf("C3PO_200: %u\n", value);
  value = readTurfioReg(tiofd, 0x5);
  printf("C3PO_033: %u\n", value);
  value = readTurfioReg(tiofd, 0x6);
  printf("DEAD_CNT: %u   PPS_CNT: %u\n", value & 0xFFFF,
	 (value & 0xFFFF0000)>>16);

  int regId=0;
  for(regId=0;regId<64;regId++) {
    value = readTurfioReg(tiofd, regId);
    printf("regId %#x : %5.5d  -- %5.5d\n",regId, (value & 0xFFFF), (value & 0xFFFF0000)>>16);
  }
	 


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
