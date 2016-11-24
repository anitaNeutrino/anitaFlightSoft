#ifndef MAGNETIC_LIB_H
#define MAGNETIC_LIB_H

#include <time.h>


typedef struct magnetic_answer 
{
  double D; /*declination */
  double I; /*inclination  */
  double H; /*horizontal field strength */
  double N; /* north component */ 
  double E; /* east component  */  
  double Z; /* down component */
  double F; /* total field strength */ 

  /* derivatives of above */
  double dD, dI, dH, dN, dE, dZ, dF; 
} magnetic_answer_t; 


int magneticModel(const char * model_file, magnetic_answer_t * answer, 
           int year, int month, int day, 
           double longitude, double latitude, double altitude_in_km); 



int solveForAttitude( const double Bfield[3], const double BfieldErr[3],
                       double northVector[3], double northVectorErr[3], 
                       double lon, double lat, double alt, time_t t0 ); 







#endif
