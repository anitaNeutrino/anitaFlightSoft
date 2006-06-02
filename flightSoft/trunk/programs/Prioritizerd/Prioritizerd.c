/*! \file Prioritizerd.c
    \brief The Prioritizerd program that creates Event objects 
    
    Reads the event objects written by Eventd, assigns a priority to each event based on the likelihood that it is an interesting event. Events with the highest priority will be transmitted to the ground first.
    March 2005 rjn@mps.ohio-state.edu
*/
#include "Prioritizerd.h"

int useUsbDisks=0;
int maxEventsPerDir=1000;
char eventdEventDir[FILENAME_MAX];
char eventdEventLinkDir[FILENAME_MAX];

char prioritizerdEventDir[FILENAME_MAX];
char prioritizerdEventLinkDir[FILENAME_MAX];

char prioritizerdPidFile[FILENAME_MAX];

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
	tempString=kvpGetString("eventdEventDir");
	if(tempString) {
	    strncpy(eventdEventDir,tempString,FILENAME_MAX-1);
	    strncpy(eventdEventLinkDir,tempString,FILENAME_MAX-1);
	    strcat(eventdEventLinkDir,"/link");
	    makeDirectories(eventdEventLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting eventdEventDir");
	    fprintf(stderr,"Error getting eventdEventDir\n");
	}

	//Output and Link Directories
	tempString=kvpGetString("prioritizerdEventDir");
	if(tempString) {
	    strncpy(prioritizerdEventDir,tempString,FILENAME_MAX-1);
	    sprintf(prioritizerdEventLinkDir,"%s/link",tempString);
	    makeDirectories(prioritizerdEventLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get prioritizerdEventDir");
	    fprintf(stderr,"Couldn't get prioritizerdEventDir\n");
	}
	

    }
    
    
    retVal=0;
    /* Main event getting loop. */
    while(1) {
	usleep(1);
	numEventLinks=getListofLinks(eventdEventLinkDir,&eventLinkList);
	printf("Got %d events\n",numEventLinks);
	/* What to do with our events? */	
	for(count=0;count<numEventLinks;count++) {
 	    printf("%s\n",eventLinkList[count]->d_name); 
	    sscanf(eventLinkList[count]->d_name,"hd_%d.dat",&doingEvent);
	    sprintf(linkFilename,"%s/%s",eventdEventLinkDir,
		    eventLinkList[count]->d_name);
	    sprintf(hdFilename,"%s/hd_%d.dat",eventdEventDir,
		    doingEvent);
	    sprintf(bodyFilename,"%s/ev_%d.dat",eventdEventDir,
		    doingEvent);
	    

	    retVal=fillBody(&theBody,bodyFilename);
	    retVal=fillHeader(&theHeader,hdFilename);
	    
	    //Must determine priority here
	    priority=1;
	    theHeader.priority=1;

	    //Write body and header for Archived
	    sprintf(archiveBodyFilename,"%s/ev_%lu.dat",prioritizerdEventDir,
		    theHeader.eventNumber);
	    writeBody(&theBody,archiveBodyFilename);
	    sprintf(archiveHdFilename,"%s/hd_%lu.dat",prioritizerdEventDir,
		    theHeader.eventNumber);
	    writeHeader(&theHeader,archiveHdFilename);
	    makeLink(archiveHdFilename,prioritizerdEventLinkDir);
    
	    //Write Header and make Link for telemetry 	    
	    sprintf(telemHdFilename,"%s/hd_%d.dat",HEADER_TELEM_DIR,
		    doingEvent);
	    retVal=writeHeader(&theHeader,telemHdFilename);
	    makeLink(telemHdFilename,HEADER_TELEM_LINK_DIR);

	    /* Delete input */
	    removeFile(linkFilename);
	    removeFile(bodyFilename);
	    removeFile(hdFilename);
	}
	
	/* Free up the space used by dir queries */
        for(count=0;count<numEventLinks;count++)
            free(eventLinkList[count]);
        free(eventLinkList);
//	usleep(10000);
    }	
}

