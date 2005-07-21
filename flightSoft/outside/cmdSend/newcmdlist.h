/* newcmdlist.h */

#ifndef _CMDLIST_H
#define _CMDLIST_H

#define KEYBD_CMD_MAX	128	/* maximum ascii key */
#define SHUTDOWN_HALT	129	/* shuts computer down, must be powered down and then up to restart*/
#define REBOOT          130	/* reboots flight computer */
#define KILL_PROGS	131	/* kills all? daemons */
#define RESPAWN  	132	/* respawns all? daemons*/

#define TURN_GPS_ON	150	/* turns on GPS and magnetometer via relay*/
#define TURN_GPS_OFF	151	/* turns off GPS and magnetometer via relay*/
#define TURN_RFCM_ON	152	/*turns on RFCM and magnetometer via relay*/
#define TURN_RFCM_OFF	153	/* turns off RFCM and magnetometer via relay */
#define TURN_CALPULSER_ON	154	/*turns on CalPulser and magnetometer via relay */
#define TURN_CALPULSER_OFF	155	/*turns off CalPulser and magnetometer via relay */
#define TURN_ND_ON	156	/*turns the Noise Diode on via relay*/
#define TURN_ND_OFF     157     /*turns the Noise Diode off via relay*/	
#define TURN_ALL_ON	158	/*turns on ND, CalPulser, GPS and RFCM via relays*/
#define TURN_ALL_OFF	159	/*turns oFF ND, CalPulser, GPS and RFCM via relays*/

#define TRIG_CALPULSER	170	/* triggers the CalPulser */
#define SET_CALPULSER   171	/* sets the CalPulser Address (?) */
#define SET_SS_GAIN     172	/* sets the Sun Sensor gain */

#define GPS      	180	/* does something for the GPS*/
#define SIP_GPS   	181	/* does something with the SIP GPS data*/
#define SUN_SENSORS	182	/* does something with the sun sensors */
#define MAGNETOMETER	183	/* does something with the Magnetometer*/
#define ACCEL    	184	/* does something with the accelerometer*/

#define CLEAN_DIRS	200	/* cleans all? directories */

#define SEND_CONFIG	210	/* sends down current all? config files  */
#define DEFAULT_CONFIG	211	/* returns to default configuration*/

#define CHNG_PRIORIT	220	/* changes the prioritizer*/
#define SURF     	230	/* does something for the SURF*/
#define TURF     	240	/* does something for the TURF*/
#define RFCM     	250	/* does something for the RFCM*/

#endif /* _CMDLIST_H */
