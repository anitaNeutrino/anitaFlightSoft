/* serial.h
 *
 * Marty Olevitch, May '05
 */

#ifndef _SERIAL_H
#define _SERIAL_H

int serial_end(int fd);
int serial_init(char *port, long baudrate);

#endif // _SERIAL_H

