/*!
  @file anitaTimingCalib.c
  @date 30 Oct 2014
  @author Ben Strutt

  @brief
  Set of functions to sanitize the steps required to calibrate raw anita data in-line, before sending to the GPU.
  Will it be fast enough?

  @detailed
  Two things need to be done, I'll try them on the CPU first:

  1.) Cross correlating the clocks to get the relative surf-to-surf clock jitter. I will use fftw3.
  2.) Interpolating the waveforms to give evenly sampled time domains. 
      I will use gsl interpolation for the highly upsamped clock.
      I will use linear interpolation for the normal channels, where I am just evenly resampling.

  These functions should do it all, hiding the annoying APIs of the gsl and fftw libraries behind
  some friendly functions which handle ANITA structs. (It might be quick enough since it's the 
  combinatorics of the cross-correlatsions of interferometry that makes things slow).
 */


#include "anitaTimingCalib.h"

/*--------------------------------------------------------------------------------------------------------------*/
/* Evil globals */
#define NUM_SAMP 256
#define NUM_SURF 12
#define NUM_RCO 2
#define NUM_WRAP_POSSIBILITIES 2
#define upsampleFactor 16
#define numUpsampledClockSamples 256*upsampleFactor

gsl_interp_accel *acc;
const gsl_interp_type *akimaSpline;
gsl_spline *spline;

fftw_plan clockPlanForward;
fftw_plan clockPlanReverse;

//const int numSurfs = 10;  /* Change this 10 to ACTIVE_SURFS for real thing! */
#define numSurfs ACTIVE_SURFS
double justBinByBin[numSurfs][LABRADORS_PER_SURF][2][MAX_NUMBER_SAMPLES];
double epsilonFromBenS[numSurfs][LABRADORS_PER_SURF][2] = {{{1}}};
double clockCrossCorr[numSurfs][LABRADORS_PER_SURF] = {{-9999}};
int rcoLatchStart[numSurfs][LABRADORS_PER_SURF];
int rcoLatchEnd[numSurfs][LABRADORS_PER_SURF];

/* double volts2[numSurfs][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES]; */
double volts[numSurfs][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
int nSamps[numSurfs][CHANNELS_PER_SURF];

double times[numSurfs][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
/* double times2[numSurfs][LABRADORS_PER_SURF][NUM_RCO][NUM_WRAP_POSSIBILITIES][MAX_NUMBER_SAMPLES][MAX_NUMBER_SAMPLES]; */
int numExtras[numSurfs][LABRADORS_PER_SURF][NUM_RCO][NUM_WRAP_POSSIBILITIES][MAX_NUMBER_SAMPLES] = {{{{{0}}}}};

double clockJitters[numSurfs];
int nClockSamps[numSurfs];

int whbs[numSurfs];
int earliestSampleInds[numSurfs];
int latestSampleInds[numSurfs];
int rcos[numSurfs];
int labChips[numSurfs];

double relativeCableDelays[numSurfs][CHANNELS_PER_SURF];

double* clockTimeDomain;
fftw_complex* clockFreqDomain;
fftw_complex* clockFreqHolder;

/*--------------------------------------------------------------------------------------------------------------*/
/* Functions - initialization and clean up. */
void prepareTimingCalibThings(){
  acc = gsl_interp_accel_alloc();
  akimaSpline = gsl_interp_akima;

  readInCalibratedDeltaTs("justBinByBin.dat");
  /* readInClockCrossCorr("crossCorrClocksPeakPhi.dat"); */
  readInEpsilons("epsilonFromBenS.dat");
  readInRcoLatchDelay("rcoLatchDelay.dat");
  readInRelativeCableDelay("relativeCableDelays.dat");

  clockTimeDomain = fftw_malloc(sizeof(double)*numUpsampledClockSamples);
  clockFreqDomain = fftw_malloc(sizeof(fftw_complex)*numUpsampledClockSamples);
  clockFreqHolder = fftw_malloc(sizeof(fftw_complex)*numUpsampledClockSamples);
  clockPlanForward = fftw_plan_dft_r2c_1d(numUpsampledClockSamples, clockTimeDomain, 
					  clockFreqDomain, FFTW_MEASURE);
  clockPlanReverse = fftw_plan_dft_c2r_1d(numUpsampledClockSamples, clockFreqDomain, 
					  clockTimeDomain, FFTW_MEASURE);

  /* preCalculateTimeArrays(); */
}

void tidyUpTimingCalibThings(){
  gsl_interp_accel_free (acc);

  fftw_destroy_plan(clockPlanForward);
  fftw_destroy_plan(clockPlanReverse);
  fftw_free(clockTimeDomain);
  fftw_free(clockFreqDomain);
  fftw_free(clockFreqHolder);
}









/*--------------------------------------------------------------------------------------------------------------*/
/* Interpolation functions. GSL for fancy things, linear for speed... */
/*--------------------------------------------------------------------------------------------------------------*/
double* interpolateWaveform(int nRaw, double* rawWave, double* unevenTimes, 
			    int nInterp, double t0interp, double dtNsInterp, int clockNum){

  spline = gsl_spline_alloc (akimaSpline, nRaw);
  gsl_spline_init (spline, unevenTimes, rawWave, nRaw);
  
  double* interpWave = malloc(nInterp*sizeof(double));

  float time = t0interp;
  int sampInd = 0;

  /* int rawInd = 0; */

  if(clockNum>=0){
    nClockSamps[clockNum] = 0;
  }


  for(sampInd=0; sampInd<nInterp; sampInd++){

    /* If we are inside known time band then interpolate. */
    if(time >= unevenTimes[0] && time <= unevenTimes[nRaw-1]){
      interpWave[sampInd] = gsl_spline_eval(spline, time, acc);
      if( clockNum>=0){
	nClockSamps[clockNum]++;
      }
    }

    /* Zero waveform outside set of known unevenTimes. */
    else{
      interpWave[sampInd] = 0;
    }

    /* if(clockNum<0){ */
    /*   printf("t = %lf, v = %lf\n", time, interpWave[sampInd]); */
    /* } */


    time += dtNsInterp;
  }

  gsl_spline_free(spline);
  gsl_interp_accel_reset(acc);

  return interpWave;

}



double* linearlyInterpolateWaveform(int nRaw, double* rawWave, double* unevenTimes, 
				    int nInterp, double t0interp, double dtNsInterp){

  double* interpWave = malloc(nInterp*sizeof(double));

  float time = t0interp;
  int voltsSamp = 0;
  int t1Ind = 0;
  int t2Ind = 0;
  
  for(voltsSamp=0; voltsSamp<nInterp; voltsSamp++){

    while(unevenTimes[t2Ind] <= time && t2Ind < nRaw-1){
      t2Ind++;
    }
    t1Ind = t2Ind;
    while(unevenTimes[t1Ind] >= time && t1Ind > 0){
      t1Ind--;
    }

    /* If we are inside known time band, then interpolate. */
    if(time >= unevenTimes[0] && time <= unevenTimes[nRaw-1]){
      interpWave[voltsSamp] = (time - unevenTimes[t1Ind])*(rawWave[t2Ind] - rawWave[t1Ind])/(unevenTimes[t2Ind] - unevenTimes[t1Ind]) + rawWave[t1Ind];
    }

    /* Zero waveform outside set of known unevenTimes. */
    else{
      interpWave[voltsSamp] = 0;
    }

    time += dtNsInterp;
  }

  return interpWave;
}







/* Jitter in TURF hold being implemented across surfs is fixed by cross correlating clocks */
double findClockJitterCorrection(int numSamples, double* clock1, double* clock2, double deltaT_ns, int surf, int lab){

  if(surf==0){
    /* by definition */
    return 0;
  }

  const double clockFreq_MHz = 33;
  double clockPeriod_ns = 1./(clockFreq_MHz*1e-3);
  int clockPeriod_samples = clockPeriod_ns/deltaT_ns;

  //  const double lowPassFreq = 150;
  //  const double lowPassFreq = 400;
  const double lowPassFreq = clockFreq_MHz*1.5;
   /* const double lowPassFreq = 1e11; */

  int numFreqs = numSamples/2+1;
  double deltaF_MHz = 1e3/(deltaT_ns*numSamples);
  /* printf("clock1 in frequency domain...\n"); */
  /* for(int samp=0; samp<numSamples; samp++){ */
  /*   printf("samp %d = %lf\n", samp, clock1[samp]); */
  /* } */

  /* memcpy(clockTimeDomain+numSamples/2, clock1, sizeof(double)*numSamples/2); */
  /* memcpy(clockTimeDomain, clock1+numSamples/2, sizeof(double)*numSamples/2); */
  memcpy(clockTimeDomain, clock1, sizeof(double)*numSamples);
  fftw_execute(clockPlanForward);
  memcpy(clockFreqHolder, clockFreqDomain, sizeof(fftw_complex)*(numSamples/2+1));


  /* memcpy(clockTimeDomain+numSamples/2, clock2, sizeof(double)*numSamples/2); */
  /* memcpy(clockTimeDomain, clock2+numSamples/2, sizeof(double)*numSamples/2); */
  memcpy(clockTimeDomain, clock2, sizeof(double)*numSamples);
  fftw_execute(clockPlanForward);

  /* Now cross correlate */
  int freqInd=0;
  double freq=0;
  double normFactor = numSamples*numSamples;

  for(freqInd=0; freqInd<numFreqs; freqInd++){

    /* Need temp variable to hold values during multi-step calculation */
    fftw_complex X = {0, 0};

    if(freq < lowPassFreq){
      /* I am hungover so I need to write out the algebra. Don't judge me. */
      /* (a-ib)(c+id) = ac + iad - ibc + db = (ac + bd) + i(ad - bc)  */

      X[0] = (clockFreqDomain[freqInd][0]*clockFreqHolder[freqInd][0] + clockFreqDomain[freqInd][1]*clockFreqHolder[freqInd][1]);
    
      X[1] = (-clockFreqDomain[freqInd][0]*clockFreqHolder[freqInd][1] + clockFreqDomain[freqInd][1]*clockFreqHolder[freqInd][0]);
    }

    clockFreqDomain[freqInd][0] = X[0]/normFactor;
    clockFreqDomain[freqInd][1] = X[1]/normFactor;

    freq += deltaF_MHz;
  }
  
  fftw_execute(clockPlanReverse);
  /*So now cross correlations should be in the clockTimeDomain array*/
  
  /* Let's give them their own array */
  double* crossCorrelatedClocks = malloc(sizeof(double)*numSamples);
  memcpy(crossCorrelatedClocks, clockTimeDomain, sizeof(double)*numSamples);

  int maxIndGr = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, 0, clockPeriod_samples/2);
  int maxIndLe = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, numSamples-clockPeriod_samples/2, numSamples);
  int maxInd = crossCorrelatedClocks[maxIndGr] > crossCorrelatedClocks[maxIndLe] ? maxIndGr : maxIndLe;
  double clockCor = maxInd > numSamples/2 ? (maxInd-numSamples)*deltaT_ns : deltaT_ns*maxInd;

  /* char cccname[1024]; */
  /* sprintf(cccname, "/tmp/ccc%d.txt", surf); */
  /* FILE* ccc = fopen(cccname, "w"); */
  /* int cInd=0; */
  /* fprintf(ccc, "%d %lf #", maxInd, clockCor); */
  /* for(cInd=0; cInd<numSamples; cInd++){ */
  /*   fprintf(ccc, "%lf %lf\n", cInd*deltaT_ns, crossCorrelatedClocks[cInd]); */
  /* } */
  /* fclose(ccc);   */


  /* int maxInd = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, 0, numSamples); */
  /* double clockCor = maxInd > numSamples/2 ? (maxInd-numSamples)*deltaT_ns : deltaT_ns*maxInd; */
  /* int badGuessFlag=0; */

  /* if(fabs(clockCor-clockCrossCorr[surf][lab])>clockPeriod_ns/2) { */
  /*   /\* Failed: away from the mean by more than half a clock cycle... try again...*\/ */

  /*   if(clockCor>clockCrossCorr[surf][lab]) { */
  /*     /\* above mean value, restrict to searching "below" the maxInd for the clockCor... *\/ */

  /*     badGuessFlag = 1; */
  /*     if(clockCor>=0 && maxInd>2*upsampleFactor){ */
  /* 	/\* Must search below clockCor value, but clockCor value is less than zero*\/ */

  /* 	int i = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, numSamples/2, numSamples); */
  /* 	int j = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, 0, maxInd-2*upsampleFactor); */
	
  /* 	double c1 = crossCorrelatedClocks[i]; */
  /* 	double c2 = crossCorrelatedClocks[j]; */
  /* 	if(c1 > c2){ */
  /* 	  maxInd = i; */
  /* 	} */
  /* 	else{ */
  /* 	  maxInd = j; */
  /* 	} */
  /*     } */
  /*     else{/\* Must search below clockCor value and clockCor is greater than (or equal to) zero*\/ */
  /* 	maxInd = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, numSamples/2+1, numSamples); */
  /*     } */
  /*   } */
  /*   else{/\* below mean value, restrict to searching "above" this value... *\/ */
  /*     badGuessFlag = -1; */
  /*     if(clockCor<0 && numSamples-maxInd>2*upsampleFactor){ */
  /* 	/\* Must search below clockCor and clockCor value is less than zero*\/ */
  /* 	int i = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, maxInd+2*upsampleFactor, numSamples); */
  /* 	int j = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, 0, numSamples/2); */

  /* 	double c1 = crossCorrelatedClocks[i]; */
  /* 	double c2 = crossCorrelatedClocks[j]; */
  /* 	if(c1 > c2){ */
  /* 	  maxInd = i; */
  /* 	} */
  /* 	else{ */
  /* 	  maxInd = j; */
  /* 	} */
  /*     } */
  /*     else{ */
  /* 	maxInd = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, maxInd+2*upsampleFactor, numSamples/2); */
  /*     } */
  /*   } */
  /*   clockCor = maxInd > numSamples/2 ? (maxInd-numSamples)*deltaT_ns : deltaT_ns*maxInd; */
  /* } */

  free(crossCorrelatedClocks);

  return clockCor;

}



/* Probably pointless */
void normalize(int numSamples, double* data){
  int samp=0;
  double sum=0;
  double square=0;
  for(samp=0; samp<numSamples; samp++){
    sum += data[samp];
    square += data[samp]*data[samp];
  }
  double mean = sum/numSamples;
  double rms = sqrt(square/numSamples - mean*mean);

  for(samp=0; samp<numSamples; samp++){
    data[samp] = (data[samp] - mean)/rms;
  }
}

int findIndexOfMaximumWithinLimits(double* array, int startInd, int stopInd){
  /* 
     Returns -1 on failure.
     Wraps negative indices and allows choice of direction to iterate.
     Always returns "true" index values, even if wrapping was used.
     Search indices are inclusive of both startInd and stopInd.
  */
  int i=0;
  int maxIndex = -1;
  double max=-99999999;

  for(i=startInd; i<stopInd; i++){
    if(array[i] > max){
      max = array[i];
      maxIndex = i;
    }
  }
  return maxIndex;
}


/* void justUnwrapVolts(PedSubbedEventBody_t pedSubBody){ */

/*   /\* printf("Unwrapping!\n"); *\/ */

/*   int surf=0; */
/*   for(surf=0;surf<numSurfs;surf++) { */
/*     int chan=0; */
/*     int lab = labChips[surf]; */
/*     int rco = rcos[surf]; */
/*     int earliestSampleInd = earliestSampleInds[surf]; */
/*     int earliestSample = earliestSampleInd; */
/*     int latestSample = latestSampleInds[surf]; */
/*     int whb = whbs[surf];//latestSample < earliestSample ? 0 : 1; */



/*     for(chan=0;chan<CHANNELS_PER_SURF;chan++) { */
/*       int chanIndex=surf*CHANNELS_PER_SURF + chan; */

/*       int ant = abs(surfToAntMap[surf][chan]); */
/*       int orientFactor = ant < 16 ? -1 : 1; */

/*       if(earliestSample==0) */
/* 	earliestSample++; */

/*       if(latestSample==0) */
/* 	latestSample=259; */

/*       int samp=0; */
/*       //Now do the unwrapping */
/*       int index=0; */
/*       if(latestSample<earliestSample) { */
/* 	//We have two RCOs */
/* 	if(earliestSample<256) { */
/* 	  //Lets do the first segment up	 */
/* 	  for(samp=earliestSample;samp<256;samp++) { */
/* 	    volts2[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[samp]*orientFactor; */
/* 	    index++; */
/* 	  } */
/* 	} */


/* 	if(latestSample>=1) { */
/* 	  /\* We are going to ignore sample zero for now *\/ */
/* 	  int nextExtra = 256; */
/* 	  int extraCount = numExtras[surf][lab][rco][whb][earliestSampleInd]; */

/* 	  for(samp=1;samp<=latestSample;samp++) { */
/* 	    if(extraCount>0 && samp==1){ */
/* 	      /\* if(nextExtra<260 && samp==1) { *\/ */
/* 	      /\*   if(extraTime<time-0.22) { *\/ */
/* 	      /\* printf("surf = %d, chan = %d, index = %d, samp = %d, numExtras = %d\n", surf, chan, index, samp, numExtras[surf][chan][lab][rco][whb][earliestSampleInd]); *\/ */
/* 	      volts2[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[nextExtra]*orientFactor;//rawArray[surf][chan][nextExtra]; */
/* 	      nextExtra++; */
/* 	      index++;	  */
/*    	      samp--; */
/* 	      extraCount--; */
/* 	      continue; */
/* 	      /\*   } *\/ */
/* 	      /\* } *\/ */
/* 	    } */
/* 	    volts2[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[samp]*orientFactor; */
/* 	    index++; */
/* 	  } */
/* 	} */
/*       } */
/*       else { */
/* 	/\* Only one rco *\/ */
/* 	for(samp=earliestSample;samp<=latestSample;samp++) { */
/* 	  volts2[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[samp]*orientFactor; */
/* 	  index++; */
/* 	} */
/*       } */
/*       nSamps[surf][chan]=index; */
/*     } */


/*   } */
/* } */



int getLabChip(PedSubbedEventBody_t pedSubBody, int chanInd){

  return pedSubBody.channel[chanInd].header.chipIdFlag & 0x3;

}


int getRco(PedSubbedEventBody_t pedSubBody, int chanInd){

  return (pedSubBody.channel[chanInd].header.chipIdFlag & 0x4) >> 2;

}

int getEarliestSample(PedSubbedEventBody_t pedSubBody, int chanInd){ 
  int lastHitBus = getLastHitBus(pedSubBody, chanInd);
  int firstHitbus = getFirstHitBus(pedSubBody, chanInd);
  int wrappedHitBus = getWrappedHitBus(pedSubBody, chanInd);
  int earliestSample = 0;
  if(!wrappedHitBus) {
    earliestSample = lastHitBus + 1;
  }
  else {
    earliestSample = firstHitbus + 1;
  }
  if(earliestSample == 0) earliestSample = 1;
  if(earliestSample < 260) return earliestSample;
  return 1;
}

int getLatestSample(PedSubbedEventBody_t pedSubBody, int chanInd){ 
  int lastHitBus = getLastHitBus(pedSubBody, chanInd);
  int firstHitbus = getFirstHitBus(pedSubBody, chanInd);
  int wrappedHitBus = getWrappedHitBus(pedSubBody, chanInd);
  int latestSample = 259;
  if(!wrappedHitBus) {
    latestSample = firstHitbus - 1;
  }
  else {
    latestSample=lastHitBus - 1;
  }
  if(latestSample > 0) return latestSample;
  return 259;
}

int getLastHitBus(PedSubbedEventBody_t pedSubBody, int chanInd){ 
  int lastHitBus = pedSubBody.channel[chanInd].header.lastHitbus;
  int firstHitBus = getFirstHitBus(pedSubBody, chanInd);
  if(lastHitBus < firstHitBus){
    lastHitBus += 256;
  }
  return lastHitBus;
}
int getFirstHitBus(PedSubbedEventBody_t pedSubBody, int chanInd){ 
  return pedSubBody.channel[chanInd].header.firstHitbus;
}
int getWrappedHitBus(PedSubbedEventBody_t pedSubBody, int chanInd){ 
  return (pedSubBody.channel[chanInd].header.chipIdFlag&0x8)>>3;
}




void processEventAG(PedSubbedEventBody_t pedSubBody){

  //  unsigned int fC3poNum = theHeader.turfio.c3poNum;
  //  double clockPeriod = 1./0.033;

  double fEpsilonTempScale= 1;

  /* if(fC3poNum) { */
  /*   clockPeriod=1e9/fC3poNum; */
  /* } */
  //  double tempFactor=1; //eventPtr->getTempCorrectionFactor();
  //  double tempFactor = fTempFactorGuess[entry];
  double tempFactor = 1; //fTempFactorGuess[entry];
  int surf=0;
  for(surf=0;surf<numSurfs;surf++) {
    int chan=0;
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {

      int chanIndex=surf*CHANNELS_PER_SURF + chan;
      
      int ant = abs(surfToAntMap[surf][chan]);
#ifdef CALIBRATION
      int orientFactor = 1; /* Injecting directly, don't care that some antennas are flipped.*/
#else
      int orientFactor = ant < 16 && chan!=8 ? -1 : 1; /* Top row of antennas are flipped, but don't flip the clock! */
#endif
      /* printf("ant %d, surf %d, chan %d, orientFactor %d\n", ant, surf, chan, orientFactor); */
      int labChip = labChips[surf];
      int rco = rcos[surf];
      int earliestSample = earliestSampleInds[surf];
      int latestSample = latestSampleInds[surf];

      if(earliestSample==0)
	earliestSample++;

      if(latestSample==0)
	latestSample=259;

      int samp=0;
      //Now do the unwrapping
      int index=0;
      double time=0;
      if(latestSample<earliestSample) {
	//We have two RCOs
	int nextExtra=256;
	double extraTime=0;
	if(earliestSample<256) {
	  //Lets do the first segment up
	  for(samp=earliestSample;samp<256;samp++) {
	    int binRco=1-rco;
	    volts[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[samp]*orientFactor;
	    times[surf][chan][index]=time;
	    if(samp==255) {
	      extraTime=time+(justBinByBin[surf][labChip][binRco][samp])*tempFactor;
	    }
	    else {
	      time+=(justBinByBin[surf][labChip][binRco][samp])*tempFactor;
	    }
	    index++;
	  }
	  //time+=epsilonFromBenS[surf][labChip][rco]*tempFactor*fEpsilonTempScale;
	  //time+=epsilonFromBenS[surf][labChip][1-rco]*tempFactor*fEpsilonTempScale;
	  time+=epsilonFromBenS[surf][labChip][rco]*tempFactor*fEpsilonTempScale;
	}
	else {
	  nextExtra=260;
	  extraTime=0;
	}
	
	
	if(latestSample>=1) {
	  /* We are going to ignore sample zero for now */
	  time+=(justBinByBin[surf][labChip][rco][0])*tempFactor;
	  for(samp=1;samp<=latestSample;samp++) {
	    int binRco=rco;
	    if(nextExtra<260 && samp==1) {
	      if(extraTime<time-0.22) {
	      binRco=1-rco;
	      volts[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[nextExtra]*orientFactor;
	      times[surf][chan][index]=extraTime;
	      if(nextExtra<259) {
		extraTime+=(justBinByBin[surf][labChip][binRco][nextExtra])*tempFactor;
	      }
	      nextExtra++;
	      index++;
   	      samp--;
	      continue;
	      }
	    }
	    volts[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[samp];
	    times[surf][chan][index]=time;
	    if(samp<259) {
	      time+=(justBinByBin[surf][labChip][binRco][samp])*tempFactor;
	    }
	    index++;
	  }
	}
      }
      else {
	/* Only one rco */
	time=0;
	for(samp=earliestSample;samp<=latestSample;samp++) {
	  int binRco=rco;
	  volts[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[samp]*orientFactor;
	  times[surf][chan][index]=time;
	  if(samp<259) {
	    time+=(justBinByBin[surf][labChip][binRco][samp])*tempFactor;
	  }
	  index++;
	}
      }
      nSamps[surf][chan]=index;
    }
    /* Okay now add Stephen's check to make sure that all */
    /*    the channels on the SURF have the same number of points. */
    for(chan=0;chan<8;chan++) {
      if(nSamps[surf][chan]<nSamps[surf][8]) {
    	nSamps[surf][chan]=nSamps[surf][8];
    	int samp=0;
    	for(samp=0;samp<nSamps[surf][8];samp++) {
    	  times[surf][chan][samp]=times[surf][8][samp];
    	}
      }

      if(nSamps[surf][chan]>nSamps[surf][8]) {
    	nSamps[surf][chan]=nSamps[surf][8];
    	int samp=0;
    	for(samp=0;samp<nSamps[surf][8];samp++) {
    	  times[surf][chan][samp]=times[surf][8][samp];
    	}
      }
    }
  }
}




void doTimingCalibration(int entry, AnitaEventHeader_t theHeader,
			 PedSubbedEventBody_t pedSubBody,
			 double* finalVolts[]){

  const double deltaT = 1./2.6;

  int surf=0;
  for(surf=0; surf<numSurfs; surf++){
    int chanIndex = CHANNELS_PER_SURF*surf + 8;
    earliestSampleInds[surf] = getEarliestSample(pedSubBody, chanIndex);
    latestSampleInds[surf] = getLatestSample(pedSubBody, chanIndex);
    whbs[surf] = latestSampleInds[surf] < earliestSampleInds[surf] ? 0 : 1;
    //    rcos[surf] = 1 - getRco(pedSubBody, chanIndex);
    rcos[surf] = getRco(pedSubBody, chanIndex);
    int fhb = getFirstHitBus(pedSubBody, chanIndex);
    labChips[surf] = getLabChip(pedSubBody, chanIndex);

    /* Fix rco latch delay */
    if(fhb >= rcoLatchStart[surf][labChips[surf]] && fhb <= rcoLatchEnd[surf][labChips[surf]] && !(earliestSampleInds[surf]<latestSampleInds[surf])){
      rcos[surf] = 1 - rcos[surf];
    }
  }
  /* printf("ABout to unwrap!\n"); */
  /* justUnwrapVolts(pedSubBody); */
  processEventAG(pedSubBody);


  /* Upsample clocks */
  double* interpClocks[numSurfs];


  for(surf=0; surf<numSurfs; surf++){
    int lab = labChips[surf];
    int rco = rcos[surf];
    int earliestSample = earliestSampleInds[surf];
    int whb = whbs[surf];

    interpClocks[surf] = interpolateWaveform(nSamps[surf][8],
					     /* volts2[surf][8], */
					     /* times2[surf][lab][rco][whb][earliestSample], */
					     volts[surf][8],
					     times[surf][8],
					     numUpsampledClockSamples, 
					     /* times2[surf][lab][rco][whb][earliestSample][0],  */
					     times[surf][8][0],
					     deltaT/upsampleFactor, surf);
    /* int samp=0; */

    /* double derMax = -1000; */
    /* for(samp=0; samp<numUpsampledClockSamples-1; samp++){ */
    /*   /\* Hack for upsampled edge *\/ */
    /*   interpClocks[surf][samp] = interpClocks[surf][samp+1] == 0 ? 0 : interpClocks[surf][samp]; */
      
    /*   /\* Der *\/ */
    /*   interpClocks[surf][samp] = interpClocks[surf][samp+1] - interpClocks[surf][samp]; */
    /*   interpClocks[surf][samp] = interpClocks[surf][samp] > 0 ? interpClocks[surf][samp] : 0; */
    /*   derMax = derMax > interpClocks[surf][samp] ? derMax : interpClocks[surf][samp]; */
    /* } */
    /* interpClocks[surf][numUpsampledClockSamples-1] = 0; */
    /* for(samp=0; samp<numUpsampledClockSamples; samp++){ */
    /*   interpClocks[surf][samp] = interpClocks[surf][samp] < derMax/2 ? 0 : interpClocks[surf][samp]; */
    /* }     */
      /* interpClocks[surf][samp] = samp > numUpsampledClockSamples / 2 ? 0 : interpClocks[surf][samp]; */

    /* normalize(256*upsampleFactor, interpClocks[surf]); */
  }

  /* Extract clock jitter from interpolated clocks relative to clock 0. */
  double largestClockJitter=0;
  clockJitters[0] = 0;
  for(surf=1; surf<numSurfs; surf++){
    clockJitters[surf] = findClockJitterCorrection(numUpsampledClockSamples, interpClocks[0],
    						   interpClocks[surf], deltaT/upsampleFactor,
    						   surf, labChips[surf]);
    largestClockJitter = clockJitters[surf]>largestClockJitter ? clockJitters[surf] : largestClockJitter;
  }

  double startTime = deltaT*(double)nearbyint((int)(-largestClockJitter/deltaT));


  for(surf=0; surf<numSurfs; surf++){
    /* int lab = labChips[surf]; */
    /* int rco = rcos[surf]; */
    /* int earliestSample = earliestSampleInds[surf]; */
    /* int whb = whbs[surf];//latestSample < earliestSample ? 0 : 1; */

    int chan=0;
    for(chan=0; chan<CHANNELS_PER_SURF-1; chan++){

      double newTimes[260] = {0};
      int samp = 0;
      for(samp=0; samp<nSamps[surf][chan]; samp++){
	//newTimes[samp] = times2[surf][lab][rco][whb][earliestSample][samp] + relativeCableDelays[surf][chan];
	newTimes[samp] = times[surf][chan][samp] + relativeCableDelays[surf][chan];
      	newTimes[samp] -= clockJitters[surf];

	/* if(samp==0){ */
	/*   printf("surf %d, chan %d, ant %d, clockJitter = %lf, relativeCableDelay = %lf, startTime = %lf, newTimes[0] = %lf\n", surf, chan, surfToAntMap[surf][chan], clockJitters[surf], relativeCableDelays[surf][chan], startTime, newTimes[0]); */
	/* } */
      }

      //finalVolts[surf*CHANNELS_PER_SURF + chan] = linearlyInterpolateWaveform(nSamps[surf][chan],
      finalVolts[surf*CHANNELS_PER_SURF + chan] = interpolateWaveform(nSamps[surf][chan],
								      //volts2[surf][chan],
								      volts[surf][chan],
								      newTimes,
								      //times[surf][chan],
								      256, 
								      startTime,
								      deltaT, -1);
    }
  }

  /* Tidy up. */
  for(surf=0; surf<numSurfs; surf++){
    free(interpClocks[surf]);
  }


}
















/* void preCalculateTimeArrays(){//PedSubbedEventBody_t pedSubBody){ */
/*   /\*  */
/*      OK, since the number of samples can be obtained from the 2 hitbus locations. */
/*      I have to add up the times 108 times per event!? */
/*      There's only MAX_NUMBER_SAMPLES * NUM_RCO * LABRADORS_PER_SURF * ACTIVE_SURFS  */
/*      possible ways to do it... */
/*      So let's get the number of samples and look up time array from first sample.      */
/*   *\/ */

/*   double fEpsilonTempScale= 1; */

/*   /\* if(fC3poNum) { *\/ */
/*   /\*   clockPeriod=1e9/fC3poNum; *\/ */
/*   /\* } *\/ */
/*   //  double tempFactor=1; //eventPtr->getTempCorrectionFactor(); */
/*   //  double tempFactor = fTempFactorGuess[entry]; */
/*   double tempFactor = 1; //fTempFactorGuess[entry]; */

/*   int earliestSampleInd=0; */
/*   for(earliestSampleInd=0; earliestSampleInd<MAX_NUMBER_SAMPLES; earliestSampleInd++){ */
/*     int earliestSample = earliestSampleInd; */
/*     int whb=0; */
/*     for(whb=0; whb<NUM_WRAP_POSSIBILITIES; whb++){ */
/*       int latestSample = 0; */
/*       if(whb==0){ // then we are not wrapped... */
/* 	latestSample = (earliestSample + MAX_NUMBER_SAMPLES - 1)%MAX_NUMBER_SAMPLES; */
/*       } */
/*       else{ */
/* 	latestSample = MAX_NUMBER_SAMPLES - 1; */
/*       } */

/*     int rco = 0; */
/*       for(rco=0; rco<NUM_RCO; rco++){ */
/* 	int lab=0; */
/* 	for(lab=0; lab<LABRADORS_PER_SURF; lab++){ */
/* 	  int surf=0; */
/* 	  for(surf=0;surf<numSurfs;surf++) { */
/* 	    int chan=8; /\* Same times across surf. Let's do clock channel only *\/ */
/* 	    for(chan=0;chan<CHANNELS_PER_SURF;chan++) { */

/* 	      if(earliestSample==0) */
/* 		earliestSample++; */

/* 	      if(latestSample==0) */
/* 		latestSample=259; */

/* 	      /\* Need to remember how many extra samples for volts unwrapping without looking at times *\/ */
/* 	      numExtras[surf][lab][rco][whb][earliestSampleInd] = 0; */

/* 	      int samp=0; */
/* 	      /\* for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) { *\/ */
/* 	      /\* 	pedSubBody.channel[chanIndex].data[samp]=pedSubBody.channel[chanIndex].data[samp]; *\/ */
/* 	      /\* } *\/ */
      
/* 	      //Now do the unwrapping */
/* 	      int index=0; */
/* 	      //	      double time=0; */
/* 	      double time=0; */
/* 	      //printf("surf %d, chan %d, delay %lf\n", surf, chan, time); */
/* 	      if(latestSample<earliestSample) { */
/* 		//	      if(!whb) { */
/* 		//We have two RCOs */
/* 		int nextExtra=256; */
/* 		double extraTime=0;	 */
/* 		if(earliestSample<256) { */
/* 		  //Lets do the first segment up	 */
/* 		  for(samp=earliestSample;samp<256;samp++) { */
/* 		    int binRco=1-rco; */
/* 		    times2[surf][lab][rco][whb][earliestSampleInd][index]=time; */
/* 		    if(samp==255) { */
/* 		      extraTime=time+(justBinByBin[surf][lab][binRco][samp])*tempFactor; */
/* 		    } */
/* 		    else { */
/* 		      time+=(justBinByBin[surf][lab][binRco][samp])*tempFactor; */
/* 		    } */
/* 		    index++; */
/* 		  } */
/* 		  //time+=epsilonFromBenS[surf][lab][rco]*tempFactor*fEpsilonTempScale; */
/* 		  //time+=epsilonFromBenS[surf][lab][1-rco]*tempFactor*fEpsilonTempScale; */
/* 		  time+=epsilonFromBenS[surf][lab][rco]*tempFactor*fEpsilonTempScale; */
/* 		} */
/* 		else { */
/* 		  nextExtra=260; */
/* 		  extraTime=0; */
/* 		} */
	
	
/* 		if(latestSample>=1) { */
/* 		  /\* We are going to ignore sample zero for now *\/ */
/* 		  time+=(justBinByBin[surf][lab][rco][0])*tempFactor; */
/* 		  for(samp=1;samp<=latestSample;samp++) { */
/* 		    int binRco=rco; */
/* 		    if(nextExtra<260 && samp==1) { */
/* 		      if(extraTime<time-0.22) { */
/* 			binRco=1-rco; */
/* 			times2[surf][lab][rco][whb][earliestSampleInd][index]=extraTime; */
/* 			if(nextExtra<259) { */
/* 			  extraTime+=(justBinByBin[surf][lab][binRco][nextExtra])*tempFactor; */
/* 			} */
/* 			nextExtra++; */
/* 			numExtras[surf][lab][rco][whb][earliestSampleInd]++; */
/* 			index++;	  */
/* 			samp--; */
/* 			continue; */
/* 		      } */
/* 		    } */
/* 		    times2[surf][lab][rco][whb][earliestSampleInd][index]=time; */
/* 		    if(samp<259) { */
/* 		      time+=(justBinByBin[surf][lab][binRco][samp])*tempFactor; */
/* 		    } */
/* 		    index++; */
/* 		  } */
/* 		  /\* printf("TIME = %lf\n", time); *\/ */
/* 		} */
/* 	      } */
/* 	      else { */
/* 		/\* Only one rco *\/ */
/* 		for(samp=earliestSample;samp<=latestSample;samp++) { */
/* 		  int binRco=rco; */
/* 		  //		  if( latestSample == 259 ) binRco=1-rco; // will this work??? */
/* 		  times2[surf][lab][rco][whb][earliestSampleInd][index]=time; */
/* 		  if(samp<259) { */
/* 		    time+=(justBinByBin[surf][lab][binRco][samp])*tempFactor; */
/* 		  } */
/* 		  index++; */
/* 		} */
/* 	      } */
/* 	    } */
/* 	  } */
/* 	} */
/*       } */
/*     } */
/*   } */
/* } */

















/**************************************************************************************************************/
/* Functions to read in calibration numbers from text (.dat) files. Pretty boring really...                   */
/**************************************************************************************************************/

void readInCalibratedDeltaTs(const char* fileName){

  FILE* inFile = fopen(fileName, "r");
  double defaultVal = 1./2.6;

  if( inFile != NULL){
    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<6; wordInd++){
      char word[10];
      fscanf(inFile, "%s ", word);
    }
  }  
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %lf\n", fileName , defaultVal);
  }
  /* Got past header, now read in calibrated deltaTs */
  int surfInd=0;
  for(surfInd=0; surfInd<numSurfs; surfInd++){
    int labInd=0;
    for(labInd=0; labInd<LABRADORS_PER_SURF; labInd++){
      int rco=0;
      for(rco=0; rco<2; rco++){
	/* DEFINED OPPOSITE IN FIRMWARE AND SOFTWARE! DEALT WITH HERE!? */
	int binRco = 1 - rco;
	//	int binRco = rco;
	/**************************************************************/

	int samp=0;
	for(samp=0; samp<MAX_NUMBER_SAMPLES; samp++){
	  if( inFile != NULL){
	    int surfVal, chipVal, rcoVal, sampleVal;
	    fscanf(inFile, "%d %d %d %d %lf", &surfVal, &chipVal, &rcoVal, &sampleVal, &justBinByBin[surfInd][labInd][binRco][samp]);
	    /* printf("%d\t%d\t%d\t%d\t%lf\n", surfVal, chipVal, rcoVal, sampleVal, justBinByBin[surfInd][labInd][rco][samp]); */
	  }
	  else{
	    justBinByBin[surfInd][labInd][binRco][samp] = defaultVal;
	  }
	}
      }
    }
  }

  if(inFile != NULL){
    fclose(inFile);
  }

}



void readInRcoLatchDelay(const char* fileName){
  FILE* inFile = fopen(fileName, "r");
  int startDefaultVal = 1;
  int defaultVal = 20; /* ish ? */

  if( inFile != NULL){
    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<4; wordInd++){
      char word[10];
      fscanf(inFile, "%s ", word);
    }
  }
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %d\n", fileName , defaultVal);
  }
  /* Got past header, now read in calibrated deltaTs */
  int surfInd=0;
  for(surfInd=0; surfInd<numSurfs; surfInd++){
    int labInd=0;
    for(labInd=0; labInd<LABRADORS_PER_SURF; labInd++){
      if( inFile != NULL){
	int surfVal, chipVal;
	fscanf(inFile, "%d %d %d %d", &surfVal, &chipVal, &rcoLatchStart[surfInd][labInd], &rcoLatchEnd[surfInd][labInd]);
	/* printf("%d %d %d %d\n", surfVal, chipVal, rcoLatchStart[surfInd][labInd], rcoLatchEnd[surfInd][labInd]); */
      }
      else{
	rcoLatchStart[surfInd][labInd] = startDefaultVal;
	rcoLatchEnd[surfInd][labInd] = defaultVal;
      }
    }
  }

  if(inFile != NULL){
    fclose(inFile);
  }

}




void readInRelativeCableDelay(const char* fileName){
  FILE* inFile = fopen(fileName, "r");
  int defaultVal = 0;

  if( inFile != NULL){
    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<3; wordInd++){
      char word[10];
      fscanf(inFile, "%s ", word);
    }
  }
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %d\n", fileName , defaultVal);
  }
  /* Got past header, now read in calibrated deltaTs */
  int surfInd=0;
  for(surfInd=0; surfInd<numSurfs; surfInd++){
    int chanInd=0;
    for(chanInd=0; chanInd<CHANNELS_PER_SURF-1; chanInd++){
      if( inFile != NULL){
	int surfVal, chanVal;
	double tempVal;
	fscanf(inFile, "%d %d %lf", &surfVal, &chanVal, &tempVal);
	relativeCableDelays[surfVal][chanVal] = tempVal;
	/* printf("surf %d, chan %d, relativeCableDelays[surfVal][chanVal] = %lf\n", surfVal, chanVal, tempVal); */
      }
      else{
	relativeCableDelays[surfInd][chanInd] = defaultVal;
      }
    }
  }

  if(inFile != NULL){
    fclose(inFile);
  }

}

void readInEpsilons(const char* fileName){

  FILE* inFile = fopen(fileName, "r");
  //  double defaultVal = 1./2.6;
  double defaultVal = 1;

  if(inFile != NULL){
    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<5; wordInd++){
      char word[10];
      fscanf(inFile, "%s ", word);
    }
  }
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %lf\n", fileName , defaultVal);
  }
  int surfInd;
  for(surfInd=0; surfInd<numSurfs; surfInd++){
    int labInd=0;
    for(labInd=0; labInd<LABRADORS_PER_SURF; labInd++){
      int rco=0;
      for(rco=0; rco<2; rco++){
	int binRco = 1 - rco;
	if(inFile!=NULL){
	  int surfVal, labVal, rcoVal;
	  fscanf(inFile, "%d %d %d %lf", &surfVal, &labVal, &rcoVal, &epsilonFromBenS[surfInd][labInd][binRco]);
	  /* printf("%d %d %d %lf\n", surfVal, labVal, rcoVal, epsilonFromBenS[surfInd][labInd][rco]); */
	}
	else{
	  epsilonFromBenS[surfInd][labInd][rco] = defaultVal;
	}
      }
    }
  }
  if(inFile!=NULL){
    fclose(inFile);
  }

}

void readInClockCrossCorr(const char* fileName){
  FILE* inFile = fopen(fileName, "r");

  double defaultVal = 0;
  if(inFile!=NULL){

    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<4; wordInd++){
      //  for(wordInd=0; wordInd<3; wordInd++){
      char word[10];
      fscanf(inFile, "%s ", word);
    }
  }
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %lf\n", fileName , defaultVal);
  }

  int surfInd;
  for(surfInd=0; surfInd<numSurfs; surfInd++){
    int labInd=0;
    for(labInd=0; labInd<LABRADORS_PER_SURF; labInd++){
      if(inFile!=NULL){
	int surfVal, labVal;
	fscanf(inFile, "%d %d %lf", &surfVal, &labVal, &clockCrossCorr[surfInd][labInd]);
	//      printf("%d %d %lf\n", surfVal, labVal, clockCrossCorr[surfInd][labInd]);
      }
      else{
	clockCrossCorr[surfInd][labInd] = defaultVal;
      }
    }
  }  
}

