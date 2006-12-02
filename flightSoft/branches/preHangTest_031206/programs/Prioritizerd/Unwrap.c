/* Unwrap the Labrador records */
#include "Unwrap.h"

unsigned char wrapOffset[ACTIVE_SURFS][LABRADORS_PER_SURF][CHANNELS_PER_SURF];

int readWrapOffsets(int offsetOption){
     int surf,lab,chan;
     /* just set all to offsetOption for now; 
	if needed negative options can be used to control
	reading these from a file.*/
     for (surf=0; surf<ACTIVE_SURFS; surf++){
	  for (lab=0;lab<LABRADORS_PER_SURF;lab++){
	       for (chan=0; chan<CHANNELS_PER_SURF;chan++){
		    wrapOffset[surf][lab][chan]=offsetOption;
		}
	   }
      }
     return 0;
}

int unwrapTransient(AnitaEventBody_t *rawSurfEvent,
		    AnitaTransientBody8_t *surfTransientPS, 
		    AnitaTransientBody8_t *surfTransientUW)
{
     int digCh, surf,chan,chip,samp, splice;
     unsigned char chanId,chipIdFlag;
     unsigned char firstHitbus, lastHitbus, wrapped, offset;
     // initialize the output transient to zero for debuggging
     // this can be commented out once we are sure everything is unwrapped OK
     for (digCh=0; digCh<NUM_DIGITZED_CHANNELS; digCh++){
	  for(samp=0; samp<MAX_NUMBER_SAMPLES; samp++){
	       (surfTransientUW->ch[digCh]).data[samp]=0;
	  }
     }
     // end initialization
     for (digCh=0; digCh<NUM_DIGITZED_CHANNELS; digCh++){
	  chanId=((rawSurfEvent->channel[digCh]).header).chanId;
	  chipIdFlag=((rawSurfEvent->channel[digCh]).header).chipIdFlag;
	  surf=chanId/9; chan=chanId%9;
	  chip=(chipIdFlag & 0x3);
	  wrapped=(chipIdFlag & 0x08)>>3;
	  firstHitbus=((rawSurfEvent->channel[digCh]).header).firstHitbus;
	  lastHitbus=((rawSurfEvent->channel[digCh]).header).lastHitbus;
	  /* wrapped refers to whether the hitbus wraps around the end of 
	     record.  If the hitbus wraps the data don't, and vice versa...*/
	  if (wrapped==0){ /* good data is from lastHitbus+1 to end,
			      then from beginning to firstHitbus-1
			   */
	       for (samp=lastHitbus+1;samp<MAX_NUMBER_SAMPLES; samp++){
		    (surfTransientUW->ch[digCh]).data[samp-(lastHitbus+1)]=
			 (surfTransientPS->ch[digCh]).data[samp];
	       }
	       /* this wrote MAX_NUMBER_SAMPLES-(lastHitbus+1) samples */
	       splice=MAX_NUMBER_SAMPLES-(lastHitbus+1);
	       offset=wrapOffset[surf][chip][chan];
	       /* offset is how many samples from the end 
		  are repeated at beginning */
	       for (samp=offset;samp<firstHitbus;samp++){
		    (surfTransientUW->ch[digCh]).data[samp+splice-offset]=
			 (surfTransientPS->ch[digCh]).data[samp];
	       }
	       /* this wrote firstHitbus-offset samples*/
	       (surfTransientUW->ch[digCh]).valid_samples=
		    splice+(firstHitbus-offset);
	  }
	  else if (wrapped==1){ /*good data is from firstHitbus+1
				  to lastHitbus-1
				*/
	       for (samp=firstHitbus+1;samp<lastHitbus; samp++){
		    (surfTransientUW->ch[digCh]).data[samp-(firstHitbus+1)]=
			 (surfTransientPS->ch[digCh]).data[samp];
	       }
	       (surfTransientUW->ch[digCh]).valid_samples=
		    lastHitbus-(firstHitbus+1);
	  }
     }
     return 0;
}
