/*! \file slowLib.c
    \brief Some useful functions for the calculation of the slow rate packets.
    
    Calculates things like average RF power and a couple fo other bits and pieces. 
   
   September 2006 rjn@mps.ohio-state.edu
    

*/

#include "slowLib/slowLib.h"


typedef struct {
    float rfPwr[ACTIVE_SURFS][RFCHAN_PER_SURF];
    float scalerRates[TRIGGER_SURFS][ANTS_PER_SURF]; 
    float scalerRatesSq[TRIGGER_SURFS][ANTS_PER_SURF]; 
    float upperL2Rates[PHI_SECTORS];
    float lowerL2Rates[PHI_SECTORS];
    float l3Rates[PHI_SECTORS];    
} SlowRateRFCalcStruct_t


FullSurfHkStruct_t theSurfHks[300];
TurfRateStruct_t theTurfRates[300];
int numSurfHks=0;
int numTurfRates=0;
int currentSurfHk=0;
int currentTurfRate;



void addSurfHkToAverage(FullSurfHkStruct_t *hkStruct)
{


}


void addTurfRateToAverage(TurfRateStruct_t *turfRate)
{


}


