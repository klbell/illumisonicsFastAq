#include "Source.h"

LPCTSTR saveDirectory = _T("C:\\Users\\PARS1\\Desktop\\PARS\\Data\\Oct 19_EOMRTTest\\");

bool useMTTrans = false;

// For fast mixed imaging
int XStepSize = 5 / 4; // um
int XTotSize = 100; // um

int main()
{
	int scanMode;


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
	PARSFastMixWelcome();

	bool bFastMix = 1;

	initializeGageStream(bFastMix);

	return 0;
}

int runFree()
{
	return 0;
}