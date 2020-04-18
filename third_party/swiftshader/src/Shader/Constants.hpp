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

#ifndef sw_Constants_hpp
#define sw_Constants_hpp

#include "Common/Types.hpp"

namespace sw
{
	struct Constants
	{
		Constants();

		unsigned int transposeBit0[16];
		unsigned int transposeBit1[16];
		unsigned int transposeBit2[16];

		ushort4 cWeight[17];
		float4 uvWeight[17];
		float4 uvStart[17];

		unsigned int occlusionCount[16];

		byte8 maskB4Q[16];
		byte8 invMaskB4Q[16];
		word4 maskW4Q[16];
		word4 invMaskW4Q[16];
		dword4 maskD4X[16];
		dword4 invMaskD4X[16];
		qword maskQ0Q[16];
		qword maskQ1Q[16];
		qword maskQ2Q[16];
		qword maskQ3Q[16];
		qword invMaskQ0Q[16];
		qword invMaskQ1Q[16];
		qword invMaskQ2Q[16];
		qword invMaskQ3Q[16];
		dword4 maskX0X[16];
		dword4 maskX1X[16];
		dword4 maskX2X[16];
		dword4 maskX3X[16];
		dword4 invMaskX0X[16];
		dword4 invMaskX1X[16];
		dword4 invMaskX2X[16];
		dword4 invMaskX3X[16];
		dword2 maskD01Q[16];
		dword2 maskD23Q[16];
		dword2 invMaskD01Q[16];
		dword2 invMaskD23Q[16];
		qword2 maskQ01X[16];
		qword2 maskQ23X[16];
		qword2 invMaskQ01X[16];
		qword2 invMaskQ23X[16];
		word4 maskW01Q[4];
		dword4 maskD01X[4];
		word4 mask565Q[8];

		unsigned short sRGBtoLinear8_16[256];
		unsigned short sRGBtoLinear6_16[64];
		unsigned short sRGBtoLinear5_16[32];

		unsigned short linearToSRGB12_16[4096];
		unsigned short sRGBtoLinear12_16[4096];

		// Centroid parameters
		float4 sampleX[4][16];
		float4 sampleY[4][16];
		float4 weight[16];

		// Fragment offsets
		int Xf[4];
		int Yf[4];

		float4 X[4];
		float4 Y[4];

		dword maxX[16];
		dword maxY[16];
		dword maxZ[16];
		dword minX[16];
		dword minY[16];
		dword minZ[16];
		dword fini[16];

		dword4 maxPos;

		float4 unscaleByte;
		float4 unscaleSByte;
		float4 unscaleShort;
		float4 unscaleUShort;
		float4 unscaleInt;
		float4 unscaleUInt;
		float4 unscaleFixed;

		float half2float[65536];
	};

	extern Constants constants;
}

#endif   // sw_Constants_hpp
