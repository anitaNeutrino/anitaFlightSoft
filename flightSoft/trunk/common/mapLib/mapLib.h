/*! \file mapLib.h
    \brief Map library that contains the conversions from phi,ant,pol to
    surf and channel.
    
    Maps
    May 2008 -- rjn@hep.ucl.ac.uk 
    (conversions taken from Chris Williams williams.2560@ous.edu)
*/


#ifndef MAPLIB_H
#define MAPLIB_H

#ifdef __cplusplus
extern "C" {
#endif
#include "includes/anitaMapping.h"
#include "includes/anitaStructures.h"

  void fillSurfFromPhiAndTierAndPol(SurfAntMapStruct_t *mapPtr);
  void fillSurfFromAntAndPol(SurfAntMapStruct_t *mapPtr);
  void fillPhiFromSurf(SurfAntMapStruct_t *mapPtr);
  int getLogicalIndexFromAnt(int ant,  AntennaPol_t pol);
  int getLogicalIndexFromPhi(int phi,  AntennaTier_t tier, AntennaPol_t pol);


  int getSourcePhiAndDistance(double anitaLatLonAlt[3],
			      double anitaHPR[3],
			      double sourceLatLonAlt[3],
			      int *phiDir,
			      double *distance);

  inline int getLogicalIndexFromSurf(int surf, int chan) 
  {return GetChanIndex(surf,chan);}
  
  
  /*Converters between latitude, longitude, and altitude to WGS84 cartesian coordinates*/
  void ltLgAlToGeoXYZ(double latLonAlt[3], double xyz[3]);
  void geoXYZToLtLgAl(double xyz[3], double latLonAlt[3]);

  //Convertes between ANTA cartesian coordinates and North, west, Up coordinates
  //for given latitude, longitude, altitude, heading, pitch and roll
  void aXYZToANWU(double aXYZ[3], double hpr[3], double aNWU[3]);
  void aNWUToAXYZ(double aNWU[3], double hpr[3], double aXYZ[3]);

#ifdef __cplusplus
}
#endif
#endif //MAPLIB_H
