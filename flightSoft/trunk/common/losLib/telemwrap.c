/* telemwrap.c
 *
 * Marty Olevitch, May '08
 */

#include "telemwrap.h"
#include "crc_simple.h"
#include <string.h>

static unsigned short Startbuf[] = {
      0xF00D
    , 0xD0CC
    , 0xAE00	// For default LOS data
};
static int Startbuf_sizeb =
    sizeof(short) * (sizeof(Startbuf)/sizeof(Startbuf[0]));

static unsigned short Endbuf[] = {
      0xAEFF
    , 0xC0FE
    , 0xF1EA
};
static int Endbuf_sizeb =
    sizeof(short) * (sizeof(Endbuf)/sizeof(Endbuf[0]));

static unsigned long Buffer_count = 0L;
static int Datasource = TW_LOS;

int
telemwrap_init(int datasource)
{
    if (TW_LOS != datasource && TW_SIP != datasource) {
	return -1;
    }
    if (TW_SIP == datasource) {
	// For SIP data, ID_HDR is 0xAE01 instead of 0xAE00
	Startbuf[2]++;
    }
    Datasource = datasource;
    return 0;
}

int
telemwrap(unsigned short *databuf, unsigned short *wrapbuf,
    unsigned short nbytes)
{
    unsigned char *cp;
    unsigned char *nbytesp;
    int odd_number_of_science_bytes =  (nbytes % 2 != 0);
    unsigned char *scidatap;

    cp = (unsigned char *)wrapbuf;

    // Initial header words
    memcpy(cp, Startbuf, Startbuf_sizeb);
    cp += Startbuf_sizeb;

    // Buffer count
    memcpy(cp, &Buffer_count, sizeof(Buffer_count));
    cp += sizeof(Buffer_count);
    Buffer_count++;

    nbytesp = cp;
    cp += sizeof(nbytes);

    // Science data
    scidatap = cp;
    memcpy(cp, databuf, nbytes);
    cp += nbytes;

    if (odd_number_of_science_bytes) {
	nbytes++;
	*cp++ = TW_PAD_BYTE;
    }

    // Number of science bytes
    memcpy(nbytesp, &nbytes, sizeof(short));

    // Checksum
    {
	unsigned short checksum;
	checksum = crc_short((unsigned short *)scidatap,
	    nbytes / sizeof(short));
	memcpy(cp, &checksum, sizeof(checksum));
	cp += sizeof(checksum);
    }

    // End header words
    memcpy(cp, Endbuf, Endbuf_sizeb);
    cp += Endbuf_sizeb;

    return (cp - (unsigned char *)wrapbuf);
}

