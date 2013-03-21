
#include "../ioscarrier/ioscarrier.h"
#include "ios482.h"

/*
{+D}
    SYSTEM:			Library Software

    FILENAME:       rmid482.c

    MODULE NAME:    rmid482 - read I.D.

    VERSION:        A

    CREATION DATE:	04/01/09

    DESIGNED BY:    F.J.M

    CODED BY:       F.J.M.
    
    ABSTRACT:       This module is used to read the I.D.

    CALLING
        SEQUENCE:   rmid482(ptr);
                    where:
                        ptr (pointer to structure)
                            Pointer to the configuration block structure.

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
    for the board.  A pointer to the Configuration Block will
    be passed to this routine.  The routine will use a pointer
    within the Configuration Block together with offsets
    to reference the registers on the Board and will transfer the 
    status information from the Board to the Configuration Block.
*/



void rmid482(c_blk)
struct cblk482 *c_blk;
{
	int i;	            /* loop index */
	word idProm[12];    /* holds ID prom */

/*
    ENTRY POINT OF ROUTINE

    read board id information
*/

    ReadIOSID(c_blk->nHandle, c_blk->slotLetter, &idProm[0], sizeof idProm / 2 ); /* read from carrier */

    for(i = 0; i < 12; i++ )
 	   c_blk->id_prom[i] = (byte)idProm[i];
}
