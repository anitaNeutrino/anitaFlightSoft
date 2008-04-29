/*  \file Acqd.h
    \brief Header for Acqd
    It's a header file what are you expecting? 
    July 2005 rjn@mps.ohio-state.edu
*/

#ifndef ACQD_H
#define ACQD_H

#include "AcqdDefs.h"
      
//Forward Declarations
int initializeDevices(PlxDevObject_t *surfHandles, PlxDevObject_t *turfioHandle, PlxDevKey_t *surfLoc, PlxDevKey_t *turfioLoc);
void clearDevices(PlxDevObject_t *surfHandles, PlxDevObject_t *turfioHandle);
PlxStatus_t setSurfControl(PlxDevObject_t *surfHandle, SurfControlAction_t action);
PlxStatus_t setTurfControl(PlxDevObject_t *turfioHandle, TurfControlAction_t action);
char *surfControlActionAsString(SurfControlAction_t action);
char *turfControlActionAsString(TurfControlAction_t action);
int readConfigFile();
int init_param(int argn, char **argv,  int *n, unsigned short *dacVal);
void writeSurfData(char *directory, unsigned short *wv_data,unsigned long evNum);
void writeTurf(char *directory, TurfioStruct_t *data_turf,unsigned long evNum);
PlxStatus_t readPlxDataWord(PlxDevObject_t *handle, unsigned int *dataWord);
PlxStatus_t readPlxDataShort(PlxDevObject_t *handle, unsigned short *dataWord);
void calculateStatistics();
int getEventNumber();
void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr);
AcqdErrorCode_t readSurfEventData(PlxDevObject_t *surfHandles);
AcqdErrorCode_t readSurfEventDataVer2(PlxDevObject_t *surfHandles);
AcqdErrorCode_t readSurfEventDataVer3(PlxDevObject_t *surfHandles);
AcqdErrorCode_t readSurfHkData(PlxDevObject_t *surfHandles);
AcqdErrorCode_t readTurfEventDataVer2(PlxDevObject_t *turfioHandle);
AcqdErrorCode_t readTurfEventDataVer3(PlxDevObject_t *turfioHandle);
//PlxStatus_t setTurfTriggerMode(PlxDevObject_t turfioHandle, TriggerMode_t trigMode);
void setDACThresholds(PlxDevObject_t *surfHandles);
PlxStatus_t setBarMap(PlxDevObject_t *surfHandles, PlxDevObject_t *turfioHandle);
PlxStatus_t unsetBarMap(PlxDevObject_t *surfHandles,PlxDevObject_t *turfioHandle);
PlxStatus_t writeAntTrigMask(PlxDevObject_t *turfioHandle);
void setGloablDACThreshold(PlxDevObject_t *surfHandles, unsigned short threshold);
//PlxStatus_t blockReadPlxData(PlxDevObject_t *handle, unsigned short *dataArray, int numBytes);
int updateThresholdsUsingPID();
int writeSurfHousekeeping(int dataOrTelem);
int writeTurfHousekeeping(int dataOrTelem);
void makeSubAltDir();
//int bufferedTurfHkWrite(TurfRateStruct_t *turfPtr, char *baseDir);
void prepWriterStructs();
int getChanIndex(int surf, int chan);
void myUsleep(int usec);


	
	


#endif //ACQD_H
