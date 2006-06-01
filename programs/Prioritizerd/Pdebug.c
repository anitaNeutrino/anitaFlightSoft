/* Debugging driver for the prioritizer routines */
#include <stdlib.h>
#include <stdio.h>
#include "anitaFlight.h"
#include "anitaStructures.h"
#include "utilLib/utilLib.h"

int main(int argc, char *argv[]){
     int theEvent, retVal;
     double UTCtime;
     char headerName[FILENAME_MAX],bodyName[FILENAME_MAX];
     char rawDumpName[FILENAME_MAX];
     FILE *rawDumpFile;
     int i,j;
     AnitaEventHeader_t theHeader;
     AnitaEventBody_t theBody;

     if (argc!=2){
	  printf("Usage: Pdebug <header filename>");
	  exit(-1);
     }
     sscanf(argv[1],"hd_%d.dat",&theEvent);
     sprintf(headerName,"hd_%d.dat",theEvent);
     sprintf(bodyName,"ev_%d.dat",theEvent);
     sprintf(rawDumpName,"rd_%d.txt",theEvent);
     retVal=fillBody(&theBody,bodyName);
     retVal=fillHeader(&theHeader,headerName);
// the raw dump file is formatted for use with gnuplot
// indexing is separated by newlines
// zeroth index contains the header dump
// indices 1..81 are surf records
     rawDumpFile=fopen(rawDumpName,"w");
     fprintf(rawDumpFile,"#ANITA Event Raw Dumpfile for event %d\n",theEvent);
     UTCtime=theHeader.unixTime+1e-6*theHeader.unixTimeUs;
     fprintf(rawDumpFile,"%16.6f #UTC Unix Time (s)\n",UTCtime);
     fprintf(rawDumpFile,"%lu #GPS SubTime (ns)\n",theHeader.gpsSubTime);
     fprintf(rawDumpFile,"%lu #Event Number\n",theHeader.eventNumber);
     fprintf(rawDumpFile,"%hhu #Priority\n",theHeader.priority);
     fprintf(rawDumpFile,"%hu #Calib Status\n",theHeader.calibStatus);
     fprintf(rawDumpFile,"\n\n");// end index 0
     for (i=0; i<NUM_DIGITZED_CHANNELS; i++){
	  //header info gets put in a comment invisible to gnuplot
	  //but accessible to awk.
	  fprintf(rawDumpFile,"#ChanID %hhu\n",
		  ((theBody.channel[i]).header).chanId);
	  fprintf(rawDumpFile,"#ChipIDFlag %#hhx\n",
		  ((theBody.channel[i]).header).chipIdFlag);
	  fprintf(rawDumpFile,"#FirstHitbus %hhu LastHitbus %hhu\n", 
		  ((theBody.channel[i]).header).firstHitbus,
		  ((theBody.channel[i]).header).lastHitbus);
	  for (j=0;j<MAX_NUMBER_SAMPLES; j++){
	       fprintf(rawDumpFile,"%i %hu\n",i,(theBody.channel[i]).data[j]);
	  }
	  fprintf(rawDumpFile,"\n\n"); //end index i+1
     }
     fclose(rawDumpFile);
     return 0;
}
