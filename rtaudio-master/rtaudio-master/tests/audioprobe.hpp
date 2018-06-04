#pragma once
#define  _CRT_SECURE_NO_WARNINGS
#include "WAVhandling.hpp"
class RtAudio;

struct PedalboardSettings
{
	bool recording = false;
	bool playbacking = false;

	bool compressor = false;
	
	bool delay = false;
	int delayTime = 10000.f; float delayFeedback = 0.5f;

	bool reverb = false;
	
	bool distortion = false;
	float distortionTimbre = 1.5f, distortionDepth = 1.5f;

	bool overdrive = false;
	float overdriveGain = .95f;

	bool fuzz = false;
	float fuzzGain = 1.5f, fuzzMix = 1.f;
	int wavReadHead = 0;
	Wave gPlaybackWave;
	float *floatWaveData = NULL;
	std::vector <float> inputFreqKnobs;
	std::vector <float> outputFreqKnobs;
};

class STMaudio
{
public:
	STMaudio();
	~STMaudio();
	void Run();
	float *GetCircularInputBuffer();
	float *GetCircularOutputBuffer();
	int GetWriteHead();

	float *GetInputFreqMagnitudes();
	float *GetInputFreqReal();
	float *GetInputFreqImag();

	float *GetOutputFreqMagnitudes();
	float *GetOutputFreqReal();
	float *GetOutputFreqImag();

	PedalboardSettings& GetSettings() { return settings; }

	void Record();
	void StopRecording();
	void PlayWAV(const char *filename);
	void StopWAV();
	bool& IsRecording();
	bool& IsPlayingWAV();
private:
	RtAudio *adc;
	PedalboardSettings settings;
};

