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
    int beamStatus=0,goodFlag=0;
    	
    if(argc<2) {
	fprintf(stderr,"Usage %s <log file>\n",argv[0]);
	exit(0);
    }
    
    strncpy(logFile,argv[1],FILENAME_MAX-1);
    fpLog=fopen(logFile,"w");
    if(!fpLog) {
	printf("Couldn't open %s\n",logFile);
	exit(0);
    }

    printf("Enter User's name:\n");
    fgets(usersName,FILENAME_MAX,stdin);
    printf("Enter Run (nick)name:\n");
    fgets(runName,FILENAME_MAX,stdin);

    goodFlag=0;
    while(!goodFlag) {				
	printf("Is the beam on? (y/n)\n");
	testChar=(char)getc(stdin);
	if(testChar=='y' || testChar=='Y') {
	    beamStatus=1;
	    goodFlag=1;
	}
	else if(testChar=='n' || testChar=='n') {
	    beamStatus=0;
	    goodFlag=1;
	}
    }
	
    fgets(bigBuffer,2048,stdin);
    printf("Enter Run description (upto 2000 characters):\n");
    fgets(bigBuffer,2048,stdin);




    fprintf(fpLog,"User: %s\n",usersName);          
    fprintf(fpLog,"Run: %s\n",runName);   
    if(beamStatus) 
	fprintf(fpLog,"Beam is on\n\n");
    else
	fprintf(fpLog,"Beam is off\n\n");
    fprintf(fpLog,"Description: \n%s",bigBuffer);
    
    fclose(fpLog);
    return 0;

}
