#include "MediaFuncs.h"

// Settings

float satLevel = 0.5; // percetn of max where signal saturation is forced


// Other Variables
sf::RenderWindow *window;
int captureCount = 0;
int16 *peakData;
int16 **mirrData;
uInt8 colormap[256][3];

int FastMixScale = 2;
int imageWidth = 500 * FastMixScale;
int imageHeight = 200 * FastMixScale;

int interpLevel = 1; // size (in pixels) of output pixels;
int interpHeight, interpWidth;
int **testgrid;
int **rendergrid;
int **testgridCount;
int searchSizeX = 0; // how far does fancy interpolation look?
int searchSizeY = 0;

int maxX = 0, maxY = 0, maxSig = 0, minX = 0, minY = 0, minSig = 0;

int xLoc, yLoc;
float halfX, halfY, rangeX, rangeY, rangeSig;
int intensity;




void OpenRTWindow()
{
	window = new sf::RenderWindow(sf::VideoMode(imageWidth, imageHeight), "IS Scope RT");
}

void OpenRTWindowFastMix(int stepTotalX)
{
	imageWidth = stepTotalX*FastMixScale;
	window = new sf::RenderWindow(sf::VideoMode(imageWidth, imageHeight), "IS Scope RT");
}

void initializeWindowVars(bool MT)
{
	// Get segment count
	uInt32 segmentCount;
	if (MT)
	{
		segmentCount = getSegmentCountMT();
	}
	else
	{
		segmentCount = getSegmentCount();
	}

	// Setup plotting vairables
	peakData = new int16 [segmentCount];
	mirrData = new int16*[2];
	for (int n = 0; n < 2; n++)
	{
		mirrData[n] = new int16[segmentCount];
	}

	// Load colormap
	loadColorMap();

	// Setup test grid for interpolation
	interpHeight = imageHeight / interpLevel;
	interpWidth = imageWidth / interpLevel;

	testgrid = new int*[interpHeight];
	rendergrid = new int*[interpHeight];
	testgridCount = new int*[interpHeight];

	for (int i = 0; i < interpHeight; i++)
	{
		testgrid[i] = new int[interpWidth];
		rendergrid[i] = new int[interpWidth];
		testgridCount[i] = new int[interpWidth];

		for (int j = 0; j < interpWidth; j++)
		{
			testgrid[i][j] = 0;
			rendergrid[i][j] = 0;
			testgridCount[i][j] = 0;
		}
	}
}

void initializeWindowVarsFastMix(int stepTotalX)
{
	// Get segment count
	uInt32 segmentCount = getSegmentCount();

	// Setup plotting vairables
	peakData = new int16[segmentCount];
	mirrData = new int16*[1];
	for (int n = 0; n < 2; n++)
	{
		mirrData[n] = new int16[segmentCount];
	}

	// Load colormap
	loadColorMap();

	//imageWidth = stepTotalX*FastMixScale;
	//imageHeight = 50 * FastMixScale;
	

	rendergrid = new int*[imageHeight];
	
	for (int i = 0; i < imageHeight; i++)
	{
		rendergrid[i] = new int[imageWidth];

		for (int j = 0; j < imageWidth; j++)
		{
			rendergrid[i][j] = 0;
		}
	}
}

void minMaxExtract(void*  pWorkBuffer, uInt32 u32TransferSize)
{
	// Convert in data
	int16* actualData = (int16*)pWorkBuffer;
	int totalLength = u32TransferSize * 4;

	int16 * peakRawData;
	peakRawData = new int16[u32TransferSize];


	// Extract signal data
	for (int16 n = 0; n < u32TransferSize; n ++)
	{
		peakRawData[n] = actualData[4 * n];
	}

	
	int16 tempMax = 0;
	for (int16 n = 0; n < u32TransferSize; n++)
	{
		if (peakRawData[n] > tempMax)
		{
			tempMax = peakRawData[n];
		}
	}

	peakData[captureCount] = tempMax;

	// Extract Mirror data
	mirrData[0][captureCount] = actualData[1]; 
	mirrData[1][captureCount] = actualData[2];

	captureCount++;
}

void fastMixExtract(void*  pWorkBuffer, uInt32 u32TransferSize)
{
	// Convert in data
	int16* actualData = (int16*)pWorkBuffer;
	int totalLength = u32TransferSize * 4;

	int16 * peakRawData;
	peakRawData = new int16[u32TransferSize];


	// Extract signal data
	for (int16 n = 0; n < u32TransferSize; n++)
	{
		peakRawData[n] = actualData[4 * n];
	}


	int16 tempMax = 0;
	for (int16 n = 0; n < u32TransferSize; n++)
	{
		if (peakRawData[n] > tempMax)
		{
			tempMax = peakRawData[n];
		}
	}

	peakData[captureCount] = tempMax;

	// Extract Mirror data
	mirrData[0][captureCount] = actualData[1];

	captureCount++;
}

void minMaxExtractMT(void*  pWorkBuffer, uInt32 u32TransferSize, uInt32 totalSamplesTrans)
{
	// Convert in data
	int16* rawData = (int16*)pWorkBuffer;	
	int totalLength = u32TransferSize * 4;
	int offset = 28; // offset which seems tobe added infront of each measurement

	int totalMeasCount = totalSamplesTrans / (totalLength + offset) - 1;
	int16* actualData = new int16[totalMeasCount*totalLength];

	// Restructure raw data into actual data
	int n = 0;
	for (int i = 0; i < totalMeasCount; i++)
	{
		for (int j = offset; j < offset + totalLength; j++)
		{
			actualData[n++] = rawData[i*(totalLength + offset) + j];
		}
	}

		
	int16 * peakRawData;
	peakRawData = new int16[u32TransferSize];

	int measNum;
	for (measNum = 0; measNum < totalMeasCount; measNum++)
	{
		// Extract signal data
		for (int n = 0; n < u32TransferSize; n++)
		{
			peakRawData[n] = actualData[(4 * n) + (measNum * totalLength)];
		}

		//hilbert(peakRawData,u32TransferSize);
		int16 tempMax = 0;
		for (int16 n = 0; n < u32TransferSize; n++)
		{
			if (peakRawData[n] > tempMax)
			{
				tempMax = peakRawData[n];
			}
		}

		peakData[captureCount + measNum] = tempMax;

		// Extract Mirror data
		mirrData[0][captureCount + measNum] = actualData[1 + (measNum * totalLength)];
		mirrData[1][captureCount + measNum] = actualData[2 + (measNum * totalLength)];
	}	

	captureCount += measNum;
}

int updateScopeWindow()
{
	sf::Image scopeImage;
	sf::Texture scopeTexture;
	sf::Sprite background;	
	sf::Color color;

	int xLoc, yLoc;
	int halfX, halfY, rangeX, rangeY;
	float rangeSig;
	int intensity;

	scopeImage.create(imageWidth, imageHeight, sf::Color::Black);
	
	// Find min max values
	findMinMax();	

	// Find range and half values
	rangeX = maxX - minX;
	rangeY = maxY - minY;
	rangeSig = maxSig - minSig;
	halfX = rangeX / 2;
	halfY = rangeY / 2;
	
	// plot points on test grid
	for (int n = 0; n < captureCount; n++)
	{
		// Determine draw location
		xLoc = ((float)(mirrData[0][n] - minX)*0.98 / rangeX) * interpWidth;
		yLoc = ((float)(mirrData[1][n] - minY)*0.98 / rangeY) * interpHeight;

		// Determine pixel intensity
		intensity = (((float)(peakData[n] - minSig)*1.5 / rangeSig)) * 255;
		intensity = min(240, intensity);

		testgrid[yLoc][xLoc] += intensity;
		testgridCount[yLoc][xLoc]++;
	}

	// Average grid pixels which have multiple occurrances
	for (int i = 0; i < interpHeight; i++)
	{
		for (int j = 0; j < interpWidth; j++)
		{
			if (testgridCount[i][j] > 1)
			{
				testgrid[i][j] = testgrid[i][j] / testgridCount[i][j];
			}
		}
	}

	bool oldAverage = false;
	int sUp, sDown, sRight, sLeft; // for fancy averaging
	float wUp, wDown, wLeft, wRight, wDen;
	float wMult = 2;

	if (oldAverage)
	{
		// Do basic spactial averaging and draw
		for (int i = 1; i < interpHeight - 1; i++)
		{
			for (int j = 1; j < interpWidth - 1; j++)
			{
				// spactial averaging
				if (testgridCount[i][j] == 0)
				{
					testgrid[i][j] = (testgrid[i - 1][j] + testgrid[i + 1][j] + testgrid[i][j - 1] + testgrid[i][j + 1]) / 4;
					for (int avg = 0; avg < 2; avg++)
					{
						testgrid[i][j] = (2 * testgrid[i][j] + testgrid[i - 1][j] + testgrid[i + 1][j] +
							testgrid[i][j - 1] + testgrid[i][j + 1]) / 6;
					}
				}


				intensity = testgrid[i][j];

				// Draw
				color.r = colormap[intensity][0];
				color.g = colormap[intensity][1];
				color.b = colormap[intensity][2];

				for (int a = i*interpLevel; a <= (i + 1)*interpLevel; a++)
				{
					for (int b = j*interpLevel; b <= (j + 1)*interpLevel; b++)
					{
						scopeImage.setPixel(a, b, color);
					}
				}
			}
		}
	}
	else {
		// Do fancy spactial averaging and draw
		for (int i = 1; i < interpHeight - 1; i++)
		{
			for (int j = 1; j < interpWidth - 1; j++)
			{
				// spactial averaging
				if (testgridCount[i][j] == 0)
				{
					// find closest above
					for (int search = 1; i + search < interpHeight; search++)
					{
						if (testgridCount[i + search][j] != 0 || search == searchSizeX || i + search == interpHeight - 1)
						{
							sUp = (float)search;
							break;
						}
					}

					// find closest below
					for (int search = 1; i - search >= 0 ; search++)
					{
						if (testgridCount[i - search][j] != 0 || search == searchSizeX || i - search == 0)
						{
							sDown = (float)search;
							break;
						}
					}

					// find closest right
					for (int search = 1; j + search < interpWidth; search++)
					{
						if (testgridCount[i][j + search] != 0 || search == searchSizeY || j + search == interpWidth - 1)
						{
							sRight = (float)search;
							break;
						}
					}

					// find closest left
					for (int search = 1; j - search >= 0; search++)
					{
						if (testgridCount[i][j - search] != 0 || search == searchSizeY || j - search == 0)
						{
							sLeft = (float)search;
							break;
						}
					}

					wUp = 1.0f / ((float)sUp * wMult);
					wDown = 1.0f / ((float)sDown  * wMult);
					wRight = 1.0f / ((float)sRight * wMult);
					wLeft = 1.0f / ((float)sLeft * wMult);

					wDen = wUp + wDown + wRight + wLeft;

					rendergrid[i][j] = (int)((wDown * (float)testgrid[i - sDown][j] + wUp * (float)testgrid[i + sUp][j] + 
						wLeft * (float)testgrid[i][j - sLeft] + wRight * (float)testgrid[i][j + sRight]) / wDen);					
				}
				else {
					rendergrid[i][j] = testgrid[i][j];
				}

				intensity = rendergrid[i][j];

				// Draw
				color.r = colormap[intensity][0];
				color.g = colormap[intensity][1];
				color.b = colormap[intensity][2];

				for (int a = i*interpLevel; a <= (i + 1)*interpLevel; a++)
				{
					for (int b = j*interpLevel; b <= (j + 1)*interpLevel; b++)
					{
						scopeImage.setPixel(a, b, color);
					}
				}
			}
		}
	}
	
	/*
	// plot points of image
	for (int n = 0; n < captureCount; n++)
	{
		// Determine draw location
		xLoc = ((float)(mirrData[0][n] - minX)*0.98/ rangeX) * imageWidth;
		yLoc = ((float)(mirrData[1][n] - minY)*0.98/ rangeY) * imageHeight;

		// Determine pixel intensity
		intensity = (((float)(peakData[n] - minSig) / rangeSig)) * 255;
		intensity = min(220, intensity);

		// Apply color map if one is loeaded, otherwise use grayscale
		if (colormap)
		{
 			color.r = colormap[intensity][0];
			color.g = colormap[intensity][1];
			color.b = colormap[intensity][2];
		}
		else {
			color.r = intensity;
			color.g = intensity;
			color.b = intensity;
		}

		// plot
		for (int a = 0; a <= 3; a++)
		{
			for (int b = 0; b <= 3; b++)
			{
				scopeImage.setPixel(xLoc+a, yLoc+b, color);
			}
		}
	}
	*/
	
	sf::IntRect r1(0, 0, imageWidth, imageHeight);
	scopeTexture.loadFromImage(scopeImage, r1);

	// Draw stuff
	window->clear();
	background.setTexture(scopeTexture);
	window->draw(background);
	window->display();

	resetWindowVars();
	
	// Check if clossing
	if (checkWindowCommands())
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int updateScopeWindowFastMix(int stepTotalX, int stageLoc)
{
	// Get handles
	sf::Image scopeImage;
	sf::Texture scopeTexture;
	sf::Sprite background;
	sf::Color color;
			
	scopeImage.create(imageWidth*FastMixScale, imageHeight * FastMixScale, sf::Color::Black);

	// Find min max values
	findMinMax();

	// Find range and half values
	rangeX = maxX - minX;
	rangeY = stepTotalX;
	rangeSig = maxSig - minSig;
	halfX = rangeX / 2;
	halfY = rangeY / 2;

	
	bool oldAverage = false;
	int sUp, sDown, sRight, sLeft; // for fancy averaging
	float wUp, wDown, wLeft, wRight, wDen;
	float wMult = 2;

	
	
	
	// plot points of image
	for (int n = 0; n < captureCount; n++)
	{
		// Determine draw location
		yLoc = ((float)(mirrData[0][n] - minX)*0.98 / rangeX) * (imageHeight / (FastMixScale));
		xLoc = stageLoc + halfY + 1;

		if (xLoc >= imageWidth){ xLoc = imageWidth - 1; }
		if (yLoc >= imageHeight){ yLoc = imageHeight - 1; }

		//_ftprintf(stdout, _T("%d, %d\n"),xLoc, yLoc);

		// Determine pixel intensity
		intensity = (((float)(peakData[n] - minSig) / rangeSig)) * 255;
		intensity = min(220, intensity);

		rendergrid[yLoc][xLoc] = intensity;		
	}

	for (int x = 0; x < imageWidth; x++)
	{
		for (int y = 0; y < imageHeight; y++)
		{

			intensity = rendergrid[y][x];

			// Apply color map if one is loeaded, otherwise use grayscale
			if (colormap)
			{
				color.r = colormap[intensity][0];
				color.g = colormap[intensity][1];
				color.b = colormap[intensity][2];
			}
			else {
				color.r = intensity;
				color.g = intensity;
				color.b = intensity;
			}


			/*// plot
			for (int a = 0; a <= 1; a++)
			{
				for (int b = 0; b <= 1; b++)
				{
					scopeImage.setPixel(x*1 + a, y * 1 + b, color);
				}
			}*/

			// plot
			for (int a = 0; a <= FastMixScale-1; a++)
			{
				for (int b = 0; b <= FastMixScale-1; b++)
				{
					scopeImage.setPixel(x*(FastMixScale) + a, y * (FastMixScale) + b, color);
				}
			}
		}
	}
	
	

	sf::IntRect r1(0, 0, imageWidth, imageHeight);
	scopeTexture.loadFromImage(scopeImage, r1);

	// Draw stuff
	window->clear();
	background.setTexture(scopeTexture);
	window->draw(background);
	window->display();

	resetWindowVars();

	// Check if clossing
	if (checkWindowCommands())
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int makeImageRealtime(void* pWorkBuffer)
{
	int16* actualData = (int16 *)pWorkBuffer;
	
	
	/*
	sf::CircleShape shape(100.f);
	shape.setFillColor(sf::Color::Green);

	
	sf::Event event;
	while (window->pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
			window->close();
	}
	
	window->clear();
	window->draw(shape);
	window->display();
	*/


	return 0;
}

void resetWindowVars()
{
	// Reset counter
	captureCount = 0; 

	// Zero test grid
	for (int i = 0; i < interpHeight; i++)
	{
		for (int j = 0; j < interpWidth; j++)
		{
			testgrid[i][j] = 0;
			testgridCount[i][j] = 0;
		}
	}	
}

int checkWindowCommands()
{
	sf::Event event;
	while (window->pollEvent(event))
	{
		// "close requested" event: we close the window
		if (event.type == sf::Event::Closed)
		{
			window->close();
			return 1;
		}			
	}
	return 0;
}

void findMinMax()
{
	// Find max values
	for (int n = 0; n < captureCount; n++)
	{
		if (maxX < mirrData[0][n])
		{
			maxX = mirrData[0][n];
		}

		if (maxY < mirrData[1][n])
		{
			maxY = mirrData[1][n];
		}

		if (maxSig < peakData[n])
		{
			maxSig = peakData[n];
		}

		if (minX > mirrData[0][n])
		{
			minX = mirrData[0][n];
		}

		if (minY > mirrData[1][n])
		{
			minY = mirrData[1][n];
		}

		if (minSig > peakData[n])
		{
			minSig = peakData[n];
		}
	}
}


void loadColorMap()
{
	uInt8 colormapRead[257][3];
	FILE * colorFile;
	int inColorCount;
	int inInd;

	// open file
	colorFile = fopen("ColormapHot.txt", "r");	
	if (!colorFile)
	{
		_ftprintf(stdout,"Failled to load colormap from file");
		getchar();
	}

	// read in values
	int n = 0;

	for (int n = 0; n < 500; n++)
	{
		int checkScan = fscanf(colorFile, "%d %d %d", &colormapRead[n][0], &colormapRead[n][1], &colormapRead[n][2]);	
		if (checkScan == -1) // End of file
		{
			inColorCount = n-1;
			break;
		}
	}

	// Close file
	fclose(colorFile);	
	colorFile = NULL;

	
	float readMul = 256.0/(float)inColorCount;

	for (int m = 0; m < 256; m++)
	{
		inInd = (int)(m/readMul);
		colormap[m][0] = colormapRead[inInd][0];
		colormap[m][1] = colormapRead[inInd][1];
		colormap[m][2] = colormapRead[inInd][2];
	}
	
}
