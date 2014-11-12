/* sipcom.h - routines to communicate with the SIP
 *
 * Marty Olevitch, May '05
 */

#ifndef SIPCOM_H_
#define SIPCOM_H_

/*
  Type: slowrate_callback
  	Signature of the slowrate data callback.

	The slowrate data callback function must be of the form
>		void func(void);
 */
typedef void (*slowrate_callback)(void);

/*
  Type: gps_callback
  	Signature of the GPS data callback.

	The GPS data callback function must be of the form
>		void func(float longi, float lat, float alt);
 */
typedef void (*gps_callback)(float longi, float lat, float alt);

/*
  Type: gpstime_callback
  	Signature of the GPS time callback.

	The GPS time callback function must be of the form
            void func(float time_of_week, unsigned short,
>                       week_number, float utc_time_offset, float cpu_time);
 */
typedef void (*gpstime_callback)(float time_of_week, unsigned short
    week_number, float utc_time_offset, float cpu_time);

/*
  Type: mks_press_alt_callback
  	Signature of the mks pressure altitude callback.

	The mks pressure altitude callback function must be of the form
>	void func(unsigned short hi, unsigned short mid, unsigned short lo);
 */
typedef void (*mks_press_alt_callback)(unsigned short hi,
                                unsigned short mid, unsigned short lo);
/*
  Type: cmd_callback
  	Signature of the command callback function.
 
  	The command callback function must be of the form
>  	void func(unsigned char *);

	Each time the sipcom library parses an incoming command, the
	function will be called. The unsigned char buffer will contain all
	the bytes of the command. The callback function should do whatever
	is necessary to execute the command.
 */
typedef void (*cmd_callback)(unsigned char *);

/*
 * Macro: COMM1
 * 	Use COMM1 port.
 *
 * Macro: COMM2
 * 	Use COMM2 port.
 */
#define COMM1 0
#define COMM2 1
#define HIGHRATE 2
#define VERY_HIGHRATE 3

/*
 * Function: sipcom_init
 *   	Initializes SIP communications.
 *
 * Description:
 *
 *   	- Initializes the serial ports for COMM1, COMM2, high rate (OMNI)
 *   	  TDRSS data, and very high rate (HGA) tdrss data.
 *   	- Initializes the SIP throttling module.
 *   	- Starts two threads that read input from COMM1 and COMM2.
 *
 * Parameters:
 * 	throttle_rate - maximum number of bytes/sec to write to SIP.
 * 	    See <sipcom_highrate_set_throttle> for an important warning on
 * 	    the value of the throttle rate.
 *
 * 	config_dir - name of directory containing configuration	files that
 * 	    tell us the serial line assignments.
 *
 * 	enable - a bitmask of which lines to enable. If a bit is set to 1,
 * 	    then that line will use the SIPMUX protocol for data transfer,
 * 	    else, if the bit is cleared to zero, it will not use SIPMUX on
 * 	    that line.
 *
 * 	    The COMM1, COMM2, HIGHRATE (OMNI), and VERY_HIGHRATE (HGA)
 * 	    macros should be used to access the bits. For example, to set
 * 	    only HGA to use SIPMUX:
 * 	        unsigned char enable = 0;
 * 	        enable |= (1 << VERY_HIGHRATE);
 *
 * 	    or to enable SIPMUX on COMM1 and COMM2:
 * 	        unsigned char enable = 0;
 * 	        enable |= (1 << COMM1);
 * 	        enable |= (1 << COMM2);
 *
 * Returns:
 *	0 - all went well
 *	< 0 - there were problems, in which case, use <sipcom_strerror> to
 *	retrieve a hopefully meaningful error message.
 *
 * See also:
 * 	- <sipcom_reset>
 * 	- <sipcom_highrate_set_throttle> for IMPORTANT WARNING!
 */
int sipcom_init(unsigned long throttle_rate, char *config_dir,
                                                unsigned char sipmux_enable);

/*
 * Function: sipcom_end
 * 	Terminates sipcom operations if sipcom_wait() has been called.
 *
 * Description:
 *	This function causes sipcom_wait() to stop waiting and to terminate
 *	the two SIP input reading threads. If sipcom_wait() has not been
 *	called, then sipcom will not terminate.
 *
 * See also:
 * 	- <sipcom_init>
 * 	- <sipcom_wait>
 */
void sipcom_end();

/*
 * Function: sipcom_strerror
 *	Retrieves the most recent error message
 *
 * Returns:
 *     pointer to C-string - This string should not be modified. If you
 *     need to modify it, first make a copy of the string (not just the
 *     pointer).
 */
char *sipcom_strerror();

/* Function: sipcom_wait
 * 	Wait for sipcom_end to tell us to stop waiting and then terminate
 *	the two reader threads.
 *
 * Description:
 * 	Recall that <sipcom_init> starts two threads (one to read input
 * 	from the SIP on port COMM1 and one on port COMM2). <sipcom_wait>
 * 	will block until <sipcom_end> signals that it is time to quit; then
 * 	both of these threads will be cancelled. It should be called at the
 * 	point in your program where you want to turn control over to
 * 	sipcom.
 */
void sipcom_wait();

/*
 * Function: sipcom_reset
 * 	Prepare to reinitialize sipcom.
 *
 * Description:
 *
 * 	Ordinarily, <sipcom_init> should only be run once.
 * 	If it becomes necessary to run <sipcom_init> more than once, this
 * 	function <sipcom_reset> must be run first. Note that before doing
 * 	that, you should make sure that the two SIP reader threads have
 * 	terminated (by having called sipcom_wait() and then sipcom_end()).
 *
 * See also:
 * 	- <sipcom_init>
 */
void sipcom_reset();

/*
 * Function: sipcom_highrate_write
 * 	Write data to the SIP high rate TDRSS port.
 *
 * Description:
 *
 * 	Use this function to write data to the high rate TDRSS port. This
 * 	function takes care of throttling the rate at which we write to the
 * 	SIP. All the requested bytes should be written to the SIP without
 * 	exceeding the throttle rate specified in <sipcom_init> or
 * 	<sipcom_highrate_set_throttle>.
 *
 * Parameters:
 * 	buf - pointer to data to be written
 * 	nbytes - number of bytes to write. There is no limit to the number
 * 		of bytes to write, but I wouldn't exceed the throttle rate
 * 		by too much.
 *
 * Returns:
 *	0 - all went well
 *	< 0 - there were problems, in which case, use <sipcom_strerror> to
 *		retrieve a hopefully meaningful error message.
 */
int sipcom_highrate_write(unsigned char *buf, unsigned short nbytes);

/*
 * Function: sipcom_very_highrate_write
 * 	Write data to the SIP very high rate (HGA antenna) TDRSS port.
 *
 * Description:
 *
 * 	Use this function to write data to the very high rate (HGA antenna)
 * 	TDRSS port. No throttling is necessary for writing to this port.
 *
 * Parameters:
 * 	buf - pointer to data to be written
 * 	nbytes - number of bytes to write.
 *
 * Returns:
 *	0 - all went well
 *	< 0 - there were problems, in which case, use <sipcom_strerror> to
 *		retrieve a hopefully meaningful error message.
 */
int sipcom_very_highrate_write(unsigned char *buf, unsigned short nbytes);

unsigned short * sipcom_wrap_buffer(
        unsigned char *buf, unsigned short nbytes, short *wrapbytes);

unsigned short * openport_wrap_buffer(
        unsigned char *buf, unsigned short nbytes, short *wrapbytes);
/*
 * Function: sipcom_highrate_set_throttle
 * 	Change the maximum rate at which we write high rate data to SIP
 *
 * Description:
 * 	Use this function to change to maximum rate (in bytes/sec). It
 * 	should have been originally set in the <sipcom_init> call.
 *
 * WARNING:
 * 	As of Summer, 2005, the SIP can sustain a maximum rate of about
 * 	6,000 bits/second or 750 bytes/second. If this rate is exceeded for
 * 	more than a few seconds, the SIP's buffers will overflow and data
 * 	WILL BE LOST! To be on the safe side, I recommend not exceeding
 * 	about 5500 bits/second, or about 688 bytes/second. Furthermore, the
 * 	NSBF engineers may set the rate lower than the maximum, in which
 * 	case, you should set the throttle rate to be less than that.
 * 	Hopefully, the NSBF will let you know about this situation, but you
 * 	should nevertheless perform checks on the received data to make
 * 	sure that buffers are not being dropped.
 *
 * Parameters:
 * 	rate - maximum number of bytes/sec to write to SIP
 *
 * See also:
 * 	- <sipcom_init>
 * 	- <sipcom_highrate_write>
 */
void sipcom_highrate_set_throttle(unsigned long rate);

/*
 * Function: sipcom_set_cmd_callback
 * 	Specify the command-processing callback function.
 *
 * Description:
 *
 * 	Call this function to pass the library a pointer to your function
 * 	that will be used to process each command received from the ground.
 * 	Each time a command is parsed on either the <COMM1> or <COMM2>
 * 	ports, your function will be executed and provided with a buffer
 * 	containing the command bytes.
 *
 * Parameters:
 * 	f - pointer to callback function f
 *
 * See also:
 * 	- <cmd_callback> for the signature of your callback function.
 */
void sipcom_set_cmd_callback(cmd_callback f);

/*
 * Function: sipcom_set_cmd_length
 * 	Specify length of each ground command.
 *
 * Description:
 *
 * 	In order to parse incoming ground commands, the sipcom library must
 * 	know the value of the first byte of each command (its ID) and the
 * 	length in bytes of the command. The length includes the ID byte.
 *
 * Parameters:
 * 	id - the unique first byte that identifies this command.
 * 	length - length in bytes of this command, including the ID byte.
 *
 * Returns:
 * 	0 - Maybe it should not return a value?
 *
 * See also:
 * 	- <sipcom_set_cmd_callback>
 */
int sipcom_set_cmd_length(unsigned char id, unsigned char len);

/*
 * Function: sipcom_slowrate_write
 * 	Write data to a slow rate port.
 *
 * Description:
 * 	
 * 	Use this function to write data to one of the slow rate ports. Note
 * 	that a maximum of 240 bytes can be written with this function. It
 * 	should be called inside your <slowrate_callback> function, as the
 * 	SIP won't accept slow rate data until after it requests the data.
 * 	These data requests typically occur every 30 seconds.
 *
 * Parameters:
 *	comm - which slow rate port to use. Should be either the macro
 *		<COMM1> or <COMM2>. Usually, one port sends the data via
 *		TDRSS and the other IRIDIUM.
 * 	buf - the bytes to send.
 * 	nbytes - number of bytes to send. Maximum of 240 bytes.
 *
 * Returns:
 * 	0 - all went well.
 * 	< 0 - there were problems, in which case, use <sipcom_strerror> to
 * 	retrieve a hopefully meaningful error message.
 *
 * See also:
 * 	- <slowrate_callback>
 * 	- <sipcom_set_slowrate_callback>
 */
int sipcom_slowrate_write(int comm, unsigned char *buf, unsigned char nbytes);

/*
 * Function: sipcom_set_slowrate_callback
 * 	Specify the slowrate data callback function.
 *
 * Description:
 *
 * 	Call this function to pass the library a pointer to your function
 * 	that will be called when we get a data request from the SIP. These
 * 	requests indicate that the SIP is ready to receive more slow rate
 * 	data. Each time the data request is received on a particular port,
 * 	the callback for that port is executed. The callback should write
 * 	the data to that same port using <sipcom_slowrate_write>.
 *
 * Parameters:
 *	comm - which slow rate port to use. Should be either the macro
 *		<COMM1> or <COMM2>. Usually, one port sends the data via
 *		TDRSS and the other IRIDIUM.
 *
 *	 f - a pointer to your <slowrate_callback> function.
 *
 * Returns:
 * 	0 - all went well.
 * 	< 0 - there were problems, in which case, use <sipcom_strerror> to
 * 	retrieve a hopefully meaningful error message.
 *
 * See also:
 * 	- <sipcom_slowrate_write>
 * 	- <slowrate_callback>
 */
int sipcom_set_slowrate_callback(int comm, slowrate_callback f);

void sipcom_set_gps_callback(gps_callback f);
void sipcom_set_gpstime_callback(gpstime_callback f);
void sipcom_set_mks_press_alt_callback(mks_press_alt_callback f);
int sipcom_request_gps_pos(void);
int sipcom_request_gps_time(void);
int sipcom_request_mks_pressure_altitude(void);

#endif // SIPCOM_H_
