#include "GageFuncs.h"



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
LPCTSTR			szIniFile = _T("ASTransfer.ini");
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
int							nRawMeasureCount;
int16						*iRawData;
int							**saveStage;
int							stageCount = 0;


CSSYSTEMINFO				CsSysInfo = { 0 };
CSACQUISITIONCONFIG			CsAcqCfg = { 0 };
CSAPPLICATIONDATA			CsAppData = { 0 };
OUT_PARAMS_TRANSFERDATA		OutParams = { 0 };
CONSOLE_SCREEN_BUFFER_INFO	ScreenInfo = { 0 };

CSHANDLE					hSystem = 0;
CSHANDLE					g_hSystem = 0;
BOOL						g_bAborted = FALSE;


BOOL WINAPI ControlHandler(DWORD dwCtrlType);
int32 TransferTimeStamp(CSHANDLE hSystem, uInt32 u32SegmentStart, uInt32 u32SegmentCount, void* pTimeStamp);

int initializeGageAS()
{
	fprintf(stdout, ("\nInitializing Gage card... "));

	/*
	Initializes the CompuScope boards found in the system. If the
	system is not found a message with the error code will appear.
	Otherwise i32Status will contain the number of systems found.
	*/
	i32Status = CsInitialize();

	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}

	/*
	Get the system. This sample program only supports one system. If
	2 systems or more are found, the first system that is found
	will be the system that will be used. hSystem will hold a unique
	system identifier that is used when referencing the system.
	*/
	i32Status = CsGetSystem(&g_hSystem, 0, 0, 0, 0);

	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}

	/*
	Get the handle for the events.
	Instead of polling for the system status, we will wait for specific events to occur.
	If the call did fail, (i.e. Events not supported) we should free the CompuScope system
	so it's available for another application
	*/
	i32Status = CsGetEventHandle(g_hSystem, ACQ_EVENT_TRIGGERED, &hTriggerEvent);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	i32Status = CsGetEventHandle(g_hSystem, ACQ_EVENT_END_BUSY, &hEndAcqEvent);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	/*
	Get System information. The u32Size field must be filled in
	prior to calling CsGetSystemInfo
	*/
	CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CsGetSystemInfo(g_hSystem, &CsSysInfo);

	/*
	Display the system name from the driver
	*/
	_ftprintf(stdout, _T("Board Name: %s\n\n"), CsSysInfo.strBoardName);

	i32Status = CsAs_ConfigureSystem(g_hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);

	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status)
		{
			/*
			Display message but continue on using defaults.
			*/
			_ftprintf(stdout, _T("\nCannot find %s - using default parameters."), szIniFile);
		}
		else
		{
			/*
			Otherwise the call failed.  If the call did fail we should free the CompuScope
			system so it's available for another application
			*/
			DisplayErrorString(i32Status);
			CsFreeSystem(g_hSystem);
			return(-1);
		}
	}
	/*
	If the return value is greater than  1, then either the application,
	acquisition, some of the Channel and / or some of the Trigger sections
	were missing from the ini file and the default parameters were used.
	*/
	if (CS_USING_DEFAULT_ACQ_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for acquisition. Using defaults."));

	if (CS_USING_DEFAULT_CHANNEL_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for one or more Channels. Using defaults for missing items."));

	if (CS_USING_DEFAULT_TRIGGER_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for one or more Triggers. Using defaults for missing items."));

	i32Status = CsAs_LoadConfiguration(g_hSystem, szIniFile, APPLICATION_DATA, &CsAppData);

	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status)
			_ftprintf(stdout, _T("\nUsing default application parameters."));
		else
		{
			DisplayErrorString(i32Status);
			CsFreeSystem(g_hSystem);
			return (-1);
		}
	}
	else if (CS_USING_DEFAULT_APP_DATA & i32Status)
	{
		/*
		If the return value is CS_USING_DEFAULT_APP_DATA (defined in ConfigSystem.h)
		then there was no entry in the ini file for Application and we will use
		the application default values, which were set in CsAs_LoadConfiguration.
		*/
		_ftprintf(stdout, _T("\nNo ini entry for application data. Using defaults"));
	}

	/*
	Commit the values to the driver.  This is where the values get sent to the
	hardware.  Any invalid parameters will be caught here and an error returned.
	*/
	i32Status = CsDo(g_hSystem, ACTION_COMMIT);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}
	/*
	Get acquisition config info for use afterwards.
	*/
	CsAcqCfg.u32Size = sizeof(CsAcqCfg);
	i32Status = CsGet(g_hSystem, CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCtrlHandler(ControlHandler, TRUE);
	/*
	We need to allocate a buffer
	for transferring the data
	*/
	pBuffer = VirtualAlloc(NULL, (size_t)(CsAppData.i64TransferLength * CsAcqCfg.u32SampleSize), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pBuffer)
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory\n"));
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	/*
	We also need to allocate a buffer for transferring the timestamp
	*/
	pTimeStamp = (int64 *)VirtualAlloc(NULL, (size_t)(CsAppData.u32TransferSegmentCount * sizeof(int64)), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pTimeStamp)
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory\n"));
		if (NULL != pBuffer)
			VirtualFree(pBuffer, 0, MEM_RELEASE);
		return (-1);
	}
	if (TYPE_FLOAT == CsAppData.i32SaveFormat)
	{
		/*
		Allocate another buffer to pass the data that is going to be converted
		into voltages
		*/
		pVBuffer = (float *)VirtualAlloc(NULL, (size_t)(CsAppData.i64TransferLength * sizeof(float)), MEM_COMMIT, PAGE_READWRITE);
		if (NULL == pVBuffer)
		{
			_ftprintf(stderr, _T("\nUnable to allocate memory\n"));
			CsFreeSystem(g_hSystem);
			if (NULL != pBuffer)
				VirtualFree(pBuffer, 0, MEM_RELEASE);
			if (NULL != pTimeStamp)
				VirtualFree(pTimeStamp, 0, MEM_RELEASE);
			return (-1);
		}
	}
	pDoubleData = (double *)VirtualAlloc(NULL, (size_t)(CsAppData.u32TransferSegmentCount * sizeof(double)), MEM_COMMIT, PAGE_READWRITE);

	/*
	We need to create the event in order to be notified of the completion
	*/
	hTransferEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


	// Initialize savin variables

	nMeasureSize = (int)CsAppData.i64TransferLength;
	nMeasureTotal = (int)CsAppData.u32TransferSegmentCount;


	iRawData = new int16[nMeasureSize*nMeasureTotal];


	return 0;
}

int initializeGage(float stepSize)
{
	/*
	Initializes the CompuScope boards found in the system. If the
	system is not found a message with the error code will appear.
	Otherwise i32Status will contain the number of systems found.
	*/
	i32Status = CsInitialize();

	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}
	/*
	Get the system. This sample program only supports one system. If
	2 systems or more are found, the first system that is found
	will be the system that will be used. hSystem will hold a unique
	system identifier that is used when referencing the system.
	*/
	i32Status = CsGetSystem(&hSystem, 0, 0, 0, 0);

	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}
	/*
	Get System information. The u32Size field must be filled in
	prior to calling CsGetSystemInfo
	*/
	CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CsGetSystemInfo(hSystem, &CsSysInfo);

	/*
	Display the system name from the driver
	*/
	_ftprintf(stdout, _T("Gage Board Name: %s\n"), CsSysInfo.strBoardName);

	i32Status = CsAs_ConfigureSystem(hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);

	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status)
		{
			/*
			Display message but continue on using defaults.
			*/
			_ftprintf(stdout, _T("\nCannot find %s - using default parameters."), szIniFile);
		}
		else
		{
			/*
			Otherwise the call failed.  If the call did fail we should free the CompuScope
			system so it's available for another application
			*/
			DisplayErrorString(i32Status);
			CsFreeSystem(hSystem);
			return(-1);
		}
	}
	/*
	If the return value is greater than  1, then either the application,
	acquisition, some of the Channel and / or some of the Trigger sections
	were missing from the ini file and the default parameters were used.
	*/
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
			DisplayErrorString(i32Status);
			CsFreeSystem(hSystem);
			return -1;
		}
	}
	else if (CS_USING_DEFAULT_APP_DATA & i32Status)
	{
		/*
		If the return value is CS_USING_DEFAULT_APP_DATA (defined in ConfigSystem.h)
		then there was no entry in the ini file for Application and we will use
		the application default values, which have already been set.
		*/
		_ftprintf(stdout, _T("\nNo ini entry for application data. Using defaults."));
	}

	/*
	Commit the values to the driver.  This is where the values get sent to the
	hardware.  Any invalid parameters will be caught here and an error returned.
	*/
	i32Status = CsDo(hSystem, ACTION_COMMIT);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(hSystem);
		return (-1);
	}

	/*
	Get the current sample size, resolution and offset parameters from the driver
	by calling CsGet for the ACQUISTIONCONFIG structure. These values are used
	when saving the file.
	*/
	CsAcqCfg.u32Size = sizeof(CsAcqCfg);
	i32Status = CsGet(hSystem, CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(hSystem);
		return (-1);
	}

	/*
	We need to allocate a buffer
	for transferring the data
	*/
	pBuffer = VirtualAlloc(NULL, (size_t)(CsAppData.i64TransferLength * CsAcqCfg.u32SampleSize), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pBuffer)
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory\n"));
		CsFreeSystem(hSystem);
		return (-1);
	}

	/*
	We also need to allocate a buffer for transferring the timestamp
	*/
	pTimeStamp = (int64 *)VirtualAlloc(NULL, (size_t)(CsAppData.u32TransferSegmentCount * sizeof(int64)), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pTimeStamp)
	{
		_ftprintf(stderr, _T("\nUnable to allocate memory\n"));
		if (NULL != pBuffer)
			VirtualFree(pBuffer, 0, MEM_RELEASE);
		return (-1);
	}
	if (TYPE_FLOAT == CsAppData.i32SaveFormat)
	{
		/*
		Allocate another buffer to pass the data that is going to be converted
		into voltages
		*/
		pVBuffer = (float *)VirtualAlloc(NULL, (size_t)(CsAppData.i64TransferLength * sizeof(float)), MEM_COMMIT, PAGE_READWRITE);
		if (NULL == pVBuffer)
		{
			_ftprintf(stderr, _T("\nUnable to allocate memory\n"));
			CsFreeSystem(hSystem);
			if (NULL != pBuffer)
				VirtualFree(pBuffer, 0, MEM_RELEASE);
			if (NULL != pTimeStamp)
				VirtualFree(pTimeStamp, 0, MEM_RELEASE);
			return (-1);
		}
	}
	pDoubleData = (double *)VirtualAlloc(NULL, (size_t)(CsAppData.u32TransferSegmentCount * sizeof(double)), MEM_COMMIT, PAGE_READWRITE);


	// Initialize output variables
	nMeasureSize = (int)CsAppData.i64TransferLength;
	nMeasureTotal = (int)CsAppData.u32TransferSegmentCount;

	iRawData = new int16[nMeasureSize*nMeasureTotal];

	saveStage = new int*[2];
	for (int n = 0; n < 2; n++)
	{
		saveStage[n] = new int[nMeasureTotal];
	}


	// Open Save file
	_stprintf(SaveFileName, _T("E:\\GageData\\WideFOVData.txt"));
	fid = fopen(SaveFileName, "w");
	_ftprintf(fid, _T("%d\n%d\n"), nMeasureSize, (int)stepSize); // print stage location as first line
	fclose(fid);


	return (0);
}

int collectDataAS()
{
	/*
	Start the data acquisition
	*/
	i32Status = CsDo(g_hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}
	return 0;
}

int collectData()
{
	/*
	Start the data Acquisition
	*/

	i32Status = CsDo(hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(hSystem);
		return (-1);
	}

	return 0;
}

void checkScanCompleteAS()
{
	int nLoop = -1;
	_ftprintf(stdout, "\tWaiting for capture to finish : ");

	for (;;)
	{
		dwRet = WaitForSingleObject(hEndAcqEvent, 100);

		if (WAIT_OBJECT_0 == dwRet)
			break;

		nLoop++;

		switch (nLoop % 4)
		{
		case 0:
			lstrcpy(szMsg, ("|"));
			break;

		case 1:
			lstrcpy(szMsg, ("/"));
			break;

		case 2:
			lstrcpy(szMsg, ("-"));
			break;

		case 3:
			lstrcpy(szMsg, ("\\"));
			break;
		}

		GetConsoleScreenBufferInfo(hConsole, &ScreenInfo);
		WriteConsole(hConsole, &szMsg, lstrlen(szMsg), &dwWritten, NULL);
		SetConsoleCursorPosition(hConsole, ScreenInfo.dwCursorPosition);
	}

	_ftprintf(stdout, "Done.\n");
}

int checkScanComplete()
{
	/*
	DataCaptureComplete queries the system to see when
	the capture is complete
	*/

	if (!DataCaptureComplete(hSystem))
	{
		fprintf(stdout, ("Data Capture Failed\n\n"));
		CsFreeSystem(hSystem);
		getchar();
		return (-1);
	}
	return 0;
}

int saveGageDataAS()
{
	int nMulRecGroup;
	int nTimeStampIndex;

	i32TickFrequency = TransferTimeStamp(g_hSystem, CsAppData.u32TransferSegmentStart, CsAppData.u32TransferSegmentCount, pTimeStamp);

	/*
	If TransferTimeStamp fails, i32TickFrequency will be negative,
	which represents an error code. If there is an error we'll set
	the time stamp info in pDoubleData to 0.
	*/
	ZeroMemory(pDoubleData, CsAppData.u32TransferSegmentCount * sizeof(double));
	if (CS_SUCCEEDED(i32TickFrequency))
	{
		/*
		Allocate a buffer of doubles to store the the timestamp data after we have
		converted it to microseconds.
		*/
		for (u32Count = 0; u32Count < CsAppData.u32TransferSegmentCount; u32Count++)
		{
			/*
			The number of ticks that have ocurred / tick count(the number of ticks / second)
			= the number of seconds elapsed. Multiple by 1000000 to get the number of
			mircoseconds
			*/
			pDoubleData[u32Count] = (double)(*(pTimeStamp + u32Count)) * 1.e6 / (double)(i32TickFrequency);
		}
	}

	/*
	Now transfer the actual acquired data for each desired multiple group.
	Fill in the InData structure for transferring the data
	*/
	InData.u32Mode = TxMODE_DEFAULT;
	InData.i64StartAddress = CsAppData.i64TransferStartPosition;
	InData.i64Length = CsAppData.i64TransferLength;
	InData.pDataBuffer = pBuffer;
	InData.hNotifyEvent = &hTransferEvent;

	u32ChannelIndexIncrement = CsAs_CalculateChannelIndexIncrement(&CsAcqCfg, &CsSysInfo);


	for (u32ChannelNumber = 1; u32ChannelNumber <= 1; u32ChannelNumber += u32ChannelIndexIncrement)
	{

		nRawMeasureCount = 0;

		InData.u16Channel = (uInt16)u32ChannelNumber;

		_ftprintf(stdout, "\tTransfer from channel %i: ", u32ChannelNumber);
		GetConsoleScreenBufferInfo(hConsole, &ScreenInfo);


		/*
		Variable that will contain either raw data or data in Volts depending on requested format
		*/
		void* pSrcBuffer = NULL;
		ZeroMemory(pBuffer, (size_t)(CsAppData.i64TransferLength * CsAcqCfg.u32SampleSize));
		InData.u16Channel = (uInt16)u32ChannelNumber;


		for (nMulRecGroup = CsAppData.u32TransferSegmentStart, nTimeStampIndex = 0; nMulRecGroup < (int)(CsAppData.u32TransferSegmentStart + CsAppData.u32TransferSegmentCount);
			nMulRecGroup++, nTimeStampIndex++)
		{
			InData.u32Segment = (uInt32)nMulRecGroup;

			/*
			Asynchronous Transfer Operation. Keep the token for progress report
			The token will be used further down to retrieve the status of the transfer.
			*/
			i32Status = CsTransferAS(g_hSystem, &InData, &OutData, &i32Token);
			if (CS_FAILED(i32Status))
			{
				DisplayErrorString(i32Status);
				CsFreeSystem(g_hSystem);
				CloseHandle(hTransferEvent);
				VirtualFree(pBuffer, 0, MEM_RELEASE);
				return (-1);
			}

			for (;;)
			{
				/*
				Wait 100 ms with the event created before
				Returning with WAIT_OBJECT_0 implies that the whole transfer has been completed
				Returning with WAIT_TIMEOUT implies that 100 ms have passed and the transfer is still going
				*/
				dwRet = WaitForSingleObject(hTransferEvent, 100);

				if (WAIT_OBJECT_0 == dwRet)
				{
					break;
				}
			}

			// Save to output variable
			int16 *actualData = (int16 *)pBuffer;
			int nSkipSize1 = nRawMeasureCount*nMeasureSize;

			if (u32ChannelNumber == 1)
			{
				for (int i = 0; i < nMeasureSize; i++)
				{
					iRawData[i + nSkipSize1] = actualData[i];
				}
			}
			nRawMeasureCount++;
		}

		i32Status = CsGetTransferASResult(g_hSystem, i32Token, &i64Bytes);
		_ftprintf(stdout, "Completed (%4.2f MB)\n", (float)(i64Bytes*InData.u32Segment) / 1000000.0);
	}


	// Open Save files
	_stprintf(PeakFileName, _T("E:\\GageData\\MechPeakData.txt"));
	fidPeak = fopen(PeakFileName, "w");

	// Write depth
	_ftprintf(fidPeak, _T("%d\n"), nMeasureSize);

	int nSkipSize = nMeasureTotal*nMeasureSize;

	/*
	for (int n = 0; n < nSkipSize; n++)
	{
		_ftprintf(fidPeak, _T("%d\n"), iRawData[n]); // print peakData
	}*/

	fwrite(iRawData, sizeof(int16), nSkipSize, fidPeak);


	fclose(fidPeak);
	fidPeak = NULL;


	delete iRawData;
	return 0;
}

int saveStageData(int stageLocation[], int toX, int toY)
{
	if (stageLocation[0] == toX)// not moving in X
	{
		if (toY > stageLocation[1])
		{
			for (int n = stageLocation[1]; n <= toY; n++)
			{
				saveStage[0][stageCount] = stageLocation[0];
				saveStage[1][stageCount] = n;
			}
		}
		else
		{
			for (int n = stageLocation[1]; n >= toY; n--)
			{
				saveStage[0][stageCount] = stageLocation[0];
				saveStage[1][stageCount] = n;
			}
		}
	}
	else // moving in X
	{
		if (toX > stageLocation[0])
		{
			for (int n = stageLocation[0]; n <= toX; n++)
			{
				saveStage[0][stageCount] = n;
				saveStage[1][stageCount] = stageLocation[1];
			}
		}
		else
		{
			for (int n = stageLocation[0]; n >= toX; n--)
			{
				saveStage[0][stageCount] = n;
				saveStage[1][stageCount] = stageLocation[1];
			}
		}
	}
	stageCount++;

	return 0;
}

int saveGageData()
{

	/*
	Acquisition is now complete.
	Call the function TransferTimeStamp. This function is used to transfer the timestamp
	data. The i32TickFrequency, which is returned from this fuction, is the clock rate
	of the counter used to acquire the timestamp data.
	*/
	_ftprintf(stdout, "Transfering... ", u32ChannelNumber);
	_ftprintf(stdout, "Expected to take %d s... ", (int)((float)CsAppData.u32TransferSegmentCount*(94.0/1e6)));
	clock_t startClockTime = clock(), endClockTime;
	i32TickFrequency = TransferTimeStamp(hSystem, CsAppData.u32TransferSegmentStart, CsAppData.u32TransferSegmentCount, pTimeStamp);

	/*
	If TransferTimeStamp fails, i32TickFrequency will be negative,
	which represents an error code. If there is an error we'll set
	the time stamp info in pDoubleData to 0.
	*/
	ZeroMemory(pDoubleData, CsAppData.u32TransferSegmentCount * sizeof(double));
	if (CS_SUCCEEDED(i32TickFrequency))
	{
		/*
		Allocate a buffer of doubles to store the the timestamp data after we have
		converted it to microseconds.
		*/
		for (u32Count = 0; u32Count < CsAppData.u32TransferSegmentCount; u32Count++)
		{
			/*
			The number of ticks that have ocurred / tick count(the number of ticks / second)
			= the number of seconds elapsed. Multiple by 1000000 to get the number of
			mircoseconds
			*/
			pDoubleData[u32Count] = (double)(*(pTimeStamp + u32Count)) * 1.e6 / (double)(i32TickFrequency);
		}
	}

	/*
	Now transfer the actual acquired data for each desired multiple group.
	Fill in the InData structure for transferring the data
	*/
	InData.u32Mode = TxMODE_DEFAULT;
	InData.i64StartAddress = CsAppData.i64TransferStartPosition;
	InData.i64Length = CsAppData.i64TransferLength;
	InData.pDataBuffer = pBuffer;
	//u32ChannelIndexIncrement = CsAs_CalculateChannelIndexIncrement(&CsAcqCfg, &CsSysInfo);


	// transfer channel 1
	u32ChannelNumber = 1;


	int nMulRecGroup;
	int nTimeStampIndex;
	nRawMeasureCount = 0;

	/*
	Variable that will contain either raw data or data in Volts depending on requested format
	*/
	void* pSrcBuffer = NULL;

	ZeroMemory(pBuffer, (size_t)(CsAppData.i64TransferLength * CsAcqCfg.u32SampleSize));
	InData.u16Channel = (uInt16)u32ChannelNumber;



	for (nMulRecGroup = CsAppData.u32TransferSegmentStart, nTimeStampIndex = 0; nMulRecGroup < (int)(CsAppData.u32TransferSegmentStart + CsAppData.u32TransferSegmentCount);
		nMulRecGroup++, nTimeStampIndex++)
	{
		/*
		Transfer the captured data
		*/
		InData.u32Segment = (uInt32)nMulRecGroup;
		i32Status = CsTransfer(hSystem, &InData, &OutData);

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

		int16 *actualData = (int16 *)pBuffer;
		int nSkipSize = nRawMeasureCount*nMeasureSize;

		for (int i = 0; i < nMeasureSize; i++)
		{
			iRawData[i + nSkipSize] = actualData[i];
		}
		nRawMeasureCount++;
	}

	// transfer is finished
	endClockTime = clock() - startClockTime;
	_ftprintf(stdout, "Completed (%4.2f MB)\n", (float)(244*(float)InData.u32Segment) / 1000000.0);
	_ftprintf(stdout, "Transfer Completed in %d s.\n\n", (int)(endClockTime / (double)CLOCKS_PER_SEC));
	_ftprintf(stdout, "Saving Data... ");


	// Open Save files
	int nCount = 0;
	while (1)
	{		
		nCount++;
		_stprintf(PeakFileName, _T("E:\\GageData\\MechPeakDataScan%d.txt"),nCount);
		fidPeak = fopen(PeakFileName, "r");
			if (fidPeak)
			{
				fclose(fidPeak);
			}
			else
			{
				fidPeak = fopen(PeakFileName, "w");
				break;
			}
	}		

	//_stprintf(PeakFileName, _T("E:\\GageData\\MechPeakData.txt"));
	//fidPeak = fopen(PeakFileName, "w");

	// Write depth
	_ftprintf(fidPeak, _T("%d\n"), nMeasureSize);

	int nSkipSize = nMeasureTotal*nMeasureSize;

	/*
	for (int n = 0; n < nSkipSize; n++)
	{
	_ftprintf(fidPeak, _T("%d\n"), iRawData[n]); // print peakData
	}*/

	fwrite(iRawData, sizeof(int16), nSkipSize, fidPeak);

	fclose(fidPeak);
	fidPeak = NULL;

	_ftprintf(stdout, "Complete!\n\n");

	delete iRawData;
	return 0;
}

int checkBufferSize()
{
	return (int)CsAppData.u32TransferSegmentCount;
}


BOOL WINAPI ControlHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_BREAK_EVENT:
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	{
		g_bAborted = TRUE;
		CsDo(g_hSystem, ACTION_ABORT);
		return (TRUE);
	}
	break;
	}

	return FALSE;
}

int32 TransferTimeStamp(CSHANDLE hSystem, uInt32 u32SegmentStart, uInt32 u32SegmentCount, void* pTimeStamp)
{
	IN_PARAMS_TRANSFERDATA		InTSData = { 0 };
	OUT_PARAMS_TRANSFERDATA		OutTSData = { 0 };
	int32						i32Status = CS_SUCCESS;

	InTSData.u16Channel = 1;
	InTSData.u32Mode = TxMODE_TIMESTAMP;
	InTSData.i64StartAddress = 1;
	InTSData.i64Length = (int64)u32SegmentCount;
	InTSData.u32Segment = u32SegmentStart;

	ZeroMemory(pTimeStamp, (size_t)(u32SegmentCount * sizeof(int64)));
	InTSData.pDataBuffer = pTimeStamp;

	i32Status = CsTransfer(hSystem, &InTSData, &OutTSData);
	if (CS_FAILED(i32Status))
	{
		/*
		if the error is INVALID_TRANSFER_MODE it just means that this systems
		doesn't support time stamp. We can continue on with the program.
		*/
		if (CS_INVALID_TRANSFER_MODE == i32Status)
			_ftprintf(stderr, _T("\nTime stamp is not supported in this system.\n"));
		else
			DisplayErrorString(i32Status);

		VirtualFree(pTimeStamp, 0, MEM_RELEASE);
		pTimeStamp = NULL;
		return (i32Status);
	}
	/*
	Tick count is stored in OutTSData.i32LowPart.
	*/
	return OutTSData.i32LowPart;
}