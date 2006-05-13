/*! \file CmdTest.c
    \brief Simple command line program that takes commands from command line
    arguments and places a CommandStruct_t object in the directory for Cmdd.
    
    Converts command line commands into CommandStruct_t objects for Cmdd

    July 2005  rjn@mps.ohio-state.edu
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
#include "anitaCommand.h"

/* Global variables */
int cmdLengths[256];
int numCmds=256;
char cmddCommandDir[FILENAME_MAX];
char cmddCommandLinkDir[FILENAME_MAX];
int writeCommandAndLink(CommandStruct_t *theCmd);
void readConfigFile();

int main(int argc, char **argv) {
    CommandStruct_t theCmd;
    char *progName=basename(argv[0]);
    int count;

    readConfigFile();
    theCmd.numCmdBytes=0;
    if(argc>1) {
       for(count=1;count<argc;count++) {
           theCmd.cmd[count-1]=(unsigned char)atoi(argv[count]);
           theCmd.numCmdBytes++;
       }
    }
    else {
	fprintf(stderr,"Usage:\n\t%s <cmd byte 1> .... <cmd byte N> (N<=20)\n",
		progName);
	exit(0);	
    }

    if(theCmd.numCmdBytes!=cmdLengths[(int)theCmd.cmd[0]]) {
	fprintf(stderr,"Command %d requires %d bytes\n",(int)theCmd.cmd[0],cmdLengths[(int)theCmd.cmd[0]]);
	exit(0);
    }
    return writeCommandAndLink(&theCmd);
}

void readConfigFile() {
    /* Config file thingies */
    int status=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    char *tempString;

    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status &= configLoad ("Cmdd.config","lengths") ;
    eString = configErrorString (status) ;
    if (status == CONFIG_E_OK) {
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
	if(kvpStatus!=KVP_E_OK) {
	    
	    fprintf(stderr,"Problem getting cmdLengths -- %s\n",
		    kvpErrorString(kvpStatus));
	}
	tempString=kvpGetString("cmddCommandDir");
	if(tempString) {
	    strncpy(cmddCommandDir,tempString,FILENAME_MAX);
	    sprintf(cmddCommandLinkDir,"%s/link",cmddCommandDir);
	    makeDirectories(cmddCommandLinkDir);       		
	}
	else {
	    fprintf(stderr,"Couldn't get cmddCommandDir\n");
	}
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }
}


int writeCommandAndLink(CommandStruct_t *theCmd) {
    time_t unixTime;
    time(&unixTime);
    int retVal=0;
    char filename[FILENAME_MAX];

    sprintf(filename,"%s/cmd_%ld.dat",cmddCommandDir,unixTime);
    {
	FILE *pFile;
	int fileTag=1;
	pFile = fopen(filename,"rb");
	while(pFile!=NULL) {
	    fclose(pFile);
	    sprintf(filename,"%s/cmd_%ld_%d.dat",cmddCommandDir,unixTime,fileTag);
	    pFile=fopen(filename,"rb");
	}
    }
    writeCmd(theCmd,filename);
    makeLink(filename,cmddCommandLinkDir);    
    return retVal;
}
