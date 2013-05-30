#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>


#include <sys/types.h>

#include <time.h>


int main (int argc, char **argv)
{   
  int t=0;
  while(t<10000000) {
    t++;
    if(t>500000) {
      t-=40000;
    }
  }
}
