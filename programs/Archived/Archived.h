/*! \file Archived.h
  \brief The Archived program that writes Events to disk
    
  May 2006 rjn@mps.ohio-state.edu
*/
#ifndef ARCHIVED_H
#define ARCHIVED_H

#include "includes/anitaFlight.h"
#include "includes/anitaStructures.h"
#include "utilLib/utilLib.h"


typedef enum {
    ARCHIVE_RAW=1,
    ARCHIVE_PEDSUBBED=2,
    ARCHIVE_ENCODED=3,
    ARCHIVE_PEDSUBBED_ENCODED=4
} ArchivedDataType_t;


void writeOutputToDisk(int numBytes, unsigned char *outputBuffer, AnitaEventHeader_t *hdPtr, AnitaEventWriterStruct_t *eventWriterPtr) ;


void writeOutputForTelem(int numBytes,unsigned char *outputBuffer, AnitaEventHeader_t *hdPtr, AnitaEventWriterStruct_t *eventWriterPtr);
void processEvent();
void checkEvents();
int readConfigFile();
void prepIndexWriter();
void prepEventWriterStruct(AnitaEventWriterStruct_t *eventWriterPtr);
char *getFilePrefix(ArchivedDataType_t dataType);
int shouldWeThrowAway(int pri);


#endif //ARCHIVED_H
