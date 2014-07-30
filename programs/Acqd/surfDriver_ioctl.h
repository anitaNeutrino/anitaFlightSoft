#include <linux/ioctl.h>


struct surfHousekeepingRequest {
    unsigned int length;
    unsigned int *address;
};

struct surfDriverRead {
    unsigned int address;
    unsigned int value;
};

/* we use A1 as our magic, for no good reason */
/* but it is good steak sauce */
#define SURF_IOC_MAGIC 0xA1

#define SURF_IOCHK _IO(SURF_IOC_MAGIC, 0)
#define SURF_IOCDATA _IO(SURF_IOC_MAGIC, 1)
#define SURF_IOCCLEARALL _IO(SURF_IOC_MAGIC, 2)
#define SURF_IOCCLEARHK _IO(SURF_IOC_MAGIC, 3)
#define SURF_IOCCLEAREVENT _IO(SURF_IOC_MAGIC, 4)
#define SURF_IOCEVENTREADY _IO(SURF_IOC_MAGIC, 5)

#define SURF_IOCENABLEINT _IO(SURF_IOC_MAGIC, 6)
#define SURF_IOCDISABLEINT _IO(SURF_IOC_MAGIC, 7)
#define SURF_IOCQUERYINT _IO(SURF_IOC_MAGIC, 8)

#define SURF_IOCGETSLOT _IOR(SURF_IOC_MAGIC, 9, short)
#define SURF_IOCHKREAD _IOR(SURF_IOC_MAGIC, 10, struct surfHousekeepingRequest)
#define SURF_IOCREAD _IOWR(SURF_IOC_MAGIC, 11, struct surfDriverRead)
#define SURF_IOCWRITE _IOWR(SURF_IOC_MAGIC, 12, struct surfDriverRead)
#define SURF_IOC_MAXNR 12
