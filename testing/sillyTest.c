#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#include <sys/types.h>

#include <time.h>


#include "includes/anitaStructures.h"


#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"

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

    printf("Size of int %ld  (long %ld)\n",sizeof(int),sizeof(long)); 
    printf("Size of GenericHeader_t: %ld\n",sizeof(GenericHeader_t));
    printf("Size of SlowRateHkStruct_t: %ld\n",sizeof(SlowRateHkStruct_t)); 
    printf("Size of SlowRateRFStruct_t: %ld\n",sizeof(SlowRateRFStruct_t));  
    printf("Size of SlowRateFull_t: %ld\n",sizeof(SlowRateFull_t)); 
    printf("Size of TurfioStruct_t: %ld\n",sizeof(TurfioStruct_t));  
    printf("Size of SlacTurfioStruct_t: %ld\n",sizeof(SlacTurfioStruct_t)); 
    printf("Size of SurfChannelFull_t: %ld\n",sizeof(SurfChannelFull_t));
    printf("Size of RawSurfChannelHeader_t: %ld\n",
	   sizeof(RawSurfChannelHeader_t));
    printf("Size of EncodedSurfChannelHeader_t: %ld\n",
	   sizeof(EncodedSurfChannelHeader_t));
    printf("Size of EncodedSurfPacketHeader_t: %ld\n",
	   sizeof(EncodedSurfPacketHeader_t));
    printf("Size of EncodedPedSubbedSurfPacketHeader_t: %ld\n",
	   sizeof(EncodedPedSubbedSurfPacketHeader_t));
    printf("Size of EncodedPedSubbedChannelPacketHeader_t: %ld\n",
	   sizeof(EncodedPedSubbedChannelPacketHeader_t));
    printf("Size of RawWaveformPacket_t: %ld\n",sizeof(RawWaveformPacket_t));
    printf("Size of RawSurfPacket_t: %ld\n",sizeof(RawSurfPacket_t));
    printf("Size of AnitaEventHeader_t: %ld\n",sizeof(AnitaEventHeader_t));
    printf("Size of TurfRateStruct_t: %ld\n",sizeof(TurfRateStruct_t));
    printf("Size of SummedTurfRateStruct_t: %ld\n",sizeof(SummedTurfRateStruct_t));
    printf("Size of FullSurfHkStruct_t: %ld\n",sizeof(FullSurfHkStruct_t));
    printf("Size of AveragedSurfHkStruct_t: %ld\n",sizeof(AveragedSurfHkStruct_t));
    printf("Size of AnitaEventBody_t: %ld\n",sizeof(AnitaEventBody_t));
    printf("Size of PedSubbedEventBody_t: %ld\n",sizeof(PedSubbedEventBody_t));
    printf("Size of SurfPacket_t: %ld\n",sizeof(RawSurfPacket_t));
    printf("Size of GpsSatInfo_t: %ld\n",sizeof(GpsSatInfo_t));    
    printf("Size of GpsG12PosStruct_t: %ld\n",sizeof(GpsG12PosStruct_t));
    printf("Size of GpsAdu5PatStruct_t: %ld\n",sizeof(GpsAdu5PatStruct_t));
    printf("Size of GpsAdu5VtgStruct_t: %ld\n",sizeof(GpsAdu5VtgStruct_t));
    printf("Size of GpsG12SatStruct_t: %ld\n",sizeof(GpsG12SatStruct_t));
    printf("Size of GpsAdu5SatStruct_t: %ld\n",sizeof(GpsAdu5SatStruct_t));   
    printf("Size of AnalogueDataStruct_t: %ld\n",sizeof(AnalogueDataStruct_t));
    printf("Size of FullAnalogueStruct_t: %ld\n",sizeof(FullAnalogueStruct_t));
    printf("Size of SBSTemperatureDataStruct_t: %ld\n",sizeof(SBSTemperatureDataStruct_t));
    printf("Size of MagnetometerDataStruct_t: %ld\n",sizeof(MagnetometerDataStruct_t));
    printf("Size of HkDataStruct_t: %ld\n",sizeof(HkDataStruct_t));
    printf("Size of CommandEcho_t: %ld\n",sizeof(CommandEcho_t));
    printf("Size of DiskSpaceStruct_t: %ld\n",sizeof(DiskSpaceStruct_t));
    printf("Size of QueueStruct_t: %ld\n",sizeof(QueueStruct_t));
    printf("Size of ProcessInfo_t: %ld\n",sizeof(ProcessInfo_t));
    printf("Size of MonitorStruct_t: %ld\n",sizeof(MonitorStruct_t));
    printf("Size of OtherMonitorStruct_t: %ld\n",sizeof(OtherMonitorStruct_t));
    printf("Size of RunStart_t: %ld\n",sizeof(RunStart_t));
    printf("Size of LabChipChannelPedStruct_t: %ld\n",sizeof(LabChipChannelPedStruct_t));
    printf("Size of FullLabChipPedStruct_t: %ld\n",sizeof(FullLabChipPedStruct_t));
    printf("Size of FullPedStruct_t: %ld\n",sizeof(FullPedStruct_t));
    printf("Size of AcqdStartStruct_t: %ld\n",sizeof(AcqdStartStruct_t));
    printf("Size of LogWatchdStart_t: %ld\n",sizeof(LogWatchdStart_t));
    printf("Size of ZippedPacket_t: %ld\n",sizeof(ZippedPacket_t));
    printf("Size of ZippedFile_t: %ld\n",sizeof(ZippedFile_t));

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

    unsigned int numbers[10]={0,1,2,3,4,5,6,7,8,9};
    printf("simpleIntCrc: %u\n",simpleIntCrc(numbers,10));

    AnitaEventHeader_t theHeader;
    memset(&theHeader,0,sizeof(AnitaEventHeader_t));
    theHeader.eventNumber=10;
    fillGenericHeader(&theHeader,PACKET_HD,sizeof(AnitaEventHeader_t));
    printf("theHeader.gHdr.checksum:\t%u\n",theHeader.gHdr.checksum);
    printf("checkPacket: %d\n",checkPacket(&theHeader));

    return 0;
}

