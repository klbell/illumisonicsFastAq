#ifndef STAGEMOVES_H
#define STAGEMOVES_H

#include <NIDAQmx.h>



void moveXStage(int steps, uInt8 clockSig[]);
void moveYStage(int steps, uInt8 clockSig[]);
void XDIR(int in);
void YDIR(int in);
void XON(int in);
void YON(int in);
int microStep(int in);
int getStageClockSpeed();

#endif