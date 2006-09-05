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
#include "compressLib/compressLib.h"
#include "pedestalLib/pedestalLib.h"

#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"



// Directories and gubbins 
char archivedPidFile[FILENAME_MAX];

//char currentEventIndex[FILENAME_MAX];
//char currentEventDir[FILENAME_MAX];
//char currentEventBackupDir[FILENAME_MAX];
int priorityEncodingVal[NUM_PRIORITIES];
float priorityFractionRaw[NUM_PRIORITIES];

char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];


//Event Structures
AnitaEventHeader_t theHead;
AnitaEventBody_t theBody;
PedSubbedEventBody_t pedSubBody;

//Encoding Buffer
unsigned char outputBuffer[MAX_WAVE_BUFFER];

int printToScreen=0;
int verbosity=0;
int writeTelem=0;

int eventDiskBitMask;
AnitaEventWriterStruct_t eventWriter;
AnitaHkWriterStruct_t indexWriter;


char bladeName[FILENAME_MAX];
char usbIntName[FILENAME_MAX];
char usbExtName[FILENAME_MAX];

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
    signal(SIGTERM, sigUsr2Handler);

    //Dont' wait for children
    signal(SIGCLD, SIG_IGN); 
   
    /* Load Config */
    readConfigFile();
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;
    if (status == CONFIG_E_OK) {
	eventDiskBitMask=kvpGetInt("eventDiskBitMask",1);
	tempString=kvpGetString("archivedPidFile");
	if(tempString) {
	    strncpy(archivedPidFile,tempString,FILENAME_MAX);
	    writePidFile(archivedPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedPidFile");
	    fprintf(stderr,"Couldn't get archivedPidFile\n");
	}

	tempString=kvpGetString("bladeName");
	if(tempString) {
	    strncpy(bladeName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get bladeName");
	    fprintf(stderr,"Couldn't get bladeName\n");
	}


	tempString=kvpGetString("usbIntName");
	if(tempString) {
	    strncpy(usbIntName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbIntName");
	    fprintf(stderr,"Couldn't get usbIntName\n");
	}



	tempString=kvpGetString("usbExtName");
	if(tempString) {
	    strncpy(usbExtName,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get usbExtName");
	    fprintf(stderr,"Couldn't get usbExtName\n");
	}



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

    closeEventFilesAndTidy(&eventWriter);
    closeHkFilesAndTidy(&indexWriter);
    unlink(archivedPidFile);
    syslog(LOG_INFO,"Archived terminating");
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
	writeTelem=kvpGetInt("writeTelem",0);
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
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
    char currentPSBodyname[FILENAME_MAX];

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
	sprintf(currentPSBodyname,"%s/psev_%lu.dat",PRIORITIZERD_EVENT_DIR,
		theHead.eventNumber);
	retVal=fillPedSubbedBody(&pedSubBody,currentPSBodyname);
//	printf("Event %lu, Body %lu, PS Body %lu\n",theHead.eventNumber,
//	       theBody.eventNumber,pedSubBody.eventNumber);


	processEvent();
	removeFile(currentLinkname);
	removeFile(currentBodyname);
	removeFile(currentPSBodyname);
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

    CompressErrorCode_t retVal;
    EncodeControlStruct_t diskEncCntl;
    EncodeControlStruct_t telemEncCntl;
    int surf,chan,numBytes;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
//	    diskEncCntl.encTypes[surf][chan]=(ChannelEncodingType_t) priorityEncodingVal[(theHead.priority & 0xf)];
	    diskEncCntl.encTypes[surf][chan]= ENCODE_NONE;
	    telemEncCntl.encTypes[surf][chan]= ENCODE_LOSSLESS_BINARY;
	}
    }
 
//Steps
    //1) PedestalSubtract (and shift to 11bits)
    //2) Pack Event For On Board Storage
    //3) Pack Event For Telemetry

//    //1) PedestalSubtract (and shift to 11bits) 
//    subtractCurrentPeds(&theBody,&pedSubBody);

    //2) Pack Event For On Board Storage
    //Now depending on which option is selected we may either write out pedSubbed or not pedSubbed events to the hard disk (or just AnitaEventBody_t structs)
    memset(outputBuffer,0,MAX_WAVE_BUFFER);
    retVal=packEvent(&theBody,&diskEncCntl,outputBuffer,&numBytes);
    if(retVal==COMPRESS_E_OK) {
	writeOutputToDisk(numBytes);
    }
    else {
	syslog(LOG_ERR,"Error compressing event %lu\n",theBody.eventNumber);
	fprintf(stderr,"Error compressing event %lu\n",theBody.eventNumber);
    }

    //3) Pack Event For Telemetry
    //Again we have a range of options for event telemetry
    //For now we'll just hard code it to something and see how that goes
    memset(outputBuffer,0,MAX_WAVE_BUFFER);
    retVal=packPedSubbedEvent(&pedSubBody,&telemEncCntl,outputBuffer,&numBytes);
    if(writeTelem) {
	if(retVal==COMPRESS_E_OK)
	    writeOutputForTelem(numBytes);
	else {
	    syslog(LOG_ERR,"Error compressing event %lu for telemetry\n",theBody.eventNumber);
	    fprintf(stderr,"Error compressing event %lu for telemetry\n",theBody.eventNumber);
	}
    }
    
    

}



void writeOutputToDisk(int numBytes) {
    IndexEntry_t indEnt;
    int retVal;

    retVal=cleverEncEventWrite((unsigned char*)outputBuffer,numBytes,&theHead,&eventWriter);
    if(printToScreen && verbosity>1) {
	printf("Event %lu, (%d bytes)  %s \t%s\n",theHead.eventNumber,
	       numBytes,eventWriter.currentEventFileName[0],
	       eventWriter.currentHeaderFileName[0]);
    }

    //Now write the index
    indEnt.eventNumber=theHead.eventNumber;
    indEnt.eventDiskBitMask=eventWriter.writeBitMask;
    strncpy(indEnt.bladeLabel,bladeName,9);
    strncpy(indEnt.usbIntLabel,usbIntName,9);
    strncpy(indEnt.usbExtLabel,usbExtName,9);
    cleverIndexWriter(&indEnt,&indexWriter);

 
}

void writeOutputForTelem(int numBytes) {
    int retVal;
    char headName[FILENAME_MAX];
    char bodyName[FILENAME_MAX];
    int pri=theHead.priority&0xf;
   
    if(pri>9) pri=9;

    sprintf(bodyName,"%s/ev_%lu.dat",eventTelemDirs[pri],theHead.eventNumber);
    sprintf(headName,"%s/hd_%lu.dat",eventTelemDirs[pri],theHead.eventNumber);
    retVal=normalSingleWrite((unsigned char*)outputBuffer,bodyName,numBytes);
    if(retVal<0) {
	printf("Something wrong while writing %s\n",bodyName);
    }
    else {
	writeHeader(&theHead,headName);
	makeLink(headName,eventTelemLinkDirs[pri]);
    }

 
}



void prepWriterStructs() {
    int diskInd;
    if(printToScreen) 
	printf("Preparing Writer Structs\n");

    //Event Writer
    strncpy(eventWriter.relBaseName,EVENT_ARCHIVE_DIR,FILENAME_MAX-1);
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	eventWriter.currentHeaderFilePtr[diskInd]=0;
	eventWriter.currentEventFilePtr[diskInd]=0;
    }
    eventWriter.writeBitMask=eventDiskBitMask;

    //Index Writer
    strncpy(indexWriter.relBaseName,ANITA_INDEX_DIR,FILENAME_MAX-1);
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	indexWriter.currentFilePtr[diskInd]=0;
    }
    indexWriter.writeBitMask=0x12;
}


