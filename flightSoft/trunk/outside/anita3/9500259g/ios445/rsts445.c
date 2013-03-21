
#include "../ioscarrier/ioscarrier.h"
#include "ios445.h"

/* 
{+D}
    SYSTEM:			Library Software

    FILENAME:       rsts445.c

    MODULE NAME:    rsts445 - read status of board

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    FJM

    CODED BY:       FJM
    
    ABSTRACT:       This module is used to read status of the board.

    CALLING
        SEQUENCE:   rsts445(ptr);
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

    This module is used to perform the read status function for the
    board.  A pointer to the Status Block will be passed to this routine.
    The routine will use a pointer within the Status Block together with
    offsets to reference the registers on the Board and will transfer the 
    status information from the Board to the Status Block.
*/



void rsts445(s_blk)
struct sblk445 *s_blk;

{
/*
    declare local storage
*/
    int i;              /* loop index */
    word idProm[12];    /* holds ID prom */

/*
    ENTRY POINT OF ROUTINE
    read board id information
*/

    ReadIOSID(s_blk->nHandle, s_blk->slotLetter, &idProm[0], sizeof idProm / 2); /* read from carrier */

    for(i = 0; i < 12; i++ )
 	   s_blk->id_prom[i] = (byte)idProm[i];
}


