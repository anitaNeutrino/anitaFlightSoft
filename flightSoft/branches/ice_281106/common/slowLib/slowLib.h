/*! \file slowLib.h
    \brief Some useful functions for the calculation of the slow rate packets.
    
    Calculates things like average RF power and a couple fo other bits and pieces. 
   
   September 2006 rjn@mps.ohio-state.edu
*/


#ifndef SLOWLIB_H
#define SLOWLIB_H

#include "includes/anitaStructures.h"

//RF thingies (to be called by Acqd)
void addSurfHkToAverage(FullSurfHkStruct_t *hkStruct);
void addTurfRateToAverage(TurfRateStruct_t *turfRate);
void writeCurrentRFSlowRateObject(float globalTriggerRate, unsigned long lastEventNumber);

//Hk Thingies filled by SIPd


#endif /* SLOWLIB_H */
