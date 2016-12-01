#include "includes/anitaFlight.h" 
#include "tuffLib/tuffLib.h" 
#include "utilLib/utilLib.h"
#include "configLib/configLib.h" 
#include "linkWatchLib/linkWatchLib.h"
#include "kvpLib/keyValuePair.h" 
#include <poll.h>
#include <unistd.h>
#include <dirent.h> 
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
 *  Actually now that it tries to analyze the heading, it's somewhat less stupid. 
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
static int adjustAccordingToHeading[NUM_TUFF_NOTCHES]; 
static int useMagnetometerForHeading = 0;  
static int degreesFromNorthToNotch[NUM_TUFF_NOTCHES]; 
static int phiSectorOffset[NUM_TUFF_NOTCHES]; 
static int slopeThresholdToNotchNextSector[NUM_TUFF_NOTCHES]; 
static int maxHeadingAge = 120;  

static int setCaps[NUM_TUFF_NOTCHES]; 
static int capArray[NUM_TUFF_NOTCHES][NUM_RFCM][NUM_TUFFS_PER_RFCM]; 





/** The device descriptor */ 
static tuff_dev_t * device; 
int tuff_fd; 

/* Array telling us if TUFF was found at startup or not */ 
static int tuff_responds[NUM_RFCM]; 

static const unsigned char default_start_sector[NUM_TUFF_NOTCHES] = DEFAULT_TUFF_START_SECTOR; 
static const unsigned char default_end_sector[NUM_TUFF_NOTCHES] = DEFAULT_TUFF_END_SECTOR; 
static const int MAX_ATTEMPTS=3; 

static AnitaHkWriterStruct_t tuffWriter; 
static AnitaHkWriterStruct_t tuffRawCmdWriter; 
static TuffNotchStatus_t tuffStruct; 
static TuffNotchStatus_t configTuffStruct; 
static TuffNotchStatus_t lastTuffStruct; 


static int telemeterCount = 0; 



//Store NUM_HEADINGS values of heading history
// in a circular buffer 

#define NUM_HEADINGS 10 



static float headingHistoryA[NUM_HEADINGS]; 
static float headingHistoryB[NUM_HEADINGS]; 
static float headingTimesA[NUM_HEADINGS]; 
static float headingTimesB[NUM_HEADINGS]; 
static float headingWeightsA[NUM_HEADINGS];
static float headingWeightsB[NUM_HEADINGS];

static float headingLatitudeG12; 
static float headingLongitudeG12; 
static float headingAltitudeG12; 

static float headingHistoryMag[NUM_HEADINGS];
static float headingTimesMag[NUM_HEADINGS];
static float headingWeightsMag[NUM_HEADINGS];
static int headingIndexA = -1; 
static int headingIndexB = -1; 
static int headingIndexMag = -1; 


typedef struct 
{
  float slope; 
  float intercept; 
  float slope_err; 
  float intercept_err; 
  int npoints; 
} regression_result_t; 

int weightedLinearRegression(float heading[NUM_HEADINGS], float times[NUM_HEADINGS], float weights[NUM_HEADINGS], regression_result_t * result);



void unwrapHeadings(float * harr)
{
  int i; 
  float adjust = 0;
  float last = 0; 
  int bad[NUM_HEADINGS];
  for (i = 0; i < NUM_HEADINGS; i++) 
  {
    if (harr[i] >=0 &&  harr[i] <= 360)
    {
      if (harr[i] - last + adjust > 180)
      {
        adjust -=360; 
      }
      else if (harr[i] - last + adjust < -180)
      {
        adjust += 360; 
      }

      harr[i] += adjust; 
      last = harr[i]; 
      bad[i] = 0; 
    }
    else 
    {
      bad[i] = 1;
    }
  }


  for (i = 0; i < NUM_HEADINGS; i++)
  {
    if (!bad[i] && harr[i] < 0) harr[i] += 360; 
  }



}

static float tdiff(float tod1, float tod2) 
{

  float diff; 

  // invalid 
  if (tod1 < 0 || tod2 < 0) return -999999; 


  diff = tod1- tod2; 

  // oops, we wrapped around midnight 
  if (diff > 12*60*60) 
  {
    diff -= 24*60*60; 
  }
  else if (diff < -12*60*60)
  {
    diff += 24*60*60; 
  }

  return diff; 
}




int analyzeHeading() 
{
  char buf[FILENAME_MAX]; 
  struct dirent ** list; 
  int i; 
  unsigned int timeNow = time(NULL); 
  int errA, errB, errMag; 
  regression_result_t regressedA;
  regression_result_t regressedB;
  regression_result_t regressedMag; 

  float heading_estimate; 
  float heading_sumw2; 

  float heading_slope_estimate; 
  float heading_slope_sumw2; 
  int nASeen   = 0; 
  int nBSeen   = 0;
  int readG12 = 0;
 
  int nseen = 0; 


  timeNow %= (3600*24); //seconds since midnight 

  if (headingIndexA < 0) 
  {
    // initialize to uninitialized
    for (i = 0; i < NUM_HEADINGS; i++) 
    {
      headingHistoryA[i]     = -1; 
      headingTimesA[i]       = -1; 
      headingWeightsA[i]     = -1;
      headingHistoryB[i]     = -1; 
      headingTimesB[i]       = -1;
      headingWeightsB[i]     = -1;      
      headingHistoryMag[i]   = -1; 
      headingTimesMag[i]     = -1;
      headingWeightsMag[i]   = -1;
 
    }

    headingLatitudeG12  = -1;
    headingLongitudeG12 = -1;
    headingAltitudeG12  = -1;
  }

    headingIndexA = NUM_HEADINGS -1; 
    headingIndexB = NUM_HEADINGS -1; 
    headingIndexMag =NUM_HEADINGS -1; 

  /* Here, we read in the magnetometer data. 
   *
   * We want to keep the last NUM_HEADINGS of attitude data and the last G12 data as a last resort if the ADU5's fail. 
   *
   * We will delete any additional files other than NUM_HEADINGS. The reason it is done in this dumb way is to preserve the 
   * history past a reset. 
   *
   */

  i = scandir(GPSD_HEADING_LINK_DIR, &list,0,&alphasort);  

   if (i < 0) 
   {
     syslog(LOG_WARNING,"Trying to analyze headings, but GPSD_HEADING_LINK_DIR  doesn't exist)"); 
     free(list); 
     return -1; 
   }
   
   
  //We do this in reverse order , which means we will see things in reverse time order
  //which is what we want
  while(i--) 
  {
    float tod; 
   
    GpsAdu5PatStruct_t pat;
    GpsG12PosStruct_t  pos;
    
    sprintf(buf, "%s/%s",GPSD_HEADING_LINK_DIR, list[i]->d_name); 
    //this is a pat 
    if(strstr(list[i]->d_name,"patA") && nASeen++ < NUM_HEADINGS)
    {


        if (genericReadOfFile((unsigned char *) &pat, buf, sizeof(pat)) == -1)
        {
          syslog(LOG_ERR, "Trouble reading %s\n", buf); 
          continue; 
        }

        tod = pat.timeOfDay/1000. ; 
        headingTimesA[headingIndexA]   = tod;

        if (pat.heading < 0 || pat.heading > 360) pat.heading = -1; 
        headingHistoryA[headingIndexA] = pat.heading ;
        headingWeightsA[headingIndexA] = 1./(pat.brms*pat.brms + pat.mrms*pat.mrms);
//        printf("%f \n", headingWeightsA[headingIndexA]); 
        headingIndexA--;

    }

    //this is ADU5B and we haven't seen more than NUM_HEADINGS things
    else if(strstr(list[i]->d_name,"patB") && nBSeen++ < NUM_HEADINGS)
    {


      if (genericReadOfFile((unsigned char *) &pat, buf, sizeof(pat)) == -1)
      {
        syslog(LOG_ERR, "Trouble reading %s\n", buf); 
        continue; 
      }

      tod =  pat.timeOfDay/1000.; 

      if (pat.heading < 0 || pat.heading > 360) pat.heading = -1; 
      headingTimesB[headingIndexB]   = tod; 
      headingHistoryB[headingIndexB] = pat.heading;
      headingWeightsB[headingIndexB] = 1./(pat.brms*pat.brms + pat.mrms*pat.mrms);
      headingIndexB--; 
    }


    else if (strstr(list[i]->d_name,"pos") && !readG12)
    {
        
      if (genericReadOfFile((unsigned char *) &pos, buf, sizeof(pos)) == -1)
      {
        syslog(LOG_ERR, "Trouble reading %s\n", buf); 
      }
        
      headingLatitudeG12   = pos.latitude;
      headingLongitudeG12  = pos.longitude;
      headingAltitudeG12   = pos.altitude;
      readG12 = 1;
    }
      
    else if (list[i]->d_name[0] != '.') //weird file, or we have seen too many of these so we can delete it! 
    {
      //delete the link and the file

      if(unlink(buf))
      {
        syslog(LOG_WARNING, "Trouble deleting %s, errno: %d\n", buf, errno); 
      }

      sprintf(buf,"%s/%s",GPSD_HEADING_DIR, list[i]->d_name); 

      //printf("Deleting %s\n", buf); 
      if (unlink(buf))
      {
          syslog(LOG_WARNING, "Trouble deleting %s, errno: %d\n", buf, errno); 
      }
    }    
  }

  free(list); 


  //now read and process headings from magnetometer 
  //same logic here. 
  
  i = scandir(MAGNETOMETER_LINK_DIR, &list,0,alphasort); 
  if (i < 0 && useMagnetometerForHeading) 
  {
    syslog(LOG_WARNING,"Trying to analyze headings, but MAGNETOMETER_LINK_DIR  doesn't exist)"); 
    free(list); 
    return -1; 
  }


  while(i--) 
  {
    float time; 
    TimedMagnetometerDataStruct_t magdata; 
    sprintf(buf, "%s/%s",MAGNETOMETER_LINK_DIR, list[i]->d_name); 

    if (useMagnetometerForHeading && nseen++ < NUM_HEADINGS) 
    {
      if (genericReadOfFile((unsigned char*) &magdata,buf, sizeof(magdata))  == -1)
      {
         syslog(LOG_ERR, "Trouble reading %s\n", buf); 
         continue; 
      }
      time = magdata.unixTime + magdata.unixTime * 1e-6; 

      headingTimesMag[headingIndexMag] = time; 
      headingIndexMag --;

      //TODO compute heading! 

    }
    else if (list[i]->d_name[0] != '.') 
    {
        //delete the link and the file
        if(unlink(buf))
        {
          syslog(LOG_WARNING, "Trouble deleting %s, errno: %d\n", buf, errno); 

        }
        sprintf(buf,"%s/%s",MAGNETOMETER_DIR, list[i]->d_name); 
        if (unlink(buf))
        {
          syslog(LOG_WARNING, "Trouble deleting %s, errno: %d\n", buf, errno); 
        }
 
    }
  }
  
  free(list); 


  //excise any times that are too old and set to 0 for now 

  for (i = 0; i < NUM_HEADINGS; i++) 
  {
    if (maxHeadingAge && tdiff(headingTimesA[i],timeNow) < -maxHeadingAge)
    {
      headingTimesA[i] = -9999; 
      headingHistoryA[i] = -1; 
      headingWeightsA[i] = 0; 
    }
    else
    {
      headingTimesA[i] -= timeNow; 
    }

    if (maxHeadingAge && tdiff(headingTimesB[i],timeNow)  <- maxHeadingAge)
    {
      headingTimesB[i] = -9999; 
      headingHistoryB[i] = -1; 
      headingWeightsB[i] = 0; 
    }
    else
    {
      headingTimesB[i] -= timeNow; 
    }

    if (useMagnetometerForHeading) 
    {
      if (maxHeadingAge && tdiff(headingTimesMag[i],timeNow) < - maxHeadingAge) 
      {
        headingTimesMag[i] = -9999; 
        headingHistoryMag[i] = -1; 
        headingWeightsMag[i] = 0; 
      }
      else
      {
        headingTimesMag[i] -= timeNow; 
      }
    }
  }


  unwrapHeadings(headingHistoryA); 
  unwrapHeadings(headingHistoryB); 
  unwrapHeadings(headingHistoryMag); 
  if (printToScreen)
  {
    printf("HeadingA timeHeadingA weightHeadingA HeadingB timeHeadingB weightHeadingB\n"); 
    for (i = 0; i < NUM_HEADINGS; i++)
    {

      printf("%0.2f %0.2f %0.2f %0.2f %0.2f %0.2f\n",
          headingHistoryA[i], 
          headingTimesA[i], 
          headingWeightsA[i], 
          headingHistoryB[i], 
          headingTimesB[i], 
          headingWeightsB[i]) ; 
    }
  }


  
  // weighted linear regression
  // 0: slope 1: intercept 2: slope error 3: intercept error
  errA = weightedLinearRegression(headingHistoryA, headingTimesA, headingWeightsA, &regressedA);
  errB = weightedLinearRegression(headingHistoryB, headingTimesB, headingWeightsB, &regressedB);
  errMag = weightedLinearRegression(headingHistoryMag, headingTimesMag, headingWeightsMag, &regressedMag);
  

  if (errA && errB && errMag) 
  {
//    printf("%d %d %d\n", errA,errB,errMag); 
    return -1;  //no information 
  }

  //now let's get the value at now from weighted average 
  //
  
  
  heading_estimate = 0; 
  heading_sumw2 = 0; 
  heading_slope_estimate= 0; 
  heading_slope_sumw2 = 0; 
  

  if (!errA) 
  {
    float w = 1./(regressedA.intercept_err * regressedA.intercept_err); 
    float ws = 1./(regressedA.slope_err * regressedA.slope_err); 
    heading_estimate +=  regressedA.intercept  * w; 
    heading_slope_estimate +=  regressedA.slope * ws;
    heading_sumw2 += w; 
    heading_slope_sumw2 += w; 
  }

 if (!errB) 
  {
    float w = 1./(regressedB.intercept_err * regressedB.intercept_err); 
    float ws = 1./(regressedB.slope_err * regressedB.slope_err); 
    heading_estimate +=  regressedB.intercept  * w; 
    heading_slope_estimate +=  regressedB.slope * ws;
    heading_sumw2 += w; 
    heading_slope_sumw2 += w; 
  }

 if (useMagnetometerForHeading && !errMag) 
  {
    float w = 1./(regressedMag.intercept_err * regressedMag.intercept_err); 
    float ws = 1./(regressedMag.slope_err * regressedMag.slope_err); 
    heading_estimate +=  regressedMag.intercept  * w; 
    heading_slope_estimate +=  regressedMag.slope * ws;
    heading_sumw2 += w; 
    heading_slope_sumw2 += w; 
  }


    

  heading_estimate /= heading_sumw2; 
  heading_slope_estimate /= heading_slope_sumw2; 



  if (printToScreen) 
  {
    printf("Heading estimate: %f +/= %f\n", heading_estimate, 1./sqrt(heading_sumw2)); 
    printf("Heading slope estimate: %f +/= %f\n", heading_slope_estimate, 1./sqrt(heading_slope_sumw2)); 


  }

  for (i = 0; i < NUM_TUFF_NOTCHES; i++) 
  {
    if (adjustAccordingToHeading[i]) 
    {
      float phi_start_notch = heading_estimate - degreesFromNorthToNotch[i] + phiSectorOffset[i];  // this is probably wrong
      float phi_stop_notch = heading_estimate + degreesFromNorthToNotch[i] + phiSectorOffset[i]; 
      while ( phi_start_notch < 0 ) phi_start_notch += 360; 
      while ( phi_stop_notch > 360 ) phi_stop_notch -= 360; 
      while ( phi_stop_notch < 0 ) phi_stop_notch += 360; 
      while ( phi_stop_notch > 360 ) phi_stop_notch -= 360; 

      tuffStruct.startSectors[i] =   (phi_start_notch) /  22.5; 
      tuffStruct.endSectors[i] =   (int) (ceil((phi_stop_notch) /  22.5)) % 16; 

      if (heading_slope_sumw2 && slopeThresholdToNotchNextSector[i] > 0 && heading_slope_estimate > slopeThresholdToNotchNextSector[i])
        tuffStruct.endSectors[i] = (tuffStruct.endSectors[i] + 1) % 16; 

      if (heading_slope_sumw2 && slopeThresholdToNotchNextSector[i] > 0 && heading_slope_estimate < -slopeThresholdToNotchNextSector[i])
        tuffStruct.startSectors[i] = (tuffStruct.startSectors[i] - 1) % 16; 

      if (printToScreen)
      {
        printf("Setting range of notch %d to [%d,%d]\n", i, tuffStruct.startSectors[i], tuffStruct.endSectors[i]); 
      }
    }
  }

  return 0; 

}



int weightedLinearRegression(float heading[NUM_HEADINGS],  float times[NUM_HEADINGS],  float weights[NUM_HEADINGS], regression_result_t * regressed)
{

  float sumXXA = 0;
  float sumXYA = 0;
  float sumXA  = 0;
  float sumYA  = 0;
  float sumWA  = 0;
  int   numA   = 0;
  float delta; 
  int i=0;
  
  for (i = 0; i < NUM_HEADINGS; i++)
  {
    if (heading[i] >=0){
      sumXYA += heading[i]*times[i]*weights[i];
      sumXXA += times[i]*times[i]*weights[i];
      sumXA  += times[i]*weights[i];
      sumYA  += heading[i]*weights[i];
      sumWA  += weights[i];
      numA++;
    }
  } 
  
  delta  = (sumWA*sumXXA - sumXA*sumXA);
 
  regressed->slope         = (sumWA*sumXYA - sumXA*sumYA)  / delta ;        // slope
  regressed->intercept     = (sumYA*sumXXA - sumXA*sumXYA) / delta ;        // intercept
  regressed->slope_err     = sqrt(sumWA/delta);                             // slope error
  regressed->intercept_err = sqrt(sumXXA/delta);                            // intercept error

  if (numA>1) return 0;
  else return -1;
  
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
    }

    else if (readTemperatures) 
    {
      float temp = tuff_getTemperature(device,i,1); 
      if (temp <= -273)  // this means it timed out or other error
      {

        if (temp == -999) 
        {
              syslog(LOG_ERR, "Tuffd self-terminating."); 
              sleep(2); 
              raise(SIGTERM); 
        }

        syslog(LOG_ERR," Reading temperature on irfcm %d timed out or otherwise failed, could it have dropped out?\n", i); 
        tuffStruct.temperatures[i] = 127; 
      }
      
      else if (temp < -127) tuffStruct.temperatures[i] = -127; //bottomed out... but hard to believe
      else if (temp > 127) tuffStruct.temperatures[i]  = 127; //maxed out... but hard to believe

      else 
      {
         tuffStruct.temperatures[i] = (char) (0.5  + temp); // round 
      }

    }
    else
    {

      tuffStruct.temperatures[i] = -128;; 
    }


    if (printToScreen) 
    {
      printf("Writing temperature %d for RFCM %d%s\n", tuffStruct.temperatures[i], i, readTemperatures ? "" : " (temperature reading disabled) "); 
    }

  }

  telemeterCount++; 

  fillGenericHeader(&tuffStruct, PACKET_TUFF_STATUS, sizeof(TuffNotchStatus_t)); 
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
  char tmpname[512]; 
  char bintmpname[512]; 
  FILE * outfile; 
  FILE * binfile; 
  time_t theTime; 
  unsigned short binstatus;

  time(&theTime); 

  tuffStruct.notchSetTime = theTime; 
  //open temporary file to avoid race conditions 
  sprintf(tmpname,"%s~",TUFF_NOTCH_LOOKUP); 
  outfile = fopen(tmpname,"w"); 
  sprintf(bintmpname,"%s~",TUFF_NOTCH_BIN_LOOKUP); 
  binfile = fopen(bintmpname,"w"); 
  if (!outfile) syslog(LOG_ERR, "Could not open %s for writing", tmpname); 


  if (outfile) fprintf(outfile,"Date: %s\n", ctime(&theTime));


  tuffStruct.notchSetTime = theTime; 

  for (i = 0; i < NUM_TUFF_NOTCHES; i++)
  {
    syslog(LOG_INFO, "Tuffd: Setting notch %d range to [%d %d],", i, tuffStruct.startSectors[i], tuffStruct.endSectors[i]); 
    binstatus = 0; 

    if (outfile) fprintf(outfile, "\nNOTCH %d: ", i); 
    if ( tuffStruct.startSectors[i] == 16 && tuffStruct.endSectors[i] == 16) 
    {
      if (outfile) fprintf(outfile, "DISABLED"); 
      if (binfile) fwrite(&binstatus,2,1,binfile); 
    }
    else
    {
      if (outfile) fprintf(outfile, "ENABLED FOR PHI SECTORS %d to %d", tuffStruct.startSectors[i]+1, tuffStruct.endSectors[i]+1); 

      if (binfile)
      {
        int j; 
        if (tuffStruct.startSectors[i] <= tuffStruct.endSectors[i]) 
        {
          for (j = tuffStruct.startSectors[i]; j <= tuffStruct.endSectors[i]; j++)
          {
            binstatus |= (1 << j); 
          }
        }
        else 
        {
          for (j = tuffStruct.startSectors[i]; j <= tuffStruct.endSectors[i] + 16; j++)
          {
            binstatus |= (1 << (j % 16)); 
          }

        }


        if (binfile) fwrite(&binstatus,2,1,binfile); 
      }
    }
    tuff_setNotchRange(device, i, tuffStruct.startSectors[i], tuffStruct.endSectors[i]); 
  }

  if (outfile)
  {

    fprintf(outfile,"\n");

    for (i = 0; i < NUM_RFCM; i++) 
    {
      if (!tuff_responds[i]) printf("IRFCM %d DID NOT RESPOND\n",i); 
    }
    fclose(outfile); 

    //rename on top of old file 
    rename(tmpname, TUFF_NOTCH_LOOKUP); 
  }

  if (binfile)
  {
    fclose(binfile);
    rename(bintmpname, TUFF_NOTCH_BIN_LOOKUP); 
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

  
  signal(SIGCLD, SIG_IGN);
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
//       disableNeobrick=kvpGetInt("disableNeobrick",1);
//       if(disableNeobrick)
//         bitmask&=~NEOBRICK_DISK_MASK;
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

int handleCaps()
{
  int kvpStatus; 
  int i;
  kvpReset(); 
  kvpStatus = configLoad("Tuffd.config","cap"); 
  if (kvpStatus == CONFIG_E_OK) 
  {
    int nread = NUM_TUFF_NOTCHES; 
    int tmp[NUM_TUFF_NOTCHES]; 
    kvpStatus = kvpGetIntArray("setCapsOnStartup",tmp ,&nread); 

    if (kvpStatus !=CONFIG_E_OK || nread != NUM_TUFF_NOTCHES)
    {
        syslog( LOG_ERR, "Problem reading in setCapsOnStartup. Turning all to zero"); 
        memset(setCaps,0,sizeof(setCaps)); 
    }
    else
    {
      memcpy(setCaps,tmp, sizeof(setCaps)); 

    }

    for (i = 0; i < NUM_TUFF_NOTCHES; i++)
    {
      if (setCaps[i]) 
      {
        char key[128]; 
        sprintf(key,"notch%dCaps",i); 
        int captmp[NUM_RFCM * NUM_TUFFS_PER_RFCM]; 
        nread = NUM_RFCM * NUM_TUFFS_PER_RFCM; 
        kvpStatus = kvpGetIntArray(key, captmp, &nread) ; 

        if (kvpStatus !=CONFIG_E_OK || nread != NUM_RFCM * NUM_TUFFS_PER_RFCM)
        {
          syslog(LOG_ERR, "Problem reading in %s Disabling setCaps[%d]", key,i); 
          setCaps[i] = 0; 
        }
        else
        {
          memcpy(capArray[i], captmp, sizeof(captmp)); 
        }
      }
    }
  }
  else 
  {
    return -1; 
  }

  int nfailed = 0; 

  for (i = 0; i < NUM_TUFF_NOTCHES; i++)
  {
    if (setCaps[i])
    {
      int rfcm, chan; 
      syslog(LOG_INFO,"Setting caps for notch %d",i); 
      for (rfcm = 0; rfcm < NUM_RFCM; rfcm++)
      {
        for (chan = 0; chan < NUM_TUFFS_PER_RFCM; chan++)
        {
          if (printToScreen) 
          {
            printf("trying to set cap to %d for notch %d on RFCM %d, chan %d\n", capArray[i][rfcm][chan],i,rfcm,chan); 
          }
          tuff_setCap(device, rfcm, chan, i, capArray[i][rfcm][chan]); 
          if(tuff_waitForAck(device, rfcm, 1))
          {
            syslog(LOG_ERR,"Timed out while trying to set cap to %d for notch %d on RFCM %d, chan %d\n", capArray[i][rfcm][chan],i,rfcm,chan); 
            fprintf(stderr,"Timed out while trying to set cap to %d for notch %d on RFCM %d, chan %d\n", capArray[i][rfcm][chan],i,rfcm,chan); 
            nfailed++; 
          }
        }
      }
    }
  }

  return nfailed; 
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

    sleepAmount = kvpGetInt("sleepAmount",1); 
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
    int nread = 2*NUM_TUFF_NOTCHES;
    int tmp[3];
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


    useMagnetometerForHeading = kvpGetInt("useMagnetometerForHeading", 0); 
    maxHeadingAge = kvpGetInt("maxHeadingAge", 120); 


    nread = NUM_TUFF_NOTCHES; 
    kvpStatus = kvpGetIntArray("adjustAccordingToHeading", tmp, &nread);
    if (kvpStatus != CONFIG_E_OK || nread != NUM_TUFF_NOTCHES) 
    {
      syslog(LOG_ERR, "Problem reading in adjsut according to heading, turning off"); 
      memset(adjustAccordingToHeading,0, sizeof(adjustAccordingToHeading)); 
    }
    else
    {
      if (printToScreen)
      {
        int i; 
        printf(" Setting adjustAccording to heading to: [ "); 
        for (i = 0; i < 3; i++) printf( "%d ", tmp[i]); 
        printf("]\n"); 
      }

      memcpy(adjustAccordingToHeading, tmp, sizeof(adjustAccordingToHeading)); 
    }
    

    nread = NUM_TUFF_NOTCHES; 
    kvpStatus = kvpGetIntArray("degreesFromNorthToNotch", tmp, &nread);
    if (kvpStatus != CONFIG_E_OK || nread != NUM_TUFF_NOTCHES) 
    {
      int i ; 
      syslog(LOG_ERR, "Problem reading in degreesFromNorthToNotch, turning to 90"); 
      for (i = 0; i < 3; i++) degreesFromNorthToNotch[i] = 90; 
    }
    else
    {
      if (printToScreen)
      {
        int i; 
        printf(" Setting degreesFromNorthToNotch to heading to: [ "); 
        for (i = 0; i < 3; i++) printf( "%d ", tmp[i]); 
        printf("]\n"); 
      }

      memcpy(degreesFromNorthToNotch, tmp, sizeof(degreesFromNorthToNotch)); 
    }
    

    nread = NUM_TUFF_NOTCHES; 
    kvpStatus = kvpGetIntArray("slopeThresholdToNotchNextSector", tmp, &nread);
    if (kvpStatus != CONFIG_E_OK || nread != NUM_TUFF_NOTCHES) 
    {
      int i;
      syslog(LOG_ERR, "Problem reading in slopeThresholdToNotchNextSector, turning to 23"); 
      for (i = 0; i < 3; i++) slopeThresholdToNotchNextSector[i] = 23; 
    }
    else
    {
      if (printToScreen)
      {
        int i; 
        printf(" Setting slopeThresholdToNotchNextSector to heading to: [ "); 
        for (i = 0; i < 3; i++) printf( "%d ", tmp[i]); 
        printf("]\n"); 
      }

      memcpy(slopeThresholdToNotchNextSector, tmp, sizeof(slopeThresholdToNotchNextSector)); 
    }

    nread = NUM_TUFF_NOTCHES; 
    kvpStatus = kvpGetIntArray("phiSectorOffsetFromNorth", tmp, &nread);
    if (kvpStatus != CONFIG_E_OK || nread != NUM_TUFF_NOTCHES) 
    {
      int i ; 
      syslog(LOG_ERR, "Problem reading in phiSectorOffsetFromNorth, turning to 45"); 
      for (i = 0; i < 3; i++) phiSectorOffset[i] = 45; 
    }
    else
    {
      if (printToScreen)
      {
        int i; 
        printf(" Setting phiSectorOffsetFromNorth  to: [ "); 
        for (i = 0; i < 3; i++) printf( "%d ", tmp[i]); 
        printf("]\n"); 
      }

      memcpy(phiSectorOffset, tmp, sizeof(phiSectorOffset)); 
    }
 

  }
  else { configStatus+=4; }

  memcpy(&configTuffStruct, &tuffStruct, sizeof(configTuffStruct)); 


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
  memset(setCaps, 0, sizeof(zeroes)); 

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
    // Well, why don't we try running anita serial service again? 
    syslog(LOG_ERR,"Trying to restart anitaserial service"); 
    sleep(1); //just in case 
    system("systemctl restart anitaserial") ;
    device = tuff_open(TUFF_DEVICE);

    if (!device) 
    {
      syslog(LOG_ERR,"Tuffd could not open the Tuff. Aborting.."); 
      cleanup(); 
    return 1; 
    }
  }
 

  tuff_fd = tuff_getfd(device); 


  // reset and ping the tuffs 
  // This is a bit silly... resetting multiple times shouldn't really help at all, 
  // but whatever! 
  for (i = 0; i < NUM_RFCM; i++) 
  {
    unsigned rfcm = i ; 
    int gotit; 
    if (nattempts == 0) 
    {
      printf("Resetting RFCM %d\n", i); 
      tuff_setQuietMode(device,false); 
      tuff_reset(device, i); 
      tuff_setQuietMode(device,false); 
      gotit = tuff_waitForAck(device,i,1); 
      if (!gotit) 
      {
          printf("Got ack from %d!\n", i); 
      }
    }

    //first try to ping them 
    gotit = tuff_pingiRFCM(device,1,1,&rfcm); 
    printf("%d %d\n", i, gotit) ; 
    
    if (!gotit) 
    {
      printf("Resetting RFCM %d\n", i); 
      tuff_setQuietMode(device,false); 
      tuff_reset(device, i); 
      tuff_setQuietMode(device,false); 
      gotit = tuff_waitForAck(device,i,1); 
      if (!gotit) 
      {
        printf("Got ack from %d!\n", i); 
      }
//      sleep(1); 
      if(!tuff_pingiRFCM(device,2,1,&rfcm))
      {
        fprintf(stderr, "Did not get ping from TUFF %d in 2 seconds... trying to reset again. nattempts=%d \n",i, nattempts); 
        syslog(LOG_INFO, "Did not get ping from TUFF %d in 2 seconds... trying to reset again. nattempts=%d \n",i,nattempts); 
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

  //set caps if we want 
  handleCaps(); 

  //shut them up
  tuff_setQuietMode(device,true); 

  // set up raw command watch director
  

  makeDirectories(TUFF_RAWCMD_LINK_DIR); 
  makeDirectories(GPSD_HEADING_LINK_DIR); 
  makeDirectories(MAGNETOMETER_LINK_DIR); 

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

    // Don't read config if we know it's the HKd  or GPS, since that doesn't modify the config
    if (senderOfSigUSR1 != ID_GPSD || senderOfSigUSR1 != ID_HKD)
    {
      read_config_ok = readConfig(); 
    }
    else
    {
      if (printToScreen) 
      {
        if (senderOfSigUSR1 == ID_GPSD) 
          printf("Woke up by GPSd!\n"); 
        if (senderOfSigUSR1 == ID_HKD) 
          printf("Woke up by HKd!\n"); 
      }
    }


    if (read_config_ok)
    {
      syslog(LOG_ERR, "Tuffd had trouble reading the config. Returned: %d", read_config_ok); 
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
      if (memcmp(adjustAccordingToHeading, zeroes, sizeof(zeroes)))
      {
        //this means we couldn't adjust according to heading for some reason, so we should set the
        //notches back to those specified in the config 
        if (analyzeHeading() < 0) 
        {
            memcpy(tuffStruct.startSectors, configTuffStruct.startSectors, sizeof(tuffStruct.startSectors)); 
            memcpy(tuffStruct.endSectors, configTuffStruct.endSectors, sizeof(tuffStruct.endSectors)); 
        }
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


      retVal = writeState(justChanged); 

      if (retVal) 
      {
        syslog(LOG_ERR,"writeState returned %d\n", retVal); 

      }
      justChanged = 0; 


      if (currentState == PROG_STATE_RUN) //check again, although still technically a race condition
      {
        struct pollfd pollstruct; 
        int pollRet; 
        pollstruct.fd = tuff_fd; 
        pollstruct.events = POLLIN; 
        pollRet =  poll(&pollstruct, 1, 1000 * (sleepAmount < 1 ? 1 : sleepAmount)); 
        while (pollRet > 0 && pollstruct.revents & POLLIN) 
        {
          char buf[256]; 
          int first_time = 1; 
          int nread = 0; 
          memset(buf,'.' , sizeof(buf)); 
          while (nread  < pollRet) 
          {
            char * where; 
            nread+= read(tuff_fd, first_time ? buf : buf + 128, first_time ? 256: 128); 
            // this is a dumb trick to avoid splitting a message across reads
            if (!first_time)
            {
              memcpy(buf, buf+128, 128); 
            }

            if (( where = strstr(buf,"boot")))
            {

              char * whereto = strchr(where, 'v'); 

              if (!whereto) // this means we haven't read this far yet, so let's continue reading 
                continue; 

              *whereto = 0; 

              syslog(LOG_ERR, "Tuffd detected irfcm reset. Message: \"%s\"", where); 
              fprintf(stderr,"Detected irfcm reset. Quitting after 2 seconds\n"); 
              sleep(2); 
              raise(SIGTERM); 
            }


            first_time = 0; 
          }

          pollRet =  poll(&pollstruct, 1, 10); 
        }
      }
    }

  } while(currentState == PROG_STATE_INIT); 

  cleanup(); 

  return 0; 
}


