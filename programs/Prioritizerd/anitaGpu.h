/*
  Author: Ben Strutt
  Email: b.strutt.12@ucl.ac.uk
  Description: My attempt to hide all the GPU complexities behind some plug in and play functions...
*/


#ifndef ANITA_GPU
#define ANITA_GPU



/* c library include */
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>

/* My stuff */
#include "openCLwrapper.h"
#include "myInterferometryConstants.h"
#ifdef TSTAMP
#include "tstamp.h"
#endif

/* Anita flight types */
#include "includes/anitaStructures.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"

/* Stolen from pedestalLib.c */
#define MAX_VAL 600


/*
   If you want to take a look at how long things are taking,
   then keep "#define TSTAMP", although bear in mind that
   things are moving towareds a non-blocking configuration.
*/

/***********************/
/* Function prototypes */
/***********************/

/* calculates the deltaT between two antennas for a plane wave arriving from a given direction */
float getDeltaTExpected(int ant1, int ant2,double phiWave, double thetaWave,
			const float* phiArray, const float* rArray, const float* zArray);


/* generates look up deltaT table for GPU interferometry calcs */
// short* fillDeltaTArrays();
float* fillDeltaTArrays();

/* for measuring speed of GPU calcs */
void timeStamp(int stampNo, int numEvents, cl_event* waitList);

/* Simple named functions which handle initialization, calculation and destruction of GPU objects */
void prepareGpuThings();
void tidyUpGpuThings();
//void addEventToGpuQueue(int eventInd, PedSubbedEventBody_t pedSubBody, AnitaEventHeader_t theHeader, const int alreadyUnwrappedAndCalibrated);

void addEventToGpuQueue(int eventInd, double* finalVolts[], AnitaEventHeader_t theHeader);
void mainGpuLoop(int nEvents, AnitaEventHeader_t* theHeader, GpuPhiSectorPowerSpectrumStruct_t* payloadPowSpec, int writePowSpecPeriodSeconds);

void assignPriorities(int nEvents, AnitaEventHeader_t* theHeader, float* imagePeakVal);
float compareForSort(const void* a, const void* b);


int unwrapAndBaselinePedSubbedEventBen(PedSubbedEventBody_t *pedSubBdPtr, AnitaTransientBodyF_t *uwTransPtr, const int alreadyUnwrappedAndCalibrated);

void updateLongDynamicPassFilter(int pol, int ant, short* longDynamicPassFilter, int* isLocalMinimum, int* isLocalMaximum,  float spikeThresh_dB, int startFreqInd, int endFreqInd);

/***********************/
/*       Structs       */
/***********************/
typedef struct {
  float betterImagePeakVal;
  unsigned short eventIndex;
} PrioritySort_t;


/***********************/
/*       Globals       */
/***********************/

/*
static const int anita3ChanInd[16][3][2] = { { { 102, 106 }, { 82, 86 }, { 57, 61 } },
					     { { 46, 50 }, { 30, 34 }, { 1, 5 } },
					     { { 93, 97 }, { 73, 77 }, { 66, 70 } },
					     { { 41, 37 }, { 21, 25 }, { 10, 14 } },
					     { { 100, 104 }, { 75, 79 }, { 55, 59 } },
					     { { 39, 43 }, { 28, 32 }, { 12, 16 } },
					     { { 91, 95 }, { 84, 88 }, { 64, 68 } },
					     { { 48, 52 }, { 19, 23 }, { 3, 7 } },
					     { { 101, 105 }, { 81, 85 }, { 56, 60 } },
					     { { 45, 49 }, { 29, 33 }, { 0, 4 } },
					     { { 92, 96 }, { 72, 76 }, { 65, 69 } },
					     { { 36, 40 }, { 20, 24 }, { 9, 13 } },
					     { { 99, 103 }, { 74, 78 }, { 54, 58 } },
					     { { 38, 42 }, { 27, 31 }, { 11, 15 } },
					     { { 90, 94 }, { 83, 87 }, { 63, 67 } },
					     { { 47, 51 }, { 18, 22 }, { 2, 6 } } };
*/

/* Copied from AnitaGeomTool 27/11/2014*/
static const int antToSurfMap[NUM_ANTENNAS]={11,5,10,4,11,4,10,5,11,5,10,4,11,4,10,5,
					     9,3,8,2,8,3,9,2,9,3,8,2,8,3,9,2,
					     6,0,7,1,6,1,7,0,6,0,7,1,6,1,7,0};


static const int hAntToChan[NUM_ANTENNAS]={7,5,7,1,5,7,5,7,6,4,6,4,4,6,4,6,
					   5,7,5,7,7,5,7,5,4,6,4,6,6,4,6,4,
					   7,5,7,5,5,7,5,7,6,4,6,4,4,6,4,6};


static const int vAntToChan[NUM_ANTENNAS]={3,1,3,5,1,3,1,3,2,0,2,0,0,2,0,2,
					   1,3,1,3,3,1,3,1,0,2,0,2,2,0,2,0,
					   3,1,3,1,1,3,1,3,2,0,2,0,0,2,0,2};


#define maxPlatforms 2
cl_uint myPlatform;
cl_uint myDevice;
cl_platform_id platformIds[maxPlatforms];

cl_int status;

extern cl_context context;

extern size_t dlistSize;
extern cl_device_id deviceList[2];

extern cl_command_queue commandQueue;

extern uint showCompileLog;
extern cl_uint numDevicesToUse;

extern cl_program prog;

extern cl_mem_flags memFlags;

extern cl_kernel normalizationKernel;

/* I/O buffers */
extern buffer* rawBufferVPol;
extern buffer* numSampsBufferVPol;
extern buffer* phiSectorTriggerBufferVPol;
extern buffer* rawBufferHPol;
extern buffer* numSampsBufferHPol;
extern buffer* phiSectorTriggerBufferHPol;

extern buffer* numEventsInQueueBuffer;
extern buffer* binToBinDifferenceThresh_dBBuffer;
extern buffer* absMagnitudeThresh_dBmBuffer;

/* Internal buffers */
extern buffer* rmsBuffer;
extern buffer* squareBuffer;
extern buffer* meanBuffer;
extern buffer* normalBuffer;

extern cl_kernel fourierKernel;
extern buffer* fourierBuffer;
extern buffer* complexScratchBuffer;
extern buffer* numEventsBuffer;

extern cl_kernel eventPowSpecKernel;
extern buffer* powSpecBuffer;
extern buffer* passFilterBuffer;
extern buffer* staticPassFilterBuffer;
extern buffer* longDynamicPassFilterBuffer;
extern buffer* powSpecScratchBuffer;
extern buffer* isMinimaBuffer;
extern buffer* passFilterLocalBuffer;

extern cl_kernel filterWaveformsKernel;
extern buffer* newRmsBuffer;
extern buffer* complexSquareLocalBuffer;

extern cl_kernel circularCorrelationKernel;
extern buffer* circularCorrelationBuffer;
extern buffer* complexCorrScratch;

extern cl_kernel imageKernel;
extern buffer* imageBuffer;
extern buffer* lookupBuffer;

// short* offsetInd;
extern float* offsetInd;

extern cl_kernel findImagePeakInThetaKernel;
extern buffer* corrScratchBuffer4;
extern buffer* indScratchBuffer4;
extern buffer* maxThetaIndsBuffer;

extern cl_kernel findImagePeakInPhiSectorKernel;
extern buffer* imagePeakValBuffer;
extern buffer* imagePeakThetaBuffer;
extern buffer* imagePeakPhiBuffer;
extern buffer* corrScratchBuffer2;
extern buffer* phiIndScratchBuffer;
extern buffer* thetaIndScratchBuffer;

extern cl_kernel findMaxPhiSectorKernel;
extern buffer* imagePeakValBuffer2;
extern buffer* imagePeakThetaBuffer2;
extern buffer* imagePeakPhiBuffer2;
extern buffer* imagePeakPhiSectorBuffer;
extern buffer* corrScratchBuffer3;
extern buffer* phiIndScratchBuffer2;
extern buffer* thetaIndScratchBuffer2;
extern buffer* phiSectScratchBuffer;

extern cl_kernel iFFTFilteredWaveformsKernel;
extern buffer* complexScratchBuffer2;

extern cl_kernel coherentSumKernel;
extern buffer* coherentWaveBuffer;

extern cl_kernel hilbertKernel;
extern buffer* hilbertBuffer;
extern buffer* complexScratchBuffer3;

extern cl_kernel hilbertPeakKernel;
extern buffer* hilbertPeakBuffer;
extern buffer* scratchBuffer;
extern buffer* invFftBuffer;

extern struct timeval startTime;
extern struct timezone dummy;
extern struct rusage resourcesStart;


cl_event normalEvent;
static const size_t gNormWorkSize[3] = {NUM_SAMPLES/4, NUM_ANTENNAS, NUM_EVENTS};
static const size_t lNormWorkSize[3] = {NUM_SAMPLES/4, 1, 1};

cl_event fourierEvent;
static const size_t gFourierWorkSize[3] = {NUM_SAMPLES/4, NUM_ANTENNAS, NUM_EVENTS};
static const size_t lFourierWorkSize[3] = {NUM_SAMPLES/4, 1, 1};

cl_event powSpecEvent;
static const size_t gPowSpecWorkSize[2] = {NUM_SAMPLES/4, NUM_ANTENNAS};
static const size_t lPowSpecWorkSize[2] = {NUM_SAMPLES/4, 1};

cl_event filterEvent;
static const size_t gFilterWorkSize[3] = {NUM_SAMPLES/4, NUM_ANTENNAS, NUM_EVENTS};
static const size_t lFilterWorkSize[3] = {NUM_SAMPLES/4, 1, 1};

cl_event circCorrelationEvent;
static const size_t gCircCorrWorkSize[3] = {NUM_SAMPLES/4, NUM_PHI_SECTORS*NUM_GLOBAL_COMBOS_PER_PHI_SECTOR, NUM_EVENTS};
static const size_t lCircCorrWorkSize[3] = {NUM_SAMPLES/4, 1, 1};

cl_event peakCorrEvent;
static const size_t gPeakCorrWorkSize[3] = {NUM_SAMPLES/4, NUM_PHI_SECTORS*NUM_GLOBAL_COMBOS_PER_PHI_SECTOR, NUM_EVENTS};
static const size_t lPeakCorrWorkSize[3] = {NUM_SAMPLES/4, 1, 1};

cl_event imageEvent;
static const size_t gImageWorkSize[3] = {NUM_BINS_THETA/4, NUM_BINS_PHI, NUM_EVENTS};
static const size_t lImageWorkSize[3] = {NUM_BINS_THETA/4, 1, 1};

cl_event findImagePeakInThetaEvent;
static const size_t gImageThetaWorkSize[3] = {NUM_BINS_THETA, NUM_BINS_PHI, NUM_EVENTS};
static const size_t lImageThetaWorkSize[3] = {NUM_BINS_THETA, 1, 1};

cl_event findImagePeakInPhiSectorEvent;
static const size_t gFindImagePeakWorkSize[3] = {NUM_BINS_PHI, NUM_PHI_SECTORS, NUM_EVENTS};
static const size_t lFindImagePeakWorkSize[3] = {NUM_BINS_PHI, 1, 1};

cl_event findMaxPhiSectorEvent;
static const size_t gFindImagePeakWorkSize2[2] = {NUM_PHI_SECTORS, NUM_EVENTS};
static const size_t lFindImagePeakWorkSize2[2] = {NUM_PHI_SECTORS, 1};

cl_event invFFTWaveformsEvent;
static const size_t gInvFFTSize[3] = {NUM_SAMPLES/4, NUM_ANTENNAS, NUM_EVENTS};
static const size_t lInvFFTSize[3] = {NUM_SAMPLES/4, 1, 1};

cl_event coherentSumEvent;
static const size_t gCoherentSumKernelWorkSize[2] = {NUM_SAMPLES/4, NUM_EVENTS};
static const size_t lCoherentSumKernelWorkSize[2] = {NUM_SAMPLES/4, 1};

cl_event hilbertEvent;
static const size_t gHilbertWorkSize[2] = {NUM_SAMPLES/4, NUM_EVENTS};
static const size_t lHilbertWorkSize[2] = {NUM_SAMPLES/4, 1};

cl_event hilbertPeakEvent;
static const size_t gHilbertPeakWorkSize[2] = {NUM_SAMPLES/4, NUM_EVENTS};
static const size_t lHilbertPeakWorkSize[2] = {NUM_SAMPLES/4, 1};


/* Input */
short* numEventSamples;
float* eventData;
short phiTrig[NUM_POLARIZATIONS*NUM_EVENTS*NUM_PHI_SECTORS];
unsigned short* prioritizerStuffs;

/* Output */
float imagePeakVal[NUM_POLARIZATIONS*NUM_EVENTS];
float hilbertPeak[NUM_POLARIZATIONS*NUM_EVENTS];
int imagePeakTheta2[NUM_POLARIZATIONS*NUM_EVENTS];
int imagePeakPhi2[NUM_POLARIZATIONS*NUM_EVENTS];
int imagePeakPhiSector[NUM_POLARIZATIONS*NUM_EVENTS];
float* powSpec;
short* passFilter;
short* longDynamicPassFilter;
short* staticPassFilter;
float* tempPowSpecHolder;
// short* anyFailDifference;
// short* anyFailMagnitude;

/* Variables from the opencl API to synchoronize output */
cl_event readImagePeakVal;
cl_event readImagePeakTheta;
cl_event readImagePeakPhi;
cl_event readImagePeakPhiSector;
cl_event readHilbert;


#endif /* ANITA_GPU */
