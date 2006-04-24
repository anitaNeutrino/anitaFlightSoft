int makeStokes(TransientChannel3_t *Hor, TransientChannel3_t *Ver, 
	       AnitaStokes6_t *Stokes)
{
     int samp;
     short valid_samples;
     /* fill the Stokes transient structure
	definititions are per Hecht

     I_0= 1/2 (H^2 +V^2) ;  S_0=2*I_0
     I_1= H^2            ;  S_1=2*I_1-2*I_0=H^2-V^2
     I_2=1/2(H+V)^2      ;  S_2=2*I_2-2*I_0=2HV

     We scale the Stokes parameters down by a factor of two to keep them
     on a better integer scale; multiplying by two just appends a zero,
     wasting a bit. So our definitions are:

     S_0=1/2(H^2+V^2); S_1=1/2(H^2-V^2); S_2=HV

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
	  (Stokes->S1)data[samp]=((Hor->data[samp])*(Hor->data[samp])/2)
	       -((Ver->data[samp])*(Ver->data[samp])/2);
	  (Stokes->S2)data[samp]=(Hor->data[samp])*(Ver->data[samp]);
     }
     (Stokes->S0).valid_samples=valid_samples;
     (Stokes->S1).valid_samples=valid_samples;
     (Stokes->S2).valid_samples=valid_samples;
}
