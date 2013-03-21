
#include "../carrier/carrier.h"
#include "ip409.h"

/* 
{+D}
    SYSTEM:             Acromag IP409 Digital I/O  Board

    FILENAME:           wpnt409.c
        
    MODULE NAME:        wpnt409() - write output point
                            
    VERSION:            B
    
    CREATION DATE:      05/19/98
    
    CODED BY:           FJM
        
    ABSTRACT:           Module writes an output value to a single I/O point.
    
    CALLING SEQUENCE:   status = wpnt409(ptr, port, point, value);
                          where:
                            status (long)
                              The returned error status.
							ptr (pointer to structure)
							  Pointer to the configuration block structure.
                            port (unsigned)
                              The target I/O port number.
                            point (unsigned)
                              The target I/O point number
                            value (unsigned)
                              The output value.
                            
    MODULE TYPE:        long
    
    I/O RESOURCES:
    
    SYSTEM RESOURCES:   

    MODULES CALLED:
    
    REVISIONS:
    
  DATE	     BY	    PURPOSE
  --------  ----    ------------------------------------------------
  04/01/11   FJM    Changed carrier include to carrier.h
    
{-D}
*/


/*  
    MODULES FUNCTION DETAILS

        This module writes a value (0 or 1) to a point of an output port.
        The valid values of "port" are 0 to 1 and "point" are from 0 to 15.
        Otherwise, a -1 is returned.
*/
    
long wpnt409(c_blk, port, point, value)

struct cblk409 *c_blk;
unsigned port;              /* the I/O port number */
unsigned point;             /* the I/O point of a port */
unsigned value;             /* the output value */

{

/*
    DECLARE LOCAL DATA AREAS:
*/

    word bpos;		    /* bit position */
    word temp;

/*
    ENTRY POINT OF ROUTINE
*/

    if (port > 1 || point > 15 || value > 1)    /* error checking */
       return(-1);
 
    bpos = 1 << point;
    value <<= point;
    temp = input_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->io_port[port]);
    output_word( c_blk->nHandle, (word*)&c_blk->brd_ptr->io_port[port], ((temp & ~bpos) | value) );
    return(0);
}
