/* validate.c - <short description of this file/module> */

/* G.J.Crone, University College London */

/*
 * Current CVS Tag:
 * $Header: /work1/anitaCVS/flightSoft/common/configLib/test/validate.c,v 1.1 2004/08/31 15:48:09 rjn Exp $
 */
 
/* 
 * Modification History : DO NOT EDIT - MAINTAINED BY CVS 
 * $Log: validate.c,v $
 * Revision 1.1  2004/08/31 15:48:09  rjn
 * Um... lots of additions. The biggest ones are switching from shared to static libraries, and the additon of configLib and kvpLib to read formatted config files. These two libraries were created by Gordon Crone (gjc@hep.ucl.ac.uk) and are in spirit released under the GPL license.
 * Also the first of the actually programs (Eventd) has been started. At the moment it just opens a socket to fakeAcqd. Other sockets and better handling of multiple sockets in socketLib are the next things on my to do list.
 *
 * Revision 1.1  2001/08/17 16:33:02  gjc
 * First version of program to check validity of cinfig files.
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
#include <string.h>
#include <errno.h>

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"

/* defines */

/* typedefs */

/* globals */   

/* locals */

/* forward declarations */
int main (int argc, char* argv[])
{
   ConfigErrorCode status ;
   char* eString ;
   FILE* validation ;
   char* fileSpec ;
   char  filename[FILENAME_MAX] ;
   char  validationFilename[FILENAME_MAX] ;
   char  buffer[1024] ;
   char* tPtr ;
   char* vPtr ;
   int errors ;

   if (argc < 2) {
      printf ("Usage:  %s  <filename>\n", argv[0]) ;
      return (EXIT_FAILURE) ;
   }

   strncpy (filename, argv[1], FILENAME_MAX) ;

   /* First just go through the file and check that all sections parse */
   status = configValidate (filename) ;
   if (status != CONFIG_E_OK) {
      eString = configErrorString (status) ;
      printf ("status %d (%s) validating %s\n",
              status, eString, argv[1]) ;

     return (EXIT_FAILURE) ;
   }

   errors = 0 ;
   /* Now check the specifics for this config file */
   strncpy (validationFilename, filename, FILENAME_MAX) ;
   strncat (validationFilename, ".validation", FILENAME_MAX) ;
   fileSpec = configFileSpec (validationFilename) ;
   validation = fopen (fileSpec, "r") ;
   if (validation == NULL) {
      if (errno == ENOENT) {
         printf ("validation specification file %s missing,"
                 " no further checks possible.\n",
                 fileSpec) ;
         return (EXIT_SUCCESS) ;
      }
      else {
         perror ("fopen") ;
         return (EXIT_FAILURE) ;
      }
   }
   tPtr = fgets (buffer, sizeof(buffer), validation) ;
   if (tPtr != NULL) {
      tPtr[strlen(tPtr)-1] =  0 ;
      configLoad (filename, buffer) ;
      while (tPtr != NULL) {
         tPtr = fgets (buffer, sizeof(buffer), validation) ;
         if ((tPtr != NULL) && (strlen (tPtr) > 1)) {
            tPtr[strlen(tPtr)-1] =  0 ;
            vPtr = kvpGetString (buffer) ;
            if (vPtr == NULL) {
               printf ("Error required value <%s> not found\n",
                       buffer) ;
               errors++ ;
            }
         }
      }
   }
   if (errors == 0) {
      printf ("%d errors found\n", errors) ;
      return (EXIT_FAILURE) ;
   }
   else {
      return (EXIT_SUCCESS) ;
   }
}
