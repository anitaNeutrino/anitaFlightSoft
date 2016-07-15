#include "anitaFlight.h" 

#include <errno.h>
#include <stdio.h> 


static FILE * oneshots[NUM_RTLS];


static char cmd[1024]; 


int main(int nargs, char ** args) 
{

  int read_config_ok; 
  read_config_ok = readConfig(); 
  setUpInterrupts(); 

  do 
  {

    readConfig(); 

    while (currentState == PROG_STATE_RUN) 
    {

      Rtld

      // Spawn the processes 
      for (int i = 0; i < NUM_RTLS; i++) 
      {

        int rtl_id_num = i+1; 
        char serial[32]; 
        sprintf(serial, "RTL%d", i+1); 
        sprintf(cmd, "RTL_singleshot_power -d %s  -f %d:%d:%d -g %f %s.out", serial, start, end, step, gain, serial); 
        oneshots[i] = popen (cmd, "r"); 
        if (!oneshots[i])
        {
          syslog(LOG_ERR, "popen with err %d on command: %s", errno, cmd); 
        }
      }

      for (int i = 0; i < NUM_RTLS; i++) 
      {
        pclose(oneshots[i]); 
      }
    }

  } while (currentState == PROG_STATE_INIT) 


}
