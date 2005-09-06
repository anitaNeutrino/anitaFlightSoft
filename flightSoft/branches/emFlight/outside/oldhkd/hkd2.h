#include "../carrier/apc8620.h"
#include "../ip220/ip220.h"
#include "../ip320/ip320.h"
#include "../ip470/ip470.h"
#include <string.h>
#include <sys/timeb.h>
#include <math.h>
#include <signal.h>


#define L_DIR "/home/anita/log/hkd/" 

/////////////////////////////////////
/* tilt_read */
////////////////////////

#include <stdio.h>     /* Standard input/output definitions */ 
#include <stdlib.h>
#include <ctype.h>
#include <string.h>    /* String function definitions */ 
#include <unistd.h>    /* UNIX standard function definitions */ 
#include <fcntl.h>     /* File control definitions */ 
#include <errno.h>     /* Error number definitions */ 
/*  #include <asm/termios.h>   */ /* POSIX terminal control definitions  older version of termios*/

////////////////////////////////////
/* ASHTECH G12 readout code.*/
/*  #include <stdio.h>  */

#include<malloc.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<time.h>
#include<sys/time.h>
/*  #include<unistd.h> */
#include<signal.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
/*  #include <string.h> */
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/shm.h>
/*  #include <fcntl.h> */
#include <termio.h>
#include <termios.h> 

#define COMMAND_SIZE 100
#define DATA_SIZE 128
#define BAUDRATE B9600
#define CHARACTER_SIZE CS8

#define COMMAND "$PASHQ,POS\n" //Query lat/lon message


#define COMMAND1 "$GPZDA\n" 
