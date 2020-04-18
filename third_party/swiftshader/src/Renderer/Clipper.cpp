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

#include "Clipper.hpp"

#include "Polygon.hpp"
#include "Renderer.hpp"
#include "Common/Debug.hpp"

namespace sw
{
	Clipper::Clipper(bool symmetricNormalizedDepth)
	{
		n = symmetricNormalizedDepth ? -1.0f : 0.0f;
	}

	Clipper::~Clipper()
	{
	}

	unsigned int Clipper::computeClipFlags(const float4 &v)
	{
		return ((v.x > v.w)     ? CLIP_RIGHT  : 0) |
		       ((v.y > v.w)     ? CLIP_TOP    : 0) |
		       ((v.z > v.w)     ? CLIP_FAR    : 0) |
		       ((v.x < -v.w)    ? CLIP_LEFT   : 0) |
		       ((v.y < -v.w)    ? CLIP_BOTTOM : 0) |
		       ((v.z < n * v.w) ? CLIP_NEAR   : 0) |
		       Clipper::CLIP_FINITE;   // FIXME: xyz finite
	}

	bool Clipper::clip(Polygon &polygon, int clipFlagsOr, const DrawCall &draw)
	{
		if(clipFlagsOr & CLIP_FRUSTUM)
		{
			if(clipFlagsOr & CLIP_NEAR)   clipNear(polygon);
			if(polygon.n >= 3) {
			if(clipFlagsOr & CLIP_FAR)    clipFar(polygon);
			if(polygon.n >= 3) {
			if(clipFlagsOr & CLIP_LEFT)   clipLeft(polygon);
			if(polygon.n >= 3) {
			if(clipFlagsOr & CLIP_RIGHT)  clipRight(polygon);
			if(polygon.n >= 3) {
			if(clipFlagsOr & CLIP_TOP)    clipTop(polygon);
			if(polygon.n >= 3) {
			if(clipFlagsOr & CLIP_BOTTOM) clipBottom(polygon);
			}}}}}
		}

		if(clipFlagsOr & CLIP_USER)
		{
			int clipFlags = draw.clipFlags;
			DrawData &data = *draw.data;

			if(polygon.n >= 3) {
			if(clipFlags & CLIP_PLANE0) clipPlane(polygon, data.clipPlane[0]);
			if(polygon.n >= 3) {
			if(clipFlags & CLIP_PLANE1) clipPlane(polygon, data.clipPlane[1]);
			if(polygon.n >= 3) {
			if(clipFlags & CLIP_PLANE2) clipPlane(polygon, data.clipPlane[2]);
			if(polygon.n >= 3) {
			if(clipFlags & CLIP_PLANE3) clipPlane(polygon, data.clipPlane[3]);
			if(polygon.n >= 3) {
			if(clipFlags & CLIP_PLANE4) clipPlane(polygon, data.clipPlane[4]);
			if(polygon.n >= 3) {
			if(clipFlags & CLIP_PLANE5) clipPlane(polygon, data.clipPlane[5]);
			}}}}}}
		}

		return polygon.n >= 3;
	}

	void Clipper::clipNear(Polygon &polygon)
	{
		const float4 **V = polygon.P[polygon.i];
		const float4 **T = polygon.P[polygon.i + 1];

		int t = 0;

		for(int i = 0; i < polygon.n; i++)
		{
			int j = i == polygon.n - 1 ? 0 : i + 1;

			float di = V[i]->z - n * V[i]->w;
			float dj = V[j]->z - n * V[j]->w;

			if(di >= 0)
			{
				T[t++] = V[i];

				if(dj < 0)
				{
					clipEdge(polygon.B[polygon.b], *V[i], *V[j], di, dj);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
			else
			{
				if(dj > 0)
				{
					clipEdge(polygon.B[polygon.b], *V[j], *V[i], dj, di);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
		}

		polygon.n = t;
		polygon.i += 1;
	}

	void Clipper::clipFar(Polygon &polygon)
	{
		const float4 **V = polygon.P[polygon.i];
		const float4 **T = polygon.P[polygon.i + 1];

		int t = 0;

		for(int i = 0; i < polygon.n; i++)
		{
			int j = i == polygon.n - 1 ? 0 : i + 1;

			float di = V[i]->w - V[i]->z;
			float dj = V[j]->w - V[j]->z;

			if(di >= 0)
			{
				T[t++] = V[i];

				if(dj < 0)
				{
					clipEdge(polygon.B[polygon.b], *V[i], *V[j], di, dj);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
			else
			{
				if(dj > 0)
				{
					clipEdge(polygon.B[polygon.b], *V[j], *V[i], dj, di);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
		}

		polygon.n = t;
		polygon.i += 1;
	}

	void Clipper::clipLeft(Polygon &polygon)
	{
		const float4 **V = polygon.P[polygon.i];
		const float4 **T = polygon.P[polygon.i + 1];

		int t = 0;

		for(int i = 0; i < polygon.n; i++)
		{
			int j = i == polygon.n - 1 ? 0 : i + 1;

			float di = V[i]->w + V[i]->x;
			float dj = V[j]->w + V[j]->x;

			if(di >= 0)
			{
				T[t++] = V[i];

				if(dj < 0)
				{
					clipEdge(polygon.B[polygon.b], *V[i], *V[j], di, dj);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
			else
			{
				if(dj > 0)
				{
					clipEdge(polygon.B[polygon.b], *V[j], *V[i], dj, di);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
		}

		polygon.n = t;
		polygon.i += 1;
	}

	void Clipper::clipRight(Polygon &polygon)
	{
		const float4 **V = polygon.P[polygon.i];
		const float4 **T = polygon.P[polygon.i + 1];

		int t = 0;

		for(int i = 0; i < polygon.n; i++)
		{
			int j = i == polygon.n - 1 ? 0 : i + 1;

			float di = V[i]->w - V[i]->x;
			float dj = V[j]->w - V[j]->x;

			if(di >= 0)
			{
				T[t++] = V[i];

				if(dj < 0)
				{
					clipEdge(polygon.B[polygon.b], *V[i], *V[j], di, dj);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
			else
			{
				if(dj > 0)
				{
					clipEdge(polygon.B[polygon.b], *V[j], *V[i], dj, di);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
		}

		polygon.n = t;
		polygon.i += 1;
	}

	void Clipper::clipTop(Polygon &polygon)
	{
		const float4 **V = polygon.P[polygon.i];
		const float4 **T = polygon.P[polygon.i + 1];

		int t = 0;

		for(int i = 0; i < polygon.n; i++)
		{
			int j = i == polygon.n - 1 ? 0 : i + 1;

			float di = V[i]->w - V[i]->y;
			float dj = V[j]->w - V[j]->y;

			if(di >= 0)
			{
				T[t++] = V[i];

				if(dj < 0)
				{
					clipEdge(polygon.B[polygon.b], *V[i], *V[j], di, dj);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
			else
			{
				if(dj > 0)
				{
					clipEdge(polygon.B[polygon.b], *V[j], *V[i], dj, di);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
		}

		polygon.n = t;
		polygon.i += 1;
	}

	void Clipper::clipBottom(Polygon &polygon)
	{
		const float4 **V = polygon.P[polygon.i];
		const float4 **T = polygon.P[polygon.i + 1];

		int t = 0;

		for(int i = 0; i < polygon.n; i++)
		{
			int j = i == polygon.n - 1 ? 0 : i + 1;

			float di = V[i]->w + V[i]->y;
			float dj = V[j]->w + V[j]->y;

			if(di >= 0)
			{
				T[t++] = V[i];

				if(dj < 0)
				{
					clipEdge(polygon.B[polygon.b], *V[i], *V[j], di, dj);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
			else
			{
				if(dj > 0)
				{
					clipEdge(polygon.B[polygon.b], *V[j], *V[i], dj, di);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
		}

		polygon.n = t;
		polygon.i += 1;
	}

	void Clipper::clipPlane(Polygon &polygon, const Plane &p)
	{
		const float4 **V = polygon.P[polygon.i];
		const float4 **T = polygon.P[polygon.i + 1];

		int t = 0;

		for(int i = 0; i < polygon.n; i++)
		{
			int j = i == polygon.n - 1 ? 0 : i + 1;

			float di = p.A * V[i]->x + p.B * V[i]->y + p.C * V[i]->z + p.D * V[i]->w;
			float dj = p.A * V[j]->x + p.B * V[j]->y + p.C * V[j]->z + p.D * V[j]->w;

			if(di >= 0)
			{
				T[t++] = V[i];

				if(dj < 0)
				{
					clipEdge(polygon.B[polygon.b], *V[i], *V[j], di, dj);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
			else
			{
				if(dj > 0)
				{
					clipEdge(polygon.B[polygon.b], *V[j], *V[i], dj, di);
					T[t++] = &polygon.B[polygon.b++];
				}
			}
		}

		polygon.n = t;
		polygon.i += 1;
	}

	inline void Clipper::clipEdge(float4 &Vo, const float4 &Vi, const float4 &Vj, float di, float dj) const
	{
		float D = 1.0f / (dj - di);

		Vo.x = (dj * Vi.x - di * Vj.x) * D;
		Vo.y = (dj * Vi.y - di * Vj.y) * D;
		Vo.z = (dj * Vi.z - di * Vj.z) * D;
		Vo.w = (dj * Vi.w - di * Vj.w) * D;
	}
}
