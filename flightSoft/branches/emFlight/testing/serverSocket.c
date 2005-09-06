
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "anitaFlight.h"
#include "socketLib/socketLib.h"
     
#define PORT    5555
     

int main(int argc, char *argv[])
{
    char *progName=basename(argv[0]);
    int numWithData,numServs,readRetVal;
    char tempBuf[MAX_LENGTH];
    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    numServs=prepServerWithNames(PORT,"franz","ferdinand");
    while (1) {
	if(checkForNewConns(PORT)==1) break;
	sleep(1);
    }
/*     printf("Here %d\n",numServs); */
    sleep(1);
    while(1) {
	numWithData=checkOpenConnections();	
/* 	printf("Num with data %d\n",numWithData); */
	if(numWithData) {
	    printf("Here you fool\n");
	    readRetVal=readDataFromPort(PORT,tempBuf);
	    if(readRetVal>0) {
		printf("Here you silly silly bastard, you got %d bytes\n",
		       readRetVal);
/* 		tempBuf[readRetVal]='\0'; */
		printf("Read %d bytes from port, message was %s.\n",
		       readRetVal,tempBuf);
	    }
	    else if(readRetVal<0) {
		printf("Got error %d from reading port.\n",readRetVal);
		if(readRetVal==SOCKET_E_EOF) {
		    printf("Port closed\n");
		    break;
		}
	    }
	    if(readRetVal==0) {
		printf("No data on port.\n");
	    }
	}
	sleep(1);
    }
    printf("Num with data %d\n",numWithData);


}


/* int main (void) */
/* { */
/*     int theSock=makeServerSocket(PORT); */
/*     printf("The socket on port %d with number %d.\n",PORT,theSock); */
    
/*     int newFileDes; */
/*     while (1) { */
/* 	newFileDes= */
/* 	    checkForNewClientAndShake(theSock,"franz","ferdinand"); */
/* 	if(newFileDes) break; */
/* 	sleep(1); */
/*     } */
    
/*     char tempBuf[MAX_LENGTH]; */
/*     while(1) { */
/* 	int retVal=checkAndReadSocketNow(newFileDes,tempBuf); */
/* 	/\*printf("Here %d\t%d \n",newFileDes,retVal);*\/ */
/* 	if(retVal==1) { */
/* 	    printf("Got data: %s",tempBuf); */
/* 	} */
/* 	else if(retVal==-1) { */
/* 	    printf("Socket closed... trying to establish new connection\n"); */
/* 	    while (1) { */
/* 		newFileDes= */
/* 		    checkForNewClientAndShake(theSock,"franz","ferdinand"); */
/* 		if(newFileDes) break; */
/* 		sleep(1); */
/* 	    } */
/* 	} */
/* 	sleep(1); /\*Infinite loop for now *\/ */
/*     } */
/*     close(newFileDes); */
/*     close(theSock); */
/* } */


