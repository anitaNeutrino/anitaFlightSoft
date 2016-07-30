/*! \file RTLd.c

   \brief the main RTLd daemon that takes spectra using the $20 usb dongles

   Spawns one-shot processes to take each spectrum so that I didn't have to modify the 
   programs as much. They could be done as separate threads instead, but whatever. 

   To communicate, mmap a shared location. This location can then 
   be used to query the spectrum elsewhere if desired. 

   July 2016 cozzyd@kicp.uchicago.edu 

*/ 


#define RESET_USB_HUB_COMMAND "systemctl start rtlrestart"

/* Flight software */ 
#include "includes/anitaFlight.h" 
#include "includes/anitaStructures.h" 
#include "kvpLib/keyValuePair.h" 
#include "configLib/configLib.h"
#include "utilLib/utilLib.h" 
#include "sys/wait.h"

#include "RTL_common.h" 

/// standard stuff

#include <errno.h>
#include <stdio.h> 
#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>


const char *  tmpdir  ="/tmp/rtl/"; 



static FILE * oneshots[NUM_RTLSDR];
static char cmd[1024]; 
/** will be populated with RTL1 .. N.  The serial number should be set in eeprom */ 
static char serials[NUM_RTLSDR][32]; 
static AnitaHkWriterStruct_t rtlWriter; 
static RtlSdrPowerSpectraStruct_t * spectra[NUM_RTLSDR]; 


static unsigned nfails[NUM_RTLSDR]; 

static unsigned startFrequency; 
static unsigned endFrequency; 
static unsigned stepFrequency; 
static float gain[NUM_RTLSDR]; 
static int printToScreen = 1; 
static int verbosity = 0; 
static int bitmask; 
static int telemEvery = 1; 
static int failThreshold = 5; 
static int gracefulTimeout = 60; 
static int failTimeout = 5; 
static int disabled[NUM_RTLSDR]; 
static int config_disabled[NUM_RTLSDR]; 

void cleanup(); 

static int readConfig()
{
  int status; 
  int nread = NUM_RTLSDR; 
  int config_status = 0; 
  char * eString; 
  kvpReset(); 
  status = configLoad ("RTLd.config","output") ;
  if (status == CONFIG_E_OK) 
  {
    printToScreen = kvpGetInt("printToScreen",1); 
    verbosity = kvpGetInt("verbosity",0); 
    if (printToScreen) 
    {
      printf("printToScreen: %d; verbosity:%d\n", printToScreen, verbosity); 
    }
  }
  else
  {
    eString=configErrorString (status) ;
    syslog(LOG_ERR,"Error reading RTLd.config: %s\n",eString);
    config_status++; 
  }


  kvpReset(); 
  status = configLoad("RTLd.config","rtl"); 

  if (status == CONFIG_E_OK)
  {
    startFrequency = kvpGetInt("startFreq", 180000000); 
    endFrequency = kvpGetInt("endFreq",  1250000000); 
    stepFrequency = kvpGetInt("stepFreq", 300000); 
    telemEvery = kvpGetInt("telemEvery",1); 

    if (printToScreen)
    {
       printf("startFrequency: %d; endFrequency:%d; stepFrequency: %d;   telemEvery: %d\n", startFrequency, endFrequency, stepFrequency, telemEvery); 
    }

    status = kvpGetFloatArray("gain", gain, &nread); 
    if (printToScreen)
    {
       printf("nread: %d\n", nread); 	
    }

    //set the rest to zero if not enough were set
    if (nread < NUM_RTLSDR) 
    {
      memset(gain + nread, 0,  (NUM_RTLSDR - nread) * sizeof(*gain)); 
    }
     
    nread = NUM_RTLSDR; 

    memset(config_disabled,0,sizeof(disabled)); 
    status = kvpGetIntArray("disabled", config_disabled, &nread); 

    failThreshold = kvpGetInt("failThreshold", 5); 
    failTimeout = kvpGetInt("failTimeout", 5); 
    gracefulTimeout = kvpGetInt("gracefulTimeout", 60); 


    if (printToScreen)
    {
    printf("gracefulTimeout: %d\n",gracefulTimeout); 
    printf("failTimeout: %d\n",failTimeout); 
    }



    if (status != KVP_E_OK || nread > NUM_RTLSDR)
    {
      syslog(LOG_ERR, "Error loading RTL SDR gains. Might have caused buffer overflow, so going to quit!"); 
      cleanup(); 
      exit(1); 
    }
  }
  else
  {
    eString=configErrorString (status) ;
    syslog(LOG_ERR,"Error reading RTLd.config: %s\n",eString);
    config_status++; 
  }

  return config_status; 
}





static void setupWriter()
{
  int diskind; 
  sprintf(rtlWriter.relBaseName, RTL_ARCHIVE_DIR); 
  sprintf(rtlWriter.filePrefix,"rtl"); 
  //todo
  rtlWriter.writeBitMask= bitmask; 
  for (diskind = 0; diskind < DISK_TYPES; diskind++)
  {
    rtlWriter.currentFilePtr[diskind] = 0; 
  }
}


void setupBreakfast() 
{
  int i;
  for (i = 0; i < NUM_RTLSDR; i++)
  {
    sprintf(serials[i], "RTL%d", i+1); 
  }
}


void cleanup()
{
  int i; 


  //avoid infinite seg faults, entertaining as they may be!
  signal(SIGSEGV, SIG_DFL);

  for (i = 0; i < NUM_RTLSDR; i++) 
  {
    munmap(spectra[i], sizeof(RtlSdrPowerSpectraStruct_t)) ; 
    unlink_shared_RTL_region(serials[i]); 
  }

  closeHkFilesAndTidy(&rtlWriter);
  unlink(RTLD_PID_FILE); 
  syslog(LOG_INFO,"RTLd terminating");    
}

int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(RTLD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,RTLD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(RTLD_PID_FILE);
  return 0;
}


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

void setupDirs() 
{

  makeDirectories(RTL_TELEM_DIR); 
  makeDirectories(RTL_TELEM_LINK_DIR); 

}



void setupShared()
{
  int i = 0; 
  for (i = 0; i < NUM_RTLSDR; i++) 
  {
    spectra[i] = open_shared_RTL_region(serials[i], 1); 
  }
}

void handleBadSigs(int sig) 
{
  syslog(LOG_WARNING,"RTLd received sig %d -- will exit immediately\n",sig); 

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


int main(int nargs, char ** args) 
{

  int read_config_ok; 
  int retVal; 

  //start by resetting the hub 
  

 

  retVal = sortOutPidFile(args[0]); 

  if (retVal) return retVal; 


  /* Setup log */
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog (args[0], LOG_PID, ANITA_LOG_FACILITY) ;
  syslog(LOG_INFO,"RTld Starting");    


  if (fork() == 0)
  {
    char const* argv[] = { "usbreset", RESET_USB_HUB_ARG, 0}; 
    char const* envp[] = { 0} ; 

    execve(RESET_USB_HUB_COMMAND,&argv[0],&envp[0]); 
  }

  wait(&retVal); 

  if (retVal)
  {
    fprintf(stderr,"Problem running %s\n", RESET_USB_HUB_COMMAND); 
    syslog(LOG_ERR,"Problem running %s\n", RESET_USB_HUB_COMMAND); 
    return 1; 
  }


 
  read_config_ok = readConfig(); 

  setupSignals(); 
  setupBreakfast(); 
  setupBitmask(); 
  setupWriter(); 
  setupDirs(); 
  setupShared(); 



 
  mkdir(tmpdir, 0755); 

  //unnecessary since ftruncate already does this 
  //memset(spectrum,0, sizeof(RtlSdrPowerSpectraStruct_t)); 

  do 
  {
    int i; 
    int telemCount; 
    read_config_ok = readConfig(); 

    if (read_config_ok)
    {
      syslog(LOG_ERR, "RTLd: Error reading config!\n"); 
      cleanup(); 
      exit(2); 
    }

    // reset fail counter 
    memset(nfails, 0, sizeof(nfails)); 
    memcpy(disabled, config_disabled, sizeof(config_disabled)); 


    currentState = PROG_STATE_RUN; 

    while (currentState == PROG_STATE_RUN) 
    {


      int nspawned = 0; 
      // Spawn the processes 
      for (i = 0; i < NUM_RTLSDR; i++) 
      {

        //CD QUICK HACK, THIS SHOULD PROBABLY NOT BE THE DEFAULT
        if (disabled[i] && !config_disabled[i]) 
        {
          return 1; 
        }
        nspawned++; 

        char * serial = serials[i]; 
        sprintf(cmd, "timeout  -k %ds %ds RTL_singleshot_power -d %s  -c 0.5 -f %d:%d:%d -g %f %s/%s.out", 
                      failTimeout, gracefulTimeout, serial, startFrequency, endFrequency, stepFrequency, gain[i], tmpdir, serial); 
        oneshots[i] = popen (cmd, "r"); 
        if (printToScreen) 
        {
          printf("%s\n",cmd); 
        }

        if (!oneshots[i])
        {
          syslog(LOG_ERR, "popen with err %d on command: %s", errno, cmd); 
        }
      }

      sleep(1); //just because

      //wait for them to finish
      for (i = 0; i < NUM_RTLSDR; i++) 
      {

        if (!disabled[i])
        {
          int ret= pclose(oneshots[i]); 

          if (printToScreen) 
          {
            printf("%d returned %d\n",i,ret); 
          }

          if (ret != 0)
          {
            syslog(LOG_INFO, "RTLd: %s returned unclean value %d.", serials[i], ret); 

            if (ret == 124) 
            {
              syslog(LOG_INFO, "RTLd: %s failed to finish within timeout. Return value: %d", serials[i], ret); 
            }

            else  //don't seem to always get consistent return values, so anything non-zero should be treated as an error
            {
              syslog(LOG_ERR, "RTLd: %s had to be kill -9ed or could not start RTL_singleshot_power. This has happened %d times before.", serials[i], nfails[i]); 
              nfails[i]++; 
              if (nfails[i] >= failThreshold) 
              {
                syslog(LOG_ERR, "RTLd: %s has exceeded maximum fail count (%d out of %d). Disabling. ", serials[i], nfails[i], failThreshold); 
                disabled[i] = 1; 
              }

              spectra[i]->startFreq = 666;  
              sprintf((char*) spectra[i]->spectrum, "This returned %d. nfails is now %u", ret, nfails[i]); 
              spectra[i]->nFreq = 0;  
              spectra[i]->unixTimeStart = time(NULL); 
              spectra[i]->scanTime = 65535;
            }
          }

        }
        else 
        {

          spectra[i]->startFreq = 0;  
          spectra[i]->nFreq = 0;  
          spectra[i]->gain = 0; 
          spectra[i]->unixTimeStart = time(NULL); 
          spectra[i]->scanTime = 65535;

        }

        if (verbosity > 0) 
        {
          dumpSpectrum(spectra[i]); 
        }


        //fill header
        fillGenericHeader(spectra[i], PACKET_RTLSDR_POW_SPEC, sizeof(RtlSdrPowerSpectraStruct_t)); 


        if (nspawned == 0) 
        {

          syslog(LOG_ERR, "RTLd: no processes spawned... all RTL's disabled! will sleep for a bit.\n"); 
          sleep(10); 
        }
        

      }


      telemCount++; 

      //telemeter , if necessary 
      if (telemEvery && telemCount >= telemEvery && nspawned)  //don't telemeter if empty 
      {
        if (printToScreen)
        {
          printf("Writing telemetry files!\n"); 
        }

        for (i = 0; i < NUM_RTLSDR; i++)
        {
          if (disabled[i]) continue; 
          char fileName[FILENAME_MAX]; 
          sprintf(fileName,"%s/rtl_%s_%d.dat",RTL_TELEM_DIR,serials[i], spectra[i]->unixTimeStart);
          retVal=writeStruct(spectra[i],fileName,sizeof(RtlSdrPowerSpectraStruct_t));  
          retVal=makeLink(fileName,RTL_TELEM_LINK_DIR);  
          telemCount = 0; 
        }
      }

      if (printToScreen)
      {
          printf("Writing telemetry diskfiles: \n"); 
      }
      //write to disk
      for (i = 0; i < NUM_RTLSDR; i++) 
      {
        if (printToScreen) printf(" ..%d..",i); 
        retVal  = cleverHkWrite((unsigned char*) spectra[i], sizeof(RtlSdrPowerSpectraStruct_t), spectra[i]->unixTimeStart, &rtlWriter); 
      }
      
        if (printToScreen) printf(" ..done\n"); 
    }
 
  } while (currentState == PROG_STATE_INIT) ; 


  cleanup() ; 
  return 0; 
}


