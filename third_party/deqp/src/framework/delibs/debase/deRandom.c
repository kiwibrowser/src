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
 * \brief Random number generation.
 *//*--------------------------------------------------------------------*/

#include "deRandom.h"

#include <float.h>
#include <math.h>

DE_BEGIN_EXTERN_C

/*--------------------------------------------------------------------*//*!
 * \brief Initialize a random number generator with a given seed.
 * \param rnd	RNG to initialize.
 * \param seed	Seed value used for random values.
 *//*--------------------------------------------------------------------*/
void deRandom_init (deRandom* rnd, deUint32 seed)
{
	rnd->x = (deUint32)(-(int)seed ^ 123456789);
	rnd->y = (deUint32)(362436069 * seed);
	rnd->z = (deUint32)(521288629 ^ (seed >> 7));
	rnd->w = (deUint32)(88675123 ^ (seed << 3));
}

/*--------------------------------------------------------------------*//*!
 * \brief Get a pseudo random uint32.
 * \param rnd	Pointer to RNG.
 * \return Random uint32 number.
 *//*--------------------------------------------------------------------*/
deUint32 deRandom_getUint32 (deRandom* rnd)
{
	deUint32 w = rnd->w;
	deUint32 t;

	t = rnd->x ^ (rnd->x << 11);
	rnd->x = rnd->y;
	rnd->y = rnd->z;
	rnd->z = w;
	rnd->w = w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
	return w;
}

/*--------------------------------------------------------------------*//*!
 * \brief Get a pseudo random uint64.
 * \param rnd	Pointer to RNG.
 * \return Random uint64 number.
 *//*--------------------------------------------------------------------*/
deUint64 deRandom_getUint64 (deRandom* rnd)
{
	deUint64 x = deRandom_getUint32(rnd);
	return x << 32 | deRandom_getUint32(rnd);
}

/*--------------------------------------------------------------------*//*!
 * \brief Get a pseudo random float in range [0, 1[.
 * \param rnd	Pointer to RNG.
 * \return Random float number.
 *//*--------------------------------------------------------------------*/
float deRandom_getFloat (deRandom* rnd)
{
	return (float)(deRandom_getUint32(rnd) & 0xFFFFFFFu) / (float)(0xFFFFFFFu+1);
}

/*--------------------------------------------------------------------*//*!
 * \brief Get a pseudo random float in range [0, 1[.
 * \param rnd	Pointer to RNG.
 * \return Random float number.
 *//*--------------------------------------------------------------------*/
double deRandom_getDouble (deRandom* rnd)
{
	DE_STATIC_ASSERT(FLT_RADIX == 2);
	return ldexp((double)(deRandom_getUint64(rnd) & ((1ull << DBL_MANT_DIG) - 1)),
				 -DBL_MANT_DIG);
}

/*--------------------------------------------------------------------*//*!
 * \brief Get a pseudo random boolean value (DE_FALSE or DE_TRUE).
 * \param rnd	Pointer to RNG.
 * \return Random float number.
 *//*--------------------------------------------------------------------*/
deBool deRandom_getBool (deRandom* rnd)
{
	deUint32 val = deRandom_getUint32(rnd);
	return ((val & 0xFFFFFF) < 0x800000);
}

DE_END_EXTERN_C
