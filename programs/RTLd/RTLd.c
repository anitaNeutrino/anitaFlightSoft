/*! \file RTLd.c

   \brief the main RTLd daemon that takes spectra using the $20 usb dongles

   Spawns one-shot processes to take each spectrum so that I didn't have to modify the 
   programs as much. They could be done as separate threads instead, but whatever. 

   To communicate, mmap a shared location. This location can then 
   be used to query the spectrum elsewhere if desired. 

   July 2016 cozzyd@kicp.uchicago.edu 

*/ 

/* Flight software */ 
#include "includes/anitaFlight.h" 
#include "includes/anitaStructures.h" 
#include "kvpLib/keyValuePair.h" 
#include "configLib/configLib.h"
#include "utilLib/utilLib.h" 

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


static unsigned startFrequency; 
static unsigned endFrequency; 
static unsigned stepFrequency; 
static unsigned nFrequencies; 
static float gain[NUM_RTLSDR]; 
static int printToScreen = 1; 
static int verbosity = 0; 
static int bitmask; 
static int telemEvery = 1; 

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
    startFrequency = kvpGetInt("startFrequency", 180000000); 
    endFrequency = kvpGetInt("endFrequency",  1250000000); 
    nFrequencies = kvpGetInt("nSteps", 4096); 

    if (printToScreen)
    {
       printf("startFrequency: %d; endFrequency:%d; nFrequencies %d \n", startFrequency, endFrequency, nFrequencies); 
    }

    //do some math here 
    stepFrequency = (endFrequency - startFrequency) / (nFrequencies); 
    
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


  retVal = sortOutPidFile(args[0]); 

  if (retVal) return retVal; 


  /* Setup log */
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog (args[0], LOG_PID, ANITA_LOG_FACILITY) ;
  syslog(LOG_INFO,"RTld Starting");    


  read_config_ok = readConfig(); 

  setupSignals(); 
  setupBreakfast(); 
  setupWriter(); 
  setupBitmask(); 
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



    currentState = PROG_STATE_RUN; 

    while (currentState == PROG_STATE_RUN) 
    {


      // Spawn the processes 
      for (i = 0; i < NUM_RTLSDR; i++) 
      {

        char * serial = serials[i]; 
        sprintf(cmd, "RTL_singleshot_power -d %s  -f %d:%d:%d -g %f %s/%s.out", 
                      serial, startFrequency, endFrequency, stepFrequency, gain[i], tmpdir, serial); 
        oneshots[i] = popen (cmd, "r"); 
        if (!oneshots[i])
        {
          syslog(LOG_ERR, "popen with err %d on command: %s", errno, cmd); 
        }
      }

      //wait for them to finish
      for (i = 0; i < NUM_RTLSDR; i++) 
      {
        pclose(oneshots[i]); 

        if (verbosity > 0) 
        {
          dumpSpectrum(spectra[i]); 
        }


        //fill header
        fillGenericHeader(spectra[i], PACKET_RTLSDR_POW_SPEC, sizeof(RtlSdrPowerSpectraStruct_t)); 


        

      }


      telemCount++; 

      //telemeter , if necessary 
      if (telemCount >= telemEvery) 
      {
        if (printToScreen)
        {
          printf("Writing telemetry files!\n"); 
        }

        for (i = 0; i < NUM_RTLSDR; i++)
        {
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


