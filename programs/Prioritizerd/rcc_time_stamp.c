/************************************************************************/
/*                                                                      */
/*   file: tstamp.c                                                     */
/*   author: Markus Joos, CERN-EP/ESS                                   */
/*                                                                      */
/*   This is the timestamping library.                                  */
/*                                                                      */
/*   History:                                                           */
/*   13.Aug.98  MAJO  created                                           */
/*   22.Sep.98  MAJO  Separate header file created                      */
/*   26.Feb.99  MAJO  additional run control functions added            */
/*    5.Mar.99  MAJO  more functions added                              */
/*   26.Jul.99  JOP   change algorithm in ts_duration                   */
/*   27.Jul.99  JOP   NGU  PC - i386 functions added                    */
/*    2.Aug.99  MAJO  Improved algorithm in ts_duration        		*/
/*   16.Aug.99  NGU   cpu frequency reading added for Linux             */
/*   13.Aug.01  MAJO  cpu frequency reading added for CES RIO3          */
/*                    IOM error system introduced                       */
/*   16.Jul.02  MAJO  Ported to RCC environment				*/
/*   23.Sep.02  MAJO  ts_wait added					*/
/*   19.Oct.04  MAJO  ts_wait_until, ts_offset and ts_compare added	*/
/*                                                                      */
/*******Copyright Ecosoft 2007 - Made from 80% recycled bytes************/ 

#include "tstamp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/file.h>
#include <sys/types.h>
//#include "DFDebug/DFDebug.h"
//#include "rcc_error/rcc_error.h"

// Globals
static int isopen = 0, got_freq = 0, tst0 = 0;
static double frequency_high, frequency_low, frequency_high_us, frequency_low_us;
static tstamp tzero;
static FILE *dfile;

int tsmode[MAX_HANDLES], tsnum[MAX_HANDLES], tsmax[MAX_HANDLES], tsactive[MAX_HANDLES];
tstamp *ts[MAX_HANDLES], *tsp;

/******************************/
TS_ErrorCode_t ts_get_freq(void)
/******************************/
{
  FILE *fd;
  int i1;
  float Frequency_low_us = 1000;
  char dumc[64];

  // Determine the frequency of the clock 
  if ((fd = fopen("/proc/cpuinfo", "r") ) == 0)
  { 
     fprintf(stderr, "ts_get_freq: cannot open the file cpuinfo\n");
     return(TSE_NO_FREQ); 
  }
  /* BAFS - 20th June 2014
     The original idiot author set this to be i1<30 and was failing on fedora 20 anita-3 install.
     One should do some EOF checking, but who has the time?
     This still isn't good, but fuck it - it works.*/
  for(i1 = 1; i1 < 100; i1++) 
  {
    fscanf (fd, "%s ", dumc);
    /*    printf("%d - %s\n ", i1, dumc);*/
    if (strcmp(dumc, "MHz") == 0) 
      break;
  }
  
  fscanf (fd, "%s ", dumc);
  /*  printf("dumc = %s\n", dumc);  */
  fscanf (fd, "%f", &Frequency_low_us);
  /*  printf("  low - %4.2f\n", frequency_low);*/
  frequency_low_us = (double)Frequency_low_us;
  frequency_high_us = (frequency_low_us) / 4294967296.0;
  fclose(fd);

  frequency_high = frequency_high_us * 1000000.0;
  frequency_low = frequency_low_us * 1000000.0;

  got_freq = 1;

  /* BAFS addition - debug */
  /*
  printf("%s\n", dumc);
  printf("Clock speeds from /proc/cpuinfo...\n");
  printf("  low - %4.2f\n", frequency_low);
  printf("  low us- %d\n", frequency_low_us);
  printf("  high - %d\n", frequency_high);
  printf("  high us - %4.2f\n", frequency_high_us);
  */

  return(0);
}


/******************************************/
TS_ErrorCode_t ts_open(int size, int handle)
/******************************************/
{
  TS_ErrorCode_t ret;

  if (!got_freq)
  {
    ret = ts_get_freq();
    if (ret)
    {
       return(1);
    }
  }

  if (handle != 0){  
     if (handle > MAX_HANDLES || handle < 1) {
        return 1;
     }

     // Allocate an aray for the time stamps
     tsmax[handle] = size;
     if (size) {
        ts[handle] = (tstamp *)malloc(size * sizeof(tstamp));
     }
     else {
        return 1;
     }

     tsactive[handle] = TS_STOP_TS;
     tsnum[handle] = 0;
     tsmode[handle] = TS_MODE_NORING;
  }

  isopen++;
  return 0;
}


/********************************************/
TS_ErrorCode_t ts_mode(int handle, u_int mode)
/********************************************/
{
  int loop;
  
  if (!isopen)
  {
     return TSE_IS_CLOSED;
  }   
  
  if (handle > MAX_HANDLES || handle < 1)
  {
     return TSE_ILL_HANDLE;
  }
  
  if (mode != TS_MODE_RING && mode != TS_MODE_NORING){
     return TSE_ILL_MODE;
  }
    
  tsmode[handle] = mode;

  if (mode == TS_MODE_RING) {
    for(loop = 0; loop < tsmax[handle]; loop++) {
      tsp = (tstamp *)&ts[handle][loop];
      tsp->low = 0;
      tsp->high = 0;
      tsp->data = 0;
    }
  }
  return 0;
}


/********************************************/
TS_ErrorCode_t ts_save(int handle, char *name){
/********************************************/
  // Write the time stamps from the array to an ASCII file and reset the array
  int res, loop, num;
  if (handle > MAX_HANDLES || handle < 1)
    return TSE_ILL_HANDLE;
    
  if(tsmode[handle] == TS_MODE_RING || tsmax[handle]) { 
    dfile = fopen(name,"w+");
    if (dfile == 0) {
      return TSE_PFILE;
    }

    // Save the frequencies
    fprintf(dfile, "%e\n", frequency_high);
    fprintf(dfile, "%e\n", frequency_low);

    if (tsmode[handle] == TS_MODE_RING)
      num = tsmax[handle];
    else
      num = tsnum[handle];
      
    for(loop = 0; loop < num; loop++)
      fprintf(dfile, "%u %u %u\n", ts[handle][loop].high, ts[handle][loop].low, ts[handle][loop].data);

    res = fclose(dfile);
    if (res) {
      return TSE_PFILE;
    }
    tsnum[handle] = 0;
  } 
  return TSE_OK;
}


/*********************************/
TS_ErrorCode_t ts_close(int handle)
/*********************************/
{
  
  if (handle > MAX_HANDLES || handle < 0)
    return TSE_ILL_HANDLE;
    
  if (isopen > 0) {
    isopen--;
    if (handle)
      free((void *)ts[handle]);
  }
  else {
    return TSE_IS_CLOSED;
  }
  return TSE_OK;
}


/*********************************************/


/***************************/
TS_ErrorCode_t ts_sett0(void)
/***************************/
{
  if (!isopen) {
     return TSE_IS_CLOSED;
  }

  if (!tst0)  {
    ts_clock(&tzero);
    tst0 = 1;
  }

  return TSE_OK;
}
 

/*********************************/
TS_ErrorCode_t ts_start(int handle)
/*********************************/
{
  if (!isopen) {
     return TSE_IS_CLOSED;
  }
  
  if (handle > MAX_HANDLES || handle < 1)
    return TSE_ILL_HANDLE;
    
  tsactive[handle] &= TS_START_TS;

  return TSE_OK;
}


/*********************************/
TS_ErrorCode_t ts_pause(int handle)
/*********************************/
{
  if (!isopen) {
     return  TSE_IS_CLOSED;
  }
  
  if (handle > MAX_HANDLES || handle < 1)
    return TSE_ILL_HANDLE;
    
  tsactive[handle] |= TS_STOP_TS;
  return TSE_OK;
}


/**********************************/
TS_ErrorCode_t ts_resume(int handle)
/**********************************/
{
  if (!isopen) {
     return TSE_IS_CLOSED;
  }
  
  if (handle > MAX_HANDLES || handle < 1)
    return TSE_ILL_HANDLE;
    
  tsactive[handle] &= TS_START_TS;
  return TSE_OK;
}


/************************************************/
TS_ErrorCode_t ts_elapsed (tstamp t1, float *time)
/************************************************/
{
  // Calculate the time elapsed between a time stamp and tzero in us
  float fdiff, low, hi;
  int diff;

  if (!isopen) {
     return TSE_IS_CLOSED;
  }

  if (!tst0)  // tzero has not yet been recorded
    return TSE_NO_REF;

  diff = t1.low - tzero.low;
  fdiff = (float)diff;
  low = fdiff / frequency_low_us;

  diff = t1.high - tzero.high;
  fdiff = (float)diff;
  hi = fdiff / frequency_high_us;

  *time = hi + low;
  return TSE_OK;
}


/*************************************/
float ts_duration(tstamp t1, tstamp t2)
/*************************************/
{
  // Calculate the time elapsed between two time stamps (for on-line processing)
  double time1, time2;
  int ret;
  
  if (!isopen) {
     return TSE_IS_CLOSED;
  }

  if (!got_freq)
  {
    ret = ts_get_freq();
    if(ret)
      return TSE_NO_FREQ;
  }
  time1 = (double)t1.high / frequency_high + (double)t1.low / frequency_low;
  time2 = (double)t2.high / frequency_high + (double)t2.low / frequency_low;
  return (time2 - time1);
}


/**********************************/
int ts_compare(tstamp t1, tstamp t2) 
/**********************************/
{
  // Compare two timestamps
  // Returns:
  //  1 if t1>t2
  // -1 if t1<t2 
  //  0 if t1=t2

  if (!isopen) {
     return TSE_IS_CLOSED;
  }
    
  if (t1.high > t2.high) return 1;
  if (t1.high < t2.high) return -1;
  if (t1.low > t2.low) return 1;
  if (t1.low < t2.low) return -1;
  return 0;  
}


/***********************************************/
TS_ErrorCode_t ts_offset(tstamp *ts, u_int usecs)
/***********************************************/
{
  // Offset a given timestamp ts by a given time in usec  

  long long int th64, ticks, ts64;
  
  if (!isopen) {
     return TSE_IS_CLOSED;
  }

  ticks = (unsigned long long int)((double)usecs * frequency_low_us);

  
  th64 = ts->high;
  ts64 = (th64 << 32) + ts->low;

  ts64 += ticks;


  ts->low = ts64 & 0xffffffff;
  ts->high = ts64 >> 32;
  
  
  return TSE_OK;
}


/***********************************/
TS_ErrorCode_t ts_clock(tstamp *time)
/***********************************/
{
  // Record a time stamp for on-line processing
  u_int a, b;

  if (!isopen) {
     return TSE_IS_CLOSED;
  }

  __asm__ volatile ("rdtsc" :"=a" (b), "=d" (a));
  time->high = a;
  time->low = b;
  return TSE_OK;
}


/* 
   BAFS copied from tstamp.h to avoid problems with linker in C.
   Moral of the story - don't define the function in the header in C!
*/
inline TS_ErrorCode_t ts_record(int handle, int udata){
  // Record a time stamp together with an user defined identifier if there is space left in the array
   tstamp *tsp;
   if (tsactive[handle] == 0) {
    tsp = (tstamp *)&ts[handle][tsnum[handle]];
    __asm__ volatile  ("rdtsc":"=a" (tsp->low), "=d" (tsp->high));
    tsp->data = udata;
    if (++tsnum[handle] >= tsmax[handle]) {
      if (tsmode[handle] == TS_MODE_NORING)
        tsactive[handle] |= TS_FULL;
      else
        tsnum[handle] = 0;
    }
  }
  return TSE_OK;
}
