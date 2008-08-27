#include "ip470.h"


/*
{+D}
   SYSTEM:          Acromag I/O Board

   MODULE NAME:     isr_470.c - interrupt handler

   VERSION:         A

   CREATION DATE:  10/30/95

   CODED BY:        FM

   ABSTRACT:        These routines perform interrupt exception handling
		    for interrupts on the IP470 I/O board.

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

   For level operation, each input bit must be connected to a switch input. 
   
   The polarity of input bits is controlled in nibbles by a single bit in 
   the Event Control Register.    If this bit is a '1' positive edeges are
   sensed.  If this bit is a '0' negative edeges are sensed.
   
   Each time an input changes to the state programmed into the sense register
   an interrupt is generated.
*/




void isr_470(void* pData)

{

/*
	Local data areas
*/

struct handler_data* hData;   /*  Interrupt handler structure */
struct cblk470* cblk;	      /*  configuration block */

int i;		                /* loop control */
byte saved_bank;                /* saved bank value */
byte i_stat;                    /* interrupt status */
byte p_mask;                    /* port interrupt bit mask */

/*
	Entry point of routine:
*/

    hData = (struct handler_data*)pData;
    cblk = (struct cblk470*)hData->hd_ptr;
    saved_bank = bank_select(BANK1, cblk); /* set & save bank select */
        
    i_stat = input_byte((byte*)&cblk->brd_ptr->port[6].b_select); /* interrupt status */
    for(i = 0; i < 6; i++)      /* check ports 0-5 for an interrupt pending */
    {
      p_mask = ( 1 << i);       /* form port interrupt bit mask */
      if((p_mask & i_stat) != 0) /* port with interrupt pending? */
      {

        cblk->event_status[i] |= input_byte((byte*)&cblk->brd_ptr->port[i].b_select); /* interrupt sense */

	/* write 0s to clear the interrupting bits */
        output_byte((byte*)&cblk->brd_ptr->port[i].b_select,(~(cblk->event_status[i]))); /* write 0 to clear the interrupting bit */

/*
   User code can be added here to complete the interrupt service routine

           Example:   user_function();
*/

        output_byte((byte*)&cblk->brd_ptr->port[i].b_select, 0xFF); /* re-enable sense inputs */ 
      }
    }

    bank_select(saved_bank, cblk); /* restore bank select */
}




