#ifndef UNWRAP_H
#define UNWRAP_H

#include "includes/anitaFlight.h"
#include "includes/anitaStructures.h"


#ifdef __cplusplus
extern "C"
{
#endif

int readWrapOffsets(int offsetOption);

int unwrapTransient(AnitaEventBody_t *rawSurfEvent,
		    AnitaTransientBody8_t *SurfTransientPS, 
		    AnitaTransientBody8_t *surfTransientUW);
#ifdef __cplusplus
}
#endif

#endif /* UNWRAP_H */