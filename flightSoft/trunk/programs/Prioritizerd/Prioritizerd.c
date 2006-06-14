/*! \file Prioritizerd.c
    \brief The Prioritizerd program that creates Event objects 
    
    Reads the event objects written by Eventd, assigns a priority to each event based on the likelihood that it is an interesting event. Events with the highest priority will be transmitted to the ground first.
    March 2005 rjn@mps.ohio-state.edu
*/
#include "Prioritizerd.h"
//#define TIME_DEBUG 1

int useUsbDisks=0;
int maxEventsPerDir=1000;
//char acqdEventDir[FILENAME_MAX];
//char eventdEventDir[FILENAME_MAX];
//char eventdEventLinkDir[FILENAME_MAX];

//char prioritizerdEventDir[FILENAME_MAX];
//char prioritizerdEventLinkDir[FILENAME_MAX];

char prioritizerdPidFile[FILENAME_MAX];

#ifdef TIME_DEBUG
FILE *timeFile;
#endif    
int main (int argc, char *argv[])
{
    int retVal,count;
    char linkFilename[FILENAME_MAX];
    char hdFilename[FILENAME_MAX];
    char bodyFilename[FILENAME_MAX];
    char telemHdFilename[FILENAME_MAX];
    char archiveHdFilename[FILENAME_MAX];
    char archiveBodyFilename[FILENAME_MAX];

    char *tempString;
    int priority;
//    float probWriteOut9=0.03; /* Will become a config file thingy */

    /* Config file thingies */
    int status=0;
    int doingEvent=0;
    //    char* eString ;

    /* Directory reading things */
    struct dirent **eventLinkList;
    int numEventLinks;
    
    /* Log stuff */
    char *progName=basename(argv[0]);

   /*Event object*/
    AnitaEventHeader_t theHeader;
    AnitaEventBody_t theBody;


#ifdef TIME_DEBUG
    struct timeval timeStruct2;
    timeFile = fopen("/tmp/priTimeLog.txt","w");
    if(!timeFile) {
	fprintf(stderr,"Couldn't open time file\n");
	exit(0);
    }
#endif
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
/*     eString = configErrorString (status) ; */

    if (status == CONFIG_E_OK) {
	useUsbDisks=kvpGetInt("useUsbDisks",0);
	maxEventsPerDir=kvpGetInt("maxEventsPerDir",1000);
	tempString=kvpGetString("prioritizerdPidFile");
	if(tempString) {
	    strncpy(prioritizerdPidFile,tempString,FILENAME_MAX-1);
	    writePidFile(prioritizerdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdPidFile");
	    fprintf(stderr,"Error getting prioritizerdPidFile\n");
	}


	//Output and Link Directories

    }

    makeDirectories(HEADER_TELEM_LINK_DIR);
    makeDirectories(HEADER_TELEM_DIR);
    makeDirectories(EVENTD_EVENT_LINK_DIR);
    makeDirectories(PRIORITIZERD_EVENT_LINK_DIR);
    makeDirectories(ACQD_EVENT_LINK_DIR);

 
    retVal=0;
    /* Main event getting loop. */
    while(1) {

#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"0 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
//	usleep(1);
	numEventLinks=getListofLinks(EVENTD_EVENT_LINK_DIR,&eventLinkList);

	if(!numEventLinks) {
	    usleep(1000);
	    continue;
	}

#ifdef TIME_DEBUG
	gettimeofday(&timeStruct2,NULL);
	fprintf(timeFile,"1 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
//	printf("Got %d events\n",numEventLinks);
	/* What to do with our events? */	
	for(count=0;count<numEventLinks;count++) {
// 	    printf("%s\n",eventLinkList[count]->d_name); 
	    sscanf(eventLinkList[count]->d_name,"hd_%d.dat",&doingEvent);
	    sprintf(linkFilename,"%s/%s",EVENTD_EVENT_LINK_DIR,
		    eventLinkList[count]->d_name);
	    sprintf(hdFilename,"%s/hd_%d.dat",EVENTD_EVENT_DIR,
		    doingEvent);
	    sprintf(bodyFilename,"%s/ev_%d.dat",ACQD_EVENT_DIR,
		    doingEvent);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"2 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    
	    
	    retVal=fillBody(&theBody,bodyFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"3 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    retVal=fillHeader(&theHeader,hdFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"4 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    
	    //Must determine priority here
	    priority=1;
	    theHeader.priority=1;
	
	    //Now Fill Generic Header and calculate checksum
	    fillGenericHeader(&theHeader,PACKET_HD,sizeof(AnitaEventHeader_t));
    
	    //Write body and header for Archived
	    sprintf(archiveBodyFilename,"%s/ev_%lu.dat",PRIORITIZERD_EVENT_DIR,
		    theHeader.eventNumber);
	    writeBody(&theBody,archiveBodyFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"5 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    sprintf(archiveHdFilename,"%s/hd_%lu.dat",PRIORITIZERD_EVENT_DIR,
		    theHeader.eventNumber);
	    writeHeader(&theHeader,archiveHdFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"6 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    makeLink(archiveHdFilename,PRIORITIZERD_EVENT_LINK_DIR);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"7 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
    
	    //Write Header and make Link for telemetry 	    
	    sprintf(telemHdFilename,"%s/hd_%d.dat",HEADER_TELEM_DIR,
		    doingEvent);
	    retVal=writeHeader(&theHeader,telemHdFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"8 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    makeLink(telemHdFilename,HEADER_TELEM_LINK_DIR);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"9 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    
	    /* Delete input */
	    removeFile(linkFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"10 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    removeFile(bodyFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"11 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    removeFile(hdFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"12 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	}
	
	/* Free up the space used by dir queries */
        for(count=0;count<numEventLinks;count++)
            free(eventLinkList[count]);
        free(eventLinkList);
//	usleep(10000);
    }	
}

