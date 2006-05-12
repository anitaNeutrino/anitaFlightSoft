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
//void tryToSendData();
//int bufferAndWrite(unsigned char *buf, short nbytes,int doWrite);
//int fillBufferWithPackets();
int doWrite();
int readConfig();
int checkHeaders(int maxCopy);


/*Config Thingies*/
char losdPidFile[FILENAME_MAX];

//Packet Dirs
char eventTelemDirs[FILENAME_MAX][NUM_PRIORITIES];
char headerTelemDir[FILENAME_MAX];
char cmdTelemDir[FILENAME_MAX];
char surfHkTelemDir[FILENAME_MAX];
char turfHkTelemDir[FILENAME_MAX];
char hkTelemDir[FILENAME_MAX];
char gpsTelemDir[FILENAME_MAX];
char monitorTelemDir[FILENAME_MAX];

//Packet Link Dirs
char eventTelemLinkDirs[FILENAME_MAX][NUM_PRIORITIES];
char headerTelemLinkDir[FILENAME_MAX];
char cmdTelemLinkDir[FILENAME_MAX];
char surfHkTelemLinkDir[FILENAME_MAX];
char turfHkTelemLinkDir[FILENAME_MAX];
char hkTelemLinkDir[FILENAME_MAX];
char gpsTelemLinkDir[FILENAME_MAX];
char monitorTelemLinkDir[FILENAME_MAX];

int losBus;
int losSlot;
int verbosity;
int printToScreen;

// Bandwidth variables
int eventBandwidth=80;
int priorityBandwidths[NUM_PRIORITIES];


/*Global Variables*/
unsigned char rawBuffer[LOS_MAX_BYTES*5];
unsigned char losBuffer[LOS_MAX_BYTES];
int numBytesInBuffer=0;


int main(int argc, char *argv[])
{
    int pk,retVal;
    char *tempString;

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
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("losdPidFile");
	if(tempString) {
	    strncpy(losdPidFile,tempString,FILENAME_MAX);
	    writePidFile(losdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get losdPidFile");
	    fprintf(stderr,"Couldn't get losdPidFile\n");
	}
	
	tempString=kvpGetString("baseHouseTelemDir");
	if(tempString) {
	    strncpy(cmdTelemDir,tempString,FILENAME_MAX-1);
	    strncpy(surfHkTelemDir,tempString,FILENAME_MAX-1);
	    strncpy(turfHkTelemDir,tempString,FILENAME_MAX-1);
	    strncpy(hkTelemDir,tempString,FILENAME_MAX-1);
	    strncpy(gpsTelemDir,tempString,FILENAME_MAX-1);
	    strncpy(monitorTelemDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get baseHouseTelemDir");
	    fprintf(stderr,"Couldn't get baseHouseTelemDir\n");
	}
	
	tempString=kvpGetString("cmdEchoTelemSubDir");
	if(tempString) {
	    sprintf(cmdTelemDir,"%s/%s",cmdTelemDir,tempString);
	    sprintf(cmdTelemLinkDir,"%s/link",cmdTelemDir);
	    makeDirectories(cmdTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get cmdEchoTelemSubDir");
	    fprintf(stderr,"Couldn't get cmdEchoTelemSubDir\n");
	}
	
	tempString=kvpGetString("hkTelemSubDir");
	if(tempString) {
	    sprintf(hkTelemDir,"%s/%s",hkTelemDir,tempString);
	    sprintf(hkTelemLinkDir,"%s/link",hkTelemDir);
	    makeDirectories(hkTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get hkTelemSubDir");
	    fprintf(stderr,"Couldn't get hkTelemSubDir\n");
	}
	
	tempString=kvpGetString("surfHkTelemSubDir");
	if(tempString) {
	    sprintf(surfHkTelemDir,"%s/%s",surfHkTelemDir,tempString);
	    sprintf(surfHkTelemLinkDir,"%s/link",surfHkTelemDir);
	    makeDirectories(surfHkTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get surfHkTelemSubDir");
	    fprintf(stderr,"Couldn't get surfHkTelemSubDir\n");
	}
	
	tempString=kvpGetString("turfHkTelemSubDir");
	if(tempString) {
	    sprintf(turfHkTelemDir,"%s/%s",turfHkTelemDir,tempString);
	    sprintf(turfHkTelemLinkDir,"%s/link",turfHkTelemDir);
	    makeDirectories(turfHkTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get turfHkTelemSubDir");
	    fprintf(stderr,"Couldn't get turfHkTelemSubDir\n");
	}
	
	tempString=kvpGetString("gpsTelemSubDir");
	if(tempString) {
	    sprintf(gpsTelemDir,"%s/%s",gpsTelemDir,tempString);
	    sprintf(gpsTelemLinkDir,"%s/link",gpsTelemDir);
	    makeDirectories(gpsTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsTelemSubDir");
	    fprintf(stderr,"Couldn't get gpsTelemSubDir\n");
	}
	
	tempString=kvpGetString("monitorTelemSubDir");
	if(tempString) {
	    sprintf(monitorTelemDir,"%s/%s",monitorTelemDir,tempString);
	    sprintf(monitorTelemLinkDir,"%s/link",monitorTelemDir);
	    makeDirectories(monitorTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get monitorTelemSubDir");
	    fprintf(stderr,"Couldn't get monitorTelemSubDir\n");
	}
	
	tempString=kvpGetString("headerTelemDir");
	if(tempString) {
	    strncpy(headerTelemDir,tempString,FILENAME_MAX-1);
	    sprintf(headerTelemLinkDir,"%s/link",headerTelemDir);
	    makeDirectories(headerTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get headerTelemSubDir");
	    fprintf(stderr,"Couldn't get headerTelemSubDir\n");
	}

	tempString=kvpGetString("baseEventTelemDir");
	if(tempString) {
	    for(pk=0;pk<NUM_PRIORITIES;pk++) {
		sprintf(eventTelemDirs[pk],"%s/pk%d",tempString,pk);
		sprintf(eventTelemLinkDirs[pk],"%s/link",eventTelemDirs[pk]);
		makeDirectories(eventTelemLinkDirs[pk]);
	    }
	}
	else {
	    syslog(LOG_ERR,"Couldn't get baseEventTelemDir");
	    fprintf(stderr,"Couldn't get baseEventTelemDir\n");
	    exit(0);
	}

	    
	

	//printf("%d\t%s\n",kvpStatus,kvpErrorString(kvpStatus));
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }
    retVal=readConfig();
    retVal=initDevice();
    if(retVal!=0) {
	return 1;
    }

    
    do {
	if(verbosity) printf("Initializing LOSd\n");
	retVal=readConfig();
	currentState=PROG_STATE_RUN;
        while(currentState==PROG_STATE_RUN) {
//	    tryToSendData();
	    checkHeaders(4000);
	    doWrite();
	    usleep(1);
	}
    } while(currentState==PROG_STATE_INIT);

//    fprintf(stderr, "Bye bye\n");
    return 0;
}

/* int fillBufferWithPackets()  */
/* { */

/*     int pk; */
/*     int retVal,count; */
/*     int numLinks=0; */
/*     int filledBuffer=0; */
/*     int numBytes=0; */
/*     char currentFilename[FILENAME_MAX]; */
/*     char currentLinkname[FILENAME_MAX]; */
/*     int fd; */
/*     struct dirent **linkList; */
/*     for(pk=1;pk<numPacketDirs;pk++) { */
/* 	numLinks=getListofLinks(packetLinkDir[pk],&linkList); */
/* 	if(numLinks<=0) continue; */
/* 	if(verbosity) printf("Got %d links in %s\n",numLinks,packetLinkDir[pk]); */
/* //	if(numLinks) break; */
	
/* //	if(numLinks==0) return 0; */
/* 	//Need to put something here so that it doesn't dick around  */
/* 	//forever in pk4, when there is data waiting in pk1, etc. */
/* //    printf("%d links in packet %d\n",numLinks,pk); */
/* 	/\* Nasty RJN Hack *\/ */
/* 	if(numLinks>500) { */
/* 	    for(count=0;count<100;count++) {	     */
/* 		sprintf(currentFilename,"%s/%s", */
/* 			packetDir[pk],linkList[count]->d_name); */
/* 		sprintf(currentLinkname,"%s/%s", */
/* 			packetLinkDir[pk],linkList[count]->d_name); */
		
/* 		removeFile(currentLinkname); */
/* 		unlink(currentFilename); */
/* 	    } */
/* 	} */
/* 	for(count=0;count<numLinks;count++) */
/* 	    free(linkList[count]); */
/* 	free(linkList); */
/* 	numLinks=getListofLinks(packetLinkDir[pk],&linkList); */
/* 	if(numLinks<=0) continue; */
/* 	for(count=numLinks-1;count>=0;count--) { */
	
/* 	    sprintf(currentFilename,"%s/%s", */
/* 		    packetDir[pk],linkList[count]->d_name); */
/* 	    sprintf(currentLinkname,"%s/%s", */
/* 		    packetLinkDir[pk],linkList[count]->d_name); */
/* 	    fd = open(currentFilename,O_RDONLY); */
/* //	fp = fopen(currentFilename,"rb"); */
/* 	    if(fd == 0) { */
/* //	if(fp==NULL) { */
/* 		syslog(LOG_ERR,"Error opening file, will delete: %s",currentFilename); */
/* 		fprintf(stderr,"Error opening file, will delete: %s\n",currentFilename); */
		
/* 		removeFile(currentFilename); */
/* 		removeFile(currentLinkname); */
/* 		//Mayhaps we should delete */
/* 		continue; */
/* 	    } */
	    
	
	    
/* 	    if(verbosity>1) {  */
/* 		printf("Opened file: %s\n",currentFilename); */
/* 	    } */
	
/* 	    numBytes=read(fd,theBuffer,BSIZE); */
/* 	    if(numBytes<=0) { */
/* //		removeFile(currentLinkname); */
/* 		unlink(currentLinkname); */
/* 		unlink(currentFilename); */
/* 		continue; */
/* 	    } */
/* 	    close (fd); */
/* 	    retVal=0; */
/* //	    retVal = los_write((unsigned char *)theBuffer, numBytes); */
/* 	    retVal = bufferAndWrite((unsigned char *)theBuffer, numBytes,0); */
/* 	    if (retVal < 0) { */
/* 		syslog(LOG_ERR,"Couldn't send file: %s",currentFilename); */
/* 		fprintf(stderr, "Couldn't telemeterize: %s\n\t%s\n",currentFilename,los_strerror()); */
/* 		break; */
/* 	    } */
/* 	    else { */
/* 		removeFile(currentLinkname); */
/* 		removeFile(currentFilename); */
/* //	    unlink(currentLinkname); */
/* //	    unlink(currentFilename); */
/* 		if(retVal==0) { */
/* 		    filledBuffer=1; */
/* 		    break; */
/* 		} */
/* 	    }    	    	     */
/* 	    if((numLinks-count)>maxPacketsPerDir) break; */
	    
/* 	} */
	
/* 	for(count=0;count<numLinks;count++) */
/* 	    free(linkList[count]); */
/* 	free(linkList); */
/* 	if(filledBuffer) return 1; */
/*     } */
/*     return 0; */
/* }     */
    

/* void tryToSendData() */
/* { */
/*     int retVal,eventCount; */
/*     int numEventLinks=0; */
/*     int numBytes=0; */
/*     char currentFilename[FILENAME_MAX]; */
/*     char currentLinkname[FILENAME_MAX]; */

/* //    int numItems=0; */
/* //    FILE *fp; */
/*     int fd; */
/*     struct dirent **eventLinkList; */
    
/*     numEventLinks=getListofLinks(eventLinkDir,&eventLinkList); */
/*     if(verbosity) printf("Got %d links in %s\n",numEventLinks,eventLinkDir); */
/*     if(!numEventLinks) { */
/* 	fillBufferWithPackets(); */
/*     }	 */
/*     else { */
/* 	/\* Nasty RJN Hack *\/ */
/* 	if(numEventLinks>500) { */
/* 	    for(eventCount=0;eventCount<100;eventCount++) { */
/* 		sprintf(currentFilename,"%s/%s", */
/* 			    eventDir,eventLinkList[eventCount]->d_name); */
/* 		sprintf(currentLinkname,"%s/%s", */
/* 			eventLinkDir,eventLinkList[eventCount]->d_name); */
/* 		unlink(currentLinkname); */
/* 		unlink(currentFilename); */
/* 	    } */
/* 	} */
	
/* 	for(eventCount=0;eventCount<numEventLinks;eventCount++) */
/* 	    free(eventLinkList[eventCount]); */
/* 	free(eventLinkList); */
/* 	numEventLinks=getListofLinks(eventLinkDir,&eventLinkList); */
/* 	if(numEventLinks<=0) { */
/* 	    fillBufferWithPackets(); */
/* 	    if(numBytesInBuffer) doWrite(); */
/* 	    return; */
/* 	} */
/* 	for(eventCount=numEventLinks-1;eventCount>=0;eventCount--) {	     */
/* 	    sprintf(currentFilename,"%s/%s", */
/* 		    eventDir,eventLinkList[eventCount]->d_name); */
/* 	    sprintf(currentLinkname,"%s/%s", */
/* 		    eventLinkDir,eventLinkList[eventCount]->d_name); */
/* 	    fd = open(currentFilename,O_RDONLY); */
/* 	    if(fd == 0) { */
/* 		syslog(LOG_ERR,"Error opening file, will delete: %s",currentFilename); */
/* 		fprintf(stderr,"Error opening file, will delete: %s\n",currentFilename);		 */
/* 		removeFile(currentFilename); */
/* 		removeFile(currentLinkname); */
/* 		//Mayhaps we should delete */
/* 		continue; */
/* 	    } */
/* 	    if(verbosity>1) {  */
/* 		printf("Opened Event File: %s\n",currentFilename); */
/* 	    } */
	    
/* 	    numBytes=read(fd,theBuffer,BSIZE); */
/* 	    if(numBytes<=0) { */
/* 		removeFile(currentLinkname); */
/* 		unlink(currentFilename); */
/* 		continue; */
/* 	    } */
/* 	    close (fd); */
/* 	    retVal=0; */
/* //	    retVal = bufferAndWrite((unsigned char *)theBuffer, numBytes,0); */
/* 	    retVal = los_write((unsigned char *)theBuffer, numBytes); */
/* //	    retVal = los_write((unsigned char *)theBuffer, numBytes); */
/* 	    if(verbosity>1) printf("retVal %d\n",retVal); */
/* 	    if (retVal < 0) { */
/* 		syslog(LOG_ERR,"Couldn't send file: %s",currentFilename); */
/* 		fprintf(stderr, "Couldn't telemeterize: %s\n\t%s\n",currentFilename,los_strerror()); */
/* 		break; */
/* 	    } */
/* 	    else { */
/* 		removeFile(currentLinkname); */
/* 		removeFile(currentFilename); */
/* //		unlink(currentFilename); */
/* //		fillBufferWithPackets(); */
/* //		if(numBytesInBuffer) doWrite();     */
/* 	    }    	    	     */
/* 	    if((numEventLinks-eventCount)>maxEventsInARow) break; */
/* 	} */
	
/* 	for(eventCount=0;eventCount<numEventLinks;eventCount++) */
/* 	    free(eventLinkList[eventCount]); */
/* 	free(eventLinkList); */
/*     } */
    
/*     fillBufferWithPackets(); */
/*     if(numBytesInBuffer) doWrite();  */
/* } */


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

int doWrite() {
    
    return los_write(losBuffer,numBytesInBuffer);
    numBytesInBuffer=0;
    
}

int readConfig()
// Load LOSd config stuff
{
    // Config file thingies
    KvpErrorCode kvpStatus=0;
    int status=0,tempNum,count;
    char* eString ;
    kvpReset();
    status = configLoad ("LOSd.config","output") ;
    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",-1);
	verbosity=kvpGetInt("verbosity",-1);
	if(printToScreen<0) {
	    syslog(LOG_WARNING,
		   "Couldn't fetch printToScreen, defaulting to zero");
	    printToScreen=0;
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading LOSd.config: %s\n",eString);
	fprintf(stderr,"Error reading LOSd.config: %s\n",eString);
    }
    kvpReset();
    status = configLoad ("LOSd.config","losd");
    if(status == CONFIG_E_OK) {
	losBus=kvpGetInt("losBus",1);
	losSlot=kvpGetInt("losSlot",1);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading LOSd.config: %s\n",eString);
	fprintf(stderr,"Error reading LOSd.config: %s\n",eString);
    }
    kvpReset();
    status = configLoad ("LOSd.config","bandwidth") ;
    if(status == CONFIG_E_OK) {
	eventBandwidth=kvpGetInt("eventBandwidth",80);
	tempNum=10;
	kvpStatus = kvpGetIntArray("priorityBandwidths",priorityBandwidths,&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priorityBandwidths): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priorityBandwidths): %s\n",
			kvpErrorString(kvpStatus));
	}
	else {
	    for(count=0;count<tempNum;count++) {
		
	    }
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading LOSd.config: %s\n",eString);
	fprintf(stderr,"Error reading LOSd.config: %s\n",eString);
    }

    return status;
}

int checkHeaders(int maxCopy)
/* Looks in the event header dir and fills buffer up to maxCopy */
{
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int fd,numLinks,count,numBytes,totalBytes=0;
    struct dirent **linkList;


    numLinks=getListofLinks(headerTelemLinkDir,&linkList); 
    if(numLinks<=0) {
	return 0;
    }
        
    for(count=numLinks-1;count>=0;count--) {
	sprintf(currentFilename,"%s/%s",headerTelemDir,
		linkList[count]->d_name);
	sprintf(currentLinkname,"%s/%s",
		headerTelemLinkDir,linkList[count]->d_name);
	fd = open(currentFilename,O_RDONLY);
	if(fd == 0) {
	    syslog(LOG_ERR,"Error opening file, will delete: %s",currentFilename);
	    fprintf(stderr,"Error opening file, will delete: %s\n",currentFilename);
	    removeFile(currentFilename);
	    removeFile(currentLinkname);
	    continue;
	}
	if(verbosity>1) {
	    printf("Opened  File: %s\n",currentFilename);
	}
	    
	numBytes=read(fd,&(losBuffer[numBytesInBuffer]),LOS_MAX_BYTES-numBytesInBuffer);
	if(numBytes<=0) {
	    removeFile(currentLinkname);
	    unlink(currentFilename);
	    continue;
	}
	numBytesInBuffer+=numBytes;
	totalBytes+=numBytes;
	close (fd);
	removeFile(currentLinkname);
	removeFile(currentFilename);

	if((totalBytes+sizeof(AnitaEventHeader_t))>maxCopy ||
	   (numBytesInBuffer+sizeof(AnitaEventHeader_t))>LOS_MAX_BYTES) break;
    }
    
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
    
    return totalBytes;
}



