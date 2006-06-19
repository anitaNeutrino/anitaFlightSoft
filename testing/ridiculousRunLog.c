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
    char whichBottomAntennaFacingBeam[FILENAME_MAX];
    char gridLocations[FILENAME_MAX];
    char beamCurrent[FILENAME_MAX];
    char bunchesPerSecond[FILENAME_MAX];
    	
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
    if(beamStatus) {
	printf("Enter beam current:\n");
	fgets(beamCurrent,FILENAME_MAX,stdin);
	printf("Enter bunches per seoond:\n");
	fgets(bunchesPerSecond,FILENAME_MAX,stdin);
    }
    
    printf("Which Bottom ring antenna is facing the beam?\n");
    fgets(whichBottomAntennaFacingBeam,FILENAME_MAX,stdin);
    printf("Enter grid location\n");
    fgets(gridLocations,FILENAME_MAX,stdin);
    
	

    printf("Enter Run description (upto 2000 characters):\n");
    fgets(bigBuffer,2048,stdin);
    



    fprintf(fpLog,"User: %s\n",usersName);          
    fprintf(fpLog,"Run: %s\n",runName);   
    if(beamStatus) {
	fprintf(fpLog,"Beam is on\n\n");
	fprintf(fpLog,"Beam current: %s\n",beamCurrent);   
	fprintf(fpLog,"Bunches per second: %s\n",bunchesPerSecond);   
    }
    else
	fprintf(fpLog,"Beam is off\n\n");
    fprintf(fpLog,"Beam is facing bottom antenna: %s\n",whichBottomAntennaFacingBeam);
    fprintf(fpLog,"In grid location: %s\n\n",gridLocations);
    fprintf(fpLog,"Description: \n%s",bigBuffer);
    
    fclose(fpLog);
    return 0;

}
