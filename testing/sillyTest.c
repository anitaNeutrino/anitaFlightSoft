#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>


#include <sys/types.h>

#include <time.h>


#include "anitaStructures.h"


#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"

void waitUntilNextSecond();
int main (void)
{
   /*  int status=0; */
/*     int thePort=0; */
/*     char* eString ; */
/*     char *fileName="anitaSoft.config"; */

/*     kvpReset () ; */
/*     status = configLoad (fileName,"global") ; */
/*     eString = configErrorString (status) ; */
/*     printf ("status %d (%s) from loading %s\n", */
/* 	    status, eString, fileName) ; */
/*     if (status == SOCKET_E_OK) { */
/* 	thePort= kvpGetInt ("portEventdPrioritzerd",0) ; */
/*     } */
/*     printf("\nThe port is: %d\n\n",thePort); */

/*     printf("Size of channel_data: %d\n",sizeof(channel_data)); */
/*     printf("Size of event_header: %d\n",sizeof(event_header)); */


/*     printf("%d %x %d %x %d %x %d %x %d %x\n",KVP_E_OK,KVP_E_OK,KVP_E_DUPLICATE,KVP_E_DUPLICATE,KVP_E_NOTFOUND,KVP_E_NOTFOUND,KVP_E_SYSTEM,KVP_E_SYSTEM,KVP_E_BADKEY,KVP_E_BADKEY); */
  

    waitUntilNextSecond();
}


void waitUntilNextSecond()
{
    time_t start;
    start=time(NULL);
    printf("Start time %d\n",start);

    while(start==time(NULL)) usleep(100);
    
}
