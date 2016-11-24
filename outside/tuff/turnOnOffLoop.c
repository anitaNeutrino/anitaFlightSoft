#include "tuffLib/tuffLib.h" 
#include <stdio.h>
#include <unistd.h>


int main(int argc, char **argv)
{

  tuff_dev_t * dev  = tuff_open(argv[1]); 
  FILE * fin = fopen(argv[2], "r"); 
  int sleep_amount = argc > 3 ? atoi(argv[3]) : 10; 
  int antenna; 
  char ring; 
  char pol; 
  int irfcm; 
  int ch; 
  int i; 

  //start by opening turning off everything

  tuff_setNotchRange(dev, 0, 16, 16);
  tuff_setNotchRange(dev, 1, 16, 16);
  tuff_setNotchRange(dev, 2, 16, 16);

  for (i = 0; i < 96; i++) 
  {
    fscanf(fin,"%02d%c%c %d %d", &antenna, &ring,&pol, &irfcm, &ch);
    printf("Found %02d%c%c. Has irfcm %d ch %d\n",antenna,ring,pol,irfcm, ch); 
    tuff_setChannelNotches(dev, irfcm, ch, 7);
    sleep(sleep_amount); 
  }; 

 

  tuff_close(dev); 

  return 0;
}
