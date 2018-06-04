#pragma once
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
class Wave
{
public:
	char ChunkID[5];
	int ChunkSize = -1;
	char Format[5];
	char Subchunk1ID[5];
	int SubChunk1Size = -1;
	short int AudioFormat = -1;
	short int NumChannels = -1;
	int SampleRate = -1;
	int ByteRate = -1;
	short int BlockAlign = -1;
	short int BitsPerSample = -1;
	short int ExtraParamsSize = -1;
	char SubChunk2ID[5];
	int SubChunk2Size = -1;
	int ExtraParams = -1;
	signed short int *data = NULL;

	Wave()
	{
		sprintf_s(ChunkID, "RIFF");
		sprintf_s(Format,"WAVE");
		sprintf_s(Subchunk1ID,"fmt ");
		sprintf_s(SubChunk2ID, "data");
	}
	void ClearData()
	{
		if(data)
			delete data;
	}
};

void PrintWave(Wave &wave);

Wave ReadWaveFileFormat(FILE *fp);

Wave ReadWaveFile(const char * file);

//////////////////////////////////////////////////////////////////////////////////////////////

short int WriteWaveFileFormat(FILE *fp, Wave &wave);

std::vector<short int> createWAVsampleRate(const char *fname, std::vector<float> & maxAmplitudes, std::vector<float> &soundFreqs, int NumSamples);