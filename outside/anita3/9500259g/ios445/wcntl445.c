
#include "../ioscarrier/ioscarrier.h"
#include "ios445.h"

/* 
{+D}
    SYSTEM:			Library Software

    FILENAME:       wcntl445.c

    MODULE NAME:    wcntl445(); - routine that writes the control register

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    FJM

    CODED BY:       FJM
                                                         
    ABSTRACT:       This module is the routine which writes the control
                    register.
 
    CALLING
        SEQUENCE:   status = wcntl(ptr,value);
                          where:
                            status (long)
                              The returned error status.
							ptr (pointer to structure)
							  Pointer to the configuration block structure.
                            value (byte)
                              The output value.
                        

    MODULE TYPE:    long

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

long wcntl445(c_blk, value)
struct sblk445 *c_blk;
byte    value;               /* value to write to control register */
{
/*
    ENTRY POINT OF ROUTINE
*/

    output_byte( c_blk->nHandle, (byte*)&c_blk->brd_ptr->cntl_reg, value);
    return(0);
}
