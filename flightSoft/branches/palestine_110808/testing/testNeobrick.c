#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <ftpLib/ftplib.h>

#include "includes/anitaStructures.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "pedestalLib/pedestalLib.h"
#include "compressLib/compressLib.h"
#include "linkWatchLib/linkWatchLib.h"


int wdNeo=0;


void makeFtpDirectories(char *theTmpDir) 
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
	retVal=ftp_mkdir(newDir);
// 	    printf("%s\t%s\n",theTmpDir,newDir);
/* 	    printf("Ret Val %d\n",retVal); */
	subDir = strtok(NULL,"/");
    }
       
}

int main(int argc, char**argv) {

    if(argc<2)
    {
	printf("Usage %s - /tmp/neobrick/current/somefile\n",basename(argv[0]));
	return 0;
    }

    // open the FTP connection   
    if (ftp_open("10.0.0.2", "anonymous", "")) {
	fprintf(stderr,"ftp_open -- %s\n",strerror(errno));
    }


    int runNumber=getRunNumber();
    char fileName[FILENAME_MAX];
    sprintf(fileName,"/mnt/data/run%d/%s",runNumber,&argv[1][21]);
    
    printf("New file -- %s\n",fileName);
    
    char *thisFile=basename(fileName);
    char *thisDir=dirname(fileName);
    makeFtpDirectories(thisDir);
    ftp_cd(thisDir);
    printf("%s -- %s\n",thisFile,thisDir);
    ftp_putfile(argv[1],thisFile,0,0);
    unlink(argv[1]);

//  ftp_mkdir("/mnt/data/run3000");
//  ftp_cd("/mnt/");
//  ftp_putfile("/tmp/update", "statFile",0,0);
//  ftp_cd("/mnt/");
//  ftp_getfile("statFile", "/tmp/didItWork",0);

  // the end 
  ftp_close();
  return 0;
}
