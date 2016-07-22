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

/* int fillDefaultBFileHeader(char *inputStream, int length){//, RawAdu5BFileHeader_t *hdrPtr){ */

/*   //  char inputStream[100]; */

/*   //  memcpy(hdrPtr, inputStream, sizeof *hdrPtr); */

/*   return 0; */

/* } */


int fillRawMBNStruct(char *inputStream, int length, RawAdu5MBNStruct_t *mbnPtr){

  if (length!=(sizeof *mbnPtr)) return -1;
  
  memcpy(mbnPtr, inputStream, sizeof *mbnPtr);

  return 0;
}

int fillRawPBNStruct(char *inputStream, int length, RawAdu5PBNStruct_t *mbnPtr){

  if (length!=(sizeof *mbnPtr)) return -1;
  
  memcpy(mbnPtr, inputStream, sizeof *mbnPtr);

  return 0;
}

int fillRawSNVStruct(char *inputStream, int length, RawAdu5SNVStruct_t *mbnPtr){

  if (length!=(sizeof *mbnPtr)) return -1;
  
  memcpy(mbnPtr, inputStream, sizeof *mbnPtr);

  return 0;
}

int fillRawATTStruct(char *inputStream, int length, RawAdu5ATTStruct_t *mbnPtr){

  if (length!=(sizeof *mbnPtr)) return -1;
  
  memcpy(mbnPtr, inputStream, sizeof *mbnPtr);

  return 0;
}

unsigned short calculateSumOfWords(char *inputStream, int numBytes)
{
  unsigned short checksum=0;
  int i;
  for(i=0;i<numBytes;i++) {
    checksum+=(unsigned short)inputStream[i];
  }
  return checksum;
}



unsigned char calculateBytewiseXOR(char *inputStream, int numBytes)
{
  unsigned char checksum=0;
  int i;
  for(i=0;i<numBytes;i++) {
    checksum^=inputStream[i];
  }
  return checksum;
}
