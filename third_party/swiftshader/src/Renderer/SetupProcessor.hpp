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

#ifndef sw_SetupProcessor_hpp
#define sw_SetupProcessor_hpp

#include "Context.hpp"
#include "RoutineCache.hpp"
#include "Shader/VertexShader.hpp"
#include "Shader/PixelShader.hpp"
#include "Common/Types.hpp"

namespace sw
{
	struct Primitive;
	struct Triangle;
	struct Polygon;
	struct Vertex;
	struct DrawCall;
	struct DrawData;

	class SetupProcessor
	{
	public:
		struct States
		{
			unsigned int computeHash();

			bool isDrawPoint               : 1;
			bool isDrawLine                : 1;
			bool isDrawTriangle            : 1;
			bool isDrawSolidTriangle       : 1;
			bool interpolateZ              : 1;
			bool interpolateW              : 1;
			bool perspective               : 1;
			bool pointSprite               : 1;
			unsigned int positionRegister  : BITS(VERTEX_OUTPUT_LAST);
			unsigned int pointSizeRegister : BITS(VERTEX_OUTPUT_LAST);
			CullMode cullMode              : BITS(CULL_LAST);
			bool twoSidedStencil           : 1;
			bool slopeDepthBias            : 1;
			bool vFace                     : 1;
			unsigned int multiSample       : 3;   // 1, 2 or 4
			bool rasterizerDiscard         : 1;

			struct Gradient
			{
				unsigned char attribute : BITS(VERTEX_OUTPUT_LAST);
				bool flat               : 1;
				bool wrap               : 1;
			};

			union
			{
				struct
				{
					Gradient color[2][4];
					Gradient texture[8][4];
					Gradient fog;
				};

				Gradient gradient[MAX_FRAGMENT_INPUTS][4];
			};
		};

		struct State : States
		{
			State(int i = 0);

			bool operator==(const State &states) const;

			unsigned int hash;
		};

		typedef bool (*RoutinePointer)(Primitive *primitive, const Triangle *triangle, const Polygon *polygon, const DrawData *draw);

		SetupProcessor(Context *context);

		~SetupProcessor();

	protected:
		State update() const;
		Routine *routine(const State &state);

		void setRoutineCacheSize(int cacheSize);

	private:
		Context *const context;

		RoutineCache<State> *routineCache;
	};
}

#endif   // sw_SetupProcessor_hpp
