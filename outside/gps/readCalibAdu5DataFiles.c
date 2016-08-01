/*! \file readCalibAdu5DataFiles
    \brief The program which make data files for the ADU5 Calibration
    
    Talks to one of the ADU5 GPS Calibration units and tries to generate the binary files needed for GPS calibration. Almost certainly doesn't work yet, but maybe it will in the long term

    July 2016 r.nichol@ucl.ac.uk
*/


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

#include "includes/anitaStructures.h"

int main(int argc, char **argv) {
  if(argc<2) {
    printf("Usage:\n%s <file>\n",argv[0]);
    return -1;
  }

  printf("sizeof(RawAdu5BFileHeader_t)=%lu\n",sizeof(RawAdu5BFileHeader_t));
  printf("sizeof(RawAdu5BFileRawNav_t)=%lu\n",sizeof(RawAdu5BFileRawNav_t));
  printf("sizeof(RawAdu5BFileChanObs_t)=%lu\n",sizeof(RawAdu5BFileChanObs_t));
  printf("sizeof(RawAdu5BFileSatelliteHeader_t)=%lu\n",sizeof(RawAdu5BFileSatelliteHeader_t));
  printf("sizeof(RawAdu5MBNStruct_t)=%lu\n",sizeof(RawAdu5MBNStruct_t));
  printf("sizeof(RawAdu5SNVStruct_t)=%lu\n",sizeof(RawAdu5SNVStruct_t));
  printf("sizeof(RawAdu5PBNStruct_t)=%lu\n",sizeof(RawAdu5PBNStruct_t));
  printf("sizeof(RawAdu5ATTStruct_t)=%lu\n",sizeof(RawAdu5ATTStruct_t));


  struct RawAdu5BFileHeader bFileHeader;
  struct RawAdu5BFileRawNav bFileRawNav;
  struct RawAdu5BFileSatelliteHeader bFileSatHeader[100]; //Arbitray large array
  struct RawAdu5BFileChanObs bFileChanObs[100];
  
  FILE *bFilePtr;

  double whatTime = 0;
  unsigned int slips = 0;
  bFilePtr=fopen(argv[1],"rb");
  if (!bFilePtr) {
    printf("Unable to open file!");
    return 1;
  }


  fread(&bFileHeader,sizeof(RawAdu5BFileHeader_t),1,bFilePtr);

  printf("version:\t%s\n",bFileHeader.version);
  printf("raw_version:\t%d\n",(int)bFileHeader.raw_version);
  printf("rcvr_type:\t%s\n",bFileHeader.rcvr_type);
  printf("chan_ver:\t%s\n",bFileHeader.chan_ver);
  printf("nav_ver:\t%s\n",bFileHeader.nav_ver);
  printf("capability:\t%d\n",bFileHeader.capability);
  printf("wb_start:\t%d\n",bFileHeader.wb_start);
  printf("num_obs_type:\t%d\n",(int)bFileHeader.num_obs_type);

  int countStuff=0;
  while(1) {
    int numObjs=fread(&bFileRawNav,sizeof(RawAdu5BFileRawNav_t),1,bFilePtr);
    printf("countStuff=%d numObjs=%d sizeof(RawAdu5BFileRawNav_t)=%lu\n",countStuff,numObjs,sizeof(RawAdu5BFileRawNav_t));
    //    break;
    if(numObjs<1) break;
    countStuff++;
    
    if (bFileRawNav.rcv_time-whatTime>1){
      slips++;
      printf("whatTime: %lf bFileRawNav.rcv_time:\t%lf\n",whatTime, bFileRawNav.rcv_time);
    }
    whatTime=bFileRawNav.rcv_time;
    /* printf("bFileRawNav.sitename:\t%s\n",bFileRawNav.sitename);  */
    /* printf("bFileRawNav.rcv_time:\t%lf\n",bFileRawNav.rcv_time);  */
    /* printf("bFileRawNav.navx:\t%lf\n",bFileRawNav.navx);  */
    /* printf("bFileRawNav.navy:\t%lf\n",bFileRawNav.navy);  */
    /* printf("bFileRawNav.navz:\t%lf\n",bFileRawNav.navz);  */
    /* printf("bFileRawNav.navxdot:\t%f\n",bFileRawNav.navxdot);  */
    /* printf("bFileRawNav.navydot:\t%f\n",bFileRawNav.navydot);  */
    /* printf("bFileRawNav.navzdot:\t%f\n",bFileRawNav.navzdot);  */
    /* printf("bFileRawNav.navt:\t%lf\n",bFileRawNav.navt);  */
    /* printf("bFileRawNav.navtdot:\t%lf\n",bFileRawNav.navtdot);  */
    /* printf("bFileRawNav.pdop:\t%d\n",bFileRawNav.pdop);  */
    /* printf("bFileRawNav.num_sats:\t%d\n",(int)bFileRawNav.num_sats);  */
    

    //    return 1;
    int satNum=0;
    for(satNum=0;satNum<(int)bFileRawNav.num_sats;satNum++) {
      fread(&bFileSatHeader[satNum],sizeof(RawAdu5BFileSatelliteHeader_t),1,bFilePtr);
      /* printf("bFileSatHeader[%d].svprn:\t%d\n",satNum,(int)bFileSatHeader[satNum].svprn);  */
      /* printf("bFileSatHeader[%d].elevation:\t%d\n",satNum,(int)bFileSatHeader[satNum].elevation);  */
      /* printf("bFileSatHeader[%d].azimuth:\t%d\n",satNum,(int)bFileSatHeader[satNum].azimuth);  */
      /* printf("bFileSatHeader[%d].chnind:\t%d\n",satNum,(int)bFileSatHeader[satNum].chnind);  */
      
      
      fread(&bFileChanObs[satNum],sizeof(RawAdu5BFileChanObs_t),1,bFilePtr);
      /* printf("bFileChanObs[%d].raw_range:\t%lf\n",satNum,bFileChanObs[satNum].raw_range); */
      /* printf("bFileChanObs[%d].smth_corr:\t%f\n",satNum,bFileChanObs[satNum].smth_corr); */
      /* printf("bFileChanObs[%d].smth_count:\t%d\n",satNum,bFileChanObs[satNum].smth_count); */
      /* printf("bFileChanObs[%d].polarity_known:\t%d\n",satNum,(int)bFileChanObs[satNum].polarity_known); */
      /* printf("bFileChanObs[%d].warning:\t%d\n",satNum,(int)bFileChanObs[satNum].warning); */
      /* printf("bFileChanObs[%d].goodbad:\t%d\n",satNum,(int)bFileChanObs[satNum].goodbad); */
      /* printf("bFileChanObs[%d].ireg:\t%d\n",satNum,(int)bFileChanObs[satNum].ireg); */
      /* printf("bFileChanObs[%d].qa_phase:\t%d\n",satNum,(int)bFileChanObs[satNum].qa_phase); */
      /* printf("bFileChanObs[%d].doppler:\t%d\n",satNum,bFileChanObs[satNum].doppler); */
      /* printf("bFileChanObs[%d].carphase:\t%lf\n",satNum,bFileChanObs[satNum].carphase); */
      //    break;
    }
  }
    fclose(bFilePtr);
    printf("Number of slips: \t%d\n",slips);
    return 0;
    
  }
