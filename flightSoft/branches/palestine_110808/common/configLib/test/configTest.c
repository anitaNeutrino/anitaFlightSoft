/* configTest.c - simple test program for config library */

/* G.J.Crone, University College London */

/*
 * Current CVS Tag:
 * $Header: /work1/anitaCVS/flightSoft/common/configLib/test/configTest.c,v 1.2 2004/08/31 17:51:21 rjn Exp $
 */
 
/* 
 * Modification History : DO NOT EDIT - MAINTAINED BY CVS 
 * $Log: configTest.c,v $
 * Revision 1.2  2004/08/31 17:51:21  rjn
 * Stuck a load of syslog messages into the configLib stuff. Previosuly it used some strange wrapper around syslog, due to implementation issues on the operating system used by some parts of the MINOS daq.
 *
 * Revision 1.1  2004/08/31 15:48:09  rjn
 * Um... lots of additions. The biggest ones are switching from shared to static libraries, and the additon of configLib and kvpLib to read formatted config files. These two libraries were created by Gordon Crone (gjc@hep.ucl.ac.uk) and are in spirit released under the GPL license.
 * Also the first of the actually programs (Eventd) has been started. At the moment it just opens a socket to fakeAcqd. Other sockets and better handling of multiple sockets in socketLib are the next things on my to do list.
 *
 * Revision 1.3  2002/06/12 12:22:43  gjc
 * Changed loading message from info to debug on level 1
 *
 * Revision 1.2  2001/08/17 16:30:53  gjc
 * Return an error if a value is given outside the context of any section.
 *
 * Revision 1.1  2001/07/24 13:36:44  gjc
 * First check in of new package
 *
 *
 */ 

/*
DESCRIPTION
<Insert a description of the file/module here>
INCLUDE FILES: <place list of any relevant header files here>
*/

/* includes */
#include <stdio.h>
#include <syslog.h>

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"

/* defines */

/* typedefs */

/* globals */   

/* locals */

/* forward declarations */
int testConfig (
                char* fileName,
                char* sections
                )
{
   int status ;
   char* eString ;

   kvpReset () ;
   printf ("\nAttempting to load %s sections from %s\n",
           sections, fileName) ;
   status = configLoad (fileName, sections) ;
   eString = configErrorString (status) ;
   printf ("status %d (%s) from loading %s\n",
           status, eString, fileName) ;

   return (status) ;
}

int main (int argc, char* argv[])
{
   int status ;
   int passed ;
   char* gp0 ;
   char* gp1 ;
   char* op0 ;
   int ntests ;

   openlog ("configTest", LOG_PID, LOG_LOCAL4) ;

   passed = 0 ;
   ntests = 0 ;

   /*logDebugLevelSet (1) ;*/

   status = testConfig ("goodConfig", "global,other") ;
   ntests++ ;
   if (status == CONFIG_E_OK) {
      passed++ ;
      gp0 = kvpGetString ("gParam0") ;
      gp1 = kvpGetString ("gParam1") ;
      op0 = kvpGetString ("oParam0") ;
      printf ("gParam0 = <%s>,  gParam1 = <%s>,  oParam0 = <%s>\n",
              gp0, gp1, op0) ;
   }

   printf ("\nWriting merged blocks into mergedConfig\n") ;
   status = configStore ("mergedConfig", "merged") ;
   ntests++ ;
   if (status == CONFIG_E_OK) {
      passed++ ;
   }

   status = testConfig ("mergedConfig", "merged") ;
   ntests++ ;
   if (status == CONFIG_E_OK) {
      passed++ ;
      gp0 = kvpGetString ("gParam0") ;
      gp1 = kvpGetString ("gParam1") ;
      op0 = kvpGetString ("oParam0") ;
      printf ("gParam0 = <%s>,  gParam1 = <%s>,  oParam0 = <%s>\n",
              gp0, gp1, op0) ;
   }

   printf ("switching off debug\n");
/*   logDebugLevelSet (0);*/
   status = testConfig ("goodConfig", "global,local") ;
   ntests++ ;
   if (status == CONFIG_E_SECTION) {
      passed++ ;
   }

   status = testConfig ("badConfig", "global,junk") ;
   ntests++ ;
   printf ("x=%d\n", kvpGetInt ("x",666)) ;
   if (status == CONFIG_E_UNNAMED) {
      passed++ ;
   }


   status = testConfig ("noCloseConfig", "global,other") ;
   ntests++ ;
   if (status == CONFIG_E_EOF) {
      passed++ ;
   }

   status = testConfig ("nonexistantConfig", "global,other") ;
   ntests++ ;
   if (status == CONFIG_E_NOFILE) {
      passed++ ;
   }

   status = testConfig ("bigConfig", "global,other") ;
   ntests++ ;
   if (status == CONFIG_E_NOFILE) {
      passed++ ;
   }

   if (passed == ntests) {
      printf ("\n\nAll passed OK\n") ;
   }

   return (0) ;
}
