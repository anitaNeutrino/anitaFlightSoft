#ifndef GENERATESCORE_H
#define GENERATESCORE_H_H
#include <anitaStructures.h>
#include <anitaFlight.h>
#include "AnitaInstrument.h"

typedef struct {
     int overallPeakSize[3]; //
     int overallPeakLoc[3]; //
     int overallPeakPhi[3]; //
     int overallPeakRing[3]; //
     int numPeaks[PHI_SECTORS][2]; //
    int peakSize[3][PHI_SECTORS][2]; // 0 is upper, 1 is lower
     int peakLocation[3][PHI_SECTORS][2]; //
     int totalOccupancy[PHI_SECTORS][2]; //
} AnitaSectorAnalysis_t;
    
#ifdef __cplusplus
extern "C"
{
#endif
    void AnalyseSectorLogic(AnitaSectorLogic_t *secLogPtr,
			    AnitaSectorAnalysis_t *secAnaPtr);		
    int GetSecAnaScore(AnitaSectorAnalysis_t *secAnaPtr);
#ifdef __cplusplus
}
#endif
#endif /* GENERATESCORE_H_H_H */
