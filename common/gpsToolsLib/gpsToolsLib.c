/*! \file gpsToolsLib.c
    \brief Some useful functions for playing with the GPS
    
    For now it's just a couple of functions, but it may well grow.
   
     July 2016 r.nichol@ucl.ac.uk
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

#include "gpsToolsLib/gpsToolsLib.h"



unsigned short calculateSumOfWords(char *inputStream, int numBytes)
{
  unsigned short checksum=0;
  for(int i=0;i<numBytes;i++) {
    checksum+=(unsigned short)inputStream[i];
  }
  return checksum;
}



unsigned char calculateBytewiseXOR(char *inputStream, int numBytes)
{
  unsigned char checksum=0;
  for(int i=0;i<numBytes;i++) {
    checksum^=inputStream[i];
  }
  return checksum;
}
