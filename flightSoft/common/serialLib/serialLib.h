/*! \file socketLib.h
    \brief Some useful functions when you're using the serial/whiteheat communications
    
    For now it's just a couple of functions, but it may well grow.
   
   April 2005 rjn@mps.ohio-state.edu
*/
/*
 * Current CVS Tag:
 * $Header: /work1/anitaCVS/flightSoft/common/serialLib/serialLib.h,v 1.1 2005/04/01 16:48:25 rjn Exp $
 */
 
/* 
 * Modification History : DO NOT EDIT - MAINTAINED BY CVS 
 * $Log: serialLib.h,v $
 * Revision 1.1  2005/04/01 16:48:25  rjn
 * Added serialLib which will house all the functions necessary for talking through the whiteheat, and other serial connections.
 *
 * Revision 1.1  2004/09/01 14:49:17  rjn
 * Renamed mySerial to serialLib
 *
 *
 */ 
#ifndef SERIALLIB_H
#define SERIALLIB_H

#define BAUDRATE B9600
#define CHARACTER_SIZE CS8

/* Function that just toggles the CRTSCTS flag, needed after a reboot for some reason. */
int openGPSDevice(char devName[]);
int openMagnetometerDevice(char devName[]);
int toggleCRTCTS(char devName[]);
int setGPSTerminalOptions(int fd);
int setMagnetometerTerminalOptions(int fd);

#endif /* SERIALLIB_H */
