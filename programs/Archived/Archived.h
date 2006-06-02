/*! \file Archived.h
  \brief The Archived program that writes Events to disk
    
  May 2006 rjn@mps.ohio-state.edu
*/
#ifndef ARCHIVED_H
#define ARCHIVED_H

#include "anitaFlight.h"
#include "anitaStructures.h"

#define MAX_EVENTS_PER_DIR 1000
#define MAX_WAVE_BUFFER NUM_DIGITZED_CHANNELS*MAX_NUMBER_SAMPLES*4

unsigned short simpleCrcShort(unsigned short *p, unsigned long n);
void writeOutput(int numBytes);
int encodeWaveNone(unsigned char *buffer, SurfChannelFull_t *chanPtr);
int encodeChannel(EncodingType_t encType, SurfChannelFull_t *chanPtr, unsigned char *buffer);
void processEvent();
void checkEvents();
int readConfigFile();
void prepWriterStructs();

#endif //ARCHIVED_H
