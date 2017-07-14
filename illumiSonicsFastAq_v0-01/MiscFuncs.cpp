#include "MiscFuncs.h"





void illuWelcome()
{
	_ftprintf(stdout, "\n-------------------------------------");
	_ftprintf(stdout, "\n-    IllumiSonics Scope v0.01.02    -");
	_ftprintf(stdout, "\n-------------------------------------\n");
	_ftprintf(stdout, "Previously Modified 2016-10-19");
}

int getScanMode()
{
	int response;
	_ftprintf(stdout, "\n\n\n New Scan \n");
	_ftprintf(stdout,     "----------\n(0 = Exit Program, 1 = PARS Single Capture, 2 = PARS Real-Time)\nPlease enter mode: ");
	int returnVal = scanf("%d", &response);
	if (!returnVal)
	{
		response = 0;		
	}

	fflush(stdin); // flush stdin buffer
	return response;
}

void PARSWelcome(LPCTSTR saveDirectory)
{
	_ftprintf(stdout, "\n\n\n");
	_ftprintf(stdout, "\n-    PARS Scan    -");
	_ftprintf(stdout, "\n-------------------\n");
	_ftprintf(stdout, "Current save directory: %s\n", saveDirectory);
}

void PARSRTWelcome()
{
	_ftprintf(stdout, "\n\n\n");
	_ftprintf(stdout, "\n-    PARS Real-Time    -");
	_ftprintf(stdout, "\n------------------------\n");
}