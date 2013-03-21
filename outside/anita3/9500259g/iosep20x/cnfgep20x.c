
#include "../ioscarrier/ioscarrier.h"
#include "iosep20x.h"

/*
{+D}
    SYSTEM:         Library Software

    MODULE NAME:    cnfgep20x - configure ep20x board

    VERSION:        A

    CREATION DATE:  04/01/09

    CODED BY:	    FJM

    ABSTRACT:	    This module is used to perform the configure function
                    for the board.

    CALLING
	SEQUENCE:	    cnfgep20x(ptr);

			    where:
				ptr (pointer to structure)
				    Pointer to the configuration block structure.

    MODULE TYPE:      void

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

    This module is used to perform the "configure board" function
    for the board.  A pointer to the Configuration Block will
    be passed to this routine.  The routine will use a pointer
    within the Configuration Block to reference the registers
    on the Board.  Based on flag bits in the Attribute and
    Parameter Flag words in the Configuration Block, the board
    will be configured and various registers will be updated with
    new information which will be transfered from the Configuration
    Block to registers on the Board.
*/



void cnfgep20x(c_blk)

struct cblkep20x *c_blk;

{

/*
    declare local storage
*/


word control;

/*
    ENTRY POINT OF ROUTINE:
*/

/*
    If reset is selected, then do so
*/
    if(c_blk->param & RESET && c_blk->reset )
	    output_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->control, (word)0x8000);

/*
    Check to see if the Interrupt Vector Register is to be updated.
    Update the Vector Register before enabling Global Interrupts so that
    the board will always output the correct vectors upon interrupt.
*/
    if(c_blk->param & VECT )
	    output_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->int_vect, (word)c_blk->vector);

/*  Select model */

    if(c_blk->param & SEL_MODEL )
    {
       control = input_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->control);
       control &= 0xF8FF;       /* mask out model bits */
       switch( c_blk->model )
       {
          case 1:	/* iosep202/4 */
            control |= 0x0100;
          break;

          case 2:	/* iosep203 */
            control |= 0x0400;
          break; 
          
          case 3:	/* iosep201 */
            control |= 0x0700;
          break; 
          
          default:	/* leave it unselected */
          break;
        }
	output_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->control, control);
    }


/*
    If the Direction Register is to be updated, then update it.
*/

    if(c_blk->param & DIR)
            output_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->dir_reg, c_blk->direction);

/*
    If the Type Select Register is to be updated, then update it.
*/

    if(c_blk->param & INT_TYPE)
	    output_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->type_reg, c_blk->type);

/*
    If the Interrupt Polarity Register is to be updated . . .
*/

    if(c_blk->param & INT_POLARITY)
		output_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->pol_reg, c_blk->polarity);

/*
    If the Interrupt Status Register is to be updated, then update it.
*/

    if(c_blk->param & INT_STATUS)
	    output_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->sts_reg, c_blk->int_status);

/*
    If the Interrupt Enable Register is to be updated then do so.
*/

    if(c_blk->param & INT_ENAB)
	    output_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->en_reg, c_blk->enable);
}


