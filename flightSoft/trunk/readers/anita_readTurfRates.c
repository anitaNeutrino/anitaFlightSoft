// read anita housekeeping turf rates file
// gcc anita_readTurfRates.c -o ~/bin/anita_readTurfRates -lm -lz
// P. Gorham 6/2/2006

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

/* Turf rate blocks currently come in 100 event clumps,
  in a gzipped file, so use zlib functions */

// anitaStructures.h assumes a subdirectory "./includes"
// with a couple of other include files in it.
#include "anitaStructures.h"

  gzFile TRfile;

main(int argc,char *argv[])
{

	TurfRateStruct_t TR;
	int i,j,k,n;

	if(argc<2){
		fprintf(stderr,"usage: anita_readTurfRates [filename]\n");
		exit(0);
		}

	TRfile = gzopen(argv[1],"r");

      for(n=0;n<100;n++){
	  gzread(TRfile, &TR, sizeof(TurfRateStruct_t));

	  fprintf(stdout,"UnixTime: %d.%d\nL1 rates (kHz):\n",\
		 TR.unixTime,TR.unixTimeUs);

/* L1 header */
fprintf(stdout,\
"        s1      s2      s3      s4      s5      s6      s7      s8\n");

/* TURF L1 rate */
	  for(i=0;i<ANTS_PER_SURF;i++){
		fprintf(stdout,"A%d: ",i);
		for(j=0;j<TRIGGER_SURFS;j++){
		fprintf(stdout,"%6d  ",TR.l1Rates[j][i]);
		}
	    fprintf(stdout,"\n");  // for every line
	   }
          fprintf(stdout,"L2upper (Hz):\n");
	  for(i=0;i<PHI_SECTORS;i++){
		fprintf(stdout,"%4d ",(int)(TR.upperL2Rates[i]));
		}
	  fprintf(stdout,"\n");
	  fflush(stdout);
          fprintf(stdout,"L2lower (Hz):\n");
	  for(i=0;i<PHI_SECTORS;i++){
		fprintf(stdout,"%4d ",(int)(TR.lowerL2Rates[i]));
		}
	  fprintf(stdout,"\n");
	  fflush(stdout);
          fprintf(stdout,"L3 (Hz):\n");
	  for(i=0;i<PHI_SECTORS;i++){
		fprintf(stdout,"%4d ",(int)(TR.l3Rates[i]));
		}
	  fprintf(stdout,\
          "\n-------------------------------------------------------------------------------\n"); 
 	  fflush(stdout);


     }

	gzclose(TRfile);	
}
