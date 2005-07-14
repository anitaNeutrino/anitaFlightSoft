/*! \file Cmdd.c
    \brief The Cmdd program that receives commands from SIPd 
           and acts upon them 
    
    Receives command bytes from SIPd and then carries out the relevant action.

    July 2005  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
#include "anitaFlight.h"

int executeCommand();

/* Global variables */
int cmdLengths[256];

int cmdBytes[20];
int numCmdBytes=0;

int main (int argc, char *argv[])
{
//    int retVal;
    int numCmds=256,count;

    /* Config file thingies */
    int status=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    
    /* Log stuff */
    char *progName=basename(argv[0]);

     /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status &= configLoad ("Cmdd.config","lengths") ;
    eString = configErrorString (status) ;
    
    if (status == CONFIG_E_OK) {
/* 	printf("Here\n"); */
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
	//printf("%d\t%s\n",kvpStatus,kvpErrorString(kvpStatus));
    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
    }

/*     for(count=0;count<numCmds;count++) { */
/* 	if(cmdLengths[count]) { */
/* 	    printf("%d\t%d\n",count,cmdLengths[count]); */
/* 	} */
/*     } */
    
    /* For now I'm just going to write this as a command line program, 
       at some point in the future it will metamorphosis into a proper
       daemon that either listens on a socket or looks in a directory for 
       commands. */
    if(argc>1) {
	for(count=1;count<argc;count++) {
	    cmdBytes[count-1]=atoi(argv[count]);
	    numCmdBytes++;
	}
    }
    else {
	fprintf(stderr,"Usage:\n\t%s <cmd byte 1> .... <cmd byte N> (N<=20)\n",
		progName);
	exit(0);
    }
//    printf("%d %d %d\n",cmdBytes[0],numCmdBytes,cmdLengths[cmdBytes[0]]);

    if(cmdLengths[cmdBytes[0]]!=numCmdBytes) {
	syslog(LOG_ERR,"Wrong number of command bytes for %d: expected %d, got %d\n",cmdBytes[0],cmdLengths[cmdBytes[0]],numCmdBytes);	
	fprintf(stderr,"Wrong number of command bytes for %d: expected %d, got %d\n",cmdBytes[0],cmdLengths[cmdBytes[0]],numCmdBytes);
	exit(0);
    }
    return executeCommand();
}

int executeCommand() 
{
    modifyConfigInt("GPSd.config","output","printToScreen",0);
    return 0;
}


