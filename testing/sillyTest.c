#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>


#include <sys/types.h>

#include <time.h>


#include "anitaStructures.h"


#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"


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

//    TurfioStruct_t turfio;
//    AnitaEventHeader_t header;

 
    printf("Size of GenericHeader_t: %d\n",sizeof(GenericHeader_t)); 
    printf("Size of TurfioStruct_t: %d\n",sizeof(TurfioStruct_t));  
    printf("Size of SlacTurfioStruct_t: %d\n",sizeof(SlacTurfioStruct_t)); 
    printf("Size of SurfChannelFull_t: %d\n",sizeof(SurfChannelFull_t));
    printf("Size of RawSurfChannelHeader_t: %d\n",
	   sizeof(RawSurfChannelHeader_t));
    printf("Size of EncodedSurfChannelHeader_t: %d\n",
	   sizeof(EncodedSurfChannelHeader_t));
    printf("Size of EncodedSurfPacketHeader_t: %d\n",
	   sizeof(EncodedSurfPacketHeader_t));
    printf("Size of EncodedPedSubbedSurfPacketHeader_t: %d\n",
	   sizeof(EncodedPedSubbedSurfPacketHeader_t));
    printf("Size of EncodedWaveformPacket_t: %d\n",
	   sizeof(EncodedWaveformPacket_t));
    printf("Size of RawWaveformPacket_t: %d\n",sizeof(RawWaveformPacket_t));
    printf("Size of RawSurfPacket_t: %d\n",sizeof(RawSurfPacket_t));
    printf("Size of AnitaEventHeader_t: %d\n",sizeof(AnitaEventHeader_t));
    printf("Size of TurfRateStruct_t: %d\n",sizeof(TurfRateStruct_t));
    printf("Size of FullSurfHkStruct_t: %d\n",sizeof(FullSurfHkStruct_t));
    printf("Size of AnitaEventBody_t: %d\n",sizeof(AnitaEventBody_t));
    printf("Size of PedSubbedEventBody_t: %d\n",sizeof(PedSubbedEventBody_t));
    printf("Size of SurfPacket_t: %d\n",sizeof(RawSurfPacket_t));
    printf("Size of GpsSatInfo_t: %d\n",sizeof(GpsSatInfo_t));    
    printf("Size of GpsG12PosStruct_t: %d\n",sizeof(GpsG12PosStruct_t));
    printf("Size of GpsAdu5PatStruct_t: %d\n",sizeof(GpsAdu5PatStruct_t));
    printf("Size of GpsAdu5VtgStruct_t: %d\n",sizeof(GpsAdu5VtgStruct_t));
    printf("Size of GpsG12SatStruct_t: %d\n",sizeof(GpsG12SatStruct_t));
    printf("Size of GpsAdu5SatStruct_t: %d\n",sizeof(GpsAdu5SatStruct_t));   
    printf("Size of AnalogueDataStruct_t: %d\n",sizeof(AnalogueDataStruct_t));
    printf("Size of FullAnalogueStruct_t: %d\n",sizeof(FullAnalogueStruct_t));
    printf("Size of SBSTemperatureDataStruct_t: %d\n",sizeof(SBSTemperatureDataStruct_t));
    printf("Size of MagnetometerDataStruct_t: %d\n",sizeof(MagnetometerDataStruct_t));
    printf("Size of HkDataStruct_t: %d\n",sizeof(HkDataStruct_t));
    printf("Size of CommandEcho_t: %d\n",sizeof(CommandEcho_t));
    printf("Size of DiskSpaceStruct_t: %d\n",sizeof(DiskSpaceStruct_t));
    printf("Size of QueueStruct_t: %d\n",sizeof(QueueStruct_t));
    printf("Size of MonitorStruct_t: %d\n",sizeof(MonitorStruct_t));
    printf("Size of LabChipChannelPedStruct_t: %d\n",sizeof(LabChipChannelPedStruct_t));
    printf("Size of FullLabChipPedStruct_t: %d\n",sizeof(FullLabChipPedStruct_t));
    printf("Size of FullPedStruct_t: %d\n",sizeof(FullPedStruct_t));


/*     printf("\n\n");   */
/*     printf("\n&turfio:\t\t%u\n&turfio.otherBits:\t%u\n&turfio.trigType:\t%u\n&turfio.trigNum:\t%u\n&turfio.trigTime:\t%u\n&turfio.ppsNum:\t%u\n&turfio.c3poNum:\t%u\n", */
/* 	   (int)&turfio, */
/* 	   (int)&(turfio.otherBits)-(int)&turfio, */
/* 	   (int)&(turfio.trigType)-(int)&turfio, */
/* 	   (int)&(turfio.trigNum)-(int)&turfio, */
/* 	   (int)&(turfio.trigTime)-(int)&turfio, */
/* 	   (int)&(turfio.ppsNum)-(int)&turfio, */
/* 	   (int)&(turfio.c3poNum)-(int)&turfio); */
/*     printf("\n\n"); */
/*     printf("\n&header:\t\t%u\n&header.gHdr:\t%u\n&header.unixTime:\t%u\n&header.numChannels:\t%u\n&header.numSamples:\t%u\n&header.calibStatus:\t%u\n&header.turfio:\t%u\n&header.priority:\t%u\n", */
/* 	   (int)&header, */
/* 	   (int)&(header.gHdr)-(int)&header, */
/* 	   (int)&(header.unixTime)-(int)&header, */
/* 	   (int)&(header.numChannels)-(int)&header, */
/* 	   (int)&(header.numSamples)-(int)&header, */
/* 	   (int)&(header.calibStatus)-(int)&header, */
/* 	   (int)&(header.turfio)-(int)&header, */
/* 	   (int)&(header.priority)-(int)&header); */

}


/* void waitUntilNextSecond() */
/* { */
/*     time_t start; */
/*     start=time(NULL); */
/*     printf("Start time %d\n",start); */

/*     while(start==time(NULL)) usleep(100); */
    
/* } */
