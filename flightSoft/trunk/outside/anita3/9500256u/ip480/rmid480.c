
#include "../carrier/carrier.h"
#include "ip480.h"

/*
{+D}
    SYSTEM:         Library Software - ip480 Board

    FILENAME:       rmid480.c

    MODULE NAME:    rmid480 - read I.D. of ip480 board

    VERSION:        B

    CREATION DATE:  05/19/98

    DESIGNED BY:    F.J.M

    CODED BY:       F.J.M.
    
    ABSTRACT:       This module is used to read the I.D. of the ip480 board.

    CALLING
        SEQUENCE:   rmid480(ptr);
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

  DATE	     BY	    PURPOSE
  --------  ----    ------------------------------------------------
  04/01/11   FJM    Changed carrier include to carrier.h

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:

    This module is used to perform the read status function
    for the ip480 board.  A pointer to the Configuration Block will
    be passed to this routine.  The routine will use a pointer
    within the Configuration Block together with offsets
    to reference the registers on the Board and will transfer the 
    status information from the Board to the Configuration Block.
*/



void rmid480(c_blk)
struct cblk480 *c_blk;
{
	int i;	            /* loop index */
	word idProm[12];    /* holds ID prom */

/*
    ENTRY POINT OF ROUTINE

    read board id information
*/

    ReadIpackID(c_blk->nHandle, c_blk->slotLetter, &idProm[0], sizeof idProm / 2 ); /* read from carrier */

    for(i = 0; i < 12; i++ )
 	   c_blk->id_prom[i] = (byte)idProm[i];
}
