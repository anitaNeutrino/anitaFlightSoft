/*! \file Archived.h
  \brief The Archived program that writes Events to disk
    
  May 2006 rjn@mps.ohio-state.edu
*/
#ifndef ARCHIVED_H
#define ARCHIVED_H

#include "includes/anitaFlight.h"
#include "includes/anitaStructures.h"


typedef enum {
    ARCHIVE_RAW=1,
    ARCHIVE_PEDSUBBED,
    ARCHIVE_ENCODED,
    ARCHIVE_PEDSUBBED_ENCODED
} ArchivedDataType_t;


void writeOutputToDisk(int numBytes);
void writeOutputForTelem(int numBytes);
void processEvent();
void checkEvents();
int readConfigFile();
void prepWriterStructs();
char *getFilePrefix(ArchivedDataType_t dataType);

#endif //ARCHIVED_H
