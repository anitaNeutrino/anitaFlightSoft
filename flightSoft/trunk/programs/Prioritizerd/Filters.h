#ifndef FILTERS_H
#define FILTERS_H
#include <anitaStructures.h>
#ifdef __cplusplus
extern "C"
{
#endif

     void NullFilterAll(AnitaTransientBody8_t *in,AnitaTransientBody3_t *out);
     void NullFilter(TransientChannel8_t *inch,TransientChannel3_t *outch);

#ifdef __cplusplus
}
#endif
#endif /*FILTERS_H*/
