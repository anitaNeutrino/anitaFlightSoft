/* los.h - library routines for the Washington U. LOS telemetry board.
 *
 * Marty Olevitch
 * Jul '05
 */

#ifndef _LOS_H
#define _LOS_H

/*
 * Macro: LOS_MAX_BYTES
 * 	Maximum number of science bytes that can be handled by liblos.
 * 	Currently 8100.
 */
#define LOS_MAX_BYTES 8100

/*
 * Macro: LOS_MAX_WORDS
 * 	Maximum number of science words (16 bits) that can be handled by
 * 	liblos.
 */
#define LOS_MAX_WORDS (LOS_MAX_BYTES / sizeof(unsigned short))

/*
 * Macro:    START_HDR    
 *      value 0xF00D
 */
#define START_HDR	0xf00d

/*
 * Macro:    AUX_HDR      
 *      value 0xD0CC
 */
#define AUX_HDR		0xd0cc

/*
 * Macro:    SW_START_HDR 
 *      value 0xAE00
 */
#define SW_START_HDR	0xae00

/*
 * Macro:    END_HDR      
 *      value 0xC0FE
 */
#define END_HDR		0xc0fe

/*
 * Macro:    SW_END_HDR   
 *      value 0xAEFF
 */
#define SW_END_HDR	0xaeff

/*
 * Macro:    FILL_BYTE    
 *      value 0xDA
 *
 * See <Data format> for details of how the above macros are used.
 */
#define FILL_BYTE	0xda

/*
 * Function: los_init
 * 	Initializes LOS telemetry communications.
 *
 * Description:
 *
 * 	- Attempts to "open" the Washington U LOS telemetry board.
 * 	- Does an <los_reset()>.
 * 	- Checks that board memory address 0 has the correct value. If the
 * 	value is incorrect, this is not a WU telemetry board.
 *
 * Parameters:
 * 	bus - PCI bus that the telemetry board should be in.
 * 	slot - slot the board should be in.
 * 	intr - if nonzero, use interrupts, else poll if okay to read/write
 * 	wr - if nonzero, we are writing data, else reading
 * 	ms - number of ms to delay between polls. Use -1 to accept default
 * 	     of 100 ms.
 *
 * Returns:
 * 	0 - all went well
 * 	-1 - could not open device
 * 	-2 - bad read of address 0
 * 	-3 - address 0 has incorrect value
 * 	-4 - couldn't set up interrupt
 *
 * See also:
 * 	<los_strerror> for error messages if all did not go well.
 */
int los_init(unsigned char bus, unsigned char slot, int intr, int wr, int ms);

/* Function: los_end
 * 	Shuts down the LOS telemetry system.
 *
 * Description:
 *
 * 	- Disables interrupt for the LOS board.
 * 	- "Closes" the board.
 *
 * Returns:
 * 	0 - all went well
 * 	-1 - error disabling interrupt
 * 	-2 - error closing device
 */
int los_end(void);

/* Function: los_strerror
 *
 * Description:
 *
 * 	Get information about the most recent error.
 *
 * Returns:
 * 	Pointer to a C-string containing a hopefully useful error message.
 */
char *los_strerror(void);

/* Function: los_version
 *
 * Description:
 *
 * 	Return a C-string giving the version number of the library.
 */
char *los_version(void);

/* Function: los_write
 *
 * Description:
 *
 * 	Write data to the LOS telemetry board (and from there to the GSE).
 * 	Before using this routine for the first time, one must execute
 * 	<los_init>.
 *
 * Parameters:
 *
 * 	buf - pointer to array of bytes to write as data
 * 	nbytes - number of bytes to write (maximum = <LOS_MAX_BYTES>)
 *
 * Returns:
 * 	0 - all went well
 * 	-1 - nbytes was too big
 * 	-2 - wait for board interrupt timed out or failed
 * 	-3 - write to board failed
 * 	-4 - not initialized for writing
 * 	-5 - poll for DATA_READY timed out
 */
int los_write(unsigned char *buf, short nbytes);

/* Function: los_read
 *
 * Description:
 *
 * 	Read data from the LOS telemetry board. Before using this routine
 * 	for the first time, one must execute <los_init>
 *
 * Parameters:
 *
 * 	buf - pointer to array of bytes to read to (should be at least
 * 	    8192 bytes long).
 * 	nbytes - pointer to number of bytes actually read
 *
 * Returns:
 * 	0 - all went well.
 * 	-1 - lost sync or improperly ended buffer
 * 	-2 - wait for board interrupt timed out or failed
 * 	-4 - not initialized for reading
 * 	-5 - poll for DATA_AVAIL timed out
 */
int los_read(unsigned char *buf, short *nbytes);

/* Function: los_reset
 *
 * Description:
 * 	Issue a software reset to the LOS board.
 * 	This is done as part of <los_init>.
 *
 * See also:
 * 	<los_init>
 */
void los_reset(void);
#endif // _LOS_H
