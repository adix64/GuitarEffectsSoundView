#include "RtAudio.h"
#include <iostream>
#include <map>
#include <algorithm>
#define FRAMESPERBUFFER 256

void applyDelay(float *out, float *in, int delayTime = 10000, float feedback = 0.5f);

void applyOverdrive(float *out, float *in, float gain);

void applyFuzz(float *out, float *in, float gain, float mix);

void applyDistorsion(float *out, float *in, float timbre, float depth);

void applyCompression(float *out, float *in);

int record(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
	double streamTime, RtAudioStreamStatus status, void *userData);