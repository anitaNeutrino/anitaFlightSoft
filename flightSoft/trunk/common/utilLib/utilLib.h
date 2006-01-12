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
unsigned short getDiskSpace(char *dirName);
unsigned short countFilesInDir(char *dirName);

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
int writeGpsPat(GpsAdu5PatStruct_t *patPtr, char *filename);
int writeGpsVtg(GpsAdu5VtgStruct_t *vtgPtr, char *filename);
int writeGpsPos(GpsG12PosStruct_t *posPtr, char *filename);
int writeGpsAdu5Sat(GpsAdu5SatStruct_t *satPtr, char *filename);
int writeGpsG12Sat(GpsG12SatStruct_t *satPtr, char *filename);
int writeGpsTTT(GpsSubTime_t *tttPtr, char *filename);
int writeHk(HkDataStruct_t *hkPtr, char *filename);
int writeCmdEcho(CommandEcho_t *echoPtr, char *filename);
int writeMonitor(MonitorStruct_t *monitorPtr, char *filename);


// Signal handling routines

// Signal handling stuff
// this variable is used when catching signals 
ProgramStateCode currentState; 
void sigUsr1Handler(int sig); 
void sigUsr2Handler(int sig);
void writePidFile(char *fileName);

#endif /* UTILLIB_H */
