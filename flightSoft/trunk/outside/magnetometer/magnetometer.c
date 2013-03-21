

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>

#define BAUDRATE B38400
#define CHARACTER_SIZE CS8

#define COMMAND_SIZE 250
#define DATA_SIZE 256

void toggleCRTCTS(char devName[]);

int main(int argc, char ** argv)
{
  char devName[]="/dev/ttyS0"; 
  
  int fd, i, retVal ; 
  struct termios options;
  char buff[COMMAND_SIZE]; 
  char data[DATA_SIZE]={' '};
  
    
/* Silly hack for reboot problem*/
  toggleCRTCTS(devName);
  /* open a serial port */
  fd = open(devName, O_RDWR | O_NOCTTY);
  if(fd<0)
    {
      perror("open");
	printf(" unable to open port \n");
	exit(1);
    }
  else
    printf(" Port Opened\n");
  
  
  retVal=tcgetattr(fd, &options);             /* get current port settings  */
  if(retVal<0) {
    perror("tcgetattr");
    printf("Something fucked\n");     
  }
  
  cfsetispeed(&options, BAUDRATE);    /* set input speed  */
  cfsetospeed(&options, BAUDRATE);    /* set output speed */

  options.c_cflag &= ~PARENB;         /* clear the parity bit  */
  options.c_cflag &= ~CSTOPB;         /* clear the stop bit  */
  options.c_cflag &= ~CSIZE;          /* clear the character size  */
  options.c_cflag |= CHARACTER_SIZE;  /* set charater size to 8 bits  */
  options.c_cflag &= ~CRTSCTS;        /* clear the flow control bits  */
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* raw input mode  */
  options.c_cflag |= (CLOCAL | CREAD);
  options.c_oflag |= ( ONLCR | OPOST);
  /*  printf("%d %d %d %d %s %d %d\n",options.c_iflag,options.c_oflag,options.c_cflag,options.c_lflag,options.c_cc,options.c_ispeed,options.c_ospeed); */

  
  tcsetattr(fd, TCSANOW, &options);   /* activate the settings */
  /*  printf("%d %d %d %d %s %d %d\n",options.c_iflag,options.c_oflag,options.c_cflag,options.c_lflag,options.c_cc,options.c_ispeed,options.c_ospeed); */
  
  
  sprintf(buff,"SomeCommand\n");
  
  printf("buff -- %s\n",buff);
/* printf("wrote buff -- %s\n",buff); */

/* read & print output */
  while(1) {
    //Write the command
    retVal=write(fd, buff, strlen(buff));
    printf("wrote buff -- %s\t%d\n",buff,retVal);
    //Optional sleep
    usleep(500);	

    //Read some outout back (if there is any)
    retVal=read(fd, data, DATA_SIZE);
    /*     printf("DATA SIZE %d\n",DATA_SIZE); */

    //Print the output along with the time
    printf("%ld\t",time(NULL));
    for(i=0; i < retVal; i++) {
      /* 	     if(data[i]=='>') break; */
      printf("%c", data[i]);
      data[i]=0;
    }
    printf("\n");
    sleep(10);
  }


close(fd);

}



void toggleCRTCTS(char devName[])
/*! This is needed on the whiteheat ports after a reboot, after a power cycle it comes up in the correct state. 
*/
{    
    int fd; 
    struct termios options;

/* open a serial port */
    fd = open(devName, O_RDWR | O_NOCTTY |O_NONBLOCK );
 
    if(fd<0)
    {
	perror("open");
	printf(" unable to open port \n");
	exit(1);
    }

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
    options.c_cflag |= CRTSCTS;        /* Toggle on */
    options.c_lflag &= (ICANON | ECHO | ECHOE | ISIG); /* raw input mode  */
    options.c_cflag |= (CLOCAL | CREAD); 

    options.c_oflag &= ~(OPOST | ONLCR );
/*     options.c_oflag |= ( ONLCR | OPOST); */
 
    tcsetattr(fd, TCSANOW, &options);   /* activate the settings  */
    close(fd);	

}

