#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char** argv) {
    FILE *fpLog;
    char logFile[FILENAME_MAX];
    char usersName[FILENAME_MAX];
    char runName[FILENAME_MAX];
    char testChar;
    char bigBuffer[2048];
    int beamStatus;
    	
    if(argc<2) {
	fprintf(stderr,"Usage %s <log file>\n",argv[0]);
	exit(0);
    }
    
    strncpy(logFile,argv[1],FILENAME_MAX-1);
    fpLog=fopen(logFile,"r");

    printf("Enter Users Name:\n");
    gets
    


    return 0;

}
