/* telemwrap.c
 *
 * Marty Olevitch, May '08
 */

#include "telemwrap.h"
#include "crc_simple.h"
#include "highrate.h"

#include <string.h>

static unsigned short Startbuf[] = {
      START_HDR
    , AUX_HDR
    , ID_HDR	// For default LOS data
};
static int Startbuf_sizeb =
    sizeof(short) * (sizeof(Startbuf)/sizeof(Startbuf[0]));

static const unsigned short Odd_number_of_bytes_bit = 0x0002;

static unsigned short Endbuf[] = {
      SW_END_HDR
    , END_HDR
    , END_HDR2
};
static int Endbuf_sizeb =
    sizeof(short) * (sizeof(Endbuf)/sizeof(Endbuf[0]));

static unsigned long Buffer_count[3] = { 1L, 1L, 1L};

int
telemwrap_init(int datasource)
{
    if (TW_LOS != datasource && TW_SIP != datasource) {
	return -1;
    }
    /*
    if (TW_SIP == datasource) {
	// For SIP data, ID_HDR is 0xAE01 instead of 0xAE00
	Startbuf[2]++;
    }
    */
    return 0;
}

int
telemwrap(unsigned short *databuf, unsigned short *wrapbuf,
    unsigned short nbytes, int xtype, int datasource)
{
    unsigned char *cp;
    unsigned short *idhdrp;
    unsigned char *nbytesp;
    int odd_number_of_science_bytes =  (nbytes % 2 != 0);
    unsigned char *scidatap;
    unsigned short my_startbuf[3];

    memcpy(my_startbuf, Startbuf, Startbuf_sizeb);

    // For SIP data, ID_HDR is 0xAE01 instead of 0xAE00
    my_startbuf[2] += datasource;

    cp = (unsigned char *)wrapbuf;

    // Initial header words
    memcpy(cp, my_startbuf, Startbuf_sizeb);
    idhdrp = wrapbuf+2;
    cp += Startbuf_sizeb;

    // Buffer count
    memcpy(cp, &Buffer_count[xtype], sizeof(Buffer_count[xtype]));
    cp += sizeof(Buffer_count[xtype]);
    Buffer_count[xtype]++;

    nbytesp = cp;
    cp += sizeof(nbytes);

    // Science data
    scidatap = cp;
    memcpy(cp, databuf, nbytes);
    cp += nbytes;

    if (odd_number_of_science_bytes) {
	*idhdrp |= Odd_number_of_bytes_bit;
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

