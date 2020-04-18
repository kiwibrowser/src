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

#ifndef LIBGLES_CM_MATHUTIL_H_
#define LIBGLES_CM_MATHUTIL_H_

#include "common/debug.h"
#include "Common/Math.hpp"

namespace es1
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
}

#endif   // LIBGLES_CM_MATHUTIL_H_
