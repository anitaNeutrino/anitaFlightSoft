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


  struct RawAdu5EFileStruct eFileStruct;
  FILE *eFilePtr;
  
  eFilePtr=fopen(argv[1],"rb");
  if (!eFilePtr) {
    printf("Unable to open file!");
    return 1;
  }
  

  double 

  int countStuff=0;
  while(1) {
    int numObjs=fread(&eFileStruct,sizeof(RawAdu5EFileStruct_t),1,eFilePtr);
    printf("countStuff=%d numObjs=%d sizeof(RawAdu5BFileRawNav_t)=%lu\n",countStuff,numObjs,sizeof(RawAdu5BFileRawNav_t));

    if(numObjs<1) break;
    countStuff++;

    printf("eFileStruct.prnnum        :\t%d\n",  eFileStruct.prnnum        );      
    printf("eFileStruct.weekNumber   :\t%u\n",  eFileStruct.weekNumber   );      
    printf("eFileStruct.secondsInWeek :\t%d\n",  eFileStruct.secondsInWeek );      
    printf("eFileStruct.groupDelay   :\t%f\n",  eFileStruct.groupDelay   );      
    printf("eFileStruct.aodc   :\t%d\n",  eFileStruct.aodc   );      
    printf("eFileStruct.toc   :\t%d\n",  eFileStruct.toc   );      
    printf("eFileStruct.af2   :\t%f\n",  eFileStruct.af2   );      
    printf("eFileStruct.af1   :\t%f\n",  eFileStruct.af1   );      
    printf("eFileStruct.af0   :\t%f\n",  eFileStruct.af0   );      
    printf("eFileStruct.aode   :\t%d\n",  eFileStruct.aode   );      
    printf("eFileStruct.deltaN   :\t%f\n",  eFileStruct.deltaN   );      
    printf("eFileStruct.m0   :\t%f\n",  eFileStruct.m0   );      
    printf("eFileStruct.eccentricity  :\t%f\n",  eFileStruct.eccentricity  );      
    printf("eFileStruct.roota   :\t%f\n",  eFileStruct.roota   );      
    printf("eFileStruct.toe   :\t%d\n",  eFileStruct.toe   );      
    printf("eFileStruct.cic   :\t%f\n",  eFileStruct.cic   );      
    printf("eFileStruct.crc   :\t%f\n",  eFileStruct.crc   );      
    printf("eFileStruct.cis   :\t%f\n",  eFileStruct.cis   );      
    printf("eFileStruct.crs   :\t%f\n",  eFileStruct.crs   );      
    printf("eFileStruct.cuc   :\t%f\n",  eFileStruct.cuc   );      
    printf("eFileStruct.cus   :\t%f\n",  eFileStruct.cus   );      
    printf("eFileStruct.omega0   :\t%f\n",  eFileStruct.omega0   );      
    printf("eFileStruct.omega   :\t%f\n",  eFileStruct.omega   );      
    printf("eFileStruct.i0   :\t%f\n",  eFileStruct.i0   );      
    printf("eFileStruct.omegadot   :\t%f\n",  eFileStruct.omegadot   );      
    printf("eFileStruct.idot   :\t%f\n",  eFileStruct.idot   );      
    printf("eFileStruct.accuracy   :\t%u\n",  eFileStruct.accuracy   );      
    printf("eFileStruct.health   :\t%u\n",  eFileStruct.health   );      
    printf("eFileStruct.fit   :\t%u\n",  eFileStruct.fit   );      
    
  }
  fclose(eFilePtr);
  return 0;
  
}
