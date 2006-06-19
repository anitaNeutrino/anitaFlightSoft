/* File Prioritizerd.h */
#ifndef PRIORITIZERD_H
#define PRIORITIZERD_H

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>


#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"

/* Structure defininitions */

typedef struct {
    float re; 
    float im; 
} complex;  /* for FFT */

typedef struct {
    float data[MAX_NUMBER_SAMPLES];
} channel_volts;

typedef struct {
    short numChannels;
    short numSamples;
    channel_volts channel[NUM_DIGITZED_CHANNELS];
} event_volts;

/* Function Definitions */

void writePacketsAndHeader(AnitaEventBody_t *bodyPtr, AnitaEventHeader_t *hdPtr); 

/* int setPriority(AnitaEventHeader_t *headerPtr, AnitaEventBody_t *bodyPtr); */
/* void convertToVoltageAndGetStats(AnitaEventBody_t *bodyPtr, event_volts *voltsPtr, double means[], double rmss[], int peakBins[], int trigPeakBins[], int trigBin, int trigHalfWindow); */
/* void nrFFTForward(complex data[], int nn); */
/* void nrFFT(float data[], int nn, int isign); */
/* void getPowerSpec(event_volts *voltsPtr, float *pwr[], float *freq, float fs1); */
/* int checkPayloadFrequencies( float *power[], float freq[], int numChannels, int numFreqs); */
/* int writeOutputFiles(AnitaEventHeader_t *hdPtr, AnitaEventBody_t *bodyPtr, char eventDir[], char backupDir[], char linkBaseDir[], float probWriteOut9); */

/* Directories and gubbins */
extern int useUsbDisks, maxEventsPerDir;
extern char eventdEventDir[FILENAME_MAX];
extern char eventdEventLinkDir[FILENAME_MAX];
extern char headerTelemDir[FILENAME_MAX];
extern char headerTelemLinkDir[FILENAME_MAX];
extern char eventTelemDir[NUM_PRIORITIES][FILENAME_MAX]; 
extern char eventTelemLinkDir[NUM_PRIORITIES][FILENAME_MAX]; 

extern char eventArchiveDir[FILENAME_MAX];
extern char eventUSBArchiveDir[FILENAME_MAX];
extern char prioritizerdPidFile[FILENAME_MAX];
#endif /* PRIORITIZERD_H */

