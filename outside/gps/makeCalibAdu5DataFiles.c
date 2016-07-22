/*! \file makeCalibAdu5DataFiles
    \brief The program which make data files for the ADU5 Calibration
    
    Talks to one of the ADU5 GPS Calibration units and tries to generate the binary files needed for GPS calibration. Almost certainly doesn't work yet, but maybe it will in the long term

    July 2016 r.nichol@ucl.ac.uk
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

#include "includes/anitaFlight.h"
#include "includes/anitaStructures.h"
#include "serialLib/serialLib.h"
#include "gpsToolsLib/gpsToolsLib.h"

#define COMMAND_SIZE 1024
#define DATA_SIZE 10240
#define COMMAND "$PASHS,ELM,0\r\n"  /* set elevation mask angle to 0 degree */


int fillBFileRawNav(RawAdu5BFileRawNav_t *bFileRawNav, RawAdu5PBNStruct_t pbn){

  sprintf(bFileRawNav->sitename, "%s", pbn.sitename);
  bFileRawNav->rcv_time=pbn.pben_time/1e3;
  bFileRawNav->navx=pbn.navx;
  bFileRawNav->navy=pbn.navy;
  bFileRawNav->navz=pbn.navz;
  bFileRawNav->navxdot=pbn.navxdot;
  bFileRawNav->navydot=pbn.navydot;
  bFileRawNav->navzdot=pbn.navzdot;
  bFileRawNav->navt=(double)pbn.navt;
  bFileRawNav->navtdot=(double)pbn.navtdot;
  bFileRawNav->pdop=pbn.pdop;
  bFileRawNav->num_sats=0;


  return 0;
}

int fillBFileSatHeader(RawAdu5BFileSatelliteHeader_t *bFileSatHeader, RawAdu5MBNStruct_t mbn){

  bFileSatHeader->svprn=mbn.svpm;
  bFileSatHeader->elevation=mbn.el;
  bFileSatHeader->azimuth=mbn.az;
  bFileSatHeader->chnind=mbn.chnind;

  return 0;
}

int fillBFileChanObs( RawAdu5BFileChanObs_t *bFileChanObs,  RawAdu5MBNStruct_t mbn){

  bFileChanObs->raw_range=mbn.raw_range;
  bFileChanObs->smth_corr= (float)((mbn.smoothing && 0xff0000)>>24);
  bFileChanObs->smth_count=(mbn.smoothing && 0xffffff);
  bFileChanObs->polarity_known=mbn.polarity_know;
  bFileChanObs->warning=mbn.warn;
  bFileChanObs->goodbad=mbn.good_bad;
  bFileChanObs->ireg=mbn.ireg;
  bFileChanObs->qa_phase=mbn.qa_phase;
  bFileChanObs->doppler=mbn.doppler;
  bFileChanObs->carphase=mbn.full_phase;

  return 0;
}


int main(int argc, char **argv) {
  if(argc<3) {
    printf("Usage:\n%s <A or B> <num seconds>\n",argv[0]);
    return -1;
  }

  char *whichAdu5Dev=ADU5A_DEV_NAME;
  if(argv[1][0]=='B') whichAdu5Dev=ADU5B_DEV_NAME;
  int numSeconds=atoi(argv[2]);  
  int retVal=0;
  int fdAdu5;

  printf("Trying to calibrate %s by taking data for %d seconds\n",whichAdu5Dev,numSeconds);
  
  retVal=openGpsDevice(whichAdu5Dev);	
  if(retVal<=0) {
    printf("Couldn't open: %s\n",whichAdu5Dev);
    return -1;
  }
  else fdAdu5=retVal;


 
  
  char buff[COMMAND_SIZE];
  char data[DATA_SIZE]={' '};
  int bytesRead=0;
  int i=0;
  
  time_t startTime = time(NULL);

  sprintf(buff, "$PASHQ,PRT\r\n");
  strcat(buff,"$PASHS,NME,ALL,OFF\r\n"); //Turn off NMEA messages
  strcat(buff,"$PASHS,OUT,A\r\n"); //Turn off raw data messages to port A
  strcat(buff,"$PASHS,OUT,B\r\n"); //Turn off raw data messages to port B
  strcat(buff,"$PASHS,ELM,10\r\n"); //Set elevation mask to 10 degrees
  strcat(buff,"$PASHS,RCI,001.0\r\n"); //Set raw data rate to 1 second
  strcat(buff,"$PASHS,PDS,OFF\r\n"); // Special setting needed for calibration must be switched back on at end
  strcat(buff,"$PASHS,OUT,A,MBN,SNV,PBN,ATT,BIN\r\n"); //Request the output

  /* send the commands to ADU5  */
  write(fdAdu5, buff, strlen(buff));
  
  RawAdu5MBNStruct_t mbnPtr;
  RawAdu5PBNStruct_t pbnPtr;
  RawAdu5SNVStruct_t snvPtr;
  RawAdu5ATTStruct_t attPtr;

  struct RawAdu5BFileHeader bFileHeader;
  struct RawAdu5BFileRawNav bFileRawNav;
  struct RawAdu5BFileSatelliteHeader bFileSatHeader[100]; //Arbitray large array
  struct RawAdu5BFileChanObs bFileChanObs[100];

  int numSat = 0;
  fillDefaultBFileHeader(&bFileHeader);

  FILE * pFile;
  pFile = fopen ("myCalibFile.bin", "wb");

  // First we write the RawHeadFile
  fwrite (&bFileHeader , sizeof(char), sizeof(RawAdu5BFileHeader_t), pFile);

  for(i=0; i < strlen(buff); i++) printf("%c", buff[i]);
  
  printf("\n");
  sleep(1);
  
  /* read & print output */
  char tempData[DATA_SIZE];
  // Data stuff for ADU5                                                                                                         
  time_t nowTime = time(NULL);
  while(nowTime < startTime + numSeconds) {
    static char adu5Output[DATA_SIZE]="";
    static int adu5OutputLength=0;
    static int lastStar=-10;
    sleep(1);
    retVal=isThereDataNow(fdAdu5);
    usleep(5);
    //    printf("Check ADU5A got retVal %d\n",retVal);                                                                            
    if(retVal!=1) {
      sleep(1);
      nowTime=time(NULL);
      continue;
    }
    retVal=read(fdAdu5, tempData, DATA_SIZE);
    printf("Num bytes %d\n",retVal);
    if(retVal>0) {
      for(i=0; i < retVal; i++) {
	printf("%c", tempData[i]);
	if(adu5OutputLength==0 && tempData[i]!='$') continue;
	adu5Output[adu5OutputLength++]=tempData[i];
	//	bytesRead=read(fdAdu5, data, DATA_SIZE);
	//	printf("read  returned: %d\n",bytesRead);
	if(adu5OutputLength>1) {
	  if(adu5Output[0]=='$' && adu5Output[adu5OutputLength-1]=='$') {
	    printf("Got two dollars\t%d\n",adu5OutputLength);
	    if(adu5Output[1]=='P' && adu5Output[2]=='A' && adu5Output[3]=='S' && 
	       adu5Output[4]=='H' && adu5Output[5]=='R' && adu5Output[6]==',') {
	      printf("Got $PASHR,\n");
	      if(adu5Output[7]=='A' && adu5Output[8]=='T' && adu5Output[9]=='T') {
		if(adu5OutputLength-1==61) {
		  printf("Got ATT\n");		
		  //Right length
		  fillRawATTStruct(adu5Output, adu5OutputLength, &attPtr);
		  adu5OutputLength=1;
		  continue;
		}
		else if(adu5OutputLength-1<61) continue;
	      }
	      else if(adu5Output[7]=='P' && adu5Output[8]=='B' && adu5Output[9]=='N') {
		if(adu5OutputLength-1==69) {
		  printf("Got PBN\n");		
		  //Right length
		  fillRawPBNStruct(adu5Output, adu5OutputLength, &pbnPtr);
		  fillBFileRawNav(&bFileRawNav, pbnPtr);

		  numSat = 0;
		  
		  retVal=read(fdAdu5, tempData, DATA_SIZE);
		  adu5Output[adu5OutputLength++]=tempData[i];
		  while (adu5Output[7]=='M' && adu5Output[8]=='C' && adu5Output[9]=='A') {
		    //Handle MCA
		    if(adu5OutputLength-1==50) {
		      printf("Got MCA\n");		
		      //Right length
		      fillRawMBNStruct(adu5Output, adu5OutputLength, &mbnPtr);
		      fillBFileSatHeader(&bFileSatHeader[numSat], mbnPtr);
		      fillBFileChanObs(&bFileChanObs[numSat],  mbnPtr);
		    
		      adu5OutputLength=1;
		      adu5Output[adu5OutputLength++]=tempData[i];
		      retVal=read(fdAdu5, tempData, DATA_SIZE);
		      numSat++;
		      continue;
		    }

		  }
		  bFileRawNav.num_sats=(char)numSat;
		  fwrite ( &bFileRawNav  , sizeof(char), sizeof(RawAdu5BFileRawNav_t), pFile);
		  int isat;
		  for (isat=0;isat<numSat;isat++){
		      fwrite (&bFileSatHeader[numSat] , sizeof(char), sizeof(RawAdu5BFileSatelliteHeader_t), pFile);
		      fwrite (&bFileChanObs[numSat] , sizeof(char), sizeof(RawAdu5BFileChanObs_t), pFile);
		  }
		  adu5OutputLength=1;
		  continue;
		}
		else if(adu5OutputLength-1<69) continue;
	      }
	      else if(adu5Output[7]=='S' && adu5Output[8]=='N' && adu5Output[9]=='V') {
		if(adu5OutputLength-1==145) {
		  printf("Got SNV\n");
		  //Right length
		  fillRawSNVStruct(adu5Output, adu5OutputLength, &snvPtr);

		  adu5OutputLength=1;
		  continue;
		}
		else if(adu5OutputLength-1<145) continue;
	      }
	      else if(adu5Output[7]=='T' && adu5Output[8]=='T' && adu5Output[9]=='T') {
		if(adu5OutputLength-1==34) {
		  printf("Got TTT\n");		
		  //Right length
		  
		  adu5OutputLength=1;
		  continue;
		}
		else if(adu5OutputLength-1<34) continue;
	      }

	      
	    }
	    if(adu5OutputLength>30) { //RJN arbitrary number for now
	      //Reset the counter for now
	      printf("Resetting counter\n");
	      adu5OutputLength=1;
	      continue;
	    }
 
	  }
	}
      }
    }
    usleep(500);	
    nowTime=time(NULL);
  }
      
  // Steps for real
  // Work out
  fclose (pFile);
  

  
  sprintf(buff, "$PASHQ,PRT\r\n");
  strcat(buff,"$PASHQ,RIO\r\n");
  strcat(buff,"$PASHS,NME,ALL,OFF\r\n"); //Turn off NMEA messages
  strcat(buff,"$PASHS,OUT,A\r\n"); //Turn off raw data messages to port A
  strcat(buff,"$PASHS,OUT,B\r\n"); //Turn off raw data messages to port B
  strcat(buff,"$PASHS,ELM,10\r\n"); //Set elevation mask to 10 degrees
  strcat(buff,"$PASHS,PDS,ON\r\n"); // Special setting needed for calibration must be switched back on at end
  
  write(fdAdu5, buff, strlen(buff));
  sleep(5);
  {
    bytesRead=read(fdAdu5, data, DATA_SIZE);
    printf("read  returned: %d\n",bytesRead);
    if(bytesRead>0) {
      for(i=0; i <bytesRead; i++) printf("%c", data[i]);
      //	    printf("\n");
    }
  }
  
  
  close(fdAdu5);

}
