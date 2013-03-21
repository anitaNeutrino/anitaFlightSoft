
#include "../ioscarrier/ioscarrier.h"
#include "ios482.h"

/*
{+D}
    SYSTEM:			Library Software

    FILENAME:		wprt482.c

    MODULE NAME:	wprt482() - write output port

    VERSION:		A

    CREATION DATE:	04/01/09

    CODED BY:		FJM

    ABSTRACT:		This module writes output values to a single port.

    CALLING SEQUENCE:	status = wprt482(ptr, value);
			  where:
			    status (long)
			      The returned error status.
			    ptr (pointer to structure)
			      Pointer to the board memory map.
			    value (unsigned)
			      The output value.

    MODULE TYPE:	long

    I/O RESOURCES:

    SYSTEM RESOURCES:

    MODULES CALLED:

    REVISIONS:

      DATE	BY	PURPOSE
    --------   -----	---------------------------------------------------

{-D}
*/


/*
    MODULES FUNCTION DETAILS
*/


long wprt482(ptr, value)


struct cblk482 *ptr;
unsigned value; 	    /* the output value */

{

/*
    ENTRY POINT OF ROUTINE
*/

	output_word(ptr->nHandle, (word*)&ptr->brd_ptr->DigitalOut, value);
	return(0);
}
