#include "includes/anitaFlight.h" 
#include "tuffLib/tuffLib.h" 
#include "utilLib/utilLib.h"
#include "configLib/configLib.h" 
#include "linkWatchLib/linkWatchLib.h"
#include "kvpLib/keyValuePair.h" 
#include <unistd.h>
#include <time.h>
#include <execinfo.h>
#include <math.h>
#include <errno.h> 

#define NUM_PHI PHI_SECTORS 

/* * *   *    *       *           *              *                   *     * ** * ** * ** 
 * This program is really stupid... it just waits for changes and then applies them?   ***
 *                                                                  
 *  I guess Cmdd could just do all of this 
 *
 *  Actually now that it tries to analyze the power spectra, it's somewhat less stupid. 
 **************************************** * * * *    *     *        * * *     ***     * * *
 */ 


static int printToScreen = 1; 
static int verbosity = 0; 
static int sleepAmount = 1; 
static int readTemperatures = 1; 
static int bitmask = 0; 

static int telemeterEvery = 10; 
static int telemeterAfterChange = 1; 


static int zeroes[NUM_TUFF_NOTCHES]; 
static int adjustAccordingToGPU[NUM_TUFF_NOTCHES]; 
static float gpu_bins[NUM_TUFF_NOTCHES];   
static float tuff_threshold[NUM_TUFF_NOTCHES]; 


/** The device descriptor */ 
static tuff_dev_t * device; 

/* Array telling us if TUFF was found at startup or not */ 
static int tuff_responds[NUM_RFCM]; 

static const unsigned char default_start_sector[NUM_TUFF_NOTCHES] = DEFAULT_TUFF_START_SECTOR; 
static const unsigned char default_end_sector[NUM_TUFF_NOTCHES] = DEFAULT_TUFF_END_SECTOR; 
static const int MAX_ATTEMPTS=3; 

static AnitaHkWriterStruct_t tuffWriter; 
static AnitaHkWriterStruct_t tuffRawCmdWriter; 
static TuffNotchStatus_t tuffStruct; 
static TuffNotchStatus_t lastTuffStruct; 

static float vals[NUM_PHI][NUM_TUFF_NOTCHES]; 


static int telemeterCount = 0; 

int analyzeGPUSpectrum()
{
  int notch; 
  int bin0[NUM_TUFF_NOTCHES]; 
  int bin1[NUM_TUFF_NOTCHES]; 
  int min_phi_above_threshold[NUM_TUFF_NOTCHES]; 
  int max_phi_above_threshold[NUM_TUFF_NOTCHES]; 
  float frac[NUM_TUFF_NOTCHES]; 
  int phi; 
  char theFilename[FILENAME_MAX];

  //read in the spectra, compute the values 


  for (notch = 0; notch < NUM_TUFF_NOTCHES; notch++)  
  {
    if (!adjustAccordingToGPU[notch]) continue; 
    bin0[notch] = gpu_bins[notch]; 
    bin1[notch] = gpu_bins[notch] + 1; 
    frac[notch] = gpu_bins[notch] - ((int) gpu_bins[notch]); 
    min_phi_above_threshold[notch] = -1; 
    max_phi_above_threshold[notch] = -1; 
  }

  for (phi = 0; phi < NUM_PHI; phi++) 
  {
    int retVal; 
    //make links to try to avoid any stupid race condition
    sprintf(theFilename,"%s/gpuPowSpec_phi%d.dat",GPU_SPECTRUM_DIR, phi); 
    retVal=makeLink(theFilename,GPU_SPECTRUM_LINK_DIR);
    if (!retVal)
    {
      syslog(LOG_ERR, "Tuffd: Could not calculate notches from phi spectra because could not load one of them. Abandoning effort\n"); 
      return 1; 
    }
  }

  for (phi = 0; phi < NUM_PHI; phi++) 
  {
    GpuPhiSectorPowerSpectrumStruct_t spectrum; 
    int ring, pol; 
    int retVal; 
    float val = 0;
    sprintf(theFilename,"%s/gpuPowSpec_phi%d.dat",GPU_SPECTRUM_LINK_DIR, phi); 
    if (genericReadOfFile((unsigned char * ) &spectrum, theFilename, sizeof(spectrum)) == -1) 
    {
      syslog(LOG_ERR, "Trouble reading %s\n", theFilename); 
    }

    for (notch = 0; notch < NUM_TUFF_NOTCHES; notch++) 
    {
      if (!adjustAccordingToGPU[notch]) continue; 
      for (ring = 0; ring < NUM_ANTENNA_RINGS; ring++)
      {
        for (pol = 0; pol < 2 ;pol++) 
        {
          /* Scale by short -> float conversion, then log to linear, then divide by 6 (3 antennas * 2 pols) */ 
          double linear_val0 =  pow(10,spectrum.powSpectra[ring][pol].bins[bin0[notch]] * 64.15/32767. / 10.)/6.;
          double linear_val1 =  pow(10,spectrum.powSpectra[ring][pol].bins[bin1[notch]] * 64.15/32767. / 10.)/6.;

          val += (1-frac[notch]) * linear_val0 + (frac[notch]) * linear_val1; 
        }
      }



      vals[phi][notch] = val; 

      if (val > tuff_threshold[notch]) 
      {
        if (min_phi_above_threshold[notch] == -1) 
        {
          min_phi_above_threshold[notch] = phi; 
        }

        if (phi > max_phi_above_threshold[notch]) 
        {
          max_phi_above_threshold[notch] = phi; 
        }
      }

      printf("Total power in phi sector %d, notch %d is %f\n", phi, notch, val); 
    }


    
    retVal = removeFile(theFilename); 
    if (retVal) 
    {
      syslog(LOG_ERR, "Could not remove file %s\n",theFilename); 
    }
  }

  /**
  *
  * Now we have all the values. The strategy for picking 
  * the filtered sectors will be to find all sectors above threshold and 
  * then pick whichever maximum range that covers all of them has the higher average
  *
  *
  * TODO: Perhaps there should be an option to OR the automatic notches with the
  * notches from the config. 
  *
  */ 

  for (notch = 0; notch < NUM_TUFF_NOTCHES; notch ++) 
  {

    if (!adjustAccordingToGPU[notch]) continue; 
    // no phi sectors above thresholds, no notch needed
    if (min_phi_above_threshold[notch] ==-1) 
    {
      tuffStruct.startSectors[notch] = 16; 
      tuffStruct.endSectors[notch] = 16; 
    }

    //only one phi sector to mask 
    else if (min_phi_above_threshold[notch] == max_phi_above_threshold[notch])
    {
      tuffStruct.startSectors[notch] = min_phi_above_threshold[notch]; 
      tuffStruct.startSectors[notch] = min_phi_above_threshold[notch]; 
        
    }

    // check which way has the higher average
    else
    {
      float sum_increasing;  
      float sum_decreasing; 
      int n_increasing = max_phi_above_threshold[notch]+1 - min_phi_above_threshold[notch]; 
      int n_decreasing = 2+ NUM_PHI - n_increasing; 

      for (phi = min_phi_above_threshold[notch]; phi <= max_phi_above_threshold[notch]; phi++) 
      {
        sum_increasing += vals[phi][notch];
      }

      for (phi = max_phi_above_threshold[notch]; phi <= min_phi_above_threshold[notch] + NUM_PHI; phi++)

      {
        sum_decreasing += vals[phi % NUM_PHI][notch]; 
      }


      if (sum_increasing / n_increasing > sum_decreasing / n_decreasing) 
      {
        tuffStruct.startSectors[notch] = min_phi_above_threshold[notch]; 
        tuffStruct.endSectors[notch] = max_phi_above_threshold[notch]; 
      }
      else
      {
        tuffStruct.startSectors[notch] = max_phi_above_threshold[notch]; 
        tuffStruct.endSectors[notch] = min_phi_above_threshold[notch]; 
      }
    }
  }


  //TODO: Think about this, is this what we want??? 
  tuffStruct.notchSetTime = time(0);

  return 0; 
}

int writeState(int changed)
{
  if (printToScreen) printf("writeState(%d)\n", changed); 
  int i = 0; 
  time_t t = time(NULL); 

  tuffStruct.unixTime = t; 
  for (i = 0; i < NUM_RFCM; i++)
  {

    if (!tuff_responds[i]) 
    {
      tuffStruct.temperatures[i] = 127;  
      printf("Writing temperature %d for RFCM %d\n", tuffStruct.temperatures[i],i); 
      continue; 
    }

    tuffStruct.temperatures[i] = (char)  (readTemperatures ? (0.5 + tuff_getTemperature(device,i)) : -128); 

    if (printToScreen) 
    {
      printf("Writing temperature %d for RFCM %d%s\n", tuffStruct.temperatures[i], i, readTemperatures ? "" : " (temperature reading disabled) "); 
    }

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


    fillGenericHeader(&tuffStruct, PACKET_TUFF_STATUS, sizeof(TuffNotchStatus_t)); 
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

  for (i = 0; i < NUM_TUFF_NOTCHES; i++)
  {
    syslog(LOG_INFO, "Tuffd: Setting notch %d range to [%d %d],", i, tuffStruct.startSectors[i], tuffStruct.endSectors[i]); 
    tuff_setNotchRange(device, i, tuffStruct.startSectors[i], tuffStruct.endSectors[i]); 
  }

  return 0; 
}

void cleanup() 
{

  //avoid infinite seg faults, entertaining as they may be!
  signal(SIGSEGV, SIG_DFL);

  tuff_close(device); 
  closeHkFilesAndTidy(&tuffWriter);
  closeHkFilesAndTidy(&tuffRawCmdWriter);
  unlink(TUFFD_PID_FILE); 
  syslog(LOG_INFO,"Tuffd terminating"); 
}

static void setupWriter()
{
  int diskind; 
  sprintf(tuffWriter.relBaseName, TUFF_ARCHIVE_DIR); 
  sprintf(tuffWriter.filePrefix,"tuff"); 

  sprintf(tuffRawCmdWriter.relBaseName, TUFF_ARCHIVE_DIR); 
  sprintf(tuffRawCmdWriter.filePrefix,"tuff_rawcmd_"); 

  tuffWriter.writeBitMask= bitmask; 
  tuffRawCmdWriter.writeBitMask= bitmask; 

  for (diskind = 0; diskind < DISK_TYPES; diskind++)
  {
    tuffWriter.currentFilePtr[diskind] = 0; 
    tuffRawCmdWriter.currentFilePtr[diskind] = 0; 
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
  exit(127 + sig); 
}




void setupSignals()
{
  struct sigaction act; 
  act.sa_sigaction = sigactionUsr1Handler; 
  act.sa_flags = SA_SIGINFO; 

  // this way we know who sent us the signal! 
  if (sigaction(SIGUSR1, &act,NULL)) 
  {
    syslog(LOG_ERR,"Problem setting sigaction. errno=%d\n", errno); 
  }

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
    if (sleepAmount < 1) sleepAmount =1; //don't do stupid things 
    readTemperatures = kvpGetInt("readTemperature",1); 
    telemeterEvery = kvpGetInt("telemEvery",10); 
    telemeterAfterChange = kvpGetInt("telemAfterChange",1); 

  }
  else { configStatus+=2; }


  kvpStatus = configLoad("Tuffd.config","notch"); 
  if (kvpStatus == CONFIG_E_OK) 
  {
    int notch_tmp [2*NUM_TUFF_NOTCHES]; 
    int int_tmp [NUM_TUFF_NOTCHES]; 
    float float_tmp [NUM_TUFF_NOTCHES]; 
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
        printf(" Loaded notches from config file! \n"); 
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

    nread = NUM_TUFF_NOTCHES; 

    kvpStatus = kvpGetIntArray("adjustAccordingToGPU", int_tmp, &nread); 
    if (kvpStatus != CONFIG_E_OK || nread != NUM_TUFF_NOTCHES) 
    {
      syslog(LOG_ERR, "Problem reading in adjustAccordingToGpu, will set to false"); 
      memcpy(adjustAccordingToGPU, zeroes, sizeof(zeroes)); 
    }
    else
    {
      memcpy(adjustAccordingToGPU, int_tmp, sizeof(int_tmp)); 
    }

    nread = NUM_TUFF_NOTCHES; 
    kvpStatus = kvpGetFloatArray("gpuBins", float_tmp, &nread); 
    if (kvpStatus != CONFIG_E_OK || nread != NUM_TUFF_NOTCHES) 
    {
      syslog(LOG_ERR, "Problem reading in gpu_bins, disabling adjustAccordingToGpu"); 
      memcpy(adjustAccordingToGPU, zeroes, sizeof(zeroes)); 
    }
    else
    {
      memcpy(gpu_bins, float_tmp, sizeof(float_tmp)); 
    }

    nread = NUM_TUFF_NOTCHES; 
    kvpStatus = kvpGetFloatArray("gpuThresholds", float_tmp, &nread); 
    if (kvpStatus != CONFIG_E_OK || nread != NUM_TUFF_NOTCHES) 
    {
      syslog(LOG_ERR, "Problem reading in gpu thresholds, disabling adjustAccordingToGpu"); 
      memcpy(adjustAccordingToGPU, zeroes, sizeof(zeroes)); 
    }
    else
    {
      int i; 
      for (i = 0; i < NUM_TUFF_NOTCHES; i++) 
      {
        tuff_threshold[i] = pow(10, float_tmp[i]/10);  // use linear scaling everywhere 
      }
    }
  }
  else { configStatus+=4; }


  return configStatus; 
}

int main(int nargs, char ** args) 
{

  int read_config_ok; 
  int retVal; 
  int justChanged = 1; 
  int i;
  int wd; 
  int nattempts = 0; 
  memset(zeroes, 0, sizeof(zeroes)); 


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
    return 1; 
  }
 



  // reset and ping the tuffs 
  for (i = 0; i < NUM_RFCM; i++) 
  {
    //first try to ping them 
    
    unsigned rfcm = i ; 
    int gotit = tuff_pingiRFCM(device,2,1,&rfcm); 
    printf("%d %d\n", i, gotit) ; 
      
    
    if (!gotit) 
    {
      printf("Resetting RFCM %d\n", i); 
      tuff_setQuietMode(device,false); 
      tuff_reset(device, i); 
      tuff_setQuietMode(device,false); 
      tuff_waitForAck(device,i,1); 
      printf("Got ack from %d!\n", i); 
//      sleep(1); 
      if(!tuff_pingiRFCM(device,2,1,&rfcm))
      {
        fprintf(stderr, "Did not get ping from TUFF %d in 2 seconds... trying to reset again. nattempts=%d \n",i, nattempts); 
        syslog(LOG_INFO, "Did not get ping from TUFF %d in 5 seconds... trying to reset again. nattempts=%d \n",i,nattempts); 
        nattempts++; 

        if (nattempts >= MAX_ATTEMPTS) //give up eventually 
        {
          syslog(LOG_ERR, "Tuffd giving up on hearing from iRFCM %d after %d bad attempts\n", i, MAX_ATTEMPTS); 
          fprintf(stderr, "Tuffd giving up on hearing from iRFCM %d after %d bad attempts\n", i, MAX_ATTEMPTS); 
          tuff_responds[i] = 0; 
          nattempts = 0; 
          continue; 
        }

        i--; 
        continue; 
      }

    }
    nattempts = 0; 
    printf("Ping received for %d\n", i); 
    tuff_responds[i] = 1; 
  }

  //shut them up
  tuff_setQuietMode(device,true); 

  // set up raw command watch director
  
  makeDirectories(TUFF_RAWCMD_LINK_DIR); 
  makeDirectories(GPU_SPECTRUM_DIR); 
  makeDirectories(GPU_SPECTRUM_LINK_DIR); 

  wd = setupLinkWatchDir(TUFF_RAWCMD_LINK_DIR); 
  if (!wd) 
  {
    fprintf(stderr,"Unable to watch %s\n",TUFF_RAWCMD_LINK_DIR);
    syslog(LOG_ERR,"Unable to watch %s\n",TUFF_RAWCMD_LINK_DIR);
  }

  memset(&lastTuffStruct,0,sizeof(TuffNotchStatus_t)); 

  //set senderOfSigUSR1 to ID_NOT_AN_ID as it is uninitialized
  senderOfSigUSR1 = ID_NOT_AN_ID; 



  do
  {
    int numlinks; 
    if (printToScreen) printf("Reading config: \n"); 

    // Don't read config if we know it's the Prioritizer, since that doesn't modify the config
    if (senderOfSigUSR1 != ID_PRIORITIZERD)
    {
      read_config_ok = readConfig(); 
    }
    else
    {
      if (printToScreen) 
      {
        printf("Woke up by prioritizer!\n"); 
      }
    }


    if (read_config_ok)
    {
      syslog(LOG_ERR, "Tuffd had trouble reading the config. Returned: %d", read_config_ok); 
    }

    //set the notches BEFORE checking for link dirs
    
    if (memcmp(adjustAccordingToGPU, zeroes, sizeof(zeroes))) 
    {
      analyzeGPUSpectrum(); 
    }
    //check if tuff notch status is same 
    justChanged = justChanged || memcmp(lastTuffStruct.startSectors, tuffStruct.startSectors, sizeof(tuffStruct.startSectors)); 
    justChanged = justChanged || memcmp(lastTuffStruct.endSectors, tuffStruct.endSectors, sizeof(tuffStruct.endSectors));  

    //if it changed, set the notches and copy the tuff struct to last tuff struct 
    if (justChanged)
    {
      setNotches(); 
      memcpy(&lastTuffStruct, &tuffStruct, sizeof(TuffNotchStatus_t)); 
    }

    checkLinkDirs(1,0); 
    numlinks = getNumLinks(wd); 
    printf("Numlinks: %d\n", numlinks); 
    //check for raw commands 
    while (numlinks)
    {
      TuffRawCmd_t raw; 
      char * fname = getFirstLink(wd); 
      char rd_linkbuf[FILENAME_MAX]; 
      char rd_buf[FILENAME_MAX]; 
      char wr_buf[FILENAME_MAX]; 

      sprintf(rd_linkbuf, "%s/%s", TUFF_RAWCMD_LINK_DIR, fname); 
      sprintf(rd_buf, "%s/%s", TUFF_RAWCMD_DIR, fname); 

      //read in request
      genericReadOfFile((unsigned char *) &raw, rd_linkbuf, sizeof(raw)); 

      //set the time 
      raw.enactedTime = time(0); 

      //enact the command 
      if (printToScreen)
      {
        printf("Tuff is executing raw command: IRFCM:%d, Stack:%d Command:0x%04x\n", raw.irfcm, raw.tuffStack, raw.cmd ); 
      }
      syslog(LOG_INFO, "Tuff is executing raw command: IRFCM:%d, Stack:%d Command:0x%04x\n", raw.irfcm, raw.tuffStack, raw.cmd ); 
      tuff_rawCommand(device, raw.irfcm, raw.tuffStack, raw.cmd); 

      // save it for telemetry 
      fillGenericHeader(&raw, PACKET_TUFF_RAW_CMD, sizeof(TuffRawCmd_t)); 
      sprintf(wr_buf, "%s/%s", TUFF_TELEM_DIR, fname); 
      if (writeStruct(&raw, wr_buf, sizeof(TuffRawCmd_t)))
      {
        syslog(LOG_ERR, "writeStruct failed for raw command\n"); 
      }

      if (makeLink(wr_buf, TUFF_TELEM_LINK_DIR))
      {
        syslog(LOG_ERR, "makeLink failed for raw command\n"); 
      }

      //save it for posterity
      if ( cleverHkWrite((unsigned char*) &raw, sizeof(TuffRawCmd_t), raw.enactedTime, &tuffRawCmdWriter))  
      {
        syslog(LOG_ERR,"cleverHkWrite failed for raw command\n"); 

      }

      //delete files
      removeFile(rd_linkbuf); 
      removeFile(rd_buf); 
      checkLinkDirs(1,0); 
      numlinks = getNumLinks(wd); 
    }



    currentState = PROG_STATE_RUN; 


    while (currentState == PROG_STATE_RUN)
    {
      retVal = writeState(justChanged); 
      if (retVal) 
      {
        syslog(LOG_ERR,"writeState returned %d\n", retVal); 

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


