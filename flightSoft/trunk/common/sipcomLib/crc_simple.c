/* crc.c - calculate simple crc
 *
 * Marty Olevitch, Jan '01
 */

#include "crc_simple.h"

static char Crc_simple_version[] = "crc_simple 1.0";

char *
crc_version(void)
{
    return Crc_simple_version;
}

unsigned short
crc_short(unsigned short *p, unsigned long n)
{
    unsigned short sum = 0;
    unsigned long i;
    for (i=0L; i<n; i++) {
	sum += *p++;
    }
    return ((0xffff - sum) + 1);
}

unsigned char
crc_char(unsigned char *p, unsigned long n)
{
    unsigned char sum = 0;
    unsigned long i;
    for (i=0L; i<n; i++) {
	sum += *p++;
    }
    return ((0xff - sum) + 1);
}

unsigned long
crc_long(unsigned long *p, unsigned long n)
{
    unsigned long sum = 0;
    unsigned long i;
    for (i=0L; i<n; i++) {
	sum += *p++;
    }
    return ((0xffffffff - sum) + 1);
}

#ifdef MAIN_TEST
/* compile test program with cc -DMAIN_TEST -o crc crc_simple.c */
#include <stdio.h>
int
main(int argc, char *argv[])
{
    unsigned long a[10000];
    unsigned long i;
    unsigned short val;
    unsigned char valc;
    unsigned long vall;

    printf("version %s\n", crc_version());
    for (i=0; i<10000L; i++) {
	a[i] = i*2;
    }
    vall = crc_long(a, 10000L);
    valc = crc_char((unsigned char *)a, 10000L * sizeof(long) / sizeof(char));
    val = crc_short((unsigned short *)a, 10000L * sizeof(long) / sizeof(short));
    printf("%u %04x  %u %02x  %lu %08x\n", val, val, valc, valc, vall, vall);
}
#endif /* MAIN_TEST */
