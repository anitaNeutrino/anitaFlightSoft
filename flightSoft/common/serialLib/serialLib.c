/*! \file serialLib.c
    \brief Some useful functions when you're using the serial/whiteheat communications
    
    For now it's just a couple of functions, but it may well grow.
   
   April 2005 rjn@mps.ohio-state.edu
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "serialLib/serialLib.h"

#define GPS_BAUDRATE B9600
#define MAGNETOMETER_BAUDRATE B38400

int openGPSDevice(char devName[])
/*! Does exactly what it says on the tin
 */
{
    int retVal,fd;
    retVal=toggleCRTCTS(devName);
    if(retVal<0) return retVal;
    fd = open(devName, O_RDWR | O_NOCTTY);
    if(fd<0) { 	
	syslog(LOG_ERR,"open %s: %s",devName,strerror(errno)); 
	return -1;
    }
    retVal=setGPSTerminalOptions(fd);
    if(retVal<0) {
	close(fd);
	return retVal;
    }
    return fd;
}

int openMagnetometerDevice(char devName[])
/*! Does exactly what it says on the tin
 */
{
    int retVal,fd;
    retVal=toggleCRTCTS(devName);
    if(retVal<0) return retVal;
    fd = open(devName, O_RDWR | O_NOCTTY);
    if(fd<0) { 	
	syslog(LOG_ERR,"open %s: %s",devName,strerror(errno)); 
	return -1;
    }
    retVal=setMagnetometerTerminalOptions(fd);
    if(retVal<0) {
	close(fd);
	return retVal;
    }
    return fd;
}

int setGPSTerminalOptions(int fd)
/*! Sets the various termios options needed. 
*/
{
    int retVal=0;
    struct termios options;
/* port settings */
    retVal=tcgetattr(fd, &options); /* get current port settings */
    if(retVal<0) {
	syslog(LOG_ERR,"tcgetattr on fd %d: %s",fd,strerror(errno));
	return -1;
    }
        
    options.c_iflag=1;
    options.c_oflag=0;
    options.c_cflag=3261;
    options.c_lflag=0;
    strncpy(options.c_cc,"",2);
    options.c_ispeed=13;
    options.c_ospeed=13;

    cfsetispeed(&options, GPS_BAUDRATE);    /* set input speed */
    cfsetospeed(&options, GPS_BAUDRATE);    /* set output speed */

    options.c_cflag &= ~PARENB;         /* clear the parity bit  */
    options.c_cflag &= ~CSTOPB;         /* clear the stop bit  */
    options.c_cflag &= ~CSIZE;          /* clear the character size  */
    options.c_cflag |= CHARACTER_SIZE;  /* set charater size to 8 bits  */
    options.c_cflag &= ~CRTSCTS;        /* clear the flow control bits  */
    options.c_lflag &= (ICANON | ECHO | ECHOE | ISIG); /* raw input mode  */
    options.c_cflag |= (CLOCAL | CREAD); 

    options.c_oflag &= ~(OPOST | ONLCR );
/*     options.c_oflag |= ( ONLCR | OPOST); */
 
    retVal=tcsetattr(fd, TCSANOW, &options);   /* activate the settings  */
    if(retVal<0) {
	syslog(LOG_ERR,"tcsetattr on fd %d: %s",fd,strerror(errno));
	return -1;
    }
    return 0;
}



int setMagnetometerTerminalOptions(int fd)
/*! Sets the various termios options needed. 
*/
{
    int retVal=0;
    struct termios options;
/* port settings */
    retVal=tcgetattr(fd, &options);            /* get current port settings */
    if(retVal<0) {
	syslog(LOG_ERR,"tcgetattr on fd %d: %s",fd,strerror(errno));
	return -1;
    }

    cfsetispeed(&options, GPS_BAUDRATE);    /* set input speed */
    cfsetospeed(&options, GPS_BAUDRATE);    /* set output speed */

    options.c_cflag &= ~PARENB;         /* clear the parity bit  */
    options.c_cflag &= ~CSTOPB;         /* clear the stop bit  */
    options.c_cflag &= ~CSIZE;          /* clear the character size  */
    options.c_cflag |= CHARACTER_SIZE;  /* set charater size to 8 bits  */
    options.c_cflag &= ~CRTSCTS;        /* clear the flow control bits  */
    options.c_lflag &= (ICANON | ECHO | ECHOE | ISIG); /* raw input mode  */
    options.c_cflag |= (CLOCAL | CREAD); 
    options.c_oflag |= ( ONLCR | OPOST);

 
    retVal=tcsetattr(fd, TCSANOW, &options);   /* activate the settings  */
    if(retVal<0) {
	syslog(LOG_ERR,"tcsetattr on fd %d: %s",fd,strerror(errno));
	return -1;
    }
    return 0;
}


int toggleCRTCTS(char devName[])
/*! This is needed on the whiteheat ports after a reboot, after a power cycle it comes up in the correct state. 
*/
{    
    int fd,retVal; 
    struct termios options;

/* open a serial port */
    fd = open(devName, O_RDWR | O_NOCTTY |O_NONBLOCK ); 
    if(fd<0)
    {
	syslog(LOG_ERR,"open %s: %s",devName,strerror(errno)); 
	return -1;
    }

/* port settings */
    retVal=tcgetattr(fd, &options);            /* get current port settings */
    if(retVal<0) {
	syslog(LOG_ERR,"tcgetattr on fd %d: %s",fd,strerror(errno));
	return -1;
    }

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
    if(retVal<0) {
	syslog(LOG_ERR,"tcsetattr on fd %d: %s",fd,strerror(errno));
	return -1;
    }
    close(fd);	

    return 0;
}


