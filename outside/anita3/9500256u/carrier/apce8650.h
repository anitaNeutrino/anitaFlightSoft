
/*
{+D}
    SYSTEM:         Linux

    FILENAME:	    apce8650.h

    MODULE NAME:    Functions common to the apce8650 example software.

    VERSION:	    A

    CREATION DATE:  04/01/11

    DESIGNED BY:    FJM

    CODED BY:	    FJM

    ABSTRACT:       This file contains the definitions, structures
                    and prototypes for apce8650.

    CALLING
	SEQUENCE:

    MODULE TYPE:

    I/O RESOURCES:

    SYSTEM
	RESOURCES:

    MODULES
	CALLED:

    REVISIONS:

  DATE	  BY	    PURPOSE
-------  ----	------------------------------------------------


{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:

	This file contains the definitions, structures and prototypes for apce8650.
*/


#ifndef BUILDING_FOR_KERNEL
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* Required for FC4 */
#include <stdlib.h>     /* malloc */
#include <string.h>     /* memset */
#endif /* BUILDING_FOR_KERNEL */

#ifndef BOOL
typedef int BOOL;
#endif /* BOOL */
#ifndef ULONG
typedef uint32_t ULONG;
#endif /* ULONG */
#ifndef byte
typedef unsigned char byte;	/*  custom made byte data type */
#endif /* byte */
#ifndef word
typedef unsigned short word;	/* custom made word data type */
#endif /* word */
#ifndef CARSTATUS
typedef int CARSTATUS;	/*  Custom made CARSTATUS data type, used as return value from the carrier functions. */
#endif /* CARSTATUS */


#if	!defined(FALSE) || (FALSE!=0)
#define FALSE		0	/* Boolean value for false */
#endif

#if	!defined(TRUE) || (TRUE!=1)
#define TRUE		1	/* Boolean value for true */
#endif

#define SLOT_A 	0x41	/* Value for slot A */
#define SLOT_B	0x42	/* Value for slot B */
#define SLOT_C 	0x43	/* Value for slot C */
#define SLOT_D 	0x44	/* Value for slot D */


#define SLOT_A_IO_OFFSET	 0x0180		/*  Slot A IO space addr. offset from carrier base addr. */
#define SLOT_A_ID_OFFSET	 0x0040		/*  Slot A ID space addr. offset from carrier base addr. */
#define SLOT_A_MEM_OFFSET	 0x00800000	/*  Slot A MEM space addr. */
#define SLOT_B_IO_OFFSET	 0x0200		/*  Slot B IO space addr. offset from carrier base addr. */
#define SLOT_B_ID_OFFSET	 0x0080		/*  Slot B ID space addr. offset from carrier base addr. */
#define SLOT_B_MEM_OFFSET	 0x01000000	/*  Slot B MEM space addr. */
#define SLOT_C_IO_OFFSET	 0x0280		/*  Slot C IO space addr. offset from carrier base addr. */
#define SLOT_C_ID_OFFSET	 0x00C0		/*  Slot C ID space addr. offset from carrier base addr. */
#define SLOT_C_MEM_OFFSET	 0x01800000	/*  Slot C MEM space addr. */
#define SLOT_D_IO_OFFSET	 0x0300		/*  Slot D IO space addr. offset from carrier base addr. */
#define SLOT_D_ID_OFFSET	 0x0100		/*  Slot D ID space addr. offset from carrier base addr. */
#define SLOT_D_MEM_OFFSET	 0x02000000	/*  Slot D MEM space addr. */

#define MAX_CARRIERS 4	/* maximum number of carriers */
#define MAX_SLOTS    4	/* maximum number of  IP slots */

#define SOFTWARE_RESET		0x0100		/*  Value to OR with control register to reset carrier */
#define TIME_OUT_INT_ENABLE 0x0008		/* IP access time out interrupt enable */
#define APC_INT_ENABLE		0x0004		/* IP module interrupt enable */
#define APC_INT_PENDING_CLEAR 0x0020	/* IP Module interrupt pending bit, clear interrupts */
#define FLASH_BUSY			0x0400		/* FLASH busy status */

#define IPA_INT0_PENDING 0x0001		/* IP A Int 0 Interrupt Pending bit */
#define IPA_INT1_PENDING 0x0002		/* IP A Int 1 Interrupt Pending bit */
#define IPB_INT0_PENDING 0x0004		/* IP B Int 0 Interrupt Pending bit */
#define IPB_INT1_PENDING 0x0008		/* IP B Int 1 Interrupt Pending bit */
#define IPC_INT0_PENDING 0x0010		/* IP C Int 0 Interrupt Pending bit */
#define IPC_INT1_PENDING 0x0020		/* IP C Int 1 Interrupt Pending bit */
#define IPD_INT0_PENDING 0x0040		/* IP D Int 0 Interrupt Pending bit */
#define IPD_INT1_PENDING 0x0080		/* IP D Int 1 Interrupt Pending bit */
#define TIME_OUT_PENDING 0x0400		/* Time out interrupt pending */

#define CARRIER_INT_MASK 0x3FF		/* pending mask */


/*	Carrier attributes */

#define VME_CARRIER			1 << 4		/* VME type carrier */
#define ISA_CARRIER			2 << 4		/* ISA type carrier */
#define PCI_CARRIER			3 << 4		/* PCI type carrier */

#define CARRIER_MEM			1 << 2		/* carrier supports memory space */
#define CARRIER_CLK			1			/* carrier supports 32MHz IP clocking */


/* 
	CARSTATUS return values
	Errors will have most significant bit set and are preceded with an E_.
	Success values will be succeeded with an S_.
*/
#define ERROR			0x8000	/* general */
#define E_OUT_OF_MEMORY 	0x8001	/*  Out of memory status value */
#define E_OUT_OF_CARRIERS	0x8002	/*  All carrier spots have been taken */
#define E_INVALID_HANDLE	0x8003	/*  no carrier exists for this handle */
#define E_INVALID_SLOT		0x8004	/*  no slot available with this number */
#define E_NOT_INITIALIZED	0x8006	/*  carrier not initialized */
#define E_NOT_IMPLEMENTED	0x8007	/*  Function is not implemented */
#define E_NO_INTERRUPTS 	0x8008	/*  Carrier will be unable to handle interrupts */
#define S_OK			0x0000	/*  Everything worked successfully */


/*
	APCE8650	PCIe Carrier information
*/
#define APCE8650_VENDOR_ID 0x16D5	/* Acromag's vendor ID */
#define APCE8650_DEVICE_ID 0x5901	/* Acromag's device ID */

/* 
	PCIe Carrier data structure
*/

typedef struct
{
	int nHandle;				/* handle of this carrier structure */
	int nIntLevel;				/* Interrupt level of Carrier */
	int nInteruptID;			/* ID of interrupt handler */
	int nDevInstance;			/* Device Instance */
	int nCarrierDeviceHandle;	/* handle to an open carrier device */
	long lBaseAddress;			/* pointer to base address of carrier board */
	long lMemBaseAddress;		/* pointer to base address of carrier board memory space */
	BOOL bInitialized;			/* Carrier initialized flag */
	BOOL bIntEnabled;			/* interrupts enabled flag */
	word uCarrierID;			/* Carrier Identification value from open */
	char devname[64];			/* device name */
}CARRIERDATA_STRUCT;

typedef struct
{
	word controlReg;	/* Status/Control Register */
	word intPending;	/* Interrupt Pending Register */
	word slotAInt0;		/* Slot A interrupt 0 select space */
	word slotAInt1;		/* Slot A interrupt 1 select space */
	word slotBInt0;		/* Slot B interrupt 0 select space */
	word slotBInt1;		/* Slot B interrupt 1 select space */
	word slotCInt0;		/* Slot C interrupt 0 select space */
	word slotCInt1;		/* Slot C interrupt 1 select space */
	word slotDInt0;		/* Slot D interrupt 0 select space */
	word slotDInt1;		/* Slot D interrupt 1 select space */
	word noslotEInt0;
	word noslotEInt1;
	word IPClockControl;/* IP Clock Control Register */
	word ID_Register;	/* 16 bit non-volatile identifier */
}PCIe_BOARD_MEMORY_MAP;


/*
    Defined below is the Interrupt Handler Data Structure.
    The interrupt handler is provided with a pointer that points
    to this structure.  From this the handler has a link back to
    its related process and common data area.  The usage of the members
    is not absolute.  Their typical uses are described below.
*/
struct handler_data
    {
    int h_pid;          /* Handler related process i.d. number.
                           Needed to know if handler is going to wake up,
                           put to sleep, or suspend a process.*/
    char *hd_ptr;       /* Handler related data pointer.
                           Needed to know if handler is going to access
                           a process' data area. */
    };


/* Interrupt service routine data structure for a single slot */
struct isr_data                     /* can be enlarged as required */
{
	unsigned long slot_io_address;  /* this slots I/O address */
	unsigned long slot_mem_address;	/* this slots mem address */
	unsigned long slot_letter;      /* A, B, C or D */
	unsigned long dev_num[8];       /* for 57x modules */
};

struct carrier_isr_data             /* ISR routine handler structure for a single carrier */
{
  struct isr_data    slot_isr_data[MAX_SLOTS];
};


/*
	Function Prototypes
*/


byte GetLastSlotLetter(void);
CARSTATUS GetIpackAddress(int nHandle, char chSlot, long* pAddress);
CARSTATUS ReadIpackID(int nHandle, char chSlot, word* pWords, int nWords);
CARSTATUS EnableInterrupts(int nHandle);
CARSTATUS DisableInterrupts(int nHandle);
CARSTATUS SetInterruptLevel(int nHandle, word uLevel);
CARSTATUS GetInterruptLevel(int nHandle, word* pLevel);
CARSTATUS SetInterruptHandler(int nHandle, char chSlot, int nRequestNumber,
							int nVector, void(*pHandler)(void*), void* pData);
CARSTATUS GetCarrierAddress(int nHandle, long* pAddress);
CARSTATUS SetCarrierAddress(int nHandle, long lAddress);

CARSTATUS InitCarrierLib(void);
CARSTATUS CarrierOpen(int nDevInstance, int* pHandle);
CARSTATUS CarrierClose(int nHandle);
CARSTATUS CarrierInitialize(int nHandle);
CARSTATUS AccessVPD( int nHandle, ULONG Address, ULONG* Data, ULONG Cycle );

CARSTATUS WakeUp57x(int nHandle, int DevNum);
CARSTATUS SetInfo57x(int nHandle, int slot, int channel, int DevNum);
unsigned int Block57x(int nHandle, int DevNum, long unsigned int* dwStatus);

/* carrier enhancements */
CARSTATUS GetMemoryAddress(int nHandle, long* pAddress);
CARSTATUS SetMemoryAddress(int nHandle, long lAddress);
CARSTATUS GetIpackMemoryAddress(int nHandle, char chSlot, long* pAddress);
CARSTATUS SetIPClockControl(int nHandle, char chSlot, word uControl);
CARSTATUS GetIPClockControl(int nHandle, char chSlot, word* pControl);
CARSTATUS GetCarrierID(int nHandle, word* pCarrierID);
CARSTATUS SetAutoAckDisable(int nHandle, word uState);
CARSTATUS GetAutoAckDisable(int nHandle, word* pState);
CARSTATUS GetTimeOutAccess(int nHandle, word* pState);
CARSTATUS GetIPErrorBit(int nHandle, word* pState);

/*  Functions used by above functions */
void AddCarrier(CARRIERDATA_STRUCT* pCarrier);
void DeleteCarrier(int nHandle);
CARRIERDATA_STRUCT* GetCarrier(int nHandle);
byte input_byte(int nHandle, byte*);/* function to read an input byte */
word input_word(int nHandle, word*);/* function to read an input word */
void output_byte(int nHandle, byte*, byte);	/* function to output a byte */
void output_word(int nHandle, word*, word);	/* function to output a word */
long input_long(int nHandle, long*);			/* function to read an input long */
void output_long(int nHandle, long*, long);	/* function to output a long */
void blocking_start_convert(int nHandle, word *p, word v);
void blocking_start_convert_byte(int nHandle, byte *p, byte v);
