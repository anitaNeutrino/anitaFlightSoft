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
#define upsampleFactor 1
#define numUpsampledClockSamples 256*upsampleFactor
#define lengthClockFFT numUpsampledClockSamples*2

static gsl_interp_accel *acc;
static const gsl_interp_type *akimaSpline;
static gsl_spline *spline;
static gsl_interp *interp;


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
static int asymSaturation = 500;
int anitaCalibVersion = 4;



static float maxThresholdBottomToTopRingPeakToPeakRatio = 2.8;
static float minThresholdBottomToTopRingPeakToPeakRatio = 1.1;
int skipBlastRatio[NUM_POLARIZATIONS][NUM_PHI] = {{0}};

/*--------------------------------------------------------------------------------------------------------------*/
/* Functions - initialization and clean up. */
void prepareTimingCalibThings(){

  kvpReset();
  KvpErrorCode err = KVP_E_OK;
  err = (KvpErrorCode) configLoad ("Prioritizerd.config","prioritizerd") ;
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to load Prioritizerd.config returned %s\n", kvpErrorString(err));
    syslog(LOG_ERR, "Warning! Trying to load Prioritizerd.config returned %s\n", kvpErrorString(err));
  }

  int numPhi = NUM_PHI;
  err = kvpGetIntArray ("skipBlastRatioHPol", &skipBlastRatio[0][0], &numPhi);
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to do load skipBlastRatioHPol from Prioritizerd.config returned %s\n", kvpErrorString(err));
    syslog(LOG_ERR, "Warning! Trying to do load skipBlastRatioHPol from Prioritizerd.config returned %s\n", kvpErrorString(err));
  }

  err = kvpGetIntArray ("skipBlastRatioVPol", &skipBlastRatio[1][0], &numPhi);
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to do load skipBlastRatioVPol from Prioritizerd.config returned %s\n", kvpErrorString(err));
    syslog(LOG_ERR, "Warning! Trying to do load skipBlastRatioVPol from Prioritizerd.config returned %s\n", kvpErrorString(err));
  }


  maxThresholdBottomToTopRingPeakToPeakRatio = kvpGetFloat("blastRatioMax", 2.8);
  minThresholdBottomToTopRingPeakToPeakRatio = kvpGetFloat("blastRatioMin", 1.1);

  /* { */
  /*   int pol=0; */
  /*   for(pol=0; pol < 2; pol++){ */
  /*     int phi=0; */
  /*     for(phi=0; phi < 16; phi++){ */
  /* 	printf("%d\t%d\t%d\n", pol, phi, skipBlastRatio[pol][phi]); */
  /*     } */
  /*   } */
  /* } */

  positiveSaturation = kvpGetInt("positiveSaturation", 1000);
  negativeSaturation = kvpGetInt("negativeSaturation", -1000);
  asymSaturation = kvpGetInt("asymSaturation", 500);
  anitaCalibVersion = kvpGetInt("anitaCalibVersion", 4);

  acc = gsl_interp_accel_alloc();
  akimaSpline = gsl_interp_akima;
  /* akimaSpline = gsl_interp_linear; */

  spline = NULL;
  interp = NULL;


  /* const char* flightSoftDir=getenv("ANITA_FLIGHT_SOFT_DIR"); */
  const char* flightSoftDir="/home/anita/flightSoft";

  char justBinByBinFileName[FILENAME_MAX];
  sprintf(justBinByBinFileName, "%s/programs/Prioritizerd/justBinByBin.dat", flightSoftDir);
  readInCalibratedDeltaTs(justBinByBinFileName);

  //readInEpsilons("/home/anita/flightSoft/programs/Prioritizerd/epsilonFromBenS.dat");
  char epsilonFromBenSFileName[FILENAME_MAX];
  sprintf(epsilonFromBenSFileName, "%s/programs/Prioritizerd/epsilonFromBenS.dat", flightSoftDir);
  readInEpsilons(epsilonFromBenSFileName);

  //readInRcoLatchDelay("/home/anita/flightSoft/programs/Prioritizerd/rcoLatchDelay.dat");
  char rcoLatchDelayFileName[FILENAME_MAX];
  sprintf(rcoLatchDelayFileName, "%s/programs/Prioritizerd/rcoLatchDelay.dat", flightSoftDir);
  readInRcoLatchDelay(rcoLatchDelayFileName);


  char relativeCableDelaysFileName[FILENAME_MAX];
  sprintf(relativeCableDelaysFileName, "%s/programs/Prioritizerd/relativeCableDelaysAnita%d.dat", flightSoftDir, anitaCalibVersion);
  readInRelativeCableDelay(relativeCableDelaysFileName);

  char simpleVoltageCalibrationHarmFileName[FILENAME_MAX];
  sprintf(simpleVoltageCalibrationHarmFileName, "%s/programs/Prioritizerd/simpleVoltageCalibrationAnita%d.txt", flightSoftDir, anitaCalibVersion);
  readInVoltageCalib(simpleVoltageCalibrationHarmFileName);


  clockTimeDomain = (double*) fftw_malloc(sizeof(double)*lengthClockFFT);
  clockFreqDomain = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*lengthClockFFT);
  clockFreqHolder = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*lengthClockFFT);
  clockPlanForward = fftw_plan_dft_r2c_1d(lengthClockFFT, clockTimeDomain,
					  clockFreqDomain, FFTW_MEASURE);
  clockPlanReverse = fftw_plan_dft_c2r_1d(lengthClockFFT, clockFreqDomain,
					  clockTimeDomain, FFTW_MEASURE);


  /* maxBottomToTopRatio = kvpGetFloat("blastMaxBottomToTopRatio", 3); */

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


  /* interp = gsl_interp_alloc (akimaSpline, nRaw); */
  /* gsl_interp_init (interp, unevenTimes, rawWave, nRaw); */



  double* interpWave = (double*) malloc(nInterp*sizeof(double));

  float time = t0interp;
  int sampInd = 0;

  /* int rawInd = 0; */

  /* if(clockNum>=0){ */
  /*   nClockSamps[clockNum] = 0; */
  /* } */

  /* t0interp = unevenTimes[0]; */

  int firstGoodSamp = -1;
  double sum=0;
  int numRealSamp = 0;
  for(sampInd=0; sampInd<nInterp; sampInd++){

    /* If we are inside known time band then interpolate. */
    if(time >= unevenTimes[0] && time <= unevenTimes[nRaw-1]){
      /* interpWave[sampInd] = gsl_interp_eval(interp, unevenTimes, rawWave, time, acc); */

      interpWave[sampInd] = gsl_spline_eval(spline, time, acc);
      /* interpWave[sampInd] = gsl_spline_eval(spline, time, acc); */
      /* interpWave[sampInd] = rawWave[sampInd]; */
      if(firstGoodSamp < 0){
    	firstGoodSamp = sampInd;
      }
      sum += interpWave[sampInd];
      numRealSamp++;
    }

    /* /\* Zero waveform outside set of known unevenTimes. *\/ */
    else{
      interpWave[sampInd] = 0;
    }

    time += dtNsInterp;
  }

  double mean = sum / numRealSamp;

  for(sampInd = firstGoodSamp; sampInd < firstGoodSamp + numRealSamp; sampInd++){
    interpWave[sampInd] -= mean;
  }


  gsl_spline_free(spline);
  /* gsl_interp_free(interp); */
  gsl_interp_accel_reset(acc);


  return interpWave;

}




double* dontInterpolateWaveform(int nRaw, double* rawWave, double* unevenTimes,
				int nInterp, double t0interp, double dtNsInterp){


  double* interpWave = (double*) malloc(nInterp*sizeof(double));

  int firstGoodSamp = -1;
  double sum=0;
  int numRealSamp = 0;
  int outInd = 0;
  int sampInd=0;
  double time = t0interp;
  for(sampInd=0; sampInd<nInterp; sampInd++){

    /* If we are inside known time band then interpolate. */
    if(time >= unevenTimes[0] && time <= unevenTimes[nRaw-1]){
      /* interpWave[sampInd] = gsl_interp_eval(interp, unevenTimes, rawWave, time, acc); */

      interpWave[sampInd] = rawWave[outInd];
      outInd++;
      /* interpWave[sampInd] = gsl_spline_eval(spline, time, acc); */
      /* interpWave[sampInd] = rawWave[sampInd]; */
      if(firstGoodSamp < 0){
    	firstGoodSamp = sampInd;
      }
      sum += interpWave[sampInd];
      numRealSamp++;
    }

    /* /\* Zero waveform outside set of known unevenTimes. *\/ */
    else{
      interpWave[sampInd] = 0;
    }

    time += dtNsInterp;
  }

  double mean = sum / numRealSamp;

  for(sampInd = firstGoodSamp; sampInd < firstGoodSamp + numRealSamp; sampInd++){
    interpWave[sampInd] -= mean;
  }



  return interpWave;

}



double* linearlyInterpolateWaveform(int nRaw, double* rawWave, double* unevenTimes,
				    int nInterp, double t0interp, double dtNsInterp){

  double* interpWave = (double*) malloc(nInterp*sizeof(double));

  double time=t0interp;
  int voltSamp=0;
  int unevenInd = 1;
  for(voltSamp=0; voltSamp < nInterp; voltSamp++){

    if(time < unevenTimes[0] || time > unevenTimes[nRaw-1] || unevenInd >= nRaw - 1){
      interpWave[voltSamp] = 0;
    }
    else{
      interpWave[voltSamp] = (time - unevenTimes[unevenInd-1])*(rawWave[unevenInd] - rawWave[unevenInd-1])/(unevenTimes[unevenInd] - unevenTimes[unevenInd-1]) + rawWave[unevenInd-1];

    }

    time += dtNsInterp;
    if(time > unevenTimes[unevenInd]){
      unevenInd++;
    }
  }


  /* float time = t0interp; */
  /* int voltsSamp = 0; */
  /* int t1Ind = 0; */
  /* int t2Ind = 0; */

  /* for(voltsSamp=0; voltsSamp<nInterp; voltsSamp++){ */

  /*   while(unevenTimes[t2Ind] <= time && t2Ind < nRaw-1){ */
  /*     t2Ind++; */
  /*   } */
  /*   t1Ind = t2Ind; */
  /*   while(unevenTimes[t1Ind] >= time && t1Ind > 0){ */
  /*     t1Ind--; */
  /*   } */

  /*   /\* If we are inside known time band, then interpolate. *\/ */
  /*   if(time >= unevenTimes[0] && time <= unevenTimes[nRaw-1]){ */
  /*     interpWave[voltsSamp] = (time - unevenTimes[t1Ind])*(rawWave[t2Ind] - rawWave[t1Ind])/(unevenTimes[t2Ind] - unevenTimes[t1Ind]) + rawWave[t1Ind]; */
  /*   } */

  /*   /\* Zero waveform outside set of known unevenTimes. *\/ */
  /*   else{ */
  /*     interpWave[voltsSamp] = 0; */
  /*   } */

  /*   time += dtNsInterp; */
  /* } */


  return interpWave;
}






/* Jitter in TURF hold being implemented across surfs is fixed by cross correlating clocks */
double findClockJitterCorrection(int numSamples, double* clock1, double* clock2, double deltaT_ns, int surf, int lab){

  if(surf==0){
    /* by definition */
    return 0;
  }


  /* if(surf>0){ */
  /*   /\* by definition *\/ */
  /*   return 0; */
  /* } */

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

      if(nSamps[surf][chan]!=nSamps[surf][8]){
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

  // Set bad event flags to zero
  theHeader->prioritizerStuff = 0;



  // extract SURF numbers and unwrap event
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



  // this puts the data into the volts[][][] global array
  processEventAG(pedSubBody);












  // now we do some checks for self-triggered blasts / SURF saturation

  float topRingPeakToPeak[NUM_POLARIZATIONS][NUM_PHI] = {{0}};
  float bottomRingPeakToPeak[NUM_POLARIZATIONS][NUM_PHI] = {{0}};
  float eventVMax = -1e9;
  float eventVMin = +1e9;
  float eventAsym = 0;

  // Search for saturation and self triggered blasts.
  for(surf=0; surf<ACTIVE_SURFS; surf++){
    int chan=0;
    int samp = 0;
    for(chan=0; chan<8; chan++){
      /* int chanIndex=surf*CHANNELS_PER_SURF + chan; */
      float vMax = -1e9;
      float vMin =  1e9;

      for(samp=0; samp < nSamps[surf][chan]; samp++){
	if(volts[surf][chan][samp] > vMax){
	  vMax = volts[surf][chan][samp];
	}
	if(volts[surf][chan][samp] < vMin){
	  vMin = volts[surf][chan][samp];
	}
      };

      float asym = vMax + vMin;
      float peakToPeak = vMax - vMin;

      int ant = abs(surfToAntMap[surf][chan]) - 1;

      int phi = ant%NUM_PHI;
      int ring = ant/NUM_PHI;
      int pol = surfToAntMap[surf][chan] < 0 ? 1 : 0;

      if(ring==0){
	topRingPeakToPeak[pol][phi] = peakToPeak;
      }
      else if(ring == 2){
	bottomRingPeakToPeak[pol][phi] = peakToPeak;
      }

      // now compare channel numbers to event numbers
      if(fabs(asym) > fabs(eventAsym)){
	eventAsym = asym;
      }
      if(vMax > eventVMax){
	eventVMax = vMax;
      }
      if(vMin < eventVMin){
	eventVMin = vMin;
      }

    }
  }

  int phi=0;
  int maxPhi, maxPol;

  int pol = 0;
  int numRatiosTested = 0;
  float maxBottomToTopPeakToPeakRatio = 0;
  for(pol=0; pol < NUM_POLARIZATIONS; pol++){
    for(phi=0; phi < NUM_PHI; phi++){
      if(bottomRingPeakToPeak[pol][phi]/topRingPeakToPeak[pol][phi] > maxBottomToTopPeakToPeakRatio
	 && !skipBlastRatio[pol][phi]){
	maxBottomToTopPeakToPeakRatio = bottomRingPeakToPeak[pol][phi]/topRingPeakToPeak[pol][phi];
	maxPol = pol;
	maxPhi = phi;
	numRatiosTested++;
      }
    }
  }



  // Set surf saturation flag
  if (fabs(eventAsym) > asymSaturation || eventVMax > positiveSaturation || eventVMin < negativeSaturation){
    theHeader->prioritizerStuff |= 1;
  }


  // Set self triggered blast flag
  if (numRatiosTested > 0 && (maxBottomToTopPeakToPeakRatio > maxThresholdBottomToTopRingPeakToPeakRatio ||
			      maxBottomToTopPeakToPeakRatio < minThresholdBottomToTopRingPeakToPeakRatio) ){
    theHeader->prioritizerStuff |= 2;
  }



  if(theHeader->prioritizerStuff > 0){
    printf("eventNumber %u, vMax %4.2f, vMin %4.2f, vAsym %4.2f, maxPeakToPeakRatio %4.2f, maxPol %d, maxPhi %d, prioritizerStuff %hu\n", theHeader->eventNumber, eventVMax, eventVMin, eventAsym, maxBottomToTopPeakToPeakRatio, maxPol, maxPhi, theHeader->prioritizerStuff);
  }



  /* Upsample clocks */
  double* interpClocks[ACTIVE_SURFS];
  for(surf=0; surf<ACTIVE_SURFS; surf++){
    /* interpClocks[surf] = linearlyInterpolateWaveform(nSamps[surf][8], */
    interpClocks[surf] = interpolateWaveform(nSamps[surf][8],
    /* interpClocks[surf] = dontInterpolateWaveform(nSamps[surf][8], */
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


  double startTime=1e9;
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

      /* finalVolts[surf*CHANNELS_PER_SURF + chan] = linearlyInterpolateWaveform(nSamps[surf][chan], */
      finalVolts[surf*CHANNELS_PER_SURF + chan] = interpolateWaveform(nSamps[surf][chan],
      /* finalVolts[surf*CHANNELS_PER_SURF + chan] = dontInterpolateWaveform(nSamps[surf][chan], */
									  volts[surf][chan],
									  newTimes,
									  NUM_SAMPLES,
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
    syslog(LOG_ERR, "Couldn't find %s, assuming all calibration values = %lf\n", fileName , defaultVal);
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
    syslog(LOG_ERR, "Couldn't find %s, assuming all calibration values = %d\n", fileName , defaultVal);
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
  float defaultVal = 1;

  if( inFile != NULL){
    /* Skip human friendly header, after all I am a robot. */
    int wordInd=0;
    for(wordInd=0; wordInd<5; wordInd++){
      char word[100];
      fscanf(inFile, "%s ", word);
    }
  }
  else{
    fprintf(stderr, "Couldn't find %s, assuming all calibration values = %1.1f\n", fileName , defaultVal);
    syslog(LOG_ERR, "Couldn't find %s, assuming all calibration values = %1.1f\n", fileName , defaultVal);
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
    syslog(LOG_ERR, "Couldn't find %s, assuming all calibration values = %lf\n", fileName , defaultVal);
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
