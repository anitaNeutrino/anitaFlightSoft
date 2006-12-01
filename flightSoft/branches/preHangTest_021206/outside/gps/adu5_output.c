/*--------------------------------------------
  This is an ADU5 GPS data output code.
  It saves the output from the ADU5 into a 
  file every two minutes. The file is named 
  as "adu%YEAR%MONTH%DAY%HOUR%MINUTE.dat". 
  For example, "adu200407031040.dat" means 
  the file was created at 10:40am on July 
  3rd of the year 2004.

  Bei Cai, Michael DuVernois,  09/08/04
  --------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>

#include "serialLib/serialLib.h"

#define DATA_SIZE 300
#define MINUTES 2


int main() {
    char devName[]="/dev/ttyUSB0";

    int fd, n; 
    FILE *fpr;
    char filename0[15];
    time_t cTime;
    struct tm *sTime;
    struct termios options;
    char data[DATA_SIZE]={' '};
    int prev_min = 0, current_min = 0;
    char filename[30] = "adu";
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

/* create filename  */
    strftime(filename0, 30, "%Y%m%d%H%M", sTime);
    strcat(filename, filename0);
    strcat(filename, ".dat");

/* create a file  */
    if(( fpr=fopen(filename, "a"))==NULL) {
	printf("cannot open file\n");			
	exit(1);					
    }	

    printf("%s prev_min %d, current_min %d\n", filename, prev_min, current_min);

    while(1) {
	cTime = time(NULL);				
	sTime = localtime(&cTime);			
	current_min = sTime->tm_min;
	
	/* create a new file every "MINUTES" minutes  */
	
	if(abs(current_min-prev_min)>= MINUTES) {
	    prev_min = current_min; 	 
	    fclose(fpr); 	 

	    strftime(filename0, 30, "%Y%m%d%H%M", sTime);
	    strcpy(filename, "adu");
	    strcat(filename, filename0);
	    strcat(filename, ".dat");
	    printf("%s prev_min %d, current_min %d\n", filename, prev_min, current_min);
 
	    if(( fpr=fopen(filename, "a"))==NULL) {	
		printf("cannot open file\n");			
		exit(1);					
	    }	
	}

	/* read the output of the ADU5 into data  */
	n = read(fd, data, DATA_SIZE);
	printf("n is %d\n", n);       /* print on the screen how many bits it reads  */

	fwrite(data, n, 1, fpr);      /* write the output into the file  */
	data[n] = '\0';
	printf("data is %s", data);   /* print on the screen the output  */
    }

    close(fd); 

}
