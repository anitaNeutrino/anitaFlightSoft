#ifndef ANTIA_TIMING_CALIB_H
#define ANTIA_TIMING_CALIB_H

/* Standard shizniz */
#include <math.h>
#include <string.h>

/* For interpolation */
#include "gsl/gsl_interp.h"
#include "gsl/gsl_spline.h"
#include "gsl/gsl_errno.h"

/* for CPU FFTs */
#include <fftw3.h>

/* Anita flight software thingymebobs */
#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"

#include "myInterferometryConstants.h"

/* Functions, if only C had constructors and destructors... */
void prepareTimingCalibThings();
void tidyUpTimingCalibThings();

/* Boring read in calibration data functions... */
void readInCalibratedDeltaTs(const char* fileName);
void readInEpsilons(const char* fileName);
void readInGainCalib(const char* fileName);
void readInVoltageCalib(const char* fileName);
void readInRelativeCableDelay(const char* fileName);


void doTimingCalibration(int entry, AnitaEventHeader_t* theHeader, PedSubbedEventBody_t pedSubBody, double* finalVolts[], int disableGpu);


double* interpolateWaveform(int nRaw, double* rawWave, double* times,
			    int nInterp, double t0interp, double dtNsInterp);
double* linearlyInterpolateWaveform(int nRaw, double* rawWave, double* unevenTimes,
				    int nInterp, double t0interp, double dtNsInterp);


double* simpleBandPass(double* volts, int length, double dt, double highPassMHz, double lowPassMHz);

double findClockJitterCorrection(int numSamples, fftw_complex* clockFreqs1, fftw_complex* clockFreqs2, double deltaT_ns, int surf, int lab);

void normalize(int numSamples, double* data);
void readInRcoLatchDelay(const char* fileName);

int findIndexOfMaximumWithinLimits(double* array, int startInd, int stopInd);
double findClockJitterCorrectionLikeRyan(int n1, double* clock1, int n2, double* clock2, double deltaT_ns, int surf, int lab);

void justUnwrapVolts(PedSubbedEventBody_t pedSubBody);
void preCalculateTimeArrays();



/*
   These are copied from AnitaEventCalibrator.cxx (and other useful classes) in Ryan's ROOT things
   then edited to compile in C.
*/
// void processEventAG(AnitaEventHeader_t theHeader, PedSubbedEventBody_t pedSubBody);
//void processEventAG(int entry, AnitaEventHeader_t theHeader, PedSubbedEventBody_t pedSubBody);
//void processEventAG(int entry, PedSubbedEventBody_t pedSubBody);
int getLabChip(PedSubbedEventBody_t pedSubBody, int chanInd);
int getRco(PedSubbedEventBody_t pedSubBody, int chanInd);

int getEarliestSample(PedSubbedEventBody_t pedSubBody, int chanInd);
int getLatestSample(PedSubbedEventBody_t pedSubBody, int chanInd);

int getLastHitBus(PedSubbedEventBody_t pedSubBody, int chanInd);
int getFirstHitBus(PedSubbedEventBody_t pedSubBody, int chanInd);
int getWrappedHitBus(PedSubbedEventBody_t pedSubBody, int chanInd);


void processEventAG(PedSubbedEventBody_t pedSubBody);


static const int surfToAntMap[ACTIVE_SURFS][RFCHAN_PER_SURF]= {{-42,-34,-48,-40,42,34,48,40},
							       {-44,-36,-46,-38,44,36,46,38},
							       {-32,-24,-28,-20,32,24,28,20},
							       {-30,-22,-26,-18,30,22,26,18},
							       {-12,4,-14,-6,12,-4,14,6},
							       {-10,-2,-16,-8,10,2,16,8},
							       {-45,-37,-41,-33,45,37,41,33},
							       {-47,-39,-43,-35,47,39,43,35},
							       {-27,-19,-29,-21,27,19,29,21},
							       {-25,-17,-31,-23,25,17,31,23},
							       {-15,-7,-11,-3,15,7,11,3},
							       {-13,-5,-9,-1,13,5,9,1}};

#endif /*ANTIA_TIMING_CALIB_H*/
