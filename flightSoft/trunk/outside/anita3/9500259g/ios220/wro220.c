
#include "../ioscarrier/ioscarrier.h"
#include "ios220.h"

/*
{+D}
    SYSTEM:         Library Software - ios220

    FILENAME:       wro220.c

    MODULE NAME:    wro220 - write analog output.

    VERSION:        Rev A

    CREATION DATE:  04/01/09

    DESIGNED BY:    F.J.M.

    CODED BY:       F.J.M

    ABSTRACT:       This module is used to perform the write output function.

    CALLING
        SEQUENCE:   wro220(ptr,channel,data);
                    where:
        		ptr (pointer to structure)
		            Pointer to the configuration block structure.
                        channel (unsigned short)
                            Value of the analog output channel number (0-7).
                        data (unsigned short)
                            Value of the analog output data.

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

    This module is used to perform the write output function.
    A structure pointer to the board
    memory map, the analog output channel number value, and the
    analog output data value will be passed to this routine.  The routine
    writes the output data to the analog output channel register on
    the board.
*/


void wro220(c_blk,channel,data)

struct cblk220 *c_blk;
word channel;               /* analog output channel number */
word data;                  /* data to write to the output channel */
{


/*
    ENTRY POINT OF ROUTINE:
    Write the output data to the output channel on the board.
*/

    output_word( c_blk->nHandle, (word *)&c_blk->brd_ptr->dac_reg[channel], data );
}
