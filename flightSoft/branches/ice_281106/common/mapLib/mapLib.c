/*! \file mapLib.c
    \brief Map library converts to and from things
    
    Some functions that do some useful things
    June 2006  rjn@mps.ohio-state.edu
*/

#include "mapLib/mapLib.h"
#include "includes/anitaStructures.h"
#include <stdio.h>
#include <stdlib.h>


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

