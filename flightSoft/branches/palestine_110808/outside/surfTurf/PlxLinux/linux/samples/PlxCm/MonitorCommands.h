#ifndef _MONITOR_COMMANDS_H
#define _MONITOR_COMMANDS_H

/******************************************************************************
 *
 * File Name:
 *
 *      MonitorCommands.h
 *
 * Description:
 *
 *      Header for the Monitor commands
 *
 * Revision History:
 *
 *      05-31-02 : PCI SDK v3.50
 *
 ******************************************************************************/


#include "RegisterDefs.h"




/*************************************
*          Functions
*************************************/
void
Command_Version(
    void
    );

void
Command_Help(
    void
    );

void
Command_Dev(
    char *Args
    );

void
Command_Variables(
    void
    );

void
Command_DisplayCommonBufferProperties(
    void
    );

void
Command_Scan(
    void
    );

void
Command_Reset(
    void
    );

void
Command_PciRegs(
    char *Args
    );

void
Command_Regs(
    U8    Regs,
    char *Args
    );

void
Command_Eep(
    char *Args
    );

void
Command_MemRead(
    char *Args,
    char  size
    );

void
Command_MemWrite(
    char *Args,
    char  size
    );

void
Command_IoRead(
    char *Args,
    char  size
    );

void
Command_IoWrite(
    char *Args,
    char  size
    );



#endif
