
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
#include "../ios560/basicCAN/ios560b.h"

/*
{+D}
   SYSTEM:          Acromag Board

   MODULE NAME:     ios560bisr - basicCAN interrupt handler

   VERSION:         B

   CREATION DATE:   03/19/10

   CODED BY:        FJM

   ABSTRACT:        These routines perform interrupt exception handling
					for interrupts on the board.

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


void ios560bisr(void* pData)

{

volatile struct handler_data *hdp;              /* pointer to handler data structure */
volatile struct isr_data *idp;                  /* pointer to ISR data structure */
volatile struct mapmem560b *mem_ptr;            /* pointer to board memory map */
volatile struct map560b *brd_ptr;               /* base address of the board I/O space */
volatile char ipend, i_stat;

/*
	Entry point of routine:
	Initialize pointers
*/

	hdp = (struct handler_data *)pData; /* get pointer to handler data structure */

	idp = (struct isr_data *)hdp->hd_ptr; /* get pointer to isr data structure */

	mem_ptr = (struct mapmem560b *)idp->slot_mem_address; /* get memory address */
	brd_ptr = (struct map560b *)idp->slot_io_address; /* get I/O address */
	
	/* get the interrupt pending status for both channels */
	ipend = readb(&brd_ptr->StatusRegister);

	if( ipend & 1)              /* determine if channel 0 has an interrupt pending */
	{
	  i_stat = readb(&mem_ptr->can_ir); /* interrupt status  read clears the interrupt flags */
printk("\nIOS560b ISR Ch0 a=%lX s=%02X\n", (unsigned long)mem_ptr, i_stat & 0xff );
	}

	if( ipend & 4)              /* determine if channel 1 has an interrupt pending */
	{
	  i_stat = readb(&mem_ptr->can_ir + 0x200); /* interrupt status  read clears the interrupt flags */
printk("\nIOS560b ISR Ch1 a=%lX s=%02X\n", (unsigned long)mem_ptr, i_stat  & 0xff );
	}
}





#include "../ios560/peliCAN/ios560p.h"

/*
{+D}
   SYSTEM:          Acromag Board

   MODULE NAME:     ios560pisr - peliCAN interrupt handler

   VERSION:         A

   CREATION DATE:   03/19/10

   CODED BY:        FJM

   ABSTRACT:        These routines perform interrupt exception handling
					for interrupts on the board.

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


void ios560pisr(void* pData)

{

volatile struct handler_data *hdp;              /* pointer to handler data structure */
volatile struct isr_data *idp;                  /* pointer to ISR data structure */
volatile char *mem_ptr;                         /* pointer to board memory map */
volatile char *brd_ptr;                         /* base address of the board I/O space */
volatile char ipend, i_stat;

/*
	Entry point of routine:
	Initialize pointers
*/

	hdp = (struct handler_data *)pData; /* get pointer to handler data structure */

	idp = (struct isr_data *)hdp->hd_ptr; /* get pointer to isr data structure */

	mem_ptr = (char *)idp->slot_mem_address; /* get memory address */
	brd_ptr = (char *)idp->slot_io_address; /* get I/O address */
	
	/* get the interrupt pending status for both channels */
	ipend = readb( brd_ptr + StatusRegister);

	if( ipend & 1)              /* determine if channel 0 has an interrupt pending */
	{
	  i_stat = readb(mem_ptr + 0 + CAN_IR); /* interrupt status  read clears the interrupt flags */
printk("\nIOS560p ISR Ch0 a=%lX s=%02X\n", (unsigned long)mem_ptr, i_stat & 0xff );
	}

	if( ipend & 4)              /* determine if channel 1 has an interrupt pending */
	{
	  i_stat = readb(mem_ptr + 0x200 + CAN_IR); /* interrupt status  read clears the interrupt flags */
printk("\nIOS560p ISR Ch1 a=%lX s=%02X\n", (unsigned long)mem_ptr, i_stat & 0xff );
	}
}
