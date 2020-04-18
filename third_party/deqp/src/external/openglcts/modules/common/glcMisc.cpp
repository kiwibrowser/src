/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file glcMisc.cpp
 * \brief Miscellaneous helper functions.
 */ /*-------------------------------------------------------------------*/

#include "glcMisc.hpp"

using namespace glw;

namespace glcts
{

/* utility functions from the book OpenGL ES 2.0 Programming Guide */
/* -15 stored using a single precision bias of 127 */
const unsigned int HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP = 0x38000000;
/* max exponent value in single precision that will be converted */
/* to Inf or Nan when stored as a half-float */
const unsigned int HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP = 0x47800000;
/* 255 is the max exponent biased value */
const unsigned int FLOAT_MAX_BIASED_EXP		 = (0xFF << 23);
const unsigned int HALF_FLOAT_MAX_BIASED_EXP = (0x1F << 10);

GLhalf floatToHalfFloat(float f)
{
	union {
		float		 v;
		unsigned int x;
	};
	v				  = f;
	unsigned int sign = (GLhalf)(x >> 31);
	unsigned int mantissa;
	unsigned int exp;
	GLhalf		 hf;

	/* get mantissa */
	mantissa = x & ((1 << 23) - 1);
	/* get exponent bits */
	exp = x & FLOAT_MAX_BIASED_EXP;
	if (exp >= HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP)
	{
		/* check if the original single precision float number is a NaN */
		if (mantissa && (exp == FLOAT_MAX_BIASED_EXP))
		{
			/* we have a single precision NaN */
			mantissa = (1 << 23) - 1;
		}
		else
		{
			/* 16-bit half-float representation stores number as Inf */
			mantissa = 0;
		}
		hf = (((GLhalf)sign) << 15) | (GLhalf)(HALF_FLOAT_MAX_BIASED_EXP) | (GLhalf)(mantissa >> 13);
	}
	/* check if exponent is <= -15 */
	else if (exp <= HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP)
	{
		/* store a denorm half-float value or zero */
		exp = (HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP - exp) >> 23;
		mantissa |= (1 << 23);
		mantissa >>= (14 + exp);
		hf = (((GLhalf)sign) << 15) | (GLhalf)(mantissa);
	}
	else
	{
		hf = (((GLhalf)sign) << 15) | (GLhalf)((exp - HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP) >> 13) |
			 (GLhalf)(mantissa >> 13);
	}

	return hf;
}

/* -15 stored using a single precision bias of 127 */
const unsigned int FLOAT11_MIN_BIASED_EXP_AS_SINGLE_FP_EXP = 0x38000000;
/* max exponent value in single precision that will be converted */
/* to Inf or Nan when stored as a half-float */
const unsigned int FLOAT11_MAX_BIASED_EXP_AS_SINGLE_FP_EXP = 0x47800000;
const unsigned int FLOAT11_MAX_BIASED_EXP				   = (0x1F << 6);

GLuint floatToUnisgnedF11(float f)
{
	union {
		float		 v;
		unsigned int x;
	};
	v				  = f;
	unsigned int sign = (GLhalf)(x >> 31);
	unsigned int mantissa;
	unsigned int exp;
	GLuint		 f11;

	/* get mantissa */
	mantissa = x & ((1 << 23) - 1);
	/* get exponent bits */
	exp = x & FLOAT_MAX_BIASED_EXP;

	/* minus f32 value */
	if (sign > 0)
	{
		/* f32 NaN -> f11 NaN */
		if (mantissa && (exp == FLOAT_MAX_BIASED_EXP))
		{
			f11 = (GLuint)(FLOAT11_MAX_BIASED_EXP) | (GLuint)(0x0000003F);
		}
		/* Others round to 0.0 */
		else
		{
			f11 = 0x00000000;
		}

		return f11;
	}

	/* only check positive value below */
	if (exp >= FLOAT11_MAX_BIASED_EXP_AS_SINGLE_FP_EXP)
	{
		/* check if the original single precision float number is a NaN */
		if (mantissa && (exp == FLOAT_MAX_BIASED_EXP))
		{
			/* we have a single precision NaN */
			mantissa = (1 << 23) - 1;
		}
		else
		{
			/* 11-bit float representation stores number as Inf */
			mantissa = 0;
		}
		f11 = (GLuint)(FLOAT11_MAX_BIASED_EXP) | (GLuint)(mantissa >> 17);
	}
	/* check if exponent is <= -15 */
	else if (exp <= FLOAT11_MIN_BIASED_EXP_AS_SINGLE_FP_EXP)
	{
		/* store a denorm 11-bit float value or zero */
		exp = (FLOAT11_MIN_BIASED_EXP_AS_SINGLE_FP_EXP - exp) >> 23;
		mantissa |= (1 << 23);
		if (18 + exp >= sizeof(mantissa) * 8)
		{
			mantissa = 0;
		}
		else
		{
			mantissa >>= (18 + exp);
		}
		f11 = mantissa;
	}
	else
	{
		f11 = ((exp - FLOAT11_MIN_BIASED_EXP_AS_SINGLE_FP_EXP) >> 17) | (mantissa >> 17);
	}

	return f11;
}

/* -15 stored using a single precision bias of 127 */
const unsigned int FLOAT10_MIN_BIASED_EXP_AS_SINGLE_FP_EXP = 0x38000000;
/* max exponent value in single precision that will be converted */
/* to Inf or Nan when stored as a half-float */
const unsigned int FLOAT10_MAX_BIASED_EXP_AS_SINGLE_FP_EXP = 0x47800000;
const unsigned int FLOAT10_MAX_BIASED_EXP				   = (0x1F << 5);

GLuint floatToUnisgnedF10(float f)
{
	union {
		float		 v;
		unsigned int x;
	};
	v				  = f;
	unsigned int sign = (GLhalf)(x >> 31);
	unsigned int mantissa;
	unsigned int exp;
	GLuint		 f10;

	/* get mantissa */
	mantissa = x & ((1 << 23) - 1);
	/* get exponent bits */
	exp = x & FLOAT_MAX_BIASED_EXP;

	/* minus f32 value */
	if (sign > 0)
	{
		/* f32 NaN -> f10 NaN */
		if (mantissa && (exp == FLOAT_MAX_BIASED_EXP))
		{
			f10 = (GLuint)(FLOAT10_MAX_BIASED_EXP) | (GLuint)(0x0000001F);
		}
		/* Others round to 0.0 */
		else
		{
			f10 = 0x00000000;
		}

		return f10;
	}

	/* only check positive value below */
	if (exp >= FLOAT10_MAX_BIASED_EXP_AS_SINGLE_FP_EXP)
	{
		/* check if the original single precision float number is a NaN */
		if (mantissa && (exp == FLOAT_MAX_BIASED_EXP))
		{
			/* we have a single precision NaN */
			mantissa = (1 << 23) - 1;
		}
		else
		{
			/* 10-bit float representation stores number as Inf */
			mantissa = 0;
		}
		f10 = (GLuint)(FLOAT10_MAX_BIASED_EXP) | (GLuint)(mantissa >> 18);
	}
	/* check if exponent is <= -15 */
	else if (exp <= FLOAT10_MIN_BIASED_EXP_AS_SINGLE_FP_EXP)
	{
		/* store a denorm 11-bit float value or zero */
		exp = (FLOAT10_MIN_BIASED_EXP_AS_SINGLE_FP_EXP - exp) >> 23;
		mantissa |= (1 << 23);
		if (19 + exp >= sizeof(mantissa) * 8)
		{
			mantissa = 0;
		}
		else
		{
			mantissa >>= (19 + exp);
		}
		f10 = mantissa;
	}
	else
	{
		f10 = ((exp - FLOAT10_MIN_BIASED_EXP_AS_SINGLE_FP_EXP) >> 18) | (mantissa >> 18);
	}

	return f10;
}

float halfFloatToFloat(GLhalf hf)
{
	unsigned int sign	 = (unsigned int)(hf >> 15);
	unsigned int mantissa = (unsigned int)(hf & ((1 << 10) - 1));
	unsigned int exp	  = (unsigned int)(hf & HALF_FLOAT_MAX_BIASED_EXP);
	union {
		float		 f;
		unsigned int ui;
	};

	if (exp == HALF_FLOAT_MAX_BIASED_EXP)
	{
		/* we have a half-float NaN or Inf */
		/* half-float NaNs will be converted to a single precision NaN */
		/* half-float Infs will be converted to a single precision Inf */
		exp = FLOAT_MAX_BIASED_EXP;
		if (mantissa)
			mantissa = (1 << 23) - 1; /* set all bits to indicate a NaN */
	}
	else if (exp == 0x0)
	{
		/* convert half-float zero/denorm to single precision value */
		if (mantissa)
		{
			mantissa <<= 1;
			exp = HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;
			/* check for leading 1 in denorm mantissa */
			while ((mantissa & (1 << 10)) == 0)
			{
				/* for every leading 0, decrement single precision exponent by 1 */
				/* and shift half-float mantissa value to the left */
				mantissa <<= 1;
				exp -= (1 << 23);
			}
			/* clamp the mantissa to 10-bits */
			mantissa &= ((1 << 10) - 1);
			/* shift left to generate single-precision mantissa of 23-bits */
			mantissa <<= 13;
		}
	}
	else
	{
		/* shift left to generate single-precision mantissa of 23-bits */
		mantissa <<= 13;
		/* generate single precision biased exponent value */
		exp = (exp << 13) + HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;
	}
	ui = (sign << 31) | exp | mantissa;
	return f;
}

float unsignedF11ToFloat(GLuint f11)
{
	unsigned int mantissa = (unsigned int)(f11 & ((1 << 6) - 1));
	unsigned int exp	  = (unsigned int)(f11 & FLOAT11_MAX_BIASED_EXP);
	union {
		float		 f;
		unsigned int ui;
	};

	if (exp == FLOAT11_MAX_BIASED_EXP)
	{
		/* we have a f11 NaN or Inf */
		/* f11 NaNs will be converted to a single precision NaN */
		/* f11 Infs will be converted to a single precision Inf */
		exp = FLOAT_MAX_BIASED_EXP;
		if (mantissa)
			mantissa = (1 << 23) - 1; /* set all bits to indicate a NaN */
	}
	else if (exp == 0x0)
	{
		/* convert f11 zero/denorm to single precision value */
		if (mantissa)
		{
			mantissa <<= 1;
			exp = FLOAT11_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;
			/* check for leading 1 in denorm mantissa */
			while ((mantissa & (1 << 10)) == 0)
			{
				/* for every leading 0, decrement single precision exponent by 1 */
				/* and shift half-float mantissa value to the left */
				mantissa <<= 1;
				exp -= (1 << 23);
			}
			/* clamp the mantissa to 6-bits */
			mantissa &= ((1 << 6) - 1);
			/* shift left to generate single-precision mantissa of 23-bits */
			mantissa <<= 17;
		}
	}
	else
	{
		/* shift left to generate single-precision mantissa of 23-bits */
		mantissa <<= 17;
		/* generate single precision biased exponent value */
		exp = (exp << 17) + FLOAT11_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;
	}
	ui = exp | mantissa;
	return f;
}

float unsignedF10ToFloat(GLuint f10)
{
	unsigned int mantissa = (unsigned int)(f10 & ((1 << 5) - 1));
	unsigned int exp	  = (unsigned int)(f10 & FLOAT10_MAX_BIASED_EXP);
	union {
		float		 f;
		unsigned int ui;
	};

	if (exp == FLOAT10_MAX_BIASED_EXP)
	{
		/* we have a f11 NaN or Inf */
		/* f11 NaNs will be converted to a single precision NaN */
		/* f11 Infs will be converted to a single precision Inf */
		exp = FLOAT_MAX_BIASED_EXP;
		if (mantissa)
			mantissa = (1 << 23) - 1; /* set all bits to indicate a NaN */
	}
	else if (exp == 0x0)
	{
		/* convert f11 zero/denorm to single precision value */
		if (mantissa)
		{
			mantissa <<= 1;
			exp = FLOAT10_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;
			/* check for leading 1 in denorm mantissa */
			while ((mantissa & (1 << 10)) == 0)
			{
				/* for every leading 0, decrement single precision exponent by 1 */
				/* and shift half-float mantissa value to the left */
				mantissa <<= 1;
				exp -= (1 << 23);
			}
			/* clamp the mantissa to 5-bits */
			mantissa &= ((1 << 5) - 1);
			/* shift left to generate single-precision mantissa of 23-bits */
			mantissa <<= 18;
		}
	}
	else
	{
		/* shift left to generate single-precision mantissa of 23-bits */
		mantissa <<= 18;
		/* generate single precision biased exponent value */
		exp = (exp << 18) + FLOAT10_MIN_BIASED_EXP_AS_SINGLE_FP_EXP;
	}
	ui = exp | mantissa;
	return f;
}

} // glcts
