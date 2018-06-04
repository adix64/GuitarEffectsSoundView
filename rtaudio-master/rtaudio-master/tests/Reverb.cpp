#include "GuitarEffects.hpp"

#define FRAMESPERBUFFER 256
#define REVERBBUFFERSIZE 48000

float reverbBuffer[REVERBBUFFERSIZE];
int reverbInput = 0;
int reverbOutput = 0;

void applyReverb(float *out, float *in, int reverbTime, float feedback)
{
	for (int bufptr = 0; bufptr < FRAMESPERBUFFER; bufptr++)
	{
		if (reverbInput >= REVERBBUFFERSIZE) {
			reverbInput = 0;
		}
		reverbOutput = reverbInput - reverbTime;

		if (reverbOutput < 0) {
			reverbOutput = reverbOutput + REVERBBUFFERSIZE;
		}

		reverbBuffer[reverbInput] = (in)[bufptr] + (reverbBuffer[reverbOutput] * feedback);
		(out)[bufptr] = cos(reverbBuffer[reverbInput] + 0.5);
		reverbInput++;
	}
}