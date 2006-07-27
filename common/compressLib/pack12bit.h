/* pack12bit.h */

#ifndef _PACK_H
#define _PACK_H
#define PACKVALS 4	// no. of 16-bit values packed into each unit
#define PACKWDS  3	// no. of 16-bit words the values are packed into
void unpack12bit(unsigned short *w, void *srcp);
void pack12bit(unsigned short *w, unsigned char *destp);
#endif /* _PACK_H */
