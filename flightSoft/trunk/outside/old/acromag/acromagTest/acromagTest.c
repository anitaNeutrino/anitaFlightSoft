#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "carrier/apc8620.h"

int nCarriers = 0;
int *carrierHandles;
void acromag_readid(unsigned short *id, char slot);
int acromag_setup();
int acromag_scan();
void parseID(unsigned short *id);
int acromag_close();

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      printf("acromagTest:\n");
      printf("\tusage: acromagTest devname [devname2...]\n");
      printf("\t       where devname is the device name of the\n");
      printf("\t       Acromag carrier (/dev/apc8620, etc)\n");
      exit(1);
    }
  argc--;
  argv++;
  nCarriers = argc;
  if (acromag_setup(argv, nCarriers)) exit(1);
  if (acromag_scan()) exit(2);
  acromag_close();
  exit(0);
}

int acromag_close()
{
  int i;
  printf("acromag_close():\n");
  for (i=0;i<nCarriers;i++)
    {
      if (CarrierClose(carrierHandles[i]) != S_OK) {
	printf("\t Unable to close carrier.\n");
      }
    }
  return 0;
}

int acromag_setup(char **devnames, int ndev)
{
  int i;

  printf("acromag_setup():\n");
  if (InitCarrierLib() != S_OK) {
    printf("\tCarrier library failure\n");
    return 1;
  }

  carrierHandles = (int *) malloc(sizeof(int)*ndev);
  for (i=0;i<ndev;i++)
    {
      if (CarrierOpenDev(0, carrierHandles+i, devnames[i]) != S_OK) {
	printf("\tUnable to Open instance of carrier.\n");
	return 2;
      }
      if (CarrierInitialize(carrierHandles[i]) != S_OK) {
	printf("\tUnable to initialize the carrier.\n");
	return 3;
      }
    }
  return 0;
}

int acromag_scan()
{
  unsigned short id[16];

  printf("acromag_scan():\n");
  memset(id, 0, sizeof(unsigned short)*16);
  acromag_readid(id, 'A');
  memset(id, 0, sizeof(unsigned short)*16);
  acromag_readid(id, 'B');
  memset(id, 0, sizeof(unsigned short)*16);
  acromag_readid(id, 'C');
  memset(id, 0, sizeof(unsigned short)*16);
  acromag_readid(id, 'D');
  return 0;
}

void acromag_readid(unsigned short *id, char slot)
{
  ACRO_CSTATUS stat;
  int i;
  
  for (i=0;i<nCarriers;i++)
    {      
      stat = ReadIpackID(carrierHandles[i], slot, id, 16);
      if (stat != S_OK)
	{
	  printf("\tRead failure for channel %c\n", slot);
	}
      else
	{
	  printf("\tID readout for channel %c in carrier %d:\n", slot, i);
	  parseID(id);
	}
    }
}

void parseID(unsigned short *id)
{
  char ipac_string[5];
  unsigned char vendor, board;

  ipac_string[0] = id[0] & 0xFF;
  ipac_string[1] = id[1] & 0xFF;
  ipac_string[2] = id[2] & 0xFF;
  ipac_string[3] = id[3] & 0xFF;
  ipac_string[4] = 0x0;
  if (!strcmp(ipac_string,"IPAC"))
    printf("Instrument Pack\t");
  else
    printf("Unknown Type (%x%x%x%x)\t", ipac_string[0],ipac_string[1],ipac_string[2],ipac_string[3]);
  
  id += 4;
  vendor = *id++ & 0xFF;
  board = *id++ & 0xFF;
  
  if (vendor == 0xA3)
    printf("Acromag\t");
  else
    printf("Unknown Vendor (%x)\t", vendor);
  if (board == 0x08)
    printf("IP470 Digital Out\n");
  else if (board == 0x32)
    printf("IP320 Analog In\n");
  else
    printf("Unknown Board (%d)\n", board);
}
