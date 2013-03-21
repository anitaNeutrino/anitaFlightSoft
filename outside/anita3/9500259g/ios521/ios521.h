/*
{+D}
    SYSTEM:         Serial Port Software

    FILE NAME:      ios521.h

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       This module contains the definitions
                    used by the serial IO library.
    CALLING
	SEQUENCE:

    MODULE TYPE:    header file

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

    This module contains the definitions used by the serial IO library.
*/

#if	!defined(TX_RX_REG)

/*
   UART ADDRESS OFFSET CONSTANTS
*/

#define TX_RX_REG       0       /* offset to Tx Rx register */
#define BAUD_LOW        0       /* offset to low byte Baud Rate register */
#define BAUD_HIGH       2       /* offset to high byte Baud Rate register */
#define INT_ENABLE_REG  2       /* offset to Interrupt Enable register */
#define INT_ID          4       /* offset to Interrupt ID register */
#define LINE_CONT       6       /* offset to Line Control register */
#define MODEM_CONT      8       /* offset to Modem Control register */
#define LINE_STAT      10       /* offset to Line Status register */
#define STAT_232       12       /* offset to Modem Status Register */
#define FIFO_CONT       4       /* offset to FIFO Control Register */
#define IVECTOR        14       /* offset to Interrupt Vector Register */


unsigned char InitBuffer(int carrier, int ios_slot, int port);
unsigned char kiread(int carrier, int ios_slot, int port);
unsigned char kirstat(int carrier, int ios_slot, int port);
unsigned char kistat232(int carrier, int ios_slot, int port);

/* ISR */

void ios52xisr(void* pAddr);	/* interrupt handler for IOS521 */
#endif
