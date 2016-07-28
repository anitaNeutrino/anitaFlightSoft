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
#include <endian.h>
#include <stdint.h>
#include <stddef.h>

#include "gpsToolsLib/gpsToolsLib.h"


static void swapDoubleBytes(double *toswap)
{
  uint64_t as_int = *((uint64_t*) toswap); 
  uint64_t byte_swapped = be64toh(as_int); 
  double as_double = *( (double*) &byte_swapped); 
  *toswap = as_double; 
}

static void swapFloatBytes(float *toswap)
{
  uint32_t as_int = *((uint32_t*) toswap); 
  uint32_t byte_swapped = be32toh(as_int); 
  float as_double = *( (float*) &byte_swapped); 
  *toswap = as_double; 
}


static void swapIntBytes(int * toswap) 
{
  *toswap = be32toh(*toswap); 
}




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
  if (calculateBytewiseXOR( ((char*)&mbnPtr->sequence_tag), offsetof(RawAdu5MBNStruct_t,checkSum)-offsetof(RawAdu5MBNStruct_t,sequence_tag) )!=mbnPtr->checkSum ){ 
    printf("Check sum problem \n");
    return -1;
  } 
  swapDoubleBytes(&mbnPtr->full_phase); 
  swapDoubleBytes(&mbnPtr->raw_range); 
  swapIntBytes(&mbnPtr->doppler); 
  swapIntBytes(&mbnPtr->smoothing); 
  return 0;
}

int fillRawPBNStruct(char *inputStream, int length, RawAdu5PBNStruct_t *pbnPtr){

  if (length!=sizeof(RawAdu5PBNStruct_t)) return -1;
  
  memcpy(pbnPtr, inputStream, sizeof(RawAdu5PBNStruct_t));
  
  /* if (calculateSumOfWords( ((char*)&pbnPtr->pben_time), offsetof(RawAdu5PBNStruct_t,checkSum)-offsetof(RawAdu5PBNStruct_t,pben_time) )!=pbnPtr->checkSum ){  */
  /*   printf("Check sum problem \n"); */
  /*   return -1; */
  /* }  */
  swapIntBytes(&pbnPtr->pben_time); 
  swapDoubleBytes(&pbnPtr->navx); 
  swapDoubleBytes(&pbnPtr->navy); 
  swapDoubleBytes(&pbnPtr->navz); 
  swapFloatBytes(&pbnPtr->navt); 
  swapFloatBytes(&pbnPtr->navxdot); 
  swapFloatBytes(&pbnPtr->navydot); 
  swapFloatBytes(&pbnPtr->navzdot); 
  swapFloatBytes(&pbnPtr->navtdot); 

  return 0;
}

int fillRawSNVStruct(char *inputStream, int length, RawAdu5SNVStruct_t *snvPtr){

  if (length!=sizeof(RawAdu5SNVStruct_t)) return -1;
  
  memcpy(snvPtr, inputStream, sizeof(RawAdu5SNVStruct_t ));

  /* if (calculateSumOfWords( ((char*)&snvPtr->weekNumber), offsetof(RawAdu5SNVStruct_t,checkSum)-offsetof(RawAdu5SNVStruct_t,weekNumber) )!=snvPtr->checkSum ){  */
  /*   printf("Check sum problem \n"); */
  /*   return -1; */
  /* }  */
  
  swapIntBytes(&snvPtr->secondsInWeek);
  swapFloatBytes(&snvPtr->groupDelay);
  swapIntBytes(&snvPtr->aodc);
  swapIntBytes(&snvPtr->toc);
  swapFloatBytes(&snvPtr->af2);
  swapFloatBytes(&snvPtr->af1);
  swapFloatBytes(&snvPtr->af0);
  swapIntBytes(&snvPtr->aode);
  swapFloatBytes(&snvPtr->deltaN);
  swapDoubleBytes(&snvPtr->m0);
  swapDoubleBytes(&snvPtr->eccentricity);
  swapDoubleBytes(&snvPtr->roota);
  swapIntBytes(&snvPtr->toe);
  swapFloatBytes(&snvPtr->cic);
  swapFloatBytes(&snvPtr->crc);
  swapFloatBytes(&snvPtr->cis);
  swapFloatBytes(&snvPtr->crs);
  swapFloatBytes(&snvPtr->cuc);
  swapFloatBytes(&snvPtr->cus);
  swapDoubleBytes(&snvPtr->omega0);
  swapDoubleBytes(&snvPtr->omega);
  swapDoubleBytes(&snvPtr->i0);
  swapFloatBytes(&snvPtr->omegadot);
  swapFloatBytes(&snvPtr->idot);

  return 0;
}

int fillRawATTStruct(char *inputStream, int length, RawAdu5ATTStruct_t *attPtr){

  if (length!=sizeof(RawAdu5ATTStruct_t )) return -1;
  
  memcpy(attPtr, inputStream, sizeof(RawAdu5ATTStruct_t ));

  /* if (calculateSumOfWords( ((char*)&attPtr->head), offsetof(RawAdu5ATTStruct_t,checkSum)-offsetof(RawAdu5ATTStruct_t,head) )!=attPtr->checkSum ){  */
  /*   printf("Check sum problem \n"); */
  /*   return -1; */
  /* }  */
  
  swapDoubleBytes(&attPtr->head);
  swapDoubleBytes(&attPtr->roll);
  swapDoubleBytes(&attPtr->pitch);
  swapDoubleBytes(&attPtr->brms);
  swapDoubleBytes(&attPtr->mrms);
  swapIntBytes(&attPtr->timeOfWeek);

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
