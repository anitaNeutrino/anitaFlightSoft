
#include "../ioscarrier/ioscarrier.h"
#include "ios320.h"

/*
{+D}
    SYSTEM:         Library Software

    FILENAME:       rsts320.c

    MODULE NAME:    rsts320 - read status of board

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    F.J.M

    CODED BY:       F.J.M.
    
    ABSTRACT:       This module is used to read status of the board.

    CALLING
        SEQUENCE:   rsts320(ptr);
                    where:
                        ptr (pointer to structure)
                            Pointer to the status block structure.

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

    This module is used to perform the read status function
    for the board.  A pointer to the Status Block will
    be passed to this routine.  The routine will use a pointer
    within the Status Block together with offsets
    to reference the registers on the Board and will transfer the 
    status information from the Board to the Status Block.
*/



void rsts320(c_blk)
struct cblk320 *c_blk;
{
/*
    declare local storage
*/

    int i;                      /* loop index */
    word idProm[12];		/* holds ID prom */

/*
    ENTRY POINT OF ROUTINE:
    read register information
*/

    c_blk->control = input_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->cntl_reg );


    ReadIOSID(c_blk->nHandle, c_blk->slotLetter, &idProm[0], sizeof idProm / 2); /* read from carrier */

    for(i = 0; i < 12; i++ )
 	   c_blk->id_prom[i] = (byte)idProm[i];
}
