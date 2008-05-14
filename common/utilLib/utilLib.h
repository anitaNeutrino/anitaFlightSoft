/*! \file utilLib.h
  \brief Utility library, full of (hopefully) useful stuff
    
  Some functions that do some useful things
  October 2004  rjn@mps.ohio-state.edu
*/


#ifndef UTILLIB_H
#define UTILLIB_H


#ifdef __cplusplus
extern "C" {
#endif

/* Includes */

#include "includes/anitaStructures.h"
#include "includes/anitaCommand.h"
#include <time.h>
#include <zlib.h>
#include <stdio.h>
#include <signal.h>

  //Hack for SIGCLD name change
#ifndef SIGCLD
#define SIGCLD SIGCHLD
#endif

    typedef enum {
	PROG_STATE_INIT=0,
	PROG_STATE_RUN=1,
	PROG_STATE_TERMINATE
    } ProgramStateCode;
          

    typedef struct {
	FILE *currentFilePtr[DISK_TYPES]; //For the 5 disk types
	unsigned int writeBitMask; //1-satablade, 2-satamini,3-usb,4-neobrick,5-pmc
	int writeCount[DISK_TYPES];
	int fileCount[DISK_TYPES];
	int dirCount[DISK_TYPES];
	char relBaseName[FILENAME_MAX];
	char filePrefix[FILENAME_MAX];
	char currentDirName[DISK_TYPES][FILENAME_MAX];
	char currentSubDirName[DISK_TYPES][FILENAME_MAX];
	char currentFileName[DISK_TYPES][FILENAME_MAX];
    } AnitaHkWriterStruct_t;

    typedef struct {
	FILE* currentEventFilePtr[DISK_TYPES]; //For the 5 disk types
	FILE* currentHeaderFilePtr[DISK_TYPES];
	unsigned int writeBitMask; //1-satablade, 2-satamini,3-usb,4-neobrick,5-pmc
	unsigned int satabladeCloneMask;
	unsigned int sataminiCloneMask;
	unsigned int usbCloneMask;
	unsigned int fileEpoch;
	unsigned int gotData;
	unsigned int justHeader;
	char filePrefix[FILENAME_MAX];
	char relBaseName[FILENAME_MAX];
	char currentEventFileName[DISK_TYPES][FILENAME_MAX];
	char currentHeaderFileName[DISK_TYPES][FILENAME_MAX];
	char currentDirName[DISK_TYPES][FILENAME_MAX];
	char currentSubDirName[DISK_TYPES][FILENAME_MAX];
    } AnitaEventWriterStruct_t;

    void makeDirectories(char *theTmpDir);
    int is_dir(const char *path);
    int makeLink(const char *theFile, const char *theLinkDir);
    int moveFile(const char *theFile, const char *theDir);
    int copyFile(const char *theFile, const char *theDir);
    int copyFileToFile(const char *theFile, const char *newFile);
    char *readFile(const char *theFile, unsigned int *numBytes);
    int removeFile(const char *theFile);
  int getRunNumber();
  int getIdMask(ProgramId_t prog);
  char *getPidFile(ProgramId_t prog);
  char *getProgName(ProgramId_t prog);


    unsigned int getDiskSpace(char *dirName);
    unsigned short countFilesInDir(char *dirName);

/* Time stuff */
    void addDay(struct tm *timeinfo);

/* IO routines for inter-process communication */
    int fillHeader(AnitaEventHeader_t *theEventHdPtr, char *filename);
    int fillHeaderWithThisEvent(AnitaEventHeader_t *hdPtr, char *filename,
				unsigned int eventNumber);
    int fillBody(AnitaEventBody_t *theEventBodyPtr, char *filename);
    int fillPedSubbedBody(PedSubbedEventBody_t *theEventBodyPtr, char *filename);
    int fillGpsStruct(GpsSubTime_t *theGpsStruct, char *filename);
    int fillCalibStruct(CalibStruct_t *theStruct, char *filename);
    int fillCommand(CommandStruct_t *theStruct, char *filename);
    int fillUsefulPedStruct(PedestalStruct_t *pedPtr, char *filename);
    int fillLabChipPedstruct(FullLabChipPedStruct_t *pedPtr, char *filename);
    int readEncodedEventFromFile(unsigned char *buffer, char *filename,
				 unsigned int eventNumber);
    int readSingleEncodedEvent(unsigned char *buffer, char *filename);

    int writeGpsdStartStruct(GpsdStartStruct_t *startPtr, char *filename);
    int writeHeader(AnitaEventHeader_t *hdPtr, char *filename);
    int writeBody(AnitaEventBody_t *bodyPtr, char *filename);
    int writePedSubbedBody(PedSubbedEventBody_t *bodyPtr, char *filename);
    int writeZippedBody(AnitaEventBody_t *bodyPtr, char *filename);
    int writeWaveformPacket(RawWaveformPacket_t *wavePtr, char *filename);
    int writeSurfPacket(RawSurfPacket_t *surfPtr, char *filename);
  int writeGpsGga(GpsGgaStruct_t *ggaPtr, char *filename);
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
    int writePedCalcStruct(PedCalcStruct_t *pedPtr, char *filename);
    int writeUsefulPedStruct(PedestalStruct_t *pedPtr, char *filename);
    int writeLabChipPedStruct(FullLabChipPedStruct_t *pedPtr, char *filename);
    int writeSlowRate(SlowRateFull_t *slowPtr, char *filename);
    int writeCommandAndLink(CommandStruct_t *theCmd);

    int genericReadOfFile(unsigned char *buffer, char *filename, int maxBytes);


    char *getCurrentHkDir(char *baseHkDir,unsigned int unixTime);
    char *getCurrentHkFilename(char *currentDir, char *prefix, 
			       unsigned int unixTime);

    int zippedSingleWrite(unsigned char *buffer, char *filename, int numBytes);
    int normalSingleWrite(unsigned char *buffer, char *filename, int numBytes);
    int touchFile(char *filename);
    int checkFileExists(char *filename);

    int cleverHkWrite(unsigned char *buffer, int numBytes,unsigned int unixTime, AnitaHkWriterStruct_t *awsPtr);
    int cleverEventWrite(unsigned char *outputBuffer, int numBytes,AnitaEventHeader_t *hdPtr, AnitaEventWriterStruct_t *awsPtr);
    int closeEventFilesAndTidy(AnitaEventWriterStruct_t *awsPtr);
    int closeHkFilesAndTidy(AnitaHkWriterStruct_t *awsPtr);
    int cleverIndexWriter(IndexEntry_t *indPtr, AnitaHkWriterStruct_t *awsPtr);

//Zipping packets and files
    int zipBuffer(char *input, char *output, unsigned int inputBytes, unsigned int *outputBytes);
    int unzipBuffer(char *input, char *output, unsigned int inputBytes, unsigned int *outputBytes);
    int zipFileInPlace(char *filename);
    int zipFileInPlaceAndClone(char *filename, unsigned int cloneMask,int baseInd);
    int zipBufferedFileAndMove(char *nonBufFilename);
    int zipBufferedFileAndCloneAndMove(char *nonBufFilename,unsigned int cloneMask,int baseInd);
    int makeZippedPacket(char *input, unsigned int numBytes, char *output, unsigned int numBytesOut);
    int unzipZippedPacket(ZippedPacket_t *zipPacket, char *output, unsigned int numBytesOut);
// Signal handling routines

// Signal handling stuff
// this variable is used when catching signals 
#ifndef RYAN_HACK_17
    extern ProgramStateCode currentState; 
#endif
    void sigUsr1Handler(int sig); 
    void sigUsr2Handler(int sig);
    void writePidFile(char *fileName);
    int checkPidFile(char *fileName);


//Generic Header Stuff
    void fillGenericHeader(void *thePtr, PacketCode_t code, unsigned short numBytes);
    int checkPacket(void *thePtr);
    unsigned int simpleIntCrc(unsigned int *p, unsigned int n);


    char *packetCodeAsString(PacketCode_t code);

#ifdef __cplusplus
}
#endif
#endif /* UTILLIB_H */
