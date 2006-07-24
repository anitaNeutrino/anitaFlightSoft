/*! \file pedestalLib.h
    \brief Pedestal library, full of (hopefully) useful stuff
    
    Some functions that do some useful things
    October 2004  rjn@mps.ohio-state.edu
*/


#ifndef PEDESTALLIB_H
#define PEDESTALLIB_H

#include "anitaStructures.h"

void addEventToPedestals(AnitaEventBody_t *bdPtr);
void resetPedCalc();
void writePedestals();

int subtractCurrentPeds(AnitaEventBody_t *rawBdPtr,
			PedSubbedEventBody_t *pedSubBdPtr);
int addCurrentPeds(PedSubbedEventBody_t *pedSubBdPtr,
		   AnitaEventBody_t *rawBdPtr);
int checkCurrentPedLink();
int loadCurrentPeds();



#endif //PEDESTALLIB_H
