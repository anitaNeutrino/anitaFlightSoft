/* pack.c - pack and unpack 4 12-bit quantities into 3 16-bit words
 *
 * Marty Olevitch
 */

#include <string.h>
#include "pack12bit.h"

#ifndef MEMCPY
/* For unix, use the following: */
#define MEMCPY	memcpy
#endif

void
unpack12bit(unsigned short *w, void *srcp)
{
    char *dumPtr = (char *) srcp;
    unsigned short val;
    w[3] = 0;

    MEMCPY((char *)&val, dumPtr, sizeof(short));
    w[0] = val & 0x0fff;
    w[3] += ((val & 0xf000) >> 12);

    MEMCPY((char *)&val, dumPtr + sizeof(short), sizeof(short));
    w[1] = val & 0x0fff;
    w[3] += ((val & 0xf000) >> 8);

    MEMCPY((char *)&val, dumPtr + (2 * sizeof(short)), sizeof(short));
    w[2] = val & 0x0fff;
    w[3] += ((val & 0xf000) >> 4);
}

void
pack12bit(unsigned short *w, unsigned char *destp)
{
    unsigned short val;

    val = (w[0] & 0x0fff) + ((w[3] << 12) & 0xf000);
    MEMCPY(destp, (char *)&val, sizeof(short));

    val = (w[1] & 0x0fff) + ((w[3] <<  8) & 0xf000);
    MEMCPY(destp+2, (char *)&val, sizeof(short));

    val = (w[2] & 0x0fff) + ((w[3] <<  4) & 0xf000);
    MEMCPY(destp+4, (char *)&val, sizeof(short));
}
