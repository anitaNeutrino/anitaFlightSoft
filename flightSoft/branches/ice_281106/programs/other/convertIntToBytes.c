#include <stdio.h>


int main(int argc, char **argv) 
{
    if(argc!=2) {
	printf("Usage: convertIntToBytes <integer>\n");
    }
    int theInt=atoi(argv[1]);
    printf("Bytes:\t%d %d\n",(theInt&0xff),((theInt&0xff00)>>8));


}
