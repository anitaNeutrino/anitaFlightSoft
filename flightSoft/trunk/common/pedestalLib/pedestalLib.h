/*! \file pedestalLib.h
    \brief Pedestal library, full of (hopefully) useful stuff
    
    Some functions that do some useful things
    October 2004  rjn@mps.ohio-state.edu
*/


#ifndef PEDESTALLIB_H
#define PEDESTALLIB_H

#ifdef __cplusplus
extern "C" {
#endif
#include "includes/anitaStructures.h"

//Pedestal calculation routines
void addEventToPedestals(AnitaEventBody_t *bdPtr);
void resetPedCalc();
void resetPedCalcWithTime(unsigned int unixTime);
void writePedestals();
void writePedestalsWithTime(unsigned int unixTime);

//Pedestal subtraction routines
int subtractCurrentPeds(AnitaEventBody_t *rawBdPtr,
			PedSubbedEventBody_t *pedSubBdPtr);
int subtractThesePeds(AnitaEventBody_t *rawBdPtr,
		      PedSubbedEventBody_t *pedSubBdPtr,
		      unsigned int whichPeds);
int subtractPedsFromFile(AnitaEventBody_t *rawBdPtr,
			 PedSubbedEventBody_t *pedSubBdPtr,
			 char *filename);

int subtractPedsFromBufferedFile(AnitaEventBody_t *rawBdPtr,
				 PedSubbedEventBody_t *pedSubBdPtr,
				 char *filename,
				 int forceReload);
//Raw routine do not use, use above wrappers instead
int doPedSubtraction(AnitaEventBody_t *rawBdPtr,
		     PedSubbedEventBody_t *pedSubBdPtr);


//Pedestal addition routines
int addPeds(PedSubbedEventBody_t *pedSubBdPtr,
	    AnitaEventBody_t *rawBdPtr);
int addThesePeds(PedSubbedEventBody_t *pedSubBdPtr,
		 AnitaEventBody_t *rawBdPtr,
		 unsigned int whichPeds);
int addPedsFromFile(PedSubbedEventBody_t *pedSubBdPtr,
		    AnitaEventBody_t *rawBdPtr,
		    char *filename);
//Raw routine do not use, use above wrappers instead
int doPedAddition(PedSubbedEventBody_t *pedSubBdPtr,
		  AnitaEventBody_t *rawBdPtr);


//File management etc
int checkCurrentPedLink();
int loadCurrentPeds();
int loadThesePeds(unsigned int whichPeds);
int loadPedsFromFile(char *filename);

void dumpPeds();
void dumpThesePeds(PedestalStruct_t *pedStruct);

//Unwrapping code
    int unwrapAndBaselinePedSubbedEvent(PedSubbedEventBody_t *pedSubBdPtr,
					AnitaTransientBodyF_t *uwTransPtr);


#ifdef __cplusplus
}
#endif
#endif //PEDESTALLIB_H
