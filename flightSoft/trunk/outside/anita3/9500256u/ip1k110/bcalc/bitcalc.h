/* bitcalc.h - header for bitcalc console app  */

/* Acromag Incorporated */

#define DWORD unsigned long
#define WORD unsigned short
#define BYTE unsigned char
#include <stdio.h>


int GetOption (void);

int Calculate (void);
int ClockGenSubmenu(int nType);

short ClockCalc100(double dClockFreq, short nIpClock, DWORD* ,
                             WORD* , WORD* , BYTE* );
short ClockCalc110 (double dClockFreq, short nIpClock, double* ,
                              WORD* , WORD* , BYTE* );


