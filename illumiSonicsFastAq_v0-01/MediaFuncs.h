#ifndef MEDIA_H
#define MEDIA_H

#include "Source.h"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <fftw3.h>

#define WINDOW_WIDTH = 600
#define WINDOW_HEIGHT = 600

// Media functions
void OpenRTWindow();
void initializeWindowVars(bool MT); // Initializes all relavent plotting and image variables
void minMaxExtract(void*  pWorkBuffer, uInt32 u32TransferSize);
void minMaxExtractMT(void* pWorkBuffer, uInt32 u32TransferSize, uInt32 totalSampleTransfered);
int updateScopeWindow();
int makeImageRealtime(void* pWorkBuffer);
void resetWindowVars(); // Resets counting variables for next pass
int checkWindowCommands();
void findMinMax();
void loadColorMap();

#endif // !MEDIA_H




