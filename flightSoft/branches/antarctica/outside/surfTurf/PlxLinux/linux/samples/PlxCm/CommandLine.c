/*********************************************************************
 *
 * Module Name:
 *
 *     CommandLine.c
 *
 * Abstract:
 *
 *     Functions to support Command-line processing
 *
 * Revision History:
 *
 *      04-31-02 : PCI SDK v3.50
 *
 ********************************************************************/


#include <string.h>
#include "CommandLine.h"
#include "PlxTypes.h"




/**********************************************************
 *
 * Function   :  htol
 *
 * Description:  Converts a Hex string to a 64-bit value
 *
 *********************************************************/
ULONGLONG
htol(
    char *hexString
    )
{
    U8        count;
    ULONGLONG value;


    value = 0;

    for (count=0; count<strlen(hexString); count++)
    {
        value = value << 4;

        if ( (hexString[count] >= 'A') && (hexString[count] <= 'F') )
            value = value + (hexString[count] - 'A' + 0xA);
        else if ( (hexString[count] >= 'a') && (hexString[count] <= 'f') )
            value = value + (hexString[count] - 'a' + 0xA);
        else
            value = value + (hexString[count] - '0');
    }

    return value;
}




/**********************************************************
 *
 * Function   :
 *
 * Description:
 *
 *********************************************************/
void
GetNextArg(
    char **Args,
    char  *buffer
    )
{
    while (**Args == ' ' || **Args == '\t')
        (*Args)++;

    while (**Args != ' ' && **Args != '\t' && **Args != '\0')
        *buffer++ = *(*Args)++;

    *buffer = '\0';
}
