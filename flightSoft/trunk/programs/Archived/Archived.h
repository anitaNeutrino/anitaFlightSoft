/*! \file Archived.h
  \brief The Archived program that writes Events to disk
    
  May 2006 rjn@mps.ohio-state.edu
*/
#ifndef ARCHIVED_H
#define ARCHIVED_H

#include "anitaFlight.h"
#include "anitaStructures.h"

#define MAX_EVENTS_PER_DIR 1000

void writeOutput(int numBytes);
void processEvent();
void checkEvents();
int readConfigFile();
void prepWriterStructs();

#endif //ARCHIVED_H
