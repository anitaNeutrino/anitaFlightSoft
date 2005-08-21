/*! \file socketLib.h
    \brief Utility library, full of (hopefully) useful stuff
    
    Some functions that do some useful things
    October 2004  rjn@mps.ohio-state.edu
*/


#ifndef UTILLIB_H
#define UTILLIB_H

typedef enum {
    PROG_STATE_INIT=0,
    PROG_STATE_RUN=1,
    PROG_STATE_TERMINATE
} ProgramStateCode;


/* Includes */
#include <dirent.h>
#include "anitaStructures.h"
#include <time.h>

void makeDirectories(char *theTmpDir);
int is_dir(const char *path);
int makeLink(const char *theFile, const char *theLinkDir);
int moveFile(const char *theFile, const char *theDir);
int copyFile(const char *theFile, const char *theDir);
int removeFile(const char *theFile);
int filterHeaders(const struct dirent *dir);
int filterOnDats(const struct dirent *dir);
int getListofLinks(const char *theEventLinkDir, struct dirent ***namelist);

/* Time stuff */
void addDay(struct tm *timeinfo);


/* IO routines for inter-process communication */
int fillHeader(AnitaEventHeader_t *theEventHdPtr, char *filename);
int fillBody(AnitaEventBody_t *theEventBodyPtr, char *filename);
int fillGpsStruct(GpsSubTime_t *theGpsStruct, char *filename);
int fillCalibStruct(CalibStruct_t *theStruct, char *filename);

int writeHeader(AnitaEventHeader_t *hdPtr, char *filename);
int writeBody(AnitaEventBody_t *bodyPtr, char *filename);
int writeWaveformPacket(WaveformPacket_t *wavePtr, char *filename);
int writeSurfPacket(SurfPacket_t *surfPtr, char *filename);
int writeGPSPat(GpsPatStruct_t *patPtr, char *filename);
int writeGPSSat(GpsSatStruct_t *satPtr, char *filename);
int writeGPSTTT(GpsSubTime_t *tttPtr, char *filename);
int writeHk(HkDataStruct_t *hkPtr, char *filename);


// Signal handling routines

// Signal handling stuff
// this variable is used when catching signals 
ProgramStateCode currentState; 
void sigIntHandler(int sig); 
void sigTermHandler(int sig);
void writePidFile(char *fileName);

#endif /* UTILLIB_H */
