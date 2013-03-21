
#include "../ioscarrier/ioscarrier.h"
#include "ios231.h"

/*
{+D}
    SYSTEM:         Library Software

    MODULE NAME:    rdo231 - read analog output.

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    F.J.M.

    CODED BY:       F.J.M

    ABSTRACT:       This module is used to perform the read output function
                    for the board.

    CALLING
        SEQUENCE:   data = rdo231(ptr,channel);
                    where:
                        data (unsigned short)
                            Value of the analog output data register.
                        ptr (pointer to structure)
                            Pointer to the board memory map structure.
                        channel (unsigned short)
                            Value of the analog output channel number (0-15).

    MODULE TYPE:    void

    I/O RESOURCES:  

    SYSTEM
        RESOURCES:

    MODULES
        CALLED:     

    REVISIONS:      

  DATE    BY        PURPOSE
-------  ----   ------------------------------------------------

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:

    This module is used to perform the read output function
    for the board.  A structure pointer to the board
    memory map, the analog output channel number value, and the
    analog output data value will be passed to this routine.  The routine
    reads the output data register on the board.
*/


word rdo231(c_blk,channel)

struct cblk231 *c_blk;
word channel;               /* analog output channel number */
{

		int i;

/*
    ENTRY POINT OF ROUTINE:
    Read the output data register.
*/

    for( i = 25; i; i--)
    {
      if( input_word(c_blk->nHandle, (word *)&c_blk->brd_ptr->dac_status) & ( 1 << channel ))
          break;
    }

    return( input_word( c_blk->nHandle, (word *)&c_blk->brd_ptr->dac_reg[channel]));
}
