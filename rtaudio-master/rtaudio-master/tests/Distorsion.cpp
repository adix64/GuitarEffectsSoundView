#include "GuitarEffects.hpp"

void applyDistorsion(float *out, float *in, float timbre, float depth)
{
	float tmp;
	float timbreInverse = (1 - (timbre * 0.099)) * 10; //inverse scaling from timbre
	for (int bufptr = 0; bufptr<FRAMESPERBUFFER; bufptr++) {
		tmp = (in)[bufptr] * depth;
		tmp = tanh((tmp * (timbre + 1)));
		tmp = (tmp * ((0.1 + timbre) * timbreInverse));
		tmp = cos((tmp + (timbre + 0.25)));
		tmp = tanh(tmp * (timbre + 1));
		tmp = tmp * 0.5;

		(out)[bufptr] = tmp;
	}
}