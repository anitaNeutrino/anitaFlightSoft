
/*
{+D}
    SYSTEM:         Serial Port Software

    FILE NAME:      portio.h

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       This module contains the definitions and structures
                    used by the serial IO library.
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
    the serial IO library.
*/

/*
    DEFINITIONS:
*/

typedef unsigned char bool;

#define CLOSED  FALSE
#define OPEN    TRUE

#define TIMEOUT        -1      /* function timeout indicator */

#define ROWS            7      /* number of rows in a ramreg structure */

#define MAXPORTS       16      /* maximum number of ports */

#define INT_REQUEST_0  0	/* interrupt request 0 line on carrier */
#define INT_REQUEST_1  1	/* interrupt request 1 line on carrier */

/*
   INTERRUPT VECTORS
*/

#define VECTOR0      192          /* interrupt vector for port 0 */
#define VECTOR1      VECTOR0 + 2  /* interrupt vector for port 1 */
#define VECTOR2      VECTOR0 + 4  /* interrupt vector for port 2 */
#define VECTOR3      VECTOR0 + 6  /* interrupt vector for port 3 */
#define VECTOR4      VECTOR0 + 8  /* interrupt vector for port 4 */
#define VECTOR5      VECTOR0 + 10 /* interrupt vector for port 5 */
#define VECTOR6      VECTOR0 + 12 /* interrupt vector for port 6 */
#define VECTOR7      VECTOR0 + 14 /* interrupt vector for port 7 */

/*
   BIT POSITIONS IN THE INTERRUPT ENABLE REGISTER
*/

#define IRXRDY         1        /* receiver ready */
#define ITBE           2        /* transmitter empty */
#define ILINESTAT      4        /* receiver line status */
#define IMODEMSTAT     8        /* modem status */

/*
   UART CONSTANTS
*/

#define DMASK         0xFC      /* 1111 1100 */
#define D5_MASK          0
#define D6_MASK          1
#define D7_MASK          2
#define D8_MASK          3

#define STOPMASK      0xFB      /* 1111 1011 */
#define ONE_MASK         0
#define ONE5_MASK        4
#define TWO_MASK         4

#define PARITYMASK    0xC7      /* 1100 0111 */
#define NONE_MASK        0
#define ODD_MASK         8
#define EVEN_MASK     0x18      /* 0001 1000 */
#define MARK_MASK     0x28      /* 0010 1000 */
#define SPACE_MASK    0x38      /* 0011 1000 */

#define RCV_MASK         1      /* RxRDY mask */
#define XMIT_MASK     0x20      /* TxRDY mask */
#define TSRE_MASK     0x40      /* transmitter shift register empty mask */

#define LOOP_MASK     0xEF      /* loopback mask */

#define LOOP_ON          1      /* loopback enable mask */
#define LOOP_OFF         0      /* loopback disable mask */

#define FIFO_MASK     0xFE      /* FIFO mask */

#define FIFO_ENABLE      1      /* FIFO enable mask */
#define FIFO_DISABLE     0      /* FIFO disable mask */

#define FIFO_OFF         0      /* disable FIFO */
#define FIFO_ON          1      /* enable FIFO */

#define TRIGGER_MASK  0x3F      /* FIFO trigger mask */

/*
For the IOS52x module,
the actual trigger levels are 8, 16, 56, and 60 for the following byte masks
*/

#define BYTE_1MASK       0      /* FIFO trigger mask */
#define BYTE_4MASK    0x40      /* FIFO trigger mask */
#define BYTE_8MASK    0x80      /* FIFO trigger mask */
#define BYTE_14MASK   0xC0      /* FIFO trigger mask */

#define BYTE_1           0      /* FIFO triggers on one byte */
#define BYTE_4           1      /* FIFO triggers on four bytes */
#define BYTE_8           2      /* FIFO triggers on eight bytes */
#define BYTE_14          3      /* FIFO triggers on fourteen bytes */

/*
   CONSTANTS FOR RAM REGISTER NUMBERS
*/

#define BAUDLO           0      /* 0 = baud low byte */
#define BAUDHI           1      /* 1 = baud high byte */
#define INTR             2      /* 2 = interrupt enable */
#define FORMAT           3      /* 3 = data format */
#define OUT232           4      /* 4 = RS232 control */
#define FIFO             5      /* 5 = FIFO control */
#define IVECT            6      /* 6 = interrupt vector */


/*
    BAUD RATE DIVISOR CONSTANTS
*/

#define BH50       0x48         /* 50 baud */
#define BL50       0x00
#define BH75       0x30         /* 75 baud */
#define BL75       0x00
#define BH150      0x18         /* 150 baud */
#define BL150      0x00
#define BH300      0x0C         /* 300 baud */
#define BL300      0x00
#define BH600      0x06         /* 600 baud */
#define BL600      0x00
#define BH1200     0x03         /* 1200 baud */
#define BL1200     0x00
#define BH2400     0x01         /* 2400 baud */
#define BL2400     0x80
#define BH3600     0x01         /* 3600 baud */
#define BL3600     0x00
#define BH4800     0x00         /* 4800 baud */
#define BL4800     0xC0
#define BH7200     0x00         /* 7200 baud */
#define BL7200     0x80
#define BH9600     0x00         /* 9600 baud */
#define BL9600     0x60
#define BH14K4     0x00         /* 14400 baud */
#define BL14K4     0x40
#define BH19K2     0x00         /* 19200 baud */
#define BL19K2     0x30
#define BH28K8     0x00         /* 28800 baud */
#define BL28K8     0x20
#define BH38K4     0x00         /* 38400 baud */
#define BL38K4     0x18
#define BH57K6     0x00         /* 57600 baud */
#define BL57K6     0x10
#define BH76K8     0x00         /* 76800 baud */
#define BL76K8     0x0C
#define BH153K6    0x00         /* 153600 baud */
#define BL153K6    0x06
#define BH230K     0x00         /* 230400 baud */
#define BL230K     0x04
#define BH921K6    0x00         /* 921600 baud */
#define BL921K6    0x01

/*
   RS232 OUTPUT CONTROL CONSTANTS
*/

#define OFF           0         /* RS-232 negated */
#define ON            1         /* RS-232 asserted */

#define DTRMASK    0xFE         /* 1111 1110 */
#define RTSMASK    0xFD         /* 1111 1101 */
#define GP02MASK   0xF7         /* 1111 0111 */
#define GP01MASK   0xFB         /* 1111 1011 */
#define BRKMASK    0xBF         /* 1011 1111 */

#define CTSMASK    0x10         /* 0001 0000 */
#define DSRMASK    0x20         /* 0010 0000 */
#define RIMASK     0x40         /* 0100 0000 */
#define DCDMASK    0x80         /* 1000 0000 */

/*
   GENERAL CONSTANTS
*/


#define NUMITEMS         5     /* items in 'smask' array */
#define BAUDS           20     /* number of baud rates supported */

#define B50			0      /* 50 baud */
#define B75			1      /* 75 baud */
#define B150		2      /* 150 baud */
#define B300		3      /* 300 baud */
#define B600		4      /* 600 baud */
#define B1200		5      /* 1200 baud */
#define B2400		6      /* 2400 baud */
#define B3600		7      /* 3600 baud */
#define B4800		8      /* 4800 baud */
#define B7200		9      /* 7200 baud */
#define B9600		10     /* 9600 baud */
#define B14K4		11     /* 14400 baud */
#define B19K2		12     /* 19200 baud */
#define B28K8		13     /* 28800 baud */
#define B38K4		14     /* 38400 baud */
#define B57K6		15     /* 57600 baud */
#define B76K8		16     /* 76800 baud */
#define B153K6		17     /* 115200 baud */
#define B230K		18     /* 230400 baud */
#define B921K6      19     /* 921600 baud */

#define UNUSED          -2     /* initializer (must be < -1) */
#define INITIALIZED      0     /* initializer (set up during init) */

/*
   DATA FORMAT CONSTANTS
*/

/* PARITY */

#define PNONE            0
#define PODD             1
#define PEVEN            2
#define PMARK            3
#define PSPACE           4

/* STOP BITS */

#define STOP1            0
#define STOP1_5          1
#define STOP2            2

/* DATA LENGTH */

#define DL5              0
#define DL6              1
#define DL7              2
#define DL8              3

/*
   ERROR CODES
*/

#define ARGUMENT_RANGE   1      /* argument out of range */
#define BAD_FUNCTION     2      /* function not supported */
#define NO_HARDWARE      3      /* hardware not supported */
#define BUSY             4      /* port already open */
#define NOT_FOUND        5      /* no such port */

/*
   RAM REGISTER STRUCTURE
*/

struct ramregbits
{
   byte rmask;                  /* mask to reset bit(s) in register */
   byte ramregnum;              /* RAM register number */
   byte current;                /* code for current value */
   byte init;                   /* code to use at initialization */
   int offset;                  /* offset of the physical UART register */
   short smask[NUMITEMS];       /* masks for each setting */
};

/*
   BAUD DATA STRUCTURES
*/

struct baud
 {
   int offlo;                   /* offset of low-order baud rate register */
   int offhi;                   /* offset of high-order baud rate register */
   int divisor[BAUDS][2];       /* array of two-byte divisors */
   byte current;                /* position in array of current divisor */
   byte init;                   /* position in array of initialization value */
   byte regl;                   /* low-order byte of baud rate divisor */
   byte regh;                   /* high-order byte of baud rate divisor */
 };


/*
   RS-232 OUTPUT STRUCTURE
*/

struct out232
{
   byte rmask;                  /* mask to reset bit(s) in register */
   byte ramregnum;              /* RAM register number */
   bool current;                /* code for current value */
   bool init;                   /* code to use at initialization */
   int offset;                  /* offset of the physical UART register */
};

/*
   RS-232 INPUT STRUCTURE
*/

struct in232
{
   byte mask;                   /* mask to isolate active bit */
   byte current;                /* current value */
   int offset;                  /* offset of the physical UART register */
};


/*
    Defined below is the structure which is used to hold the
    interrupt source information.
*/

struct int_source
{
    char slotLetter;            /* IOS slot letter */
    int nHandle;                /* handle to an open carrier board */
    BOOL bCarrier;              /* flag indicating a carrier is open */
    BOOL bInitialized;          /* flag indicating ready to talk to board */	
    byte id_prom[33];           /* id prom contents. */
};


/*
    Defined below is the structure for the serial port
*/

typedef struct serial
{
    byte *base_addr;            /* pointer to base address of IP module */
    int data_off;               /* offset from base to data port */
    int status_off;             /* offset from base to status port */
    unsigned rcvmask;           /* RxRDY mask */
    unsigned xmitmask;          /* TxRDY mask */
    byte (*getbyte)();          /* pointer to read routine */
    byte (*rstat)();            /* pointer to receiver status */
    byte (*savrstat)();         /* saved pointer to receiver status */
    byte (*read)();             /* pointer to read routine */
    byte (*savread)();          /* saved pointer to read routine */
    void (*putbyte)();          /* pointer to write routine */
    byte (*xstat)();            /* pointer to xmitter status */
    void (*xmit)();             /* pointer to xmit routine */
    byte (*read232)();          /* pointer to RS232 status routine */
    byte (*savread232)();       /* saved pointer to RS232 status routine */
    byte (*ramreg)[2];          /* pointer to 2-dim array */
    int  ramregsize;            /* length of array */
    struct ramregbits *ivect;   /* pointer to interrupt vector structure */
    struct ramregbits *fcont;   /* pointer to FIFO control structure */
    struct ramregbits *ftrig;   /* pointer to FIFO trigger structure */
    struct ramregbits *par;     /* pointer to parity structure */
    struct ramregbits *sb;      /* pointer to stop bit structure */
    struct ramregbits *dl;      /* pointer to data length structure */
    struct baud *br;            /* pointer to baud rate structure */
    struct out232 *rts;         /* pointer to RTS structure */
    struct out232 *dtr;         /* pointer to DTR structure */
    struct out232 *gpo1;        /* pointer to OUTPUT 1 structure */
    struct out232 *gpo2;        /* pointer to OUTPUT 2 structure */
    struct out232 *brkbit;      /* pointer to break structure */
    struct out232 *loop;        /* pointer to loopback structure */
    struct in232 *cts;          /* pointer to CTS structure */
    struct in232 *dsr;          /* pointer to DSR structure */
    struct in232 *dcd;          /* pointer to DCD structure */
    struct in232 *ri;           /* pointer to RI structure */
    short devnum;               /* opened port number */
    byte (*intstat)();          /* pointer to interrupt receiver status */
    byte (*intread)();          /* pointer to interrupt read routine */
    byte (*intread232)();       /* pointer to RS232 interrupt status routine */
    bool intflag;               /* interrupt on/off flag */
    struct int_source *is_ptr;  /* interrupt source information */
} IO;



/*
    Defined below is the structure for serial port management
*/

struct iostatus
{
  IO *typ;                      /* IO pointer for this port */
  bool openflag;                /* currently open? */
  bool intok;                   /* interrupts supported? */
};

/*
    Defined below is the structure for serial port interrupt management
*/

struct ids
{
  byte ienable;                 /* mask for interrupt enable register */
  struct out232 *savegpo2;      /* save address of output 2 structure */
}; 



/*
    DECLARE MODULES CALLED:
*/

    void handshake();
    void psts521();             /* routine calls the Read I.D. Command */
    void rsts521();             /* routine to read I.D. information */
    int sendbrk();              /* send break signal */
    void term();                /* terminal emulator */
    IO *p_open();               /* open serial port */
    int p_close();              /* close serial port */
    void setup();               /* setup data format */
    int p_inchar();             /* get a single character */
    void p_putc();              /* put a single character */
    int getrts();               /* Gets the corresponding RS-232 output pin state. */
    int getdtr();               /* Gets the corresponding RS-232 output pin state. */
    int getgpo1();              /* Gets the corresponding RS-232 output pin state. */
    int getgpo2();              /* Gets the corresponding RS-232 output pin state. */
    int getcts();               /* Gets the corresponding RS-232 output pin state. */
    int getdsr();               /* Gets the corresponding RS-232 output pin state. */
    int getdcd();               /* Gets the corresponding RS-232 output pin state. */
    int getri();                /* Gets the corresponding RS-232 output pin state. */
    int getparity();            /* read UART parity */
    int getstops();             /* read UART stop bits */
    int getdl();                /* read UART data bits */
    int getbaud();              /* read UART baud rate */
    int getloopback();          /* read UARTs loopback control */
    int getfifotrigger();       /* read UART FIFO trigger point */
    int getfifocontrol();       /* read UART FIFO control */

    void setup232();            /* setup RS232 */
    void setupfifo();           /* setup FIFO controls */
    void setuploop();           /* setup loopback control */
    int setfifocontrol();       /* configure UART FIFO control */
    int setfifotrigger();       /* configure UART FIFO trigger point */
    int setparity();            /* configure UART parity */
    int setstops();             /* configure UART stop bits */
    int setdatalen();           /* configure UART data bits */
    int setbaud();              /* configure UART baud rate */
    int setrts();               /* Sets or resets the corresponding RS-232 output pin. */
    int setdtr();               /* Sets or resets the corresponding RS-232 output pin. */
    int setgpo1();              /* Sets or resets the corresponding RS-232 output pin. */
    int setgpo2();              /* Sets or resets the corresponding RS-232 output pin. */
    void waitsre();             /* wait for shift register enpty */
    int setloopback();          /* configure UARTs loopback control */
    void setrambaud();          /* This function is used to change the baud rate */
    int setrambits();           /* Builds bit pattern in RAM register. */
    void delay();               /* delay routine */
