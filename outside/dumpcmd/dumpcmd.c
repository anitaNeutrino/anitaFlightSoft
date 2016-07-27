#include "includes/anitaStructures.h"
#include <stdio.h> 


int main(int nargs, char ** args) 
{

  CommandStruct_t cmd; 
  int i; 

  if (nargs  < 2) 
  {
    printf("Usage dumpcmd file.dat\n"); 
    return 1; 


  }

  FILE * f = fopen(args[1], "r"); 
  if (!f) 
  {
    printf("Could not open file %s\n", args[1]); 
    return 2; 
  }

  fread(&cmd, sizeof(cmd), 1, f); 


  printf("Num bytes: %d\n", cmd.numCmdBytes); 

  for (i = 0; i < MAX_CMD_LENGTH; i++)
  {
    printf("Byte %d: %x\n", i, cmd.cmd[i]); 
  }

  printf("From sip: %d\n", cmd.fromSipd); 

  fclose(f); 
  return 0; 

}
