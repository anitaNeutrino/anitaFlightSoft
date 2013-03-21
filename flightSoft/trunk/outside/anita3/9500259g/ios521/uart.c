
#include "../ioscarrier/ioscarrier.h"
#include "ios521.h"
#include "portio.h"		/* serial port include file */

/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    uart.c

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Data structure initialization

    CALLING
	SEQUENCE:

    MODULE TYPE:

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
*/

/*
	Interrupt data structure
*/

struct ids intlist[MAXPORTS] =  /* initialize structure members */
  {
    { IRXRDY /*| IMODEMSTAT */, INITIALIZED },    /* port 1 */
    { IRXRDY /*| IMODEMSTAT */, INITIALIZED },    /* port 2 */
    { IRXRDY /*| IMODEMSTAT */, INITIALIZED },    /* port 3 */
    { IRXRDY /*| IMODEMSTAT */, INITIALIZED },    /* port 4 */
    { IRXRDY /*| IMODEMSTAT */, INITIALIZED },    /* port 5 */
    { IRXRDY /*| IMODEMSTAT */, INITIALIZED },    /* port 6 */
    { IRXRDY /*| IMODEMSTAT */, INITIALIZED },    /* port 7 */
    { IRXRDY /*| IMODEMSTAT */, INITIALIZED },    /* port 8 */
    { INITIALIZED, INITIALIZED },            /* port 9 not present */
    { INITIALIZED, INITIALIZED },            /* port 10 not present */
    { INITIALIZED, INITIALIZED },            /* port 11 not present */
    { INITIALIZED, INITIALIZED },            /* port 12 not present */
    { INITIALIZED, INITIALIZED },            /* port 13 not present */
    { INITIALIZED, INITIALIZED },            /* port 14 not present */
    { INITIALIZED, INITIALIZED },            /* port 15 not present */
    { INITIALIZED, INITIALIZED },            /* port 16 not present */
  };


/*
	Return type declarations
*/

void xmit();
byte rcv(), rcvstat(), xmitstat(), stat232(), istat232(), irstat(), iread();
byte input_byte( int nHandle, byte *p);
void output_byte(int nHandle, byte *p, byte v);


/*
	INITIALIZATIONS FOR COMMUNICATIONS PORT 0
*/

struct baud baud0 =
 {
   BAUD_LOW,                    /* offset of UART lsb baud register */
   BAUD_HIGH,                   /* offset of UART msb baud register */
   {
     {BH50,	  BL50},   {BH75,	   BL75},    {BH150,  BL150},  {BH300,  BL300},
     {BH600,  BL600},  {BH1200,  BL1200},  {BH2400, BL2400}, {BH3600, BL3600},
     {BH4800, BL4800}, {BH7200,  BL7200},  {BH9600, BL9600}, {BH14K4, BL14K4},
     {BH19K2, BL19K2}, {BH28K8,  BL28K8},  {BH38K4, BL38K4}, {BH57K6, BL57K6},
     {BH76K8, BL76K8}, {BH153K6, BL153K6}, {BH230K, BL230K}, {BH921K6, BL921K6}
    },
   INITIALIZED,                 /* position of current baud divisor in table */
   B9600,                       /* initial baud at 9600 */
   BAUDLO,                      /* RAM register for low byte */
   BAUDHI                       /* RAM register for high byte */
 };


struct ramregbits parity0 =
{
   PARITYMASK,                  /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   PNONE,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { NONE_MASK, ODD_MASK, EVEN_MASK, -1, -1 } /* masks */
};


struct ramregbits stops0 =
{
   STOPMASK,                    /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   STOP1,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { ONE_MASK, ONE5_MASK, TWO_MASK, -1, -1 } /* masks */
};


struct ramregbits dlen0 =
{
   DMASK,                       /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   DL8,                         /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { D5_MASK, D6_MASK, D7_MASK, D8_MASK, -1 } /* masks */
};


struct ramregbits fifoen0 =
{
   FIFO_MASK,                   /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   FIFO_ON,                     /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { FIFO_DISABLE, FIFO_ENABLE, -1, -1, -1 } /* masks */
};


struct ramregbits fifotrig0 =
{
   TRIGGER_MASK,                /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   BYTE_1,                      /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { BYTE_1MASK, BYTE_4MASK, BYTE_8MASK, BYTE_14MASK, -1 } /* masks */
};


struct ramregbits intvect0 =
{
   INITIALIZED,                 /* mask to reset bits in this register */
   IVECT,                       /* register number */
   INITIALIZED,                 /* array position of current setting */
   INITIALIZED,                 /* array position to use in initialization */
   IVECTOR,                     /* interrupt vector register offset from base */
   { VECTOR0, -1, -1, -1, -1 }  /* masks */
};


byte ramreg0[ROWS][2]=
    {
    /* ram  saved */
      {	0,    0 }, 		/* 0 = baud low byte */
      {	0,    0 },		/* 1 = baud high byte */
      {	0,    0 },		/* 2 = interrupt enable */
      {	0,    0 },		/* 3 = data format */
      {	0,    0 },		/* 4 = RS232 output control */
      {	0,    0 },		/* 5 = FIFO control */
      {	0,    0 }		/* 6 = interrupt vector */
    };


/*
   RS-232 STRUCTURES
*/

struct out232 rts0 =            /* RTS */
{
   RTSMASK,                     /* mask to reset RTS */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 dtr0 =            /* DTR */
{
   DTRMASK,                     /* mask to reset DTR */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp1out0 =         /* GP01 */
{
   GP01MASK,                    /* mask to reset GP01 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp2out0 =         /* GP02 */
{
   GP02MASK,                    /* mask to reset GP02 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 breakb0 =         /* BREAK */
{
   BRKMASK,                     /* mask to reset break */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   LINE_CONT                    /* LCR register offset from base */
};


struct out232 loop0 =           /* loopback */
{
   LOOP_MASK,                   /* mask to reset loopback */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   LOOP_OFF,                    /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};



struct in232 cts0 =             /* CTS */
{
   CTSMASK,                     /* mask for CTS */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 dsr0 =             /* DSR */
{
   DSRMASK,                     /* mask for DSR */
   INITIALIZED,                 /* current setting */
   STAT_232,                    /* MSR register offset from base */
};

struct in232 dcd0 =             /* DCD */
{
   DCDMASK,                     /* mask for DCD */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 ri0 =              /* RI */
{
   RIMASK,                      /* mask for RI */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};


IO port0 =                      /* declare IO module 0 */
  {
    INITIALIZED,                /* base address of serial port 0 */
    TX_RX_REG,                  /* offset to UART data port */
    LINE_STAT,                  /* offset to UART status port */
    RCV_MASK,                   /* RxRDY mask */
    XMIT_MASK,                  /* TxRDY mask */
    input_byte,                 /* routine to input byte from UART */
    rcvstat,                    /* pointer to serial receiver status routine */
    rcvstat,                    /* saved pointer to serial receiver status routine */
    rcv,                        /* pointer to serial fetch routine */
    rcv,                        /* saved pointer to serial fetch routine */
	(void*)output_byte,			/* routine to output byte to UART */
    xmitstat,                   /* pointer to serial xmitter status routine */
    xmit,                       /* pointer to serial xmit routine */
    stat232,                    /* pointer to serial RS232 status routine */
    stat232,                    /* saved pointer to serial RS232 status routine */
    ramreg0,                    /* pointer to RAM register array */
    ROWS,                       /* number of rows in RAM register array */
    &intvect0,                  /* interrupt vector structure */
    &fifoen0,                   /* fifo enable structure */
    &fifotrig0,                 /* fifo trigger structure */
    &parity0,                   /* parity structure */
    &stops0,                    /* stop bit structure */
    &dlen0,                     /* data length structure */
    &baud0,                     /* baud rate structure */
    &rts0,                      /* RS-232 RTS output structure */
    &dtr0,                      /* RS-232 DTR output structure */
    &gp1out0,                   /* RS-232 GP01 output structure */
    &gp2out0,                   /* RS-232 GP02 output structure */
    &breakb0,                   /* break bit */
    &loop0,                     /* loopback test structure */
    &cts0,                      /* RS-232 CTS input structure */
    &dsr0,                      /* RS-232 DSR input structure */
    &dcd0,                      /* RS-232 DCD input structure */
    &ri0,                       /* RS-232 RI input structure */
    INITIALIZED,                /* port number supplied during open */
    irstat,                     /* pointer to serial receiver status routine */
    iread,                      /* pointer to serial read routine */
    istat232,                   /* pointer to serial RS232 status routine */
    FALSE,                      /* interrupts ON/OFF flag */
};


/*
	INITIALIZATIONS FOR COMMUNICATIONS PORT 1
*/

struct baud baud1 =
 {
   BAUD_LOW,                    /* offset of UART lsb baud register */
   BAUD_HIGH,                   /* offset of UART msb baud register */
   {
     {BH50,	  BL50},   {BH75,	   BL75},    {BH150,  BL150},  {BH300,  BL300},
     {BH600,  BL600},  {BH1200,  BL1200},  {BH2400, BL2400}, {BH3600, BL3600},
     {BH4800, BL4800}, {BH7200,  BL7200},  {BH9600, BL9600}, {BH14K4, BL14K4},
     {BH19K2, BL19K2}, {BH28K8,  BL28K8},  {BH38K4, BL38K4}, {BH57K6, BL57K6},
     {BH76K8, BL76K8}, {BH153K6, BL153K6}, {BH230K, BL230K}, {BH921K6, BL921K6}
    },
   INITIALIZED,                 /* position of current baud divisor in table */
   B9600,                       /* initial baud at 9600 */
   BAUDLO,                      /* RAM register for low byte */
   BAUDHI                       /* RAM register for high byte */
 };


struct ramregbits parity1 =
{
   PARITYMASK,                  /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   PNONE,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { NONE_MASK, ODD_MASK, EVEN_MASK, -1, -1 } /* masks */
};


struct ramregbits stops1 =
{
   STOPMASK,                    /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   STOP1,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { ONE_MASK, ONE5_MASK, TWO_MASK, -1, -1 } /* masks */
};


struct ramregbits dlen1 =
{
   DMASK,                       /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   DL8,                         /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { D5_MASK, D6_MASK, D7_MASK, D8_MASK, -1 } /* masks */
};


struct ramregbits fifoen1 =
{
   FIFO_MASK,                   /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   FIFO_ON,                     /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { FIFO_DISABLE, FIFO_ENABLE, -1, -1, -1 } /* masks */
};


struct ramregbits fifotrig1 =
{
   TRIGGER_MASK,                /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   BYTE_1,                      /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { BYTE_1MASK, BYTE_4MASK, BYTE_8MASK, BYTE_14MASK, -1 } /* masks */
};


struct ramregbits intvect1 =
{
   INITIALIZED,                 /* mask to reset bits in this register */
   IVECT,                       /* register number */
   INITIALIZED,                 /* array position of current setting */
   INITIALIZED,                 /* array position to use in initialization */
   IVECTOR,                     /* interrupt vector register offset from base */
   { VECTOR1, -1, -1, -1, -1 }  /* masks */
};


byte ramreg1[ROWS][2]=
    {
    /* ram  saved */
      {	0,    0 }, 		/* 0 = baud low byte */
      {	0,    0 },		/* 1 = baud high byte */
      {	0,    0 },		/* 2 = interrupt enable */
      {	0,    0 },		/* 3 = data format */
      {	0,    0 },		/* 4 = RS232 output control */
      {	0,    0 },		/* 5 = FIFO control */
      {	0,    0 }		/* 6 = interrupt vector */
    };


/*
   RS-232 STRUCTURES
*/

struct out232 rts1 =            /* RTS */
{
   RTSMASK,                     /* mask to reset RTS */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 dtr1 =            /* DTR */
{
   DTRMASK,                     /* mask to reset DTR */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp1out1 =         /* GP01 */
{
   GP01MASK,                    /* mask to reset GP01 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp2out1 =         /* GP02 */
{
   GP02MASK,                    /* mask to reset GP02 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 breakb1 =         /* BREAK */
{
   BRKMASK,                     /* mask to reset break */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   LINE_CONT                    /* LCR register offset from base */
};


struct out232 loop1 =           /* loopback */
{
   LOOP_MASK,                   /* mask to reset loopback */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   LOOP_OFF,                    /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};



struct in232 cts1 =             /* CTS */
{
   CTSMASK,                     /* mask for CTS */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 dsr1 =             /* DSR */
{
   DSRMASK,                     /* mask for DSR */ 
   INITIALIZED,                 /* current setting */
   STAT_232,                    /* MSR register offset from base */
};

struct in232 dcd1 =             /* DCD */
{
   DCDMASK,                     /* mask for DCD */ 
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 ri1 =              /* RI */
{
   RIMASK,                      /* mask for RI */ 
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};


IO port1 =                      /* declare IO module 1 */
  {
    INITIALIZED,                /* base address of serial port 1 */
    TX_RX_REG,                  /* offset to UART data port */
    LINE_STAT,                  /* offset to UART status port */
    RCV_MASK,                   /* RxRDY mask */
    XMIT_MASK,                  /* TxRDY mask */
    input_byte,                 /* routine to input byte from UART */
    rcvstat,                    /* pointer to serial receiver status routine */
    rcvstat,                    /* saved pointer to serial receiver status routine */
    rcv,                        /* pointer to serial fetch routine */
    rcv,                        /* saved pointer to serial fetch routine */
	(void*)output_byte,			/* routine to output byte to UART */
    xmitstat,                   /* pointer to serial xmitter status routine */
    xmit,                       /* pointer to serial xmit routine */
    stat232,                    /* pointer to serial RS232 status routine */
    stat232,                    /* saved pointer to serial RS232 status routine */
    ramreg1,                    /* pointer to RAM register array */
    ROWS,                       /* number of rows in RAM register array */
    &intvect1,                  /* interrupt vector structure */
    &fifoen1,                   /* fifo enable structure */
    &fifotrig1,                 /* fifo trigger structure */
    &parity1,                   /* parity structure */
    &stops1,                    /* stop bit structure */
    &dlen1,                     /* data length structure */
    &baud1,                     /* baud rate structure */
    &rts1,                      /* RS-232 RTS output structure */
    &dtr1,                      /* RS-232 DTR output structure */
    &gp1out1,                   /* RS-232 GP01 output structure */
    &gp2out1,                   /* RS-232 GP02 output structure */
    &breakb1,                   /* break bit */
    &loop1,                     /* loopback test structure */
    &cts1,                      /* RS-232 CTS input structure */
    &dsr1,                      /* RS-232 DSR input structure */
    &dcd1,                      /* RS-232 DCD input structure */
    &ri1,                       /* RS-232 RI input structure */
    INITIALIZED,                /* port number supplied during open */
    irstat,                     /* pointer to serial receiver status routine */
    iread,                      /* pointer to serial read routine */
    istat232,                   /* pointer to serial RS232 status routine */
    FALSE,                      /* interrupts ON/OFF flag */
};


/*
	INITIALIZATIONS FOR COMMUNICATIONS PORT 2
*/

struct baud baud2 =
 {
   BAUD_LOW,                    /* offset of UART lsb baud register */
   BAUD_HIGH,                   /* offset of UART msb baud register */
   {
     {BH50,	  BL50},   {BH75,	   BL75},    {BH150,  BL150},  {BH300,  BL300},
     {BH600,  BL600},  {BH1200,  BL1200},  {BH2400, BL2400}, {BH3600, BL3600},
     {BH4800, BL4800}, {BH7200,  BL7200},  {BH9600, BL9600}, {BH14K4, BL14K4},
     {BH19K2, BL19K2}, {BH28K8,  BL28K8},  {BH38K4, BL38K4}, {BH57K6, BL57K6},
     {BH76K8, BL76K8}, {BH153K6, BL153K6}, {BH230K, BL230K}, {BH921K6, BL921K6}
    },
   INITIALIZED,                 /* position of current baud divisor in table */
   B9600,                       /* initial baud at 9600 */
   BAUDLO,                      /* RAM register for low byte */
   BAUDHI                       /* RAM register for high byte */
 };


struct ramregbits parity2 =
{
   PARITYMASK,                  /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   PNONE,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { NONE_MASK, ODD_MASK, EVEN_MASK, -1, -1 } /* masks */
};


struct ramregbits stops2 =
{
   STOPMASK,                    /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   STOP1,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { ONE_MASK, ONE5_MASK, TWO_MASK, -1, -1 } /* masks */
};


struct ramregbits dlen2 =
{
   DMASK,                       /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   DL8,                         /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { D5_MASK, D6_MASK, D7_MASK, D8_MASK, -1 } /* masks */
};


struct ramregbits fifoen2 =
{
   FIFO_MASK,                   /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   FIFO_ON,                     /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { FIFO_DISABLE, FIFO_ENABLE, -1, -1, -1 } /* masks */
};


struct ramregbits fifotrig2 =
{
   TRIGGER_MASK,                /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   BYTE_1,                      /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { BYTE_1MASK, BYTE_4MASK, BYTE_8MASK, BYTE_14MASK, -1 } /* masks */
};


struct ramregbits intvect2 =
{
   INITIALIZED,                 /* mask to reset bits in this register */
   IVECT,                       /* register number */
   INITIALIZED,                 /* array position of current setting */
   INITIALIZED,                 /* array position to use in initialization */
   IVECTOR,                     /* interrupt vector register offset from base */
   { VECTOR2, -1, -1, -1, -1 }  /* masks */
};


byte ramreg2[ROWS][2]=
    {
    /* ram  saved */
      {	0,    0 }, 		/* 0 = baud low byte */
      {	0,    0 },		/* 1 = baud high byte */
      {	0,    0 },		/* 2 = interrupt enable */
      {	0,    0 },		/* 3 = data format */
      {	0,    0 },		/* 4 = RS232 output control */
      {	0,    0 },		/* 5 = FIFO control */
      {	0,    0 }		/* 6 = interrupt vector */
    };


/*
   RS-232 STRUCTURES
*/

struct out232 rts2 =            /* RTS */
{
   RTSMASK,                     /* mask to reset RTS */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 dtr2 =            /* DTR */
{
   DTRMASK,                     /* mask to reset DTR */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp1out2 =         /* GP01 */
{
   GP01MASK,                    /* mask to reset GP01 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp2out2 =         /* GP02 */
{
   GP02MASK,                    /* mask to reset GP02 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 breakb2 =         /* BREAK */
{
   BRKMASK,                     /* mask to reset break */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   LINE_CONT                    /* LCR register offset from base */
};


struct out232 loop2 =           /* loopback */
{
   LOOP_MASK,                   /* mask to reset loopback */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   LOOP_OFF,                    /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};



struct in232 cts2 =             /* CTS */
{
   CTSMASK,                     /* mask for CTS */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 dsr2 =             /* DSR */
{
   DSRMASK,                     /* mask for DSR */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 dcd2 =             /* DCD */
{
   DCDMASK,                     /* mask for DCD */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 ri2 =              /* RI */
{
   RIMASK,                      /* mask for RI */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};


IO port2 =                      /* declare IO module 2 */
  {
    INITIALIZED,                /* base address of serial port 2 */
    TX_RX_REG,                  /* offset to UART data port */
    LINE_STAT,                  /* offset to UART status port */
    RCV_MASK,                   /* RxRDY mask */
    XMIT_MASK,                  /* TxRDY mask */
    input_byte,                 /* routine to input byte from UART */
    rcvstat,                    /* pointer to serial receiver status routine */
    rcvstat,                    /* saved pointer to serial receiver status routine */
    rcv,                        /* pointer to serial fetch routine */
    rcv,                        /* saved pointer to serial fetch routine */
	(void*)output_byte,			/* routine to output byte to UART */
    xmitstat,                   /* pointer to serial xmitter status routine */
    xmit,                       /* pointer to serial xmit routine */
    stat232,                    /* pointer to serial RS232 status routine */
    stat232,                    /* saved pointer to serial RS232 status routine */
    ramreg2,                    /* pointer to RAM register array */
    ROWS,                       /* number of rows in RAM register array */
    &intvect2,                  /* interrupt vector structure */
    &fifoen2,                   /* fifo enable structure */
    &fifotrig2,                 /* fifo trigger structure */
    &parity2,                   /* parity structure */
    &stops2,                    /* stop bit structure */
    &dlen2,                     /* data length structure */
    &baud2,                     /* baud rate structure */
    &rts2,                      /* RS-232 RTS output structure */
    &dtr2,                      /* RS-232 DTR output structure */
    &gp1out2,                   /* RS-232 GP01 output structure */
    &gp2out2,                   /* RS-232 GP02 output structure */
    &breakb2,                   /* break bit */
    &loop2,                     /* loopback test structure */
    &cts2,                      /* RS-232 CTS input structure */
    &dsr2,                      /* RS-232 DSR input structure */
    &dcd2,                      /* RS-232 DCD input structure */
    &ri2,                       /* RS-232 RI input structure */
    INITIALIZED,                /* port number supplied during open */
    irstat,                     /* pointer to serial receiver status routine */
    iread,                      /* pointer to serial read routine */
    istat232,                   /* pointer to serial RS232 status routine */
    FALSE,                      /* interrupts ON/OFF flag */
};


/*
	INITIALIZATIONS FOR COMMUNICATIONS PORT 3
*/

struct baud baud3 =
 {
   BAUD_LOW,                    /* offset of UART lsb baud register */
   BAUD_HIGH,                   /* offset of UART msb baud register */
   {
     {BH50,	  BL50},   {BH75,	   BL75},    {BH150,  BL150},  {BH300,  BL300},
     {BH600,  BL600},  {BH1200,  BL1200},  {BH2400, BL2400}, {BH3600, BL3600},
     {BH4800, BL4800}, {BH7200,  BL7200},  {BH9600, BL9600}, {BH14K4, BL14K4},
     {BH19K2, BL19K2}, {BH28K8,  BL28K8},  {BH38K4, BL38K4}, {BH57K6, BL57K6},
     {BH76K8, BL76K8}, {BH153K6, BL153K6}, {BH230K, BL230K}, {BH921K6, BL921K6}
    },
   INITIALIZED,                 /* position of current baud divisor in table */
   B9600,                       /* initial baud at 9600 */
   BAUDLO,                      /* RAM register for low byte */
   BAUDHI                       /* RAM register for high byte */
 };


struct ramregbits parity3 =
{
   PARITYMASK,                  /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   PNONE,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { NONE_MASK, ODD_MASK, EVEN_MASK, -1, -1 } /* masks */
};


struct ramregbits stops3 =
{
   STOPMASK,                    /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   STOP1,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { ONE_MASK, ONE5_MASK, TWO_MASK, -1, -1 } /* masks */
};


struct ramregbits dlen3 =
{
   DMASK,                       /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   DL8,                         /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { D5_MASK, D6_MASK, D7_MASK, D8_MASK, -1 } /* masks */
};


struct ramregbits fifoen3 =
{
   FIFO_MASK,                   /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   FIFO_ON,                     /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { FIFO_DISABLE, FIFO_ENABLE, -1, -1, -1 } /* masks */
};


struct ramregbits fifotrig3 =
{
   TRIGGER_MASK,                /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   BYTE_1,                      /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { BYTE_1MASK, BYTE_4MASK, BYTE_8MASK, BYTE_14MASK, -1 } /* masks */
};


struct ramregbits intvect3 =
{
   INITIALIZED,                 /* mask to reset bits in this register */
   IVECT,                       /* register number */
   INITIALIZED,                 /* array position of current setting */
   INITIALIZED,                 /* array position to use in initialization */
   IVECTOR,                     /* interrupt vector register offset from base */
   { VECTOR3, -1, -1, -1, -1 }  /* masks */
};


byte ramreg3[ROWS][2]=
    {
    /* ram  saved */
      {	0,    0 }, 		/* 0 = baud low byte */
      {	0,    0 },		/* 1 = baud high byte */
      {	0,    0 },		/* 2 = interrupt enable */
      {	0,    0 },		/* 3 = data format */
      {	0,    0 },		/* 4 = RS232 output control */
      {	0,    0 },		/* 5 = FIFO control */
      {	0,    0 }		/* 6 = interrupt vector */
    };


/*
   RS-232 STRUCTURES
*/

struct out232 rts3 =            /* RTS */
{
   RTSMASK,                     /* mask to reset RTS */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 dtr3 =            /* DTR */
{
   DTRMASK,                     /* mask to reset DTR */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp1out3 =         /* GP01 */
{
   GP01MASK,                    /* mask to reset GP01 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp2out3 =         /* GP02 */
{
   GP02MASK,                    /* mask to reset GP02 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 breakb3 =         /* BREAK */
{
   BRKMASK,                     /* mask to reset break */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   LINE_CONT                    /* LCR register offset from base */
};


struct out232 loop3 =           /* loopback */
{
   LOOP_MASK,                   /* mask to reset loopback */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   LOOP_OFF,                    /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};



struct in232 cts3 =             /* CTS */
{
   CTSMASK,                     /* mask for CTS */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 dsr3 =             /* DSR */
{
   DSRMASK,                     /* mask for DSR */
   INITIALIZED,                 /* current setting */
   STAT_232,                    /* MSR register offset from base */
};

struct in232 dcd3 =             /* DCD */
{
   DCDMASK,                     /* mask for DCD */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 ri3 =              /* RI */
{
   RIMASK,                      /* mask for RI */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};


IO port3 =                      /* declare IO module 3 */
  {
    INITIALIZED,                /* base address of serial port 3 */
    TX_RX_REG,                  /* offset to UART data port */
    LINE_STAT,                  /* offset to UART status port */
    RCV_MASK,                   /* RxRDY mask */
    XMIT_MASK,                  /* TxRDY mask */
    input_byte,                 /* routine to input byte from UART */
    rcvstat,                    /* pointer to serial receiver status routine */
    rcvstat,                    /* saved pointer to serial receiver status routine */
    rcv,                        /* pointer to serial fetch routine */
    rcv,                        /* saved pointer to serial fetch routine */
	(void*)output_byte,			/* routine to output byte to UART */
    xmitstat,                   /* pointer to serial xmitter status routine */
    xmit,                       /* pointer to serial xmit routine */
    stat232,                    /* pointer to serial RS232 status routine */
    stat232,                    /* saved pointer to serial RS232 status routine */
    ramreg3,                    /* pointer to RAM register array */
    ROWS,                       /* number of rows in RAM register array */
    &intvect3,                  /* interrupt vector structure */
    &fifoen3,                   /* fifo enable structure */
    &fifotrig3,                 /* fifo trigger structure */
    &parity3,                   /* parity structure */
    &stops3,                    /* stop bit structure */
    &dlen3,                     /* data length structure */
    &baud3,                     /* baud rate structure */
    &rts3,                      /* RS-232 RTS output structure */
    &dtr3,                      /* RS-232 DTR output structure */
    &gp1out3,                   /* RS-232 GP01 output structure */
    &gp2out3,                   /* RS-232 GP02 output structure */
    &breakb3,                   /* break bit */
    &loop3,                     /* loopback test structure */
    &cts3,                      /* RS-232 CTS input structure */
    &dsr3,                      /* RS-232 DSR input structure */
    &dcd3,                      /* RS-232 DCD input structure */
    &ri3,                       /* RS-232 RI input structure */
    INITIALIZED,                /* port number supplied during open */
    irstat,                     /* pointer to serial receiver status routine */
    iread,                      /* pointer to serial read routine */
    istat232,                   /* pointer to serial RS232 status routine */
    FALSE,                      /* interrupts ON/OFF flag */
};


/*
	INITIALIZATIONS FOR COMMUNICATIONS PORT 4
*/

struct baud baud4 =
 {
   BAUD_LOW,                    /* offset of UART lsb baud register */
   BAUD_HIGH,                   /* offset of UART msb baud register */
   {
     {BH50,	  BL50},   {BH75,	   BL75},    {BH150,  BL150},  {BH300,  BL300},
     {BH600,  BL600},  {BH1200,  BL1200},  {BH2400, BL2400}, {BH3600, BL3600},
     {BH4800, BL4800}, {BH7200,  BL7200},  {BH9600, BL9600}, {BH14K4, BL14K4},
     {BH19K2, BL19K2}, {BH28K8,  BL28K8},  {BH38K4, BL38K4}, {BH57K6, BL57K6},
     {BH76K8, BL76K8}, {BH153K6, BL153K6}, {BH230K, BL230K}, {BH921K6, BL921K6}
    },
   INITIALIZED,                 /* position of current baud divisor in table */
   B9600,                       /* initial baud at 9600 */
   BAUDLO,                      /* RAM register for low byte */
   BAUDHI                       /* RAM register for high byte */
 };


struct ramregbits parity4 =
{
   PARITYMASK,                  /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   PNONE,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { NONE_MASK, ODD_MASK, EVEN_MASK, -1, -1 } /* masks */
};


struct ramregbits stops4 =
{
   STOPMASK,                    /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   STOP1,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { ONE_MASK, ONE5_MASK, TWO_MASK, -1, -1 } /* masks */
};


struct ramregbits dlen4 =
{
   DMASK,                       /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   DL8,                         /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { D5_MASK, D6_MASK, D7_MASK, D8_MASK, -1 } /* masks */
};


struct ramregbits fifoen4 =
{
   FIFO_MASK,                   /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   FIFO_ON,                     /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { FIFO_DISABLE, FIFO_ENABLE, -1, -1, -1 } /* masks */
};


struct ramregbits fifotrig4 =
{
   TRIGGER_MASK,                /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   BYTE_1,                      /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { BYTE_1MASK, BYTE_4MASK, BYTE_8MASK, BYTE_14MASK, -1 } /* masks */
};


struct ramregbits intvect4 =
{
   INITIALIZED,                 /* mask to reset bits in this register */
   IVECT,                       /* register number */
   INITIALIZED,                 /* array position of current setting */
   INITIALIZED,                 /* array position to use in initialization */
   IVECTOR,                     /* interrupt vector register offset from base */
   { VECTOR4, -1, -1, -1, -1 }  /* masks */
};


byte ramreg4[ROWS][2]=
    {
    /* ram  saved */
      {	0,    0 }, 		/* 0 = baud low byte */
      {	0,    0 },		/* 1 = baud high byte */
      {	0,    0 },		/* 2 = interrupt enable */
      {	0,    0 },		/* 3 = data format */
      {	0,    0 },		/* 4 = RS232 output control */
      {	0,    0 },		/* 5 = FIFO control */
      {	0,    0 }		/* 6 = interrupt vector */
    };


/*
   RS-232 STRUCTURES
*/

struct out232 rts4 =            /* RTS */
{
   RTSMASK,                     /* mask to reset RTS */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 dtr4 =            /* DTR */
{
   DTRMASK,                     /* mask to reset DTR */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp1out4 =         /* GP01 */
{
   GP01MASK,                    /* mask to reset GP01 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp2out4 =         /* GP02 */
{
   GP02MASK,                    /* mask to reset GP02 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 breakb4 =         /* BREAK */
{
   BRKMASK,                     /* mask to reset break */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   LINE_CONT                    /* LCR register offset from base */
};


struct out232 loop4 =           /* loopback */
{
   LOOP_MASK,                   /* mask to reset loopback */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   LOOP_OFF,                    /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};



struct in232 cts4 =             /* CTS */
{
   CTSMASK,                     /* mask for CTS */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 dsr4 =             /* DSR */
{
   DSRMASK,                     /* mask for DSR */
   INITIALIZED,                 /* current setting */
   STAT_232,                    /* MSR register offset from base */
};

struct in232 dcd4 =             /* DCD */
{
   DCDMASK,                     /* mask for DCD */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 ri4 =              /* RI */
{
   RIMASK,                      /* mask for RI */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};


IO port4 =                      /* declare IO module 4 */
  {
    INITIALIZED,                /* base address of serial port 4 */
    TX_RX_REG,                  /* offset to UART data port */
    LINE_STAT,                  /* offset to UART status port */
    RCV_MASK,                   /* RxRDY mask */
    XMIT_MASK,                  /* TxRDY mask */
    input_byte,                 /* routine to input byte from UART */
    rcvstat,                    /* pointer to serial receiver status routine */
    rcvstat,                    /* saved pointer to serial receiver status routine */
    rcv,                        /* pointer to serial fetch routine */
    rcv,                        /* saved pointer to serial fetch routine */
	(void*)output_byte,			/* routine to output byte to UART */
    xmitstat,                   /* pointer to serial xmitter status routine */
    xmit,                       /* pointer to serial xmit routine */
    stat232,                    /* pointer to serial RS232 status routine */
    stat232,                    /* saved pointer to serial RS232 status routine */
    ramreg4,                    /* pointer to RAM register array */
    ROWS,                       /* number of rows in RAM register array */
    &intvect4,                  /* interrupt vector structure */
    &fifoen4,                   /* fifo enable structure */
    &fifotrig4,                 /* fifo trigger structure */
    &parity4,                   /* parity structure */
    &stops4,                    /* stop bit structure */
    &dlen4,                     /* data length structure */
    &baud4,                     /* baud rate structure */
    &rts4,                      /* RS-232 RTS output structure */
    &dtr4,                      /* RS-232 DTR output structure */
    &gp1out4,                   /* RS-232 GP01 output structure */
    &gp2out4,                   /* RS-232 GP02 output structure */
    &breakb4,                   /* break bit */
    &loop4,                     /* loopback test structure */
    &cts4,                      /* RS-232 CTS input structure */
    &dsr4,                      /* RS-232 DSR input structure */
    &dcd4,                      /* RS-232 DCD input structure */
    &ri4,                       /* RS-232 RI input structure */
    INITIALIZED,                /* port number supplied during open */
    irstat,                     /* pointer to serial receiver status routine */
    iread,                      /* pointer to serial read routine */
    istat232,                   /* pointer to serial RS232 status routine */
    FALSE,                      /* interrupts ON/OFF flag */
};


/*
	INITIALIZATIONS FOR COMMUNICATIONS PORT 5
*/

struct baud baud5 =
 {
   BAUD_LOW,                    /* offset of UART lsb baud register */
   BAUD_HIGH,                   /* offset of UART msb baud register */
   {
     {BH50,	  BL50},   {BH75,	   BL75},    {BH150,  BL150},  {BH300,  BL300},
     {BH600,  BL600},  {BH1200,  BL1200},  {BH2400, BL2400}, {BH3600, BL3600},
     {BH4800, BL4800}, {BH7200,  BL7200},  {BH9600, BL9600}, {BH14K4, BL14K4},
     {BH19K2, BL19K2}, {BH28K8,  BL28K8},  {BH38K4, BL38K4}, {BH57K6, BL57K6},
     {BH76K8, BL76K8}, {BH153K6, BL153K6}, {BH230K, BL230K}, {BH921K6, BL921K6}
    },
   INITIALIZED,                 /* position of current baud divisor in table */
   B9600,                       /* initial baud at 9600 */
   BAUDLO,                      /* RAM register for low byte */
   BAUDHI                       /* RAM register for high byte */
 };


struct ramregbits parity5 =
{
   PARITYMASK,                  /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   PNONE,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { NONE_MASK, ODD_MASK, EVEN_MASK, -1, -1 } /* masks */
};


struct ramregbits stops5 =
{
   STOPMASK,                    /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   STOP1,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { ONE_MASK, ONE5_MASK, TWO_MASK, -1, -1 } /* masks */
};


struct ramregbits dlen5 =
{
   DMASK,                       /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   DL8,                         /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { D5_MASK, D6_MASK, D7_MASK, D8_MASK, -1 } /* masks */
};


struct ramregbits fifoen5 =
{
   FIFO_MASK,                   /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   FIFO_ON,                     /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { FIFO_DISABLE, FIFO_ENABLE, -1, -1, -1 } /* masks */
};


struct ramregbits fifotrig5 =
{
   TRIGGER_MASK,                /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   BYTE_1,                      /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { BYTE_1MASK, BYTE_4MASK, BYTE_8MASK, BYTE_14MASK, -1 } /* masks */
};


struct ramregbits intvect5 =
{
   INITIALIZED,                 /* mask to reset bits in this register */
   IVECT,                       /* register number */
   INITIALIZED,                 /* array position of current setting */
   INITIALIZED,                 /* array position to use in initialization */
   IVECTOR,                     /* interrupt vector register offset from base */
   { VECTOR5, -1, -1, -1, -1 }  /* masks */
};


byte ramreg5[ROWS][2]=
    {
    /* ram  saved */
      {	0,    0 }, 		/* 0 = baud low byte */
      {	0,    0 },		/* 1 = baud high byte */
      {	0,    0 },		/* 2 = interrupt enable */
      {	0,    0 },		/* 3 = data format */
      {	0,    0 },		/* 4 = RS232 output control */
      {	0,    0 },		/* 5 = FIFO control */
      {	0,    0 }		/* 6 = interrupt vector */
    };


/*
   RS-232 STRUCTURES
*/

struct out232 rts5 =            /* RTS */
{
   RTSMASK,                     /* mask to reset RTS */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 dtr5 =            /* DTR */
{
   DTRMASK,                     /* mask to reset DTR */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp1out5 =         /* GP01 */
{
   GP01MASK,                    /* mask to reset GP01 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp2out5 =         /* GP02 */
{
   GP02MASK,                    /* mask to reset GP02 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 breakb5 =         /* BREAK */
{
   BRKMASK,                     /* mask to reset break */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   LINE_CONT                    /* LCR register offset from base */
};


struct out232 loop5 =           /* loopback */
{
   LOOP_MASK,                   /* mask to reset loopback */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   LOOP_OFF,                    /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};



struct in232 cts5 =             /* CTS */
{
   CTSMASK,                     /* mask for CTS */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 dsr5 =             /* DSR */
{
   DSRMASK,                     /* mask for DSR */
   INITIALIZED,                 /* current setting */
   STAT_232,                    /* MSR register offset from base */
};

struct in232 dcd5 =             /* DCD */
{
   DCDMASK,                     /* mask for DCD */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 ri5 =              /* RI */
{
   RIMASK,                      /* mask for RI */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};


IO port5 =                      /* declare IO module 5 */
  {
    INITIALIZED,                /* base address of serial port 5 */
    TX_RX_REG,                  /* offset to UART data port */
    LINE_STAT,                  /* offset to UART status port */
    RCV_MASK,                   /* RxRDY mask */
    XMIT_MASK,                  /* TxRDY mask */
    input_byte,                 /* routine to input byte from UART */
    rcvstat,                    /* pointer to serial receiver status routine */
    rcvstat,                    /* saved pointer to serial receiver status routine */
    rcv,                        /* pointer to serial fetch routine */
    rcv,                        /* saved pointer to serial fetch routine */
	(void*)output_byte,			/* routine to output byte to UART */
    xmitstat,                   /* pointer to serial xmitter status routine */
    xmit,                       /* pointer to serial xmit routine */
    stat232,                    /* pointer to serial RS232 status routine */
    stat232,                    /* saved pointer to serial RS232 status routine */
    ramreg5,                    /* pointer to RAM register array */
    ROWS,                       /* number of rows in RAM register array */
    &intvect5,                  /* interrupt vector structure */
    &fifoen5,                   /* fifo enable structure */
    &fifotrig5,                 /* fifo trigger structure */
    &parity5,                   /* parity structure */
    &stops5,                    /* stop bit structure */
    &dlen5,                     /* data length structure */
    &baud5,                     /* baud rate structure */
    &rts5,                      /* RS-232 RTS output structure */
    &dtr5,                      /* RS-232 DTR output structure */
    &gp1out5,                   /* RS-232 GP01 output structure */
    &gp2out5,                   /* RS-232 GP02 output structure */
    &breakb5,                   /* break bit */
    &loop5,                     /* loopback test structure */
    &cts5,                      /* RS-232 CTS input structure */
    &dsr5,                      /* RS-232 DSR input structure */
    &dcd5,                      /* RS-232 DCD input structure */
    &ri5,                       /* RS-232 RI input structure */
    INITIALIZED,                /* port number supplied during open */
    irstat,                     /* pointer to serial receiver status routine */
    iread,                      /* pointer to serial read routine */
    istat232,                   /* pointer to serial RS232 status routine */
    FALSE,                      /* interrupts ON/OFF flag */
};


/*
	INITIALIZATIONS FOR COMMUNICATIONS PORT 6
*/

struct baud baud6 =
 {
   BAUD_LOW,                    /* offset of UART lsb baud register */
   BAUD_HIGH,                   /* offset of UART msb baud register */
   {
     {BH50,	  BL50},   {BH75,	   BL75},    {BH150,  BL150},  {BH300,  BL300},
     {BH600,  BL600},  {BH1200,  BL1200},  {BH2400, BL2400}, {BH3600, BL3600},
     {BH4800, BL4800}, {BH7200,  BL7200},  {BH9600, BL9600}, {BH14K4, BL14K4},
     {BH19K2, BL19K2}, {BH28K8,  BL28K8},  {BH38K4, BL38K4}, {BH57K6, BL57K6},
     {BH76K8, BL76K8}, {BH153K6, BL153K6}, {BH230K, BL230K}, {BH921K6, BL921K6}
    },
   INITIALIZED,                 /* position of current baud divisor in table */
   B9600,                       /* initial baud at 9600 */
   BAUDLO,                      /* RAM register for low byte */
   BAUDHI                       /* RAM register for high byte */
 };


struct ramregbits parity6 =
{
   PARITYMASK,                  /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   PNONE,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { NONE_MASK, ODD_MASK, EVEN_MASK, -1, -1 } /* masks */
};


struct ramregbits stops6 =
{
   STOPMASK,                    /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   STOP1,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { ONE_MASK, ONE5_MASK, TWO_MASK, -1, -1 } /* masks */
};


struct ramregbits dlen6 =
{
   DMASK,                       /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   DL8,                         /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { D5_MASK, D6_MASK, D7_MASK, D8_MASK, -1 } /* masks */
};


struct ramregbits fifoen6 =
{
   FIFO_MASK,                   /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   FIFO_ON,                     /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { FIFO_DISABLE, FIFO_ENABLE, -1, -1, -1 } /* masks */
};


struct ramregbits fifotrig6 =
{
   TRIGGER_MASK,                /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   BYTE_1,                      /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { BYTE_1MASK, BYTE_4MASK, BYTE_8MASK, BYTE_14MASK, -1 } /* masks */
};


struct ramregbits intvect6 =
{
   INITIALIZED,                 /* mask to reset bits in this register */
   IVECT,                       /* register number */
   INITIALIZED,                 /* array position of current setting */
   INITIALIZED,                 /* array position to use in initialization */
   IVECTOR,                     /* interrupt vector register offset from base */
   { VECTOR6, -1, -1, -1, -1 }  /* masks */
};


byte ramreg6[ROWS][2]=
    {
    /* ram  saved */
      {	0,    0 }, 		/* 0 = baud low byte */
      {	0,    0 },		/* 1 = baud high byte */
      {	0,    0 },		/* 2 = interrupt enable */
      {	0,    0 },		/* 3 = data format */
      {	0,    0 },		/* 4 = RS232 output control */
      {	0,    0 },		/* 5 = FIFO control */
      {	0,    0 }		/* 6 = interrupt vector */
    };


/*
   RS-232 STRUCTURES
*/

struct out232 rts6 =            /* RTS */
{
   RTSMASK,                     /* mask to reset RTS */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 dtr6 =            /* DTR */
{
   DTRMASK,                     /* mask to reset DTR */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp1out6 =         /* GP01 */
{
   GP01MASK,                    /* mask to reset GP01 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp2out6 =         /* GP02 */
{
   GP02MASK,                    /* mask to reset GP02 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 breakb6 =         /* BREAK */
{
   BRKMASK,                     /* mask to reset break */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   LINE_CONT                    /* LCR register offset from base */
};


struct out232 loop6 =           /* loopback */
{
   LOOP_MASK,                   /* mask to reset loopback */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   LOOP_OFF,                    /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};



struct in232 cts6 =             /* CTS */
{
   CTSMASK,                     /* mask for CTS */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 dsr6 =             /* DSR */
{
   DSRMASK,                     /* mask for DSR */
   INITIALIZED,                 /* current setting */
   STAT_232,                    /* MSR register offset from base */
};

struct in232 dcd6 =             /* DCD */
{
   DCDMASK,                     /* mask for DCD */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 ri6 =              /* RI */
{
   RIMASK,                      /* mask for RI */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};


IO port6 =                      /* declare IO module 6 */
  {
    INITIALIZED,                /* base address of serial port 6 */
    TX_RX_REG,                  /* offset to UART data port */
    LINE_STAT,                  /* offset to UART status port */
    RCV_MASK,                   /* RxRDY mask */
    XMIT_MASK,                  /* TxRDY mask */
    input_byte,                 /* routine to input byte from UART */
    rcvstat,                    /* pointer to serial receiver status routine */
    rcvstat,                    /* saved pointer to serial receiver status routine */
    rcv,                        /* pointer to serial fetch routine */
    rcv,                        /* saved pointer to serial fetch routine */
	(void*)output_byte,			/* routine to output byte to UART */
    xmitstat,                   /* pointer to serial xmitter status routine */
    xmit,                       /* pointer to serial xmit routine */
    stat232,                    /* pointer to serial RS232 status routine */
    stat232,                    /* saved pointer to serial RS232 status routine */
    ramreg6,                    /* pointer to RAM register array */
    ROWS,                       /* number of rows in RAM register array */
    &intvect6,                  /* interrupt vector structure */
    &fifoen6,                   /* fifo enable structure */
    &fifotrig6,                 /* fifo trigger structure */
    &parity6,                   /* parity structure */
    &stops6,                    /* stop bit structure */
    &dlen6,                     /* data length structure */
    &baud6,                     /* baud rate structure */
    &rts6,                      /* RS-232 RTS output structure */
    &dtr6,                      /* RS-232 DTR output structure */
    &gp1out6,                   /* RS-232 GP01 output structure */
    &gp2out6,                   /* RS-232 GP02 output structure */
    &breakb6,                   /* break bit */
    &loop6,                     /* loopback test structure */
    &cts6,                      /* RS-232 CTS input structure */
    &dsr6,                      /* RS-232 DSR input structure */
    &dcd6,                      /* RS-232 DCD input structure */
    &ri6,                       /* RS-232 RI input structure */
    INITIALIZED,                /* port number supplied during open */
    irstat,                     /* pointer to serial receiver status routine */
    iread,                      /* pointer to serial read routine */
    istat232,                   /* pointer to serial RS232 status routine */
    FALSE,                      /* interrupts ON/OFF flag */
};


/*
	INITIALIZATIONS FOR COMMUNICATIONS PORT 7
*/

struct baud baud7 =
 {
   BAUD_LOW,                    /* offset of UART lsb baud register */
   BAUD_HIGH,                   /* offset of UART msb baud register */
   {
     {BH50,	  BL50},   {BH75,	   BL75},    {BH150,  BL150},  {BH300,  BL300},
     {BH600,  BL600},  {BH1200,  BL1200},  {BH2400, BL2400}, {BH3600, BL3600},
     {BH4800, BL4800}, {BH7200,  BL7200},  {BH9600, BL9600}, {BH14K4, BL14K4},
     {BH19K2, BL19K2}, {BH28K8,  BL28K8},  {BH38K4, BL38K4}, {BH57K6, BL57K6},
     {BH76K8, BL76K8}, {BH153K6, BL153K6}, {BH230K, BL230K}, {BH921K6, BL921K6}
    },
   INITIALIZED,                 /* position of current baud divisor in table */
   B9600,                       /* initial baud at 9600 */
   BAUDLO,                      /* RAM register for low byte */
   BAUDHI                       /* RAM register for high byte */
 };


struct ramregbits parity7 =
{
   PARITYMASK,                  /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   PNONE,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { NONE_MASK, ODD_MASK, EVEN_MASK, -1, -1 } /* masks */
};


struct ramregbits stops7 =
{
   STOPMASK,                    /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   STOP1,                       /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { ONE_MASK, ONE5_MASK, TWO_MASK, -1, -1 } /* masks */
};


struct ramregbits dlen7 =
{
   DMASK,                       /* mask to reset bits in this register */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* array position of current setting */
   DL8,                         /* array position to use in initialization */
   LINE_CONT,                   /* LCR register offset from base */
   { D5_MASK, D6_MASK, D7_MASK, D8_MASK, -1 } /* masks */
};


struct ramregbits fifoen7 =
{
   FIFO_MASK,                   /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   FIFO_ON,                     /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { FIFO_DISABLE, FIFO_ENABLE, -1, -1, -1 } /* masks */
};


struct ramregbits fifotrig7 =
{
   TRIGGER_MASK,                /* mask to reset bits in this register */
   FIFO,                        /* register number */
   INITIALIZED,                 /* array position of current setting */
   BYTE_1,                      /* array position to use in initialization */
   FIFO_CONT,                   /* FIFO register offset from base */
   { BYTE_1MASK, BYTE_4MASK, BYTE_8MASK, BYTE_14MASK, -1 } /* masks */
};


struct ramregbits intvect7 =
{
   INITIALIZED,                 /* mask to reset bits in this register */
   IVECT,                       /* register number */
   INITIALIZED,                 /* array position of current setting */
   INITIALIZED,                 /* array position to use in initialization */
   IVECTOR,                     /* interrupt vector register offset from base */
   { VECTOR7, -1, -1, -1, -1 }  /* masks */
};


byte ramreg7[ROWS][2]=
    {
    /* ram  saved */
      {	0,    0 }, 		/* 0 = baud low byte */
      {	0,    0 },		/* 1 = baud high byte */
      {	0,    0 },		/* 2 = interrupt enable */
      {	0,    0 },		/* 3 = data format */
      {	0,    0 },		/* 4 = RS232 output control */
      {	0,    0 },		/* 5 = FIFO control */
      {	0,    0 }		/* 6 = interrupt vector */
    };


/*
   RS-232 STRUCTURES
*/

struct out232 rts7 =            /* RTS */
{
   RTSMASK,                     /* mask to reset RTS */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 dtr7 =            /* DTR */
{
   DTRMASK,                     /* mask to reset DTR */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp1out7 =         /* GP01 */
{
   GP01MASK,                    /* mask to reset GP01 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 gp2out7 =         /* GP02 */
{
   GP02MASK,                    /* mask to reset GP02 */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};


struct out232 breakb7 =         /* BREAK */
{
   BRKMASK,                     /* mask to reset break */
   FORMAT,                      /* register number */
   INITIALIZED,                 /* current setting */
   OFF,                         /* startup state */
   LINE_CONT                    /* LCR register offset from base */
};


struct out232 loop7 =           /* loopback */
{
   LOOP_MASK,                   /* mask to reset loopback */
   OUT232,                      /* register number */
   INITIALIZED,                 /* current setting */
   LOOP_OFF,                    /* startup state */
   MODEM_CONT                   /* MCR register offset from base */
};



struct in232 cts7 =             /* CTS */
{
   CTSMASK,                     /* mask for CTS */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 dsr7 =             /* DSR */
{
   DSRMASK,                     /* mask for DSR */
   INITIALIZED,                 /* current setting */
   STAT_232,                    /* MSR register offset from base */
};

struct in232 dcd7 =             /* DCD */
{
   DCDMASK,                     /* mask for DCD */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};

struct in232 ri7 =              /* RI */
{
   RIMASK,                      /* mask for RI */
   INITIALIZED,                 /* current setting */
   STAT_232                     /* MSR register offset from base */
};


IO port7 =                      /* declare IO module 7 */
  {
    INITIALIZED,                /* base address of serial port 7 */
    TX_RX_REG,                  /* offset to UART data port */
    LINE_STAT,                  /* offset to UART status port */
    RCV_MASK,                   /* RxRDY mask */
    XMIT_MASK,                  /* TxRDY mask */
    input_byte,                 /* routine to input byte from UART */
    rcvstat,                    /* pointer to serial receiver status routine */
    rcvstat,                    /* saved pointer to serial receiver status routine */
    rcv,                        /* pointer to serial fetch routine */
    rcv,                        /* saved pointer to serial fetch routine */
	(void*)output_byte,			/* routine to output byte to UART */
    xmitstat,                   /* pointer to serial xmitter status routine */
    xmit,                       /* pointer to serial xmit routine */
    stat232,                    /* pointer to serial RS232 status routine */
    stat232,                    /* saved pointer to serial RS232 status routine */
    ramreg7,                    /* pointer to RAM register array */
    ROWS,                       /* number of rows in RAM register array */
    &intvect7,                  /* interrupt vector structure */
    &fifoen7,                   /* fifo enable structure */
    &fifotrig7,                 /* fifo trigger structure */
    &parity7,                   /* parity structure */
    &stops7,                    /* stop bit structure */
    &dlen7,                     /* data length structure */
    &baud7,                     /* baud rate structure */
    &rts7,                      /* RS-232 RTS output structure */
    &dtr7,                      /* RS-232 DTR output structure */
    &gp1out7,                   /* RS-232 GP01 output structure */
    &gp2out7,                   /* RS-232 GP02 output structure */
    &breakb7,                   /* break bit */
    &loop7,                     /* loopback test structure */
    &cts7,                      /* RS-232 CTS input structure */
    &dsr7,                      /* RS-232 DSR input structure */
    &dcd7,                      /* RS-232 DCD input structure */
    &ri7,                       /* RS-232 RI input structure */
    INITIALIZED,                /* port number supplied during open */
    irstat,                     /* pointer to serial receiver status routine */
    iread,                      /* pointer to serial read routine */
    istat232,                   /* pointer to serial RS232 status routine */
    FALSE,                      /* interrupts ON/OFF flag */
};


/*
   CONTAINS THE NUMBER OF SERIAL PORTS IN THE SYSTEM
*/

struct iostatus iolist[MAXPORTS] = /* 8 ports on IOS521 */
{
  { &port0,    CLOSED, FALSE },    /* port 1 */
  { &port1,    CLOSED, FALSE },    /* port 2 */
  { &port2,    CLOSED, FALSE },    /* port 3 */
  { &port3,    CLOSED, FALSE },    /* port 4 */
  { &port4,    CLOSED, FALSE },    /* port 5 */
  { &port5,    CLOSED, FALSE },    /* port 6 */
  { &port6,    CLOSED, FALSE },    /* port 7 */
  { &port7,    CLOSED, FALSE },    /* port 8 */
  { (void *)0, CLOSED, FALSE },    /* port not present */
  { (void *)0, CLOSED, FALSE },    /* port not present */
  { (void *)0, CLOSED, FALSE },    /* port not present */
  { (void *)0, CLOSED, FALSE },    /* port not present */
  { (void *)0, CLOSED, FALSE },    /* port not present */
  { (void *)0, CLOSED, FALSE },    /* port not present */
  { (void *)0, CLOSED, FALSE },    /* port not present */
  { (void *)0, CLOSED, FALSE }     /* port not present */
};


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    xmit

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Send data using IO pointer.

    CALLING
	SEQUENCE:   void xmit(io, c)
			Where:
				io (pointer*) to struct IO
				c  (byte) character to send
    MODULE TYPE:

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
*/


void xmit(io, c)

register IO *io;                        /* pointer to IO structure */
byte c;                                 /* char to send */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + io->data_off,c);  /* send the byte */
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    xmitstat

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Get transmitter status using IO pointer.

    CALLING
	SEQUENCE:   byte xmitstat(io)
			Where:
				return (byte) transmitter status
				io (pointer*) to struct IO
    MODULE TYPE:

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
*/


byte xmitstat(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return((*io->getbyte)(io->is_ptr->nHandle, io->base_addr + io->status_off) & io->xmitmask);  /* read the byte */
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    rcvstat

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Get receiver status using IO pointer.

    CALLING
	SEQUENCE:   byte rcvstat(io)
			Where:
				return (byte) receiver status
				io (pointer*) to struct IO
    MODULE TYPE:

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
*/


byte rcvstat(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return((*io->getbyte)(io->is_ptr->nHandle, io->base_addr + io->status_off) & io->rcvmask);  /* read the byte */
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    rcv

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Get data using IO pointer.

    CALLING
	SEQUENCE:   byte rcv(io)
			Where:
				return (byte) byte read
				io (pointer*) to struct IO
    MODULE TYPE:

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
*/


byte rcv(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return((*io->getbyte)(io->is_ptr->nHandle, io->base_addr + io->data_off));  /* read the byte */
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setrambaud

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       This function is used to change the baud rate.

    CALLING
	SEQUENCE:   void setrambaud(io, bp, lbaud, hbaud)
			Where:
			   io (pointer*) to struct IO
			   bp (pointer*) to struct ramregbits
			   lbaud (byte) low byte of baud divisor
			   hbaud (byte) high byte of baud divisor
    MODULE TYPE:

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

   The baud rate is changed by writing lbaud and hbaud to the UART's
   LSB and MSB Baud registers,  These registers can be addressed only
   if bit 7 in the Data Format register is TRUE.  After writing to
   these registers, the Data Format register is restored.
*/


void setrambaud(io, bp, lbaud, hbaud)

register IO *io;                        /* pointer to IO structure */
struct baud *bp;                        /* pointer to baud structure */
byte lbaud;                             /* low byte of baud divisor */
byte hbaud;                             /* high byte of baud divisor */

{

/*
    DECLARE LOCAL DATA AREAS:
*/

  unsigned tmp;                         /* temp copy of format register */

/*
    ENTRY POINT OF ROUTINE:
*/

   tmp = (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + io->par->offset) | 0x80;
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + io->par->offset, tmp);
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + bp->offlo, lbaud);
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + bp->offhi, hbaud);
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + io->par->offset, tmp & 0x7F);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    p_config

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Save the current UART values in the RAM array.

    CALLING
	SEQUENCE:   void p_config(io)
			Where:
			   io (pointer*) to struct IO

    MODULE TYPE:

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

   Save the current UART values in the 'saved' column of the RAM array.
   The Baud Rate Divisor registers are saved first.
   Next, the Interrupt Enable register is saved, followed by the Data
   Format register, and finally the Modem Control register.
   A loop copies these values into the 'copy' columns.
   Both columns contain identical, current copies of the UART's registers.
*/


void p_config(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    DECLARE LOCAL DATA AREAS:
*/

  byte tmp;
  int i;

/*
    ENTRY POINT OF ROUTINE:
*/

/*
   Copies the contents of the UART's registers into the IO's RAM array.
*/

   tmp = (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + io->par->offset) | 0x80;

   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + io->par->offset, tmp);

   io->ramreg[BAUDLO][1] =
		(*io->getbyte)(io->is_ptr->nHandle, io->base_addr + io->br->offlo);

   io->ramreg[BAUDHI][1] =
		(*io->getbyte)(io->is_ptr->nHandle, io->base_addr + io->br->offhi);

   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + io->par->offset, tmp & 0x7F);

   io->ramreg[INTR][1] =
		(*io->getbyte)(io->is_ptr->nHandle, io->base_addr + INT_ENABLE_REG);

   io->ramreg[FORMAT][1] =
		(*io->getbyte)(io->is_ptr->nHandle, io->base_addr + io->par->offset);

   io->ramreg[OUT232][1] =
		(*io->getbyte)(io->is_ptr->nHandle, io->base_addr + MODEM_CONT);

   io->ramreg[IVECT][1] =
		(*io->getbyte)(io->is_ptr->nHandle, io->base_addr + IVECTOR);

/*
   The FIFO might be a write only register, 0 is a safe value to read
*/

   io->ramreg[FIFO][1] = 0;


   for(i = 0; i < ROWS; i++)  /* copy 'saved' column into 'copy' column */
       io->ramreg[i][0] = io->ramreg[i][1];
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    p_restore

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Restores the state of the UART.

    CALLING
	SEQUENCE:   void p_restore(io)
			Where:
			   io (pointer*) to struct IO

    MODULE TYPE:

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

   Restores the state of the UART, this function writes the original
   values stored in the 'saved' column of the RAM register to the UART.
*/


void p_restore(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   setrambaud(io,io->br,io->ramreg[BAUDLO][1],io->ramreg[BAUDHI][1]);
   setrambits(io,io->par,io->ramreg[FORMAT][1]);
   setrambits(io,io->fcont,io->ramreg[FIFO][1]);
   setrambits(io,io->ftrig,io->ramreg[FIFO][1]);
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + INT_ENABLE_REG, io->ramreg[INTR][1]);
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + IVECTOR, io->ramreg[IVECT][1]);
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + MODEM_CONT, io->ramreg[OUT232][1]);
}


/*
{+D}
    SYSTEM:         IP500 Software

    MODULE NAME:    enableint.c

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Activates UART interrupts.

    CALLING
	SEQUENCE:

    MODULE TYPE:    void

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

    Activates UART interrupts.
*/



void enableint(io, port)

register IO *io;
int port;

{

/*
    ENTRY POINT OF ROUTINE:
*/

/*
	Read registers to clear possible interrupts
*/

   (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + TX_RX_REG);
   (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + INT_ENABLE_REG);
   (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + INT_ID);
   (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + LINE_CONT);
   (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + MODEM_CONT);
   (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + LINE_STAT);
   (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + STAT_232);

   setgpo2(io, ON);             /* enable UART interrupts */
   intlist[port].savegpo2 = io->gpo2; /* save address of gpo2 structure */
   io->gpo2 = (void *)0;        /* remove gpo2 control */

   /* Enable UART interrupts */ 
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + INT_ENABLE_REG, intlist[port].ienable);
}


/*
{+D}
    SYSTEM:         IP500 Software

    MODULE NAME:    disableint.c

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       deactivates UART interrupts.

    CALLING
	SEQUENCE:

    MODULE TYPE:    void

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

    Deactivates UART interrupts.
*/



void disableint(io, port)

register IO *io;
int port;

{

/*
    ENTRY POINT OF ROUTINE:

    Disable UART interrupts
*/

   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + INT_ENABLE_REG, 0);

   io->gpo2 = intlist[port].savegpo2; /* restore address of gpo2 structure */
   setgpo2(io, OFF);         /* restore UART interrupts */
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    waitsre

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Wait for the transmitter holding register to become empty

    CALLING
	SEQUENCE:   void waitsre(io)
			Where:
			   io (pointer*) to struct IO
    MODULE TYPE:

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
*/


void waitsre(io)
IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:

   Wait for the transmitter shift register to become empty.
*/
   
   while(((*io->getbyte)(io->is_ptr->nHandle, io->base_addr + io->status_off) & TSRE_MASK) == 0)
   ;
}


/*
{+D}
	SYSTEM:         Serial Port Software

	MODULE NAME:    delay.c

	VERSION:        V1.0

	CREATION DATE:  04/01/09

	DESIGNED BY:    FJM

	CODED BY:       FJM

	ABSTRACT:       Waits tsecs system timing intervals.

	CALLING
	  SEQUENCE:     void = delay(timeout);
			Where
				timeout (int) value in 54.9 mS ticks

	MODULE TYPE:

	I/O
	  RESOURCES:    None

	SYSTEM
	  RESOURCES:    None

	MODULES
	  CALLED:

	REVISIONS:

  DATE      BY     PURPOSE
---------  ----   -------------------------------------------------------

{-D}
*/

/*
   MODULES FUNCTIONAL DETAILS:
*/


void delay(timeout)

int timeout;


{

/*
	Declare local data areas
*/

  unsigned ctime;	     /* current tick time */

/*
	Entry point of routine the delay is 54.9mS/count
*/

  ctime = (unsigned)timeout;
  ctime *= 55;
  ctime /= 10;		     /* approx 10ms/tick */
  usleep(ctime * 10000);
}
