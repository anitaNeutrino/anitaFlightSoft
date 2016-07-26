#include "RTL_common.h" 
#include "syslog.h" 

#include <sys/mman.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h> 
#include <string.h>
#include <stdio.h>
#include <time.h>


RtlSdrPowerSpectraStruct_t * open_shared_RTL_region(const char * serial, int parent)
{
  int shared_fd, retVal; 
  char buf[1024];
  sprintf(buf, RTLD_SHARED_SPECTRUM_NAME, serial); 

  shared_fd = shm_open(buf,parent ? O_CREAT | O_RDWR : O_RDWR, 0666); 
  if (shared_fd < 0) 
  {
    syslog(LOG_ERR, "Could not open shared memory region\n"); 
    perror("open_shared_RTL_region"); 
    return 0; 
  }

  if (parent) 
  {
    retVal = ftruncate(shared_fd, sizeof(RtlSdrPowerSpectraStruct_t)); 
  }


  if (retVal) 
  {
    syslog(LOG_ERR, "Could not resize shared memory region\n"); 
    return 0; 
  }
 
  
  return mmap(NULL, sizeof(RtlSdrPowerSpectraStruct_t), 
              PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0); 
 

}

void unlink_shared_RTL_region(const char * serial) 
{
  char buf[FILENAME_MAX];
  sprintf(buf, RTLD_SHARED_SPECTRUM_NAME, serial); 
  shm_unlink(buf); 
}


void dumpSpectrum(const RtlSdrPowerSpectraStruct_t * dump) 
{
  struct tm *temp = localtime((time_t*) &dump->unixTimeStart); 
  char buf[128]; 
  double timetook = dump->scanTime / 10.; 
  double fgain = dump->gain / 10. ; 
  int i =0; 
  double f = dump->startFreq; 
  double db; 

  strftime(buf,128, "%D %T", temp); 
  printf("  Scan from RTL%d started at %s which took %f\n " , dump->rtlNum, buf, timetook); 
  printf("  gain was: %f\n",fgain); 

  printf(" -------------- <spectrum N=%d> -------------------------------\n", dump->nFreq); 
  for (i = 0; i < dump->nFreq; i++) 
  {
    db = ((double) dump->spectrum[i]) / RTL_SPECTRUM_DBM_SCALE + RTL_SPECTRUM_DBM_OFFSET; 
    printf ("   %f Hz:  %f dBm \n", f, db); 
    f+= dump->freqStep; 
  }

  printf(" ---------------</spectrum>--------------------------------\n"); 

  



}
