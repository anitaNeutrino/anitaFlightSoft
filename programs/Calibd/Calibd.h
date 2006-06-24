/*! \file Calibd.h
    \brief First version of the Calibd program 
    
    Calibd is the daemon that controls the digital acromag and so it's role is 
    to toggle relays, set sunsensor gains and switch between the ports on the 
    RF switch.
    April 2006  rjn@mps.ohio-state.edu
*/

// Output functions
void writeStatus();

// Acromag Functions 
void acromagSetup();
void ip470Setup();
int setRelays();
int setSwitchState();
int setAttenuatorState();
int toggleRelay(int port, int chan);
int setMultipleLevels(int basePort, int baseChan, int nbits, int value);

// Config stuff
int readConfigFile();

//Relay locations 
#define RFCM1_ON_LOGIC 23
#define RFCM1_OFF_LOGIC 22
#define RFCM2_ON_LOGIC 21
#define RFCM2_OFF_LOGIC 20
#define RFCM3_ON_LOGIC 19
#define RFCM3_OFF_LOGIC 18
#define RFCM4_ON_LOGIC 7
#define RFCM4_OFF_LOGIC 6
#define VETO_ON_LOGIC 5
#define VETO_OFF_LOGIC 4
#define GPS_ON_LOGIC 3
#define GPS_OFF_LOGIC 2
#define CAL_PULSER_ON_LOGIC 1
#define CAL_PULSER_OFF_LOGIC 0
#define ATTENUATOR_LSB 13
#define RFSWITCH_LSB 9


//Acromag
#define IP470_CARRIER "/dev/apc8620dos"
#define IP470_SLOT 'A'


#define NUM_SWITCH_PORTS 4
#define NUM_ATTEN_SETTINGS 8

//CalPulser wiring weirdness 
int rfSwitchPortMap[NUM_SWITCH_PORTS]={8,4,2,1};
int attenuatorSettingsMap[NUM_ATTEN_SETTINGS]={7,3,5,1,6,2,4,0};
