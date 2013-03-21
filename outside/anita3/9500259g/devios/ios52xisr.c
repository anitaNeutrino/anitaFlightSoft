
#ifndef BUILDING_FOR_KERNEL
#define BUILDING_FOR_KERNEL	/* controls conditional inclusion in file ioscarrier.h */
#endif


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/uaccess.h>


#include "../ioscarrier/ioscarrier.h"
#include "../ios520/ios520.h"
#include "../ios521/ios521.h"


#define RCV_MASK         1      /* RxRDY mask */
#define XMIT_MASK     0x20      /* TxRDY mask */
#define TSRE_MASK     0x40      /* transmitter shift register empty mask */


#define BUFLENGTH 2048			/* 1/2 is status 1/2 is data */
struct							/* Interrupt buffer management variables */
{
   unsigned short InputBuffer[8][BUFLENGTH];
   unsigned short *Intbuff[8];	/* start of buffer */
   unsigned short *Eoibuff[8];	/* end of buffer */
   unsigned short *Ihead[8];
   unsigned short *Itail[8];
   unsigned char ModemStatus[8];
}IB[4];




/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    kiread

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Initialize interrupt buffer management variables.

    CALLING
        SEQUENCE:   void InitBuffer(int carrier, int ios_slot, int port)
                        Where:
                           int carrier,	 not used
						   int ios_slot,			 carrier slot letter A, B, C, D
						   int port,				 Serial port number 0 - 7
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


unsigned char InitBuffer(int carrier, int ios_slot, int port)
{
   unsigned long flags;

   ios_slot -= SLOT_A;
   local_irq_save(flags);
   memset( &IB[ios_slot].InputBuffer[port], 0, sizeof(IB[ios_slot].InputBuffer[port]));
   IB[ios_slot].ModemStatus[port] = 0;
   IB[ios_slot].Intbuff[port] = &IB[ios_slot].InputBuffer[port][0];
   IB[ios_slot].Eoibuff[port] = &IB[ios_slot].InputBuffer[port][BUFLENGTH];
   IB[ios_slot].Ihead[port] = IB[ios_slot].Itail[port] = IB[ios_slot].Intbuff[port];
   local_irq_restore(flags);
   return((unsigned char)0);
}

/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    kiread

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Read character from interrupt buffer.

    CALLING
        SEQUENCE:   void kiread(int carrier, int ios_slot, int port)
                        Where:
                           volatile char *map_ptr,	 not used
						   int ios_slot,			 carrier slot letter A, B, C, D
						   int port,				 Serial port number 0 - 7
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

unsigned char kiread(int carrier, int ios_slot, int port)
{
   unsigned char c;

   ios_slot -= SLOT_A;						   /* convert ios slot to index */
   c = (unsigned char)*IB[ios_slot].Itail[port];		   /* get from buffer at pointer */

   if((++IB[ios_slot].Itail[port]) == IB[ios_slot].Eoibuff[port])  /* if at end of buffer */
     IB[ios_slot].Itail[port] = IB[ios_slot].Intbuff[port];        /* reset pointer */

   return(c);
}

/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    kistat

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Read status of the character in the interrupt buffer.

    CALLING
        SEQUENCE:   void kistat(int carrier, int ios_slot, int port)
                        Where:
                           volatile char *map_ptr,	 not used
						   int ios_slot,			 carrier slot letter A, B, C, D
						   int port,				 Serial port number 0 - 7
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


unsigned char kirstat(int carrier, int ios_slot, int port)
{
   unsigned char c;

   c = 0;
   ios_slot -= SLOT_A;			/* convert ios slot to index */
   if((IB[ios_slot].Ihead[port]) != IB[ios_slot].Itail[port])
     c = (unsigned char) ((*IB[ios_slot].Itail[port] >> 8) & RCV_MASK);

   return(c);
}

/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    kistat232

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Read status of the RS232 control lines.

    CALLING
        SEQUENCE:   void kistat232(int carrier, int ios_slot, int port)
                        Where:
                           volatile char *map_ptr,	 not used
						   int ios_slot,			 carrier slot letter A, B, C, D
						   int port,				 Serial port number 0 - 7
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

unsigned char kistat232(int carrier, int ios_slot, int port)
{
    ios_slot -= SLOT_A;			/* convert ios slot to index */
   return(IB[ios_slot].ModemStatus[port]);
}




/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    irxrdy

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Byte-read interrupt subhandler for RxRDY interrupts.

    CALLING
        SEQUENCE:   void irxrdy(volatile char *map_ptr, int ios_slot, int port)
                        Where:
                           volatile char *map_ptr,	 not used
						   int ios_slot,			 carrier slot letter A, B, C, D
						   int port,				 Serial port number 0 - 7
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

   Status and data for each character are stored in the interrupt buffer.
   The Line Status register is read and placed in the high 8 bits of the
   interrupt data buffer. The Data register is read and placed in the low
   8 bits of the interrupt data buffer at 'ihead' and the buffer pointer
   is incremented.  If 'ihead' points past the end of the buffer, it is
   reset to point to the start of the buffer.
   The interrupt buffer is empty when the two pointers are equal.
 */



void irxrdy(volatile char *map_ptr, int ios_slot, int port)

{

/*
    DECLARE LOCAL DATA AREAS:
*/

   word tmp;
   int i;
/*
    ENTRY POINT OF ROUTINE:
*/

   for( i = 0; i < 8; i++)
   {
     while(1)
     {
       /* get receiver status in tmp */
       tmp = (word)readb( &map_ptr[(i << 4)+LINE_STAT] );
     
       /* check receiver ready exit if data not available */
       if((tmp & RCV_MASK) == FALSE)
           break;

       /* read the receiver data and combine status and data in buffer */
       *IB[ios_slot].Ihead[i] = (word)(tmp << 8) | (word)readb( &map_ptr[(i << 4)+TX_RX_REG]);  /* read the byte */

       if((++IB[ios_slot].Ihead[i]) == IB[ios_slot].Eoibuff[i])	/* if at end of buffer */
        IB[ios_slot].Ihead[i] = IB[ios_slot].Intbuff[i];	/* reset pointer */
     }
   }
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    irs232

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Byte-read interrupt handler for RS232 interrupts.

    CALLING
        SEQUENCE:   void irs232(volatile char *map_ptr, int ios_slot, int port)
                        Where:
                           volatile char *map_ptr,	 not used
						   int ios_slot,			 carrier slot letter A, B, C, D
						   int port,				 Serial port number 0 - 7
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
*/


void irs232(volatile char *map_ptr, int ios_slot, int port)

{

/*
    ENTRY POINT OF ROUTINE:
*/
   /* read UART status */
   IB[ios_slot].ModemStatus[port] = readb( &map_ptr[(port << 4)+STAT_232]);
}







/*
{+D}
   SYSTEM:          Acromag IOS Board

   MODULE NAME:     ios520isr.c - interrupt handler

   VERSION:         B

   CREATION DATE:   04/01/09

   CODED BY:        FJM

   ABSTRACT:        These routines perform interrupt exception handling
                    for interrupts on the I/O board.

   CALLING
	SEQUENCE:       This subroutine runs as a result of an interrupt occuring.

   MODULE TYPE:

   I/O RESOURCES:

   SYSTEM
	RESOURCES:

   MODULES
	CALLED:

   REVISIONS:

 DATE      BY       PURPOSE
-------- ----  ------------------------------------------------
01/23/13 FJM   Fedora 18 update, removed include of asm/system.h.

{-D}
*/

/*
   MODULES FUNCTIONAL DETAILS:
*/


/*
    Dummies for other interrupts
*/


void itbe(volatile char *map_ptr, int ios_slot, int port) { /* cleared by reading INT_ID */ }

void iserr(volatile char *map_ptr, int ios_slot, int port) { readb( &map_ptr[(port << 4)+LINE_STAT]);  /* read the status */
                                                             irxrdy(map_ptr, ios_slot, port);}  /* and the character*/





void ios52xisr(void* pAddr)

{

volatile struct handler_data *hdp;          /* pointer to handler data structure */
volatile struct isr_data *idp;              /* pointer to ISR data structure */
volatile char *map_ptr;                     /* pointer to board I/O map */
int port;
unsigned irr_index;
unsigned long ios_slot;

/*
	Entry point of routine:
	Initialize pointers
*/

    hdp = (struct handler_data *)pAddr;     /* get pointer to handler data structure */
    idp = (struct isr_data *)hdp->hd_ptr;   /* get pointer to isr data structure */
    map_ptr = (char *)idp->slot_io_address; /* get I/O address */
    ios_slot =(unsigned long)idp->slot_letter;
/*
printk("IOS52x ISR %lX %c\n", (unsigned long)map_ptr, (unsigned char)ios_slot);
*/
    ios_slot -= SLOT_A;                     /* convert ios slot to index */
    for( port = 0; port < 8; port++)        /* loop thru checking all ports for an interrupt pending */
    {
      while(1)                              /* process all interrupts from this device */
      {
       irr_index = (unsigned)readb( &map_ptr[(port << 4)+INT_ID] );	/* interrupt ID */

       if(irr_index & 1)                    /* exit when all inerrupts from this device are processed */
         break;

       irr_index &= 0x3F;                   /* strip bits 6 & 7 */
       irr_index >>= 1;                     /* form numeric index to interrupt handler functions */     

       switch(irr_index)
       {
        case 2:
        case 6:
          irxrdy(map_ptr, ios_slot, port);  /* code in IRR >> 1 = 2 or code in IRR >> 1 = 6 */
        break;
        case 0:
          irs232(map_ptr, ios_slot, port);  /* code in IRR >> 1 = 0 */
        break;
        case 3:
          iserr(map_ptr, ios_slot, port);   /* code in IRR >> 1 = 3 */
        break;
        case 1:
          itbe(map_ptr, ios_slot, port);    /* code in IRR >> 1 = 1 */
        break;

       }
      }
    }
}

