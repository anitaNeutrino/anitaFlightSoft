/*! \file mapLib.h
    \brief Map library that contains the conversions from phi,ant,pol to
    surf and channel.
    
    Maps
    June 2006  rjn@mps.ohio-state.edu
*/


#ifndef MAPLIB_H
#define MAPLIB_H

#include "anitaMapping.h"

void fillSurfFromPhiAndTierAndPol(SurfAntMapStruct_t *mapPtr);
void fillSurfFromAntAndPol(SurfAntMapStruct_t *mapPtr);
void fillPhiFromSurf(SurfAntMapStruct_t *mapPtr);
int getLogicalIndexFromAnt(int ant,  AntennaPol_t pol);
int getLogicalIndexFromPhi(int phi,  AntennaTier_t tier, AntennaPol_t pol);
int getLogicalIndexFromSurf(int surf, int chan);

#endif //MAPLIB_H
