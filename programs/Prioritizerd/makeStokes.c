int makeStokes(TransientChannel3_t *Hor, TransientChannel3_t *Ver, 
	       AnitaStokes6_t *Stokes)
{
     int samp,order;
     short valid_samples;
     /* fill the Stokes transient structure
	definititions are per Hecht

     I_0= 1/2 (H^2 +V^2) ;  S_0=2*I_0
     I_1= H^2            ;  S_1=2*I_1-2*I_0=H^2-V^2
     I_2= 1/2(H+V)^2     ;  S_2=2*I_2-2*I_0=2HV
     I_3= RCP power      ;  S_3=2*I_3-2*I_0=2hilb(V)*H     

     We scale the Stokes parameters down by a factor of two to keep them
     on a better integer scale; multiplying by two just appends a zero,
     wasting a bit. So our definitions are:

     S_0=1/2(H^2+V^2); S_1=1/2(H^2-V^2); S_2=HV; S_3=hilb(V)*H

     For S_3 we deal with the nonlocality and phase shift in the
     numerical Hilbert transform by applying the symmetric transform
     conjugate to it to the H data.

     The 1/2 gets distributed inside the parentheses in the first two cases.

     */
     if (Hor->valid_samples<Ver->valid_samples){
	  valid_samples=Hor->valid_samples;
     }
     else{
	  valid_samples=Ver->valid_samples;
     }
     do (samp=0; samp<valid_samples; samp++){
	  (Stokes->S0).data[samp]=((Hor->data[samp])*(Hor->data[samp])/2)
	       +((Ver->data[samp])*(Ver->data[samp])/2);
	  (Stokes->S1).data[samp]=((Hor->data[samp])*(Hor->data[samp])/2)
	       -((Ver->data[samp])*(Ver->data[samp])/2);
	  (Stokes->S2).data[samp]=(Hor->data[samp])*(Ver->data[samp]);
	  /* S3 gets offset by 1/2 sample from the rest.  
	     This is not important for the type of coincidence 
	     we'll be doing here. */
	  if (samp==0){
	       (Stokes->S3).data[samp]=((Hor->data[0]+Hor->data[1])/2)
		    *((Ver->data[0]+Hor->data[1])/2);
	       /* this is just a fillin to keep the number 
		  of samples the same */
	  }
	  else if(samp<HILBERT_ORDER+1){
	       order=samp-1;
	       (Stokes->S3)data[samp]=intHilbertFilter(order,(Ver->data)+samp)
		    *intHilbertSymFilter(order,(Hor->data)+samp);
	       /* keep the filter order within the array; 
		  a zero order transform goes back one sample
		  and forward zero, etc.*/
	  }
	  else if ((valid_samples-samp)<HILBERT_ORDER+1){
	       order=(valid_samples-samp)-1;
	       (Stokes->S3)data[samp]=intHilbertFilter(order,(Ver->data)+samp)
		    *intHilbertSymFilter(order,(Hor->data)+samp);
	  }
	  else{
	       order=HILBERT_ORDER;
	       (Stokes->S3)data[samp]=intHilbertFilter(order,(Ver->data)+samp)
		    *intHilbertSymFilter(order,(Hor->data)+samp);
	  }

     }
     (Stokes->S0).valid_samples=valid_samples;
     (Stokes->S1).valid_samples=valid_samples;
     (Stokes->S2).valid_samples=valid_samples;
     (Stokes->S3).valid_samples=valid_samples;
}
