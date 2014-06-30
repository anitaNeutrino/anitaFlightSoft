/*! \file LogWatchd.c
  \brief The LogWatchd program that watches the logs 
    
  Watches the logs and prof filesystem and sends down useful and informative thingies
  May 2008 rjn@hep.ucl.ac.uk
*/
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include <libgen.h> //For Mac OS X

//Flightsoft Includes
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "linkWatchLib/linkWatchLib.h"
#include "includes/anitaStructures.h"
#include "includes/anitaFlight.h"

#define MAX_RAW_BUFFER 4000

int readConfigFile();
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
int tailFile(char *filename, int numLines);
int catFile(char *filename);
int makeZippedFilePackets(char *filename,int fileTag);
void sendStartStruct(LogWatchdStart_t *startPtr);

/* Directories and gubbins */
int printToScreen=1;
int verbosity=0;
int startMessageTailLines=400;
int startAnitaTailLines=400;
int startBootTailLines=400;
int startProcPartitions=1;
int startProcInterrupts=1;
int startProcMounts=1;
int startLspci=1;
int startLsusb=1;

int hkDiskBitMask=0;
int disableUsb=0;
int disableNeobrick=0;
int disableHelium1=0;
int disableHelium2=0;


int main (int argc, char *argv[])
{
    int retVal;
    char currentFilename[FILENAME_MAX];
    int numLinks=0,wdLog=0,count=0;

    /* Directory reading things */
    char *tempString=0;

    /* Log stuff */
    char *progName=basename(argv[0]);
    LogWatchdStart_t theStart;
    LogWatchRequest_t theRequest;

    // Sort out PID File
    retVal=sortOutPidFile(progName);
    if(retVal!=0) {
      return retVal;
    }
    
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);
    signal(SIGTERM, handleBadSigs);
    signal(SIGINT, handleBadSigs);
    signal(SIGSEGV, handleBadSigs);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
    syslog(LOG_INFO,"LogWatchd starting");    
    
    makeDirectories(LOGWATCH_LINK_DIR);
    makeDirectories(REQUEST_TELEM_LINK_DIR);

    if(wdLog==0) {
      wdLog=setupLinkWatchDir(LOGWATCH_LINK_DIR);
      if(wdLog<=0) {
	fprintf(stderr,"Unable to watch %s\n",ACQD_EVENT_LINK_DIR);
	syslog(LOG_ERR,"Unable to watch %s\n",ACQD_EVENT_LINK_DIR);
	exit(0);
      }
      numLinks=getNumLinks(wdLog);
    }   


    do {
	retVal=readConfigFile();
	if(printToScreen) 
	    printf("Initializing LogWatchd\n");
	if(retVal!=CONFIG_E_OK) {
	    ///Arrgh
	}
	retVal=0;

	//Send the start bits and pieces
	sendStartStruct(&theStart);
	if(startProcPartitions) catFile("/proc/partitions");
	if(startProcMounts) catFile("/proc/mounts");
	if(startProcInterrupts) catFile("/proc/interrupts");
	if(startMessageTailLines)
	  tailFile("/var/log/messages",startMessageTailLines);
	if(startAnitaTailLines)
	  tailFile("/var/log/anita.log",startAnitaTailLines);
	if(startBootTailLines)
	  tailFile("/var/log/boot.log",startBootTailLines);
	
	catFile("/home/anita/flightSoft/config/anitaSoft.config");
	catFile("/home/anita/flightSoft/config/Acqd.config");
	catFile("/home/anita/flightSoft/config/Archived.config");
	catFile("/home/anita/flightSoft/config/Calibd.config");
	catFile("/home/anita/flightSoft/config/Cmdd.config");
	catFile("/home/anita/flightSoft/config/Eventd.config");
	catFile("/home/anita/flightSoft/config/GPSd.config");
	catFile("/home/anita/flightSoft/config/Hkd.config");
	catFile("/home/anita/flightSoft/config/LogWatchd.config");
	catFile("/home/anita/flightSoft/config/LOSd.config");
	catFile("/home/anita/flightSoft/config/Monitord.config");
	catFile("/home/anita/flightSoft/config/Neobrickd.config");
	catFile("/home/anita/flightSoft/config/Playbackd.config");
	catFile("/home/anita/flightSoft/config/Prioritizerd.config");
	catFile("/home/anita/flightSoft/config/SIPd.config");
	
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	  retVal=checkLinkDirs(1,0);
	  numLinks=getNumLinks(wdLog);
	  /* 	printf("Got %d log links\n",numLinks); */
	  if(numLinks<1) {
	    continue;
	  }
	  if(verbosity>2 && printToScreen) 
	    fprintf(stderr,"There are %d log links.\n",numLinks);

	  for(count=0;count<numLinks;count++) {
	    tempString=getFirstLink(wdLog);
	    sprintf(currentFilename,"%s/%s",LOGWATCH_LINK_DIR,tempString);


	    //Do something
	    if(!fillLogWatchRequest(&theRequest,currentFilename)) {
	      if(theRequest.numLines==0)
		catFile(theRequest.filename);
	      else
		tailFile(theRequest.filename,theRequest.numLines);
	    }

	    //Delete Files
	    removeFile(currentFilename);
	    sprintf(currentFilename,"%s/%s",LOGWATCH_DIR,tempString);
	    removeFile(currentFilename);
	  } //logLink loop
	}
    } while(currentState==PROG_STATE_INIT); 
    unlink(LOGWATCHD_PID_FILE);
    syslog(LOG_INFO,"LogWatchd terminating");
    return 0;
}


int readConfigFile() 
/* Load LogWatchd config stuff */
{
    /* Config file thingies */
    int status=0;
    char* eString ;


 /* Load Global Config */
    kvpReset();
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    /* Get Device Names and config stuff */
    if (status == CONFIG_E_OK) {
	hkDiskBitMask=kvpGetInt("hkDiskBitMask",0);
	disableUsb=kvpGetInt("disableUsb",1);
	if(disableUsb)
	    hkDiskBitMask&=~USB_DISK_MASK;
	disableNeobrick=kvpGetInt("disableNeobrick",1);
	if(disableNeobrick)
	    hkDiskBitMask&=~NEOBRICK_DISK_MASK;
	disableHelium2=kvpGetInt("disableHelium2",1);
	if(disableHelium2)
	    hkDiskBitMask&=~HELIUM2_DISK_MASK;
	disableHelium1=kvpGetInt("disableHelium1",1);
	if(disableHelium1)
	    hkDiskBitMask&=~HELIUM1_DISK_MASK;
    }
    kvpReset () ;
    status = configLoad ("LogWatchd.config","output") ;
    status = configLoad ("LogWatchd.config","logwatchd") ;
    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);	   
	startMessageTailLines=kvpGetInt("startMessageTailLines",400);
	startAnitaTailLines=kvpGetInt("startAnitaTailLines",400);
	startBootTailLines=kvpGetInt("startBootTailLines",400);
	startProcPartitions=kvpGetInt("startProcPartitions",1);
	startProcMounts=kvpGetInt("startProcMounts",1);
	startLspci=kvpGetInt("startLspci",1);
	startLsusb=kvpGetInt("startLsusb",1);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading LogWatchd.config: %s\n",eString);
	fprintf(stderr,"Error reading LogWatchd.config: %s\n",eString);	    
    }
    return status;
}


void handleBadSigs(int sig)
{
    fprintf(stderr,"Received sig %d -- will exit immediately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immediately\n",sig); 
    unlink(LOGWATCHD_PID_FILE);
    syslog(LOG_INFO,"LogWatchd terminating");    
    exit(0);
}


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(LOGWATCHD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,LOGWATCHD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(LOGWATCHD_PID_FILE);
  return 0;
}


int tailFile(char *filename, int numLines) {
    static int counter=0;
    char tailFilename[FILENAME_MAX];
    sprintf(tailFilename,"/tmp/tail_%s",basename(filename));
    char theCommand[FILENAME_MAX];
    sprintf(theCommand,"tail -n %d %s > %s",numLines,filename,tailFilename);
    system(theCommand);
    makeZippedFilePackets(tailFilename,counter);
    counter++;    
    return 0;
}

int catFile(char *filename) {
    static int counter=0;
    FILE *fp = fopen(filename,"r");
    int testBytes=0;
    if(fp) {
	fseek(fp,0,SEEK_END);
	testBytes=ftell(fp);
	fclose(fp);
    }
    if(testBytes==0) {
	char catFilename[FILENAME_MAX];
	sprintf(catFilename,"/tmp/cat_%s",basename(filename));
	char theCommand[FILENAME_MAX];
	sprintf(theCommand,"cat %s > %s",filename,catFilename);
	system(theCommand);
	makeZippedFilePackets(catFilename,counter);
	unlink(catFilename);
    }
    else {
	makeZippedFilePackets(filename,counter);
    }
    counter++;    
    return 0;
}

int makeZippedFilePackets(char *filename,int fileTag) 
{
    int retVal;
    time_t rawtime;
    time(&rawtime);
    ZippedFile_t *zipFilePtr;
    char outputName[FILENAME_MAX];
    char *inputBuffer=0;
    char *tempBuffer=0;
    char bufferToSend[100+MAX_RAW_BUFFER];
    char zippedBuffer[1000+MAX_RAW_BUFFER];
    
    unsigned int numBytesIn=0,numBytesOut=0,bytesThisSegment=0;
    int bytesRemaining=0;
    int count=0;
    int segmentNum=0;
    
    inputBuffer=readFile(filename,&numBytesIn);
    //    fprintf(stderr,"%s\n",inputBuffer);
    //    for(count=0;count<numBytesIn;count++) {
    //      if(inputBuffer[count]=='\n')
    //	printf("Line ends at %d\n",count);
    //    }
    if(!inputBuffer || !numBytesIn) {
	syslog(LOG_ERR,"Couldn't send %s, numBytesIn %d\n",filename,
	       numBytesIn);	
	return 0;
    }


    tempBuffer=inputBuffer;
    bytesRemaining=numBytesIn;
    
    while(bytesRemaining>0) {
      //      fprintf(stderr,"bytesRemaining %d, tempBuffer %d\n",bytesRemaining,(int)tempBuffer);
      if(bytesRemaining>MAX_RAW_BUFFER) {
	for(count=MAX_RAW_BUFFER;count>=0;count--) 
	  if(tempBuffer[count]=='\n')
	    break;
      
      
	bytesThisSegment=count+2;
	strncpy(bufferToSend,tempBuffer,count);
	bufferToSend[count+1]='\0';
	tempBuffer = &tempBuffer[count+1];
	bytesRemaining-=count+1;
      }
      else {
	bytesThisSegment=bytesRemaining;
	strncpy(bufferToSend,tempBuffer,bytesRemaining);
	bufferToSend[bytesRemaining]='\0';
	bytesRemaining=0;
      }
      //      fprintf(stderr,"%s\n",bufferToSend);
      numBytesOut=1000+MAX_RAW_BUFFER;

      zipFilePtr = (ZippedFile_t*) zippedBuffer;
      zipFilePtr->unixTime=rawtime;
      zipFilePtr->numUncompressedBytes=bytesThisSegment;
      zipFilePtr->segmentNumber=segmentNum;
      strncpy(zipFilePtr->filename,basename(filename),60);
      retVal=zipBuffer(bufferToSend,&zippedBuffer[sizeof(ZippedFile_t)],bytesThisSegment,&numBytesOut);
      if(retVal==0) {		
	fillGenericHeader(zipFilePtr,PACKET_ZIPPED_FILE,numBytesOut+sizeof(ZippedFile_t));
	sprintf(outputName,"%s/zipFile_%d_%d_%u.dat",REQUEST_TELEM_DIR,
		fileTag,segmentNum,zipFilePtr->unixTime);
	normalSingleWrite((unsigned char*)zippedBuffer,
			  outputName,numBytesOut+sizeof(ZippedFile_t));
	makeLink(outputName,REQUEST_TELEM_LINK_DIR);		
      }

      segmentNum++;
    }
    free(inputBuffer);
    if(printToScreen) fprintf(stderr,"Sent %s in %d segments\n",filename,segmentNum);
    return rawtime;
}


void sendStartStruct(LogWatchdStart_t *startPtr)
{
  char fileName[FILENAME_MAX];
  int retVal;
  time_t rawTime;
  FILE *fpUptime = fopen("/proc/uptime","r");   
  if(!fpUptime) {
    fprintf(stderr,"Couldn't open /proc/uptime -- %s\n",strerror(errno));
    syslog(LOG_ERR,"Couldn't open /proc/uptime -- %s\n",strerror(errno));
  }
  else {
    fscanf(fpUptime,"%f %f",&(startPtr->upTime),&(startPtr->idleTime));
    fclose(fpUptime);
  }
  startPtr->runNumber=getRunNumber();
  time(&rawTime);
  startPtr->unixTime=rawTime;
  fillGenericHeader(startPtr,PACKET_LOGWATCHD_START,sizeof(LogWatchdStart_t));


  sprintf(fileName,"%s/lwstart_%u.dat",REQUEST_TELEM_DIR,startPtr->unixTime);
  retVal=writeStruct(startPtr,fileName,sizeof(LogWatchdStart_t));
  retVal=makeLink(fileName,REQUEST_TELEM_LINK_DIR);
  retVal=simpleMultiDiskWrite(startPtr,sizeof(LogWatchdStart_t),startPtr->unixTime,STARTUP_ARCHIVE_DIR,"logwatchd",hkDiskBitMask);
}
