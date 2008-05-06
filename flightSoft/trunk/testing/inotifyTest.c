#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>


#include <sys/types.h>

#include <time.h>

//inotify includes
#include <inotifytools/inotifytools.h>
#include <inotifytools/inotify.h>


#include "includes/anitaStructures.h"


#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"


int main (int argc, char **argv)
{    
  int events=0;
  if(!inotifytools_initialize()) {
    fprintf(stderr,"Couldn't initialize inotify\n");
    return -1;
  }
  //Initialize stats
  inotifytools_initialize_stats();

  // Attempt to watch file
  // If events is still 0, make it all events.
  events=IN_CREATE;
  if (events == 0)
    events = IN_ALL_EVENTS;
  int retVal=inotifytools_watch_file( "/tmp", events );
  if(!retVal) {
    fprintf(stderr,"Error watching file /tmp\n");
    return -1;
  }
  fprintf( stderr, "Watches established.\n" );

  struct inotify_event * event;
  unsigned long int timeout = 1;
  int keepGoing=1;
  while(keepGoing) {
    printf("inotifytools_get_stat_total:\t%d\n",inotifytools_get_stat_total(events));
    event = inotifytools_next_event( timeout );
    if ( !event ) {
      if ( !inotifytools_error() ) {
      fprintf(stderr,"Timed out after %d seconds\n",timeout);
      continue;
      return -1;
      }
      else {
	fprintf(stderr, "%s\n", strerror( inotifytools_error() ) );
	return -2;
      }
    }
    inotifytools_printf( event, "%w %,e %f\n" );
    sleep(1);
    //    keepGoing=0;
  }
    
    



  return 0;
}
