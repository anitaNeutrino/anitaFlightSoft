#include <anitaFlight.h>
#include "Filters.h"

void NullFilterAll(AnitaTransientBody8_t *in, AnitaTransientBody3_t *out)
{
     int i;
     for (i=0;i<NUM_DIGITZED_CHANNELS;i++){
	  /*if (i%9 != 8){*/ //most filters will skip clocks, but not this one
	  NullFilter(&(in->ch[i]),&(out->ch[i]));
     }
}

void NullFilter(TransientChannel8_t *inch,TransientChannel3_t *outch)
{
     int j;
     for (j=0; j<(inch->valid_samples); j++){
	  outch->data[j]=(Fixed3_t)inch->data[j]>>5;
     }
     outch->baseline=(Fixed3_t)(inch->baseline)>>5;
     outch->valid_samples=inch->valid_samples;
}

// the matched filter will go here--need to make a template.
