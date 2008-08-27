/*! \file linkWatchLib.c
    \brief Utility library to watch link dirs in either fifo or lifo mode
    
    Some functions to help ease the usage 
*/

#include "linkWatchLib/linkWatchLib.h"
#include <linkWatchLib/inotify.h>

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

//inotify includes
//#include <inotifytools/inotifytools.h>


extern  int versionsort(const void *a, const void *b);


#define MAX_WATCHES 100
#define MAX_LINKS 1000
#define MAX_LINK_NAME 100

static unsigned int numWatches=0;
static int inotify_fd=0;
static int wdArray[MAX_WATCHES]={0};
static char watchDirArray[MAX_WATCHES][FILENAME_MAX];
static int numLinkArray[MAX_WATCHES]={0};
//static int linkNameArray[MAX_WATCHES][MAX_LINKS][MAX_LINK_NAME];

struct listItem {
  int watchIndex;
  char linkName[100];
  struct listItem *prev;
  struct listItem *next;
};

struct listItem *firstLink[MAX_WATCHES]={0};
struct listItem *lastLink[MAX_WATCHES]={0};


int getWatchIndex(int watchNumber)
{
  static int i=-1;
  static int wd=0;
  if(wd==watchNumber)
    return i;

  for(i=0;i<numWatches;i++) {
    if(watchNumber==wdArray[i]) {
      wd=wdArray[i];
      return i;
    }
  }
  return -1;
}

void prepLinkList(int watchIndex)
//Checks the directory as is and fills in the linkList
{
  int i=0;
  struct dirent **linkList;    
  int numLinks=getListofLinks(watchDirArray[watchIndex],&linkList);
  fprintf(stderr,"There are %d links in %s\n",numLinks,watchDirArray[watchIndex]);
  if(numLinks) {
    for(i=0;i<numLinks;i++) {
      addLink(watchIndex,linkList[i]->d_name);
    }    
    while(numLinks) {
      free(linkList[numLinks-1]);
      numLinks--;
    }
    free(linkList);    
  }
  
}

int setupLinkWatchDir(char *linkDir)
//returns -1 for failure otherwise it returns the watch index number
{
  int watchEvent=IN_CREATE; //Only care about the creation of new link files
  int wd=0;
  //Initialize (if this is the first call)
  static int alreadInit=0;
  if(!alreadInit) {
    inotify_fd = inotify_init();
    if (inotify_fd < 0)     {
      syslog(LOG_ERR,"Couldn't initialize inotify %s\n",strerror(errno));
      fprintf(stderr,"Couldn't initialize inotify %s\n",strerror(errno));
      return -1;
    }
    alreadInit=1;
  }

  //Now add linkDir to the watch list
  if(numWatches>=MAX_WATCHES) {
    syslog(LOG_ERR,"Watching too many dirs can not add a new one\n");
    fprintf(stderr,"Watching too many dirs can not add a new one\n");
    return -1;
  }

  //  retVal=inotifytools_watch_file( linkDir, watchEvent );
  wd=inotify_add_watch(inotify_fd,linkDir,watchEvent);
  if(wd<0) {
    syslog(LOG_ERR,"Error watching dir %s\n",linkDir);
    fprintf(stderr,"Error watching dir %s\n",linkDir);
    return -1;
  }

  wdArray[numWatches]=wd;
  strncpy(watchDirArray[numWatches],linkDir,FILENAME_MAX);
  printf("Setting up wd %d on linkDir: %s %s\n",wd,linkDir,watchDirArray[numWatches]);
  numWatches++;
  prepLinkList(numWatches-1);
  //There's a race condition here and it's possible that the same file will be included twice. Lets see if it actually happens. I'll probably add something that checks if a new link is identical to the old one.
  return wd;
}    

int refreshLinkDirs()
{
    //First step is delete (from memory not disk) all the items in the list
    int watchInd=0,watchNumber=0;
    char *testVal=0;
    for(watchInd=0;watchInd<numWatches;watchInd++) {
	watchNumber=wdArray[watchInd];
	do {
	    testVal=getLastLink(watchNumber);
	} while(testVal!=NULL);
	prepLinkList(watchInd);
    }       	       
    return 0;
}


int checkLinkDirs(int timeoutSec, int timeOutUSec)
{
  //Checks all link dirs
  // returns 1 if there are new links
  // 0 if there are no new links
  int i=0;
  int gotSomething=0;
  int countEvents=0;
  char buffer[1000];
  struct inotify_event *event=(struct inotify_event*)&buffer[0];
  static ssize_t this_bytes;
  //  static unsigned int bytes_to_read;
  static int rc;
  static fd_set read_fds;
  
  static struct timeval read_timeout;
  read_timeout.tv_sec = timeoutSec;
  read_timeout.tv_usec = 0;
  static struct timeval * read_timeout_ptr;
  if(timeoutSec==0 && timeOutUSec==0)
    read_timeout_ptr = NULL;
  else
    read_timeout_ptr= &read_timeout;
  
 
  do {
    //    printf("Here in loop\n");
    FD_ZERO(&read_fds);
    FD_SET(inotify_fd, &read_fds);
    rc = select(inotify_fd + 1, &read_fds,
		NULL, NULL, read_timeout_ptr);
    if ( rc < 0 ) {
      // error
      fprintf(stderr,"Error using select to look for new events %s\n",
	      strerror(errno));
      break;
    }
    else if ( rc == 0 ) {
      //      fprintf(stderr,"Timed out\n");
      break;
    }
    read_timeout_ptr->tv_sec=0;
    read_timeout_ptr->tv_usec=1;
    //    printf("After select -- rc = %d\n",rc);
    // wait until we have enough bytes to read a single event (maybe this bit isn't necessary
 /*    do { */
/*       rc = ioctl( inotify_fd, FIONREAD, &bytes_to_read ); */
/*       printf("bytes to read %d  (want %d)\n",bytes_to_read,sizeof(struct inotify_event)); */
/*     } while ( !rc && */
/* 	      bytes_to_read < sizeof(struct inotify_event)); */

//    printf("After ioctl\n");
    //Try to read single event
    this_bytes = read(inotify_fd, buffer,1000);
    //    		      sizeof(struct inotify_event));
    if(this_bytes<0) {
      fprintf(stderr,"Error reading from %d  -- %s\n",inotify_fd,strerror(errno));
    }
    //    printf("After read  (%d bytes read)\n",this_bytes);
    //    printf("Read %d %u %u %u %s\n",event->wd,event->mask,event->cookie,
    //	   event->len,event->name);
    int upto_byte=0;
    while(upto_byte<this_bytes) {
      //Who knows how many events we just read
      event = (struct inotify_event*)&buffer[upto_byte];
      for(i=0;i<numWatches;i++) {
	if(event->wd==wdArray[i]) {
	//Got a match
	  addLink(i,event->name);
	//Should now copy the filename from event->name in to some 
	//list of links thingy
	  break;
	}
      }
      upto_byte+=sizeof(struct inotify_event)+event->len;
      countEvents++;
    }
    gotSomething=1;
  } while (1);
  
  if(gotSomething) {
    //    printf("Read %d events\n",countEvents);
    return countEvents;
  }
  return 0;
}


int getNumLinks(int wd)
  //returns number of links, zero, -1 for failure
{
  int i=0;
  for(i=0;i<numWatches;i++) {
    if(wd==wdArray[i])
      return numLinkArray[i];
  }
  return -1;
}

void addLink(int watchIndex, char *linkName)
{
  //Check if watchIndex is in range, for now assume it is
  if(!lastLink[watchIndex]) {
    //Then this is our first link
    lastLink[watchIndex]=(struct listItem*) malloc(sizeof(struct listItem));
    memset(lastLink[watchIndex],0,sizeof(struct listItem));
    
    firstLink[watchIndex]=lastLink[watchIndex];
    firstLink[watchIndex]->watchIndex=watchIndex;
    strncpy(firstLink[watchIndex]->linkName,linkName,99);
    firstLink[watchIndex]->prev=0; //It's the first
    firstLink[watchIndex]->next=0; //It's also the last    
  }
  else {
    // We have other links in our list
    struct listItem *newLink = (struct listItem*) malloc(sizeof(struct listItem));
    lastLink[watchIndex]->next=newLink;
    newLink->prev=lastLink[watchIndex];
    lastLink[watchIndex]=newLink;
    lastLink[watchIndex]->watchIndex=watchIndex;
    strncpy(lastLink[watchIndex]->linkName,linkName,99);
    lastLink[watchIndex]->next=0; //It's the last    
  }
  numLinkArray[watchIndex]++;
	  
}

char *getLastLink(int watchNumber) 
{
  //The returnend string is a static variable that will be over written
  int watchIndex=getWatchIndex(watchNumber);
  static char linkName[FILENAME_MAX];
  if(!lastLink[watchIndex])
    return NULL;

  //  strncpy(linkName,watchDirArray[watchIndex],FILENAME_MAX);
  //  strncat(linkName,lastLink[watchIndex]->linkName,100);
  strncpy(linkName,lastLink[watchIndex]->linkName,100);
  struct listItem *nextToLast=lastLink[watchIndex]->prev;
  if(nextToLast) {
    nextToLast->next=0;
    free(lastLink[watchIndex]);
    lastLink[watchIndex]=nextToLast;
  }
  else {
    //This is the only item in the list
    free(lastLink[watchIndex]);
    lastLink[watchIndex]=firstLink[watchIndex]=0;
  }
  numLinkArray[watchIndex]--;
  return linkName;
}


char *getFirstLink(int watchNumber) 
{
  //For now assume watchIndex is just wd-1 
  //The returnend string is a static variable that will be over written
  int watchIndex=getWatchIndex(watchNumber);
  static char linkName[FILENAME_MAX];
  if(!firstLink[watchIndex])
    return NULL;

  //  strncpy(linkName,watchDirArray[watchIndex],FILENAME_MAX);
  //  strncat(linkName,firstLink[watchIndex]->linkName,100);
  strncpy(linkName,firstLink[watchIndex]->linkName,100);
  //  printf("%s %s %s\n",linkName,watchDirArray[watchIndex],firstLink[watchIndex]->linkName);
  struct listItem *nextToFirst=firstLink[watchIndex]->next;
  if(nextToFirst) {
    //Remove the first item and carry on
    nextToFirst->prev=0;
    free(firstLink[watchIndex]);
    firstLink[watchIndex]=nextToFirst;
  }
  else {
    //This is the only item in the list
    free(firstLink[watchIndex]);
    firstLink[watchIndex]=lastLink[watchIndex]=0;
  }
  numLinkArray[watchIndex]--;
  return linkName;
}


int getListofLinks(const char *theEventLinkDir, struct dirent ***namelist)
{
/*     int count; */
    static int errorCounter=0;
    int n = scandir(theEventLinkDir, namelist, filterOnDats, versionsort);
    if (n < 0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"scandir %s: %s",theEventLinkDir,strerror(errno));
	    fprintf(stderr,"scandir %s: %s\n",theEventLinkDir,strerror(errno));
	    errorCounter++;
	}
	    
    }	
 /*    for(count=0;count<n;count++)  */
/* 	printf("%s\n",(*namelist)[count]->d_name); */
    return n;	    
}

int getListofPurgeFiles(const char *theEventLinkDir, struct dirent ***namelist)
{
/*     int count; */
    static int errorCounter=0;
    int n = scandir(theEventLinkDir, namelist, filterOnGzs, versionsort);
    if (n < 0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"scandir %s: %s",theEventLinkDir,strerror(errno));
	    fprintf(stderr,"scandir %s: %s\n",theEventLinkDir,strerror(errno));
	    errorCounter++;
	}
	    
    }	
 /*    for(count=0;count<n;count++)  */
/* 	printf("%s\n",(*namelist)[count]->d_name); */
    return n;	    
}


int filterHeaders(const struct dirent *dir)
{
    if(strncmp(dir->d_name,"hd",2)==0)
	return 1;
    return 0;
}


int filterOnDats(const struct dirent *dir)
{

    if(strstr(dir->d_name,".dat")!=NULL)
	return 1;
    return 0;
}


int filterOnGzs(const struct dirent *dir)
{

    if(strstr(dir->d_name,".gz")!=NULL)
	return 1;
    return 0;
}
