

/*
{+D}
    SYSTEM:         APC8620.h

    FILENAME:	    APC8620.h

    MODULE NAME:    Functions common to the APC8620 example software.

    VERSION:	    A

    CREATION DATE:  06/06/01

    DESIGNED BY:    FJM

    CODED BY:	    FJM

    ABSTRACT:       This file contains the definitions, structures
                    and prototypes for APC8620.

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

	This file contains the definitions, structures and prototypes for APC8620.
*/



#ifndef __KERNEL__
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#endif /* __KERNEL__ */


typedef int BOOL;
typedef unsigned long ULONG;
typedef unsigned char byte;	/*  custom made byte data type */
typedef unsigned short word;	/* custom made word data type */
typedef int ACRO_CSTATUS;	/*  Custom made ACRO_CSTATUS data type, used */
                        /*   as return value from the carrier functions. */

#define TRUE	1	/* Boolean value for true */
#define FALSE	0	/* Boolean value for false */

#define SLOT_A 	0x41	/* Value for slot A */
#define SLOT_B	0x42	/* Value for slot B */
#define SLOT_C 	0x43	/* Value for slot C */
#define SLOT_D	0x44	/* Value for slot D */
#define SLOT_E	0x45	/* Value for slot E */

#define SLOT_A_IO_OFFSET 0x0180	/*  Slot A IO space addr. offset from carrier base addr. */
#define SLOT_A_ID_OFFSET 0x0040	/*  Slot A ID space addr. offset from carrier base addr. */
#define SLOT_B_IO_OFFSET 0x0200	/*  Slot B IO space addr. offset from carrier base addr. */
#define SLOT_B_ID_OFFSET 0x0080	/*  Slot B ID space addr. offset from carrier base addr. */
#define SLOT_C_IO_OFFSET 0x0280	/*  Slot C IO space addr. offset from carrier base addr. */
#define SLOT_C_ID_OFFSET 0x00C0	/*  Slot C ID space addr. offset from carrier base addr. */
#define SLOT_D_IO_OFFSET 0x0300	/*  Slot D IO space addr. offset from carrier base addr. */
#define SLOT_D_ID_OFFSET 0x0100	/*  Slot D ID space addr. offset from carrier base addr. */
#define SLOT_E_IO_OFFSET 0x0380	/*  Slot E IO space addr. offset from carrier base addr. */
#define SLOT_E_ID_OFFSET 0x0140	/*  Slot E ID space addr. offset from carrier base addr. */

#define MAX_CARRIERS 5	/* maximum number of carriers */

#define SOFTWARE_RESET 0x8000	/*  Value to OR with control register to reset carrier */
#define TIME_OUT_INT_ENABLE 0x0008	/* IP access time out interrupt enable */
#define APC_INT_ENABLE	0x0004		/* IP module interrupt enable */
#define APC_INT_PENDING_CLEAR 0x0020	/* IP Module interrupt pending bit, clear interrupts */

#define IPA_INT0_PENDING 0x0001		/* IP A Int 0 Interrupt Pending bit */
#define IPA_INT1_PENDING 0x0002		/* IP A Int 1 Interrupt Pending bit */
#define IPB_INT0_PENDING 0x0004		/* IP B Int 0 Interrupt Pending bit */
#define IPB_INT1_PENDING 0x0008		/* IP B Int 1 Interrupt Pending bit */
#define IPC_INT0_PENDING 0x0010		/* IP C Int 0 Interrupt Pending bit */
#define IPC_INT1_PENDING 0x0020		/* IP C Int 1 Interrupt Pending bit */
#define IPD_INT0_PENDING 0x0040		/* IP D Int 0 Interrupt Pending bit */
#define IPD_INT1_PENDING 0x0080		/* IP D Int 1 Interrupt Pending bit */
#define IPE_INT0_PENDING 0x0100		/* IP E Int 0 Interrupt Pending bit */
#define IPE_INT1_PENDING 0x0200		/* IP E Int 1 Interrupt Pending bit */
#define TIME_OUT_PENDING 0x0400		/* Time out interrupt pending */


/* 
	ACRO_CSTATUS return values
	Errors will have most significant bit set and are preceded with an E_.
	Success values will be succeeded with an S_.
*/
#define ERROR 0x8000 /* general */
#define E_OUT_OF_MEMORY 	0x8001	/*  Out of memory status value */
#define E_OUT_OF_CARRIERS	0x8002	/*  All carrier spots have been taken */
#define E_INVALID_HANDLE	0x8003	/*  no carrier exists for this handle */
#define E_INVALID_SLOT		0x8004	/*  no slot available with this number */
#define E_NOT_INITIALIZED	0x8006	/*  carrier not initialized */
#define E_NOT_IMPLEMENTED   0x8007;	/*  Function is not implemented */
#define E_NO_INTERRUPTS 	0x8008;	/*  Carrier will be unable to handle interrupts */
#define S_OK			    0x0000	/*  Everything worked successfully */



/*
	APC8620	PCI Carrier information
*/
#define APC8620_VENDOR_ID 0x10b5	/* PLX vendor ID */
#define APC8620_DEVICE_ID 0x1024	/* Acromag's device ID */

/* 
	PCI Carrier data structure
*/

typedef struct
{
	int nHandle;					/* handle of this carrier structure */
	long lBaseAddress;				/* pointer to base address of carrier board */
	int nIntLevel;					/* Interrupt level of Carrier */
	int nInteruptID;				/* ID of interrupt handler */
	BOOL bInitialized;				/* Carrier initialized flag */
	BOOL bIntEnabled;				/* interrupts enabled flag */
	int nCarrierDeviceHandle;		/* handle to an open carrier device */
}CARRIERDATA_STRUCT;

typedef struct
{
	word controlReg;	/*  Status/Control Register */
	word intPending;	/*  Interrupt Pending Register */
	word slotAInt0;		/*  Slot A interrupt 0 select space */
	word slotAInt1;		/*  Slot A interrupt 1 select space */
	word slotBInt0;		/*  Slot B interrupt 0 select space */
	word slotBInt1;		/*  Slot B interrupt 1 select space */
	word slotCInt0;		/*  Slot C interrupt 0 select space */
	word slotCInt1;		/*  Slot C interrupt 1 select space */
	word slotDInt0;		/*  Slot D interrupt 0 select space */
	word slotDInt1;		/*  Slot D interrupt 1 select space */
	word slotEInt0;		/*  Slot E interrupt 0 select space */
	word slotEInt1;		/*  Slot E interrupt 1 select space */
}PCI_BOARD_MEMORY_MAP;

/*
	Global variables
*/
int	gNumberCarriers;		/*  Number of carrier boards that have been opened */

CARRIERDATA_STRUCT *gpCarriers[MAX_CARRIERS];	/* pointer to the carrier boards */


/* Function Prototypes */

ACRO_CSTATUS GetCarrierAddress(int nHandle, long* pAddress);
ACRO_CSTATUS SetCarrierAddress(int nHandle, long lAddress);
ACRO_CSTATUS GetIpackAddress(int nHandle, char chSlot, long* pAddress);
ACRO_CSTATUS ReadIpackID(int nHandle, char chSlot, word* pWords, int nWords);
ACRO_CSTATUS EnableInterrupts(int nHandle);
ACRO_CSTATUS DisableInterrupts(int nHandle);
ACRO_CSTATUS SetInterruptLevel(int nHandle, word uLevel);
ACRO_CSTATUS GetInterruptLevel(int nHandle, word* pLevel);
ACRO_CSTATUS SetInterruptHandler(int nHandle, char chSlot, int nRequestNumber,
							int nVector, void(*pHandler)(void*), void* pData);
ACRO_CSTATUS InitCarrierLib(void);
ACRO_CSTATUS CarrierOpen(int nDevInstance, int* pHandle);
ACRO_CSTATUS CarrierOpenDev(int nDevInstance, int* pHandle, char *devname);
ACRO_CSTATUS CarrierClose(int nHandle);
ACRO_CSTATUS CarrierInitialize(int nHandle);
byte GetLastSlotLetter();	

/*  Functions used by above functions */
void AddCarrier(CARRIERDATA_STRUCT* pCarrier);
void DeleteCarrier(int nHandle);
CARRIERDATA_STRUCT* GetCarrier(int nHandle);
const struct sigevent  *CarrierIntHandler( void *nParam, int id);

void output_word(int nHandle, word *p, word v);
void output_byte(int nHandle, byte *p, byte v);
byte input_byte(int nHandle, byte *p);
word input_word(int nHandle, word *p);
