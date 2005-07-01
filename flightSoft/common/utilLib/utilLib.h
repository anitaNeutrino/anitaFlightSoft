/*! \file socketLib.h
    \brief Utility library, full of (hopefully) useful stuff
    
    Some functions that do some useful things
    October 2004  rjn@mps.ohio-state.edu
*/

/* Includes */

#ifndef UTILLIB_H
#define UTILLIB_H

#include <dirent.h>
#include "anitaStructures.h"

void makeDirectories(char *theTmpDir);
int is_dir(const char *path);
int makeLink(const char *theFile, const char *theLinkDir);
int moveFile(const char *theFile, const char *theDir);
int removeFile(const char *theFile);
int filterHeaders(const struct dirent *dir);
int filterOnDats(const struct dirent *dir);
int getListofLinks(const char *theEventLinkDir, struct dirent ***namelist);


/* IO routines for inter-process communication */
int fillHeader(anita_event_header *theEventHdPtr, char *filename);
int fillBody(anita_event_body *theEventBodyPtr, char *filename);
int fillGpsStruct(gpsSubTimeStruct *theGpsStruct, char *filename);
int fillCalibStruct(calibStruct *theStruct, char *filename);

int writeHeader(anita_event_header *hdPtr, char *filename);
int writeBody(anita_event_body *bodyPtr, char *filename);

#endif /* UTILLIB_H */
