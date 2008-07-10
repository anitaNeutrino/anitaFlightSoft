/*  A cross-correlation of two waveforms using the FFT
    method. See Numerical recipes.

  Tuned for ANITA waveform data. 


--Peter Gorham August 2006 

Modified by Ped to use fftw;
adapted to flight software environment JJB
8/14/06
power spectrum peak cutter added JJB
Sun Nov 26 05:11:19 2006
analytic signal return version added JJB
July 7, 2008

*/


#include <stdio.h>
//#include <values.h>
#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <complex.h>
#include "fftw3.h"  //local copy used to simplify builds


#define NPTS 256              /* size of the waveform and template arrays */
                              /*        must be power of 2 */
#define WFRAC  0.33333        /* lead fraction of waveform */
                              /*        to use for noise statistics */
	
			      


extern int MethodMask,FFTPeakMaxA,FFTPeakMaxB,FFTPeakWindowL,FFTPeakWindowR;

extern int FFTNumChannels;


int fftwcorr_anal_1x(float wfm[], float tmpl[], float cout[])
  /*  wfm[] = pointer to float waveform data, 
      tmpl[] = pointer to template for correlation 
      cout[] =  pointer to return correlation,twice as long
     this version returns the analytic signal
     normalized to unit rms power
     everything here is now done with full complex transforms
     ffts are NOT double length padded with zeros to eliminate wraparound
    ---------------
     This routine returns 0 for success, and >0 for errors:
      return(1) :  bad standard deviation (wfm replaced with raw corr)
      return(2) :  mean significantly non-zero (wfm replaced with corr/sdev) */
{ 
  static int first=1;
  static fftw_complex *tmpl_fft;
  static fftw_complex *wfm_in;
  static fftw_complex *wfm_fft;
  static fftw_complex *anal;
  static fftw_plan fft_plan;
  static fftw_plan ifft_plan;
  double rms, rmssum;
  double power_spectrum[NPTS],power_avg,power_peakA,power_peakB; 
  int i,peakok;
  static int mpts = WFRAC*NPTS;  /* use lead fraction for noise stats */

  if(first){
    // Allocate local arrays for math operations
    wfm_in=(fftw_complex*)fftw_malloc(NPTS*sizeof(fftw_complex));
    anal=(fftw_complex*)fftw_malloc(NPTS*sizeof(fftw_complex));
    wfm_fft=(fftw_complex *)fftw_malloc(NPTS*sizeof(fftw_complex));
    tmpl_fft=(fftw_complex *)fftw_malloc(NPTS*sizeof(fftw_complex));
    // Create FFT plans
    fft_plan=fftw_plan_dft_1d(NPTS,wfm_in,wfm_fft,FFTW_FORWARD,FFTW_MEASURE);
    ifft_plan=fftw_plan_dft_1d(NPTS,wfm_fft,anal,FFTW_BACKWARD,FFTW_MEASURE);
    // Calculate FFT of template which we will need to do only once
    for(i=0;i<NPTS;++i) wfm_in[i]=tmpl[i];
//    for (i=NPTS;i<2*NPTS; i++) wfm_in[i]=0.;
    fftw_execute(fft_plan);
    for(i=0;i<NPTS;++i) tmpl_fft[i]=conj(wfm_fft[i]);
    //for(i=0;i<NPTS;++i) printf("%d: %g %+g i\n",i,__real__ tmpl_fft[i],__imag__ tmpl_fft[i]);
    // Show that initialization has been done
    first=0;
  }
  // Calculate FFT of sample wavefrom
  for(i=0;i<NPTS;++i) wfm_in[i]=wfm[i];
  for (i=NPTS;i<NPTS; i++) wfm_in[i]=0.;
  fftw_execute(fft_plan);
  peakok=0;
  //look at fft for peaks exceeding 
  if ((MethodMask&0x0002)!=0){
       power_avg=0.;
       power_peakA=0.;
       power_peakB=0.;
       for (i=0; i<NPTS/2; i++){
	    power_spectrum[i]=wfm_fft[i]*conj(wfm_fft[i]);
	    power_avg+=power_spectrum[i];
	    if (power_spectrum[i]>power_peakA) power_peakA=power_spectrum[i];
	    if (((i>FFTPeakWindowL)&&(i<FFTPeakWindowR)) &&
		 (power_spectrum[i]>power_peakB)) power_peakB=power_spectrum[i];
       }
       power_avg=power_avg/NPTS;
       if (power_peakA>power_avg*(float)FFTPeakMaxA/100.) peakok=-1;
       else if (power_peakB>power_avg*(float)FFTPeakMaxB/100.) peakok=-2;
  }
//  for(i=0;i<2*NPTS;i++) fprintf(stderr,"%e %e %e %e\n",creal(wfm_fft[i]),cimag(wfm_fft[i]),creal(tmpl_fft[i]),cimag(tmpl_fft[i]));
//  fprintf(stderr,"%i\n",peakok);
//  peakok=0;
  if (peakok>=0){
       // Now calculate cross-corelation in place by multiplying FFTs
       for(i=0;i<=NPTS;++i) wfm_fft[i]=wfm_fft[i]*tmpl_fft[i];
       //now have FFT of croscorrelated waveform.  Transform to
       //get FFT of analytic signal
       wfm_fft[0]=0.;//enforce AC coupling
       for(i=1;i<NPTS/2;i++) wfm_fft[i]*=2;
       for(i=NPTS/2;i<=NPTS;i++) wfm_fft[i]=0.;
       // Execute inverse FFT
       fftw_execute(ifft_plan);
       for (i=0;i<NPTS;i++) {
	    anal[i]=cabs(anal[i])/(2*NPTS);
       }
       /* get statistics on first part of waveform */
       /* if there is non-thermal noise or signal there, 
	  this won't work very well */
       rms = rmssum = 0.0;
       for(i=1;i<=mpts;i++){
	    rmssum += anal[i]*conj(anal[i]); //is real from above, just being careful...
	    
       }
       rms = sqrt(rmssum/((double)(mpts)));
       
       /* this section to handle goofy data gracefully */
       if(rms <= 0.0) {  // how could it be?? 
	    for(i=0;i<NPTS;++i){
		 cout[i] = creal(anal[i]); 
	    }  /* don't normalize */
	    return(1); /* return here since sdev is not usable */
       }
  
       /* end of stdev section */
  
       /* replace the waveform with normalized correlation */
       for(i=0;i<NPTS;++i){
	    cout[i] = creal(anal[i])/rms; 
       }
  
       return(0);  /* in this case all is well */
  }
  else
  {
       //if we fail, cut this channel
       //an ugly hack--zero out the waveform
       for (i=0;i<NPTS; i++){
	    cout[i]=0.;
       }
       //another ugly hack--increment globabl varaible FFTNumChannels
       //this must be zeroed tested in main program
       FFTNumChannels++; 
       return(0);
  }
}


int fftwcorr_anal_2x(float wfm[], float tmpl[], float cout[])
  /*  wfm[] = pointer to float waveform data, 
      tmpl[] = pointer to template for correlation 
      cout[] =  pointer to return correlation,twice as long
     this version returns the analytic signal
     normalized to unit rms power
     everything here is now done with full complex transforms
     ffts are double length padded with zeros to eliminate wraparound
    ---------------
     This routine returns 0 for success, and >0 for errors:
      return(1) :  bad standard deviation (wfm replaced with raw corr)
      return(2) :  mean significantly non-zero (wfm replaced with corr/sdev) */
{ 
  static int first=1;
  static fftw_complex *tmpl_fft;
  static fftw_complex *wfm_in;
  static fftw_complex *wfm_fft;
  static fftw_complex *anal;
  static fftw_plan fft_plan;
  static fftw_plan ifft_plan;
  double rms, rmssum;
  double power_spectrum[NPTS],power_avg,power_peakA,power_peakB; 
  int i,peakok;
  static int mpts = WFRAC*NPTS;  /* use lead fraction for noise stats */

  if(first){
    // Allocate local arrays for math operations
    wfm_in=(fftw_complex*)fftw_malloc(2*NPTS*sizeof(fftw_complex));
    anal=(fftw_complex*)fftw_malloc(2*NPTS*sizeof(fftw_complex));
    wfm_fft=(fftw_complex *)fftw_malloc(2*NPTS*sizeof(fftw_complex));
    tmpl_fft=(fftw_complex *)fftw_malloc(2*NPTS*sizeof(fftw_complex));
    // Create FFT plans
    fft_plan=fftw_plan_dft_1d(2*NPTS,wfm_in,wfm_fft,FFTW_FORWARD,FFTW_MEASURE);
    ifft_plan=fftw_plan_dft_1d(2*NPTS,wfm_fft,anal,FFTW_BACKWARD,FFTW_MEASURE);
    // Calculate FFT of template which we will need to do only once
    for(i=0;i<NPTS;++i) wfm_in[i]=tmpl[i];
    for (i=NPTS;i<2*NPTS; i++) wfm_in[i]=0.;
    fftw_execute(fft_plan);
    for(i=0;i<2*NPTS;++i) tmpl_fft[i]=conj(wfm_fft[i]);
    //for(i=0;i<NPTS;++i) printf("%d: %g %+g i\n",i,__real__ tmpl_fft[i],__imag__ tmpl_fft[i]);
    // Show that initialization has been done
    first=0;
  }
  // Calculate FFT of sample wavefrom
  for(i=0;i<NPTS;++i) wfm_in[i]=wfm[i];
  for (i=NPTS;i<2*NPTS; i++) wfm_in[i]=0.;
  fftw_execute(fft_plan);
  peakok=0;
  //look at fft for peaks exceeding 
  if ((MethodMask&0x0002)!=0){
       power_avg=0.;
       power_peakA=0.;
       power_peakB=0.;
       for (i=0; i<NPTS; i++){
	    power_spectrum[i]=wfm_fft[i]*conj(wfm_fft[i]);
	    power_avg+=power_spectrum[i];
	    if (power_spectrum[i]>power_peakA) power_peakA=power_spectrum[i];
	    if (((i>FFTPeakWindowL)&&(i<FFTPeakWindowR)) &&
		 (power_spectrum[i]>power_peakB)) power_peakB=power_spectrum[i];
       }
       power_avg=power_avg/NPTS;
       if (power_peakA>power_avg*(float)FFTPeakMaxA/100.) peakok=-1;
       else if (power_peakB>power_avg*(float)FFTPeakMaxB/100.) peakok=-2;
  }
//  for(i=0;i<2*NPTS;i++) fprintf(stderr,"%e %e %e %e\n",creal(wfm_fft[i]),cimag(wfm_fft[i]),creal(tmpl_fft[i]),cimag(tmpl_fft[i]));
//  fprintf(stderr,"%i\n",peakok);
//  peakok=0;
  if (peakok>=0){
       // Now calculate cross-corelation in place by multiplying FFTs
       for(i=0;i<=2*NPTS;++i) wfm_fft[i]=wfm_fft[i]*tmpl_fft[i];
       //now have FFT of croscorrelated waveform.  Transform to
       //get FFT of analytic signal
       wfm_fft[0]=0.;//enforce AC coupling
       for(i=1;i<NPTS;i++) wfm_fft[i]*=2;
       for(i=NPTS;i<=2*NPTS;i++) wfm_fft[i]=0.;
       // Execute inverse FFT
       fftw_execute(ifft_plan);
       for (i=0;i<2*NPTS;i++) {
	    anal[i]=cabs(anal[i])/(2*NPTS);
       }
       /* get statistics on first part of waveform */
       /* if there is non-thermal noise or signal there, 
	  this won't work very well */
       rms = rmssum = 0.0;
       for(i=1;i<=mpts;i++){
	    rmssum += anal[i]*conj(anal[i]); //is real from above, just being careful...
	    
       }
       rms = sqrt(rmssum/((double)(mpts)));
       
       /* this section to handle goofy data gracefully */
       if(rms <= 0.0) {  // how could it be?? 
	    for(i=0;i<NPTS;++i){
		 cout[i] = creal(anal[i]); 
	    }  /* don't normalize */
	    return(1); /* return here since sdev is not usable */
       }
  
       /* end of stdev section */
  
       /* replace the waveform with normalized correlation */
       for(i=0;i<NPTS;++i){
	    cout[i] = creal(anal[i])/rms; 
       }
  
       return(0);  /* in this case all is well */
  }
  else
  {
       //if we fail, cut this channel
       //an ugly hack--zero out the waveform
       for (i=0;i<NPTS; i++){
	    cout[i]=0.;
       }
       //another ugly hack--increment globabl varaible FFTNumChannels
       //this must be zeroed tested in main program
       FFTNumChannels++; 
       return(0);
  }
}



int fftwcorr(float wfm[], float tmpl[], float cout[])
  /*  wfm[] = pointer to float waveform data, 
      tmpl[] = pointer to template for correlation 
      cout[] =  pointer to return correlation,twice as long
     normalized to unit standard deviation
    ---------------
     This routine returns 0 for success, and >0 for errors:
      return(1) :  bad standard deviation (wfm replaced with raw corr)
      return(2) :  mean significantly non-zero (wfm replaced with corr/sdev) */
{ 
  static int first=1;
  static fftw_complex *tmpl_fft;
  static double *wfm_in;
  static fftw_complex *wfm_fft;
  static double *corr;
  static fftw_plan fft_plan;
  static fftw_plan ifft_plan;
  double stdev, mean, meansum, varsum;
  double power_spectrum[NPTS],power_avg,power_peakA,power_peakB; 
  int i,peakok;
  static int mpts = WFRAC*NPTS;  /* use lead fraction for noise stats */

  if(first){
    // Allocate local arrays for math operations
    wfm_in=(double*)calloc(NPTS, sizeof(double));
    //wfm_in=(fftw_complex *)fftw_malloc(NPTS*sizeof(fftw_complex));
    corr=(double*)calloc(2*NPTS, sizeof(double));
    wfm_fft=(fftw_complex *)fftw_malloc(NPTS*sizeof(fftw_complex));
    tmpl_fft=(fftw_complex *)fftw_malloc(NPTS*sizeof(fftw_complex));
    // Create FFT plans
    fft_plan=fftw_plan_dft_r2c_1d(NPTS,wfm_in,wfm_fft,FFTW_MEASURE);
    ifft_plan=fftw_plan_dft_c2r_1d(NPTS,wfm_fft,corr,FFTW_MEASURE);
    // Calculate FFT of template which we will need to do only once
    for(i=0;i<NPTS;++i) wfm_in[i]=tmpl[i];
    fftw_execute(fft_plan);
    for(i=0;i<NPTS;++i) tmpl_fft[i]=conj(wfm_fft[i]);
    //for(i=0;i<NPTS;++i) printf("%d: %g %+g i\n",i,__real__ tmpl_fft[i],__imag__ tmpl_fft[i]);
    // Show that initialization has been done
    first=0;
  }
  // Calculate FFT of sample wavefrom
  for(i=0;i<NPTS;++i) wfm_in[i]=wfm[i];
  fftw_execute(fft_plan);
  peakok=0;
  //look at fft for peaks exceeding 
  if ((MethodMask&0x0002)!=0){
       power_avg=0.;
       power_peakA=0.;
       power_peakB=0.;
       for (i=0; i<NPTS; i++){
	    power_spectrum[i]=wfm_fft[i]*conj(wfm_fft[i]);
	    power_avg+=power_spectrum[i];
	    if (power_spectrum[i]>power_peakA) power_peakA=power_spectrum[i];
	    if (((i>FFTPeakWindowL)&&(i<FFTPeakWindowR)) &&
		 (power_spectrum[i]>power_peakB)) power_peakB=power_spectrum[i];
       }
       power_avg=power_avg/NPTS;
       if (power_peakA>power_avg*(float)FFTPeakMaxA/100.) peakok=-1;
       else if (power_peakB>power_avg*(float)FFTPeakMaxB/100.) peakok=-2;
  }

  if (peakok>=0){
       // Now calculate cross-corelation in place by multyplying FFTs
       for(i=0;i<=2*NPTS;++i) wfm_fft[i]=wfm_fft[i]*tmpl_fft[i];
       
       // Execute inverse FFT
       fftw_execute(ifft_plan);
       
       /* get statistics on first par of waveform */
       /* if there is non-thermal noise or signal there, 
	  this won't work very well */
       mean = meansum =  varsum = stdev = 0.0;
       for(i=1;i<=mpts;i++){
	    meansum += corr[i];
	    varsum += fabs(corr[i]*corr[i]);
       }
       
       mean = meansum/((double)(mpts));
       stdev = sqrt(varsum/((double)(mpts)) - (mean*mean) );
       
       /* this section to handle goofy data gracefully */
       if(stdev <= 0.0) {  // how could it be?? nature finds a way
	    for(i=0;i<2*NPTS;++i){
		 cout[i] = corr[i]; 
	    }  /* don't normalize to sdev */
	    return(1); /* return here since sdev is not usable */
       }
  
       /* end of stdev section */
  
       /* replace the waveform with normalized correlation */
       for(i=0;i<NPTS;++i){
	    cout[i] = corr[i]/stdev; 
       }
  
       if(fabs(mean) > 3.0*stdev) return(2);
  
       return(0);  /* in this case all is well */
  }
  else
  {
       //if we fail, cut this channel
       //an ugly hack--zero out the waveform
       for (i=0;i<2*NPTS; i++){
	    cout[i]=0.;
       }
       //another ugly hack--increment globabl varaible FFTNumChannels
       //this must be zeroed tested in main program
       FFTNumChannels++; 
       return(0);
  }
}

