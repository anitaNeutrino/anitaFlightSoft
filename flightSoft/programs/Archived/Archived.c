/*! \file Archived.c
  \brief The Archived program that creates Event objects 
    
  Takes data from Acqd (waveforms), GPSd (gps time) and Calibd (calibration)
  and forms event objects. It passes these objects on to Prioritizerd.
  August 2004  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "anitaFlight.h"

#define MAX_FILES_PER_COMMAND 10


int readConfigFile();
void executeCommand(char *tarCommand);
void checkAdu5TTT();
void checkAdu5PAT();
void checkAdu5SAT();
void checkHk();

/*Global Variables*/
int gpsTTTNumber=100;
int gpsPATNumber=100;
int gpsSATNumber=100;
int hkNumber=100;
int eventBodyNumber=1;
int eventHeaderNumber=100;
int gzipGps=1;
int gzipHk=1;
int gzipEventBody=1;
int gzipEventHeader=1;
int printToScreen=1;
int useSecondDisk=0;

/* Directories and gubbins */
char archivedMainDisk[FILENAME_MAX];
char archivedSecondDisk[FILENAME_MAX];
char archivedGpsAdu5TTTDir[FILENAME_MAX];
char archivedGpsAdu5PATDir[FILENAME_MAX];
char archivedGpsAdu5SATDir[FILENAME_MAX];
char archivedEventDir[FILENAME_MAX];
char archivedHkDir[FILENAME_MAX];
char archivedCalibDir[FILENAME_MAX];
char gpsdAdu5TTTArchiveDir[FILENAME_MAX];
char gpsdAdu5TTTArchiveLinkDir[FILENAME_MAX];
char gpsdAdu5PATArchiveDir[FILENAME_MAX];
char gpsdAdu5PATArchiveLinkDir[FILENAME_MAX];
char gpsdAdu5SATArchiveDir[FILENAME_MAX];
char gpsdAdu5SATArchiveLinkDir[FILENAME_MAX];
char hkdArchiveDir[FILENAME_MAX];
char hkdArchiveLinkDir[FILENAME_MAX];
char prioritizerdArchiveDir[FILENAME_MAX];
char prioritizerdArchiveLinkDir[FILENAME_MAX];

int main (int argc, char *argv[])
{
    int retVal;
    char *tempString;
    /* Config file thingies */
    int status=0;
    char* eString ;

    char tempDir[FILENAME_MAX];
    
    /* Log stuff */
    char *progName=basename(argv[0]);
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Set signal handlers */
    signal(SIGINT, sigIntHandler);
    signal(SIGTERM,sigTermHandler);
   
    /* Load Config */
    readConfigFile();
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;
    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("archivedMainDisk");
	if(tempString) {
	    strncpy(archivedMainDisk,tempString,FILENAME_MAX-1);	    
	    makeDirectories(archivedMainDisk);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedMainDisk");
	    fprintf(stderr,"Couldn't get archivedMainDisk\n");
	    exit(0);
	}
	tempString=kvpGetString("archivedSecondDisk");
	if(tempString) {
	    strncpy(archivedSecondDisk,tempString,FILENAME_MAX-1);
	    makeDirectories(archivedSecondDisk);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedSecondDisk");
	    fprintf(stderr,"Couldn't get archivedSecondDisk\n");
	    exit(0);
	}
	tempString=kvpGetString("archivedGpsAdu5TTTDir");
	if(tempString) {
	    strncpy(archivedGpsAdu5TTTDir,tempString,FILENAME_MAX-1);
	    sprintf(tempDir,"%s/%s",archivedMainDisk,archivedGpsAdu5TTTDir);
	    makeDirectories(tempDir);
	    if(useSecondDisk) {
		sprintf(tempDir,"%s/%s",
			archivedSecondDisk,archivedGpsAdu5TTTDir);
		makeDirectories(tempDir);
	    }
		
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedGpsAdu5TTTDir");
	    fprintf(stderr,"Couldn't get archivedGpsAdu5TTTDir\n");
	    exit(0);
	}
	tempString=kvpGetString("archivedGpsAdu5PATDir");
	if(tempString) {
	    strncpy(archivedGpsAdu5PATDir,tempString,FILENAME_MAX-1);
	    sprintf(tempDir,"%s/%s",archivedMainDisk,archivedGpsAdu5PATDir);
	    makeDirectories(tempDir);
	    if(useSecondDisk) {
		sprintf(tempDir,"%s/%s",
			archivedSecondDisk,archivedGpsAdu5PATDir);
		makeDirectories(tempDir);
	    }
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedGpsAdu5PATDir");
	    fprintf(stderr,"Couldn't get archivedGpsAdu5PATDir\n");
	    exit(0);
	}
	tempString=kvpGetString("archivedGpsAdu5SATDir");
	if(tempString) {
	    strncpy(archivedGpsAdu5SATDir,tempString,FILENAME_MAX-1);
	    sprintf(tempDir,"%s/%s",archivedMainDisk,archivedGpsAdu5SATDir);
	    makeDirectories(tempDir);
	    if(useSecondDisk) {
		sprintf(tempDir,"%s/%s",
			archivedSecondDisk,archivedGpsAdu5SATDir);
		makeDirectories(tempDir);
	    }
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedGpsAdu5SATDir");
	    fprintf(stderr,"Couldn't get archivedGpsAdu5SATDir\n");
	    exit(0);
	}
	tempString=kvpGetString("archivedEventDir");
	if(tempString) {
	    strncpy(archivedEventDir,tempString,FILENAME_MAX-1);
	    sprintf(tempDir,"%s/%s",archivedMainDisk,archivedEventDir);
	    makeDirectories(tempDir);
	    if(useSecondDisk) {
		sprintf(tempDir,"%s/%s",
			archivedSecondDisk,archivedEventDir);
		makeDirectories(tempDir);
	    }
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedEventDir");
	    fprintf(stderr,"Couldn't get archivedEventDir\n");
	    exit(0);
	}
	tempString=kvpGetString("archivedHkDir");
	if(tempString) {
	    strncpy(archivedHkDir,tempString,FILENAME_MAX-1);
	    sprintf(tempDir,"%s/%s",archivedMainDisk,archivedHkDir);
	    makeDirectories(tempDir);
	    if(useSecondDisk) {
		sprintf(tempDir,"%s/%s",
			archivedSecondDisk,archivedHkDir);
		makeDirectories(tempDir);
	    }
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedHkDir");
	    fprintf(stderr,"Couldn't get archivedHkDir\n");
	    exit(0);
	}
	tempString=kvpGetString("archivedCalibDir");
	if(tempString) {
	    strncpy(archivedCalibDir,tempString,FILENAME_MAX-1);
	    sprintf(tempDir,"%s/%s",archivedMainDisk,archivedCalibDir);
	    makeDirectories(tempDir);
	    if(useSecondDisk) {
		sprintf(tempDir,"%s/%s",
			archivedSecondDisk,archivedCalibDir);
		makeDirectories(tempDir);
	    }
	}
	else {
	    syslog(LOG_ERR,"Couldn't get archivedCalibDir");
	    fprintf(stderr,"Couldn't get archivedCalibDir\n");
	    exit(0);
	}
	tempString=kvpGetString("gpsdAdu5TTTArchiveDir");
	if(tempString) {
	    strncpy(gpsdAdu5TTTArchiveDir,tempString,FILENAME_MAX-1);
	    makeDirectories(gpsdAdu5TTTArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdAdu5TTTArchiveDir");
	    fprintf(stderr,"Couldn't get gpsdAdu5TTTArchiveDir\n");
	    exit(0);
	}
	tempString=kvpGetString("gpsdAdu5TTTArchiveLinkDir");
	if(tempString) {
	    strncpy(gpsdAdu5TTTArchiveLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(gpsdAdu5TTTArchiveLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdAdu5TTTArchiveLinkDir");
	    fprintf(stderr,"Couldn't get gpsdAdu5TTTArchiveLinkDir\n");
	    exit(0);
	}
	tempString=kvpGetString("gpsdAdu5PATArchiveDir");
	if(tempString) {
	    strncpy(gpsdAdu5PATArchiveDir,tempString,FILENAME_MAX-1);
	    makeDirectories(gpsdAdu5PATArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdAdu5PATArchiveDir");
	    fprintf(stderr,"Couldn't get gpsdAdu5PATArchiveDir\n");
	    exit(0);
	}
	tempString=kvpGetString("gpsdAdu5PATArchiveLinkDir");
	if(tempString) {
	    strncpy(gpsdAdu5PATArchiveLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(gpsdAdu5PATArchiveLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdAdu5PATArchiveLinkDir");
	    fprintf(stderr,"Couldn't get gpsdAdu5PATArchiveLinkDir\n");
	    exit(0);
	}
	tempString=kvpGetString("gpsdAdu5SATArchiveDir");
	if(tempString) {
	    strncpy(gpsdAdu5SATArchiveDir,tempString,FILENAME_MAX-1);
	    makeDirectories(gpsdAdu5SATArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdAdu5SATArchiveDir");
	    fprintf(stderr,"Couldn't get gpsdAdu5SATArchiveDir\n");
	    exit(0);
	}
	tempString=kvpGetString("gpsdAdu5SATArchiveLinkDir");
	if(tempString) {
	    strncpy(gpsdAdu5SATArchiveLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(gpsdAdu5SATArchiveLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get gpsdAdu5SATArchiveLinkDir");
	    fprintf(stderr,"Couldn't get gpsdAdu5SATArchiveLinkDir\n");
	    exit(0);
	}
	tempString=kvpGetString("hkdArchiveDir");
	if(tempString) {
	    strncpy(hkdArchiveDir,tempString,FILENAME_MAX-1);
	    makeDirectories(hkdArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get hkdArchiveDir");
	    fprintf(stderr,"Couldn't get hkdArchiveDir\n");
	    exit(0);
	}
	tempString=kvpGetString("hkdArchiveLinkDir");
	if(tempString) {
	    strncpy(hkdArchiveLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(hkdArchiveLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get hkdArchiveLinkDir");
	    fprintf(stderr,"Couldn't get hkdArchiveLinkDir\n");
	    exit(0);
	}
	tempString=kvpGetString("prioritizerdArchiveDir");
	if(tempString) {
	    strncpy(prioritizerdArchiveDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get prioritizerdArchiveDir");
	    fprintf(stderr,"Couldn't get prioritizerdArchiveDir\n");
	    exit(0);
	}
	tempString=kvpGetString("prioritizerdArchiveLinkDir");
	if(tempString) {
	    strncpy(prioritizerdArchiveLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdArchiveLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get prioritizerdArchiveLinkDir");
	    fprintf(stderr,"Couldn't get prioritizerdArchiveLinkDir\n");
	    exit(0);
	}

    }
    

    retVal=0;
    /* Main event getting loop. */

    do {
	if(printToScreen) printf("Initalizing Archived\n");
	retVal=readConfigFile();
	if(retVal<0) {
	    syslog(LOG_ERR,"Problem reading GPSd.config");
	    printf("Problem reading GPSd.config\n");
	    exit(1);
	}
	currentState=PROG_STATE_RUN;
	while(currentState==PROG_STATE_RUN) {
	    checkAdu5TTT();
	    checkAdu5PAT();
	    checkAdu5SAT();
	    checkHk();
	    sleep(1);
	}
    } while(currentState==PROG_STATE_INIT);    
    return 0;
}


void executeCommand(char *tarCommand) {
    if(printToScreen) printf("%s\n\n",tarCommand);
    int retVal=system(tarCommand);
    if(retVal<0) {
	syslog(LOG_ERR,"Archived problem tarring");
	fprintf(stderr,"Archived problem tarring\n");
    }

}


int readConfigFile() 
/* Load Archived config stuff */
{
    /* Config file thingies */
    int status=0;
    char* eString ;
    kvpReset();
    status = configLoad ("Archived.config","archived") ;
    status += configLoad ("Archived.config","output") ;

    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",0);
	gpsTTTNumber=kvpGetInt("gpsTTTNumber",100);
	gpsPATNumber=kvpGetInt("gpsPATNumber",100);
	gpsSATNumber=kvpGetInt("gpsSATNumber",100);
	hkNumber=kvpGetInt("hkNumber",100);
	eventBodyNumber=kvpGetInt("eventBodyNumber",1);
	eventHeaderNumber=kvpGetInt("eventHeaderNumber",100);
	gzipGps=kvpGetInt("gzipGps",1);
	gzipHk=kvpGetInt("gzipHk",1);
	gzipEventBody=kvpGetInt("gzipEventBody",1);
	gzipEventHeader=kvpGetInt("gzipEventHeader",1);
	useSecondDisk=kvpGetInt("useSecondDisk",0);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading Archived.config: %s\n",eString);
    }
    
    return status;
}

void checkAdu5TTT() 
{
    int count;
    /* Directory reading things */
    struct dirent **linkList;
    int numLinks;

    long unixTime,subTime;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    char tarCommand[FILENAME_MAX*MAX_FILES_PER_COMMAND];
    char gzipCommand[FILENAME_MAX];
    char gzipFile[FILENAME_MAX];
    char tempDir[FILENAME_MAX];

    numLinks=getListofLinks(gpsdAdu5TTTArchiveLinkDir,&linkList);
    if(printToScreen) printf("Found %d links\n",numLinks);
    if(numLinks>gpsTTTNumber) {
	//Do something
	sscanf(linkList[0]->d_name,
	       "gps_%ld_%ld.dat",&unixTime,&subTime);
	sprintf(tarCommand,"cd %s ; tar -cf /tmp/adu5ttt%ld_%ld.tar ",gpsdAdu5TTTArchiveDir,unixTime,subTime);
	
	for(count=0;count<numLinks;count++) {
	    if((((count)%MAX_FILES_PER_COMMAND)==0) && count) {
//		    printf("%s\n",tarCommand);
		executeCommand(tarCommand);
		sprintf(tarCommand,"cd %s ; tar -rf /tmp/adu5ttt%ld_%ld.tar ",gpsdAdu5TTTArchiveDir,unixTime,subTime);
	    }	
	    strcat(tarCommand,linkList[count]->d_name);
	    strcat(tarCommand," ");
		
	}
	executeCommand(tarCommand);	   
	if(gzipGps) {
	    sprintf(gzipCommand,"gzip /tmp/adu5ttt%ld_%ld.tar",unixTime,subTime);
	    executeCommand(gzipCommand);
	    sprintf(gzipFile,"/tmp/adu5ttt%ld_%ld.tar.gz",unixTime,subTime);		
	}
	else sprintf(gzipFile,"/tmp/adu5ttt%ld_%ld.tar",unixTime,subTime);
	if(useSecondDisk) {
	    sprintf(tempDir,"%s/%s",archivedSecondDisk,archivedGpsAdu5TTTDir);
	    copyFile(gzipFile,tempDir);
	}
	sprintf(tempDir,"%s/%s",archivedMainDisk,archivedGpsAdu5TTTDir);
	moveFile(gzipFile,tempDir);
	    
//	    exit(0);
	for(count=0;count<numLinks;count++) {
	    sprintf(currentFilename,"%s/%s",gpsdAdu5TTTArchiveDir,
		    linkList[count]->d_name);
	    sprintf(currentLinkname,"%s/%s",gpsdAdu5TTTArchiveLinkDir,
		    linkList[count]->d_name);
	    removeFile(currentLinkname);
	    removeFile(currentFilename);
	}
    }
	

    /* Free up the space used by dir queries */
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
	    
}


void checkAdu5PAT() 
{
    int count;
    /* Directory reading things */
    struct dirent **linkList;
    int numLinks;

    long unixTime;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    char tarCommand[FILENAME_MAX*MAX_FILES_PER_COMMAND];
    char gzipCommand[FILENAME_MAX];
    char gzipFile[FILENAME_MAX];
    char tempDir[FILENAME_MAX];

    numLinks=getListofLinks(gpsdAdu5PATArchiveLinkDir,&linkList);
    if(printToScreen) printf("Found %d links\n",numLinks);
    if(numLinks>gpsPATNumber) {
	//Do something
	sscanf(linkList[0]->d_name,
	       "gps_%ld.dat",&unixTime);
	sprintf(tarCommand,"cd %s ; tar -cf /tmp/adu5pat%ld.tar ",gpsdAdu5PATArchiveDir,unixTime);
	
	for(count=0;count<numLinks;count++) {
	    if((((count)%MAX_FILES_PER_COMMAND)==0) && count) {
//		    printf("%s\n",tarCommand);
		executeCommand(tarCommand);
		sprintf(tarCommand,"cd %s ; tar -rf /tmp/adu5pat%ld.tar ",gpsdAdu5PATArchiveDir,unixTime);
	    }	
	    strcat(tarCommand,linkList[count]->d_name);
	    strcat(tarCommand," ");
		
	}
	executeCommand(tarCommand);	   
	if(gzipGps) {
	    sprintf(gzipCommand,"gzip /tmp/adu5pat%ld.tar",unixTime);
	    executeCommand(gzipCommand);
	    sprintf(gzipFile,"/tmp/adu5pat%ld.tar.gz",unixTime);		
	}
	else sprintf(gzipFile,"/tmp/adu5pat%ld.tar",unixTime);

	if(useSecondDisk) {
	    sprintf(tempDir,"%s/%s",archivedSecondDisk,archivedGpsAdu5PATDir);
	    copyFile(gzipFile,tempDir);
	}
	sprintf(tempDir,"%s/%s",archivedMainDisk,archivedGpsAdu5PATDir);
	moveFile(gzipFile,tempDir);
//	    exit(0);
	for(count=0;count<numLinks;count++) {
	    sprintf(currentFilename,"%s/%s",gpsdAdu5PATArchiveDir,
		    linkList[count]->d_name);
	    sprintf(currentLinkname,"%s/%s",gpsdAdu5PATArchiveLinkDir,
		    linkList[count]->d_name);
	    removeFile(currentLinkname);
	    removeFile(currentFilename);
	}
    }
	
    /* Free up the space used by dir queries */
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
	    
}


void checkAdu5SAT() 
{
    int count;
    /* Directory reading things */
    struct dirent **linkList;
    int numLinks;

    long unixTime;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    char tarCommand[FILENAME_MAX*MAX_FILES_PER_COMMAND];
    char gzipCommand[FILENAME_MAX];
    char gzipFile[FILENAME_MAX];
    char tempDir[FILENAME_MAX];

    numLinks=getListofLinks(gpsdAdu5SATArchiveLinkDir,&linkList);
    if(printToScreen) printf("Found %d links\n",numLinks);
    if(numLinks>gpsSATNumber) {
	//Do something
	sscanf(linkList[0]->d_name,
	       "gps_%ld.dat",&unixTime);
	sprintf(tarCommand,"cd %s ; tar -cf /tmp/adu5sat%ld.tar ",gpsdAdu5SATArchiveDir,unixTime);
	
	for(count=0;count<numLinks;count++) {
	    if((((count)%MAX_FILES_PER_COMMAND)==0) && count) {
//		    printf("%s\n",tarCommand);
		executeCommand(tarCommand);
		sprintf(tarCommand,"cd %s ; tar -rf /tmp/adu5sat%ld.tar ",gpsdAdu5SATArchiveDir,unixTime);
	    }	
	    strcat(tarCommand,linkList[count]->d_name);
	    strcat(tarCommand," ");
		
	}
	executeCommand(tarCommand);	   
	if(gzipGps) {
	    sprintf(gzipCommand,"gzip /tmp/adu5sat%ld.tar",unixTime);
	    executeCommand(gzipCommand);
	    sprintf(gzipFile,"/tmp/adu5sat%ld.tar.gz",unixTime);		
	}
	else sprintf(gzipFile,"/tmp/adu5sat%ld.tar",unixTime);
	if(useSecondDisk) {
	    sprintf(tempDir,"%s/%s",archivedSecondDisk,archivedGpsAdu5SATDir);
	    copyFile(gzipFile,tempDir);
	}
	sprintf(tempDir,"%s/%s",archivedMainDisk,archivedGpsAdu5SATDir);
	moveFile(gzipFile,tempDir); 
//	    exit(0);
	for(count=0;count<numLinks;count++) {
	    sprintf(currentFilename,"%s/%s",gpsdAdu5SATArchiveDir,
		    linkList[count]->d_name);
	    sprintf(currentLinkname,"%s/%s",gpsdAdu5SATArchiveLinkDir,
		    linkList[count]->d_name);
	    removeFile(currentLinkname);
	    removeFile(currentFilename);
	}
    }
	
    /* Free up the space used by dir queries */
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
	    
}


void checkHk() 
{
    int count;
    /* Directory reading things */
    struct dirent **linkList;
    int numLinks;

    long unixTime;
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    char tarCommand[FILENAME_MAX*MAX_FILES_PER_COMMAND];
    char gzipCommand[FILENAME_MAX];
    char gzipFile[FILENAME_MAX];
    char tempDir[FILENAME_MAX];
//    char *testString

    numLinks=getListofLinks(hkdArchiveLinkDir,&linkList);
    if(printToScreen) printf("Found %d links\n",numLinks);
    if(numLinks>hkNumber) {
	//Do something
	if(strstr(linkList[0]->d_name,"raw"))
	   sscanf(linkList[0]->d_name,
		  "hk_%ld.raw.dat",&unixTime);
	if(strstr(linkList[0]->d_name,"cal"))
	   sscanf(linkList[0]->d_name,
		  "hk_%ld.raw.dat",&unixTime);
	if(strstr(linkList[0]->d_name,"avz"))
	   sscanf(linkList[0]->d_name,
		  "hk_%ld.raw.dat",&unixTime);
	sprintf(tarCommand,"cd %s ; tar -cf /tmp/hk%ld.tar ",hkdArchiveDir,unixTime);
	
	for(count=0;count<numLinks;count++) {
	    if((((count)%MAX_FILES_PER_COMMAND)==0) && count) {
//		    printf("%s\n",tarCommand);
		executeCommand(tarCommand);
		sprintf(tarCommand,"cd %s ; tar -rf /tmp/hk%ld.tar ",hkdArchiveDir,unixTime);
	    }	
	    strcat(tarCommand,linkList[count]->d_name);
	    strcat(tarCommand," ");
		
	}
	executeCommand(tarCommand);	   
	if(gzipGps) {
	    sprintf(gzipCommand,"gzip /tmp/hk%ld.tar",unixTime);
	    executeCommand(gzipCommand);
	    sprintf(gzipFile,"/tmp/hk%ld.tar.gz",unixTime);		
	}
	else sprintf(gzipFile,"/tmp/hk%ld.tar",unixTime);
	if(useSecondDisk) {
	    sprintf(tempDir,"%s/%s",archivedSecondDisk,archivedHkDir);
	    copyFile(gzipFile,tempDir);
	}
	sprintf(tempDir,"%s/%s",archivedMainDisk,archivedHkDir);
	moveFile(gzipFile,tempDir); 
//	    exit(0);
	for(count=0;count<numLinks;count++) {
	    sprintf(currentFilename,"%s/%s",hkdArchiveDir,
		    linkList[count]->d_name);
	    sprintf(currentLinkname,"%s/%s",hkdArchiveLinkDir,
		    linkList[count]->d_name);
	    removeFile(currentLinkname);
	    removeFile(currentFilename);
	}
    }
	
    /* Free up the space used by dir queries */
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
	    
}
