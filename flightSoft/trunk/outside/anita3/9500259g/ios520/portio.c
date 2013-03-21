
#include "../ioscarrier/ioscarrier.h"
#include "ios520.h"
#include "portio.h"		/* serial port include file */

/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    p_inchar

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Reads a byte from the serial port pointed to by io,
                    if one is ready else returns.

    CALLING
	SEQUENCE:   int p_inchar(io)
			Where:
			   return (int) byte read if one was ready, else -1
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

    Read byte from the serial port pointed to by io, if one is ready
    else returns.
*/


int p_inchar(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   if((*io->rstat)(io))
      return((*io->read)(io));  /* read byte */

   return(-1);
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    p_getc

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Waits for a byte from the serial port pointed to by io.

    CALLING
	SEQUENCE:   byte p_getc(io)
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

   Waits for a byte from the serial port pointed to by io.
*/


byte p_getc(io)

register IO *io;                     /* pointer to io structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   while((*io->rstat)(io) == 0)         /* wait for character ready */
   ;
   return((*io->read)(io));             /* read character */
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    p_putc

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Send byte to the serial port pointed to by io.

    CALLING
	SEQUENCE:   void p_putc(io,c)
			Where:
				io (pointer*) to struct IO
				c (byte) byte to send
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

   Send byte to the serial port pointed to by io.
*/


void p_putc(io,c)

register IO *io;                        /* pointer to io structure */
byte c;                                 /* char to send */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   while((*io->xstat)(io) == 0)         /* wait for transmitter clear */
   ;
   (*io->xmit)(io,c);                   /* send character */
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setrambits

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Builds bit pattern in RAM register.

    CALLING
	SEQUENCE:   int setrambits(io, r, pos)
			Where:
			   return (int) 0 if successful or
				   OUT_OF_RANGE - argument out of range
				   NO_FUNCTION  - function not supported
				   NO_HARDWARE  - not supported by hardware
			   io (pointer*) to struct IO
			   r (pointer*) to struct ramregbits
			   pos (byte) position of value in array to install
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

   Builds a bit pattern in the RAM register array ramreg by
   ANDing and ORing, then writes the bit pattern to the UART.
*/


int setrambits(io, r, pos)

register IO *io;                        /* pointer to IO structure */
struct ramregbits *r;                   /* pointer to ramregbits structure */
byte pos;                               /* array position in list to write */

{

/*
    DECLARE LOCAL DATA AREAS:
*/

   byte tmp;

/*
    ENTRY POINT OF ROUTINE:
*/

   if(pos >= NUMITEMS)          /* argument range check */
      return(ARGUMENT_RANGE);

   if(r == (void *)0)           /* pointer is 0 if function not supported */
      return(BAD_FUNCTION);

   if(r->smask[pos] == -1)      /* no hardware */
      return(NO_HARDWARE);

   io->ramreg[r->ramregnum][0] &= r->rmask;     /* reset bits */
   tmp = io->ramreg[r->ramregnum][0] |= r->smask[pos];
   r->current = pos;            /* update current table position */
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + r->offset, tmp); /* write out new value */
   return(0);
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setparity

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets the parity.

    CALLING
	SEQUENCE:   int setparity(io, pos)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
				   NO_HARDWARE     - setting not supported
						     by hardware
			   io (pointer*) to struct IO
			   pos (byte) array position of value to install
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


int setparity(io, pos)

register IO *io;                        /* pointer to IO structure */
byte pos;                               /* array position of value to write */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(setrambits(io, io->par, pos));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setdatalen

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets the data length.

    CALLING
	SEQUENCE:   int setdatalen(io, pos)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
				   NO_HARDWARE     - setting not supported
						     by hardware
			   io (pointer*) to struct IO
			   pos (byte) array position of value to install
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


int setdatalen(io, pos)

register IO *io;                        /* pointer to IO structure */
byte pos;                               /* array position of value to write */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(setrambits(io, io->dl, pos));
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setstops

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets stop bits.

    CALLING
	SEQUENCE:   int setstops(io, pos)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
				   NO_HARDWARE     - setting not supported
						     by hardware
			   io (pointer*) to struct IO
			   pos (byte) array position of value to install
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


int setstops(io, pos)

register IO *io;                        /* pointer to IO structure */
byte pos;                               /* array position of value to write */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(setrambits(io, io->sb, pos));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setfifocontrol

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets the FIFO control.

    CALLING
	SEQUENCE:   int setfifocontrol(io, pos)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
				   NO_HARDWARE     - setting not supported
						     by hardware
			   io (pointer*) to struct IO
			   pos (byte) array position of value to install
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


int setfifocontrol(io, pos)

register IO *io;                        /* pointer to IO structure */
byte pos;                               /* array position of value to write */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(setrambits(io, io->fcont, pos));
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setfifotrigger

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets the FIFO trigger point.

    CALLING
	SEQUENCE:   int setfifotrigger(io, pos)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
				   NO_HARDWARE     - setting not supported
						     by hardware
			   io (pointer*) to struct IO
			   pos (byte) array position of value to install
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


int setfifotrigger(io, pos)

register IO *io;                        /* pointer to IO structure */
byte pos;                               /* array position of value to write */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(setrambits(io, io->ftrig, pos));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setintvector

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets the interrupt vetor.

    CALLING
	SEQUENCE:   int setintvector(io, pos)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
				   NO_HARDWARE     - setting not supported
						     by hardware
			   io (pointer*) to struct IO
			   pos (byte) array position of value to install
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


int setintvector(io, pos)

register IO *io;                        /* pointer to IO structure */
byte pos;                               /* array position of value to write */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(setrambits(io, io->ivect, pos));
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getparity

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current parity setting.

    CALLING
	SEQUENCE:   int getparity(io)
			Where:
			   return (int) current setting or -1 for error
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

int getparity(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/
   if(io->par == (void *)0)
     return(-1);
   return(io->par->current);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getdl

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current data length setting.

    CALLING
	SEQUENCE:   int getdl(io)
			Where:
			   return (int) current setting or -1 for error
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

int getdl(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/
   if(io->dl == (void *)0)
     return(-1);
   return(io->dl->current);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getstops

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current stop bit setting.

    CALLING
	SEQUENCE:   int getstops(io)
			Where:
			   return (int) current setting or -1 for error
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

int getstops(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   if(io->sb == (void *)0)
     return(-1);
   return(io->sb->current);
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getfifocontrol

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current FIFO control setting.

    CALLING
	SEQUENCE:   int getfifocontrol(io)
			Where:
			   return (int) current setting or -1 for error
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

int getfifocontrol(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   if(io->fcont == (void *)0)
     return(-1);
   return(io->fcont->current);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getfifotrigger

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current FIFO trigger setting.

    CALLING
	SEQUENCE:   int getfifotrigger(io)
			Where:
			   return (int) current setting or -1 for error
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

int getfifotrigger(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   if(io->ftrig == (void *)0)
     return(-1);
   return(io->ftrig->current);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getintvector

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current interrupt vector setting.

    CALLING
	SEQUENCE:   int getintvector(io)
			Where:
			   return (int) current setting or -1 for error
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

int getintvector(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   if(io->ivect == (void *)0)
     return(-1);
   return(io->ivect->current);
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setbaud

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets the baud rate to the array position of the element
                    structure's divisor array.

    CALLING
	SEQUENCE:   int setbaud(io, pos)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
				   NO_HARDWARE     - setting not supported
						     by hardware
			   io (pointer*) to struct IO
			   pos (byte) array position of value to install

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


int setbaud(io, pos)

register IO *io;                        /* pointer to IO structure */
byte pos;                               /* array position of value to install */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   if(pos >= BAUDS)             /* argument range check */
      return(ARGUMENT_RANGE);

   if(io->br == (void *)0)      /* baud rate not supported in hardware */
      return(BAD_FUNCTION);

   if(io->br->divisor[pos][1] == -1 || io->br->divisor[pos][0] == -1)
      return(BAD_FUNCTION);     /* requested baud rate not availible */

   io->br->current = pos;       /* update current baud table position */
   setrambaud(io, io->br, io->br->divisor[pos][1], io->br->divisor[pos][0]);
   return(0);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getbaud

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current baud rate setting.

    CALLING
	SEQUENCE:   int getbaud(io)
			Where:
			   return (int) current setting or -1 for error
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

int getbaud(io)

register IO *io;                        /* pointer to IO structure */

{


/*
    ENTRY POINT OF ROUTINE:
*/
   if(io->br == (void *)0)
     return(-1);
   return(io->br->current);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    set232

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Turns selected output ON/OFF.

    CALLING
	SEQUENCE:   int set232(io, p, state)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
			   io (pointer*) to struct IO
			   p (pointer*) to struct out232
			   state (bool) state to write
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


int set232(io, p, state)

register IO *io;                        /* pointer to IO structure */
struct out232 *p;                       /* pointer to out232 structure */
bool state;                             /* state to write */

{


/*
    ENTRY POINT OF ROUTINE:
*/

   if(state != ON && state != OFF)      /* argument range check */
      return(ARGUMENT_RANGE);

   if(p == (void *)0)                   /* function not supported in hardware */
      return(BAD_FUNCTION);

   io->ramreg[p->ramregnum][0] &= p->rmask;     /* reset bit */
   if(state == ON)
       io->ramreg[p->ramregnum][0] |= ~p->rmask; /* set bit */
   p->current = state;                          /* update current state */
   /*  write out the new value */
   (*io->putbyte)(io->is_ptr->nHandle, io->base_addr + p->offset, io->ramreg[p->ramregnum][0]);
   return(0);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setrts

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets or resets the corresponding RS-232 output pin.

    CALLING
	SEQUENCE:   int setrts(io, state)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
			   io (pointer*) to struct IO
			   state (bool) state to write the output
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


int setrts(io, state)
register IO *io;                        /* pointer to IO structure */
bool state;                             /* state to write the output pin */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(set232(io, io->rts, state));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setdtr

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets or resets the corresponding RS-232 output pin.

    CALLING
	SEQUENCE:   int setdtr(io, state)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
			   io (pointer*) to struct IO
			   state (bool) state to write the output

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


int setdtr(io, state)
register IO *io;                        /* pointer to IO structure */
bool state;                             /* state to write the output pin */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(set232(io, io->dtr, state));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setgpo1

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets or resets the corresponding RS-232 output pin.

    CALLING
	SEQUENCE:   int setgpo1(io, state)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
			   io (pointer*) to struct IO
			   state (bool) state to write the output

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


int setgpo1(io, state)
register IO *io;                        /* pointer to IO structure */
bool state;                             /* state to write the output pin */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(set232(io, io->gpo1, state));
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setgpo2

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets or resets the corresponding RS-232 output pin.

    CALLING
	SEQUENCE:   int setgpo2(io, state)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
			   io (pointer*) to struct IO
			   state (bool) state to write the output
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


int setgpo2(io, state)
register IO *io;                        /* pointer to IO structure */
bool state;                             /* state to write the output pin */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(set232(io, io->gpo2, state));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    sendbrk

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets or resets the corresponding RS-232 output pin.

    CALLING
	SEQUENCE:   int sendbrk(io)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
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


int sendbrk(io)
register IO *io;                        /* pointer to IO structure */

{

/*
    DECLARE MODULES CALLED:
*/

   void p_putc();               /* serial port putchar routine */
   void waitsre();              /* wait for shift register empty */

/*
    ENTRY POINT OF ROUTINE:
*/

   waitsre(io); /* Wait for the transmitter shift register to become empty */
   set232(io, io->brkbit, ON);
   p_putc(io,0);        /* delay must be longer than 1 character time */
   waitsre(io);
   p_putc(io,0);
   waitsre(io); /* Wait for the transmitter shift register to become empty */
   return(set232(io, io->brkbit, OFF));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setloopback

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Sets or resets the loopback test bit.

    CALLING
	SEQUENCE:   int setloopback(io, state)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
			   io (pointer*) to struct IO
			   state (bool) state to write the output
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


int setloopback(io, state)
register IO *io;                        /* pointer to IO structure */
bool state;                             /* state to write the output pin */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return(set232(io, io->loop, state));
}




/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getdtr

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current state of the corresponding RS-232
		    output pin.

    CALLING
	SEQUENCE:   int getdtr(io)
			Where:
			   return (int) current setting or BAD_FUNCTION
					for error
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

int getdtr(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/
   if(io->dtr == (void *)0)
       return(BAD_FUNCTION);

   return(io->dtr->current);
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getrts

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current state of the corresponding RS-232
		    output pin.

    CALLING
	SEQUENCE:   int getrts(io)
			Where:
			   return (int) current setting or BAD_FUNCTION
					for error
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

int getrts(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/
   if(io->rts == (void *)0)
       return(BAD_FUNCTION);

   return(io->rts->current);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getgpo1

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current state of the corresponding RS-232
		    output pin.

    CALLING
	SEQUENCE:   int getgpo1(io)
			Where:
			   return (int) current setting or BAD_FUNCTION
					for error
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

int getgpo1(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/
   if(io->gpo1 == (void *)0)
       return(BAD_FUNCTION);

   return(io->gpo1->current);
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getgpo2

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current state of the corresponding RS-232
		    output pin.

    CALLING
	SEQUENCE:   int getgpo2(io)
			Where:
			   return (int) current setting or BAD_FUNCTION
					for error
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

int getgpo2(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/
   if(io->gpo2 == (void *)0)
       return(BAD_FUNCTION);

   return(io->gpo2->current);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getloopback

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the current state of the loopback bit.

    CALLING
	SEQUENCE:   int getloopback(io)
			Where:
			   return (int) current setting or BAD_FUNCTION
					for error
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

int getloopback(io)

register IO *io;                        /* pointer to IO structure */

{



/*
    ENTRY POINT OF ROUTINE:
*/
   if(io->loop == (void *)0)
       return(BAD_FUNCTION);

   return(io->loop->current);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    stat232

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the status of the RS-232 input.

    CALLING
	SEQUENCE:   int stat232(io, p)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
			   io (pointer*) to struct IO
			   p (pointer*) to struct in232

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

   For polled operation.  Read the status of an RS232 input.
*/


int stat232(io, p)

register IO *io;                        /* pointer to IO structure */
struct in232 *p;                        /* pointer to in232 structure */

{


/*
    ENTRY POINT OF ROUTINE:
*/

   if(p == (void *)0)           /* function not supported in hardware */
      return(BAD_FUNCTION);

   p->current = (*io->getbyte)(io->is_ptr->nHandle, io->base_addr + p->offset) & p->mask;
   return(p->current ? TRUE : FALSE);
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getcts

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Reads the state of the RS-232 input.

    CALLING
	SEQUENCE:   int getcts(io)
			Where:
			   return (int) TRUE/FALSE or
				   BAD_FUNCTION - function not supported
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


int getcts(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return((*io->read232)(io, io->cts));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getdsr

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Reads the state of the RS-232 input.

    CALLING
	SEQUENCE:   int getdsr(io)
			Where:
			   return (int) TRUE/FALSE or
				   BAD_FUNCTION - function not supported
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


int getdsr(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return((*io->read232)(io, io->dsr));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getdcd

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Reads the state of the RS-232 input.

    CALLING
	SEQUENCE:   int getdcd(io)
			Where:
			   return (int) TRUE/FALSE or
				   BAD_FUNCTION - function not supported
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


int getdcd(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return((*io->read232)(io, io->dcd));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    getri

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Reads the state of the RS-232 input.

    CALLING
	SEQUENCE:   int getri(io)
			Where:
			   return (int) TRUE/FALSE or
				   BAD_FUNCTION - function not supported
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


int getri(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   return((*io->read232)(io, io->ri));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    p_open

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       "Opens" a IO.

    CALLING
	SEQUENCE:   IO *p_open(port)
			Where:
			   return (IO*) 0 on error, error code in p_errno
			   port (unsigned) port to open

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

   The current configuration of the I/O port is saved for later restoration.
   The  baud rate, parity, stop bits, and other parameters are set to
   default values.  If successful, a pointer to the IO structure is returned.
   If unsuccessful, a error code for the failure is placed in the global
   variable 'p_errno'.
*/


int p_errno;                            /* GLOBAL VARIABLE ERROR NUMBER */


IO *p_open(port)

unsigned port;                          /* pointer to IO structure */

{

/*
    DECLARE EXTERNAL DATA AREAS:
*/

   extern int p_errno;          /* error code storage */
   extern struct iostatus iolist[];

/*
    DECLARE LOCAL DATA AREAS:
*/

   IO *io;                      /* IO pointer to return */
   byte *l;

/*
    DECLARE MODULES CALLED:
*/
   void p_config();             /* configure UART */
   int setintvector();          /* configure UART interrupt vector */
   int setfifocontrol();        /* configure UART FIFO control */
   int setfifotrigger();        /* configure UART FIFO trigger point */
   int setloopback();           /* configure UART loopback control */
   int setparity();             /* configure UART parity */
   int setstops();              /* configure UART stop bits */
   int setdatalen();            /* configure UART data bits */
   int setbaud();               /* configure UART baud rate */
   int setdtr();                /* configure UART RS-232 DTR signal */
   int setrts();                /* configure UART RS-232 RTS signal */
   int setgpo1();               /* configure UART RS-232 GPO1 signal */
   int setgpo2();               /* configure UART RS-232 GPO2 signal */
   void enableint();            /* enable interrupts */
   void InitSerialBuffers();	/* initialize */

/*
    ENTRY POINT OF ROUTINE:
*/

   if(port >= (unsigned)MAXPORTS)    /* legal port number? */
   {
     p_errno = NOT_FOUND;
     return((void *)0);
   }

   if(iolist[port].typ == (void *)0)    /* IO exists? */
   {
     p_errno = NO_HARDWARE;
     return((void *)0);
   }

   if(iolist[port].openflag != CLOSED)  /* already in use? */
   {
     p_errno = BUSY;
     return((void *)0);
   }

   io = iolist[port].typ;               /* IO pointer from table */
   p_config(io);                        /* configure UART */

   setgpo2(io, io->gpo2->init);         /* configure UART RS-232 GPO2 signal */
   setgpo1(io, io->gpo1->init);         /* configure UART RS-232 GPO1 signal */
   setdtr(io, io->dtr->init);           /* configure UART RS-232 DTR signal */
   setrts(io, io->rts->init);           /* configure UART RS-232 RTS signal */
   setfifocontrol(io, io->fcont->init); /* configure UART FIFO control */
   setfifotrigger(io, io->ftrig->init); /* configure UART FIFO trigger */
   setloopback(io, io->loop->init);     /* configure UART loopback control */
   setparity(io, io->par->init);        /* configure UART parity */
   setstops(io, io->sb->init);          /* configure UART stop bits */
   setdatalen(io, io->dl->init);        /* configure UART data bits */
   setbaud(io, io->br->init);           /* configure UART baud rate */

   iolist[port].openflag = OPEN;        /* mark open */
   io->devnum = (short)port;            /* put port number in IO */

/*
   set up for interrupts
*/

   if((iolist[port].intok == TRUE) && (io->intstat != (void *)0)
	&& (io->intread != (void *)0) && (io->intread232 != (void *)0))
   {
     io->rstat = io->intstat;        /* change polled routine pointers... */
     io->read = io->intread;
     io->read232 = io->intread232;   /* ... to interrupt routine pointers */
     setintvector(io, io->ivect->init); /* configure UART interrupt vector */
     l = (byte *)((io->is_ptr->slotLetter & 0xff) << 8);
     l += (io->devnum & 0xff);
     InitSerialBuffers(io->is_ptr->nHandle, l);
     enableint(io, port);            /* enable interrupts */
     io->intflag = TRUE;
   }
   return(io);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    p_close

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       "Close" a IO port.

    CALLING
	SEQUENCE:   int p_close(io)
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


int p_close(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    DECLARE EXTERNAL DATA AREAS:
*/


   extern struct iostatus iolist[];

/*
    DECLARE LOCAL DATA AREAS:
*/

   int i;

/*
    DECLARE MODULES CALLED:
*/

   void p_restore();            /* restore UART */
   void disableint();           /* disable interrupts */

/*
    ENTRY POINT OF ROUTINE:
*/

   if(io == (void *)0)
     return(NO_HARDWARE);

   for (i = 0; i < MAXPORTS; i++)
   {
     if((io == iolist[i].typ) && (iolist[i].openflag == OPEN))
     {
       iolist[i].openflag = CLOSED;
       break;
     }
   }

   if(i == MAXPORTS)                    /* no match in list */
     return(NO_HARDWARE);

   if(io->intflag == TRUE)
   {
     disableint(io, io->devnum);        /* disable interrupts */
     io->intflag = FALSE;
   }

   p_restore(io);                       /* restore UART */
   io->rstat = io->savrstat;        /* change back to polled... */
   io->read = io->savread;
   io->read232 = io->savread232;   /* ... routine pointers */
   io->devnum = (short)UNUSED;          /* mark port as unused */
   return(0);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    istat232

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Returns the status of the RS-232 inputs during
		    interrupts.

    CALLING
	SEQUENCE:   int istat232(io, p)
			Where:
			   return (int) 0 if successful or
				   ARGUMENT_RANGE  - argument out of range
				   BAD_FUNCTION    - function not supported
			   io (pointer*) to struct IO
			   p (pointer*) to struct in232

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

   When an RS232 interrupt occurs, the Modem Status register is read.
   The bit values read are placed in the individual structures 'current'
   data member for each of the modem status bits.
   When the state of a modem control line is needed, just return the
   'current' member's 0 or 1 value.
*/


int istat232(io, p)

register IO *io;                        /* pointer to IO structure */
struct in232 *p;                        /* pointer to in232 structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   byte c;
   byte *l;

   l = (byte *)((io->is_ptr->slotLetter & 0xff) << 8);
   l += (io->devnum & 0xff);
   c = int_read_232(io->is_ptr->nHandle, l);
   io->cts->current = c & io->cts->mask; /* update CTS */
   io->dsr->current = c & io->dsr->mask; /* update DSR */
   io->dcd->current = c & io->dcd->mask; /* update DCD */
   io->ri->current  = c & io->ri->mask;  /* update RI */
   if(p == (void *)0)           /* function not supported in hardware */
      return(BAD_FUNCTION);

   return(p->current ? TRUE : FALSE);
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    irstat

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Character-ready for interrupt operation.

    CALLING
	SEQUENCE:   byte irstat(io)
			Where:
			   return (byte) buffer empty
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

   The interrupt buffer is empty when the two pointers are equal.
   Status and data for each character are stored in the interrupt buffer.
   Calls to read status return the status value for the next data byte
   in the interrupt buffer.
   Calls to read character return the data byte from the interrupt buffer.
*/


byte irstat(io)

register IO *io;                        /* pointer to IO structure */

{

/*
    ENTRY POINT OF ROUTINE:
*/

   byte *p;
   p = (byte *)((io->is_ptr->slotLetter & 0xff) << 8);
   p += (io->devnum & 0xff);
   return(int_read_stat(io->is_ptr->nHandle, p));
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    iread

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Character read from interrupt buffer.

    CALLING
	SEQUENCE:   byte iread(io)
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

   The interrupt buffer is empty when the two pointers are equal.
   Status and data for each character are stored in the interrupt buffer.
   Calls to read status return the status value for the next data byte
   in the interrupt buffer.
   Calls to read character return the next data byte from the interrupt
   buffer.
*/


byte iread(io)

register IO *io;                        /* pointer to IO structure */

{


/*
    DECLARE LOCAL DATA AREAS:
*/

   byte *p;
/*
    ENTRY POINT OF ROUTINE:
*/

   p = (byte *)((io->is_ptr->slotLetter & 0xff) << 8);
   p += (io->devnum & 0xff);
   return(int_read(io->is_ptr->nHandle, p));
}
