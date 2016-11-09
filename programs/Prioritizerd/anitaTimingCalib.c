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
  2.) Interpolating the waveforms to give evenly sampled time domains. Uses gsl library.

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
#define lengthClockFFT numUpsampledClockSamples*2

static gsl_interp_accel *acc;
static const gsl_interp_type *akimaSpline;
static gsl_spline *spline;

static fftw_plan clockPlanForward;
static fftw_plan clockPlanReverse;

static double justBinByBin[ACTIVE_SURFS][LABRADORS_PER_SURF][2][MAX_NUMBER_SAMPLES];
static double epsilonFromBenS[ACTIVE_SURFS][LABRADORS_PER_SURF][2] = {{{1}}};
static double voltageCalibHarm[ACTIVE_SURFS][CHANNELS_PER_SURF-1][LABRADORS_PER_SURF] = {{{1}}};
static int rcoLatchStart[ACTIVE_SURFS][LABRADORS_PER_SURF];
static int rcoLatchEnd[ACTIVE_SURFS][LABRADORS_PER_SURF];
static double relativeCableDelays[ACTIVE_SURFS][CHANNELS_PER_SURF];

static double volts[ACTIVE_SURFS][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];
static int nSamps[ACTIVE_SURFS][CHANNELS_PER_SURF];

static double times[ACTIVE_SURFS][CHANNELS_PER_SURF][MAX_NUMBER_SAMPLES];

static double clockJitters[ACTIVE_SURFS];
//int nClockSamps[ACTIVE_SURFS];

static int whbs[ACTIVE_SURFS];
static int earliestSampleInds[ACTIVE_SURFS];
static int latestSampleInds[ACTIVE_SURFS];
static int rcos[ACTIVE_SURFS];
static int labChips[ACTIVE_SURFS];

static double* clockTimeDomain;
static fftw_complex* clockFreqDomain;
static fftw_complex* clockFreqHolder;

static int positiveSaturation = 1000;
static int negativeSaturation = -1000;
static float maxBottomToTopRatio = 5; 

/*--------------------------------------------------------------------------------------------------------------*/
/* Functions - initialization and clean up. */
void prepareTimingCalibThings(){
  acc = gsl_interp_accel_alloc();
  akimaSpline = gsl_interp_akima;

  readInCalibratedDeltaTs("/home/anita/flightSoft/programs/Prioritizerd/justBinByBin.dat");
  readInEpsilons("/home/anita/flightSoft/programs/Prioritizerd/epsilonFromBenS.dat");
  readInRcoLatchDelay("/home/anita/flightSoft/programs/Prioritizerd/rcoLatchDelay.dat");
  readInRelativeCableDelay("/home/anita/flightSoft/programs/Prioritizerd/relativeCableDelays.dat");
  readInVoltageCalib("/home/anita/flightSoft/programs/Prioritizerd/simpleVoltageCalibrationHarm.txt");

  clockTimeDomain = (double*) fftw_malloc(sizeof(double)*lengthClockFFT);
  clockFreqDomain = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*lengthClockFFT);
  clockFreqHolder = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*lengthClockFFT);
  clockPlanForward = fftw_plan_dft_r2c_1d(lengthClockFFT, clockTimeDomain, 
					  clockFreqDomain, FFTW_MEASURE);
  clockPlanReverse = fftw_plan_dft_c2r_1d(lengthClockFFT, clockFreqDomain, 
					  clockTimeDomain, FFTW_MEASURE);

  kvpReset();
  KvpErrorCode err = KVP_E_OK;
  err = (KvpErrorCode) configLoad ("Prioritizerd.config","prioritizerd") ;
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to load Prioritizerd.config returned %s\n", kvpErrorString(err));
  }
  positiveSaturation = kvpGetInt("positiveSaturation", 1000);
  negativeSaturation = kvpGetInt("negativeSaturation", -1000);
  maxBottomToTopRatio = kvpGetInt("blastMaxBottomToTopRatio", 100); 

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
/* Interpolation functions. GSL for fancy things. */
/*--------------------------------------------------------------------------------------------------------------*/
double* interpolateWaveform(int nRaw, double* rawWave, double* unevenTimes, 
			    int nInterp, double t0interp, double dtNsInterp){

  spline = gsl_spline_alloc (akimaSpline, nRaw);
  gsl_spline_init (spline, unevenTimes, rawWave, nRaw);
  
  double* interpWave = (double*) malloc(nInterp*sizeof(double));

  float time = t0interp;
  int sampInd = 0;

  /* int rawInd = 0; */

  /* if(clockNum>=0){ */
  /*   nClockSamps[clockNum] = 0; */
  /* } */


  for(sampInd=0; sampInd<nInterp; sampInd++){

    /* If we are inside known time band then interpolate. */
    if(time >= unevenTimes[0] && time <= unevenTimes[nRaw-1]){
      interpWave[sampInd] = gsl_spline_eval(spline, time, acc);
      /* if( clockNum>=0){ */
      /* 	nClockSamps[clockNum]++; */
      /* } */
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

  double* interpWave = (double*) malloc(nInterp*sizeof(double));

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

  const double lowPassFreq = 150;
  //  const double lowPassFreq = 400;
  //  const double lowPassFreq = clockFreq_MHz*1.5;
   /* const double lowPassFreq = 1e11; */

  //  int numFreqs = numSamples/2+1;
  int numFreqs = lengthClockFFT/2+1;
  double deltaF_MHz = 1e3/(deltaT_ns*numSamples);

  int firstRealSamp = (lengthClockFFT - numSamples)/2;

  int samp=0;
  double y=0;
  for(samp=0; samp<lengthClockFFT; samp++){
    if(samp<firstRealSamp || samp>=firstRealSamp+numSamples){
      y = 0;
    }
    else{
      y = clock1[samp - firstRealSamp];
    }
    clockTimeDomain[samp] = y;
  }

   /* memcpy(clockTimeDomain, clock1, sizeof(double)*numSamples); */
  fftw_execute(clockPlanForward);
  memcpy(clockFreqHolder, clockFreqDomain, sizeof(fftw_complex)*(numSamples/2+1));


  for(samp=0; samp<lengthClockFFT; samp++){
    if(samp<firstRealSamp || samp>=firstRealSamp+numSamples){
      y = 0;
    }
    else{
      y = clock2[samp - firstRealSamp];
    }
    clockTimeDomain[samp] = y;
  }
  /* memcpy(clockTimeDomain, clock2, sizeof(double)*numSamples); */
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
  double* crossCorrelatedClocks = (double*) malloc(sizeof(double)*lengthClockFFT);
  //  double* crossCorrelatedClocks = malloc(sizeof(double)*numSamples);
  //  memcpy(crossCorrelatedClocks, clockTimeDomain, sizeof(double)*numSamples);
  double xVals[lengthClockFFT];

  for(samp = 0;samp<lengthClockFFT;samp++) {
    if(samp<lengthClockFFT/2) {
      //Positive
      //      xVals[i+(N/2)]=(i*deltaT)+offset;
      //      yVals[i+(N/2)]=corVals[i];
      crossCorrelatedClocks[samp+(lengthClockFFT/2)]=clockTimeDomain[samp];
      xVals[samp+(lengthClockFFT/2)]=(samp*deltaT_ns);
      // xval = samp*deltaT + deltaT;
    }
    else {
      //Negative
      //      xVals[i-(N/2)]=((samp-lengthFFT)*deltaT)+offset;
      //      yVals[i-(N/2)]=corVals[i];	  
      crossCorrelatedClocks[samp-(lengthClockFFT/2)]=clockTimeDomain[samp];
      xVals[samp-(lengthClockFFT/2)] = (samp-lengthClockFFT)*deltaT_ns;
    }
  }

  /* int maxIndGr = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, 0, clockPeriod_samples/2); */
  /* int maxIndLe = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, numSamples-clockPeriod_samples/2, numSamples); */

  /* int maxInd = crossCorrelatedClocks[maxIndGr] > crossCorrelatedClocks[maxIndLe] ? maxIndGr : maxIndLe; */
  /* double clockCor = maxInd > numSamples/2 ? (maxInd-numSamples)*deltaT_ns : deltaT_ns*maxInd; */

  int maxInd = findIndexOfMaximumWithinLimits(crossCorrelatedClocks, lengthClockFFT/2-clockPeriod_samples/2, lengthClockFFT/2+clockPeriod_samples/2);
  //  double clockCor = (maxInd-lengthClockFFT/2)*deltaT_ns;
  double clockCor = xVals[maxInd] ;//(maxInd-lengthClockFFT/2)*deltaT_ns;

  /* printf("maxInd = %d, clockCor = %lf\n", maxInd, clockCor); */

  free(crossCorrelatedClocks);

  return clockCor;

}




int findIndexOfMaximumWithinLimits(double* array, int startInd, int stopInd){
  /* 
     Returns -1 on failure.
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

  double fEpsilonTempScale= 1;
  double tempFactor = 1; //fTempFactorGuess[entry];
  int surf=0;
  for(surf=0;surf<ACTIVE_SURFS;surf++) {
    int chan=0;
    for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
      int chanIndex=surf*CHANNELS_PER_SURF + chan;
      
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
      double voltCalib = 1;

      if(chan != 8){ // don't fuck with the clock...
      time = relativeCableDelays[surf][chan];
      voltCalib = voltageCalibHarm[surf][chan][labChip]; 
#ifdef CALIBRATION
      if(surf==0 && chan==0){
	printf("Calibration mode...\n");
      }
#else

#endif

      }
      if(latestSample<earliestSample) {
	//We have two RCOs
	int nextExtra=256;
	double extraTime=0;
	if(earliestSample<256) {
	  //Lets do the first segment up
	  for(samp=earliestSample;samp<256;samp++) {
	    int binRco=1-rco;
	    volts[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[samp]*voltCalib;
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
	      volts[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[nextExtra]*voltCalib;
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
	    volts[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[samp]*voltCalib;
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
	  volts[surf][chan][index]=(double)pedSubBody.channel[chanIndex].data[samp]*voltCalib;
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

      if(nSamps[surf][chan]!=nSamps[surf][8])
      {
        nSamps[surf][chan]=nSamps[surf][8];
        memcpy( times[surf][chan], times[surf][8], nSamps[surf][8] * sizeof(double)); 
      }
    }
    /* int samp=0; */
    /* for(samp=0; samp<nSamps[surf][8]; samp++){ */
    /*   printf("surf %d clock[%d] = %lf\t", surf, samp, volts[surf][8][samp]); */
    /*   printf("surf %d clockTimes[%d] = %lf\n", surf, samp, times[surf][8][samp]); */
    /* } */
    
  }
}



void doTimingCalibration(int entry, AnitaEventHeader_t* theHeader,
			 PedSubbedEventBody_t pedSubBody,
			 double* finalVolts[]){


  double maxVppTop = 0; 
  double maxVppBottom = 0; 
  theHeader->prioritizerStuff = 0;

  /* printf("Before... theHeader->prioritizerStuff = %hu\n", theHeader->prioritizerStuff); */

  const double deltaT = NOMINAL_SAMPLING;

  int surf=0;
  for(surf=0; surf<ACTIVE_SURFS; surf++){
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

  /* printf("After... theHeader->prioritizerStuff = %hu\n", theHeader->prioritizerStuff); */
  processEventAG(pedSubBody);



  for(surf=0; surf<ACTIVE_SURFS; surf++){
    int chan=0;
    int samp = 0; 
    for(chan=0; chan<8; chan++){
      int chanIndex=surf*CHANNELS_PER_SURF + chan;
      double vmax = -1e9; 
      double vmin =  1e9; 
      double vpp = 0;
      int ant = abs(surfToAntMap[surf][chan]);
      if((pedSubBody.channel[chanIndex].xMax > positiveSaturation || pedSubBody.channel[chanIndex].xMin < negativeSaturation) && theHeader->prioritizerStuff == 0){
        theHeader->prioritizerStuff = 1;
      }


      //now check for blasts 

      if (ant < 16 || ant >= 32) 
      {
        for (samp = 0; samp < nSamps[surf][chan]; samp++) 
        {
          double v = volts[surf][chan][samp] ;
          if ( v < vmin) vmin = v; 
          if ( v > vmax) vmax = v; 
        }
        vpp = vmax-vmin; 

        if (ant < 16 && vpp > maxVppTop) maxVppTop = vpp; 
        else if (ant >= 32 && vpp > maxVppBottom) maxVppBottom = vpp; 
      }

    }
  }

  if (maxVppBottom / maxVppTop > maxBottomToTopRatio) 
  {
    theHeader->prioritizerStuff |= 2; 
  }









  /* int numUpZC[ACTIVE_SURFS] = {0}; */
  /* double upZCs[ACTIVE_SURFS][10] = {{0}}; */

  /* for(surf=0; surf<ACTIVE_SURFS; surf++){ */
  /*   /\* Let's look for upgoing zero-crossings *\/ */

  /*   /\* /\\* Normalize clocks *\\/ *\/ */
  /*   int samp=0; */
  /*   double meanHigh = 0; */
  /*   int numHigh = 0; */
  /*   double meanLow = 0; */
  /*   int numLow = 0; */
  /*   for(samp=0; samp<nSamps[surf][8]; samp++){ */
  /*     if(volts[surf][8][samp]>0){ */
  /*   	meanHigh += volts[surf][8][samp]; */
  /*   	numHigh++; */
  /*     } */
  /*     else{ */
  /*   	meanLow += volts[surf][8][samp]; */
  /*   	numLow++; */
  /*     } */
  /*   } */
  /*   meanHigh/=numHigh; */
  /*   meanLow/=numLow; */
  /*   double offset = (meanHigh+meanLow)/2; */
  /*   double maxVal = meanHigh - offset; */
  /*   for(samp=0; samp<nSamps[surf][8]; samp++){ */
  /*     volts[surf][8][samp] -= offset; */
  /*     volts[surf][8][samp]/=maxVal; */
  /*   } */

  /*   /\* for(samp=0; samp<nSamps[surf][8]-1; samp++){ *\/ */
  /*   /\*   /\\* printf("surf %d, volts[surf][8][%d] = %lf\n", surf, samp, volts[surf][8][samp]); *\\/ *\/ */
  /*   /\*   if(volts[surf][8][samp]<0 && volts[surf][8][samp+1]>=0){ *\/ */
  /*   /\* 	/\\* printf("ZC!!!!\n"); *\\/ *\/ */
  /*   /\* 	double m = (volts[surf][8][samp+1] - volts[surf][8][samp])/(times[surf][8][samp+1]-times[surf][8][samp]); *\/ */
  /*   /\* 	upZCs[surf][numUpZC[surf]] = times[surf][8][samp] - volts[surf][8][samp]/m; *\/ */
  /*   /\* 	numUpZC[surf]++; *\/ */
  /*   /\*   } *\/ */
  /*   /\* } *\/ */
  /*   /\* int zc=0; *\/ */
  /*   /\* printf("surf %d, numUpZC = %d\n", surf, numUpZC[surf]); *\/ */
  /*   /\* for(zc=0; zc<numUpZC[surf]; zc++){ *\/ */
  /*   /\*   printf("surf %d, zc %d, t = %lf\n", surf, zc, upZCs[surf][zc]); *\/ */
  /*   /\* } *\/ */
  /* } */

  
  /* clockJitters[0] = 0; */
  /* clockJitters[1] = 2.09135; */
  /* clockJitters[2] = 0.649038; */
  /* clockJitters[3] = 4.03846; */
  /* clockJitters[4] = 1.4423; */
  /* clockJitters[5] = -2.42788; */
  /* clockJitters[6] = -5; */
  /* clockJitters[7] = -1.53579; */
  /* clockJitters[8] = -6.5625; */
  /* clockJitters[9] = -2.09135; */
  /* clockJitters[10] = -0.3125; */
  /* clockJitters[11] = -3.67788; */

  /* /\* Find clock jitter correction from upgoing zero crossings... *\/ */
  /* for(surf=1; surf<ACTIVE_SURFS; surf++){ */
  /*   clockJitters[surf] = 0; */
  /*   if(numUpZC[surf]==numUpZC[0]){ */
  /*     int zc=0; */
  /*     for(zc=0; zc<numUpZC[surf]; zc++){ */
  /* 	clockJitters[surf] += upZCs[surf][zc] - upZCs[0][zc]; */
  /*     } */
  /*     clockJitters[surf] /= numUpZC[surf]; */
  /*   } */
  /*   else{ */
  /*     int maxOffset = abs(numUpZC[surf] - numUpZC[0]); */
  /*     int smallerNumZCs = numUpZC[0] < numUpZC[surf] ? numUpZC[0] : numUpZC[surf]; */
  /*     int largerNumZCs = numUpZC[0] < numUpZC[surf] ? numUpZC[surf] : numUpZC[0]; */
  /*     int offset=0; */
  /*     double smallerClockJitter = 100000; */
  /*     for(offset=-maxOffset; offset<=maxOffset; offset++){ */
  /* 	clockJitters[surf] = 0; */
  /* 	int zc=0; */
  /* 	for(zc=0; zc<largerNumZCs; zc++){ */
  /* 	  if(zc + offset >=0 && zc + offset< numUpZC[surf]){ */
  /* 	    clockJitters[surf] += upZCs[surf][zc+offset] - upZCs[0][zc]; */
  /* 	  } */
  /* 	} */
  /* 	clockJitters[surf] /= smallerNumZCs; */
  /* 	smallerClockJitter  = fabs(clockJitters[surf]) < fabs(smallerClockJitter) ? clockJitters[surf] : smallerClockJitter; */
  /*     } */
  /*     clockJitters[surf] = smallerClockJitter; */
  /*   } */
  /* } */


  /* Upsample clocks */
  double* interpClocks[ACTIVE_SURFS];
  for(surf=0; surf<ACTIVE_SURFS; surf++){  
    /* double* tempClock = interpolateWaveform(nSamps[surf][8], */
    /* 					    volts[surf][8], */
    /* 					    times[surf][8], */
    /* 					    nSamps[surf][8], */
    /* 					    times[surf][8][0], */
    /* 					    deltaT); */
    /* double tempTimes[NUM_SAMPLES]; */
    /* int samp=0; */
    /* for(samp=0; samp<NUM_SAMPLES; samp++){ */
    /*   tempTimes[samp] = samp*deltaT; */
    /* } */

    /* double* tempClock2 = simpleBandPass(tempClock, nSamps[surf][8], deltaT, 0, 150); */

    /* interpClocks[surf] = interpolateWaveform(NUM_SAMPLES, */
    /* 					     tempClock2, */
    /* 					     tempTimes, */
    /* 					     numUpsampledClockSamples, */
    /* 					     times[surf][8][0], */
    /* 					     deltaT/upsampleFactor); */
    /* free(tempClock); */
    /* free(tempClock2); */

    /* for(int samp=0; samp< nSamps[surf][8]; samp++){ */
    /*   printf("times[%d][8][%d] = %lf\n", surf, samp, times[surf][8][samp]); */
    /* } */
    interpClocks[surf] = interpolateWaveform(nSamps[surf][8],
    					     volts[surf][8],
    					     times[surf][8],
    					     numUpsampledClockSamples,
    					     times[surf][8][0],
    					     deltaT/upsampleFactor);

  }

  /* Extract clock jitter from interpolated clocks relative to clock 0. */
  clockJitters[0] = 0;
  for(surf=1; surf<ACTIVE_SURFS; surf++){
    /* printf("eventNumber %d, clockJitters... surf %d\n", theHeader->eventNumber, surf); */
    clockJitters[surf] = findClockJitterCorrection(numUpsampledClockSamples, interpClocks[0],
    						   interpClocks[surf], deltaT/upsampleFactor,
    						   surf, labChips[surf]);
  }


  double startTime=10000;
  for(surf=0; surf<ACTIVE_SURFS; surf++){
    int chan=0;
    for(chan=0; chan<CHANNELS_PER_SURF; chan++){
      if(times[surf][chan][0] - clockJitters[surf] < startTime){
	startTime = times[surf][chan][0] - clockJitters[surf];
      }
      /* printf("eventNumber %u, surf %d, chan %d, clockJitter %lf, t0 %lf\n", theHeader->eventNumber, surf, chan, clockJitters[surf], times[surf][chan][0] - clockJitters[surf]); */
    }
  }

  for(surf=0; surf<ACTIVE_SURFS; surf++){
    int chan=0;
    //    for(chan=0; chan<CHANNELS_PER_SURF-1; chan++){
    for(chan=0; chan<CHANNELS_PER_SURF; chan++){
      double newTimes[260] = {0};
      int samp = 0;
      for(samp=0; samp<nSamps[surf][chan]; samp++){
	newTimes[samp] = times[surf][chan][samp];
      	newTimes[samp] -= clockJitters[surf];
      }

      //finalVolts[surf*CHANNELS_PER_SURF + chan] = linearlyInterpolateWaveform(nSamps[surf][chan],
      finalVolts[surf*CHANNELS_PER_SURF + chan] = interpolateWaveform(nSamps[surf][chan],
								      //volts2[surf][chan],
								      volts[surf][chan],
								      newTimes,
								      //times[surf][chan],
								      256, 
								      startTime,
								      deltaT);

    }
  }



  /* Tidy up. */
  for(surf=0; surf<ACTIVE_SURFS; surf++){
    free(interpClocks[surf]);
  }


}





double* simpleBandPass(double* volts, int length, double dt, double highPassMHz, double lowPassMHz){

  fftw_complex* fftOut = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*NUM_SAMPLES);
  double* fftIn = (double*) malloc(sizeof(double)*NUM_SAMPLES);
  
  int samp=0;
  for(samp=0; samp<length; samp++){
    fftIn[samp] = volts[samp];
  }
  if(length<NUM_SAMPLES){
    for(samp=length; samp<NUM_SAMPLES; samp++){
      fftIn[samp] = 0;
    }
  }
  else{
    fprintf(stderr, "simpleBandPass issues: length = %d, needs to be less than %d\n", length, NUM_SAMPLES);
  }

  fftw_plan thePlan = fftw_plan_dft_r2c_1d(NUM_SAMPLES,fftIn,fftOut,FFTW_MEASURE);
  if(!thePlan) {
    printf("Bollocks\n");
  }

  fftw_execute(thePlan);
  fftw_destroy_plan(thePlan);


  int freqInd=0;
  double df = 1e3/(NUM_SAMPLES*dt);
  double freq=0;
  for(freqInd=0; freqInd<(NUM_SAMPLES/2)+1; freqInd++){
    if(freq < highPassMHz || freq > lowPassMHz){
      fftOut[freqInd][0] = 0;
      fftOut[freqInd][1] = 0;
    }
    freq+=df;
  }

  fftw_plan thePlan2 = fftw_plan_dft_c2r_1d(NUM_SAMPLES,fftOut,fftIn,FFTW_MEASURE);
  fftw_execute(thePlan2);
  fftw_free(fftOut);
  fftw_destroy_plan(thePlan2);  

  return fftIn;
}














/**************************************************************************************************************/
/* Functions to read in calibration numbers from text (.dat) files. Pretty boring really...                   */
/**************************************************************************************************************/

void readInCalibratedDeltaTs(const char* fileName){

  FILE* inFile = fopen(fileName, "r");
  double defaultVal = NOMINAL_SAMPLING;

  if( inFile != NULL){
    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<6; wordInd++){
      char word[100];
      fscanf(inFile, "%s ", word);
    }
  }  
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %lf\n", fileName , defaultVal);
  }
  /* Got past header, now read in calibrated deltaTs */
  int surfInd=0;
  for(surfInd=0; surfInd<ACTIVE_SURFS; surfInd++){
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
	    /* printf("%d\t%d\t%d\t%d\t%lf\n", surfVal, chipVal, rcoVal, sampleVal, justBinByBin[surfInd][labInd][binRco][samp]); */
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
      char word[100];
      fscanf(inFile, "%s ", word);
    }
  }
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %d\n", fileName , defaultVal);
  }
  /* Got past header, now read in calibrated deltaTs */
  int surfInd=0;
  for(surfInd=0; surfInd<ACTIVE_SURFS; surfInd++){
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


void readInVoltageCalib(const char* fileName){
  FILE* inFile = fopen(fileName, "r");
  int defaultVal = 1;

  if( inFile != NULL){
    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<5; wordInd++){
      char word[100];
      fscanf(inFile, "%s ", word);
    }
  }
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %d\n", fileName , defaultVal);
  }
  /* Got past header, now read in calibrated deltaTs */
  int surfInd=0;
  for(surfInd=0; surfInd<ACTIVE_SURFS; surfInd++){
    int chanInd=0;
    for(chanInd=0; chanInd<CHANNELS_PER_SURF-1; chanInd++){
      int labInd=0;
      for(labInd=0; labInd<LABRADORS_PER_SURF; labInd++){
	if( inFile != NULL){
	  int surfVal, chanVal, labVal;
	  double tempVal;
	  fscanf(inFile, "%d %d %d %lf", &surfVal, &chanVal, &labVal, &tempVal);
	  voltageCalibHarm[surfVal][chanVal][labVal] = tempVal;
	  /* printf("surf %d, chan %d, lab %d, voltageCalibHarm[surfVal][chanVal][labVal] = %lf\n", surfVal, chanVal, labVal, tempVal); */
	}
	else{
	  voltageCalibHarm[surfInd][chanInd][labInd] = defaultVal;
	}
      }
    }
  }

  if(inFile != NULL){
    fclose(inFile);
  }
  return;
}




void readInRelativeCableDelay(const char* fileName){
  FILE* inFile = fopen(fileName, "r");
  int defaultVal = 0;

  if( inFile != NULL){
    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<3; wordInd++){
      char word[100];
      fscanf(inFile, "%s ", word);
    }
  }
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %d\n", fileName , defaultVal);
  }
  /* Got past header, now read in calibrated deltaTs */
  int surfInd=0;
  for(surfInd=0; surfInd<ACTIVE_SURFS; surfInd++){
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
  double defaultVal = 1;

  if(inFile != NULL){
    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<5; wordInd++){
      char word[100];
      fscanf(inFile, "%s ", word);
    }
  }
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %lf\n", fileName , defaultVal);
  }
  int surfInd;
  for(surfInd=0; surfInd<ACTIVE_SURFS; surfInd++){
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
