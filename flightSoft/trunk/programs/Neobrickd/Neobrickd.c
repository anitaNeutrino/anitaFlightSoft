/*! \file Neobrickd.c
  \brief The Neobrickd program that writes to the Neobrick

  Neobrickd looks in a to be determined directory for a to be determined file
  format that contains the address of a file to write to the Neobrick.
  July 2008  rjn@hep.ucl.ac.uk
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>



//Flightsoft Includes
#include "includes/anitaStructures.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "ftpLib/ftplib.h"
#include "pedestalLib/pedestalLib.h"
#include "compressLib/compressLib.h"
#include "linkWatchLib/linkWatchLib.h"
#include "linkWatchLib/inotify.h"

#define MAX_NEO_WATCHES 2000


// Directories and gubbins 
int printToScreen=0;
int verbosity=0;
int disableNeobrick=0;
int wdNeo=0;
int inotify_fd=0;
int runNumber=0;
char *watchDirArray[MAX_NEO_WATCHES]={0};


int addWatchDirRecursive(char *watchDir);
int setupRecursiveWatchDir(char *watchDir);
int checkDirsAndSendFiles(int timeoutSec, int timeOutUSec);
void makeFtpDirectories(char *theTmpDir);
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);
int readConfigFile();
int sendThisFile(char *tmpFilename); //Returns number of bytes
void replaceCurrentWithRun(char *runFilename, char *curFilename);



int main(int argc, char**argv) {
  int retVal;
  time_t lastStatusUpdate=0;
  time_t rawTime;
  unsigned int totalBytes=0;
  int theseBytes=0;
  struct timeval firstTime;
  struct timeval lastTime;

  
  // Log stuff 
  char *progName=basename(argv[0]);
  
  // Sort out PID File
  retVal=sortOutPidFile(progName);
  if(retVal!=0) {
    return retVal;
  }
  
  /* Set signal handlers */
  signal(SIGUSR1, sigUsr1Handler);
  signal(SIGUSR2, sigUsr2Handler);
  signal(SIGTERM, handleBadSigs);
  signal(SIGINT, handleBadSigs);
  signal(SIGSEGV, handleBadSigs);

  /* Setup log */
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
  

  makeDirectories(NEOBRICK_WATCH_DIR);    
  makeDirectories(LOGWATCH_LINK_DIR);
  

  retVal=readConfigFile();
  


  do {
    retVal=readConfigFile();
    if(printToScreen) 
	printf("Initializing Neobrickd -- disableNeobrick %d\n",disableNeobrick);

    if(!disableNeobrick) {
      // open the FTP connection   
      if (ftp_open("10.0.0.2", "anonymous", "")) {
	fprintf(stderr,"ftp_open -- %s\n",strerror(errno));
	syslog(LOG_ERR,"ftp_open -- %s\n",strerror(errno));
	syslog(LOG_ERR,"Disabling neobrick writing because ftp_open failed\nAll neobrick bound data will be deleted\n");	
	disableNeobrick=1;
	//Keep running to make sure we are deleting enobrick files
      }
    }
    
    runNumber=getRunNumber();

    //Now initialise the watch
    setupRecursiveWatchDir(NEOBRICK_WATCH_DIR);

    //About to start main event loop
    currentState=PROG_STATE_RUN;

    gettimeofday(&firstTime,NULL);

    while(currentState==PROG_STATE_RUN) {
      time(&rawTime);
      if(rawTime>lastStatusUpdate+30) {
	if(!disableNeobrick) {
	  printf("Getting status file\n");
	  ftp_getfile("/neobrick.status","/tmp/anita/neobrick.status",0);
	  telemeterFile("/tmp/anita/neobrick.status",0);
	}
	lastStatusUpdate=rawTime;
      }
		
      //Now all the magic is hidden inside this call to checkDirsAndSendFiles
      struct timeval before;
      struct timeval after;
      gettimeofday(&before,0);
      retVal=checkDirsAndSendFiles(1,0);
      gettimeofday(&after,0);
      if(retVal>0) {
	theseBytes=retVal;
	totalBytes+=retVal;
	float timeDiff=after.tv_sec-before.tv_sec;
	timeDiff+=1e-6*(after.tv_usec-before.tv_usec);
	printf("Sent %d bytes to Neobrick in %3.3f secs, rate %f bytes/s\n",
	       theseBytes,timeDiff,theseBytes/timeDiff);
	lastTime=after;
	timeDiff=lastTime.tv_sec-firstTime.tv_sec;
	timeDiff+=1e-6*(lastTime.tv_usec-firstTime.tv_usec);
	printf("Sent %d bytes to Neobrick in %3.3f secs, rate %f bytes/s\n",
	       totalBytes,timeDiff,totalBytes/timeDiff);

      }

  
    }
    ftp_close();
  } while(currentState==PROG_STATE_INIT);
    
  
  // the end 
  unlink(NEOBRICKD_PID_FILE);
  return 0;
}


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
  
  char ftpName[FILENAME_MAX];
  replaceCurrentWithRun(ftpName,watchDir);
  makeFtpDirectories(ftpName);



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
	//	printf("What is next_file %s\nSomething to send??\n",next_file);
	sendThisFile(next_file);
	unlink(next_file);
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


int checkDirsAndSendFiles(int timeoutSec, int timeOutUSec)
{
  //Checks all watch dirs
  // returns the number of bytes sent to the ftp server
  // 0 if there are no files to send
  int gotSomething=0;
  int countBytes=0;
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
	  //	  fprintf(stderr,"Got a file %s\n",newFile);
	  countBytes+=sendThisFile(newFile);
	  unlink(newFile);
	  free(newFile);
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
      //      printf("wd %d, name %s\n",event->wd,event->name);

      upto_byte+=sizeof(struct inotify_event)+event->len;
    }
    gotSomething=1;
  } while (1);
  
  if(gotSomething) {
    printf("Processed %d bytes\n",countBytes);
    return countBytes;
  }
  return 0;
}




int sendThisFile(char *tmpFilename)
// Returns the number of bytes sent
{   
  printf("sendThisFile -- %s\n",tmpFilename);
  if(disableNeobrick) return 0;
  char fileName[FILENAME_MAX];
  replaceCurrentWithRun(fileName,tmpFilename);
  //    if(printToScreen)
  //      printf("New file -- %s\n",fileName); 
  //  char *thisFile=basename(fileName);
  //  char *thisDir=dirname(fileName);
  //  makeFtpDirectories(thisDir);
  //  ftp_cd(thisDir);
  //    printf("%s -- %s\n",thisFile,thisDir);
  unsigned int bufSize=0;
  //  struct timeval before;
  //  struct timeval after;
  
  //  gettimeofday(&before,NULL);
  ftp_putfile(tmpFilename,fileName,0,0,&bufSize);
  //  gettimeofday(&after,NULL);
  
  //  float timeDiff=after.tv_sec-before.tv_sec;
  //  timeDiff+=1e-6*(after.tv_usec-before.tv_usec);
  //    printf("Sent %d bytes to Neobrick in %3.3f secs, rate %f bytes/s\n",
  //	   bufSize,timeDiff,bufSize/timeDiff);
  return bufSize;
 
}


void makeFtpDirectories(char *theTmpDir)
{
  printf("makeFtpDirectories -- %s\n",theTmpDir);
  if(disableNeobrick) return;
    char copyDir[FILENAME_MAX];
    char newDir[FILENAME_MAX];
    char *subDir;
    int retVal;
    
    strncpy(copyDir,theTmpDir,FILENAME_MAX);

    strcpy(newDir,"");

    subDir = strtok(copyDir,"/");
    while(subDir != NULL) {
	sprintf(newDir,"%s/%s",newDir,subDir);
	retVal=ftp_mkdir(newDir);
// 	    printf("%s\t%s\n",theTmpDir,newDir);
/* 	    printf("Ret Val %d\n",retVal); */
	subDir = strtok(NULL,"/");
    }
       
}


void handleBadSigs(int sig)
{
  
    fprintf(stderr,"Received sig %d -- will exit immeadiately\n",sig); 
    syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig); 
    unlink(NEOBRICKD_PID_FILE);
    syslog(LOG_INFO,"Neobrickd terminating");    
    exit(0);
}


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(NEOBRICKD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,NEOBRICKD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(NEOBRICKD_PID_FILE);
  return 0;
}

int readConfigFile() 
/* Load Neobrickd config stuff */
{
    /* Config file thingies */
    int status=0;
    char* eString ;

    kvpReset();
    status = configLoad ("Neobrickd.config","output") ;
    status = configLoad ("anitaSoft.config","global") ;
    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",0);
	verbosity=kvpGetInt("verbosity",0);
	disableNeobrick=kvpGetInt("disableNeobrick",0);	
    }
    else {
      eString=configErrorString (status) ;
      syslog(LOG_ERR,"Error reading Neobrickd.config: %s\n",eString);
      fprintf(stderr,"Error reading Neobrickd.config: %s\n",eString);
      
    }
    
    return status;
}



void replaceCurrentWithRun(char *runFilename, char *curFilename)
{
  sprintf(runFilename,"/data/run%d/%s",runNumber,&curFilename[21]);    
}
