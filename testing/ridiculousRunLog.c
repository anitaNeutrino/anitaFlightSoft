#include <stdio.h>
#include <stdlib.h>



int main(int argc, char** argv) {
    FILE *fp;
    char usersName[FILENAME_MAX];
    char runName[FILENAME_MAX];
    char bigBuffer[FILENAME_MAX];
	


    if(argc<2) {
	fprintf(stderr,"Usage %s <log file>\n",argv[0]);
	exit(0);
    }

    return 0;

}
