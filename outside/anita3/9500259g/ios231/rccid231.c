
#include "../ioscarrier/ioscarrier.h"
#include "ios231.h"

/*
{+D}
    SYSTEM:         Library Software

    MODULE NAME:    rccid231 - read calibration coefficients and I.D.

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    F.J.M

    CODED BY:       F.J.M.
    
    ABSTRACT:       This module is used to read calibration coefficients and I.D. prom.

    CALLING
        SEQUENCE:   rccid231(ptr);
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
    coefficients for the board.  A pointer to the Configuration
    Block will be passed to this routine.  The routine will use a pointer
    within the Configuration Block together with offsets to reference the
    registers on the Board and will transfer information from the
    Board to the Configuration Block.
*/



void rccid231(c_blk)
struct cblk231 *c_blk;

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
      c_blk->off_buf[i]  = ( input_byte(  c_blk->nHandle, (byte*)&c_blk->brd_ptr->cal_map[i].offset_msb ) << 8);
      c_blk->off_buf[i] |= input_byte(  c_blk->nHandle, (byte*)&c_blk->brd_ptr->cal_map[i].offset_lsb );
      
	  c_blk->gain_buf[i] =  ( input_byte(  c_blk->nHandle, (byte*)&c_blk->brd_ptr->cal_map[i].gain_msb ) << 8);
	  c_blk->gain_buf[i] |= input_byte(  c_blk->nHandle, (byte*)&c_blk->brd_ptr->cal_map[i].gain_lsb );
    }

/*
    read board id information
*/
    ReadIOSID(c_blk->nHandle, c_blk->slotLetter, &idProm[0], sizeof idProm / 2); /* read from carrier */

    for(i = 0; i < 12; i++ )
 	   c_blk->id_prom[i] = (byte)idProm[i];
}
