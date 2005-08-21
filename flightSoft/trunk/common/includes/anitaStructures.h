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

#define MAX_SATS 30

typedef enum {
    PACKET_HD = 0x100,
    PACKET_WV = 0x101,
    PACKET_SURF = 0x102,
    PACKET_GPSD_PAT = 0x200,
    PACKET_GPSD_SAT = 0x201,
    PACKET_HKD = 0x300
} PacketCode_t;

typedef struct {
    PacketCode_t code;
} GenericHeader_t;

// TURFIO data structure, TV test
/* typedef struct { */
/*     unsigned short rawtrig; // raw trigger number */
/*     unsigned short L3Atrig; // same as raw trig */
/*     unsigned long pps1;     // 1PPS */
/*     unsigned long time;     // trig time */
/*     unsigned long interval; // interval since last trig */
/*     unsigned int rate[2][4]; // Antenna trigger output rates (Hz) */
/* } TurfioStruct_t; */

/* typedef struct { */
/*     unsigned char otherBits; */
/*     unsigned char trigType; */
/*     unsigned char trigNumByte1; */
/*     unsigned char trigNumByte2; */
/*     unsigned char trigNumByte3; */
/*     unsigned char trigTimeByte1; */
/*     unsigned char trigTimeByte2; */
/*     unsigned char trigTimeByte3; */
/*     unsigned char trigTimeByte4; */
/*     unsigned char ppsNumByte1; */
/*     unsigned char ppsNumByte2; */
/*     unsigned char ppsNumByte3; */
/*     unsigned char ppsNumByte4; */
/*     unsigned char c3poByte1; */
/*     unsigned char c3poByte2; */
/*     unsigned char c3poByte3; */
/*     unsigned char c3poByte4; */
/* } TurfioStruct_t; */
    
  
typedef struct {
    unsigned char otherBits;
    unsigned char trigType;
    unsigned long trigNum;
    unsigned long trigTime;
    unsigned long ppsNum;
    unsigned long c3poNum;
} TurfioStruct_t;
    

typedef struct {
    short ch_id;      /* does exactly what it says on the tin */
    short ch_fs;      /* full scale (maybe mV)*/
    short ch_offs;    /* offset in ADCs */
    char  ch_couple;  /* flag for type of coupling */
    char  ch_bw;      /* flag for type of bw limit */
    signed char ch_mean;      /*  mean, percent of full scale */
    unsigned char ch_sdev;    /*  std deviation, percent of full scale */
} SurfChannelHeader_t;


typedef struct {
//    SurfChannelHeader_t header;
    unsigned short data[MAX_NUMBER_SAMPLES];
} SurfChannelFull_t;


typedef struct {
    GenericHeader_t gHdr;
    int unixTime;       /* unix UTC sec*/
    int unixTimeUs;     /* unix UTC microsec */
    int gpsSubTime;     /* the GPS fraction of second (in ns) (for the one event per second that gets tagged with it */
    int eventNumber;    /* Global event number */
    short numChannels;  /* In case we turn part of array off, or whatever. */
    short numSamples;   /* total number of samples per waveform == 256*/
    char calibStatus;   /* Were we flashing the pulser? */
    TurfioStruct_t turfio; /*The 32 byte TURFIO data*/
    char priority;
} AnitaEventHeader_t;

typedef struct {
    SurfChannelFull_t channel[NUM_DIGITZED_CHANNELS];
} AnitaEventBody_t;

typedef struct {
    AnitaEventHeader_t header;
    AnitaEventBody_t body;
} AnitaEventFull_t;

typedef struct {
    GenericHeader_t gHdr;
    int eventNumber;
    char packetNumber; //0 to 23;
    SurfChannelFull_t waveform;
} WaveformPacket_t;


typedef struct {
    GenericHeader_t gHdr;
    int eventNumber;
    char packetNumber; //0 to 1;
    SurfChannelFull_t waveform[CHANNELS_PER_SURF];
} SurfPacket_t;

typedef struct {
    int unixTime;
    int subTime;
} GpsSubTime_t;

typedef struct {
    GenericHeader_t gHdr;
    long unixTime;
    long timeOfDay;
    double heading;
    double pitch;
    double roll;
    double mrms;
    double brms;
    char attFlag;
    double latitude;
    double longitude;
    double altitude;
} GpsPatStruct_t;

typedef struct {
    int prn;
    int azimuth;
    int elevation;
    int snr;
    int flag; //1 is usable, 0 is not
} GpsSatInfo_t;

typedef struct {
    GenericHeader_t gHdr;
    long unixTime;
    int numSats;
    GpsSatInfo_t sat[MAX_SATS];
} GpsSatStruct_t;


typedef enum {
    IP320_RAW=0x100,
    IP320_AVZ=0x200,
    IP320_CAL=0x300
} AnalogueCode_t;

typedef struct {
    int unixTime;
    char status;
} CalibStruct_t;

typedef struct {
    unsigned short data[CHANS_PER_IP320];
} AnalogueDataStruct_t;

typedef struct {
    long data[CHANS_PER_IP320];
} AnalogueCorrectedDataStruct_t;

typedef struct {
    AnalogueCode_t code;
    AnalogueDataStruct_t board[NUM_IP320_BOARDS];
} FullAnalogueStruct_t;

typedef struct {
    int temp[2];
} SBSTemperatureDataStruct_t;

typedef struct {
    double x;
    double y;
    double z;
} MagnetometerDataStruct_t;

typedef struct {    
    GenericHeader_t gHdr;
    long unixTime;
    FullAnalogueStruct_t ip320;
    MagnetometerDataStruct_t mag;
    SBSTemperatureDataStruct_t sbs;
} HkDataStruct_t;

typedef struct {    
    unsigned short threshold;
    unsigned short scaler[ACTIVE_SURFS][SCALERS_PER_SURF];
} SimpleScalerStruct_t;


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




#endif /* ANITA_STRUCTURES_H */
