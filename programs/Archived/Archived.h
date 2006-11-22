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
    ARCHIVE_PEDSUBBED=2,
    ARCHIVE_ENCODED=3,
    ARCHIVE_PEDSUBBED_ENCODED=4
} ArchivedDataType_t;


void writeOutputToDisk(int numBytes);
void writeOutputForTelem(int numBytes);
void processEvent();
void checkEvents();
int readConfigFile();
void prepWriterStructs();
char *getFilePrefix(ArchivedDataType_t dataType);
int shouldWeThrowAway(int pri);


#endif //ARCHIVED_H
