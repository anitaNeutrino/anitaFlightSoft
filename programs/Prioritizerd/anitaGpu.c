#include "anitaGpu.h"


cl_context context = 0;

size_t dlistSize;
cl_device_id deviceList[2];

cl_command_queue commandQueue = 0;

uint showCompileLog;
cl_uint numDevicesToUse;

cl_program prog = 0;

cl_mem_flags memFlags;

cl_kernel normalizationKernel = 0;

/* I/O buffers */
buffer* rawBufferVPol = NULL;
buffer* numSampsBufferVPol = NULL;
buffer* phiSectorTriggerBufferVPol = NULL;
buffer* rawBufferHPol = NULL;
buffer* numSampsBufferHPol = NULL;
buffer* phiSectorTriggerBufferHPol = NULL;

buffer* numEventsInQueueBuffer = NULL;
buffer* binToBinDifferenceThresh_dBBuffer = NULL;
buffer* absMagnitudeThresh_dBmBuffer = NULL;

/* Internal buffers */
buffer* rmsBuffer = NULL;
buffer* squareBuffer = NULL;
buffer* meanBuffer = NULL;
buffer* normalBuffer = NULL;

cl_kernel fourierKernel = 0;
buffer* fourierBuffer = NULL;
buffer* complexScratchBuffer = NULL;
buffer* numEventsBuffer = NULL;

cl_kernel eventPowSpecKernel = 0;
buffer* powSpecBuffer = NULL;
buffer* passFilterBuffer = NULL;
buffer* longDynamicPassFilterBuffer = NULL;
buffer* staticPassFilterBuffer = NULL;
buffer* powSpecScratchBuffer = NULL;
buffer* isMinimaBuffer = NULL;
buffer* passFilterLocalBuffer = NULL;
buffer* skipUpdatingAvePowSpec = NULL;

cl_kernel filterWaveformsKernel = 0;
buffer* newRmsBuffer = NULL;
buffer* complexSquareLocalBuffer = NULL;

cl_kernel circularCorrelationKernel = 0;
buffer* circularCorrelationBuffer = NULL;
buffer* complexCorrScratch = NULL;

cl_kernel imageKernel = 0;
buffer* imageBuffer = NULL;
buffer* lookupBuffer = NULL;

float* offsetInd;

cl_kernel findImagePeakInThetaKernel = 0;
buffer* corrScratchBuffer4 = NULL;
buffer* indScratchBuffer4 = NULL;
buffer* maxThetaIndsBuffer = NULL;

cl_kernel findImagePeakInPhiSectorKernel = 0;
buffer* imagePeakValBuffer = NULL;
buffer* imagePeakThetaBuffer = NULL;
buffer* imagePeakPhiBuffer = NULL;
buffer* corrScratchBuffer2 = NULL;
buffer* phiIndScratchBuffer = NULL;
buffer* thetaIndScratchBuffer = NULL;

cl_kernel findMaxPhiSectorKernel = 0;
buffer* imagePeakValBuffer2 = NULL;
buffer* imagePeakThetaBuffer2 = NULL;
buffer* imagePeakPhiBuffer2 = NULL;
buffer* imagePeakPhiSectorBuffer = NULL;
buffer* corrScratchBuffer3 = NULL;
buffer* phiIndScratchBuffer2 = NULL;
buffer* thetaIndScratchBuffer2 = NULL;
buffer* phiSectScratchBuffer = NULL;

cl_kernel iFFTFilteredWaveformsKernel = 0;
buffer* complexScratchBuffer2 = NULL;

cl_kernel coherentSumKernel = 0;
buffer* coherentWaveBuffer = NULL;

cl_kernel hilbertKernel = 0;
buffer* hilbertBuffer = NULL;
buffer* complexScratchBuffer3 = NULL;

cl_kernel hilbertPeakKernel = 0;
buffer* hilbertPeakBuffer = NULL;
buffer* scratchBuffer = NULL;
buffer* invFftBuffer = NULL;

struct timeval startTime;
struct timezone dummy;
struct rusage resourcesStart;


/* Globals, to be overwritten by values from Prioritizerd.config */
/* Used to assign priority, should never assign priority 0!*/
float priorityParamsLowBinEdge[NUM_PRIORITIES] = {100, 0.04, 0.05, 0.03, 0.02, 0.01, 0.00, 0, 0, 0};
float priorityParamsHighBinEdge[NUM_PRIORITIES] = {99, 0.05, 1.00, 0.04 ,0.03 ,0.02 ,0.01, 1, 1, 1};


/* GPU option flags that take values from Prioritizerd.config */
int invertTopRingInSoftware = 0;
int printCalibTextFile = 0;

/* Globals, to be overwritten by values from Prioritizerd.config */
/* Used to assign priority, should never assign priority 0!*/
#define MAX_NOTCHES 10
float staticNotchesLowEdgeMHz[MAX_NOTCHES] = {0,0,0,0,0,0,0,0,0,0};
float staticNotchesHighEdgeMHz[MAX_NOTCHES] = {0,0,0,0,0,0,0,0,0,0};


/* Used to map imagePeak and hilbertPeak to a single figure of merit */
float slopeOfImagePeakVsHilbertPeak;
float interceptOfImagePeakVsHilbertPeak;
float absMagnitudeThresh_dBm;


/* Use reconstructed angle to tweak priorities */
float thetaAngleLowForDemotion =-40;
float thetaAngleHighForDemotion =10;
int thetaAnglePriorityDemotion=1;


/* Used for filtering in the GPU */
float binToBinDifferenceThresh_dB;
#define floatToShortConversionForPacket 32767./64.15


const float THETA_RANGE = 150;
const float PHI_RANGE = 22.5;


float* longTimeAvePowSpec = NULL;
int longTimeAvePowSpecNumEvents = 0;
unsigned int longTimeStartTime = 0;

int debugMode = 0;

// For printing calibration numbers to /tmp
FILE* gpuOutput = NULL;

#define NUM_FREQ_BINS_IN_ANITA_BAND 99

void prepareGpuThings(){


  /* Read in GPU output to priority mappings */
  kvpReset();
  KvpErrorCode err = KVP_E_OK;
  int numPriorities = NUM_PRIORITIES;
  err = configLoad ("Prioritizerd.config","prioritizerd") ;
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to load Prioritizerd.config returned %s\n", kvpErrorString(err));
  }
  err = kvpGetFloatArray ("priorityParamsLowBinEdge", priorityParamsLowBinEdge, &numPriorities);
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to do load priorityParamsLowBinEdge from Prioritizerd.config returned %s\n", kvpErrorString(err));
  }

  err = kvpGetFloatArray ("priorityParamsHighBinEdge", priorityParamsHighBinEdge, &numPriorities);
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to do load priorityParamsHighBinEdge from Prioritizerd.config returned %s\n", kvpErrorString(err));
  }

  int maxNotches = MAX_NOTCHES;
  err = kvpGetFloatArray ("staticNotchesLowEdgeMHz", staticNotchesLowEdgeMHz, &maxNotches);
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to do load staticNotchesLowEdgeMHz from Prioritizerd.config returned %s\n", kvpErrorString(err));
  }
  err = kvpGetFloatArray ("staticNotchesHighEdgeMHz", staticNotchesHighEdgeMHz, &maxNotches);
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to do load staticNotchesHighEdgeMHz from Prioritizerd.config returned %s\n", kvpErrorString(err));
  }


  binToBinDifferenceThresh_dB=kvpGetFloat("binToBinDifferenceThresh_dB",5);
  absMagnitudeThresh_dBm=kvpGetFloat("absMagnitudeThresh_dBm",50);
  interceptOfImagePeakVsHilbertPeak = kvpGetFloat ("interceptOfImagePeakVsHilbertPeak", 1715.23327523);
  slopeOfImagePeakVsHilbertPeak = kvpGetFloat ("slopeOfImagePeakVsHilbertPeak", 49.10747809);

  thetaAngleLowForDemotion = kvpGetFloat("thetaAngleLowForDemotion", -40);
  thetaAngleHighForDemotion = kvpGetFloat("thetaAngleHighForDemotion", 10);
  thetaAnglePriorityDemotion = kvpGetInt("thetaAnglePriorityDemotion", 1);

  invertTopRingInSoftware = kvpGetInt("invertTopRingInSoftware", 0);
  /* printf("invertTopRingInSoftware = %d\n", invertTopRingInSoftware); */

  printCalibTextFile = kvpGetInt("printCalibTextFile", 0);


  gpuOutput = NULL;
  if(printCalibTextFile > 0){
    gpuOutput = fopen("/tmp/gpuOutput.dat", "w");
    printf("printCalibTextFile flag is set! Will generate a file /tmp/gpuOutput.dat with lovely things in it.\n");
  }

  /* Large buffers, which will be mapped to GPU memory */
  /* These should have a factor of NUM_POLARIZATIONS in, as we do VPol first, then HPol  */
  /* Both are mapped to V/H */
  numEventSamples = malloc(NUM_POLARIZATIONS*NUM_EVENTS*NUM_ANTENNAS*sizeof(float));
  eventData = malloc(NUM_POLARIZATIONS*NUM_EVENTS*NUM_ANTENNAS*NUM_SAMPLES*sizeof(float));
  powSpec = malloc(NUM_POLARIZATIONS*NUM_ANTENNAS*(NUM_SAMPLES/2)*sizeof(float));
  passFilter = malloc(NUM_POLARIZATIONS*NUM_ANTENNAS*(NUM_SAMPLES/2)*sizeof(short));
  staticPassFilter = malloc((NUM_SAMPLES/2)*sizeof(short));
  prioritizerStuffs = malloc(NUM_EVENTS*sizeof(unsigned short));


  const int numFreqsHacky = NUM_SAMPLES/2;
  int freqInd=0;
  for(freqInd=0; freqInd < numFreqsHacky; freqInd++){
    staticPassFilter[freqInd] = 1;
  }
  const double df = 1e3/(NOMINAL_SAMPLING*NUM_SAMPLES);

  int notchInd = 0;
  for(notchInd = 0; notchInd < MAX_NOTCHES; notchInd++){
    printf("The Prioritizerd has static notch %d has low edge at %.1f MHz and high edge at %.1f MHz\n",
	   notchInd, staticNotchesLowEdgeMHz[notchInd], staticNotchesHighEdgeMHz[notchInd]);

    // first get the bin that contains the low bin edge
    for(freqInd=0; freqInd < numFreqsHacky; freqInd++){
      double f = freqInd*df;
      if(f >= staticNotchesLowEdgeMHz[notchInd] && f < staticNotchesHighEdgeMHz[notchInd]){
	// then we notch this frequency
	staticPassFilter[freqInd] = 0;
      }
    }
  }


  longTimeAvePowSpec = malloc(sizeof(float)*NUM_POLARIZATIONS*NUM_ANTENNAS*NUM_SAMPLES/2);
  longDynamicPassFilter = malloc(sizeof(short)*NUM_POLARIZATIONS*NUM_ANTENNAS*NUM_SAMPLES/2);
  int pol=0;
  for(pol=0; pol<NUM_POLARIZATIONS; pol++){
    int ant=0;
    for(ant = 0; ant < NUM_ANTENNAS; ant++){
      const int longBaseInd = (pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2);
      for(freqInd=0; freqInd < NUM_SAMPLES/2; ++freqInd){
	longDynamicPassFilter[longBaseInd + freqInd] = 1;
      }
    }
  }

  /* Use opencl functions to find out what platforms and devices we can use for this program. */
  myPlatform = 0; /* Here we choose platform 0 in advance of querying to see what's there. */
  myDevice = 0;   /* And device 0... one might have to change these as required. */
  getPlatformAndDeviceInfo(platformIds, maxPlatforms, myPlatform, CL_DEVICE_TYPE_GPU);

  /* Create context (list of platforms & devices which prog can use) */
  cl_context_properties properties[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platformIds[myPlatform], 0};

  context = clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, NULL, NULL, &status);
  statusCheck(status, "clCreateContextFromType");

  status = clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(deviceList), deviceList, &dlistSize);
  statusCheck(status, "clGetContextInfo");

  // create command queue, which is used to send instructions (kernels) to devices (e.g. the GPU)
  commandQueue = clCreateCommandQueue(context, deviceList[myDevice], 0, &status);
  statusCheck(status, "clCreateCommandQueue");

  /*
     At this point the connection to the GPU via the X-server should have been made.
  */

  showCompileLog = 1;
  numDevicesToUse = 1;

  const char* flightSoftDir=getenv("ANITA_FLIGHT_SOFT_DIR");
  char prioritizerDir[FILENAME_MAX];
  sprintf(prioritizerDir, "%s/programs/Prioritizerd", flightSoftDir);

  char clCompileFlags[FILENAME_MAX];
  sprintf(clCompileFlags, "-w -I%s", prioritizerDir);

  char kernelSourceCodeFileName[FILENAME_MAX];
  sprintf(kernelSourceCodeFileName, "%s/programs/Prioritizerd/kernels.cl", flightSoftDir);
  prog = compileKernelsFromSource(kernelSourceCodeFileName, clCompileFlags, context, deviceList, numDevicesToUse, showCompileLog);



  /*
     With the program compiled, we need to pick out the individual kernels by
     their name in the kernels.cl file.
  */

  memFlags=CL_MEM_USE_PERSISTENT_MEM_AMD;






  normalizationKernel = createKernel(prog, "normalizeAndPadWaveforms");
  rawBufferVPol = createBuffer(context, 0, sizeof(float)*NUM_EVENTS*NUM_ANTENNAS*NUM_SAMPLES, "f", "rawBufferVPol");
  rawBufferHPol = createBuffer(context, 0, sizeof(float)*NUM_EVENTS*NUM_ANTENNAS*NUM_SAMPLES, "f", "rawBufferHPol");
  numSampsBufferVPol = createBuffer(context, memFlags, sizeof(short)*NUM_EVENTS*NUM_ANTENNAS, "s","numSampsBufferVPol");
  numSampsBufferHPol = createBuffer(context, memFlags, sizeof(short)*NUM_EVENTS*NUM_ANTENNAS, "s","numSampsBufferHPol");
  rmsBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS*NUM_ANTENNAS, "f","rmsBuffer");
  phiSectorTriggerBufferVPol = createBuffer(context, memFlags, sizeof(short)*NUM_EVENTS*NUM_PHI_SECTORS, "s", "phiSectorTriggerBufferVPol");
  phiSectorTriggerBufferHPol = createBuffer(context, memFlags, sizeof(short)*NUM_EVENTS*NUM_PHI_SECTORS, "s", "phiSectorTriggerBufferHPol");
  squareBuffer = createLocalBuffer(sizeof(float)*NUM_SAMPLES, "squareBuffer");
  meanBuffer = createLocalBuffer(sizeof(float)*NUM_SAMPLES, "meanBuffer");
  normalBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS*NUM_ANTENNAS*NUM_SAMPLES, "f", "normalBuffer");
  #define numNormalArgs 6
  buffer* normalArgs[numNormalArgs] = {numSampsBufferVPol, rmsBuffer, rawBufferVPol, normalBuffer, squareBuffer, meanBuffer};
  setKernelArgs(normalizationKernel, numNormalArgs, normalArgs, "normalizationKernel");









  /* The fourier kernel does a GPU FFT on each channel. */
  fourierKernel = createKernel(prog, "fourierTransformNormalizedData");
  fourierBuffer = createBuffer(context, memFlags, sizeof(float)*2*NUM_EVENTS*NUM_ANTENNAS*NUM_SAMPLES, "f", "fourierBuffer");
  complexScratchBuffer = createLocalBuffer(sizeof(float)*2*NUM_SAMPLES, "complexScratchBuffer");
#define numFourierArgs 3
  buffer* fourierArgs[numFourierArgs] = {normalBuffer, fourierBuffer, complexScratchBuffer};
  setKernelArgs(fourierKernel, numFourierArgs, fourierArgs, "fourierKernel");








  /* Send the read in value of the derivative threshold filter to the GPU */
  binToBinDifferenceThresh_dBBuffer = createBuffer(context, memFlags, sizeof(float), "f", "binToBinDifferenceThresh_dBBuffer");
  copyArrayToGPU(commandQueue, binToBinDifferenceThresh_dBBuffer, &binToBinDifferenceThresh_dB);
  absMagnitudeThresh_dBmBuffer = createBuffer(context, memFlags, sizeof(float), "f", "absMagnitudeThresh_dBmBuffer");
  copyArrayToGPU(commandQueue, absMagnitudeThresh_dBmBuffer, &absMagnitudeThresh_dBm);

  numEventsInQueueBuffer = createBuffer(context, memFlags, sizeof(int), "i", "numEventsInQueueBuffer");

  /* Makes power spectrum from fourier transform and figures out filtering logic... */
  eventPowSpecKernel = createKernel(prog, "makeAveragePowerSpectrumForEvents");
  skipUpdatingAvePowSpec = createBuffer(context, memFlags, sizeof(unsigned short)*NUM_EVENTS, "s", "skipUpdatingAvePowSpec");
  powSpecBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_ANTENNAS*NUM_SAMPLES/2, "f", "powSpecBuffer");
  passFilterBuffer = createBuffer(context, memFlags, sizeof(short)*NUM_ANTENNAS*NUM_SAMPLES/2, "s", "passFilterBuffer");
  longDynamicPassFilterBuffer = createBuffer(context, memFlags, sizeof(short)*NUM_ANTENNAS*NUM_SAMPLES/2, "s", "longDynamicPassFilterBuffer");

  powSpecScratchBuffer = createLocalBuffer(sizeof(float)*NUM_SAMPLES/2, "powSpecScratchBuffer");
  passFilterLocalBuffer = createLocalBuffer(sizeof(short)*NUM_SAMPLES/2, "passFilterLocalBuffer");


#define numPowSpecArgs 10
  buffer* powSpecArgs[numPowSpecArgs] = {skipUpdatingAvePowSpec, fourierBuffer, powSpecBuffer, powSpecScratchBuffer, passFilterBuffer,longDynamicPassFilterBuffer,  binToBinDifferenceThresh_dBBuffer, numEventsInQueueBuffer, passFilterLocalBuffer, absMagnitudeThresh_dBmBuffer};
  setKernelArgs(eventPowSpecKernel, numPowSpecArgs, powSpecArgs, "eventPowSpecKernel");




















  /* filters the waveforms */
  staticPassFilterBuffer = createBuffer(context, memFlags, sizeof(short)*NUM_SAMPLES/2, "s", "staticPassFilterBuffer");
  copyArrayToGPU(commandQueue, staticPassFilterBuffer, staticPassFilter);

  filterWaveformsKernel = createKernel(prog, "filterWaveforms");
  complexSquareLocalBuffer = createLocalBuffer(sizeof(float)*NUM_SAMPLES, "complexSquareLocalBuffer");
  newRmsBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS*NUM_ANTENNAS, "f","newRmsBuffer");


#define numFilterArgs 6
  buffer* filterArgs[numFilterArgs] = {passFilterBuffer, staticPassFilterBuffer, longDynamicPassFilterBuffer, fourierBuffer, complexSquareLocalBuffer, newRmsBuffer};
  setKernelArgs(filterWaveformsKernel, numFilterArgs, filterArgs, "filterWaveformKernel");










  /* Kernel to do (circular) cross-correlations in the fourier domain with the output of the FFTs.*/
  circularCorrelationKernel = createKernel(prog, "crossCorrelationFourierDomain");
  circularCorrelationBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS*NUM_PHI_SECTORS*NUM_GLOBAL_COMBOS_PER_PHI_SECTOR*NUM_SAMPLES, "f", "circularCorrelationBuffer");
  complexCorrScratch = createLocalBuffer(sizeof(float)*2*NUM_SAMPLES, "complexCorrScratch");
#define numCircCorrArgs 4
  buffer* circCorrelationArgs[numCircCorrArgs] = {phiSectorTriggerBufferVPol, fourierBuffer, complexCorrScratch, circularCorrelationBuffer};
  setKernelArgs(circularCorrelationKernel, numCircCorrArgs, circCorrelationArgs, "circularCorrelationKernel");


















  /* Kernel to sum the time domain cross correlations to make in image, only does triggered phi-sectors.*/
  imageKernel = createKernel(prog, "makeImageFromFourierDomain");
  imageBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS*NUM_PHI_SECTORS*NUM_BINS_THETA*NUM_BINS_PHI, "f", "imageBuffer");
  lookupBuffer = createBuffer(context, memFlags,sizeof(float)*NUM_PHI_SECTORS*NUM_LOCAL_COMBOS_PER_PHI_SECTOR*NUM_BINS_THETA*NUM_BINS_PHI, "f", "lookupBuffer");
  /* lookupBuffer = createBuffer(context, memFlags,sizeof(short)*NUM_PHI_SECTORS*NUM_LOCAL_COMBOS_PER_PHI_SECTOR*NUM_BINS_THETA*NUM_BINS_PHI, "s", "lookupBuffer");   */
  offsetInd = fillDeltaTArrays();
  copyArrayToGPU(commandQueue, lookupBuffer, offsetInd);
#define numImageArgs 4
  buffer* imageArgs[numImageArgs] = {phiSectorTriggerBufferVPol, circularCorrelationBuffer, imageBuffer, lookupBuffer};
  setKernelArgs(imageKernel, numImageArgs, imageArgs, "imageKernel");





















  /*
    Kernel to compare the bins in the interferformetric image to find the peak.
    There are lots of comparisons so we can do this in three steps. This is step one.
  */
  findImagePeakInThetaKernel = createKernel(prog, "findImagePeakInTheta");
  corrScratchBuffer4 = createLocalBuffer(sizeof(float)*NUM_BINS_THETA, "corrScratchBuffer4");
  indScratchBuffer4 = createLocalBuffer(sizeof(int)*NUM_BINS_THETA, "indScratchBuffer4");
  maxThetaIndsBuffer = createBuffer(context, memFlags, sizeof(int)*NUM_EVENTS*NUM_PHI_SECTORS*NUM_BINS_PHI, "i", "maxThetaIndsBuffer");

#define numImagePeakThetaArgs 5
  buffer* imagePeakThetaArgs[numImagePeakThetaArgs] = {imageBuffer, phiSectorTriggerBufferVPol, corrScratchBuffer4, indScratchBuffer4, maxThetaIndsBuffer};
  setKernelArgs(findImagePeakInThetaKernel, numImagePeakThetaArgs, imagePeakThetaArgs, "findImagePeakInThetaKernel");














  /*
     Kernel to compare the bins in the interferformetric image to find the peak. This is step two.
  */
  findImagePeakInPhiSectorKernel = createKernel(prog, "findImagePeakInPhiSector");
  imagePeakValBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS*NUM_PHI_SECTORS, "f", "imagePeakValBuffer");
  imagePeakThetaBuffer = createBuffer(context, memFlags, sizeof(int)*NUM_EVENTS*NUM_PHI_SECTORS*NUM_BINS_PHI, "i", "imagePeakThetaBuffer");
  imagePeakPhiBuffer = createBuffer(context, memFlags, sizeof(int)*NUM_EVENTS*NUM_PHI_SECTORS, "i", "imagePeakPhiBuffer");
  corrScratchBuffer2 = createLocalBuffer(sizeof(float)*NUM_BINS_PHI, "corrScratchBuffer2");
  phiIndScratchBuffer = createLocalBuffer(sizeof(int)*NUM_BINS_PHI, "phiIndScratchBuffer");
  thetaIndScratchBuffer = createLocalBuffer(sizeof(int)*NUM_BINS_PHI, "thetaIndScratchBuffer");
#define numFindPeakArgs 8
  buffer* findPeakArgs[numFindPeakArgs] = {maxThetaIndsBuffer,  imageBuffer, corrScratchBuffer2, thetaIndScratchBuffer, phiIndScratchBuffer, imagePeakValBuffer, imagePeakThetaBuffer, imagePeakPhiBuffer};
  setKernelArgs(findImagePeakInPhiSectorKernel, numFindPeakArgs, findPeakArgs, "findImagePeakInPhiSectorKernel");












  /*
    Kernel to compare the image maximum in each phi-sector so we have the "global" image maximum.
    This is step three... phew!
  */
  findMaxPhiSectorKernel = createKernel(prog, "findMaxPhiSector");
  imagePeakValBuffer2 = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS, "f", "imagePeakValBuffer2");
  imagePeakThetaBuffer2 = createBuffer(context, memFlags, sizeof(int)*NUM_EVENTS, "i", "imagePeakThetaBuffer2");
  imagePeakPhiBuffer2 = createBuffer(context, memFlags, sizeof(int)*NUM_EVENTS, "i", "imagePeakPhiBuffer2");
  imagePeakPhiSectorBuffer = createBuffer(context, memFlags, sizeof(int)*NUM_EVENTS, "i", "imagePeakPhiSectorBuffer");
  corrScratchBuffer3 = createLocalBuffer(sizeof(float)*NUM_PHI_SECTORS, "corrScratchBuffer3");
  phiIndScratchBuffer2 = createLocalBuffer(sizeof(int)*NUM_PHI_SECTORS, "phiIndScratchBuffer2");
  thetaIndScratchBuffer2 = createLocalBuffer(sizeof(int)*NUM_PHI_SECTORS,"thetaIndScratchBuffer2");
  phiSectScratchBuffer = createLocalBuffer(sizeof(int)*NUM_PHI_SECTORS,"phiSectScratchBuffer2");
#define numFindMaxPhiSectArgs 11
  buffer* findMaxPhiSectArgs[numFindMaxPhiSectArgs] = {corrScratchBuffer3, phiIndScratchBuffer2, thetaIndScratchBuffer2, phiSectScratchBuffer, imagePeakValBuffer, imagePeakThetaBuffer, imagePeakPhiBuffer, imagePeakValBuffer2, imagePeakThetaBuffer2, imagePeakPhiBuffer2, imagePeakPhiSectorBuffer};
  setKernelArgs(findMaxPhiSectorKernel, numFindMaxPhiSectArgs, findMaxPhiSectArgs, "findMaxPhiSectorKernel");














  /*
    Now we have a direction peak direction we want to coherently sum the waveforms.
    We need a kernel to inverse fourier transform the waveforms in the direction of the peak.
  */
  iFFTFilteredWaveformsKernel = createKernel(prog, "invFourierTransformFilteredWaveforms");
  complexScratchBuffer2 = createLocalBuffer(sizeof(float)*2*NUM_SAMPLES, "complexScratchBuffer2");
  invFftBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS*NUM_ANTENNAS*NUM_SAMPLES, "f", "invFftBuffer");

#define numInvFFTArgs 4
  buffer* invFFTArgs[numInvFFTArgs] = {invFftBuffer, fourierBuffer, complexScratchBuffer2, imagePeakPhiSectorBuffer};
  /* buffer* invFFTArgs[numInvFFTArgs] = {normalBuffer, fourierBuffer, complexScratchBuffer2, imagePeakPhiSectorBuffer};   */
  setKernelArgs(iFFTFilteredWaveformsKernel, numInvFFTArgs, invFFTArgs, "iFFTFilteredWaveformsKernel");














  /*
    Kernel to sum the inverse fourier transformed waveforms with the offset implied by the image peak.
  */
  coherentSumKernel = createKernel(prog, "coherentlySumWaveForms");
  coherentWaveBuffer = createBuffer(context, 0, sizeof(float)*NUM_EVENTS*NUM_SAMPLES, "f", "coherentWaveBuffer");
#define numCoherentArgs 7
  buffer* coherentArgs[numCoherentArgs] = {imagePeakThetaBuffer2, imagePeakPhiBuffer2, imagePeakPhiSectorBuffer, invFftBuffer, coherentWaveBuffer, newRmsBuffer, lookupBuffer};
  /* buffer* coherentArgs[numCoherentArgs] = {imagePeakThetaBuffer2, imagePeakPhiBuffer2, imagePeakPhiSectorBuffer, normalBuffer, coherentWaveBuffer, rmsBuffer, lookupBuffer};   */
  setKernelArgs(coherentSumKernel, numCoherentArgs, coherentArgs, "coherentSumKernel");















  /*
    Kernel to create hilbert envelope from coherently summed waveform.
  */
  hilbertKernel = createKernel(prog, "makeHilbertEnvelope");
  hilbertBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS*NUM_SAMPLES, "f", "hilbertBuffer");
  complexScratchBuffer3 = createLocalBuffer(sizeof(float)*2*NUM_SAMPLES, "complexScratchBuffer3");
#define numHilbertArgs 3
  buffer* hilbertArgs[numHilbertArgs] = {coherentWaveBuffer, complexScratchBuffer3, hilbertBuffer};
  setKernelArgs(hilbertKernel, numHilbertArgs, hilbertArgs, "hilbertKernel");

  /* Find the peak values of the coherently summed waveform */
  hilbertPeakKernel = createKernel(prog, "findHilbertEnvelopePeak");
  hilbertPeakBuffer = createBuffer(context, memFlags, sizeof(float)*NUM_EVENTS, "f", "hilbertPeakPeakBuffer");
  scratchBuffer = createLocalBuffer(sizeof(float)*NUM_SAMPLES/4, "scratchBuffer");
#define numHilbertPeakArgs 3
  buffer* hilbertPeakArgs[numHilbertPeakArgs] = {hilbertBuffer, scratchBuffer, hilbertPeakBuffer};
  setKernelArgs(hilbertPeakKernel, numHilbertPeakArgs, hilbertPeakArgs, "hilbertPeakKernel");






#ifdef TSTAMP
  TS_OPEN(4000,1);
  TS_START(1);
#endif



  gettimeofday(&startTime,&dummy);
  if(getrusage(RUSAGE_SELF, &resourcesStart)){
    perror("Error from getrusage");
  }

}











void addEventToGpuQueue(int eventInd, double* finalVolts[], AnitaEventHeader_t header){

  int antInd=0;
  /* For each channel, copy the data into the places where the GPU expects it. */

  for(antInd=0; antInd<NUM_ANTENNAS; antInd++){
    int vPolChanInd = antToSurfMap[antInd]*9 + vAntToChan[antInd];
    int hPolChanInd = antToSurfMap[antInd]*9 + hAntToChan[antInd];

    int ringFactor = 1;
    if(invertTopRingInSoftware > 0 && antInd < NUM_PHI){
      ringFactor = -1;
    }

    /* Lay out the vpol data in the 1st half of the arrays and hpol data in the second half...*/
    int samp=0;
    for(samp=0; samp<NUM_SAMPLES; samp++){
      eventData[eventInd*NUM_ANTENNAS*NUM_SAMPLES + antInd*NUM_SAMPLES + samp] = ringFactor*(float)finalVolts[vPolChanInd][samp];

      /* printf("%f, ", eventData[eventInd*NUM_ANTENNAS*NUM_SAMPLES + antInd*NUM_SAMPLES + samp]); */

    }
    /* printf("\n\n"); */

    numEventSamples[eventInd*NUM_ANTENNAS + antInd] = NUM_SAMPLES;



    for(samp=0; samp<NUM_SAMPLES; samp++){

      eventData[(NUM_EVENTS+eventInd)*NUM_ANTENNAS*NUM_SAMPLES + antInd*NUM_SAMPLES + samp] = ringFactor*(float)finalVolts[hPolChanInd][samp];
    }
    numEventSamples[(NUM_EVENTS+eventInd)*NUM_ANTENNAS + antInd] = NUM_SAMPLES;
  }

  prioritizerStuffs[eventInd] = header.prioritizerStuff;

  /* Unpack the "L3" trigger, actually the L2 triggered phi-sectors. */
  int phiInd=0;
  for(phiInd=0; phiInd<NUM_PHI_SECTORS; phiInd++){
    /* VPOL */
    phiTrig[eventInd*NUM_PHI_SECTORS + phiInd] = 1 & (header.turfio.l3TrigPattern >> phiInd);
    /* HPOL, goes in back half of array */
    phiTrig[(NUM_EVENTS+eventInd)*NUM_PHI_SECTORS + phiInd] = 1 & (header.turfio.l3TrigPatternH>>phiInd);
  }
}
















void mainGpuLoop(int nEvents, AnitaEventHeader_t* header, GpuPhiSectorPowerSpectrumStruct_t* payloadPowSpec, int writePowSpecPeriodSeconds){

  int phi=0;
  for(phi=0; phi<NUM_PHI_SECTORS; phi++){
    if(payloadPowSpec[phi].unixTimeFirstEvent==0){
      payloadPowSpec[phi].phiSector=phi;
      payloadPowSpec[phi].firstEventInAverage = header[0].eventNumber;
      payloadPowSpec[phi].unixTimeFirstEvent = header[0].unixTime;
    }
  }

  if(longTimeAvePowSpecNumEvents==0){
    longTimeStartTime = header[0].unixTime;
  }


  /* Start time stamping. */
  uint stamp;
  stamp = 1;
  timeStamp(stamp++, 0, NULL);

  cl_event writeNumEventsToGPU = writeBuffer(commandQueue, numEventsInQueueBuffer,
					     &nEvents, 0, NULL);
  timeStamp(stamp++, 0, NULL);

  /*
     In order to minimize time, maximize asynchronization of data transfer by sending both
     polarizations off to the GPU before starting calculations with them.
  */


  cl_event prioritizerStuffToGpu = writeBuffer(commandQueue, skipUpdatingAvePowSpec,
					       prioritizerStuffs, 0, NULL);

  cl_event dataToGpuEvents[NUM_POLARIZATIONS][4];

  dataToGpuEvents[0][0] = writeBuffer(commandQueue, rawBufferVPol,
				      eventData, 0, NULL);
  dataToGpuEvents[0][1] = writeBuffer(commandQueue, numSampsBufferVPol,
				      numEventSamples, 0, NULL);
  dataToGpuEvents[0][2] = writeBuffer(commandQueue, phiSectorTriggerBufferVPol,
				      phiTrig, 0, NULL);
  dataToGpuEvents[0][3] = writeBuffer(commandQueue, longDynamicPassFilterBuffer,
				      longDynamicPassFilter, 0, NULL);


  dataToGpuEvents[1][0] = writeBuffer(commandQueue, rawBufferHPol,
				     &eventData[NUM_EVENTS*NUM_ANTENNAS*NUM_SAMPLES], 0, NULL);
  dataToGpuEvents[1][1] = writeBuffer(commandQueue, numSampsBufferHPol,
				     &numEventSamples[NUM_EVENTS*NUM_ANTENNAS], 0, NULL);
  dataToGpuEvents[1][2] = writeBuffer(commandQueue, phiSectorTriggerBufferHPol,
				     &phiTrig[NUM_EVENTS*NUM_PHI_SECTORS], 0, NULL);
  dataToGpuEvents[1][3] = writeBuffer(commandQueue, longDynamicPassFilterBuffer,
				      &longDynamicPassFilter[NUM_ANTENNAS*NUM_SAMPLES/2], 0, NULL);


  cl_event dataFromGpuEvents[NUM_POLARIZATIONS][7];

  /* Loop over both polarizations */
  uint polInd=0;
  for(polInd = 0; polInd < NUM_POLARIZATIONS; polInd++){

    /*
      I have set the system up to have two buffers, I need to change between these buffers,
      so the GPU does the interferometry stuff on the buffer containing data and while the system
      copies memory to the unused one. Asynchronous memory transfers are fun!
    */

    /* Only change required is directing the GPU kernels to a different place in GPU memory to get the data. */
    if(polInd == 0){
      setKernelArg(normalizationKernel, 0, numSampsBufferVPol, "normalizationKernel");
      setKernelArg(normalizationKernel, 2, rawBufferVPol, "normalizationKernel");
      setKernelArg(circularCorrelationKernel, 0, phiSectorTriggerBufferVPol, "circularCorrelationKernel");
      setKernelArg(imageKernel, 0, phiSectorTriggerBufferVPol, "imageKernel");
      setKernelArg(findImagePeakInThetaKernel, 1, phiSectorTriggerBufferVPol, "findImagePeakInThetaKernel");
    }
    else{
      setKernelArg(normalizationKernel, 0, numSampsBufferHPol, "normalizationKernel");
      setKernelArg(normalizationKernel, 2, rawBufferHPol, "normalizationKernel");
      setKernelArg(circularCorrelationKernel, 0, phiSectorTriggerBufferHPol, "circularCorrelationKernel");
      setKernelArg(imageKernel, 0, phiSectorTriggerBufferHPol, "imageKernel");
      setKernelArg(findImagePeakInThetaKernel, 1, phiSectorTriggerBufferHPol, "findImagePeakInThetaKernel");
    }
    timeStamp(stamp++, 3, &dataToGpuEvents[polInd][0]);


    /* Normalization zero-means the waveform and sets RMS=1. */
    status = clEnqueueNDRangeKernel(commandQueue, normalizationKernel, 3, NULL,
				    gNormWorkSize, lNormWorkSize, 3,
				    &dataToGpuEvents[polInd][0], &normalEvent);
    statusCheck(status, "clEnqueueNDRangeKernel normalizationKernel");
    timeStamp(stamp++, 1, &normalEvent);



    /* Fourier transforms the normalized data. */
    status = clEnqueueNDRangeKernel(commandQueue, fourierKernel, 3, NULL,
				    gFourierWorkSize, lFourierWorkSize, 1, &normalEvent, &fourierEvent);
    statusCheck(status, "clEnqueueNDRangeKernel fourierKernel");
    timeStamp(stamp++, 1, &fourierEvent);

    cl_event powSpecDependencies[3] = {writeNumEventsToGPU, fourierEvent, prioritizerStuffToGpu};
    status = clEnqueueNDRangeKernel(commandQueue, eventPowSpecKernel, 2, NULL, gPowSpecWorkSize, lPowSpecWorkSize, 3, powSpecDependencies, &powSpecEvent);
    statusCheck(status, "clEnqueueNDRangeKernel eventPowSpecKernel");
    timeStamp(stamp++, 1, &powSpecEvent);

    /* Copy the power spectrum + other info back the CPU */
    dataFromGpuEvents[polInd][5] = readBuffer(commandQueue, powSpecBuffer,
					      &powSpec[polInd*NUM_ANTENNAS*NUM_SAMPLES/2],
					      1, &powSpecEvent);
    timeStamp(stamp++, 1, &dataFromGpuEvents[polInd][5]);

    /* Filters frequency bins based on the steepness of the power spectra */
    status = clEnqueueNDRangeKernel(commandQueue, filterWaveformsKernel, 3, NULL, gFilterWorkSize, lFilterWorkSize, 1, &powSpecEvent, &filterEvent);
    statusCheck(status, "clEnqueueNDRangeKernel filterWaveformsKernel");
    timeStamp(stamp++, 1, &filterEvent);

    /* Copies the filtering numbers back from the GPU */
    dataFromGpuEvents[polInd][6] = readBuffer(commandQueue, passFilterBuffer,
					      &passFilter[polInd*NUM_ANTENNAS*NUM_SAMPLES/2],
					      1, &filterEvent);
    timeStamp(stamp++, 1, &dataFromGpuEvents[polInd][6]);

    /* Does the cross-correlation in the fourier domain with the normalized fourier transformed data. */
    status = clEnqueueNDRangeKernel(commandQueue, circularCorrelationKernel, 3, NULL, gCircCorrWorkSize, lCircCorrWorkSize, 1, &filterEvent, &circCorrelationEvent);
    statusCheck(status, "clEnqueueNDRangeKernel circularCorrelationKernel");
    timeStamp(stamp++, 1, &circCorrelationEvent);



    /* Sums the cross-correlations into an "interferometric image" */
    status = clEnqueueNDRangeKernel(commandQueue, imageKernel, 3, NULL, gImageWorkSize, lImageWorkSize, 1, &circCorrelationEvent, &imageEvent);
    statusCheck(status, "clEnqueueNDRangeKernel imageKernel");
    timeStamp(stamp++, 1, &imageEvent);

    /* Looks at the image: for each row in phi, find the location and value of the maximum value in that row. */
    status = clEnqueueNDRangeKernel(commandQueue, findImagePeakInThetaKernel, 3, NULL, gImageThetaWorkSize, lImageThetaWorkSize, 1, &imageEvent, &findImagePeakInThetaEvent);
    statusCheck(status, "clEnqueueNDRangeKernel findImagePeakInTheta");
    timeStamp(stamp++, 1, &findImagePeakInThetaEvent);

    /* For each of those maxes in theta, compare across the bins in phi within each phi-sector */
    status = clEnqueueNDRangeKernel(commandQueue, findImagePeakInPhiSectorKernel, 3, NULL, gFindImagePeakWorkSize, lFindImagePeakWorkSize, 1, &findImagePeakInThetaEvent, &findImagePeakInPhiSectorEvent);
    statusCheck(status, "clEnqueueNDRangeKernel findImagePeakInPhiSectorKernel");
    timeStamp(stamp++, 1, &findImagePeakInPhiSectorEvent);

    /* Compare each maximum in theta-phi in a phi-sector to find the global image maximum. */
    status = clEnqueueNDRangeKernel(commandQueue, findMaxPhiSectorKernel, 2, NULL, gFindImagePeakWorkSize2, lFindImagePeakWorkSize2, 1, &findImagePeakInPhiSectorEvent, &findMaxPhiSectorEvent);
    statusCheck(status, "clEnqueueNDRangeKernel findMaxPhiSectorEvent");
    timeStamp(stamp++, 1, &findMaxPhiSectorEvent);

    /* Copy those results back to the CPU (non-blocking) */
    dataFromGpuEvents[polInd][0] = readBuffer(commandQueue, imagePeakValBuffer2,
					      &imagePeakVal[polInd*NUM_EVENTS],
					      1, &findMaxPhiSectorEvent);
    dataFromGpuEvents[polInd][1] = readBuffer(commandQueue, imagePeakThetaBuffer2,
					      &imagePeakTheta2[polInd*NUM_EVENTS],
					      1, &findMaxPhiSectorEvent);
    dataFromGpuEvents[polInd][2] = readBuffer(commandQueue, imagePeakPhiBuffer2,
					      &imagePeakPhi2[polInd*NUM_EVENTS],
					      1, &findMaxPhiSectorEvent);
    dataFromGpuEvents[polInd][3] = readBuffer(commandQueue, imagePeakPhiSectorBuffer,
					      &imagePeakPhiSector[polInd*NUM_EVENTS],
					      1, &findMaxPhiSectorEvent);
    timeStamp(stamp++, 4, &dataFromGpuEvents[polInd][0]);

    /* Now we know the phi-sector maximum, inverse fourier transform the waveforms from that region */
    status = clEnqueueNDRangeKernel(commandQueue, iFFTFilteredWaveformsKernel, 3, NULL, gInvFFTSize, lInvFFTSize, 1, &findMaxPhiSectorEvent, &invFFTWaveformsEvent);
    statusCheck(status, "clEnqueueNDRangeKernel iFFTFilteredWaveformsKernel");
    timeStamp(stamp++, 1, &invFFTWaveformsEvent);

    /* Coherently sum the waveforms with the delta-t implied by the peak image location */
    status = clEnqueueNDRangeKernel(commandQueue, coherentSumKernel, 2, NULL, gCoherentSumKernelWorkSize, lCoherentSumKernelWorkSize, 1, &invFFTWaveformsEvent, &coherentSumEvent);
    statusCheck(status, "clEnqueueNDRangeKernel coherentSumKernel");
    timeStamp(stamp++, 1, &coherentSumEvent);

    /* Calculate the hilbert envelope of the coherently summed waveform */
    status = clEnqueueNDRangeKernel(commandQueue, hilbertKernel, 2, NULL, gHilbertWorkSize, lHilbertWorkSize, 1, &coherentSumEvent, &hilbertEvent);
    statusCheck(status, "clEnqueueNDRangeKernel hilbertKernel");
    timeStamp(stamp++,1, &hilbertEvent);

    /* Find the peak value of the hilbert envelope */
    status = clEnqueueNDRangeKernel(commandQueue, hilbertPeakKernel, 2, NULL, gHilbertPeakWorkSize, lHilbertPeakWorkSize, 1, &hilbertEvent, &hilbertPeakEvent);
    statusCheck(status, "clEnqueueNDRangeKernel findHilbertPeak");
    timeStamp(stamp++, 1, &hilbertPeakEvent);

    /* Copy hilbert envelope peak value back to the CPU */
    dataFromGpuEvents[polInd][4] = readBuffer(commandQueue, hilbertPeakBuffer,
					      &hilbertPeak[polInd*NUM_EVENTS],
					      1, &hilbertPeakEvent);
    timeStamp(stamp++, 1, &dataFromGpuEvents[polInd][4]);







    /*
      Heavy handed debugging! Dump all the data from every buffer into text files.
      This is insanely slow so will exit the program after the main gpu function is called once.
    */

    if(debugMode > 0){
      clFinish(CommandQueue);
      dumpBuffersToTextFiles(polInd);
    }
  }

  /* Since all copy GPU-to-CPU buffer reading commands are non-blocking we actually need to wait... */
  clWaitForEvents(14, &dataFromGpuEvents[0][0]);
  timeStamp(stamp++, 14, &dataFromGpuEvents[0][0]);


  int eventInd=0;
  for(eventInd=0; eventInd < nEvents; eventInd++){
    /* Pick better polarization */
    float imagePeakV = imagePeakVal[eventInd];
    float imagePeakH = imagePeakVal[NUM_EVENTS + eventInd];

    short polBit;
    unsigned int index2;
    /* 0 is hpol, 1 is vpol */
    if(imagePeakV >= imagePeakH){
      polBit = 1;
      index2 = eventInd;
    }
    else{
      polBit = 0;
      index2 = eventInd + NUM_EVENTS;
    }

    int threshFlag = 0;
    int diffFlag = 0;

    /* printf("anitaGpu start... header[eventInd].prioritizerStuff= %hu\n", header[eventInd].prioritizerStuff); */

    /* Read previously set lowest bit saturation flag */
    unsigned short saturationFlag = (header[eventInd].prioritizerStuff & 1);
    unsigned short blastFlag = (header[eventInd].prioritizerStuff >> 1);
    header[eventInd].prioritizerStuff = 0;

    /* Value goes between 0->1, get 16 bit precision by multiplying by maximum value of unsigned short... */
    header[eventInd].imagePeak = (unsigned short)(imagePeakVal[index2]*65535);
    header[eventInd].coherentSumPeak = (unsigned short) hilbertPeak[index2];
    header[eventInd].peakThetaBin = (unsigned char) imagePeakTheta2[index2];
    /* float thetaDegPeak = -1*THETA_RANGE*((double)header[eventInd].peakThetaBin/NUM_BINS_THETA - 0.5); */
    float thetaDegPeak = THETA_RANGE*((double)header[eventInd].peakThetaBin/NUM_BINS_THETA - 0.5);

    /* Only need 10 bits for this number. */
    header[eventInd].prioritizerStuff |= (0x3ff & ((unsigned short) imagePeakPhiSector[index2]*NUM_BINS_PHI + imagePeakPhi2[index2]));

    /* Left bit shift by 1, then set lowest bit as polarization bit */
    /* Now at 0x7fff */
    header[eventInd].prioritizerStuff <<= 1;
    header[eventInd].prioritizerStuff |= polBit;
    /* Still have 5 bits remaining, going to use up two more... */
    header[eventInd].prioritizerStuff |= (threshFlag << 12);
    header[eventInd].prioritizerStuff |= (diffFlag << 13);
    /* Now 3 bits remaining...! */
    header[eventInd].prioritizerStuff |= (saturationFlag << 14);
    /* Now 2 bits remaining */
    header[eventInd].prioritizerStuff |= (blastFlag << 15);


    /* Now actually assign the priority in the header file*/
    float higherImagePeak = imagePeakV > imagePeakH ? imagePeakV : imagePeakH;
    float normalizedHilbertPeak = (hilbertPeak[index2] - interceptOfImagePeakVsHilbertPeak)/slopeOfImagePeakVsHilbertPeak;
    float priorityParam = sqrt(normalizedHilbertPeak*normalizedHilbertPeak + higherImagePeak*higherImagePeak);

    /* printf("Interferometric figures of merit... %f \t %f \t %f \t %f \n", higherImagePeak, hilbertPeak[index2], normalizedHilbertPeak, priorityParam); */

    int priority = 0;
    if(saturationFlag > 0){
      /* Found saturation when unwrapping */
      priority = 9;
      /* printf("Found saturation\n"); */
    }
    if(blastFlag > 0){
      priority = 8;
      /* printf("Blast Flag\n"); */
    }
    /* else if(diffFlag==1 || threshFlag==1){ */
    /*   /\* There was CW in these events... not an optimal solution...*\/ */
    /*   /\* priority = 8; *\/ */
    /*   /\* When CW is filtered out go up one in priority  *\/ */
    /*   printf("diffFlag = %d, threshFlag = %d\n", diffFlag, threshFlag); */
    /*   priority++; */
    /* } */
    else{
      for(priority=1; priority<NUM_PRIORITIES; priority++){
	if(priorityParam >= priorityParamsLowBinEdge[priority] && priorityParam<priorityParamsHighBinEdge[priority]){

	  /* printf("Found my priority! %f is between %f and %f at priority %d\n", */
	  /* 	 priorityParam, */
	  /* 	 priorityParamsLowBinEdge[priority], */
	  /* 	 priorityParamsHighBinEdge[priority], */
	  /* 	 priority); */
	  break;
	}
      }
    }

    /* printf("(header[eventInd].prioritizerStuff & 0x4000)>>14 = %hu\n", (header[eventInd].prioritizerStuff & 0x4000)>>14); */
    /* printf("saturationFlag = %hu\n", saturationFlag); */
    /* printf("priority = %d\n", priority); */

    /* Finally tweak priority based on reconstructed angle .. */
    if((thetaDegPeak > thetaAngleHighForDemotion || thetaDegPeak < thetaAngleLowForDemotion) && priority <6){
      priority += thetaAnglePriorityDemotion;
    }

    if(priority<=0 || priority >= 10){ /* Check I didn't mess up... */
      char errorString[FILENAME_MAX];
      sprintf(errorString, "Something went wrong with priority assignment for event number %u. (It got assigned priority %d. I will assign priority 9.\n", header[eventInd].eventNumber, priority);

      // inform the world of your mistakes
      fprintf(stderr, "%s", errorString);
      syslog(LOG_ERR, "%s", errorString);
      priority = 9;
    }

    header[eventInd].priority = priority;

    /* printf("eventNumber %u, saturationFlag %hu, priority %d\n", header[eventInd].eventNumber, saturationFlag, priority); */
    /* printf("I assigned eventNumber %u priority %d\n", header[eventInd].eventNumber, priority); */

    if(printCalibTextFile > 0){
      fprintf(gpuOutput, "%u %f %f %d %d %d %lf %lf %lf %d ",
	      header[eventInd].eventNumber,
	      imagePeakVal[index2],
	      hilbertPeak[index2],
	      imagePeakTheta2[index2],
	      imagePeakPhi2[index2],
	      imagePeakPhiSector[index2],
	      priorityParam,
	      normalizedHilbertPeak,
	      higherImagePeak,
	      priority);

      fprintf(gpuOutput, "\n");
    }
  }




  // it does seem to be rather retarded than the CPU and GPU both calculate this normalization...
  // but it's probably not the end of the world
  int numToSkip = 0;
  for(eventInd=0; eventInd < nEvents; eventInd++){
    if(prioritizerStuffs[eventInd] > 0){
      numToSkip++;
    }
  }


  const int numEventsInPowSpec = nEvents - numToSkip;
  // Here we add the power spectra calcualated on this GPU loop to the rolling average
  for(polInd=0; polInd < NUM_POLARIZATIONS; polInd++){
    int ant = 0;
    for(ant=0; ant < NUM_ANTENNAS; ant++){
      int freqInd = 0;
      for(freqInd = 0; freqInd < NUM_SAMPLES/2; freqInd++){
	if(isinf(powSpec[(polInd*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd]) || isnan(powSpec[(polInd*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd])){
	  if(isinf(powSpec[(polInd*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd])){
	    printf("I got an inf!!! at %d %d %d \n", polInd, ant, freqInd);
	  }
	  else if(isnan(powSpec[(polInd*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd])){
	    printf("I got a nan!!! at %d %d %d \n", polInd, ant, freqInd);
	  }
	  /* printf("Let's find the bastard. I dumped all the buffers and I'm quitting!\n"); */
	  /* handleBadSigs(); */
	}

	longTimeAvePowSpec[(polInd*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd] += numEventsInPowSpec*powSpec[(polInd*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd];
      }
    }
  }

  longTimeAvePowSpecNumEvents += (numEventsInPowSpec);


  const int longTime = 28;

  if(header[nEvents-1].unixTime - longTimeStartTime >= longTime || debugMode > 0){
    FILE* fOut = fopen("/tmp/longTimePowSpec.txt", "w");
    int pol = 0;
    for(pol=0; pol < NUM_POLARIZATIONS; pol++){
      int ant = 0;
      for(ant=0; ant < NUM_ANTENNAS; ant++){
        int freqInd = 0;
	for(freqInd = 0; freqInd < NUM_SAMPLES/2; freqInd++){
	  longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd]/=longTimeAvePowSpecNumEvents;

	  const float ohmTermination  = 50;
	  longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd]/=ohmTermination;

	  const float deltaF = 1e3/(NUM_SAMPLES*NOMINAL_SAMPLING);

	  longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd]*=deltaF;

	  // convert to dB, do I want to do this?
	  float temp = longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*NUM_SAMPLES/2 + freqInd];
	  if(temp > 0){
	    temp = 10*log10(temp);
	  }
	  else{
	    temp = -40;
	  }
	  longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + freqInd] = temp;

	  fprintf(fOut, "%f ", temp);
	}
	/* fprintf(fOut, "\n"); */


	const float diffThresh_dB = 1; // CONFIG
	const float spikeThresh_dB = 2; // CONFIG
	const float rollOffFrequency_MHz = 600; // CONFIG

	// find local maxima and local minima of the power spectrum
	int isLocalMinimum[NUM_SAMPLES/2] = {0};
	int isLocalMaximum[NUM_SAMPLES/2] = {0};
	int isAboveDiffThresh[NUM_SAMPLES/2] = {0};

	float y0 = longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2)];
	float y1 = longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + 1];
	isLocalMinimum[0] = y0 < y1 ? 1 : 0;
	isLocalMaximum[0] = y0 > y1 ? 1 : 0;
	isAboveDiffThresh[0] = y1 - y0 > diffThresh_dB ? 1 : 0;

	for(freqInd=1; freqInd < NUM_SAMPLES/2 - 1; freqInd++){
	  float y0 = longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + freqInd - 1];
	  float y1 = longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + freqInd];
	  float y2 = longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + freqInd + 1];

	  isLocalMaximum[freqInd] = (y1 > y0 && y1 > y2) ? 1 : 0;
	  isLocalMinimum[freqInd] = (y1 < y0 && y1 < y2) ? 1 : 0;

	  isAboveDiffThresh[freqInd] = y2 - y1 > diffThresh_dB ? 1 : 0;
	}

	y0 = longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + NUM_SAMPLES/2 - 2];
	y1 = longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + NUM_SAMPLES/2 - 1];
	isLocalMinimum[NUM_SAMPLES/2 - 1] = y0 > y1 ? 1 : 0;
	isLocalMaximum[NUM_SAMPLES/2 -1] = y0 < y1 ? 1 : 0;

	isAboveDiffThresh[NUM_SAMPLES/2 - 1] = 0; // this point must be zero








	for(freqInd=0; freqInd < NUM_SAMPLES/2; freqInd++){
	  fprintf(fOut, "%f ", isLocalMaximum[freqInd]*longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + freqInd]);
	}

	for(freqInd=0; freqInd < NUM_SAMPLES/2; freqInd++){
	  fprintf(fOut, "%f ", isLocalMinimum[freqInd]*longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + freqInd]);
	}

	for(freqInd=0; freqInd < NUM_SAMPLES/2; freqInd++){
	  fprintf(fOut, "%f ", isAboveDiffThresh[freqInd]*longTimeAvePowSpec[(pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2) + freqInd]);
	}








	// every frequency bin passes (=1) by default.
	// I'm setting that condition here
	const int longBaseInd = (pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2);
	for(freqInd=0; freqInd < NUM_SAMPLES/2; ++freqInd){
	  longDynamicPassFilter[longBaseInd + freqInd] = 1;
	}


	/* const float deltaF_MHz = 1e3/(NUM_SAMPLES*NOMINAL_SAMPLING); */
	/* int startInd = floor(rollOffFrequency_MHz/deltaF_MHz); */
	/* int endInd = NUM_SAMPLES/2; */
	int startInd = 0;
	int endInd = NUM_SAMPLES/2;

	updateLongDynamicPassFilter(pol, ant, longDynamicPassFilter, isLocalMinimum, isLocalMaximum,  spikeThresh_dB, startInd, endInd);

	/* startInd = startInd + 1; */
	/* endInd = 0; */
	/* updateLongDynamicPassFilter(pol, ant, longDynamicPassFilter, isLocalMinimum, isLocalMaximum,  spikeThresh_dB, startInd, endInd); */


	for(freqInd=0; freqInd < NUM_SAMPLES/2; freqInd++){
	  fprintf(fOut, "%f ", longDynamicPassFilter[longBaseInd+freqInd]*longTimeAvePowSpec[longBaseInd + freqInd]);
	}

	fprintf(fOut, "\n");
      }
    }

    fclose(fOut);
    printf("There were %d events in this longTime GPU average power spectrum\n", longTimeAvePowSpecNumEvents);
    handleBadSigs();
  }



  /* if(payloadPowSpec[0].unixTimeLastEvent - payloadPowSpec[0].unixTimeFirstEvent >= writePowSpecPeriodSeconds){ */
  /*   for(polInd=0; polInd<2; polInd++){ */
  /*     int phi=0; */
  /*     for(phi=0; phi<NUM_PHI_SECTORS; phi++){ */
  /* 	int ring=0; */
  /* 	for(ring=0; ring<NUM_ANTENNA_RINGS; ring++){ */
  /* 	  int antInd = phi + NUM_PHI_SECTORS*ring; */
  /* 	  int bin=0; */
  /* 	  for(bin=0; bin<NUM_FREQ_BINS_IN_ANITA_BAND; bin++){ */
  /* 	    payloadPowSpec[phi].powSpectra[ring][polInd].bins[bin] = (short) (floatToShortConversionForPacket*10*log10(tempPowSpecHolder[polInd*NUM_ANTENNAS*NUM_FREQ_BINS_IN_ANITA_BAND + antInd*NUM_FREQ_BINS_IN_ANITA_BAND + bin]/50)); */
  /* 	  } */
  /* 	} */
  /*     } */
  /*   } */
  /* } */



  /* /\* Add information to gpuPowSpec packet *\/ */
  /* int offset=20; */
  /* for(polInd=0; polInd<2; polInd++){ */
  /*   int phi=0; */
  /*   for(phi=0; phi<NUM_PHI_SECTORS; phi++){ */
  /*     int ring=0; */
  /*     for(ring=0; ring<NUM_ANTENNA_RINGS; ring++){ */
  /* 	int antInd = phi + NUM_PHI_SECTORS*ring; */
  /* 	int bin=0; */
  /* 	for(bin=0; bin<NUM_FREQ_BINS_IN_ANITA_BAND; bin++){ */
  /* 	  /\* tempPowSpecHolder[polInd*NUM_ANTENNAS*NUM_FREQ_BINS_IN_ANITA_BAND + antInd*NUM_FREQ_BINS_IN_ANITA_BAND + bin]+= (nEvents*powSpec[(polInd*NUM_ANTENNAS + antInd)*NUM_SAMPLES/2 + offset + bin]); *\/ */
  /* 	} */
  /*     } */
  /*   } */
  /* } */
  /* for(phi=0; phi<NUM_PHI_SECTORS; phi++){ */
  /*   payloadPowSpec[phi].unixTimeLastEvent = header[nEvents-1].unixTime; */
  /*   payloadPowSpec[phi].numEventsAveraged += nEvents; */
  /* } */

  /* timeStamp(stamp++, 0, NULL); */


  if(debugMode > 0){
    printf("debugMode flag = %d.\n", debugMode);
    printf("Exiting program.\n");

    if(gpuOutput!=NULL){
      fclose(gpuOutput);
      gpuOutput = NULL;
    }
    handleBadSigs();
  }


  /* printf("\n"); */
}


void tidyUpGpuThings(){
  /*
     A free for every malloc (and the GPU equivalent).
     Closes output files.
     Prints resource information.
     Needless to say, nothing will work after this function is called.
  */

  if(gpuOutput!=NULL){
    fclose(gpuOutput);
    gpuOutput = NULL;
  }

#ifdef TSTAMP
  TS_SAVE(1,"../timingAnalysis/stompings.dat");
#endif

  struct rusage resourcesEnd;
  if(getrusage(RUSAGE_SELF, &resourcesEnd)){
    perror("Error from getrusage");
  }
  struct timeval endTime;
  gettimeofday(&endTime,&dummy);
  float elapsedTime=(endTime.tv_sec-startTime.tv_sec) +
    (endTime.tv_usec-startTime.tv_usec)/1000000.0;
  float elapsedUserTime=(resourcesEnd.ru_utime.tv_sec-resourcesStart.ru_utime.tv_sec) +
    (resourcesEnd.ru_utime.tv_usec-resourcesStart.ru_utime.tv_usec)/1000000.0;
  float elapsedSystemTime=(resourcesEnd.ru_stime.tv_sec-resourcesStart.ru_stime.tv_sec) +
    (resourcesEnd.ru_stime.tv_usec-resourcesStart.ru_stime.tv_usec)/1000000.0;
  printf("Elapsed time %4.2fs (%4.2fs user, %4.2fs system).\n", elapsedTime, elapsedUserTime, elapsedSystemTime);
  printf("%ld voluntary context switches (%ld involuntary).\n\n\n",
	 resourcesEnd.ru_nvcsw-resourcesStart.ru_nvcsw, resourcesEnd.ru_nivcsw-resourcesStart.ru_nivcsw);

  if(eventData){
    free(eventData);
  }
  if(offsetInd){
    free(offsetInd);
  }
  if(numEventSamples){
    free(numEventSamples);
  }
  if(passFilter){
    free(passFilter);
  }
  if(staticPassFilter){
    free(staticPassFilter);
  }
  if(longTimeAvePowSpec){
    free(longTimeAvePowSpec);
  }

  if(commandQueue){
    clFlush(commandQueue);
  }

  if(lookupBuffer){
    destroyBuffer(lookupBuffer);
  }
  if(rawBufferVPol){
    destroyBuffer(rawBufferVPol);
  }
  if(rawBufferHPol){
    destroyBuffer(rawBufferHPol);
  }
  if(numSampsBufferVPol){
    destroyBuffer(numSampsBufferVPol);
  }
  if(numSampsBufferHPol){
    destroyBuffer(numSampsBufferHPol);
  }
  if(normalBuffer){
    destroyBuffer(normalBuffer);
  }
  if(fourierBuffer){
    destroyBuffer(fourierBuffer);
  }
  if(circularCorrelationBuffer){
    destroyBuffer(circularCorrelationBuffer);
  }
  if(imageBuffer){
    destroyBuffer(imageBuffer);
  }
  if(passFilterBuffer){
    destroyBuffer(passFilterBuffer);
  }
  if(staticPassFilterBuffer){
    destroyBuffer(staticPassFilterBuffer);
  }
  if(powSpecBuffer){
    destroyBuffer(powSpecBuffer);
  }
  if(maxThetaIndsBuffer){
    destroyBuffer(maxThetaIndsBuffer);
  }
  if(imagePeakValBuffer){
    destroyBuffer(imagePeakValBuffer);
  }
  if(imagePeakThetaBuffer){
    destroyBuffer(imagePeakThetaBuffer);
  }
  if(imagePeakPhiBuffer){
    destroyBuffer(imagePeakPhiBuffer);
  }
  if(imagePeakValBuffer2){
    destroyBuffer(imagePeakValBuffer2);
  }
  if(imagePeakThetaBuffer2){
    destroyBuffer(imagePeakThetaBuffer2);
  }
  if(imagePeakPhiBuffer2){
    destroyBuffer(imagePeakPhiBuffer2);
  }
  if(imagePeakPhiSectorBuffer){
    destroyBuffer(imagePeakPhiSectorBuffer);
  }
  if(invFftBuffer){
    destroyBuffer(invFftBuffer);
  }

  if(coherentWaveBuffer){
    destroyBuffer(coherentWaveBuffer);
  }
  if(hilbertBuffer){
    destroyBuffer(hilbertBuffer);
  }
  if(hilbertPeakBuffer){
    destroyBuffer(hilbertPeakBuffer);
  }

  if(isMinimaBuffer){
    destroyBuffer(isMinimaBuffer);
  }
  if(passFilterLocalBuffer){
    destroyBuffer(passFilterLocalBuffer);
  }

  if(normalizationKernel){
    clReleaseKernel(normalizationKernel);
  }
  if(fourierKernel){
    clReleaseKernel(fourierKernel);
  }
  if(imageKernel){
    clReleaseKernel(imageKernel);
  }
  if(findImagePeakInPhiSectorKernel){
    clReleaseKernel(findImagePeakInPhiSectorKernel);
  }
  if(findMaxPhiSectorKernel){
    clReleaseKernel(findMaxPhiSectorKernel);
  }
  if(coherentSumKernel){
    clReleaseKernel(coherentSumKernel);
  }
  if(hilbertKernel){
    clReleaseKernel(hilbertKernel);
  }
  if(hilbertPeakKernel){
    clReleaseKernel(hilbertPeakKernel);
  }
  if(iFFTFilteredWaveformsKernel){
    clReleaseKernel(iFFTFilteredWaveformsKernel);
  }

  if(prog){
    clReleaseProgram(prog);
  }

  if(commandQueue){
    clReleaseCommandQueue(commandQueue);
  }

  if(context){
    clReleaseContext(context);
  }
}

 void timeStamp(int stampNo, int numEvents, cl_event* eventList){
#ifdef TSTAMP
#ifdef WAIT_FOR_QUEUES_BEFORE_STAMP
  if(eventList != NULL || numEvents < 1){
    clWaitForEvents(numEvents, eventList);
  }
#endif
  TS_RECORD(1, stampNo);
#endif

}

float getDeltaTExpected(int ant1, int ant2,double phiWave, double thetaWave,
		      const float* phiArray, const float* rArray, const float* zArray)
{
  double tanThetaW = tan(-thetaWave);
  double part1 = zArray[ant1]*tanThetaW - rArray[ant1]*cos(phiWave-phiArray[ant1]);
  double part2 = zArray[ant2]*tanThetaW - rArray[ant2]*cos(phiWave-phiArray[ant2]);
  // nb have *-1 compared to ANITA library definition, I think it makes more sense to do
  // t2 - t1 NOT t1 - t2
  double tdiff = 1e9*((cos(-thetaWave) * (part2 - part1))/SPEED_OF_LIGHT); // Returns time in ns
  /* tdiff /= NOMINAL_SAMPLING; */
  /* return floor(tdiff + 0.5); */
  return tdiff;
}

// Various bits taken from AnitaGeometry.cxx on 29/11/2013
float* fillDeltaTArrays(){
  /* short* offsetInd = malloc(sizeof(short)*NUM_PHI_SECTORS*NUM_LOCAL_COMBOS_PER_PHI_SECTOR*NUM_BINS_PHI*NUM_BINS_THETA); */
  float* offsetInd = malloc(sizeof(float)*NUM_PHI_SECTORS*NUM_LOCAL_COMBOS_PER_PHI_SECTOR*NUM_BINS_PHI*NUM_BINS_THETA);

  float rArray[NUM_ANTENNAS]={0.9675,0.7402,0.9675,0.7402,0.9675,0.7402,0.9675,0.7402,0.9675,0.7402,0.9675,0.7402,0.9675,0.7402,0.9675,0.7402,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447,2.0447};

  float phiArrayDeg[NUM_ANTENNAS] = {0.0,22.5,45.0,67.5,90.0,112.5,135.0,157.5,180.0,202.5,225.0,247.5,270.0,292.5,315.0,337.5,0.0,22.5,45.0,67.5,90.0,112.5,135.0,157.5,180.0,202.5,225.0,247.5,270.0,292.5,315.0,337.5,0.0,22.5,45.0,67.5,90.0,112.5,135.0,157.5,180.0,202.5,225.0,247.5,270.0,292.5,315.0,337.5};

  float zArray[NUM_ANTENNAS] = {-1.4407,-2.4135,-1.4407,-2.4135,-1.4407,-2.4135,-1.4407,-2.4135,-1.4407,-2.4135,-1.4407,-2.4135,-1.4407,-2.4135,-1.4407,-2.4135,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-5.1090,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951,-6.1951};

  float phiArray[NUM_ANTENNAS] = {0};

  kvpReset();
  KvpErrorCode err = KVP_E_OK;
  int nAnts = NUM_ANTENNAS;

  err = configLoad ("Prioritizerd.config","antennalocations") ;
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to load Prioritizerd.config returned %s\n", kvpErrorString(err));
  }
  err = kvpGetFloatArray ("phiArrayDeg", phiArrayDeg, &nAnts);
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to do load phiArray from Prioritizerd.config returned %s\n", kvpErrorString(err));
  }
  err = kvpGetFloatArray ("rArray", rArray, &nAnts);
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to do load rArray from Prioritizerd.config returned %s\n", kvpErrorString(err));
  }
  err = kvpGetFloatArray ("zArray", zArray, &nAnts);
  if(err!=KVP_E_OK){
    fprintf(stderr, "Warning! Trying to do load zArray from Prioritizerd.config returned %s\n", kvpErrorString(err));
  }

  int m_ant1[NUM_PHI_SECTORS][NUM_LOCAL_COMBOS_PER_PHI_SECTOR];
  int m_ant2[NUM_PHI_SECTORS][NUM_LOCAL_COMBOS_PER_PHI_SECTOR];

  int ant=0;
  for(ant=0; ant<NUM_ANTENNAS; ant++){
       phiArray[ant] = phiArrayDeg[ant]*PI/180;
       /* phiArrayDeg[ant] = phiArray[ant]*180/PI; */
       /* printf("ant %d, phiArray %lf, phiArrayDeg %lf, zArray %lf, rArray %lf\n", ant, */
       /* 	      phiArray[ant], phiArrayDeg[ant], zArray[ant], rArray[ant]); */
  }


  uint phiSectorInd = 0;
  uint localCombo = 0;
  for(phiSectorInd=0; phiSectorInd < NUM_PHI_SECTORS; phiSectorInd++){
    for(localCombo=0; localCombo < NUM_LOCAL_COMBOS_PER_PHI_SECTOR; localCombo++){
      // phiSector - 1... self + right
      if(localCombo < 3){
	m_ant1[phiSectorInd][localCombo] = (phiSectorInd+15)%16 + localCombo*NUM_PHI_SECTORS;
	m_ant2[phiSectorInd][localCombo] = (m_ant1[phiSectorInd][localCombo] + NUM_PHI_SECTORS)%NUM_ANTENNAS;
      }
      else if(localCombo < NUM_GLOBAL_COMBOS_PER_PHI_SECTOR){
	m_ant1[phiSectorInd][localCombo] = (phiSectorInd+15)%NUM_PHI_SECTORS + NUM_PHI_SECTORS*((localCombo-3)/3);
	m_ant2[phiSectorInd][localCombo] = phiSectorInd + (localCombo%3)*NUM_PHI_SECTORS;
      }
      else{
	int localInd = localCombo % NUM_GLOBAL_COMBOS_PER_PHI_SECTOR;
	m_ant1[phiSectorInd][localCombo] = m_ant1[phiSectorInd][localInd] + localCombo/NUM_GLOBAL_COMBOS_PER_PHI_SECTOR;
	if(m_ant1[phiSectorInd][localInd]/16 - m_ant1[phiSectorInd][localCombo]/16 != 0){
	  //	if(m_ant1[phiSectorInd][localCombo] % 16 == 0){
	  m_ant1[phiSectorInd][localCombo] -= 16;
	}
	m_ant2[phiSectorInd][localCombo] = m_ant2[phiSectorInd][localInd] + localCombo/NUM_GLOBAL_COMBOS_PER_PHI_SECTOR;
	if(m_ant2[phiSectorInd][localInd]/16 - m_ant2[phiSectorInd][localCombo]/16 != 0){
	  //	if(m_ant2[phiSectorInd][localCombo] % 16 == 0){
	  m_ant2[phiSectorInd][localCombo] -= 16;
	}
      }
    }
  }

  /* int minDelta=1; */
  /* int maxDelta=-1; */
  uint phiSector = 0;
  uint combo = 0;
  uint phiInd = 0;
  uint thetaInd = 0;
  for(phiSector = 0; phiSector < NUM_PHI_SECTORS; phiSector++){
    for(combo=0; combo < NUM_LOCAL_COMBOS_PER_PHI_SECTOR; combo++){
      int ant1 = m_ant1[phiSector][combo];
      int ant2 = m_ant2[phiSector][combo];
      for(phiInd = 0; phiInd < NUM_BINS_PHI; phiInd++){
	/* double phiDeg = phiArray[phiSector]*180./PI + PHI_RANGE*((double)phiInd/NUM_BINS_PHI - 0.5); */
	double phiDeg = phiArrayDeg[phiSector] + PHI_RANGE*((double)phiInd/NUM_BINS_PHI - 0.5);
	double phiWave = phiDeg*PI/180; //TMath::DegToRad();
	for(thetaInd = 0; thetaInd < NUM_BINS_THETA; thetaInd++){
	  double thetaDeg = THETA_RANGE*((double)thetaInd/NUM_BINS_THETA - 0.5);
	  double thetaWave = thetaDeg*PI/180; //TMath::DegToRad();
	  /* short offset = getDeltaTExpected(ant1, ant2, phiWave, thetaWave, phiArray, rArray, zArray); */
	  float offset = getDeltaTExpected(ant1, ant2, phiWave, thetaWave, phiArray, rArray, zArray);
	  /* if(phiSector==15 && phiInd == 0 && thetaInd == NUM_BINS_THETA/2){ */
	  /*   printf("%d %d %hd\n", ant1, ant2, offset); */
	  /* } */

	  /* if(offset < 0){ */
	  /*   offset += NUM_SAMPLES; */
	  /* } */
	  uint offsetIndInd = (phiSector*NUM_LOCAL_COMBOS_PER_PHI_SECTOR*NUM_BINS_PHI*NUM_BINS_THETA
			       + combo*NUM_BINS_PHI*NUM_BINS_THETA
			       + phiInd*NUM_BINS_THETA
			       + thetaInd);
	  offsetInd[offsetIndInd] = offset;
	  //	  printf("%d\t", offset);
	}
      }
    }
  }

  return offsetInd;

}





/**
 * This function is supposed to reduce code dupliation at the end of the main GPU loop.
 *
 * @param pol is the polarization
 * @param ant is the antenna
 * @param longDynamicPassFilter
 * @param isLocalMinimum
 * @param isLocalMaximum
 * @param spikeThresh_dB
 */
 void updateLongDynamicPassFilter(int pol, int ant, short* longDynamicPassFilter, int* isLocalMinimum, int* isLocalMaximum,  float spikeThresh_dB, int startFreqInd, int endFreqInd){


   int lookingForSpikeStart = 1;
   int spikeStartBin = 0;
   int spikePeakBin = 0;
   const int longBaseInd = (pol*NUM_ANTENNAS + ant)*(NUM_SAMPLES/2);

   int loopDeltaFreqInd = endFreqInd > startFreqInd ? 1 : -1;

   int freqInd = 0;
   /* for(freqInd=0; freqInd < NUM_SAMPLES/2; ++freqInd){ */
   for(freqInd=startFreqInd; freqInd != endFreqInd; freqInd += loopDeltaFreqInd){

     if(lookingForSpikeStart > 0){
       if(isLocalMinimum[freqInd] > 0){
	 // start of a spike
	 spikeStartBin = freqInd;
	 lookingForSpikeStart = 0;
       }
     }
     else{ // then looking for localMaxima
       if(isLocalMaximum[freqInd]){
	 spikePeakBin = freqInd;

	 // is the spike big enough?
	 float spikeSize_dB = longTimeAvePowSpec[longBaseInd + spikePeakBin] - longTimeAvePowSpec[longBaseInd + spikeStartBin];
	 if(spikeSize_dB > spikeThresh_dB){

	   // then filter twice the distance from the maximum to the minimum
	   int freqInd2 = spikeStartBin;
	   /* for(freqInd2 = spikeStartBin; freqInd2 < 2*spikePeakBin - spikeStartBin; freqInd2++){ */
	   for(freqInd2 = spikeStartBin; freqInd2 != 2*spikePeakBin - spikeStartBin; freqInd2+=loopDeltaFreqInd){
	     longDynamicPassFilter[longBaseInd+freqInd2] = 0;
	   }
	 }

	 // find next minimum
	 lookingForSpikeStart = 1;
       }
     }
   }
 }


 void dumpBuffersToTextFiles(int polInd){

    printBufferToTextFile2(commandQueue, "skipUpdatingAvePowSpec", polInd, skipUpdatingAvePowSpec, NUM_EVENTS, nEvents);
    printBufferToTextFile2(commandQueue, "longDynamicPassFilterBuffer", polInd, longDynamicPassFilterBuffer, NUM_EVENTS, nEvents);
    printBufferToTextFile2(commandQueue, "rawBuffer", polInd, rawBufferHPol, NUM_EVENTS, nEvents);
    printBufferToTextFile2(commandQueue, "phiSectorTriggerBuffer", polInd, phiSectorTriggerBufferHPol, NUM_EVENTS, 1);
    printBufferToTextFile2(commandQueue, "numSampsBuffer", polInd, numSampsBufferHPol, NUM_EVENTS, 1);


   printBufferToTextFile2(commandQueue, "normalBuffer", polInd, normalBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "rmsBuffer", polInd, rmsBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "fourierBuffer", polInd, fourierBuffer, NUM_EVENTS, nEvents);
   printBufferToTextFile2(commandQueue, "powSpecBuffer", polInd, powSpecBuffer, 1, 1);
   printBufferToTextFile2(commandQueue, "passFilterBuffer", polInd, passFilterBuffer, 1, 1);
   printBufferToTextFile2(commandQueue, "staticPassFilterBuffer", polInd, staticPassFilterBuffer, 1, 1);
   printBufferToTextFile2(commandQueue, "fourierBuffer2", polInd, fourierBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "circularCorrelationBuffer", polInd, circularCorrelationBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "image", polInd, imageBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "maxThetaInds", polInd, maxThetaIndsBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "imagePeakValBuffer", polInd, imagePeakValBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "imagePeakValBuffer2", polInd, imagePeakValBuffer2, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "imagePeakPhiBuffer", polInd, imagePeakPhiBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "imagePeakPhiBuffer2", polInd, imagePeakPhiBuffer2, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "imagePeakPhiSectorBuffer", polInd, imagePeakPhiSectorBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "imagePeakThetaBuffer2", polInd, imagePeakThetaBuffer2, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "normalBuffer2", polInd, normalBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "newRmsBuffer", polInd, newRmsBuffer, NUM_EVENTS, 1);

   printBufferToTextFile2(commandQueue, "coherentWaveForm", polInd, coherentWaveBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "hilbertEnvelope", polInd, hilbertBuffer, NUM_EVENTS, 1);
   printBufferToTextFile2(commandQueue, "hilbertPeak", polInd, hilbertPeakBuffer, NUM_EVENTS, 1);


   printf("PolInd %d buffers printed...\n", polInd);

 }
