/*! \file Prioritizerd.c
    \brief The Prioritizerd program that creates Event objects 
    
    Reads the event objects written by Eventd, assigns a priority to each event based on the likelihood that it is an interesting event. Events with the highest priority will be transmitted to the ground first.
    March 2005 rjn@mps.ohio-state.edu
*/
#include "Prioritizerd.h"
#include "AnitaInstrument.h"
#include "GenerateScore.h"
#include "Filters.h"
#include "pedestalLib/pedestalLib.h"
#include "utilLib/utilLib.h"
#include <sys/time.h>
#include <signal.h>

//#define TIME_DEBUG 1
//#define WRITE_DEBUG_FILE

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
int holdoff=0;
int delay=0;
int hornGuardOffset=30;
int hornGuardWidth=20;
int hornGuardThresh=50;
int coneGuardOffset=30;
int coneGuardWidth=20;
int coneGuardThresh=50;

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
    int priority,score,score3,score4;
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
    //original analysis
    AnitaChannelDiscriminator_t theDiscriminator;
    AnitaSectorLogic_t theMajority;
    AnitaSectorAnalysis_t sectorAna;
    //nonupdating discriminator method
    AnitaChannelDiscriminator_t theNonupdating;
    AnitaSectorLogic_t theMajorityNoUp;
    AnitaSectorLogic_t theHorizontal;
    AnitaSectorLogic_t theVertical;
    AnitaCoincidenceLogic_t theCoincidenceAll;
    AnitaCoincidenceLogic_t theCoincidenceH;
    AnitaCoincidenceLogic_t theCoincidenceV;
    int MaxAll, MaxH, MaxV;
    //xcorr peak boxcar method
    AnitaChannelDiscriminator_t theBoxcar;
    AnitaSectorLogic_t theMajorityBoxcar;
    AnitaSectorLogic_t theMajorityBoxcarH;
    AnitaSectorLogic_t theMajorityBoxcarV;
    AnitaCoincidenceLogic_t theCoincidenceBoxcarAll;
    AnitaCoincidenceLogic_t theCoincidenceBoxcarH;
    AnitaCoincidenceLogic_t theCoincidenceBoxcarV;
    int MaxBoxAll,MaxBoxH,MaxBoxV;
#ifdef TIME_DEBUG
    struct timeval timeStruct2;
    timeFile = fopen("/tmp/priTimeLog.txt","w");
    if(!timeFile) {
	fprintf(stderr,"Couldn't open time file\n");
	exit(0);
    }
#endif

#ifdef WRITE_DEBUG_FILE
    FILE *debugFile = fopen("/tmp/priDebugFile.txt","w");
    if(!debugFile) {
	fprintf(stderr,"Couldn't open debug file\n");
	exit(0);
    }
#endif
    	


    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, sigUsr2Handler);
    signal(SIGINT, sigUsr2Handler);
    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
/*     eString = configErrorString (status) ; */

    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("prioritizerdPidFile");
	if(tempString) {\
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
	retVal=readConfig();
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

	    
//	    printf("Event %lu, Body %lu, PS Body %lu\n",theHeader.eventNumber,
//	       theBody.eventNumber,pedSubBody.eventNumber);

#ifdef TIME_DEBUG
	    gettimeofday(&timeStruct2,NULL);
	    fprintf(timeFile,"5 %ld %ld\n",timeStruct2.tv_sec,timeStruct2.tv_usec);  
#endif

	    unwrapAndBaselinePedSubbedEvent(&pedSubBody,&unwrappedBody);
	    BuildInstrumentF(&unwrappedBody,&theInstrument);
	    HornMatchedFilterAll(&theInstrument,&theXcorr);
	    


//	    printf("%lu %d %d %f %f %f\n",theBody.eventNumber,theBody.channel[0].data[160],pedSubBody.channel[0].data[160],unwrappedBody.ch[0].data[160],theInstrument.topRing[0][0].data[160],theXcorr.topRing[0][0].data[160]);

	    
	    //Now we get to dick around for a while
//	    printf("400 %d %f %d\n",hornDiscWidth,coneThresh,coneDiscWidth);

// Ordinalry coincidence and scoring
/* 	    DiscriminateFChannels(&theXcorr,&theDiscriminator, */
/* 				  400,hornDiscWidth, */
/* 				  coneThresh,coneDiscWidth); */
/* 	    FormSectorMajority(&theDiscriminator,&theMajority, */
/* 			       hornSectorWidth); */
/* 	    AnalyseSectorLogic(&theMajority,&sectorAna); */
/* 	    score=GetSecAnaScore(&sectorAna); */
/* 	    score4=score; */
#ifdef WRITE_DEBUG_FILE
//	    fwrite(&sectorAna,sizeof(AnitaSectorAnalysis_t),1,debugFile);
	    fprintf(debugFile,"%lu 4 %d\n",theHeader.eventNumber,score);
#endif
/* 	    DiscriminateFChannels(&theXcorr,&theDiscriminator, */
/* 				  300,hornDiscWidth, */
/* 				  coneThresh,coneDiscWidth); */
/* 	    FormSectorMajority(&theDiscriminator,&theMajority, */
/* 			       hornSectorWidth); */
/* 	    AnalyseSectorLogic(&theMajority,&sectorAna); */
/* 	    score=GetSecAnaScore(&sectorAna); */
/* 	    score3=score; */
#ifdef WRITE_DEBUG_FILE
//	    fwrite(&sectorAna,sizeof(AnitaSectorAnalysis_t),1,debugFile);
	    fprintf(debugFile,"%lu 3 %d\n",theHeader.eventNumber,score);
#endif
// nonupdating discriminators and majorities
/* 	    DiscriminateFChannels_noup(&theXcorr,&theNonupdating, */
/* 				       hornThresh,hornDiscWidth, */
/* 				       coneThresh,coneDiscWidth, */
/* 				       holdoff); */
/* 	    FormSectorMajority(&theNonupdating,&theMajorityNoUp,hornSectorWidth); */
/* 	    FormSectorMajorityPol(&theNonupdating,&theHorizontal,hornSectorWidth,0); */
/* 	    FormSectorMajorityPol(&theNonupdating,&theVertical,hornSectorWidth,1); */
/* 	    MaxAll=FormSectorCoincidence(&theMajorityNoUp,&theCoincidenceAll, */
/* 				  delay,2*hornSectorWidth-2,2*hornSectorWidth-1); */
/* 	    MaxH=FormSectorCoincidence(&theHorizontal,&theCoincidenceH, */
/* 				  delay,hornSectorWidth-1,hornSectorWidth-1); */
/* 	    MaxV=FormSectorCoincidence(&theVertical,&theCoincidenceV, */
/* 				delay,hornSectorWidth-1,hornSectorWidth-1); */
	    //xcorr peak boxcar method
	    PeakBoxcarAll(&theXcorr,&theBoxcar,
		   hornDiscWidth,hornGuardOffset,
		   hornGuardWidth,hornGuardThresh,
		   coneDiscWidth,coneGuardOffset,
		   coneGuardWidth,coneGuardThresh);
	    FormSectorMajority(&theBoxcar,&theMajorityBoxcar,
			       hornSectorWidth);
	    FormSectorMajorityPol(&theBoxcar,&theMajorityBoxcarH,
				  hornSectorWidth,0);
	    FormSectorMajorityPol(&theBoxcar,&theMajorityBoxcarV,
				  hornSectorWidth,1);
	    MaxBoxAll=FormSectorCoincidence(&theMajorityBoxcar,
					    &theCoincidenceBoxcarAll,
					    8,2*hornSectorWidth-1,
					    2*hornSectorWidth-2);
	    MaxBoxH=FormSectorCoincidence(&theMajorityBoxcarH,
					  &theCoincidenceBoxcarH,
					  8,hornSectorWidth-1,
					  hornSectorWidth-1);
	    MaxBoxV=FormSectorCoincidence(&theMajorityBoxcarV,
					  &theCoincidenceBoxcarV,
					  8,hornSectorWidth-1,
					  hornSectorWidth-1);
	    //Sillyness forever...
	    //Must determine priority here
//	    priority=1;
//revised scoring here
	    priority=9;
	    if (MaxBoxH>=2*hornSectorWidth || MaxBoxV>=2*hornSectorWidth) 
		 priority=1;
	    else if (MaxBoxAll>=4*hornSectorWidth-4 ||
		     MaxBoxH>=2*hornSectorWidth-1 || 
		     MaxBoxV>=2*hornSectorWidth-1 ) 
		 priority=2
/* 	    else if (MaxH>=2*hornSectorWidth || MaxV>=2*hornSectorWidth)  */
/* 		 priority=3; */
/* 	    else if (MaxAll>=4*hornSectorWidth-4) priority=4; */
/* 	    else if (MaxH>=2*hornSectorWidth-1 || MaxH>=2*hornSectorWidth-1)  */
/* 		 priority=4; */
/* 	    else if(score4>=600) priority=5; */
/* 	    else if(score3>1900) priority=6; */
/* 	    else if(score3>1500) priority=6; */
/* 	    else if(score3>1000) priority=7; */
/* 	    else if(score3>600) priority=8; */

	    theHeader.priority=priority;
	
	    //Now Fill Generic Header and calculate checksum
	    fillGenericHeader(&theHeader,theHeader.gHdr.code,sizeof(AnitaEventHeader_t));
  
	     
	    //Write body and header for Archived
	    sprintf(archiveBodyFilename,"%s/ev_%lu.dat",PRIORITIZERD_EVENT_DIR,
		    theHeader.eventNumber);
	    rename(bodyFilename,archiveBodyFilename);
//	    unlink(bodyFilename);

//	    sprintf(archiveBodyFilename,"%s/psev_%lu.dat",PRIORITIZERD_EVENT_DIR,
//                    theHeader.eventNumber);
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

//	    wasteTime(&theBody);

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
	hornDiscWidth=kvpGetInt("hornDiscWidth",16);
	coneDiscWidth=kvpGetInt("coneDiscWidth",16);
	hornSectorWidth=kvpGetInt("hornSectorWidth",3);
	holdoff=kvpGetInt("holdoff",39);
	delay=kvpGetInt("delay",8);
	hornGuardOffset=kvpGetInt("hornGuardOffset",30);
	hornGuardWidth=kvpGetInt("hornGuardWidth",20);
	hornGuardThresh=kvpGetInt("hornGuardThresh",50);
	coneGuardOffset=kvpGetInt("coneGuardOffset",30);
	coneGuardWidth=kvpGetInt("coneGuardWidth",20);
	coneGuardThresh=kvpGetInt("coneGuardThresh",50);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Prioritizerd.config: %s\n",eString);
	fprintf(stderr,"Error reading Prioritizerd.config: %s\n",eString);
    }
    return status;
}
