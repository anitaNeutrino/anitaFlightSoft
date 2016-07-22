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
#define PLAYBACKD_ID_MASK 0x800
#define LOGWATCHD_ID_MASK 0x1000
#define NTUD_ID_MASK 0x2000
#define OPENPORTD_ID_MASK 0x4000
#define RTLD_ID_MASK 0x8000
#define TUFFD_ID_MASK 0x1000 
#define ALL_ID_MASK 0xffff


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
    ID_PLAYBACKD,
    ID_LOGWATCHD,
    ID_NTUD,
    ID_OPENPORTD,
    ID_RTLD, 
    ID_TUFFD,
    ID_NOT_AN_ID
} ProgramId_t;




#define NUM_ANITA_COMMANDS 256 


typedef enum {
    CMD_TAIL_VAR_LOG_MESSAGES = 1,
    CMD_TAIL_VAR_LOG_ANITA = 2,
    CMD_START_NEW_RUN = 3,
    CMD_MAKE_NEW_RUN_DIRS = 4,
    LOG_REQUEST_COMMAND = 10,
    JOURNALCTL_REQUEST_COMMAND =11,

    RTLD_COMMAND = 28,  

    CMD_REALLY_KILL_PROGS = 127,
    CMD_SIPD_REBOOT = 128,
    CMD_SHUTDOWN_HALT = 129,

    CMD_REBOOT = 130,
    CMD_KILL_PROGS = 131,
    CMD_RESPAWN_PROGS =132,
    CMD_START_PROGS = 133,
    CMD_MOUNT_ARGH = 134,
    CMD_DISABLE_DISK= 135,
    CMD_MOUNT_NEXT_USB = 136,
    CMD_MOUNT_NEXT_NTU = 137, //Switch for blade v mini
    CMD_EVENT_DISKTYPE = 138,
    CMD_HK_DISKTYPE = 139,

    ARCHIVE_STORAGE_TYPE = 140,
    ARCHIVE_PRI_DISK = 141,
    ARCHIVE_PRI_ENC_TYPE=142,
    ARCHIVE_DECIMATE_PRI = 144, //For each disk
    ARCHIVE_GLOBAL_DECIMATE = 145, 
    TELEM_TYPE = 146,
    TELEM_PRI_ENC_TYPE = 147,
    ARCHIVE_PPS_PRIORITIES = 148,
    ARCHIVE_PPS_DECIMATE = 149,

    CMD_TURN_AMPLITES_ON = 150,
    CMD_TURN_AMPLITES_OFF = 151,
    CMD_TURN_BZ_AMPAS_ON = 152,
    CMD_TURN_BZ_AMPAS_OFF = 153,
    CMD_TURN_NTU_AMPAS_ON = 154,
    CMD_TURN_NTU_AMPAS_OFF = 155,
    CMD_TURN_SHORT_BOARDS_ON = 156,
    CMD_TURN_SHORT_BOARDS_OFF = 157,
    CMD_TURN_NTU_SSD_5V_ON = 158,
    CMD_TURN_NTU_SSD_5V_OFF = 159,

    CMD_TURN_NTU_SSD_12V_ON = 160,
    CMD_TURN_NTU_SSD_12V_OFF = 161,
    CMD_TURN_ALL_ON = 162,
    CMD_TURN_ALL_OFF = 163,    
 
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
    GPSD_EXTRA_COMMAND = 189,

    SET_HK_PERIOD = 190,
    SET_HK_CAL_PERIOD = 191,
    SET_HK_TELEM_EVERY = 192,
    SIPD_CONTROL_COMMAND = 195,
    LOSD_CONTROL_COMMAND =196,

  

    CLEAN_DIRS = 200,
    CLEAR_RAMDISK = 201,
   
    SEND_CONFIG = 210,
    DEFAULT_CONFIG =211,
    SWITCH_CONFIG =212,
    LAST_CONFIG=213,
    SAVE_CONFIG=214,

    MONITORD_RAMDISK_KILL_ACQD = 220,
    MONITORD_RAMDISK_DUMP_DATA = 221,
    MONITORD_MAX_ACQD_WAIT = 222,
    MONITORD_PERIOD = 223,
    MONITORD_USB_THRESH = 224,
    MONITORD_NTU_THRESH = 225,
    MONITORD_MAX_QUEUE = 226, 
    MONITORD_INODES_KILL_ACQD = 227, 
    MONITORD_INODES_DUMP_DATA = 228,        

    ACQD_ADU5_TRIG_FLAG = 230,
    ACQD_G12_TRIG_FLAG = 231,
    ACQD_SOFT_TRIG_FLAG = 232,
    ACQD_SOFT_TRIG_PERIOD = 233,
    //    ACQD_ENABLE_CHAN_SERVO = 234, //Was 2 bytes
    //    ACQD_SET_PID_GOAL = 235, //Was 3 bytes
    ACQD_PEDESTAL_RUN = 236,
    ACQD_THRESHOLD_SCAN =237,
    //    ACQD_SET_ANT_TRIG_MASK = 238, //Was 5 bytes
    //    ACQD_SET_SURF_BAND_TRIG_MASK = 239, //Was 6 bytes
    //    ACQD_SET_GLOBAL_THRESHOLD = 240, //Was 3 bytes


    ACQD_EXTRA_COMMAND = 240, // 3 bytes
    ACQD_REPROGRAM_TURF = 241,
    ACQD_SURFHK_TELEM_EVERY = 243,
    ACQD_TURFHK_TELEM_EVERY =244,
    ACQD_NUM_EVENTS_PEDESTAL = 245,
    ACQD_THRESH_SCAN_STEP_SIZE = 246,
    ACQD_THRESH_SCAN_POINTS_PER_STEP = 247,
    //    ACQD_CHAN_PID_GOAL_SCALE = 248, //Was 5
    //    ACQD_SET_RATE_SERVO = 249, //Was 3
    ACQD_RATE_COMMAND = 250, //Guess at 10 for now
  


    EVENTD_MATCH_GPS = 251,
    GPS_PHI_MASK_COMMAND = 252,
    PRIORITIZERD_COMMAND =253,  
    PLAYBACKD_COMMAND =254, 
    TUFFD_COMMAND = 255 
} CommandCode_t;

typedef enum {
  ACQD_RATE_ENABLE_CHAN_SERVO=1,
  ACQD_RATE_SET_PID_GOALS=2,
  ACQD_RATE_SET_L1_TRIG_MASK=3,
  ACQD_RATE_SET_PHI_MASK=4,
  ACQD_RATE_SET_SURF_BAND_TRIG_MASK=5,
  ACQD_RATE_SET_CHAN_PID_GOAL_SCALE=6,
  ACQD_RATE_SET_RATE_SERVO=7,
  ACQD_RATE_ENABLE_DYNAMIC_PHI_MASK=8,
  ACQD_RATE_ENABLE_DYNAMIC_ANT_MASK=9,
  ACQD_RATE_SET_DYNAMIC_PHI_MASK_OVER=10,  //Set over rate and over window
  ACQD_RATE_SET_DYNAMIC_PHI_MASK_UNDER=11,  //Set under rate and under window
  ACQD_RATE_SET_DYNAMIC_ANT_MASK_OVER=12, //Set over rate and over window
  ACQD_RATE_SET_DYNAMIC_ANT_MASK_UNDER=13, //Set under rate and under window
  ACQD_RATE_SET_GLOBAL_THRESHOLD=14,
  ACQD_RATE_SET_GPS_PHI_MASK=15,
  ACQD_SET_NADIR_PID_GOALS=16,
  ACQD_SET_PID_PGAIN=17,
  ACQD_SET_PID_IGAIN=18,
  ACQD_SET_PID_DGAIN=19,
  ACQD_SET_PID_IMAX=20,
  ACQD_SET_PID_IMIN=21,
  ACQD_SET_PID_AVERAGE=22,
  ACQD_RATE_SET_PHI_MASK_HPOL=23,
  ACQD_RATE_SET_L1_TRIG_MASK_HPOL=24,
} AcqdRateCommandCode_t;

typedef enum {
  ACQD_DISABLE_SURF = 127,
  ACQD_SET_TURF_RATE_AVERAGE = 128,
  ACQD_SET_PHOTO_SHUTTER_MASK = 140,  
  ACQD_SET_PPS_SOURCE = 141,  
  ACQD_SET_REF_CLOCK_SOURCE = 142  
} AcqdExtraCommand_t;
  
  
  
 
typedef enum {
  PRI_PANIC_QUEUE_LENGTH=1,
  PRI_PARAMS_LOW_BIN_EDGE=2,
  PRI_PARAMS_HIGH_BIN_EDGE=3,
  PRI_SLOPE_IMAGE_HILBERT=4,
  PRI_INTERCEPT_IMAGE_HILBERT=5,
  PRI_BIN_TO_BIN_THRESH=6,
  PRI_ABS_MAG_THRESH=7,
  PRI_THETA_ANGLE_DEMOTION_LOW=8,
  PRI_THETA_ANGLE_DEMOTION_HIGH=9,
  PRI_THETA_ANGLE_DEMOTION_PRIORITY=10,
  PRI_POWER_SPECTRUM_PERIOD=11,
  PRI_ANT_PHI_POS=12,
  PRI_ANT_R_POS=13,
  PRI_ANT_Z_POS=14,
  PRI_POS_SATUATION=15,
  PRI_NEG_SATUATION=16
} PrioritizerdCommandCode_t;


#define NBITS_FOR_RTL_INDEX 3
typedef enum
{
  RTL_SET_TELEM_EVERY = 1, 
  RTL_SET_START_FREQUENCY=2, 
  RTL_SET_END_FREQUENCY=3, 
  RTL_SET_GAIN = 4, 
  RTL_SET_FREQUENCY_STEP =5
} RTLdCommandCode_t; 

typedef enum
{
  TUFF_SET_NOTCH = 1, 
  TUFF_SEND_RAW = 2, 
  TUFF_SET_SLEEP_AMOUNT =3, 
  TUFF_SET_READ_TEMPERATURE =4 , 
  TUFF_SET_TELEM_EVERY = 5, 
  TUFF_SET_TELEM_AFTER_CHANGE = 6 
  
} TuffCommandCode_t; 

  
typedef enum {
  GPS_PHI_MASK_ENABLE=1,
  GPS_PHI_MASK_UPDATE_PERIOD=2,
  GPS_PHI_MASK_SET_SOURCE_LATITUDE=3,
  GPS_PHI_MASK_SET_SOURCE_LONGITUDE=4,
  GPS_PHI_MASK_SET_SOURCE_ALTITUDE=5,
  GPS_PHI_MASK_SET_SOURCE_HORIZON=6,
  GPS_PHI_MASK_SET_SOURCE_WIDTH=7
} GpsPhiMaskCommandCode_t;
    
typedef enum {
    PLAY_GET_EVENT=1,    
    PLAY_START_PRI=2,
    PLAY_STOP_PRI=3,
    PLAY_USE_DISK=4,
    PLAY_START_EVENT=5,
    PLAY_START_PLAY=6,
    PLAY_STOP_PLAY=7,
    PLAY_SLEEP_PERIOD=8
} PlaybackCommandCode_t;

typedef enum {
  SIPD_SEND_WAVE = 127,
  SIPD_THROTTLE_RATE = 128,
  SIPD_PRIORITY_BANDWIDTH = 129,
  SIPD_HEADERS_PER_EVENT = 130,
  SIPD_HK_TELEM_ORDER = 131,
  SIPD_HK_TELEM_MAX_PACKETS = 132
} SipdCommandCode_t;

typedef enum {
  LOSD_SEND_DATA = 1,
  LOSD_PRIORITY_BANDWIDTH = 2,
  LOSD_HK_BANDWIDTH = 3
} LosdCommandCode_t;
  
typedef enum {
  LOG_REQUEST_JOURNALCTL = 1,
  LOG_REQUEST_ANITA = 2,
  LOG_REQUEST_SECURITY = 3,
  LOG_REQUEST_NTU = 4,
  LOG_REQUEST_BOOT = 5, 
  LOG_REQUEST_PROC_CPUINFO = 6,
  LOG_REQUEST_PROC_DEVICES = 7,
  LOG_REQUEST_PROC_DISKSTATS = 8,
  LOG_REQUEST_PROC_FILESYSTEMS = 9,
  LOG_REQUEST_PROC_INTERRUPTS = 10,
  LOG_REQUEST_PROC_IOMEM = 11,
  LOG_REQUEST_PROC_IOPORTS = 12,
  LOG_REQUEST_PROC_LOADAVG =13,
  LOG_REQUEST_PROC_MEMINFO =14,
  LOG_REQUEST_PROC_MISC = 15,
  LOG_REQUEST_PROC_MODULES = 16,
  LOG_REQUEST_PROC_MOUNTS = 17,
  LOG_REQUEST_PROC_MTRR = 18,
  LOG_REQUEST_PROC_PARTITIONS = 19,
  LOG_REQUEST_PROC_SCHEDDEBUG = 20,
  LOG_REQUEST_PROC_SCHEDSTAT = 21,
  LOG_REQUEST_PROC_STAT = 22,
  LOG_REQUEST_PROC_SWAPS = 23,
  LOG_REQUEST_PROC_TIMERLIST = 24,
  LOG_REQUEST_PROC_TIMERSTATS= 25,
  LOG_REQUEST_PROC_UPTIME = 26,
  LOG_REQUEST_PROC_VERSION = 27,
  LOG_REQUEST_PROC_VMCORE = 28,
  LOG_REQUEST_PROC_VMSTAT = 29,
  LOG_REQUEST_PROC_ZONEINFO = 30,
  LOG_REQUEST_NTU_STATUS = 31,
  LOG_REQUEST_FILE=32
} LogRequestCommand_t;


typedef enum {
  JOURNALCTL_OPT_COMM=0,
  JOURNALCTL_OPT_PRIORITY=1,
  JOURNALCTL_OPT_SYSLOG_FACILITY=2,
  JOURNALCTL_NO_OPT=3
} JournalctlOptionCommand_t;


typedef enum {
  GPS_SET_GGA_PERIOD = 130,
  GPS_SET_PAT_TELEM_EVERY = 131,
  GPS_SET_VTG_TELEM_EVERY = 132,
  GPS_SET_SAT_TELEM_EVERY = 133,
  GPS_SET_GGA_TELEM_EVERY = 134,
  GPS_SET_POS_TELEM_EVERY = 135,
  GPS_SET_INI_RESET_FLAG = 136,
  GPS_SET_ELEVATION_MASK = 137,
  GPS_SET_CYC_PHASE_ERROR = 138,
  GPS_SET_MXB_BASELINE_ERROR = 139,
  GPS_SET_MXM_PHASE_ERROR = 140
} GpsExtraCommand_t;

#endif /* ANITA_COMMAND_H */
