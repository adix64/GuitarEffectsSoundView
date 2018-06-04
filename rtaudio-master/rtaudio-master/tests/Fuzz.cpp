#include "GuitarEffects.hpp"
void applyFuzz(float *out, float *in, float gain, float mix)
{
	float q;
	for (int bufptr = 0; bufptr < FRAMESPERBUFFER; bufptr++)
	{
		if ((in)[bufptr] == 0.f)
		{
			(out)[bufptr] = 0.f;
			continue;
		}
		q = (in)[bufptr] / abs((in)[bufptr]);
		(out)[bufptr] = q * (1.f - exp(gain * q * (in)[bufptr]));
		(out)[bufptr] = mix * (out)[bufptr] + (1 - mix) * (in)[bufptr];
	}
}
