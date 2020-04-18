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

#include "Constants.hpp"

#include "Common/Math.hpp"
#include "Common/Half.hpp"

#include <memory.h>

namespace sw
{
	Constants constants;

	Constants::Constants()
	{
		static const unsigned int transposeBit0[16] =
		{
			0x00000000,
			0x00000001,
			0x00000010,
			0x00000011,
			0x00000100,
			0x00000101,
			0x00000110,
			0x00000111,
			0x00001000,
			0x00001001,
			0x00001010,
			0x00001011,
			0x00001100,
			0x00001101,
			0x00001110,
			0x00001111
		};

		static const unsigned int transposeBit1[16] =
		{
			0x00000000,
			0x00000002,
			0x00000020,
			0x00000022,
			0x00000200,
			0x00000202,
			0x00000220,
			0x00000222,
			0x00002000,
			0x00002002,
			0x00002020,
			0x00002022,
			0x00002200,
			0x00002202,
			0x00002220,
			0x00002222
		};

		static const unsigned int transposeBit2[16] =
		{
			0x00000000,
			0x00000004,
			0x00000040,
			0x00000044,
			0x00000400,
			0x00000404,
			0x00000440,
			0x00000444,
			0x00004000,
			0x00004004,
			0x00004040,
			0x00004044,
			0x00004400,
			0x00004404,
			0x00004440,
			0x00004444
		};

		memcpy(&this->transposeBit0, transposeBit0, sizeof(transposeBit0));
		memcpy(&this->transposeBit1, transposeBit1, sizeof(transposeBit1));
		memcpy(&this->transposeBit2, transposeBit2, sizeof(transposeBit2));

		static const ushort4 cWeight[17] =
		{
			{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},   // 0xFFFF / 1  = 0xFFFF
			{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},   // 0xFFFF / 1  = 0xFFFF
			{0x8000, 0x8000, 0x8000, 0x8000},   // 0xFFFF / 2  = 0x8000
			{0x5555, 0x5555, 0x5555, 0x5555},   // 0xFFFF / 3  = 0x5555
			{0x4000, 0x4000, 0x4000, 0x4000},   // 0xFFFF / 4  = 0x4000
			{0x3333, 0x3333, 0x3333, 0x3333},   // 0xFFFF / 5  = 0x3333
			{0x2AAA, 0x2AAA, 0x2AAA, 0x2AAA},   // 0xFFFF / 6  = 0x2AAA
			{0x2492, 0x2492, 0x2492, 0x2492},   // 0xFFFF / 7  = 0x2492
			{0x2000, 0x2000, 0x2000, 0x2000},   // 0xFFFF / 8  = 0x2000
			{0x1C71, 0x1C71, 0x1C71, 0x1C71},   // 0xFFFF / 9  = 0x1C71
			{0x1999, 0x1999, 0x1999, 0x1999},   // 0xFFFF / 10 = 0x1999
			{0x1745, 0x1745, 0x1745, 0x1745},   // 0xFFFF / 11 = 0x1745
			{0x1555, 0x1555, 0x1555, 0x1555},   // 0xFFFF / 12 = 0x1555
			{0x13B1, 0x13B1, 0x13B1, 0x13B1},   // 0xFFFF / 13 = 0x13B1
			{0x1249, 0x1249, 0x1249, 0x1249},   // 0xFFFF / 14 = 0x1249
			{0x1111, 0x1111, 0x1111, 0x1111},   // 0xFFFF / 15 = 0x1111
			{0x1000, 0x1000, 0x1000, 0x1000},   // 0xFFFF / 16 = 0x1000
		};

		static const float4 uvWeight[17] =
		{
			{1.0f / 1.0f,  1.0f / 1.0f,  1.0f / 1.0f,  1.0f / 1.0f},
			{1.0f / 1.0f,  1.0f / 1.0f,  1.0f / 1.0f,  1.0f / 1.0f},
			{1.0f / 2.0f,  1.0f / 2.0f,  1.0f / 2.0f,  1.0f / 2.0f},
			{1.0f / 3.0f,  1.0f / 3.0f,  1.0f / 3.0f,  1.0f / 3.0f},
			{1.0f / 4.0f,  1.0f / 4.0f,  1.0f / 4.0f,  1.0f / 4.0f},
			{1.0f / 5.0f,  1.0f / 5.0f,  1.0f / 5.0f,  1.0f / 5.0f},
			{1.0f / 6.0f,  1.0f / 6.0f,  1.0f / 6.0f,  1.0f / 6.0f},
			{1.0f / 7.0f,  1.0f / 7.0f,  1.0f / 7.0f,  1.0f / 7.0f},
			{1.0f / 8.0f,  1.0f / 8.0f,  1.0f / 8.0f,  1.0f / 8.0f},
			{1.0f / 9.0f,  1.0f / 9.0f,  1.0f / 9.0f,  1.0f / 9.0f},
			{1.0f / 10.0f, 1.0f / 10.0f, 1.0f / 10.0f, 1.0f / 10.0f},
			{1.0f / 11.0f, 1.0f / 11.0f, 1.0f / 11.0f, 1.0f / 11.0f},
			{1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f},
			{1.0f / 13.0f, 1.0f / 13.0f, 1.0f / 13.0f, 1.0f / 13.0f},
			{1.0f / 14.0f, 1.0f / 14.0f, 1.0f / 14.0f, 1.0f / 14.0f},
			{1.0f / 15.0f, 1.0f / 15.0f, 1.0f / 15.0f, 1.0f / 15.0f},
			{1.0f / 16.0f, 1.0f / 16.0f, 1.0f / 16.0f, 1.0f / 16.0f},
		};

		static const float4 uvStart[17] =
		{
			{-0.0f / 2.0f,   -0.0f / 2.0f,   -0.0f / 2.0f,   -0.0f / 2.0f},
			{-0.0f / 2.0f,   -0.0f / 2.0f,   -0.0f / 2.0f,   -0.0f / 2.0f},
			{-1.0f / 4.0f,   -1.0f / 4.0f,   -1.0f / 4.0f,   -1.0f / 4.0f},
			{-2.0f / 6.0f,   -2.0f / 6.0f,   -2.0f / 6.0f,   -2.0f / 6.0f},
			{-3.0f / 8.0f,   -3.0f / 8.0f,   -3.0f / 8.0f,   -3.0f / 8.0f},
			{-4.0f / 10.0f,  -4.0f / 10.0f,  -4.0f / 10.0f,  -4.0f / 10.0f},
			{-5.0f / 12.0f,  -5.0f / 12.0f,  -5.0f / 12.0f,  -5.0f / 12.0f},
			{-6.0f / 14.0f,  -6.0f / 14.0f,  -6.0f / 14.0f,  -6.0f / 14.0f},
			{-7.0f / 16.0f,  -7.0f / 16.0f,  -7.0f / 16.0f,  -7.0f / 16.0f},
			{-8.0f / 18.0f,  -8.0f / 18.0f,  -8.0f / 18.0f,  -8.0f / 18.0f},
			{-9.0f / 20.0f,  -9.0f / 20.0f,  -9.0f / 20.0f,  -9.0f / 20.0f},
			{-10.0f / 22.0f, -10.0f / 22.0f, -10.0f / 22.0f, -10.0f / 22.0f},
			{-11.0f / 24.0f, -11.0f / 24.0f, -11.0f / 24.0f, -11.0f / 24.0f},
			{-12.0f / 26.0f, -12.0f / 26.0f, -12.0f / 26.0f, -12.0f / 26.0f},
			{-13.0f / 28.0f, -13.0f / 28.0f, -13.0f / 28.0f, -13.0f / 28.0f},
			{-14.0f / 30.0f, -14.0f / 30.0f, -14.0f / 30.0f, -14.0f / 30.0f},
			{-15.0f / 32.0f, -15.0f / 32.0f, -15.0f / 32.0f, -15.0f / 32.0f},
		};

		memcpy(&this->cWeight, cWeight, sizeof(cWeight));
		memcpy(&this->uvWeight, uvWeight, sizeof(uvWeight));
		memcpy(&this->uvStart, uvStart, sizeof(uvStart));

		static const unsigned int occlusionCount[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

		memcpy(&this->occlusionCount, &occlusionCount, sizeof(occlusionCount));

		for(int i = 0; i < 16; i++)
		{
			maskB4Q[i][0] = -(i >> 0 & 1);
			maskB4Q[i][1] = -(i >> 1 & 1);
			maskB4Q[i][2] = -(i >> 2 & 1);
			maskB4Q[i][3] = -(i >> 3 & 1);
			maskB4Q[i][4] = -(i >> 0 & 1);
			maskB4Q[i][5] = -(i >> 1 & 1);
			maskB4Q[i][6] = -(i >> 2 & 1);
			maskB4Q[i][7] = -(i >> 3 & 1);

			invMaskB4Q[i][0] = ~maskB4Q[i][0];
			invMaskB4Q[i][1] = ~maskB4Q[i][1];
			invMaskB4Q[i][2] = ~maskB4Q[i][2];
			invMaskB4Q[i][3] = ~maskB4Q[i][3];
			invMaskB4Q[i][4] = ~maskB4Q[i][4];
			invMaskB4Q[i][5] = ~maskB4Q[i][5];
			invMaskB4Q[i][6] = ~maskB4Q[i][6];
			invMaskB4Q[i][7] = ~maskB4Q[i][7];

			maskW4Q[i][0] = -(i >> 0 & 1);
			maskW4Q[i][1] = -(i >> 1 & 1);
			maskW4Q[i][2] = -(i >> 2 & 1);
			maskW4Q[i][3] = -(i >> 3 & 1);

			invMaskW4Q[i][0] = ~maskW4Q[i][0];
			invMaskW4Q[i][1] = ~maskW4Q[i][1];
			invMaskW4Q[i][2] = ~maskW4Q[i][2];
			invMaskW4Q[i][3] = ~maskW4Q[i][3];

			maskD4X[i][0] = -(i >> 0 & 1);
			maskD4X[i][1] = -(i >> 1 & 1);
			maskD4X[i][2] = -(i >> 2 & 1);
			maskD4X[i][3] = -(i >> 3 & 1);

			invMaskD4X[i][0] = ~maskD4X[i][0];
			invMaskD4X[i][1] = ~maskD4X[i][1];
			invMaskD4X[i][2] = ~maskD4X[i][2];
			invMaskD4X[i][3] = ~maskD4X[i][3];

			maskQ0Q[i] = -(i >> 0 & 1);
			maskQ1Q[i] = -(i >> 1 & 1);
			maskQ2Q[i] = -(i >> 2 & 1);
			maskQ3Q[i] = -(i >> 3 & 1);

			invMaskQ0Q[i] = ~maskQ0Q[i];
			invMaskQ1Q[i] = ~maskQ1Q[i];
			invMaskQ2Q[i] = ~maskQ2Q[i];
			invMaskQ3Q[i] = ~maskQ3Q[i];

			maskX0X[i][0] = maskX0X[i][1] = maskX0X[i][2] = maskX0X[i][3] = -(i >> 0 & 1);
			maskX1X[i][0] = maskX1X[i][1] = maskX1X[i][2] = maskX1X[i][3] = -(i >> 1 & 1);
			maskX2X[i][0] = maskX2X[i][1] = maskX2X[i][2] = maskX2X[i][3] = -(i >> 2 & 1);
			maskX3X[i][0] = maskX3X[i][1] = maskX3X[i][2] = maskX3X[i][3] = -(i >> 3 & 1);

			invMaskX0X[i][0] = invMaskX0X[i][1] = invMaskX0X[i][2] = invMaskX0X[i][3] = ~maskX0X[i][0];
			invMaskX1X[i][0] = invMaskX1X[i][1] = invMaskX1X[i][2] = invMaskX1X[i][3] = ~maskX1X[i][0];
			invMaskX2X[i][0] = invMaskX2X[i][1] = invMaskX2X[i][2] = invMaskX2X[i][3] = ~maskX2X[i][0];
			invMaskX3X[i][0] = invMaskX3X[i][1] = invMaskX3X[i][2] = invMaskX3X[i][3] = ~maskX3X[i][0];

			maskD01Q[i][0] = -(i >> 0 & 1);
			maskD01Q[i][1] = -(i >> 1 & 1);
			maskD23Q[i][0] = -(i >> 2 & 1);
			maskD23Q[i][1] = -(i >> 3 & 1);

			invMaskD01Q[i][0] = ~maskD01Q[i][0];
			invMaskD01Q[i][1] = ~maskD01Q[i][1];
			invMaskD23Q[i][0] = ~maskD23Q[i][0];
			invMaskD23Q[i][1] = ~maskD23Q[i][1];

			maskQ01X[i][0] = -(i >> 0 & 1);
			maskQ01X[i][1] = -(i >> 1 & 1);
			maskQ23X[i][0] = -(i >> 2 & 1);
			maskQ23X[i][1] = -(i >> 3 & 1);

			invMaskQ01X[i][0] = ~maskQ01X[i][0];
			invMaskQ01X[i][1] = ~maskQ01X[i][1];
			invMaskQ23X[i][0] = ~maskQ23X[i][0];
			invMaskQ23X[i][1] = ~maskQ23X[i][1];
		}

		for(int i = 0; i < 8; i++)
		{
			mask565Q[i][0] =
			mask565Q[i][1] =
			mask565Q[i][2] =
			mask565Q[i][3] = (i & 0x1 ? 0x001F : 0) | (i & 0x2 ? 0x07E0 : 0) | (i & 0x4 ? 0xF800 : 0);
		}

		for(int i = 0; i < 4; i++)
		{
			maskW01Q[i][0] =  -(i >> 0 & 1);
			maskW01Q[i][1] =  -(i >> 1 & 1);
			maskW01Q[i][2] =  -(i >> 0 & 1);
			maskW01Q[i][3] =  -(i >> 1 & 1);

			maskD01X[i][0] =  -(i >> 0 & 1);
			maskD01X[i][1] =  -(i >> 1 & 1);
			maskD01X[i][2] =  -(i >> 0 & 1);
			maskD01X[i][3] =  -(i >> 1 & 1);
		}

		for(int i = 0; i < 256; i++)
		{
			sRGBtoLinear8_16[i] = (unsigned short)(sw::sRGBtoLinear((float)i / 0xFF) * 0xFFFF + 0.5f);
		}

		for(int i = 0; i < 64; i++)
		{
			sRGBtoLinear6_16[i] = (unsigned short)(sw::sRGBtoLinear((float)i / 0x3F) * 0xFFFF + 0.5f);
		}

		for(int i = 0; i < 32; i++)
		{
			sRGBtoLinear5_16[i] = (unsigned short)(sw::sRGBtoLinear((float)i / 0x1F) * 0xFFFF + 0.5f);
		}

		for(int i = 0; i < 0x1000; i++)
		{
			linearToSRGB12_16[i] = (unsigned short)(clamp(sw::linearToSRGB((float)i / 0x0FFF) * 0xFFFF + 0.5f, 0.0f, (float)0xFFFF));
			sRGBtoLinear12_16[i] = (unsigned short)(clamp(sw::sRGBtoLinear((float)i / 0x0FFF) * 0xFFFF + 0.5f, 0.0f, (float)0xFFFF));
		}

		for(int q = 0; q < 4; q++)
		{
			for(int c = 0; c < 16; c++)
			{
				for(int i = 0; i < 4; i++)
				{
					const float X[4] = {+0.3125f, -0.3125f, -0.1250f, +0.1250f};
					const float Y[4] = {+0.1250f, -0.1250f, +0.3125f, -0.3125f};

					sampleX[q][c][i] = c & (1 << i) ? X[q] : 0.0f;
					sampleY[q][c][i] = c & (1 << i) ? Y[q] : 0.0f;
					weight[c][i] = c & (1 << i) ? 1.0f : 0.0f;
				}
			}
		}

		const int Xf[4] = {-5, +5, +2, -2};   // Fragment offsets
		const int Yf[4] = {-2, +2, -5, +5};   // Fragment offsets

		memcpy(&this->Xf, &Xf, sizeof(Xf));
		memcpy(&this->Yf, &Yf, sizeof(Yf));

		static const float4 X[4] = {{-0.3125f, -0.3125f, -0.3125f, -0.3125f},
					                {+0.3125f, +0.3125f, +0.3125f, +0.3125f},
					                {+0.1250f, +0.1250f, +0.1250f, +0.1250f},
					                {-0.1250f, -0.1250f, -0.1250f, -0.1250f}};

		static const float4 Y[4] = {{-0.1250f, -0.1250f, -0.1250f, -0.1250f},
		                            {+0.1250f, +0.1250f, +0.1250f, +0.1250f},
		                            {-0.3125f, -0.3125f, -0.3125f, -0.3125f},
		                            {+0.3125f, +0.3125f, +0.3125f, +0.3125f}};

		memcpy(&this->X, &X, sizeof(X));
		memcpy(&this->Y, &Y, sizeof(Y));

		const dword maxX[16] = {0x00000000, 0x00000001, 0x00000100, 0x00000101, 0x00010000, 0x00010001, 0x00010100, 0x00010101, 0x01000000, 0x01000001, 0x01000100, 0x01000101, 0x01010000, 0x01010001, 0x01010100, 0x01010101};
		const dword maxY[16] = {0x00000000, 0x00000002, 0x00000200, 0x00000202, 0x00020000, 0x00020002, 0x00020200, 0x00020202, 0x02000000, 0x02000002, 0x02000200, 0x02000202, 0x02020000, 0x02020002, 0x02020200, 0x02020202};
		const dword maxZ[16] = {0x00000000, 0x00000004, 0x00000400, 0x00000404, 0x00040000, 0x00040004, 0x00040400, 0x00040404, 0x04000000, 0x04000004, 0x04000400, 0x04000404, 0x04040000, 0x04040004, 0x04040400, 0x04040404};
		const dword minX[16] = {0x00000000, 0x00000008, 0x00000800, 0x00000808, 0x00080000, 0x00080008, 0x00080800, 0x00080808, 0x08000000, 0x08000008, 0x08000800, 0x08000808, 0x08080000, 0x08080008, 0x08080800, 0x08080808};
		const dword minY[16] = {0x00000000, 0x00000010, 0x00001000, 0x00001010, 0x00100000, 0x00100010, 0x00101000, 0x00101010, 0x10000000, 0x10000010, 0x10001000, 0x10001010, 0x10100000, 0x10100010, 0x10101000, 0x10101010};
		const dword minZ[16] = {0x00000000, 0x00000020, 0x00002000, 0x00002020, 0x00200000, 0x00200020, 0x00202000, 0x00202020, 0x20000000, 0x20000020, 0x20002000, 0x20002020, 0x20200000, 0x20200020, 0x20202000, 0x20202020};
		const dword fini[16] = {0x00000000, 0x00000080, 0x00008000, 0x00008080, 0x00800000, 0x00800080, 0x00808000, 0x00808080, 0x80000000, 0x80000080, 0x80008000, 0x80008080, 0x80800000, 0x80800080, 0x80808000, 0x80808080};

		memcpy(&this->maxX, &maxX, sizeof(maxX));
		memcpy(&this->maxY, &maxY, sizeof(maxY));
		memcpy(&this->maxZ, &maxZ, sizeof(maxZ));
		memcpy(&this->minX, &minX, sizeof(minX));
		memcpy(&this->minY, &minY, sizeof(minY));
		memcpy(&this->minZ, &minZ, sizeof(minZ));
		memcpy(&this->fini, &fini, sizeof(fini));

		static const dword4 maxPos = {0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFE};

		memcpy(&this->maxPos, &maxPos, sizeof(maxPos));

		static const float4 unscaleByte = {1.0f / 0xFF, 1.0f / 0xFF, 1.0f / 0xFF, 1.0f / 0xFF};
		static const float4 unscaleSByte = {1.0f / 0x7F, 1.0f / 0x7F, 1.0f / 0x7F, 1.0f / 0x7F};
		static const float4 unscaleShort = {1.0f / 0x7FFF, 1.0f / 0x7FFF, 1.0f / 0x7FFF, 1.0f / 0x7FFF};
		static const float4 unscaleUShort = {1.0f / 0xFFFF, 1.0f / 0xFFFF, 1.0f / 0xFFFF, 1.0f / 0xFFFF};
		static const float4 unscaleInt = {1.0f / 0x7FFFFFFF, 1.0f / 0x7FFFFFFF, 1.0f / 0x7FFFFFFF, 1.0f / 0x7FFFFFFF};
		static const float4 unscaleUInt = {1.0f / 0xFFFFFFFF, 1.0f / 0xFFFFFFFF, 1.0f / 0xFFFFFFFF, 1.0f / 0xFFFFFFFF};
		static const float4 unscaleFixed = {1.0f / 0x00010000, 1.0f / 0x00010000, 1.0f / 0x00010000, 1.0f / 0x00010000};

		memcpy(&this->unscaleByte, &unscaleByte, sizeof(unscaleByte));
		memcpy(&this->unscaleSByte, &unscaleSByte, sizeof(unscaleSByte));
		memcpy(&this->unscaleShort, &unscaleShort, sizeof(unscaleShort));
		memcpy(&this->unscaleUShort, &unscaleUShort, sizeof(unscaleUShort));
		memcpy(&this->unscaleInt, &unscaleInt, sizeof(unscaleInt));
		memcpy(&this->unscaleUInt, &unscaleUInt, sizeof(unscaleUInt));
		memcpy(&this->unscaleFixed, &unscaleFixed, sizeof(unscaleFixed));

		for(int i = 0; i <= 0xFFFF; i++)
		{
			half2float[i] = (float)reinterpret_cast<half&>(i);
		}
	}
}