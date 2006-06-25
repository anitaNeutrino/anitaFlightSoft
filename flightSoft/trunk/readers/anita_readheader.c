// read anita header file
// gcc anita_readheader.c -o ~/bin/anita_readheader -lm -lz
// P. Gorham 6/2/2006

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

/* headers currently come in 100 event clumps,
  in a gzipped file, so use zlib functions */

// anitaStructures.h assumes a subdirectory "./includes"
// with a couple of other include files in it.
#include "anitaStructures.h"

  gzFile hdfile;

#define AH  hin

main(int argc,char *argv[])
{

	AnitaEventHeader_t hin;
	int i, jhd, jstart;
	char hdname[80];

	if(argc<2){
		fprintf(stderr,"usage: anita_readheader [starting file number ]\n");
		fprintf(stderr,"\tassumes files contain 100 events, gzipped format; reads whole directory.\n");
		exit(0);
		}

	jhd = 0;
	jstart=atoi(argv[1]);

	sprintf(hdname,"hd_%d.dat.gz",jstart+jhd);
//	fprintf(stderr,"file = %s\n", hdname);
//	fflush(stderr);

   while ( (hdfile = gzopen(hdname,"r")) != Z_NULL) {  ;

//	fprintf(stderr,"file = %s\n", hdname);

      for(i=0;i<100;i++){
	gzread(hdfile, &hin, sizeof(AnitaEventHeader_t));

/* partial header title line: */
	if ( (int)(1000.*(i%22)) == 0) fprintf(stdout,\
"#ev UnTim trfUpWrd trgType l3Tp1Cnt trigN trigT pps c3po upL1 loL1 upL2 loL2 L3\n");

/* partial header */
	fprintf(stdout,"%6d %12.6f %x %x %d %d %9d %d %6.3f %4X %4X %4X %4X %4X\n",\
	AH.eventNumber, (AH.unixTime&0xFFFF)+(AH.unixTimeUs)/1.e6,\
	AH.turfUpperWord,AH.turfio.trigType&0xf,AH.turfio.l3Type1Count,\
	AH.turfio.trigNum,AH.turfio.trigTime,AH.turfio.ppsNum,AH.turfio.c3poNum/1.e6,\
	AH.turfio.upperL1TrigPattern,AH.turfio.lowerL1TrigPattern,AH.turfio.upperL2TrigPattern,\
	AH.turfio.lowerL2TrigPattern,\
	AH.turfio.l3TrigPattern);
	}
	gzclose(hdfile);
	jhd += 100;
	sprintf(hdname,"hd_%d.dat.gz",jstart+jhd);
    }

	//fprintf(stderr,"no more files; change directories...\n");
	exit(0);
}
