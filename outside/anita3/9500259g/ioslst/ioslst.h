

/*
{+D}
    SYSTEM:         Acromag IOS Board

    FILENAME:		iosslt.h

    MODULE NAME:	iosslt.h

    VERSION:		A

    CREATION DATE:	04/01/09

    CODED BY:		FJM

    ABSTRACT:		This "inlcude" file contains all the major data
                    structures and memory map used in the software

    CALLING SEQUENCE:

    MODULE TYPE:	include file

    I/O RESOURCES:

    SYSTEM RESOURCES:

    MODULES CALLED:

    REVISIONS:

      DATE	BY	PURPOSE
    --------   -----	---------------------------------------------------

{-D}
*/



/*
    Defined below is the structure which is used to hold the
    board's status information.
*/

struct sblklst
{
    byte id_prom[32];  /* board ID Prom */
};

/*
    Defined below is the structure which is used to hold the
    board's configuration information.
*/

struct cblklst
{
    struct maplst *brd_ptr; /* base address of the I/O board */
    struct sblklst *sblk_ptr;/* Status block pointer */
    char slotLetter;   /* IOS slot letter */
    int nHandle;	   /* handle to an open carrier board */
    BOOL bCarrier;	   /* flag indicating a carrier is open */
    BOOL bInitialized; /* flag indicating ready to talk to board */	
};






/* Declare functions called */
int ExtractModel( unsigned char *rawid, unsigned char *model );
byte input_byte(int nHandle, byte*);/* function to read an input byte */
word input_word(int nHandle, word*);/* function to read an input word */
void output_byte(int nHandle, byte*, byte);	/* function to output a byte */
void output_word(int nHandle, word*, word);	/* function to output a word */
long input_long(int nHandle, long*);			/* function to read an input long */
void output_long(int nHandle, long*, long);	/* function to output a long */
