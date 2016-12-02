#include "cpuPrioritizer.h" 
#include "anitaMapping.h" 
#include <string.h>
#include "anitaFlight.h"
#include <stdlib.h>


#define MAX_THRESHOLDS 5 

static float rms[12][8]; 
static int integration_length = 10;  
static float thresholds[MAX_THRESHOLDS]; 


int doAnalysis(int first, int last, const PedSubbedEventBody_t * body); 


int readCpuConfig()
{  

  return 0; 
}


typedef struct
{

  int Vmax; 
  int Vmin; 
  int ncycles[MAX_THRESHOLDS]; 
  float fraction_power_above [MAX_THRESHOLDS]; 
  int powerMax; 
  int powerAvg; 
  int powerMaxTime; 
} waveform_result_t; 


//ncycles goes up when we go from state high to state low or state low to state high  

typedef enum
{
  STATE_NEITHER,
  STATE_HIGH,
  STATE_LOW
} waveform_state_t;

int processWaveform(const SurfChannelPedSubbed_t * wf, waveform_result_t * result, float the_rms)
{
  int i,ii,j; 
  int power = 0;
  int nseen = 0;
  int ifirst = (wf->header.chanId & 3 ) ? wf->header.firstHitbus + 1 : 0; 
  int ilast = (wf->header.chanId & 3 ) ? wf->header.lastHitbus - 1 + MAX_NUMBER_SAMPLES : wf->header.firstHitbus-1; 
  float thresh[MAX_THRESHOLDS]; 
  waveform_state_t state[MAX_THRESHOLDS]; 


  result->Vmax = 0; 
  result->Vmin = 0; 
  result->powerMax = 0; 
  result->powerAvg = 0;

  for (i = 0; i < MAX_THRESHOLDS; i++) 
  {
    result->ncycles[i] = 0; 
    result->fraction_power_above[i] = 0; 
    thresh[i] = the_rms * thresholds[i]; 
    state[i] = STATE_NEITHER; 
  }

  for (ii = ifirst; ii <=ilast; ii++)
  {
    i = ii % MAX_NUMBER_SAMPLES; 

    power += wf->data[i]; 

    if (nseen >= integration_length-1) 
    {
      if (nseen >= integration_length)
      {
        power-= wf->data[i-integration_length]; 
      }

      if (power > result->powerMax)
      {
        result->powerMax = power; 
        result->powerMaxTime = nseen; 
      }

      result->powerAvg += power; 
      for (j = 0; j < MAX_THRESHOLDS; j++)
      {
        if (power > thresh[j])
          result->fraction_power_above[j]++; 
      }
    }

    
    if (wf->data[i] > result->Vmax)
    {
      result->Vmax = wf->data[i]; 
    }

    if (wf->data[i] < result->Vmin)
    {
      result->Vmin = wf->data[i]; 
    }


    for (j = 0;j < MAX_THRESHOLDS; j++)
    {
      switch (state[j]) 
      {
        case STATE_NEITHER: 
          if (wf->data[i] > thresh[j])
          {
            state[j] = STATE_HIGH; 
          }
          else if ( wf->data[i] < -thresh[j])
          {
            state[j] = STATE_LOW; 
          }

          break;

        case STATE_HIGH : 
          if (wf->data[i] <  -thresh[j])
          {
            result->ncycles[j]++; 
            state[j] = STATE_LOW; 
          }
          break;

        case STATE_LOW : 
          if (wf->data[i] >  thresh[j])
          {
            result->ncycles[j]++; 
            state[j] = STATE_HIGH; 
          }
          break;

        default: 
          break; 
      }

    }

    for (j = 0; j < MAX_THRESHOLDS; j++)
    {
      result->fraction_power_above[j] /= (MAX_NUMBER_SAMPLES - integration_length); 
    }

    result->powerAvg /= (MAX_NUMBER_SAMPLES - integration_length); 

    nseen++; 
  }

  return 0; 
}




int cpuPrioritizer(const AnitaEventHeader_t * hd, const PedSubbedEventBody_t*  body)
{


  //if this is a soft trigger, update the rms's 
  if (hd->turfio.trigType & 0xe)
  {
    int surf, chan; 
    for (surf = 0; surf < ACTIVE_SURFS; surf++) 
    {
      for (chan = 0; chan < CHANNELS_PER_SURF -1; chan++)
      {
        //TODO should we take an exponential average here?
        rms[surf][chan] = body->channel[surf * CHANNELS_PER_SURF + chan].rms;
      }
    }


    return 9; 
  }

  //find the triggered phi sectors. Analyze them and their neighbors
  unsigned short trig = hd->turfio.l3TrigPattern; 
  if (!trig) return 9; 

  unsigned n_triggered = __builtin_popcount(trig); 

  //TODO check the counting here
  int first = __builtin_ctz(trig); 
  int last = 15-(__builtin_clz(trig)-16); 

  if (n_triggered > 8)  // probably a payload blast.
    return 8; 

  if (last == 15 && first == 0) //hard case, this means we have a wraparound. also they're reversed
  {
    int tmp;
    while (trig & ( 1 << (1+first))) first++; 
    while (trig & ( 1 << (last-1))) last--; 

      //swap them 
    tmp = first; 
    first = last; 
    last = tmp; 
  }


  //add an extra phi sector on both sides

  first = (first -1) % 16; 
  last  = (last +1) %16; 


  return doAnalysis(first,last, body); 
}
    

int doAnalysis(int first, int last, const PedSubbedEventBody_t * body) 
{
  if (first > last) last += 16; 
  //now the hard part, have to decide how to assign the priority 
  // Factors: 
  //  -- mean SNR 
  //  -- mean PSNR 
  //  -- mean NCycles
  //  -- mean NFraction Above
  //  -- mean (powerMaxTimeBottom + powerMaxTimeTop)/2 - powerMaxTimeTop 


  float mean_snr = 0; 
  float mean_psnr = 0; 
  float mean_ncycles[MAX_THRESHOLDS]; 
  float mean_fraction_above[MAX_THRESHOLDS]; 
  memset(mean_ncycles, 0, sizeof(mean_ncycles));
  memset(mean_fraction_above, 0, sizeof(mean_fraction_above));
  int nphi = 0; 
  float mean_time_difference; 
  int i; 
 
  for (i = first; i <= last; i++) 
  {
       int real_i = i %16; 
       int pol, ring; 
       waveform_result_t result[3][2]; 
       for (ring = 0; ring < 3; ring++)
       {
         int ant = ring * NUM_PHI + real_i; 
         for (pol = 0; pol < 2; pol++)
         {
           int surf = antToSurfMap[ant]; 
           int chan = pol == 0 ? hAntToChan[ant] : vAntToChan[ant]; 
           int idx = surf * CHANNELS_PER_SURF + chan; 
           int ithresh; 
           processWaveform(&body->channel[idx], &result[ring][pol], rms[surf][chan]); 
           mean_snr += (result[ring][pol].Vmax-result[ring][pol].Vmin) / (2 * rms[surf][chan]); 
           mean_psnr += (result[ring][pol].powerMax) / (rms[surf][chan]); 
           for (ithresh = 0; ithresh < MAX_THRESHOLDS; ithresh++)
           {
             mean_ncycles[ithresh] += result[ring][pol].ncycles[ithresh]; 
             mean_fraction_above[ithresh] += result[ring][pol].fraction_power_above[ithresh]; 
           }
         }
       }
       mean_time_difference += (result[0][0].powerMaxTime + result[0][1].powerMaxTime+ result[1][0].powerMaxTime+ result[1][1].powerMaxTime)/4. - (result[2][0].powerMaxTime+ result[2][1].powerMaxTime)/2.; 
       nphi++; 
  }


  mean_snr /= (nphi * 6); 
  mean_psnr /= (nphi * 6); 
  mean_time_difference /= (nphi * 6); 



  return 0; 
}
