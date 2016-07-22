#include "includes/anitaFlight.h" 
#include "tuffLib/tuffLib.h" 
#include "utilLib/utilLib.h"
#include "configLib/configLib.h" 
#include "kvpLib/keyValuePair.h" 
#include <unistd.h>
#include <time.h>
#include <execinfo.h>


/* * *   *    *       *           *              *                   *     * ** * ** * ** 
 * This program is really stupid... it just waits for changes and then applies them?   ***
 *                                                                  
 *  I guess Cmdd could just do all of this 
 **************************************** * * * *    *     *        * * *     ***     * * *
 */ 


static int printToScreen = 1; 
static int verbosity = 0; 
static int sleepAmount = 1; 
static int readTemperatures = 1; 
static int bitmask = 0; 

static int telemeterEvery = 10; 
static int telemeterAfterChange = 1; 

static int sendRawCommand  = 0; 
static int rawCommand[3]; 

tuff_dev_t * device; 

static const unsigned char default_start_sector[NUM_TUFF_NOTCHES] = DEFAULT_TUFF_START_SECTOR; 
static const unsigned char default_end_sector[NUM_TUFF_NOTCHES] = DEFAULT_TUFF_END_SECTOR; 

static AnitaHkWriterStruct_t tuffWriter; 
static TuffNotchStatus_t tuffStruct; 


static int telemeterCount = 0; 

int writeState(int changed)
{
  int i = 0; 
  time_t t = time(NULL); 

  tuffStruct.unixTime = t; 
  for (i = 0; i < NUM_RFCM; i++)
  {
    tuffStruct.temperatures[i] = (char)  (readTemperatures ? tuff_getTemperature(device,i) : -128); 
  }

  telemeterCount++; 

  if ( (telemeterAfterChange && changed) || (telemeterEvery && telemeterCount >= telemeterEvery))
  {
    char fileName[FILENAME_MAX]; 
    int retVal; 

    if (printToScreen)
    {
      printf("Writing telemetry files!\n"); 
    }

    sprintf(fileName,"%s/tuff_%d.dat",TUFF_TELEM_DIR, tuffStruct.unixTime);
    retVal=writeStruct(&tuffStruct,fileName,sizeof(TuffNotchStatus_t));  
    if (retVal) syslog(LOG_ERR, "writeStruct returned %d\n", retVal); 
    retVal=makeLink(fileName,TUFF_TELEM_LINK_DIR);  
    if (retVal) syslog(LOG_ERR, "makeLink returned %d\n", retVal); 
    telemeterCount = 0; 
  }


  return cleverHkWrite((unsigned char*) &tuffStruct, sizeof(TuffNotchStatus_t), tuffStruct.unixTime, &tuffWriter); 
}

int setNotches()
{
  int i; 
  int ret; 

  for (i = 0; i < NUM_TUFF_NOTCHES; i++)
  {
    syslog(LOG_INFO, "Tuffd: Setting notch %d range to [%d %d],", i, tuffStruct.startSectors[i], tuffStruct.endSectors[i]); 
    ret = tuff_setNotchRange(device, i, tuffStruct.startSectors[i], tuffStruct.endSectors[i]); 
    ret <<= 1; 
  }

  return ret; 
}

void cleanup() 
{

  //avoid infinite seg faults, entertaining as they may be!
  signal(SIGSEGV, SIG_DFL);

  tuff_close(device); 
  closeHkFilesAndTidy(&tuffWriter);
  unlink(TUFFD_PID_FILE); 
  syslog(LOG_INFO,"Tuffd terminating"); 
}

static void setupWriter()
{
  int diskind; 
  sprintf(tuffWriter.relBaseName, TUFF_ARCHIVE_DIR); 
  sprintf(tuffWriter.filePrefix,"tuff"); 
  //todo
  tuffWriter.writeBitMask= bitmask; 
  for (diskind = 0; diskind < DISK_TYPES; diskind++)
  {
    tuffWriter.currentFilePtr[diskind] = 0; 
  }
}

void handleBadSigs(int sig) 
{
  syslog(LOG_WARNING,"Tuffd received sig %d -- will exit immediately\n",sig); 

  if (sig == SIGSEGV) 
  {
    size_t size; 
    void * traceback[20]; 
    size = backtrace(traceback, 20); 
    backtrace_symbols_fd(traceback,size, STDERR_FILENO); 
  }

  cleanup(); 
  exit(0); 
}



void setupSignals()
{
  signal(SIGUSR1, sigUsr1Handler);
  signal(SIGUSR2, sigUsr2Handler);
  signal(SIGTERM, handleBadSigs);
  signal(SIGINT, handleBadSigs);
  signal(SIGSEGV, handleBadSigs);
}

int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(TUFFD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,TUFFD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(TUFFD_PID_FILE);
  return 0;
}



/** Can we please move this into a library?  utilLib? */ 
void setupBitmask()
{
  int status, disableUsb, disableHelium2, disableHelium1; 
  char * eString; 
  
  /* Load Global Config */
  kvpReset () ;
  status = configLoad (GLOBAL_CONF_FILE,"global") ;

  /* Get Device Names and config stuff */
  if (status == CONFIG_E_OK) 
  {
    bitmask=kvpGetInt("hkDiskBitMask",0);
    disableUsb=kvpGetInt("disableUsb",1);
    if(disableUsb)
	    bitmask&=~USB_DISK_MASK;
//	disableNeobrick=kvpGetInt("disableNeobrick",1);
//	if(disableNeobrick)
//	  bitmask&=~NEOBRICK_DISK_MASK;
    disableHelium2=kvpGetInt("disableHelium2",1);
    if(disableHelium2)
	    bitmask&=~HELIUM2_DISK_MASK;
    disableHelium1=kvpGetInt("disableHelium1",1);
    if(disableHelium1)
        bitmask&=~HELIUM1_DISK_MASK;
  }
  else
  {
    eString = configErrorString (status) ;
    syslog(LOG_ERR,"Error reading %s -- %s",GLOBAL_CONF_FILE,eString);
  }
}

int readConfig() 
{
  int kvpStatus=0;
  //char* eString ;//TODO: actually print out bad things that happen
  int configStatus = 0; 
  int empty[4] = {0,0,0,0}; 
  time_t tm; 
 
  kvpReset();
  kvpStatus = configLoad ("Tuffd.config","output") ;
  if(kvpStatus == CONFIG_E_OK)
  {
   	printToScreen=kvpGetInt("printToScreen",1);
  	verbosity=kvpGetInt("verbosity",0);
  }
  else { configStatus+=1; }


  kvpStatus = configLoad("Tuffd.config","behavior"); 

  if (kvpStatus == CONFIG_E_OK) 
  {

    sleepAmount = kvpGetInt("sleepAmount",10); 
    readTemperatures = kvpGetInt("readTemperature",1); 
    telemeterEvery = kvpGetInt("telemEvery",10); 
    telemeterAfterChange = kvpGetInt("telemAfterChange",1); 

  }
  else { configStatus+=2; }


  kvpStatus = configLoad("Tuffd.config","notch"); 
  if (kvpStatus == CONFIG_E_OK) 
  {
    int notch_tmp [2*NUM_TUFF_NOTCHES]; 
    int nread = 2*NUM_TUFF_NOTCHES;
    int ok = 1; 

    kvpStatus = kvpGetIntArray("notchPhiSectors", notch_tmp, &nread); 

    if (kvpStatus != CONFIG_E_OK || nread != 2*NUM_TUFF_NOTCHES) 
    {
      syslog(LOG_ERR, "Problem reading in notch phi sectors, will set to defaults."); 
      ok = 0; 
    }

    if (ok)
    {
      int i = 0; 
      if (printToScreen) 
      {
        printf(" Loaded notches from config file! "); 
        for (i = 0; i < NUM_TUFF_NOTCHES; i++)
        {
          printf("   Requested Notch %d:  (%d, %d)\n", i, notch_tmp[2*i], notch_tmp[2*i+1]); 
        }
      }

      for (i = 0; i < NUM_TUFF_NOTCHES; i++)
      {
        tuffStruct.startSectors[i] = notch_tmp[2*i]; 
        tuffStruct.endSectors[i] = notch_tmp[2*i+1]; 
      }
    }

    else
    {
      if (printToScreen) 
      {
        printf("  Problem loading notches from config file, will soon set to defaults." ); 
      }
      memcpy(&tuffStruct.startSectors, default_start_sector, sizeof(tuffStruct.startSectors)); 
      memcpy(&tuffStruct.endSectors, default_end_sector, sizeof(tuffStruct.endSectors)); 
    }
  }
  else { configStatus+=4; }

  kvpStatus = configLoad ("Tuffd.config","raw") ;
  if(kvpStatus == CONFIG_E_OK)
  {
    int rawLength = 4;
    int tempRaw[rawLength]; 
    kvpGetIntArray("rawCmd",tempRaw, &rawLength); 
    sendRawCommand = tempRaw[0]; 
    memcpy(rawCommand, tempRaw+1, sizeof(rawCommand)); 
  }
  else { configStatus+=8; }


  configModifyIntArray( "Tuffd.config","raw","rawCmd", empty, 4,&tm); 



  return configStatus; 
}

int main(int nargs, char ** args) 
{

  int read_config_ok; 
  int retVal; 
  int justChanged; 
  int i;
  unsigned int irfcmList[NUM_RFCM]; 
  int numPongs; 


  retVal = sortOutPidFile(args[0]); 
  if (retVal) return retVal; 

  /* setup log */
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog (args[0], LOG_PID, ANITA_LOG_FACILITY) ;
  syslog(LOG_INFO,"Tuffd Starting");    

  setupSignals(); 
  setupBitmask(); 
  setupWriter(); 


  // open the tuff 
  device = tuff_open(TUFF_DEVICE);

  if (!device) 
  {
    syslog(LOG_ERR,"Tuffd could not open the Tuff. Aborting.."); 
    cleanup(); 
  }
 
  // check if we can talk to it and reset 
  for (i = 0; i < NUM_RFCM; i++) 
  {
    irfcmList[i] = i; 
  }

  numPongs = tuff_pingiRFCM(device, 10, NUM_RFCM, irfcmList); 

  if (numPongs!= NUM_RFCM)
  {
    syslog(LOG_ERR," Tuffd could not get pongs from all of the IRFCM's! Heard %d pongs.\n", numPongs); 
    cleanup(); 
  }

  // reset the tuffs 
  for (i = 0; i < NUM_RFCM; i++) 
  {
    tuff_reset(device, i); 
  }

  //shut them up
  tuff_setQuietMode(device,true); 

  do
  {
    read_config_ok = readConfig(); 

    if (!read_config_ok)
    {
      syslog(LOG_ERR, "Tuffd had trouble reading the config. Returned: %d", read_config_ok); 
    }

    if (sendRawCommand)
    {
      tuff_rawCommand(device, rawCommand[0], rawCommand[1], rawCommand[3]); 
    }

    setNotches(); 
    justChanged = 1; 
    currentState = PROG_STATE_RUN; 

    while (currentState == PROG_STATE_RUN)
    {
      retVal = writeState(justChanged); 
      if (!retVal) 
      {
        // do something 

      }
      justChanged = 0; 

      if (currentState == PROG_STATE_RUN) //check again, although still technically a race condition
      {
        sleep(sleepAmount);  //this will be woken up by signals 
      }
    }

  } while(currentState == PROG_STATE_INIT); 

  cleanup(); 

  return 0; 
}


