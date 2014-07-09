/* serial.c
 *
 * Marty Olevitch, May '05
 */

#include "serial.h"
#include "sipcom_impl.h"

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

static int is_CTS_high(int fd);
static int clear_rts(int fd);
static int set_rts(int fd);

int
serial_init(char *port, long baudrate, unsigned char _sipmux_enable)
{
    struct termios newopts;
    int fd;
    speed_t baud;

fprintf(stderr,"SERIAL_INIT a: port %s: _sipmux_enable is %x\n",
                                                      port, _sipmux_enable);
    if (baudrate == 19200) {
	baud = B19200;
    } else if (baudrate == 1200) {
	baud = B1200;
    } else if (baudrate == 2400) {
	baud = B2400;
    } else if (baudrate == 9600) {
	baud = B9600;
    } else if (baudrate == 38400) {
	baud = B38400;
    } else if (baudrate == 57600) {
	baud = B57600;
    } else if (baudrate == 115200) {
	baud = B115200;
    } else {
	set_error_string(
	    "serial_init: baud rate must be 115200, "
            "57600, 38400, 19200, 9600, 2400, or 1200, not %ld\n",
	    baudrate);
	return -2;
    }

    fd = open(port, O_RDWR);
    if (fd == -1) {
	set_error_string(
	    "serial_init: can't open '%s' (%s)\n", port, strerror(errno));
	return -1;
    }

    if (tcgetattr(fd, &newopts)) {
	set_error_string(
	    "serial_init: bad tcgetattr newopts '%s' (%s)\n",
	    port, strerror(errno));
	return -1;
    }

    cfsetispeed(&newopts, baud);
    cfsetospeed(&newopts, baud);
    
    if (_sipmux_enable) {
        newopts.c_cflag |= (CRTSCTS | CLOCAL);
    } else {
        newopts.c_cflag |= (CLOCAL | CREAD);
    }

    /* set 8N1 - 8 bits, no parity, 1 stop bit */
    newopts.c_cflag &= ~PARENB;
    newopts.c_cflag &= ~CSTOPB;
    newopts.c_cflag &= ~CSIZE;
    newopts.c_cflag |= CS8;

    newopts.c_iflag &= ~(INLCR | ICRNL);
    newopts.c_iflag &= ~(IXON | IXOFF | IXANY);	// no XON/XOFF 

    newopts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	// raw input 
    newopts.c_oflag &= ~OPOST;	// raw output 
    
    if (tcflush(fd, TCIOFLUSH) == -1) {
	set_error_string("serial_init: bad tcflush on '%s' (%s)\n",
	    port, strerror(errno));
        return -1;
    }
    if (tcsetattr(fd, TCSANOW, &newopts) == -1) {
	set_error_string("serial_init: bad tcsetattr on '%s' (%s)\n",
	    port, strerror(errno));
        return -1;
    }

    if (_sipmux_enable) {
	clear_rts(fd);
    }

    return fd;
}

int serial_sipmux_begin(int fd, unsigned int timeout_us)
{
    long to_count = timeout_us;

    int ret = set_rts(fd);    // Raise CTS for receiver.
    if (ret) {
	set_error_string(
	    "serial_sipmux_begin: bad RTS raise (%s)\n", strerror(errno));
        return -1;
    }

    // Wait for our CTS to go high. We will time out after about timeout_us
    // microseconds.
    while (!is_CTS_high(fd)) {
        usleep(1);
	if (to_count-- < 0) {
	    set_error_string(
		"serial_sipmux_begin: CTS timeout (us=%u)\n", timeout_us);
	    clear_rts(fd);       	// Lowers CTS for receiver.
	    return -2;
	}
    }

    return 0;
}

int serial_sipmux_end(int fd, unsigned int timeout_us)
{
    int ret;

    ret = tcdrain(fd);
    if (ret) {
	set_error_string(
	    "serial_sipmux_end: bad tcdrain (%s)\n", strerror(errno));
	clear_rts(fd);    // Lower CTS for receiver.
        return -3;
    }

    ret = clear_rts(fd);    // Lower CTS for receiver.
    if (ret) {
	set_error_string(
	    "serial_sipmux_end: bad RTS lower (%s)\n", strerror(errno));
        return -1;
    }

    /*
     * The electronics (as of 6-29-11) doesn't lower our CTS, so we don't
     * wait for it.
     *
    // Wait for our CTS to go low. We will time out after about timeout_us
    // microseconds.
    {
	unsigned int n = 0;
	while (is_CTS_high(fd)) {
	    usleep(1);
	    if (n++ > timeout_us) {
		return -2;
	    }
	}
    }
    */

    return 0;
}

int
serial_end(int fd)
{
    if (close(fd)) {
	set_error_string( "serial_end: ERROR: bad close on port (%s)\n",
	    strerror(errno));
	return -1;
    }
    return 0;
}

static int
is_CTS_high(int fd)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) == -1) {
        return -5;
    }

    return (status & TIOCM_CTS);
}

static int
set_rts(int fd)
{
    int status = 0;

    if (ioctl(fd, TIOCMGET, &status) == -1) {
        return -3;
    }

    status |= TIOCM_RTS;

    if (ioctl(fd, TIOCMSET, &status) == -1) {
        return -4;
    }

    return 0;
}

static int
clear_rts(int fd)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) == -1) {
        return -3;
    }

    status &= ~TIOCM_RTS;

    if (ioctl(fd, TIOCMSET, &status) == -1) {
        return -4;
    }
    return 0;
}
