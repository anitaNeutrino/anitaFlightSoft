#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include <sys/types.h>
#include <time.h>
#include <math.h>


#include "includes/anitaStructures.h"

#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "pedestalLib/pedestalLib.h"
#include "compressLib/compressLib.h"


int main(int argc, char**argv) {
/*     if(argc<2) { */
/* 	printf("Usage: %s <event telem file>\n",basename(argv[0])); */
/* 	return 0; */
/*     } */
    zipFileInPlace("/home/rjn/flightSoft/README");
}
