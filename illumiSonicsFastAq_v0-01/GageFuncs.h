#ifndef GAGEFUNCS_H
#define GAGEFUNCS_H

#include <complex>
#include <cmath>
#include <windows.h>
#include <stdio.h>
#include <CsPrototypes.h>
#include "CsExpert.h"

#include "Source.h"
#include "CsAppSupport.h"
#include "CsTchar.h"
#include "CsSdkMisc.h"

#define	MAX_CARDS_COUNT		10					/* Max number of cards supported in a M/S Compuscope system */
#define TRANSFER_TIMEOUT	10000
#define STREAM_BUFFERSZIZE	0x200000
#define STM_SECTION			 _T("StmConfig")	/* section name in ini file */
#define MAX_SEGMENTS_COUNT	25000				/* Max number of segments in a single file */
#define OUT_FILE			"BufferSums.txt"	/* name of the output file */


#define programFailed(i32Status)\
{\
	DisplayErrorString(i32Status);\
	return (-1);\
}

#define programFailedSys(i32Status,hSystem)\
{\
	DisplayErrorString(i32Status);\
	CsFreeSystem(hSystem);\
	return (-1);\
}

#define memoryAllocFailed(hSystem)\
{\
	_ftprintf(stderr, _T("\nUnable to allocate memory to hold analysis results.\n"));\
	CsFreeSystem(hSystem);\
	return (-1);\
}


typedef struct
{
	uInt32		u32TransferTimeout;
	TCHAR		strResultFile[MAX_PATH];
	BOOL		bDoAnalysis;			/* Turn on or off data analysis */
}CSSTMCONFIG, *PCSSTMCONFIG;

typedef struct
{
	uInt32		u32BufferSizeBytes;
	uInt32		u32TransferTimeout;
	TCHAR		strResultFile[MAX_PATH];
	BOOL		bDoAnalysis;			/* Turn on or off data analysis */
}CsStreamConfig, *PCsStreamConfig;


// Functions

// PARS single Capture
int initializeGageSingleCap();
int collectData();
int checkScanComplete();
int saveGageData(LPCTSTR saveDirectory);
int releaseGageSingleCap();

// MT Gage car functions
int initializeGageStreamMT();
int gageStreamRealtimeMT();
int32	LoadStreamConfiguration(LPCTSTR szIniFile, PCsStreamConfig pConfig);
DWORD WINAPI CardStream(void *CardIndex);
void UpdateProgress(uInt32 u32Elapsed, LONGLONG llSamples);

// Real-Time functions
int initializeGageStream();
int gageStreamRealtime();
int16 findMaxVal(void*  pWorkBuffer);
int32 InitializeStream(CSHANDLE hSystem);
int releaseGageRT();

int programFailedF(int32 i32Status);
int programFailedF(int32 i32Status, CSHANDLE hSystem);
void checkIniSettings(int32 i32Status);
int32 LoadStmConfiguration(LPCTSTR szIniFile, PCSSTMCONFIG pConfig);
void UpdateProgress(DWORD dwTickStart, uInt32 u32UpdateInterval, unsigned long long llSamples);

// Mics Functions
uInt32 getSegmentCount();
uInt32 getSegmentCountMT();

#endif




