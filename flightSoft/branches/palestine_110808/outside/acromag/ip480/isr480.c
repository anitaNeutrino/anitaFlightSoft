#include "ip480.h"


/*
{+D}
   SYSTEM:	    Acromag Input Board

   MODULE NAME:     isr_480.c - interrupt handler

   VERSION:	    A

   CREATION DATE:   05/19/98

   CODED BY:	    FM

   ABSTRACT:	    This routine performs interrupt exception handling
                    for the IP480 Input board.

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


void isr_480(void* pData)

{

/*
	Local data areas
*/

int i,j;
UWORD i_stat;
struct handler_data* hData;   /*  Interrupt handler structure */
struct cblk480* cblk;	      /*  configuration block */

/*
	Entry point of routine:
*/

  hData = (struct handler_data*)pData;
  cblk = (struct cblk480*)hData->hd_ptr;

  i_stat = input_word( (word *)(cblk->brd_ptr + InterruptPending) );
  if(cblk->num_chan == 2)	/* check if it's a 2 or 6 channel board */
  {
    i_stat &= 0x0300;		/* and off the unused upper bits */
    j = 2;
  }
  else
  {
    i_stat &= 0x3F00;		/* and off the unused bits and save the useful ones */
    j = 6;
  }


  if( i_stat )			/* any interrupt(s) pending? */
  {
    cblk->event_status |= (byte)(i_stat >> 8 ); /* update event status */

    /* check each bit for an interrupt pending state */
    for( i = 0; i < j; i++ )			/* check each counter timer */
    {
	  if( i_stat & (1 << (i + 8)) )		/* build interrupt clear word */
		i_stat &= (~(1 << (i + 8)));	/* clear interrupting bit */
	  else
		i_stat |= (1 << (i + 8));	/* set bit to ignore */
    }
    output_word((word *)(cblk->brd_ptr + InterruptPending), i_stat); /* write interrupt pending */
  }
}

