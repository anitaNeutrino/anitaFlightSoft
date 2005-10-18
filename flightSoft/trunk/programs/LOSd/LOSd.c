/* LOSd - Program that talks to sip.
 *
 * Ryan Nichol, August '05
 * Started off as a modified version of Marty's driver program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include <fcntl.h>


/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "losLib/los.h"

int initDevice();
void tryToSendData();
int bufferAndWrite(unsigned char *buf, short nbytes,int doWrite);
int fillBufferWithPackets();
int doWrite();

/*Config Thingies*/
char losdPacketDir[FILENAME_MAX];
char losdPidFile[FILENAME_MAX];

char eventLinkDir[FILENAME_MAX];
char packetLinkDir[8][FILENAME_MAX];
char eventDir[FILENAME_MAX];
char packetDir[8][FILENAME_MAX];
int numPacketDirs=0;
int maxPacketsPerDir=50;
int maxEventsInARow=100;
int losBus;
int losSlot;
int verbosity;


/*Global Variables*/
#define BSIZE 10000 //Silly hack until I packet up Events
unsigned char theBuffer[BSIZE];
unsigned char bufferToWrite[LOS_MAX_BYTES];
short numBytesInBuffer=0;


int main(int argc, char *argv[])
{
    int pk,retVal;
    char *tempString;
    char tempDir[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;
    
    char *progName=basename(argv[0]);
   
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("SIPd.config","global") ;
    status += configLoad ("SIPd.config","losd") ;
    eString = configErrorString (status) ;


    if (status == CONFIG_E_OK) {
/* 	printf("Here\n"); */
	numPacketDirs=kvpGetInt("numPacketDirs",9);
	losBus=kvpGetInt("losBus",1);
	losSlot=kvpGetInt("losSlot",11);
	verbosity=kvpGetInt("verbosity",0);
	maxPacketsPerDir=kvpGetInt("maxPacketsPerDir",50);
	tempString=kvpGetString("losdPidFile");
	if(tempString) {
	    strncpy(losdPidFile,tempString,FILENAME_MAX);
	    writePidFile(losdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get losdPidFile");
	    fprintf(stderr,"Couldn't get losdPidFile\n");
	}
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

    retVal=initDevice();
    if(retVal!=0) {
	return 1;
    }
    
    sprintf(eventLinkDir,"%s/pk0/link",losdPacketDir);
    sprintf(eventDir,"%s/pk0",losdPacketDir);
    for(pk=1;pk<numPacketDirs;pk++) {
	sprintf(packetLinkDir[pk],"%s/pk%d/link",losdPacketDir,pk);
	sprintf(packetDir[pk],"%s/pk%d",losdPacketDir,pk);
    }


    do {
	if(verbosity) printf("Initializing LOSd\n");
	currentState=PROG_STATE_RUN;
        while(currentState==PROG_STATE_RUN) {
	    tryToSendData();
	    usleep(1);
	}
    } while(currentState==PROG_STATE_INIT);

//    fprintf(stderr, "Bye bye\n");
    return 0;
}

int fillBufferWithPackets() 
{

    int pk;
    int retVal,count;
    int numLinks=0;
    int filledBuffer=0;
    int numBytes=0;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int fd;
    struct dirent **linkList;
    for(pk=1;pk<numPacketDirs;pk++) {
	numLinks=getListofLinks(packetLinkDir[pk],&linkList);
	if(numLinks<=0) continue;
	if(verbosity) printf("Got %d links in %s\n",numLinks,packetLinkDir[pk]);
//	if(numLinks) break;
	
//	if(numLinks==0) return 0;
	//Need to put something here so that it doesn't dick around 
	//forever in pk4, when there is data waiting in pk1, etc.
//    printf("%d links in packet %d\n",numLinks,pk);
	/* Nasty RJN Hack */
	if(numLinks>500) {
	    for(count=0;count<100;count++) {	    
		sprintf(currentFilename,"%s/%s",
			packetDir[pk],linkList[count]->d_name);
		sprintf(currentLinkname,"%s/%s",
			packetLinkDir[pk],linkList[count]->d_name);
		
		removeFile(currentLinkname);
		unlink(currentFilename);
	    }
	}
	for(count=0;count<numLinks;count++)
	    free(linkList[count]);
	free(linkList);
	numLinks=getListofLinks(packetLinkDir[pk],&linkList);
	if(numLinks<=0) continue;
	for(count=numLinks-1;count>=0;count--) {
	
	    sprintf(currentFilename,"%s/%s",
		    packetDir[pk],linkList[count]->d_name);
	    sprintf(currentLinkname,"%s/%s",
		    packetLinkDir[pk],linkList[count]->d_name);
	    fd = open(currentFilename,O_RDONLY);
//	fp = fopen(currentFilename,"rb");
	    if(fd == 0) {
//	if(fp==NULL) {
		syslog(LOG_ERR,"Error opening file, will delete: %s",currentFilename);
		fprintf(stderr,"Error opening file, will delete: %s\n",currentFilename);
		
		removeFile(currentFilename);
		removeFile(currentLinkname);
		//Mayhaps we should delete
		continue;
	    }
	    
	
	    
	    if(verbosity>1) { 
		printf("Opened file: %s\n",currentFilename);
	    }
	
	    numBytes=read(fd,theBuffer,BSIZE);
	    if(numBytes<=0) {
//		removeFile(currentLinkname);
		unlink(currentLinkname);
		unlink(currentFilename);
		continue;
	    }
	    close (fd);
	    retVal=0;
//	    retVal = los_write((unsigned char *)theBuffer, numBytes);
	    retVal = bufferAndWrite((unsigned char *)theBuffer, numBytes,0);
	    if (retVal < 0) {
		syslog(LOG_ERR,"Couldn't send file: %s",currentFilename);
		fprintf(stderr, "Couldn't telemeterize: %s\n\t%s\n",currentFilename,los_strerror());
		break;
	    }
	    else {
		removeFile(currentLinkname);
		removeFile(currentFilename);
//	    unlink(currentLinkname);
//	    unlink(currentFilename);
		if(retVal==0) {
		    filledBuffer=1;
		    break;
		}
	    }    	    	    
	    if((numLinks-count)>maxPacketsPerDir) break;
	    
	}
	
	for(count=0;count<numLinks;count++)
	    free(linkList[count]);
	free(linkList);
	if(filledBuffer) return 1;
    }
    return 0;
}    
    

void tryToSendData()
{
    int retVal,eventCount;
    int numEventLinks=0;
    int numBytes=0;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];

//    int numItems=0;
//    FILE *fp;
    int fd;
    struct dirent **eventLinkList;

    
    numEventLinks=getListofLinks(eventLinkDir,&eventLinkList);
    if(verbosity) printf("Got %d links in %s\n",numEventLinks,eventLinkDir);
    if(!numEventLinks) {
	fillBufferWithPackets();
    }	
    else {
	/* Nasty RJN Hack */
	if(numEventLinks>500) {
	    for(eventCount=0;eventCount<100;eventCount++) {
		sprintf(currentFilename,"%s/%s",
			    eventDir,eventLinkList[eventCount]->d_name);
		sprintf(currentLinkname,"%s/%s",
			eventLinkDir,eventLinkList[eventCount]->d_name);
		unlink(currentLinkname);
		unlink(currentFilename);
	    }
	}
	
	for(eventCount=0;eventCount<numEventLinks;eventCount++)
	    free(eventLinkList[eventCount]);
	free(eventLinkList);
	numEventLinks=getListofLinks(eventLinkDir,&eventLinkList);
	if(numEventLinks<=0) {
	    fillBufferWithPackets();
	    if(numBytesInBuffer) doWrite();
	    return;
	}
	for(eventCount=numEventLinks-1;eventCount>=0;eventCount--) {	    
	    sprintf(currentFilename,"%s/%s",
		    eventDir,eventLinkList[eventCount]->d_name);
	    sprintf(currentLinkname,"%s/%s",
		    eventLinkDir,eventLinkList[eventCount]->d_name);
	    fd = open(currentFilename,O_RDONLY);
	    if(fd == 0) {
		syslog(LOG_ERR,"Error opening file, will delete: %s",currentFilename);
		fprintf(stderr,"Error opening file, will delete: %s\n",currentFilename);		
		removeFile(currentFilename);
		removeFile(currentLinkname);
		//Mayhaps we should delete
		continue;
	    }
	    if(verbosity>1) { 
		printf("Opened Event File: %s\n",currentFilename);
	    }
	    
	    numBytes=read(fd,theBuffer,BSIZE);
	    if(numBytes<=0) {
		removeFile(currentLinkname);
		unlink(currentFilename);
		continue;
	    }
	    close (fd);
	    retVal=0;
//	    retVal = bufferAndWrite((unsigned char *)theBuffer, numBytes,0);
	    retVal = los_write((unsigned char *)theBuffer, numBytes);
//	    retVal = los_write((unsigned char *)theBuffer, numBytes);
	    if(verbosity>1) printf("retVal %d\n",retVal);
	    if (retVal < 0) {
		syslog(LOG_ERR,"Couldn't send file: %s",currentFilename);
		fprintf(stderr, "Couldn't telemeterize: %s\n\t%s\n",currentFilename,los_strerror());
		break;
	    }
	    else {
		removeFile(currentLinkname);
		removeFile(currentFilename);
//		unlink(currentFilename);
//		fillBufferWithPackets();
//		if(numBytesInBuffer) doWrite();    
	    }    	    	    
	    if((numEventLinks-eventCount)>maxEventsInARow) break;
	}
	
	for(eventCount=0;eventCount<numEventLinks;eventCount++)
	    free(eventLinkList[eventCount]);
	free(eventLinkList);
    }
    
    fillBufferWithPackets();
    if(numBytesInBuffer) doWrite(); 
}


int initDevice() {
    int retVal=los_init((unsigned char)losBus,(unsigned char)losSlot,0,1,100);
    if(retVal!=0) {
	syslog(LOG_ERR,"Problem opening LOS board: %s",los_strerror());	
	fprintf(stderr,"Problem opening LOS board: %s\n",los_strerror()); 
    }
    else if(verbosity) {
	printf("Succesfully opened board: Bus %d, Slot %d\n",losBus,losSlot);
    }


    return retVal;
}

int bufferAndWrite(unsigned char *buf, short nbytes, int doWrite) 
{
    int retVal=0; 
/*     if((numBytesInBuffer+nbytes)<(LOS_MAX_BYTES-1000)) { */
/* 	strcat(bufferToWrite,buf); */
/* 	numBytesInBuffer+=nbytes; */
/* 	return 0; */
/*     } */
/*     else { */
/* 	printf("Sending %d bytes\n",numBytesInBuffer); */
/* 	retVal=los_write((unsigned char*)bufferToWrite,numBytesInBuffer); */
/* //	bzero(bufferToWrite,LOS_MAX_BYTES); */
/* 	strcat(bufferToWrite,buf); */
/* 	numBytesInBuffer=nbytes; */
/* 	return retVal; */
/*     } */
//    printf("%d %d\n",numBytesInBuffer,nbytes);
    if((numBytesInBuffer+nbytes)<(LOS_MAX_BYTES-1000)) {
//	strcat(bufferToWrite,buf);
//	sprintf(bufferToWrite,"%s%s",bufferToWrite,buf);
	memcpy(&bufferToWrite[numBytesInBuffer],buf,nbytes);
	numBytesInBuffer+=nbytes;
	return numBytesInBuffer;
    }
    if(verbosity) printf("Sending %d bytes\n",numBytesInBuffer);
    retVal=los_write(bufferToWrite,numBytesInBuffer);
//    bzero(bufferToWrite,LOS_MAX_BYTES);
//    sprintf(bufferToWrite,"%s",buf);
    numBytesInBuffer=0;
    memcpy(buf,&bufferToWrite[numBytesInBuffer],nbytes);
    numBytesInBuffer+=nbytes;
    return retVal;
}

int doWrite() {
    
    return los_write(bufferToWrite,numBytesInBuffer);
    
}
