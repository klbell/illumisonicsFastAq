#include "StageMoves.h"

int motorSpeed = 1000000; // in Hz

void moveXStage(int steps, uInt8 clockSig[])
{
	int bits = steps * 2;

	TaskHandle	XClockOut = 0;
	uInt8 *signal = new uInt8[bits];
	for (int i = 0; i < bits; i += 2) {
		signal[i] = clockSig[0];
		signal[i + 1] = clockSig[1];
	}

	DAQmxCreateTask("X", &XClockOut);
	DAQmxCreateDOChan(XClockOut, "Dev1/port0/line0", "X", DAQmx_Val_ChanForAllLines); // Xmotor
	DAQmxCfgSampClkTiming(XClockOut, NULL, motorSpeed, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, bits); // Ext. Trig
	DAQmxWriteDigitalLines(XClockOut, bits, 0, 1000.0, DAQmx_Val_GroupByChannel, signal, NULL, NULL); // X Motor signal	
	DAQmxStartTask(XClockOut); // Pulse motor
	DAQmxWaitUntilTaskDone(XClockOut, 1000.0); // Wait for action to be completed
	DAQmxStopTask(XClockOut);
	DAQmxClearTask(XClockOut);

	delete signal;
}


void moveYStage(int steps, uInt8 clockSig[])
{
	int bits = steps * 2;

	TaskHandle	YClockOut = 0;
	uInt8 *signal = new uInt8[bits];
	for (int i = 0; i < bits; i += 2) {
		signal[i] = clockSig[0];
		signal[i + 1] = clockSig[1];
	}

	DAQmxCreateTask("Y", &YClockOut);
	DAQmxCreateDOChan(YClockOut, "Dev1/port0/line1", "Y", DAQmx_Val_ChanForAllLines); // Ymotor
	DAQmxCfgSampClkTiming(YClockOut, NULL, motorSpeed, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, bits); // Int. Trig
	DAQmxWriteDigitalLines(YClockOut, bits, 0, 1000.0, DAQmx_Val_GroupByChannel, signal, NULL, NULL); // Y Motor signal 
	DAQmxStartTask(YClockOut); // Pulse motor
	DAQmxWaitUntilTaskDone(YClockOut, 1000.0); // Wait for action to be completed
	DAQmxStopTask(YClockOut);
	DAQmxClearTask(YClockOut);

	delete signal;
}


void XDIR(int in)
{
	TaskHandle XDir = 0;	
	uInt8 dirSig[2] = { in, in };

	DAQmxCreateTask("XDir", &XDir);
	DAQmxCreateDOChan(XDir, "Dev1/port0/line2", "XDir", DAQmx_Val_ChanForAllLines); // Xdirection
	DAQmxCfgSampClkTiming(XDir, NULL, 3000.0, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, 2); // Int. Trig
	DAQmxWriteDigitalLines(XDir, 2, 0, 1000.0, DAQmx_Val_GroupByChannel, dirSig, NULL, NULL); // turn Off 
	DAQmxStartTask(XDir);
	DAQmxWaitUntilTaskDone(XDir, 1.0); // Wait for action to be completed
	DAQmxStopTask(XDir);
	DAQmxClearTask(XDir);
}

void YDIR(int in)
{
	TaskHandle YDir = 0;
	uInt8 dirSig[2] = { in, in };

	DAQmxCreateTask("YDir", &YDir);
	DAQmxCreateDOChan(YDir, "Dev1/port0/line3", "YDir", DAQmx_Val_ChanForAllLines); // Ydirection
	DAQmxCfgSampClkTiming(YDir, NULL, 3000.0, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, 2); // Int. Trig
	DAQmxWriteDigitalLines(YDir, 2, 0, 1000.0, DAQmx_Val_GroupByChannel, dirSig, NULL, NULL); // turn Off 
	DAQmxStartTask(YDir);
	DAQmxWaitUntilTaskDone(YDir, 1.0); // Wait for action to be completed
	DAQmxStopTask(YDir);
	DAQmxClearTask(YDir);
}

void XON(int in)
{
	TaskHandle XOn = 0;	
	uInt8 onSig[2] = { in, in };

	DAQmxCreateTask("XON", &XOn);
	DAQmxCreateDOChan(XOn, "Dev1/port0/line4", "XON", DAQmx_Val_ChanForAllLines); // Xon
	DAQmxCfgSampClkTiming(XOn, NULL, 3000.0,  DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, 2); // Int. Trig
	DAQmxWriteDigitalLines(XOn, 2, 0, 1000.0, DAQmx_Val_GroupByChannel, onSig, NULL, NULL); // turn Off 
	DAQmxStartTask(XOn);
	DAQmxWaitUntilTaskDone(XOn, 1.0); // Wait for action to be completed
	DAQmxStopTask(XOn);
	DAQmxClearTask(XOn);
}

void YON(int in)
{
	TaskHandle YOn = 0;	
	uInt8 onSig[2] = { in, in };

	DAQmxCreateTask("YON", &YOn);
	DAQmxCreateDOChan(YOn, "Dev1/port0/line5", "YON", DAQmx_Val_ChanForAllLines); // Yon	
	DAQmxCfgSampClkTiming(YOn, NULL, 3000.0, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, 2); // Ext. Trig
	DAQmxWriteDigitalLines(YOn, 2, 0, 1000.0, DAQmx_Val_GroupByChannel, onSig, NULL, NULL); // turn Off 
	DAQmxStartTask(YOn);
	DAQmxWaitUntilTaskDone(YOn, 1.0); // Wait for action to be completed
	DAQmxStopTask(YOn);
	DAQmxClearTask(YOn);
}


int microStep(int in)
{
	uInt8 MS1Sig[2];
	uInt8 MS2Sig[2];

	// Pull to zero for active in current controller mode
	switch (in){
	case 1 : // no mirostep
		MS1Sig[0] = 0;	MS1Sig[1] = 0;
		MS2Sig[0] = 0;	MS2Sig[1] = 0;
		break;
	case 2 : // half step
		MS1Sig[0] = 1;	MS1Sig[1] = 1;
		MS2Sig[0] = 0;	MS2Sig[1] = 0;
		break;
	case 4: // quarter step
		MS1Sig[0] = 0;	MS1Sig[1] = 0;
		MS2Sig[0] = 1;	MS2Sig[1] = 1;
		break;
	case 8: // eighth step
		MS1Sig[0] = 1;	MS1Sig[1] = 1;
		MS2Sig[0] = 1;	MS2Sig[1] = 1;
		break;
	default :
		return -1;
	}

	TaskHandle microStep = 0;

	DAQmxCreateTask("MS1", &microStep);
	DAQmxCreateDOChan(microStep, "Dev1/port0/line6", "MS1", DAQmx_Val_ChanForAllLines); // MS1
	DAQmxCfgSampClkTiming(microStep, NULL, 3000.0, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, 2); // Ext. Trig
	DAQmxWriteDigitalLines(microStep, 2, 0, 1000.0, DAQmx_Val_GroupByChannel, MS1Sig, NULL, NULL); // MS1 out 
	DAQmxStartTask(microStep);
	DAQmxWaitUntilTaskDone(microStep, 1.0); // Wait for action to be completed
	DAQmxStopTask(microStep);
	DAQmxClearTask(microStep);

	microStep = 0;

	DAQmxCreateTask("MS2", &microStep);
	DAQmxCreateDOChan(microStep, "Dev1/port0/line7", "MS2", DAQmx_Val_ChanForAllLines); // MS1
	DAQmxCfgSampClkTiming(microStep, NULL, 3000.0, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, 2); // Ext. Trig
	DAQmxWriteDigitalLines(microStep, 2, 0, 1000.0, DAQmx_Val_GroupByChannel, MS2Sig, NULL, NULL); // MS1 out 
	DAQmxStartTask(microStep);
	DAQmxWaitUntilTaskDone(microStep, 1.0); // Wait for action to be completed
	DAQmxStopTask(microStep);
	DAQmxClearTask(microStep);
	
	return 0;
}

int getStageClockSpeed()
{
	return motorSpeed;
}