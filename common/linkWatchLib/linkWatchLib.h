/*! \file linkWatchLib.h
  \brief Utility library to help ease the watching of link directories
    
  April 2008, rjn@hep.ucl.ac.uk
*/


#ifndef LINKWATCHLIB_H
#define LINKWATCHLIB_H


// System Includes
#include <time.h>
#include <zlib.h>
#include <stdio.h>
#ifndef __CINT__
#include <dirent.h>
#endif


int setupLinkWatchDir(char *linkDir); //returns -1 for failure otherwise it returns the watch index number
int checkLinkDirs(int timeout); //Returns 1 if there is new data, zero otherwise
int getNumLinks(int watchNumber); //returns number of links, zero, -1 for failure

char *getFirstLink(int watchNumber); //returns the filename of the first link
char *getLastLink(int watchNumber); //returns the filename of the most recent link (no idea how)


//Internal function that uses linked list to keep track of the links
void prepLinkList(int watchIndex);
void addLink(int watchIndex, char *linkName); 
int getWatchIndex(int watchNumber);


//More internal functions
#ifndef __CINT__

int filterHeaders(const struct dirent *dir);
int filterOnDats(const struct dirent *dir);
int filterOnGzs(const struct dirent *dir);
int getListofLinks(const char *theEventLinkDir, struct dirent ***namelist);
int getListofPurgeFiles(const char *theEventLinkDir, 
			struct dirent ***namelist);

#endif

#endif /* LINKWATCHLIB_H */
