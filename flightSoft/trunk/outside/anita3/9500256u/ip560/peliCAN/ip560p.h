
/* #define SELECT_BIG_ENDIAN */	/* define this to compile for big_endian applications */

/*
{+D}
    SYSTEM:	    Acromag IP560 CAN I/O Board

    FILENAME:		IP560p.h

    MODULE NAME:	IP560p.h

    VERSION:		A

    CREATION DATE:	03/19/10

    CODED BY:		FJM

    ABSTRACT:		This "inlcude" file contains all the major data
		    structures and memory map used in the software

    CALLING
	  SEQUENCE:	#include "ip560p.h"

    MODULE TYPE:	include file

    I/O RESOURCES:

    SYSTEM RESOURCES:

    MODULES CALLED:

    REVISIONS:

    DATE	   BY		PURPOSE
    --------   -----	---------------------------------------------------

{-D}
*/



/*
    Bit mask positions
*/


#ifndef BUILDING_FOR_KERNEL
#define OURID	    0x0000     /* CAN ID */
#define ACCEPTMASK  0xFF       /* accept mask */

/* Control register */
#define CR_RR	   (1<<0)      /* Reset Request */
#define CR_RIE	   (1<<1)      /* Receive interrupt enable */
#define CR_TIE	   (1<<2)      /* Transmit interrupt enable */
#define CR_EIE	   (1<<3)      /* Error interrupt enable */
#define CR_OIE	   (1<<4)      /* Overrun interrupt enable */

/* Mode register */
#define MOD_RM	   (1<<0)      /* Reset Mode */
#define MOD_LOM    (1<<1)      /* Listen Only Mode */
#define MOD_STM    (1<<2)      /* Self Test Mode */
#define MOD_AFM    (1<<3)      /* Acceptance Filter Mode */
#define MOD_SM	   (1<<4)      /* Sleep Mode */

/* Command register */
#define CMR_TR	   (1<<0)      /* Transmission request */
#define CMR_AT	   (1<<1)      /* Abort Transmission */
#define CMR_RRB    (1<<2)      /* Release Receive Buffer */
#define CMR_CDO    (1<<3)      /* Clear Data Overrun */
#define CMR_SRR    (1<<4)      /* Self reception request */

/* Status register */
#define SR_RBS	   (1<<0)      /* Receive Buffer Status */
#define SR_DOS	   (1<<1)      /* Data Overrun Status */
#define SR_TBS	   (1<<2)      /* Transmit Buffer Status */
#define SR_TCS	   (1<<3)      /* Transmission Complete Status */
#define SR_RS	   (1<<4)      /* Receive Status */
#define SR_TS	   (1<<5)      /* Transmit Status */
#define SR_ES	   (1<<6)      /* Error Status  */
#define SR_BS	   (1<<7)      /* Bus Status */

/* Interrupt enable register */
#define IER_DIS_INT (0<<0)     /* Disable interrupts */
#define IER_RIE     (1<<0)     /* Receive Interrupt Enable */
#define IER_TIE     (1<<1)     /* Transmit Interrupt Enable */
#define IER_EIE     (1<<2)     /* Error Warning Interrupt Enable */
#define IER_DOIE    (1<<3)     /* Data Overrun Interrupt Enable */
#define IER_WUIE    (1<<4)     /* Wake-Up Interrupt Enable */
#define IER_EPIE    (1<<5)     /* Error Passive Interrupt Enable */
#define IER_ALIE    (1<<6)     /* Arbitration Lost Interrupt Enable */
#define IER_BEIE    (1<<7)     /* Bus Error Interrupt Enable */

#define INT_REQUEST_0  0       /* interrupt request 0 line on carrier */
#define INT_REQUEST_1  1       /* interrupt request 1 line on carrier */
#endif /* BUILDING_FOR_KERNEL */


/*
    Defined below is the memory map template for the IP560 Board.
    This structure provides access to the various registers on the board.
*/


#ifdef SELECT_BIG_ENDIAN	/* Big ENDIAN structure */
/* Big ENDIAN Memory map of board I/O space */
#define ControlRegister0 1	/* I/O Control register */
#define ControlRegister1 3	/* I/O Control register */
#define StatusRegister	 5	/* I/O Status register */
#define IntVectARegister 7	/* I/O IntVectA register */
#define IntVectBRegister 9	/* I/O IntVectB register */

/* Big ENDIAN Memory map of board in PeliCAN mode */
/* reset & operating modes */
#define CAN_MOD 	 1     /* Mode register */
#define CAN_CMR 	 3     /* Command register */
#define CAN_SR		 5     /* Status register */
#define CAN_IR		 7     /* Interrupt register */
#define CAN_IER 	 9     /* Interrupt enable register */
#define CAN_BTR0	13     /* Bus timing register 0 */
#define CAN_BTR1	15     /* Bus timing register 1 */
#define CAN_OCR 	17     /* Output control register */
#define CAN_ALC 	23     /* Arbitration lost capture */
#define CAN_ECC 	25     /* Error code capture register */
#define CAN_EWL 	27     /* Error warning limit register */
#define CAN_RXERR	29     /* Receive error counter */
#define CAN_TXERR	31     /* Transmit error counter */
#define CAN_CDR 	63     /* Clock divider register */

/* reset mode */
#define CAN_ACR0	33     /* Acceptance code register 0 */
#define CAN_ACR1	35     /* Acceptance code register 1 */
#define CAN_ACR2	37     /* Acceptance code register 2 */
#define CAN_ACR3	39     /* Acceptance code register 3 */
#define CAN_AMR0	41     /* Acceptance mask register 0 */
#define CAN_AMR1	43     /* Acceptance mask register 1 */
#define CAN_AMR2	45     /* Acceptance mask register 2 */
#define CAN_AMR3	47     /* Acceptance mask register 3 */

/* operating mode */
#define CAN_FIR 	33     /* Frame information register */
#define CAN_ID1 	35     /* Identifier 1 */
#define CAN_ID2 	37     /* Identifier 2 */
#define CAN_ID3 	39     /* Identifier 3 (EFF) */
#define CAN_ID4 	41     /* Identifier 4 (EFF) */

#define CAN_DATA_SFF(x) (39 + (x)) /* Data registers standard frame format */
#define CAN_DATA_EFF(x) (43 + (x)) /* Data registers extended frame format */
#else
/* Little ENDIAN Memory map of board I/O space */
#define ControlRegister0 0	/* I/O Control register */
#define ControlRegister1 2	/* I/O Control register */
#define StatusRegister	 4	/* I/O Status register */
#define IntVectARegister 6	/* I/O IntVectA register */
#define IntVectBRegister 8	/* I/O IntVectB register */

/* Little ENDIAN Memory map of board in PeliCAN mode */
/* reset & operating modes */
#define CAN_MOD 	 0     /* Mode register */
#define CAN_CMR 	 2     /* Command register */
#define CAN_SR		 4     /* Status register */
#define CAN_IR		 6     /* Interrupt register */
#define CAN_IER 	 8     /* Interrupt enable register */
#define CAN_BTR0	12     /* Bus timing register 0 */
#define CAN_BTR1	14     /* Bus timing register 1 */
#define CAN_OCR 	16     /* Output control register */
#define CAN_ALC 	22     /* Arbitration lost capture */
#define CAN_ECC 	24     /* Error code capture register */
#define CAN_EWL 	26     /* Error warning limit register */
#define CAN_RXERR	28     /* Receive error counter */
#define CAN_TXERR	30     /* Transmit error counter */
#define CAN_CDR 	62     /* Clock divider register */

/* reset mode */
#define CAN_ACR0	32     /* Acceptance code register 0 */
#define CAN_ACR1	34     /* Acceptance code register 1 */
#define CAN_ACR2	36     /* Acceptance code register 2 */
#define CAN_ACR3	38     /* Acceptance code register 3 */
#define CAN_AMR0	40     /* Acceptance mask register 0 */
#define CAN_AMR1	42     /* Acceptance mask register 1 */
#define CAN_AMR2	44     /* Acceptance mask register 2 */
#define CAN_AMR3	46     /* Acceptance mask register 3 */

/* operating mode */
#define CAN_FIR 	32     /* Frame information register */
#define CAN_ID1 	34     /* Identifier 1 */
#define CAN_ID2 	36     /* Identifier 2 */
#define CAN_ID3 	38     /* Identifier 3 (EFF) */
#define CAN_ID4 	40     /* Identifier 4 (EFF) */

#define CAN_DATA_SFF(x) (38 + (x)) /* Data registers standard frame format */
#define CAN_DATA_EFF(x) (42 + (x)) /* Data registers extended frame format */
#endif /* SELECT_BIG_ENDIAN */


#ifndef BUILDING_FOR_KERNEL
/*
    Defined below is the structure which is used to hold the
    board's xmit & receive message information.
*/

struct CAN_msg
{
  byte format;		      /* 0 - STANDARD, 1- EXTENDED IDENTIFIER */
  byte type;		      /* 0 - DATA FRAME, 1 - REMOTE FRAME */
  word id;		      /* Identifier */
  byte len;		      /* Length of data field in bytes */
  byte data[8]; 	      /* Data field */
};



/*
    Defined below is the structure which is used to hold the
    board's configuration information.
*/

struct cblk560
{
   byte *brd_ptr;	      /* base address of the board I/O space */
   byte *brd_memptr;	      /* base address of I/O board memory space */
   struct CAN_msg CAN_TxMsg;  /* CAN message for sending */
   struct CAN_msg CAN_RxMsg;  /* CAN message for receiving */
   char slotLetter;	      /* Slot letter */
   byte vector; 	      /* Interrupt Vector */
   byte acr[4]; 	      /* Acceptance Code */
   byte amr[4]; 	      /* Acceptance Mask */
   byte btr0;		      /* Bus Timing 0 */
   byte btr1;		      /* Bus Timing 1 */
   byte ocr;		      /* Output Control */
   byte cdr;		      /* Clock Divider */
   byte mr;		      /* Mode */
   byte cmr;		      /* Command */
   byte controller;	      /* current controller being serviced */
   byte transceiver_enable;   /* enabled/disabled */
   byte transceiver_standby;  /* standby/active */
   byte bus_clock;		  /* bus clock 8/32MHz */
   int nHandle; 	      /* handle to an open carrier board */
   int hflag;		      /* interrupt handler installed flag */
   BOOL bCarrier;	      /* flag indicating a carrier is open */
   BOOL bInitialized;	      /* flag indicating ready to talk to board */
   unsigned short idProm[12]; /* holds ID prom */
};




/* Declare functions called */

int sja1000init(struct cblk560 *);  /* routine to configure the board */
void sja1000reg_dump(struct cblk560 *c_blk);
int sja1000_xmit (struct cblk560 *c_blk);
int sja1000_recv (struct cblk560 *c_blk);
void sja1000_get_btrs(struct cblk560 *c_blk, unsigned int index );

long get_param(void);			/* input a parameter */
void scfg560(struct cblk560 *c_blk);
void selectch560(int *current_channel);
void edit_xmit_buffer(struct cblk560 *c_blk);
void putMsg(struct cblk560 *c_blk);
void getMsg(struct cblk560 *c_blk);

byte input_byte(int nHandle, byte*);/* function to read an input byte */
word input_word(int nHandle, word*);/* function to read an input word */
void output_byte(int nHandle, byte*, byte);	/* function to output a byte */
void output_word(int nHandle, word*, word);	/* function to output a word */
#endif /* BUILDING_FOR_KERNEL */

/* ISR */

void isr_560p(void* pData);	/* interrupt handler for IP560 */

