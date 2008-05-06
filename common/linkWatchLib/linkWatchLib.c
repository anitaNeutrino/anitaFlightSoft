/*! \file linkWatchLib.c
    \brief Utility library to watch link dirs in either fifo or lifo mode
    
    Some functions to help ease the usage 
*/

#include "linkWatchLib/linkWatchLib.h"

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
#include <inotifytools/inotify.h>

#define MAX_WATCHES 100
#define MAX_LINKS 1000
#define MAX_LINK_NAME 100

static unsigned int numWatches=0;
static int inotify_fd=0;
static int wdArray[MAX_WATCHES]={0};
static int numLinkArray[MAX_WATCHES]={0};
//static int linkNameArray[MAX_WATCHES][MAX_LINKS][MAX_LINK_NAME];



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
  printf("Setting up wd %d on linkDir: %s\n",wd,linkDir);
  numWatches++;
  return wd;
}    




int checkLinkDirs(int timeout)
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
  static unsigned int bytes_to_read;
  static int rc;
  static fd_set read_fds;
  
  static struct timeval read_timeout;
  read_timeout.tv_sec = timeout;
  read_timeout.tv_usec = 0;
  static struct timeval * read_timeout_ptr;
  read_timeout_ptr = ( timeout <= 0 ? NULL : &read_timeout );
  
 
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
      event = &buffer[upto_byte];
      for(i=0;i<numWatches;i++) {
	if(event->wd==wdArray[i]) {
	//Got a match
	  numLinkArray[i]++;
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

char *getFirstLink(int wd); //returns the filename of the first link
char *getLastLink(int wd); //returns the filename of the most recent link (no idea how)
