#ifndef UNWRAP_H
#define UNWRAP_H

#include "anitaFlight.h"
#include "anitaStructures.h"

int readWrapOffsets(int offsetOption);

int unwrapTransient(AnitaEventBody_t *rawSurfEvent,
		    AnitaTransientBody8_t *SurfTransientPS, 
		    AnitaTransientBody8_t *surfTransientUW);

#endif /* UNWRAP_H */
