/* SubtractPedestals.h JJB 2006-04-19 */
#ifndef SUBTRACTPEDESTALS_H
#define SUBTRACTPEDESTALS_H

#include "anitaFlight.h"
#include "anitaStructures.h"

typedef enum {
    kZeroPed,
    kDefaultPed,
    kLastPed,
    kPenultimatePed
} PedestalOption_t;
#ifdef __cplusplus
extern "C"
{
#endif

int readPedestals(PedestalOption_t opt);
/* pedOption 0=all pedestals are zero
   pedOption 1=standard pedestal set (written preflight and permanent)
   pedOption 2=most recent pedestals taken
   pedOption 3=next most recent pedestals 
*/

int subtractPedestals(AnitaEventBody_t *rawSurfEvent,AnitaTransientBody8_t *surfTransientPS);
/* 
   The RawSurfChannelHeader with each SurfChannelFull 
   has the chip information */
#ifdef __cplusplus
}
#endif


#endif /* SUBTRACTPEDESTALS_H */
