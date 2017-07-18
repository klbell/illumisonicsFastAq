#include "Source.h"

#define NUMSTAGELOCATIONS 20000 // make sure at least 10% larger than segment count

LPCTSTR saveDirectory = _T("C:\\Users\\PARS1\\Desktop\\PARS\\Data\\Oct 19_EOMRTTest\\");

bool useMTTrans = false;

// For fast mixed imaging
float XStepSize = 5 / 4; // um
float XTotSize = 100; // um

int main()
{
	int scanMode;

	// Motors off
	XON(0);
	YON(0);

	illuWelcome();
	
	// Primary program loop
	while (1)
	{
		// New Scan, get desired scan
		scanMode = getScanMode();
		
		switch (scanMode)
		{
		case 0:					// Exit program
			return 0;
		case 1: runPARS();		// PARS single capture mode
			break;
		case 2: runPARSRT();	// PARS real-time capture mode
			break;
		case 3: runFastMix();
			break;
		/*case 4: runFree();
			break;*/
		default: 
			_ftprintf(stdout, "Invalid selection\n\n");
		}
	}
	
	return 0;
}

int runPARS()
{
	PARSWelcome(saveDirectory);
	initializeGageSingleCap(); // Setup Gage card
	collectData(); // Collect data
	checkScanComplete(); // Check to see when capture is completed
	saveGageData(saveDirectory); // Transfer and save all that data
	releaseGageSingleCap(); // Release Gage card

	return 0;
}

int runPARSRT()
{
	PARSRTWelcome();

	if (useMTTrans) // Multi threaded transfer
	{
		initializeGageStreamMT();
		OpenRTWindow();
		initializeWindowVars(true);

		int frameCount = 0;

		while (!gageStreamRealtimeMT())
		{
			if (updateScopeWindow())
			{
				break;
			}	
			_ftprintf(stdout, _T("\nFrame count: %d\n"), ++frameCount);
		}
	}
	else {		
		bool bFastMix = 0;

		initializeGageStream(bFastMix); // Setup Gage card
		OpenRTWindow(); // Launch view window
		initializeWindowVars(false); // Set ploting variables

		while (!gageStreamRealtime())
		{
			if (updateScopeWindow())
			{
				break;
			}
		}

		releaseGageRT(); // Release Gage card
	}

	return 0;
}

int runFastMix()
{
	// Setup
	int			stepX = 1;

	int			stageLocation = 0;
	int16		*stageLocationSave; // Ensure that this is made large enough for some spillover
	int			stageCount = 0;

	stageLocationSave = new int16[NUMSTAGELOCATIONS];
	int stepTotalX = XTotSize / XStepSize;

	// Motors off
	XON(0);
	YON(0);

	////////////////////////////////
	// Initialize DAQ settings 
	///////////////////////////////

	TaskHandle	directionX = 0;
	TaskHandle	directionY = 0;

	DAQmxCreateTask("XDir", &directionX);
	DAQmxCreateTask("YDir", &directionY);

	DAQmxCreateAOVoltageChan(directionX, "Dev1/ao1", "XDir", 0.0, 5.0, DAQmx_Val_Volts, NULL); // X Direction
	DAQmxCreateAOVoltageChan(directionY, "Dev1/ao0", "YDir", 0.0, 5.0, DAQmx_Val_Volts, NULL); // Y Direction

	float64 fiveVoltOut[2] = { 5.0, 5.0 }; // 5.0 V
	float64 zeroVoltOut[2] = { 0.0, 0.0 }; // 0.0 V

	//Must be 2 bits
	uInt8 outClock[2] = { 0, 1 };
	uInt8 onSig[2] = { 1, 1 };
	uInt8 offSig[2] = { 0, 0 };


	PARSFastMixWelcome();

	bool bFastMix = 1;

	initializeGageStream(bFastMix);
	OpenRTWindow(); // Launch view window
	initializeWindowVars(false); // Set ploting variables

	

	////////////////////////////////
	// Prep for scan
	///////////////////////////////

	// Motors on
	XON(1);
	YON(1);

	// Scan variables
	int steps;
	//int gageCollectStatus;
	int XDir = 1;
	int saveCount = 1;

	// Move to start position
	steps = stepTotalX / 2;
	DAQmxWriteAnalogF64(directionX, 1, 1, 0.0, DAQmx_Val_GroupByChannel, zeroVoltOut, NULL, NULL); // dir
	moveXStage(steps, outClock);
	stageLocation -= steps;


	////////////////////
	// Scan
	////////////////////

	while (1)
	{
		// Scan along X
		if (XDir == 1)
		{
			DAQmxWriteAnalogF64(directionX, 1, 1, 0.0, DAQmx_Val_GroupByChannel, fiveVoltOut, NULL, NULL); // dir 				
			stageLocation += stepX;
		}
		else {
			DAQmxWriteAnalogF64(directionX, 1, 1, 0.0, DAQmx_Val_GroupByChannel, zeroVoltOut, NULL, NULL); // dir 				
			stageLocation -= stepX;
		}
		moveXStage(stepX, outClock); // Move


		gageStreamRealtimeFastMix();
		updateScopeWindowFastMix(stepTotalX, stageLocation);


		// Reverse if at end of stage
		if (stageLocation > stepTotalX / 2 | stageLocation < -stepTotalX / 2)
		{
			if (XDir == 1)
			{
				XDir = 0;
			}
			else {
				XDir = 1;
			}
		}
	}



	/*
	while (!gageStreamRealtime())
	{
		if (updateScopeWindow())
		{
			break;
		}
	}
	*/

	releaseGageRT(); // Release Gage card

	return 0;
}

int runFree()
{
	return 0;
}