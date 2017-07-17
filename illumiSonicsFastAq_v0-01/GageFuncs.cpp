#include "GageFuncs.h"


//Do note that much of this code is taken from the GageScope C SDK example files.


// General Variables
CSSYSTEMINFO				CsSysInfo = { 0 };
CSACQUISITIONCONFIG			CsAcqCfg = { 0 };
CSACQUISITIONCONFIG			g_CsAcqCfg = { 0 };
CSAPPLICATIONDATA			CsAppData = { 0 };
OUT_PARAMS_TRANSFERDATA		OutParams = { 0 };
CONSOLE_SCREEN_BUFFER_INFO	ScreenInfo = { 0 };

CSHANDLE					hSystem = 0;
CSHANDLE					g_hSystem = 0;
BOOL						g_bAborted = FALSE;

int totalCount = 0;
int totalCount2 = 0;

// PARS Single Capture Variables
uInt32			i;
int32			i32Status = CS_SUCCESS;
int32			i32Token = 0;
DWORD			dwWritten;
DWORD			dwRet;
TCHAR			szMsg[100];
HANDLE			hTransferEvent = NULL;
HANDLE			hTriggerEvent = NULL;
HANDLE			hConsole = NULL;
HANDLE			hEndAcqEvent = NULL;
int64			i64Bytes = 0;
void*			pBuffer = NULL;
LPCTSTR			szIniFile = _T("PARSSingleCap.ini");
uInt32						u32Mode;
uInt32						u32ChannelIndexIncrement;
uInt32						u32ChannelNumber;
IN_PARAMS_TRANSFERDATA		InData = { 0 };
OUT_PARAMS_TRANSFERDATA		OutData = { 0 };
float*						pVBuffer = NULL;
int64*						pTimeStamp = NULL;
double*						pDoubleData;
int32						i32TickFrequency = 0;
uInt32						u32Count;
FILE*						fid;

TCHAR						PeakFileName[MAX_PATH];
TCHAR						SaveFileName[MAX_PATH];
FILE						*fidPeak;
int							nMeasureSize;
int							nMeasureTotal;
int							nEOMStates = 2;
int							nRawMeasureCount;
int16						*iRawData;
int							**mirrSave;
int							*eomSave;
int							mirrTemp, eomTemp;
int						*outputStream;
int							stageCount = 0;


// Real-Time variables
HANDLE		g_hThread[MAX_CARDS_COUNT] = { 0 };
LONGLONG	g_llLastSamples[MAX_CARDS_COUNT] = { 0 };
LONGLONG	g_llTotalSamples[MAX_CARDS_COUNT] = { 0 };
volatile HANDLE		g_hStreamStarted = NULL;
volatile HANDLE		g_hStreamAbort = NULL;
volatile HANDLE		g_hStreamError = NULL;
volatile HANDLE		g_hThreadReadyForStream = NULL;
int64		g_i64TsFrequency = 0;
int64		g_i64DataInSegment_Samples = 0;
uInt32		g_u32SegmentTail_Bytes = 0;
CsStreamConfig				g_StreamConfig;
DWORD						dwWaitStatus;
DWORD						dwThreadId;
volatile BOOL				bStreamExit = FALSE;

int64*						i16MaxValues;
uInt32						u32SegmentTail_Bytes = 0;
uInt32						u32DataSegmentInBytes = 0;
uInt32						u32TransferSize = 0;
void*						pBuffer1 = NULL;
void*						pBuffer2 = NULL;
void*						pCurrentBuffer = NULL;
void*						pWorkBuffer = NULL;
CSSTMCONFIG					StmConfig;



// PARS Single Capture Functions


int initializeGageSingleCap()
{
	// Initialize board
	i32Status = CsInitialize();

	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status);
	}
	
	// Get system handle
	i32Status = CsGetSystem(&hSystem, 0, 0, 0, 0);

	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status);
	}
	
	// Get system info
	CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CsGetSystemInfo(hSystem, &CsSysInfo);

	// Display name of board, currently disabled cause it doesn't really matter....	
	//_ftprintf(stdout, _T("Gage Board Name: %s\n"), CsSysInfo.strBoardName);
	
	// Configure the system
	i32Status = CsAs_ConfigureSystem(hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);

	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status)
		{			
			_ftprintf(stdout, _T("\nCannot find %s - using default parameters."), szIniFile);
		}
		else
		{
			_ftprintf(stdout, "\nCall to hardware failed, ensure other gage programs are closed.");
			programFailedSys(i32Status, hSystem);
		}
	}

	
	// Load settings from .ini file
	if (CS_USING_DEFAULT_ACQ_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for acquisition. Using defaults."));

	if (CS_USING_DEFAULT_CHANNEL_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for one or more Channels. Using defaults for missing items."));

	if (CS_USING_DEFAULT_TRIGGER_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for one or more Triggers. Using defaults for missing items."));

	i32Status = CsAs_LoadConfiguration(hSystem, szIniFile, APPLICATION_DATA, &CsAppData);

	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status)
		{
			_ftprintf(stdout, _T("\nUsing default application parameters."));
		}
		else
		{
			programFailedSys(i32Status, hSystem);
		}
	}
	else if (CS_USING_DEFAULT_APP_DATA & i32Status)
	{
		// If no .ini file is found
		_ftprintf(stdout, _T("\nNo ini entry for application data. Using defaults."));
	}

	// Apply settings to Gage card
	i32Status = CsDo(hSystem, ACTION_COMMIT);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}

	// Get current sample size
	CsAcqCfg.u32Size = sizeof(CsAcqCfg);
	i32Status = CsGet(hSystem, CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}

	// Allocate data transfer buffer
	pBuffer = VirtualAlloc(NULL, (size_t)(CsAppData.i64TransferLength * CsAcqCfg.u32SampleSize), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pBuffer)
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory\n"));
		CsFreeSystem(hSystem);
		return (-1);
	}

	// Initialize output variables
	nMeasureSize = (int)CsAppData.i64TransferLength;
	nMeasureTotal = (int)CsAppData.u32TransferSegmentCount;

	iRawData = new int16[nMeasureSize*nMeasureTotal];

	mirrSave = new int*[2];
	for (int n = 0; n < 2; n++)
	{
		mirrSave[n] = new int[nMeasureTotal];
	}

	eomSave = new int[nMeasureTotal];
	
	return (0);
}

int collectData()
{
	// Start data aquisition
	_ftprintf(stdout, "\nBegining Single Capture\n");
	i32Status = CsDo(hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}

	return 0;
}

int checkScanComplete()
{
	// Queries the system if it is completed
	if (!DataCaptureComplete(hSystem))
	{
		fprintf(stdout, ("Data Capture Failed\n\n"));
		CsFreeSystem(hSystem);
		getchar();
		return (-1);
	}
	return 0;
}

int saveGageData(LPCTSTR saveDirectory)
{
	// Get that transfer going!
	_ftprintf(stdout, "Transfering... ", u32ChannelNumber);
	_ftprintf(stdout, "Expected to take %d s... ", (int)((float)CsAppData.u32TransferSegmentCount*(44.0 / 1e6)*4));
	clock_t startClockTime = clock(), endClockTime;

	// Prepare inData structre for transfer
	InData.u32Mode = TxMODE_DEFAULT;
	InData.i64StartAddress = CsAppData.i64TransferStartPosition;
	InData.i64Length = CsAppData.i64TransferLength;
	InData.pDataBuffer = pBuffer;

	// Get this thing
	u32ChannelIndexIncrement = CsAs_CalculateChannelIndexIncrement(&CsAcqCfg, &CsSysInfo);

	// For each channel
	for (u32ChannelNumber = 1; u32ChannelNumber <= CsSysInfo.u32ChannelCount; u32ChannelNumber += u32ChannelIndexIncrement)
	{
		int nMulRecGroup;
		int nTimeStampIndex;
		nRawMeasureCount = 0;

		// Output buffer variable
		void* pSrcBuffer = NULL;

		ZeroMemory(pBuffer, (size_t)(CsAppData.i64TransferLength * CsAcqCfg.u32SampleSize));
		InData.u16Channel = (uInt16)u32ChannelNumber;

		// For each multiRec
		for (nMulRecGroup = CsAppData.u32TransferSegmentStart, nTimeStampIndex = 0; nMulRecGroup < (int)(CsAppData.u32TransferSegmentStart + CsAppData.u32TransferSegmentCount);
			nMulRecGroup++, nTimeStampIndex++)
		{
			// Transfer data
			InData.u32Segment = (uInt32)nMulRecGroup;
			i32Status = CsTransfer(hSystem, &InData, &OutData);

			// Check if failed for some reason
			if (CS_FAILED(i32Status))
			{
				DisplayErrorString(i32Status);
				if (NULL != pBuffer)
					VirtualFree(pBuffer, 0, MEM_RELEASE);
				if (NULL != pVBuffer)
					VirtualFree(pVBuffer, 0, MEM_RELEASE);
				if (NULL != pTimeStamp)
					VirtualFree(pTimeStamp, 0, MEM_RELEASE);
				if (NULL != pDoubleData)
					VirtualFree(pDoubleData, 0, MEM_RELEASE);
				CsFreeSystem(hSystem);
				return (-1);
			}
			
			// Get data out of buffer
			int16 *actualData = (int16 *)pBuffer;

			// If PA data (Ch1)
			if (u32ChannelNumber == 1)
			{
				int nSkipSize = nRawMeasureCount*nMeasureSize;

				for (int i = 0; i < nMeasureSize; i++)
				{
					iRawData[i + nSkipSize] = actualData[i];
				}		

			} // If xMirror data (Ch2)
			else if (u32ChannelNumber == 2)
			{
				// Take running average
				mirrTemp = 0;
				for (int i = 0; i < nMeasureSize; i++)
				{
					mirrTemp = mirrTemp + actualData[i];
				}

				mirrSave[0][nRawMeasureCount] = mirrTemp / nMeasureSize;

			} // If yMirror data (Ch3)
			else if (u32ChannelNumber == 3)
			{
				// Take running average
				mirrTemp = 0;
				for (int i = 0; i < nMeasureSize; i++)
				{
					mirrTemp = mirrTemp + actualData[i];
				}

				mirrSave[1][nRawMeasureCount] = mirrTemp / nMeasureSize;

			} // If EOM data (Ch4)
			else
			{
				// Take running average
				eomTemp = 0;
				for (int i = 0; i < nMeasureSize; i++)
				{
					eomTemp = eomTemp + actualData[i];
				}

				eomSave[nRawMeasureCount] = eomTemp / nMeasureSize;
			}
			
			
			
			// Increment for saves
			nRawMeasureCount++;
		}		 
	}

	// transfer is finished
	endClockTime = clock() - startClockTime;
	_ftprintf(stdout, "Completed (%4.2f MB)\n", (float)(244 * (float)InData.u32Segment) / 1000000.0);
	_ftprintf(stdout, "Transfer Completed in %d s.\n\n", (int)(endClockTime / (double)CLOCKS_PER_SEC));
	_ftprintf(stdout, "Saving Data... ");


	// Open Save file
	int nCount = 0;
	while (1)
	{
		// Generate name for count number
		nCount++;
		if (nCount < 10)
		{
			_stprintf(PeakFileName, _T("%sPARSScanData00%d.txt"), saveDirectory, nCount);
		} 
		else if(nCount >= 10 && nCount < 100)
		{
			_stprintf(PeakFileName, _T("%sPARSScanData0%d.txt"), saveDirectory, nCount);
		}
		else
		{
			_stprintf(PeakFileName, _T("%sPARSScanData%d.txt"), saveDirectory, nCount);
		}

		// If name is available then open for writting
		fidPeak = fopen(PeakFileName, "r");
		if (fidPeak)
		{
			fclose(fidPeak); // Otherwise close and move on
		}
		else
		{
			fidPeak = fopen(PeakFileName, "wb");
			break;
		}
	}
	
	
	// Build output string
	int paddingSize = 3; // How many calibration variables we have (3 for EOM, 2 without)
	int outputSize = (nMeasureSize + 3)*nMeasureTotal + paddingSize; // the +2 is for the mirror possition information (+3 with EOM is also there)
	outputStream = new int[outputSize];

	// Calibration variables
	outputStream[0] = nMeasureSize; // Samples per measurement
	outputStream[1] = nMeasureTotal; // Total amount of measurements
	outputStream[2] = nEOMStates; // Total number of EOOM states

	int measNum = 0;
	for (int i = paddingSize; i < outputSize; i += (nMeasureSize + 3))
	{
		// Add mirror data
		outputStream[i] = mirrSave[0][measNum];
		outputStream[i + 1] = mirrSave[1][measNum];

		// Add EOM data
		outputStream[i + 2] = eomSave[measNum];

		// Add PA data
		int nSkipSize = measNum*nMeasureSize;
		for (int j = i + 3, n = 0; j < i + 3 + nMeasureSize; j++,n++)
		{
			outputStream[j] = iRawData[n + nSkipSize];
		}
		measNum++;
	}

	fwrite(outputStream, sizeof(int), (size_t)outputSize, fidPeak);
	fclose(fidPeak);
	fidPeak = NULL;

	_ftprintf(stdout, "Complete!\n\n");

	delete iRawData;
	return 0;
}

int releaseGageSingleCap()
{
	// Free Buffers
	if (NULL != pVBuffer)
	{
		VirtualFree(pVBuffer, 0, MEM_RELEASE);
		pVBuffer = NULL;
	}

	if (NULL != pBuffer)
	{
		VirtualFree(pBuffer, 0, MEM_RELEASE);
		pBuffer = NULL;
	}

	// Free CompuScope system
	i32Status = CsFreeSystem(hSystem);
	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status);
	}	

	return (0);
}


// Real-Time Streaming Functions

int initializeGageStreamMT()
{
	int32						i32Status = CS_SUCCESS;
	uInt32						u32Mode;
	LPCTSTR						szIniFile = _T("ISS_Settings.ini");
	uInt32						u32DataSegmentWithTailInBytes = 0;
	

	// Initialize Board
	i32Status = CsInitialize();

	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status);
	}
	

	// Get system handle
	i32Status = CsGetSystem(&g_hSystem, 0, 0, 0, 0);
	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status);
	}
	

	// Get system Information
	CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CsGetSystemInfo(g_hSystem, &CsSysInfo);
	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status);
	}

		
	// Configure system from .ini file
	i32Status = CsAs_ConfigureSystem(g_hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);
	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status) // Invalid file name
		{
			_ftprintf(stdout, _T("\nCannot find %s - using default parameters."), szIniFile);
		}
		else
		{
			_ftprintf(stdout, "\nCall to hardware failed, ensure other gage programs are closed.");
			programFailedSys(i32Status, hSystem);
		}
	}


	// Load info from .ini file
	i32Status = LoadStreamConfiguration(szIniFile, &g_StreamConfig);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}
	if (CS_USING_DEFAULTS == i32Status)
	{
		_ftprintf(stdout, _T("\nNo ini entry for Stm configuration. Using defaults."));
	}
	

	// Check that board supports streaming
	i32Status = InitializeStream(g_hSystem);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}

	// Create events for streams
	g_hStreamStarted = CreateEvent(NULL, FALSE, FALSE, NULL);
	g_hStreamAbort = CreateEvent(NULL, TRUE, FALSE, NULL);
	g_hStreamError = CreateEvent(NULL, TRUE, FALSE, NULL);
	g_hThreadReadyForStream = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == g_hStreamStarted || NULL == g_hStreamAbort || NULL == g_hStreamError || NULL == g_hThreadReadyForStream)
	{
		_ftprintf(stderr, _T("\nUnable to create events for synchronization.\n"));
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	g_CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);


	// Get user aquisition data
	i32Status = CsGet(g_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &g_CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}

	// This is where you can intercept the .ini values and set sstuff manually in the g_CsAcqCfq variable and another one mayybe...

	
	// Commit values to driver
	i32Status = CsDo(g_hSystem, ACTION_COMMIT);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}


	// Get the timestamp frequency. The frequency will be changed depending on the channel mode (SINGLE, DUAL ...)
	i32Status = CsGet(g_hSystem, CS_PARAMS, CS_TIMESTAMP_TICKFREQUENCY, &g_i64TsFrequency);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}
	

	//  Create threads for Stream. In M/S system, we have to create one thread per card
	uInt16 n = 1, i = 0;

	for (n = 1, i = 0; n <= CsSysInfo.u32BoardCount; n++, i++)
	{
		g_hThread[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		
		(HANDLE)CreateThread(NULL, 0, CardStream, (void*)n, 0, &dwThreadId);
		if ((HANDLE)-1 == g_hThread[i])
		{
			// Fail to create the streaming thread for the n card.
			// Set the event g_hStreamAbort to terminate all threads
			SetEvent(g_hStreamAbort);
			_ftprintf(stderr, _T("Unable to create thread for card %d.\n"), n);
			CsFreeSystem(g_hSystem);
			return (-1);					
		}
	}

	return (0);
}

int gageStreamRealtimeMT()
{	
	int32						i32Status = CS_SUCCESS;
	uInt32						u32TickStart = 0;
	uInt32						u32TickNow = 0;
	volatile BOOL				bDone = FALSE;
	long long					llTotalSamples = 0;
	uInt16						n = 1, i = 0;
	int32						i32SegCount = 0;

	

	// Wait for the event g_g_hThreadReadyForStream to make sure that the thread was successfully created and are ready for stream
	if (WAIT_TIMEOUT == WaitForSingleObject(g_hThreadReadyForStream, 5000))
	{
		// Something is wrong. It is not suppose to take that long
		// Set the event g_hStreamAbort to terminate all threads
		_ftprintf(stderr, _T("Thread initialization error on card %d.\n"), n);
		SetEvent(g_hStreamAbort);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	int32 test = CsGetStatus(g_hSystem);
		
	// Start the data acquisition
	//printf("\nStart streaming. Press ESC to abort\n\n");
	i32Status = CsDo(g_hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{				
		programFailedSys(i32Status, hSystem);
	}

	u32TickStart = u32TickNow = GetTickCount();

	// Set the event g_hStreamStarted so that the other threads can start to transfer data
	SetEvent(g_hStreamStarted);


	// Main aquisition loop
	while (!bDone)
	{
		// Are we being asked to quit? 
		if (_kbhit())
		{
			switch (toupper(_getch()))
			{
			case 27:
				SetEvent(g_hStreamAbort);
				bDone = TRUE;
				break;
			default:
				MessageBeep(MB_ICONHAND);
				break;
			}
		}
				
		dwWaitStatus = WaitForMultipleObjects(CsSysInfo.u32BoardCount, g_hThread, TRUE, 1000);

		ResetEvent(g_hThread[0]);

		if (WAIT_OBJECT_0 == dwWaitStatus)
		{
			// All Streaming threads have terminated			
			bDone = TRUE;
		}

		u32TickNow = GetTickCount();

		// Calcaulate the sum of all data received so far
		llTotalSamples = 0;
		for (uInt16 i = 0; i < CsSysInfo.u32BoardCount; i++)
		{
			llTotalSamples += g_llTotalSamples[i];
		}

		
		//UpdateProgress(u32TickNow - u32TickStart, llTotalSamples);
	}

	ResetEvent(g_hStreamStarted);

	//	Abort the current acquisition 
	i32Status = CsDo(g_hSystem, ACTION_ABORT);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}
	
	return (0);
}

int32 LoadStreamConfiguration(LPCTSTR szIniFile, PCsStreamConfig pConfig)
{
	TCHAR	szDefault[MAX_PATH];
	TCHAR	szString[MAX_PATH];
	TCHAR	szFilePath[MAX_PATH];
	int		nDummy;

	CsStreamConfig CsStmCfg;
	/*
	Set defaults in case we can't read the ini file
	*/
	CsStmCfg.u32BufferSizeBytes = STREAM_BUFFERSZIZE;
	CsStmCfg.u32TransferTimeout = TRANSFER_TIMEOUT;
	strcpy(CsStmCfg.strResultFile, _T(OUT_FILE));

	if (NULL == pConfig)
	{
		return (CS_INVALID_PARAMETER);
	}

	GetFullPathName(szIniFile, MAX_PATH, szFilePath, NULL);

	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(szFilePath))
	{
		*pConfig = CsStmCfg;
		return (CS_USING_DEFAULTS);
	}

	if (0 == GetPrivateProfileSection(STM_SECTION, szString, 100, szFilePath))
	{
		*pConfig = CsStmCfg;
		return (CS_USING_DEFAULTS);
	}

	nDummy = 0;
	CsStmCfg.bDoAnalysis = (0 != GetPrivateProfileInt(STM_SECTION, _T("DoAnalysis"), nDummy, szFilePath));

	nDummy = CsStmCfg.u32TransferTimeout;
	CsStmCfg.u32TransferTimeout = GetPrivateProfileInt(STM_SECTION, _T("TimeoutOnTransfer"), nDummy, szFilePath);

	nDummy = CsStmCfg.u32BufferSizeBytes;
	CsStmCfg.u32BufferSizeBytes = GetPrivateProfileInt(STM_SECTION, _T("BufferSize"), nDummy, szFilePath);

	_stprintf(szDefault, _T("%s"), CsStmCfg.strResultFile);
	GetPrivateProfileString(STM_SECTION, _T("ResultsFile"), szDefault, szString, MAX_PATH, szFilePath);
	_tcscpy(CsStmCfg.strResultFile, szString);

	*pConfig = CsStmCfg;
	return (CS_SUCCESS);
}

DWORD WINAPI CardStream(void *CardIndex)
{
	uInt16				nCardIndex = ((uInt16 )CardIndex);
	void*				pBuffer1 = NULL;
	void*				pBuffer2 = NULL;
	void*				pCurrentBuffer = NULL;
	void*				pWorkBuffer = NULL;

	int64				i64DataInSegmentPlusTail_Bytes = 0;
	uInt32				u32TransferSize_Samples = 0;
	int32				i32Status;

	BOOL				bDone = FALSE;
	uInt32				u32LoopCount = 0;
	uInt32				u32ProcessedSegments = 0;
	uInt32				u32ErrorFlag = 0;
	HANDLE				WaitEvents[2];
	DWORD				dwWaitStatus;
	DWORD				dwRetCode = 0;
	uInt32				u32ActualLength = 0;
	uInt8				u8EndOfData = 0;
	BOOL				bStreamCompletedSuccess = FALSE;
	int64				i64SamplesRemainBeforeTs = 0;
	int64				i64BytesRemainBeforeTs = 0;
	uInt32				u32BytesRemainBeforeSegment = 0;
	int64*				pi64TimeStamp;

	FILE*				pFile = 0;
	uInt32				u32NbrFiles = 0;
	uInt32				u32SegmentCount = 0;

	// Calculate size of buffer needed
	g_i64DataInSegment_Samples = (g_CsAcqCfg.i64SegmentSize*(g_CsAcqCfg.u32Mode & CS_MASKED_MODE));

	// timestamp info
	i32Status = CsGet(g_hSystem, 0, CS_SEGMENTTAIL_SIZE_BYTES, &g_u32SegmentTail_Bytes);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		ExitThread(1);
	}

	// Round the buffersize into DWORD (4 bytes) boundary
	g_StreamConfig.u32BufferSizeBytes = g_StreamConfig.u32BufferSizeBytes / 4 * 4;

	/*	To simplify the algorithm in the analysis function, the buffer zize used for transfer should be multiple or a fraction of the size of segment
	Resize the buffer size.
	*/
	i64DataInSegmentPlusTail_Bytes = g_i64DataInSegment_Samples*g_CsAcqCfg.u32SampleSize + g_u32SegmentTail_Bytes;
	u32TransferSize_Samples = g_StreamConfig.u32BufferSizeBytes / g_CsAcqCfg.u32SampleSize;

	_ftprintf(stderr, _T("\nActual Transfer Size (Samples) = %d\n"), u32TransferSize_Samples);

	pi64TimeStamp = (int64 *)calloc(min(g_CsAcqCfg.u32SegmentCount, MAX_SEGMENTS_COUNT), sizeof(int64));
	if (NULL == pi64TimeStamp)
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory to hold analysis results.\n"));
		ExitThread(1);
	}

	// Allocate buffers for transfering data
	i32Status = CsStmAllocateBuffer(g_hSystem, nCardIndex, g_StreamConfig.u32BufferSizeBytes, &pBuffer1);
	if (CS_FAILED(i32Status))
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory for stream buffer 1.\n"));
		free(pi64TimeStamp);
		ExitThread(1);
	}

	i32Status = CsStmAllocateBuffer(g_hSystem, nCardIndex, g_StreamConfig.u32BufferSizeBytes, &pBuffer2);
	if (CS_FAILED(i32Status))
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory for stream buffer 2.\n"));
		CsStmFreeBuffer(g_hSystem, nCardIndex, pBuffer1);
		free(pi64TimeStamp);
		ExitThread(1);
	}


	while (!bStreamExit)
	{
		bStreamCompletedSuccess = FALSE;

		// So far so good ...
		// Let the main thread know that this thread is ready for stream
		SetEvent(g_hThreadReadyForStream);

		// Wait for the start acquisition event from the main thread
		WaitEvents[0] = g_hStreamStarted;
		WaitEvents[1] = g_hStreamAbort;
		dwWaitStatus = WaitForMultipleObjects(2, WaitEvents, FALSE, INFINITE);

		if ((WAIT_OBJECT_0 + 1) == dwWaitStatus)
		{
			// Aborted from user or error
			CsStmFreeBuffer(g_hSystem, nCardIndex, pBuffer1);
			CsStmFreeBuffer(g_hSystem, nCardIndex, pBuffer2);
			free(pi64TimeStamp);
			ExitThread(1);
		}

		// Stream aquitition started
		i64SamplesRemainBeforeTs = g_i64DataInSegment_Samples;
		i64BytesRemainBeforeTs = g_i64DataInSegment_Samples *g_CsAcqCfg.u32SampleSize;

		u32BytesRemainBeforeSegment = g_u32SegmentTail_Bytes;
		while (!(bDone || bStreamCompletedSuccess))
		{
			// Check if user has aborted or an error has occured
			if (WAIT_OBJECT_0 == WaitForSingleObject(g_hStreamAbort, 0))
				bDone = TRUE;

			// Flip transfer buffers
			if (u32LoopCount & 1)
			{
				pCurrentBuffer = pBuffer2;
			}
			else
			{
				pCurrentBuffer = pBuffer1;
			}

			i32Status = CsStmTransferToBuffer(g_hSystem, nCardIndex, pCurrentBuffer, u32TransferSize_Samples);
			if (CS_FAILED(i32Status))
			{
				DisplayErrorString(i32Status);
				break;
			}

			if (g_StreamConfig.bDoAnalysis)
			{
				if (NULL != pWorkBuffer)
				{									
					// Do analysis
					minMaxExtractMT(pWorkBuffer, g_CsAcqCfg.i64SegmentSize, u32TransferSize_Samples);
				}
			}

			// wait till transfer to current buffer is completed
			i32Status = CsStmGetTransferStatus(g_hSystem, nCardIndex, g_StreamConfig.u32TransferTimeout, &u32ErrorFlag, &u32ActualLength, &u8EndOfData);
			if (CS_SUCCEEDED(i32Status))
			{
				// Calculate the total of data for this card
				g_llTotalSamples[nCardIndex - 1] += u32TransferSize_Samples;
				bStreamCompletedSuccess = (0 != u8EndOfData);

				if (STM_TRANSFER_ERROR_FIFOFULL & u32ErrorFlag)
				{
					SetEvent(g_hStreamError);
					_ftprintf(stdout, _T("\nStream Fifo full on card %d !!!\n"), nCardIndex);
					bDone = TRUE;
				}
			}
			else
			{
				bDone = TRUE;
				if (CS_STM_TRANSFER_TIMEOUT == i32Status)
				{
					// timeout
					SetEvent(g_hStreamError);
					_ftprintf(stdout, _T("\nStream transfer timeout on card %d !!!\n"), nCardIndex);
				}
				else if (CS_STM_TRANSFER_ABORTED == i32Status)
				{
					_ftprintf(stdout, _T("\nStream transfer aborted on card %d !!!\n"), nCardIndex);
				}
				else // some other error 
				{
					SetEvent(g_hStreamError);
					DisplayErrorString(i32Status);
				}
			}

			pWorkBuffer = pCurrentBuffer;
			

			u32LoopCount++;
			//_ftprintf(stdout, _T("\nloopCount: %d\n"), u32LoopCount);
		}

		if (g_StreamConfig.bDoAnalysis)
		{
			// Do analysis on the last buffer
			//minMaxExtractMT(pWorkBuffer, g_CsAcqCfg.i64SegmentSize, u32TransferSize_Samples);
		}

		SetEvent(g_hThread[0]);
	}


	free(pi64TimeStamp);
	CsStmFreeBuffer(g_hSystem, 0, pBuffer1);
	CsStmFreeBuffer(g_hSystem, 0, pBuffer2);

	if (bStreamCompletedSuccess)
	{
		dwRetCode = 0;
	}
	else
	{
		// Stream operation has been aborted by user or errors
		dwRetCode = 1;
	}

	ExitThread(dwRetCode);	
}

void UpdateProgress(uInt32 u32Elapsed, LONGLONG llSamples)
{
	uInt32	h = 0;
	uInt32	m = 0;
	uInt32	s = 0;
	double	dRate;

	if (u32Elapsed > 0)
	{
		dRate = (llSamples / 1000000.0) / (u32Elapsed / 1000.0);

		if (u32Elapsed >= 1000)
		{
			if ((s = u32Elapsed / 1000) >= 60)	// Seconds
			{
				if ((m = s / 60) >= 60)			// Minutes
				{
					if ((h = m / 60) > 0)		// Hours
						m %= 60;
				}

				s %= 60;
			}
		}
		printf("\rTotal data: %0.2f MS, Rate  %6.2f MS/s, Elapsed Time: %u:%02u:%02u", llSamples / 1000000.0, dRate, h, m, s);
	}
}

/****************************************************
*****************************************************/


int initializeGageStream(bool bFastMix)
{
	int32						i32Status = CS_SUCCESS;	
	uInt32						u32Mode; 	
	uInt32						u32DataSegmentWithTailInBytes = 0;

	if (bFastMix)
	{
		szIniFile = _T("ISS_SettingsFastMix.ini");
	}
	else
	{
		szIniFile = _T("ISS_Settings.ini");
	}

	// Initialize board
	i32Status = CsInitialize(); 
	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status);
	}


	// Get system handle
	i32Status = CsGetSystem(&hSystem, 0, 0, 0, 0); 
	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status);
	}


	// Get system info
	CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CsGetSystemInfo(hSystem, &CsSysInfo);
	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status, hSystem);
	}


	// Configure system from .ini file
	i32Status = CsAs_ConfigureSystem(hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);
	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status) // Invalid file name
		{
			_ftprintf(stdout, _T("\nCannot find %s - using default parameters."), szIniFile);
		}
		else
		{
			_ftprintf(stdout, "\nCall to hardware failed, ensure other gage programs are closed.");
			programFailedSys(i32Status, hSystem);			
		}
	}


	// Check .ini settings
	checkIniSettings(i32Status);


	// Load application info from .ini
	i32Status = LoadStmConfiguration(szIniFile, &StmConfig);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}
	if (CS_USING_DEFAULTS == i32Status)
	{
		_ftprintf(stdout, _T("\nNo ini entry for Stm configuration. Using defaults."));
	}


	// Check that streaming is supported by card
	i32Status = InitializeStream(hSystem);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}


	// Get aquisition size
	CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);


	// Get user's aquisition data
	i32Status = CsGet(hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		programFailedSys(i32Status, hSystem);
	}

	
	// Allocate memory
	i16MaxValues = (int64 *)calloc(CsAcqCfg.u32SegmentCount, sizeof(int64));
	if (NULL == i16MaxValues)
	{
		memoryAllocFailed(hSystem);
	}


	// Get streaming tag information 
	i32Status = CsGet( hSystem, 0, CS_SEGMENTTAIL_SIZE_BYTES, &u32SegmentTail_Bytes);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		ExitThread(1);
	}


	// Allocate transfer buffers. Here we will use 2, so that one can be streamed to while the other is processed
	u32DataSegmentInBytes = (uInt32)(CsAcqCfg.i64SegmentSize * CsAcqCfg.u32SampleSize *(CsAcqCfg.u32Mode & CS_MASKED_MODE));
	u32DataSegmentWithTailInBytes = u32DataSegmentInBytes + u32SegmentTail_Bytes;
	u32TransferSize = u32DataSegmentWithTailInBytes / CsAcqCfg.u32SampleSize;

	i32Status = CsStmAllocateBuffer(hSystem, 0, u32DataSegmentWithTailInBytes, &pBuffer1);
	if (CS_FAILED(i32Status))
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory for stream buffer 1.\n"));
		free(i16MaxValues);
		CsFreeSystem(hSystem);
		return (-1);
	}

	i32Status = CsStmAllocateBuffer(hSystem, 0, u32DataSegmentWithTailInBytes, &pBuffer2);
	if (CS_FAILED(i32Status))
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory for stream buffer 2.\n"));
		CsStmFreeBuffer(hSystem, 0, pBuffer1);
		free(i16MaxValues);
		CsFreeSystem(hSystem);
		return (-1);
	}


	// Commit values to driver
	i32Status = CsDo(hSystem, ACTION_COMMIT);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		free(i16MaxValues);
		CsStmFreeBuffer(hSystem, 0, pBuffer1);
		CsStmFreeBuffer(hSystem, 0, pBuffer2);
		CsFreeSystem(hSystem);
		return (-1);
	}

	
	printf("\nCard initialized properly, ready to start streaming...\n\n");
	return 0;
}

int gageStreamRealtime()
{
	BOOL						bDone = FALSE;
	BOOL						bErrorData = FALSE;
	BOOL						bStreamCompletedSuccess = FALSE;
	uInt8						u8EndOfData = 0;
	int32						i32Status = CS_SUCCESS;	
	uInt32						u32TickStart = 0;
	uInt32						u32LoopCount = 0;
	uInt32						u32SaveCount = 0;
	uInt32						u32ErrorFlag = 0;
	uInt32						u32ActualLength = 0;
	long long					llTotalSamples = 0;
	

	printf("\nStarting stream. Press ESC to abort\n\n");


	// Start data aquisition
	i32Status = CsDo(hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		free(i16MaxValues);
		CsStmFreeBuffer(hSystem, 0, pBuffer1);
		CsStmFreeBuffer(hSystem, 0, pBuffer2);
		CsFreeSystem(hSystem);
		return (-1);
	}

	// get tick count
	u32TickStart = GetTickCount();


	// Primary capture loop
	// !(bDone || bStreamCompletedSuccess)
	while (1)
	{
		// Determine which buffer will get capture data
		if (u32LoopCount & 1)
		{
			pCurrentBuffer = pBuffer2;
		}
		else
		{
			pCurrentBuffer = pBuffer1;
		}

		i32Status = CsStmTransferToBuffer(hSystem, 0, pCurrentBuffer, u32TransferSize);
		if (CS_FAILED(i32Status))
		{
			DisplayErrorString(i32Status);
			break;
		}

		if (StmConfig.bDoAnalysis)
		{
			if (NULL != pWorkBuffer)
			{
				// Do analysis
				minMaxExtract(pWorkBuffer, CsAcqCfg.i64SegmentSize);
			}
		}
		
		
		// Wait for transfer on current buffer to finish
		i32Status = CsStmGetTransferStatus(hSystem, 0, StmConfig.u32TransferTimeout, &u32ErrorFlag, &u32ActualLength, &u8EndOfData);
		if (CS_SUCCEEDED(i32Status))
		{
			llTotalSamples += u32TransferSize;
			bStreamCompletedSuccess = (0 != u8EndOfData);

			if (STM_TRANSFER_ERROR_FIFOFULL & u32ErrorFlag)
			{
				_ftprintf(stdout, _T("\nStream Fifo full !!!"));
				bDone = TRUE;
			}
		}
		else
		{
			bDone = TRUE;
			if (CS_STM_TRANSFER_TIMEOUT == i32Status)
			{
				// Timeout
				_ftprintf(stdout, _T("\nStream transfer timeout. !!!"));
			}
			else if (CS_STM_TRANSFER_ABORTED == i32Status)
			{
				_ftprintf(stdout, _T("\nStream transfer aborted. !!!"));
			}
			else 
			{
				DisplayErrorString(i32Status);
			}
		}

		pWorkBuffer = pCurrentBuffer;

		u32LoopCount++;

		/* Are we being asked to quit? */
		if (_kbhit())
		{
			switch (toupper(_getch()))
			{
			case 27:
				bDone = TRUE;
				break;
			default:
				MessageBeep(MB_ICONHAND);
				break;
			}
		}

		// Show on the screen the result in every about 900ms sec 
		UpdateProgress(u32TickStart, 900, llTotalSamples);
	}


	// Abort current aquisition
	CsDo(hSystem, ACTION_ABORT);


	// Pull last data point from buffer
	if (StmConfig.bDoAnalysis)
	{
		if (u32SaveCount < CsAcqCfg.u32SegmentCount)
		{
			minMaxExtract(pWorkBuffer, CsAcqCfg.i64SegmentSize);
		}
	}
	
	return 0;
}

int gageStreamRealtimeFastMix()
{
	BOOL						bDone = FALSE;
	BOOL						bErrorData = FALSE;
	BOOL						bStreamCompletedSuccess = FALSE;
	uInt8						u8EndOfData = 0;
	int32						i32Status = CS_SUCCESS;
	uInt32						u32TickStart = 0;
	uInt32						u32LoopCount = 0;
	uInt32						u32SaveCount = 0;
	uInt32						u32ErrorFlag = 0;
	uInt32						u32ActualLength = 0;
	long long					llTotalSamples = 0;


	//printf("\nStarting stream. Press ESC to abort\n\n");


	// Start data aquisition
	i32Status = CsDo(hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		free(i16MaxValues);
		CsStmFreeBuffer(hSystem, 0, pBuffer1);
		CsStmFreeBuffer(hSystem, 0, pBuffer2);
		CsFreeSystem(hSystem);
		return (-1);
	}

	// get tick count
	u32TickStart = GetTickCount();


	// Primary capture loop
	// !(bDone || bStreamCompletedSuccess)
	while (1)
	{
		// Determine which buffer will get capture data
		if (u32LoopCount & 1)
		{
			pCurrentBuffer = pBuffer2;
		}
		else
		{
			pCurrentBuffer = pBuffer1;
		}

		i32Status = CsStmTransferToBuffer(hSystem, 0, pCurrentBuffer, u32TransferSize);
		if (CS_FAILED(i32Status))
		{
			DisplayErrorString(i32Status);
			break;
		}

		if (StmConfig.bDoAnalysis)
		{
			if (NULL != pWorkBuffer)
			{
				// Do analysis
				fastMixExtract(pWorkBuffer, CsAcqCfg.i64SegmentSize);
			}
		}


		// Wait for transfer on current buffer to finish
		i32Status = CsStmGetTransferStatus(hSystem, 0, StmConfig.u32TransferTimeout, &u32ErrorFlag, &u32ActualLength, &u8EndOfData);
		if (CS_SUCCEEDED(i32Status))
		{
			llTotalSamples += u32TransferSize;
			bStreamCompletedSuccess = (0 != u8EndOfData);

			if (STM_TRANSFER_ERROR_FIFOFULL & u32ErrorFlag)
			{
				_ftprintf(stdout, _T("\nStream Fifo full !!!"));
				bDone = TRUE;
			}
		}
		else
		{
			bDone = TRUE;
			if (CS_STM_TRANSFER_TIMEOUT == i32Status)
			{
				// Timeout
				_ftprintf(stdout, _T("\nStream transfer timeout. !!!"));
			}
			else if (CS_STM_TRANSFER_ABORTED == i32Status)
			{
				_ftprintf(stdout, _T("\nStream transfer aborted. !!!"));
			}
			else
			{
				DisplayErrorString(i32Status);
			}
		}

		pWorkBuffer = pCurrentBuffer;

		u32LoopCount++;

		/* Are we being asked to quit? */
		if (_kbhit())
		{
			switch (toupper(_getch()))
			{
			case 27:
				bDone = TRUE;
				break;
			default:
				MessageBeep(MB_ICONHAND);
				break;
			}
		}

		// Show on the screen the result in every about 900ms sec 
		UpdateProgress(u32TickStart, 900, llTotalSamples);
	}


	// Abort current aquisition
	CsDo(hSystem, ACTION_ABORT);


	// Pull last data point from buffer
	if (StmConfig.bDoAnalysis)
	{
		if (u32SaveCount < CsAcqCfg.u32SegmentCount)
		{
			fastMixExtract(pWorkBuffer, CsAcqCfg.i64SegmentSize);
		}
	}

	return 0;
}

int16 findMaxVal(void*  pWorkBuffer)
{
	// Pull values from puffer
	int16 *actualData = (int16 *)pWorkBuffer;

	return 0;
}

int32 InitializeStream(CSHANDLE hSystem)
{
	int32	i32Status = CS_SUCCESS;
	int64	i64ExtendedOptions = 0;

	CSACQUISITIONCONFIG CsAcqCfg = { 0 };

	CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);

	/*
	Get user's acquisition Data
	*/
	i32Status = CsGet(hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (i32Status);
	}
	/*
	Check if selected system supports Expert Stream
	And set the correct image to be used.
	*/
	CsGet(hSystem, CS_PARAMS, CS_EXTENDED_BOARD_OPTIONS, &i64ExtendedOptions);

	if (i64ExtendedOptions & CS_BBOPTIONS_STREAM)
	{
		_ftprintf(stdout, _T("\nSelecting Expert Stream from image 1."));
		CsAcqCfg.u32Mode |= CS_MODE_USER1;
	}
	else if ((i64ExtendedOptions >> 32) & CS_BBOPTIONS_STREAM)
	{
		_ftprintf(stdout, _T("\nSelecting Expert Stream from image 2."));
		CsAcqCfg.u32Mode |= CS_MODE_USER2;
	}
	else
	{
		_ftprintf(stdout, _T("\nCurrent system does not support Expert Stream."));
		_ftprintf(stdout, _T("\nApplication terminated."));
		return CS_MISC_ERROR;
	}

	/*
	Sets the Acquisition values down the driver, without any validation,
	for the Commit step which will validate system configuration.
	*/

	i32Status = CsSet(hSystem, CS_ACQUISITION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return CS_MISC_ERROR;
	}

	return CS_SUCCESS; // Success
}

int releaseGageRT()
{
	// Free Buffers
	if (NULL != pVBuffer)
	{
		VirtualFree(pVBuffer, 0, MEM_RELEASE);
		pVBuffer = NULL;
	}

	if (NULL != pBuffer)
	{
		VirtualFree(pBuffer, 0, MEM_RELEASE);
		pBuffer = NULL;
	}

	// Free CompuScope system
	i32Status = CsFreeSystem(hSystem);
	if (CS_FAILED(i32Status))
	{
		programFailed(i32Status);
	}

	return (0);
}


inline int programFailedF(int32 i32Status)
{
	DisplayErrorString(i32Status);
	return (-1);
}

inline int programFailedF(int32 i32Status, CSHANDLE hSystem)
{
	DisplayErrorString(i32Status);
	CsFreeSystem(hSystem);
	return (-1);
}

void checkIniSettings(int32 i32Status)
{
	if (CS_USING_DEFAULT_ACQ_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for acquisition. Using defaults."));

	if (CS_USING_DEFAULT_CHANNEL_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for one or more Channels. Using defaults for missing items."));

	if (CS_USING_DEFAULT_TRIGGER_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for one or more Triggers. Using defaults for missing items."));
}

int32 LoadStmConfiguration(LPCTSTR szIniFile, PCSSTMCONFIG pConfig)
{
	TCHAR	szDefault[MAX_PATH];
	TCHAR	szString[MAX_PATH];
	TCHAR	szFilePath[MAX_PATH];
	int		nDummy;

	CSSTMCONFIG CsStmCfg;
	/*
	Set defaults in case we can't read the ini file
	*/
	CsStmCfg.u32TransferTimeout = TRANSFER_TIMEOUT;
	strcpy(CsStmCfg.strResultFile, _T(OUT_FILE));

	if (NULL == pConfig)
	{
		return (CS_INVALID_PARAMETER);
	}

	GetFullPathName(szIniFile, MAX_PATH, szFilePath, NULL);

	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(szFilePath))
	{
		*pConfig = CsStmCfg;
		return (CS_USING_DEFAULTS);
	}

	if (0 == GetPrivateProfileSection(STM_SECTION, szString, 100, szFilePath))
	{
		*pConfig = CsStmCfg;
		return (CS_USING_DEFAULTS);
	}

	nDummy = 0;
	CsStmCfg.bDoAnalysis = (0 != GetPrivateProfileInt(STM_SECTION, _T("DoAnalysis"), nDummy, szFilePath));

	nDummy = CsStmCfg.u32TransferTimeout;
	CsStmCfg.u32TransferTimeout = GetPrivateProfileInt(STM_SECTION, _T("TimeoutOnTransfer"), nDummy, szFilePath);

	_stprintf(szDefault, _T("%s"), CsStmCfg.strResultFile);
	GetPrivateProfileString(STM_SECTION, _T("ResultsFile"), szDefault, szString, MAX_PATH, szFilePath);
	_tcscpy(CsStmCfg.strResultFile, szString);

	*pConfig = CsStmCfg;
	return (CS_SUCCESS);
}

void UpdateProgress(DWORD	dwTickStart, uInt32 u32UpdateInterval_ms, unsigned long long llSamples)
{
	static	DWORD	dwTickLastUpdate = 0;
	uInt32	h, m, s;
	DWORD	dwElapsed;
	DWORD	dwTickNow = GetTickCount();


	if ((dwTickNow - dwTickLastUpdate > u32UpdateInterval_ms))
	{
		double dRate;

		dwTickLastUpdate = dwTickNow;
		dwElapsed = dwTickNow - dwTickStart;

		if (dwElapsed)
		{
			dRate = (llSamples / 1000000.0) / (dwElapsed / 1000.0);
			h = m = s = 0;
			if (dwElapsed >= 1000)
			{
				if ((s = dwElapsed / 1000) >= 60)	// Seconds
				{
					if ((m = s / 60) >= 60)			// Minutes
					{
						if ((h = m / 60) > 0)		// Hours
							m %= 60;
					}

					s %= 60;
				}
			}
			printf("\rTotal data: %0.2f MS, Rate: %6.2f MS/s, Elapsed Time: %u:%02u:%02u  ", llSamples / 1000000.0, dRate, h, m, s);
		}
	}
}


// Mics Functions

uInt32 getSegmentCount()
{
	return CsAcqCfg.u32SegmentCount;
}

uInt32 getSegmentCountMT()
{
	return g_CsAcqCfg.u32SegmentCount;
}

