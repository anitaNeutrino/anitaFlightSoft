#ifndef _CMD_LINE_H
#define _CMD_LINE_H

/******************************************************************************
 *
 * File Name:
 *
 *      CommandLine.h
 *
 * Description:
 *
 *      Header for the functions to support Command-line processing
 *
 * Revision History:
 *
 *      05-31-02 : PCI SDK v3.50
 *
 ******************************************************************************/


#include "PlxTypes.h"




/*************************************
 *          Functions
 ************************************/
ULONGLONG
htol(
    char *hexString
    );

void
GetNextArg(
    char **Args,
    char  *buffer
    );




#endif
