#include "AnitaInstrument.h"

//indices have surf channel counting from zero.
int topRingIndex[16][2]={{4,0},//phi=1
			 {13,9},//phi=2
			 {22,18},//phi=3
			 {31,27},//phi=4
			 {40,36},//phi=5
			 {49,45},//phi=6
			 {58,54},//phi=7
			 {67,63},//phi=8
			 {5,1},//phi=9
			 {14,10},//phi=10
			 {23,19},//phi=11
			 {32,28},//phi=12
			 {41,37},//phi=13
			 {50,46},//phi=14
			 {59,55},//phi=15
			 {68,64}};//phi=16

int botRingIndex[16][2]={{6,2},//phi=1
			 {15,11},//phi=2
			 {24,20},//phi=3
			 {33,29},//phi=4
			 {42,38},//phi=5
			 {51,47},//phi=6
			 {60,56},//phi=7
			 {69,65},//phi=8
			 {7,3},//phi=9
			 {16,12},//phi=10
			 {25,21},//phi=11
			 {34,30},//phi=12
			 {43,39},//phi=13
			 {52,48},//phi=14
			 {61,57},//phi=15
			 {70,66}};//phi=16

int biconeIndex[4]={72,73,74,75}; //phi=2,6,10,14
int disconeIndex[4]={77,78,79,76}; //phi=4,8,12,16

void BuildInstrument3(AnitaTransientBody3_t *surfData,
		     AnitaInstrument3_t  *antennaData)
{			 
     int i,j;
     for (i=0;i<16;i++){
	  for (j=0; j<2; j++){
	       antennaData->topRing[i][j]=&(surfData->ch[topRingIndex[i][j]]);
	       antennaData->botRing[i][j]=&(surfData->ch[botRingIndex[i][j]]);
	  }
     }
     for (i=0;i<4;i++){
	  antennaData->bicone[i]=&(surfData->ch[biconeIndex[i]]);
	  antennaData->discone[i]=&(surfData->ch[disconeIndex[i]]);
     }
}

void Instrument3toF(AnitaInstrument3_t *intData,
		     AnitaInstrumentF_t  *floatData)
{			 
     int i,j,k;
     for (i=0;i<16;i++){
	  for (j=0; j<2; j++){
	       for (k=0; k<(intData->topRing[i][j])->valid_samples;k++){
		    (floatData->topRing[i][j]).data[k]= 
			 (intData->topRing[i][j])->data[k]/8.;
	       }
	       (floatData->topRing[i][j]).valid_samples
		    =(intData->topRing[i][j])->valid_samples;
	  
	       for (k=0; k<(intData->botRing[i][j])->valid_samples;k++){
		    (floatData->botRing[i][j]).data[k]= 
			 (intData->botRing[i][j])->data[k]/8.;
	       }
	       (floatData->botRing[i][j]).valid_samples
		    =(intData->botRing[i][j])->valid_samples;
	  }
     }
     for (i=0;i<4;i++){
       for (k=0; k<(intData->bicone[i])->valid_samples;k++){
		    (floatData->bicone[i]).data[k]= 
			 (intData->bicone[i])->data[k]/8.;
	       }
	       (floatData->bicone[i]).valid_samples
		    =(intData->bicone[i])->valid_samples;
       for (k=0; k<(intData->discone[i])->valid_samples;k++){
		    (floatData->discone[i]).data[k]= 
			 (intData->discone[i])->data[k]/8.;
	       }
	       (floatData->discone[i]).valid_samples
		    =(intData->discone[i])->valid_samples;
     }
}


int RMS(TransientChannel3_t *in)
{
     int i;
     double accum=0;
     double dresult;
     int result;
     for (i=0;i<(in->valid_samples); i++){
	  accum += (in->data[i])*(in->data[i]);
     }
     accum /= (in->valid_samples);
     dresult= sqrt(accum) +0.5;
     result= dresult;
     if (result==0) result=1; //don't want zero for the RMS 
     return result;           // or we get zero threshold!
}

float RMSF(TransientChannelF_t *in)
{
     int i;
     float accum=0;
     float result;
     for (i=0;i<(in->valid_samples); i++){
	  accum += (in->data[i])*(in->data[i]);
     }
     accum /= (in->valid_samples);
     result= sqrtf(accum);
     return result;
}

void Discriminate(TransientChannel3_t *in,LogicChannel_t *out,
		  int threshold, int width)
{
     int i,k;
     out->valid_samples=in->valid_samples;
     k=0;
     for (i=0;i<(in->valid_samples); i++){
	  if (abs(in->data[i])>threshold){
	       k=width;
	  }
	  if (k>0){
	       out->data[i]=1;
	       k--;
	  }
	  else{
	       out->data[i]=0;
	  }
     }
}

void DiscriminateF(TransientChannelF_t *in,LogicChannel_t *out,
		 float threshold, int width)
{
     int i,k;
     out->valid_samples=in->valid_samples;
     k=0;
     for (i=0;i<(in->valid_samples); i++){
	  if (fabsf(in->data[i])>threshold){
	       k=width;
	  }
	  if (k>0){
	       out->data[i]=1;
	       k--;
	  }
	  else{
	       out->data[i]=0;
	  }
     }
}

void DiscriminateChannels(AnitaInstrument3_t *in,
			  AnitaChannelDiscriminator_t *out,
			  int hornthresh,int hornwidth,
			  int conethresh,int conewidth)
{
     int phi,pol,i, thresh;
     double dhornthresh;
     dhornthresh= ((double) hornthresh)/100.;
     for (phi=0;phi<16; phi++){
	  for (pol=0;pol<2;pol++){
	       thresh = (int)(RMS(in->topRing[phi][pol])*dhornthresh); 
	       Discriminate(in->topRing[phi][pol],&(out->topRing[phi][pol]),
			    thresh,hornwidth);
	       thresh = (int)(RMS(in->botRing[phi][pol])*dhornthresh); 
	       Discriminate(in->botRing[phi][pol],&(out->botRing[phi][pol]),
			    thresh,hornwidth);
	  }
     }
	  
     for (i=0; i<4; i++){	       
	  thresh= (int)(RMS(in->bicone[i]) 
			     * (double) conethresh/100.);
	       Discriminate(in->bicone[i],&(out->bicone[i]),
			    thresh,conewidth);
	  thresh= (int)(RMS(in->discone[i]) 
			     * (double) conethresh/100.);
	       Discriminate(in->discone[i],&(out->discone[i]),
			    thresh,conewidth);
     }
	  
}

void DiscriminateFChannels(AnitaInstrumentF_t *in,
			  AnitaChannelDiscriminator_t *out,
			  int hornthresh,int hornwidth,
			  int conethresh,int conewidth)
{
     int phi,pol,i; 
     float thresh, fhornthresh;
     fhornthresh= ((float) hornthresh)/100.;
     for (phi=0;phi<16; phi++){
	  for (pol=0;pol<2;pol++){
	       thresh = (RMSF(&(in->topRing[phi][pol]))*fhornthresh); 
	       DiscriminateF(&(in->topRing[phi][pol]),
			     &(out->topRing[phi][pol]),
			     thresh,hornwidth);
	       thresh = (RMSF(&(in->botRing[phi][pol]))*fhornthresh); 
	       DiscriminateF(&(in->botRing[phi][pol]),
			     &(out->botRing[phi][pol]),
			     thresh,hornwidth);
	  }
     }
	  
     for (i=0; i<4; i++){	       
	  thresh= (RMSF(&(in->bicone[i])) 
			     * (float) conethresh/100.);
	       DiscriminateF(&(in->bicone[i]),&(out->bicone[i]),
			    thresh,conewidth);
	  thresh= (RMSF(&(in->discone[i])) 
			     * (float) conethresh/100.);
	       DiscriminateF(&(in->discone[i]),&(out->discone[i]),
			    thresh,conewidth);
     }
	  
}

void FormSectorMajority(AnitaChannelDiscriminator_t *in,
			AnitaSectorLogic_t *out,
			int sectorWidth)
{
     //this is brute force--one can save some adds by doing
     //ringwise sums and thaen taking differences rather than
     //resumming
     int sector_phi,phi,pol,i,j,minvalid;
     //top ring
     for(sector_phi=0;sector_phi<16;sector_phi++){
	  for (j=0;j<MAX_NUMBER_SAMPLES; j++){
	       (out->topRing[sector_phi]).data[j]=0;
	  }
	  minvalid=MAX_NUMBER_SAMPLES;
	  for(pol=0;pol<2;pol++){
	       for (i=0;i<sectorWidth;i++){
		    phi=(sector_phi+i)%16;
		    for (j=0;j<(in->topRing[phi][pol]).valid_samples; j++){
			 (out->topRing[sector_phi]).data[j] += (in->topRing[phi][pol]).data[j];
		    }
		    if (minvalid > (in->topRing[phi][pol]).valid_samples) 
			 minvalid=(in->topRing[phi][pol]).valid_samples;
	       }
	  }
	  (out->topRing[sector_phi]).valid_samples = minvalid;
     }
     //bottom ring
     for(sector_phi=0;sector_phi<16;sector_phi++){
	  for (j=0;j<MAX_NUMBER_SAMPLES; j++){
	       (out->botRing[sector_phi]).data[j]=0;
	  }
	  minvalid=MAX_NUMBER_SAMPLES;
	  for(pol=0;pol<2;pol++){
	       for (i=0;i<sectorWidth;i++){
		    phi=(sector_phi+i)%16;
		    for (j=0;j<(in->botRing[phi][pol]).valid_samples; j++){
			 (out->botRing[sector_phi]).data[j] += (in->botRing[phi][pol]).data[j];
		    }
		    if (minvalid > (in->botRing[phi][pol]).valid_samples) 
			 minvalid=(in->botRing[phi][pol]).valid_samples;
	       }
	  }
	  (out->botRing[sector_phi]).valid_samples = minvalid;
     }
     //bicones
     for (j=0;j<MAX_NUMBER_SAMPLES; j++){
	  (out->bicone).data[j]=0;
     }
     minvalid=MAX_NUMBER_SAMPLES;
     for (i=0;i<4;i++){
	  for (j=0;j<(in->bicone[i]).valid_samples; j++){
	       (out->bicone).data[j] += (in->bicone[i]).data[j];
	  }
	  if (minvalid > (in->bicone[i]).valid_samples) 
	       minvalid=(in->bicone[i]).valid_samples;
     }
     (out->bicone).valid_samples = minvalid;
     //discones
     for (j=0; j<MAX_NUMBER_SAMPLES; j++){
	  (out->discone).data[j]=0;
     }
     minvalid=MAX_NUMBER_SAMPLES;
     for (i=0;i<4;i++){
	  for (j=0;j<(in->discone[i]).valid_samples; j++){
	       (out->discone).data[j] += (in->discone[i]).data[j];
	  }
	  if (minvalid > (in->discone[i]).valid_samples) 
	       minvalid=(in->discone[i]).valid_samples;
     }
     (out->discone).valid_samples = minvalid;
}
