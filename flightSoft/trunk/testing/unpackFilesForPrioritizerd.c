#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <time.h>


#include "includes/anitaStructures.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"

#define READ_NEW_DATA

extern  int versionsort(const void *a, const void *b);
int getListofHeaders(const char *theEventLinkDir, struct dirent ***namelist);
int writeAndMakeLink(AnitaEventHeader_t *theHeaderPtr, AnitaEventBody_t *theBodyPtr);
int fillBodyFromFileNew(AnitaEventBody_t *bodyPtr, gzFile openFile);
int fillBodyFromFileSlac(AnitaEventBody_t *bodyPtr, gzFile openFile);
int getFirstHeaderNumber(const char *theBaseDir);
int decodeChannel(char *buffer,EncodedSurfChannelHeader_t *encHdr, 
		  SurfChannelFull_t *chanPtr);
int decodeChannelSlac(char *buffer,SlacEncodedSurfChannelHeader_t *encHdr, 
		      SurfChannelFull_t *chanPtr);
#define EVENT_RATE 5 //Hz


int getChanIndex(int surf, int chan) {
    //Assumes chan 0-8 and surf 0-9
    return chan + 9*surf;

}

int printToScreen=0;
int verbosity=0;

char bigBuffer[100000];

/*Event object*/
AnitaEventHeader_t theHeader;
AnitaEventBody_t theBody;

int main(int argc, char** argv) {
    char dirName[FILENAME_MAX];
    char realName[FILENAME_MAX];
    int count=0,eventNum=0,doingEvent=0;
    int numBytesHead=0,count2,retVal=0;
    int dirNum=0,subDirNum;
    int usleepNum=(int)((float)1000000)/((float)EVENT_RATE);
//    GenericHeader_t *gHdr;
    int eventCount=0;


    
    makeDirectories(PRIORITIZERD_EVENT_LINK_DIR);
    makeDirectories(EVENTD_EVENT_LINK_DIR);
    makeDirectories(ACQD_EVENT_LINK_DIR);

    /* Directory reading things */
    char bigEventFileName[FILENAME_MAX];
    char bigHeaderFileName[FILENAME_MAX];

    struct dirent **headerList;
    int numLinks;
    int moreData=1;

    //File handles
    gzFile inputEvent;
    gzFile inputHead;


    if(argc<2) {
	printf("Usage:\t%s <packetDir> <optional event rate>\n",basename(argv[0]));
	return 0;
    }
    strncpy(dirName,argv[1],FILENAME_MAX-1);
    if(argc==3) {
	usleepNum=(int)((float)1000000)/(atof(argv[2]));
    }


    printf("Input Dir: %s\n",dirName);
    printf("Body Output Dir: %s\n",ACQD_EVENT_DIR);
    printf("Header Output Dir: %s\n",EVENTD_EVENT_DIR);
    printf("Link Dir: %s\n",EVENTD_EVENT_LINK_DIR);
    
//    for(eventNum=0;1;eventNum+=10000) {
    eventNum=getFirstHeaderNumber(dirName);
    while(1) {

	dirNum=1000000*(int)((eventNum)/1000000);
	subDirNum=10000*(int)((eventNum)/10000);

	sprintf(realName,"%s/ev%d/ev%d/",dirName,dirNum,subDirNum);
	printf("Dir: %s\n",realName);
	//insert while loop here
//	exit(0);
	numLinks=getListofHeaders(realName,&headerList);
	if(numLinks<=0) break;
	for(count=0;count<numLinks;count++) {
	    sscanf(headerList[count]->d_name,"hd_%d.dat.gz",&doingEvent);
	    sprintf(bigEventFileName,"%s/encev_%d.dat.gz",realName,doingEvent);
	    sprintf(bigHeaderFileName,"%s/hd_%d.dat.gz",realName,doingEvent);
//	printf("%s\n",bigHeaderFileName);
//	printf("%s\n",bigEventFileName); 
	    inputEvent=gzopen(bigEventFileName,"rb");
	    inputHead=gzopen(bigHeaderFileName,"rb");
	    
	    for(count2=0;count2<EVENTS_PER_FILE;count2++) {
		numBytesHead=gzread(inputHead,&theHeader,sizeof(AnitaEventHeader_t));
//		numBytesEvent=gzread(inputEvent,&theBody,sizeof(AnitaEventBody_t));
		
#ifdef READ_NEW_DATA
		retVal=fillBodyFromFileNew(&theBody,inputEvent);
#else
		retVal=fillBodyFromFileSlac(&theBody,inputEvent);
#endif
		if(retVal!=0 ||
		   numBytesHead!=sizeof(AnitaEventHeader_t)) {
		    moreData=0;
		    break;
		}
		if(theHeader.eventNumber%100==0)
		    printf("Got Event %lu\t%lu\n",theHeader.eventNumber,theBody.eventNumber);
		if(0) {
		    printf("channel[0].header.chanId = %d\n",theBody.channel[0].header.chanId);
		    printf("channel[1].header.chanId = %d\n",theBody.channel[1].header.chanId);
		}
		
		
		writeAndMakeLink(&theHeader,&theBody);
		usleep(usleepNum);
		eventCount++;
		if(theHeader.eventNumber%100==99) break;
		
	    }
	    gzclose(inputEvent);
	    gzclose(inputHead);
//	    if(count2!=EVENTS_PER_FILE) break;       
	}	
	if(!moreData)
	    break;
	eventNum=theHeader.eventNumber+1;
    }
    if(eventCount) {
	printf("Read %d events\n",eventCount);
    }
    return 0;
}


int getFirstHeaderNumber(const char *theBaseDir) 
{
    struct dirent **dirlist;
    struct dirent **subdirlist;
    struct dirent **filelist;
    char nextDir[FILENAME_MAX];
    char fileName[FILENAME_MAX];
    int nDir,nSub,nFile,firstNumber;
    nDir=scandir(theBaseDir,&dirlist,0,versionsort);
    if(nDir<=0) {
	fprintf(stderr,"Error reading dir %s (%d)\n",theBaseDir,nDir);
	return 0;
    }
    sprintf(nextDir,"%s/%s",theBaseDir,dirlist[2]->d_name);
    printf("Next Dir is %s\n",nextDir);

    nSub=scandir(nextDir,&subdirlist,0,versionsort);
    if(nSub<=0) {
	fprintf(stderr,"Error reading dir %s (%d)\n",theBaseDir,nSub);
	return 0;
    }
    sprintf(nextDir,"%s/%s",nextDir,subdirlist[2]->d_name);
    printf("Next Dir is %s\n",nextDir);

    nFile=getListofHeaders(nextDir,&filelist);
    if(nFile<=0) {
	fprintf(stderr,"Error reading dir %s (%d)\n",theBaseDir,nFile);
	return 0;
    }
    sprintf(fileName,"%s/%s",nextDir,filelist[0]->d_name);
    printf("File is %s\n",fileName);

    sscanf(filelist[0]->d_name,"hd_%d.dat.gz",&firstNumber);
    

    //Need to add free's here
    return firstNumber;
    
}

int getListofHeaders(const char *theEventLinkDir, struct dirent ***namelist)
{
//    int count;
    int n = scandir(theEventLinkDir, namelist, filterHeaders, versionsort);
    if (n < 0) {
	syslog(LOG_ERR,"scandir %s: %s",theEventLinkDir,strerror(errno));
    }	
//     for(count=0;count<n;count++)  
// 	printf("%s\n",(*namelist)[count]->d_name); 
    return n;	    
}


int writeAndMakeLink(AnitaEventHeader_t *theHeaderPtr, AnitaEventBody_t *theBodyPtr)
{
    char theFilename[FILENAME_MAX];
    int retVal;
//    FILE *testfp;
    
    /* Write ev_ file first */
    sprintf(theFilename,"%s/ev_%ld.dat",ACQD_EVENT_DIR,
	    theHeaderPtr->eventNumber);
    retVal=writeBody(theBodyPtr,theFilename);
    

    /* Should probably do something with retVal */
       
    sprintf(theFilename,"%s/hd_%ld.dat",EVENTD_EVENT_DIR,
	    theHeaderPtr->eventNumber);
    if(printToScreen && verbosity) printf("Writing %s\n",theFilename);
    retVal=writeHeader(theHeaderPtr,theFilename);

    /* Make links, not sure what to do with return value here */
    retVal=makeLink(theFilename,EVENTD_EVENT_LINK_DIR);
    
    
    return retVal;
}



int fillBodyFromFileSlac(AnitaEventBody_t *bodyPtr, gzFile openFile) {
    SlacEncodedSurfChannelHeader_t *chanHdPtr;
    EncodedSurfPacketHeader_t surfHeader;
    int count=0,numBytesToRead,numBytesRead;
    int surf,chan;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	
	//Need to read next header
	numBytesRead=gzread(openFile,&surfHeader,sizeof(EncodedSurfPacketHeader_t));
	if(numBytesRead!=sizeof(EncodedSurfPacketHeader_t)) {
	    gzclose(openFile);
	    fprintf(stderr,"Only read %d (of %d)  bytes\n",numBytesRead,sizeof(EncodedSurfPacketHeader_t));
	    return -7;
	}
//	printf("Event %lu\n",surfHeader.eventNumber);
	bodyPtr->eventNumber=surfHeader.eventNumber;
	numBytesToRead=surfHeader.gHdr.numBytes-sizeof(EncodedSurfPacketHeader_t);	
	numBytesRead=gzread(openFile,bigBuffer,numBytesToRead);
	if(numBytesRead!=numBytesToRead) {
	    gzclose(openFile);
	    fprintf(stderr,"Only read %d (of %d) bytes\n",numBytesRead,sizeof(EncodedSurfPacketHeader_t));
	    return -6; 
	}
	
	count=0;
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
//	    printf("RJN -- %d %d\n",chan,count);
	    chanHdPtr = (SlacEncodedSurfChannelHeader_t*) &bigBuffer[count];
	    count+=sizeof(SlacEncodedSurfChannelHeader_t);
	    decodeChannelSlac(&bigBuffer[count],chanHdPtr,
			  &(bodyPtr->channel[getChanIndex(surf,chan)]));
	    count+=chanHdPtr->numBytes;	    	    
	}	    	    	        	
    }
    return 0;
}


int fillBodyFromFileNew(AnitaEventBody_t *bodyPtr, gzFile openFile) {
    EncodedSurfChannelHeader_t *chanHdPtr;
    EncodedSurfPacketHeader_t surfHeader;
    int count=0,numBytesToRead,numBytesRead;
    int surf,chan;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	
	//Need to read next header
	numBytesRead=gzread(openFile,&surfHeader,sizeof(EncodedSurfPacketHeader_t));
	if(numBytesRead!=sizeof(EncodedSurfPacketHeader_t)) {
	    gzclose(openFile);
	    fprintf(stderr,"Only read %d (of %d)  bytes\n",numBytesRead,sizeof(EncodedSurfPacketHeader_t));
	    return -7;
	}
//	printf("Event %lu\n",surfHeader.eventNumber);
	bodyPtr->eventNumber=surfHeader.eventNumber;
	numBytesToRead=surfHeader.gHdr.numBytes-sizeof(EncodedSurfPacketHeader_t);	
	numBytesRead=gzread(openFile,bigBuffer,numBytesToRead);
	if(numBytesRead!=numBytesToRead) {
	    gzclose(openFile);
	    fprintf(stderr,"Only read %d (of %d) bytes\n",numBytesRead,sizeof(EncodedSurfPacketHeader_t));
	    return -6; 
	}
	
	count=0;
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
//	    printf("RJN -- %d %d\n",chan,count);
	    chanHdPtr = (EncodedSurfChannelHeader_t*) &bigBuffer[count];
	    count+=sizeof(EncodedSurfChannelHeader_t);
	    decodeChannel(&bigBuffer[count],chanHdPtr,
			  &(bodyPtr->channel[getChanIndex(surf,chan)]));
	    count+=chanHdPtr->numBytes;	    	    
	}	    	    	        	
    }
    return 0;
}


int decodeChannelSlac(char *buffer,SlacEncodedSurfChannelHeader_t *encHdr, 
		  SurfChannelFull_t *chanPtr) {
    chanPtr->header.chanId=encHdr->rawHdr.chanId;
    chanPtr->header.chipIdFlag=encHdr->rawHdr.chipIdFlag;
    chanPtr->header.firstHitbus=encHdr->rawHdr.firstHitbus;
    chanPtr->header.lastHitbus=encHdr->rawHdr.lastHitbus;
    switch(encHdr->encType) {
//	case ENCODE_SOMETHING:
//	    break;	    
	case ENCODE_NONE:
	default:
	  memcpy(&(chanPtr->data[0]),buffer,encHdr->numBytes);
	    break;   	   
    }
    return 0;

    //Need to add checksum check
}

int decodeChannel(char *buffer,EncodedSurfChannelHeader_t *encHdr, 
		  SurfChannelFull_t *chanPtr) {
    chanPtr->header=encHdr->rawHdr;
    switch(encHdr->encType) {
//	case ENCODE_SOMETHING:
//	    break;	    
	case ENCODE_NONE:
	default:
	  memcpy(&(chanPtr->data[0]),buffer,encHdr->numBytes);
	    break;   	   
    }
    return 0;

    //Need to add checksum check
}
