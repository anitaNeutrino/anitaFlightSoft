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


/* Hardware stuff -- might move to a config file */
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


#define CHANS_PER_IP320 40
#define NUM_IP320_BOARDS 3


#define NUM_PRIORITIES 10
#define NUM_USBDISKS 64

#define GLOBAL_CONF_FILE "anitaSoft.config"

#define EVENT_LINK_INDEX "/mnt/data/index/event"
#define GPS_INDEX "/mnt/data/index/gps"
#define HK_INDEX "/mnt/data/index/hk"
#define SURF_HK_INDEX "/mnt/data/index/surfHk"
#define TURF_HK_INDEX "/mnt/data/index/turfHk"
#define MONITOR_INDEX "/mnt/data/index/monitor"
#define CMD_INDEX "/mnt/data/index/cmd"

#define MAIN_DATA_DISK_LINK "/mnt/dataCurrent"
#define BACKUP_DATA_DISK_LINK "/mnt/dataBackup"

#define MAX_EVENT_SIZE NUM_DIGITZED_CHANNELS*MAX_NUMBER_SAMPLES*4

#endif /* ANITA_FLIGHT_H */

