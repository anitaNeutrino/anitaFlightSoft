/*  \file AcqdDefs.h
    \brief Header for Acqd Definitions
    It's a header file what are you expecting? 
    March 2008 rjn@hep.ucl.ac.uk
*/

#ifndef ACQDDEFS_H
#define ACQDDEFS_H

// Plx includes
#include "PciRegs.h"
#include "PlxApi.h"
/*  #include "PlxInit.h" */
/*  #include "RegDefs.h" */
#include "Reg9030.h"

// Anita includes
#include "includes/anitaFlight.h"
#include "includes/anitaStructures.h"

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
#define GetHitBus(A) (((A)&HITBUS)>>12)
#define GetStartBit(A) (((A)&START_B)>>14)
#define GetStopBit(A) (((A)&STOP_B)>>15)

//Constants
#define N_SAMP 260
#define N_SAMP_EFF 256
#define N_CHAN 9
#define N_RFCHAN RFCHAN_PER_SURF
#define N_CHIP 4
#define N_RFTRIG SCALERS_PER_SURF  // 32 RF trigger channels per SURF board

typedef PLX_DEVICE_OBJECT PlxDevObject_t;
typedef PLX_NOTIFY_OBJECT PlxNotifyObject_t;
typedef PLX_DEVICE_KEY PlxDevKey_t;
typedef PLX_STATUS PlxStatus_t;
typedef PLX_INTERRUPT PlxInterrupt_t;

typedef enum __SURF_control_act {  
    SurfClearAll,
    SurfClearEvent,
    SurfClearHk,
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

typedef struct {
    unsigned char test[8];
} TurfioTestPattern_t;
      
	
	


#endif //ACQDDEFS_H
