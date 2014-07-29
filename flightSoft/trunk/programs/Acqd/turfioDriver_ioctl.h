#include <linux/ioctl.h>

/* we use A1 as our magic, for no good reason */
/* but it is good steak sauce */
#define TURFIO_IOC_MAGIC 0xA1
/* SURFs have 0-15, LOS has 16-31, we take 32-47 */
#define TURFIO_IOC_MINNR 0x20

struct turfioDriverRead {
  unsigned int address;
  unsigned int value;
};

struct turfioHousekeepingRequest 
{
   unsigned int *address;
   unsigned int length;
};
   

#define TURFIO_IOCSETGPIO _IOW(TURFIO_IOC_MAGIC, TURFIO_IOC_MINNR+0, unsigned int)
#define TURFIO_IOCGETGPIO _IOR(TURFIO_IOC_MAGIC, TURFIO_IOC_MINNR+1, unsigned int)
#define TURFIO_IOCREAD _IOWR(TURFIO_IOC_MAGIC, TURFIO_IOC_MINNR+2, struct turfioDriverRead)
#define TURFIO_IOCWRITE _IOWR(TURFIO_IOC_MAGIC, TURFIO_IOC_MINNR+3, struct turfioDriverRead)
#define TURFIO_IOCHK _IOWR(TURFIO_IOC_MAGIC, TURFIO_IOC_MINNR+4, struct turfioHousekeepingRequest)
#define TURFIO_IOCCLEARALL _IO(TURFIO_IOC_MAGIC, TURFIO_IOC_MINNR+5)
#define TURFIO_IOCCLEAREVT _IO(TURFIO_IOC_MAGIC, TURFIO_IOC_MINNR+6)
#define TURFIO_IOC_MAXNR TURFIO_IOC_MINNR+6
