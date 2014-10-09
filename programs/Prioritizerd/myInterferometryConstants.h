/*
  Author: Ben Strutt
  Email: b.strutt.12@ucl.ac.uk
  Description: My attempt to hide all the GPU complexities behind some plug in and play functions...
*/

// Note, only using #define when those "variables" are used in definition of other constants
// I've found arithmetic with "variables" declared with #define not to work
// The compiler doesn't know what to do with them... 

#ifndef MYINTERFEROMETRYCONSTANTS_H
#define MYINTERFEROMETRYCONSTANTS_H

/* Warning! There is a significant amount of hard coded dependancy on NUM_SAMPLES == 1024
   naively changing this number will really bugger up the code.
   Since the ANITA-3 flight is using the LAB3 digitizers, NUM_SAMPLES == 251ish.
   In fact, NUM_SAMPLES == (251ish * 2)ish since now the firmware reads out two chips in succession
   with approximately 90ns of delay, which is shorter than the ~100ns of single labchip readout.
   So there is some overlap.
   For now, let's do the simplest thing first, pretend there's no overlap and give the number of 
   samples to the GPU and zero pad the waveforms to 1024 samples long.
   Essay comment over.
*/   

#define NUM_SAMPLES 1024
#define NUM_ANTENNAS 48
#define NUM_POLARIZATIONS 2
#define NUM_GLOBAL_COMBOS_PER_PHI_SECTOR 12
#define NUM_LOCAL_COMBOS_PER_PHI_SECTOR 27
#define NUM_EVENTS 128
#define NUM_PHI_SECTORS 16
#define NUM_RINGS 3
#define NOMINAL_SAMPLING 1./2.6f /*0.25f*/
#define NUM_BINS_PHI 64 /* This is for a single phi-sector */
#define NUM_BINS_THETA 256
#define PI 3.141592653589f
#define SPEED_OF_LIGHT 2.99792458e8

#endif
