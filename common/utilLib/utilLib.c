/*! \file utilLib.c
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
#include <libgen.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <zlib.h>
#include <fcntl.h>
#define NO_ZLIB

#define OPEN_CLOSE_ALL_THE_TIME

extern  int versionsort(const void *a, const void *b);

int cleverHkWrite(char *buffer, int numBytes, unsigned long unixTime, AnitaWriterStruct_t *awsPtr)
{
    int retVal=0;
    static int errorCounter=0;
    char *tempDir;
    pid_t childPid;
    char *gzipArg[] = {"gzip",awsPtr->currentFileName,(char*)0};
    
    if(!awsPtr->currentFilePtr) {
	//First time
	tempDir=getCurrentHkDir(awsPtr->baseDirname,unixTime);
	strncpy(awsPtr->currentDirName,tempDir,FILENAME_MAX-1);
	awsPtr->dirCount=0;
	free(tempDir);
	tempDir=getCurrentHkDir(awsPtr->currentDirName,unixTime);
	strncpy(awsPtr->currentSubDirName,tempDir,FILENAME_MAX-1);
	awsPtr->fileCount=0;
	free(tempDir);
	
	tempDir=getCurrentHkFilename(awsPtr->currentSubDirName,
				     awsPtr->filePrefix,unixTime);
	strncpy(awsPtr->currentFileName,tempDir,FILENAME_MAX-1);
	free(tempDir);
	awsPtr->currentFilePtr=fopen(awsPtr->currentFileName,"wb");	    
	awsPtr->writeCount=0;						      
    }
    
    //Check if we need a new file or directory
    if(awsPtr->writeCount>=awsPtr->maxWritesPerFile) {
	//Need a new file
	fclose(awsPtr->currentFilePtr);
	childPid=fork();
	if(childPid==0) {
	    //Child
//	    awsPtr->currentFileName;
	    execv("/bin/gzip",gzipArg);
	    exit(0);
	}
	else if(childPid<0) {
	    //Something wrong
	    syslog(LOG_ERR,"Couldn't zip file %s\n",awsPtr->currentFileName);
	    fprintf(stderr,"Couldn't zip file %s\n",awsPtr->currentFileName);
	}
	
	

	awsPtr->fileCount++;
	if(awsPtr->fileCount>=awsPtr->maxFilesPerDir) {
	    awsPtr->dirCount++;
	    if(awsPtr->dirCount>=awsPtr->maxSubDirsPerDir) {
		tempDir=getCurrentHkDir(awsPtr->baseDirname,unixTime);
		strncpy(awsPtr->currentDirName,tempDir,FILENAME_MAX-1);
		awsPtr->dirCount=0;
		free(tempDir);
	    }
	    
	    tempDir=getCurrentHkDir(awsPtr->currentDirName,unixTime);
	    strncpy(awsPtr->currentSubDirName,tempDir,FILENAME_MAX-1);
	    awsPtr->fileCount=0;
	    free(tempDir);
	}
	
	tempDir=getCurrentHkFilename(awsPtr->currentSubDirName,
				     awsPtr->filePrefix,unixTime);
	strncpy(awsPtr->currentFileName,tempDir,FILENAME_MAX-1);
	free(tempDir);
	awsPtr->currentFilePtr=fopen(awsPtr->currentFileName,"wb");	    

	awsPtr->writeCount=0;						      
    }

    awsPtr->writeCount++;
    if(errorCounter<100) {
	retVal=fwrite(buffer,numBytes,1,awsPtr->currentFilePtr);
	if(retVal<0) {
	    errorCounter++;
	    printf("Error (%d of 100) writing to file (write %d) -- %s (%d)\n",
		   errorCounter,awsPtr->writeCount,
		   strerror(errno),retVal);
	}
	retVal=fflush(awsPtr->currentFilePtr);  
//	printf("Here %d -- fflush RetVal %d \n",awsPtr->writeCount,
//	       retVal);
    }
    else return -1;
	        	       
    return retVal;

    
}

int cleverRawEventWrite(AnitaEventBody_t *bdPtr,AnitaEventHeader_t *hdPtr, AnitaEventWriterStruct_t *awsPtr)
{
    int retVal=0;
    static int errorCounter=0;
    int dirNum;
//    int numberOfBytes;
//    char *bufferToZip;
//    char zippedFilename[FILENAME_MAX];
    
    if(!awsPtr->currentEventFilePtr) {
	//First time

	//Make base dir
	dirNum=(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir*awsPtr->maxSubDirsPerDir)*(hdPtr->eventNumber/(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir*awsPtr->maxSubDirsPerDir));
	sprintf(awsPtr->currentDirName,"%s/ev%d",awsPtr->baseDirname,dirNum);
	makeDirectories(awsPtr->currentDirName);
	
	awsPtr->fileCount=0;

	//Make sub dir
	dirNum=(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir)*(hdPtr->eventNumber/(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir));
	sprintf(awsPtr->currentSubDirName,"%s/ev%d",awsPtr->currentDirName,dirNum);
	makeDirectories(awsPtr->currentSubDirName);

	awsPtr->dirCount=0;

	//Make files
	dirNum=(awsPtr->maxWritesPerFile)*(hdPtr->eventNumber/awsPtr->maxWritesPerFile);
	sprintf(awsPtr->currentHeaderFileName,"%s/hd_%d.dat.gz",
		awsPtr->currentSubDirName,dirNum);
	awsPtr->currentHeaderFilePtr=gzopen(awsPtr->currentHeaderFileName,"wb5");

	awsPtr->writeCount=0;
	if(awsPtr->currentHeaderFilePtr<0) {	    
	    if(errorCounter<100) {
		errorCounter++;
		printf("Error (%d of 100) trying to open file %s\n",
		       errorCounter,awsPtr->currentHeaderFileName);
	    }
	}

	sprintf(awsPtr->currentEventFileName,"%s/ev_%d.dat.gz",
		awsPtr->currentSubDirName,dirNum);
	awsPtr->currentEventFilePtr=
	    gzopen(awsPtr->currentEventFileName,"wb5");
	if(awsPtr->currentEventFilePtr<0) {	    
	    if(errorCounter<100) {
		errorCounter++;
		printf("Error (%d of 100) trying to open file %s\n",
		       errorCounter,awsPtr->currentEventFileName);
	    }
	}
    }
    if(awsPtr->currentEventFilePtr && awsPtr->currentHeaderFilePtr) {
	if(awsPtr->writeCount>=awsPtr->maxWritesPerFile) {
#ifndef OPEN_CLOSE_ALL_THE_TIME // In NOT defined
	    gzclose(awsPtr->currentEventFilePtr);
	    gzclose(awsPtr->currentHeaderFilePtr);
#endif

	    awsPtr->fileCount++;
	    if(awsPtr->fileCount>=awsPtr->maxFilesPerDir) {
		awsPtr->dirCount++;
		if(awsPtr->dirCount>=awsPtr->maxSubDirsPerDir) {
		    //Make base dir
		    dirNum=(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir*awsPtr->maxSubDirsPerDir)*(hdPtr->eventNumber/(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir*awsPtr->maxSubDirsPerDir));
		    sprintf(awsPtr->currentDirName,"%s/ev%d",awsPtr->baseDirname,dirNum);
		    makeDirectories(awsPtr->currentDirName);
		    awsPtr->dirCount=0;
		}
		
		//Make sub dir
		dirNum=(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir)*(hdPtr->eventNumber/(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir));
		sprintf(awsPtr->currentSubDirName,"%s/ev%d",awsPtr->currentDirName,dirNum);
		makeDirectories(awsPtr->currentSubDirName);
		awsPtr->fileCount=0;
	    }
	
	    //Make files
	    dirNum=(awsPtr->maxWritesPerFile)*(hdPtr->eventNumber/awsPtr->maxWritesPerFile);
	    sprintf(awsPtr->currentHeaderFileName,"%s/hd_%d.dat.gz",
		    awsPtr->currentSubDirName,dirNum);
//	    printf("Trying to open %s\n",awsPtr->currentHeaderFileName);
	    awsPtr->currentHeaderFilePtr=gzopen(awsPtr->currentHeaderFileName,"wb5");
	    if(awsPtr->currentHeaderFilePtr<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentHeaderFileName);
		}
	    }
	    
	    sprintf(awsPtr->currentEventFileName,"%s/ev_%d.dat.gz",
		    awsPtr->currentSubDirName,dirNum);
	    awsPtr->currentEventFilePtr=gzopen(awsPtr->currentEventFileName,"wb5");
	    if(awsPtr->currentEventFilePtr<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentEventFileName);
		}
	    }
	    awsPtr->writeCount=0;
	}
#ifdef OPEN_CLOSE_ALL_THE_TIME
	else {
	    awsPtr->currentEventFilePtr=gzopen(awsPtr->currentEventFileName,"ab5");	    
	    if(awsPtr->currentHeaderFilePtr<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentHeaderFileName);
		}
	    }
	    awsPtr->currentHeaderFilePtr=gzopen(awsPtr->currentHeaderFileName,"ab5");	    
	    if(awsPtr->currentEventFilePtr<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentEventFileName);
		}
	    }
	}
#endif

	awsPtr->writeCount++;
	if(errorCounter<100 && awsPtr->currentEventFilePtr && awsPtr->currentHeaderFilePtr) {
//	    retVal=fwrite(hdPtr,sizeof(AnitaEventHeader_t),1,awsPtr->currentHeaderFilePtr);
	    retVal=gzwrite(awsPtr->currentHeaderFilePtr,hdPtr,sizeof(AnitaEventHeader_t));
	    if(retVal<0) {
		errorCounter++;
		printf("Error (%d of 100) writing to file (write %d) -- %s (%d)\n",
		       errorCounter,awsPtr->writeCount,
		       gzerror(awsPtr->currentHeaderFilePtr,&retVal),retVal);
	    }
//	    else {
//		fflush(awsPtr->currentHeaderFilePtr);  
#ifdef OPEN_CLOSE_ALL_THE_TIME
	    gzclose(awsPtr->currentHeaderFilePtr);
#endif
//	    }

//	    retVal=fwrite(bdPtr,sizeof(AnitaEventBody_t),1,awsPtr->currentEventFilePtr);
	    retVal=gzwrite(awsPtr->currentEventFilePtr,bdPtr,sizeof(AnitaEventBody_t));
	    if(retVal<0) {
		errorCounter++;
		printf("Error (%d of 100) writing to file (write %d) -- %s (%d)\n",
		       errorCounter,awsPtr->writeCount,
		       gzerror(awsPtr->currentEventFilePtr,&retVal),retVal);
	    }
//	    else {
//		fflush(awsPtr->currentEventFilePtr);  
#ifdef OPEN_CLOSE_ALL_THE_TIME	   
	    gzclose(awsPtr->currentEventFilePtr);
#endif
//	    }
	    

	}
	else return -1;
	
	
    }
    return -1;


}

int cleverEncEventWrite(char *outputBuffer, int numBytes,AnitaEventHeader_t *hdPtr, AnitaEventWriterStruct_t *awsPtr)
{
    int retVal=0;
    static int errorCounter=0;
    int dirNum;
//    int numberOfBytes;
//    char *bufferToZip;
//    char zippedFilename[FILENAME_MAX];
    
    if(!awsPtr->currentEventFilePtr) {
	//First time

	//Make base dir
	dirNum=(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir*awsPtr->maxSubDirsPerDir)*(hdPtr->eventNumber/(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir*awsPtr->maxSubDirsPerDir));
	sprintf(awsPtr->currentDirName,"%s/ev%d",awsPtr->baseDirname,dirNum);
	makeDirectories(awsPtr->currentDirName);
	
	awsPtr->fileCount=0;

	//Make sub dir
	dirNum=(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir)*(hdPtr->eventNumber/(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir));
	sprintf(awsPtr->currentSubDirName,"%s/ev%d",awsPtr->currentDirName,dirNum);
	makeDirectories(awsPtr->currentSubDirName);

	awsPtr->dirCount=0;

	//Make files
	dirNum=(awsPtr->maxWritesPerFile)*(hdPtr->eventNumber/awsPtr->maxWritesPerFile);
	sprintf(awsPtr->currentHeaderFileName,"%s/hd_%d.dat.gz",
		awsPtr->currentSubDirName,dirNum);
	awsPtr->currentHeaderFilePtr=gzopen(awsPtr->currentHeaderFileName,"wb5");

	awsPtr->writeCount=0;
	if(awsPtr->currentHeaderFilePtr<0) {	    
	    if(errorCounter<100) {
		errorCounter++;
		printf("Error (%d of 100) trying to open file %s\n",
		       errorCounter,awsPtr->currentHeaderFileName);
	    }
	}

	sprintf(awsPtr->currentEventFileName,"%s/encev_%d.dat.gz",
		awsPtr->currentSubDirName,dirNum);
	awsPtr->currentEventFilePtr=
	    gzopen(awsPtr->currentEventFileName,"wb5");
	if(awsPtr->currentEventFilePtr<0) {	    
	    if(errorCounter<100) {
		errorCounter++;
		printf("Error (%d of 100) trying to open file %s\n",
		       errorCounter,awsPtr->currentEventFileName);
	    }
	}
    }
    if(awsPtr->currentEventFilePtr && awsPtr->currentHeaderFilePtr) {
	if(awsPtr->writeCount>=awsPtr->maxWritesPerFile) {
#ifndef OPEN_CLOSE_ALL_THE_TIME // In NOT defined
	    gzclose(awsPtr->currentEventFilePtr);
	    gzclose(awsPtr->currentHeaderFilePtr);
#endif

	    awsPtr->fileCount++;
	    if(awsPtr->fileCount>=awsPtr->maxFilesPerDir) {
		awsPtr->dirCount++;
		if(awsPtr->dirCount>=awsPtr->maxSubDirsPerDir) {
		    //Make base dir
		    dirNum=(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir*awsPtr->maxSubDirsPerDir)*(hdPtr->eventNumber/(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir*awsPtr->maxSubDirsPerDir));
		    sprintf(awsPtr->currentDirName,"%s/ev%d",awsPtr->baseDirname,dirNum);
		    makeDirectories(awsPtr->currentDirName);
		    awsPtr->dirCount=0;
		}
		
		//Make sub dir
		dirNum=(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir)*(hdPtr->eventNumber/(awsPtr->maxWritesPerFile*awsPtr->maxFilesPerDir));
		sprintf(awsPtr->currentSubDirName,"%s/ev%d",awsPtr->currentDirName,dirNum);
		makeDirectories(awsPtr->currentSubDirName);
		awsPtr->fileCount=0;
	    }
	
	    //Make files
	    dirNum=(awsPtr->maxWritesPerFile)*(hdPtr->eventNumber/awsPtr->maxWritesPerFile);
	    sprintf(awsPtr->currentHeaderFileName,"%s/hd_%d.dat.gz",
		    awsPtr->currentSubDirName,dirNum);
//	    printf("Trying to open %s\n",awsPtr->currentHeaderFileName);
	    awsPtr->currentHeaderFilePtr=gzopen(awsPtr->currentHeaderFileName,"wb5");
	    if(awsPtr->currentHeaderFilePtr<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentHeaderFileName);
		}
	    }
	    
	    sprintf(awsPtr->currentEventFileName,"%s/encev_%d.dat.gz",
		    awsPtr->currentSubDirName,dirNum);
	    awsPtr->currentEventFilePtr=gzopen(awsPtr->currentEventFileName,"wb5");
	    if(awsPtr->currentEventFilePtr<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentEventFileName);
		}
	    }
	    awsPtr->writeCount=0;
	}
#ifdef OPEN_CLOSE_ALL_THE_TIME
	else {
	    awsPtr->currentEventFilePtr=gzopen(awsPtr->currentEventFileName,"ab5");	    
	    if(awsPtr->currentHeaderFilePtr<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentHeaderFileName);
		}
	    }
	    awsPtr->currentHeaderFilePtr=gzopen(awsPtr->currentHeaderFileName,"ab5");	    
	    if(awsPtr->currentEventFilePtr<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentEventFileName);
		}
	    }
	}
#endif

	awsPtr->writeCount++;
	if(errorCounter<100 && awsPtr->currentEventFilePtr && awsPtr->currentHeaderFilePtr) {
//	    retVal=fwrite(hdPtr,sizeof(AnitaEventHeader_t),1,awsPtr->currentHeaderFilePtr);
	    retVal=gzwrite(awsPtr->currentHeaderFilePtr,hdPtr,sizeof(AnitaEventHeader_t));
	    if(retVal<0) {
		errorCounter++;
		printf("Error (%d of 100) writing to file (write %d) -- %s (%d)\n",
		       errorCounter,awsPtr->writeCount,
		       gzerror(awsPtr->currentHeaderFilePtr,&retVal),retVal);
	    }
//	    else {
//		fflush(awsPtr->currentHeaderFilePtr);  
#ifdef OPEN_CLOSE_ALL_THE_TIME
	    gzclose(awsPtr->currentHeaderFilePtr);
#endif
//	    }

//	    retVal=fwrite(bdPtr,sizeof(AnitaEventBody_t),1,awsPtr->currentEventFilePtr);
	    retVal=gzwrite(awsPtr->currentEventFilePtr,outputBuffer,numBytes);
	    if(retVal<0) {
		errorCounter++;
		printf("Error (%d of 100) writing to file (write %d) -- %s (%d)\n",
		       errorCounter,awsPtr->writeCount,
		       gzerror(awsPtr->currentEventFilePtr,&retVal),retVal);
	    }
//	    else {
//		fflush(awsPtr->currentEventFilePtr);  
#ifdef OPEN_CLOSE_ALL_THE_TIME	   
	    gzclose(awsPtr->currentEventFilePtr);
#endif
//	    }
	    

	}
	else return -1;
	
	
    }
    return -1;


}



char *getCurrentHkDir(char *baseHkDir,unsigned long unixTime)
{
    char *newDir;   
    int ext=0;
    newDir=malloc(FILENAME_MAX);
    sprintf(newDir,"%s/sub_%lu",baseHkDir,unixTime);
    while(is_dir(newDir)) {
	ext++;
	sprintf(newDir,"%s/sub_%lu_%d",baseHkDir,unixTime,ext);
    }
    makeDirectories(newDir);
    return newDir;    
}

char *getCurrentHkFilename(char *currentDir, char *prefix, 
			   unsigned long unixTime) 
{
    char *newFile;
    int ext=0;
    FILE *fpTest;
    newFile=malloc(FILENAME_MAX);
    sprintf(newFile,"%s/%s_%lu.dat",currentDir,prefix,unixTime);
    fpTest=fopen(newFile,"r");   
    while(fpTest) {
	fclose(fpTest);
	ext++;
	sprintf(newFile,"%s/%s_%lu.dat_%d",currentDir,prefix,unixTime,ext);
	fpTest=fopen(newFile,"r");
    }
    return newFile;
}
    

int normalSingleWrite(char *buffer, char *filename, int numBytes)
{
    static int errorCounter=0;
   int numObjs;    
   FILE *outfile = fopen (filename, "wb");
   if(outfile == NULL) {
       if(errorCounter<100) {
	   syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	   fprintf(stderr,"fopen: %s -- %s\n",strerror(errno),filename);
	   errorCounter++;
       }
       return -1;
   }   
   numObjs=fwrite(buffer,numBytes,1,outfile);
   fclose(outfile);
   return 0;
   
}

int zippedSingleWrite(char *buffer, char *filename, int numBytes) 
{
    static int errorCounter=0;
   int numObjs;    
   gzFile outfile = gzopen (filename, "wb5");  
   if(outfile == NULL) { 
       if(errorCounter<100) {
	   syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	   fprintf(stderr,"fopen: %s -- %s\n",strerror(errno),filename);
	   errorCounter++;
       }
       return -1;
   } 
   numObjs=gzwrite(outfile,buffer,numBytes);
   gzclose(outfile);  
   return 0;   
}


void makeDirectories(char *theTmpDir) 
{
    char copyDir[FILENAME_MAX];
    char newDir[FILENAME_MAX];
    char *subDir;
    int retVal;
    
    strncpy(copyDir,theTmpDir,FILENAME_MAX);

    strcpy(newDir,"");

    subDir = strtok(copyDir,"/");
    while(subDir != NULL) {
	sprintf(newDir,"%s/%s",newDir,subDir);
	retVal=is_dir(newDir);
	if(!retVal) {	
	    retVal=mkdir(newDir,0777);
// 	    printf("%s\t%s\n",theTmpDir,newDir);
/* 	    printf("Ret Val %d\n",retVal); */
	}
	subDir = strtok(NULL,"/");
    }
       

}

int makeLink(const char *theFile, const char *theLinkDir)
{
    char *justFile=basename((char *)theFile);
    char newFile[FILENAME_MAX];
    sprintf(newFile,"%s/%s",theLinkDir,justFile);
//    printf("Linking %s to %s\n",theFile,newFile);
    return symlink(theFile,newFile);

}


int moveFile(const char *theFile, const char *theDir)
{

    int retVal;
    char *justFile=basename((char *)theFile);
    char newFile[FILENAME_MAX];
    sprintf(newFile,"%s/%s",theDir,justFile);
    retVal=link(theFile,newFile);
    retVal+=unlink(theFile);
    return retVal;
    
}


int copyFile(const char *theFile, const char *theDir)
{

    char *justFile=basename((char *)theFile);
    char newFile[FILENAME_MAX];
    sprintf(newFile,"%s/%s",theDir,justFile);
    return link(theFile,newFile);
}


int removeFile(const char *theFile)
{
    static int errorCounter=0;
    int retVal=unlink(theFile);
    if(retVal!=0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Error (%d of 100) removing %s:\t%s",errorCounter,theFile,strerror(errno));
	    fprintf(stderr,"Error (%d of 100) removing %s:\t%s\n",errorCounter,theFile,strerror(errno));
	    errorCounter++;
	}
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
    static int errorCounter=0;
    int n = scandir(theEventLinkDir, namelist, filterOnDats, versionsort);
    if (n < 0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"scandir %s: %s",theEventLinkDir,strerror(errno));
	    fprintf(stderr,"scandir %s: %s\n",theEventLinkDir,strerror(errno));
	}
	    
    }	
 /*    for(count=0;count<n;count++)  */
/* 	printf("%s\n",(*namelist)[count]->d_name); */
    return n;	    
}


unsigned short getDiskSpace(char *dirName) {
    struct statfs diskStat;
    static int errorCounter=0;
    int retVal=statfs(dirName,&diskStat); 
    if(retVal<0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Unable to get disk space %s: %s",dirName,strerror(errno));  
	    fprintf(stderr,"Unable to get disk space %s: %s\n",dirName,strerror(errno));            
	}
//	if(printToScreen) fprintf(stderr,"Unable to get disk space %s: %s\n",dirName,strerror(errno));       
	return -1;
    }    
    unsigned long bytesAvailable=(diskStat.f_bfree*diskStat.f_bsize);
    unsigned short megabytesAvailable=(unsigned short)(bytesAvailable/(1024*1024));
    return megabytesAvailable;
/*     printf("%lu %d\n",bytesAvailable,megabytesAvailable); */
    /*    if(printToScreen) { */
/* 	printf("Dir: %s\n",dirName); */
/* 	printf("Ret Val: %d\n",retVal); */
/* 	printf("Bytes Available: %ld\n",bytesAvailable); */
/* 	printf("MegaBytes Available: %d\n",megabytesAvailable); */
/* 	printf("Available Blocks: %ld\n",diskStat.f_bavail); */
/* 	printf("Free Blocks: %ld\n",diskStat.f_bfree); */
/* 	printf("Total Blocks: %ld\n",diskStat.f_blocks); */
/* 	printf("Block Size: %d\n",diskStat.f_bsize); */
/* //	printf("Free File Nodes: %ld\n",diskStat.f_ffree); */
/* //	printf("Total File Nodes: %ld\n",diskStat.f_files); */
/* //	printf("Type Of Info: %d\n",diskStat.f_type); */
/* //    printf("File System Id: %d\n",diskStat.f_fsid); */
/*     } */
//    return megabytesAvailable;
}

unsigned short countFilesInDir(char *dirName) {
    static int errorCounter=0;
    unsigned short fileCount=0;
    struct dirent *subPtr;
    DIR *TheDir = opendir(dirName);
    while (TheDir) {
	errno = 0;
	if ((subPtr = readdir(TheDir)) != NULL) {
	    if(subPtr->d_name[0]!='.') {
		fileCount++;
//		printf("File Number %d\t%s\n",fileCount,subPtr->d_name);
	    }
	} 
	else {
	    if (errno == 0) {
		closedir(TheDir);
		break;
	    }
	    else {
		closedir(TheDir);
		if(errorCounter<100) {
		    syslog(LOG_ERR,"Error reading directory %s:\t%s",
			   dirName,strerror(errno));
		    fprintf(stderr,"Error reading directory %s:\t%s\n",
			    dirName,strerror(errno));
		}
		    
		return -1;
	    }
	}
    }    
//    if(printToScreen && verbosity>1) 
//	printf("%s contains %d thingies\n",dirName,fileCount);
    return fileCount;	
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


int fillCalibStruct(CalibStruct_t *theStruct, char *filename)
{
    int numBytes=genericReadOfFile((char*)theStruct,filename,sizeof(CalibStruct_t));
    if(numBytes==sizeof(CalibStruct_t)) return 0;
    return numBytes;

}


int fillCommand(CommandStruct_t *cmdPtr, char *filename)
{
    int numBytes=genericReadOfFile((char*)cmdPtr,filename,sizeof(CommandStruct_t));
    if(numBytes==sizeof(CommandStruct_t)) return 0;
    return numBytes;
}


int fillHeader(AnitaEventHeader_t *theEventHdPtr, char *filename)
{
    int numBytes=genericReadOfFile((char*)theEventHdPtr,filename,
				   sizeof(AnitaEventHeader_t));
    if(numBytes==sizeof(AnitaEventHeader_t)) return 0;
    return numBytes;  
}


int fillHeaderWithThisEvent(AnitaEventHeader_t *hdPtr, char *filename,
			    unsigned long eventNumber)
{
    static int errorCounter=0;
    int numBytes;  
    int readNum=0;
//    int firstEvent=0;
//    int error=0;  
    
    gzFile infile = gzopen (filename, "rb");    
    if(infile == NULL) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	    fprintf(stderr,"fopen: %s ---  %s\n",strerror(errno),filename);
	    errorCounter++;
	}
	return -1;
    }   

    //Read first event
    numBytes=gzread(infile,hdPtr,sizeof(AnitaEventHeader_t));
    if(numBytes!=sizeof(AnitaEventHeader_t)) {
	gzclose(infile);
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytes,filename);
	    fprintf(stderr,"Only read %d bytes from %s\n",numBytes,filename);
	    errorCounter++;
	}	
	return -2;
    }

    if(abs(((int)eventNumber-(int)hdPtr->eventNumber))>1000) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Events %lu and %lu can not both be in file %s\n",eventNumber,hdPtr->eventNumber,filename);
	    fprintf(stderr,"Events %lu and %lu can not both be in file %s\n",eventNumber,hdPtr->eventNumber,filename);
	    errorCounter++;
	}	
	return -3;
    }
	
    readNum=(eventNumber-hdPtr->eventNumber);
    if(readNum) {	    
	if(readNum>1) 
	    gzseek(infile,sizeof(AnitaEventHeader_t)*(readNum-1),SEEK_CUR);
	
	numBytes=gzread(infile,hdPtr,sizeof(AnitaEventHeader_t));
	
	if(numBytes!=sizeof(AnitaEventHeader_t)) {
	    gzclose(infile);
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytes,filename);
		fprintf(stderr,"Only read %d bytes from %s\n",numBytes,filename);
		errorCounter++;
	    }	
	    return -4;
	}
    }
    gzclose(infile);
    return 0;
}


int readEncodedEventFromFile(char *buffer, char *filename,
			     unsigned long eventNumber) 
{
    EncodedSurfPacketHeader_t *surfHeader = (EncodedSurfPacketHeader_t*) &buffer[0];
    static int errorCounter=0;
    int count=0;
    int numBytesRead;  
    int numBytesToSeek;
    int numBytesToRead;
    
    gzFile infile = gzopen (filename, "rb");    
    if(infile == NULL) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	    fprintf(stderr,"fopen: %s ---  %s\n",strerror(errno),filename);
	    errorCounter++;
	}
	return -1;
    }   

    //Read first header
    numBytesRead=gzread(infile,surfHeader,sizeof(EncodedSurfPacketHeader_t));
    if(numBytesRead!=sizeof(EncodedSurfPacketHeader_t)) {
	gzclose(infile);
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytesRead,filename);
	    fprintf(stderr,"Only read %d bytes from %s\n",numBytesRead,filename);
	    errorCounter++;
	}	
	return -2;
    }

    printf("First read %lu (want %lu)\n\t%s\n",surfHeader->eventNumber,eventNumber,filename);

    if(abs(((int)eventNumber-(int)surfHeader->eventNumber))>1000) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Events %lu and %lu can not both be in file %s\n",eventNumber,surfHeader->eventNumber,filename);
	    fprintf(stderr,"Events %lu and %lu can not both be in file %s\n",eventNumber,surfHeader->eventNumber,filename);
	    errorCounter++;
	}	
	gzclose(infile);
	return -3;
    }
    int surfCounter=0;

    while(surfHeader->eventNumber!=eventNumber) {
	numBytesToSeek=surfHeader->gHdr.numBytes-sizeof(EncodedSurfPacketHeader_t);	
//	printf("%d\t%lu\t%lu\t\t(seek: %d)\n",surfCounter,surfHeader->eventNumber,eventNumber,
//	       numBytesToSeek);
	surfCounter++;

	gzseek(infile,numBytesToSeek,SEEK_CUR);
	numBytesRead=gzread(infile,surfHeader,sizeof(EncodedSurfPacketHeader_t));
	if(numBytesRead!=sizeof(EncodedSurfPacketHeader_t)) {
	    gzclose(infile);
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytesRead,filename);
		fprintf(stderr,"Only read %d bytes from %s\n",numBytesRead,filename);
		errorCounter++;
	    }	
	    return -4;
	}
	
	
	if(abs(((int)eventNumber-(int)surfHeader->eventNumber))>1000) {

	    if(errorCounter<100) {
		syslog(LOG_ERR,"Events %lu and %lu can not both be in file %s\n",eventNumber,surfHeader->eventNumber,filename);
		fprintf(stderr,"Events %lu and %lu can not both be in file %s\n",eventNumber,surfHeader->eventNumber,filename);
		errorCounter++;
	    }	
	    gzclose(infile);
	    return -5;
	}
    }

    //Now ready to read event
    count=sizeof(EncodedSurfPacketHeader_t);
    for(surfCounter=0;surfCounter<9;surfCounter++) {
	numBytesToRead=surfHeader->gHdr.numBytes-sizeof(EncodedSurfPacketHeader_t);	
	numBytesRead=gzread(infile,&buffer[count],
			    numBytesToRead);
	if(numBytesRead!=numBytesToRead) {
	    gzclose(infile);
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytesRead,filename);
		fprintf(stderr,"Only read %d bytes from %s\n",numBytesRead,filename);
		errorCounter++;
	    }	
	    return -6;
	}
	count+=numBytesRead;

	if(surfCounter<8) {
	    //Need to read next header
	    surfHeader = (EncodedSurfPacketHeader_t*) &buffer[count];
	    
	    numBytesRead=gzread(infile,surfHeader,sizeof(EncodedSurfPacketHeader_t));
	    if(numBytesRead!=sizeof(EncodedSurfPacketHeader_t)) {
		gzclose(infile);
		if(errorCounter<100) {
		    syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytesRead,filename);
		    fprintf(stderr,"Only read %d bytes from %s\n",numBytesRead,filename);
		    errorCounter++;
		}	
		return -7;
	    }
	    count+=sizeof(EncodedSurfPacketHeader_t);
	    
	}
	
    }
    
    gzclose(infile);
    return count;


}



int fillBody(AnitaEventBody_t *theEventBodyPtr, char *filename)
{ 
    int numBytes=genericReadOfFile((char*)theEventBodyPtr,filename,
				   sizeof(AnitaEventBody_t));
    if(numBytes==sizeof(AnitaEventBody_t)) return 0;
    return numBytes;  
}


int fillGpsStruct(GpsSubTime_t *tttPtr, char *filename)
{
    int numBytes=genericReadOfFile((char*)tttPtr,filename,
				   sizeof(GpsSubTime_t));
    if(numBytes==sizeof(GpsSubTime_t)) return 0;
    return numBytes;  
}

int genericReadOfFile(char *buffer, char *filename, int maxBytes) 
/*!
  Reads all the bytes in a file and returns the number of bytes read
*/
{
    int numBytes;
#ifdef NO_ZLIB
    int fd;
    fd = open(filename,O_RDONLY);
    if(fd == 0) {
	return -1;
    }
    numBytes=read(fd,buffer,maxBytes);
    close (fd);
    if(numBytes<=0) {
	return -2;
    }
    return numBytes;
#else
    gzFile infile = gzopen(filename,"rb");
    if(infile == NULL) {
	syslog (LOG_ERR,"gzopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }       
    numBytes=gzread(infile,buffer,maxBytes);
    gzclose(infile);
    if(numBytes<=0) {
	return -2;
    }
    return numBytes;
#endif
}


int writeHeader(AnitaEventHeader_t *hdPtr, char *filename)
/* Writes the header pointed to by hdPtr to filename */
{
    return normalSingleWrite((char*)hdPtr,filename,sizeof(AnitaEventHeader_t));
}

int writeBody(AnitaEventBody_t *bodyPtr, char *filename)
/* Writes the body pointed to by bodyPtr to filename */
{
#ifdef NO_ZLIB
    return normalSingleWrite((char*)bodyPtr,filename,sizeof(AnitaEventBody_t));
#else
    return zippedSingleWrite((char*)bodyPtr,filename,sizeof(AnitaEventBody_t));
#endif
}


int writeZippedBody(AnitaEventBody_t *bodyPtr, char *filename)
/* Writes the body pointed to by bodyPtr to filename */
{
    return zippedSingleWrite((char*)bodyPtr,filename,sizeof(AnitaEventBody_t));
}

int writeWaveformPacket(RawWaveformPacket_t *wavePtr, char *filename)
/* Writes the waveform pointed to by wavePtr to filename */
{
    
#ifdef NO_ZLIB
    return normalSingleWrite((char*)wavePtr,filename,sizeof(RawWaveformPacket_t));
#else
    return zippedSingleWrite((char*)wavePtr,filename,sizeof(RawWaveformPacket_t));
#endif
}


int writeSurfPacket(RawSurfPacket_t *surfPtr, char *filename)
/* Writes the surf packet pointed to by surfPtr to filename */
{
  
#ifdef NO_ZLIB
    return normalSingleWrite((char*)surfPtr,filename,sizeof(RawSurfPacket_t));
#else
    return zippedSingleWrite((char*)surfPtr,filename,sizeof(RawSurfPacket_t));
#endif
}


int writeSurfHk(FullSurfHkStruct_t *surfPtr, char *filename)
/* Writes the surf hk packet pointed to by surfPtr to filename */
{

#ifdef NO_ZLIB
    return normalSingleWrite((char*)surfPtr,filename,sizeof(FullSurfHkStruct_t));
#else
    return zippedSingleWrite((char*)surfPtr,filename,sizeof(FullSurfHkStruct_t));
#endif

}

int writeGpsPat(GpsAdu5PatStruct_t *patPtr, char *filename)
/* Writes the pat pointed to by patPtr to filename */
{

#ifdef NO_ZLIB
    return normalSingleWrite((char*)patPtr,filename,sizeof(GpsAdu5PatStruct_t));
#else
    return zippedSingleWrite((char*)patPtr,filename,sizeof(GpsAdu5PatStruct_t));
#endif

}


int writeGpsVtg(GpsAdu5VtgStruct_t *vtgPtr, char *filename)
/* Writes the vtg pointed to by vtgPtr to filename */
{

#ifdef NO_ZLIB
    return normalSingleWrite((char*)vtgPtr,filename,sizeof(GpsAdu5VtgStruct_t));
#else
    return zippedSingleWrite((char*)vtgPtr,filename,sizeof(GpsAdu5VtgStruct_t));
#endif

}

int writeGpsPos(GpsG12PosStruct_t *posPtr, char *filename)
/* Writes the pos pointed to by posPtr to filename */
{


#ifdef NO_ZLIB
    return normalSingleWrite((char*)posPtr,filename,sizeof(GpsG12PosStruct_t));
#else
    return zippedSingleWrite((char*)posPtr,filename,sizeof(GpsG12PosStruct_t));
#endif

}

int writeGpsAdu5Sat(GpsAdu5SatStruct_t *satPtr, char *filename)
/* Writes the sat pointed to by satPtr to filename */
{   

#ifdef NO_ZLIB
    return normalSingleWrite((char*)satPtr,filename,sizeof(GpsAdu5SatStruct_t));
#else
    return zippedSingleWrite((char*)satPtr,filename,sizeof(GpsAdu5SatStruct_t));
#endif

}


int writeGpsG12Sat(GpsG12SatStruct_t *satPtr, char *filename)
/* Writes the sat pointed to by satPtr to filename */
{   

#ifdef NO_ZLIB
    return normalSingleWrite((char*)satPtr,filename,sizeof(GpsG12SatStruct_t));
#else
    return zippedSingleWrite((char*)satPtr,filename,sizeof(GpsG12SatStruct_t));
#endif

}


int writeGpsTtt(GpsSubTime_t *tttPtr, char *filename)
/* Writes the ttt pointed to by tttPtr to filename */
{     
    return normalSingleWrite((char*)tttPtr,filename,sizeof(GpsSubTime_t));
}

int writeHk(HkDataStruct_t *hkPtr, char *filename)
/* Writes the hk pointed to by hkPtr to filename */
{     
#ifdef NO_ZLIB
    return normalSingleWrite((char*)hkPtr,filename,sizeof(HkDataStruct_t));
#else
    return zippedSingleWrite((char*)hkPtr,filename,sizeof(HkDataStruct_t));
#endif

}


int writeCmdEcho(CommandEcho_t *echoPtr, char *filename)
/* Writes the echo pointed to by echoPtr to filename */
{
#ifdef NO_ZLIB
    return normalSingleWrite((char*)echoPtr,filename,sizeof(CommandEcho_t));
#else
    return zippedSingleWrite((char*)echoPtr,filename,sizeof(CommandEcho_t));
#endif
}


int writeCmd(CommandStruct_t *cmdPtr, char *filename)
/* Writes the cmd pointed to by cmdPtr to filename */
{   
    return normalSingleWrite((char*)cmdPtr,filename,sizeof(CommandStruct_t));
}

int writeCalibStatus(CalibStruct_t *calibPtr, char *filename)
/* Writes the cmd pointed to by cmdPtr to filename */
{   
    return normalSingleWrite((char*)calibPtr,filename,sizeof(CalibStruct_t));
}


int writeMonitor(MonitorStruct_t *monitorPtr, char *filename)
/* Writes the monitor object pointed to by monitorPtr to filename */
{   
#ifdef NO_ZLIB
    return normalSingleWrite((char*)monitorPtr,filename,sizeof(MonitorStruct_t));
#else
    return zippedSingleWrite((char*)monitorPtr,filename,sizeof(MonitorStruct_t));
#endif

}



int writeTurfRate(TurfRateStruct_t *turfPtr, char *filename)
/* Writes the TurfRateStruct_t object pointed to by turfPtr to filename */
{

#ifdef NO_ZLIB
    return normalSingleWrite((char*)turfPtr,filename,sizeof(TurfRateStruct_t));
#else
    return zippedSingleWrite((char*)turfPtr,filename,sizeof(TurfRateStruct_t));
#endif

}



 
void sigUsr1Handler(int sig)
{
    currentState=PROG_STATE_INIT;
} 

void sigUsr2Handler(int sig)
{
    currentState=PROG_STATE_TERMINATE;
} 

void writePidFile(char *fileName)
{
    FILE *fpPid ;
    char theDir[FILENAME_MAX];
    char tempFileName[FILENAME_MAX];
    strncpy(tempFileName,fileName,FILENAME_MAX-1);
    strncpy(theDir,dirname(tempFileName),FILENAME_MAX-1);
    makeDirectories(theDir);
//    pid_t thePid;
    if (!(fpPid=fopen(fileName, "w"))) {
	fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	exit(1) ;
    }
    fprintf(fpPid,"%d\n", getpid()) ;
    fclose(fpPid) ;
}

void addDay(struct tm *timeinfo) {
    //Must do leap year check
    int year=timeinfo->tm_year;
    int isLeap=((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0));
    int daysInMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(isLeap) daysInMonth[1]=29; //Leap year
    
    timeinfo->tm_mday++;
    if(timeinfo->tm_mday>daysInMonth[timeinfo->tm_mon]) {
	timeinfo->tm_mday=1;
	timeinfo->tm_mon++;
	if(timeinfo->tm_mon>=12) {
	    timeinfo->tm_mon=0;
	    timeinfo->tm_year++;
	}
    }
}


unsigned long simpleLongCrc(unsigned long *p, unsigned long n)
{
    unsigned long sum = 0;
    unsigned long i;
    for (i=0L; i<n; i++) {
//	printf("%lu\n",*p);
	sum += *p++;
    }
    return ((0xffffffff - sum) + 1);
}

void fillGenericHeader(void *thePtr, PacketCode_t code, unsigned short numBytes) {
    unsigned long longBytes=(numBytes-sizeof(GenericHeader_t))/4;
    GenericHeader_t *gHdr= (GenericHeader_t*)thePtr;
    unsigned long *dataPtr=(unsigned long*) (thePtr+sizeof(GenericHeader_t));
    gHdr->code=code;
    gHdr->numBytes=numBytes;
    gHdr->feByte=0xfe;
    switch(code) {
	case PACKET_HD: gHdr->verId=VER_EVENT_HEADER; break;
	case PACKET_WV: gHdr->verId=VER_WAVE_PACKET; break;
	case PACKET_SURF: gHdr->verId=VER_SURF_PACKET; break;
	case PACKET_SURF_HK: gHdr->verId=VER_SURF_HK; break;
	case PACKET_TURF_RATE: gHdr->verId=VER_TURF_RATE; break;
	case PACKET_ENC_WV: gHdr->verId=VER_ENC_WAVE_PACKET; break;
	case PACKET_ENC_SURF: gHdr->verId=VER_ENC_SURF_PACKET; break;
	case PACKET_GPS_ADU5_PAT: gHdr->verId=VER_ADU5_PAT; break;
	case PACKET_GPS_ADU5_SAT: gHdr->verId=VER_ADU5_SAT; break;
	case PACKET_GPS_ADU5_VTG: gHdr->verId=VER_ADU5_VTG; break;
	case PACKET_GPS_G12_POS: gHdr->verId=VER_G12_POS; break;
	case PACKET_GPS_G12_SAT: gHdr->verId=VER_G12_SAT; break;
	case PACKET_HKD: gHdr->verId=VER_HK_FULL; break;
	case PACKET_CMD_ECHO: gHdr->verId=VER_CMD_ECHO; break;
	case PACKET_MONITOR: gHdr->verId=VER_MONITOR; break;
	default: 
	    gHdr->verId=0; break;
    }
    gHdr->checksum=simpleLongCrc(dataPtr,longBytes);

}

#define PKT_E_CHECKSUM 0x1
#define PKT_E_CODE 0x2
#define PKT_E_FEBYTE 0x4
#define PKT_E_SIZE 0x8

int checkPacket(void *thePtr)
//0 means no errors
//1 means checksum mismatch
//2 means unknown packet code
//4 means missing feByte missing
//8 means packet size mismatch
{
    int retVal=0,packetSize=0;
    GenericHeader_t *gHdr= (GenericHeader_t*)thePtr;
    unsigned long longBytes=(gHdr->numBytes-sizeof(GenericHeader_t))/4;
    unsigned long *dataPtr=(unsigned long*) (thePtr+sizeof(GenericHeader_t)); 
    unsigned long checksum=simpleLongCrc(dataPtr,longBytes);
    PacketCode_t code=gHdr->code;
    if(checksum!=gHdr->checksum) {
	printf("Checksum Mismatch (possibly %s (%d)) (%lu bytes) %lu -- %lu \n",
	       packetCodeAsString(code),code,longBytes,checksum,gHdr->checksum);	
	retVal+=PKT_E_CHECKSUM;
    }
    switch(code) {
	case PACKET_HD: packetSize=sizeof(AnitaEventHeader_t); break;	    
	case PACKET_WV: packetSize=sizeof(RawWaveformPacket_t); break;
	case PACKET_SURF: packetSize=sizeof(RawSurfPacket_t); break;
	case PACKET_SURF_HK: packetSize=sizeof(FullSurfHkStruct_t); break;
	case PACKET_TURF_RATE: packetSize=sizeof(TurfRateStruct_t); break;
	case PACKET_ENC_WV: break;
	case PACKET_ENC_SURF: break;
	case PACKET_GPS_ADU5_PAT: packetSize=sizeof(GpsAdu5PatStruct_t); break;
	case PACKET_GPS_ADU5_SAT: packetSize=sizeof(GpsAdu5SatStruct_t); break;
	case PACKET_GPS_ADU5_VTG: packetSize=sizeof(GpsAdu5VtgStruct_t); break;
	case PACKET_GPS_G12_POS: packetSize=sizeof(GpsG12PosStruct_t); break;
	case PACKET_GPS_G12_SAT: packetSize=sizeof(GpsG12SatStruct_t); break;
	case PACKET_HKD: packetSize=sizeof(HkDataStruct_t); break;
	case PACKET_CMD_ECHO: packetSize=sizeof(CommandEcho_t); break;
	case PACKET_MONITOR: packetSize=sizeof(MonitorStruct_t); break;
	case PACKET_WAKEUP_LOS: packetSize=WAKEUP_LOS_BUFFER_SIZE; break;
	case PACKET_WAKEUP_HIGHRATE: packetSize=WAKEUP_TDRSS_BUFFER_SIZE; 
	    break;
	case PACKET_WAKEUP_COMM1: packetSize=WAKEUP_LOW_RATE_BUFFER_SIZE; 
	    break;
	case PACKET_WAKEUP_COMM2: packetSize=WAKEUP_LOW_RATE_BUFFER_SIZE; 
	    break;
	default: 
	    retVal+=PKT_E_CODE; break;
    }
    if(packetSize && (packetSize!=gHdr->numBytes))
	retVal+=PKT_E_SIZE;
    if(gHdr->feByte!=0xfe) retVal+=PKT_E_FEBYTE;
    return retVal;

}

char *packetCodeAsString(PacketCode_t code) {
    char* string ;
    switch(code) {
	case PACKET_HD: string="AnitaEventHeader_t"; break;	    
	case PACKET_WV: string="RawWaveformPacket_t"; break;
	case PACKET_SURF: string="RawSurfPacket_t"; break;
	case PACKET_SURF_HK: string="FullSurfHkStruct_t"; break;
	case PACKET_TURF_RATE: string="TurfRateStruct_t"; break;
	case PACKET_ENC_WV: string="EncodedWaveformPacket_t"; break;
	case PACKET_ENC_SURF: string="EncodedSurfPacketHeader_t"; break;
	case PACKET_GPS_ADU5_PAT: string="GpsAdu5PatStruct_t"; break;
	case PACKET_GPS_ADU5_SAT: string="GpsAdu5SatStruct_t"; break;
	case PACKET_GPS_ADU5_VTG: string="GpsAdu5VtgStruct_t"; break;
	case PACKET_GPS_G12_POS: string="GpsG12PosStruct_t"; break;
	case PACKET_GPS_G12_SAT: string="GpsG12SatStruct_t"; break;
	case PACKET_HKD: string="HkDataStruct_t"; break;
	case PACKET_CMD_ECHO: string="CommandEcho_t"; break;
	case PACKET_MONITOR: string="MonitorStruct_t"; break;
	case PACKET_WAKEUP_LOS: string="LosWakeUpPacket"; break;
	case PACKET_WAKEUP_HIGHRATE: string="TdrssWakeUpPacket"; break;
	case PACKET_WAKEUP_COMM1: string="Comm1WakeUpPacket"; break;
	case PACKET_WAKEUP_COMM2: string="Comm2WakeUpPacket"; break;

	default: 
	    string="Unknown Packet Code"; break;
    }
    return string;
}
