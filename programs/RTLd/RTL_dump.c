#include "RTL_common.h" 
#include <stdio.h>
#include <stdlib.h> 

int main(int nargs, char ** args) 
{


  RtlSdrPowerSpectraStruct_t spectrum; 
  FILE * f = fopen(args[1], "r"); 
  int offset = 0; 
  int i; 

  if (nargs > 2) 
  {
    offset = atoi(args[2]); 

    for (i = 0; i < offset; i++)
    {
      fread(&spectrum, sizeof(spectrum), 1, f); 
    }
  }

  
  fread(&spectrum, sizeof(spectrum), 1, f); 

  dumpSpectrum(&spectrum); 


  return 0; 


}
