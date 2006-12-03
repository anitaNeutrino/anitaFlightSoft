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
void handleBadSigs(int sig);


#ifdef TIME_DEBUG
FILE *timeFile;
#endif    

//Global Control Variables
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
int FFTPeakMaxA=0;
int FFTPeakMaxB=0;
int FFTPeakWindowL=0;
int FFTPeakWindowR=0;
int FFTMaxChannels=0;
int RMSMax=0;
int RMSevents=0;
int WindowCut=400;
int BeginWindow=0;
int EndWindow=0;
int MethodMask=0;
int NuCut=0;

int priorityPPS1=2;
int priorityPPS2=3;

//ugly hack to count number of channels with fft peaks
//moved to AnitaInstrument.c
//int FFTNumChannels;

// global event thingees
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
AnitaChannelDiscriminator_t theBoxcarNoGuard;
//     AnitaSectorLogic_t theMajorityBoxcar;
AnitaSectorLogic_t theMajorityBoxcarH;
AnitaSectorLogic_t theMajorityBoxcarV;
//     AnitaCoincidenceLogic_t theCoincidenceBoxcarAll;
AnitaCoincidenceLogic_t theCoincidenceBoxcarH;
AnitaCoincidenceLogic_t theCoincidenceBoxcarV;
//    AnitaSectorLogic_t theMajorityBoxcar2;
AnitaSectorLogic_t theMajorityBoxcarH2;
AnitaSectorLogic_t theMajorityBoxcarV2;
//    AnitaCoincidenceLogic_t theCoincidenceBoxcarAll2;
AnitaCoincidenceLogic_t theCoincidenceBoxcarH2;
AnitaCoincidenceLogic_t theCoincidenceBoxcarV2;
int /*MaxBoxAll,*/MaxBoxH,MaxBoxV;
int /*MaxBoxAll2,*/MaxBoxH2,MaxBoxV2;
LogicChannel_t HornCounter,ConeCounter;
int HornMax;
int RMSnum;


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
//     int priority,score,score3,score4;
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

//event thingees were formerly here; now are globals

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
     signal(SIGTERM, handleBadSigs);
     signal(SIGINT, handleBadSigs);
     signal(SIGSEGV, handleBadSigs);
    
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


     do {
	  if(printToScreen) printf("Initalizing Prioritizerd\n");
	  retVal=readConfig();
	  if(retVal<0) {
	       syslog(LOG_ERR,"Problem reading Prioritizerd.config");
	       printf("Problem reading Prioritizerd.config\n");
	       exit(1);
	  }
	  currentState=PROG_STATE_RUN;
	  while(currentState==PROG_STATE_RUN) {

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
		
// all priority determination is now in AnitaInstrument.c
// communication is via global variables
		    theHeader.priority=determinePriority();		
		    // handle queue forcing of PPS here
		    int pri=theHeader.priority&0xf;
		    if((theHeader.turfio.trigType&0x2) && (priorityPPS1>=0 && priorityPPS1<=9))
			 pri=priorityPPS1;
		    if((theHeader.turfio.trigType&0x4) && (priorityPPS2>=0 && priorityPPS2<=9))
			 pri=priorityPPS2;
		    if(pri<0 || pri>9) pri=9;
		    theHeader.priority=(16*theHeader.priority)+pri;

	
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
     } while(currentState==PROG_STATE_INIT); 
     unlink(prioritizerdPidFile);    
     return 0;
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
	  FFTPeakMaxA=kvpGetInt("FFTPeakMaxA",2500);
	  FFTPeakMaxB=kvpGetInt("FFTPeakMaxB",2500);
	  FFTPeakWindowL=kvpGetInt("FFTPeakWindowL",0);
	  FFTPeakWindowR=kvpGetInt("FFTPeakWindowR",0);
	  FFTMaxChannels=kvpGetInt("FFTMaxChannels",0);
	  RMSMax=kvpGetInt("RMSMax",200);
	  RMSevents=kvpGetInt("RMSevents",1000);
	  WindowCut=kvpGetInt("WindowCut",400);
	  BeginWindow=kvpGetInt("BeginWindow",100);
	  EndWindow=kvpGetInt("EndWindow",100);
	  MethodMask=kvpGetInt("MethodMask",0xFFFF);
	  NuCut=kvpGetInt("NuCut",100);
     }
     else {
	  eString=configErrorString (status) ;
	  syslog(LOG_ERR,"Error reading Prioritizerd.config: %s\n",eString);
	  fprintf(stderr,"Error reading Prioritizerd.config: %s\n",eString);
     }
     kvpReset();
     status = configLoad ("Archived.config","archived");
     if(status == CONFIG_E_OK) {
	  priorityPPS1=kvpGetInt("priorityPPS1",3);
	  priorityPPS2=kvpGetInt("priorityPPS2",2);
     }
     else {
	  eString=configErrorString (status) ;
	  syslog(LOG_ERR,"Error reading Prioritizerd.config: %s\n",eString);
	  fprintf(stderr,"Error reading Prioritizerd.config: %s\n",eString);
     }
     return status;
}

void handleBadSigs(int sig)
{
     syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
     unlink(prioritizerdPidFile);
     syslog(LOG_INFO,"Prioritizerd terminating");
     exit(0);
}
