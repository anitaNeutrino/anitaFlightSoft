#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>
#include <ftpLib/ftplib.h>

#include "includes/anitaStructures.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "pedestalLib/pedestalLib.h"
#include "compressLib/compressLib.h"
#include "linkWatchLib/linkWatchLib.h"
#include <linkWatchLib/inotify.h>

#define MAX_NEO_WATCHES 2000

int wdNeo=0;
int inotify_fd=0;
char *watchDirArray[MAX_NEO_WATCHES]={0};


int addWatchDirRecursive(char *watchDir);
int setupRecursiveWatchDir(char *watchDir);
int checkWatchDirs(int timeoutSec, int timeOutUSec);


int setupRecursiveWatchDir(char *watchDir)
//returns -1 for failure otherwise it returns the watch index number
{
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
  return addWatchDirRecursive(watchDir);
}    

int addWatchDirRecursive(char *watchDir) {
  printf("addWatchDirRecursive -- %s\n",watchDir);
  int watchEvent=IN_CREATE; //Only care about the creation of new link files
  int wd=0;



  char * my_path;

  DIR * dir;
  dir = opendir( watchDir);
  if(!dir) {
    if(errno == ENOTDIR)
      fprintf(stderr,"%s is not a directory\n",watchDir);
    else
      fprintf(stderr,"%s doesn't exist\n",watchDir);
    return -1;
  }

  if ( watchDir[strlen(watchDir)-1] != '/' ) {
    asprintf( &my_path, "%s/", watchDir );
  }
  else {
    my_path = (char *)watchDir;
  }
  
  static struct dirent * ent;
  char * next_file;
  static struct stat64 my_stat;
  ent = readdir( dir );
  int error=0;

  // Watch each directory within this directory
  while ( ent ) {
    if ( (0 != strcmp( ent->d_name, "." )) &&
	 (0 != strcmp( ent->d_name, ".." )) ) {
      asprintf(&next_file,"%s%s", my_path, ent->d_name);
      if ( -1 == lstat64( next_file, &my_stat ) ) {
	error = errno;
	free( next_file );
	if ( errno != EACCES ) {
	  error = errno;
	  if ( my_path != watchDir ) free( my_path );
	  closedir( dir );
	  return 0;
	}
      }
      else if ( S_ISDIR( my_stat.st_mode ) &&
		!S_ISLNK( my_stat.st_mode )) {
	free( next_file );
	asprintf(&next_file,"%s%s/", my_path, ent->d_name);
	static int status;
	status = addWatchDirRecursive(next_file);
	// For some errors, we will continue.
	if ( !status && (EACCES != error) && (ENOENT != error) &&
	     (ELOOP != error) ) {
	  free( next_file );
	  if ( my_path != watchDir ) free( my_path );
	  closedir( dir );
	  return 0;
	}	
	free( next_file );
      } // if isdir and not islnk
      else {
	free( next_file );
      }
    }
    ent = readdir( dir );
    error = 0;
  }
  closedir( dir );
  
  //First up we
  wd=inotify_add_watch(inotify_fd,watchDir,watchEvent);
  if(wd<0) {
    fprintf(stderr,"Error watching dir %s\n",watchDir);
    return -1;
  }
  if(wd>=MAX_NEO_WATCHES) {
    fprintf(stderr,"Ooops wd>MAX_NEO_WATCHES -- %d %d\n",wd,MAX_NEO_WATCHES);
    exit(0);
  }
  asprintf(&watchDirArray[wd],"%s",watchDir);
  return wd;

}


int checkWatchDirs(int timeoutSec, int timeOutUSec)
{
  //Checks all watch dirs
  // returns 1 if there are new links
  // 0 if there are no new links
  int i=0;
  int gotSomething=0;
  int countEvents=0;
  char buffer[1000];
  char *newFile=0;
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

      
      asprintf(&newFile,"%s/%s",watchDirArray[event->wd],
	       event->name);

      DIR * dir;
      dir = opendir( newFile);
      if(!dir) {
	if(errno == ENOTDIR) {
	  fprintf(stderr,"Got a file %s\n",newFile);
	}
	else {
	  fprintf(stderr,"%s doesn't exist\n",newFile);
	}
      }
      else {
	//Got a new dir
	addWatchDirRecursive(newFile);
	free(newFile);
      }

      //Now what do we do with it??
      printf("wd %d, name %s\n",event->wd,event->name);

      upto_byte+=sizeof(struct inotify_event)+event->len;
      countEvents++;
    }
    gotSomething=1;
  } while (1);
  
  if(gotSomething) {
    printf("Read %d events\n",countEvents);
    return countEvents;
  }
  return 0;
}



int main(int argc, char**argv) {
  setupRecursiveWatchDir("/tmp/neobrick");
  while(1) {
    printf("checkWatchDirs: %d\n",checkWatchDirs(1,0));
  }
    
  return 0;
}
