/*! \file Calibd.h
    \brief First version of the Calibd program for ANITA-3
    
    Calibd is the daemon that controls the digital acromag and so it's role is 
    to toggle relays.
    June 2014  r.nichol@ucl.ac.uk
*/
// Output functions
void writeStatus();

// Acromag Functions 
void acromagSetup();
void ip470Setup();
int setRelays();
int toggleRelay(int port, int chan);
int setMultipleLevels(int basePort, int baseChan, int nbits, int value);
int setLevel(int port, int channel, int level);


// Config stuff
int readConfigFile();

//Relay locations 

#define AMPLITE1_ON_LOGIC 22
#define AMPLITE1_OFF_LOGIC 21
#define AMPLITE2_ON_LOGIC 20
#define AMPLITE2_OFF_LOGIC 19
#define BZAMPA1_ON_LOGIC 18
#define BZAMPA1_OFF_LOGIC 17
#define BZAMPA2_ON_LOGIC 7
#define BZAMPA1_OFF_LOGIC 6
#define NTUAMPA_ON_LOGIC 5
#define NTUAMPA_OFF_LOGIC 4

#define SB_LOGIC 12
#define NTU_SSD_5V_LOGIC 11
#define NTU_SSD_12V_LOGIC 10
#define NTU_SSD_SHUTDOWN_LOGIC 9


//Acromag
#define IP470_CARRIER "/dev/apc8620_1"
#define IP470_SLOT 'A'


