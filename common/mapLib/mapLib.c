/*! \file mapLib.c
    \brief Map library converts to and from things
    
    Some functions that do some useful things
    June 2006  rjn@mps.ohio-state.edu
*/

#include "mapLib/mapLib.h"
#include "includes/anitaStructures.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


//Constants
//const double M_PI=3.14159265358979;
const double RADTODEG=M_PI/180.;   // radians/degree  
const double DEGTORAD=180./M_PI;    // degree/rad

void fillPhiFromSurf(SurfAntMapStruct_t *mapPtr) {
    mapPtr->phi=-1;
    mapPtr->tier=kUnknownTier;
    mapPtr->antenna=-1;
    mapPtr->pol=kUnknownPol;

    if(mapPtr->surf<0 || mapPtr->surf>8 ||
       mapPtr->channel<0 || mapPtr->channel>7)
	return;

    mapPtr->antenna=surfToAntMap[mapPtr->surf][mapPtr->channel];
    if(mapPtr->surf==8) {	
	mapPtr->pol=kVertical;
	
	//Deal with discones and bicones here
	return;
    }


    mapPtr->pol=kHorizontal;
    if(mapPtr->antenna<0) {
	mapPtr->antenna*=-1;
	mapPtr->pol=kVertical;
    }
    mapPtr->antenna--;

    if(mapPtr->antenna<16) {
	//Upper rings
	if(mapPtr->antenna<8)
	    mapPtr->tier=kUpperTier;
	else
	    mapPtr->tier=kMiddleTier;	
	mapPtr->phi=upperPhiNums[mapPtr->antenna];
    }
    else {
	mapPtr->tier=kLowerTier;
	mapPtr->phi=lowerPhiNums[mapPtr->antenna-16];
    }
}


void fillSurfFromPhiAndTierAndPol(SurfAntMapStruct_t *mapPtr) {
    mapPtr->surf=-1;
    mapPtr->channel=-1;
    mapPtr->antenna=-1;
//    mapPtr->pol=kUnknownPol;

    //Need to add something for discones
    if(mapPtr->tier!=kUpperTier && 
       mapPtr->tier!=kMiddleTier &&
       mapPtr->tier!=kLowerTier)
	return;
    if(mapPtr->phi<0 || mapPtr->phi>15) 
	return;
    if(mapPtr->pol!=kHorizontal && mapPtr->pol!=kVertical)
	return;

	    
    if(mapPtr->tier==kUpperTier || mapPtr->tier==kMiddleTier)
	mapPtr->antenna=upperAntNums[mapPtr->phi];
    else if(mapPtr->tier==kLowerTier)
	mapPtr->antenna=lowerAntNums[mapPtr->phi];

    mapPtr->surf=antToSurfMap[mapPtr->antenna];
    if(mapPtr->pol==kHorizontal)
	mapPtr->channel=hAntToChan[mapPtr->antenna];
    if(mapPtr->pol==kVertical)
	mapPtr->channel=vAntToChan[mapPtr->antenna];
    
}

int getLogicalIndexFromAnt(int ant,  AntennaPol_t pol) 
{
    switch(pol) {
	case kHorizontal:
	    return GetChanIndex(antToSurfMap[ant],hAntToChan[ant]);
	case kVertical:
	    return GetChanIndex(antToSurfMap[ant],vAntToChan[ant]);
	default:	    
	    fprintf(stderr,"Unknown polarisation %d\n",pol);
	    return -1;
    }
    return -1;	       

}

int getLogicalIndexFromPhi(int phi,  AntennaTier_t tier, AntennaPol_t pol)
{
    switch(tier) {
	case kUpperTier:
	case kMiddleTier:
	    return getLogicalIndexFromAnt(upperAntNums[phi],pol);
	case kLowerTier:
	    return getLogicalIndexFromAnt(lowerAntNums[phi],pol);
	case kDisconeTier:
	    return disconeIndexArray[phi/4];
	case kBiconeTier:
	    return biconeIndexArray[phi/4];
	default:
	    fprintf(stderr,"Unknown tier %d\n",tier);
	    return -1;
    }
    return -1;	       
}

void ltLgAlToGeoXYZ(double latLonAlt[3], double xyz[3])
{
  double lat, lon, h;
  lat = latLonAlt[0]*RADTODEG;
  lon = latLonAlt[1]*RADTODEG;
  h   = latLonAlt[2];
  double r = 6378137.0;//Earth's equatorial radius
  double f = 1/298.257223563;//flattening factor
  double e2 = f*(2-f);//eccentricity of ellipsoid squared
  double nu = r*pow((1-e2*sin(lat)*sin(lat)),-0.5);//radius of curvature in the prime vertical
  double x = (nu+h)*cos(lat)*cos(lon);
  double y = (nu+h)*cos(lat)*sin(lon);
  double z = (nu*(1-e2)+h)*sin(lat);
  
  xyz[0]=x;
  xyz[1]=y;
  xyz[2]=z;
}

void geoXYZToLtLgAl(double xyz[3], double latLonAlt[3])
{
  double x, y, z;
  x = xyz[0];
  y = xyz[1];
  z = xyz[2];
  double a = 6378137.0;//Earth's equatorial radius
  double f = 1/298.257223563;//flattening factor
  double e2 = f*(2-f);//eccentricity of ellipsoid squared
  
  double r =sqrt(x*x + y*y); 
  double lat = atan2(z,r);
  double lat2 = 500; //unrealistic value to avoid compiling warning for uninitialized variable
  double nu = 0;     //unrealistic value to avoid compiling warning for uninitialized variable
  int k=0;
  double diff = 8.983e-06; //1 meter on earth's surface in degrees

  while(abs(lat-lat2)>diff)
    {
      lat2 = lat;
      nu  = a*pow((1-e2*sin(lat2)*sin(lat2)),-0.5);//radius of curvature in the prime vertical
      lat = atan2((z + nu*e2*sin(lat2)),r);
      k++;
    }

  double lon = atan2(y,x);
  double h = (sqrt(x*x + y*y)/cos(lat)) - nu;
  
    
  latLonAlt[0] = lat*DEGTORAD;
  latLonAlt[1] = lon*DEGTORAD;
  latLonAlt[2] = h;
}


  /*Converters between ANITA cartesian coordinates and WGS84 cartesian coordinates for any point in space, 
    for given latitude, longitude, altitude, heading, pitch, and roll for ANITA*/
void geoXYZToAXYZ(double xyz[3], double aLtLgAl[3], double hpr[3], double axyz[3])
{
  double aNWU[3]={0};
  double apXYZ[3]={0};

  double lat = abs(aLtLgAl[0])*RADTODEG;
  double lon = abs(aLtLgAl[1])*RADTODEG;
  ltLgAlToGeoXYZ(aLtLgAl,apXYZ);

  double x = (xyz[0] - apXYZ[0]);
  double y = (xyz[1] - apXYZ[1]);
  double z = (xyz[2] - apXYZ[2]);

  aNWU[0]=  cos(lon)*x + sin(lon)*y;                                 
  aNWU[1]= -sin(lat)*sin(lon)*x + sin(lat)*cos(lon)*y - cos(lat)*z;
  aNWU[2]=  cos(lat)*sin(lon)*x - cos(lat)*cos(lon)*y - sin(lat)*z;

  //Need to add something that converts from aNWU to aXYZ and fix all the 
  // silly naming things I can't remember
  //  out = aNWUToAXYZ(aNWU, hpr);
  //  return out;
}

void aXYZToGeoXYZ(double axyz[3], double aLtLgAl[3], double hpr[3], double xyz[3])
{



}
