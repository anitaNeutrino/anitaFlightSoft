#include <time.h>
#include <stdio.h>
#include <stdlib.h>            
#include <string.h>
#include <ctype.h>
#include <math.h> 
#include "magneticLib.h"

//magnetometer is in the center of phi sector 4 on anita4, so probably 3 phi sector heading shift is necessary

#define NaN log(-1.0)
#define FT2KM (1.0/0.0003048)
#define PI 3.141592654
#define RAD2DEG (180.0/PI)



static void crossProduct(const double a[3], const double b[3], double v[3]) 
{
  v[0] = a[1]*b[2] - a[2]*b[1];
  v[1] = a[2]*b[0] - a[0]*b[2];
  v[2] = a[0]*b[1] - a[1]*b[0];
}

static double dotProduct(const double a[3], const double b[3]) 
{
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; 
}




static double vabs (const double v[3]) 
{
  return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]); 
}

//Bfield is str8 from magnetometer, b is calculated from ADU5 position
int solveForAttitude( const double Bfield[3], const double BfieldErr[3],
                       double attitude[3], double attitudeErr[3], 
                       double lon, double lat, double alt, time_t t0) 
{


  struct tm t; 
  int ret; 
  magnetic_answer_t mag; 
  gmtime_r(&t0, &t); 
  ret = magneticModel("IGRF12.COF",&mag, t.tm_year+1900, t.tm_mon+1, t.tm_mday+1, lon,lat,alt/1000); 
	//time and alt are modified to match how magneticModel expects it


  //this is the expected magnetic field in a right-handed-coordinate system
  double b[3]; 

  b[0] = mag.N * 1e-5; //magnetometer reads out in gauss, program give nT
  b[1] = mag.E * 1e-5; 
  b[2] = mag.Z * 1e-5;

  double Bf[3] = {Bfield[0], Bfield[1], Bfield[2] - BfieldErr[2]}; // correct calibration error in z
	double v[3];
  crossProduct(Bf, b, v); 
  double c = dotProduct(Bf,b); 

	double heading = atan2(v[2], c - Bf[2]*b[2]);

	//for now gonna fill attitudeErr with these rotated guys to help with calibration

	//now that we have the heading, rotate into it and find pitch next
	//correct for calibration error here after rotating into heading
	double BfieldR[3] = {cos(heading) * Bfield[0] - sin(heading) * Bfield[1] - BfieldErr[0], sin(heading) * Bfield[0] + cos(heading) * Bfield[1] - BfieldErr[1], Bf[2]};

	attitudeErr[0] = BfieldR[0];
	attitudeErr[1] = BfieldR[1];
	attitudeErr[2] = BfieldR[2];

	crossProduct(BfieldR, b, v);
	c = dotProduct(BfieldR, b);

	double pitch = atan2(v[1], c - BfieldR[1]*b[1]);

	//now that we have pitch, rotate again for roll
	double BfieldRR[3] = {cos(pitch) * BfieldR[0] + sin(pitch) * BfieldR[2], BfieldR[1], -sin(pitch) * BfieldR[0] + cos(pitch) * BfieldR[2]};

	crossProduct(BfieldRR, b, v);
	c = dotProduct(BfieldRR, b);

	double roll = atan2(v[0], c - BfieldRR[0]*b[0]);

	double phi_sector = 22.5; //magnetometer seems to be ~4.5 sector over for anita 4
	heading = -1*heading * RAD2DEG + 180 + 4.5 * phi_sector;//anita 4 needs this -1 but i dont really know why

	attitude[0] = heading; //all our readings are in degrees
	attitude[1] = pitch * RAD2DEG;
	attitude[2] = roll * RAD2DEG;

  return 0; 
} 



