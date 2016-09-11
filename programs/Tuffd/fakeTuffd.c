#include "utilLib/utilLib.h"
#include <stdlib.h>


int sortOutPidFile(char *progName)
{
  
  int retVal=checkPidFile(TUFFD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,TUFFD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(TUFFD_PID_FILE);
  return 0;
}

void cleanup()
{
  unlink(TUFFD_PID_FILE); 
  syslog(LOG_INFO,"fakeTuffd terminating"); 
}

void handleBadSigs(int sig) 
{
  syslog(LOG_WARNING,"Tuffd received sig %d -- will exit immediately\n",sig); 

  cleanup(); 
  exit(127 + sig); 
}

void setupSignals()
{
  struct sigaction act; 
  act.sa_sigaction = sigactionUsr1Handler; 
  act.sa_flags = SA_SIGINFO; 

  // this way we know who sent us the signal! 
  sigaction(SIGUSR1, &act,NULL); 

//  signal(SIGUSR1, sigUsr1Handler);
  signal(SIGUSR2, sigUsr2Handler);
  signal(SIGTERM, handleBadSigs);
  signal(SIGINT, handleBadSigs);
  signal(SIGSEGV, handleBadSigs);
}

int main(int nargs, char ** args) 
{

  sortOutPidFile("Tuffd") ; 
  setupSignals(); 

  do 
  {
    currentState = PROG_STATE_RUN;  

    while(currentState == PROG_STATE_RUN)
    {
      sleep(1); 
    }
  } while(currentState == PROG_STATE_INIT); 



  cleanup(); 
  return 0; 

}
