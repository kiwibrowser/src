/*-------------------------------------------------------------------------
 * drawElements Base Portability Library
 * -------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief 16-bit floating-point math.
 *//*--------------------------------------------------------------------*/

#include "deFloat16.h"

DE_BEGIN_EXTERN_C

deFloat16 deFloat32To16 (float val32)
{
	deUint32	sign;
	int			expotent;
	deUint32	mantissa;
	union
	{
		float		f;
		deUint32	u;
	} x;

	x.f			= val32;
	sign		= (x.u >> 16u) & 0x00008000u;
	expotent	= (int)((x.u >> 23u) & 0x000000ffu) - (127 - 15);
	mantissa	= x.u & 0x007fffffu;

	if (expotent <= 0)
	{
		if (expotent < -10)
		{
			/* Rounds to zero. */
			return (deFloat16) sign;
		}

		/* Converted to denormalized half, add leading 1 to significand. */
		mantissa = mantissa | 0x00800000u;

		/* Round mantissa to nearest (10+e) */
		{
			deUint32 t = 14u - expotent;
			deUint32 a = (1u << (t - 1u)) - 1u;
			deUint32 b = (mantissa >> t) & 1u;

			mantissa = (mantissa + a + b) >> t;
		}

		return (deFloat16) (sign | mantissa);
	}
	else if (expotent == 0xff - (127 - 15))
	{
		if (mantissa == 0u)
		{
			/* InF */
			return (deFloat16) (sign | 0x7c00u);
		}
		else
		{
			/* NaN */
			mantissa >>= 13u;
			return (deFloat16) (sign | 0x7c00u | mantissa | (mantissa == 0u));
		}
	}
	else
	{
		/* Normalized float. */
		mantissa = mantissa + 0x00000fffu + ((mantissa >> 13u) & 1u);

		if (mantissa & 0x00800000u)
		{
			/* Overflow in mantissa. */
			mantissa  = 0u;
			expotent += 1;
		}

		if (expotent > 30)
		{
			/* \todo [pyry] Cause hw fp overflow */
			return (deFloat16) (sign | 0x7c00u);
		}

		return (deFloat16) (sign | ((deUint32)expotent << 10u) | (mantissa >> 13u));
	}
}

/*--------------------------------------------------------------------*//*!
 * \brief Round the given number `val` to nearest even by discarding
 *        the last `numBitsToDiscard` bits.
 * \param val value to round
 * \param numBitsToDiscard number of (least significant) bits to discard
 * \return The rounded value with the last `numBitsToDiscard` removed
 *//*--------------------------------------------------------------------*/
static deUint32 roundToNearestEven (deUint32 val, const deUint32 numBitsToDiscard)
{
	const deUint32	lastBits	= val & ((1 << numBitsToDiscard) - 1);
	const deUint32	headBit		= val & (1 << (numBitsToDiscard - 1));

	DE_ASSERT(numBitsToDiscard > 0 && numBitsToDiscard < 32);	/* Make sure no overflow. */
	val >>= numBitsToDiscard;

	if (headBit == 0)
	{
		return val;
	}
	else if (headBit == lastBits)
	{
		if ((val & 0x1) == 0x1)
		{
			return val + 1;
		}
		else
		{
			return val;
		}
	}
	else
	{
		return val + 1;
	}
}

deFloat16 deFloat32To16Round (float val32, deRoundingMode mode)
{
	union
	{
		float		f;		/* Interpret as 32-bit float */
		deUint32	u;		/* Interpret as 32-bit unsigned integer */
	} x;
	deUint32	sign;		/* sign : 0000 0000 0000 0000 X000 0000 0000 0000 */
	deUint32	exp32;		/* exp32: biased exponent for 32-bit floats */
	int			exp16;		/* exp16: biased exponent for 16-bit floats */
	deUint32	mantissa;

	/* We only support these two rounding modes for now */
	DE_ASSERT(mode == DE_ROUNDINGMODE_TO_ZERO || mode == DE_ROUNDINGMODE_TO_NEAREST_EVEN);

	x.f			= val32;
	sign		= (x.u >> 16u) & 0x00008000u;
	exp32		= (x.u >> 23u) & 0x000000ffu;
	exp16		= (int) (exp32) - 127 + 15;	/* 15/127: exponent bias for 16-bit/32-bit floats */
	mantissa	= x.u & 0x007fffffu;

	/* Case: zero and denormalized floats */
	if (exp32 == 0)
	{
		/* Denormalized floats are < 2^(1-127), not representable in 16-bit floats, rounding to zero. */
		return (deFloat16) sign;
	}
	/* Case: Inf and NaN */
	else if (exp32 == 0x000000ffu)
	{
		if (mantissa == 0u)
		{
			/* Inf */
			return (deFloat16) (sign | 0x7c00u);
		}
		else
		{
			/* NaN */
			mantissa >>= 13u;	/* 16-bit floats has 10-bit for mantissa, 13-bit less than 32-bit floats. */
			/* Make sure we don't turn NaN into zero by | (mantissa == 0). */
			return (deFloat16) (sign | 0x7c00u | mantissa | (mantissa == 0u));
		}
	}
	/* The following are cases for normalized floats.
	 *
	 * * If exp16 is less than 0, we are experiencing underflow for the exponent. To encode this underflowed exponent,
	 *   we can only shift the mantissa further right.
	 *   The real exponent is exp16 - 15. A denormalized 16-bit float can represent -14 via its exponent.
	 *   Note that the most significant bit in the mantissa of a denormalized float is already -1 as for exponent.
	 *   So, we just need to right shift the mantissa -exp16 bits.
	 * * If exp16 is 0, mantissa shifting requirement is similar to the above.
	 * * If exp16 is greater than 30 (0b11110), we are experiencing overflow for the exponent of 16-bit normalized floats.
	 */
	/* Case: normalized floats -> zero */
	else if (exp16 < -10)
	{
		/* 16-bit floats have only 10 bits for mantissa. Minimal 16-bit denormalized float is (2^-10) * (2^-14). */
		/* Expecting a number < (2^-10) * (2^-14) here, not representable, round to zero. */
		return (deFloat16) sign;
	}
	/* Case: normalized floats -> zero and denormalized halfs */
	else if (exp16 <= 0)
	{
		/* Add the implicit leading 1 in mormalized float to mantissa. */
		mantissa |= 0x00800000u;
		/* We have a (23 + 1)-bit mantissa, but 16-bit floats only expect 10-bit mantissa.
		 * Need to discard the last 14-bits considering rounding mode.
		 * We also need to shift right -exp16 bits to encode the underflowed exponent.
		 */
		if (mode == DE_ROUNDINGMODE_TO_ZERO)
		{
			mantissa >>= (14 - exp16);
		}
		else
		{
			/* mantissa in the above may exceed 10-bits, in which case overflow happens.
			 * The overflowed bit is automatically carried to exponent then.
			 */
			mantissa = roundToNearestEven(mantissa, 14 - exp16);
		}
		return (deFloat16) (sign | mantissa);
	}
	/* Case: normalized floats -> normalized floats */
	else if (exp16 <= 30)
	{
		if (mode == DE_ROUNDINGMODE_TO_ZERO)
		{
			return (deFloat16) (sign | ((deUint32)exp16 << 10u) | (mantissa >> 13u));
		}
		else
		{
			mantissa	= roundToNearestEven(mantissa, 13);
			/* Handle overflow. exp16 may overflow (and become Inf) itself, but that's correct. */
			exp16		= (exp16 << 10u) + (mantissa & (1 << 10));
			mantissa	&= (1u << 10) - 1;
			return (deFloat16) (sign | ((deUint32) exp16) | mantissa);
		}
	}
	/* Case: normalized floats (too large to be representable as 16-bit floats) */
	else
	{
		/* According to IEEE Std 754-2008 Section 7.4,
		 * * roundTiesToEven and roundTiesToAway carry all overflows to Inf with the sign
		 *   of the intermediate  result.
		 * * roundTowardZero carries all overflows to the format’s largest finite number
		 *   with the sign of the intermediate result.
		 */
		if (mode == DE_ROUNDINGMODE_TO_ZERO)
		{
			return (deFloat16) (sign | 0x7bffu); /* 111 1011 1111 1111 */
		}
		else
		{
			return (deFloat16) (sign | (0x1f << 10));
		}
	}

	/* Make compiler happy */
	return (deFloat16) 0;
}

float deFloat16To32 (deFloat16 val16)
{
	deUint32 sign;
	deUint32 expotent;
	deUint32 mantissa;
	union
	{
		float		f;
		deUint32	u;
	} x;

	x.u			= 0u;

	sign		= ((deUint32)val16 >> 15u) & 0x00000001u;
	expotent	= ((deUint32)val16 >> 10u) & 0x0000001fu;
	mantissa	= (deUint32)val16 & 0x000003ffu;

	if (expotent == 0u)
	{
		if (mantissa == 0u)
		{
			/* +/- 0 */
			x.u = sign << 31u;
			return x.f;
		}
		else
		{
			/* Denormalized, normalize it. */

			while (!(mantissa & 0x00000400u))
			{
				mantissa <<= 1u;
				expotent -=  1u;
			}

			expotent += 1u;
			mantissa &= ~0x00000400u;
		}
	}
	else if (expotent == 31u)
	{
		if (mantissa == 0u)
		{
			/* +/- InF */
			x.u = (sign << 31u) | 0x7f800000u;
			return x.f;
		}
		else
		{
			/* +/- NaN */
			x.u = (sign << 31u) | 0x7f800000u | (mantissa << 13u);
			return x.f;
		}
	}

	expotent = expotent + (127u - 15u);
	mantissa = mantissa << 13u;

	x.u = (sign << 31u) | (expotent << 23u) | mantissa;
	return x.f;
}

DE_END_EXTERN_C
