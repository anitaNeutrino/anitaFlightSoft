#ifndef FILTERS_H
#define FILTERS_H
#include <anitaStructures.h>
#include "AnitaInstrument.h"
#ifdef __cplusplus
extern "C"
{
#endif

     void NullFilterAll(AnitaTransientBody8_t *in,AnitaTransientBody3_t *out);
     void NullFilter(TransientChannel8_t *inch,TransientChannel3_t *outch);


     void HornMatchedFilter(TransientChannelF_t *inch,
			    TransientChannelF_t *outch);
     void HornMatchedFilterAll(AnitaInstrumentF_t *in, 
			       AnitaInstrumentF_t *out);

     void HornMatchedFilterSmooth(TransientChannelF_t *inch,
			    TransientChannelF_t *outch);
     void HornMatchedFilterAllSmooth(AnitaInstrumentF_t *in, 
			       AnitaInstrumentF_t *out);

     void HornMatchedFilterAllBlind(AnitaInstrumentF_t *in, 
				    AnitaInstrumentF_t *out,
				    AnitaInstrumentF_t *pow_out);

//     void BiconeMatchedFilter(TransientChannelF_t *inch,
//			    TransientChannelF_t *outch);
//     void DisconeMatchedFilter(TransientChannelF_t *inch,
//			    TransientChannelF_t *outch);

#ifdef __cplusplus
}
#endif
#endif /*FILTERS_H*/
