/*! \file mapLib.c
    \brief Map library converts to and from things
    
    Some functions that do some useful things
    June 2006  rjn@mps.ohio-state.edu
*/

#include "mapLib/mapLib.h"
#include "includes/anitaMapping.h"
#include "includes/anitaStructures.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


//Constants
//const double M_PI=3.14159265358979;
const double DEGTORAD=M_PI/180.;   // radians/degree  
const double RADTODEG=180./M_PI;    // degree/rad

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

void convertLtLgAlToGeoXYZ(double latLonAlt[3], double xyz[3])
{
  double theta=(90. - fabs(latLonAlt[0]))*DEGTORAD;
  double phi=(90.-latLonAlt[1]);
  if(phi<0) phi+=360;
  phi*=DEGTORAD;

  double h=latLonAlt[2];
  double r = 6378137.0;//Earth's equatorial radius
  double f = 1/298.257223563;//flattening factor
  double e2 = f*(2-f);//eccentricity of ellipsoid squared
  double nu = r*pow((1-e2*sin(latLonAlt[0]*DEGTORAD)*sin(latLonAlt[0]*DEGTORAD)),-0.5); //radius

  double x = (nu+h)*sin(theta)*cos(phi);
  double y = (nu+h)*sin(theta)*sin(phi);
  double z = (nu*(1-e2)+h)*cos(theta);
  xyz[0]=x;
  xyz[1]=y;
  xyz[2]=z;
}

void ltLgAlToGeoXYZ(double latLonAlt[3], double xyz[3])
{
  double lat, lon, h;
  lat = latLonAlt[0]*DEGTORAD;
  lon = latLonAlt[1]*DEGTORAD;
  h   = latLonAlt[2];

  double r = 6378137.0;//Earth's equatorial radius
  double f = 1/298.257223563;//flattening factor
  double e2 = f*(2-f);//eccentricity of ellipsoid squared
  double nu = r*pow((1-e2*sin(lat)*sin(lat)),-0.5);//radius of curvature in the prime vertical


  double x = (nu+h)*cos(lat)*cos(lon);
  double y = (nu+h)*cos(lat)*sin(lon);
  double z = (nu*(1-e2)+h)*sin(lat);
  //  printf("lat %f lon %f h %f\nx %f y %f z %f\n",lat,lon,h,x,y,z);


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
  
    
  latLonAlt[0] = lat*RADTODEG;
  latLonAlt[1] = lon*RADTODEG;
  latLonAlt[2] = h;
}


  /*Converters between ANITA cartesian coordinates and WGS84 cartesian coordinates for any point in space, 
    for given latitude, longitude, altitude, heading, pitch, and roll for ANITA*/
void geoXYZToAXYZ(double xyz[3], double aLtLgAl[3], double hpr[3], double axyz[3])
{
   double aNWU[3]={0};
   double apXYZ[3]={0};

   double lat = abs(aLtLgAl[0])*DEGTORAD;
   double lon = abs(aLtLgAl[1])*DEGTORAD;

 /*  double theta=(90. - fabs(aLtLgAl[0]))*DEGTORAD; */
/*   double phi=(90.- aLtLgAl[1]); */
/*   if(phi<0) phi+=360; */
/*   phi*=DEGTORAD; */

   ltLgAlToGeoXYZ(aLtLgAl,apXYZ);

   double x = (xyz[0] - apXYZ[0]);
   double y = (xyz[1] - apXYZ[1]);
   double z = (xyz[2] - apXYZ[2]);

   aNWU[0]=  cos(lon)*x + sin(lon)*y;                                 
   aNWU[1]= -sin(lat)*sin(lon)*x + sin(lat)*cos(lon)*y - cos(lat)*z;
   aNWU[2]=  cos(lat)*sin(lon)*x - cos(lat)*cos(lon)*y - sin(lat)*z;

   //  Need to add something that converts from aNWU to aXYZ and fix all the 
   //  silly naming things I can't remember
   aNWUToAXYZ(aNWU,hpr,axyz);

   //  Need to do something with heading at some point

/*   axyz[0]=cos(theta)*(xyz[0]*cos(phi)-xyz[1]*sin(phi)) + xyz[2]*sin(theta); */
/*   axyz[1]=(xyz[0]*sin(phi) + xyz[1]*cos(phi)); */
/*   axyz[2]=-1*sin(theta)*(xyz[0]*cos(phi)-xyz[1]*sin(phi)) + xyz[2]*cos(theta); */

}

void aXYZToGeoXYZ(double axyz[3], double aLtLgAl[3], double hpr[3], double xyz[3])
{
  double aNWU[3]={0};
  double apXYZ[3]={0};
  xyz[0]=0;
  xyz[1]=0;
  xyz[2]=0;

  double lat = abs(aLtLgAl[0])*DEGTORAD;
  double lon = abs(aLtLgAl[1])*DEGTORAD;
  aXYZToANWU(xyz, hpr, aNWU);
  ltLgAlToGeoXYZ(aLtLgAl,apXYZ);
  
  xyz[0]= cos(lon)*aNWU[0] - sin(lon)*sin(lat)*aNWU[1] + sin(lon)*cos(lat)*aNWU[2] + apXYZ[0];
  xyz[1]= sin(lon)*aNWU[0] + cos(lon)*sin(lat)*aNWU[1] - cos(lon)*sin(lat)*aNWU[2] + apXYZ[1];
  xyz[2]= cos(lat)*aNWU[1] - sin(lat)*aNWU[2] + apXYZ[2];


}

void aXYZToANWU(double aXYZ[3], double hpr[3], double aNWU[3])
{
  double a = hpr[0]*DEGTORAD -M_PI/4;
  double b = hpr[1]*DEGTORAD;
  double c = hpr[2]*DEGTORAD;

  /* conversion for aXYZ to aNWU using rotation matrices as follows:
   * 
   *            aNWU = RhRpRr.aXYZ
   *
   *		Rh >= heading rotation
   * 		Rp >= pitch rotation
   *		Rr >= roll rotation
   *
   */


  aNWU[0]=  cos(a)*cos(b)*aXYZ[0] + (sin(a)*cos(c) - cos(a)*sin(b)*sin(c))*aXYZ[1] + (-sin(a)*sin(c)-cos(a)*sin(b)*cos(c))*aXYZ[2];
  aNWU[1]=  -sin(a)*cos(b)*aXYZ[0] + (cos(a)*cos(c) + sin(a)*sin(b)*sin(c))*aXYZ[1] + (-cos(a)*sin(c) + sin(a)*sin(b)*cos(c))*aXYZ[2];
  aNWU[2]=  sin(b)*aXYZ[0] + sin(c)*cos(b)*aXYZ[1] + cos(b)*cos(c)*aXYZ[2];


}

void aNWUToAXYZ(double aNWU[3], double hpr[3], double aXYZ[3])
{
  double a = hpr[0]*DEGTORAD - M_PI/4;
  double b = hpr[1]*DEGTORAD;
  double c = hpr[2]*DEGTORAD;

  /* conversion for aNWU to aXYZ using inverse rotation matrices denoted as inv(Rx)
   *
   *            aXYZ = inv(Rr)inv(Rp)inv(Rh).aNWU
   *
   */

  aXYZ[0]= cos(a)*cos(b)*aNWU[0] - cos(b)*sin(a)*aNWU[1] - sin(b)*aNWU[2];
  aXYZ[1]= (sin(c)*sin(b)*cos(a) + cos(c)*sin(a))*aNWU[0] + (-sin(c)*sin(b)*sin(a) + cos(c)*cos(a))*aNWU[1] + cos(b)*sin(c)*aNWU[2];
  aXYZ[2]= (cos(c)*sin(b)*cos(a) - sin(c)*sin(a))*aNWU[0] + (-cos(c)*sin(b)*sin(a) - sin(c)*cos(a))*aNWU[1] + cos(c)*cos(b)*aNWU[2];


}


int getSourcePhiAndDistance(double anitaLatLonAlt[3],
			    double anitaHPR[3],
			    double sourceLatLonAlt[3],
			    int *phiDir,
			    double *distance)
{
  //
  //  double anitaXYZ[3]={0};
  //  double myAnitaXYZ[3]={0};
  //  double sourceXYZ[3]={0};
  //  double sourceAXYZ[3]={0};
  double thetaPhiDist[3];
  //  ltLgAlToGeoXYZ(anitaLatLonAlt,anitaXYZ);
  //  convertLtLgAlToGeoXYZ(anitaLatLonAlt,myAnitaXYZ);
  //  printf("\t\tanitaXYZ - %f %f %f\n",anitaXYZ[0],anitaXYZ[1],anitaXYZ[2]);
  //  printf("\t\tmyAnitaXYZ - %f %f %f\n",myAnitaXYZ[0],myAnitaXYZ[1],myAnitaXYZ[2]);
  //  ltLgAlToGeoXYZ(sourceLatLonAlt,sourceXYZ);
  //  printf("\t\tsourceXYZ - %f %f %f\n",sourceXYZ[0],sourceXYZ[1],sourceXYZ[2]);
  //  geoXYZToAXYZ(sourceXYZ,anitaLatLonAlt,anitaHPR,sourceAXYZ);
  //  printf("\t\tsourceAXYZ - %f %f %f\n",sourceAXYZ[0],sourceAXYZ[1],sourceAXYZ[2]);
  getThetaAndPhiPayload(anitaLatLonAlt,anitaHPR,sourceLatLonAlt,thetaPhiDist);



  //  *distance=sqrt(sourceAXYZ[0]*sourceAXYZ[0]+
  //		sourceAXYZ[1]*sourceAXYZ[1]+
  //		sourceAXYZ[2]*sourceAXYZ[2]);

  //Need to fix below to be robust and not seg v
  //  double azimuth=RADTODEG*atan2(sourceAXYZ[1],sourceAXYZ[0]);


  //  printf("Phi %f -- Dist %f\n",thetaPhiDist[1],thetaPhiDist[2]);
  //  printf("distance %f -- azimuth %f\n",*distance,azimuth);
  double azimuth=thetaPhiDist[1];
  *distance=thetaPhiDist[2];
   //Hack for now 

  //Here azimuth is relative to the ADU5 aft-fore direction
  if(azimuth<0)
    azimuth+=360;

  if(azimuth>11.25 && azimuth<=33.75)*phiDir=2;
  else if(azimuth>33.75 && azimuth<=56.25)*phiDir=3;
  else if(azimuth>56.25 && azimuth<=78.75)*phiDir=4;
  else if(azimuth>78.75 && azimuth<=101.25)*phiDir=5;
  else if(azimuth>101.25 && azimuth<=123.75)*phiDir=6;
  else if(azimuth>123.75 && azimuth<=146.25)*phiDir=7;
  else if(azimuth>146.25 && azimuth<=168.75)*phiDir=8;
  else if(azimuth>168.75 && azimuth<=191.25)*phiDir=9;
  else if(azimuth>191.25 && azimuth<=213.75)*phiDir=10;
  else if(azimuth>213.75 && azimuth<=236.25)*phiDir=11;
  else if(azimuth>236.25 && azimuth<=258.75)*phiDir=12;
  else if(azimuth>258.75 && azimuth<=281.25)*phiDir=13;
  else if(azimuth>281.25 && azimuth<=303.75)*phiDir=14;
  else if(azimuth>303.75 && azimuth<=326.25)*phiDir=15;
  else if(azimuth>326.25 && azimuth<=348.75)*phiDir=0;
  else if(azimuth>348.75 && azimuth<=360)*phiDir=1;
  else if(azimuth>=0 && azimuth<=11.25)*phiDir=1;
  else printf("\n\nERROR OBJECT ANITA AZIMUTH OUT OF RANGE\n\n");

  //  *phiDir=-1;
  //  *distance=-1;
  return -1;
}

void rotateX(double input[3], double output[3], double angle) {
   //rotate vector around X
   double s = sin(angle);
   double c = cos(angle);
   double yy = input[0];
   output[0]=input[0];
   output[1] = c*yy - s*input[2];
   output[2] = s*yy + c*input[2];
}

//______________________________________________________________________________
void rotateY(double input[3], double output[3],double angle) {
   //rotate vector around Y
   double s = sin(angle);
   double c = cos(angle);
   double zz = input[2];
   output[2] = c*zz - s*input[0];
   output[1] = input[1];
   output[0] = s*zz + c*input[0];
}

//______________________________________________________________________________
void rotateZ(double input[3], double output[3],double angle) {
   //rotate vector around Z
   double s = sin(angle);
   double c = cos(angle);
   double xx = input[0];
   output[0] = c*xx - s*input[1];
   output[1] = s*xx + c*input[1];
   output[2] = input[2];
}

double getThetaFromLat(double lat)
{
  return (90.- lat)*DEGTORAD; 
}
 

double getPhiFromLon(double lon)
{
  //Need to fix this somehow
  double phi_deg = 90. - lon;
  if(phi_deg<0) phi_deg+=360;
  return phi_deg*DEGTORAD;
}

void getThetaAndPhiPayload(double balloonLatLonAlt[3], double balloonHpr[3],
			   double sourceLatLonAlt[3], double thetaPhiDist[3])
{

  //  double r = 6378137.0;//Earth's equatorial radius
  //  double f = 1/298.257223563;//flattening factor
  //  double e2 = f*(2-f);//eccentricity of ellipsoid squared
  //  printf("r %f -- f %f -- e2 %f\n",r,f,e2);
  //  double nu = r/sqrt((1-e2*sin(fabs(balloonLatLonAlt[0]))*sin(fabs(balloonLatLonAlt[0]))));//radius of curvature in the prime vertical
  //  printf("r %f nu %f\n",r,nu);

  
  //  printf("thetaBalloon %f -- phiBalloon %f\n",thetaBalloon,phiBalloon);

  //Get vector from Earth's centre to source
  double sourceXYZ[3]={0};
  double balloonXYZ[3]={0};
  double tempXYZ[3]={0};
  double rotatedXYZ[3]={0};
  double deltaXYZ[3]={0};
  convertLtLgAlToGeoXYZ(sourceLatLonAlt,sourceXYZ);
  convertLtLgAlToGeoXYZ(balloonLatLonAlt,balloonXYZ);

  //  double thetaBalloonOld=getThetaFromLat(fabs(balloonLatLonAlt[0]));
  //  double phiBalloonOld=getPhiFromLon(balloonLatLonAlt[1]);

  double phiBalloon=atan2(balloonXYZ[1],balloonXYZ[0]);
  double thetaBalloon=atan2(sqrt(balloonXYZ[0]*balloonXYZ[0] + 
				 balloonXYZ[1]*balloonXYZ[1]), balloonXYZ[2]);
  //  printf("%f %f -- old %f %f\n",phiBalloon,thetaBalloon,phiBalloonOld,thetaBalloonOld);

  //Now some rotations
  rotateZ(sourceXYZ,tempXYZ,-1*phiBalloon);
  rotateY(tempXYZ,rotatedXYZ,-1*thetaBalloon);
  
  deltaXYZ[0]=rotatedXYZ[0]-balloonXYZ[0];
  deltaXYZ[1]=rotatedXYZ[1]-balloonXYZ[1];
  deltaXYZ[2]=rotatedXYZ[2]-balloonXYZ[2];
  //  printf("deltaXYZ %f %f %f\n",deltaXYZ[0],deltaXYZ[1],deltaXYZ[2]);
  double distToBalloon=sqrt(deltaXYZ[0]*deltaXYZ[0] +
			    deltaXYZ[1]*deltaXYZ[1] +
			    deltaXYZ[2]*deltaXYZ[2]);
  double horizDist=sqrt(rotatedXYZ[0]*rotatedXYZ[0] +
			rotatedXYZ[1]*rotatedXYZ[1]);
  double thetaWave=atan((balloonXYZ[2] - rotatedXYZ[2])/horizDist);
  double phiWave=atan2(rotatedXYZ[1],rotatedXYZ[0]);

  //  printf("thetaWave %f -- phiWave %f\n",thetaWave*RADTODEG,phiWave*RADTODEG);

  double heading=balloonHpr[0];
  if(heading>=0 && heading<=360) {
    phiWave+=heading*DEGTORAD;
    if(phiWave>2*M_PI)
      phiWave-=2*M_PI;
    if(phiWave<0)
      phiWave+=2*M_PI;
  }
  
  thetaPhiDist[0]=thetaWave*RADTODEG;
  thetaPhiDist[1]=phiWave*RADTODEG;
  thetaPhiDist[2]=distToBalloon;
}

int getPhiSeparation(int phiA, int phiB)
{
  int theDiff=abs(phiA-phiB);
  if(phiA>phiB && ((16+phiB)-phiA)<theDiff)
    theDiff=(16+phiB)-phiA;
  if(phiB>phiA && ((16+phiA)-phiB)<theDiff)
    theDiff=(16+phiA)-phiB;
  return theDiff;
}
