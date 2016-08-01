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
#include <endian.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>

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


  /* printf("bFileRawNav.sitename:\t%s\n",bFileRawNav->sitename); */
  /* printf("bFileRawNav.rcv_time:\t%lf\n",bFileRawNav->rcv_time); */
  /* printf("bFileRawNav.navx:\t%lf\n",bFileRawNav->navx); */
  /* printf("bFileRawNav.navy:\t%lf\n",bFileRawNav->navy); */
  /* printf("bFileRawNav.navz:\t%lf\n",bFileRawNav->navz); */
  /* printf("bFileRawNav.navxdot:\t%f\n",bFileRawNav->navxdot); */
  /* printf("bFileRawNav.navydot:\t%f\n",bFileRawNav->navydot); */
  /* printf("bFileRawNav.navzdot:\t%f\n",bFileRawNav->navzdot); */
  /* printf("bFileRawNav.navt:\t%lf\n",bFileRawNav->navt); */
  /* printf("bFileRawNav.navtdot:\t%lf\n",bFileRawNav->navtdot); */
  /* printf("bFileRawNav.pdop:\t%d\n",bFileRawNav->pdop); */
  /* printf("bFileRawNav.num_sats:\t%d\n",(int)bFileRawNav->num_sats); */

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
  bFileChanObs->smth_count=(unsigned short)(mbn.smoothing && 0xffffff);
  bFileChanObs->polarity_known=mbn.polarity_know;
  bFileChanObs->warning=mbn.warn;
  bFileChanObs->goodbad=mbn.good_bad;
  bFileChanObs->ireg=mbn.ireg;
  bFileChanObs->qa_phase=mbn.qa_phase;
  bFileChanObs->doppler=mbn.doppler;
  bFileChanObs->carphase=mbn.full_phase;

  return 0;
}

int fillEFileStruct( RawAdu5EFileStruct_t *eFileStruct,  RawAdu5SNVStruct_t snv){

  eFileStruct->prnnum               = snv.prnnum               ;
  eFileStruct->weekNumber	    = snv.weekNumber	       ;
  eFileStruct->secondsInWeek	    = snv.secondsInWeek	       ;
  eFileStruct->groupDelay	    = snv.groupDelay	       ;
  eFileStruct->aodc		    = snv.aodc		       ;
  eFileStruct->toc		    = snv.toc		       ;
  eFileStruct->af2		    = snv.af2		       ;
  eFileStruct->af1		    = snv.af1		       ;
  eFileStruct->af0		    = snv.af0		       ;
  eFileStruct->aode		    = snv.aode		       ;
  eFileStruct->deltaN	            = snv.deltaN	       ;
  eFileStruct->m0		    = snv.m0		       ;
  eFileStruct->eccentricity	    = snv.eccentricity	       ;
  eFileStruct->roota		    = snv.roota		       ;
  eFileStruct->toe		    = snv.toe		       ;
  eFileStruct->cic		    = snv.cic		       ;
  eFileStruct->crc		    = snv.crc		       ;
  eFileStruct->cis		    = snv.cis		       ;
  eFileStruct->crs		    = snv.crs		       ;
  eFileStruct->cuc		    = snv.cuc		       ;
  eFileStruct->cus		    = snv.cus		       ;
  eFileStruct->omega0	            = snv.omega0	       ;
  eFileStruct->omega		    = snv.omega		       ;
  eFileStruct->i0		    = snv.i0		       ;
  eFileStruct->omegadot	            = snv.omegadot	       ;
  eFileStruct->idot		    = snv.idot		       ;
  eFileStruct->accuracy	            = snv.accuracy	       ;
  eFileStruct->health	            = snv.health	       ;
  eFileStruct->fit		    = snv.fit		       ;

  return 0;
}

int fillAFileStruct( RawAdu5AFileStruct_t *aFileStruct,  RawAdu5ATTStruct_t att){

  aFileStruct->head        = att.head       ;
  aFileStruct->roll        = att.roll       ;
  aFileStruct->pitch       = att.pitch      ;
  aFileStruct->brms        = att.brms       ;
  aFileStruct->mrms        = att.mrms       ;
  aFileStruct->timeOfWeek  = att.timeOfWeek ;
  aFileStruct->reset       = att.reset      ;
  aFileStruct->spare       = att.spare      ;
  
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
  strcat(buff,"$PASHS,NME,ALL,A,OFF\r\n"); //Turn off NMEA messages
  strcat(buff,"$PASHS,OUT,A\r\n"); //Turn off raw data messages to port A
  strcat(buff,"$PASHS,OUT,B\r\n"); //Turn off raw data messages to port B
  strcat(buff,"$PASHS,ELM,10\r\n"); //Set elevation mask to 10 degrees
  strcat(buff,"$PASHS,RCI,001.0\r\n"); //Set raw data rate to 1 second
  strcat(buff,"$PASHS,PDS,OFF\r\n"); // Special setting needed for calibration must be switched back on at end
  strcat(buff,"$PASHS,SPD,A,9\r\n");
  //  strcat(buff,"$PASHS,OUT,A,MBN,SNV,PBN,ATT,BIN\r\n"); //Request the output
  strcat(buff,"$PASHS,OUT,A,MBN,SNV,PBN,BIN\r\n"); //Request the output


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

  struct RawAdu5EFileStruct eFileStruct;
  struct RawAdu5AFileStruct aFileStruct;

  int numSat = 0;
  unsigned int numPbn=0;
  unsigned int numErrors=0;
  unsigned int numSnv=0;
  unsigned int mben_left=-999;
  unsigned short sequence_tag = 0;
  fillDefaultBFileHeader(&bFileHeader);

  printf("version:\t%s\n",bFileHeader.version);
  printf("raw_version:\t%d\n",(int)bFileHeader.raw_version);
  printf("rcvr_type:\t%s\n",bFileHeader.rcvr_type);
  printf("chan_ver:\t%s\n",bFileHeader.chan_ver);
  printf("nav_ver:\t%s\n",bFileHeader.nav_ver);
  printf("capability:\t%d\n",bFileHeader.capability);
  printf("wb_start:\t%d\n",bFileHeader.wb_start);
  printf("num_obs_type:\t%d\n",(int)bFileHeader.num_obs_type);

  char timenow[32];
  strftime(timenow, 32, "%Y_%m_%d_time_%H_%M_%S", localtime(&startTime)); 
  char dir[200];
  sprintf(dir, "/mnt/helium1/gps/gpsCalibTest/*s", timenow);
  mkdir (dir, 0777);
  char dayOfYear[32];
  strftime(dayOfYear, 32, "%j", localtime(&startTime));

  // B stands for binary satellite measurement file
  // CALI is a user-defined site name, four-character filename template
  // A is the session designator, a letter from A to Z
  // 16 is the last two digits of current year
  // 1200 is the day of the year

  char filename[100];
  sprintf(filename, "%s/BCALIA16.%s", dir, dayOfYear);
  FILE * BFile;
  BFile = fopen (filename, "wb");

  // E = Ephemeris file
  // CALI = Site name
  // A = Session letter, from A to Z
  // 02 = Last two digits of current year
  // 163 = Day of year

  FILE * EFile;
  sprintf(filename, "%s/ECALIA16.%s", dir, dayOfYear);
  EFile = fopen (filename, "wb");

  FILE * AFile;
  sprintf(filename, "%s/ACALIA16.%s", dir, dayOfYear);
  AFile = fopen (filename, "wb");

  // First we write the RawHeadFile
  fwrite (&bFileHeader , sizeof(char), sizeof(RawAdu5BFileHeader_t), BFile);


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
//    static int lastStar=-10;
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
    /* printf("Num bytes %d\n",retVal);  */
    if(retVal>0) {
      for(i=0; i < retVal; i++) {
	/* printf("%c", tempData[i]);   */
	if(adu5OutputLength==0 && tempData[i]!='$') continue;
	adu5Output[adu5OutputLength++]=tempData[i];
	//	bytesRead=read(fdAdu5, data, DATA_SIZE); 
	/* printf("read  returned: %d\n",bytesRead); */
	if(adu5OutputLength>1) {
	  if(adu5Output[0]=='$' && adu5Output[adu5OutputLength-1]=='$') {
	    /* printf("Got two dollars\t%d\n",adu5OutputLength); */
	    if(adu5Output[1]=='P' && adu5Output[2]=='A' && adu5Output[3]=='S' && 
	       adu5Output[4]=='H' && adu5Output[5]=='R' && adu5Output[6]==',') {
	      /* printf("Got $PASHR,\n"); */
	      if(adu5Output[7]=='A' && adu5Output[8]=='T' && adu5Output[9]=='T') {
		if(adu5OutputLength-1==61) {
		  /* printf("Got ATT\n");		  */
		  //Right length
		  /* fillRawATTStruct(adu5Output, adu5OutputLength-1, &attPtr);  */
		  /* fillAFileStruct(&aFileStruct, attPtr); */
		  /* fwrite (&aFileStruct , sizeof(char), sizeof(RawAdu5AFileStruct_t), AFile); */
		  adu5OutputLength=1;
		  continue;
		}
		else if(adu5OutputLength-1<61) continue;
	      }
	      else if(adu5Output[7]=='P' && adu5Output[8]=='B' && adu5Output[9]=='N') {
		if(adu5OutputLength-1==69) {
		  printf("Got PBN\n"); 
		  //Right length
		  fillRawPBNStruct(adu5Output, adu5OutputLength-1, &pbnPtr);
		  numSat++;
		  bFileRawNav.num_sats = (char)numSat;
		  fillBFileRawNav(&bFileRawNav, pbnPtr);
		  fwrite (&bFileRawNav , sizeof(char), sizeof(RawAdu5BFileRawNav_t), BFile);

		  if ((mben_left+1)!=numSat){
		    printf("PROBLEM: mben_left=%d and numSat=%d\n", mben_left+1, numSat);
		    numErrors++;
		  }

		  int isat=0;
		  for (isat=0;isat<numSat;isat++){
		    fwrite (&bFileSatHeader[isat] , sizeof(char), sizeof(RawAdu5BFileSatelliteHeader_t), BFile);
		    fwrite (&bFileChanObs[isat] , sizeof(char), sizeof(RawAdu5BFileChanObs_t), BFile);
		  }
		  numSat = 0;
		  adu5OutputLength=1;
		  numPbn++;
		  continue;
		} else if(adu5OutputLength-1<69) continue;
	      }
	      else if (adu5Output[7]=='M' && adu5Output[8]=='C' && adu5Output[9]=='A') { 
		//Handle MCA 
		    if(adu5OutputLength-1==50) { 
		      //Right length 

		      fillRawMBNStruct(adu5Output, adu5OutputLength-1, &mbnPtr);
		      if (sequence_tag==mbnPtr.sequence_tag){ 
			numSat++;
		      } else {
			sequence_tag=mbnPtr.sequence_tag;
			mben_left=mbnPtr.mben_left;
			numSat=0;
		      }
		       printf("Got MCA, sequence_tags: %u and %u numSat %d and mben_left: %d\n", sequence_tag, mbnPtr.sequence_tag, numSat, mbnPtr.mben_left); 
		      fillBFileSatHeader(&bFileSatHeader[numSat], mbnPtr); 
		      fillBFileChanObs(&bFileChanObs[numSat],  mbnPtr); 
		  

		      adu5OutputLength=1; 
		      continue; 
		    }  else if(adu5OutputLength-1<50) continue;

		  } 
	      else if(adu5Output[7]=='S' && adu5Output[8]=='N' && adu5Output[9]=='V') {
		if(adu5OutputLength-1==145) {
		   printf("Got SNV\n");  
		  /* //Right length */
		   fillRawSNVStruct(adu5Output, adu5OutputLength-1, &snvPtr); 
		   /* printf("snv.snvHeader:\t%s\n",     snvPtr.snvHeader); */
		   /* printf("snv.weekNumber:\t%u\n",    snvPtr.weekNumber); */
		   /* printf("snv.secondsInWeek:\t%d\n", snvPtr.secondsInWeek); */
		   /* printf("snv.groupDelay:\t%f\n",    snvPtr.groupDelay);   */
		   fillEFileStruct(&eFileStruct, snvPtr);


		   /* printf("eFileStruct.prnnum        :\t%d\n",  eFileStruct.prnnum        );       */
		   /* printf("eFileStruct.weekNumber   :\t%u\n",  eFileStruct.weekNumber   );       */
		   /* printf("eFileStruct.secondsInWeek :\t%d\n",  eFileStruct.secondsInWeek );       */
		   /* printf("eFileStruct.groupDelay   :\t%f\n",  eFileStruct.groupDelay   );       */
		   /* printf("eFileStruct.aodc   :\t%d\n",  eFileStruct.aodc   );       */
		   /* printf("eFileStruct.toc   :\t%d\n",  eFileStruct.toc   );       */
		   /* printf("eFileStruct.af2   :\t%f\n",  eFileStruct.af2   );       */
		   /* printf("eFileStruct.af1   :\t%f\n",  eFileStruct.af1   );       */
		   /* printf("eFileStruct.af0   :\t%f\n",  eFileStruct.af0   );       */
		   /* printf("eFileStruct.aode   :\t%d\n",  eFileStruct.aode   );       */
		   /* printf("eFileStruct.deltaN   :\t%f\n",  eFileStruct.deltaN   );       */
		   /* printf("eFileStruct.m0   :\t%f\n",  eFileStruct.m0   );       */
		   /* printf("eFileStruct.eccentricity  :\t%f\n",  eFileStruct.eccentricity  );       */
		   /* printf("eFileStruct.roota   :\t%f\n",  eFileStruct.roota   );       */
		   /* printf("eFileStruct.toe   :\t%d\n",  eFileStruct.toe   );       */
		   /* printf("eFileStruct.cic   :\t%f\n",  eFileStruct.cic   );       */
		   /* printf("eFileStruct.crc   :\t%f\n",  eFileStruct.crc   );       */
		   /* printf("eFileStruct.cis   :\t%f\n",  eFileStruct.cis   );       */
		   /* printf("eFileStruct.crs   :\t%f\n",  eFileStruct.crs   );       */
		   /* printf("eFileStruct.cuc   :\t%f\n",  eFileStruct.cuc   );       */
		   /* printf("eFileStruct.cus   :\t%f\n",  eFileStruct.cus   );       */
		   /* printf("eFileStruct.omega0   :\t%f\n",  eFileStruct.omega0   );       */
		   /* printf("eFileStruct.omega   :\t%f\n",  eFileStruct.omega   );       */
		   /* printf("eFileStruct.i0   :\t%f\n",  eFileStruct.i0   );       */
		   /* printf("eFileStruct.omegadot   :\t%f\n",  eFileStruct.omegadot   );       */
		   /* printf("eFileStruct.idot   :\t%f\n",  eFileStruct.idot   );       */
		   /* printf("eFileStruct.accuracy   :\t%u\n",  eFileStruct.accuracy   );       */
		   /* printf("eFileStruct.health   :\t%u\n",  eFileStruct.health   );       */
		   /* printf("eFileStruct.fit   :\t%u\n",  eFileStruct.fit   );       */

		   numSnv++;
		   fwrite (&eFileStruct , sizeof(char), sizeof(RawAdu5EFileStruct_t), EFile);
		  adu5OutputLength=1;
		  continue;
		}
		else if(adu5OutputLength-1<145) continue;
	      }
	      else if(adu5Output[7]=='T' && adu5Output[8]=='T' && adu5Output[9]=='T') {
		if(adu5OutputLength-1==34) {
		  /* printf("Got TTT\n");		 */
		  //Right length
		  
		  adu5OutputLength=1;
		  continue;
		}
		else if(adu5OutputLength-1<34) continue;
	      }

	      
	    }
	    if(adu5OutputLength>30) { //RJN arbitrary number for now
	      //Reset the counter for now
	      //	      printf("Resetting counter\n");
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
  fclose (BFile);
  fclose (EFile);
  fclose (AFile);
  

  
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
  
  printf("\nNumber of pbns received %d\n", numPbn);
  printf("\nNumber of snvs received %d\n", numSnv);
  printf("\nNumber of ERRORS in MBNs %d\n", numErrors);

  close(fdAdu5);

  return 0; 
}
