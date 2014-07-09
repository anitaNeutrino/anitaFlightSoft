/* serial.h
 *
 * Marty Olevitch, May '05
 */

#ifndef _SERIAL_H
#define _SERIAL_H

int serial_end(int fd);
int serial_init(char *port, long baudrate, unsigned char _sipmux_enable);

/* serial_sipmux_begin - signal the receiver we are ready to write data;
 * wait for it to respond that it is ready to receive.
 *
 * Parameters:
 *      fd          file descriptor to manipulate.
 *
 *      timeout_us  Length of time (in microseconds) to wait for receiver
 *                  response.
 *
 * Returns:
 *      0   All went well and it is now okay to write data.
 *      -1  Couldn't set RTS high.
 *      -2  Timed out waiting for CTS high.
 */
int serial_sipmux_begin(int fd, unsigned int timeout_us);

/* serial_sipmux_end - signal the receiver we have finished writing data;
 * wait for it to acknowlege.
 *
 * Parameters:
 *      fd          file descriptor to manipulate.
 *
 *      timeout_us  Length of time (in microseconds) to wait for receiver
 *                  response.
 *
 * Returns:
 *      0   All went well.
 *      -1  Couldn't set RTS low.
 *      -2  Timed out waiting for CTS low.
 */
int serial_sipmux_end(int fd, unsigned int timeout_us);

#endif // _SERIAL_H

