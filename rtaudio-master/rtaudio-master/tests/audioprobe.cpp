#include "audioprobe.hpp"
#include "RtAudio.h"
#include <iostream>
#include <map>
#include <algorithm>
#include "GuitarEffects.hpp"

#include "fftw-3.3.5-dll32\fftw3.h"
#include <direct.h>
#define GetCurrentDir _getcwd


float viewedInputSamples[32768];
float viewedOutputSamples[32768];

float inputFreqMagnitudes[FRAMESPERBUFFER];
float inputFreqReal[FRAMESPERBUFFER];
float inputFreqImag[FRAMESPERBUFFER];

float outputFreqMagnitudes[FRAMESPERBUFFER];
float outputFreqReal[FRAMESPERBUFFER];
float outputFreqImag[FRAMESPERBUFFER];

fftwf_plan p1, p2, p3;
fftwf_complex inputFreqTransform[FRAMESPERBUFFER];
fftwf_complex outputFreqTransform[FRAMESPERBUFFER];

int writeHead = 0;

float* STMaudio::GetCircularInputBuffer() { return viewedInputSamples; }
float* STMaudio::GetCircularOutputBuffer() { return viewedOutputSamples; }
int STMaudio::GetWriteHead() { return writeHead; }

float *STMaudio::GetInputFreqMagnitudes() { return inputFreqMagnitudes; }
float *STMaudio::GetInputFreqReal() { return inputFreqReal; }
float *STMaudio::GetInputFreqImag() { return inputFreqImag; }

float *STMaudio::GetOutputFreqMagnitudes() { return outputFreqMagnitudes; }
float *STMaudio::GetOutputFreqReal() { return outputFreqReal; }
float *STMaudio::GetOutputFreqImag() { return outputFreqImag; }

//
//fftw_plan fftw_plan_dft_r2c_1d(int n, double *in, fftw_complex *out,
//	unsigned flags);
//fftw_plan fftw_plan_dft_c2r_1d(int n, fftw_complex *in, double *out,
//	unsigned flags);

float changeInterval(float x, float a, float b, float c, float d)
{
	return c + (x - a) / (b - a) * (d - c);
}

int MainLoop(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
	double streamTime, unsigned int status, void *userData)
{
	PedalboardSettings &settings = *((PedalboardSettings*)userData);
	//freqTransform = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FRAMESPERBUFFER);

	float *inBuf = (float*)inputBuffer;
	if (settings.playbacking)
	{
		inBuf = &(settings.floatWaveData[settings.wavReadHead]);
		settings.wavReadHead += nBufferFrames;
		if (settings.wavReadHead >= settings.gPlaybackWave.SubChunk2Size / 2)
		{
			settings.playbacking = false;
			settings.gPlaybackWave.ClearData();
			if (settings.floatWaveData)
				delete settings.floatWaveData;
			inBuf = (float*)inputBuffer;
		}
	}
	float outBuf[FRAMESPERBUFFER];
	p1 = fftwf_plan_dft_r2c_1d(FRAMESPERBUFFER, (float*)inBuf, inputFreqTransform, FFTW_ESTIMATE);
	memcpy(outBuf, inBuf, FRAMESPERBUFFER * sizeof(float));
	fftwf_execute(p1); /* repeat as needed */
	int blockSz = FRAMESPERBUFFER / 16;
	register float mulFact;
	register int iModBlockSz, iDivBlockSz;
	for(int i = 0; i < FRAMESPERBUFFER / 2; i++)
	{
		iModBlockSz = i % blockSz;
		iDivBlockSz = i / blockSz;
		mulFact = changeInterval(i,
					i - iModBlockSz, i - iModBlockSz + blockSz,
					settings.inputFreqKnobs[iDivBlockSz],
					settings.inputFreqKnobs[iDivBlockSz + 1]);
		inputFreqTransform[i][0] *= mulFact;
		inputFreqTransform[i][1] *= mulFact;
	}
	p2 = fftwf_plan_dft_c2r_1d(FRAMESPERBUFFER, inputFreqTransform, (float*)outBuf, FFTW_ESTIMATE);
	fftwf_execute(p2);
	for (int bufptr = 0; bufptr < FRAMESPERBUFFER; bufptr++)
	{//Downscale result by N
		((float*)outBuf)[bufptr] = ((float*)outBuf)[bufptr] / (float)FRAMESPERBUFFER;/// *5.f;
	}

	//if (status)
	//	std::cout << "Stream overflow detected!" << std::endl;
	
	if (settings.overdrive)
		applyOverdrive((float*)outBuf, (float*)outBuf, settings.overdriveGain);
	if (settings.fuzz)
		applyFuzz((float*)outBuf, (float*)outBuf, settings.fuzzGain, settings.fuzzMix);
	if (settings.distortion)
		applyDistorsion((float*)outBuf, (float*)outBuf, settings.distortionTimbre, settings.distortionDepth);
	if (settings.delay)
		applyDelay((float*)outBuf, (float*)outBuf, settings.delayTime, settings.delayFeedback);
	if (settings.compressor)
		applyCompression((float*)outBuf, (float*)outBuf);

	p3 = fftwf_plan_dft_r2c_1d(FRAMESPERBUFFER, (float*)outBuf, outputFreqTransform, FFTW_ESTIMATE);

	fftwf_execute(p3); /* repeat as needed */

	for (int bufptr = 0; bufptr < FRAMESPERBUFFER; bufptr++)
	{
		{
			float &re = inputFreqTransform[bufptr][0];
			float &im = inputFreqTransform[bufptr][1];
			inputFreqMagnitudes[bufptr] = sqrt(re * re + im * im);
		}
		{
			float &re = outputFreqTransform[bufptr][0];
			float &im = outputFreqTransform[bufptr][1];
			outputFreqMagnitudes[bufptr] = sqrt(re * re + im * im);
		}

		//printf("<%.1f>[%.1f]", ((float*)inputBuffer)[bufptr],((float*)outBuf)[bufptr]);
		viewedInputSamples[writeHead] = ((float*)inBuf)[bufptr];
		viewedOutputSamples[writeHead] = ((float*)outBuf)[bufptr];
		writeHead = (writeHead + 1) % 32768;
	}
	for (int i = 0; i < FRAMESPERBUFFER; i++)
	{
		((float*)outputBuffer)[2 * i] = outBuf[i];
		((float*)outputBuffer)[2 * i + 1] = outBuf[i];
	}
	fftwf_destroy_plan(p1);
	fftwf_destroy_plan(p2);
	fftwf_destroy_plan(p3);
	//for (int i = 0; i < 5; i++)
	return 0;
}
void STMaudio::PlayWAV(const char *filename)
{
	settings.gPlaybackWave.ClearData();
	settings.gPlaybackWave = ReadWaveFile(filename);
	if (settings.floatWaveData)
		delete settings.floatWaveData;
	settings.floatWaveData = new float[settings.gPlaybackWave.SubChunk2Size / 2];

	for (int i = 0; i < settings.gPlaybackWave.SubChunk2Size / 2; i++)
	{
		float dt = (float)(settings.gPlaybackWave.data[i]) / (float)SHRT_MAX;
		if(!i % 100)
			printf("%.2f ", dt);
		settings.floatWaveData[i] = dt;
	}
	settings.playbacking = true;
}

STMaudio::STMaudio()
{
	settings.inputFreqKnobs.resize(9, 1.f);
}

STMaudio::~STMaudio()
{
	
	//fftwf_free(out);

	try {
		// Stop the stream
		adc->stopStream();
	}
	catch (RtAudioError& e) {
		e.printMessage();
	}

	if (adc->isStreamOpen()) adc->closeStream();
}

void STMaudio::Run()
{

	char cCurrentPath[FILENAME_MAX];
	GetCurrentDir(cCurrentPath, sizeof(cCurrentPath));
	printf("CWD %s\n", cCurrentPath);

	adc = new RtAudio();

	// Create an api map.
	std::map<int, std::string> apiMap;
	apiMap[RtAudio::MACOSX_CORE] = "OS-X Core Audio";
	apiMap[RtAudio::WINDOWS_ASIO] = "Windows ASIO";
	apiMap[RtAudio::WINDOWS_DS] = "Windows Direct Sound";
	apiMap[RtAudio::WINDOWS_WASAPI] = "Windows WASAPI";
	apiMap[RtAudio::UNIX_JACK] = "Jack Client";
	apiMap[RtAudio::LINUX_ALSA] = "Linux ALSA";
	apiMap[RtAudio::LINUX_PULSE] = "Linux PulseAudio";
	apiMap[RtAudio::LINUX_OSS] = "Linux OSS";
	apiMap[RtAudio::RTAUDIO_DUMMY] = "RtAudio Dummy";

	std::vector< RtAudio::Api > apis;
	RtAudio::getCompiledApi(apis);

	std::cout << "\nRtAudio Version " << RtAudio::getVersion() << std::endl;

	std::cout << "\nCompiled APIs:\n";
	for (unsigned int i = 0; i<apis.size(); i++)
		std::cout << "  " << apiMap[apis[i]] << std::endl;

	RtAudio::DeviceInfo info;

	std::cout << "\nCurrent API: " << apiMap[adc->getCurrentApi()] << std::endl;

	unsigned int devices = adc->getDeviceCount();
	std::cout << "\nFound " << devices << " device(s) ...\n";
	int defaultOutputDevNumChannels;
	int defaultInputDevNumChannels;
	for (unsigned int i = 0; i<devices; i++) {
		info = adc->getDeviceInfo(i);

		std::cout << "\nDevice Name = " << info.name << '\n';
		if (info.probed == false)
			std::cout << "Probe Status = UNsuccessful\n";
		else {
			std::cout << "Probe Status = Successful\n";
			std::cout << "Output Channels = " << info.outputChannels << '\n';
			std::cout << "Input Channels = " << info.inputChannels << '\n';
			std::cout << "Duplex Channels = " << info.duplexChannels << '\n';
			if (info.isDefaultOutput)
			{
				std::cout << "This is the default output device.\n";
				defaultOutputDevNumChannels = info.outputChannels;
			}
			else std::cout << "This is NOT the default output device.\n";
			if (info.isDefaultInput)
			{
				std::cout << "This is the default input device.\n";
				defaultInputDevNumChannels = info.inputChannels;
			}

			else std::cout << "This is NOT the default input device.\n";
			if (info.nativeFormats == 0)
				std::cout << "No natively supported data formats(?)!";
			else {
				std::cout << "Natively supported data formats:\n";
				if (info.nativeFormats & RTAUDIO_SINT8)
					std::cout << "  8-bit int\n";
				if (info.nativeFormats & RTAUDIO_SINT16)
					std::cout << "  16-bit int\n";
				if (info.nativeFormats & RTAUDIO_SINT24)
					std::cout << "  24-bit int\n";
				if (info.nativeFormats & RTAUDIO_SINT32)
					std::cout << "  32-bit int\n";
				if (info.nativeFormats & RTAUDIO_FLOAT32)
					std::cout << "  32-bit float\n";
				if (info.nativeFormats & RTAUDIO_FLOAT64)
					std::cout << "  64-bit float\n";
			}
			if (info.sampleRates.size() < 1)
				std::cout << "No supported sample rates found!";
			else {
				std::cout << "Supported sample rates = ";
				for (unsigned int j = 0; j<info.sampleRates.size(); j++)
					std::cout << info.sampleRates[j] << " ";
			}
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;

	////////////////////////////////////////////////////
	if (adc->getDeviceCount() < 1) {
		std::cout << "\nNo audio devices found!\n";
		exit(0);
	}

	RtAudio::StreamParameters inparameters;
	inparameters.deviceId = adc->getDefaultInputDevice();
	inparameters.nChannels = 1;
	inparameters.firstChannel = 0;
	unsigned int sampleRate = 44100;
	unsigned int bufferFrames = 256; // 256 sample frames

	RtAudio::StreamParameters outparameters;
	outparameters.deviceId = adc->getDefaultOutputDevice();
	outparameters.nChannels = 2;
	outparameters.firstChannel = 0;

	try {
		adc->openStream(&outparameters, &inparameters, RTAUDIO_FLOAT32,
			sampleRate, &bufferFrames, &(MainLoop), &settings);
		adc->startStream();
	}
	catch (RtAudioError& e) {
		e.printMessage();
		exit(0);
	}
	std::cout << "Hello World, I'm an Digital Pedalboard & Audio Visualizer app. Enjoy me!";
}
