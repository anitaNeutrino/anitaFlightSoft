
#include "../carrier/apc8620.h"
#include "ip340.h"

/*
{+D}

    SYSTEM:	    Library Software - ip340 Board

    MODULE NAME:    move_data ip340

    VERSION:	    V1.0

    CREATION DATE:  11/29/95

    CODED BY:	    F.J.M.

    ABSTRACT:	    This acquires the input conversions for the ip340 board.

    CALLING
	SEQUENCE:   move_data(ptr);
		    where:
			ptr (pointer to structure)
			    Pointer to the configuration block structure.

    MODULE TYPE:    int

    I/O RESOURCES:

    SYSTEM
	RESOURCES:

    MODULES
	CALLED:

    REVISIONS:

  DATE	  BY	    PURPOSE
-------  ----	------------------------------------------------

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:
*/



void move_data(c_blk)


struct cblk340 *c_blk;

{


/*
    declare local storage
*/

    int i;
    sint temp_data;

/*
    ENTRY POINT OF ROUTINE:
*/


/*  Move data to array */

	for( i = 0; i < 16; i++)
		c_blk->s_count[i] = 0;

      while((input_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->cntl_reg) & 0x100) != 0)  /* while FIFO not empty get data */
      {
        temp_data = (sint)input_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->fifo_data);  /* must read data first then the channel tag */

        i = (sint)input_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->chan_tag) & 0xF;  /* get channel number for data */

        if(c_blk->s_count[i] > SA_SIZE - 1 )
	     c_blk->s_count[i] = 0;

        c_blk->s_raw_buf[i][c_blk->s_count[i]++] = temp_data;
      }
}




/*
{+D}
   SYSTEM:	    Acromag Input Board

   MODULE NAME:     isr_340.c - interrupt handler

   VERSION:	    A

   CREATION DATE:  11/08/95

   CODED BY:	    FM

   ABSTRACT:	    These routines perform interrupt exception handling
			    for interrupts on the IP340 Input board.

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


void isr_340(void* pData)

{


/*
	Local data areas
*/

struct handler_data* hData;   /*  Interrupt handler structure */
struct cblk340* cblk;	      /*  configuration block */

/*
	Entry point of routine:
*/

	/*  Initialize pointers */
	hData = (struct handler_data*)pData;
	cblk = (struct cblk340*)hData->hd_ptr;

/*
        Move data
*/

	move_data(cblk);
}




