/*! \file socketLib.h
    \brief Prototype socket library wrapper for the GNU socket library.
    
    Just a few functions to ease the use of the GNU socket library stuff.
    Hopefully this library will provide the mechanism for all 
    inter-process communication in the ANITA flight software.
    July 2004  rjn@mps.ohio-state.edu
*/
/*
 * Current CVS Tag:
 * $Header: /work1/anitaCVS/flightSoft/common/socketLib/socketLib.h,v 1.5 2004/10/08 20:08:41 rjn Exp $
 */
 
/* 
 * Modification History : DO NOT EDIT - MAINTAINED BY CVS 
 * $Log: socketLib.h,v $
 * Revision 1.5  2004/10/08 20:08:41  rjn
 * Updated a lot of things. fakeAcqd.c is coming along, and now communicates with Eventd through the file system (the filesystem in question will be a ramdisk on the flight computer)
 *
 * Revision 1.4  2004/09/08 18:02:53  rjn
 * More playing with socketLib. And a few changes as --pedantic is now one of the compiler options.
 *
 * Revision 1.3  2004/09/07 13:52:21  rjn
 * Added dignified multiple port handling capabilities to socketLib. Not fully functional... but getting there.
 *
 * Revision 1.2  2004/09/01 15:09:13  rjn
 * Just playing with CVS
 *
 * Revision 1.1  2004/09/01 14:49:17  rjn
 * Renamed mySocket to socketLib
 *
 *
 */ 

/* Includes */
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>



typedef enum {
    SOCKET_E_OK = 0,
    SOCKET_E_NOTPREPPED = -0x100, /*-256*/
    SOCKET_E_MAXSOCKETS,
    SOCKET_E_DUPLICATEPORT,
    SOCKET_E_CREATE,
    SOCKET_E_DUPLICATESOCK,
    SOCKET_E_BADSHAKE,
    SOCKET_E_BOTHCLIENTANDSERVER, /*-250*/
    SOCKET_E_IMPOSSIBLE,
    SOCKET_E_NOCONNECTION,
    SOCKET_E_SENDNOTREADY,
    SOCKET_E_SENDFAILURE,
    SOCKET_E_RECVFAILURE,
    SOCKET_E_EOF,
    SOCKET_E_SELECT,
    SOCKET_E_SOCKET,
    SOCKET_E_SETSOCKOPT,
    SOCKET_E_BIND,
    SOCKET_E_LISTEN,
    SOCKET_E_CONNECT,
    SOCKET_E_RECV,
    SOCKET_E_ACCEPT,
    SOCKET_E_UNKOWNHOST,
    SOCKET_E_PORTERRORSET
} SocketErrorCode ;


/*!
  Is the maximum data length, in bytes, that can be sent at a time. Will need
  to be changed at some point in the future.
*/
#define MAX_LENGTH  1024
/*!
  The number of sockets that can be opened by a single program (the actual 
  number is double this as server and client sockets are counted separately).
  
*/
#define MAX_SOCKETS  10   


/* Simple functions that creates socket on port port on localhost */
/* the return value is the File Descriptor of the socket */ 
int makeServerSocket (uint16_t port);
int makeClientSocket (uint16_t port);



/* The simple functions that create the socket addresses */
void initSockAddrLocal (struct sockaddr_in *name, uint16_t port);
SocketErrorCode initSockAddr (struct sockaddr_in *name,const char *hostname,
			      uint16_t port);


/* Simple read write functions */
int sendToSocket (int filedes, void *buffer, int length, int flags);
int recvFromSocket (int filedes, void *buffer, int flags);


/* Check for new connections to the server socket */
/* Returns zero if there aren't any, the filedes of the new connection */
/* if there is, and -1 if it gets confused. */
/* This will have to be improved */
int checkForNewClientConnection(int filedes);


/* Simplest most encapsulated functions to check if there is data to be
   read, or if it is possible to write data */
int isSocketReadyForSendingNow(int filedes);
int isSocketReadyForReceivingNow(int filedes);
int isSocketReadyForSending(int filedes, struct timeval *waitTimePtr);
int isSocketReadyForReceiving(int filedes, struct timeval *waitTimePtr);

/* Simple functions for handshaking */
/* After client connects it sends its magic word, server receives clients magic word and responds in kind. */
int serverSideHandshake(int fileDes, char *myMagicWord, char *yourMagicWord);
int clientSideHandshake(int fileDes, char *myMagicWord, char *yourMagicWord);


/* Simple check socket that reads any available data */
int checkAndReadSocketNow(int filedes, void *buffer);
int checkAndReadSocket(int filedes, void *buffer, struct timeval *waitTimePtr);


/* Checks for new client connections, and does the silly handshake 
   if they are there */
int checkForNewClientAndShake(int theSock,char *myMagicWord,
			      char *yourMagicWord);
int connectToServerAndShake(uint16_t thePort,char *myMagicWord,char *yourMagicWord);


/* These functions provide the most encapsualted access to the sockets.
   socketLib maintains a list of all open and pending sockets, and upon request
   services them. */
static int getServerIndex(uint16_t thePort);
static int getClientIndex(uint16_t thePort);

static int serverSideShake(int servIndex);
static int clientSideShake(int clientIndex);

/* Prep functions just store the port numbers we're interested in and ready 
   the sockets for connection */
int prepServer(uint16_t thePort);
int prepServerWithNames(uint16_t thePort,char *myName,char *hisName);
int prepClient(uint16_t thePort);
int prepClientWithNames(uint16_t thePort,char *myName,char *hisName);

/* These functions actually make the connections, performing handshaking if
   requested. These are single port only functions. There will be other
   functions that look at all the interesting ports together. */
int checkForNewConns(uint16_t thePort);
int tryToConnectToServer(uint16_t thePort);

/* Does exactly what it says on the tin. Send data to port blah */
int sendDataOnPort(uint16_t thePort, void *theBuffer, int length);
static int sendDataToServerSocket(int theIndex, void *theBuffer, int length);
static int sendDataToClientSocket(int theIndex, void *theBuffer, int length);

/* Checks port for data and reads it if it's sitting there 
   waiting to be read. */
int readDataFromPort(uint16_t thePort,void *theBuffer);
static int readDataFromServerSocket(int theIndex, void *theBuffer);
static int readDataFromClientSocket(int theIndex, void *theBuffer);

/* This function looks at the connected sockets, and checks for new data on 
   them. */
int checkOpenConnections();

/* These functions look at all unconnected sockets, and sees if it can connect
   them. */
int tryToOpenAllServerConnections();
int tryToOpenAllClientConnections();

