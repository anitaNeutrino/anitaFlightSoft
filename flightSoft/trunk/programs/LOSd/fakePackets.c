#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"


/*Config Thingies*/
char losdPacketDir[FILENAME_MAX];
int numPacketDirs=0;


int main(int argc, char *argv[])
{
    int pk;//,retVal;
    char *tempString;
    char tempDir[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;
    

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("SIPd.config","global") ;
    status += configLoad ("SIPd.config","losd") ;
    eString = configErrorString (status) ;


    if (status == CONFIG_E_OK) {
/* 	printf("Here\n"); */
	numPacketDirs=kvpGetInt("numPacketDirs",9);
	tempString=kvpGetString("losdPacketDir");
	if(tempString) {
	    strncpy(losdPacketDir,tempString,FILENAME_MAX);
	    for(pk=0;pk<numPacketDirs;pk++) {
		sprintf(tempDir,"%s/pk%d/link",losdPacketDir,pk);
		makeDirectories(tempDir);
	    }
		
	}
	else {
	    syslog(LOG_ERR,"Couldn't get losdPacketDir");
	    fprintf(stderr,"Couldn't get losdPacketDir\n");
	}
	//printf("%d\t%s\n",kvpStatus,kvpErrorString(kvpStatus));
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }
    AnitaEventHeader_t theHeader;
    theHeader.gHdr.code=PACKET_HD;
    int evNum=1;
    struct timeval timeStruct;
    char fileName[FILENAME_MAX];
    char dirName[FILENAME_MAX];
    while(1) {
	gettimeofday(&timeStruct,NULL);
	theHeader.unixTime=timeStruct.tv_sec;
	theHeader.unixTimeUs=timeStruct.tv_usec;
	theHeader.eventNumber=evNum;
	evNum++;      
	sprintf(fileName,"%s/pk%d/hd_%d.dat",losdPacketDir,3,theHeader.eventNumber);
	FILE *pFile;
	int n;
	if ((pFile=fopen(fileName, "wb")) == NULL) { 
	    printf("Failed to open file, %s\n", fileName) ;
	    return 1;
	}	
	if ((n=fwrite(&theHeader, sizeof(AnitaEventHeader_t),1,pFile))!=1)
	    printf("Failed to write all header data. wrote only %d.\n", n) ;
	fclose(pFile);
	
	sprintf(dirName,"%s/pk%d/link/",losdPacketDir,3);
	makeLink(fileName,dirName);
	usleep(10000);
    }
    return 0;
}


