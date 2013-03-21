
/* #define SELECT_BIG_ENDIAN */	/* define this to compile for big_endian applications */

/*
{+D}
    SYSTEM:         Acromag IOS560 CAN I/O Board

    FILENAME:       IOS560b.h

    MODULE NAME:    IOS560b.h

    VERSION:        A

    CREATION DATE:  03/19/10

    CODED BY:       FJM

    ABSTRACT:       This "inlcude" file contains all the major data
                    structures and memory map used in the software

    CALLING
	  SEQUENCE:   #include "ios560b.h"

    MODULE TYPE:    include file

    I/O RESOURCES:

    SYSTEM RESOURCES:

    MODULES CALLED:

    REVISIONS:

    DATE	   BY    	PURPOSE
    --------   -----	---------------------------------------------------

{-D}
*/


/*
    Bit mask positions
*/

#ifndef BUILDING_FOR_KERNEL
#define OURID       0x0000     /* CAN ID */
#define ACCEPTMASK  0xFF       /* accept mask */

/* Control register */
#define CR_DIS_INT	0          /* interrupts disabled */
#define CR_RR      (1<<0)      /* Reset Request */
#define CR_RIE     (1<<1)      /* Receive interrupt enable */
#define CR_TIE     (1<<2)      /* Transmit interrupt enable */
#define CR_EIE     (1<<3)      /* Error interrupt enable */
#define CR_OIE     (1<<4)      /* Overrun interrupt enable */

/* Command register */
#define CMR_TR     (1<<0)      /* Transmission request */
#define CMR_AT     (1<<1)      /* Abort Transmission */
#define CMR_RRB    (1<<2)      /* Release Receive Buffer */
#define CMR_CDO    (1<<3)      /* Clear Data Overrun */
#define CMR_SRR    (1<<4)      /* Self reception request */

/* Status register */
#define SR_RBS     (1<<0)      /* Receive Buffer Status */
#define SR_DOS     (1<<1)      /* Data Overrun Status */
#define SR_TBS     (1<<2)      /* Transmit Buffer Status */
#define SR_TCS     (1<<3)      /* Transmission Complete Status */
#define SR_RS      (1<<4)      /* Receive Status */
#define SR_TS      (1<<5)      /* Transmit Status */
#define SR_ES      (1<<6)      /* Error Status  */
#define SR_BS      (1<<7)      /* Bus Status */

#define INT_REQUEST_0  0	/* interrupt request 0 line on carrier */
#define INT_REQUEST_1  1	/* interrupt request 1 line on carrier */
#endif /* BUILDING_FOR_KERNEL */


/*
    Defined below is the memory map template for the IOS560 Board.
    This structure provides access to the various registers on the board.
*/


#ifdef SELECT_BIG_ENDIAN       /* Big ENDIAN structures */
struct map560b                 /* Memory map of board I/O space */
{
    byte unusedio1;            /* undefined */
    byte ControlRegister0;     /* Control register 0 */
    byte unusedio2;            /* undefined */
    byte ControlRegister1;     /* Control register 1 */
    byte unusedio3;            /* undefined */
    byte StatusRegister;       /* I/O Status register */
    byte unusedio4;            /* undefined */
    byte IntVectARegister;     /* I/O IntVectA register */
    byte unusedio5;            /* undefined */
    byte IntVectBRegister;     /* I/O IntVectB register */
};
struct mapmem560b              /* Memory map of board in basicCAN mode */
{
    byte unused1;              /* undefined */
    byte can_cr;               /* Control register */

    byte unused2;              /* undefined */
    byte can_cmr;              /* Command register */

    byte unused3;              /* undefined */
    byte can_sr;               /* Status register */

    byte unused4;              /* undefined */
    byte can_ir;               /* Interrupt register */

    byte unused5;              /* undefined */
    byte can_acr;              /* Acceptance Code register */

    byte unused6;              /* undefined */
    byte can_amr;              /* Acceptance Mask Register */

    byte unused7;              /* undefined */
    byte can_btr0;             /* Bus Timing register 0 */

    byte unused8;              /* undefined */
    byte can_btr1;             /* Bus Timing register 1 */

    byte unused9;              /* undefined */
    byte can_ocr;              /* Output Control register */

    byte unused10[2];          /* undefined */

    byte unused11;             /* undefined */
    byte can_txid;             /* Identifier byte 1 */

    byte unused12;             /* undefined */
    byte can_txlen;            /* Identifier byte 2 */

    struct
    {
     byte tnu0;                /* undefined */
     byte tbuf;
    } tx[8];                   /* xmit registers */

    byte unused21;             /* undefined */
    byte can_rxid;             /* Identifier byte 1 */

    byte unused22;             /* undefined */
    byte can_rxlen;            /* Identifier byte 0 */

    struct
    {
     byte rnu0;                /* undefined */
     byte rbuf;
    } rx[8];                   /* recv registers */
    
    byte unused31[2];          /* undefined */
    
    byte unused32;             /* undefined */
    byte can_cdr;              /* Clock Divider register */
};
#else                          /* Little ENDIAN structures */
struct map560b                 /* Memory map of board I/O space */
{
    byte ControlRegister0;     /* Control register 0 */
    byte unusedio1;            /* undefined */
    byte ControlRegister1;     /* Control register 1 */
    byte unusedio2;            /* undefined */
    byte StatusRegister;       /* I/O Status register */
    byte unusedio3;            /* undefined */
    byte IntVectARegister;     /* I/O IntVectA register */
    byte unusedio4;            /* undefined */
    byte IntVectBRegister;     /* I/O IntVectB register */
};
struct mapmem560b              /* Memory map of board in basicCAN mode */
{
    byte can_cr;               /* Control register */
    byte unused1;              /* undefined */

    byte can_cmr;              /* Command register */
    byte unused2;              /* undefined */

    byte can_sr;               /* Status register */
    byte unused3;              /* undefined */

    byte can_ir;               /* Interrupt register */
    byte unused4;              /* undefined */

    byte can_acr;              /* Acceptance Code register */
    byte unused5;              /* undefined */

    byte can_amr;              /* Acceptance Mask Register */
    byte unused6;              /* undefined */

    byte can_btr0;             /* Bus Timing register 0 */
    byte unused7;              /* undefined */

    byte can_btr1;             /* Bus Timing register 1 */
    byte unused8;              /* undefined */

    byte can_ocr;              /* Output Control register */
    byte unused9;              /* undefined */

    byte unused10[2];          /* undefined */

    byte can_txid;             /* Identifier byte 1 */
    byte unused11;             /* undefined */

    byte can_txlen;            /* Identifier byte 2 */
    byte unused12;             /* undefined */

    struct
    {
     byte tbuf;
     byte tnu0;                /* undefined */
    } tx[8];                   /* xmit registers */


    byte can_rxid;             /* Identifier byte 1 */
    byte unused21;             /* undefined */

    byte can_rxlen;            /* Identifier byte 0 */
    byte unused22;             /* undefined */

    struct
    {
     byte rbuf;
     byte rnu0;                /* undefined */
    } rx[8];                   /* recv registers */

    byte unused31[2];          /* undefined */
    
    byte can_cdr;              /* Clock Divider register */
};
#endif /* SELECT_BIG_ENDIAN */


#ifndef BUILDING_FOR_KERNEL
/*
    Defined below is the structure which is used to hold the
    board's xmit & receive message information.
*/

struct CAN_msg
{
  byte format;                /* 0 - STANDARD, 1- EXTENDED IDENTIFIER */
  byte type;                  /* 0 - DATA FRAME, 1 - REMOTE FRAME */
  word id;                    /* Identifier */
  byte len;                   /* Length of data field in bytes */
  byte data[8];               /* Data field */
};


/*
    Defined below is the structure which is used to hold the
    board's configuration information.
*/

struct cblk560
{
   struct map560b *brd_ptr;   /* base address of the board I/O space */
   struct mapmem560b *brd_memptr; /* base address of board memory space */
   struct CAN_msg CAN_TxMsg;  /* CAN message for sending */
   struct CAN_msg CAN_RxMsg;  /* CAN message for receiving */
   char slotLetter;           /* Slot letter */
   byte vector;               /* Interrupt Vector */
   byte acr;                  /* Acceptance Code */
   byte amr;                  /* Acceptance Mask */
   byte btr0;                 /* Bus Timing 0 */
   byte btr1;                 /* Bus Timing 1 */
   byte ocr;                  /* Output Control */
   byte cdr;                  /* Clock Divider */
   byte cr;                   /* Control */
   byte cmr;                  /* Command */
   byte controller;           /* current controller being serviced */
   byte transceiver_enable;   /* enabled/disabled */
   byte transceiver_standby;  /* standby/active */
   byte bus_clock;	     	  /* bus clock 8/32MHz */
   int nHandle;               /* handle to an open carrier board */
   int hflag;                 /* interrupt handler installed flag */
   BOOL bCarrier;             /* flag indicating a carrier is open */
   BOOL bInitialized;         /* flag indicating ready to talk to board */
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

void ios560bisr(void* pData);	/* interrupt handler for IOS560 */

