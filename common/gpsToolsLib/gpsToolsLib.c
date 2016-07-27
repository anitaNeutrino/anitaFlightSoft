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

void fillDefaultBFileHeader(RawAdu5BFileHeader_t *hdrPtr){

  sprintf(hdrPtr->version, "Version:"); 
  hdrPtr->raw_version=3;
  sprintf(hdrPtr->rcvr_type,"ADU5-0"); 
  sprintf(hdrPtr->chan_ver,"YG04"); 
  sprintf(hdrPtr->nav_ver,"____"); 
  hdrPtr->capability=1;
  hdrPtr->wb_start=0;
  hdrPtr->num_obs_type=1;

}


int fillRawMBNStruct(char *inputStream, int length, RawAdu5MBNStruct_t *mbnPtr){

  if (length!=sizeof(RawAdu5MBNStruct_t)) return -1;
  
  memcpy(mbnPtr, inputStream, sizeof(RawAdu5MBNStruct_t));

  return 0;
}

int fillRawPBNStruct(char *inputStream, int length, RawAdu5PBNStruct_t *mbnPtr){

  if (length!=sizeof(RawAdu5PBNStruct_t)) return -1;
  
  memcpy(mbnPtr, inputStream, sizeof(RawAdu5PBNStruct_t));

  return 0;
}

int fillRawSNVStruct(char *inputStream, int length, RawAdu5SNVStruct_t *mbnPtr){

  if (length!=sizeof(RawAdu5SNVStruct_t)) return -1;
  
  memcpy(mbnPtr, inputStream, sizeof(RawAdu5SNVStruct_t ));

  return 0;
}

int fillRawATTStruct(char *inputStream, int length, RawAdu5ATTStruct_t *mbnPtr){

  if (length!=sizeof(RawAdu5ATTStruct_t )) return -1;
  
  memcpy(mbnPtr, inputStream, sizeof(RawAdu5ATTStruct_t ));

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
