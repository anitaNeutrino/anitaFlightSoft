#include "Prioritizerd.h"


/* Directories and gubbins */
extern char eventdEventDir[FILENAME_MAX];
extern char eventdEventLinkDir[FILENAME_MAX];
extern char headerTelemDir[FILENAME_MAX];
extern char headerTelemLinkDir[FILENAME_MAX];
extern char eventTelemDir[NUM_PRIORITIES][FILENAME_MAX]; 
extern char eventTelemLinkDir[NUM_PRIORITIES][FILENAME_MAX]; 

extern char eventArchiveDir[FILENAME_MAX];
extern char eventUSBArchiveDir[FILENAME_MAX];
extern char prioritizerdPidFile[FILENAME_MAX];

void writePacketsAndHeader(AnitaEventBody_t *bodyPtr, AnitaEventHeader_t *hdPtr) 
{
/*     int chan; */
/*     char packetName[FILENAME_MAX]; */
/*     WaveformPacket_t wavePacket; */
/*     wavePacket.gHdr.code=PACKET_WV; */
/*     for(chan=0;chan<hdPtr->numChannels;chan++) { */
/* 	sprintf(packetName,"%s/wvpk_%d_%d.dat",prioritizerdTelemWvDir,hdPtr->eventNumber,chan); */
/* //	printf("Packet: %s\n",packetName); */
/* 	wavePacket.eventNumber=hdPtr->eventNumber; */
/* 	wavePacket.packetNumber=chan; */
/* 	memcpy(&(wavePacket.waveform),&(bodyPtr->channel[chan]),sizeof(SurfChannelFull_t)); */
/* 	writeWaveformPacket(&wavePacket,packetName); */
/* 	makeLink(packetName,prioritizerdTelemWvLinkDir); */
/*     } */
    int surf,retVal=0;
    char packetName[FILENAME_MAX];
    char headerName[FILENAME_MAX];
    SurfPacket_t surfPacket;
    surfPacket.gHdr.code=PACKET_SURF;
    for(surf=0;surf<(hdPtr->numChannels)/CHANNELS_PER_SURF;surf++) {
	sprintf(packetName,"%s/surfpk_%d_%d.dat",eventTelemDir[(int)hdPtr->priority],hdPtr->eventNumber,surf);
	surfPacket.eventNumber=hdPtr->eventNumber;
//	surfPacket.packetNumber=surf;
	memcpy(&(surfPacket.waveform[0]),&(bodyPtr->channel[CHANNELS_PER_SURF*surf]),sizeof(SurfChannelFull_t)*CHANNELS_PER_SURF);
	writeSurfPacket(&surfPacket,packetName);
//	printf("Wrote %s\n",packetName);
//	makeLink(packetName,prioritizerdTelemWvLinkDir);
    }
    sprintf(headerName,"%s/hd_%d.dat",eventTelemDir[(int)hdPtr->priority],hdPtr->eventNumber);
    retVal=writeHeader(hdPtr,headerName);
    retVal=makeLink(headerName,eventTelemLinkDir[(int)hdPtr->priority]);
}
