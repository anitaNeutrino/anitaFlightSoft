#include "ip330.h"

/*
{+D}
   SYSTEM:	    Acromag Input Board

   MODULE NAME:     isr_330.c - interrupt handler

   VERSION:	    A

   CREATION DATE:  11/08/95

   CODED BY:	    FM

   ABSTRACT:	    These routines perform interrupt exception handling
		    for interrupts on the IP330 Input board.

   CALLING
	SEQUENCE:   This subroutine runs as a result of an interrupt occuring.

   MODULE TYPE:

   I/O RESOURCES:

   SYSTEM
	RESOURCES:

   MODULES
	CALLED:

   REVISIONS:

 DATE	   BY	    PURPOSE
-------- ----  ------------------------------------------------

{-D}
*/

/*
   MODULES FUNCTIONAL DETAILS:
*/


void isr_330(void* pData)

{


/*
	Local data areas
*/

int  i;     	              /* loop control */
struct handler_data* hData;   /*  Interrupt handler structure */
struct cblk330* cblk;	      /*  configuration block */

/*
	Entry point of routine:
*/

	/*  Initialize pointers */
	hData = (struct handler_data*)pData;
	cblk = (struct cblk330*)hData->hd_ptr;

/*
        Move data flags and data to the scan array
*/

      cblk->missed_data[0] = input_word((word*)&cblk->brd_ptr->mdr_reg[0]); /* copy missed data */
      cblk->missed_data[1] = input_word((word*)&cblk->brd_ptr->mdr_reg[1]);

      cblk->new_data[0] = input_word((word*)&cblk->brd_ptr->ndr_reg[0]); /* copy new data flags */
      cblk->new_data[1] = input_word((word*)&cblk->brd_ptr->ndr_reg[1]);

      for( i = cblk->s_channel; i < 32; i++)    /* move data */
         cblk->s_raw_buf[i] = input_word((word*)&cblk->brd_ptr->data[i]);
}

