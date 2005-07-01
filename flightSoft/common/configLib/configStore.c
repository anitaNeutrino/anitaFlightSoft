/* configStore.c - <short description of this file/module> */

/* G.J.Crone, University College London */

/*
 * Current CVS Tag:
 * $Header: /work1/anitaCVS/flightSoft/common/configLib/configStore.c,v 1.4 2005/06/15 16:22:00 rjn Exp $
 */
 
/* 
 * Modification History : DO NOT EDIT - MAINTAINED BY CVS 
 * $Log: configStore.c,v $
 * Revision 1.4  2005/06/15 16:22:00  rjn
 * Fixed a lot of silly warnings
 *
 * Revision 1.3  2004/10/13 20:27:37  rjn
 * Added Mike's GPS code to the repository. And fixed a couple of minor bugs.
 *
 * Revision 1.2  2004/08/31 17:51:21  rjn
 * Stuck a load of syslog messages into the configLib stuff. Previosuly it used some strange wrapper around syslog, due to implementation issues on the operating system used by some parts of the MINOS daq.
 *
 * Revision 1.1  2004/08/31 15:48:09  rjn
 * Um... lots of additions. The biggest ones are switching from shared to static libraries, and the additon of configLib and kvpLib to read formatted config files. These two libraries were created by Gordon Crone (gjc@hep.ucl.ac.uk) and are in spirit released under the GPL license.
 * Also the first of the actually programs (Eventd) has been started. At the moment it just opens a socket to fakeAcqd. Other sockets and better handling of multiple sockets in socketLib are the next things on my to do list.
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
#include <stdlib.h>
#include <syslog.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"

/* defines */

/* typedefs */

/* globals */   

/* locals */

/* forward declarations */

/********************************************************************
*
* storeConfig - Write current contents of kvp buffer to file
*
* <Insert longer description here>
*
* RETURNS: 0 => success,  -1 => failure
*
*/
ConfigErrorCode configStore (
                             char* fileName,
                             char* blockName
                             )
{
   int config ;
   //   char* errMessage ;
   char *configPath = 0 ;
   struct passwd *myPwent ;
   int myUid ;
   struct stat fileStat ;
   char fileSpec[FILENAME_MAX] ;
   int status ;
   char tag[BLOCKNAME_MAX+3] ;

   configPath = getenv ("ANITA_CONFIG_DIR") ;
   if (configPath == NULL) {
      /* Environment variable not set so use current dir if there's
         already a config file here */
      if (stat (fileName, &fileStat) == 0) {
         configPath = "./" ;
      }
      else {
         /* Finally default to home directory */
         myUid = getuid () ;
         myPwent = getpwuid (myUid) ;
         configPath = myPwent->pw_dir ;
      }
   }

   strcpy (fileSpec, configPath) ;
   strcat (fileSpec, "/") ;
   strcat (fileSpec, fileName) ;
   syslog(LOG_INFO,"storeConfig writing params to: %s", fileSpec) ;

   config = creat (fileSpec, 0664) ;
   if (config == -1) {
      syslog (LOG_ERR,"storeConfig: error %d creating %s", errno, fileSpec) ;
      return (CONFIG_E_SYSTEM) ;
   }
   else {
      snprintf (tag, BLOCKNAME_MAX+2, "<%s>\n", blockName) ;
      write (config, tag, strlen (tag)) ;
      status = kvpWrite (config) ;
      if (status == -1) {
	  /*errMessage = kvpErrorString (kvpError()) ;*/
         syslog (LOG_ERR,"storeConfig: kvpWrite failed %s",kvpErrorString (kvpError()) ) ;
         return (CONFIG_E_KVP) ;
      }
      snprintf (tag, BLOCKNAME_MAX+3, "\n</%s>\n", blockName) ;
      write (config, tag, strlen (tag)) ;
      return (CONFIG_E_OK) ;
   }
}

