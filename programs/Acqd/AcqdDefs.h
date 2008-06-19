/*  \file AcqdDefs.h
    \brief Header for Acqd Definitions
    It's a header file what are you expecting? 
    March 2008 rjn@hep.ucl.ac.uk
*/

#ifndef ACQDSDDEFS_H
#define ACQDSDDEFS_H


// Anita includes
#include "includes/anitaFlight.h"
#include "includes/anitaStructures.h"

//Definitions
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

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
#define GetHitBus(A) (((A)&HITBUS)>>12)
#define GetStartBit(A) (((A)&START_B)>>14)
#define GetStopBit(A) (((A)&STOP_B)>>15)

//Constants
#define N_SAMP 260
#define N_SAMP_EFF 256
#define N_CHAN 9
#define N_RFCHAN RFCHAN_PER_SURF
#define N_CHIP 4


typedef enum __SURF_control_act {  
    SurfClearAll,
    SurfClearEvent,
    SurfClearHk,
    SurfHkMode,
    SurfDataMode,
    SurfEnableInt,
    SurfDisableInt
} SurfControlAction_t ;


typedef enum __SURF_status_flag {  
    SurfEventReady,
    SurfIntState,
    SurfBusInfo
} SurfStatusFlag_t ;

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
    ACQD_E_SURFREAD,
    ACQD_E_TRANSFER,
    ACQD_E_CANTCOUNT,
    ACQD_E_BADFD,
    ACQD_E_UNKNOWN_CMD,
    ACQD_E_SET_ERR,
    ACQD_E_NO_TURFIO,
    ACQD_E_MISSING_SURF,
    ACQD_E_TURFIO_GPIO,
    ACQD_E_SURF_GPIO,
    ACQD_E_CLOSE,
    ACQD_E_TURFIO_IOCTL,
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

typedef struct {
    unsigned char test[8];
} TurfioTestPattern_t;
      
int logicalScalerToRawScaler[SCALERS_PER_SURF]={8,10,12,14,9,11,13,15,16,18,20,22,17,19,21,23}; //Note raw counts from 0
int rawScalerToLogicScaler[RAW_SCALERS_PER_SURF]={-1,-1,-1,-1,-1,-1,-1,-1,0,4,1,5,2,6,3,7,8,12,9,13,10,14,11,15,-1,-1,-1,-1,-1,-1,-1,-1};

#endif //ACQDSDDEFS_H
