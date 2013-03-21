
/*
{+D}
    SYSTEM:         Acromag IOS570 I/O Board

    FILENAME:       IOS570.h

    MODULE NAME:    IOS570.h

    VERSION:        A

    CREATION DATE:  03/19/10

    CODED BY:       FJM

    ABSTRACT:       This "inlcude" file contains all the major data
                    structures and memory map used in the software

    CALLING
	  SEQUENCE: #include "ios570.h"

    MODULE TYPE:    include file

    I/O RESOURCES:

    SYSTEM RESOURCES:

    MODULES CALLED:

    REVISIONS:

    DATE	   BY    	PURPOSE
    --------   -----	---------------------------------------------------

{-D}
*/


#ifndef BUILDING_FOR_KERNEL
#define INT_REQUEST_0  0	/* interrupt request 0 line on carrier */
#define INT_REQUEST_1  1	/* interrupt request 1 line on carrier */

#define MAX_CH		2 	/* the number of channels */
#define CH2_ID_VALUE	0x53	/* IDPROM value for two channel board */
#define CH0_REG_OFFSET	0x00000 /* second channel register offset */
#define CH1_REG_OFFSET	0x40000 /* second channel register offset */
#define MEM_OFFSET	0x20000	/* channel memory offset */
#endif /* BUILDING_FOR_KERNEL */

/*
    Defined below is the memory map template for the IOS570 Board.
    This structure provides access to the various registers on the board.
*/

struct map570             /* Memory map of board I/O space */
{
   word ControlReg;       /* I/O Control Register */
   word StatusReg;        /* I/O Status Register */
   word IntVectAReg;      /* I/O IntVectA Register */
   word IntVectBReg;      /* I/O IntVectB Register */
};

#ifndef BUILDING_FOR_KERNEL
/*
    Defined below is the structure which is used to hold the
    board's configuration information.
*/

struct cblk570
{
   struct map570 *brd_ptr;    /* base address of the board I/O space */
   byte *brd_memptr;          /* base address of I/O board memory */
   char slotLetter;           /* Slot letter */
   byte vector;               /* Interrupt Vector */
   byte bus_clock;            /* bus clock 8/32MHz */
   byte controller;           /* current controller being serviced */
   byte TransceiverInhibitA;  /* control register bits */
   byte TransceiverInhibitB;
   byte MasterClear;
   byte SelfTestEnable;
   byte RemoteTerminalAddressLatch;
   byte TagClockSource;
   byte InterruptEnable;
   int nHandle;               /* handle to an open carrier board */
   BOOL bCarrier;             /* flag indicating a carrier is open */
   BOOL bInitialized;         /* flag indicating ready to talk to board */
   unsigned short idProm[12]; /* holds ID prom */
};




/* Declare functions called */

void BcDemo(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);
void BcdBuf(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);

void RtdBuf(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);
void RtMode(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);
void RtPoll(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);
void RtIrq(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);

void RtMtDemo(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);
void RtMtCmbDemo(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);

void MtPoll(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);
void MtIrq(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);

void aceTest(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, char *pMemTestVec, int hDevice, int slot, int chan);
void looptest(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan);

long get_param(void);			/* input a parameter */
void selectch570(int *current_channel);
void scfg570(struct cblk570 *c_blk);
int init570(struct cblk570 *c_blk);

byte input_byte(int nHandle, byte*);/* function to read an input byte */
word input_word(int nHandle, word*);/* function to read an input word */
void output_byte(int nHandle, byte*, byte);	/* function to output a byte */
void output_word(int nHandle, word*, word);	/* function to output a word */
#endif /* BUILDING_FOR_KERNEL */
/* ISR */

void ios57xisr(void* pData);	/* interrupt handler for IOS57x */
