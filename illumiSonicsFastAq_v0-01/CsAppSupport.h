
#ifndef __CS_APP_SUPPORT_H__
#define __CS_APP_SUPPORT_H__

#include <windows.h>
#include <stdio.h>
#include "CsPrototypes.h"

#if defined (_WIN32)
#define CS_SUPPORT_API __stdcall
#else
#define CS_SUPPORT_API
#endif

#ifndef CS_MASKED_MODE
#define CS_MASKED_MODE			0x0000000f
#endif

#ifndef CS_MODE_OCT
#define CS_MODE_OCT     		0x8
#endif

static const int TYPE_DEC	= 0;
static const int TYPE_HEX	= 1;
static const int TYPE_FLOAT = 2;
static const int TYPE_SIG   = 3;
static const int TYPE_BIN   = 4;

/* If this is the value passed for time stamp then ignore it */
static const double NO_TIME_STAMP_VALUE = -1.0;

typedef struct _FILEHEADERSTRUCT
{
	int64 i64Start;
	int64 i64Length;
	int64 i64SampleRate;
	double dTimeStamp;
	uInt32 u32SegmentNumber;
	uInt32 u32SampleSize;
	int32 i32SampleRes;
	int32 i32SampleOffset;
	uInt32 u32InputRange;
	int32 i32DcOffset;
	uInt32 u32SegmentCount;
	uInt32 u32AverageCount;
} FileHeaderStruct;

#ifndef INVALID_FILE_ATTRIBUTES
	#pragma message("WARNING: Need to have the Platform SDK installed and configured")
#endif /* !INVALID_FILE_ATTRIBUTES */

/* Defines used for error codes when saving a file */


/* Defines used for return values when configuring a system */

#define CS_USING_DEFAULT_ACQ_DATA		2
#define CS_USING_DEFAULT_CHANNEL_DATA	4
#define CS_USING_DEFAULT_TRIGGER_DATA	8
#define CS_USING_DEFAULT_APP_DATA		16
#define CS_USING_DEFAULTS				32

/* Defines for what to configure */
#define ACQUISITION_DATA	1
#define CHANNEL_DATA		2
#define TRIGGER_DATA		3
#define APPLICATION_DATA	4


#define DEFAULT_TRANSFER_START_POSITION 0
#define DEFAULT_TRANSFER_LENGTH 4096
#define DEFAULT_TRANSFER_SEGMENT_START 1
#define DEFAULT_TRANSFER_SEGMENT_COUNT 1
#define DEFAULT_PAGE_SIZE 32768
#define DEFAULT_SAVE_FILE_NAME _T("GAGE_FILE")
#define DEFAULT_SAVE_FILE_FORMAT TYPE_DEC
#define DEFAULT_FIR_DATA_FILE_NAME _T("") // no file

typedef struct
{
	int64 i64TransferStartPosition;
	int64 i64TransferLength;
	uInt32 u32TransferSegmentStart;
	uInt32 u32TransferSegmentCount; // -1 means all
	uInt32 u32PageSize;
	int32 i32SaveFormat;
	TCHAR lpszSaveFileName[100];
	TCHAR lpszFirDataFileName[100];
} CSAPPLICATIONDATA, *PCSAPPLICATIONDATA;

#ifdef __cplusplus
extern "C"{
#endif

int32 SetAcquisitionParameters(const CSHANDLE hSystem, const CSACQUISITIONCONFIG* const pCsAcquisitionCfg);
int32 SetChannelParameters(const CSHANDLE hSystem, const CSCHANNELCONFIG* const pCsChannelCfg);
int32 SetTriggerParameters(const CSHANDLE hSystem, const CSTRIGGERCONFIG* const pCsTriggerCfg);
int32 CS_SUPPORT_API CsAs_ConvertToVolts(const int64 i64Depth, const uInt32 u32InputRange, const uInt32 u32SampleSize, 
										 const int32 i32SampleOffset, const int32 i32SampleResolution, const int32 i32DcOffset,
										 const void* const pBuffer,  float* const pVBuffer);
uInt32 CS_SUPPORT_API CsAs_CalculateChannelIndexIncrement(const CSACQUISITIONCONFIG* const pAqcCfg, const CSSYSTEMINFO* const pSysInfo );


int32 CS_SUPPORT_API CsAs_ConfigureSystem(const CSHANDLE hSystem, const int nChannelCount, const int nTriggerCount, const LPCTSTR szIniFile, uInt32* pu32Mode);

/*
	Ini file must have complete path
*/
int32 CS_SUPPORT_API CsAs_LoadConfiguration(const CSHANDLE hSystem, const LPCTSTR szIniFile, const int32 i32Type, void* pConfig);

int32 LoadAcquisitionConfiguration(const CSHANDLE hSystem, const LPCTSTR szIniFile, PCSACQUISITIONCONFIG pConfig);
int32 LoadChannelConfiguration(const CSHANDLE hSystem, const LPCTSTR szIniFile, PCSCHANNELCONFIG pConfig);
int32 LoadTriggerConfiguration(const CSHANDLE hSystem, const LPCTSTR szIniFile, PCSTRIGGERCONFIG pConfig);
int32 LoadApplicationData(const LPCTSTR szIniFile, PCSAPPLICATIONDATA pConfig);
void CS_SUPPORT_API CsAs_SetApplicationDefaults(CSAPPLICATIONDATA *pAppData);



int32 CS_SUPPORT_API CsAs_SaveFile(LPCTSTR szFileName, void* pBuffer, const int sType, FileHeaderStruct *pHeader);
int32 CS_SUPPORT_API CsAs_SaveSigFile(LPCTSTR szFileName, LPCTSTR szChannelName, LPCTSTR szChannelComment, const void* const pBuffer, const size_t szSize, CSSIGSTRUCT* pSigStruct);
LPCTSTR CS_SUPPORT_API CsAs_GetLastFileError(void);

#ifdef __cplusplus
}
#endif 

#endif // __CS_APP_SUPPORT_H__