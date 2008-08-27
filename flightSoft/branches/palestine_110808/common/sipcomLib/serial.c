/* serial.c
 *
 * Marty Olevitch, May '05
 */

#include "serial.h"
#include "sipcom_impl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>

struct termios Origopts;

int
serial_init(char *port, long baudrate)
{
    struct termios newopts;
    int fd;
    speed_t baud;

    if (baudrate == 19200) {
	baud = B19200;
    } else if (baudrate == 1200) {
	baud = B1200;
    } else {
	set_error_string(
	    "serial_init: baud rate must be 19200 or 1200, not %ld\n",
	    baudrate);
	return -2;
    }

    fd = open(port, O_RDWR | O_NDELAY);
    if (fd == -1) {
	set_error_string(
	    "serial_init: can't open '%s' (%s)\n",
	    port, strerror(errno));
	return -1;
    } else {
	fcntl(fd, F_SETFL, 0);	/* blocking reads */
	//fcntl(fd, F_SETFL, FNDELAY);	/* non-blocking reads */
    }

    tcgetattr(fd, &Origopts);
    tcgetattr(fd, &newopts);

    cfsetispeed(&newopts, baud);
    cfsetospeed(&newopts, baud);
    
    newopts.c_cflag |= (CLOCAL | CREAD);   /* enable receiver, local mode */

    /* set 8N1 - 8 bits, no parity, 1 stop bit */
    newopts.c_cflag &= ~PARENB;
    newopts.c_cflag &= ~CSTOPB;
    newopts.c_cflag &= ~CSIZE;
    newopts.c_cflag |= CS8;

    newopts.c_iflag &= ~(INLCR | ICRNL);
    newopts.c_iflag &= ~(IXON | IXOFF | IXANY);	/* no XON/XOFF */

    newopts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	/* raw input */
    newopts.c_oflag &= ~OPOST;	/* raw output */

    tcsetattr(fd, TCSANOW, &newopts);
    return fd;
}

int
serial_end(int fd)
{
    tcsetattr(fd, TCSANOW, &Origopts);
    if (close(fd)) {
	set_error_string( "serial_end: ERROR: bad close on port (%s)\n",
	    strerror(errno));
	return -1;
    }
    return 0;
}
