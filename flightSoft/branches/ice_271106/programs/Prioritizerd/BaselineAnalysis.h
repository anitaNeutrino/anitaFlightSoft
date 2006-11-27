#ifndef BASELINEANALYSIS_H
#define BASELINEANALYSIS_H
#include <anitaStructures.h>
#include <anitaFlight.h>
#include "AnitaInstrument.h"

#ifdef __cplusplus
extern "C"
{
#endif
     float det3(float a[3][3]);
     float det3p(float *a[3]);
     int solve3(float a[3][3], float b[3], float x[3]);
     int FitBaselines(BaselineAnalysis_t *theBL);
#ifdef __cplusplus
}
#endif
#endif /* BASELINEANALYSIS_H */
