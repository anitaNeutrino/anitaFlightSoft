#include <anitaFlight.h>
#include "Filters.h"
#include "AnitaInstrument.h"
#include "fftwxcor.h"
#include <stdio.h>
#include <stdlib.h>

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







void HornMatchedFilter(TransientChannelF_t *inch,TransientChannelF_t *outch)
{
//     int status,i;
    int i;
     static int first=1;
     static float template[256];
     float corr_out[512];
     FILE *fptmpl;
     char templateName[FILENAME_MAX];
     if (first){
	  //read in template
#ifdef USING_PVIEW	
	 sprintf(templateName,"horn_template.dat");
#else
	 char *anitaDir=getenv("ANITA_FLIGHT_SOFT_DIR");
	 if(anitaDir) 
	     sprintf(templateName,"%s/programs/Prioritizerd/horn_template.dat",anitaDir);
	 else
	     sprintf(templateName,"horn_template.dat");
#endif
//	 printf("%s\n",templateName);
	 fptmpl=fopen(templateName,"r");
	     if (fptmpl==NULL){
	       fprintf(stderr,"Unable to open horn template.\n");
	       exit(-1);
	  }
	  for (i=0;i<256;i++){
	       fscanf(fptmpl,"%f",&(template[i]));
	  }
	  fclose(fptmpl);
	  first=0;
     }
     fftwcorr(inch->data,template,corr_out);
     for (i=0;i<inch->valid_samples;i++){
	  outch->data[i]=corr_out[i];
     }
     outch->valid_samples=inch->valid_samples;
}

void HornMatchedFilterAll(AnitaInstrumentF_t *in, 
			  AnitaInstrumentF_t *out)
{
     int i,j;
     for (i=0;i<16;i++){
	  for (j=0;j<2;j++){
	       HornMatchedFilter(&(in->topRing[i][j]),&(out->topRing[i][j]));
	       HornMatchedFilter(&(in->botRing[i][j]),&(out->botRing[i][j]));
	  }
     }
     for (i=0;i<4;i++){
	  HornMatchedFilter(&(in->bicone[i]),&(out->bicone[i]));
	  HornMatchedFilter(&(in->discone[i]),&(out->discone[i]));
     }
			    
}

void HornMatchedFilterSmooth(TransientChannelF_t *inch,TransientChannelF_t *outch)
{
//     int status,i;
    int i;
     static int first=1;
     static float template[256];
     float corr_out[512];
     float stdev;
     FILE *fptmpl;
     char templateName[FILENAME_MAX];
     if (first){
	  //read in template
#ifdef USING_PVIEW	
	 sprintf(templateName,"horn_template.dat");
#else
	 char *anitaDir=getenv("ANITA_FLIGHT_SOFT_DIR");
	 if(anitaDir) 
	     sprintf(templateName,"%s/programs/Prioritizerd/horn_template.dat",anitaDir);
	 else
	     sprintf(templateName,"horn_template.dat");
#endif
//	 printf("%s\n",templateName);
	 fptmpl=fopen(templateName,"r");
	     if (fptmpl==NULL){
	       fprintf(stderr,"Unable to open horn template.\n");
	       exit(-1);
	  }
	  for (i=0;i<256;i++){
	       fscanf(fptmpl,"%f",&(template[i]));
	  }
	  fclose(fptmpl);
	  first=0;
     }
     fftwcorr(inch->data,template,corr_out);
//three point binomial smoothing of the absolute value
     for (i=1;i<inch->valid_samples-1;i++){
	  outch->data[i-1]=0.25*fabsf(corr_out[i-1])
	       +0.5*fabsf(corr_out[i])+0.25*fabsf(corr_out[i+1]);
     }
     outch->valid_samples=inch->valid_samples-2;
//now renormailze to the rms of the first ~third
     stdev=0.;
     for (i=0; i<85; i++){
	  stdev+=outch->data[i]*outch->data[i];
     }
     stdev=sqrt(stdev/85.);
     if (stdev>0){
	  for (i=0; i<outch->valid_samples;i++){
	       outch->data[i]=outch->data[i]/stdev;
	  }
     }
}

void HornMatchedFilterAllSmooth(AnitaInstrumentF_t *in, 
			  AnitaInstrumentF_t *out)
{
     int i,j;
     for (i=0;i<16;i++){
	  for (j=0;j<2;j++){
	       HornMatchedFilterSmooth(&(in->topRing[i][j]),&(out->topRing[i][j]));
	       HornMatchedFilterSmooth(&(in->botRing[i][j]),&(out->botRing[i][j]));
	  }
     }
     for (i=0;i<4;i++){
	  HornMatchedFilterSmooth(&(in->bicone[i]),&(out->bicone[i]));
	  HornMatchedFilterSmooth(&(in->discone[i]),&(out->discone[i]));
     }
			    
}
    

void HornMatchedFilterAllBlind(AnitaInstrumentF_t *in, 
			  AnitaInstrumentF_t *out,AnitaInstrumentF_t *pow_out)
{
     int i,j,k,ii;
     float mean;
     AnitaInstrumentF_t pow_tmp;
     for (i=0;i<16;i++){
	  for (j=0;j<2;j++){
	       HornMatchedFilter(&(in->topRing[i][j]),&(out->topRing[i][j]));
	       HornMatchedFilter(&(in->botRing[i][j]),&(out->botRing[i][j]));
	  }
	  // fill pow_out pol 0 with the smoothed sum of the squares if the two polarizations
	  // set the record length to the shorter of the two
	  (pow_tmp.topRing[i][0]).valid_samples=
	       (out->topRing[i][0].valid_samples>out->topRing[i][1].valid_samples)?
	       out->topRing[i][1].valid_samples:
	       out->topRing[i][0].valid_samples;
	  //compute the total power
	  for (k=0; k<(pow_tmp.topRing[i][0]).valid_samples; k++){
	       (pow_tmp.topRing[i][0]).data[k]=
		    out->topRing[i][0].data[k]*out->topRing[i][0].data[k]
		    +out->topRing[i][1].data[k]*out->topRing[i][1].data[k];
	  } 
	  // smooth it into pow_out; three point 1-2-1 Boxcar
	  pow_out->topRing[i][0].valid_samples=(pow_tmp.topRing[i][0]).valid_samples-2;
	  for (k=1; k<(pow_tmp.topRing[i][0]).valid_samples-1; k++){
	       pow_out->topRing[i][0].data[k-1]=0.25*(pow_tmp.topRing[i][0]).data[k-1]
		    +0.5*(pow_tmp.topRing[i][0]).data[k]+0.25*(pow_tmp.topRing[i][0]).data[k+1];
	  }
	  //normalize
	  mean=0.;
	  for (ii=0; ii<85; ii++){
	       mean+=pow_out->topRing[i][0].data[ii];
	  }
	  mean=mean/85.;
	  if (mean>0){
	       for (ii=0; ii<pow_out->topRing[i][0].valid_samples;ii++){
		    pow_out->topRing[i][0].data[ii] /= mean;
	       }
	  }
	  //copy into the other polarization so we don't break anything
	  for (k=0; k<(pow_out->topRing[i][0]).valid_samples; k++){
//	       pow_out->topRing[i][1].data[k]=pow_out->topRing[i][0].data[k];
	       pow_out->topRing[i][1].data[k]=1.;
	  }
	  // now do the bottom ring horn
	  // fill pow_out pol 0 with the smoothed sum of the squares if the two polarizations
	  // set the record length to the shorter of the two
	  (pow_tmp.botRing[i][0]).valid_samples=
	       (out->botRing[i][0].valid_samples>out->botRing[i][1].valid_samples)?
	       out->botRing[i][1].valid_samples:
	       out->botRing[i][0].valid_samples;
	  //compute the total power
	  for (k=0; k<(pow_tmp.botRing[i][0]).valid_samples; k++){
	       (pow_tmp.botRing[i][0]).data[k]=
		    out->botRing[i][0].data[k]*out->botRing[i][0].data[k]
		    +out->botRing[i][1].data[k]*out->botRing[i][1].data[k];
	  } 
	  // smooth it into pow_out; three point 1-2-1 Boxcar
	  pow_out->botRing[i][0].valid_samples=(pow_tmp.botRing[i][0]).valid_samples-2;
	  pow_out->botRing[i][1].valid_samples=pow_out->botRing[i][0].valid_samples;
	  for (k=1; k<(pow_tmp.botRing[i][0]).valid_samples-1; k++){
	       pow_out->botRing[i][0].data[k-1]=0.25*(pow_tmp.botRing[i][0].data[k-1])
		    +0.5*(pow_tmp.botRing[i][0]).data[k]+0.25*(pow_tmp.botRing[i][0]).data[k+1];
	  }
	  //normalize
	  mean=0.;
	  for (ii=0; ii<85; ii++){
	       mean+=pow_out->botRing[i][0].data[ii];
	  }
	  mean=mean/85.;
	  if (mean>0){
	       for (ii=0; ii<pow_out->botRing[i][0].valid_samples;ii++){
		    pow_out->botRing[i][0].data[ii] /= mean;
	       }
	  }
	  //copy into the other polarization so we don't break anything
	  for (k=0; k<(pow_out->botRing[i][0]).valid_samples; k++){
//	       pow_out->botRing[i][1].data[k]=pow_out->botRing[i][0].data[k];
	       pow_out->botRing[i][1].data[k]=1.
	  }
     }
   
     for (i=0;i<4;i++){
	  HornMatchedFilter(&(in->bicone[i]),&(out->bicone[i]));
	  HornMatchedFilter(&(in->discone[i]),&(out->discone[i]));
     }
			    
}
      
