/*! \file anitaMapping.h
    \brief The ANITA mapping include file.
    
    This is where the SURF and Channel is mapped to antenna, polarization, tier and phi.
    June 2006  rjn@mps.ohio-state.edu
*/

#ifndef ANITA_MAPPING_H
#define ANITA_MAPPING_H
#include "includes/anitaFlight.h"

#define NUM_PHI PHI_SECTORS

//Positive is horizontal
//Negative is vetical
int antToSurfMap[32]={1,3,5,7,1,3,5,7,0,2,4,6,0,2,4,6,
		      0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7};
int hAntToChan[32]={4,4,4,4,5,5,5,5,4,4,4,4,5,5,5,5,
		    6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7};
int vAntToChan[32]={0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
		    2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3};

// Note that this array uses antenna number 1-32 as it needs
// the negative sign to indicate polarization
int surfToAntMap[9][8]={{-9,-13,-17,-25,9,13,17,25},
			{-1,-5,-18,-26,1,5,18,26},
			{-10,-14,-19,-27,10,14,19,27},
			{-2,-6,-20,-28,2,6,20,28},
			{-11,-15,-21,-29,11,15,21,29},
			{-3,-7,-22,-30,3,7,22,30},
			{-12,-16,-23,-31,12,16,23,31},
			{-4,-8,-24,-32,4,8,24,32},
			{37,38,39,40,33,34,35,36}};

int biconeIndexArray[4]={72,73,74,75}; //phi=2,6,10,14
int disconeIndexArray[4]={77,78,79,76}; //phi=4,8,12,16

//Map from phi to antenna both start counting at zero
//int upperAntNums[NUM_PHI]={9,1,10,2,11,3,12,4,13,5,14,6,15,7,16,8};
//int lowerAntNums[NUM_PHI]={17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
int upperAntNums[NUM_PHI]={8,0,9,1,10,2,11,3,12,4,13,5,14,6,15,7};
int lowerAntNums[NUM_PHI]={16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};

//and the inverse (using ANT-1 and ANT-17 with the arrays)
int upperPhiNums[NUM_PHI]={1,3,5,7,9,11,13,15,0,2,4,6,8,10,12,14};
int lowerPhiNums[NUM_PHI]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

//Discone and bicones
int coneAntNums[NUM_PHI]={-1,4,-1,1,-1,5,-1,2,-1,6,-1,3,-1,7,-1,0};
int conePhiNums[8]={15,3,7,11,1,5,9,13};

typedef enum {    
    kUpperTier=1,
    kMiddleTier=2,
    kLowerTier=4,
    kDisconeTier=8,
    kBiconeTier=16,
    kUnknownTier=-1
} AntennaTier_t;
 
typedef enum {
    kHorizontal=0,
    kVertical=1,
    kUnknownPol=-1
} AntennaPol_t;

typedef struct {
    int surf;
    int channel;
    int index;
    int phi;
    AntennaTier_t tier;
    int antenna;
    AntennaPol_t pol;    
} SurfAntMapStruct_t;


#endif //ANITA_MAPPING_H
