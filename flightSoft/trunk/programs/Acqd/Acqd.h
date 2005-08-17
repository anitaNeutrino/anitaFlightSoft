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
#define EVT_F 0x800
#define LAB_F 0x4000
#define DATA_MASK 0xfff

//Constants
#define N_SAMP 256
#define N_CHAN 8
#define N_CHIP 4
#define N_RFTRIG 32  // 32 RF trigger channels per SURF board
#define PHYS_DAC 8


typedef HANDLE PlxHandle_t;
typedef DEVICE_LOCATION PlxDevLocation_t;
typedef RETURN_CODE PlxReturnCode_t;

typedef enum __SURF_control_act {  
    SurfClearAll,
    SurfClearEvent,
    LTrig,
    EvNoD,
    LabD,
    SclD,
    RFpwD
} SurfControlAction_t ;

typedef enum __TURF_control_act { 
    SetTrigMode,
    RprgTurf,
    TurfClearEvent,
    SendSoftTrg,
    EnableRfTrg,
    EnablePPS1Trig,
    EnablePPS2Trig,
    EnableSoftTrig,
    TurfClearAll
} TurfControlAction_t ;

typedef enum __TURF_trigger_mode {
    TrigNone = 0,
    TrigRF = 0x1,
    TrigPPS1 = 0x2,
    TrigPPS2 = 0x4,
    TrigSoft = 0x8
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



      

//Forward Declarations
int initializeDevices(PlxHandle_t *surfHandles, PlxHandle_t *turfioHandle, PlxDevLocation_t *surfLoc, PlxDevLocation_t *turfioLoc);
void clearDevices(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle);
void setDACThresholds(PlxHandle_t *surfHandles, unsigned short threshold); //Only does one threshold at the moment
PlxReturnCode_t setSurfControl(PlxHandle_t surfHandle, SurfControlAction_t action);
PlxReturnCode_t setTurfControl(PlxHandle_t turfioHandle, TriggerMode_t trigMode, TurfControlAction_t action);
char *surfControlActionAsString(SurfControlAction_t action);
char *turfControlActionAsString(TurfControlAction_t action);
int readConfigFile();
int init_param(int argn, char **argv, char *directory, int *n, unsigned short *dacVal);
void writeSurfData(char *directory, unsigned short *wv_data,unsigned long evNum);
void writeTurf(char *directory, TurfioStruct_t *data_turf,unsigned long evNum);
PlxReturnCode_t readPlxDataWord(PlxHandle_t handle, unsigned short *dataWord);
void calculateStatistics();
int getEventNumber();
void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr);
AcqdErrorCode_t readEvent(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle);
//PlxReturnCode_t setTurfTriggerMode(PlxHandle_t turfioHandle, TriggerMode_t trigMode);
void setDACThresholdsOnChan(PlxHandle_t surfHandle, int chan, unsigned short values[8]);
void setDACThresholdsIndividuually(PlxHandle_t *surfHandles);
PlxReturnCode_t setBarMap(PlxHandle_t *surfHandles);
PlxReturnCode_t unsetBarMap(PlxHandle_t *surfHandles);
//PlxReturnCode_t blockReadPlxData(PlxHandle_t handle, unsigned short *dataArray, int numBytes);

#endif //ACQD_H
