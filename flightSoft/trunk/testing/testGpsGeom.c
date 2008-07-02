#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>


#include <sys/types.h>

#include <time.h>


#include "includes/anitaStructures.h"


#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "mapLib/mapLib.h"


int main (void)
{    
  double anitaLatLonAlt[3]={-80,-180,37000};
  double sourceLatLonAlt[3]={-80,0,0};
  double anitaHPR[3]={90,0,0};

  int phiDir;
  double distance;
  int heading=0;
  

  int longNum;
  for(longNum=-180;longNum<180;longNum+=90) {
    for(heading=0;heading<91;heading+=90) {
      printf("longNum %d -- heading %d\n",longNum,heading);
      anitaHPR[0]=heading;
      anitaLatLonAlt[1]=longNum;
      getSourcePhiAndDistance(anitaLatLonAlt,anitaHPR,sourceLatLonAlt,
			      &phiDir,&distance);
      
      printf("Balloon %f %f %f\nHeading %f\nSource %f %f %f\n\tPhi %d -- Distance %f\n",
	     anitaLatLonAlt[0],anitaLatLonAlt[1],anitaLatLonAlt[2],
	     anitaHPR[0],
	     sourceLatLonAlt[0],sourceLatLonAlt[1],sourceLatLonAlt[2],
	     phiDir+1,distance);
    }
  }



  int phiA,phiB;
  printf("\n\n\n");
  for(phiA=0;phiA<16;phiA++) {
    for(phiB=0;phiB<16;phiB++) {
      printf("%d %d -- %d\n",phiA,phiB,getPhiSeparation(phiA,phiB));
    }
  }

  return 0;
}

