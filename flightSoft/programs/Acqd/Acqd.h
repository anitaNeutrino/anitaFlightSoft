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

typedef HANDLE PlxHandle_t;
typedef DEVICE_LOCATION PlxDevLocation_t;
typedef RETURN_CODE PlxReturnCode_t;

typedef enum __SURF_control_act {  
    ClrAll,
    ClrEvt,
    LTrig,
    EvNoD,
    LabD,
    SclD,
    RFpwD
} SurfControlAction ;

typedef enum __TURF_control_act { 
    RstTurf,
    ClrTrg
} TurfControlAction ;

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
void setDACThresholds(PlxHandle_t *surfHandles, int threshold); //Only does one threshold at the moment
PlxReturnCode_t setSurfControl(PlxHandle_t surfHandle, SurfControlAction action);
PlxReturnCode_t setTurfControl(PlxHandle_t turfioHandle, TurfControlAction action);
char *surfControlActionAsString(SurfControlAction action);
char *turfControlActionAsString(TurfControlAction action);
int readConfigFile();
int init_param(int argn, char **argv, char *directory, int *n, int *dacVal);
void writeSurfData(char *directory, unsigned short *wv_data,unsigned long evNum);
void writeTurf(char *directory, TurfioStruct_t *data_turf,unsigned long evNum);
PlxReturnCode_t readPlxDataWord(PlxHandle_t handle, unsigned short *dataWord);
void calculateStatistics();
int getEventNumber();
void writeEventAndMakeLink(const char *theEventDir, const char *theLinkDir, AnitaEventFull_t *theEventPtr);
AcqdErrorCode_t readEvent(PlxHandle_t *surfHandles, PlxHandle_t turfioHandle);

#endif //ACQD_H
