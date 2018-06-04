#include "GuitarEffects.hpp"

float dBFSdata[FRAMESPERBUFFER];
void applyCompression(float *out, float *in)
{
	float tmpin[FRAMESPERBUFFER];
	memcpy(tmpin, in, FRAMESPERBUFFER * sizeof(float));
	float lo = -1, hi = 1, mid;
	for (int i = 0; i < FRAMESPERBUFFER; i++)
		dBFSdata[i] = 20.f * log10(fabs((tmpin)[i]));
#define LOW_THRESHOLD -60
#define HIGH_THRESHOLD -40
	float wanteddB;
	for (int i = 0; i < FRAMESPERBUFFER; i++)
	{//upward compression
		
		if (dBFSdata[i] > HIGH_THRESHOLD)
		{
			lo = -1; hi = 1;
			wanteddB = (dBFSdata[i] - HIGH_THRESHOLD) * 0.5f + HIGH_THRESHOLD;
			while (hi - lo > 0.005f)
			{
				mid = (hi + lo) * 0.5f;
				dBFSdata[i] = 20.f * log10(fabs(mid));
				if (dBFSdata[i] > wanteddB)
					hi = mid;
				else
					lo = mid;
			}
			(out)[i] = mid;
		}
		else (out)[i] = (tmpin)[i];
	}
	for (int i = 0; i < FRAMESPERBUFFER; i++)
	{//downward compression
		
		if (dBFSdata[i] < LOW_THRESHOLD)
		{
			lo = -1; hi = 1;
			wanteddB = (dBFSdata[i] - LOW_THRESHOLD) * 0.5f + LOW_THRESHOLD;
			while (hi - lo > 0.005f)
			{
				mid = (hi + lo) * 0.5f;
				dBFSdata[i] = 20.f * log10(fabs(mid));
				if (dBFSdata[i] > wanteddB)
					hi = mid;
				else
					lo = mid;
			}
			(out)[i] = mid;
		}
		else (out)[i] = (tmpin)[i];
	}
}