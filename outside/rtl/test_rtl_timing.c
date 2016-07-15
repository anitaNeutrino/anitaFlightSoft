#include "rtl-sdr.h" 
#include <time.h> 
#include <stdio.h>
#include <signal.h> 

static volatile int keepGoing = 1; 


uint32_t freqs[] = { 100e6, 150e6, 200e6, 250e6, 300e6, 350e6, 400e6, 450e6, 500e6 }; 
uint32_t buffer_size = 16384; 

int cancelHandler(int dummy) 
{
  keepGoing = 0; 

}

int main (int nargs, char ** args) 
{
  double time_tuning = 0; 
  double time_reading = 0; 
  double time_waiting = 0; 
  char buffer[buffer_size]; 
  int nread; 
  int r; 
  const size_t nfreqs = sizeof(freqs)/sizeof(*freqs); 
  size_t freq_index = 0; 
  struct timespec tp0; 
  struct timespec tp1; 
  int avgs = 0; 
 
  printf("   (Ctrl-c to stop)   \n"); 

  rtlsdr_dev_t * dev; 
  rtlsdr_open(&dev, 0); 
  rtlsdr_reset_buffer(dev); 
  rtlsdr_set_sample_rate(dev, 2.56e6); 
  usleep(10000); 

  while(keepGoing) 
  {
    
    clock_gettime(CLOCK_REALTIME, &tp0); 
    rtlsdr_set_center_freq(dev, freqs[freq_index]); 
    clock_gettime(CLOCK_REALTIME, &tp1); 

    time_tuning+= ((double) tp1.tv_sec - tp0.tv_sec) + (1e-9) * ( tp1.tv_nsec - tp0.tv_nsec); 

    clock_gettime(CLOCK_REALTIME, &tp0); 
    usleep(5000); 
    clock_gettime(CLOCK_REALTIME, &tp1); 
    time_waiting+= ((double)tp1.tv_sec - tp0.tv_sec) + (1e-9) * ( tp1.tv_nsec - tp0.tv_nsec); 

    clock_gettime(CLOCK_REALTIME, &tp0); 
    r= rtlsdr_read_sync(dev, &buffer, buffer_size, &nread) ; 
    clock_gettime(CLOCK_REALTIME, &tp1); 
    time_reading+= ((double)tp1.tv_sec - tp0.tv_sec) + (1e-9) * ( tp1.tv_nsec - tp0.tv_nsec); 

    if (nread != buffer_size)
    {
      fprintf(stderr,"BAD READ %d %d  %d!\n", nread, buffer_size, r); 
    }
      
    avgs++; 

    printf("---------------------------------------------------\n"); 
    printf(" Summary:    \n"); 
    printf("  Time Tuning: %f (Avg: %f ) \n", time_tuning, time_tuning / avgs); 
    printf("  Time Waiting: %f (Avg: %f) \n", time_waiting, time_waiting / avgs); 
    printf("  Time Reading (bufsize=%u): %f (Avg: %f) \n",buffer_size,  time_reading, time_reading / avgs); 

    freq_index = (freq_index +1) % nfreqs; 
  }



  rtlsdr_close(dev); 


  return; 

}



