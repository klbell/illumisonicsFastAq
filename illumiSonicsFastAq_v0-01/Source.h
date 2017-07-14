

#ifndef SOURCE_H
#define SOURCE_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
//#include <iostream>
#include "nidaqmxNew.h"
#include "StageMoves.h"
#include "GageFuncs.h"
#include "DAQmxErrors.h"
#include <math.h>
#include "CsAppSupport.h"
#include "CsTchar.h"
#include "CsSdkMisc.h"
#include "MiscFuncs.h"
#include "MediaFuncs.h"


int runPARS();
int runPARSRT();
int runPAM();
int runFree();


#endif