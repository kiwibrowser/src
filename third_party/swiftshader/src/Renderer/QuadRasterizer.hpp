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

#ifndef sw_QuadRasterizer_hpp
#define sw_QuadRasterizer_hpp

#include "Rasterizer.hpp"
#include "Shader/ShaderCore.hpp"
#include "Shader/PixelShader.hpp"
#include "Common/Types.hpp"

namespace sw
{
	class QuadRasterizer : public Rasterizer
	{
	public:
		QuadRasterizer(const PixelProcessor::State &state, const PixelShader *shader);
		virtual ~QuadRasterizer();

		void generate();

	protected:
		Pointer<Byte> constants;

		Float4 Dz[4];
		Float4 Dw;
		Float4 Dv[MAX_FRAGMENT_INPUTS][4];
		Float4 Df;

		UInt occlusion;

#if PERF_PROFILE
		Long cycles[PERF_TIMERS];
#endif

		virtual void quad(Pointer<Byte> cBuffer[4], Pointer<Byte> &zBuffer, Pointer<Byte> &sBuffer, Int cMask[4], Int &x, Int &y) = 0;

		bool interpolateZ() const;
		bool interpolateW() const;
		Float4 interpolate(Float4 &x, Float4 &D, Float4 &rhw, Pointer<Byte> planeEquation, bool flat, bool perspective, bool clamp);

		const PixelProcessor::State &state;
		const PixelShader *const shader;

	private:
		void rasterize(Int &yMin, Int &yMax);
	};
}

#endif   // sw_QuadRasterizer_hpp
