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
char headerTelemDir[FILENAME_MAX];
char headerTelemLinkDir[FILENAME_MAX];
char eventTelemDir[NUM_PRIORITIES][FILENAME_MAX]; 
char eventTelemLinkDir[NUM_PRIORITIES][FILENAME_MAX]; 

char eventArchiveDir[FILENAME_MAX];
char eventUSBArchiveDir[FILENAME_MAX];
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
    AnitaEventHeader_t theEventHeader;
    AnitaEventBody_t theEventBody;
    	    
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
	tempString=kvpGetString("baseEventTelemDir");
	if(tempString) {
	    for(priority=0;priority<NUM_PRIORITIES;priority++) {
		sprintf(eventTelemDir[priority],
			"%s/pri%d",tempString,priority);
		sprintf(eventTelemLinkDir[priority],
			"%s/pri%d/link",tempString,priority);
		makeDirectories(eventTelemLinkDir[priority]);
	    }
	}
	else {
	    syslog(LOG_ERR,"Error getting baseEventTelemDir");
	    fprintf(stderr,"Error getting baseEventTelemDir\n");
	}
	tempString=kvpGetString("headerTelemDir");
	if(tempString) {
	    strncpy(headerTelemDir,tempString,FILENAME_MAX-1);
	    strncpy(headerTelemLinkDir,tempString,FILENAME_MAX-1);
	    strcat(headerTelemLinkDir,"/link");
	    makeDirectories(headerTelemLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting headerTelemDir");
	    fprintf(stderr,"Error getting headerTelemDir\n");
	}
	

	tempString=kvpGetString("mainDataDisk");
	if(tempString) {
	    strncpy(eventArchiveDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error getting mainDataDisk");
	    fprintf(stderr,"Error getting mainDataDisk\n");
	}
	tempString=kvpGetString("usbDataDiskLink");
	if(tempString) {
	    strncpy(eventUSBArchiveDir,tempString,FILENAME_MAX-1);
	}
	else {
	    syslog(LOG_ERR,"Error getting usbDataDiskLink");
	    fprintf(stderr,"Error getting usbDataDiskLink\n");
	}
	    
	    
	tempString=kvpGetString("baseEventArchiveDir");
	if(tempString) {
	    sprintf(eventArchiveDir,"%s/%s/",eventArchiveDir,tempString);
	    sprintf(eventUSBArchiveDir,"%s/%s/",eventUSBArchiveDir,tempString);
	}
	else {
	    syslog(LOG_ERR,"Error getting baseHouseArchiveDir");
	    fprintf(stderr,"Error getting baseHouseArchiveDir\n");
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
/* 	    printf("%s\n",eventLinkList[count]->d_name); */
	    sscanf(eventLinkList[count]->d_name,"hd_%d.dat",&doingEvent);
	    sprintf(linkFilename,"%s/%s",eventdEventLinkDir,
		    eventLinkList[count]->d_name);
	    sprintf(hdFilename,"%s/hd_%d.dat",eventdEventDir,
		    doingEvent);
	    sprintf(bodyFilename,"%s/ev_%d.dat",eventdEventDir,
		    doingEvent);
	    

	    retVal=fillBody(&theEventBody,bodyFilename);
	    retVal=fillHeader(&theEventHeader,hdFilename);
	    
	    //Must determine priority here
	    priority=1;
	    theEventHeader.priority=1;

	    sprintf(telemHdFilename,"%s/hd_%d.dat",headerTelemDir,
		    doingEvent);
	    //Write Header and make Link (in Header directory)
	    retVal=writeHeader(&theEventHeader,telemHdFilename);
	    makeLink(telemHdFilename,headerTelemLinkDir);

	    //Write data to Main Disk 
//	    if(firstTime || (doingEvent%1000)==0) checkOutputDirs(doingEvent);
	    sprintf(archiveHdFilename,"%s/hd_%d.dat",eventArchiveDir,
		    doingEvent);
	    sprintf(archiveBodyFilename,"%s/ev_%d.dat",eventArchiveDir,
		    doingEvent);	    
	    retVal=writeHeader(&theEventHeader,archiveHdFilename);
	    retVal=writeBody(&theEventBody,archiveBodyFilename);
	    if(useUsbDisks) {
		sprintf(archiveHdFilename,"%s/hd_%d.dat",eventUSBArchiveDir,
			doingEvent);
		sprintf(archiveBodyFilename,"%s/ev_%d.dat",eventUSBArchiveDir,
			doingEvent);	    
		retVal=writeHeader(&theEventHeader,archiveHdFilename);
		retVal=writeBody(&theEventBody,archiveBodyFilename);
	    }
	    
	    // Write output for Telem	    
	    writePacketsAndHeader(&theEventBody,&theEventHeader);

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

