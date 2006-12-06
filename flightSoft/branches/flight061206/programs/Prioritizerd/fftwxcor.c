/*  A cross-correlation of two waveforms using the FFT
    method. See Numerical recipes.

  Tuned for ANITA waveform data. 


--Peter Gorham August 2006 

Modified by Ped to use fftw;
adapted to flight software environment JJB
8/14/06
power spectrum peak cutter added JJB
Sun Nov 26 05:11:19 2006

*/


#include <stdio.h>
#include <values.h>
#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <complex.h>
#include "fftw3.h"  //local copy used to simplify builds


#define NPTS 256              /* size of the waveform and template arrays */
                              /*        must be power of 2 */
#define WFRAC  0.33333        /* lead fraction of waveform */
                              /*        to use for noise statistics */
	
			      

/*-------------driver example for anitaxcor------------------------------*/
/*  ---------------- BEGIN MAIN ----------------------*/

/* main(int argc, char *argv[]){ */

/*   int retval, i, fftwcorr(),n; */
/*   float *wfm_in, *corr_out, *tmpl_in; */
/*   FILE *fpwfm, *fptmpl; */
  
/*   if(argc<3){ */
/*     fprintf(stderr, */
/* 	    "usage: anitaxcor [waveform file][template file] > corrfile\n"); */
/*     exit(0); */
/*   } */
/*   wfm_in = (float*)calloc(NPTS, sizeof(float)); */
/*   tmpl_in = (float*)calloc(NPTS, sizeof(float)); */
/*   corr_out = (float*)calloc(2*NPTS, sizeof(float)); */
  
/*   fpwfm = fopen(argv[1],"r"); */
/*   fptmpl = fopen(argv[2],"r"); */
  
/*   /\* read in the data waveform and template waveform *\/ */
/*   for(i=0;i<NPTS;i++) fscanf(fpwfm,"%f", wfm_in+i); */
/*   fclose(fpwfm); */
/*   for(i=0;i<NPTS;i++) fscanf(fptmpl,"%f", tmpl_in+i); */
/*   fclose(fptmpl); */
  
/*   n=0; */
/*   while(n++<1e5) */
/*     retval = fftwcorr(wfm_in, tmpl_in, corr_out); */
  
/*   if(retval){ */
/*     fprintf(stderr,"anitaxcor error! will proceed anyway...\n"); */
/*   }  */
  
/*   //for(i=0;i<NPTS;i++) fprintf(stdout,"%e\n",corr_out[i]); */
  
  
/*   free(wfm_in); */
/*   free(tmpl_in); */
/*   free(corr_out); */
  
/*   exit(0); */
  
/* } */

/*----------------------------------------------------*/

extern int MethodMask,FFTPeakMaxA,FFTPeakMaxB,FFTPeakWindowL,FFTPeakWindowR;

extern int FFTNumChannels;

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
       for(i=0;i<=NPTS/2;++i) wfm_fft[i]=wfm_fft[i]*tmpl_fft[i];
       
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
	    for(i=0;i<NPTS;++i){
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
       for (i=0;i<NPTS; i++){
	    cout[i]=0.;
       }
       //another ugly hack--increment globabl varaible FFTNumChannels
       //this must be zeroed tested in main program
       FFTNumChannels++; 
       return(0);
  }
}
