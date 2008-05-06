/*! \file linkWatchLib.h
  \brief Utility library to help ease the watching of link directories
    
  April 2008, rjn@hep.ucl.ac.uk
*/


#ifndef LINKWATCHLIB_H
#define LINKWATCHLIB_H


/* Includes */
#include "includes/anitaStructures.h"
#include <time.h>
#include <zlib.h>
#include <stdio.h>


int setupLinkWatchDir(char *linkDir); //returns -1 for failure otherwise it returns the watch index number
int checkLinkDirs(int timeout); //Returns 1 if there is new data, zero otherwise
int getNumLinks(int watchNumber); //returns number of links, zero, -1 for failure

char *getFirstLink(int watchNumber); //returns the filename of the first link
char *getLastLink(int watchNumber); //returns the filename of the most recent link (no idea how)



#endif /* LINKWATCHLIB_H */
