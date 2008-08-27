#ifndef GENERATESCORE_H
#define GENERATESCORE_H_H
#include <anitaStructures.h>
#include <anitaFlight.h>
#include "AnitaInstrument.h"

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
