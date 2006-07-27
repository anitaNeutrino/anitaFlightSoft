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

#define MAX_SATS 12
#define MAX_CMD_LENGTH 20

#define SetVerId(A) (0xfe | (((A)&0xff)<<8))
#define GetChanIndex(A,B) ((B)+(9*(A)))

#ifdef SLAC_DATA06
//SLAC data definitions
#define VER_EVENT_HEADER 7
#define VER_WAVE_PACKET 4
#define VER_SURF_PACKET 4
#define VER_ENC_WAVE_PACKET 4
#define VER_ENC_SURF_PACKET 4
#define VER_SURF_HK 5
#define VER_ADU5_PAT 4
#define VER_ADU5_SAT 4
#define VER_ADU5_VTG 4
#define VER_G12_POS 4
#define VER_G12_SAT 4
#define VER_HK_FULL 6
#define VER_CMD_ECHO 4
#define VER_MONITOR 5
#define VER_TURF_RATE 6
#define VER_LAB_PED 1
#define VER_FULL_PED 1
#define VER_SLOW_1 1
#define VER_SLOW_2 1
#else
//Current ones
#define VER_EVENT_HEADER 7
#define VER_WAVE_PACKET 6
#define VER_SURF_PACKET 6
#define VER_ENC_WAVE_PACKET 6
#define VER_ENC_SURF_PACKET 6
#define VER_SURF_HK 5
#define VER_ADU5_PAT 4
#define VER_ADU5_SAT 4
#define VER_ADU5_VTG 4
#define VER_G12_POS 4
#define VER_G12_SAT 4
#define VER_HK_FULL 6
#define VER_CMD_ECHO 4
#define VER_MONITOR 5
#define VER_TURF_RATE 6
#define VER_LAB_PED 1
#define VER_FULL_PED 1
#define VER_SLOW_1 1
#define VER_SLOW_2 1
#endif

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

#define SS1_MASK 0xf000
#define SS1_SHIFT 12

#define WAKEUP_LOS_BUFFER_SIZE 4000
#define WAKEUP_TDRSS_BUFFER_SIZE 500
#define WAKEUP_LOW_RATE_BUFFER_SIZE 100


//Enumerations
typedef enum {
    PACKET_HD = 0x100,
    PACKET_WV = 0x101, //Too big to telemeter
    PACKET_SURF = 0x102,
    PACKET_SURF_HK = 0x110,
    PACKET_TURF_RATE = 0x111,
    PACKET_ENC_WV = 0x120,
    PACKET_ENC_SURF = 0x121,
    PACKET_ENC_SURF_PEDSUB = 0x122,
    PACKET_ENC_EVENT_WRAPPER = 0x123,
    PACKET_PED_SUBBED_EVENT = 0x124,
    PACKET_LAB_PED = 0x130,
    PACKET_FULL_PED = 0x131, //Too big to telemeter
    PACKET_GPS_ADU5_PAT = 0x200,
    PACKET_GPS_ADU5_SAT = 0x201,
    PACKET_GPS_ADU5_VTG = 0x202,
    PACKET_GPS_G12_POS = 0x203,
    PACKET_GPS_G12_SAT = 0x204,
    PACKET_HKD = 0x300,
    PACKET_CMD_ECHO = 0x400,
    PACKET_MONITOR = 0x500,
    PACKET_WAKEUP_LOS = 0x600,
    PACKET_WAKEUP_HIGHRATE = 0x601,
    PACKET_WAKEUP_COMM1 = 0x602,
    PACKET_WAKEUP_COMM2 = 0x603,
    PACKET_SLOW1 = 0x700,
    PACKET_SLOW2 = 0x800    
} PacketCode_t;

typedef enum {
    kNoEncoding=0
} EventEncodingScheme_t;

#ifdef SLAC_DATA06   
typedef enum {
    ENCODE_NONE=0, //Done
    ENCODE_SOMETHING=0x100 //Done
} ChannelEncodingType_t; 
#else
typedef enum {
    ENCODE_NONE=0, //Done
    ENCODE_LOSSLESS_12BIT=0x100, //Done
    ENCODE_LOSSLESS_BINARY, //Done
    ENCODE_LOSSLESS_FIBONACCI, //Done
    ENCODE_LOSSLESS_BINFIB_COMBO, //Done
    ENCODE_LOSSY_BINARY=0x200,
    ENCODE_LOSSY_MULAW
} ChannelEncodingType_t;
#endif

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


typedef enum {
    IP320_RAW=0x100,
    IP320_AVZ=0x200,
    IP320_CAL=0x300
} AnalogueCode_t;



//Structures
typedef struct {
    unsigned long pedUnixTime;
    ChannelEncodingType_t encTypes[ACTIVE_SURFS][CHANNELS_PER_SURF];
} EncodeControlStruct_t;

typedef struct {
    PacketCode_t code;    
    unsigned long packetNumber; //Especially for Ped
    unsigned short numBytes;
    unsigned char feByte;
    unsigned char verId;
    unsigned long checksum;
} GenericHeader_t;


typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned long lastEventNumber;
    float latitude;
    float longitude;
    float altitude;
    unsigned short sbsTemp[2];
} SlowRateType1_t;

typedef struct {
    unsigned char trigType; //Trig type bit masks
    unsigned char l3Type1Count; //L3 counter
    unsigned short trigNum; //turf trigger counter
    unsigned long trigTime;
    unsigned long ppsNum;     // 1PPS
    unsigned long c3poNum;     // 1 number of trigger time ticks per PPS
    unsigned short upperL1TrigPattern;
    unsigned short lowerL1TrigPattern;
    unsigned short upperL2TrigPattern;
    unsigned short lowerL2TrigPattern;
    unsigned short l3TrigPattern;
    unsigned short l3TrigPattern2;
} TurfioStruct_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned long unixTimeUs;    
    unsigned short l1Rates[TRIGGER_SURFS][ANTS_PER_SURF]; // 3 of 8 counters
    unsigned char upperL2Rates[PHI_SECTORS];
    unsigned char lowerL2Rates[PHI_SECTORS];
    unsigned char l3Rates[PHI_SECTORS];
} TurfRateStruct_t;
    
#ifdef SLAC_DATA06
typedef struct {
    unsigned char chanId;   // chan+9*surf
    unsigned char chipIdFlag; // Bits 0,1 chipNum; Bit 3 hitBus wrap; 4-7 hitBusOff
    unsigned char firstHitbus;
    unsigned char lastHitbus;
    float mean; //Filled by Prioritizerd
    float rms; //Filled by Prioritizerd

} RawSurfChannelHeader_t;
#else
typedef struct {
    unsigned char chanId;   // chan+9*surf
    unsigned char chipIdFlag; // Bits 0,1 chipNum; Bit 3 hitBus wrap; 4-7 hitBusOff
    unsigned char firstHitbus; // If wrappedHitbus=0 data runs, lastHitbus+1
    unsigned char lastHitbus; //to firstHitbus-1 inclusive
    //Otherwise it runs from firstHitbus+1 to lastHitbus-1 inclusive
} RawSurfChannelHeader_t;
#endif

typedef struct {
    RawSurfChannelHeader_t rawHdr;
    ChannelEncodingType_t encType;
    unsigned short numBytes;
    unsigned short crc;
} EncodedSurfChannelHeader_t;


typedef struct {
    RawSurfChannelHeader_t header;
    unsigned short data[MAX_NUMBER_SAMPLES];
} SurfChannelFull_t;

typedef struct {
    RawSurfChannelHeader_t header;
    short xMax;
    short xMin;
    float mean; //Filled by pedestalLib
    float rms; //Filled by pedestalLib
    short data[MAX_NUMBER_SAMPLES];
} SurfChannelPedSubbed_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;       /* unix UTC sec*/
    unsigned long unixTimeUs;     /* unix UTC microsec */
    unsigned long gpsSubTime;     /* the GPS fraction of second (in ns) 
			   (for the X events per second that get 
			   tagged with it */
    unsigned long eventNumber;    /* Global event number */
    unsigned short surfMask;
    unsigned short calibStatus;   /* Were we flashing the pulser? */
    unsigned char priority; // priority and other
    unsigned char turfUpperWord; // The upper 8 bits from the TURF
    unsigned char otherFlag; //Currently unused 
    unsigned char otherFlag2; //Currently unused 
    unsigned long antTrigMask; // What was the ant trigger mask
    TurfioStruct_t turfio; /*The X byte TURFIO data*/
} AnitaEventHeader_t;


typedef struct {
    GenericHeader_t gHdr;
    unsigned long eventNumber;    /* Global event number */
    SurfChannelFull_t channel[NUM_DIGITZED_CHANNELS];
} AnitaEventBody_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long eventNumber;    /* Global event number */
    unsigned long whichPeds; //whichPedestals did we subtract
    SurfChannelPedSubbed_t channel[NUM_DIGITZED_CHANNELS];
} PedSubbedEventBody_t;



/* these are syntactic sugar to help us keep track of bit shifts */
typedef int Fixed3_t; /*rescaled integer left shifted 3 bits */
typedef int Fixed6_t; /*rescaled integer left shifted 6 bits */
typedef int Fixed8_t; /*rescaled integer left shifted 8 bits */


/*    FOR THREE STRUCTS THAT FOLLOW
      valid samples==-1 prior to unwinding 
*/

typedef struct {
     Fixed3_t data[MAX_NUMBER_SAMPLES];
     Fixed3_t baseline;
     short valid_samples;
} TransientChannel3_t;

typedef struct {
     Fixed6_t data[MAX_NUMBER_SAMPLES];
     short valid_samples;
} TransientChannel6_t;

typedef struct {
     Fixed8_t data[MAX_NUMBER_SAMPLES];
     Fixed8_t baseline;
     short valid_samples;
} TransientChannel8_t;

typedef struct {
     TransientChannel3_t ch[NUM_DIGITZED_CHANNELS]; 
} AnitaTransientBody3_t; /* final corrected transient type 
			    used to calculate power */

typedef struct {
     TransientChannel6_t ch[NUM_DIGITZED_CHANNELS]; 
} AnitaPowerBody6_t; /* power from squaring an AnitaTransientBody3 */

typedef struct {
     TransientChannel8_t ch[NUM_DIGITZED_CHANNELS]; 
} AnitaTransientBody8_t; /* used for pedestal subtraction, unwrapping, 
			    averaging, and gain correction */
typedef struct {
     TransientChannel6_t S0,S1,S2,S3;
} AnitaStokes6_t;


typedef struct {
    AnitaEventHeader_t header;
    AnitaEventBody_t body;
} AnitaEventFull_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long eventNumber;
    SurfChannelFull_t waveform;
} RawWaveformPacket_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long eventNumber;
    SurfChannelFull_t waveform[CHANNELS_PER_SURF];
} RawSurfPacket_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long eventNumber;
    unsigned long whichPeds;
    EncodedSurfChannelHeader_t chanHead;
} EncodedWaveformPacket_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long eventNumber;
} EncodedSurfPacketHeader_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long whichPeds;
    unsigned long eventNumber;
} EncodedPedSubbedSurfPacketHeader_t;


typedef struct {
    GenericHeader_t gHdr;
    unsigned long eventNumber;
    unsigned long numBytes;
} EncodedEventWrapper_t;

typedef struct {
    unsigned long unixTime;
    unsigned long subTime;
    int fromAdu5; //1 is yes , 0 is g12
} GpsSubTime_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned long unixTimeUs;
    unsigned long timeOfDay;
    float heading;
    float pitch;
    float roll;
    float mrms;
    float brms;
    float latitude;
    float longitude;
    float altitude;
    unsigned long attFlag;
} GpsAdu5PatStruct_t;

typedef struct {
    unsigned char prn;
    unsigned char elevation;
    unsigned char snr;
    unsigned char flag; 
    unsigned short azimuth;
} GpsSatInfo_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned long numSats;
    GpsSatInfo_t sat[MAX_SATS];
} GpsG12SatStruct_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned char numSats[4];
    GpsSatInfo_t sat[4][MAX_SATS];
} GpsAdu5SatStruct_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned long unixTimeUs;
    float trueCourse;
    float magneticCourse;
    float speedInKnots;
    float speedInKPH;
} GpsAdu5VtgStruct_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned long unixTimeUs;
    unsigned long timeOfDay;
    unsigned long numSats;
    float latitude;
    float longitude;
    float altitude;
    float trueCourse;
    float verticalVelocity;
    float speedInKnots;
    float pdop;
    float hdop;
    float vdop;
    float tdop;
} GpsG12PosStruct_t;

typedef struct {
    unsigned long unixTime;
    unsigned long status;
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
    unsigned short temp[2];
} SBSTemperatureDataStruct_t;

typedef struct {
    float x;
    float y;
    float z;
} MagnetometerDataStruct_t;

typedef struct {    
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned long unixTimeUs;
    FullAnalogueStruct_t ip320;
    MagnetometerDataStruct_t mag;
    SBSTemperatureDataStruct_t sbs;
} HkDataStruct_t;

typedef struct {    
    unsigned short threshold;
    unsigned short scaler[ACTIVE_SURFS][SCALERS_PER_SURF];
} SimpleScalerStruct_t;

typedef struct { 
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned long unixTimeUs;
    unsigned short globalThreshold; //set to zero if there isn't one
    unsigned short errorFlag; //Will define at some point    
    unsigned short upperWords[ACTIVE_SURFS];
    unsigned short scaler[ACTIVE_SURFS][SCALERS_PER_SURF];
    unsigned short threshold[ACTIVE_SURFS][SCALERS_PER_SURF];
    unsigned short rfPower[ACTIVE_SURFS][RFCHAN_PER_SURF];
    unsigned short surfTrigBandMask[ACTIVE_SURFS][2];
} FullSurfHkStruct_t;

typedef struct {    
    unsigned char numCmdBytes;
    unsigned char cmd[MAX_CMD_LENGTH];
} CommandStruct_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;
    unsigned short goodFlag; // 0 is bad, 1 is good
    unsigned short numCmdBytes; // number of cmd bytes (upto 10)
    unsigned char cmd[MAX_CMD_LENGTH]; // the cmd bytes
} CommandEcho_t;

typedef struct {
    unsigned short mainDisk; //In MegaBytes
    unsigned short otherDisks[5]; //In MegaBytes
    unsigned short usbDisk[NUM_USBDISKS]; //In MegaBytes
} DiskSpaceStruct_t;

typedef struct {
    unsigned short eventLinks[NUM_PRIORITIES]; //10 Priorities
    unsigned short cmdLinks;
    unsigned short headLinks;
    unsigned short gpsLinks;
    unsigned short hkLinks;
    unsigned short monitorLinks;
    unsigned short surfHkLinks;
    unsigned short turfHkLinks;
    unsigned short forcedLinks;
} QueueStruct_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTime;
    DiskSpaceStruct_t diskInfo;
    QueueStruct_t queueInfo;
} MonitorStruct_t;

typedef struct {
    unsigned char chanId;   // chan+9*surf
    unsigned char chipId; // 0-3
    unsigned short chipEntries;
    unsigned short pedMean[MAX_NUMBER_SAMPLES]; // actual value
    unsigned char pedRMS[MAX_NUMBER_SAMPLES]; //times 10
} LabChipChannelPedStruct_t;

typedef struct {
    GenericHeader_t gHdr;
    unsigned long unixTimeStart;
    unsigned long unixTimeEnd;
    LabChipChannelPedStruct_t pedChan[CHANNELS_PER_SURF];
} FullLabChipPedStruct_t;

typedef struct {
    unsigned long unixTimeStart;
    unsigned long unixTimeEnd;
    LabChipChannelPedStruct_t pedChan[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF];
} FullPedStruct_t;

typedef struct {
    unsigned long unixTimeStart;
    unsigned long unixTimeEnd;
    unsigned short chipEntries[ACTIVE_SURFS][LABRADORS_PER_SURF];
    int mean[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
    int meanSq[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
    int entries[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
    float fmean[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
    float frms[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
} PedCalcStruct_t;


typedef struct {
    unsigned long unixTime; // Corresponds to unixTimeEnd above
    unsigned long nsamples; // What was the mean occupancy
    unsigned short thePeds[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES]; /* mean pedestal */
    unsigned short pedsRMS[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES]; 
    /* 10 x RMS of the samples (not of mean)*/
} PedestalStruct_t;

#endif /* ANITA_STRUCTURES_H */
