/*  \file AcqdSd.h
    \brief Header for AcqdSd
    It's a header file what are you expecting? 
    April 2008 rjn@hep.ucl.ac.uk
*/

#ifndef ACQDSD_H
#define ACQDSD_H

#include "AcqdSdDefs.h"
      
//Forward Declarations

//Hardware functions
AcqdErrorCode_t initializeDevices(int *numDevPtr);
AcqdErrorCode_t clearDevices();
AcqdErrorCode_t closeDevices();
AcqdErrorCode_t setSurfControl(int surfId, SurfControlAction_t action);
AcqdErrorCode_t setTurfControl(TurfControlAction_t action);
AcqdErrorCode_t readSurfEventData();
AcqdErrorCode_t readSurfHkData();
AcqdErrorCode_t readTurfEventData();
AcqdErrorCode_t setDACThresholds();
AcqdErrorCode_t writeAntTrigMask();
AcqdErrorCode_t setGloablDACThreshold(unsigned short threshold);
AcqdErrorCode_t getSurfStatusFlag(int surfId, SurfStatusFlag_t flag, int *value) ;
AcqdErrorCode_t readTurfGPIOValue(unsigned int *gpioVal);
AcqdErrorCode_t setTurfGPIOValue(unsigned int gpioVal);
AcqdErrorCode_t writeDacValBuffer(int surfId, unsigned int *obuffer) ;


//Configuration functions
int readConfigFile();
int init_param(int argn, char **argv,  int *n, unsigned short *dacVal);

//Misclaneous Functions
char *surfStatusFlagAsString(SurfStatusFlag_t flag);
char *surfControlActionAsString(SurfControlAction_t action);
char *turfControlActionAsString(TurfControlAction_t action);
int updateThresholdsUsingPID();
void calculateStatistics();
int getEventNumber();
int getChanIndex(int surf, int chan);
void myUsleep(int usec);
void fillDacValBufferGlobal(unsigned int obuffer[MAX_SURFS][], unsigned short val);
void fillDacValBuffer(unsigned int obuffer[MAX_SURFS][]);


//Output functions
void writeSurfData(char *directory, unsigned short *wv_data,unsigned long evNum);
void writeTurf(char *directory, TurfioStruct_t *data_turf,unsigned long evNum);
void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr);
int writeSurfHousekeeping(int dataOrTelem);
int writeTurfHousekeeping(int dataOrTelem);
void makeSubAltDir();
void prepWriterStructs();

#endif //ACQD_H
