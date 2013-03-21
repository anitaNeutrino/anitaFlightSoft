
#include "../ioscarrier/ioscarrier.h"
#include "ios521.h"
#include "portio.h"		/* serial port include file */

/*
{+D}
    SYSTEM:         Library Software

    FILENAME:       rsts521.c

    MODULE NAME:    rsts521 - read ID of board

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       This module is used to read the ID of the board.

    CALLING
	SEQUENCE:   rsts521(ptr);
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

    This module is used to perform the read ID function for the board.
    A pointer to the int_source Block will be passed to this routine.
    The routine will use a pointer within the int_source Block together with
    offsets to reference the registers on the Board and will transfer the
    int_source information from the Board.
*/



void rsts521(is)
struct int_source *is;
{
/*
    declare local storage
*/

    int i;	        /* loop index */
    word idProm[12];    /* holds ID prom */

/*
    ENTRY POINT OF ROUTINE

    read board id information
*/

    ReadIOSID(is->nHandle, is->slotLetter, &idProm[0], sizeof idProm / 2 ); /* read from carrier */

    for(i = 0; i < 12; i++ )
 	   is->id_prom[i] = (byte)idProm[i];
}
