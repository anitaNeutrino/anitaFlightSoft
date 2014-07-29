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
#define NUM_DYN_TURF_RATE 60

#define SURF_EVENT_DATA_SIZE 1172
#define TURF_EVENT_DATA_SIZE 256


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

//TURF Register Thingies
typedef enum __TURF_register_bank {
  TurfBankControl=0,
  TurfBankEvent=2,
  TurfBankHk=3
} TurfRegisterBank_t;

typedef enum __TURF_register_control {
  TurfRegControlFrut=0x0,
  TurfRegControlVersion=0x1,
  TurfRegControlZero1=0x2,
  TurfRegControlZero2=0x3,
  TurfRegControlL1TrigMask=0x4,
  TurfRegControlPhiMask=0x6,
  TurfRegControlEventId=0x7,
  TurfRegControlTrigger=0x8,
  TurfRegControlClockStatus=0x9,
  TurfRegControlEventReady=0xa,
  TurfRegControlNextEventId=0xb,
  TurfRegControlClear=0xc
} TurfRegsiterControl_t;

typedef enum __TURFIO_register {
  TurfioRegId = 0x100,
  TurfioRegVersion = 0x101,
  TurfioRegNull = 0x102,
  TurfioRegNumber = 0x103,
  TurfioRegPhotoShutter = 0x104,
  TurfioRegPps = 0x105,
  TurfioRegTurfBank = 0x106,
  TurfioRegClock = 0x107,
  TurfioRegDynPhase = 0x108
} TurfioRegister_t;
  
#define TURFIO_DISABLE_PHOTO_SHUTTER 0x8
#define TURFIO_RESET_PHOTO_SHUTTER 0x10

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
    SendSoftTrg,
    TurfClearEvent,
    TurfClearAll,
    TurfDisableTriggers,
    TurfEnableTriggers,
    SetPhiTrigMask,
    SetL1TrigMask,
    SetEventEpoch
} TurfControlAction_t ;

typedef enum __TURF_trigger_mode {
    TrigNone = 0,
    TrigSoft = 0x1,
    TrigPPS1 = 0x2,
    TrigPPS2 = 0x4,
    TrigDisableExt = 0x8
} TriggerMode_t;

typedef enum __TURF_clear_mode {
    BitTurfClearAll = 0x1,
    BitTurfClearEvent = 0x2,
    BitTurfTrigDisable = 0x4
} TurfClearMode_t;

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
    ACQD_E_MASK,
    ACQD_E_TURF_EVENT_READY,
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

int isNadirTrig[ACTIVE_SURFS]={0,0,0,0,0,0,0,0,1,1};

#endif //ACQDSDDEFS_H
