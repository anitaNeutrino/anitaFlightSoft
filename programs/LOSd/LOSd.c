/* LOSd - Program that talks to sip.
 *
 * Ryan Nichol, August '05
 * Started off as a modified version of Marty's driver program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>

/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "losLib/los.h"

int initDevice();
void tryToSendData();

/*Config Thingies*/
char losdPacketDir[FILENAME_MAX];
char losdPidFile[FILENAME_MAX];
int numPacketDirs=0;
int losBus;
int losSlot;
int verbosity;


/*Global Variables*/
#define BSIZE 2000 //Silly hack until I packet up Events
unsigned char theBuffer[BSIZE];

int main(int argc, char *argv[])
{
    int pk,retVal;
    char *tempString;
    char tempDir[FILENAME_MAX];

    /* Config file thingies */
    int status=0;
//    KvpErrorCode kvpStatus=0;
    char* eString ;
    
    char *progName=basename(argv[0]);
   
    /* Set signal handlers */
    signal(SIGINT, sigIntHandler);
    signal(SIGTERM,sigTermHandler);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("SIPd.config","global") ;
    status += configLoad ("SIPd.config","losd") ;
    eString = configErrorString (status) ;


    if (status == CONFIG_E_OK) {
/* 	printf("Here\n"); */
	numPacketDirs=kvpGetInt("numPacketDirs",9);
	losBus=kvpGetInt("losBus",1);
	losSlot=kvpGetInt("losSlot",11);
	verbosity=kvpGetInt("verbosity",0);
	tempString=kvpGetString("losdPidFile");
	if(tempString) {
	    strncpy(losdPidFile,tempString,FILENAME_MAX);
	    writePidFile(losdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get losdPidFile");
	    fprintf(stderr,"Couldn't get losdPidFile\n");
	}
	tempString=kvpGetString("losdPacketDir");
	if(tempString) {
	    strncpy(losdPacketDir,tempString,FILENAME_MAX);
	    for(pk=0;pk<numPacketDirs;pk++) {
		sprintf(tempDir,"%s/pk%d/link",losdPacketDir,pk);
		makeDirectories(tempDir);
	    }
		
	}
	else {
	    syslog(LOG_ERR,"Couldn't get losdPacketDir");
	    fprintf(stderr,"Couldn't get losdPacketDir\n");
	}
	//printf("%d\t%s\n",kvpStatus,kvpErrorString(kvpStatus));
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }

    retVal=initDevice();
    if(retVal!=0) {
	return 1;
    }
    do {
	if(verbosity) printf("Initializing LOSd\n");
	currentState=PROG_STATE_RUN;
        while(currentState==PROG_STATE_RUN) {
	    tryToSendData();
	    usleep(1000);
	}
    } while(currentState==PROG_STATE_INIT);

//    fprintf(stderr, "Bye bye\n");
    return 0;
}


void tryToSendData()
{
//    long amtb;


//    long bufno = 1;
    int pk;
//    int bytes_avail;
    int retVal,count;
    int numLinks=0;
    long fileSize=0;
    char linkDir[FILENAME_MAX];
    char currentFilename[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int numItems=0;
    FILE *fp;
    struct dirent **linkList;



    for(pk=0;pk<numPacketDirs;pk++) {
	sprintf(linkDir,"%s/pk%d/link",losdPacketDir,pk);
	numLinks=getListofLinks(linkDir,&linkList);
	if(numLinks) break;
    }
    if(numLinks==0) return;
    //Need to put something here so that it doesn't dick around 
    //forever in pk4, when there is data waiting in pk1, etc.
    
    for(count=0;count<numLinks;count++) {

	sprintf(currentFilename,"%s/pk%d/%s",
		losdPacketDir,pk,linkList[count]->d_name);
	sprintf(currentLinkname,"%s/pk%d/link/%s",
		losdPacketDir,pk,linkList[count]->d_name);
	fp = fopen(currentFilename,"rb");
	if(fp == NULL) {
	    syslog(LOG_ERR,"Error opening file, will delete: %s",currentFilename);
	    fprintf(stderr,"Error opening file, will delete: %s\n",currentFilename);
	    
	    removeFile(currentFilename);
	    removeFile(currentLinkname);
	    //Mayhaps we should delete
	    continue;
	}


	
	// Obtain file size
	fseek(fp,0,SEEK_END);
	fileSize=ftell(fp);
	rewind(fp);
	
	if(verbosity) { 
	    printf("Opened file: %s size %ld\n",currentFilename,fileSize);
	}
	
	numItems = fread(theBuffer,1,fileSize,fp);
	if(numItems!=fileSize) {
	    syslog(LOG_ERR,"Error reading file: %s",currentFilename);
	    fprintf(stderr,"Error reading file: %s\n%s\n",currentFilename,strerror(errno));
	    removeFile(currentFilename);
	    removeFile(currentLinkname);
	    return;
	}	    
	/*** the whole file is loaded in the buffer. ***/	    
	fclose (fp);
	retVal = los_write((unsigned char *)theBuffer, fileSize);
	if (retVal != 0) {
	    syslog(LOG_ERR,"Couldn't send file: %s",currentFilename);
	    fprintf(stderr, "Couldn't telemeterize: %s\n\t%s\n",currentFilename,los_strerror());
	    break;
	}
	else {
	    removeFile(currentFilename);
	    removeFile(currentLinkname);
	}    
	

	if(count>10) break;
	
    }
    
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
    

}


int initDevice() {
    int retVal=los_init((unsigned char)losBus,(unsigned char)losSlot,0,1,-1);
    if(retVal!=0) {
	syslog(LOG_ERR,"Problem opening LOS board: %s",los_strerror());	
	fprintf(stderr,"Problem opening LOS board: %s\n",los_strerror()); 
    }
    else if(verbosity) {
	printf("Succesfully opened board: Bus %d, Slot %d\n",losBus,losSlot);
    }


    return retVal;
}
