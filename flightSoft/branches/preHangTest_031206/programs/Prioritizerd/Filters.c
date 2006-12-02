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
     
