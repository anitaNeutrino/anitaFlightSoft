/*! \file socketLib.c
    \brief Utility library, full of (hopefully) useful stuff
    
    Some functions that do some useful things
    October 2004  rjn@mps.ohio-state.edu
*/

#include "utilLib/utilLib.h"

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/dir.h>
#include <errno.h>

extern  int versionsort(const void *a, const void *b);


void makeDirectories(char *theTmpDir) 
{
    char theCommand[FILENAME_MAX+20];
    char copyDir[FILENAME_MAX];
    char newDir[FILENAME_MAX];
    char *subDir;
    int retVal;
    
    strncpy(copyDir,theTmpDir,FILENAME_MAX);

    strcpy(theCommand,"mkdir ");
    strcpy(newDir,"");

    subDir = strtok(copyDir,"/");
    while(subDir != NULL) {
	sprintf(theCommand,"%s/%s",theCommand,subDir);
	sprintf(newDir,"%s/%s",newDir,subDir);
	retVal=is_dir(newDir);
	if(!retVal) {	
	    retVal=system(theCommand);
/* 	    printf("%s\n",theCommand); */
/* 	    printf("Ret Val %d\n",retVal); */
	}
	subDir = strtok(NULL,"/");
    }
       

}

int makeLink(const char *theFile, const char *theLinkDir)
{
    char theCommand[2*FILENAME_MAX];
    int retVal;
    sprintf(theCommand,"ln -s %s %s",theFile,theLinkDir);
    retVal=system(theCommand);
    if(retVal!=0) {
	syslog(LOG_ERR,"%s returned %d",theCommand,retVal);
    }
    return retVal;    
}


int moveFile(const char *theFile, const char *theDir)
{
    char theCommand[2*FILENAME_MAX];
    int retVal;
    sprintf(theCommand,"mv %s %s",theFile,theDir);
    retVal=system(theCommand);
    if(retVal!=0) {
	syslog(LOG_ERR,"%s returned %d",theCommand,retVal);
    }
    return retVal;    
}


int removeFile(const char *theFile)
{
    char theCommand[FILENAME_MAX];
    int retVal;
    sprintf(theCommand,"rm %s",theFile);
    retVal=system(theCommand);
    if(retVal!=0) {
	syslog(LOG_ERR,"%s returned %d",theCommand,retVal);
    }
    return retVal;    
}

int is_dir(const char *path)
{
    struct stat s;
 
    if (stat(path, &s))
	return 0;
 
    return S_ISDIR(s.st_mode) ? 1 : 0;
}


int getListofLinks(const char *theEventLinkDir, struct dirent ***namelist)
{
/*     int count; */
    int n = scandir(theEventLinkDir, namelist, filterOnDats, versionsort);
    if (n < 0) {
	syslog(LOG_ERR,"scandir %s: %s",theEventLinkDir,strerror(errno));
    }	
 /*    for(count=0;count<n;count++)  */
/* 	printf("%s\n",(*namelist)[count]->d_name); */
    return n;	    
}



int filterHeaders(const struct dirent *dir)
{
    if(strncmp(dir->d_name,"hd",2)==0)
	return 1;
    return 0;
}


int filterOnDats(const struct dirent *dir)
{

    if(strstr(dir->d_name,".dat")!=NULL)
	return 1;
    return 0;
}


int fillCalibStruct(calibStruct *theStruct, char *filename)
{
    /* Takes a pointer to the next struct in the array */
    
    FILE * pFile;

    pFile = fopen (filename, "r");
    if(pFile == NULL) {
	syslog (LOG_ERR,"Couldn't open file: %s\n",filename);
	return 0;
    }
    fscanf(pFile,"%d %c",&(theStruct->unixTime),&(theStruct->status));
    fclose (pFile); 
    return 0;
}


int fillHeader(anita_event_header *theEventHdPtr, char *filename)
{
    /* Returns 0 if successful */
    int numObjs;
    FILE * pFile;
        
    pFile = fopen (filename, "rb");
    if(pFile == NULL) {
	syslog (LOG_ERR,"Couldn't open file: %s\n",filename);
	return 1;
    }
    numObjs=fread(theEventHdPtr,sizeof(anita_event_header),1,pFile);
/*     printf("Read %d objects from %s\n",numObjs,filename); */
    fclose (pFile); 
    if(numObjs==1) return 0; /*Success*/
    return 1;
}


int fillBody(anita_event_body *theEventBodyPtr, char *filename)
{
    /* Returns 0 if successful */
    int numObjs;
    FILE * pFile;
        
    pFile = fopen (filename, "rb");
    if(pFile == NULL) {
	syslog (LOG_ERR,"Couldn't open file: %s\n",filename);
	return 1;
    }
    numObjs=fread(theEventBodyPtr,sizeof(anita_event_body),1,pFile);
/*     printf("Read %d objects from %s\n",numObjs,filename); */
    fclose (pFile); 
    if(numObjs==1) return 0; /*Success*/
    return 1;
}


int fillGpsStruct(gpsSubTimeStruct *theGpsStruct, char *filename)
{
    /* Takes a pointer to the next struct in the array */
    /* Returns number of lines read*/
    int numLines=0;
    FILE * pFile;
    int unixTime,subTime;
    
    pFile = fopen (filename, "r");
    if(pFile == NULL) {
	syslog (LOG_ERR,"Couldn't open file: %s\n",filename);
	return 0;
    }
    while(fscanf(pFile,"%d %d",&unixTime,&subTime)!=EOF) {
	theGpsStruct[numLines].unixTime=unixTime;
	theGpsStruct[numLines].subTime=subTime;	
	numLines++;
    }
    fclose (pFile); 
    return numLines;
}


int writeHeader(anita_event_header *hdPtr, char *filename)
/* Writes the header pointed to by hdPtr to filename */
{
    int numObjs;
    FILE *pFILE = fopen (filename, "wb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fwrite(hdPtr,sizeof(anita_event_header),1,pFILE);
    fclose(pFILE);
    return 0;
}

int writeBody(anita_event_body *bodyPtr, char *filename)
/* Writes the body pointed to by bodyPtr to filename */
{
    int numObjs;
    FILE *pFILE = fopen (filename, "wb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fwrite(bodyPtr,sizeof(anita_event_body),1,pFILE);
    fclose(pFILE);
    return 0;
}
