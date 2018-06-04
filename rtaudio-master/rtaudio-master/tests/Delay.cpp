#include "GuitarEffects.hpp"

#define FRAMESPERBUFFER 256
#define DELAYBUFFERSIZE 44100

float delayBuffer[DELAYBUFFERSIZE];
int delayInput = 0;
int delayOutput = 0;

void applyDelay(float *out, float *in, int delayTime, float feedback)
{
	for (int bufptr = 0; bufptr<FRAMESPERBUFFER; bufptr++)
	{

		if (delayInput >= DELAYBUFFERSIZE){
			delayInput = 0;
		}

		delayOutput = delayInput - delayTime;

		if (delayOutput < 0){
			delayOutput = delayOutput + DELAYBUFFERSIZE;
		}

		delayBuffer[delayInput] = (in)[bufptr] + (delayBuffer[delayOutput] * feedback);
		(out)[bufptr] = cos(delayBuffer[delayInput] + 0.5) - 0.7f;

		delayInput++;
	}//for
}