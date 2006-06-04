/*! \file utilLib.h
    \brief Utility library, full of (hopefully) useful stuff
    
    Some functions that do some useful things
    October 2004  rjn@mps.ohio-state.edu
*/


#ifndef UTILLIB_H
#define UTILLIB_H

/* Includes */
#include <dirent.h>
#include "anitaStructures.h"
#include <time.h>
#include <zlib.h>
#include <stdio.h>


typedef enum {
    PROG_STATE_INIT=0,
    PROG_STATE_RUN=1,
    PROG_STATE_TERMINATE
} ProgramStateCode;

typedef struct {
    FILE *currentFilePtr;
    int writeCount;
    int fileCount;
    int dirCount;
    int maxSubDirsPerDir;
    int maxFilesPerDir;
    int maxWritesPerFile;
    char baseDirname[FILENAME_MAX];
    char filePrefix[FILENAME_MAX];
    char currentFileName[FILENAME_MAX];
    char currentDirName[FILENAME_MAX];
    char currentSubDirName[FILENAME_MAX];
} AnitaWriterStruct_t;

typedef struct {
    gzFile currentEventFilePtr;
    gzFile currentHeaderFilePtr;
    int writeCount;
    int fileCount;
    int dirCount;
    int maxSubDirsPerDir;
    int maxFilesPerDir;
    int maxWritesPerFile;
    char baseDirname[FILENAME_MAX];
    char currentEventFileName[FILENAME_MAX];
    char currentHeaderFileName[FILENAME_MAX];
    char currentDirName[FILENAME_MAX];
    char currentSubDirName[FILENAME_MAX];
} AnitaEventWriterStruct_t;

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
int fillHeaderWithThisEvent(AnitaEventHeader_t *hdPtr, char *filename,
			    unsigned long eventNumber);
int fillBody(AnitaEventBody_t *theEventBodyPtr, char *filename);
int fillGpsStruct(GpsSubTime_t *theGpsStruct, char *filename);
int fillCalibStruct(CalibStruct_t *theStruct, char *filename);
int fillCommand(CommandStruct_t *theStruct, char *filename);
int readEncodedEventFromFile(char *buffer, char *filename,
			     unsigned long eventNumber);

int writeHeader(AnitaEventHeader_t *hdPtr, char *filename);
int writeBody(AnitaEventBody_t *bodyPtr, char *filename);
int writeZippedBody(AnitaEventBody_t *bodyPtr, char *filename);
int writeWaveformPacket(RawWaveformPacket_t *wavePtr, char *filename);
int writeSurfPacket(RawSurfPacket_t *surfPtr, char *filename);
int writeGpsPat(GpsAdu5PatStruct_t *patPtr, char *filename);
int writeGpsVtg(GpsAdu5VtgStruct_t *vtgPtr, char *filename);
int writeGpsPos(GpsG12PosStruct_t *posPtr, char *filename);
int writeGpsAdu5Sat(GpsAdu5SatStruct_t *satPtr, char *filename);
int writeGpsG12Sat(GpsG12SatStruct_t *satPtr, char *filename);
int writeGpsTtt(GpsSubTime_t *tttPtr, char *filename);
int writeHk(HkDataStruct_t *hkPtr, char *filename);
int writeSurfHk(FullSurfHkStruct_t *hkPtr, char *filename);
int writeCmdEcho(CommandEcho_t *echoPtr, char *filename);
int writeCmd(CommandStruct_t *cmdPtr, char *filename);
int writeTurfRate(TurfRateStruct_t *turfPtr, char *filename);
int writeMonitor(MonitorStruct_t *monitorPtr, char *filename);
int writeCalibStatus(CalibStruct_t *calibPtr, char *filename);

int genericReadOfFile(char *buffer, char *filename, int maxBytes);


char *getCurrentHkDir(char *baseHkDir,unsigned long unixTime);
char *getCurrentHkFilename(char *currentDir, char *prefix, 
			   unsigned long unixTime);



int zippedSingleWrite(char *buffer, char *filename, int numBytes);
int normalSingleWrite(char *buffer, char *filename, int numBytes);


int cleverHkWrite(char *buffer, int numBytes,unsigned long unixTime, AnitaWriterStruct_t *awsPtr);
int cleverRawEventWrite(AnitaEventBody_t *bdPtr,AnitaEventHeader_t *hdPtr, AnitaEventWriterStruct_t *awsPtr);
int cleverEncEventWrite(char *outputBuffer, int numBytes,AnitaEventHeader_t *hdPtr, AnitaEventWriterStruct_t *awsPtr);


// Signal handling routines

// Signal handling stuff
// this variable is used when catching signals 
ProgramStateCode currentState; 
void sigUsr1Handler(int sig); 
void sigUsr2Handler(int sig);
void writePidFile(char *fileName);

//Generic Header Stuff
void fillGenericHeader(void *thePtr, PacketCode_t code, unsigned short numBytes);
int checkPacket(void *thePtr);
unsigned long simpleLongCrc(unsigned long *p, unsigned long n);

char *packetCodeAsString(PacketCode_t code);


#endif /* UTILLIB_H */
