/* bitcalc.c : Defines the entry point for the console application.*/
/**/

#include "bitcalc.h"
#include "math.h"


static char g_line[256];         /* storage for reading standard input*/


int main(int argc, char* argv[])
{

   int nType;           /* menu selection*/

   do
   {
      printf ("\n");
      printf ("Bitcalc Main Menu\n");
      printf ("------------------------\n");
      printf (" 1. Calculate bits for IP1K100 Clock Generator Chip\n");
      printf (" 2. Calculate bits for IP1K110 Clock Generator Chip\n");
      printf (" 3. Calculate bits for IP1K125 Clock Generator Chip\n");
      printf (" 4. Calculate bits for IPEP20X Clock Generator Chip\n");
      printf ("99. Exit\n");
      nType = GetOption();
	  printf("\n\n");
	  if((nType > 0) && (nType < 5))
		nType = ClockGenSubmenu(nType);
   }while (nType != 99);

	return 0;
}






/**************************************************************************
*
* GetOption - helper function that returns menu selection item
*
* RETURNS:  int
*/
int GetOption (void)
{
   int nOption;
   printf ("Enter option: ");
   fgets (g_line, sizeof(g_line), stdin);
   sscanf (g_line, "%d", &nOption);
   return nOption;
}



int ClockGenSubmenu(int nType)
{
	int nCmd;            /* menu selection*/
	int status;
	short nIpClock;
	double dClockFreq;
	DWORD dwError;
	WORD wShiftHigh;
	WORD wShiftLow;
	BYTE bLength;
	double dError;
	WORD wClockReg1;
	WORD wClockReg2;
	BYTE bClockReg3;

	do
	{
		printf ("\n");
		if(nType == 1)
			printf ("IP1K100 Bitcalc Menu\n");
		if(nType == 2)
			printf ("IP1K110 Bitcalc Menu\n");
		if(nType == 3)
			printf ("IP1K125 Bitcalc Menu\n");
		if(nType == 4)
			printf ("IPEP20X Bitcalc Menu\n");
		printf ("------------------------\n");
		printf (" 1. Calculate bits for 8MHz IP Clock\n");
		printf (" 2. Calculate bits for 32MHz IP Clock\n");
		printf ("99. Return to Main Menu\n");
		
		nCmd = GetOption();

		if(nCmd == 99)
			return (0);
		
		printf("\n\n");
		
		/* clear status variables*/
		if (nCmd == 1)
		{
			nIpClock = 8;
		}
		if (nCmd == 2)
		{
			nIpClock = 32;
		}
		
		if (nType == 1)
		{
			do
			{
				printf ("Enter desired frequency in MHz:  ");
				fgets (g_line, sizeof (g_line), stdin);
				sscanf (g_line, "%lf", &dClockFreq);
				status = ClockCalc100 (dClockFreq, nIpClock, &dwError, &wShiftHigh, &wShiftLow, &bLength);
				if(status == 1)
					printf("Please input a Frequency greater than 0.390625 MHz\n\n");
				if(status == 2)
					printf("Please input a Frequency less than or equal to 100 MHz\n\n");\
			}while (status != 0);

			printf ("\nExpected Frequency Error (PPM):  %d\n", dwError);
			printf ("Shift High Register Word:        %4.4X\n", wShiftHigh);
			printf ("Shift Low Register Word:         %4.4X\n", wShiftLow);
			printf ("Length Register Byte:            %2.2X\n\n\n", bLength);
			nCmd = 0;
		}
		
		if ((nType == 2)||(nType == 3)||(nType == 4))
		{
			do
			{
				printf ("Enter desired frequency in MHz:  ");
				fgets (g_line, sizeof (g_line), stdin);
				sscanf (g_line, "%lf", &dClockFreq);
				status = ClockCalc110 (dClockFreq, nIpClock, &dError, &wClockReg1, &wClockReg2, &bClockReg3);
				if(status == 1)
					printf("Please input a Frequency greater than 0.25 MHz\n\n");
				if(status == 2)
					printf("Please input a Frequency less than or equal to 150 MHz\n\n");
				if(status == 3)
					printf("An unknown error has occured\n\n");
			}while (status != 0);

			printf ("\nExpected Frequency Error (PPM):  %4.1f\n", dError);
			printf ("Clock Control Register 1:        %4.4X\n", wClockReg1);
			printf ("Clock Control Register 2:        %4.4X\n", wClockReg2);
			printf ("Clock Control Register 3:          %2.2X\n\n\n", bClockReg3);
			nCmd = 0;
		}
		
	} while (nCmd != 99);
	
	return 0;
}











/* //////////////////////////////////////////////////////////////////////// */
/* ClockCalc100 - Calculates clock programming values for the IP1K100*/
/* */
/* RECEIVES:   dClockFreq  - the desired frequency in MHz*/
/*             nIpClock    - IP clock frequency in MHz (8 or 32)*/
/*             pdwError    - receives the expected frequency error (PPM)*/
/*             pwShiftHigh - receives the Shift High Register word*/
/*             pwShiftLow  - receives the Shift Low Register word*/
/*             pbLength    - receives the Length Register byte*/
/* 
*/
short ClockCalc100 (double dClockFreq, short nIpClock, DWORD* pdwError,
					WORD* pwShiftHigh, WORD* pwShiftLow, BYTE* pbLength)
{
	double Fref;               /* Reference frequency from FPGA (IP clock Frq / 2)*/
	double Fvco;               /* Intermediate frequency*/
	double finalFvco;          /* Fvco that is used in final calculation of Program Register Word*/
	double Fout;               /* Desired Output Frequency*/
	double P, Q;               /* P and Q */
	double LOG_2;              /* Constant expression to change log base from 10 to 2*/
	double error;              /* Error of output frequency*/
	DWORD error_ppm;           /* Error of output frequency in parts per million*/
	DWORD loop_error;          /* Error for the current loop*/
	DWORD index;               /* Set to 0 for Fvco <= 80MHz; Set to 0x8000 for Fvco > 80MHz*/
	DWORD outputword;          /* Variable for bit stuffing*/
	DWORD mask;                /* Variable for bit stuffing*/
	DWORD x;                   /* Variable for bit stuffing*/
	BYTE p, q;                 /* Temporary test P and Q*/
	BYTE Pa, Qa;               /* P and Q modified for writing to Program Register*/
	BYTE duty_cycle_up;        /* Set to 0x1 to increase duty cycly by 0.7 nS*/
	BYTE duty_cycle_down;      /* Set to 0x1 to decrease duty cycly by 0.7 nS*/
	BYTE length;               /* Length of Program/Control Word*/
	int factor;
	int final_factor;
	int upper_factor_limit;
	int lower_factor_limit;
	int high_bits;
	int i, j;
	
	LOG_2 = log10((double)2.0);                                  
	
	index = 0;
	final_factor = 0;
	Fref = nIpClock * 500000.0;      /*Change IP clock freq to hertz and divide it by 2    */
	Fout = dClockFreq * 1000000.0;          /*Change to hertz*/
	error_ppm = 1000000;                /*Initialize to a high value*/
	
	/*Check limits of Output*/
	if(dClockFreq < 0.390625)		/*Frequency must be at least 390 Khz*/
	{
		return 1;
	}
	if(dClockFreq > 100.0)			/*Frequency must be at most 100 Mhz*/
	{
		return 2;
	}
	
	/*Establish limits for divisor search*/
	lower_factor_limit = (int)(50 / dClockFreq) + 1;
	upper_factor_limit = (int)(150 / dClockFreq);
	if(upper_factor_limit > 128)
		upper_factor_limit = 128;
	factor = lower_factor_limit;
	
	/*Round the divisor up to the nearest binary number with only one high bit*/
	do
	{
		high_bits = 0;
		for(j = 1; j <= upper_factor_limit; j*=2)
		{
			if((factor & j) == j)
				high_bits++;
		}
		if(high_bits > 1)
			factor++;
	}
	while(high_bits > 1);
	
	/*Find the best P and Q based on the amount of error for each possible value*/
	for(; factor <= upper_factor_limit; factor *= 2)
	{
		Fvco = Fout * (double)factor;                /*Check limits of Fvco before proceeding*/
		if((Fvco < 150000000.0)&&(Fvco > 50000000.0))  /* Must be within the physical limits of the device*/
		{
			for(p = 4; p <= 130; p++)
			{
				for(q = 3; q <= 129; q++)
				{
					if(((Fref/((float)q)) > 200000) && ((Fref/((float)q)) < 1000000)) /*Check limits of q before proceeding*/
					{
						error = 1 - ( (2*Fref*p*(1/((float)q))) / Fvco );  /*Use equation for Fvco to find error value*/
						if(error < 0.000)
							error *= -1.0;                         /*Make sure error is a positive quantity*/
						loop_error = (long)(error * 1000000);
						
						/*Set the values to be used if they're the best so far*/
						if(loop_error < error_ppm)
						{
							P = p;
							Q = q;
							error_ppm = loop_error;
							finalFvco = Fvco;
							final_factor = (int)(log10((double)factor)/LOG_2 + 0.4);         /*The 0.4 is to make sure there are no truncations*/
						}
					}
				}
			}
		}
	}
	
	if(finalFvco > 80000000)
		index = 0x8;
	
	/*P and Q must be modified before writing to Program Register*/
	Pa = (BYTE)(P - 3);
	Qa = (BYTE)(Q - 2);
	
	duty_cycle_up = 0;
	duty_cycle_down = 0;
	
	outputword = 0;
	outputword |= index;
	outputword |= (Qa << 4);
	outputword |= (final_factor << 11);
	outputword |= (duty_cycle_up << 14);
	outputword |= (Pa << 15);
	
	/*Do Bit Stuffing as needed*/
	length = 22;
	for(mask = 0, i = 1; i < (1 << length); i <<= 1)  /*(length will not exceed 29)*/
	{
		mask |= i;                    /*Set mask to include all bits tested so far*/
		if((outputword & i))          /*Compare one bit at a time*/
			if(((outputword << 1) & i))      /*Compare the bit to the right*/
				if(((outputword << 2) & i))   /*Compare the bit two to the right*/
				{
					/*When three consecutive 1's are found:*/
					x = outputword & mask;  /*Set x to hold all the bits to the right of where the stuffing will occur*/
					outputword &= ~mask; /*Remove those same bits from outputword*/
					outputword <<= 1;    /*Shifting to the left results in an extra zero*/
					outputword |= x;     /*Put the Dword back together with the newly stuffed 0 inside*/
					length++;            /*Don't forget to increment the length*/
				}
	}
	
	/*Build the high and low Program Words*/
	
	*pdwError = error_ppm;
	*pbLength = length & 0xFF;
	*pwShiftHigh = (WORD)((outputword >> 16) & 0xffff);
	*pwShiftLow = (WORD)(outputword & 0xffff);
	
	return 0;
}




/* //////////////////////////////////////////////////////////////////////// */
/* ClockCalc110 - Calculates clock programming values for the IP1K110/125*/
/*                (Code lifted from BitCalc2k1 utility)*/
/* */
/* RECEIVES:   dClockFreq  - the desired frequency in MHz*/
/*             nIpClock    - IP clock frequency in MHz (8 or 32)*/
/*             pdError     - receives the expected frequency error (PPM)*/
/*             pwClockReg1 - receives the Clock Register 1 word*/
/*             pwClockReg2 - receives the Clock Register 2 word*/
/*             pwClockReg3 - receives the Clock Register 3 byte*/
/* 
*/
short ClockCalc110 (double dClockFreq, short nIpClock, double* pdError,
					WORD* pwClockReg1, WORD* pwClockReg2, BYTE* pbClockReg3)
{
	double userfreq, clk, test1, test2, test3, expclk;
	double temperror;
	double p,q,pd,m;
	double error = 999999999;
	unsigned long pcur, qcur, pdcur;
	unsigned long control1 = 0;
	unsigned long control2 = 0;
	unsigned long control3 = 0;
	int PO = 0;
	int PB;
	double RefFreq, test3error;
	int multiplier;
	double temp, stderror;
	int pdflag = 0;
	int pdonly, o;
	
	/*Determine the clock speed*/
	if(nIpClock == 32)
	{
		control3 = 1;
		RefFreq = 32000000;
	}
	else
		RefFreq = 8000000;
	
	/*Check limits of Output*/
	if(dClockFreq < 0.25)		/*Frequency must be at least 250 Khz*/
	{
		return 1;
	}
	if(dClockFreq > 150.0)			/*Frequency must be at most 150 Mhz*/
	{
		return 2;
	}
	
	userfreq = dClockFreq;
	
	userfreq = userfreq * 1000000;
	
	/*determine post divider value so that VCO is close to 400MHz*/
	for(o=1;o<=127;o++)
	{
		temp= o * userfreq;
		if(temp>400000000)
		{
			/*account for one extra in loop above that is above limit*/
			multiplier = o - 1;
			break;
		}
		if (o==127)
			multiplier = 127;
	}
	
	/*loop to determine the value of p, q, and the post divider*/
	
	for(m=0;m<=10;m++)
	{
		pd = multiplier - m;
		/*prevent divide by zero error*/
		if (pd==0)
		{
			break;
		}
		
		for (p=8;p<=1023; p++)
		{
			for (q=2; q<=129;q++)
			{
				clk = ((RefFreq*p)/q)/pd;
				test2 = RefFreq/q;
				test1 = p * test2;
				
				/*conditions required by Cypress*/
				if ((test1>100000000) && (test1<=400000000) && (test2>250000))
				{
					temperror = fabs(userfreq-clk);
					
					
					if ((temperror < error))
					{
						
						error = temperror;
						pcur = (DWORD)p;
						qcur = (DWORD)q;
						pdcur = (DWORD)pd;
					}
				}
			}
		}
	}
	
	/*method two for small frequencies does RefFreq/Post Divider Only*/
	if (userfreq < RefFreq)
	{
		for(pdonly = 2; pdonly < 128; pdonly++)
		{
			test3 = RefFreq/pdonly;
			test3error = fabs(userfreq-test3);
			
			if ((test3error <= error))
			{
				error = test3error;
				pcur = 0;
				qcur = 0;
				pdcur = pdonly;
				pdflag = 1;
			}
		}
	}
	
	/*Calculations that are sensative to which method was used.  */
	if (pdflag == 0)
	{
		/*calculate expected clock frequency for output*/
		expclk = ((RefFreq*pcur)/qcur)/pdcur/1000000;
		
		/*qtotal=quser - 2*/
		/*q is also left shifted (logical) by one byte since it resides in the most significant byte*/
		qcur = qcur - 2;
		qcur <<= 8;
		control1 |= qcur;
		
		/*Calculate PO and PB.*/
		if ((pcur % 2) == 1)
		{
			PO = 1;
			control1 |= 0x8000;
		}
		
		PB = ((pcur - PO)/2) - 4;
		control2 |= PB;
	}
	else if(pdflag == 1)  /*pdflag == 1   PO and PB and Q are 0*/
	{
		/*calculate expected clock frequency for output*/
		expclk = RefFreq/pdcur/1000000;
		
		/*Program correct value for DIV1SRC*/
		control1 |= 0x80;
	}
	
	/*Find proper value for PUMP*/
	if (pcur <= 44)
		control2 |= 0;
	else if (pcur <= 479)
		control2 |= 0x400;
	else if (pcur <= 693)
		control2 |= 0x800;
	else if (pcur <= 799)
		control2 |= 0xC00;
	else if (pcur <= 1023)
		control2 |= 0x1000;
	else
		/* BitCalc has an "internal error" AfxMessageBox here;*/
		/* I ran this function in a loop through the acceptable range and never*/
		/* got here.  Return a bogus error code just in case.*/
		return 3;
	
	/*Find proper value for CLKSRC and div1N(pd) values */
	if(pdcur == 2)
	{
		control2 |= 0x4000;
		control1 |= 0x4;
	}
	else if(pdcur == 3)
	{
		control2 |= 0x6000;
		control1 |= 0x6;
	}
	else
	{
		control2 |= 0x2000;
		control1 |= pdcur;
	}
	
	stderror = (((expclk*1000000) - userfreq) / userfreq *1000000);
	
	*pdError = stderror;
	*pwClockReg1 = (WORD)(control1 & 0xFFFF);
	*pwClockReg2 = (WORD)(control2 & 0xFFFF);
	*pbClockReg3 = (BYTE)(control3 & 0xFF);
	
	return 0;
}
