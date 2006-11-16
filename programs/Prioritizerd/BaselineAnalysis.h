#ifndef GENERATESCORE_H
#define GENERATESCORE_H_H
#include <anitaStructures.h>
#include <anitaFlight.h>
#include "AnitaInstrument.h"

#define MAX_BASELINES 16

typedef struct {
     int nbaselines;
     float position[MAX_BASELINES][2][3];
     float boresight[MAX_BASELINES][2][3];
     float time[MAX_BASELINES][2]; //time difference in meters
     float direction[MAX_BASELINES][3];
     float angle[MAX_BASELINES];
     int valid[MAX_BASELINES];
} BaselineAnalysis_t;
    
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
