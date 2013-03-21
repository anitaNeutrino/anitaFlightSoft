
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
#include "../iosep20x/iosep20x.h"

/*
{+D}
   SYSTEM:          Acromag I/O Board

   MODULE NAME:     iosep20x.c - interrupt handler

   VERSION:         B

   CREATION DATE:   04/01/09

   CODED BY:        FJM

   ABSTRACT:        This routine performs interrupt exception handling
                    for interrupts on the I/O board.

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
01/23/13 FJM   Fedora 18 update, removed include of asm/system.h.

{-D}
*/

/*
   MODULES FUNCTIONAL DETAILS:
*/


void iosep20xisr(void* pAddr)
{

/*
	Local data areas
*/

volatile word type, enable, int_status;
volatile int  j;                        /* loop control */
volatile word b_mask;                   /* bit interrupt bit mask */
volatile struct handler_data *hdp;      /* pointer to handler data structure */
volatile struct isr_data *idp;          /* pointer to ISR data structure */
volatile struct mapep20x *map_ptr;      /* pointer to board I/O map */
/* volatile char *mem_ptr;              / * pointer to board mem map (if supported) */

/*
	Entry point of routine:
*/

	/*  Initialize pointers */
	hdp = (struct handler_data *)pAddr; /* get pointer to handler data structure */

	idp = (struct isr_data *)hdp->hd_ptr; /* get pointer to isr data structure */

	map_ptr = (struct mapep20x *)idp->slot_io_address; /* get I/O address */

	/* not used by this ISR... just an example of how to get at it */
/*	mem_ptr = (char *)idp->slot_mem_address; / * get mem address */

	type = readw( &map_ptr->type_reg );		/* interrupt type */
 	enable = readw( &map_ptr->en_reg );		/* interrupt enable */
	int_status = readw( &map_ptr->sts_reg );/* interrupt status */

printk("IOSEP20x ISR a=%lX t=%X e=%X s=%X\n", (unsigned long)map_ptr, type, enable, int_status);

	if( int_status )						/* non-zero if interrupt pending */
	{
	    /* Check each bit of the status register for a interrupt pending */
	    for( j = 0; j < 8; j++ )		/* prioritization bit 7 to 0 */
  	    {   
	        b_mask = 0x80 >> j;			/* form interrupting mask */
	        if( b_mask & int_status )	/* non-zero if interrupt pending */
	        {
		     if( b_mask & type )		/* non-zero if COS selected */
		     {  
				/* nothing for COS now */
		     }
		     else				     /* LEVEL selected */
		     {
			  enable &= (~b_mask); /* disable this bit */
		     }
			}
	    }
        writew( enable, &map_ptr->en_reg );
        writew( int_status, &map_ptr->sts_reg );/* clear pending interrupts */
	}
}
