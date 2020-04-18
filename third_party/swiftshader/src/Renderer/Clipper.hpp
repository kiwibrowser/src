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

#ifndef sw_Clipper_hpp
#define sw_Clipper_hpp

#include "Plane.hpp"
#include "Common/Types.hpp"

namespace sw
{
	struct Polygon;
	struct DrawCall;
	struct DrawData;

	class Clipper
	{
	public:
		enum ClipFlags
		{
			// Indicates the vertex is outside the respective frustum plane
			CLIP_RIGHT  = 1 << 0,
			CLIP_TOP    = 1 << 1,
			CLIP_FAR    = 1 << 2,
			CLIP_LEFT   = 1 << 3,
			CLIP_BOTTOM = 1 << 4,
			CLIP_NEAR   = 1 << 5,

			CLIP_FRUSTUM = 0x003F,

			CLIP_FINITE = 1 << 7,   // All position coordinates are finite

			// User-defined clipping planes
			CLIP_PLANE0 = 1 << 8,
			CLIP_PLANE1 = 1 << 9,
			CLIP_PLANE2 = 1 << 10,
			CLIP_PLANE3 = 1 << 11,
			CLIP_PLANE4 = 1 << 12,
			CLIP_PLANE5 = 1 << 13,

			CLIP_USER = 0x3F00
		};

		Clipper(bool symmetricNormalizedDepth);

		~Clipper();

		unsigned int computeClipFlags(const float4 &v);
		bool clip(Polygon &polygon, int clipFlagsOr, const DrawCall &draw);

	private:
		void clipNear(Polygon &polygon);
		void clipFar(Polygon &polygon);
		void clipLeft(Polygon &polygon);
		void clipRight(Polygon &polygon);
		void clipTop(Polygon &polygon);
		void clipBottom(Polygon &polygon);
		void clipPlane(Polygon &polygon, const Plane &plane);

		void clipEdge(float4 &Vo, const float4 &Vi, const float4 &Vj, float di, float dj) const;

		float n;   // Near clip plane distance
	};
}

#endif   // sw_Clipper_hpp
