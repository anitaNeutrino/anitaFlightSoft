#include "AnitaInstrument.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int FFTNumChannels; //hack to communicate fft info from the crosscorrelator

extern int MethodMask;

#ifdef ANITA2
//indices have surf channel counting from zero.
// 0 is H, 1 is V polarization
const int topRingIndex[16][2]={{ 4, 0},//phi=1
			       {49,45},//phi=2
			       {22,18},//phi=3
			       {67,63},//phi=4
			       {31,27},//phi=5
			       {58,54},//phi=6
			       {40,36},//phi=7
			       {13, 9},//phi=8
			       { 5, 1},//phi=9
			       {50,46},//phi=10
			       {23,19},//phi=11
			       {68,64},//phi=12
			       {32,28},//phi=13
			       {59,55},//phi=14
			       {41,37},//phi=15
			       {14,10}};//phi=16

const int botRingIndex[16][2]={{ 6, 2},//phi=1
			       {51,47},//phi=2
			       {24,20},//phi=3
			       {69,65},//phi=4
			       {33,29},//phi=5
			       {60,56},//phi=6
			       {42,38},//phi=7
			       {15,11},//phi=8
			       { 7, 3},//phi=9
			       {52,48},//phi=10
			       {25,21},//phi=11
			       {70,66},//phi=12
			       {34,30},//phi=13
			       {61,57},//phi=14
			       {43,39},//phi=15
			       {16,12}};//phi=16
#else //ANITA2
// ANITA 1 values
//indices have surf channel counting from zero.
// 0 is H, 1 is V polarization
const int topRingIndex[16][2]={{ 4, 0},//phi=1
			       {13, 9},//phi=2
			       {22,18},//phi=3
			       {31,27},//phi=4
			       {40,36},//phi=5
			       {49,45},//phi=6
			       {58,54},//phi=7
			       {67,63},//phi=8
			       { 5, 1},//phi=9
			       {14,10},//phi=10
			       {23,19},//phi=11
			       {32,28},//phi=12
			       {41,37},//phi=13
			       {50,46},//phi=14
			       {59,55},//phi=15
			       {68,64}};//phi=16
const int botRingIndex[16][2]={{ 6, 2},//phi=1
			       {15,11},//phi=2
			       {24,20},//phi=3
			       {33,29},//phi=4
			       {42,38},//phi=5
			       {51,47},//phi=6
			       {60,56},//phi=7
			       {69,65},//phi=8
			       { 7, 3},//phi=9
			       {16,12},//phi=10
			       {25,21},//phi=11
			       {34,30},//phi=12
			       {43,39},//phi=13
			       {52,48},//phi=14
			       {61,57},//phi=15
			       {70,66}};//phi=16
#endif //not ANITA2, use ANITA 1

#ifdef ANITA2
const int nadirIndex[8][2]={{76,72},//phi=1
			    {84,80},//phi=3
			    {77,73},//phi=5
			    {85,81},//phi=7
			    {78,74},//phi=9
			    {86,82},//phi=11
			    {79,75},//phi=13
			    {87,83}};//phi=15
#else //ANITA2
const int biconeIndex[4]={72,73,74,75}; //phi=2,6,10,14
const int disconeIndex[4]={77,78,79,76}; //phi=4,8,12,16
#endif //not ANITA2

const float topPhaseCenter[16][3]= //x,y,z in ANITA system and  meters
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

const float topDelay[16][2]=
{{54.361,54.361}, {54.361,54.361}, {54.361,54.361}, {54.361,54.361},
 {54.361,54.361}, {54.361,54.361}, {54.361,54.361}, {54.361,54.361},
 {54.361,54.361}, {54.361,54.361}, {54.361,54.361}, {54.361,54.361},
 {54.361,54.361}, {54.361,54.361}, {54.361,54.361}, {54.361,54.361}};

const float botDelay[16][2]=
{{58.711,58.711}, {58.711,58.711}, {58.711,58.711}, {58.711,58.711},
 {58.711,58.711}, {58.711,58.711}, {58.711,58.711}, {58.711,58.711},
 {58.711,58.711}, {58.711,58.711}, {58.711,58.711}, {58.711,58.711},
 {58.711,58.711}, {58.711,58.711}, {58.711,58.711}, {58.711,58.711}};

const float botPhaseCenter[16][3]=
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

#ifdef ANITA2
const float nadirPhaseCenter[8][3]=
{{+0.0000000,+0.0000000,+0.0000000},
 {+0.0000000,+0.0000000,+0.0000000},
 {+0.0000000,+0.0000000,+0.0000000},
 {+0.0000000,+0.0000000,+0.0000000},
 {+0.0000000,+0.0000000,+0.0000000},
 {+0.0000000,+0.0000000,+0.0000000},
 {+0.0000000,+0.0000000,+0.0000000},
 {+0.0000000,+0.0000000,+0.0000000}};

#else //ANITA2

const float biconePhaseCenter[4][3]=
{{+1.7649444,-1.7613122,0.6042152}, 
 {+1.7580610,+1.7587976,0.5919724},
 {-1.7659096,+1.7610074,0.6085332},
 {-1.7520412,-1.7638268,0.6001512}};

const float disconePhaseCenter[4][3]=
{{-0.0001016,-1.1253216,1.3733526},
 {+1.1294110,+0.0095504,1.3762482},
 {-0.0069088,+1.1292078,1.3707110},
 {-1.1322812,+0.0026670,1.3741146}};

#endif //not ANITA2

//nominal view direction in ACS phi degrees by phi sector
const float sectorPhiACS[16]={ +292.5,+315.0,+337.5,  +0.0,
			       +22.5 , +45.0, +67.5, +90.0,
			       +112.5,+135.0,+157.5,+180.0
			       +202.5,+225.0,+247.5,+270.0};

const float dipangle=-10.; //look angle of antenna in degrees

const float samples_per_ns=2.6; //SURF nominal
const float meters_per_ns=0.299792458; //speed of light in meters per ns

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
#ifdef ANITA2
     for (i=0;i<8;i++){
	  for (j=0; j<2; j++){
	       antennaData->nadir[i][j]=(surfData->ch[nadirIndex[i][j]]);
	  }
     }
#else //ANITA2
     for (i=0;i<4;i++){
	  antennaData->bicone[i]=(surfData->ch[biconeIndex[i]]);
	  antennaData->discone[i]=(surfData->ch[disconeIndex[i]]);
     }
#endif     
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

void DiscriminateF_noup(TransientChannelF_t *in,LogicChannel_t *out,
		   float threshold, int width,int holdoff)
{
     int i,k,h;
     out->valid_samples=in->valid_samples;
     k=0;
     h=0;
     for (i=0;i<(in->valid_samples); i++){
	  if (k>0){
	       out->data[i]=1;
	       k--;
	       if (k==0) h=holdoff;
	  }
	  else if (h>0)
	  {
	       out->data[i]=0;
	       h--;
	  }
	  else if (fabsf(in->data[i])>threshold){
	       k=width-1;
	       out->data[i]=1;
	       if (k==0) h=holdoff;
	  }
	  else{
	       out->data[i]=0;
	  }
     }
}

extern int MethodMask;
extern int NuCut;

int PeakBoxcarOne(TransientChannelF_t *in,LogicChannel_t *out,
		   int width, int guardOffset, int guardWidth,int guardThresh)
{
// out is a boxcar width wide after the maximum absolute value of the input
// Return value is the peak sample number.
//
// if guardWidth is greater than zero, looks in a region offset
// from the peak by guardOffset [guardOffset, guardOffset+guardWidth) for
// a value greater than guardThresh*100 times the peak value
//
// If the guard conditon is met, the boxcar is suppressed.
//
// hack in a 'neutrinoivity cut'
// 
// also hack in a minimum value for the peak SNR, bound to conethresh
//
     int i,guardLeft,guardRight;
     int peaksample;
     int peakcut;
     float peakvalue, guardvalue;
     int neutrinoish; // yes, I know this is silly...
     int winl,winr;
     float mean,var, weights;
     out->valid_samples=in->valid_samples;
     //find the peak 
     //if there are two equal, this gets the first one
     //zero the output array while we're looping
     peaksample=0; peakvalue=0.;
     for (i=0;i<(in->valid_samples); i++){
	  if (fabsf(in->data[i])>peakvalue){
	       peaksample=i;
	       peakvalue=fabsf(in->data[i]);
	  }
	  out->data[i]=0;
     }
     //now check the guard region
     //if guardWidth is zero, the loop gets skipped
     guardLeft=peaksample+guardOffset;
     guardRight=peaksample+guardOffset+guardWidth;
     if (guardLeft>=in->valid_samples){
	  guardLeft=in->valid_samples-1;
	  guardRight=in->valid_samples-1;
     }
     else if (guardRight>in->valid_samples){
	  guardRight=in->valid_samples;
     }
     guardvalue=0.;
     for (i=guardLeft; i<guardRight; i++){ 
	  if (fabsf(in->data[i])>guardvalue){
	       guardvalue=fabsf(in->data[i]);
	  }
     }
     if ((MethodMask & 0x200)!=0){ // check for neutrinolike appearance
	  //neutrinos have narrow peaks in the crosscorrelator
	  winl=((peaksample-50>=0)?(peaksample-50):0);
	  winr=((peaksample+50<=in->valid_samples)?
		(peaksample+50):in->valid_samples);
	  mean=0; weights=0;
	  var=0;
	  for (i=winl;i<winr;i++){
	       var+=(i-peaksample)*(i-peaksample)*fabsf(in->data[i]);
	       mean+=(i-peaksample)*fabsf(in->data[i]);
	       weights+=fabsf(in->data[i]);
	  }
	  mean=mean/weights;
	  var=var/weights-mean*mean;
	  if (var<NuCut) neutrinoish=1;
	  else neutrinoish=0;
     }
     else{
	  neutrinoish=1;
     }
     if ((MethodMask & 0x800) && (peakvalue<0.01*coneThresh)){
	  peakcut=1;
     }
     else{
	  peakcut=0;
     }
     if (!(guardvalue>guardThresh*peakvalue/100.) && (neutrinoish>0) 
	 && !peakcut){
	  for (i=peaksample;(i<peaksample+width)&&(i<out->valid_samples);i++){
	       out->data[i]=1;
	  }
     } 
     return peaksample;
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
#ifdef ANITA2
     for (i=0;i<8; i++){
	  for (pol=0;pol<2;pol++){
	       thresh = (RMSF(&(in->nadir[i][pol]))*fhornthresh); 
//	       printf("thresh %f\n",thresh);
	       DiscriminateF(&(in->nadir[i][pol]),
			     &(out->nadir[i][pol]),
			     thresh,hornwidth);
	  }
     }
#else //ANITA2
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
#endif //not ANITA2
}

void DiscriminateFChannels_noup(AnitaInstrumentF_t *in,
			       AnitaChannelDiscriminator_t *out,
			       int hornthresh,int hornwidth,
			       int conethresh,int conewidth,
			       int holdoff)
{
     int phi,pol,i; 
     float thresh, fhornthresh;
     fhornthresh= ((float) hornthresh)/100.;
     for (phi=0;phi<16; phi++){
	  for (pol=0;pol<2;pol++){
	       thresh = (RMSF(&(in->topRing[phi][pol]))*fhornthresh); 
//	       printf("thresh %f\n",thresh);
	       DiscriminateF_noup(&(in->topRing[phi][pol]),
			     &(out->topRing[phi][pol]),
			     thresh,hornwidth,holdoff);
	       thresh = (RMSF(&(in->botRing[phi][pol]))*fhornthresh); 
	       DiscriminateF_noup(&(in->botRing[phi][pol]),
			     &(out->botRing[phi][pol]),
			     thresh,hornwidth,holdoff);
	  }
     }
#ifdef ANITA2
     for (i=0;i<8; i++){
	  for (pol=0;pol<2;pol++){
	       thresh = (RMSF(&(in->nadir[i][pol]))*fhornthresh); 
//	       printf("thresh %f\n",thresh);
	       DiscriminateF_noup(&(in->nadir[i][pol]),
			     &(out->nadir[i][pol]),
			     thresh,hornwidth,holdoff);
	  }
     }
#else //ANITA2	  
     for (i=0; i<4; i++){	       
	  thresh= (RMSF(&(in->bicone[i])) 
		   * (float) conethresh/100.);
	  DiscriminateF_noup(&(in->bicone[i]),&(out->bicone[i]),
			thresh,conewidth,holdoff);
	  thresh= (RMSF(&(in->discone[i])) 
		   * (float) conethresh/100.);
	  DiscriminateF_noup(&(in->discone[i]),&(out->discone[i]),
			thresh,conewidth,holdoff);
     }
#endif //not ANITA2	  
}

void PeakBoxcarAll(AnitaInstrumentF_t *in,
		   AnitaChannelDiscriminator_t *out,
		   int hornWidth,int hornGuardOffset, 
		   int hornGuardWidth,int hornGuardThresh,
		   int coneWidth,int coneGuardOffset, 
		   int coneGuardWidth,int coneGuardThresh)
{
     int phi,pol,i; 
     for (phi=0;phi<16; phi++){
	  for (pol=0;pol<2;pol++){
	       PeakBoxcarOne(&(in->topRing[phi][pol]),
			     &(out->topRing[phi][pol]),
			     hornWidth,hornGuardOffset,
			     hornGuardWidth,hornGuardThresh);
	       PeakBoxcarOne(&(in->botRing[phi][pol]),
			     &(out->botRing[phi][pol]),
			     hornWidth,hornGuardOffset,
			     hornGuardWidth,hornGuardThresh);
	  }
     }
#ifdef ANITA2
     for (i=0;i<8; i++){
	  for (pol=0;pol<2;pol++){
	       PeakBoxcarOne(&(in->nadir[i][pol]),
			     &(out->nadir[2*i][pol]),
			     hornWidth,hornGuardOffset,
			     hornGuardWidth,hornGuardThresh);
	  }
     }
     for (i=0;i<8; i++){
	  for (pol=0;pol<2; pol++){
	       int left=2*i; 
	       int right=(2*i)%16;
	       int minvalid=((out->nadir[left][pol]).valid_samples<
			     (out->nadir[right][pol]).valid_samples?
			     (out->nadir[left][pol]).valid_samples:
			     (out->nadir[right][pol]).valid_samples);
	       int k;
	       for (k=0; k<minvalid; k++){
		    (out->nadir[2*i+1][pol]).data[k]=
			 (out->nadir[left][pol]).data[k]|
			 (out->nadir[right][pol]).data[k];
	       }
	       (out->nadir[2*i+1][pol]).valid_samples=minvalid;
	  }
     }
#else //ANITA2	  
     for (i=0; i<4; i++){	       
	  PeakBoxcarOne(&(in->bicone[i]),&(out->bicone[i]),
			coneWidth,coneGuardOffset,
			coneGuardWidth,coneGuardThresh);
	  PeakBoxcarOne(&(in->discone[i]),&(out->discone[i]),
			coneWidth,coneGuardOffset,
			coneGuardWidth,coneGuardThresh);
     }	  
#endif //not ANITA2
}

float MSRatio(TransientChannelF_t *in,int begwindow,int endwindow){
     float msbeg=0.;
     float msend=0.;
     int i;
     if (begwindow>250) begwindow=250;
     if (endwindow>250) endwindow=250;
     for (i=0; i<begwindow; i++){
	  msbeg+=in->data[i];
     }
     msbeg /= begwindow;
     for (i=in->valid_samples; i>in->valid_samples-endwindow; i--){
	  msend+=in->data[i]*in->data[i];
     }
     msend /= endwindow;
//     return (msend/msbeg);
     return (msbeg/msend);
}

float MeanRatio(TransientChannelF_t *in,int begwindow,int endwindow){
     float msbeg=0.;
     float msend=0.;
     int i;
     if (begwindow>250) begwindow=250;
     if (endwindow>250) endwindow=250;
     for (i=0; i<begwindow; i++){
	  msbeg+=in->data[i];
     }
     msbeg /= begwindow;
     for (i=in->valid_samples; i>in->valid_samples-endwindow; i--){
	  msend+=in->data[i];
     }
     msend /= endwindow;
//     return (msend/msbeg);
     return (msbeg/msend);
}

void ZeroChannel(TransientChannelF_t *in){
     int i;
     for (i=0;i<MAX_NUMBER_SAMPLES;i++){
	  in->data[i]=0.;
     }
}



int MeanCountAll(AnitaInstrumentF_t *in,int thresh, 
		int begwindow,int endwindow){
     int count=0;
     int i;
     int phi,pol;
     float fthresh=(float) thresh*0.01;
     for (phi=0;phi<16; phi++){
	  for (pol=0;pol<2;pol++){
	       if (MeanRatio(&(in->topRing[phi][pol]),
			begwindow, endwindow)>fthresh){
		    count++;
		    if ((MethodMask &0x100)!=0) 
			 ZeroChannel(&in->topRing[phi][pol]);
	       }
	       if (MeanRatio(&(in->botRing[phi][pol]),
			begwindow, endwindow)>fthresh){
		    count++;
		    if ((MethodMask &0x100)!=0) 
			 ZeroChannel(&in->botRing[phi][pol]);
	       }
	  }
     }
#ifdef ANITA2
// for ANITA 2, include nadirs and add to the count
     for (i=0;i<8; i++){
	  for (pol=0;pol<2;pol++){
	       if (MeanRatio(&(in->nadir[i][pol]),
			begwindow, endwindow)>fthresh){
		    count++;
		    if ((MethodMask &0x100)!=0) 
			 ZeroChannel(&in->nadir[i][pol]);
	       }
	  }
     }
#endif	  
     return count; 
}

int RMSCountAll(AnitaInstrumentF_t *in,int thresh, 
		int begwindow,int endwindow){
     int count=0;
     int i;
     int phi,pol;
     float fthresh=(float) thresh*0.01;
     fthresh=fthresh*fthresh; // avoid square roots at the price of one square.
     for (phi=0;phi<16; phi++){
	  for (pol=0;pol<2;pol++){
	       if (MSRatio(&(in->topRing[phi][pol]),
			begwindow, endwindow)>fthresh){
		    count++;
		    if ((MethodMask &0x100)!=0) 
			 ZeroChannel(&in->topRing[phi][pol]);
	       }
	       if (MSRatio(&(in->botRing[phi][pol]),
			begwindow, endwindow)>fthresh){
		    count++;
		    if ((MethodMask &0x100)!=0) 
			 ZeroChannel(&in->botRing[phi][pol]);
	       }
	  }
     }
#ifdef ANITA2
// for ANITA 2, include nadirs and add to the count
     for (i=0;i<8; i++){
	  for (pol=0;pol<2;pol++){
	       if (MSRatio(&(in->nadir[i][pol]),
			begwindow, endwindow)>fthresh){
		    count++;
		    if ((MethodMask &0x100)!=0) 
			 ZeroChannel(&in->nadir[i][pol]);
	       }
	  }
     }
#endif	  
     return count; 
}

int GlobalMajority(AnitaChannelDiscriminator_t *in,
		   LogicChannel_t *horns,
		   LogicChannel_t *cones,
		   int delay){
// this is used to identify too many antennas peaking at once
     int i,j,minvalid,pol,phi,thisvalid,hornmax;
     for (j=0;j<MAX_NUMBER_SAMPLES; j++){
	  horns->data[j]=0;
	  cones->data[j]=0;
     }
     minvalid=MAX_NUMBER_SAMPLES;
     for(pol=0;pol<2;pol++){
	  for (phi=0;phi<16;phi++){
	       thisvalid=
	       (in->topRing[phi][pol].valid_samples<
	       in->botRing[phi][pol].valid_samples)
	       ?in->topRing[phi][pol].valid_samples-delay 
	       :in->botRing[phi][pol].valid_samples-delay;
//fix a bounds problem in the line below by subtracting delay from the bound
	       for (j=0;j<(in->topRing[phi][pol]).valid_samples-delay; j++){
		    horns->data[j] += (in->topRing[phi][pol]).data[j+delay];
	       }
	       for (j=0;j<(in->botRing[phi][pol]).valid_samples; j++){
		    horns->data[j] += (in->botRing[phi][pol]).data[j];
	       }
	       if (minvalid > thisvalid) 
		    minvalid=thisvalid;
	  }
     }

#ifdef ANITA2
// only count the even numbered (i.e. real) nadirs
// for simplicity we don't use a delay here; the difference is small      
     for (i=0; i<8; i++){
	  phi=2*i;
	  for (pol=0; pol<2; pol++){
	       thisvalid= (in->nadir[phi][pol].valid_samples<
			   minvalid)
		    ?in->nadir[phi][pol].valid_samples 
		    :minvalid;
	       for (j=0;j<(in->nadir[phi][pol]).valid_samples; j++){
		    horns->data[j] += (in->nadir[phi][pol]).data[j];
	       }
	       if (minvalid > thisvalid) 
		    minvalid=thisvalid;
	  }
     }

#endif

     horns->valid_samples = minvalid;
     hornmax=0;
     for (i=0; i<horns->valid_samples; i++){
	  if (horns->data[i]>hornmax) hornmax=horns->data[i];
     }

#ifndef ANITA2
     minvalid=MAX_NUMBER_SAMPLES;
     for (i=0;i<4;i++){
	  for (j=0;j<(in->bicone[i]).valid_samples; j++){
	       cones->data[i] += (in->bicone[i]).data[j];
	  }
	  if (minvalid > (in->bicone[i]).valid_samples) 
	       minvalid=(in->bicone[i]).valid_samples;
	  for (j=0;j<(in->discone[i]).valid_samples; j++){
	       cones->data[j] += (in->discone[i]).data[j];
	  }
	  if (minvalid > (in->discone[i]).valid_samples) 
	       minvalid=(in->discone[i]).valid_samples;
     }
     cones->valid_samples = minvalid;
#endif

     return hornmax;
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
//			 if((in->topRing[phi][pol]).data[j]>0) {
//			    printf("Wahyyy\n");
//			 }
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

#ifdef ANITA2
#else //ANITA2
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
#endif
}

void FormSectorMajorityPol(AnitaChannelDiscriminator_t *in,
			AnitaSectorLogic_t *out,
			   int sectorWidth,int pol)
{
     //this is brute force--one can save some adds by doing
     //ringwise sums and thaen taking differences rather than
     //resumming
     int sector_phi,phi,i,j,minvalid;
     //top ring
     for(sector_phi=0;sector_phi<16;sector_phi++){
	  for (j=0;j<MAX_NUMBER_SAMPLES; j++){
	       (out->topRing[sector_phi]).data[j]=0;
	
  }
	  minvalid=MAX_NUMBER_SAMPLES;
//	  for(pol=0;pol<2;pol++){
	  for (i=0;i<sectorWidth;i++){
	       phi=(sector_phi+i)%16;
		    
		   
	       for (j=0;j<(in->topRing[phi][pol]).valid_samples; j++){
//			 if((in->topRing[phi][pol]).data[j]>0) {
//			    printf("Wahyyy\n");
//			 }
		    (out->topRing[sector_phi]).data[j] += (in->topRing[phi][pol]).data[j];
	       }
	       if (minvalid > (in->topRing[phi][pol]).valid_samples) 
		    minvalid=(in->topRing[phi][pol]).valid_samples;
	  }
     (out->topRing[sector_phi]).valid_samples = minvalid;
     }
//     }
     //bottom ring
     for(sector_phi=0;sector_phi<16;sector_phi++){
	  for (j=0;j<MAX_NUMBER_SAMPLES; j++){
	       (out->botRing[sector_phi]).data[j]=0;
	  }
	  minvalid=MAX_NUMBER_SAMPLES;
//	  for(pol=0;pol<2;pol++){
	  for (i=0;i<sectorWidth;i++){
	       phi=(sector_phi+i)%16;
	       for (j=0;j<(in->botRing[phi][pol]).valid_samples; j++){
		    (out->botRing[sector_phi]).data[j] += (in->botRing[phi][pol]).data[j];
	       }
	       if (minvalid > (in->botRing[phi][pol]).valid_samples) 
		    minvalid=(in->botRing[phi][pol]).valid_samples;
	  }
//	  }
	  (out->botRing[sector_phi]).valid_samples = minvalid;
     }
#ifdef ANITA2
//ANITA 2 nadir code goes here
#else //ANITA2

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
     minvalid=MAX_NUMBER_SAMPLES;
     for (j=0; j<MAX_NUMBER_SAMPLES; j++){
	  (out->discone).data[j]=0;
     }
     for (i=0;i<4;i++){
	  for (j=0;j<(in->discone[i]).valid_samples; j++){
	       (out->discone).data[j] += (in->discone[i]).data[j];
	  }
	  if (minvalid > (in->discone[i]).valid_samples) 
	       minvalid=(in->discone[i]).valid_samples;
     }
     (out->discone).valid_samples = minvalid;
#endif //not ANITA2
}

int FormSectorCoincidence(AnitaSectorLogic_t *majority,
			  AnitaCoincidenceLogic_t *coinc,  int delay,
			  int topthresh, int botthresh){
// compare the bottom and the top delay ns later
// if each is greater than their respective threshold,
// add the values and place it in  the sample corresponding to
// the bottom sample index.
// Return the maximum value found in any sector at any time.
     int phi,k,max;
     //delay must be positive, but...
     delay=abs(delay);
     max=0;
     for (phi=0;phi<16;phi++){
	  coinc->sector[phi].valid_samples=
	       (majority->topRing[phi].valid_samples<
		majority->botRing[phi].valid_samples)
	       ?majority->topRing[phi].valid_samples-delay 
	       :majority->botRing[phi].valid_samples-delay;
	  for (k=0;k<coinc->sector[phi].valid_samples;k++){
	       coinc->sector[phi].data[k]=0;
	       if(majority->topRing[phi].data[k+delay]>=topthresh
		  &&
		  majority->botRing[phi].data[k]>=botthresh){
		    coinc->sector[phi].data[k]=
			 majority->topRing[phi].data[k+delay]
			 +majority->botRing[phi].data[k];
		    if (coinc->sector[phi].data[k]>max){
			 max=coinc->sector[phi].data[k];
		    }
	       }
	  }
     }
     return max;
}

void peakTime(TransientChannelF_t *T, Peak_t *peak){
     int i, peakSample;
     //float total;
     //value is sign preserving...
     peakSample=0;
     peak->value=T->data[0];
     for (i=1;i<T->valid_samples; i++){
	  if (fabsf(T->data[i])>fabsf(peak->value)){
	       peak->value=T->data[i];
	       peakSample=i;
	  }
     }
     peak->time=peakSample;
/* things get ugly when there is a zero crossing between points */
     peak->time=(peak->time)*(meters_per_ns/samples_per_ns);
}

void FindPeaks(AnitaInstrumentF_t *theInst, AnitaPeak_t *thePeak){
     int i,j;
     for (i=0;i<16;i++){
	  for (j=0;j<2;j++){
	       peakTime(&theInst->topRing[i][j],
			&thePeak->topRing[i][j]);
	       peakTime(&theInst->botRing[i][j],
			&thePeak->botRing[i][j]);
	  }
     }
#ifdef ANITA2
     for (i=0;i<8;i++){
	  for (j=0;j<2;j++){
	       peakTime(&theInst->nadir[i][j],
			&thePeak->nadir[i][j]);
	  }
     }
#else //ANITA2
     for (i=0;i<4;i++){
	  peakTime(&theInst->bicone[i],&thePeak->bicone[i]);
	  peakTime(&theInst->discone[i],&thePeak->discone[i]);
     }
#endif //not ANITA2
}

int determinePriority(){
     // MethodMask switches
     // 0x1--peak boxcar (pri 1 and 3)
     // 0x2--fft peak cuts
     // 0x4--score 4 (pri 5)
     // 0x8--score 3 (pri 5)
     // 0x10--nonupdating (pri 5)
     // 0x20-- second boxcar with narrower sectors (2 & 4)
     // 0x40--blast remover (pri 8)
     // 0x80--late vs. early RMS counter (pri 7)
     // 0x100--also cut channels involved in 0x80
     // 0x200--cut channels with wide crosscorrelator peaks (not neutrinoish)
     // 0x400--look at events which fail 0x80 for promotion to pri 5,
     //                             with the offending channels cut 
     // 0x800--cut on minimum SNR for peak on boxcar, bound to conethresh
     // 0x1000--boxcar smoothing of crosscorrelator
     int priority,score,score3,score4;
     unwrapAndBaselinePedSubbedEvent(&pedSubBody,&unwrappedBody);
     BuildInstrumentF(&unwrappedBody,&theInstrument);
     FFTNumChannels=0.;
     // the next function has the side effect of counting the bad FFT peaks
     if (MethodMask & 0x1000){
	  HornMatchedFilterAllSmooth(&theInstrument,&theXcorr);
     }else{
	  HornMatchedFilterAll(&theInstrument,&theXcorr);
     }
     priority=6; // default for others not satisfied; thermal goes here we hope
     if (((MethodMask & 0x2) !=0) && (FFTNumChannels>FFTMaxChannels)){
	  // reject this event as narrowband crap
	  priority=9;
     }
     // overall majority thingee to get payload blast
     else if ((MethodMask &0x40)!=0){
	  PeakBoxcarAll(&theXcorr,&theBoxcarNoGuard,
			hornDiscWidth,0,
			0,65535,
			coneDiscWidth,0,
			0,65535);
	  HornMax=GlobalMajority(&theBoxcarNoGuard,&HornCounter,
				 &ConeCounter, delay);
	  if (HornMax>WindowCut) //too many horns peaking simultaneously
	       priority=8;
     }
     // cut on late vs. early RMS to reject blast starting during the record   
     // this can also be given the side effect of taking the channels
     // involved out of the priority 1-4 (boxcar) decision. See RMSCountAll
     // One can force the priority 7 cut to be ignored after this for promotion
     // consideration--see below.
     else if ((MethodMask & 0x80)!=0){
#ifdef ANITA2
// in ANITA 2 we are looking at RMS voltage at baseband already...
	  RMSnum=MeanCountAll(&theXcorr,RMSMax,
			      BeginWindow,EndWindow);
	  if (RMSnum>RMSevents) priority=7;
#else // ANITA2
	  RMSnum=RMSCountAll(&theXcorr,RMSMax,
			     BeginWindow,EndWindow);
	  if (RMSnum>RMSevents) priority=7;
#endif //not ANITA2
     }
// if there are no 'black marks' everything is now priority 6.
// next block gives three choices for making the initial promotion decision.
// to go from priority 6 to priority 5
// score4 is ~SLAC priority 1
// score3 is next group of SLAC priorities
// the nonupdating discriminator thingee is like those but rejects blast, 
//     but the blast rejector above for priority 8 seems to work well enough.
     if (priority==6 || (priority==7 && MethodMask&0x400)) {
          // Ordinary coincidence and scoring a al SLAC
	  if ((MethodMask&0x4)!=0){
	       DiscriminateFChannels(&theXcorr,&theDiscriminator,
				     hornThresh,hornDiscWidth,
				     coneThresh,coneDiscWidth); 
                                    //hornthresh was 400 at SLAC
	       FormSectorMajority(&theDiscriminator,&theMajority,
				  hornSectorWidth);
	       AnalyseSectorLogic(&theMajority,&sectorAna);
	       score=GetSecAnaScore(&sectorAna);
	       score4=score;
	  }
	  else
	  {
	       score4=0;
	  }
	  if ((MethodMask&0x8)!=0){
	       DiscriminateFChannels(&theXcorr,&theDiscriminator,
				     300,hornDiscWidth,
				     coneThresh,coneDiscWidth);
	       FormSectorMajority(&theDiscriminator,&theMajority,
				  hornSectorWidth);
	       AnalyseSectorLogic(&theMajority,&sectorAna);
	       score=GetSecAnaScore(&sectorAna);
	       score3=score;
	  }
	  else{
	       score3=0;
	  }
          // nonupdating discriminators and majorities
	  if ((MethodMask & 0x10)!=0){
	       DiscriminateFChannels_noup(&theXcorr,&theNonupdating,
					  hornThresh,hornDiscWidth,
					  coneThresh,coneDiscWidth,
					  holdoff);
	       FormSectorMajority(&theNonupdating,&theMajorityNoUp,
				  hornSectorWidth-1);
	       FormSectorMajorityPol(&theNonupdating,&theHorizontal,
				     hornSectorWidth-1,0);
	       FormSectorMajorityPol(&theNonupdating,&theVertical,
				     hornSectorWidth-1,1);
	       MaxAll=FormSectorCoincidence(&theMajorityNoUp,
					    &theCoincidenceAll,
					    delay,2*(hornSectorWidth-1)-2,
					    2*(hornSectorWidth-2)-2);
	       MaxH=FormSectorCoincidence(&theHorizontal,
					  &theCoincidenceH,
					  delay,hornSectorWidth-2,
					  hornSectorWidth-2);
	       MaxV=FormSectorCoincidence(&theVertical,&theCoincidenceV,
					  delay,hornSectorWidth-2,
					  hornSectorWidth-2);
	  }
	  else{
	       MaxAll=0; MaxH=0; MaxV=0;
	  }
	  if(score4>=600 || score3>=600 || 
	     MaxH>=2*(hornSectorWidth-1)-2 || MaxV>=2*(hornSectorWidth-1)-2) 
	       priority=5;
     }

//
// at this point everything is 5-9
// only priority 5 gets further scrutiny for promotion
//
//
     //xcorr peak boxcar method
     if ((MethodMask &0x21)!=0 && priority==5){
	  PeakBoxcarAll(&theXcorr,&theBoxcar,
			hornDiscWidth,hornGuardOffset,
			hornGuardWidth,hornGuardThresh,
			coneDiscWidth,coneGuardOffset,
			coneGuardWidth,coneGuardThresh);
     }
     if ((MethodMask & 0x1)!=0 && priority==5){
//			      FormSectorMajority(&theBoxcar,&theMajorityBoxcar,
//						 hornSectorWidth);
	  FormSectorMajorityPol(&theBoxcar,&theMajorityBoxcarH,
				hornSectorWidth,0);
	  FormSectorMajorityPol(&theBoxcar,&theMajorityBoxcarV,
				hornSectorWidth,1);
//	  MaxBoxAll=FormSectorCoincidence(&theMajorityBoxcar,
//			   &theCoincidenceBoxcarAll,
//			   delay,2*hornSectorWidth-1,
//			   2*hornSectorWidth-1);
	  MaxBoxH=FormSectorCoincidence(&theMajorityBoxcarH,
					&theCoincidenceBoxcarH,
					delay,hornSectorWidth-1,
					hornSectorWidth-1);
	  MaxBoxV=FormSectorCoincidence(&theMajorityBoxcarV,
					&theCoincidenceBoxcarV,
					delay,hornSectorWidth-1,
					hornSectorWidth-1);
     }
     else{
	  /* MaxBoxAll=0;*/ MaxBoxH=0; MaxBoxV=0;
     }
     //xcorr peak boxcar method with narrowed sector
     if ((MethodMask & 0x20)!=0  && priority==5){
//			FormSectorMajority(&theBoxcar,&theMajorityBoxcar2,
//					   hornSectorWidth-1);
	  FormSectorMajorityPol(&theBoxcar,&theMajorityBoxcarH2,
				hornSectorWidth-1,0);
	  FormSectorMajorityPol(&theBoxcar,&theMajorityBoxcarV2,
				hornSectorWidth-1,1);
//			MaxBoxAll2=FormSectorCoincidence(&theMajorityBoxcar2,
//							&theCoincidenceBoxcarAll2,
//							8,2*hornSectorWidth-3,
//							2*hornSectorWidth-3);
	  MaxBoxH2=FormSectorCoincidence(&theMajorityBoxcarH2,
					 &theCoincidenceBoxcarH2,
					 delay,hornSectorWidth-2,
					 hornSectorWidth-2);
	  MaxBoxV2=FormSectorCoincidence(&theMajorityBoxcarV2,
					 &theCoincidenceBoxcarV2,
					 delay,hornSectorWidth-2,
					 hornSectorWidth-2);
     }
     else{
	  /*MaxBoxAll2=0;*/ MaxBoxH2=0; MaxBoxV2=0;
     }

     if (priority == 5){ //consider promotion
	  if (MaxBoxH>=2*hornSectorWidth || MaxBoxV>=2*hornSectorWidth) 
	       //3 for 3 in both rings
	       priority=1;
	  else if (MaxBoxH2>=2*(hornSectorWidth-1) || MaxBoxV2>=2*(hornSectorWidth-1)) 
	       // 2 for 2 in both rings
	       priority=2;
	  else if (/*MaxBoxAll>=4*hornSectorWidth-4 ||*/
	       MaxBoxH>=2*hornSectorWidth-1 || 
	       MaxBoxV>=2*hornSectorWidth-1 ) 
	       priority=3; 
	  // 2/3  in eithr pol in both rings
	  else if (MaxBoxH2>=2*(hornSectorWidth-1)-1 || 
		   MaxBoxV2>=2*(hornSectorWidth-1)-1) 
	       // 2 for 2 in one ring and 1/2 in other
	       priority=4;
     }
     return priority;
}
