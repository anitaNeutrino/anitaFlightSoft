#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <libgen.h>

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
int usage(char *progName);

int main(int argc, char **argv)
{
  char *configFile;
  char *configBranch;
  char *configItem;
  char *progName = basename(argv[0]);

  char *eString;
  int status=0;
  //  int result=0;
  char modString[180];
  time_t rawtime;

  if (argc == 5)
    {
      configFile = argv[1];
      configBranch = argv[2];
      configItem = argv[3];
      strcpy(modString,argv[4]);
    }
  else
    return usage(progName);
  
  configModifyString(configFile,configBranch,configItem,modString,&rawtime);

  kvpReset();
  status = configLoad(configFile, configBranch);

  if (status == CONFIG_E_OK)
    {
/*       result = kvpGetInt(configItem,-1); */
/*       printf("%d\n",result); */
/*       if(result!=value) { */
/* 	fprintf(stderr,"Error setting %s %s %s to %s\n",configFile,configBranch, */
/* 		configItem,modString); */
/*       } */
    }
  else 
    {
      eString = configErrorString(status);
      fprintf(stderr,"Error setting %s %s %s to %s -- %s\n",configFile,configBranch,
	      configItem,modString,eString);
  }
  return 0;
}

int usage(char *progName)
{
  printf("usage:\t%s configFile configBranch configItem\n", progName);
  printf("Prints integer configItem on configBranch in configFile\n");
  return 1;
}
