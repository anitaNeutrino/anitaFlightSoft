

/* #define SELECT_BIG_ENDIAN */	/* define this to compile for big_endian applications */

/*
{+D}
    SYSTEM:         IP470 Software

    FILE NAME:      ip470.h

    VERSION:        V1.0

    CREATION DATE:  09/03/95

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       This module contains the definitions and structures
		    used by the ip470 library.
    CALLING
	SEQUENCE:

    MODULE TYPE:    header file

    I/O RESOURCES:

    SYSTEM
	RESOURCES:

    MODULES
	CALLED:

    REVISIONS:

  DATE    BY        PURPOSE
-------  ----   ------------------------------------------------

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:

    This module contains the definitions and structures used by
    the ip470 library.
*/

/*
    DEFINITIONS:
*/

typedef unsigned char bool;

#define CLOSED  FALSE
#define OPEN    TRUE

#define BANK0   (byte)0
#define BANK1   (byte)1
#define BANK2   (byte)2

#define MAXPORTS 5
#define MAXBITS  7
#define POINTVAL 1

#define RESET 2                 /* bit position in int. enable reg. */
#define INTEN 1                 /* bit position in int. enable reg. */

/*
   INTERRUPT VECTOR
*/

#define VECTOR0      0xB0       /* interrupt vector for port 0 */

#define INT_REQUEST_0  0	/* interrupt request 0 line on carrier */
#define INT_REQUEST_1  1	/* interrupt request 1 line on carrier */


/*
    Parameter mask bit positions
*/

#define ENHANCED       1        /* enhanced mode configuration */
#define MASK           2        /* write mask register */
#define EVCONTROL      4        /* event control register */
#define DEBCLOCK       8        /* debounce clock register */
#define DEBCONTROL  0x10        /* debounce control register */
#define DEBDURATION 0x20        /* debounce duration register */
#define RESET_INTEN 0x40        /* interrupt enable register */
#define VECT        0x80        /* interrupt vector register */


/*
   DATA STRUCTURES
*/
#ifdef SELECT_BIG_ENDIAN		/* Big endian structure */
struct map470
{
  struct foo
   {
     byte nu0;                  /* common to all banks */
     byte b_select;             /* bank select register */
   } port[8];
   byte nu1[0x0F];              /* not used */
   byte ier;                    /* interrupt enable register */
   byte nu2[0x0F];              /* not used */
   byte ivr;                    /* interrupt vector register */
};
#else							/* Little endian structure */
struct map470
{
  struct
  {
    byte b_select;             /* bank select register */
    byte nu0;                  /* common to all banks */
  }
  port[8];
  byte nu1[0x0E];              /* not used */
  byte ier;                    /* interrupt enable register */
  byte nu2[0x0F];              /* not used */
  byte ivr;                    /* interrupt vector register */
};
#endif /* SELECT_BIG_ENDIAN */


/*
    Defined below is the structure which is used to hold the
    board's configuration information.
*/

struct conf_blk_470
{
    struct map470 *brd_ptr; /* base address of the I/O board */
    char slotLetter;	   /* IP slot letter */
    int nHandle;	   /* handle to an open carrier board */
    BOOL bCarrier;	   /* flag indicating a carrier is open */
    BOOL bInitialized;	   /* flag indicating ready to talk to board */	
    word param;             /* parameter mask for configuring board */
    byte e_mode;            /* enhanced operation flag */
    byte mask_reg;          /* output port mask register */
    byte ev_control[2];     /* event control registers */
    byte deb_control;       /* debounce control register */
    byte deb_duration[2];   /* debounce duration registers */
    byte deb_clock;         /* debounce clock select register */
    byte enable;            /* interrupt enable register */
    byte vector;            /* interrupt vector register */
    byte id_prom[32];       /* board ID Prom */
    byte event_status[6];   /* interrupt pending per channel */
};


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



/* Declare functions called */

byte bank_select(); /* select register banks on the IP470 board */
void scfg470();     /* routine to set up the configuration param. block */
void psts470();     /* routine which calls the Read Status Command */
void cnfg470();     /* routine to configure the board */
void rsts470();     /* routine to read status information */
void rmid470();     /* read module I.D. */
long rpnt470();     /* routine to read a input point */
long rprt470();     /* routine to read the input port */
long wpnt470();     /* routine to write to a output point */
long wprt470();     /* routine to write to the output port */
long get_param();   /* input a parameter */

