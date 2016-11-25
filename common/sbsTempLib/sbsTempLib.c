
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <syslog.h>
#include <libgen.h> //For Mac OS X

#include "sbsTempLib.h"



const char *getSBSTemperatureLabel(SbsTempIndex_t index)
{
  switch(index) {
  case SBS_TEMP_0:
    return "Physical Id 0";
  case SBS_TEMP_1:
    return "Core 0";
  case SBS_TEMP_2:
    return "Core 1";
  case SBS_TEMP_3:
    return "Temp 1";
  case SBS_TEMP_4:
    return "Temp 2";
  case SBS_TEMP_5:
    return "GPU";
  default :
    return "Unknown";
  }
}



int readSBSTemperature(SbsTempIndex_t index)
{
  switch(index) {
  case SBS_TEMP_0:
    return readSBSTemperatureFile("/sys/devices/platform/coretemp.0/temp1_input");
  case SBS_TEMP_1:
    return readSBSTemperatureFile("/sys/devices/platform/coretemp.0/temp2_input");
  case SBS_TEMP_2:
    return readSBSTemperatureFile("/sys/devices/platform/coretemp.0/temp3_input");
  case SBS_TEMP_3:
  case SBS_TEMP_4:
  case SBS_TEMP_5:
    return readNTUTemps(index);
  default :
    return -1;
  }
}


int readNTUTemps(SbsTempIndex_t index) 
{
  int diskTemp[2]={0};
  int cpuTempInt;
  unsigned int unixTime;

  FILE *neoFile=fopen("/tmp/lastNtuTemps","r");
  if(neoFile) {
    fscanf(neoFile,"%u %d %d %d",&unixTime,&cpuTempInt,&diskTemp[0],&diskTemp[1]);
    fclose(neoFile);
  }
  switch(index) {
  case SBS_TEMP_3:
    return cpuTempInt;
  case SBS_TEMP_4:
    return diskTemp[0];
  case SBS_TEMP_5:
    return diskTemp[1];
  default:
    return -1;
  }
}


int readSBSTemperatureFile(const char *tempFile)
{
  static int errorCounter=0;
  int fd;
  int retVal=0;
  char temp[6];
  int temp_con;
  fd = open(tempFile, O_RDONLY);
  if(fd<0) {
    if(errorCounter<100) {
      fprintf(stderr,"Error opening %s -- %s (Error %d of 100)",tempFile,strerror(errno),errorCounter);
      syslog(LOG_ERR,"Error opening %s -- %s (Error %d of 100)",tempFile,strerror(errno),errorCounter);
    }
    retVal=-1;
    return retVal;
  }
  else {    
    retVal=read(fd, temp, 6);   
    temp[5] = 0x0;
    temp_con = convertRawToAnita(atoi(temp)); // temp_con is in 0.1 degree steps

    close(fd);
    return temp_con;
  }
  return 0;
}
