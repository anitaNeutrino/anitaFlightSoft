
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "anitaFlight.h"
#include "socketLib/socketLib.h"

#define PORT    5555


int main (int argc, char *argv[])
{   
    char *progName=basename(argv[0]);
    char *fred="fred";
    int numClients,retVal;
    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

    numClients=prepClientWithNames(PORT,"ferdinand","franz");
    while (1) {
	retVal=tryToConnectToServer(PORT);
	printf("Got retVal of %d\n",retVal);
	if(retVal==1) break;
	sleep(1);
    }
    printf("Here %d\n",numClients);
    printf("String length of %s is %d.\n",fred,strlen(fred));
    retVal=sendDataOnPort(PORT,fred,5);
    printf("Ret Val for sendDataOnPort: %d\n",retVal);
    sleep(10);
}



/* int main (void) */
/* { */
/*   char myMessage[] = "You old dirty bastard"; */

/*   int theSock = makeClientSocket (PORT); */
/*   if(!theSock) { */
/*       /\* Server probably isn't running at the moment *\/ */
/*       while(1) { */
/* 	  theSock = makeClientSocket (PORT); */
/* 	  if(theSock) break; */
/* 	  sleep(1); */
/*       } */
/*   } */

/*   printf ("A socket on port %d with number %d.\n", PORT, theSock); */
/*   if (clientSideHandshake (theSock, "ferdinand", "franz"))  */
/*       printf ("Handshake accepted\n");  */
								   
  
/*   char tempBuf[MAX_LENGTH]; */
/*   while (1) { */
/*       int retVal=checkAndReadSocketNow(theSock,tempBuf); */
/*       if(retVal==1) { */
/* 	  printf("Got data: %s",tempBuf); */
/*       } */
/*       else if(retVal==-1) { */
/* 	  printf("Socket closed... trying to establish new connection\n"); */
/* 	  while(1) { */
/* 	      theSock = makeClientSocket (PORT); */
/* 	      if(theSock) { */
/* 		  printf ("A socket on port %d with number %d.\n",  */
/* 			  PORT, theSock); */
/* 		  if (clientSideHandshake (theSock, "ferdinand", "franz")) { */
/* 		      printf ("Handshake accepted\n"); */
/* 		      break; */
/* 		  } */
/* 	      } */
/* 	      sleep(1); */
/* 	  } */
		  
/*       } */
	 
/*       sleep(1); */
/*   }   */

/*   close (theSock); */
/* } */
