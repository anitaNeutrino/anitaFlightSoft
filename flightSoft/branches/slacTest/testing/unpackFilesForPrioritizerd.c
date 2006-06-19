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
#define EVENT_RATE 5 //Hz

int printToScreen=0;
int verbosity=0;


/*Event object*/
AnitaEventHeader_t theHeader;
AnitaEventBody_t theBody;

int main(int argc, char** argv) {
    char dirName[FILENAME_MAX];
    char realName[FILENAME_MAX];
    int numBytesEvent=0,count=0,eventNum=0,doingEvent=0;
    int numBytesHead=0,count2;
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
		numBytesEvent=gzread(inputEvent,&theBody,sizeof(AnitaEventBody_t));
		if(numBytesEvent!=sizeof(AnitaEventBody_t) ||
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
