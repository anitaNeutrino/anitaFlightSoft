#include <linux/ioctl.h>

/* we use A1 as our magic, for no good reason */
/* but it is good steak sauce */
#define TURFIO_IOC_MAGIC 0xA1
/* SURFs have 0-15, LOS has 16-31, we take 32-47 */
#define TURFIO_IOC_MINNR 0x20

#define TURFIO_IOCSETGPIO _IOW(TURFIO_IOC_MAGIC, TURFIO_IOC_MINNR+0, unsigned int)
#define TURFIO_IOCGETGPIO _IOR(TURFIO_IOC_MAGIC, TURFIO_IOC_MINNR+1, unsigned int)

#define TURFIO_IOC_MAXNR TURFIO_IOC_MINNR+2
