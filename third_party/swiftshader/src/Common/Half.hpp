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

#ifndef sw_Half_hpp
#define sw_Half_hpp

namespace sw
{
	class half
	{
	public:
		half() = default;
		explicit half(float f);

		operator float() const;

		half &operator=(half h);
		half &operator=(float f);

	private:
		unsigned short fp16i;
	};

	inline half shortAsHalf(short s)
	{
		union
		{
			half h;
			short s;
		} hs;

		hs.s = s;

		return hs.h;
	}

	class RGB9E5
	{
		unsigned int R : 9;
		unsigned int G : 9;
		unsigned int B : 9;
		unsigned int E : 5;

	public:
		void toRGB16F(half rgb[3]) const
		{
			constexpr int offset = 24;   // Exponent bias (15) + number of mantissa bits per component (9) = 24

			const float factor = (1u << E) * (1.0f / (1 << offset));
			rgb[0] = half(R * factor);
			rgb[1] = half(G * factor);
			rgb[2] = half(B * factor);
		}
	};

	class R11G11B10F
	{
		unsigned int R : 11;
		unsigned int G : 11;
		unsigned int B : 10;

		static inline half float11ToFloat16(unsigned short fp11)
		{
			return shortAsHalf(fp11 << 4);   // Sign bit 0
		}

		static inline half float10ToFloat16(unsigned short fp10)
		{
			return shortAsHalf(fp10 << 5);   // Sign bit 0
		}

	public:
		void toRGB16F(half rgb[3]) const
		{
			rgb[0] = float11ToFloat16(R);
			rgb[1] = float11ToFloat16(G);
			rgb[2] = float10ToFloat16(B);
		}
	};
}

#endif   // sw_Half_hpp
