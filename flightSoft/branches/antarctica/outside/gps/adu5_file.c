/*---------------------------------------------
  This code sends a query to the ADU5 every 
  minute, and reads the output into a file.
  Please notice that the file is written only 
  when we've got enough amout of data, 
  for example, 4096 bytes.

  Bei Cai, Michael DuVernois,  09/08/04
---------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>

#include "serialLib/serialLib.h"


#define DATA_SIZE 300
#define COMMAND_SIZE 40
#define MINUTES 1
#define COMMAND "$PASHQ,STA\n" /* query satellite data  */

int main() {
     char devName[]="/dev/ttyUSB0";

int fd, n,retVal; 
FILE *fpr;
time_t cTime;
struct tm *sTime;
struct termios options;
char data[DATA_SIZE]={' '};
char buff[COMMAND_SIZE] = COMMAND;
int prev_min = 0, current_min = 0;

/* create a file  */
if(( fpr = fopen("satellite.txt", "a"))==NULL) {
  printf("cannot open file\n");			
  exit(1);					
}
 fprintf(fpr,"Fool\n");
/*  fclose(fpr); */
/*  exit(0); */


    /* Silly hack for reboot problem*/
    toggleCRTCTS(devName);
/* open a serial port */
    fd = open(devName, O_RDWR | O_NOCTTY );

if(fd<0)
   {
   printf(" unable to open port \n");
   exit(1);
   }
else
  printf(" Port Opened\n");

/* port settings  */
tcgetattr(fd, &options);             /* get current port settings  */
cfsetispeed(&options, BAUDRATE);     /* set input speed  */
cfsetospeed(&options, BAUDRATE);     /* set output speed  */

options.c_cflag &= ~PARENB;          /* clear the parity bit  */
options.c_cflag &= ~CSTOPB;          /* clear the stop bit  */
options.c_cflag &= ~CSIZE;           /* clear the character size  */
options.c_cflag |= CHARACTER_SIZE;   /* set charater size to 8 bits  */
options.c_cflag &= ~CRTSCTS;         /* clear the flow control bits  */
options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* raw input mode  */
options.c_cflag |= (CLOCAL | CREAD); 

tcsetattr(fd, TCSANOW, &options);    /* activate the settings  */

/* read local time  */
cTime = time(NULL);				
sTime = localtime(&cTime);			
prev_min = sTime->tm_min;
current_min = sTime->tm_min;


printf("prev_min %d, current_min %d\n", prev_min, current_min);

/*query satellite info and write into the file  */
write(fd, buff, sizeof(buff));/* send the command to ADU5  */
 sleep(1);
n = read(fd, data, DATA_SIZE);/* read the output of the ADU5 into data  */
/* data[n] = '\0'; */
 retVal=fprintf(fpr,"%s\n",data);
 if(retVal<0) perror("fprintf");
/*  retVal=fwrite(data, n, 1, fpr); /\* write the output into the file  *\/ */
/*  if(retVal<0) perror("fwrite"); */
printf("data is %s\n", data);   /* print on the screen the output  */

while(1) {
 cTime = time(NULL);				
 sTime = localtime(&cTime);			
 current_min = sTime->tm_min;

 /* query the satellite info every "MINUTES" minutes  */
 if(abs(current_min - prev_min) >= MINUTES) {
   prev_min = current_min;

   write(fd, buff, sizeof(buff));
   sleep(1);
   n = read(fd, data, DATA_SIZE);
   data[n] = '\0';
   retVal=fprintf(fpr,"%s",data);
   if(retVal<0) perror("fprintf");
   printf("data is %s", data);
 }
 sleep(1);
}

 close(fd); 
 fclose(fpr);
}
