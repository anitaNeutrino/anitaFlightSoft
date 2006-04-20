// highrate_impl.h - internal constants
//
// Marty Olevitch, May '05

#ifndef _HIGHRATE_IMPL_H
#define _HIGHRATE_IMPL_H

#define HR_HEADER	0xbeefcafe

struct highrate_header {
    unsigned long id;
    unsigned long bufcnt;
    unsigned long nbytes;
};

#define HR_ENDER	0xfeedface
struct highrate_ender {
    unsigned long id;
    unsigned long bufcnt;

    unsigned char chksum;
    unsigned char filler[3];
};

#endif // _HIGHRATE_IMPL_H
