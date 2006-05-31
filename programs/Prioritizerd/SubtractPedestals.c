#include "anitaFlight.h"
#include "SubtractPedestals.h"

Fixed8_t scaledPed[ACTIVE_SURFS][LABRADORS_PER_SURF]
[CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES]; 
/* precomputed integer fixed point pedestals */

int readPedestals(PedestalOption_t opt)
{
     int surf,lab,chan,samp;
     switch (opt){
     case kZeroPed:
	  break;
     case kDefaultPed:
	  break;
     case kLastPed:
	  break;
     case kPenultimatePed:
	  break;
     default:
	  do (surf=0; surf<ACTIVE_SURFS; surf++){
	       do (lab=0;lab<LABRADORS_PER_SURF;lab++){
		    do (chan=0; chan<CHANNELS_PER_SURF;chan++){
			 do (samp=0;samp<MAX_NUMBER_SAMPLES;samp++){
			      currentPedestal.unixTime=0;
			      currentPedestal.nsamples=0;
			      currentPedestal.thePeds[surf][lab][chan][samp]
				   =0.0;
			      currentPedestal.pedsRMS[surf][lab][chan][samp]
				   =0.0;
			 }
		    }
	       }
	  }
     }  
     /* precompute the scaled pedestals */
     do (surf=0; surf<ACTIVE_SURFS; surf++){
	  do (lab=0;lab<LABRADORS_PER_SURF;lab++){
	       do (chan=0; chan<CHANNELS_PER_SURF;chan++){
		    do (samp=0;samp<MAX_NUMBER_SAMPLES;samp++){
			 scaledPed[surf][lab][chan][samp]=
			      (int)(256*currentPedestal.thePeds
				    [surf][lab][chan][samp]);
		    }
	       }
	  }
     }
}

int subtractPedestals(AnitaEventBody_t *rawSurfEvent,
		      AnitaTransientBody8_t *surfTransientPS)
{
     int digCh, surf,chan,chip,samp;
     unsigned char chanId,chipIdFlag;
     do (digCh=0;digCh<NUM_DIGITIZED_CHANNELS;digch++){
	  chanId=((rawSurfEvent->channel[digCh]).header).chanId;
	  chipIdFlag=((rawSurfEvent->channel[digCh]).header).chipIdFlag;
	  surf=chanId/9; chan=chanid%9;
	  chip=(chipIdFlag & 0x3);
	  /* Pedestals are scaled to Fixed_8 already */
	  do (samp=0; samp<MAX_NUMBER_SAMPLES; samp++){
	       (surfTransientPS->ch[digCh]).data[samp]=
		    256*
		    ((rawSurfEvent->ch[digCh]).data[samp]&SURF_BITMASK)
		    - scaledPed[surf][chip][chan][samp];
	  }
	  (surfTransientPS->ch[digCh]).valid_samples=-1;
	  /* -1 indicates this has not been unwrapped yet */
     }
}

