
#include "../ioscarrier/ioscarrier.h"
#include "ios220.h"

/*
{+D}
    SYSTEM:         Library Software - ios220

    FILENAME:       rccid220.c

    MODULE NAME:    rccid220 - read calibration coefficients and I.D.

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    F.J.M

    CODED BY:       F.J.M.
    
    ABSTRACT:       This module is used to read calibration coefficients.

    CALLING
        SEQUENCE:   rccid220(ptr);
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

    This module is used to perform the read the I.D. and calibration
    coefficients.  A pointer to the Configuration
    Block will be passed to this routine.  The routine will use a pointer
    within the Configuration Block together with offsets to reference the
    registers on the Board and will transfer information from the
    Board to the Configuration Block.
*/



void rccid220(c_blk)
struct cblk220 *c_blk;

{

/*
    declare local storage
*/
    word idProm[12];		/* holds ID prom */
    int i;                      /* loop index */

/*
    ENTRY POINT OF ROUTINE:
    read gain/offset calibration information
*/

    for(i = 0; i < A_SIZE; i++)
    {
      c_blk->off_buf[i]  = input_byte( c_blk->nHandle, (byte*)&c_blk->brd_ptr->cal_map[i].offset );
      c_blk->gain_buf[i] = input_byte( c_blk->nHandle, (byte*)&c_blk->brd_ptr->cal_map[i].gain );
    }

/*
    read board id information
*/
    ReadIOSID(c_blk->nHandle, c_blk->slotLetter, &idProm[0], sizeof idProm / 2); /* read from carrier */

    for(i = 0; i < 12; i++ )
 	   c_blk->id_prom[i] = (byte)idProm[i];
}
