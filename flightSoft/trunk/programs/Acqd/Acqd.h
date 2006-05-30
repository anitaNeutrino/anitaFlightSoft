/*  \file Acqd.h
    \brief Header for Acqd
    It's a header file what are you expecting? 
    July 2005 rjn@mps.ohio-state.edu
*/

#ifndef ACQD_H
#define ACQD_H

// Plx includes
#include "PciRegs.h"
#include "PlxApi.h"
/*  #include "PlxInit.h" */
/*  #include "RegDefs.h" */
#include "Reg9030.h"

// Anita includes
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"

//Definitions
#define EVT_RDY  0004
#define LAB_F 0x4000
#define START_B 0x4000
#define STOP_B  0x8000
#define DATA_MASK 0xfff
#define LOWER16 0xffff
#define UPPER16 0xffff0000
#define HITBUS 0x1000
#define MAX_EVENTS_PER_DIR 1000




#define GetUpper16(A) (((A)&UPPER16)>>16)
#define GetLower16(A) ((A)&LOWER16)

//Constants
#define N_SAMP 260
#define N_SAMP_EFF 256
#define N_CHAN 9
#define N_RFCHAN RFCHAN_PER_SURF
#define N_CHIP 4
#define N_RFTRIG SCALERS_PER_SURF  // 32 RF trigger channels per SURF board

typedef HANDLE PlxHandle_t;
typedef DEVICE_LOCATION PlxDevLocation_t;
typedef RETURN_CODE PlxReturnCode_t;
typedef PLX_INTR PlxIntr_t;

typedef enum __SURF_control_act {  
    SurfClearAll,
    SurfClearEvent,
    LTrig,
    RDMode,
    WTMode,
    DTRead,
    IntEnb,
    DACLoad,
    CHGChp
} SurfControlAction_t ;

typedef enum __TURF_control_act { 
    SetTrigMode,
    RprgTurf,
    TurfLoadRam,
    SendSoftTrg,
    RFCBit,
    RFCClk,
//    EnableRfTrg,
    EnablePPS1Trig,
    EnablePPS2Trig,
//    EnableSoftTrig, //Always enabled
    TurfClearAll
} TurfControlAction_t ;

typedef enum __TURF_trigger_mode {
    TrigNone = 0,
    TrigPPS1 = 0x2,
    TrigPPS2 = 0x4
} TriggerMode_t;

typedef enum {
    ACQD_E_OK = 0,
    ACQD_E_LTRIG = 0x100,
    ACQD_E_EVNOD,
    ACQD_E_LABD,
    ACQD_E_SCLD,
    ACQD_E_RFPWD,
    ACQD_E_PLXBUSREAD,
    ACQD_E_TRANSFER,
    ACQD_E_UNNAMED
} AcqdErrorCode_t ;

typedef struct {
    int bus;
    int slot;
} BoardLocStruct_t;

typedef struct {
    int dState;  // Last position input
    int iState;  // Integrator state
} DacPidStruct_t;

      
//Forward Declarations
int initializeDevices(PlxHandle_t *surfHandles, PlxHandle_t *turfioHandle, PlxDevLocation_t *surfLoc, PlxDevLocation_t *turfioLoc);
void clearDevices(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle);
PlxReturnCode_t setSurfControl(PlxHandle_t surfHandle, SurfControlAction_t action);
PlxReturnCode_t setTurfControl(PlxHandle_t turfioHandle, TurfControlAction_t action);
char *surfControlActionAsString(SurfControlAction_t action);
char *turfControlActionAsString(TurfControlAction_t action);
int readConfigFile();
int init_param(int argn, char **argv,  int *n, unsigned short *dacVal);
void writeSurfData(char *directory, unsigned short *wv_data,unsigned long evNum);
void writeTurf(char *directory, TurfioStruct_t *data_turf,unsigned long evNum);
PlxReturnCode_t readPlxDataWord(PlxHandle_t handle, unsigned int *dataWord);
PlxReturnCode_t readPlxDataShort(PlxHandle_t handle, unsigned short *dataWord);
void calculateStatistics();
int getEventNumber();
void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr);
AcqdErrorCode_t readSurfEventData(PlxHandle_t *surfHandles);
AcqdErrorCode_t readSurfHkData(PlxHandle_t *surfHandles);
AcqdErrorCode_t readTurfEventData(PlxHandle_t turfioHandle);
//PlxReturnCode_t setTurfTriggerMode(PlxHandle_t turfioHandle, TriggerMode_t trigMode);
void setDACThresholds(PlxHandle_t *surfHandles);
PlxReturnCode_t setBarMap(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle);
PlxReturnCode_t unsetBarMap(PlxHandle_t *surfHandles,PlxHandle_t turfioHandle);
PlxReturnCode_t writeRFCMMask(PlxHandle_t turfioHandle);
void setGloablDACThreshold(PlxHandle_t *surfHandles, unsigned short threshold);
//PlxReturnCode_t blockReadPlxData(PlxHandle_t handle, unsigned short *dataArray, int numBytes);
int updateThresholdsUsingPID();
int writeSurfHousekeeping(int dataOrTelem);
int writeTurfHousekeeping(int dataOrTelem);
AcqdErrorCode_t readSurfEventDataVer2(PlxHandle_t *surfHandles);
void makeSubAltDir();
//int bufferedTurfHkWrite(TurfRateStruct_t *turfPtr, char *baseDir);
void prepWriterStructs();


typedef struct {
    unsigned char test[8];
} TurfioTestPattern_t;

	
	


#endif //ACQD_H
