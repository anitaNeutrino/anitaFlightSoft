/*  \file Acqd.h
    \brief Header for Acqd
    It's a header file what are you expecting? 
    April 2008 rjn@hep.ucl.ac.uk
*/

#ifndef ACQDSD_H
#define ACQDSD_H

#include "AcqdDefs.h"
      
//Forward Declarations
struct timeval;


//Hardware functions
AcqdErrorCode_t initializeDevices(int *numDevPtr);
AcqdErrorCode_t clearDevices();
AcqdErrorCode_t closeDevices();
AcqdErrorCode_t setSurfControl(int surfId, SurfControlAction_t action);
AcqdErrorCode_t setTurfControl(TurfControlAction_t action);
AcqdErrorCode_t readSurfEventData();
AcqdErrorCode_t readSurfHkData();
AcqdErrorCode_t readTurfEventDataVer3();
AcqdErrorCode_t readTurfEventDataVer5();
AcqdErrorCode_t setDACThresholds();
AcqdErrorCode_t writeAntTrigMask();
AcqdErrorCode_t setGlobalDACThreshold(unsigned short threshold);
AcqdErrorCode_t getSurfStatusFlag(int surfId, SurfStatusFlag_t flag, int *value) ;
AcqdErrorCode_t readTurfGPIOValue(unsigned int *gpioVal);
AcqdErrorCode_t setTurfGPIOValue(unsigned int gpioVal);
AcqdErrorCode_t writeDacValBuffer(int surfId, unsigned int *obuffer) ;
AcqdErrorCode_t sendClearEvent();
AcqdErrorCode_t readTurfHkData();
AcqdErrorCode_t readTurfioReg(unsigned int address, unsigned int *valPtr);
AcqdErrorCode_t setTurfioReg(unsigned int address,unsigned int value);
AcqdErrorCode_t setTriggerMasks();
AcqdErrorCode_t checkTurfEventReady(int *turfEventReady);

//Special config modes
AcqdErrorCode_t doGlobalThresholdScan();
AcqdErrorCode_t runPedestalMode();
AcqdErrorCode_t doStartTest();

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
void fillDacValBufferGlobal(unsigned int obuffer[MAX_SURFS][34], unsigned short val);
void fillDacValBuffer(unsigned int obuffer[MAX_SURFS][34]);
void handleSlowRate(struct timeval *tvPtr, unsigned int lastEvNum);
void rateCalcAndServo(struct timeval *tvPtr, unsigned int lastEvNum);
void intersperseSurfHk(struct timeval *tvPtr);
int intersperseTurfRate(struct timeval *tvPtr);
void intersperseSoftTrig(struct timeval *tvPtr);
int checkTurfRates(); //Checks the previous N TURF rates to see if we want to implement dynamic masking



//Output functions
void writeSurfData(char *directory, unsigned short *wv_data,unsigned int evNum);
void writeTurf(char *directory, TurfioStruct_t *data_turf,unsigned int evNum);
void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr);
int writeSurfHousekeeping(int dataOrTelem);
int writeTurfHousekeeping(int dataOrTelem);
void makeSubAltDir();
void prepWriterStructs();
void outputEventData();
void outputSurfHkData();
void outputTurfRateData();
void doSurfHkAverage(int flushData);
void doTurfRateSum(int flushData);
		     

#endif //ACQD_H
