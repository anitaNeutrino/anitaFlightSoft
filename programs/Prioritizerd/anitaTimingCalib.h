#ifndef ANTIA_TIMING_CALIB_H
#define ANTIA_TIMING_CALIB_H

/* Generic */
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

/* Functions */
void prepareTimingCalibThings();
void tidyUpTimingCalibThings();



void readInCalibratedDeltaTs(const char* fileName);
void readInEpsilons(const char* fileName);
void readInGainCalib(const char* fileName);
void readInClockCrossCorr(const char* fileName);

void getClockPhiNums(); /* development/testing only*/

void giveChan3ToChan2(int* a);
void doTimingCalibration(int entry, AnitaEventHeader_t theHeader, PedSubbedEventBody_t pedSubBody, double* finalVolts[]);
double* interpolateWaveform(int nRaw, double* rawWave, double* times, 
			    int nInterp, double t0interp, double dtNsInterp);
double findClockJitterCorrection(int numSamples, double* clock1, double* clock, double deltaT_ns, int surf, int lab);

void normalize(int numSamples, double* data);

int findIndexOfMaximumWithinLimits(double* array, int startInd, int stopInd);

FILE* fopenNicely(const char* fileName, const char* opt);

void justUnwrapVolts(PedSubbedEventBody_t pedSubBody);
void preCalculateTimeArrays();//(PedSubbedEventBody_t pedSubBody);
//void preCalculateTimeArrays(PedSubbedEventBody_t pedSubBody);



/* 
   These are copied from AnitaEventCalibrator.cxx (and other useful classes) in Ryan's ROOT things
   then edited to compile in C.
*/
// void processEventAG(AnitaEventHeader_t theHeader, PedSubbedEventBody_t pedSubBody);
//void processEventAG(int entry, AnitaEventHeader_t theHeader, PedSubbedEventBody_t pedSubBody);
void processEventAG(int entry, PedSubbedEventBody_t pedSubBody);
int getLabChip(PedSubbedEventBody_t pedSubBody, int chanInd);
int getRco(PedSubbedEventBody_t pedSubBody, int chanInd);

int getEarliestSample(PedSubbedEventBody_t pedSubBody, int chanInd);
int getLatestSample(PedSubbedEventBody_t pedSubBody, int chanInd);

int getLastHitBus(PedSubbedEventBody_t pedSubBody, int chanInd);
int getFirstHitBus(PedSubbedEventBody_t pedSubBody, int chanInd);
int getWrappedHitBus(PedSubbedEventBody_t pedSubBody, int chanInd);


#endif /*ANTIA_TIMING_CALIB_H*/
