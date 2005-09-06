#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>


#include <sys/types.h>

#include <time.h>


#include "anitaStructures.h"


#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"

void waitUntilNextSecond();
int main (void)
{
   /*  int status=0; */
/*     int thePort=0; */
/*     char* eString ; */
/*     char *fileName="anitaSoft.config"; */

/*     kvpReset () ; */
/*     status = configLoad (fileName,"global") ; */
/*     eString = configErrorString (status) ; */
/*     printf ("status %d (%s) from loading %s\n", */
/* 	    status, eString, fileName) ; */
/*     if (status == SOCKET_E_OK) { */
/* 	thePort= kvpGetInt ("portEventdPrioritzerd",0) ; */
/*     } */
/*     printf("\nThe port is: %d\n\n",thePort); */

     printf("Size of GenericHeader_t: %d\n",sizeof(GenericHeader_t)); 
     printf("Size of TurfioStruct_t: %d\n",sizeof(TurfioStruct_t)); 
     printf("Size of SurfChannelFull_t: %d\n",sizeof(SurfChannelFull_t));
     printf("Size of AnitaEventHeader_t: %d\n",sizeof(AnitaEventHeader_t));
     printf("Size of AnitaEventBody_t: %d\n",sizeof(AnitaEventBody_t));
     printf("Size of AnitaEventBody_t: %d\n",sizeof(AnitaEventBody_t));
     printf("Size of SurfPacket_t: %d\n",sizeof(SurfPacket_t));
     printf("Size of GpsPatStruct_t: %d\n",sizeof(GpsPatStruct_t));
     printf("Size of GpsSatStruct_t: %d\n",sizeof(GpsSatStruct_t));
     printf("Size of AnalogueDataStruct_t: %d\n",sizeof(AnalogueDataStruct_t));
     printf("Size of FullAnalogueStruct_t: %d\n",sizeof(FullAnalogueStruct_t));
     printf("Size of SBSTemperatureDataStruct_t: %d\n",sizeof(SBSTemperatureDataStruct_t));
     printf("Size of MagnetometerDataStruct_t: %d\n",sizeof(MagnetometerDataStruct_t));
     printf("Size of HkDataStruct_t: %d\n",sizeof(HkDataStruct_t));
     printf("Size of CommandEcho_t: %d\n",sizeof(CommandEcho_t));
     
     

}


/* void waitUntilNextSecond() */
/* { */
/*     time_t start; */
/*     start=time(NULL); */
/*     printf("Start time %d\n",start); */

/*     while(start==time(NULL)) usleep(100); */
    
/* } */
