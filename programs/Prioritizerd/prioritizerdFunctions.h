#ifndef PRIORITIZERD_FUNCTIONS
#define PRIORITIZERD_FUNCTIONS


/* System */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <libgen.h>

/* Flightsoft */
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "linkWatchLib/linkWatchLib.h"
#include "utilLib/utilLib.h"
#include "includes/anitaStructures.h"

/* Ben's timing calibration, GPU and all round lovely function things */
#include "openCLwrapper.h"
#include "anitaTimingCalib.h"
#include "anitaGpu.h"
#include "tstamp.h"


void panicWriteAllLinks(int wd, int panicPri, int panicQueueLength, int priorityPPS1, int priorityPPS2);
void handleBadSigs(int sig);
int sortOutPidFile(char *progName);


/* for fakePrioritizerd */
void readInEvent(PedSubbedEventBody_t *psev, AnitaEventHeader_t* head, const char* dir, int eventNumber);
void readIn100Events(const char* psevFileName,
		     PedSubbedEventBody_t *theBody,
		     const char* headFileName,
		     AnitaEventHeader_t* theHeader);

void readIn100UnzippedEvents(const char* psevFileName,
			     PedSubbedEventBody_t *theBody,
			     const char* headFileName,
			     AnitaEventHeader_t* theHeader);



void assignCpuPriorities(int eventInd, double* finalVolts[], AnitaEventHeader_t* header);

#endif /*PRIORITIZERD_FUNCTIONS*/
