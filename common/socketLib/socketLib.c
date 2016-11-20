/*! \file socketLib.c
    \brief Prototype socket library wrapper for the GNU socket library.
    
    Just a few functions to ease the use of the GNU socket library stuff.
    Hopefully this library will provide the mechanism for all 
    inter-process communication in the ANITA flight software.
    July 2004  rjn@mps.ohio-state.edu
*/

/*
 * Current CVS Tag:
 * $Header: /work1/anitaCVS/flightSoft/common/socketLib/socketLib.c,v 1.5 2005/06/15 16:22:00 rjn Exp $
 */
 
/* 
 * Modification History : DO NOT EDIT - MAINTAINED BY CVS 
 * $Log: socketLib.c,v $
 * Revision 1.5  2005/06/15 16:22:00  rjn
 * Fixed a lot of silly warnings
 *
 * Revision 1.4  2004/10/08 20:08:41  rjn
 * Updated a lot of things. fakeAcqd.c is coming along, and now communicates with Eventd through the file system (the filesystem in question will be a ramdisk on the flight computer)
 *
 * Revision 1.3  2004/09/08 18:02:53  rjn
 * More playing with socketLib. And a few changes as --pedantic is now one of the compiler options.
 *
 * Revision 1.2  2004/09/07 13:52:21  rjn
 * Added dignified multiple port handling capabilities to socketLib. Not fully functional... but getting there.
 *
 * Revision 1.1  2004/09/01 14:49:17  rjn
 * Renamed mySocket to socketLib
 *
 *
 */ 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "socketLib/socketLib.h"

/* /\* locals *\/ */
/* static int numClients=0; */
/* static int numServers=0; */

/* static char serverPrognames[MAX_SOCKETS][MAX_LENGTH]; */
/* static char clientPrognames[MAX_SOCKETS][MAX_LENGTH]; */

/* static char serverAllowedNames[MAX_SOCKETS][MAX_LENGTH]; */
/* static char clientAllowedNames[MAX_SOCKETS][MAX_LENGTH]; */

/* static uint16_t clientPortNums[MAX_SOCKETS]; */
/* static uint16_t serverPortNums[MAX_SOCKETS]; */

/* static int serverSockNums[MAX_SOCKETS]; */

/* static int clientFileDes[MAX_SOCKETS]; */
/* static int serverFileDes[MAX_SOCKETS]; */

/* static int serverShake[MAX_SOCKETS]; */
/* static int clientShake[MAX_SOCKETS]; */

/* static int serversWithData[MAX_SOCKETS]; */
/* static int clientsWithData[MAX_SOCKETS]; */

/* static int serverErrors[MAX_SOCKETS]; */
/* static int clientErrors[MAX_SOCKETS]; */




/* static int getServerIndex(uint16_t thePort) */
/* /\*!  */
/*   Just looks if thePort is in the list of prepped/open server ports */

/* *\/ */
/* { */
/* /\*    printf("Here\n");*\/ */
/*     int i; */
/*     for(i=0;i<numServers;i++) { */
/* /\*	syslog(LOG_INFO,"%d\t%d\t%d",i,serverPortNums[i],thePort);*\/ */
/* 	if(serverPortNums[i]==thePort)         */
/* 	    return i; */
/*     } */
/*     return SOCKET_E_NOTPREPPED; */
/* } */

/* static int getClientIndex(uint16_t thePort) */
/* /\*!  */
/*   Just looks if thePort is in the list of open client ports */
/* *\/ */
/* { */
/*     int i; */
/*     for(i=0;i<numClients;i++) */
/* 	if(clientPortNums[i]==thePort) */
/* 	    return i; */
/*     return SOCKET_E_NOTPREPPED; */
/* } */

/* static int serverSideShake(int servIndex) */
/* /\*!  */
/*   Tries to do the silly handshaking using the already stored names. */
/* *\/ */
/* {     */
/*     return serverSideHandshake(serverFileDes[servIndex], */
/* 			       serverPrognames[servIndex], */
/* 			       serverAllowedNames[servIndex]); */
/* } */
    
/* static int clientSideShake(int clientIndex) */
/* /\*!  */
/*   Tries to do the silly handshaking using the already stored names. */
/* *\/ */
/* {     */
/*     return  clientSideHandshake(clientFileDes[clientIndex], */
/* 				clientPrognames[clientIndex], */
/* 				clientAllowedNames[clientIndex]); */
/* }  */

   

/* int prepServer(uint16_t thePort) */
/* /\*!  */
/*   Essentially it just calls makeServerSocket and keeps track of the resulting */
/*   socket .Returns the number of server ports prepped. */
/* *\/ */
/* { */
/*     int servIndex,tempSock; */

/*     if(numServers>=MAX_SOCKETS) { */
/* 	syslog (LOG_WARNING, "Too many sockets can not service port %d", thePort); */
/* 	return SOCKET_E_MAXSOCKETS; */
/*     } */
/*     /\* Have room to make socket *\/ */

/*     if(getServerIndex(thePort)!=SOCKET_E_NOTPREPPED) { */
/* 	syslog (LOG_WARNING, "Port already prepped %d", thePort); */
/* 	return SOCKET_E_DUPLICATEPORT; */
/*     }     */
/*     /\* Haven't already made this socket *\/ */

/*     servIndex=numServers; */
/*     tempSock=makeServerSocket(thePort); */
/*     if(!tempSock) { */
/* 	syslog (LOG_WARNING, "Couldn't create socket on port %d", thePort); */
/* 	return SOCKET_E_CREATE; */
/*     } */
/*     /\* Actually succeeded in making the socket.*\/ */
    
/*     serverPortNums[servIndex]=thePort; */
/*     serverSockNums[servIndex]=tempSock; */
/*     serverFileDes[servIndex]=0; /\*Just in case I check it at some later time.*\/ */
/*     serverErrors[servIndex]=0; */
/*     serverShake[servIndex]=0; */
/*     numServers++; */
/*     return numServers;	     */
/* } */

/* int prepServerWithNames(uint16_t thePort,char *myName,char *hisName) */
/* /\*!  */
/*   Essentially it just calls makeServerSocket and keeps track of the resulting */
/*   socket. Also stores the names used for handshaking */
/* *\/ */
/* { */
/*     int retVal,servIndex; */
/*     retVal=prepServer(thePort); */
/*     if(retVal>0) { */
/* 	/\* Socket is prepped *\/ */
/* 	servIndex=retVal-1; */
/* 	strcpy(serverPrognames[servIndex],myName); */
/* 	strcpy(serverAllowedNames[servIndex],hisName); */
/* 	serverShake[servIndex]=1; */
/*     } */
/*     return retVal;	     */
/* } */
	    

/* int checkForNewConns(uint16_t thePort) */
/* /\*!  */
/*   Checks the socket identified by thePort to see if any clients are  */
/*   trying to connect. Returns 1 if there is a new connection, 0 otherwise. */
/* *\/ */
/* { */
/*     int servIndex,newFileDes=0; */
/*     servIndex=getServerIndex(thePort); */
/*     if(servIndex==SOCKET_E_NOTPREPPED) { */
/* 	syslog (LOG_WARNING, "No socket prepped for port %d %d\n", thePort, */
/* 		servIndex); */
/* 	return SOCKET_E_NOTPREPPED; */
/*     } */
/*     /\*Have already prepped this guy, so can check for new connections.*\/ */

/*     if(serverFileDes[servIndex]) { */
/* 	syslog (LOG_WARNING, "Already have a connection for port %d", thePort); */
/* 	return SOCKET_E_DUPLICATESOCK; */
/*     }	 */

/*     newFileDes=checkForNewClientConnection(serverSockNums[servIndex]); */
/*     if(newFileDes) { */
/* 	/\* Got a new connection *\/ */
/* 	serverFileDes[servIndex]=newFileDes; */
/* 	syslog(LOG_DEBUG,"Got a new connection %d, index is %d, serverShake is %d", */
/* 	       newFileDes,servIndex,serverShake[servIndex]); */
/* 	if(serverShake[servIndex]) { */
/* 	    /\* Need to perform handshake *\/ */
/* 	    if(!serverSideShake(servIndex)) { */
/* 		serverFileDes[servIndex]=0; */
/* 		return SOCKET_E_BADSHAKE; */
/* 	    } */
/* 	} */
/* 	return 1; */
/*     } */
    
/*     /\*No connection at the moment *\/ */
/*     return 0;             */
/* } */

/* int prepClient(uint16_t thePort) */
/* /\*!  */
/*   Doesn't really do very much, just adds thePort to list of ports we want */
/*   to connect to. Returns the number of client ports stored. */
/* *\/ */
/* { */
/*     int clientIndex; */
/*     if(numClients>=MAX_SOCKETS) { */
/* 	syslog (LOG_WARNING,  */
/* 		"Too many client sockets can not try port %d", thePort); */
/* 	return SOCKET_E_MAXSOCKETS; */
/*     } */
/*     /\* Have room to make socket *\/ */
    
/*     if(getClientIndex(thePort)!=SOCKET_E_NOTPREPPED) { */
/* 	syslog (LOG_WARNING, "Already connected to port %d", thePort); */
/* 	return SOCKET_E_DUPLICATEPORT; */
/*     }     */
/*     /\* It's not already there *\/   */
    
/*     clientIndex=numClients;  */
/*     clientPortNums[clientIndex]=thePort; */
/*     clientFileDes[clientIndex]=0;  */
/*     /\* Just in case I check it at some later time. *\/ */
/*     clientShake[clientIndex]=0; */
/*     clientErrors[clientIndex]=0; */
/*     numClients++; */
/*     return numClients;     */
/* } */


/* int prepClientWithNames(uint16_t thePort,char *myName,char *hisName) */
/* /\*!  */
/*   Doesn't really do very much, just adds thePort to list of ports we want */
/*   to connect to. Returns the number of client ports stored. Also stores the */
/*   names used for silly handshaking. */
/* *\/ */
/* { */
/*     int retVal,clientIndex; */
/*     retVal=prepClient(thePort); */
/*     if(retVal>0) { */
/* 	/\* Socket is prepped *\/ */
/* 	clientIndex=retVal-1; */
/* 	strcpy(clientPrognames[clientIndex],myName); */
/* 	strcpy(clientAllowedNames[clientIndex],hisName); */
/* 	clientShake[clientIndex]=1; */
/*     } */
/*     return retVal;	    */
/* } */

/* int tryToConnectToServer(uint16_t thePort) */
/* /\*!  */
/*   Tries to connect to a server on port <i>thePort</i>, calls prepClient if  */
/*   it hasn't already been prepped. Returns 1 if socket is succesfully connected. */
/* *\/ */
/* { */
/*     int clientIndex,retVal,otherRetVal; */
/*     clientIndex=getClientIndex(thePort); */
/*     if(clientIndex==SOCKET_E_NOTPREPPED) { */
/* 	retVal=prepClient(thePort); */
/* 	if(retVal<=0) /\* Some Failure *\/ */
/* 	    return retVal; */
/* 	clientIndex=retVal-1; */
/*     } */
/*     if(clientFileDes[clientIndex]) { */
/* 	/\* Already connected to this port. *\/ */
/* 	syslog (LOG_WARNING, "Already have a connection for port %d", thePort); */
/* 	return SOCKET_E_DUPLICATESOCK; */
/*     } */
    
/*     /\* Now clientIndex actually corresponds to the correct port *\/ */
/*     retVal=makeClientSocket(thePort); */
/*     syslog(LOG_DEBUG,"The retVal of makeClientSocket is %d",retVal); */
/*     if(retVal>0) {	 */
/* 	clientFileDes[clientIndex]=retVal; */
/* 	if(clientShake[clientIndex]) { */
/* 	    /\* Need to perform handshake *\/ */
/* 	    otherRetVal=clientSideShake(clientIndex); */
/* 	    syslog(LOG_DEBUG,"Got retVal %d from shaking",otherRetVal); */
/* 	    if(otherRetVal!=1) { */
/* 		syslog(LOG_ERR,"Problem shaking got retVal %d",otherRetVal); */
/* 		clientFileDes[clientIndex]=0; */
/* 		return SOCKET_E_BADSHAKE; */
/* 	    } */
/* 	} */
/* 	syslog(LOG_DEBUG,"Going to return 1 from tryToConnectToServer"); */
/* 	return 1; */
/*     } */
/*     return retVal; */
/* } */


/* int sendDataOnPort(uint16_t thePort, void *theBuffer, int length) */
/* /\*!  */
/*   Tries to send data to the socket associated with port thePort. Return value */
/*   is the number of bytes sent if successful. */
/* *\/  */
/* { */
/*     int clientRetVal,serverRetVal; */
/*     clientRetVal=getClientIndex(thePort); */
/*     serverRetVal=getServerIndex(thePort); */
/*     if(clientRetVal==serverRetVal) { */
/* 	/\* Bugger, something wrong *\/ */
/* 	if(clientRetVal==SOCKET_E_NOTPREPPED) { */
/* 	    syslog (LOG_WARNING, "No sockets associated with %d", thePort); */
/* 	    return SOCKET_E_NOTPREPPED; */
/* 	} */
/* 	else {	 */
/* 	    /\* Got to work out how to throw exceptions and  */
/* 	       that sort of nonsense. *\/ */
/* 	    syslog (LOG_ERR, "Both client and server assocaited with port %d", thePort);  */
/* 	    return SOCKET_E_BOTHCLIENTANDSERVER;     */
/* 	} */
/*     } */
/* /\*    syslog(LOG_INFO,"The retVals %d\t%d",clientRetVal,serverRetVal);*\/ */
/*     if(clientRetVal>=0) { */
	
/* 	return sendDataToClientSocket(clientRetVal,theBuffer,length); */
/*     } */
/*     else if(serverRetVal>=0) { */
/* 	return sendDataToServerSocket(serverRetVal,theBuffer,length);	 */
/*     } */
/*     /\* Can't possibly get here. *\/ */
/*     syslog(LOG_ERR, */
/* 	   "Got to place that I shouldn't have got to in sendDataOnPort"); */
/*     return SOCKET_E_IMPOSSIBLE; */

/* } */


/* static int sendDataToServerSocket(int theIndex, void *theBuffer, int length) */
/* /\*!  */
/*   Tries to send data to the socket with theIndex. */
/* *\/  */
/* { */
/*     int retVal; */
/*     if(!serverFileDes[theIndex]) { */
/* 	syslog (LOG_WARNING, "Nothing connected on port %d",  */
/* 		serverPortNums[theIndex] ); */
/* 	return SOCKET_E_NOCONNECTION; */
/*     } */
/*     if(!isSocketReadyForSendingNow(serverFileDes[theIndex])) { */
/* 	syslog (LOG_WARNING, "Cannot send data to socket on port %d",  */
/* 		serverPortNums[theIndex] ); */
/* 	return SOCKET_E_SENDNOTREADY; */
/*     } */
    
/*     retVal=sendToSocket(serverFileDes[theIndex],theBuffer,length,0); */
/*     if(retVal==SOCKET_E_SENDFAILURE) { */
/* 	syslog(LOG_ERR,"Couldn't send data to socket %d on port %d", */
/* 	       serverFileDes[theIndex],serverPortNums[theIndex]); */
/* 	return SOCKET_E_SENDFAILURE; */
/*     } */
/*     /\* Success *\/ */
/*     return retVal;        		 */
/* } */


/* static int sendDataToClientSocket(int theIndex, void *theBuffer, int length) */
/* /\*!  */
/*   Tries to send data to the socket with theIndex. */
/* *\/  */
/* { */
/*     int retVal; */
/*     if(!clientFileDes[theIndex]) { */
/* 	syslog (LOG_WARNING, "Nothing connected on port %d",  */
/* 		clientPortNums[theIndex] ); */
/* 	return SOCKET_E_NOCONNECTION; */
/*     } */
/*     if(!isSocketReadyForSendingNow(clientFileDes[theIndex])) { */
/* 	syslog (LOG_WARNING, "Cannot send data to socket on port %d",  */
/* 		clientPortNums[theIndex] ); */
/* 	return SOCKET_E_SENDNOTREADY; */
/*     } */
    
/*     retVal=sendToSocket(clientFileDes[theIndex],theBuffer,length,0); */
/*     if(retVal==SOCKET_E_SENDFAILURE) { */
/* 	syslog(LOG_ERR,"Couldn't send data to socket %d on port %d", */
/* 	       clientFileDes[theIndex],clientPortNums[theIndex]); */
/* 	return SOCKET_E_SENDFAILURE; */
/*     } */
/*     /\* Success *\/ */
/*     return retVal;        		 */
/* } */

/* int readDataFromPort(uint16_t thePort,void *theBuffer) */
/* /\*!  */
/*   Tries to read data from socket associated with thePort into theBuffer. If  */
/*   successful returns number of bytes read, if there's no data it returns 0. */
/*   (All error codes are negative) */
/* *\/  */
/* { */
/*     int clientRetVal,serverRetVal; */
/*     clientRetVal=getClientIndex(thePort); */
/*     serverRetVal=getServerIndex(thePort); */
/*     if(clientRetVal==serverRetVal) { */
/* 	/\* Bugger, something wrong *\/ */
/* 	if(clientRetVal==SOCKET_E_NOTPREPPED) { */
/* 	    syslog (LOG_WARNING, "No sockets associated with %d", thePort); */
/* 	    return SOCKET_E_NOTPREPPED; */
/* 	} */
/* 	else {	 */
/* 	    /\* Got to work out how to throw exceptions and  */
/* 	       that sort of nonsense. *\/ */
/* 	    syslog (LOG_ERR, "Both client and server assocaited with port %d", thePort);  */
/* 	    return SOCKET_E_BOTHCLIENTANDSERVER;     */
/* 	} */
/*     } */
/*     syslog(LOG_DEBUG,"The retVals are %d & %d",clientRetVal,serverRetVal); */
/*     if(clientRetVal>=0) { */
/* 	if(!clientsWithData[clientRetVal]) { */
/* 	    /\* Check to see if any new data has arrived since last check. *\/ */
/* 	    checkOpenConnections(); */
/* 	} */
/* 	if(clientsWithData[clientRetVal]) { */
/* 	    /\* Data waiting to be read. *\/ */
/* 	    return readDataFromClientSocket(clientRetVal,theBuffer); */
/* 	} */
/* 	return 0; */
/*     } */
/*     else if(serverRetVal>=0) { */
/* 	syslog(LOG_DEBUG,"You're asking for server data with index %d", */
/* 	       serverRetVal); */
/* 	if(!serversWithData[serverRetVal]) { */
/* 	    /\* Check to see if any new data has arrived since last check. *\/ */
/* 	    checkOpenConnections(); */
/* 	} */
/* 	if(serversWithData[serverRetVal]) { */
/* 	    /\* Data waiting to be read. *\/ */
/* 	    syslog(LOG_DEBUG,"And apparently he's got some data."); */
	    
/* 	    return readDataFromServerSocket(serverRetVal,theBuffer); */
/* 	} */
/* 	return 0; */
/*     } */
/*     /\* Can't possibly get here. *\/ */
/*     syslog(LOG_ERR, */
/* 	   "Got to place that I shouldn't have got to in readDataFromPort."); */
/*     return SOCKET_E_IMPOSSIBLE; */
/* } */

/* static int readDataFromServerSocket(int theIndex, void *theBuffer) */
/* /\*!  */
/*   Tries to read data from socket associated with theIndex into theBuffer. */
/*   If this has been called then serversWithData[theIndex] has been set. So we  */
/*   know the port is valid and has a socket attached. If successful returns  */
/*   number of bytes read; */
/* *\/  */
/* {     */
/*     int retVal;     */

/*     syslog(LOG_DEBUG,"Trying to read data from socket %d into buffer at %d", */
/* 	   serverFileDes[theIndex],(int)theBuffer); */
/*     retVal=recvFromSocket(serverFileDes[theIndex],theBuffer,0); */
/*     if(retVal==SOCKET_E_RECVFAILURE) {	 */
/* 	syslog(LOG_ERR,"Couldn't read data from socket %d on port %d", */
/* 	       serverFileDes[theIndex],serverPortNums[theIndex]); */
/* 	return SOCKET_E_RECVFAILURE; */
/*     } */
/*     else if(retVal==SOCKET_E_EOF) { */
/* 	syslog(LOG_ERR,"Socket %d closed on port %d", */
/* 	       serverFileDes[theIndex],serverPortNums[theIndex]); */
/* 	return SOCKET_E_EOF; */
	
/*     } */
/*     /\* Success *\/ */
/*     return retVal;    */
/* }    */

/* static int readDataFromClientSocket(int theIndex, void *theBuffer) */
/* /\*!  */
/*   Tries to read data from socket associated with theIndex into theBuffer. */
/*   If this has been called then clientsWithData[theIndex] has been set. So we  */
/*   know the port is valid and has a socket attached. If successful returns  */
/*   number of bytes read; */
/* *\/  */
/* { */
/*     int retVal;     */
/*     retVal=recvFromSocket(clientFileDes[theIndex],theBuffer,0); */
/*     if(retVal==SOCKET_E_RECVFAILURE) { */
/* 	syslog(LOG_ERR,"Couldn't read data from socket %d on port %d", */
/* 	       clientFileDes[theIndex],clientPortNums[theIndex]); */
/* 	return SOCKET_E_RECVFAILURE; */
/*     } */
/*     else if(retVal==SOCKET_E_EOF) { */
/* 	syslog(LOG_ERR,"Socket %d closed on port %d", */
/* 	       clientFileDes[theIndex],clientPortNums[theIndex]); */
/* 	return SOCKET_E_EOF; */
	
/*     } */
/*     /\* Success *\/ */
/*     return retVal;    */
/* }    */



/* int checkOpenConnections() */
/* /\*! */
/*   This function polls connected sockets to see if they need servicing.  */
/* *\/ */
/* { */
/*     fd_set active_fd_set, read_fd_set; */
/*     struct timeval waitTime; */
/*     int numReady,numWithData,i; */

/*     /\* At the moment it just polls to see if there are any connections */
/*        waiting to be serviced. *\/ */
/*     waitTime.tv_sec=0; */
/*     waitTime.tv_usec=0; */
   
/*     /\* Initialize the set of active sockets. *\/ */
/*     FD_ZERO (&active_fd_set); */
/*     for(i=0;i<numClients;i++) { */
/* 	if(clientFileDes[i]) */
/* 	    FD_SET (clientFileDes[i], &active_fd_set); */
/*     } */
/*     for(i=0;i<numServers;i++) { */
/* 	if(serverFileDes[i]) */
/* 	    FD_SET (serverFileDes[i], &active_fd_set); */
/*     } */
/*     read_fd_set = active_fd_set; */
/*     numReady=select (FD_SETSIZE, &read_fd_set, NULL, NULL, &waitTime); */
/*     if ( numReady< 0) */
/*     { */
/* 	/\* Got to do something here, all these exit calls must disappear. */
/* 	   Might have to do something more clevererererer, than just returning */
/* 	   a value. *\/ */
/* 	syslog(LOG_ERR, "select: %s",strerror(errno)); */
/* 	return SOCKET_E_SELECT; */
/*     } */
/*     numWithData=0; */
/*     if(numReady) {	 */
/* 	for(i=0;i<numClients;i++) { */
/* 	    if(clientFileDes[i])  */
/* 		if(FD_ISSET(clientFileDes[i],&read_fd_set)) { */
/* 		    clientsWithData[i]=1; */
/* 		    numWithData++; */
/* 		}  */
/* 		else clientsWithData[i]=0; */
/* 	    else clientsWithData[i]=0;	    	     */
/* 	}	 */
/* 	for(i=0;i<numServers;i++) { */
/* 	    if(serverFileDes[i])  */
/* 		if(FD_ISSET(serverFileDes[i],&read_fd_set)) { */
/* 		    serversWithData[i]=1; */
/* 		    numWithData++; */
/* 		} */
/* 		else serversWithData[i]=0; */
/* 	    else serversWithData[i]=0;	    	     */
/* 	} */
		
/*     } */
/*     return numWithData;    */
/* } */

/* int tryToOpenAllClientConnections() */
/* /\*! */
/*   This function trys to open all the prepped client connections. */
/* *\/ */
/* {    */

/*     int numNotConnected=0,numConnected=0,i,retVal,currentIndex; */
/*     int theUnconnected[MAX_SOCKETS]; */
    
/*     int numErrors=0; */
/*     //    int numReady,numShaking,numShook; */

     
/*     if(numClients) {     */
/* 	numNotConnected=0; */
/* 	numConnected=0; */
/* 	for(i=0;i<numClients;i++) { */
/* 	    if(!clientFileDes[i]) { */
/* 		theUnconnected[numNotConnected]=i; */
/* 		numNotConnected++; */
/* 	    } */
/* 	    else numConnected++; */
/* 	} */
	
/* 	if(numNotConnected) { */
/* 	    for(i=0;i<numNotConnected;i++) { */
/* 		currentIndex=theUnconnected[i]; */
/* 		retVal=makeClientSocket(clientPortNums[currentIndex]); */
/* 		if(retVal>0) { */
/* 		    clientFileDes[currentIndex]=retVal; */
/* 		    /\* What to do about the handshake?? *\/		     */
/* 		    if(clientShake[currentIndex]) { */
/* 			if(!clientSideShake(currentIndex)) { */
/* 			    clientFileDes[currentIndex]=0; */
/* 			    clientErrors[currentIndex]= */
/* 				SOCKET_E_BADSHAKE; */
/* 			    numErrors++; */
/* 			} */
/* 		    } */
/* 		} */
/* 		else if(retVal<0) { */
/* 		    numErrors++; */
/* 		    clientErrors[theUnconnected[i]]=retVal; */
/* 		} */
/* 	    } */
/* 	}	  */
/*     } */
/*     if (numErrors) */
/* 	return SOCKET_E_PORTERRORSET; */
/*     return numClients-numConnected; */
 
/* } */

/* int tryToOpenAllServerConnections() */
/* /\*! */
/*   This function trys to open all the prepped server connections. */
/* *\/ */
/* {     */
/*     fd_set active_fd_set, read_fd_set; */
/*     struct timeval waitTime; */
/*     int numNotConnected,numReady,numConnected=0,i,sock,currentIndex; */
/*     int numErrors=0; */
/*     int theUnconnected[MAX_SOCKETS]; */
    
/*     struct sockaddr_in clientname; */
/*     size_t size; */
    
/*     int newFileDes=0; */
/*     uint32_t hostName; */

/*     /\* Server stuff first. *\/ */
/*     if(numServers) {     */
/* 	/\* Give them some time to actually connect. *\/ */
/* 	waitTime.tv_sec=0; */
/* 	waitTime.tv_usec=1000; */
		
/* 	/\* Initialize the set of active sockets. *\/ */
/* 	FD_ZERO (&active_fd_set); */
	
/* 	numNotConnected=0; */
/* 	numConnected=0; */
/* 	for(i=0;i<numServers;i++) { */
/* 	    if(!serverFileDes[i]) { */
/* 		FD_SET (serverSockNums[i], &active_fd_set); */
/* 		theUnconnected[numNotConnected]=i; */
/* 		numNotConnected++; */
/* 	    } */
/* 	    else numConnected++; */
/* 	} */
/* 	if(numNotConnected) { */
/* 	    read_fd_set = active_fd_set; */
/* 	    numReady=select (FD_SETSIZE, &read_fd_set, NULL, NULL, &waitTime); */
/* 	    if ( numReady< 0) */
/* 	    {	 */
/* 		syslog(LOG_ERR,"select: %s",strerror(errno)); */
/* 		return SOCKET_E_SELECT;		 */
/* 	    } */
/* 	    /\*fprintf (stderr,"FD_SETSIZE = %d\n",FD_SETSIZE);*\/ */
/* 	    if(numReady) { */
/* 		for(i=0;i<numNotConnected;i++) { */
/* 		    newFileDes=0; */
/* 		    currentIndex=theUnconnected[i]; */
/* 		    sock=serverSockNums[currentIndex]; */
/* 		    if(FD_ISSET(sock,&read_fd_set)) { */
/* 			size = sizeof(clientname); */
/* 			newFileDes = accept(sock, */
/* 					    (struct sockaddr *) &clientname,  */
/* 					    &size); */
/* 			if(newFileDes< 0) */
/* 			{ */
/* 			    syslog(LOG_WARNING, */
/* 				   "accept: %s",strerror(errno)); */
/* 			    serverErrors[currentIndex]=SOCKET_E_ACCEPT; */
/* 			    numErrors++; */
/* 			    /\*return SOCKET_E_ACCEPT;*\/ */
/* 			} */
/* 			else { */
/* 			    hostName=clientname.sin_addr.s_addr; */
/* 			    if( hostName != htonl(INADDR_LOOPBACK)) { */
/* 				syslog(LOG_WARNING, */
/* 				       "Non localhost machine\n"); */
/* 				close(newFileDes); */
/* 				newFileDes=0;		     */
/* 			    } */
/* 			    else { */
/* 				syslog(LOG_INFO, */
/* 				       "Connect on port %hd newFileDes %d.\n", */
/* 				       ntohs (clientname.sin_port),newFileDes); */
/* 				numConnected++; */
/* 				serverFileDes[currentIndex]=newFileDes; */
/* 				if(serverShake[currentIndex]) { */
/* 				    if(!serverSideShake(currentIndex)) { */
/* 					serverFileDes[currentIndex]=0; */
/* 					serverErrors[currentIndex]= */
/* 					    SOCKET_E_BADSHAKE; */
/* 					numErrors++; */
/* 				    } */
/* 				} */
				    
/* 			    } */
			    
/* 			} */
/* 		    } */
/* 		} */
	    
/* 	    } */
/* 	} */
/*     } */
/*     if(numErrors) */
/* 	return SOCKET_E_PORTERRORSET;  */
/*     syslog(LOG_DEBUG,"Connected to %d of %d servers",numConnected,numServers); */
/*     return numServers-numConnected; */
/* } */





/* int makeServerSocket (uint16_t port) */
/* /\*! */
/*   Creates a socket on the localhost with port number [port]. Going to have to */
/*   come back and change all these exit calls. */
/* *\/ */
/* { */
    
/*     int sock; */
/*     int on=1; */
/*     struct sockaddr_in name; */
     
/*     /\* Create the socket. *\/ */
/*     sock = socket (PF_INET, SOCK_STREAM, 0); */
/*     if (sock < 0) */
/*     { */
/* 	syslog(LOG_ERR,"socket on port %d: %s",port,strerror(errno)); */
/* 	return SOCKET_E_SOCKET; */
/*     } */
/*     /\* Allow us to reuse address... I think *\/ */
/*     if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0) { */
/* 	syslog(LOG_ERR,"setsockopt on %d: %s",sock,strerror(errno)); */
/* 	return SOCKET_E_SETSOCKOPT; */
/*     } */
    
/*     /\* Give the socket a name. *\/ */
/*     name.sin_family = AF_INET; */
/*     name.sin_port = htons (port); */
/*     name.sin_addr.s_addr = htonl (INADDR_ANY); */
/*     if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) */
/*     { */
/* 	syslog(LOG_ERR,"bind on %d: %s",sock,strerror(errno)); */
/* 	return SOCKET_E_BIND; */
/*     } */
/*     /\* Very important... cause I forgot it and it didn't work *\/ */
/*     if (listen (sock, 1) < 0) */
/*     { */
/* 	syslog(LOG_ERR,"listen on %d: %s",sock,strerror(errno)); */
/* 	return SOCKET_E_LISTEN; */
/*     } */
     
/*     return sock; */
/* } */

/* int makeClientSocket (uint16_t port) */
/* /\*! */
/*   Tries to connect to a socket on the localhost with port number [port]  */
/* *\/ */
/* {    */
/*     int sock; */
/*     int errsv; */
    
/*     struct sockaddr_in servername; */
     
/*     /\* Create the socket. *\/ */
/*     sock = socket (PF_INET, SOCK_STREAM, 0); */
/*     if (sock < 0) */
/*     { */

/* 	syslog(LOG_ERR,"socket (client) on %d: %s", */
/* 	       port,strerror(errno)); */
/* 	return SOCKET_E_SOCKET; */
/*     } */
     
/*     /\* Connect to the server. *\/ */
/*     initSockAddrLocal (&servername,  port); */
/*     if (0 > connect (sock, */
/* 		     (struct sockaddr *) &servername, */
/* 		     sizeof (servername))) */
/*     {	 */
/* 	close(sock); */
/* 	errsv=errno; */
/* 	if(errsv==ECONNREFUSED) { */
/* 	    /\* Probably, server isn't ready yet. *\/ */
/* 	    return 0; */
/* 	} */
/* 	syslog(LOG_ERR,"connect (client) on %d: %s", */
/* 	       sock,strerror(errno)); */
/* 	return SOCKET_E_CONNECT; */
/*     } */
/*     return sock; */
/* } */



/* SocketErrorCode initSockAddr (struct sockaddr_in *name, */
/* 			      const char *hostname, */
/* 			      uint16_t port) */
/* /\*! */
/*   Fills the sockaddr_in structure with hostname and port info  */
/* *\/ */
/* { */
/*     struct hostent *hostinfo; */
     
/*     name->sin_family = AF_INET; */
/*     name->sin_port = htons (port); */
/*     hostinfo = gethostbyname (hostname); */
/*     if (hostinfo == NULL) */
/*     { */
/* 	syslog (LOG_ERR, "Unknown host %s.\n", hostname); */
/* 	return SOCKET_E_UNKOWNHOST; */
/*     } */
/*     name->sin_addr = *(struct in_addr *) hostinfo->h_addr; */
/*     return SOCKET_E_OK; */
/* } */

/* void initSockAddrLocal (struct sockaddr_in *name,  */
/* 			uint16_t port) */
/* /\*! */
/*   Fills the sockaddr_in structure for localhost with port [port]  */
/* *\/ */
/* { */

/*     name->sin_family = AF_INET; */
/*     name->sin_port = htons (port); */
/*     name->sin_addr.s_addr = htonl(INADDR_ANY); */
/* } */


/* int sendToSocket (int filedes, void *buffer, int length, int flags) */
/* /\*! */
/*   Send the contents of buffer, which should be length bytes in size, */
/*   to the socket identified by filedes using the flags. */
/*   At the moment the maximum data length is MAX_LENGTH. */
/* *\/ */
/* { */
/*     int nbytes; */
/*     nbytes = send (filedes, buffer, length, flags); */
/*     if (nbytes < 0) */
/*     { */
/* 	syslog(LOG_ERR,"send on %d: %s",filedes,strerror(errno)); */
/* 	return SOCKET_E_SENDFAILURE; */
/*     } */
/*     return nbytes; */
/* } */
     
/* int recvFromSocket (int filedes, void *buffer, int flags) */
/* /\*! */
/*   Try to read from the socket identified by filedes. */
/*   At the moment the maximum data length is MAX_LENGTH. */
/*   If the socket has closed then you read zero bytes, then the filedes is */
/*   closed. */
/* *\/ */
/* { */
/*     int nbytes;     */
/*     syslog(LOG_DEBUG,"Trying to read fileDes %d, into buffer %x with flags %d", */
/* 	   filedes,(int)buffer,flags); */
/*     nbytes = recv (filedes, buffer, MAX_LENGTH, flags); */
    
/*     if (nbytes < 0) */
/*     { */
/* 	/\* Read error. *\/ */
/* 	syslog(LOG_ERR,"recv on %d: %s",filedes,strerror(errno)); */
/* 	return SOCKET_E_RECVFAILURE; */
/*     } */
/*     else if (nbytes == 0) { */
/* 	/\* End-of-file. *\/ */
/* /\* 	syslog(LOG_WARNING,"Socket %d closed.",filedes); *\/ */
/* 	close(filedes); */
/* 	return SOCKET_E_EOF; */
/*     } */
/*     return nbytes; */
/* } */


/* int checkForNewClientConnection(int filedes) */
/* /\*! */
/*   Poll the socket identified by filedes to see if there are any incoming  */
/*   connections to it. If there are accept the connection and check to see  */
/*   if it comes from localhost, if all is well the new filedes wil be the return */
/*   value. If no attempt to connect is made it will return 0. If something is */
/*   amiss you get a -1. */
/* *\/ */
/* { */
/*     /\* At the moment it just polls to see if there are any connections */
/*        waiting to be serviced. *\/ */
/*     struct timeval waitTime; */
/*     struct sockaddr_in clientname; */
/*     size_t size; */
/*     int sock=filedes; */
/*     int numReady; */
/*     int newFileDes=0; */
/*     uint32_t hostName; */
/*     fd_set active_fd_set, read_fd_set; */

/*     waitTime.tv_sec=0; */
/*     waitTime.tv_usec=0; */



/*     /\* Initialize the set of active sockets. *\/ */
/*     FD_ZERO (&active_fd_set); */
/*     FD_SET (sock, &active_fd_set); */


/*     read_fd_set = active_fd_set; */
/*     numReady=select (FD_SETSIZE, &read_fd_set, NULL, NULL, &waitTime); */
/*     if ( numReady< 0) */
/*     {	 */
/* 	syslog(LOG_ERR,"select on %d: %s",filedes,strerror(errno)); */
/* 	return SOCKET_E_SELECT; */

/*     } */
/*     /\*fprintf (stderr,"FD_SETSIZE = %d\n",FD_SETSIZE);*\/ */
/*     if(numReady) { */
/* 	if(FD_ISSET(sock,&read_fd_set)) { */
/* 	    size = sizeof(clientname); */
/* 	    newFileDes =accept(sock,(struct sockaddr *) &clientname, &size); */
/* 	    if(newFileDes< 0) */
/* 	    { */
/* 		syslog(LOG_ERR,"accept on %d: %s",filedes,strerror(errno)); */
/* 		return SOCKET_E_ACCEPT; */
/* 	    } */
/* 	    hostName=clientname.sin_addr.s_addr; */
/* 	    if( hostName != htonl(INADDR_LOOPBACK)) { */
/* 		syslog(LOG_WARNING, */
/* 		       "Got request from non localhost machine\n"); */
/* 		close(newFileDes); */
/* 		newFileDes=0; */
		    
/* 	    } */
/* 	    else { */
/* 		syslog(LOG_INFO, */
/* 		       "Connect from localhost port %hd newFileDes %d.\n", */
/* 		       ntohs (clientname.sin_port),newFileDes); */
/* 	    } */
	    
/* 	} */
/*     } */
/*     return newFileDes; */
/* } */

int isSocketReadyForSendingNow(int filedes)
/*!
  Poll (using select) if socket is available to send (write) data.
*/
{
    /* At the moment it just polls to see if there are any connections
       waiting to be serviced. */
    struct timeval waitTime;
    waitTime.tv_sec=0;
    waitTime.tv_usec=0;
    return isSocketReadyForSending(filedes,&waitTime);
}

int isSocketReadyForSending(int filedes,struct timeval *waitTimePtr)
/*!
  Check (using select) if socket is available to send (write) data.
  It will wait waitTime amount of time for the select call
*/
{  
    fd_set active_fd_set, write_fd_set;
    int numReady;
    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (filedes, &active_fd_set);
    write_fd_set = active_fd_set;
    numReady=select (filedes+1, NULL, &write_fd_set, NULL, waitTimePtr);
    if ( numReady< 0)
    {
	syslog (LOG_ERR,"select on %d: %s",filedes,strerror(errno));
	return SOCKET_E_SELECT;
    }
    if(numReady) 
	if(FD_ISSET(filedes,&write_fd_set)) 
	    return 1;
    return 0;
}


int isSocketReadyForReceivingNow(int filedes)
/*!
  Poll (using select) if socket is available to receive (read) data.
  Returns 1 if data is waiting (or the socket has been closed), 0 if it isn't.
*/
{
    /* At the moment it just polls to see if there are any connections
       waiting to be serviced. */
    struct timeval waitTime;
    waitTime.tv_sec=0;
    waitTime.tv_usec=0;
    return isSocketReadyForReceiving(filedes,&waitTime);
}

int isSocketReadyForReceiving(int filedes,struct timeval *waitTimePtr)
/*!
  Check (using select) if socket is available to receive (read) data.
  It will wait waitTime amount of time for the select call.
  Returns 1 if data is waiting (or the socket has been closed), 0 if it isn't.
*/
{
    int numReady;
    fd_set active_fd_set, read_fd_set;

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (filedes, &active_fd_set);
    read_fd_set = active_fd_set;
    numReady=select (filedes+1, &read_fd_set, NULL, NULL, waitTimePtr);
    if ( numReady< 0)
    {
	syslog (LOG_ERR,"select on %d: %s",filedes,strerror(errno));
	return SOCKET_E_SELECT;
    }
    if(numReady) {
/* 	printf("numReady: %d\t",numReady); */
	if(FD_ISSET(filedes,&read_fd_set)) 
	    return 1;
    }
    return 0;
}


/* int serverSideHandshake(int fileDes, char *myMagicWord, char *yourMagicWord) */
/* /\*! */
/*   Silly handshaking based upon magic words. This is the server half. */
/* *\/ */
/* { */
    
/*     int canIReceive,canISend,nbytes; */
/*     char myBuffer[MAX_LENGTH]; */
/*     struct timeval waitTime; */
    
/*     waitTime.tv_sec=0; */
/*     waitTime.tv_usec=1000; */
    
/*     syslog(LOG_DEBUG,"Trying to shake on filedes %d, using %s and %s", */
/*       fileDes,myMagicWord,yourMagicWord); */
    
/*     canIReceive=isSocketReadyForReceiving(fileDes,&waitTime); */
/*     syslog(LOG_DEBUG,"Is socket %d ready for receiving: %d", */
/* 	   fileDes,canIReceive); */

/*     if(canIReceive) { */
/* 	nbytes=recvFromSocket(fileDes,myBuffer,0); */
/* 	syslog(LOG_DEBUG,"Got %d byte message: %s\n",nbytes,myBuffer); */
/* 	if(strcmp(yourMagicWord,myBuffer)) { */
/* 	    syslog(LOG_WARNING,"Client did not shake politely.\n"); */
/* 	    close(fileDes); */
/* 	    return SOCKET_E_BADSHAKE; */
/* 	} */
/*     } */
/*     else { */
/* 	syslog(LOG_WARNING,"Client timed out.\n"); */
/* 	close(fileDes); */
/* 	return SOCKET_E_BADSHAKE; */
/*     } */
	
/*     canISend=isSocketReadyForSending(fileDes,&waitTime); */
/*     syslog(LOG_DEBUG,"Is socket %d ready for sending: %d", */
/* 	   fileDes,canISend); */
/*     /\* Should always be???? *\/ */
    
/*     nbytes=sendToSocket(fileDes,myMagicWord,strlen(myMagicWord)+1,0); */
/*     if(nbytes==SOCKET_E_SENDFAILURE) { */
/* 	syslog(LOG_ERR,"Can not send %s to %d",myMagicWord,fileDes); */
/* 	return SOCKET_E_SENDFAILURE; */
/*     } */
/*     return 1; */
/* } */

/* int clientSideHandshake(int fileDes, char *myMagicWord, char *yourMagicWord) */
/* /\*! */
/*   Silly handshaking based upon magic words. This is the client half. */
/* *\/ */
/* {   */
/*     int canIReceive,canISend,nbytes; */
/*     struct timeval waitTime; */
/*     char myBuffer[MAX_LENGTH]; */
    
/*     waitTime.tv_sec=0; */
/*     waitTime.tv_usec=1000; */

/*     syslog(LOG_DEBUG,"Trying to shake on filedes %d, using %s and %s", */
/*       fileDes,myMagicWord,yourMagicWord); */
    
/*     canISend=isSocketReadyForSending(fileDes,&waitTime); */
/*     syslog(LOG_DEBUG,"Is socket %d ready for sending: %d", */
/* 	   fileDes,canISend); */
/*     /\* Should always be???? *\/ */
/*     sendToSocket(fileDes,myMagicWord,strlen(myMagicWord)+1,0); */

    
/*     waitTime.tv_sec=1; */
/*     waitTime.tv_usec=1000; */
/*     canIReceive=isSocketReadyForReceiving(fileDes,&waitTime); */
/*     if(canIReceive) { */
/* 	nbytes=recvFromSocket(fileDes,myBuffer,0); */
/* 	syslog(LOG_DEBUG,"Received string: %s\n",myBuffer); */
/* /\* 	printf("Got message: %s\n",myBuffer); *\/ */
/* 	if(strcmp(yourMagicWord,myBuffer)) { */
/* 	    syslog(LOG_WARNING,"Server did not shake politely.\n"); */
/* 	    close(fileDes); */
/* 	    return 0; */
/* 	} */
/* 	syslog(LOG_DEBUG,"Handshake was good"); */
/* 	return 1; */
/*     } */
/*     syslog(LOG_WARNING,"Server timed out.\n"); */
/*     close(fileDes); */
/*     return 0; */
	
/* } */


/* /\* Simple check socket that reads any available data *\/ */
/* int checkAndReadSocketNow(int filedes, void *buffer) */
/* /\*! */
/*   Poll (using select) if socket is available to receive (read) data. */
/*   And if it is read the data. */
/*   Returns 0 for no data, 1 for got data and -1 for socket closed. */
/* *\/ */
/* { */
/*     struct timeval waitTime; */
    
/*     waitTime.tv_sec=0; */
/*     waitTime.tv_usec=0; */
/*     return checkAndReadSocket(filedes,buffer,&waitTime); */
/* } */
	
	
/* int checkAndReadSocket(int filedes, void *buffer, struct timeval *waitTimePtr) */
/* /\*! */
/*   Poll (using select) if socket is available to receive (read) data. */
/*   And if it is read the data. */
/*   Returns 0 for no data, 1 for got data and -1 for socket closed. */
/* *\/ */
/* { */
    
/*     if(isSocketReadyForReceiving(filedes,waitTimePtr)) { */
	
/* 	/\* Socket is ready *\/ */

	
/* 	if(recvFromSocket(filedes,buffer,0)) { */
/* 	    /\* Got some data *\/ */
/* 	    return 1; */
/* 	} */
/* 	else { */
/* 	    /\* Socket closed *\/ */
/* 	    return -1; */
/* 	}		     */
/*     } */
/*     /\* Nothing for me *\/ */
/*     return 0; */
/* } */

/* int checkForNewClientAndShake(int theSock,char *myMagicWord,  */
/* 					    char *yourMagicWord)  */
/* /\*! Checks for new client connections, and does the silly handshake  */
/*    if they are there. Returns the new file descriptor if a connection is made */
/*    and returns 0 if it isn't. */
/* *\/ */

/* { */
/*     int newFileDes=checkForNewClientConnection(theSock);  */
/*     if(newFileDes>0) { */
/* /\*	printf("Got connection with: %d\n",newFileDes); *\/ */
/* 	if(serverSideHandshake(newFileDes,myMagicWord,yourMagicWord)==1) { */
/* /\*	    printf("Handshake accepted\n"); *\/ */
/* 	    return newFileDes; */
/* 	}   */
/* 	else { */
/* 	    syslog(LOG_WARNING,"Handshake failed\n"); */
/* 	    close(newFileDes); */
/* 	} */
/*     } */
/*     return 0; */
/* } */

/* int connectToServerAndShake(uint16_t thePort,char *myMagicWord, char *yourMagicWord)  */
/* /\*! Tries to connect to server and do the silly handshake . Returns the  */
/*   new file descriptor if a connection is made and returns 0 if it isn't. */
/* *\/ */
/* {     */
    
/*     int theSock = makeClientSocket (thePort); */
/*     /\*printf("Making client socket %d \t%d\n",thePort,theSock);*\/ */
/*     if(theSock) { */
/* 	if (clientSideHandshake (theSock,myMagicWord,yourMagicWord)) */
/* 	    return theSock; */
/* 	else { */
/* 	    syslog (LOG_WARNING,"Handshake not accepted\n"); */
/* 	} */
/* 	close(theSock);  		 */
/*     }   */
/*     /\* Server probably isn't running at the moment (or handshake failed) *\/ */
/*     return 0; */
/* } */


