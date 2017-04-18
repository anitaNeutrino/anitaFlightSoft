#ifndef MAGNETIC_LIB_H
#define MAGNETIC_LIB_H

#include <time.h>

#ifdef __cplusplus
extern"C"{
#endif

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


int magneticModel(const char * mdfile, magnetic_answer_t * answer, 
           int year, int month, int day, 
           double longitude, double latitude, double alt); //alt in km



int solveForAttitude( const double Bfield[3], const double BfieldErr[3],
                       double attitude[3], double attitudeErr[3], 
                       double lon, double lat, double alt, time_t t0 ); 



#ifdef __cplusplus
}
#endif



#endif
