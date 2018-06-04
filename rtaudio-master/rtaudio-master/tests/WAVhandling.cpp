#include "WAVhandling.hpp"
#include <iostream>
#include <algorithm>
#include <string.h>
#include <unordered_map>
#include <limits.h>
#include <math.h>

using namespace std;
#define PI 3.1415926f

template <typename T> int sign(T val)
{
	return (T(0) < val) - (val < T(0));
}

void PrintWave(Wave &wave)
{
	printf("ChunkID: %s\n", wave.ChunkID);
	printf("Chunk size: %d\n", wave.ChunkSize);
	printf("Format: %s\n", wave.Format);
	printf("Subchunk1ID: %s\n", wave.Subchunk1ID);
	printf("Subchunk1Size: %d\n", wave.SubChunk1Size);
	printf("AudioFormat: %d\n", wave.AudioFormat);
	printf("NumChannels: %d\n", wave.NumChannels);
	printf("SampleRate: %d\n", wave.SampleRate);
	printf("ByteRate: %d\n", wave.ByteRate);
	printf("BlockAlign: %d\n", wave.BlockAlign);
	printf("BitsPerSample: %d\n", wave.BitsPerSample);
	if (wave.AudioFormat != 1)
		printf("ExtraParamsSize: %d\n", wave.ExtraParamsSize);
	printf("Subchunk2ID: %s\n", wave.SubChunk2ID);
	if (wave.AudioFormat != 1)
		printf("ExtraParams: %d\n", wave.ExtraParams);
	printf("Subchunk2Size: %d\n", wave.SubChunk2Size);
}

Wave ReadWaveFileFormat(FILE *fp)
{
	Wave wave;

	fread(wave.ChunkID, 1, 4, fp);//RIFF
	fread(&(wave.ChunkSize), 4, 1, fp); // 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size) sau 36 + SubChunk2Size
	fread(wave.Format, 1, 4, fp);
	fread(wave.Subchunk1ID, 1, 4, fp);
	fread(&(wave.SubChunk1Size), 4, 1, fp);//16 ptr PCM, dimensiunea restului subchunkului
	fread(&(wave.AudioFormat), 2, 1, fp);//PCM = 1 (cuantizare liniara), altfel anumita compresie
	fread(&(wave.NumChannels), 2, 1, fp);
	fread(&(wave.SampleRate), 4, 1, fp);//rata de esantionare(8000 sau 44100, etc.)
	fread(&(wave.ByteRate), 4, 1, fp); // SampleRate * NumChannels * BitsPerSample / 8
	fread(&(wave.BlockAlign), 2, 1, fp); // NumChannels * BitsPerSample / 8, numarul de octeti ptr un esantion incluzand toate canalele
	fread(&(wave.BitsPerSample), 2, 1, fp);//8, 16, etc...
	if (wave.AudioFormat != 1)
		fread(&(wave.ExtraParamsSize), 2, 1, fp); // NumChannels * BitsPerSample / 8, numarul de octeti ptr un esantion incluzand toate canalele
	fread(wave.SubChunk2ID, 1, 4, fp);
	if (wave.AudioFormat != 1)
		fread(&(wave.ExtraParams), 4, 1, fp); // NumSamples * NumChannels * BitsPerSample / 8, nr de octeti de date

	fread(&(wave.SubChunk2Size), 4, 1, fp); // NumSamples * NumChannels * BitsPerSample / 8, nr de octeti de date
	PrintWave(wave);
	return wave;
}

Wave ReadWaveFile(const char * file)
{
	FILE *fp = fopen(file, "rb");
	printf("Reading 16bit PCM WAV file : %s...\n", file);

	Wave wave = ReadWaveFileFormat(fp);

	wave.data = new signed short int[wave.SubChunk2Size / 2];
	fread(wave.data, 2, wave.SubChunk2Size / 2, fp); // NumSamples * NumChannels * BitsPerSample / 8, nr de octeti de date
	fclose(fp);
	return wave;
}

//////////////////////////////////////////////////////////////////////////////////////////////

short int WriteWaveFileFormat(FILE *fp, Wave &wave)
{
	fwrite(wave.ChunkID, 1, 4, fp);//RIFF
	fwrite(&(wave.ChunkSize), 4, 1, fp); // 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size) sau 36 + SubChunk2Size
	fwrite(wave.Format, 1, 4, fp);
	fwrite(wave.Subchunk1ID, 1, 4, fp);
	fwrite(&(wave.SubChunk1Size), 4, 1, fp);//16 ptr PCM, dimensiunea restului subchunkului
	fwrite(&(wave.AudioFormat), 2, 1, fp);//PCM = 1 (cuantizare liniara), altfel anumita compresie
	fwrite(&(wave.NumChannels), 2, 1, fp);
	fwrite(&(wave.SampleRate), 4, 1, fp);//rata de esantionare(8000 sau 44100, etc.)
	fwrite(&(wave.ByteRate), 4, 1, fp); // SampleRate * NumChannels * BitsPerSample / 8
	fwrite(&(wave.BlockAlign), 2, 1, fp); // NumChannels * BitsPerSample / 8, numarul de octeti ptr un esantion incluzand toate canalele
	fwrite(&(wave.BitsPerSample), 2, 1, fp);//8, 16, etc...
	if (wave.AudioFormat != 1)
		fwrite(&(wave.ExtraParamsSize), 2, 1, fp); // NumChannels * BitsPerSample / 8, numarul de octeti ptr un esantion incluzand toate canalele
	fwrite(wave.SubChunk2ID, 1, 4, fp);
	if (wave.AudioFormat != 1)
		fwrite(&(wave.ExtraParams), 4, 1, fp); // NumSamples * NumChannels * BitsPerSample / 8, nr de octeti de date
	fwrite(&(wave.SubChunk2Size), 4, 1, fp); // NumSamples * NumChannels * BitsPerSample / 8, nr de octeti de date
	return wave.SubChunk2Size;
}

//////////////////////////////////////////////////////////////////////////////////////////////

std::vector<short int> createWAVsampleRate(const char *fname, std::vector<float> & maxAmplitudes, std::vector<float> &soundFreqs, int NumSamples)
{
	FILE *fp = fopen(fname, "wb");
	Wave wave;
	wave.ChunkSize = 36 + 2 * NumSamples;
	wave.SubChunk1Size = 16;
	wave.AudioFormat = 1;
	wave.NumChannels = 1;
	wave.SampleRate = 8000;
	wave.ByteRate = 16000;
	wave.BlockAlign = 2;
	wave.BitsPerSample = 16;
	wave.SubChunk2Size = 2 * NumSamples;
	WriteWaveFileFormat(fp, wave);
	short int myshort;
	std::vector<short int> samples(NumSamples);
	std::vector<float> factors(maxAmplitudes.size());

	for (int j = 0; j < soundFreqs.size(); j++)
		factors[j] = (soundFreqs[j] * 2 * PI) / 8000.f;

	for (int i = 0; i < NumSamples; i++)
	{
		myshort = 0;
		for (int j = 0; j < soundFreqs.size(); j++)
		{
			myshort += (signed short int) (maxAmplitudes[j] * sin((float)i * factors[j]));
		}
		samples[i] = myshort;
		fwrite(&myshort, 2, 1, fp);
	}
	fclose(fp);
	return samples;
}