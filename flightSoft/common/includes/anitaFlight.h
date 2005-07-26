/*! \file anitaFlight.h
    \brief The standard ANITA include file.
    
    It will end up including all those definitions that are needed 
    all over the shop. Hopefully there won't be too many of these floating 
    around.
    August 2004  rjn@mps.ohio-state.edu
*/
#ifndef ANITA_FLIGHT_H
#define ANITA_FLIGHT_H

#include <syslog.h>

#define ANITA_LOG_FACILITY LOG_LOCAL4

/* Hardware stuff -- might move to a config file */
#define ACTIVE_SURFS 3
#define MAX_SURFS 12
#define CHANNELS_PER_SURF 8
#define NUM_DIGITZED_CHANNELS ACTIVE_SURFS*CHANNELS_PER_SURF
#define MAX_NUMBER_SAMPLES 256
#define ADC_MAX 4096
#define TRIGGER_OFFSET 64 /* Made up number for where the trigger is */

#define GLOBAL_CONF_FILE "anitaSoft.config"


    

#endif /* ANITA_FLIGHT_H */

