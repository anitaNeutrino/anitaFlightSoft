/*! \file Archived.c
  \brief The Archived program that writes Events to disk
    
  May 2006 rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <zlib.h>

#include "Archived.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "anitaFlight.h"



// Directories and gubbins 
char archivedPidFile[FILENAME_MAX];
//char prioritizerdEventDir[FILENAME_MAX];
//char prioritizerdEventLinkDir[FILENAME_MAX];

char mainEventArchivePrefix[FILENAME_MAX];
char backupEventArchivePrefix[FILENAME_MAX];


//char currentEventIndex[FILENAME_MAX];
//char currentEventDir[FILENAME_MAX];
//char currentEventBackupDir[FILENAME_MAX];
int priorityEncodingVal[NUM_PRIORITIES];
float priorityFractionRaw[NUM_PRIORITIES];

char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];

int useBackupDisk=1;
int useGzip=1;

//Event Structures
AnitaEventHeader_t theHead;
AnitaEventBody_t theBody;

//Encoding Buffer
unsigned char outputBuffer[MAX_WAVE_BUFFER];

int printToScreen=0;
int verbosity=0;


AnitaEventWriterStruct_t eventWriter;
FILE *fpIndex;


int main (int argc, char *argv[])
{
    int retVal,pri;
    char *tempString;
    /* Config file thingies */
    int status=0;
    char* eString ;

    
    /* Log stuff */
    char *progName=basename(argv[0]);
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
   
    /* Load Config */
    readConfigFile();
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;
    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("archivedPidFile");
	if(tempString) {
	    strncpy(archivedPidFile,tempString,FILENAME_MAX);
	    writePidFile(archivedPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedPidFile");
	    fprintf(stderr,"Couldn't get archivedPidFile\n");
	}

	tempString=kvpGetString("baseEventArchivePrefix");
	if(tempString) {
	    sprintf(mainEventArchivePrefix,"%s/%s",MAIN_DATA_DISK_LINK,
		    tempString);
	    sprintf(backupEventArchivePrefix,"%s/%s",BACKUP_DATA_DISK_LINK,
		    tempString);	    
	}
	else {
	    syslog(LOG_ERR,"Couldn't get baseEventArchivePrefix");
	    fprintf(stderr,"Couldn't get baseEventArchivePrefix\n");
	}

	useBackupDisk=kvpGetInt("useUsbDisks",1);
				
    }
    
    //Fill event dir names
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
	sprintf(eventTelemDirs[pri],"%s/%s%d",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);
	sprintf(eventTelemLinkDirs[pri],"%s/link",eventTelemDirs[pri]);
	makeDirectories(eventTelemLinkDirs[pri]);
    }
    makeDirectories(ANITA_INDEX_DIR);
    makeDirectories(PRIORITIZERD_EVENT_LINK_DIR);
    prepWriterStructs();
    retVal=0;
    /* Main event getting loop. */
    do {
	if(printToScreen) printf("Initalizing Archived\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading Archived.config");
	    printf("Problem reading Archived.config\n");
	}
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    checkEvents();
	    usleep(100);
	}
    } while(currentState==PROG_STATE_INIT);    
    return 0;
}



int readConfigFile() 
/* Load Archived config stuff */
{
    /* Config file thingies */
    int status=0,tempNum=0;
    char* eString ;
    KvpErrorCode kvpStatus;
    kvpReset();
    status = configLoad ("Archived.config","archived") ;
    status += configLoad ("Archived.config","output") ;

    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	useGzip=kvpGetInt("useGzip",1);
	tempNum=10;
	kvpStatus = kvpGetIntArray("priorityEncodingVal",
				   priorityEncodingVal,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priorityEncodingVal): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priorityEncodingVal): %s\n",
			kvpErrorString(kvpStatus));
	}

	tempNum=10;
	kvpStatus = kvpGetFloatArray("priorityFractionRaw",
				     priorityFractionRaw,&tempNum);	
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priorityFractionRaw): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priorityFractionRaw): %s\n",
			kvpErrorString(kvpStatus));
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Archived.config: %s\n",eString);
    }
    
    return status;
}

void checkEvents() 
{
    int count,retVal;
    /* Directory reading things */
    struct dirent **linkList;
    int numLinks;

    char currentHeadname[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    char currentBodyname[FILENAME_MAX];

    numLinks=getListofLinks(PRIORITIZERD_EVENT_LINK_DIR,&linkList);
    if(printToScreen && verbosity) printf("Found %d links\n",numLinks);
    for(count=0;count<numLinks;count++) {
	sprintf(currentHeadname,"%s/%s",PRIORITIZERD_EVENT_DIR,
		linkList[count]->d_name);
	sprintf(currentLinkname,"%s/%s",PRIORITIZERD_EVENT_LINK_DIR,
		linkList[count]->d_name);
	retVal=fillHeader(&theHead,currentHeadname);

	sprintf(currentBodyname,"%s/ev_%lu.dat",PRIORITIZERD_EVENT_DIR,
		theHead.eventNumber);
	retVal=fillBody(&theBody,currentBodyname);

	processEvent();
	removeFile(currentLinkname);
	removeFile(currentBodyname);
	removeFile(currentHeadname);
    }
	
    /* Free up the space used by dir queries */
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
	    
}

void processEvent() 
// This just fills the buffer with 9 (well ACTIVE_SURFS) EncodedSurfPacketHeader_t and their associated waveforms.
{
    EncodingType_t encType=(EncodingType_t) (theHead.priority & 0xf);    
    int count=0,surfStart=0;
    static unsigned long lastEvNum=0;
    int surf=0;
    int chan=0;
//    EncodedEventWrapper_t *evWrap = (EncodedEventWrapper_t*) &outputBuffer[0];
//    count+=sizeof(EncodedEventWrapper_t);

//    if(lastEvNum && (theHead.eventNumber-lastEvNum)!=1) {
//	printf("Event %lu followed %lu\n",theHead.eventNumber,lastEvNum);
//    }

    EncodedSurfPacketHeader_t *surfHdPtr;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedSurfPacketHeader_t*) &outputBuffer[count];
	surfStart=count;
	surfHdPtr->eventNumber=theHead.eventNumber;
	count+=sizeof(EncodedSurfPacketHeader_t);
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    count+=encodeChannel(encType,
				 &theBody.channel[chan+CHANNELS_PER_SURF*surf],
				 &outputBuffer[count]);
	}
	//Fill Generic Header_t;
	fillGenericHeader(surfHdPtr,PACKET_ENC_SURF,(count-surfStart));
							 
    }

    //evWrap.numBytes=count;
    lastEvNum=theHead.eventNumber;
    writeOutput(count);
    

}

int encodeChannel(EncodingType_t encType, SurfChannelFull_t *chanPtr, unsigned char *buffer) 
{
    EncodedSurfChannelHeader_t *chanHdPtr
	=(EncodedSurfChannelHeader_t*) &buffer[0];
    int count=0;    
    int encSize=0;
    chanHdPtr->rawHdr=chanPtr->header;
    count+=sizeof(EncodedSurfChannelHeader_t);
    
    switch(encType) {
	case ENCODE_NONE:
	    encSize=encodeWaveNone(&buffer[count],chanPtr);
	    break;
	default:
	    encType=ENCODE_NONE;
	    encSize=encodeWaveNone(&buffer[count],chanPtr);
	    break;	    
    }    
    chanHdPtr->numBytes=encSize;
    chanHdPtr->crc=simpleCrcShort((unsigned short*)&buffer[count],
				  encSize/2);
    return (count+encSize);        
}

int encodeWaveNone(unsigned char *buffer,SurfChannelFull_t *chanPtr) {    
    int encSize=MAX_NUMBER_SAMPLES*sizeof(unsigned short);
    memcpy(buffer,chanPtr->data,encSize);
    return encSize;
}
    


void writeOutput(int numBytes) {
//    char headName[FILENAME_MAX];
//    char bodyName[FILENAME_MAX];
//    char realHeadName[FILENAME_MAX];
//    char realBodyName[FILENAME_MAX];
//    char realBackupHeadName[FILENAME_MAX];
//    char realBackupBodyName[FILENAME_MAX];    
    char linkName[FILENAME_MAX];
    char mainDiskDir[FILENAME_MAX];
    char backupDiskDir[FILENAME_MAX];
    
    int len,retVal;
//    FILE *fpNorm;
//    gzFile fpZip;
//    int numThings=0;
    int pri=theHead.priority&0xf;
   
    if(pri>9) pri=9;

    //Get real disk names for index and telem
    if ((len = readlink(MAIN_DATA_DISK_LINK, mainDiskDir, FILENAME_MAX-1)) != -1)
	mainDiskDir[len] = '\0';
    if ((len = readlink(BACKUP_DATA_DISK_LINK,backupDiskDir, FILENAME_MAX-1)) != -1)
	backupDiskDir[len] = '\0';	

//    fprintf(stderr,"%s %s\n",mainDiskDir,backupDiskDir);
//    fprintf(stderr,"%d %d\n",strlen(MAIN_DATA_DISK_LINK),strlen(BACKUP_DATA_DISK_LINK));

    retVal=cleverEncEventWrite(outputBuffer,numBytes,&theHead,&eventWriter);
    
    if(!fpIndex) {
//	fprintf(stderr,"%s\n",EVENT_LINK_INDEX);
	fpIndex = fopen(EVENT_LINK_INDEX,"aw");
	if(!fpIndex) {
	    fprintf(stderr,"Error opening file %s (%s)\n",EVENT_LINK_INDEX,
		    strerror(errno));
	}
	if(theHead.eventNumber%10000==0) {
	    //Do some archiving
	}
    }
//    printf("fpIndex: %d\n",(int)fpIndex);
    fprintf(fpIndex,"%lu\t%s\t%s\n",theHead.eventNumber,mainDiskDir,backupDiskDir);
    fflush(fpIndex);


    //Need to think about this maybe we should take a hundred events and then write the file
    sprintf(linkName,"%s/ev_%lu.dat",eventTelemDirs[pri],theHead.eventNumber);
    symlink(eventWriter.currentEventFileName,linkName);
//    makeLink(eventWriter.currentEventFileName,eventTelemDirs[pri]);
//    makeLink(eventWriter.currentHeaderFileName,eventTelemLinkDirs[pri]);
    sprintf(linkName,"%s/hd_%lu.dat",eventTelemLinkDirs[pri],theHead.eventNumber);
    symlink(eventWriter.currentHeaderFileName,linkName);
	   

//    makeLink(realBodyName,eventTelemDirs[pri]);
//    makeLink(realHeadName,eventTelemLinkDirs[pri]);
 
}

unsigned short simpleCrcShort(unsigned short *p, unsigned long n)
{

    unsigned short sum = 0;
    unsigned long i;
    for (i=0L; i<n; i++) {
	sum += *p++;
    }
    return ((0xffff - sum) + 1);
}


void prepWriterStructs() {
    if(printToScreen) 
	printf("Preparing Writer Structs\n");

    //Event Writer
    strncpy(eventWriter.baseDirname,mainEventArchivePrefix,FILENAME_MAX-1);
    eventWriter.currentHeaderFilePtr=0;
    eventWriter.currentEventFilePtr=0;
    eventWriter.maxSubDirsPerDir=EVENT_FILES_PER_DIR;
    eventWriter.maxFilesPerDir=EVENT_FILES_PER_DIR;
    eventWriter.maxWritesPerFile=EVENTS_PER_FILE;
}
