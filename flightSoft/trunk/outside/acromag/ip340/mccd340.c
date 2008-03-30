
#include "../carrier/apc8620.h"
#include "ip340.h"

/*
{+D}
    SYSTEM:		    Library Software - ip340 Board

    FILENAME:		    mccd340.c

    MODULE NAME:	    mccd340 - analog multi channel input corrected data

    VERSION:		    Rev A

    CREATION DATE:	    11/07/95

    CODED BY:		    F.J.M.

    ABSTRACT:		    This module is used to correct input conversions
			    for the ip340 board.

    CALLING
	SEQUENCE:	   int mccd340(ptr);
			    where:
				return (int)
				    -1 = Divide by zero detected 0 otherwise.
				ptr (pointer to structure)
				    Pointer to the configuration block structure.

    MODULE TYPE:    int

    I/O RESOURCES:

    SYSTEM
	RESOURCES:

    MODULES
	CALLED:

    REVISIONS:

  DATE	  BY	    PURPOSE
-------  ----	------------------------------------------------

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:

    This module is used to correct the input data for the
    ip340 board.  A pointer to the Configuration Block will be passed to
    this routine.  The routine will use a pointer within the Configuration
    Block to reference the registers on the Board.
*/



int mccd340(c_blk)

struct cblk340 *c_blk;
{

/*
    declare local storage
*/
    int i, j, status;
    sint denom;
    double temp;
    long *lp;
    sint *wp;

/*
    ENTRY POINT OF ROUTINE:
    Initialize variables
*/

    for(status = 0, j = 0; j < 16; j++)	/* channel data loop */
    {
      denom = c_blk->s_cal_buf[j] - c_blk->s_az_buf[j];
      if( denom == 0 )
	    status = -1 ;				/* Divide by zero detected */
      else
      {
	if(c_blk->channel_enable & (1 << j)) /* if this channel is enabled correct data */
        {
	  wp = c_blk->s_raw_buf[j];
	  lp = c_blk->s_cor_buf[j];
          for( i = 0; i < SA_SIZE; i++ )	/* correct all elements */
          {
	      temp = (double)(wp[i] - c_blk->s_az_buf[j]) * (c_blk->ref_v / (double)denom);

	      if( temp < 0.0)		/* round */
	          temp -= 0.5;
	      else
	  	  temp += 0.5;

	      lp[i] = (long)temp;
	  }
        }
      }
    }
    return(status);
}
