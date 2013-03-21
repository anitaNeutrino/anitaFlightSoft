
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
#include "../ios330/ios330.h"

/*
{+D}
   SYSTEM:          Acromag IOS Board

   MODULE NAME:     ios330isr.c - interrupt handler

   VERSION:         B

   CREATION DATE:   04/01/09

   CODED BY:        FJM

   ABSTRACT:        These routines perform interrupt exception handling
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


void ios330isr(void* pAddr)

{

volatile struct handler_data *hdp;		/* pointer to handler data structure */
volatile struct isr_data *idp;			/* pointer to ISR data structure */
volatile char *map_ptr;					/* pointer to board I/O map */

/*
	Entry point of routine:
	Initialize pointers
*/

	hdp = (struct handler_data *)pAddr; /* get pointer to handler data structure */

	idp = (struct isr_data *)hdp->hd_ptr; /* get pointer to isr data structure */

	map_ptr = (char *)idp->slot_io_address; /* get I/O address */

printk("IOS330 ISR a=%lX\n", (unsigned long)map_ptr);

}
