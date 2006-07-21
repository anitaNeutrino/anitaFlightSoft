#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <time.h>


#include "anitaStructures.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"

extern  int versionsort(const void *a, const void *b);
int getListofHeaders(const char *theEventLinkDir, struct dirent ***namelist);
int writeAndMakeLink(AnitaEventHeader_t *theHeaderPtr, AnitaEventBody_t *theBodyPtr);
int fillBodyFromFile(AnitaEventBody_t *bodyPtr, gzFile openFile);
char *getFirstHeaderFilename(const char *theBaseDir);
int decodeChannel(char *buffer,EncodedSurfChannelHeader_t *encHdr, 
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
    int numBytesEvent=0,count=0,eventNum=0,doingEvent=0;
    int numBytesHead=0,count2,retVal=0;
    int dirNum=0,subDirNum;
    int usleepNum=(int)((float)1000000)/((float)EVENT_RATE);
//    GenericHeader_t *gHdr;
    char *tempString;


    /* Config file thingies */
    int status=0;
    char* eString ;
    

    /* Directory reading things */
    char bigEventFileName[FILENAME_MAX];
    char bigHeaderFileName[FILENAME_MAX];

    struct dirent **headerList;
    int numLinks;

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
    
    for(eventNum=0;1;eventNum+=10000) {
	dirNum=1000000*(int)((eventNum)/1000000);
	subDirNum=10000*(int)((eventNum)/10000);
	
	sprintf(realName,"%s/ev%d/ev%d/",dirName,dirNum,subDirNum);
//    printf("Dir: %s\n",realName);
	//insert while loop here
	numLinks=getListofHeaders(realName,&headerList);
	if(numLinks<=0) break;
	for(count=0;count<numLinks;count++) {
	    sscanf(headerList[count]->d_name,"hd_%d.dat.gz",&doingEvent);
	    sprintf(bigEventFileName,"%s/ev_%d.dat.gz",realName,doingEvent);
	    sprintf(bigHeaderFileName,"%s/hd_%d.dat.gz",realName,doingEvent);
//	printf("%s\n",bigHeaderFileName);
//	printf("%s\n",bigEventFileName); 
	    inputEvent=gzopen(bigEventFileName,"rb");
	    inputHead=gzopen(bigHeaderFileName,"rb");
	    
	    for(count2=0;count2<EVENTS_PER_FILE;count2++) {
		numBytesHead=gzread(inputHead,&theHeader,sizeof(AnitaEventHeader_t));
//		numBytesEvent=gzread(inputEvent,&theBody,sizeof(AnitaEventBody_t));
		retVal=fillBodyFromFile(&theBody,inputEvent);
		if(retVal!=0 ||
		   numBytesHead!=sizeof(AnitaEventHeader_t)) break;
		printf("Got Event %lu\n",theHeader.eventNumber);
		if(0) {
		    printf("channel[0].header.chanId = %d\n",theBody.channel[0].header.chanId);
		    printf("channel[1].header.chanId = %d\n",theBody.channel[1].header.chanId);
		}
		
		
		writeAndMakeLink(&theHeader,&theBody);
		usleep(usleepNum);
		
	    }
	    gzclose(inputEvent);
	    gzclose(inputHead);
	    if(count2!=EVENTS_PER_FILE) break;       
	}
    }
    return 0;
}


char *getFirstHeaderFilename(const char *theBaseDir) 
{
    struct dirent ***dirlist;
    struct dirent ***subdirlist;
    int n=scandir(theBaseDir,dirlist,0,versionsort);
    if(n<=0) {
	fprintf(stderr,"Error reading dir %s (%d)\n",theBaseDir,n);
    }
    

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


int fillBodyFromFile(AnitaEventBody_t *bodyPtr, gzFile openFile) {
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
//	cout << "Event " << surfHeader.eventNumber << endl;

	numBytesToRead=surfHeader.gHdr.numBytes-sizeof(EncodedSurfPacketHeader_t);	
	numBytesRead=gzread(openFile,bigBuffer,numBytesToRead);
	if(numBytesRead!=numBytesToRead) {
	    gzclose(openFile);
	    fprintf(stderr,"Only read %d (of %d) bytes\n",numBytesRead,sizeof(EncodedSurfPacketHeader_t));
	    return -6;
	}
	
	count=0;
	for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
	    //	  cout << count << "\t" << numBytes << endl;
	    chanHdPtr = (EncodedSurfChannelHeader_t*) &bigBuffer[count];
	    count+=sizeof(EncodedSurfChannelHeader_t);
	    decodeChannel(&bigBuffer[count],chanHdPtr,
			  &(bodyPtr->channel[getChanIndex(surf,chan)]));
	    count+=chanHdPtr->numBytes;	    	    
	}	    	    	        	
    }
    return 0;
}

int decodeChannel(char *buffer,EncodedSurfChannelHeader_t *encHdr, 
		  SurfChannelFull_t *chanPtr) {
    chanPtr->header=encHdr->rawHdr;
    switch(encHdr->encType) {
	case ENCODE_SOMETHING:
	    break;	    
	case ENCODE_NONE:
	default:
	  memcpy(&(chanPtr->data[0]),buffer,encHdr->numBytes);
	    break;   	   
    }
    return 0;

    //Need to add checksum check
}
