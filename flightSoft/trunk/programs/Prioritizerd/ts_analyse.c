/*
Executable for partial analysis of the output generating by rcc_time_stamp library.
Original file inherited from Gordon Crone <g.crone@ucl.ac.uk> (I don't know who he got it from).
I have modified it so the delta_t output is in unis of us (microsconds, 1us = 10^-6 s).
I have also changed the output location to match my directory structure.

Ben Strutt <b.strutt.12@ucl.ac.uk>
March 2014
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include "tstamp.h"

#define MAX_EL 100000


typedef struct
{
  u_int num;
  float range;
} T_histo;


int main(int argc, char *argv[])
{
  int anum, num, dnum, first;
  u_int nbins, need,starte,stope,dmin, dmax, delta[MAX_EL];
  char inname[200], outname1[250], outname2[250];
  double sum, tic_to_us, fsum, fddelta, fmin, fmax, fdelta[MAX_EL], f_hi, f_lo, h_min, h_max;
  u_int64_t t1l, t2l;
  T_histo *histo;
  FILE *inf,*outf1,*outf2;
  tstamp t1, t2, data[MAX_EL];

  if (argc < 4)
  {
    printf("use: %s <filename><start_event><stop_event>[<nbins>[<h_min>[<h_max>]]]\n",argv[0]);
    printf("  Read data from file <filename> and histogram the delta-t distribution of the\n");
    printf("  time interval defined by <start_event> and <stop_event> The optional parameters\n");
    printf("  h_min and h_max are the boundaries of the range to be histogrammed. If no\n");
    printf("  value is specified for <bins>, the histogram will have 20 bins.\n");
    printf("  \n");
    printf("  Analyse will create two ASCII files:\n");
    printf("  <filename>_dt will be a list with the delta-t values calculated in units\n");
    printf("  of us (converted from clock tics).\n");
    printf("  <filename>_hist will contain the histogam data plus an overflow and an underflow bin\n");
    printf("  \n");
    printf("  Formats:\n");
    printf("  filename               => string\n");
    printf("  start_event,stop_event => u_int\n");
    printf("  h_min,h_max            => exp (e.g. 0.123456e+01)\n");
    exit(0);
   }
  if ((argc == 7) && (sscanf(argv[6], "%le", &h_max)  == 1)) {argc--;} else {h_max = -1.0;}
  if ((argc == 6) && (sscanf(argv[5], "%le", &h_min)  == 1)) {argc--;} else {h_min = -1.0;}
  if ((argc == 5) && (sscanf(argv[4], "%u",  &nbins)  == 1)) {argc--;} else {nbins = 20;}
  if ((argc == 4) && (sscanf(argv[3], "%u",  &stope)  == 1)) {argc--;} else {exit(0);}
  if ((argc == 3) && (sscanf(argv[2], "%u",  &starte) == 1)) {argc--;} else {exit(0);}
  if ((argc == 2) && (sscanf(argv[1], "%s",  inname)  == 1)) {argc--;} else {exit(0);}

  nbins += 2;
  histo = (T_histo *)malloc((nbins) * sizeof(T_histo));
  if(histo == NULL)
  {
    printf("Cannot allocate histogram buffer\n");
    exit(0);
  }
  sprintf(outname1, "../timingAnalysis/ts_%03d-%03d.dt", starte, stope);
  sprintf(outname2, "../timingAnalysis/%s_hist", inname);
  printf("Delta t values will be written to file <%s>\n", outname1);
  printf("Histogram data will be written to file <%s>\n", outname2);

  /*open files*/
  inf = fopen(inname, "r");
  if(inf == 0)
  {
    printf("Can't open input file\n");
    exit(0);
  }
  
  outf1 = fopen(outname1, "w+");
  if (outf1 == 0)
  {
    printf("Can't open output file 1\n");
    exit(0);
  }
  
  outf2 = fopen(outname2, "w+");
  if (outf2 == 0)
  {
    printf("Can't open output file 2\n");
    exit(0);
  }

  /*read the frequencies*/
  fscanf(inf, "%le", &f_hi);
  fscanf(inf, "%le", &f_lo);

  /*compute some constant(s)*/
  tic_to_us = (double)1000000.0 / (double)f_lo;

  /*read data*/
  num = 0;
  while(!feof(inf))
  {
    fscanf(inf, "%u", &data[num].high);
    fscanf(inf, "%u", &data[num].low);
    fscanf(inf, "%u", &data[num].data);
    num++;
    if (num == MAX_EL)
      break;
  }
  num--;

  /*close the input file*/
  fclose(inf);

  sum  = 0;
  anum = 0;
  dnum = 0;
  dmax = 0;
  dmin = UINT_MAX;
  if(starte == stope) {
    /*histogram a duration defined by one event*/
    first = 1;
    while(anum < num) { 
      if ((data[anum].data == starte)) {
	if(first) {
          t1 = data[anum];
          first = 0;
          anum++;
        }
	else {
          if(anum == num)
            anum++;
          else {
            t2 = data[anum];	    
	    t1l = ((u_int64_t)t1.high << 32) + t1.low;
	    t2l = ((u_int64_t)t2.high << 32) + t2.low;
	    if (t2l < t1l) {
	      printf("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
	      printf("ERROR T2 < T1\n");
	      printf("t1l        = %llu\n", t1l);
	      printf("t2l        = %llu\n", t2l);
	      printf("t1.high    = %u\n", t1.high);
	      printf("t1.low     = %u\n", t1.low);
	      printf("t2.high    = %u\n", t2.high);
	      printf("t2.low     = %u\n", t2.low);
	      printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	    } 
            delta[dnum] = t2l - t1l;
	    printf("delta[%d] = %u\n", dnum, delta[dnum]);

            if(delta[dnum] < dmin) 
	      dmin = delta[dnum]; 
            if(delta[dnum] > dmax) 
	      dmax = delta[dnum];
            sum += delta[dnum];
            dnum++;
            first = 1;
          }
        }
      }
      else
	anum++;
    } 
  }
  else {
    /*histogram a duration defined by two events*/
    need = starte;  
    while(anum <= num) {
      if (data[anum].data == starte) /*start event*/
      {
	t1 = data[anum];
	need = stope;
      }
      if ((data[anum].data == stope) && (need == stope)) /*stop event*/
      {
	t2 = data[anum];
	need = starte;
	t1l = ((u_int64_t)t1.high << 32) + t1.low;
	t2l = ((u_int64_t)t2.high << 32) + t2.low;
	if (t2l < t1l) {
	  printf("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
	  printf("ERROR T2 < T1\n");
	  printf("t1l        = %llu\n", t1l);
	  printf("t2l        = %llu\n", t2l);
	  printf("t1.high    = %u\n", t1.high);
	  printf("t1.low     = %u\n", t1.low);
	  printf("t2.high    = %u\n", t2.high);
	  printf("t2.low     = %u\n", t2.low);
	  printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	} 
	delta[dnum] = t2l - t1l;
	printf("delta[%d] = %u\n", dnum, delta[dnum]);
	if(delta[dnum] < dmin) 
	  dmin = delta[dnum]; 
	if(delta[dnum] > dmax) 
	  dmax = delta[dnum];
	sum += delta[dnum];
	dnum++;
      }
      anum++;
    }
  }
  /*
  use this algorithm to calculate delta t between the first occurences of t1 and t2
  else
    {
    need=starte;  
    while(anum<=num)
      {
      if ((data[anum].data==need) && (need==starte)) 
	{
	t1=data[anum];
	need=stope;
	}
      if ((data[anum].data==need) && (need==stope)) 
	{
	t2=data[anum];
	need=starte;
	delta[dnum]=t2.low-t1.low;
	if(delta[dnum]<dmin) dmin=delta[dnum]; 
	if(delta[dnum]>dmax) dmax=delta[dnum];
	sum+=delta[dnum];
	dnum++;
	}
      anum++;
      }
    }
  */


  /*write delta-t file (unit = clock tics)*/
  for(anum = 0; anum < dnum; anum++)
    fprintf(outf1, "%f\n", (double) delta[anum] * tic_to_us);
  fclose(outf1);

  /*convert delta-t values from clock tics to useconds*/
  for(anum = 0; anum < dnum; anum++)
    fdelta[anum] = (double)delta[anum] * tic_to_us;
  fmin = (double)dmin * tic_to_us;
  fmax = (double)dmax * tic_to_us;
  fsum = (double)sum / (double)dnum * tic_to_us;

  if(h_min != -1.0)
    fmin = (double)h_min;
  if(h_max != -1.0)
    fmax = (double)h_max;

  printf("%u durations calculated\n", dnum);
  printf("   fmin=%f us (dmin=%u clock tics)\n", fmin,dmin);
  printf("   fmax=%f us (dmax=%u clock tics)\n", fmax,dmax);
  printf("average=%f us \n", fsum);

  if(fmin >= fmax)
  {
    printf("ERROR: fmin >= fmax\n");
    exit(0);
  }

  fddelta = (fmax - fmin) / (double)(nbins - 2);

  /*initialise histogram*/
  for(anum = 1; anum < (nbins - 1); anum++)
  {
    histo[anum].range = (float)fmin + ((anum - 1) * (float)fddelta);
    histo[anum].num = 0;
  }
  histo[0].num = 0;  
  histo[(nbins - 1)].num = 0;

  /*fill histogram*/
  for(anum = 0; anum < dnum; anum++)
  {
    if(fdelta[anum] == fmax)       histo[(nbins - 2)].num++;
    else if(fdelta[anum] > fmax)   histo[(nbins - 1)].num++;
    else if(fdelta[anum] < fmin)   histo[0].num++;
    else                           histo[1 + (int)((fdelta[anum] - fmin) / fddelta)].num++;
  }

  /*write histogram to output file*/
  for(anum = 0; anum < nbins; anum++)
  {
    if(anum == 0)
      fprintf(outf2, "%u under flow\n", histo[anum].num);
    else if(anum == (nbins - 1))
      fprintf(outf2, "%u over flow\n", histo[anum].num);
    else
      fprintf(outf2, "%u %e\n", histo[anum].num, histo[anum].range);
  }
  fclose(outf2);
  free(histo);

  return 0;
}
