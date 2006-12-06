// house keeping dumper
// rjn@mps.ohio-state.edu

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>



#include "includes/anitaStructures.h"

int main(int argc,char *argv[])
{
    
    HkDataStruct_t theHk;
    int retVal=0;
    char filename[FILENAME_MAX];
    gzFile hkFile;

    if(argc<2){
	fprintf(stderr,"usage: %s <file>\n",argv[0]);
	exit(0);
    }
    
    
    sprintf(filename,"%s",argv[1]);
    hkFile=gzopen(filename,"r");
    if(hkFile==Z_NULL) {
	printf("Couldn't open file: %s\n",filename);
    }
    
    while((retVal=gzread(hkFile,&theHk,sizeof(HkDataStruct_t)))>0) {
	fprintf(stderr,"Temp %d\tTemp %d\n",theHk.sbs.temp[0],theHk.sbs.temp[1]);
    }
//    printf("Error %s\n",gzerror(
//    printf("Ret Val %d\n",retVal);
    
//    fprintf(stderr,"Temp %d\tTemp %d\n",theHk.sbs.temp[0],theHk.sbs.temp[1]);
    //fprintf(stderr,"no more files; change directories...\n");
    exit(0);
}
