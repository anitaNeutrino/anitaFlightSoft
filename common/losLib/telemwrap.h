/* telemwrap.h
 *
 * Marty Olevitch, May '08
 */

#ifndef _TELEMWRAP_H
#define _TELEMWRAP_H

/*
 * Macro: TW_LOS
 * 	Indicates using line-of-sight (LOS) data
 */
#define TW_LOS 0

/*
 * Macro: TW_SIP
 * 	Indicates using high-rate TDRSS data via the SIP.
 */
#define TW_SIP 1

/*
 * Macro: TW_PAD_BYTE
 *	Byte to pad the science data to be an even number of bytes.
 *	See the Data Format section of the telemwrap README file.
 */
#define TW_PAD_BYTE 0xDA

/*
 * Function: telemwrap_init
 * 	Initialize telemwrap.
 *
 * Description:
 *
 * 	- Sets the source of the data. The source can be either
 * 	line-of-sight (LOS) data, or SIP TDRSS data. The default is LOS.
 * 	- If you are sending LOS data, then you don't need to execute this
 * 	function.
 *
 * Parameters:
 * 	- datasource - must be either TW_LOS or TW_SIP
 *
 * Returns:
 * 	0 - all went well
 * 	-1 - datasource was something other than TW_LOS or TW_SIP.
 */
int telemwrap_init(int datasource);

/*
 * Function: telemwrap
 * 	Package the science data for transfer.
 *
 * Description:
 *
 * 	This function takes the science data and wraps it with our
 * 	telemetry header words. This includes:
 * 	- headers to indicate start and stop of data.
 * 	- indentification of data source (either SIP or LOS).
 * 	- a count of transfers (buffer count).
 * 	- number of bytes of science data.
 * 	- checksum over the science data only.
 *
 * Parameters:
 * 	databuf - (in) science data.
 * 	wrapbuf - (out) the wrapped data
 * 	nbytes -  (in) number of bytes of science data
 *
 * Returns:
 * 	Number of bytes in wrapped data buffer.
 */
int telemwrap(unsigned short *databuf, unsigned short *wrapbuf,
    unsigned short nbytes);

#endif // _TELEMWRAP_H
