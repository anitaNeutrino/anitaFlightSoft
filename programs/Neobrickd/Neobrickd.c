/*! \file Neobrickd.c
  \brief The Neobrickd program that writes to the Neobrick

  Neobrickd looks in a to be determined directory for a to be determined file
  format that contains the address of a file to write to the Neobrick.
  July 2008  rjn@hep.ucl.ac.uk
*/
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <time.h>
#include <math.h>

#include "includes/anitaStructures.h"


//Flightsoft Includes
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "ftpLib/ftplib.h"
#include "pedestalLib/pedestalLib.h"
#include "compressLib/compressLib.h"
#include "linkWatchLib/linkWatchLib.h"


// Directories and gubbins 
int printToScreen=0;
int verbosity=0;
int disableNeobrick=0;

int wdNeo=0;


void makeFtpDirectories(char *theTmpDir);
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
int readConfigFile();

int main(int argc, char**argv) {
  int retVal,count;
  int numNeos;
  char *tempString;
  char linkName[FILENAME_MAX];
  char tmpFilename[FILENAME_MAX];
  
  // Log stuff 
  char *progName=basename(argv[0]);
  
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
  
    
  makeDirectories(NEOBRICKD_LINK_DIR);
  

  retVal=readConfigFile();
  
  if(!disableNeobrick) {
    // open the FTP connection   
    if (ftp_open("10.0.0.2", "anonymous", "")) {
      fprintf(stderr,"ftp_open -- %s\n",strerror(errno));
      syslog(LOG_ERR,"ftp_open -- %s\n",strerror(errno));
    }
  }

  if(wdNeo==0) {
    wdNeo=setupLinkWatchDir(NEOBRICKD_LINK_DIR);
    if(wdNeo<=0) {
      fprintf(stderr,"Unable to watch %s\n",NEOBRICKD_LINK_DIR);
      syslog(LOG_ERR,"Unable to watch %s\n",NEOBRICKD_LINK_DIR);
      exit(0);
    }
    numNeos=getNumLinks(wdNeo);
  }   

  do {
    retVal=readConfigFile();
    if(printToScreen) 
      printf("Initializing Eventd\n");
    
    int runNumber=getRunNumber();
    currentState=PROG_STATE_RUN;
    while(currentState==PROG_STATE_RUN) {
      retVal=checkLinkDirs(1,0);
      numNeos=getNumLinks(wdNeo);
      if(numNeos<1)
	continue;

      
      for(count=0;count<numNeos;count++) {
	tempString=getFirstLink(wdNeo);
	sprintf(linkName,"%s/%s",NEOBRICKD_LINK_DIR,tempString);
	readlink(linkName,tmpFilename,FILENAME_MAX);

	printf("readlink returned %s for %s\n",tmpFilename,linkName);
	//Now need to find the filename pointed to by link
	//	unlink(tmpFilename);
	unlink(linkName);
      }
    }
  } while(currentState==PROG_STATE_INIT);
    
  
  // the end 
  ftp_close();
  unlink(NEOBRICKD_PID_FILE);
  return 0;
}


void sendThisFile(int runNumber, char *tmpFilename)
{
    char fileName[FILENAME_MAX];
    sprintf(fileName,"/mnt/data/run%d/%s",runNumber,&tmpFilename[21]);    
    if(printToScreen)
      printf("New file -- %s\n",fileName);
    
    char *thisFile=basename(fileName);
    char *thisDir=dirname(fileName);
    makeFtpDirectories(thisDir);
    ftp_cd(thisDir);
    printf("%s -- %s\n",thisFile,thisDir);
    ftp_putfile(tmpFilename,thisFile,0,0);
}


void makeFtpDirectories(char *theTmpDir)
{
    char copyDir[FILENAME_MAX];
    char newDir[FILENAME_MAX];
    char *subDir;
    int retVal;
    
    strncpy(copyDir,theTmpDir,FILENAME_MAX);

    strcpy(newDir,"");

    subDir = strtok(copyDir,"/");
    while(subDir != NULL) {
	sprintf(newDir,"%s/%s",newDir,subDir);
	retVal=ftp_mkdir(newDir);
// 	    printf("%s\t%s\n",theTmpDir,newDir);
/* 	    printf("Ret Val %d\n",retVal); */
	subDir = strtok(NULL,"/");
    }
       
}


void handleBadSigs(int sig)
{
  
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
    unlink(NEOBRICKD_PID_FILE);
    syslog(LOG_INFO,"Neobrickd terminating");    
    exit(0);
}


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(NEOBRICKD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,NEOBRICKD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(NEOBRICKD_PID_FILE);
  return 0;
}

int readConfigFile() 
/* Load Neobrickd config stuff */
{
    /* Config file thingies */
    int status=0;
    char* eString ;

    kvpReset();
    status = configLoad ("Neobrickd.config","output") ;
    status = configLoad ("anitaSoft.config","global") ;
    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	disableNeobrick=kvpGetInt("disableNeobrick",0);	
    }
    else {
      eString=configErrorString (status) ;
      syslog(LOG_ERR,"Error reading Neobrickd.config: %s\n",eString);
      fprintf(stderr,"Error reading Neobrickd.config: %s\n",eString);
      
    }
    
    return status;
}
