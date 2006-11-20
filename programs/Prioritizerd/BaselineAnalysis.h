#ifndef BASELINEANALYSIS_H
#define BASELINEANALYSIS_H
#include <anitaStructures.h>
#include <anitaFlight.h>
#include "AnitaInstrument.h"

#define MAX_BASELINES 16
#define MAX_ITER 10

typedef struct {
     int nbaselines; //filled by builder
     int ngood; //count the baselines that are not bad.  
                //If there are too few to perform the fit, 
                //don't fit.  Filled by fitter.
     int validfit; //zero for good fit; otherwise an error code
     //-1 : a singular matrix was found in the initial fit
     //-2 : a singular matrix was found in the Lagrange multiplier search
     //-3 : too many iterations in the Lagrange search
     //-4 : too few baselines to fit
     int niter; //number of itereations to get norm right
     float lambda; //Lagrange multiplier for norm constraint
     float arrival[3]; //best fit to vector from instrument to
                       //source, normalized on the unit sphere
     float norm; //fitted length of direction vector
     float position[MAX_BASELINES][2][3]; //convenience copies of
     float delay[MAX_BASELINES][2]; //time in meters
     float direction[MAX_BASELINES][3]; //unit vector from 
                                        //late to early antenna
     float length[MAX_BASELINES]; //length of baseline
     float sinangle[MAX_BASELINES]; //sine has errors which 
                                    //are linear in delay
     float cosangle[MAX_BASELINES]; //used for statB
     int bad[MAX_BASELINES]; //nonzero for 'bad' baselines
                             //integer gives pass on which baseline
                             //was rejected
} BaselineAnalysis_t;
    
#ifdef __cplusplus
extern "C"
{
#endif
     float det3(float a[3][3]);
     float det3p(float *a[3]);
     int solve3(float a[3][3], float b[3], float x[3]);
     int fitBaselines(BaselineAnalysis_t *theBL);
#ifdef __cplusplus
}
#endif
#endif /* BASELINEANALYSIS_H */
