
#include "../ioscarrier/ioscarrier.h"
#include "ios220.h"

/*
{+D}
    SYSTEM:         Library Software

    FILENAME:       cd220.c

    MODULE NAME:    cd220 - analog output corrected data

    VERSION:        Rev A

    CREATION DATE:  04/1/09

    CODED BY:       F.J.M.

    ABSTRACT:       This module is used to correct output conversions
				    for the ios220.

    CALLING
        SEQUENCE:   cd220(ptr);
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

    This module is used to correct the input data for the
    ios220 board.  A pointer to the Configuration Block will be passed to
    this routine.  The routine will use a pointer within the Configuration
    Block to reference the registers on the Board.
*/

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(A, B) ((A) > (B) ? (A) : (B))


void cd220(c_blk,channel)

struct cblk220 *c_blk;
int channel;
{

/*
    declare local storage
*/

    float f_cor;
    int i_cor;
    signed char *off_data;
    signed char *gain_data;
    word *cor_data;
    word *ideal_data;

/*
    ENTRY POINT OF ROUTINE:
    Initialize variables
*/

    off_data = (signed char *)c_blk->off_buf;
    gain_data = (signed char *)c_blk->gain_buf;
    cor_data = c_blk->cor_buf;
    ideal_data = c_blk->ideal_buf;

    f_cor = ((1.0 + ((float)gain_data[channel] / (4.0 * (float)c_blk->bit_constant))) *
	    ((float)ideal_data[channel] - ((float)c_blk->bit_constant / 2.0))) +
	    (((float)c_blk->bit_constant / 2.0) + (float)(off_data[channel] / 4.0));

    f_cor += 0.5;		/* round up */
    i_cor = (int)f_cor;

    i_cor = MIN( i_cor, (c_blk->bit_constant - 1));
    i_cor = MAX( i_cor, 0);

    cor_data[channel] = (word)i_cor;
}
