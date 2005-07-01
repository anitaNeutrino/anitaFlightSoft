/*! \file serialLib.c
    \brief Some useful functions when you're using the serial/whiteheat communications
    
    For now it's just a couple of functions, but it may well grow.
   
   April 2005 rjn@mps.ohio-state.edu
*/

/*
 * Current CVS Tag:
 * $Header: /work1/anitaCVS/flightSoft/common/serialLib/serialLib.c,v 1.3 2005/06/15 16:22:00 rjn Exp $
 */
 
/* 
 * Modification History : DO NOT EDIT - MAINTAINED BY CVS 
 * $Log: serialLib.c,v $
 * Revision 1.3  2005/06/15 16:22:00  rjn
 * Fixed a lot of silly warnings
 *
 * Revision 1.2  2005/04/04 17:13:23  rjn
 * Just a routine commit, I'm not entirely sure what has changed. I think I rmeoved an unnecessary read from the CRTCST toggle function.
 *
 * Revision 1.1  2005/04/01 16:48:25  rjn
 * Added serialLib which will house all the functions necessary for talking through the whiteheat, and other serial connections.
 *
 * Revision 1.4  2004/10/08 20:08:41  rjn
 * Updated a lot of things. fakeAcqd.c is coming along, and now communicates with Eventd through the file system (the filesystem in question will be a ramdisk on the flight computer)
 *
 * Revision 1.3  2004/09/08 18:02:53  rjn
 * More playing with serialLib. And a few changes as --pedantic is now one of the compiler options.
 *
 * Revision 1.2  2004/09/07 13:52:21  rjn
 * Added dignified multiple port handling capabilities to serialLib. Not fully functional... but getting there.
 *
 * Revision 1.1  2004/09/01 14:49:17  rjn
 * Renamed mySerial to serialLib
 *
 *
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


#define DATA_SIZE 256


void toggleCRTCTS(char devName[])
/*! This is needed on the whiteheat ports after a reboot, after a power cycle it comes up in the correct state. 
*/
{    
    int fd; 
    struct termios options;
    //    char data[DATA_SIZE]={' '};

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


