/****************************************************************************
* Copyright (c) 2001 SBS Technologies, Inc.
*
* HARDWARE        : CR7
*
* PROGRAM NAME    : libcr7.so
* MODULE NAME     : cr7.h
* VERSION         : 1.00
* TOOL            : GNU Toolchain
*
* SOFTWARE DESIGN : Michael Zappe, derived from V5C BSP from Jan Allen
* BEGINNING DATE  : 06/04/01
* DESCRIPTION     : Header file for CR7 BSP defines
*
*
* REV   BY   DATE      CHANGE
*---------------------------------------------------------------------------
*
*
****************************************************************************/
#if !defined(__cr7_h__)
#define __cr7_h__

#if defined(__KERNEL__)

#include <linux/config.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/init.h>

#endif

#define GD_CR7_NAME               "CR7"
#define GD_CR7_RELEASE_VERSION    "v1.0"

#define TRUE                   1
#define FALSE                  0

#define GD_CR7_LED_PROC_ENTRY                 "cr7/status-led"
#define GD_CR7_WDG_ENABLE_PROC_ENTRY          "cr7/watchdog-enable"
#define GD_CR7_WDG_DID_RESET_PROC_ENTRY       "cr7/watchdog-did-reset"

#define GD_CR7_EEPROM_DEFAULT_LENGTH  512

typedef int API_RESULT;

#define API_SUCCESS                   0
#define API_INTERNAL_DRIVER_ERROR    -1
#define API_DRIVER_NOT_LOADED        -2
#define API_OUT_OF_MEMORY            -3
#define API_FUNCTION_NOT_PRESENT     -4

/**************************************************
 * Watchdog Timer Functions
 **************************************************/
int fn_ApiCr7EnableWatchdog(int);
int fn_ApiCr7PingWatchdog(void);
int fn_ApiCr7DidWatchdogReset(void);
int fn_ApiCr7ResetWatchdogResetFlag(void);

/**************************************************
 * Temperature Monitor Functions
 **************************************************/
int fn_ApiCr7ReadLocalTemperature(int *);
int fn_ApiCr7ReadRemoteTemperature(int *);
int fn_ApiCr7ReadLocalTHigh(int *);
int fn_ApiCr7SetLocalTHigh(int);
int fn_ApiCr7ReadRemoteTHigh(int *);
int fn_ApiCr7SetRemoteTHigh(int);
int fn_ApiCr7ReadLocalTLow(int *);
int fn_ApiCr7SetLocalTLow(int);
int fn_ApiCr7ReadRemoteTLow(int *);
int fn_ApiCr7SetRemoteTLow(int);
int fn_ApiCr7ReadConfiguration(int *);
int fn_ApiCr7SetConfiguration(int);
int fn_ApiCr7ReadConversionRate(int *);
int fn_ApiCr7SetConversionRate(int);
int fn_ApiCr7ReadStatus(int *);
int fn_ApiCr7ResetTempSensor(void);
int fn_ApiCr7ForceTempReading(void);

/**************************************************
 * Serial EEPROM Functions
 **************************************************/
int fn_ApiCr7SerialEEPROMRead(unsigned int, char *, unsigned int);
int fn_ApiCr7SerialEEPROMWrite(unsigned int, const char *, unsigned int);
int fn_ApiCr7SerialEEPROMWriteAndVerify(unsigned int, const char *, unsigned int);

/**************************************************
 * Status LED Functions
 **************************************************/
int fn_ApiCr7GetStatusLEDState(void);
int fn_ApiCr7SetStatusLEDState(int);

const char *fn_ApiGetErrorMsg(API_RESULT);

#endif
