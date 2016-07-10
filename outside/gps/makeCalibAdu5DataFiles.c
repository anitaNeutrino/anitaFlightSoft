/*! \file makeCalibAdu5DataFiles
    \brief The program which make data files for the ADU5 Calibration
    
    Talks to one of the ADU5 GPS Calibration units and tries to generate the binary files needed for GPS calibration. Almost certainly doesn't work yet, but maybe it will in the long term

    July 2016 r.nichol@ucl.ac.uk
*/


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

#include "serialLib/serialLib.h"

#define COMMAND_SIZE 1024
#define DATA_SIZE 1024
#define COMMAND "$PASHS,ELM,0\r\n"  /* set elevation mask angle to 0 degree */

int main(int argc, char **argv) {
  if(argc<3) {
    printf("Usage:\n%s <A or B> <num seconds>",argv[0]);
    return -1;
  }

  char *whichAdu5Dev=ADU5A_DEV_NAME;
  if(argv[1][0]=='B') whichAdu5Dev=ADU5B_DEV_NAME;
  int numSeconds=atoi(argv[2]);  
  int retVal=0;
  int fdAdu5;

  printf("Trying to calibrate %s by taking data for %d seconds\n",whichAdu5Dev,numSeconds);
  
  retVal=openGpsDevice(whichAdu5Dev);	
  if(retVal<=0) {
    syslog(LOG_ERR,"Couldn't open: %s\n",whichAdu5Dev);
    if(printToScreen) printf("Couldn't open: %s\n",whichAdu5Dev);
    return -1;
  }
  else fdAdu5=retVal;


 
  
  char buff[COMMAND_SIZE];
  char data[DATA_SIZE]={' '};
  int bytesRead=0;

  time_t startTime = time(NULL);

  sprintf(buff, "$PASHQ,PRT\r\n");
  strcat(buff,"$PASHS,NME,ALL,OFF\r\n"); //Turn off NMEA messages
  strcat(buff,"$PASHS,OUT,A\r\n"); //Turn off raw data messages to port A
  strcat(buff,"$PASHS,OUT,B\r\n"); //Turn off raw data messages to port B
  strcat(buff,"$PASHS,ELM,10\r\n"); //Set elevation mask to 10 degrees
  strcat(buff,"$PASHS,RCI,001.0\r\n"); //Set raw data rate to 1 second
  strcat(buff,"$PASHS,PDS,OFF\r\n"); // Special setting needed for calibration must be switched back on at end
  strcat(buff,"$PASHS,OUT,B,MBN,SNV,PBN,ATT,BIN\r\n"); //Request the output

  /* send the commands to ADU5  */
  write(fdAdu5, buff, strlen(buff));
  
  for(i=0; i < COMMAND_SIZE; i++) printf("%c", buff[i]);
  
  printf("\n");
  sleep(5);
  
  /* read & print output */
  time_t nowTime = time(NULL);
  while(nowTime < startTime + numSeconds) {
    bytesRead=read(fdAdu5, data, DATA_SIZE);
    printf("read  returned: %d\n",bytesRead);
    if(bytesRead>0) {
      for(i=0; i <bytesRead; i++) printf("%c", data[i]);
      //	    printf("\n");
    }
    usleep(500);
  }

  
  sprintf(buff, "$PASHQ,PRT\r\n");
  strcat(buff,"$PASHS,NME,ALL,OFF\r\n"); //Turn off NMEA messages
  strcat(buff,"$PASHS,OUT,A\r\n"); //Turn off raw data messages to port A
  strcat(buff,"$PASHS,OUT,B\r\n"); //Turn off raw data messages to port B
  strcat(buff,"$PASHS,ELM,10\r\n"); //Set elevation mask to 10 degrees
  strcat(buff,"$PASHS,PDS,ON\r\n"); // Special setting needed for calibration must be switched back on at end  
  write(fdAdu5, buff, strlen(buff));
  sleep(5);
  {
    bytesRead=read(fdAdu5, data, DATA_SIZE);
    printf("read  returned: %d\n",bytesRead);
    if(bytesRead>0) {
      for(i=0; i <bytesRead; i++) printf("%c", data[i]);
      //	    printf("\n");
    }
  }
  
  
  close(fdAdu5);

}
