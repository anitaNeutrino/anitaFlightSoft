
#ifndef BUILDING_FOR_KERNEL
#define BUILDING_FOR_KERNEL	/* controls conditional inclusion in file carrier.h */
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
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/uaccess.h>


#include "../carrier/carrier.h"
#include "../ip570/ip570.h"

/*
{+D}
   SYSTEM:          Acromag Board

   MODULE NAME:     isr_570 - 1553 interrupt handler

   VERSION:         C

   CREATION DATE:   04/19/10

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
04/01/11 FJM   Changed carrier include to carrier.h
01/23/13 FJM   Fedora 18 update, removed include of asm/system.h.

{-D}
*/

/*
   MODULES FUNCTIONAL DETAILS:
*/



void isr_57x(void* pData)

{

volatile struct handler_data *hdp;              /* pointer to handler data structure */
volatile struct isr_data *idp;                  /* pointer to ISR data structure */
volatile struct map570 *brd_ptr;                /* base address of the board I/O space */
volatile char *mem_ptr;                         /* pointer to board mem map */
volatile word wISR1, wISR2, ipend;

extern wait_queue_head_t apWaitQueue[32];	/* wait queues for 57X modules */
extern int abWaitQueueFlag[32];			/* wait queues flags for 57X modules */
extern uint32_t adwIrqStatus[32];		/* individual device status */

/*
	Entry point of routine:
	Initialize pointers
*/

	hdp = (struct handler_data *)pData; /* get pointer to handler data structure */
	idp = (struct isr_data *)hdp->hd_ptr; /* get pointer to isr data structure */

	brd_ptr = (struct map570 *)idp->slot_io_address; /* get I/O address */
	mem_ptr = (char *)idp->slot_mem_address; /* get mem address */

	/* get the interrupt pending status for both channels */
	ipend = readw(&brd_ptr->StatusReg);

	if( ipend & 1)            /* determine if channel 0 has an interrupt pending */
	{
           /* Read Ch0 interrupt mask registers */
	   wISR2 = readw(&mem_ptr[0x3C]); /* interrupt status register 2 */
	   wISR1 = readw(&mem_ptr[0x0C]); /* interrupt status register 1 */
	   adwIrqStatus[idp->dev_num[0]] = ((wISR2 & 0xFFFF) << 16)|( wISR1 & 0xFFFF);

/*printk("IP57x ISR Ch0 a=%lX I= %X S=%lX\n", (unsigned long)brd_ptr,idp->dev_num[0],adwIrqStatus[idp->dev_num[0]]);*/
	   /* Wake the thread blocked in 'BLOCK_IN_DRIVER' */
	   abWaitQueueFlag[idp->dev_num[0]] = 1;   /* indicate  ISR wake-up */
	   wake_up_interruptible( &apWaitQueue[idp->dev_num[0]] );

	   /* Read 'config register 2' check if the autoclear feature is not enabled */
	   if((readw(&mem_ptr[0x00004]) & 0x0010 ) == 0)
	   {
		/* If it is 0, clear the interrupt reset bit in the start/reset register */
		writew( (int)0x0004, (void *)(char*)&mem_ptr[0x00006]);
	   }
	}

	if( ipend & 256)          /* determine if channel 1 has an interrupt pending */
	{
	   /* Read Ch1 interrupt mask registers */
	   wISR2 = readw(&mem_ptr[0x4003C]); /* interrupt status register 2 */
	   wISR1 = readw(&mem_ptr[0x4000C]); /* interrupt status register 1 */
	   adwIrqStatus[idp->dev_num[1]] = ((wISR2 & 0xFFFF) << 16)|( wISR1 & 0xFFFF);

/*printk("IP57x ISR Ch1 a=%lX I= %X S=%lX\n", (unsigned long)brd_ptr,idp->dev_num[1],adwIrqStatus[idp->dev_num[1]]);*/
	   /* Wake the thread blocked in 'BLOCK_IN_DRIVER' */
	   abWaitQueueFlag[idp->dev_num[1]] = 1;
	   wake_up_interruptible( &apWaitQueue[idp->dev_num[1]] );

	   /* Read 'config register 2' check if the autoclear feature is not enabled */
	   if((readw(&mem_ptr[0x40004]) & 0x0010 ) == 0)
	   {
		/* If it is 0, clear the interrupt reset bit in the start/reset register */
		writew( (int)0x0004, (void *)(char*)&mem_ptr[0x40006]);
	   }
	}
}

