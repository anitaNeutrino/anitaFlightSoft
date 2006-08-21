/*! \file Prioritizerd.c
    \brief The Prioritizerd program that creates Event objects 
    
    Reads the event objects written by Eventd, assigns a priority to each event based on the likelihood that it is an interesting event. Events with the highest priority will be transmitted to the ground first.
    March 2005 rjn@mps.ohio-state.edu
*/
#include "Prioritizerd.h"
#include "AnitaInstrument.h"
#include "Filters.h"
#include "pedestalLib/pedestalLib.h"
#include <sys/time.h>

//#define TIME_DEBUG 1

char prioritizerdPidFile[FILENAME_MAX];

void wasteTime(AnitaEventBody_t *bdPtr);
int readConfig();


#ifdef TIME_DEBUG
FILE *timeFile;
#endif    

//Global Variables
int printToScreen=0;
int verbosity=0;
float hornThresh=0;
int hornDiscWidth=0;
int hornSectorWidth=0;
float coneThresh=0;
int coneDiscWidth=0;



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
    PedSubbedEventBody_t pedSubBody;
    AnitaTransientBodyF_t unwrappedBody;
    AnitaInstrumentF_t theInstrument;
    AnitaInstrumentF_t theXcorr;
    AnitaChannelDiscriminator_t theDiscriminator;
    AnitaSectorLogic_t theMajority;


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
	    //Subtract Pedestals
	    subtractCurrentPeds(&theBody,&pedSubBody);

#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"5 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    unwrapAndBaselinePedSubbedEvent(&pedSubBody,&unwrappedBody);
	    BuildInstrumentF(&unwrappedBody,&theInstrument);
	    HornMatchedFilterAll(&theInstrument,&theXcorr);
	    DiscriminateFChannels(&theXcorr,&theDiscriminator,
				  hornThresh,hornDiscWidth,
				  coneThresh,coneDiscWidth);
	    FormSectorMajority(&theDiscriminator,&theMajority,
			       hornSectorWidth);




	    //Must determine priority here
	    priority=1;
	    theHeader.priority=0;
	
	    //Now Fill Generic Header and calculate checksum
	    fillGenericHeader(&theHeader,theHeader.gHdr.code,sizeof(AnitaEventHeader_t));
  
	     
	    //Write body and header for Archived
	    sprintf(archiveBodyFilename,"%s/ev_%lu.dat",PRIORITIZERD_EVENT_DIR,
		    theHeader.eventNumber);
	    rename(bodyFilename,archiveBodyFilename);

	    sprintf(archiveBodyFilename,"%s/psev_%lu.dat",PRIORITIZERD_EVENT_DIR,
                    theHeader.eventNumber);
	    writePedSubbedBody(&pedSubBody,archiveBodyFilename);


//	    writeBody(&theBody,archiveBodyFilename);
//	    writePedSubbedBody(&pedSubBody,archiveBodyFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"6 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    sprintf(archiveHdFilename,"%s/hd_%lu.dat",PRIORITIZERD_EVENT_DIR,
		    theHeader.eventNumber);
	    writeHeader(&theHeader,archiveHdFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"7 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    makeLink(archiveHdFilename,PRIORITIZERD_EVENT_LINK_DIR);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"8 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
    
	    //Write Header and make Link for telemetry 	    
	    sprintf(telemHdFilename,"%s/hd_%d.dat",HEADER_TELEM_DIR,
		    doingEvent);
	    retVal=writeHeader(&theHeader,telemHdFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"9 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    makeLink(telemHdFilename,HEADER_TELEM_LINK_DIR);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"10 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    
	    /* Delete input */
	    removeFile(linkFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"11 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
//	    removeFile(bodyFilename);
#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"12 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif
	    removeFile(hdFilename);

#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"13 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

	    wasteTime(&theBody);

#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"14 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

	}
	
	/* Free up the space used by dir queries */
        for(count=0;count<numEventLinks;count++)
            free(eventLinkList[count]);
        free(eventLinkList);
//	usleep(10000);
    }	
}

void wasteTime(AnitaEventBody_t *bdPtr) {
    int chan,samp,loop;
    for(loop=0;loop<10;loop++) {       
	for(chan=0;chan<NUM_DIGITZED_CHANNELS;chan++) {
	    for(samp=0;samp<MAX_NUMBER_SAMPLES;samp++) {
		bdPtr->channel[chan].data[samp]+=10*(-1+2*(loop%1));
	    }
	}	
    }
}


int readConfig()
// Load Prioritizerd config stuff
{
    // Config file thingies
//    KvpErrorCode kvpStatus=0;
    int status=0;
    char* eString ;
    kvpReset();
    status = configLoad ("Prioritizerd.config","output") ;
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
    status = configLoad ("Prioritizerd.config","prioritizerd");
    if(status == CONFIG_E_OK) {
	hornThresh=kvpGetFloat("hornThresh",250);
	coneThresh=kvpGetFloat("coneThresh",250);
	hornThresh=kvpGetInt("hornDiscWidth",13);
	coneThresh=kvpGetInt("coneDiscWidth",13);
	hornSectorWidth=kvpGetInt("hornSectorWidth",5);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Prioritizerd.config: %s\n",eString);
	fprintf(stderr,"Error reading Prioritizerd.config: %s\n",eString);
    }
    return status;
}
