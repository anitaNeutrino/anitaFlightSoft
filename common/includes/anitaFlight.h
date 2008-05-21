/*! \file anitaFlight.h
    \brief The standard ANITA include file.
    
    It will end up including all those definitions that are needed 
    all over the shop. Hopefully there won't be too many of these floating 
    around.
    August 2004  rjn@mps.ohio-state.edu
*/
/*! \mainpage ANITA Flight Software Doxygen Powered Documentation
 *
 * \section intro_sec Introduction
 *
 * Okay so there really isn't that much to say here. What we have is a
 * Doxygenated (yeah, I can make up words with the best of them) version
 * of the flight software code.
 *
 * \section Reminder
 *
 * You may also want to look at the <a href="http://www.physics.ohio-state.edu/~rjn/flightSoft>Flight Software homepage.</a>
 *  
 *
 */


#ifndef ANITA_FLIGHT_H
#define ANITA_FLIGHT_H


#ifndef HACK_FOR_ROOT
#include <syslog.h>
#define ANITA_LOG_FACILITY LOG_LOCAL4
#endif

#define SPEED_UP_LOSD 1

// Hardware stuff

#define ACTIVE_SURFS 10
#define MAX_SURFS 12
#define CHANNELS_PER_SURF 9
#define LABRADORS_PER_SURF 4 /*jjb 2006-04-19 */
#define NUM_DAC_CHANS 4
#define RAW_SCALERS_PER_SURF 32
#define SCALERS_PER_SURF 16
#define RFCHAN_PER_SURF 8
#define NUM_DIGITZED_CHANNELS ACTIVE_SURFS*CHANNELS_PER_SURF
#define MAX_NUMBER_SAMPLES 260
#define EFFECTIVE_SAMPLES 256
#define ADC_MAX 4096
#define TRIGGER_OFFSET 64 /* Made up number for where the trigger is */
#define PED_DEFAULT_VALUE 650


//Process Stuff
#define NUM_PROCESSES 16 //Actually a couple more than we need

//GPS stuff
#define MAX_SATS 12
#define MAX_CMD_LENGTH 20


//Acromag stuff
#define CHANS_PER_IP320 40
#define NUM_IP320_BOARDS 3

//Trigger Stuff
#define ANTS_PER_SURF 4
#define TRIGGER_SURFS 8
#define PHI_SECTORS 16

//Prioritizer Stuff
#define WRAPPED_HITBUS 0x8 //Bit 3 is the wrapped hitbus bit
#define SURF_BITMASK 0x0ffc  /* masks off the two wired-or bits for analysis */
#define ELEVEN_BITMASK 0x0ffe  /* masks off the last wired-or bit for analysis */
#define HITBUS_MASK 0x1000 //Hitbus mask for pedestals
#define HILBERT_ORDER 5 /*order used for Hilbert transforms in Stokes S3 */

//Whiteheat things
#define G12A_DEV_NAME "/dev/ttySTDRV002_5"
#define G12B_DEV_NAME "/dev/ttySTDRV002_6" //These are the two ports
#define ADU5A_DEV_NAME "/dev/ttySTDRV002_4"
#define ADU5B_DEV_NAME "/dev/ttySTDRV002_1"  //These are the two ADU5s
#define MAGNETOMETER_DEV_NAME "/dev/ttySTDRV002_3"

#define NUM_PRIORITIES 10
#define NUM_SATABLADES 8
#define NUM_SATAMINIS 4
#define NUM_USBS 31

#define GLOBAL_CONF_FILE "anitaSoft.config"


//File writing things
#define HK_PER_FILE 1000
#define HK_FILES_PER_DIR 100
#define EVENTS_PER_FILE 100
#define EVENT_FILES_PER_DIR 100
#define EVENTS_PER_INDEX 10000
#define DISK_TYPES 4 //neobrick not included in this list



//Relay Bit Masks
#define RFCM1_MASK 0x1
#define RFCM2_MASK 0x2
#define RFCM3_MASK 0x4
#define RFCM4_MASK 0x8
#define VETO_MASK 0x10
#define GPS_MASK 0x20
#define CAL_PULSER_MASK 0x40

#define RFSWITCH_MASK 0xf00
#define RFSWITCH_SHIFT 8

#define ATTEN_MASK 0xf000
#define ATTEN_SHIFT 12

#define WAKEUP_LOS_BUFFER_SIZE 4000
#define WAKEUP_TDRSS_BUFFER_SIZE 500
#define WAKEUP_LOW_RATE_BUFFER_SIZE 100

//Turf Bit Masks
#define CUR_BUF_MASK_READ 0x30
#define CUR_BUF_SHIFT_READ 4
#define CUR_BUF_MASK_FINAL 0xc
#define CUR_BUF_SHIFT_FINAL 2
#define TRIG_BUF_MASK_READ 0x60
#define TRIG_BUF_SHIFT_READ 5
#define TRIG_BUF_MASK_FINAL 0x3
#define TRIG_BUF_SHIFT_FINAL 0

//Disk Bit Masks
#define SATABLADE_DISK_MASK 0x1
#define SATAMINI_DISK_MASK 0x2
#define USB_DISK_MASK 0x4
#define NEOBRICK_DISK_MASK 0x8
#define PMC_DISK_MASK 0x10


//Mount points and fake mount points
#ifdef USE_FAKE_DATA_DIR
#define DATA_LINK "/tmp/mnt/data" //Where the index and peds are written
#define DATABACKUP_LINK "/tmp/mnt/dataBackup" //Where the index and peds are written
#define SAFE_DATA_MOUNT "/tmp/mnt/bitmicro"
#define SATAMINI_DATA_MOUNT "/tmp/mnt/satamini"
#define SATABLADE_DATA_MOUNT "/tmp/mnt/satablade"
#define USB_DATA_MOUNT "/tmp/mnt/usbint"
#define PLAYBACK_MOUNT "/tmp/mnt/playback"
#define LAST_EVENT_NUMBER_FILE "/tmp/mnt/data/numbers/lastEventNumber"
#define LAST_LOS_NUMBER_FILE "/tmp/mnt/data/numbers/lastLosNumber"
#define LAST_TDRSS_NUMBER_FILE "/tmp/mnt/data/numbers/lastTdrssNumber"
#define LAST_RUN_NUMBER_FILE "/tmp/mnt/data/numbers/lastRunNumber"
#else
#define DATA_LINK "/mnt/data" //Where the index and peds are written
#define DATABACKUP_LINK "/tmp/mnt/dataBackup" //Where the index and peds are written
#define SAFE_DATA_MOUNT "/mnt/bitmicro"
#define SATAMINI_DATA_MOUNT "/mnt/satamini"
#define SATABLADE_DATA_MOUNT "/mnt/satablade"
#define USB_DATA_MOUNT "/mnt/usbint"
#define PLAYBACK_MOUNT "/mnt/playback"
#define LAST_EVENT_NUMBER_FILE "/mnt/data/numbers/lastEventNumber"
#define LAST_LOS_NUMBER_FILE "/mnt/data/numbers/lastLosNumber"
#define LAST_TDRSS_NUMBER_FILE "/mnt/data/numbers/lastTdrssNumber"
#define LAST_RUN_NUMBER_FILE "/mnt/data/numbers/lastRunNumber"
#endif


#define DISK_BUFFER_DIR "/mnt/bitmicro/buffer"

//Index and pedestal dirs
#define ANITA_INDEX_DIR "anita/index"
#define ANITA_PRIORITY_CATCH_SUB_DIR "anita/trashed"
#define PEDESTAL_DIR "anita/pedestal"
#define CURRENT_PEDESTALS "anita/pedestal/current"

#define MAX_EVENT_SIZE NUM_DIGITZED_CHANNELS*MAX_NUMBER_SAMPLES*4

//Inter process and telem dirs.
//Housekeeping Telem Dirs
#define LOSD_CMD_ECHO_TELEM_DIR "/tmp/anita/telem/house/cmdlosd"
#define SIPD_CMD_ECHO_TELEM_DIR "/tmp/anita/telem/house/cmdsipd"
#define HK_TELEM_DIR "/tmp/anita/telem/house/hk"
#define SURFHK_TELEM_DIR "/tmp/anita/telem/house/surfhk"
#define PEDESTAL_TELEM_DIR "/tmp/anita/telem/house/pedestal"
#define TURFHK_TELEM_DIR "/tmp/anita/telem/house/turfhk"
#define ADU5A_PAT_TELEM_DIR "/tmp/anita/telem/house/gps/adu5a/pat"
#define ADU5A_SAT_TELEM_DIR "/tmp/anita/telem/house/gps/adu5a/sat"
#define ADU5A_VTG_TELEM_DIR "/tmp/anita/telem/house/gps/adu5a/vtg"
#define ADU5A_GGA_TELEM_DIR "/tmp/anita/telem/house/gps/adu5a/gga"
#define ADU5B_PAT_TELEM_DIR "/tmp/anita/telem/house/gps/adu5b/pat"
#define ADU5B_SAT_TELEM_DIR "/tmp/anita/telem/house/gps/adu5b/sat"
#define ADU5B_VTG_TELEM_DIR "/tmp/anita/telem/house/gps/adu5b/vtg"
#define ADU5B_GGA_TELEM_DIR "/tmp/anita/telem/house/gps/adu5b/gga"
#define G12_POS_TELEM_DIR "/tmp/anita/telem/house/gps/g12/pos"
#define G12_SAT_TELEM_DIR "/tmp/anita/telem/house/gps/g12/sat"
#define G12_GGA_TELEM_DIR "/tmp/anita/telem/house/gps/g12/gga"
#define MONITOR_TELEM_DIR "/tmp/anita/telem/house/monitor"
#define OTHER_MONITOR_TELEM_DIR "/tmp/anita/telem/house/other"
#define HEADER_TELEM_DIR "/tmp/anita/telem/head"
#define REQUEST_TELEM_DIR "/tmp/anita/telem/request"

#define LOSD_CMD_ECHO_TELEM_LINK_DIR "/tmp/anita/telem/house/cmdlosd/link"
#define SIPD_CMD_ECHO_TELEM_LINK_DIR "/tmp/anita/telem/house/cmdsipd/link"
#define HK_TELEM_LINK_DIR "/tmp/anita/telem/house/hk/link"
#define SURFHK_TELEM_LINK_DIR "/tmp/anita/telem/house/surfhk/link"
#define PEDESTAL_TELEM_LINK_DIR "/tmp/anita/telem/house/pedestal/link"
#define TURFHK_TELEM_LINK_DIR "/tmp/anita/telem/house/turfhk/link"
#define ADU5A_PAT_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/adu5a/pat/link"
#define ADU5A_SAT_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/adu5a/sat/link"
#define ADU5A_VTG_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/adu5a/vtg/link"
#define ADU5A_GGA_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/adu5a/gga/link"
#define ADU5B_PAT_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/adu5b/pat/link"
#define ADU5B_SAT_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/adu5b/sat/link"
#define ADU5B_VTG_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/adu5b/vtg/link"
#define ADU5B_GGA_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/adu5b/gga/link"
#define G12_POS_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/g12/pos/link"
#define G12_SAT_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/g12/sat/link"
#define G12_GGA_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/g12/gga/link"
#define MONITOR_TELEM_LINK_DIR "/tmp/anita/telem/house/monitor/link"
#define HEADER_TELEM_LINK_DIR "/tmp/anita/telem/head/link"
#define REQUEST_TELEM_LINK_DIR "/tmp/anita/telem/request/link"
#define OTHER_MONITOR_TELEM_LINK_DIR "/tmp/anita/telem/house/other/link"



//Event telemetry dirs
#define BASE_EVENT_TELEM_DIR "/tmp/anita/telem/event"
#define EVENT_PRI_PREFIX "pri"

//Inter-process communication dirs
#define ACQD_EVENT_DIR "/tmp/anita/acqd/event"
#define ACQD_EVENT_LINK_DIR "/tmp/anita/acqd/event/link"
#define EVENTD_EVENT_DIR "/tmp/anita/eventd/event"
#define EVENTD_EVENT_LINK_DIR "/tmp/anita/eventd/event/link"
#define PRIORITIZERD_EVENT_DIR "/tmp/anita/prioritizerd/event"
#define PRIORITIZERD_EVENT_LINK_DIR "/tmp/anita/prioritizerd/event/link"
#define CMDD_COMMAND_DIR "/tmp/anita/cmdd"
#define CMDD_COMMAND_LINK_DIR "/tmp/anita/cmdd/link"
#define CALIBD_STATUS_DIR "/tmp/anita/calibd"
#define CALIBD_STATUS_LINK_DIR "/tmp/anita/calibd/link"
#define GPSD_SUBTIME_DIR "/tmp/anita/gpsd/ttt"
#define GPSD_SUBTIME_LINK_DIR "/tmp/anita/gpsd/ttt/link"
#define PLAYBACK_DIR "/tmp/anita/playbackd"
#define PLAYBACK_LINK_DIR "/tmp/anita/playbackd/link"
#define LOGWATCH_DIR "/tmp/anita/logwatchd"
#define LOGWATCH_LINK_DIR "/tmp/anita/logwatchd/link"

//PID Files
#define ACQD_PID_FILE "/tmp/anita/pid/acqd.pid"
#define ARCHIVED_PID_FILE "/tmp/anita/pid/archived.pid"
#define CMDD_PID_FILE "/tmp/anita/pid/cmdd.pid"
#define CALIBD_PID_FILE "/tmp/anita/pid/calibd.pid"
#define EVENTD_PID_FILE "/tmp/anita/pid/eventd.pid"
#define GPSD_PID_FILE "/tmp/anita/pid/gpsd.pid"
#define HKD_PID_FILE "/tmp/anita/pid/hkd.pid"
#define LOSD_PID_FILE "/tmp/anita/pid/losd.pid"
#define MONITORD_PID_FILE "/tmp/anita/pid/monitord.pid"
#define PRIORITIZERD_PID_FILE "/tmp/anita/pid/prioritizerd.pid"
#define SIPD_PID_FILE "/tmp/anita/pid/sipd.pid"
#define PLAYBACKD_PID_FILE "/tmp/anita/pid/playbackd.pid"
#define LOGWATCHD_PID_FILE "/tmp/anita/pid/logwatchd.pid"

#define BASE_PRIORITY_PURGE_DIR "/mnt/data/anita/purged"


//Archiving Directories
#define CMD_ARCHIVE_DIR "/current/house/cmd"
#define CALIB_ARCHIVE_DIR "/current/house/calib"
#define HK_ARCHIVE_DIR "/current/house/hk"
#define GPS_ARCHIVE_DIR "/current/house/gps"
#define SURFHK_ARCHIVE_DIR "/current/house/surfhk"
#define TURFHK_ARCHIVE_DIR "/current/house/turfhk"
#define MONITOR_ARCHIVE_DIR "/current/house/monitor"

#define EVENT_ARCHIVE_DIR "/current/event/"

//Slow Rate Stuff
#define SLOW_RF_FILE "/tmp/anita/latestSlowRf.dat"
#define RUN_START_FILE "/tmp/anita/latestRunStart.dat"

//Hk Power Lookup
#define HK_POWER_LOOKUP "/tmp/anita/currentPower"

#endif /* ANITA_FLIGHT_H */

