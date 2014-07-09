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
  int status;
  char *result;

  if (argc == 4)
    {
      configFile = argv[1];
      configBranch = argv[2];
      configItem = argv[3];
    }
  else
    return usage(progName);

  kvpReset();
  status = configLoad(configFile, configBranch);

  if (status == CONFIG_E_OK)
    {
      result = kvpGetString(configItem);
      printf("%s\n",result);
    }
  else {
    
    eString = configErrorString(status);
    printf("Error: %s\n",eString);
    printf("\n");
  }
  return 0;
}

int usage(char *progName)
{
  printf("usage:\t%s configFile configBranch configItem\n", progName);
  printf("Prints integer configItem on configBranch in configFile\n");
  return 1;
}
