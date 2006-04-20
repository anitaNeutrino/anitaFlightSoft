/* sipthrottle.c - throttle the rate at which we write to the SIP high rate
 * TDRSS port. 
 *
 * Marty Olevitch, May '05
 */

#include "sipthrottle.h"

#include <time.h>	// time
#include <string.h>	// memset, memmove
#include <sys/time.h>	// gettimeofday

#undef NDEBUG
#include <assert.h>

#ifdef SR_DEBUG
#include <stdio.h>
#endif

static unsigned long Curtime = 0L;
static unsigned long Curcount = 0L;

#define BYTES_PER_SEC_DEFAULT 750L
static unsigned long Bytes_per_sec_limit = BYTES_PER_SEC_DEFAULT;
static unsigned long Bytes_per_unit_limit = BYTES_PER_SEC_DEFAULT;
static unsigned long Startsec = 0L;
static float Fracsec = 1.0;

static unsigned long get_curtime(void)
{
    struct timeval t;
    unsigned long usec;
    unsigned long cur;

#define USECperSEC 1000000L

    (void)gettimeofday(&t, NULL);
    usec = ((t.tv_sec - Startsec) * USECperSEC) + t.tv_usec;
    cur = usec / (USECperSEC * Fracsec);
    return cur;
}

void
sipthrottle_init(unsigned long lim, float frac)
{
    struct timeval t;
    //fprintf(stderr, "lim: %lu  frac: %f\n", lim, frac);
    sipthrottle_set_bytes_per_sec_limit(lim);
    sipthrottle_set_frac_sec(frac);
    (void)gettimeofday(&t, NULL);
    Startsec = t.tv_sec;
    Curtime = get_curtime();
}

void
sipthrottle_set_bytes_per_sec_limit(unsigned long lim)
{
    Bytes_per_sec_limit = lim;
}

void
sipthrottle_set_frac_sec(float frac)
{
    Fracsec = frac;
    Bytes_per_unit_limit = Bytes_per_sec_limit * Fracsec;
}

int
sipthrottle(unsigned long amountb)
{
    long bytes_available;
    unsigned long ntime;
    int retval = 0;

    ntime = get_curtime();
    if (ntime != Curtime) {
	// Now in new time unit.
	Curcount = 0;
	Curtime = ntime;
    }

    bytes_available = Bytes_per_unit_limit - Curcount;

    if (bytes_available < amountb) {
	// Not enough room left in this time unit.
	Curcount += bytes_available;
	retval = bytes_available;

    } else {
	Curcount += amountb;
	retval = amountb;
    }

    return retval;
}
