
int BaselineSubtract(AnitaTransientBody8_t *surfTransient){
/* subtract the DC offset, in place */
     int digCh,samp,nsamples;
     Fixed8_t baseline;
     do (digCh=0; digCh<NUM_DIGITZED_CHANNELS; digch++){
	  nsamples=(surfTransient->ch[digCh]).valid_samples;
	  baseline=0;
	  do (samp=0;samp<nsamples; samp++){
	       baseline+=(surfTransient->ch[digCh]).data[samp];
	  }
	  do (samp=0;samp<nsamples; samp++){
	       (surfTransient->ch[digCh]).data[samp] *= nsamples;
	       (surfTransient->ch[digCh]).data[samp] -= baseline;
	       (surfTransient->ch[digCh]).data[samp] /= nsamples;
	  }
	  (surfTransient->ch[digCh]).baseline=baseline/nsamples;
     }
}       
     
