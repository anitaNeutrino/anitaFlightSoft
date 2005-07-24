/*! \file anitaStructures.h
    \brief Contains the definitions of all the structures used to store 
    and move ANITA data and stuff.
    
    Some simple definitions of the structures that we'll use to store 
    ANITA data. In here there will be the structures used for transient data,
    housekeeping stuff, GPS stuff and anything else that is going to be used
    by more than one of the processes. This will all change to use bit fields,
    at some point in the future. For now this is just a place holder we can
    test data flow with.

    August 2004  rjn@mps.ohio-state.edu
*/
#ifndef ANITA_STRUCTURES_H
#define ANITA_STRUCTURES_H
#include "includes/anitaFlight.h"



typedef struct {
    short ch_id;      /* does exactly what it says on the tin */
    short ch_fs;      /* full scale (maybe mV)*/
    short ch_offs;    /* offset in ADCs */
    char  ch_couple;  /* flag for type of coupling */
    char  ch_bw;      /* flag for type of bw limit */
    signed char ch_mean;      /*  mean, percent of full scale */
    unsigned char ch_sdev;    /*  std deviation, percent of full scale */
} channel_header;


typedef struct {
    channel_header header;
    short data[MAX_NUMBER_SAMPLES];
} channel_full;


typedef struct {
    int eventNumber;    /* Global event number */
    int unixTime;       /* unix UTC*/
    int gpsSubTime;     /* the GPS fraction of second (in ns) */
    short numChannels;  /* In case we turn part of array off, or whatever. */
    short numSamples;   /* total number of samples per waveform == 256*/
    unsigned short sampInt; /* sample interval in units of 10 ps */
    short trigDelay;      /* signed delay in number of samples*/
    char holdOff;      /* number of 100ms intervals to hold between triggers */
    char clocktype;    /* start/stop external=4, external=2, continuous ext.=1, internal=0 */
    char Dtype;         /* +:triggered; -:timeout, abs(Dtype)= #10sec segs before trigger */
    char trigCouple;    /* flag for type of coupling, 0=DC, 1=AC */
    char trigSlope;     /* flag for trigger slope, pos.=0, neg.=1 */
    signed char trigSource; /* flag for trigger source, -1=ext 1, -2=ext 2, N=chan N */
    short trigLevel;    /* trigger level in 0.5 mV increments */
    char calibStatus;   /* Were we calibrating? */
    char priority;
} anita_event_header;

typedef struct {
    channel_full channel[NUM_DIGITZED_CHANNELS];
} anita_event_body;

typedef struct {
    anita_event_header header;
    anita_event_body body;
} anita_event_full;

typedef struct {
    int unixTime;
    int subTime;
} gpsSubTimeStruct;

typedef struct {
    int unixTime;
    char status;
} calibStruct;


typedef enum {
    PRI_FORCED = 0,
    PRI_CALIB = 1,
    PRI_1 = 1,
    PRI_2,
    PRI_3,
    PRI_4,
    PRI_TIMEOUT,
    PRI_6, 
    PRI_7,
    PRI_8,
    PRI_PAYLOAD
} PriorityCode;


// TURFIO data structure, TV test
typedef struct {
  unsigned short rawtrig; // raw trigger number
  unsigned short L3Atrig; // same as raw trig
  unsigned long pps1;     // 1PPS
  unsigned long time;     // trig time
  unsigned long interval; // interval since last trig
  unsigned int rate[2][4]; // Antenna trigger output rates (Hz)
} TurfioStruct;
  


#endif /* ANITA_STRUCTURES_H */
