#ifndef MAKESTOKES_H
#define MAKESTOKES_H

#include <anitaStructures.h>
#include "Hilbert.h"

#ifdef __cplusplus
extern "C"
{
#endif

void makeStokes(TransientChannel3_t *Hor, TransientChannel3_t *Ver, 
	       AnitaStokes6_t *Stokes);

#ifdef __cplusplus
}
#endif

#endif /* MAKESTOKES_H */
