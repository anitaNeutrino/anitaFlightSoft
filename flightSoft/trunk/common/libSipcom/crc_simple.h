/* crc_simple.h */

#ifndef _CRC_SIMPLE_H
#define _CRC_SIMPLE_H

unsigned short crc_short(unsigned short *p, unsigned long n);
unsigned char crc_char(unsigned char *p, unsigned long n);
unsigned long crc_long(unsigned long *p, unsigned long n);
char * crc_version(void);

#endif /* _CRC_SIMPLE_H */
