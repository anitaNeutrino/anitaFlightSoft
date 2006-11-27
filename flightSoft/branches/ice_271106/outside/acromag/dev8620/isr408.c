
#include "../carrier/apc8620.h"
#include "../ip408/ip408.h"


#define __KERNEL__
#define MODULE

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/config.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/major.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/vt_kern.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>

/*
{+D}
   SYSTEM:          Acromag IP408 I/O Board

   MODULE NAME:     isr408.c - interrupt handler

   VERSION:         A

   CREATION DATE:   6/8/98

   CODED BY:        BDB

   ABSTRACT:        These routines perform interrupt exception handling
		    for interrupts on the IP408 I/O board.

   CALLING
	SEQUENCE:   This subroutine runs as a result of an interrupt occuring.

   MODULE TYPE:

   I/O RESOURCES:

   SYSTEM
	RESOURCES:

   MODULES
	CALLED:

   REVISIONS:

 DATE      BY       PURPOSE
-------- ----  ------------------------------------------------

{-D}
*/

/*
   MODULES FUNCTIONAL DETAILS:
*/


void isr_408(void* pAddr)

{

/*
	Local data areas
*/

volatile byte type, enable, status;
volatile int  j;                        /* loop control */
volatile word b_mask;                   /* bit interrupt bit mask */
volatile struct map408 *map_ptr;	/* pointer to board memory map */

/*
	Entry point of routine:
	Initialize pointers
*/

	map_ptr = (struct map408 *)pAddr;
	/*  read status of IPACK module */
	type = readb( &map_ptr->type_reg );		/* interrupt type */
 	enable = readb( &map_ptr->en_reg );		/* interrupt enable */
	status = readb( &map_ptr->sts_reg );		/* interrupt status */
/*
printk("\nt=%X e=%X s=%X\n", type, enable, status);
*/
	if( status)					/* non-zero if interrupt pending */
	{
	  /* Check each bit of the status register for a interrupt pending */
	  for( j = 0; j < 8; j++ )	/* prioritization bit 7 to 0 */
  	  {   
	      b_mask = 0x80 >> j;	/* form interrupting mask */

	      if( b_mask & status)	/* non-zero if interrupt pending */
	      {
		  if( b_mask & type)	/* non-zero if COS selected */
		  {
/*
                        Change of State (COS) interrupts are used when the user wants interrupt
                        processing to be done each time the state of the interrupt input signal
                        changes from high to low or from low to high.
                        The user can place a service routine here. 
*/
		  }
		  else			/* LEVEL selected */
	 	  {
/*                      Disable Interrupt (DBI) approach is used when the interrupting input
                        stimulus CANNOT be removed by the interrupt handlers. This is the case
                        when the interrupt handler cannot cause the interrupt input to return
                        to its original state again by communicating with the interrupting
                        device. This may be because communication with the interrupt device is
                        not direct or because the interrupting device cannot respond by
                        clearing its interrupting signal quickly enough.
                        The user may change the polarity setting in the Interrupt Polarity
                        register so that the interrupt occurs ONLY when digital input signal
                        goes to the polarity level selected.
                        The interrupt input point is not going to cause interrupts anymore until
                        interrupts for that point are re-enabled.
                        The user can place a service routine here. 
*/
			enable &= (~b_mask); /* disable this bit */
		  }
	      }
	   } 
           writeb( enable, &map_ptr->en_reg );
           writeb( status, &map_ptr->sts_reg );/* clear pending interrupts */
/*
           In our example we will change the state of the corresponding bit
           in port 1 as a visual indication that the interrupt has been serviced.

!!!!!!!!!  The user can remove our example that the interrupt has been serviced !!!!!!!!!!!
*/
           writeb( status, &map_ptr->out_port[1] );/* write pending info to port 1 to indicate ISR ran */

	}
}




















