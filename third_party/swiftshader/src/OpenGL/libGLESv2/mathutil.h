// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// mathutil.h: Math and bit manipulation functions.

#ifndef LIBGLESV2_MATHUTIL_H_
#define LIBGLESV2_MATHUTIL_H_

#include "common/debug.h"
#include "Common/Math.hpp"
#include <limits>

namespace es2
{
inline bool isPow2(int x)
{
	return (x & (x - 1)) == 0 && (x != 0);
}

inline int log2(int x)
{
	int r = 0;
	while((x >> r) > 1) r++;
	return r;
}

inline unsigned int ceilPow2(unsigned int x)
{
	if(x != 0) x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;

	return x;
}

using sw::swap;
using sw::clamp;
using sw::clamp01;

template<const int n>
inline unsigned int unorm(float x)
{
	const unsigned int max = 0xFFFFFFFF >> (32 - n);

	if(x > 1)
	{
		return max;
	}
	else if(x < 0)
	{
		return 0;
	}
	else
	{
		return (unsigned int)(max * x + 0.5f);
	}
}

// Converts floating-point values to the nearest representable integer
inline int convert_float_int(float x)
{
	// The largest positive integer value that is exactly representable in IEEE 754 binary32 is 0x7FFFFF80.
	// The next floating-point value is 128 larger and thus needs clamping to 0x7FFFFFFF.
	static_assert(std::numeric_limits<float>::is_iec559, "Unsupported floating-point format");

	if(x > 0x7FFFFF80)
	{
		return 0x7FFFFFFF;
	}

	if(x < (signed)0x80000000)
	{
		return 0x80000000;
	}

	return static_cast<int>(roundf(x));
}

inline int convert_float_fixed(float x)
{
	return convert_float_int(static_cast<float>(0x7FFFFFFF) * x);
}
}

#endif   // LIBGLESV2_MATHUTIL_H_
