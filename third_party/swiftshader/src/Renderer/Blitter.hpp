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

#ifndef sw_Blitter_hpp
#define sw_Blitter_hpp

#include "Surface.hpp"
#include "RoutineCache.hpp"
#include "Reactor/Reactor.hpp"

#include <string.h>

namespace sw
{
	class Blitter
	{
		struct Options
		{
			Options() = default;
			Options(bool filter, bool useStencil, bool convertSRGB)
				: writeMask(0xF), clearOperation(false), filter(filter), useStencil(useStencil), convertSRGB(convertSRGB), clampToEdge(false) {}
			Options(unsigned int writeMask)
				: writeMask(writeMask), clearOperation(true), filter(false), useStencil(false), convertSRGB(true), clampToEdge(false) {}

			union
			{
				struct
				{
					bool writeRed : 1;
					bool writeGreen : 1;
					bool writeBlue : 1;
					bool writeAlpha : 1;
				};

				unsigned char writeMask;
			};

			bool clearOperation : 1;
			bool filter : 1;
			bool useStencil : 1;
			bool convertSRGB : 1;
			bool clampToEdge : 1;
		};

		struct State : Options
		{
			State() = default;
			State(const Options &options) : Options(options) {}

			bool operator==(const State &state) const
			{
				return memcmp(this, &state, sizeof(State)) == 0;
			}

			Format sourceFormat;
			Format destFormat;
			int destSamples;
		};

		struct BlitData
		{
			void *source;
			void *dest;
			int sPitchB;
			int dPitchB;
			int dSliceB;

			float x0;
			float y0;
			float w;
			float h;

			int y0d;
			int y1d;
			int x0d;
			int x1d;

			int sWidth;
			int sHeight;
		};

	public:
		Blitter();
		virtual ~Blitter();

		void clear(void *pixel, sw::Format format, Surface *dest, const SliceRect &dRect, unsigned int rgbaMask);
		void blit(Surface *source, const SliceRectF &sRect, Surface *dest, const SliceRect &dRect, const Options &options);
		void blit3D(Surface *source, Surface *dest);

	private:
		bool fastClear(void *pixel, sw::Format format, Surface *dest, const SliceRect &dRect, unsigned int rgbaMask);

		bool read(Float4 &color, Pointer<Byte> element, const State &state);
		bool write(Float4 &color, Pointer<Byte> element, const State &state);
		bool read(Int4 &color, Pointer<Byte> element, const State &state);
		bool write(Int4 &color, Pointer<Byte> element, const State &state);
		static bool GetScale(float4& scale, Format format);
		static bool ApplyScaleAndClamp(Float4 &value, const State &state, bool preScaled = false);
		static Int ComputeOffset(Int &x, Int &y, Int &pitchB, int bytes, bool quadLayout);
		static Float4 LinearToSRGB(Float4 &color);
		static Float4 sRGBtoLinear(Float4 &color);
		bool blitReactor(Surface *source, const SliceRectF &sRect, Surface *dest, const SliceRect &dRect, const Options &options);
		Routine *generate(const State &state);

		RoutineCache<State> *blitCache;
		MutexLock criticalSection;
	};
}

#endif   // sw_Blitter_hpp
