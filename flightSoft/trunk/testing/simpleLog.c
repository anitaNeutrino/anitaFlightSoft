#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>


int main(int argc, char** argv) {
    FILE *fpLog;
    char logFile[FILENAME_MAX];
    char *usersName;
    char *runDescription;

    	
    if(argc<2) {
	fprintf(stderr,"Usage %s <log file>\n",argv[0]);
	exit(0);
    }

    read_history("/tmp/simpleLog.hist");

    
    strncpy(logFile,argv[1],FILENAME_MAX-1);
    fpLog=fopen(logFile,"w");
    if(!fpLog) {
	printf("Couldn't open %s\n",logFile);
	exit(0);
    }
    printf("\n\nYou can use the up arrow key to retrieve previous input to the logger (if you don't want to retype your name, etc.\n\n");
    usersName=readline("Enter User's name:\n");
    if(usersName && *usersName)
	add_history(usersName);
    runDescription=readline("Enter Run Description (ends with Enter):\n");
    if(runDescription && *runDescription)
	add_history(runDescription);

   
    
    write_history("/tmp/simpleLog.hist");

    fprintf(fpLog,"User: %s\n",usersName);          
    fprintf(fpLog,"Run Description:\n %s\n",runDescription);  
    
    fclose(fpLog);
    return 0;

}
