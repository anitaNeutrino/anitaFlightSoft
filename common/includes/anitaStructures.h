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

#define SetVerId(A) (0xfe | (((A)&0xff)<<8))
#define GetChanIndex(A,B) ((B)+(CHANNELS_PER_SURF*(A)))

#ifdef SLAC_DATA06
//SLAC data definitions
#define VER_EVENT_BODY 7
#define VER_PEDSUBBED_EVENT_BODY 7
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
#define VER_SLOW_FULL 1
#define VER_SLOW_1 1
#define VER_SLOW_2 1
#elif ANITA_1_DATA
//ANITA 1 (2006/7) Data
#define VER_EVENT_BODY 7
#define VER_PEDSUBBED_EVENT_BODY 7
#define VER_EVENT_HEADER 9
#define SLAC_VER_EVENT_HEADER 7
#define VER_WAVE_PACKET 6
#define VER_SURF_PACKET 6
#define VER_ENC_WAVE_PACKET 6
#define VER_ENC_SURF_PACKET 6
#define VER_SURF_HK 9
#define VER_ADU5_PAT 4
#define VER_ADU5_SAT 4
#define VER_ADU5_VTG 4
#define VER_G12_POS 4
#define VER_G12_SAT 4
#define VER_HK_FULL 6
#define VER_CMD_ECHO 4
#define VER_MONITOR 7
#define VER_TURF_RATE 6
#define VER_LAB_PED 1
#define VER_FULL_PED 1
#define VER_SLOW_1 1
#define VER_SLOW_2 1
#define VER_SLOW_FULL 1
#define VER_ZIPPED_FILE 1
#define VER_ZIPPED_PACKET 1
#define VER_RUN_START 1
#define VER_OTHER_MON 1
#elif ANITA_2_DATA
#define VER_EVENT_BODY 11
#define VER_PEDSUBBED_EVENT_BODY 11
#define VER_EVENT_HEADER 13
#define SLAC_VER_EVENT_HEADER 10
#define VER_WAVE_PACKET 10
#define VER_SURF_PACKET 10
#define VER_ENC_WAVE_PACKET 10
#define VER_ENC_SURF_PACKET 10
#define VER_SURF_HK 14
#define VER_GPS_GGA 10
#define VER_ADU5_PAT 10
#define VER_ADU5_SAT 10
#define VER_ADU5_VTG 10
#define VER_G12_POS 10
#define VER_G12_SAT 10
#define VER_HK_FULL 10
#define VER_CMD_ECHO 10
#define VER_MONITOR 10
#define VER_TURF_RATE 16
#define VER_LAB_PED 10
#define VER_FULL_PED 10
#define VER_SLOW_1 10
#define VER_SLOW_2 10
#define VER_SLOW_FULL 10
#define VER_ZIPPED_FILE 10
#define VER_ZIPPED_PACKET 10
#define VER_RUN_START 10
#define VER_OTHER_MON 10
#define VER_GPSD_START 10
#define VER_LOGWATCHD_START 10
#define VER_AVG_SURF_HK 14
#define VER_SUM_TURF_RATE 16
#define VER_ACQD_START 11
#define VER_TURF_REG 10
#elif ANITA_3_DATA
#define VER_EVENT_BODY 30
#define VER_PEDSUBBED_EVENT_BODY 30
#define VER_EVENT_HEADER 33
#define SLAC_VER_EVENT_HEADER 30
#define VER_WAVE_PACKET 30
#define VER_SURF_PACKET 30
#define VER_ENC_WAVE_PACKET 30
#define VER_ENC_SURF_PACKET 30
#define VER_SURF_HK 31
#define VER_GPS_GGA 30
#define VER_ADU5_PAT 30
#define VER_ADU5_SAT 30
#define VER_ADU5_VTG 30
#define VER_G12_POS 30
#define VER_G12_SAT 30
#define VER_HK_FULL 30
#define VER_HK_SS 30
#define VER_CMD_ECHO 30
#define VER_MONITOR 31
#define VER_TURF_RATE 35
#define VER_LAB_PED 30
#define VER_FULL_PED 30
#define VER_SLOW_1 30
#define VER_SLOW_2 30
#define VER_SLOW_FULL 30
#define VER_ZIPPED_FILE 30
#define VER_ZIPPED_PACKET 30
#define VER_RUN_START 30
#define VER_OTHER_MON 30
#define VER_GPSD_START 30
#define VER_LOGWATCHD_START 30
#define VER_AVG_SURF_HK 31
#define VER_SUM_TURF_RATE 34
#define VER_ACQD_START 32
#define VER_TURF_REG 30
#define VER_TURF_EVENT_DATA 30
#define VER_GPU_POW_SPEC 30
#else //ANITA_4_DATA
#define VER_EVENT_BODY 40
#define VER_PEDSUBBED_EVENT_BODY 40
#define VER_EVENT_HEADER 40
#define SLAC_VER_EVENT_HEADER 40
#define VER_WAVE_PACKET 40
#define VER_SURF_PACKET 40
#define VER_ENC_WAVE_PACKET 40
#define VER_ENC_SURF_PACKET 40
#define VER_SURF_HK 40
#define VER_GPS_GGA 40
#define VER_ADU5_PAT 40
#define VER_ADU5_SAT 40
#define VER_ADU5_VTG 40
#define VER_G12_POS 40
#define VER_G12_SAT 40
#define VER_HK_FULL 40
#define VER_HK_SS 40
#define VER_CMD_ECHO 40
#define VER_MONITOR 40
#define VER_TURF_RATE 40
#define VER_LAB_PED 40
#define VER_FULL_PED 40
#define VER_SLOW_1 40
#define VER_SLOW_2 40
#define VER_SLOW_FULL 40
#define VER_ZIPPED_FILE 40
#define VER_ZIPPED_PACKET 40
#define VER_RUN_START 40
#define VER_OTHER_MON 40
#define VER_GPSD_START 40
#define VER_LOGWATCHD_START 40
#define VER_AVG_SURF_HK 40
#define VER_SUM_TURF_RATE 40
#define VER_ACQD_START 40
#define VER_TURF_REG 40
#define VER_TURF_EVENT_DATA 40
#define VER_GPU_POW_SPEC 40
#define VER_RTLSDR_POW_SPEC 40 
#define VER_TUFF_STATUS 40 
#define VER_TUFF_RAW_CMD 40 
#endif



//Enumerations
//!  The Packet Code
/*!
  Tells us what the packet is.
*/
typedef enum {
    PACKET_BD = 0xff, ///< AnitaEventBody_t -- No
    PACKET_HD = 0x100, ///<AnitaEventHeader_t --Yes
    PACKET_WV = 0x101, ///<RawWaveformPacket_t --Yes
    PACKET_SURF = 0x102, ///<RawSurfPacket_t -- Yes
    PACKET_HD_SLAC = 0x103, ///< Disused
    PACKET_SURF_HK = 0x110, ///<FullSurfHkStruct_t --Yes
    PACKET_TURF_RATE = 0x111, ///<TurfRateStruct_t -- Yes
    PACKET_AVG_SURF_HK = 0x112, ///<AveragedSurfHkStruct_t -- yes
    PACKET_SUM_TURF_RATE = 0x113, ///<SummedTurfRateStruct_t -- yes
    PACKET_TURF_REGISTER = 0x114, ///<TurfRegisterContents_t -- probably not
    PACKET_TURF_EVENT_DATA = 0x115, ///<TurfRawEventData_t -- probably not
    PACKET_PEDSUB_WV = 0x120, ///<PedSubbedWaveformPacket_t -- Yes
    PACKET_ENC_SURF = 0x121, ///<EncodedSurfPacketHeader_t -- Yes
    PACKET_ENC_SURF_PEDSUB = 0x122, ///<EncodedPedSubbedSurfPacketHeader_t -- Yes
    PACKET_ENC_EVENT_WRAPPER = 0x123, ///<EncodedEventWrapper_t -- No
    PACKET_PED_SUBBED_EVENT = 0x124, ///<PedSubbedEventBody_t -- No too big
    PACKET_ENC_WV_PEDSUB = 0x125, ///< EncodedPedSubbedChannelPacketHeader_t -- Yes
    PACKET_ENC_PEDSUB_EVENT_WRAPPER = 0x126, ///<EncodedEventWrapper_t -- No
    PACKET_PEDSUB_SURF = 0x127, ///<PedSubbedSurfPacket_t -- Yes 
    PACKET_LAB_PED = 0x130, ///<FullLabChipPedStruct_t -- Yes
    PACKET_FULL_PED = 0x131, ///< PedestalStruct_t -- No (Too Big)
    PACKET_GPS_ADU5_PAT = 0x200, ///<GpsAdu5PatStruct_t -- Yes
    PACKET_GPS_ADU5_SAT = 0x201, ///<GpsAdu5SatStruct_t -- Yes
    PACKET_GPS_ADU5_VTG = 0x202, ///<GpsAdu5VtgStruct_t -- Yes
    PACKET_GPS_G12_POS = 0x203, ///<GpsG12PosStruct_t -- Yes
    PACKET_GPS_G12_SAT = 0x204,///<GpsG12SatStruct_t -- Yes
    PACKET_GPS_GGA = 0x205, ///<GpsGgaStruct_t -- Yes
    PACKET_HKD = 0x300, ///< HkDataStruct_t -- Yes
    PACKET_HKD_SS = 0x301, ///< SSHkDataStruct_t -- Yes
    PACKET_CMD_ECHO = 0x400,///< CommandEcho_t -- Yes
    PACKET_MONITOR = 0x500, ///< MonitorStruct_t -- Yes
    PACKET_WAKEUP_LOS = 0x600,
    PACKET_WAKEUP_HIGHRATE = 0x601,
    PACKET_WAKEUP_COMM1 = 0x602,
    PACKET_WAKEUP_COMM2 = 0x603,
    PACKET_SLOW1 = 0x700,
    PACKET_SLOW2 = 0x800,
    PACKET_SLOW_FULL = 0x801, ///< SlowRateFull_t -- Yes
    PACKET_ZIPPED_PACKET = 0x900, ///< ZippedPacket_t -- Yes
    PACKET_ZIPPED_FILE = 0xa00, ///< ZippedFile_t -- Yes
    PACKET_RUN_START = 0xb00, ///< RunStart_t -- Yes
    PACKET_OTHER_MONITOR = 0xb01, ///< OtherMonitorStruct_t -- Yes
    PACKET_GPSD_START = 0xc00, ///< GpsdStartStruct_t -- Yes
    PACKET_LOGWATCHD_START = 0xc01, ///< LogWatchdStart_t -- Yes
    PACKET_ACQD_START = 0xc02, ///<AcqdStartStruct_t -- Yes
    PACKET_GPU_AVE_POW_SPEC = 0xd, ///<GpuPhiSectorPowerSpectrum_t -- Yes
    PACKET_RTLSDR_POW_SPEC = 0xe00 , 
    PACKET_TUFF_STATUS =0xf00, 
    PACKET_TUFF_RAW_CMD =0xf01
    
} PacketCode_t;

typedef enum {
    PACKET_FROM_G12 = 0x10000,
    PACKET_FROM_ADU5A = 0x20000,
    PACKET_FROM_ADU5B = 0x40000,
    CMD_FROM_PAYLOAD = 0x80000
} AuxPacketCode_t;

typedef enum {
    kNoEncoding=0
} EventEncodingScheme_t;

#ifdef SLAC_DATA06   
typedef enum {
    ENCODE_NONE=0, ///<Done
    ENCODE_SOMETHING=0x100 ///<Done
} ChannelEncodingType_t; 
#else
//!  The encoding enumeration
/*!
  Tells us how a waveform packet is encoded.
*/
typedef enum {
    ENCODE_NONE=0, ///<Done
    ENCODE_LOSSLESS_12BIT=0x100, ///<Done
    ENCODE_LOSSLESS_BINARY, ///<Done is just a marker for below
    ENCODE_LOSSLESS_11BIT,
    ENCODE_LOSSLESS_10BIT,
    ENCODE_LOSSLESS_9BIT,
    ENCODE_LOSSLESS_8BIT,
    ENCODE_LOSSLESS_7BIT,
    ENCODE_LOSSLESS_6BIT,
    ENCODE_LOSSLESS_5BIT,
    ENCODE_LOSSLESS_4BIT,
    ENCODE_LOSSLESS_3BIT,
    ENCODE_LOSSLESS_2BIT,
    ENCODE_LOSSLESS_1BIT,
    ENCODE_LOSSLESS_FIBONACCI, ///<Done
    ENCODE_LOSSLESS_BINFIB_COMBO=0x200, ///<Done is just a marker for below
    ENCODE_LOSSLESS_BINFIB_10BIT,
    ENCODE_LOSSLESS_BINFIB_9BIT,
    ENCODE_LOSSLESS_BINFIB_8BIT,
    ENCODE_LOSSLESS_BINFIB_7BIT,
    ENCODE_LOSSLESS_BINFIB_6BIT,
    ENCODE_LOSSLESS_BINFIB_5BIT,
    ENCODE_LOSSLESS_BINFIB_4BIT,
    ENCODE_LOSSLESS_BINFIB_3BIT,
    ENCODE_LOSSLESS_BINFIB_2BIT,
    ENCODE_LOSSLESS_BINFIB_1BIT,
    ENCODE_LOSSY_MULAW=0x300, ///<Done is just a marker for below
    ENCODE_LOSSY_MULAW_8BIT,
    ENCODE_LOSSY_MULAW_7BIT,
    ENCODE_LOSSY_MULAW_6BIT,
    ENCODE_LOSSY_MULAW_5BIT,
    ENCODE_LOSSY_MULAW_4BIT,
    ENCODE_LOSSY_MULAW_3BIT,
    ENCODE_LOSSY_MULAW_11_8,
    ENCODE_LOSSY_MULAW_11_7,
    ENCODE_LOSSY_MULAW_11_6,
    ENCODE_LOSSY_MULAW_11_5,
    ENCODE_LOSSY_MULAW_11_4,
    ENCODE_LOSSY_MULAW_11_3,
    ENCODE_LOSSY_MULAW_10_8,
    ENCODE_LOSSY_MULAW_10_7,
    ENCODE_LOSSY_MULAW_10_6,
    ENCODE_LOSSY_MULAW_10_5,
    ENCODE_LOSSY_MULAW_10_4,
    ENCODE_LOSSY_MULAW_10_3,
    ENCODE_LOSSY_MULAW_9_7,
    ENCODE_LOSSY_MULAW_9_6,
    ENCODE_LOSSY_MULAW_9_5,
    ENCODE_LOSSY_MULAW_9_4,
    ENCODE_LOSSY_MULAW_9_3,
    ENCODE_LOSSY_MULAW_8_6,
    ENCODE_LOSSY_MULAW_8_5,
    ENCODE_LOSSY_MULAW_8_4,
    ENCODE_LOSSY_MULAW_8_3,
    ENCODE_LOSSY_MULAW_7_5,
    ENCODE_LOSSY_MULAW_7_4,
    ENCODE_LOSSY_MULAW_7_3,
    ENCODE_LOSSY_MULAW_6_4,
    ENCODE_LOSSY_MULAW_6_3    
} ChannelEncodingType_t;
#endif

//!  The PriorityCode
/*!
  Unused, or largely unused.
*/
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

//!  The Hk Analogue Code
/*!
  Tells if this is Acromag data or calibration.
*/
typedef enum {
    IP320_RAW=0x100,
    IP320_AVZ=0x200,
    IP320_CAL=0x300
} AnalogueCode_t;

///////////////////////////////////////////////////////////////////////////
//Structures
///////////////////////////////////////////////////////////////////////////

//!  The Generic Header
/*!
  This is the 16 byte header that prefaces all ANITA data. The PacketCode_t 
  tells one what kind of packet it is, and the checksum can be used to
  validate the data.
*/
typedef struct {
    PacketCode_t code;    
    unsigned int packetNumber; ///<Especially for Ped
    unsigned short numBytes;
    unsigned char feByte;
    unsigned char verId;
    unsigned int checksum;
} GenericHeader_t;

//!  The Old SLAC data TURF I/O struct
/*!
  Disused
*/
typedef struct {
    unsigned char trigType; ///<Trig type bit masks
    unsigned char l3Type1Count; ///<L3 counter
    unsigned short trigNum; ///<turf trigger counter
    unsigned int trigTime;
    unsigned int ppsNum;     ///< 1PPS
    unsigned int c3poNum;     ///< 1 number of trigger time ticks per PPS
    unsigned short upperL1TrigPattern;
    unsigned short lowerL1TrigPattern;
    unsigned short upperL2TrigPattern;
    unsigned short lowerL2TrigPattern;
    unsigned short l3TrigPattern;
    unsigned short l3TrigPattern2;
} SlacTurfioStruct_t;

#ifdef SLAC_DATA06
typedef SlacTurfioStruct_t TurfioStruct_t;
#else
//!  The TURF I/O struct
/*!
  Is part of the AnitaEventHeader_t and contains all sorts of useful info
  about trigger patterns, deadTime, trigger time and trigger type.
*/
typedef struct {
  //!  The trigger type
  /*!
    0=RF, 1=PPS1, 2=PPS2, 3=Soft/Ext, 4=L3Type1, 5,6 buffer depth at trig
  */
  unsigned char trigType; ///<Trig type bit masks
  unsigned char l3Type1Count; ///<L3 counter
  unsigned short trigNum; ///<turf trigger counter
  unsigned int trigTime;
  unsigned short ppsNum;     ///< 1PPS
  unsigned short deadTime; ///< fraction = deadTime/64400
  unsigned int c3poNum;     ///< 1 number of trigger time ticks per PPS
  unsigned short l3TrigPattern;
  unsigned short l3TrigPatternH;
  unsigned char bufferDepth; ///<bits 0,1 trigTime depth 2,3 current depth
  unsigned char reserved[3];
} TurfioStruct_t;
#endif

//!  Disused
/*!
  Disused
*/
typedef struct {
    unsigned char chanId;   ///< chan+9*surf
    unsigned char chipIdFlag; ///< Bits 0,1 chipNum; Bit 3 hitBus wrap; 4-7 hitBusOff
    unsigned char firstHitbus;
    unsigned char lastHitbus;
    float mean; ///<Filled by Prioritizerd
    float rms; ///<Filled by Prioritizerd

} SlacRawSurfChannelHeader_t;

//!  Disused
/*!
  Disused
*/
typedef struct {
    SlacRawSurfChannelHeader_t rawHdr;
    ChannelEncodingType_t encType;
    unsigned short numBytes;
    unsigned short crc;
} SlacEncodedSurfChannelHeader_t;

#ifdef SLAC_DATA06
typedef SlacRawSurfChannelHeader_t RawSurfChannelHeader_t;
#else
//!  The channel header
/*!
  Contains useful info like which channel it is, which LABRADOR chip and when
  the HITBUS (the write pointer wraparound) is.
*/
typedef struct {
  //!  Channel Id
  /*!
    chan + 9*surf (0-8 is SURF 1, ... 81-89 are SURF 10)
  */
    unsigned char chanId;   // chan+9*surf
  //!  chip id bitmask
  /*!
    0:1  LABRADOR chip
    2 RCO
    3 HITBUS wrap
    4-7 HITBUS offset
  */
    unsigned char chipIdFlag; // Bits 0,1 chipNum; Bit 3 hitBus wrap; 4-7 hitBusOff
  //!  First sample of the hitbus 
  /*!
    The last sample in the waveform is [firstHitbus-1] --unless wrapped.
    Otherwise it runs from firstHitbus+1 to lastHitbus-1 inclusive
  */
    unsigned char firstHitbus; // If wrappedHitbus=0 data runs, lastHitbus+1
  //!  Last sample of the hitbus
  /*!
    The first sample in the waveform is [lastHitbus+1] -- unless wrapped.
    Otherwise it runs from firstHitbus+1 to lastHitbus-1 inclusive
  */
    unsigned char lastHitbus; //to firstHitbus-1 inclusive

} RawSurfChannelHeader_t;
#endif

//! Channel header for encoded data
/*!
  Channel header for encoded data, tells encoding type and provides checksum.
*/
typedef struct {
    RawSurfChannelHeader_t rawHdr;
    ChannelEncodingType_t encType;
    unsigned short numBytes;
    unsigned short crc;
} EncodedSurfChannelHeader_t;

//!  A complete SURF channel
/*!
  A complete SURF channel  (header + waveform)
*/
typedef struct {
    RawSurfChannelHeader_t header;
    unsigned short data[MAX_NUMBER_SAMPLES];
} SurfChannelFull_t;

//!  A complete pedestal subtracted SURF channel
/*!
  A complete pedestal subtracted SURF channel.
*/
typedef struct {
    RawSurfChannelHeader_t header;
    short xMax;
    short xMin;
    float mean; ///<Filled by pedestalLib
    float rms; ///<Filled by pedestalLib
    short data[MAX_NUMBER_SAMPLES]; ///<Pedestal subtracted and 11bit data
} SurfChannelPedSubbed_t;

//!  On board structure for calibration/relay status
/*!
  On board structure for calibration/relay status
*/
typedef struct {
    unsigned int unixTime;
    unsigned int status;
} CalibStruct_t;

//!  Acromag data array
/*!
  Acromag data array
*/
typedef struct {
    unsigned short data[CHANS_PER_IP320];
} AnalogueDataStruct_t;

//!  Acromag data array
/*!
  Acromag data array
*/
typedef struct {
    int data[CHANS_PER_IP320];
} AnalogueCorrectedDataStruct_t;

//!  Full Acromag data structure
/*!
  Full Acromag data structure comes in data, or calibration flavour.
*/
typedef struct {
    AnalogueCode_t code;
    AnalogueDataStruct_t board[NUM_IP320_BOARDS];
} FullAnalogueStruct_t;


//!  Single Acromag data structure
/*!
  Single Acromag data structure comes in data, or calibration flavour.
*/
typedef struct {
    AnalogueCode_t code;
    AnalogueDataStruct_t board;
} SingleAnalogueStruct_t;


#ifdef ANITA_1_DATA
typedef struct {
    unsigned short temp[2];
} SBSTemperatureDataStruct_t;
#elif ANITA_2_DATA
//!  The CR11 temperatures
/*!
  The CR11 temperatue structure is in (4/100) * milli deg C
*/
typedef struct {
  short temp[4]; ///< 
} SBSTemperatureDataStruct_t;
#else // ANITA_3_DATA
//!  The XCR14 temperatures
/*!
  The XCR14 temperatue structure is in (4/100) * milli deg C
*/
typedef struct {
  short temp[6]; ///< 
} SBSTemperatureDataStruct_t;
#endif

//!  The magnetometer data
/*!
  The magnetometer data
*/
typedef struct {
    float x;
    float y;
    float z;
} MagnetometerDataStruct_t;

//!  Debugging use only scaler data
/*!
  Debugging use only scaler data
*/
typedef struct {    
  unsigned int unixTime;
  unsigned int unixTimeUs;
  unsigned short scaler[ACTIVE_SURFS][32];
  unsigned short extraScaler[ACTIVE_SURFS][32];
} SimpleScalerStruct_t; //No inter used

//!  Debugging use only TURF scaler data
/*!
  Debugging use only TURF scaler data
*/
typedef struct {
  GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned int unixTimeUs;
  unsigned int whichBank;
  unsigned int values[TURF_BANK_SIZE];
} TurfRegisterContents_t;



//!  Debugging use only TURF raw event data
/*!
  Debugging use only TURF raw event data
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime;
  unsigned int unixTimeUs;
  unsigned int eventNumber;
  unsigned char rawBytes[TURF_EVENT_DATA_SIZE];
} TurfRawEventData_t;



//!  Disk Space
/*!
  Contains the disk spaces and labels of currently mounted disks. The 8 disks are:
  [0] - /tmp      MB
  [1] - /var      MB
  [2] - /home     MB
  [3] - /         MB
  [4] - /mnt/helium1       128 MB
  [5] - /mnt/helium2       128 MB
  [6] - /mnt/usbint        4 MB
  [7] - NTU disk           16 MB 
  
*/
typedef struct {
    unsigned short diskSpace[8]; ///<In units of 10 MegaBytes
    char ntuLabel[12];
    char otherLabel[12];
    char usbLabel[12];
} DiskSpaceStruct_t;

//!  Queue Stuff
/*!
  Number of links in the event and housekeeping telemetry queues
*/
typedef struct {
  unsigned short eventLinks[NUM_PRIORITIES]; ///<10 Priorities
  unsigned short hkLinks[21]; ///<Needs to be finalised once everything is settled
} QueueStruct_t;

//!  Process Information
/*!
  Process Information the time used and memory used by ANITA processess
*/
typedef struct {
  unsigned int utime[NUM_PROCESSES];
  unsigned int stime[NUM_PROCESSES];
  unsigned int vsize[NUM_PROCESSES];
} ProcessInfo_t;

//!  On board command structre
/*!
  On board command structure
*/
typedef struct {    
    unsigned char numCmdBytes;
    unsigned char cmd[MAX_CMD_LENGTH];
    unsigned int fromSipd; ///< 1 means it was a user command from SIPd, 0 is payload generated
} CommandStruct_t;

//!  On board log request struct
/*!
 On board log request struct
*/
typedef struct {
  unsigned int numLines; ///<0 results in a cat, otherwise a tail
  int logReq;
  int jclOpt;
  int optArg;
  char filename[180];
} LogWatchRequest_t;

//!  On board pedestal struct
/*!
  On board pedestal struct
*/
typedef struct {
    unsigned char chanId;   ///< chan+9*surf
    unsigned char chipId; ///< 0-3
    unsigned short chipEntries;
    unsigned short pedMean[MAX_NUMBER_SAMPLES]; ///< actual value
    unsigned char pedRMS[MAX_NUMBER_SAMPLES]; ///<times 10
} LabChipChannelPedStruct_t;

//!  On board Index struct
/*!
  On board index struct -- used by Playbackd
*/
typedef struct {
    unsigned int eventNumber;
    unsigned int runNumber;
    int eventDiskBitMask; ///<Which disks was it written to?
    char ntuLabel[12];
    char otherLabel[12];
    char usbLabel[12];
} IndexEntry_t;

//!  On board Playback request
/*!
  On board playback request
*/
typedef struct {
    unsigned int eventNumber;
    int pri;
} PlaybackRequest_t;

//!  Slow rate struct
/*!
  Slow rate struct, the RF info that is sent as part of the slow data. Needs to be updated for ANITA-II.
*/
typedef struct {
    unsigned int eventNumber;
    unsigned char rfPwrAvg[ACTIVE_SURFS][RFCHAN_PER_SURF];
    unsigned char avgScalerRates[TRIGGER_SURFS][SCALERS_PER_SURF]; ///< * 2^7
  //unsigned char rmsScalerRates[TRIGGER_SURFS][TRIGGERS_PER_SURF];
  //    unsigned char avgL1Rates[TRIGGER_SURFS]; ///< 3 of 8 counters --fix later
  //    unsigned char avgL2Rates[PHI_SECTORS]; ///< average of upper and lower
  //    unsigned char avgL3Rates[PHI_SECTORS];    
    unsigned char eventRate1Min; ///<Multiplied by 8
    unsigned char eventRate10Min; ///<Multiplied by 8
} SlowRateRFStruct_t;

//!  Slow Hk Stuff
/*!
  Slow rate hk Stuff. May need updating for ANITA-II
*/
typedef struct {
    float latitude;
    float longitude;
    short altitude;
    unsigned char temps[4];  ///<{SBS,SURF,TURF,RAD}
    unsigned char powers[4]; ///<{PV V, +24V, BAT I, 24 I}
} SlowRateHkStruct_t;



////////////////////////////////////////////////////////////////////////////
//Telemetry Structs (may be used for onboard storage)
////////////////////////////////////////////////////////////////////////////

//!  Disused
/*!
  Disused
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned int lastEventNumber;
    float latitude;
    float longitude;
    float altitude;
    unsigned short sbsTemp[2];
} SlowRateType1_t;

//!  Slow Rate Block -- Telemetered
/*!
  Slow Rate Block -- Telemetered
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime;
    SlowRateRFStruct_t rf;
    SlowRateHkStruct_t hk;
} SlowRateFull_t;

//!  Turf Rates -- Telemetered
/*!
  Turf Rates -- Telemetered
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime;
  unsigned short ppsNum; ///<It's only updated every second so no need for sub-second timing
  unsigned short deadTime; ///<How much were we dead??
  unsigned short l1Rates[PHI_SECTORS][2]; //
  unsigned char l3Rates[PHI_SECTORS][2]; /// to get Hz
  unsigned short l1TrigMask; ///< As read from TURF (16-bit upper phi, lower phi)
  unsigned short l1TrigMaskH; ///< As read from TURF (16-bit upper phi, lower phi)
  unsigned short phiTrigMask; ///< 16 bit phi-sector mask
  unsigned short phiTrigMaskH; ///< 16 bit phi-sector mask
  unsigned char errorFlag;///<Bit 1-4 bufferdepth, Bits 5,6,7 are for upper,lower,nadir trig mask match
  unsigned char reserved[3];
  unsigned int c3poNum;
} TurfRateStruct_t;

//!  Summed Turf Rates -- Telemetered
/*!
  Summed Turf Rates, rather than send down the TurfRate block every second will instead preferential send down these blocks that are summed over a minute or so.
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime; ///<Time of first hk
    unsigned short numRates; ///<Number of rates in average
    unsigned short deltaT; ///<Difference in time between first and last 
    unsigned int deadTime; ///<Summed dead time between first and last
    unsigned char bufferCount[4]; ///<Counting filled buffers
    unsigned short l3Rates[PHI_SECTORS][2]; ///</numRates to get Hz z  
    unsigned short l1TrigMask; ///<As read from TURF (16-bit phi)
    unsigned short l1TrigMaskH; ///<As read from TURF (16-bit phi)
    unsigned short phiTrigMask; ///<16-bit phi-sector mask
    unsigned short phiTrigMaskH; ///<16-bit phi-sector mask
    unsigned char errorFlag;///<Bit 1-4 bufferdepth, Bits 5,6,7 are for upper,lower,nadir trig mask match
} SummedTurfRateStruct_t;

//!  ANITA Event Header -- Telemetered
/*!
  ANITA Event Header, contains all kinds of fun information about the event
  including times, trigger patterns, event numbers and error words
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime;       ///< unix UTC sec
  unsigned int unixTimeUs;     ///< unix UTC microsec 

  //!  GPS timestamp
  /*!
     the GPS fraction of second (in ns) 
     (for the X events per second that get 
     tagged with it, note it now includes
     second offset from unixTime)
  */
  int gpsSubTime;    
  unsigned int turfEventId; ///<Turf event id that doesn't roll
  unsigned int eventNumber;    ///< Global event number 
  unsigned short calibStatus;   ///< Were we flashing the pulser? 
  unsigned char priority; ///< priority and other
  unsigned char turfUpperWord; ///< The upper 8 bits from the TURF
  unsigned char otherFlag; ///< Currently the first two surf evNums 
  //!  Error Flag
  /*!
    Bit 1 means sync slip between TURF and software
    Bit 2 is sync slip between SURF 1 and software
    Bit 3 is sync slip between SURF 10 and SURF 1
    Bit 4 is non matching TURF test pattern
    Bit 5 is startBitGood (1 is good, 0 is bad);
    Bit 6 is stopBitGood (1 is good, 0 is bad);
    Bit 7-8 TURFIO photo shutter output
  */
  unsigned char errorFlag; 
  unsigned char surfSlipFlag; ///< Sync Slip between SURF 2-9 and SURF 1
  unsigned char peakThetaBin; ///< 8-bit peak theta bin from Prioritizer
  unsigned short l1TrigMask; ///< 16-bit phi ant mask (from TURF)
  unsigned short l1TrigMaskH; ///< 16-bit phi ant mask (from TURF)
  unsigned short phiTrigMask; ///< 16-bit phi mask (from TURF)
  unsigned short phiTrigMaskH; ///< 16-bit phi mask (from TURF)
  unsigned short imagePeak; ///< 16-bit image peak from Prioritizer
  unsigned short coherentSumPeak; ///< 16-bit coherent sum peak from Prioritizer
  unsigned short prioritizerStuff; ///< TBD
  TurfioStruct_t turfio; ///<The X byte TURFIO data
} AnitaEventHeader_t;


//!  Raw waveform packet -- Telemetered
/*!
  Raw waveform packet, we nromally send down encoded packets, but have the option to send them down raw
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int eventNumber;
    SurfChannelFull_t waveform;
} RawWaveformPacket_t;


//! Pedsubbed Waveform packet -- Telemetered
/*!
  Pedestal subtracted waveform packet, we nromally send down encoded packets, but have the option to send them down raw

*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int eventNumber;
    unsigned int whichPeds;
    SurfChannelPedSubbed_t waveform;
} PedSubbedWaveformPacket_t;

//!  Raw SURF wavefom packet -- Telemetered
/*!
  Raw SURF waveform packet, we nromally send down encoded packets, but have the option to send them down raw
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int eventNumber;
    SurfChannelFull_t waveform[CHANNELS_PER_SURF];
} RawSurfPacket_t;

//!  Pedestal subtracted SURF wavefom packet -- Telemetered
/*!
  Pedestal subtracted SURF waveform packet, we nromally send down encoded packets, but have the option to send them down raw
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int eventNumber;
    unsigned int whichPeds;
    SurfChannelPedSubbed_t waveform[CHANNELS_PER_SURF];
} PedSubbedSurfPacket_t;


// typedef struct {
//      GenericHeader_t gHdr;
//      unsigned int eventNumber;
//      unsigned int whichPeds;
//      EncodedSurfChannelHeader_t chanHead;
// } EncodedWaveformPacket_t; //0x101

//!  Encoded SURF Packet header -- Telemetered
/*!
  Encoded SURF Packet header, precedes the encoded data
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int eventNumber;
} EncodedSurfPacketHeader_t;

//! Encoded PedSubbed Packet Header -- Telemetered  
/*!
  Encoded PedSubbed Packet Header, precedes the encoded waveform data.
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int eventNumber;
    unsigned int whichPeds;
} BaseWavePacketHeader_t;

typedef BaseWavePacketHeader_t EncodedPedSubbedSurfPacketHeader_t;
typedef BaseWavePacketHeader_t EncodedPedSubbedChannelPacketHeader_t;

//!  ADU5 Postion and Attitude -- Telemetered
/*!
  ADU5 Postion and Attitude
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned int unixTimeUs;
    unsigned int timeOfDay;
    float heading;
    float pitch;
    float roll;
    float mrms;
    float brms;
    float latitude;
    float longitude;
    float altitude;
    unsigned int attFlag;
} GpsAdu5PatStruct_t;

//! ADU5 Postion and Geoid  -- Telemetered
/*!
  ADU5 Position and Geoid -- Telemetered
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime;
  unsigned int unixTimeUs;
  unsigned int timeOfDay;  
  float latitude;
  float longitude;
  float altitude;
  float hdop;
  float geoidSeparation;
  float ageOfCalc;
  unsigned char posFixType;
  unsigned char numSats;
  unsigned short baseStationId;
} GpsGgaStruct_t;

//! Gps Satellite Info
/*!
  GPS Satellite Info
*/
typedef struct {
    unsigned char prn;
    unsigned char elevation;
    unsigned char snr;
    unsigned char flag; 
    unsigned short azimuth;
} GpsSatInfo_t;

//!  G12 Satellite Info -- Telemetered
/*!
  G12 Satellite Info
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned int numSats;
    GpsSatInfo_t sat[MAX_SATS];
} GpsG12SatStruct_t;

//! ADU5 Satellite Info -- Telemetered
/*!
  ADU5 Satellite Info -- May update for ANITA-II
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned char numSats[4];
    GpsSatInfo_t sat[4][MAX_SATS];
} GpsAdu5SatStruct_t;

//! ADU5 course and speed info -- Telemetered
/*!
  ADU5 course and speed info
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned int unixTimeUs;
    float trueCourse;
    float magneticCourse;
    float speedInKnots;
    float speedInKPH;
} GpsAdu5VtgStruct_t;

//! G12 Position and speed info -- Telemetered  
/*!
  G12 Position and speed info
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned int unixTimeUs;
    unsigned int timeOfDay;
    unsigned int numSats;
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

//! Gpsd Start Block -- Telemetered
/*!
  Created each time that GPSd is started
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime;
  unsigned char ackCount[3]; ///< Number of acknowledge messages from each GPS
  unsigned char nakCount[3];///< Number of NAK messages from each GPS
  unsigned char rioBitMask; 
  unsigned char tstBitMask;
} GpsdStartStruct_t;


//!  The Acqd Startup Struct -- Telemetered
/*!
  This structure is created whenever Acqd starts, it serves as a simple
  unit test to make sure everything is on and working. You can tell which
  RF channels are on by looking at the chanRMS and on the trigger side of
  things you can see the live channels by looking at the scalerValues.

  This will be telemetered
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime;
  unsigned char turfIdBytes[4];
  unsigned int turfIdVersion;
  unsigned char turfioIdBytes[4];
  unsigned int turfioIdVersion;
  unsigned char surfIdBytes[ACTIVE_SURFS][4];
  unsigned int surfIdVersion[ACTIVE_SURFS];
  unsigned char testBytes[8];
  unsigned int numEvents;
  float chanMean[ACTIVE_SURFS][CHANNELS_PER_SURF]; ///<Ped subtracted
  float chanRMS[ACTIVE_SURFS][CHANNELS_PER_SURF]; ///<Ped subtracted
  unsigned short threshVals[10];
  unsigned short scalerVals[ACTIVE_SURFS][SCALERS_PER_SURF][10];
} AcqdStartStruct_t;
  
//! Hk Data Struct -- Telemetered
/*!
  The main housekeeping data structure with acromag + SBS temps + magnetometer data
*/
typedef struct {    
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned int unixTimeUs;
    FullAnalogueStruct_t ip320;
    MagnetometerDataStruct_t mag;
    SBSTemperatureDataStruct_t sbs;
} HkDataStruct_t;


//! SS Hk Data Struct -- Telemetered
/*!
  The main housekeeping data structure with acromag + SBS temps + magnetometer data
*/
typedef struct {    
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned int unixTimeUs;
    SingleAnalogueStruct_t ip320;
} SSHkDataStruct_t;

//! SURF Hk -- Telemetered
/*!
  SURF Hk, contains thresholds, band rates (scalers) and rf power
*/
typedef struct { 
  GenericHeader_t gHdr;
  unsigned int unixTime;
  unsigned int unixTimeUs;
  unsigned short globalThreshold; ///<set to zero if there isn't one
  unsigned short errorFlag; ///<Will define at some point    
  unsigned short scalerGoals[NUM_ANTENNA_RINGS]; ///<What are we aiming for with the scaler rate
  unsigned short reserved;  
  unsigned short upperWords[ACTIVE_SURFS];
  unsigned short scaler[ACTIVE_SURFS][SCALERS_PER_SURF];
  unsigned short l1Scaler[ACTIVE_SURFS][L1S_PER_SURF];
  unsigned short threshold[ACTIVE_SURFS][SCALERS_PER_SURF];
  unsigned short setThreshold[ACTIVE_SURFS][SCALERS_PER_SURF];
  unsigned short rfPower[ACTIVE_SURFS][RFCHAN_PER_SURF];
  unsigned short surfTrigBandMask[ACTIVE_SURFS];
} FullSurfHkStruct_t;

//! Average Surf Hk -- Telemetered
/*!
  Rather than sending raw SURF Hk we will preferentially send averaged data.
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime; ///<Time of first hk
  unsigned short numHks; ///<Number of hks in average
  unsigned short deltaT; ///<Difference in time between first and last 
  unsigned int hadError; ///<Bit mask to be defined
  unsigned short globalThreshold;
  unsigned short scalerGoals[NUM_ANTENNA_RINGS];
  unsigned short avgScaler[ACTIVE_SURFS][SCALERS_PER_SURF];
  unsigned short rmsScaler[ACTIVE_SURFS][SCALERS_PER_SURF];
  unsigned short avgL1[ACTIVE_SURFS][L1S_PER_SURF];
  unsigned short rmsL1[ACTIVE_SURFS][L1S_PER_SURF];
  unsigned short avgThresh[ACTIVE_SURFS][SCALERS_PER_SURF];
  unsigned short rmsThresh[ACTIVE_SURFS][SCALERS_PER_SURF];
  unsigned short avgRFPower[ACTIVE_SURFS][RFCHAN_PER_SURF];
  unsigned short rmsRFPower[ACTIVE_SURFS][RFCHAN_PER_SURF];
  unsigned short surfTrigBandMask[ACTIVE_SURFS];
} AveragedSurfHkStruct_t;

//! Command Echo -- Telemetered
/*!
  Command Echo -- Echos commands sent to the payload including whether or not
  they were successful.
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned short goodFlag; ///< 0 is bad, 1 is good
    unsigned short numCmdBytes; ///< number of cmd bytes (upto 10)
    unsigned char cmd[MAX_CMD_LENGTH]; ///< the cmd bytes
} CommandEcho_t;

//! Monitor Block -- Telemetered  
/*!
  Monitor stuff about disk spaces, telemetry queues and processess
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime;
  DiskSpaceStruct_t diskInfo;
  QueueStruct_t queueInfo;
  ProcessInfo_t procInfo;
} MonitorStruct_t;

//! Other Monitor Block -- Telemetered  
/*!
  Monitor inodes, inter-process communication lists and processes
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime;
    unsigned int ramDiskInodes;
    unsigned int runStartTime;
    unsigned int runStartEventNumber; ///<Start eventNumber
    unsigned int runNumber; ///<Run number
    unsigned short dirFiles[3]; ///< /tmp/anita/acqd /tmp/anita/eventd /tmp/anita/prioritizerd
    unsigned short dirLinks[3]; ///< /tmp/anita/acqd /tmp/anita/eventd /tmp/anita/prioritizerd
    unsigned short processBitMask;
  unsigned short reserved;
} OtherMonitorStruct_t;

//! Pedestal Block -- Telemetered
/*!
  Pedestal Block
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTimeStart;
    unsigned int unixTimeEnd;
    LabChipChannelPedStruct_t pedChan[CHANNELS_PER_SURF];
} FullLabChipPedStruct_t;

//! Zipped packet -- Telemetered  
/*!
  Just contains a gzipped version of another packet
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int numUncompressedBytes;
} ZippedPacket_t;

//!  Zipped File -- Telemetered
/*!
  How we send down config files and log files
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime;
  unsigned short numUncompressedBytes;
  unsigned short segmentNumber;
  char filename[60];
} ZippedFile_t;

//! Run Start Block - Telemetered  
/*!
  Run start block
*/
typedef struct {
    GenericHeader_t gHdr;
    unsigned int unixTime; ///<Start time
    unsigned int eventNumber; ///<Start eventNumber
    unsigned int runNumber; ///<Run number
} RunStart_t;

//! LogWatchd Start Block -- Telemetered  
/*!
  LogWatchd Start Block created when LogWatchd starts
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTime;
  unsigned int runNumber;
  float upTime;
  float idleTime;
} LogWatchdStart_t;

/////////////////////////////////////////////////////////////////////////////
// On-board structs
////////////////////////////////////////////////////////////////////////////

//! Raw event format 
/*!
  Raw event format
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int eventNumber;    /* Global event number */
  unsigned int surfEventId[ACTIVE_SURFS]; ///<Id number from each SURF
  SurfChannelFull_t channel[NUM_DIGITZED_CHANNELS];
} AnitaEventBody_t;

//! Pedestal subtracted event format  
/*!
  Pedestal subtracted event format
*/
typedef struct {
  GenericHeader_t gHdr;
  unsigned int eventNumber;    /* Global event number */
  unsigned int surfEventId[ACTIVE_SURFS];
  unsigned int whichPeds; ///<whichPedestals did we subtract
  SurfChannelPedSubbed_t channel[NUM_DIGITZED_CHANNELS];
} PedSubbedEventBody_t;

//! Full event format -- never used  
/*!
  Full event format
*/
typedef struct {
    AnitaEventHeader_t header;
    AnitaEventBody_t body;
} AnitaEventFull_t;

//! Wrapper for data that is written encoded  
/*!
  Wrapper for data that is written encoded
*/
typedef struct {
    GenericHeader_t gHdr; ///<gHdr.numBytes includes EncodedEventWrapper_t
    unsigned int eventNumber;
    unsigned numBytes; ///<Not including the EncodedEventWrapper_t;
} EncodedEventWrapper_t; ///<Not implemented

//! GPS Event Timestamp  
/*!
  GPS Event Timestamp
*/
typedef struct {
    unsigned int unixTime;
    unsigned int subTime;
    int fromAdu5; ///<2 is ADU52, 1 is ADU51 , 0 is g12
} GpsSubTime_t;


///////////////////////////////////////////////////////////////////////
//Raw Gps Structs
//////////////////////////////////////////////////////////////////////

//! This is the MBEN struct described on page 121 of the ADU5 manual
/*!
  All of the comments come directly from the ADU5 manual
*/
typedef struct __attribute__((packed)) RawAdu5MBNStruct {
  char header[11]; ///< $PASHR,MCA,
  unsigned short sequence_tag;  ///< Sequence ID number in units of 50ms, modulo 30 minutes
  unsigned char mben_eft; ///< Number of remaining MBEN structures to be sent for current epoch.
  unsigned char svpm; ///< Satellite PRN number.
  unsigned char el; ///< Satellite elevation angle (degrees).
  unsigned char az; ///< Satellite azimuth angle (degrees).
  unsigned char chnind; ///< Channel ID (1 to 12).
  unsigned char warn; ///< Warning flag
  unsigned char good_bad; ///< I3ndicates quality of the position measurement.
  unsigned char polarity_know; ///< Indicates synchronization of receiver with NAV message
  unsigned char ireg; ///< Signal-to-noise ratio of satellite observation
  unsigned char qa_phase; ///< Phase quality indicator: 0 - 5 and 95 -100 are normal
  double full_phase; ///< Full carrier phase measurements in cycles
  double raw_range; ///< Raw range to SV (in seconds), that is, receive_time - raw_range = transmit time
  int doppler; ///< Doppler (10-4 Hz)
  int smoothing; ///< Doppler (10-4 Hz)  
  unsigned char checkSum; ///< Checksum, a bytewise exclusive OR (XOR) on all bytes from sequence_tag (just after header) to the byte before checksum. 
  char carriageReturn;
  char lineFeed;
} RawAdu5MBNStruct_t;


//! This is the SNAV struct described on page 131 of the ADU5 manual
/*!
  The SNAV epheremis raw data. All of the comments come directly from the ADU5 manual
*/
typedef struct __attribute__((packed)) RawAdu5SNVStruct {
  char snvHeader[11]; ///< $PASHR,SNV
  short weekNumber; ///< GPS week number.
  int secondsInWeek; ///< Seconds of GPS week.
  float groupDelay; ///< Group delay (sec).
  int aodc; ///< Clock data issue.
  int toc; ///< (sec).
  float af2; ///< Clock: (sec/sec2)
  float af1; ///< Clock (sec/sec)
  float af0; ///< Clock (sec)
  int aode; ///< Orbit data issue.
  float deltaN; ///< Mean anomaly correction (semi-circle/sec).
  double m0; ///< Mean anomaly at reference time (semi-circle).
  double eccentricity; ///< Eccentricity.
  double roota; ///< Square root of semi-major axis (meters p)
  int toe; ///< Reference time for orbit (sec).
  float cic; ///< Harmonic correction term (radians).
  float crc; ///< Harmonic correction term (meters).
  float cis; ///< Harmonic correction term (radians).
  float crs; ///< Harmonic correction term (meters).
  float cuc; ///< Harmonic correction term (radians).
  float cus; ///< Harmonic correction term (radians).
  double omega0; ///< Lon of Asc. node (semi-circles).
  double omega; ///< Arg. of Perigee (semi-circles).
  double i0; ///< Inclination angle at reference time (semi-circles).
  float omegadot; ///< Rate of right Asc. (semi-circles per sec).
  float idot; ///< Rate of inclination (semi-circles per sec).
  short accuracy; ///< (coded).
  short health; ///< (coded).
  short fit; ///< Curve fit interval (coded).
  char prnnum; ///< (SV PRN number -1)
  char res; ///< Reserved byte.
  unsigned short checkSum; ///< Checksum (sum of words from weekNumber to res)
  char carriageReturn;
  char lineFeed;
} RawAdu5SNVStruct_t;

//////////////////////////////////////////////////////////////////

//! This is the PBEN struct described on page 128 of the ADU5 manual
/*!
  All of the comments come directly from the ADU5 manual
*/
typedef struct __attribute__((packed)) RawAdu5PBNStruct {
  char pbenHeader[11]; ///< $PASHR,PBN
  int pben_time;  ///< GPS time in 10-3 seconds of the week when data was received.
  char sitename[4]; ///< 4-character site name (operator entered)
  double navx; ///< Station position: ECEF-X
  double navy; ///< Station position: ECEF-Y
  double navz; ///< Station position: ECEF-Z
  float navt; ///< Clock offset (meters).
  float navxdot; ///< Velocity in ECEF-X (m/sec)
  float navydot; ///< Velocity in ECEF-Y (m/sec)
  float navzdot; ///< Velocity in ECEF-Z (m/sec)
  float navtdot; ///< Clock drift.
  unsigned short pdop; ///< Position Dilution of Precision
  unsigned short checkSum; ///< Checksum (sum of words from pben_time to pdop)
  char carriageReturn;
  char lineFeed;
} RawAdu5PBNStruct_t;

//! This is the ATT struct described on page 114 of the ADU5 manual
/*!
  All of the comments come directly from the ADU5 manual
*/
typedef struct __attribute__((packed)) RawAdu5ATTStruct {
  char attHeader[11]; ///< $PASHR,ATT
  double head; ///< Heading in degrees
  double pitch; ///< Pitch in degrees 
  double roll; ///< Roll in degrees
  double brms; ///< BRMS in meters
  double mrms; ///< MRMS in meters
  int timeOfWeek; ///< Seconds-of-Week in milliseconds
  char reset; ///< Attitude reset flag
  char spare; ///< Spare byte which is not used
  unsigned short checkSum; ///< Checksum (sum of words from head to spare)
  char carriageReturn;
  char lineFeed;
} RawAdu5ATTStruct_t;


struct __attribute__((packed)) RawAdu5BFileHeader {
    char version[10];
    unsigned char raw_version;
    char rcvr_type[10];
    char chan_ver[10];
    char nav_ver[10];
    short capability;
    int wb_start;
    char num_obs_type;
    char spare[42];
}  RawAdu5BFileHeader_t;

struct __attribute__((packed)) RawAdu5BFileRawNav {
    char sitename[4];
    double rcv_time;
    double navx;
    double navy;
    double navz;
    float navxdot;
    float navydot;
    float navzdot;
    double navt;
    double navtdot;
    unsigned short pdop;
    char num_sats;     
}  RawAdu5BFileRawNav_t; 

struct __attribute__((packed)) RawAdu5BFileChanObs {
  double raw_range;
  float smth_corr;
  unsigned short smth_count;
  char polarity_known;
  unsigned char warning;
  unsigned char goodbad;
  unsigned char ireg;
  unsigned char qa_phase;
  int doppler;
  double carphase;		    
}  RawAdu5BFileChanObs_t;


struct __attribute__((packed)) RawAdu5BFileSatelliteHeader {
  unsigned char svprn;
  unsigned char elevation;
  unsigned char azimuth;
  unsigned char chnind;
}  RawAdu5BFileSatelliteHeader_t;


///////////////////////////////////////////////////////////////////////
//Utility Structures
//////////////////////////////////////////////////////////////////////
//! No idea  
/*!
  Does something to do with encoded events
*/
typedef struct {
    unsigned int pedUnixTime;
    ChannelEncodingType_t encTypes[ACTIVE_SURFS][CHANNELS_PER_SURF];
} EncodeControlStruct_t;

////////////////////////////////////////////////////////////////////////////
//Prioritizer Utitlity Structs
///////////////////////////////////////////////////////////////////////////
/// @cond 
#ifndef DOXYGEN_SHOULD_SKIP_THIS
// these are syntactic sugar to help us keep track of bit shifts 
typedef int Fixed3_t; ///<rescaled integer left shifted 3 bits 
typedef int Fixed6_t; ///<rescaled integer left shifted 6 bits 
typedef int Fixed8_t; ///<rescaled integer left shifted 8 bits 


/*    FOR THREE STRUCTS THAT FOLLOW
      valid samples==-1 prior to unwinding 
*/

//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     int data[MAX_NUMBER_SAMPLES];
     int valid_samples; 
} LogicChannel_t;

//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     Fixed3_t data[MAX_NUMBER_SAMPLES];
     Fixed3_t baseline;
     short valid_samples;
} TransientChannel3_t;

//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     Fixed6_t data[MAX_NUMBER_SAMPLES];
     short valid_samples;
} TransientChannel6_t;

//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     Fixed8_t data[MAX_NUMBER_SAMPLES];
     Fixed8_t baseline;
     short valid_samples;
} TransientChannel8_t;

//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     float data[MAX_NUMBER_SAMPLES];
     short valid_samples;
} TransientChannelF_t;

//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     float data[MAX_NUMBER_SAMPLES];
     short valid_samples;
     float RMSall;
     float RMSpre;
} TransientChannelFRMS_t;

//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     TransientChannel3_t ch[NUM_DIGITZED_CHANNELS]; 
} AnitaTransientBody3_t; /* final corrected transient type 
			    used to calculate power */
//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     TransientChannel6_t ch[NUM_DIGITZED_CHANNELS]; 
} AnitaPowerBody6_t; /* power from squaring an AnitaTransientBody3 */

//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     TransientChannel8_t ch[NUM_DIGITZED_CHANNELS]; 
} AnitaTransientBody8_t; /* used for pedestal subtraction, unwrapping, 
			    averaging, and gain correction */
//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     TransientChannelF_t ch[NUM_DIGITZED_CHANNELS]; 
} AnitaTransientBodyF_t;

//! Prioritizer utility  
/*!
  Prioritizer utility
*/
typedef struct {
     TransientChannel6_t S0,S1,S2,S3;
} AnitaStokes6_t;
/// @endcond 
#endif //DOXYGEN_SHOULD_SKIP_THIS
///////////////////////////////////////////////////////////////////////////////
////Pedestal Calculation and Storage  Structs
//////////////////////////////////////////////////////////////////////////////

//! Pedestal utility  
/*!
  Pedestal utility
*/
typedef struct {
    unsigned int unixTimeStart;
    unsigned int unixTimeEnd;
    LabChipChannelPedStruct_t pedChan[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF];
} FullPedStruct_t;

//! Pedestal utility  
/*!
  Pedestal utility
*/
typedef struct {
    unsigned int unixTimeStart;
    unsigned int unixTimeEnd;
    unsigned short chipEntries[ACTIVE_SURFS][LABRADORS_PER_SURF];
    unsigned int mean[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
    unsigned int meanSq[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
    unsigned int entries[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
    float fmean[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
    float frms[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
} PedCalcStruct_t;

//! Pedestal utility  
/*!
  Pedestal utility
*/
typedef struct {
    unsigned int unixTime; ///< Corresponds to unixTimeEnd above
    unsigned int nsamples; ///< What was the mean occupancy
  unsigned short thePeds[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES]; ///< mean pedestal 
    unsigned short pedsRMS[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES]; ///< 10 x RMS of the samples (not of mean)
} PedestalStruct_t;


typedef struct {
  short bins[99];
} GpuAnitaBandPowerSpectrumStruct_t;

typedef struct {
  GenericHeader_t gHdr;
  unsigned int unixTimeFirstEvent;
  unsigned int unixTimeLastEvent;
  unsigned int numEventsAveraged;
  unsigned int firstEventInAverage;
  unsigned char phiSector;
  unsigned char nothing;
  GpuAnitaBandPowerSpectrumStruct_t powSpectra[NUM_ANTENNA_RINGS][2];
} GpuPhiSectorPowerSpectrumStruct_t;




/*! Struct to store RTL data 
 *
 *
 *  Short is overkill for the dynamic range, and probably makes the packet too big... so maybe this will change to a char soon. 
 *  Only first nFreq of a spectrum non-zero. 
 */ 
typedef struct 
{
  GenericHeader_t gHdr; 
  unsigned int nFreq;  //< number of frequency bins actually stored
  unsigned int startFreq;  //< start frequency of output, in Hz
  unsigned int freqStep;  //< frequency step of output, in Hz 
  unsigned int unixTimeStart;  //< time when scan was started (unix time) 
  unsigned short scanTime;  //<  approximate time it scan to finish finished (in decisecs). 
  unsigned short gain;  //< LNA gain, in cBm (i.e. 10 * dBm) 
  unsigned char spectrum [RTLSDR_MAX_SPECTRUM_BINS]; //< power spectra... see anitaFlight.h for units
  unsigned char rtlNum ; //<which RTL is this? This is the SERIAL NUMBER (RTL%d), not the device enumeration order ( 4 bits is sufficient for < 15 devices) 
} RtlSdrPowerSpectraStruct_t; 


/*! 
 * Struct to store TUFF start and end phi sectors 
 **/ 
typedef struct __attribute__((packed)) 
{
  GenericHeader_t gHdr; 
  unsigned int notchSetTime;  // The last time the notches were set
  unsigned int unixTime;  // The approximate time that this was written. If temperatures are read, the temperatures are read around this time
  unsigned char startSectors[NUM_TUFF_NOTCHES]; //start sectors... 0 to 15 for real sectors, start and stop on 16 means nothing is enabled
  unsigned char endSectors[NUM_TUFF_NOTCHES]; //stop sectors... 0 to 15 for real sectors, start and stop on 16 means nothing is enabled
  char temperatures[NUM_RFCM];  //-128 if temperatures are not being read 
} TuffNotchStatus_t; 

typedef struct __attribute__((packed))
{
  GenericHeader_t gHdr; 
  unsigned int requestedTime; 
  unsigned int enactedTime; 
  unsigned short cmd; 
  unsigned char irfcm : 4; 
  unsigned char tuffStack : 4; 
} TuffRawCmd_t; 



/////////////////////////////////////////////////////////////////////////////
///// Slow Rate Stuff                                                   /////
/////////////////////////////////////////////////////////////////////////////


#endif /* ANITA_STRUCTURES_H */
