/* sipthrottle.h
 * Marty Olevitch, May '05
 */

#ifndef _SIPTHROTTLE_H
#define _SIPTHROTTLE_H

// sipthrottle_init - set the per-second rate limit (lim), and the value
// frac, which is multiplied by 1 second to produce the unit of time we
// throttle to.
void sipthrottle_init(unsigned long lim, float frac);

// sipthrottle_set_bytes_per_sec_limit - sets the per-second rate limit.
void sipthrottle_set_bytes_per_sec_limit(unsigned long lim);

// sipthrottle_set_frac_sec - set the unit of time fraction.
void sipthrottle_set_frac_sec(float frac);

// sipthrottle - do the throttling. The number of bytes that one wants to
// write is specified in amountb. The return code from this function
// specifies the number of bytes than can be written and still stay within
// the desired rate.
int sipthrottle(unsigned long amountb);

#endif // _SIPTHROTTLE_H

