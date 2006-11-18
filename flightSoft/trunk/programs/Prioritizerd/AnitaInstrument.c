#include "AnitaInstrument.h"
#include <stdlib.h>
#include <string.h>
//indices have surf channel counting from zero.
// 0 is H, 1 is V polarization
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

float topPhaseCenter[16][3]= //x,y,z in ANITA system and  meters
{{+0.343911158,-0.818999931,2.297455764},
 {+0.747353991,-0.739103994,3.264805393},
 {+0.827217576,-0.330144125,2.297691265},
 {+1.057692567,+0.011432190,3.25669364},
 {+0.825096141,+0.351276783,2.295486482},
 {+0.742882419,+0.759177493,3.256979776},
 {+0.335477367,+0.832938692,2.294432642},
 {-0.009868911,+1.065670172,3.250239882},
 {-0.347508412,+0.827665665,2.294151136},
 {-0.754570743,+0.747560707,3.257922104},
 {-0.833303369,+0.337227639,2.294410305},
 {-1.064674471,-0.005095512,3.252428434},
 {-0.828460163,-0.344022074,2.298099782},
 {-0.746015119,-0.752850657,3.260711454},
 {-0.339637450,-0.825824197,2.298436993},
 {+0.000176346,-1.057640617,3.253784245}};

float topDelay[16][2]
{{54.361,54.361}, {54.361,54.361}, {54.361,54.361}, {54.361,54.361},
 {54.361,54.361}, {54.361,54.361}, {54.361,54.361}, {54.361,54.361},
 {54.361,54.361}, {54.361,54.361}, {54.361,54.361}, {54.361,54.361},
 {54.361,54.361}, {54.361,54.361}, {54.361,54.361}, {54.361,54.361},
 {54.361,54.361}, {54.361,54.361}, {54.361,54.361}, {54.361,54.361}};

float botDelay[16][2]
{{58.711,58.711}, {58.711,58.711}, {58.711,58.711}, {58.711,58.711},
 {58.711,58.711}, {58.711,58.711}, {58.711,58.711}, {58.711,58.711},
 {58.711,58.711}, {58.711,58.711}, {58.711,58.711}, {58.711,58.711},
 {58.711,58.711}, {58.711,58.711}, {58.711,58.711}, {58.711,58.711},
 {58.711,58.711}, {58.711,58.711}, {58.711,58.711}, {58.711,58.711}};

float botPhaseCenter[16][3]=
{{+0.855172039,-2.047905126,-0.465003202},
 {+1.568636024,-1.567704113,-0.465585632},
 {+2.041443376,-0.849297952,-0.470665149},
 {+2.206983685,-0.003090258,-0.475088126},
 {+2.035058249,+0.844274021,-0.478826280},
 {+1.552698275,+1.561512075,-0.480297744},
 {+0.841429082,+2.043190584,-0.475248344},
 {-0.005778404,+2.216227158,-0.465624609},
 {-0.850403201,+2.047865227,-0.462304762},
 {-1.565934322,+1.566267349,-0.464389098},
 {-2.039320905,+0.847811004,-0.472890301},
 {-2.203564799,-0.001549400,-0.476755087},
 {-2.032988209,-0.845273619,-0.480870647},
 {-1.553616722,-1.562197379,-0.472874280},
 {-0.833912700,-2.044776636,-0.472072118},
 {+0.007623118,-2.217216949,-0.470283761}};

float biconePhaseCenter[4][3]=
{{+1.7649444,-1.7613122,0.6042152}, 
 {+1.7580610,+1.7587976,0.5919724},
 {-1.7659096,+1.7610074,0.6085332},
 {-1.7520412,-1.7638268,0.6001512}};

float disconePhaseCenter[4][3]=
{{-0.0001016,-1.1253216,1.3733526},
 {+1.1294110,+0.0095504,1.3762482},
 {-0.0069088,+1.1292078,1.3707110},
 {-1.1322812,+0.0026670,1.3741146}};

//nominal view direction in ACS phi degrees by phi sector
float sectorPhiACS[16]={ +292.5,+315.0,+337.5,  +0.0,
			 +22.5 , +45.0, +67.5, +90.0,
			 +112.5,+135.0,+157.5,+180.0
			 +202.5,+225.0,+247.5,+270.0}

float dipangle=-10. //look angle of antenna in degrees




float samples_per_ns=2.6; //SURF nominal
float meters_per_ns=0.299792458; //speed of light in meters per ns

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


void BuildInstrumentF(AnitaTransientBodyF_t *surfData,
		      AnitaInstrumentF_t  *antennaData)
{			 
     int i,j;
     for (i=0;i<16;i++){
	  for (j=0; j<2; j++){
	       antennaData->topRing[i][j]=(surfData->ch[topRingIndex[i][j]]);
	       antennaData->botRing[i][j]=(surfData->ch[botRingIndex[i][j]]);
	  }
     }
     for (i=0;i<4;i++){
	  antennaData->bicone[i]=(surfData->ch[biconeIndex[i]]);
	  antennaData->discone[i]=(surfData->ch[disconeIndex[i]]);
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
//	       printf("Here %f\n",in->data[i]);
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
//	       printf("thresh %f\n",thresh);
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
			if((in->topRing[phi][pol]).data[j]>0) {
//			    printf("Wahyyy\n");
			}
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
