#include "myInterferometryConstants.h"

inline float8 doRadix2FFT(short dataInd, float8 fftInput, __local float2* complexScratch, int dir){

  //  short debug= dir > 1 ? 1 : 0;
  dir = dir >= 0 ? 1 : -1;

  /* Normalize inverse fourier transform */
  fftInput = dir < 0 ? fftInput/NUM_SAMPLES : fftInput;

  short outInd0 = 0;
  short outInd1 = 0;
  short outInd2 = 0;
  short outInd3 = 0;

  // Factor of 4 goes from WI (up to 256) to number of samples (1024).
  // These are the indexes to be bit reversed.
  short inInd0 = dataInd*4;
  short inInd1 = dataInd*4 + 1;
  short inInd2 = dataInd*4 + 2;
  short inInd3 = dataInd*4 + 3;

  // Now do bit reversal, there are 10 bits for 1024 samples, pow(2, 10) = 1024.
#pragma unroll

  for(short bit=RADIX_BIT_SWITCH; bit>=0; bit--){
    //    for(short bit=9; bit>=0; bit--){ /* NUM_SAMPLES = 1024 */
    //  for(short bit=7; bit>=0; bit--){ /* NUM_SAMPLES = 256 */

    // Put bit in reverse position (0->9, 1->8, etc...)
    outInd0 += (inInd0 & 1)*pown(2.f,bit);
    outInd1 += (inInd1 & 1)*pown(2.f,bit);
    outInd2 += (inInd2 & 1)*pown(2.f,bit);
    outInd3 += (inInd3 & 1)*pown(2.f,bit);

    // Right bitshift, alters value of inInd
    inInd0 >>= 1;
    inInd1 >>= 1;
    inInd2 >>= 1;
    inInd3 >>= 1;
  }
  
  complexScratch[outInd0] = (float2)(fftInput.s0, fftInput.s1);
  complexScratch[outInd1] = (float2)(fftInput.s2, fftInput.s3);
  complexScratch[outInd2] = (float2)(fftInput.s4, fftInput.s5);
  complexScratch[outInd3] = (float2)(fftInput.s6, fftInput.s7);
  
  // Wait for all work items to get to this point.
  barrier(CLK_LOCAL_MEM_FENCE);

  // Now our input set of complex things are rearranged as float2s in the complexScratch.
  // This was step 0 in our FFT... one could think of it as doing 1024 FTs of length 1,
  // although really it's just a rearrangement to make subsequent steps easier.

  // Now we do the other iterations, each one doubling the length of the sub-FTs.
  // At the end of each loop iteration, we should have DFTs of length 'length'.

#pragma unroll
  for(unsigned short length = 2; length<=NUM_SAMPLES; length<<=1){/* equivalent to length*=2 */
  
    // I have arranged arrays so that the output put of even sub-ffts are on the left
    // the corresponding odd ffts are on the right, offset by half the total length
    // of the sub sequence.

    // In the sub-FT I am 'assembling' at this loop, the odd-numbered elements 
    // are positioned this after from the even ones...
    short oddOffset = length/2;


#pragma unroll
    // Each work item will do 4 samples/frequencies.
    // However, symmetry of FFT means it makes sense to do 
    // two values at each time through the loop, so have two 'localInds'
    for(short localInd = 0; localInd < 2; localInd++){

      // baseInd is the element of the fft we consider now, grabbing the required
      // even and odd factors assembled so far...
      short baseInd = 2*dataInd + localInd; // runs from 0 -> 511
      short evenInd = length*(2*baseInd/length) + (baseInd % oddOffset);
      short oddInd = evenInd + oddOffset;

      // Twiddle factor (could do this before here and put in LDS, but for now...)
      short subSequenceInd = evenInd % length;
      float theta = -2*dir*PI*subSequenceInd/length;

      // apprently (and now measured to be the case for me) native_sin, native_cos are faster than sin, cos
      float reTwiddle = native_cos(theta);
      float imTwiddle = native_sin(theta);

      // Get input to this FFT before overwriting.
      float2 complexEvenIn = complexScratch[evenInd];
      float2 complexOddIn = complexScratch[oddInd];

      // the odd part gets multiplied by the twiddle factor, complex nature of multiplication put in by hand here
      float2 oddTwiddle = (float2)(reTwiddle*complexOddIn.x - imTwiddle*complexOddIn.y,
				   imTwiddle*complexOddIn.x + reTwiddle*complexOddIn.y);

      float2 complexOut1 = complexEvenIn + oddTwiddle;
      float2 complexOut2 = complexEvenIn - oddTwiddle;

      // Write output
      complexScratch[evenInd] = complexOut1;
      complexScratch[oddInd] = complexOut2;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

  }

  float8 fftOutput = (float8)(complexScratch[4*dataInd],
  			      complexScratch[4*dataInd+1],
  			      complexScratch[4*dataInd+2],
  			      complexScratch[4*dataInd+3]);

  return fftOutput;
}




__kernel void normalizeAndPadWaveforms(__global short* numSampsBuffer,
				       __global float* rmsBuffer,
				       __global float4* rawData,
				       __global float4* normalData,
				       __local float4* square,
				       __local float4* mean){
  
  // work item identifier
  int antInd = get_global_id(1);
  int eventInd = get_global_id(2);

  int localInd = get_local_id(0);
  int dataInd = eventInd*NUM_ANTENNAS*NUM_SAMPLES/4 + antInd*NUM_SAMPLES/4 + localInd;

  
  // look up how many raw samples in the waveform
  short numSamps = numSampsBuffer[eventInd*NUM_ANTENNAS + antInd];

  // hold 4 samples of data in private memory
  /* float4 storeShort = rawData[dataInd]; */
  /* float4 store; */
  /* store.x = storeShort.x; */
  /* store.y = storeShort.y; */
  /* store.z = storeShort.z; */
  /* store.w = storeShort.w; */
  float4 store = rawData[dataInd];

  /*
    Addition to effectively zero pad the waveforms which are sent to the GPU.
  */
  store.x = localInd*4 < numSamps ? store.x : 0;
  store.y = localInd*4 + 1 < numSamps ? store.y : 0;
  store.z = localInd*4 + 2 < numSamps ? store.z : 0;
  store.w = localInd*4 + 3 < numSamps ? store.w : 0;

  // to get rms need linear combos of square and sum
  // first put in LDS
  square[localInd] = store*store/numSamps; //NUM_SAMPLES;
  mean[localInd] = store/numSamps; //NUM_SAMPLES;
  barrier(CLK_LOCAL_MEM_FENCE);

  // Now data is in LDS, add up the the pieces
  for(int offset = 1; offset < get_local_size(0); offset += offset){
    if (localInd % (2*offset) == 0){
      square [localInd] += square [localInd + offset];
      mean [localInd] += mean [localInd + offset];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  // broadcast sum to private memory
  float4 square4 = square[0];
  float4 mean4 = mean[0];
  
  // get sum of float4 components
  float x = square4.x + square4.y + square4.z + square4.w;
  float y = mean4.x + mean4.y + mean4.z + mean4.w;

  // varience = the mean of the square minus the square of the mean
  float rms = sqrt(x - y*y);

  // in case of all blank channel, which is stupendously unlikely in practise
  // but does occur in my simple test conditions
  rms = rms <= 0 ? 1 : rms;

  // only need one WI to do this
  if(localInd == 0){
    rmsBuffer[eventInd*NUM_ANTENNAS + antInd] = rms;
  }

  // normalize the data, 
  store = (store - y) / rms;

  /*
    Would have offset trailing zero-pad since we subtracted the mean values.
    So now set trailing values to zero again.
    Note that during development, the zero-padding at the start of the waveform
    (which corrects for different cable delays) is not fixed by this.
    This will eventually not be a problem as the cable delays should be fixed in
    the deltaT lookup table.
  */
  store.x = localInd*4 < numSamps ? store.x : 0;
  store.y = localInd*4 + 1 < numSamps ? store.y : 0;
  store.z = localInd*4 + 2 < numSamps ? store.z : 0;
  store.w = localInd*4 + 3 < numSamps ? store.w : 0;

  // push data to global buffer
  normalData[dataInd] = store;
}



__kernel void normaliseWaveforms(__global float* rmsBuffer,
				 __global short4* rawData,
				 __global float4* normalData,
				 __local float4* square,
				 __local float4* mean){
  
  // work item identifier
  int antInd = get_global_id(1);
  int eventInd = get_global_id(2);

  int localInd = get_local_id(0);
  int dataInd = eventInd*NUM_ANTENNAS*get_local_size(0) + antInd*get_local_size(0) + localInd;

  // hold 4 samples of data in private memory
  float4 store;
  store.x = rawData[dataInd].x;
  store.y = rawData[dataInd].y;
  store.z = rawData[dataInd].z;
  store.w = rawData[dataInd].w;

  // to get rms need linear combos of square and sum
  // first put in LDS
  square[localInd] = store*store / NUM_SAMPLES;
  mean[localInd] = store / NUM_SAMPLES;
  barrier(CLK_LOCAL_MEM_FENCE);

  // Now data is in LDS, add up the the pieces
  for(int offset = 1; offset < get_local_size(0); offset += offset){
    if (localInd % (2*offset) == 0){
      square [localInd] += square [localInd + offset];
      mean [localInd] += mean [localInd + offset];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  // broadcast sum to private memory
  float4 square4 = square[0];
  float4 mean4 = mean[0];
  
  // get sum of float4 components
  float x = square4.x + square4.y + square4.z + square4.w;
  float y = mean4.x + mean4.y + mean4.z + mean4.w;

  // varience = the mean of the square minus the square of the mean
  float rms = sqrt(x - y*y);

  // in case of all blank channel
  rms = rms <= 0 ? 1 : rms;

  // only need one WI to do this
  if(localInd == 0){
    rmsBuffer[eventInd*NUM_ANTENNAS + antInd] = rms;
  }

  // output data
  normalData[dataInd] = (store - y) / rms;
}

__kernel void crossCorrelationFourierDomain(__global short* phiSectorTrig,
					    __global float8* waveformFourier,
					    __local float2* complexScratch,
					    __global float4* circularCorrBuffer){
  short dataInd = get_global_id(0);
  short comboInd = get_global_id(1);
  short eventInd = get_global_id(2);

  // figure out where in correlation array to put fourier domain cross correlation
  int comboIndThisPhiSector = comboInd % NUM_GLOBAL_COMBOS_PER_PHI_SECTOR;
  int phiSector = comboInd / NUM_GLOBAL_COMBOS_PER_PHI_SECTOR;

  // is there a trigger to the left of the current phi
  //  bool leftPhiTrig2 = phiSectorTrig[eventInd*NUM_PHI_SECTORS + (phiSector+14)%16] > 0 ? 1 : 0;
  bool leftPhiTrig = phiSectorTrig[eventInd*NUM_PHI_SECTORS + (phiSector+15)%16] > 0 ? 1 : 0;
  // is there a trigger at current phi
  bool middlePhiTrig = phiSectorTrig[eventInd*NUM_PHI_SECTORS + phiSector] > 0 ? 1 : 0;
  // is there a trigger to the right of the current phi
  bool rightPhiTrig = phiSectorTrig[eventInd*NUM_PHI_SECTORS + (phiSector+1)%16] > 0 ? 1 : 0;
  //  bool rightPhiTrig2 = phiSectorTrig[eventInd*NUM_PHI_SECTORS + (phiSector+2)%16] > 0 ? 1 : 0;

  // "self correlations" are the first three combos in my combos per phi sector array
  //  bool doSelfCorrelations = leftPhiTrig2 || leftPhiTrig || middlePhiTrig  || rightPhiTrig || rightPhiTrig2;
  bool doSelfCorrelations = leftPhiTrig || middlePhiTrig  || rightPhiTrig;
  doSelfCorrelations = (doSelfCorrelations && (comboIndThisPhiSector < 3));

  // "right correlations" are the last 9 of 12 combos in my combos per phi sector array
  //  bool doRightCorrelations =  leftPhiTrig || middlePhiTrig || rightPhiTrig || rightPhiTrig2;
  bool doRightCorrelations =  middlePhiTrig || rightPhiTrig;
  doRightCorrelations = (doRightCorrelations && (comboIndThisPhiSector >= 3));

  // whether this work item should do cross correlations
  bool doCorrelations = (doSelfCorrelations || doRightCorrelations);

  int corrOutInd = (eventInd*NUM_PHI_SECTORS*NUM_GLOBAL_COMBOS_PER_PHI_SECTOR*NUM_SAMPLES/4
		    + phiSector*NUM_GLOBAL_COMBOS_PER_PHI_SECTOR*NUM_SAMPLES/4
		    + comboIndThisPhiSector*NUM_SAMPLES/4
		    + dataInd);

  if(doCorrelations){

    // the two antennae (waveforms) to be cross correlated
    int ant1 = comboIndThisPhiSector < 3 ? comboIndThisPhiSector*NUM_PHI_SECTORS + phiSector : ((comboIndThisPhiSector-3)/3)*NUM_PHI_SECTORS + phiSector;
    int ant2 = comboIndThisPhiSector < 3 ? (ant1 + NUM_PHI_SECTORS)%NUM_ANTENNAS : (comboIndThisPhiSector%3)*NUM_PHI_SECTORS + (phiSector + 1)%NUM_PHI_SECTORS;

    // get complex data, 4 * float2s are a float8
    int dataInd1 = eventInd*NUM_ANTENNAS*NUM_SAMPLES/4 + ant1*NUM_SAMPLES/4 + dataInd;
    int dataInd2 = eventInd*NUM_ANTENNAS*NUM_SAMPLES/4 + ant2*NUM_SAMPLES/4 + dataInd;
    float8 c1 = waveformFourier[dataInd1];
    float8 c2 = waveformFourier[dataInd2];

    // multiply them together (with one conjugated) and order swapped for ifft procedure
    float8 fftInput = (float8)((c1.s0*c2.s1 - c1.s1*c2.s0),
			       (c1.s0*c2.s0 + c1.s1*c2.s1),
			       (c1.s2*c2.s3 - c1.s3*c2.s2),
			       (c1.s2*c2.s2 + c1.s3*c2.s3),
			       (c1.s4*c2.s5 - c1.s5*c2.s4),
			       (c1.s4*c2.s4 + c1.s5*c2.s5),
			       (c1.s6*c2.s7 - c1.s7*c2.s6),
			       (c1.s6*c2.s6 + c1.s7*c2.s7));

    // since my invFFT normalization is 1/1024, need to do that twice
    // since doing invFFT of a product of two FFTs...
    fftInput/=(NUM_SAMPLES*NUM_SAMPLES);

    float8 fftOutput = doRadix2FFT(dataInd, fftInput, complexScratch, 1);

    float4 output;
    // The output also needs to be swapped.
    // so put imaginary parts into output array.
    output.x = fftOutput.s1;
    output.y = fftOutput.s3;
    output.z = fftOutput.s5;
    output.w = fftOutput.s7;

    circularCorrBuffer[corrOutInd] = output;
  }
  else{
    circularCorrBuffer[corrOutInd] = (float4)(0, 0, 0, 0);
  }
}


__kernel void makeImageFromFourierDomain(__global short* phiSectorTrigBuffer,
					 __global float* circularCorrBuffer,
					 __global float4* image,
					 __global short4* lookup){

  int eventInd = get_global_id(2);
  int phiInd = get_global_id(1);
  int thetaInd = get_global_id(0);

  for(int phiSector = 0; phiSector < NUM_PHI_SECTORS; phiSector++){

    float4 corr = 0;
    int pixelInd = phiSector*NUM_BINS_PHI*NUM_BINS_THETA/4 + phiInd*NUM_BINS_THETA/4 + thetaInd;

    /* if(phiSectorTrigBuffer[eventInd*NUM_PHI_SECTORS + (phiSector+15)%16] == 1 || */
    /* 	phiSectorTrigBuffer[eventInd*NUM_PHI_SECTORS + phiSector] == 1 || */
    /* 	phiSectorTrigBuffer[eventInd*NUM_PHI_SECTORS + (phiSector+1)%16] == 1){*/
    if(phiSectorTrigBuffer[eventInd*NUM_PHI_SECTORS + phiSector] == 1){
      for(uint comboLocal = 0; comboLocal < NUM_LOCAL_COMBOS_PER_PHI_SECTOR; comboLocal++){
	uint lookupInd = (phiSector*NUM_LOCAL_COMBOS_PER_PHI_SECTOR*NUM_BINS_THETA*NUM_BINS_PHI/4
			  + comboLocal*NUM_BINS_THETA*NUM_BINS_PHI/4
			  + phiInd*NUM_BINS_THETA/4
			  + thetaInd);
	short4 dt = lookup[lookupInd];

	int phiSectorCorr = (phiSector + 15 + comboLocal/NUM_GLOBAL_COMBOS_PER_PHI_SECTOR)%NUM_PHI_SECTORS;
	uint comboCorr = comboLocal % NUM_GLOBAL_COMBOS_PER_PHI_SECTOR;
	uint corrIndBase = (eventInd*NUM_PHI_SECTORS*NUM_GLOBAL_COMBOS_PER_PHI_SECTOR
			    + phiSectorCorr*NUM_GLOBAL_COMBOS_PER_PHI_SECTOR
			    + comboCorr);

	corr.x += circularCorrBuffer[corrIndBase*NUM_SAMPLES + dt.x];
	corr.y += circularCorrBuffer[corrIndBase*NUM_SAMPLES + dt.y];
	corr.z += circularCorrBuffer[corrIndBase*NUM_SAMPLES + dt.z];
	corr.w += circularCorrBuffer[corrIndBase*NUM_SAMPLES + dt.w];
      }
      corr/=(NUM_LOCAL_COMBOS_PER_PHI_SECTOR);
    }

    image[eventInd*NUM_PHI_SECTORS*NUM_BINS_PHI*NUM_BINS_THETA/4 + pixelInd] = corr;
  }
}


__kernel void findImagePeakInTheta(__global float* image,
				   __global short* phiTrig,
				   __local float* corrScratch,
				   __local int* indScratch,
				   __global int* maxThetaInds){

  int eventInd = get_global_id(2);
  int phiInd = get_global_id(1);
  int thetaInd = get_global_id(0);

  for(int phiSector = 0; phiSector < NUM_PHI_SECTORS; phiSector++){

    /* 
       Not really necessary, but makes for much easier interpretation of output.
       Otherwise there's non-initialized crap in the output...
       If it turns out there's a massive overhead, I'll comment this out.
    */
    if(thetaInd == 0){
      maxThetaInds[(eventInd*NUM_PHI_SECTORS + phiSector)*NUM_BINS_PHI + phiInd] = 0;
    }


    
    /* if(phiTrig[eventInd*NUM_PHI_SECTORS + phiSector]  */
    /* 	|| phiTrig[eventInd*NUM_PHI_SECTORS + (phiSector+1)%16]  */
    /* 	|| phiTrig[eventInd*NUM_PHI_SECTORS + (phiSector+15)%16]){*/
    if(phiTrig[eventInd*NUM_PHI_SECTORS + phiSector]){
    
      int pixelInd = (eventInd*NUM_PHI_SECTORS*NUM_BINS_PHI*NUM_BINS_THETA 
		      + phiSector*NUM_BINS_PHI*NUM_BINS_THETA 
		      + phiInd*NUM_BINS_THETA 
		      + thetaInd);
      float corr = image[pixelInd];

      corrScratch[thetaInd] = corr;
      indScratch[thetaInd] = thetaInd;
      barrier(CLK_LOCAL_MEM_FENCE);

      int largerThetaInd = -1;
      float largerCorr = -1;
      float corr1 = 0;
      float corr2 = 0;

      // numPixels will be decreasing...
      // it's a competition!
      for(int numPixels = NUM_BINS_THETA; numPixels > 1; numPixels/=2){
	
	// even indices will compare even/odd pixels
	if(thetaInd%2 == 0 && thetaInd < numPixels){
	  // get image vals from neighbouring pixels
	  corr1 = corrScratch[thetaInd];
	  corr2 = corrScratch[thetaInd + 1];
	
	  // larger correlation is the winner!
	  largerThetaInd = corr2 > corr1 ? indScratch[thetaInd + 1] : indScratch[thetaInd];
	  largerCorr = corr2 > corr1 ? corr2 : corr1;

	}

	// make sure all WIs have done the comparison for this iteration of the loop
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Put larger correlation through to the next round of "the contest" */
	if(thetaInd%2 == 0 && thetaInd < numPixels){
	  corrScratch[thetaInd/2] = largerCorr;
	  indScratch[thetaInd/2] = largerThetaInd;
	}

	// once every WI has done that...
	barrier(CLK_LOCAL_MEM_FENCE);
      } // next round

      if(thetaInd == 0){// put the winner into a global array
	maxThetaInds[(eventInd*NUM_PHI_SECTORS + phiSector)*NUM_BINS_PHI + phiInd] = largerThetaInd;
	//	maxThetaInds[(eventInd*NUM_PHI_SECTORS + phiSector)*NUM_BINS_PHI + phiInd] = 137;
      }
    } // if phi-sector was triggered
  }// loop over phi-sectors
}


__kernel void findImagePeakInPhiSector(__global int* thetaMaxInds,
				       __global float* image,
				       __local float* corrScratch,
				       __local int* thetaScratch,
				       __local int* phiScratch,
				       __global float* maxCorr,
				       __global int* maxTheta,
				       __global int* maxPhi){

  int phiSector = get_global_id(1);
  int eventInd = get_global_id(2);
  int phiInd = get_global_id(0);

  int thetaInd = thetaMaxInds[(eventInd*NUM_PHI_SECTORS + phiSector)*NUM_BINS_PHI + phiInd];
  corrScratch[phiInd] = image[(eventInd*NUM_PHI_SECTORS + phiSector)*NUM_BINS_PHI*NUM_BINS_THETA
			      + phiInd*NUM_BINS_THETA + thetaInd];
  thetaScratch[phiInd] = thetaInd;
  phiScratch[phiInd] = phiInd;
  barrier(CLK_LOCAL_MEM_FENCE);

  int numPixels = NUM_BINS_PHI;
  float largerCorr = -1;
  int largerThetaInd = -1;
  int largerPhiInd = -1;
  float corr1 = 0;
  float corr2 = 0;
  while(numPixels > 1){
    if(phiInd%2 == 0 && phiInd < numPixels){
      corr1 = corrScratch[phiInd];
      corr2 = corrScratch[phiInd + 1];
      largerPhiInd = corr2 > corr1 ? phiScratch[phiInd + 1] : phiScratch[phiInd];
      largerThetaInd = corr2 > corr1 ? thetaScratch[phiInd + 1] : thetaScratch[phiInd];
      largerCorr = corr2 > corr1 ? corr2 : corr1;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    if(phiInd%2 == 0 && phiInd < numPixels){
      corrScratch[phiInd/2] = largerCorr;
      phiScratch[phiInd/2] = largerPhiInd;
      thetaScratch[phiInd/2] = largerThetaInd;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    numPixels -= numPixels/2;
  }
  if(phiInd == 0){
    int maxInd = eventInd*NUM_PHI_SECTORS + phiSector;
    maxCorr[maxInd] = largerCorr;
    maxTheta[maxInd] = largerThetaInd;
    maxPhi[maxInd] = largerPhiInd;
  }
}

__kernel void findMaxPhiSector(
			       /* local memory for quick comparison of values */
			       __local float* corrScratch,
			       __local int* thetaScratch,
			       __local int* phiScratch,
			       __local int* phiSectorScratch,

			       /* input arrays spanning all phi sectors */
			       __global float* maxCorr,
			       __global int* maxTheta,
			       __global int* maxPhi,

			       /* output arrays with one entry per event */
			       __global float* maxCorr2,
			       __global int* maxTheta2,
			       __global int* maxPhi2,
			       __global int* maxPhiSector){

  int phiSector = get_global_id(0);
  int eventInd = get_global_id(1);

  int numPhiSectors = NUM_PHI_SECTORS;
  float largerCorr = -1;
  int largerThetaInd = -1;
  int largerPhiInd = -1;
  int largerPhiSector = -1;
  float corr1 = 0;
  float corr2 = 0;
  if(phiSector < numPhiSectors){
    phiScratch[phiSector] = maxPhi[eventInd*NUM_PHI_SECTORS + phiSector];
    thetaScratch[phiSector] = maxTheta[eventInd*NUM_PHI_SECTORS + phiSector];
    corrScratch[phiSector] = maxCorr[eventInd*NUM_PHI_SECTORS + phiSector];
    phiSectorScratch[phiSector] = phiSector;
    barrier(CLK_LOCAL_MEM_FENCE);

    while(numPhiSectors > 1){
      if(phiSector%2 == 0 && phiSector < numPhiSectors){
	corr1 = corrScratch[phiSector];
	corr2 = corrScratch[phiSector + 1];
	largerPhiInd = corr2 > corr1 ? phiScratch[phiSector + 1] : phiScratch[phiSector];
	largerThetaInd = corr2 > corr1 ? thetaScratch[phiSector + 1] : thetaScratch[phiSector];
	largerCorr = corr2 > corr1 ? corr2 : corr1;
	largerPhiSector = corr2 > corr1 ? phiSectorScratch[phiSector + 1] : phiSectorScratch[phiSector];
      }
      barrier(CLK_LOCAL_MEM_FENCE);
      if(phiSector%2 == 0 && phiSector < numPhiSectors){
	corrScratch[phiSector/2] = largerCorr;
	phiScratch[phiSector/2] = largerPhiInd;
	thetaScratch[phiSector/2] = largerThetaInd;
	phiSectorScratch[phiSector/2] = largerPhiSector;
      }
      barrier(CLK_LOCAL_MEM_FENCE);
      numPhiSectors -= numPhiSectors/2;
    }
    if(phiSector == 0){
      maxCorr2[eventInd] = largerCorr;
      maxTheta2[eventInd] = largerThetaInd;
      maxPhi2[eventInd] = largerPhiInd;
      maxPhiSector[eventInd] = largerPhiSector;
    }
  }
}

__kernel void coherentlySumWaveForms(__global int* maxTheta2,
				     __global int* maxPhi2,
				     __global int* maxPhiSector, 
				     __global float* normalData,
				     __global float4* coherentWaveBuffer,
				     __global float* rmsBuffer,
				     __global short* lookup){

  int eventInd = get_global_id(1);
  int phiSector = maxPhiSector[eventInd];
  int thetaInd = maxTheta2[eventInd];
  int phiInd = maxPhi2[eventInd];
  int sampBase = 4*get_global_id(0);


  // first get waveform for central antenna in the phi sector
  int ant = phiSector + 16;
  int dataInd = eventInd*NUM_ANTENNAS*NUM_SAMPLES + ant*NUM_SAMPLES;
  int samp = dataInd + sampBase;

  float4 coherentValue = 0;
  float rms = rmsBuffer[eventInd*NUM_ANTENNAS + ant];
  coherentValue.s0 = rms*normalData[samp];
  coherentValue.s1 = rms*normalData[samp+1];
  coherentValue.s2 = rms*normalData[samp+2];
  coherentValue.s3 = rms*normalData[samp+3];

  // then want other antennas in +-1 phi sector in combination with the central one
  // which corresponds to these entries in the combo array...
  // N.B. negative if order is ant-centralAnt and positive if order is centralAnt-ant
  /* int combosWeWant[8] = {-4,  -7, -10, -12,  */
  /* 			 13,  18,  19,  20}; */
  int8 combosWeWant = (int8)(-4, -7, -10, -12, 13, 18, 19, 20);

  // probably more systematic way than this... but for now
  int8 ants;

  // phiSector -1
  ants.s0 = phiSector == 0 ? 15 : phiSector - 1;
  ants.s1 = ants.s0 + 16;
  ants.s2 = ants.s1 + 16;
  // phiSector
  ants.s3 = phiSector;
  ants.s4 = phiSector + 32;
  // phiSector + 1
  ants.s5 = phiSector == 15 ? 0 : phiSector + 1;
  ants.s6 = ants.s5 + 16;
  ants.s7 = ants.s6 + 16;

  for(int selectedComboInd = 0; selectedComboInd < 8; selectedComboInd++){
    //    for(int selectedComboInd = 0; selectedComboInd < 1; selectedComboInd++){

    int ant, localCombo;
    if(selectedComboInd==0){
      ant = ants.s0;
      localCombo = combosWeWant.s0;
    }
    else if(selectedComboInd==1){
      ant = ants.s1;
      localCombo = combosWeWant.s1;
    }
    else if(selectedComboInd==2){
      ant = ants.s2;
      localCombo = combosWeWant.s2;
    }
    else if(selectedComboInd==3){
      ant = ants.s3;
      localCombo = combosWeWant.s3;
    }
    else if(selectedComboInd==4){
      ant = ants.s4;
      localCombo = combosWeWant.s4;
    }
    else if(selectedComboInd==5){
      ant = ants.s5;
      localCombo = combosWeWant.s5;
    }
    else if(selectedComboInd==6){
      ant = ants.s6;
      localCombo = combosWeWant.s6;
    }
    else{
      ant = ants.s7;
      localCombo = combosWeWant.s7;
    }

    // -1 or 1 depending on order of antennas in combo to correct deltaT
    int orderFactor = localCombo < 0 ? -1 : 1;
    localCombo = abs(localCombo); // once order info has been used...
    //    ant = ants[selectedComboInd];
    rms = rmsBuffer[eventInd*NUM_ANTENNAS + ant];

    uint lookupInd = (phiSector*NUM_LOCAL_COMBOS_PER_PHI_SECTOR*NUM_BINS_THETA*NUM_BINS_PHI
  		     + localCombo*NUM_BINS_THETA*NUM_BINS_PHI
  		     + NUM_BINS_THETA*phiInd
  		     + thetaInd);


    //    short dt = orderFactor*(lookup[lookupInd]);
    short dt = lookup[lookupInd];
    dt = orderFactor < 0 ? NUM_SAMPLES - dt : dt;

    dataInd = eventInd*NUM_ANTENNAS*NUM_SAMPLES + ant*NUM_SAMPLES;
    samp = dataInd + sampBase + dt;
    
    coherentValue.s0 += (samp >= dataInd && samp < dataInd+NUM_SAMPLES) ? rms*normalData[samp] : 0;
    coherentValue.s1 += (samp+1 >= dataInd && samp+1 < dataInd+NUM_SAMPLES) ? rms*normalData[samp+1] : 0;
    coherentValue.s2 += (samp+2 >= dataInd && samp+2 < dataInd+NUM_SAMPLES) ? rms*normalData[samp+2] : 0;
    coherentValue.s3 += (samp+3 >= dataInd && samp+3 < dataInd+NUM_SAMPLES) ? rms*normalData[samp+3] : 0;
      
  }

  coherentWaveBuffer[(eventInd*NUM_SAMPLES + sampBase)/4] = coherentValue;

}

__kernel void makeHilbertEnvelope(__global float4* coherentWaveBuffer,
				  __local float2* complexScratch,
				  __global float4* hilbertEnvelopeBuffer){

  short eventInd = get_global_id(1);
  short dataInd = get_global_id(0);
  //  int sampBase = 4*get_global_id(0);

  float4 coherentWave = coherentWaveBuffer[eventInd*NUM_SAMPLES/4 + dataInd];

  // FFT input
  float8 coherentFFTinput = (float8)(coherentWave.x, 0,
  				     coherentWave.y, 0,
  				     coherentWave.z, 0,
  				     coherentWave.w, 0);

  float8 coherentFFToutput = doRadix2FFT(dataInd, coherentFFTinput, complexScratch, 1);

  /*
    Need to multiply by -i sgn(freq) and then iFFT to get hilbert transform.
    Here sgn is the signum function def= -1 for freq <0, 0 for freq = 0, and 1 for freq > 0.
    Of the 1024 samples, 0 is the 0th freq, 1-512 are +1, 513-1023 are -1.
  */
  
  coherentFFToutput = (float8)(coherentFFToutput.s1, -coherentFFToutput.s0,
			       coherentFFToutput.s3, -coherentFFToutput.s2,
			       coherentFFToutput.s5, -coherentFFToutput.s4,
			       coherentFFToutput.s7, -coherentFFToutput.s6);

  coherentFFToutput = dataInd < 128 ? -1*coherentFFToutput : coherentFFToutput; /* Signum function */
  coherentFFToutput.s01 = dataInd == 0 ? 0 : coherentFFToutput.s01; /* Signum function */  
  coherentFFToutput.s01 = dataInd == 128 ? 0 : coherentFFToutput.s01; /* Signum function */

  /* Do the inverse fourier transform */
  float8 hilbertStep2 = doRadix2FFT(dataInd, coherentFFToutput, complexScratch, -1);

  // definition of hilbert envelope
  float4 hilbertEnvelope = sqrt(coherentWave*coherentWave + hilbertStep2.even*hilbertStep2.even);
  //  float4 hilbertEnvelope = sqrt(hilbertTransform*hilbertTransform);

  //  hilbertEnvelope = sqrt(hilbertEnvelope);

  //  float4 hilbertEnvelope = hilbertTransform;
  //  float4 hilbertEnvelope = hilbertStep2.even;

  // push to buffer
  hilbertEnvelopeBuffer[eventInd*NUM_SAMPLES/4 + dataInd] = hilbertEnvelope;
  //  hilbertEnvelopeBuffer[eventInd*NUM_SAMPLES/4 + dataInd] = hilbertTransform;
  //  hilbertEnvelopeBuffer[eventInd*NUM_SAMPLES/4 + dataInd] = dataInd;

}



__kernel void findHilbertEnvelopePeak(__global float4* coherentWave,
				      __local float* scratch,
				      __global float* absCoherentMax){
  
  int localInd = get_global_id(0);
  int eventInd = get_global_id(1);

  // get abs values
  float4 tempCoherent = coherentWave[eventInd*NUM_SAMPLES/4 + localInd];
  tempCoherent.x = fabs(tempCoherent.x);
  tempCoherent.y = fabs(tempCoherent.y);
  tempCoherent.z = fabs(tempCoherent.z);
  tempCoherent.w = fabs(tempCoherent.w);

  // find largest
  float largerCoherent = -1; // this will be smaller than the smallest since we have taken abs of each value
  largerCoherent = tempCoherent.y > tempCoherent.x ? tempCoherent.y : tempCoherent.x;
  largerCoherent = largerCoherent > tempCoherent.z ? largerCoherent : tempCoherent.z;
  largerCoherent = largerCoherent > tempCoherent.w ? largerCoherent : tempCoherent.w;

  // put into LDS
  scratch[localInd] = largerCoherent;
  barrier(CLK_LOCAL_MEM_FENCE);
  
  //  int numSamples = get_global_size(0);
  int numSamples = NUM_SAMPLES/4;
  largerCoherent = -1; // this will be smaller than the smallest since we have taken abs of each value
  float coherent1 = 0;
  float coherent2 = 0;
  while(numSamples > 1){
    if(localInd%2 == 0 && localInd < numSamples){
      coherent1 = scratch[localInd];
      coherent2 = scratch[localInd + 1];
      largerCoherent = coherent2 > coherent1 ? coherent2 : coherent1;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    if(localInd%2 == 0 && localInd < numSamples){
      scratch[localInd/2] = largerCoherent;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    numSamples -= numSamples/2;
  }

  if(localInd == 0){
    absCoherentMax[eventInd] = largerCoherent;
  }
}




__kernel void fourierTransformNormalizedData(__global float4* realIn,
					     __global float8* complexOut,
					     __local float2* complexScratch){

  // Get work item identifiers to figure out which samples to do.
  int dataInd = get_global_id(0);
  int antInd = get_global_id(1);
  int eventInd = get_global_id(2);

  // will pass to the FFT function
  float8 fftInput;

  // Use WI itentifiers to get data.
  int globalDataInd = eventInd*NUM_ANTENNAS*NUM_SAMPLES/4 + antInd*NUM_SAMPLES/4 + dataInd;

  // Forwards FFT...
  float4 data = realIn[globalDataInd];

  // Setting imaginary component as zero.
  fftInput = (float8)(data.x, 0,
		      data.y, 0,
		      data.z, 0,
		      data.w, 0);

  float8 fftOutput = doRadix2FFT(dataInd, fftInput, complexScratch, 1);

  // Forward FFT
  complexOut[globalDataInd] = fftOutput;
}


__kernel void makeAveragePowerSpectrumForEvents(__global float4* ftWaves,
						__global float* rmsBuffer,
						__global float2* powSpecOut,
						__local float2* powSpecScratch,
						__global short2* passFilterBuffer,
						__global float* binToBinDifferenceThresh_dBBuffer,
						__global int* numEventsBuffer,
						__local short2* passFilterLocalBuffer,
						__global float* absMagnitudeThresh_dBmBuffer){
  /*
    Sum Power spectra of each antenna when for each event in the batch sent to the GPU.
    I'm going to use a slightly hacky power spectrum here.
    Since the Condor 3000x likes multiples of pow(2,n),
    I will drop the Nyquist frequency bin for simplicity.
    Since everything outside the band is filtered, this doesn't actually matter...
    so NUM_SAMPLES time samples -> NUM_SAMPLES/2+1 freqs -> drop last bin -> NUM_SAMPLES/2 freqs
  */

  // WI itendifiers
  int sampInd = get_global_id(0);
  int ant = get_global_id(1);
  float rms = rmsBuffer[ant];
  float var = rms*rms;
  int numEvents = numEventsBuffer[0];

  float2 summedPowSpec = 0;

  float binToBinDifferenceThresh_dB = binToBinDifferenceThresh_dBBuffer[0];
  float maxThreshold_dBm = absMagnitudeThresh_dBmBuffer[0];

  for(int event=0; event<numEvents; event++){
    float4 samples = ftWaves[event*NUM_ANTENNAS*NUM_SAMPLES/2 + ant*NUM_SAMPLES/2 + sampInd];
    float2 samplesPow = (float2)(samples.x*samples.x + samples.y*samples.y,
  				 samples.z*samples.z + samples.w*samples.w);
    summedPowSpec += samplesPow*var;
  }

  summedPowSpec *= 2; // For negative frequencies
  summedPowSpec /= numEvents; // Average of all events

  // Put in scratch so we can share with other WIs
  powSpecScratch[sampInd] = summedPowSpec;

  // Write unfiltered power spectrum to global buffer
  powSpecOut[ant*NUM_SAMPLES/4 + sampInd] = summedPowSpec;

  summedPowSpec.x = 10*log10(summedPowSpec.x/50);
  summedPowSpec.y = 10*log10(summedPowSpec.y/50);
  powSpecScratch[sampInd] = summedPowSpec;

  barrier(CLK_LOCAL_MEM_FENCE);

  // Grab neighbouring result for difference
  float y3 = sampInd < get_global_size(0) -1 ? powSpecScratch[sampInd + 1].x : summedPowSpec.y;

  float2 diff;
  diff.x = sampInd>0 ? summedPowSpec.y - summedPowSpec.x : 0; // 0th will be tiny due to zero meaning...
  diff.y = y3 - summedPowSpec.y;

  // difference cut
  short2 passFilter;
  passFilter.x = fabs(diff.x) > binToBinDifferenceThresh_dB ? -1 : 0;
  passFilter.y = fabs(diff.y) > binToBinDifferenceThresh_dB ? -1 : 0;
  
  // maximum bin value cut
  passFilter.x += summedPowSpec.x > maxThreshold_dBm ? -2 : 0;
  passFilter.y += summedPowSpec.y > maxThreshold_dBm ? -2 : 0;
  
  // Finally the band pass...
  float highPass = 200; // MHz
  float lowPass = 1200; // MHz

  // NOMINAL_SAMPLING in ns, 1e3 takes us from GHz to MHz
  float deltaF = 1000.0/(NUM_SAMPLES*NOMINAL_SAMPLING); // MHz

  // Factor of 2 since using float2s for data...
  int highPassWI = highPass/(deltaF*2);
  int lowPassWI = lowPass/(deltaF*2);
  
  // Easy stuff first. Implement bandpass frequencies.
  passFilter.x += 2*sampInd*deltaF > highPass && 2*sampInd*deltaF < lowPass ? 0 : -4;
  passFilter.y += (2*sampInd+1)*deltaF > highPass && (2*sampInd+1)*deltaF < lowPass ? 0 : -4;

  // set anything still at 0 to 1 in order to pass...
  passFilter.x = passFilter.x < 0 ? passFilter.x : 1;
  passFilter.y = passFilter.y < 0 ? passFilter.y : 1;

  passFilterBuffer[ant*NUM_SAMPLES/4 + sampInd] = passFilter;
}



__kernel void filterWaveforms(__global short2* passFilterBuffer,
			      __global float4* waveformsFourierDomain){
  /* 
     This kernel just multiplies a bin in the fourier domain by zero or one,
     contained in the the passFilter buffer.
     Pretty simple really.
  */

  int sampInd = get_global_id(0);
  int antInd = get_global_id(1);
  int eventInd = get_global_id(2);

  short2 filterState = passFilterBuffer[antInd*NUM_SAMPLES/4 + sampInd];
  filterState.x = filterState.x < 0 ? 0 : filterState.x;
  filterState.y = filterState.y < 0 ? 0 : filterState.y;

  int corrBaseInd = eventInd*NUM_ANTENNAS*NUM_SAMPLES/2;
  int sampInd0 = antInd*NUM_SAMPLES/2 + sampInd;
  int sampInd1 = antInd*NUM_SAMPLES/2 + NUM_SAMPLES/2 - (sampInd+1);

  int corrInd0 = corrBaseInd + sampInd0;
  int corrInd1 = corrBaseInd + sampInd1;

  float4 corr0 = waveformsFourierDomain[corrInd0];
  float4 corr1 = waveformsFourierDomain[corrInd1];

  float4 filteredCorr0;
  filteredCorr0.xy = filterState.x*corr0.xy;
  filteredCorr0.zw = filterState.y*corr0.zw;

  // symmetry of power spectrum -> swapping filterState indices
  float4 filteredCorr1;
  filteredCorr1.xy = filterState.y*corr1.xy;
  filteredCorr1.zw = filterState.x*corr1.zw;

  waveformsFourierDomain[corrInd0] = filteredCorr0;
  waveformsFourierDomain[corrInd1] = filteredCorr1;

}

__kernel void invFourierTransformFilteredWaveforms(__global float4* normalData,
						   __global float8* filteredWaveforms,
						   __local float2* complexScratch,
						   __global int* maxPhiSectorBuffer){
  /*
    Call the fft function again to get waveforms back to the time domain.
    Even though each waveform was filtered before cross correlation...
  */

  // Get work item identifiers to figure out which samples to do.
  int dataInd = get_global_id(0);
  int antInd = get_global_id(1);
  int eventInd = get_global_id(2);

  int maxPhiSector = maxPhiSectorBuffer[eventInd];
  int deltaPhiSect = (antInd%NUM_PHI_SECTORS) - maxPhiSector;

  // wrap around
  deltaPhiSect = deltaPhiSect > 7 ? deltaPhiSect - NUM_PHI_SECTORS : deltaPhiSect;
  deltaPhiSect = deltaPhiSect < -7 ? deltaPhiSect + NUM_PHI_SECTORS : deltaPhiSect;
  
  /* if(deltaPhiSect < 2 && deltaPhiSect > -2){ */
    // will pass to the FFT function
    float8 fftInput;

    // Use WI itentifiers to get data.
    int globalDataInd = eventInd*NUM_ANTENNAS*NUM_SAMPLES/4 + antInd*NUM_SAMPLES/4 + dataInd;

    // Normalize inverse fft input by dividing by number of samples.
    float8 data = filteredWaveforms[globalDataInd]/NUM_SAMPLES;

    // Swap re and im whilst preparing to put data into FFT function.
    fftInput = (float8) (data.s1, data.s0, 
			 data.s3, data.s2, 
			 data.s5, data.s4, 
			 data.s7, data.s6);

    float8 fftOutput = doRadix2FFT(dataInd, fftInput, complexScratch, 1);

    float4 output;
    // The output also needs to be swapped.
    // so put imaginary parts into output array.
    output.x = fftOutput.s1;
    output.y = fftOutput.s3;
    output.z = fftOutput.s5;
    output.w = fftOutput.s7;

    normalData[globalDataInd] = output;
  /* } */
}
