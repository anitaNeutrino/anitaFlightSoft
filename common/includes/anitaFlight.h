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

#define ACTIVE_SURFS 9
#define MAX_SURFS 12
#define CHANNELS_PER_SURF 9
#define LABRADORS_PER_SURF 4 /*jjb 2006-04-19 */
#define NUM_DAC_CHANS 4
#define SCALERS_PER_SURF 32
#define RFCHAN_PER_SURF 8
#define NUM_DIGITZED_CHANNELS ACTIVE_SURFS*CHANNELS_PER_SURF
#define MAX_NUMBER_SAMPLES 260
#define ADC_MAX 4096
#define TRIGGER_OFFSET 64 /* Made up number for where the trigger is */
#define PED_DEFAULT_VALUE 1100


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

#define G12A_DEV_NAME "/dev/ttyUSB0"
#define G12B_DEV_NAME "/dev/ttyUSB3"
#define ADU5A_DEV_NAME "/dev/ttyUSB2"
#define MAGNETOMETER_DEV_NAME "/dev/ttyUSB1"

#define NUM_PRIORITIES 10
#define NUM_USBDISKS 64

#define GLOBAL_CONF_FILE "anitaSoft.config"

#ifdef USE_FAKE_DATA_DIR
#define ANITA_INDEX_DIR "/tmp/mnt/data/anita/index"
#define EVENT_LINK_INDEX "/tmp/mnt/data/anita/index/event.ind"
#define GPS_INDEX "/tmp/mnt/data/anita/index/gps.ind"
#define HK_INDEX "/tmp/mnt/data/anita/index/hk.ind"
#define SURF_HK_INDEX "/tmp/mnt/data/anita/index/surfHk.ind"
#define TURF_HK_INDEX "/tmp/mnt/data/anita/index/turfHk.ind"
#define MONITOR_INDEX "/tmp/mnt/data/anita/index/monitor.ind"
#define CMD_INDEX "/tmp/mnt/data/anita/index/cmd.ind"

#define MAIN_DATA_DISK_LINK "/tmp/mnt/dataCurrent"
#define BACKUP_DATA_DISK_LINK "/tmp/mnt/dataBackup"

//Pedestal Dirs
#define PEDESTAL_DIR "/tmp/mnt/data/anita/pedestal"
#define CURRENT_PEDESTALS "/tmp/mnt/data/anita/pedestal/current"
#else
#define ANITA_INDEX_DIR "/mnt/data/anita/index"
#define EVENT_LINK_INDEX "/mnt/data/anita/index/event.ind"
#define GPS_INDEX "/mnt/data/anita/index/gps.ind"
#define HK_INDEX "/mnt/data/anita/index/hk.ind"
#define SURF_HK_INDEX "/mnt/data/anita/index/surfHk.ind"
#define TURF_HK_INDEX "/mnt/data/anita/index/turfHk.ind"
#define MONITOR_INDEX "/mnt/data/anita/index/monitor.ind"
#define CMD_INDEX "/mnt/data/anita/index/cmd.ind"

#define MAIN_DATA_DISK_LINK "/mnt/dataCurrent"
#define BACKUP_DATA_DISK_LINK "/mnt/dataBackup"

//Pedestal Dirs
#define PEDESTAL_DIR "/mnt/data/anita/pedestal"
#define CURRENT_PEDESTALS "/mnt/data/anita/pedestal/current"
#endif


#define OTHER_DISK1 "/mnt/blade1"
#define OTHER_DISK2 "/mnt/data"
#define OTHER_DISK3 "/mnt/blade3"
#define OTHER_DISK4 "/mnt/blade4"

#define MAX_EVENT_SIZE NUM_DIGITZED_CHANNELS*MAX_NUMBER_SAMPLES*4

#define HK_PER_FILE 1000
#define HK_FILES_PER_DIR 100

#define EVENTS_PER_FILE 100
#define EVENT_FILES_PER_DIR 100



//Housekeeping Telem Dirs
#define CMD_ECHO_TELEM_DIR "/tmp/anita/telem/house/cmd"
#define HK_TELEM_DIR "/tmp/anita/telem/house/hk"
#define SURFHK_TELEM_DIR "/tmp/anita/telem/house/surfhk"
#define TURFHK_TELEM_DIR "/tmp/anita/telem/house/turfhk"
#define GPS_TELEM_DIR "/tmp/anita/telem/house/gps"
#define MONITOR_TELEM_DIR "/tmp/anita/telem/house/monitor"
#define HEADER_TELEM_DIR "/tmp/anita/telem/head"

#define CMD_ECHO_TELEM_LINK_DIR "/tmp/anita/telem/house/cmd/link"
#define HK_TELEM_LINK_DIR "/tmp/anita/telem/house/hk/link"
#define SURFHK_TELEM_LINK_DIR "/tmp/anita/telem/house/surfhk/link"
#define TURFHK_TELEM_LINK_DIR "/tmp/anita/telem/house/turfhk/link"
#define GPS_TELEM_LINK_DIR "/tmp/anita/telem/house/gps/link"
#define MONITOR_TELEM_LINK_DIR "/tmp/anita/telem/house/monitor/link"
#define HEADER_TELEM_LINK_DIR "/tmp/anita/telem/head/link"

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



#endif /* ANITA_FLIGHT_H */

