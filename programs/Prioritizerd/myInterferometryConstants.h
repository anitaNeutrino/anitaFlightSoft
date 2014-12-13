/*
  Author: Ben Strutt
  Email: b.strutt.12@ucl.ac.uk
*/

#ifndef MYINTERFEROMETRYCONSTANTS_H
#define MYINTERFEROMETRYCONSTANTS_H


/* 
   RADIX_BIT_SWITCH **MUST** = log2(NUM_SAMPLES)-1, 
   loops can be unrolled for faster GPUing 
   if log2(NUM_SAMPLES)-1 is known at compile time..
*/
//#define NUM_SAMPLES 1024
#define NUM_SAMPLES 256
//#define RADIX_BIT_SWITCH 9
#define RADIX_BIT_SWITCH 7

#define NUM_ANTENNAS 48
#define NUM_POLARIZATIONS 2
#define NUM_GLOBAL_COMBOS_PER_PHI_SECTOR 12
#define NUM_LOCAL_COMBOS_PER_PHI_SECTOR 27
#define NUM_EVENTS 128
#define NUM_PHI_SECTORS 16
#define NUM_RINGS 3
#define NOMINAL_SAMPLING (1./2.6) /*0.25f*/
#define NUM_BINS_PHI 64 /* This is for a single phi-sector */
#define NUM_BINS_THETA 256
#define PI 3.141592653589f
#define SPEED_OF_LIGHT 2.99792458e8
#define ALPHA_CHAN_INDEX 104

#endif
