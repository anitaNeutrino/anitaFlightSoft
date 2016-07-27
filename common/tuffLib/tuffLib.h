#ifndef _tuff_H_
#define _tuff_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * TUFF library.
 *
 * NOTES: tuff_dev_t is an opaque type, it should be created with tuff_open and closed with tuff_close. 
 *
 */



struct tuff_dev; 
typedef struct tuff_dev tuff_dev_t; 


/** \brief Issues a reset to a specific iRFCM.
 *
 * Note: resets don't actually affect the iRFCM (the TUFF master)
 * itself. They reset the microcontrollers on the TUFFs.
 */
int tuff_reset(tuff_dev_t * d,
		  unsigned int irfcm);

/** \brief Turns on notches on a specific iRFCM channel. 
 *
 * This function turns on the notches listed in notchMask.
 * It does not turn off the other notches.
 */
int tuff_onCommand(tuff_dev_t * d,
		      unsigned int irfcm,
		      unsigned int channel,
		      unsigned int notchMask);

/** \brief Turns off notches on a specific iRFCM channel.
 *
 * This function turns off the notches listed in notchMask.
 * It does not turn on the other notches.
 */
int tuff_offCommand(tuff_dev_t * d,
		       unsigned int irfcm,
		       unsigned int channel,
		       unsigned int notchMask);

/** \brief Specifically sets the notch values on a given channel.
 *
 * This command gets TWO ACKs, because it sends an on command
 * as well as an off command.
 *
 * Bitmask is what notches you want on.
 */
int tuff_setChannelNotches(tuff_dev_t * d,
			      unsigned int irfcm,
			      unsigned int channel,
			      unsigned int notchMask);

/** \brief Sets a capacitor value using raw commands.
 *
 * This function sets the tunable capacitor on a notch on a
 * specific channel. I don't expect these to be changed much,
 * if at all, so these just use raw commands rather than
 * anything intelligible.
 *
 * Anyway, the function works, so the readability isn't that
 * important.
 */
int tuff_setCap(tuff_dev_t * d,
		   unsigned int irfcm,
		   unsigned int channel,
		   unsigned int notch,
		   unsigned int capValue);

/** \brief Makes the TUFFs stop acking all commands.
 *
 * In non-interactive mode, once a pong has been
 * received from all TUFFs, it probably makes sense to
 * go ahead and turn off the ACKs for each command.
 *
 * Quiet mode does this. So call tuff_set(serial_fd, 1);
 *
 * From then on, TUFFs will not acknowledge any commands
 * other than a ping.
 */
int tuff_setQuietMode(tuff_dev_t * d,
			 bool quiet);

/** \brief Looks for an acknolwedgement from a given iRFCM.
 *
 * After a command, a TUFF acknowledges it with a brief 'ack'
 * as well as its iRFCM number.
 *
 * Note that it acknowledges ALL commands in the JSON message, not just once
 * for the JSON message!
 *
 * This SHOULD NOT BE CALLED if the TUFFs are in quiet mode.
 * It'll hang forever.
 */
void tuff_waitForAck(tuff_dev_t * d,
			unsigned int irfcm);

/** \brief Saves the current capacitor values to default.
 *
 * Saves all 3 tunable capacitor values into information flash
 * in the TUFF microcontrollers so they are now the new defaults.
 */
int tuff_updateCaps(tuff_dev_t * d,
		       unsigned int irfcm,
		       unsigned int channel);

/** \brief Pings a list of iRFCMs.
 *
 * To find out if the iRFCMs are present, ping a list of them
 * using this function. It returns the number of pongs received.
 *
 * Timeout is in seconds.
 */
int tuff_pingiRFCM(tuff_dev_t * d,
		      unsigned int timeout,
		      unsigned int numPing,
		      unsigned int *irfcmList);

/** \brief Ranged notch command.
 * 
 * Turns on notches for phi sectors between phiStart/phiEnd (inclusive).
 * Turns off notches outside of that range.
 *
 * The range works modulo 16, so (0,15) turns on all phi sectors, and
 * (15,0) turns on only phi sectors 15 and 0.
 */
int tuff_setNotchRange(tuff_dev_t * d,
			    unsigned int notch,
			    unsigned int phiStart,
			    unsigned int phiEnd);

/** \brief Set phi sectors for an iRFCM.
 *
 * Sets the 24 TUFF channels on an iRFCM to the assigned phi sectors.
 *
 * You can pass less than 24 channels, but it always starts with
 * channel 0, and unpassed phi sectors are unchanged.
 *
 * Note that phi sector assignments/notch range commands haven't
 * been extensively tested yet.
 */
int tuff_setPhiSectors(tuff_dev_t * d,
			  unsigned int irfcm,
			  unsigned int *phiList,
			  unsigned int nb);


/** Return the temperature from the TUFF master for a given irfcm */ 
float tuff_getTemperature(tuff_dev_t * d, unsigned int irfcm);

/** Perform a raw command.*/ 
int tuff_rawCommand(tuff_dev_t * dev, unsigned int irfcm, unsigned int tuffStack, unsigned int cmd); 


/** Get the file descriptor associated with the serial device */ 
int tuff_getfd(tuff_dev_t * dev); 

/* \brief  Open serial device using correct settings , returning our opaque handle
 *
 * Returns a tuff_dev_t * or NULL if could not open port 
 */

tuff_dev_t * tuff_open(const char * device) ; 


/** \brief Close a tuff_dev. 
 *
 * This will free the memory and restore the serial port to its old settings
 *
 */
int tuff_close(tuff_dev_t * d); 


#endif
