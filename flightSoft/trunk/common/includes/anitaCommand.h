/*! \file anitaCommand.h
  \brief Some command related thingummies
    
  Some simple definitions of the structures that we'll use to store 
  ANITA data. In here there will be the structures used for transient data,
  housekeeping stuff, GPS stuff and anything else that is going to be used
  by more than one of the processes. This will all change to use bit fields,
  at some point in the future. For now this is just a place holder we can
  test data flow with.

  August 2004  rjn@mps.ohio-state.edu
*/
#ifndef ANITA_COMMAND_H
#define ANITA_COMMAND_H

#define ACQD_ID_MASK 0x001
#define ARCHIVED_ID_MASK 0x002
#define CALIBD_ID_MASK 0x004
#define CMDD_ID_MASK 0x008
#define EVENTD_ID_MASK 0x010
#define GPSD_ID_MASK 0x020
#define HKD_ID_MASK 0x040
#define LOSD_ID_MASK 0x080
#define PRIORITIZERD_ID_MASK 0x100
#define SIPD_ID_MASK 0x200
#define MONITORD_ID_MASK 0x400
#define ALL_ID_MASK 0xfff

typedef enum {
    ID_FIRST =100,
    ID_ACQD = 100,
    ID_ARCHIVED,
    ID_CALIBD,
    ID_CMDD,
    ID_EVENTD,
    ID_GPSD,
    ID_HKD,
    ID_LOSD,
    ID_PRIORITIZERD,
    ID_SIPD,
    ID_MONITORD,
    ID_NOT_AN_ID
} ProgramId_t;

typedef enum {
    CMD_TAIL_VAR_LOG_MESSAGES = 1,
    CMD_TAIL_VAR_LOG_ANITA = 2,
    CMD_START_NEW_RUN = 3,
    CMD_SHUTDOWN_HALT = 129,
    CMD_REBOOT = 130,
    CMD_KILL_PROGS = 131,
    CMD_RESPAWN_PROGS =132,
    CMD_START_PROGS = 133,
    CMD_MOUNT = 134,
    CMD_WHITEHEAT= 135,
    CMD_MOUNT_NEXT_USB = 136,
    CMD_MOUNT_NEXT_BLADE = 137,
    CMD_EVENT_DISKTYPE = 138,
    CMD_HK_DISKTYPE = 139,
    ARCHIVE_STORAGE_TYPE = 140,
    ARCHIVE_PRI_DISK = 141,
    ARCHIVE_PRI_ENC_TYPE=142,
    ARCHIVE_ALTERNATE_USB = 143,
    ARCHIVE_DECIMATE_PRI = 144, //Not implemented
    ARCHIVE_DEFAULT_FRAC = 145, //Not implemented
    TELEM_TYPE = 146,
    TELEM_PRI_ENC_TYPE = 147,
    CMD_TURN_GPS_ON = 150,
    CMD_TURN_GPS_OFF = 151,
    CMD_TURN_RFCM_ON = 152,
    CMD_TURN_RFCM_OFF = 153,
    CMD_TURN_CALPULSER_ON = 154,
    CMD_TURN_CALPULSER_OFF = 155,
    CMD_TURN_VETO_ON = 156,
    CMD_TURN_VETO_OFF = 157,
    CMD_TURN_ALL_ON = 158,
    CMD_TURN_ALL_OFF = 159,
    SET_CALPULSER_SWITCH = 171,
    SET_CALPULSER_ATTEN = 172,
    SET_ATTEN_LOOP_PERIOD = 173,
    SET_SWITCH_LOOP_PERIOD = 174,
    SET_PULSER_OFF_PERIOD = 175,
    SET_CALIB_WRITE_PERIOD = 176,
    SET_ADU5_PAT_PERIOD = 180,
    SET_ADU5_SAT_PERIOD = 181,
    SET_G12_PPS_PERIOD = 182,
    SET_G12_PPS_OFFSET = 183,
    SET_ADU5_CALIBRATION_12 = 184,
    SET_ADU5_CALIBRATION_13 = 185,
    SET_ADU5_CALIBRATION_14 = 186,
    SET_ADU5_VTG_PERIOD = 187,
    SET_G12_POS_PERIOD = 188,
    SET_HK_PERIOD = 190,
    SET_HK_CAL_PERIOD = 191,
    CLEAN_DIRS = 200,
    SEND_CONFIG = 210,
    DEFAULT_CONFIG =211,
    SWITCH_CONFIG =212,
    LAST_CONFIG=213,
    ACQD_ADU5_TRIG_FLAG = 230,
    ACQD_G12_TRIG_FLAG = 231,
    ACQD_SOFT_TRIG_FLAG = 232,
    ACQD_SOFT_TRIG_PERIOD = 233,
    ACQD_ENABLE_CHAN_SERVO = 234,
    ACQD_SET_PID_GOAL = 235,
    ACQD_PEDESTAL_RUN = 236,
    ACQD_THRESHOLD_SCAN =237,
    ACQD_SET_ANT_TRIG_MASK = 238,
    ACQD_SET_SURF_BAND_TRIG_MASK = 239,
    ACQD_SET_GLOBAL_THRESHOLD = 240,
    ACQD_REPROGRAM_TURF = 241,
    ACQD_SURFHK_PERIOD = 242,
    ACQD_SURFHK_TELEM_EVERY = 243,
    ACQD_TURFHK_TELEM_EVERY =244,
    ACQD_NUM_EVENTS_PEDESTAL = 245,
    ACQD_THRESH_SCAN_STEP_SIZE = 246,
    ACQD_THRESH_SCAN_POINTS_PER_STEP = 247
    
} CommandCode_t;

#endif /* ANITA_COMMAND_H */
