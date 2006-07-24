#ifndef HILBERT_H
#define HILBERT_H

#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif

     void makeHilbertCoefficients();
     double hilbertFilter(int k, double *a);
     double hilbertSymFilter(int k, double *a);
     int intHilbertFilter(int k, int *a);
     int intHilbertSymFilter(int k, int *a); 

#ifdef __cplusplus
}
#endif

#endif /* HILBERT_H */

