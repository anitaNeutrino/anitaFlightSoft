/*-------------driver example for anitaxcor------------------------------*/
/*  ---------------- BEGIN MAIN ----------------------*/

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#define NPTS 256

int MethodMask=0x1867;
int FFTPeakWindowL=200;
int FFTPeakWindowR=225;
int FFTPeakMaxA=2560;
int FFTPeakMaxB=2560;
int FFTNumChannels=50;

int fftwcorr_anal_1x(float wfm[], float tmpl[], float cout[]);
int fftwcorr_anal_2x(float wfm[], float tmpl[], float cout[]);

int main(int argc, char *argv[]){

  int retval, i, fftwcorr(),n;
  float *wfm_in, *corr_out, *tmpl_in;
  FILE *fpwfm, *fptmpl;
  
  if(argc<3){
    fprintf(stderr,
	    "usage: anitaxcor [waveform file][template file] > corrfile\n");
    exit(0);
  }
  wfm_in = (float*)calloc(NPTS, sizeof(float));
  tmpl_in = (float*)calloc(NPTS, sizeof(float));
  corr_out = (float*)calloc(2*NPTS, sizeof(float));
  
  fpwfm = fopen(argv[1],"r");
  fptmpl = fopen(argv[2],"r");
  
  /* read in the data waveform and template waveform */
  for(i=0;i<NPTS;i++) fscanf(fpwfm,"%f", wfm_in+i);
  fclose(fpwfm);
  for(i=0;i<NPTS;i++) fscanf(fptmpl,"%f", tmpl_in+i);
  fclose(fptmpl);
  
  n=0;
  while(n++<1000000)
    retval = fftwcorr_anal(wfm_in, tmpl_in, corr_out);
  
  if(retval){
    fprintf(stderr,"anitaxcor error! will proceed anyway...\n");
  }
  
  for(i=0;i<NPTS;i++) fprintf(stdout,"%e\n",corr_out[i]);
  
  
  free(wfm_in);
  free(tmpl_in);
  free(corr_out);
  
  exit(0);
  
}

/*----------------------------------------------------*/
