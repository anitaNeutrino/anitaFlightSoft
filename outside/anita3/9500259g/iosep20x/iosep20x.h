


/*
{+D}
    SYSTEM:             Library Software

    MODULE NAME:        IOSEP20x.h

    VERSION:            A

    CREATION DATE:      04/01/09

    CODED BY:           FM

    ABSTRACT:           This "inlcude" file contains all the major data
                        structures and memory map used in the software

    CALLING SEQUENCE:

    MODULE TYPE:        include file

    I/O RESOURCES:

    SYSTEM RESOURCES:

    MODULES CALLED:

    REVISIONS:

      DATE      BY      PURPOSE
    --------   -----    ---------------------------------------------------

{-D}
*/



/* ///////////////////////////////////////////////////////////////////////// */
/* Select board array type by using the correct defines for your hardware.   */
/* Default is IOSEP20x.                                                      */
/* ///////////////////////////////////////////////////////////////////////// */

/*
	IOSEP20x types
*/

#define IOSEPARRAY "./utility/ninek528c.h"			/* contains ninek528c.h data array See HFileGenerator.c */





/*
    Parameter mask bit positions
*/
#ifndef BUILDING_FOR_KERNEL
#define SEL_MODEL      1        /* selelct model */
#define INT_STATUS     2        /* interrupt status registers */
#define INT_ENAB       4        /* interrupt enable registers */
#define INT_POLARITY   8        /* interrupt polarity registers */
#define INT_TYPE     0x10       /* interrupt type registers */
#define VECT         0x20       /* interrupt vector */
#define DIR          0x40       /* direction */
#define RESET        0x80       /* reset */

#define INT_REQUEST_0  0	/* interrupt request 0 line on carrier */
#define INT_REQUEST_1  1	/* interrupt request 1 line on carrier */
#endif /* BUILDING_FOR_KERNEL */


/*
    Defined below is the memory map template for the board.
    This structure provides access to the various registers on the board.
*/

struct mapep20x            /* Memory map of the I/O board */
{
    word control;          /* control register */
    word io_port[3];       /* input/output registers */
    word dir_reg;          /* Direction Register */
    word en_reg;           /* Interrupt Enable Register */
    word type_reg;         /* Interrupt Type Select Register */
    word sts_reg;          /* Status Register */
    word pol_reg;          /* Interrupt Input Polarity Register */
    word int_vect;         /* Interrupt vector */
    word mem_data;         /* memory data register */
    word mem_addr;         /* memory address register */
    word clk_shft[2];      /* clock shift high/low registers */
    word length;           /* Clock Gen program word length register */
    word trigger;          /* Clock Gen trigger register */
};

/*
    Defined below is the structure which is used to hold the
    board's status information.
*/

struct sblkep20x
{
    byte id_prom[32];   /* board ID Prom */
    byte model;         /* model selected */
    word direction;     /* direction registers */
    byte enable;        /* interrupt enable registers */
    byte type;          /* interrupt type select registers */
    byte int_status;    /* interrupt status registers */
    byte polarity;      /* interrupt input polarity registers */
    byte vector;        /* interrupt vector register */
};




/*
    Defined below is the structure which is used to hold the
    board's configuration information.
*/

struct cblkep20x
{
    struct sblkep20x *sblk_ptr; /* pointer to status block structure */
    struct mapep20x *brd_ptr;   /* base address of the I/O board */
    char slotLetter;            /* IOS slot letter */
    int nHandle;                /* handle to an open carrier board */
    BOOL bCarrier;              /* flag indicating a carrier is open */
    BOOL bInitialized;          /* flag indicating ready to talk to board */	
    word param;                 /* parameter mask for configuring board */
    word model;                 /* model selected */
    word direction;             /* direction registers */
    word enable;                /* interrupt enable registers */
    word type;                  /* interrupt type select registers */
    word int_status;            /* interrupt status registers */
    word polarity;              /* interrupt input polarity registers */
    byte vector;                /* interrupt vector register */
    byte reset;                 /* reset flag */
};






/* Declare functions called */


int PLDConfigep20x( struct cblkep20x *);/* configure PLD */
int ios_mem_ram_test(struct cblkep20x *);
void scfgep20x(struct cblkep20x *);		/* routine to set up the configuration param. block */
void pstsep20x(struct cblkep20x *);		/* routine which calls the Read Status Command */
void cnfgep20x(struct cblkep20x *);		/* routine to configure the board */
void rstsep20x(struct cblkep20x *);		/* routine to read status information */
long rpntep20x(struct cblkep20x *,
		unsigned port, unsigned point);	/* routine to read a input point */
long rprtep20x(struct cblkep20x *,
		unsigned port);					/* routine to read the input port */
long wpntep20x(struct cblkep20x *,
		unsigned port,					/* the I/O port number */
		unsigned point,					/* the I/O point of a port */
		unsigned value);				/* routine to write to a output point */
long wprtep20x(struct cblkep20x *,
		unsigned port, unsigned value);	/* routine to write to the output port */
long get_param(void);   /* input a parameter */

/* ISR */

void iosep20xisr(void* pData);	/* interrupt handler for ep20x */

