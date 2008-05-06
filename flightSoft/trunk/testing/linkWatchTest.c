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
#include "linkWatchLib/linkWatchLib.h"


int main (int argc, char **argv)
{    
  int wd=setupLinkWatchDir("/tmp/testDir/");
  while(1) {
    int retVal=checkLinkDirs(1); //Timeout of 1 second
    if(retVal) { //New stuff
      int numLinks=getNumLinks(wd);
      fprintf(stderr,"There are %d links\n",numLinks);
    }
    usleep(30000);
  }

  return 0;
}
