#ifndef DAQMXERRORS_H
#define DAQMXERRORS_H

#include <nidaqmx.h>
#include <stdio.h>
//#include <math.h>
//#include <iostream>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

#endif