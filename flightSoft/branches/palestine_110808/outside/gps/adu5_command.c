
/*--------------------------------------------
  This is an ADU5 GPS readout code.
  [data bits=8, stop bits=1, parity=none]

  Bei Cai, Michael DuVernois,  09/08/04
  --------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

#include "serialLib/serialLib.h"

#define COMMAND_SIZE 256
#define DATA_SIZE 256
#define COMMAND "$PASHS,ELM,0\r\n"  /* set elevation mask angle to 0 degree */

int main() { 
    char devName[]="/dev/ttySTDRV002_0";
    int fd, i; 
    struct termios options;
    char buff[COMMAND_SIZE] = COMMAND;
    char data[DATA_SIZE]={' '};
    int bytesRead=0;

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

/* port settings */
    tcgetattr(fd, &options);            /* get current port settings */


    options.c_iflag=1;
    options.c_oflag=0;
    options.c_cflag=3261;
    options.c_lflag=0;
    strncpy(options.c_cc,"",2);
    options.c_ispeed=13;
    options.c_ospeed=13;

    cfsetispeed(&options, BAUDRATE);    /* set input speed */
    cfsetospeed(&options, BAUDRATE);    /* set output speed */

    options.c_cflag &= ~PARENB;         /* clear the parity bit  */
    options.c_cflag &= ~CSTOPB;         /* clear the stop bit  */
    options.c_cflag &= ~CSIZE;          /* clear the character size  */
    options.c_cflag |= CHARACTER_SIZE;  /* set charater size to 8 bits  */
    options.c_cflag &= ~CRTSCTS;        /* clear the flow control bits  */
    options.c_lflag &= (ICANON | ECHO | ECHOE | ISIG); /* raw input mode  */
    options.c_cflag |= (CLOCAL | CREAD); 

    options.c_oflag &= ~(OPOST | ONLCR );
/*     options.c_oflag |= ( ONLCR | OPOST); */
 
    tcsetattr(fd, TCSANOW, &options);   /* activate the settings  */
    printf("%d %d %d %d %s %d %d\n",options.c_iflag,options.c_oflag,options.c_cflag,options.c_lflag,options.c_cc,options.c_ispeed,options.c_ospeed);


/* set relative antenna position vectors  */
    strcat(buff, "$PASHS,3DF,V12,0.000,+1.000,0.000\r\n");
    strcat(buff, "$PASHS,3DF,V13,-0.500,+0.500,0.000\r\n");
    strcat(buff, "$PASHS,3DF,V14,+0.500,+0.500,0.000\r\n");

 /*    strcat(buff, "$PASHS,3DF,ANG,10\r\n"); /\* set the maximum search angle  *\/ */
/*     strcat(buff, "$PASHS,3DF,FLT,Y\r\n");  /\* set smoothing filter on  *\/ */
/*     strcat(buff, "$PASHS,KFP,ON\r\n");     /\* enable the Kalman filter  *\/ */
    strcat(buff, "$PASHS,NME,ALL,B,OFF\r\n");
    strcat(buff, "$PASHQ,PRT\r\n");
    strcat(buff, "$PASHQ,SA4\r\n");    
    strcat(buff, "$PASHS,INI\r\n");    
//    strcat(buff, "$PASHS,NME,TTT,B,ON\r\n");
//    strcat(buff, "$PASHS,NME,PAT,B,ON\r\n");
//    strcat(buff, "$PASHS,NME,PER,5\r\n");
/* send the commands to ADU5  */
    write(fd, buff, strlen(buff));

    for(i=0; i < COMMAND_SIZE; i++) printf("%c", buff[i]);
	
    printf("\n");
    sleep(2);

/* read & print output */
    while(1) {
	bytesRead=read(fd, data, DATA_SIZE);
	printf("read  returned: %d\n",bytesRead);
	if(bytesRead>0) {
	    for(i=0; i <bytesRead; i++) printf("%c", data[i]);
//	    printf("\n");
	}
	usleep(5000000);
    }

    close(fd);

}

