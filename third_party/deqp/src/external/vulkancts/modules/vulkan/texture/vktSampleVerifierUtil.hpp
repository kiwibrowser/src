#ifndef _VKTSAMPLEVERIFIERUTIL_HPP
#define _VKTSAMPLEVERIFIERUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief GPU image sample verification
 *//*--------------------------------------------------------------------*/

#include "vktSampleVerifier.hpp"

#include "deMath.h"
#include "tcuFloatFormat.hpp"
#include "tcuTexture.hpp"
#include "vkDefs.hpp"

namespace vkt
{
namespace texture
{
namespace util
{

float		addUlp						(float							num,
										 deInt32						ulp);

deInt32		mod							(const deInt32					a,
										 const deInt32					n);
deInt32	    mirror						(const deInt32					n);

tcu::Vec2	calcLodBounds				(const tcu::Vec3&				dPdx,
										 const tcu::Vec3&				dPdy,
										 const tcu::IVec3				size,
										 const float					lodBias,
										 const float					lodMin,
										 const float					lodMax);
tcu::UVec2	calcLevelBounds				(const tcu::Vec2&				lodBounds,
										 const int						levelCount,
										 vk::VkSamplerMipmapMode		mipmapFilter);
tcu::Vec2	calcLevelLodBounds			(const tcu::Vec2&				lodBounds,
										 int							level);

void		wrapTexelGridCoordLinear	(tcu::IVec3&					baseTexel,
										 tcu::IVec3&					texelGridOffset,
										 const int						coordBits,
										 const ImgDim					dim);
void		calcTexelBaseOffset			(const tcu::IVec3&				gridCoord,
										 const int						coordBits,
										 tcu::IVec3&					baseTexel,
										 tcu::IVec3&					texelGridOffset);
void		calcTexelGridCoordRange		(const tcu::Vec3&				unnormalizedCoordMin,
										 const tcu::Vec3&				unnormalizedCoordMax,
										 const int						coordBits,
										 tcu::IVec3&					gridCoordMin,
										 tcu::IVec3&					gridCoordMax);
void		calcUnnormalizedCoordRange	(const tcu::Vec4&				coord,
										 const tcu::IVec3&				levelSize,
										 const tcu::FloatFormat&		internalFormat,
										 tcu::Vec3&						unnormalizedCoordMin,
										 tcu::Vec3&						unnormalizedCoordMax);
void		calcCubemapFaceCoords		(const tcu::Vec3&				r,
										 const tcu::Vec3&				drdx,
										 const tcu::Vec3&				drdy,
										 const int						faceNdx,
										 tcu::Vec2&						coordFace,
										 tcu::Vec2&						dPdxFace,
										 tcu::Vec2&						dPdyFace);
int			calcCandidateCubemapFaces	(const tcu::Vec3&				r);
deInt32		wrapTexelCoord				(const deInt32					coord,
										 const int						size,
										 const vk::VkSamplerAddressMode wrap);
void		wrapCubemapEdge				(const tcu::IVec2&				coord,
										 const tcu::IVec2&				size,
										 const int						faceNdx,
										 tcu::IVec2&					newCoord,
										 int&							newFaceNdx);
void		wrapCubemapCorner			(const tcu::IVec2&				coord,
										 const tcu::IVec2&				size,
										 const int						faceNdx,
										 int&							adjacentFace1,
										 int&							adjacentFace2,
										 tcu::IVec2&					cornerCoord0,
										 tcu::IVec2&					cornerCoord1,
										 tcu::IVec2&					cornerCoord2);

void		convertFormat				(const void*					pixelPtr,
										 tcu::TextureFormat				texFormat,
										 tcu::FloatFormat				internalFormat,
										 tcu::Vec4&						resultMin,
										 tcu::Vec4&						resultMax);

template <int Size>
bool isEqualRelEpsilon (const tcu::Vector<float, Size>& a, const tcu::Vector<float, Size>& b, const float epsilon)
{
	for (int compNdx = 0; compNdx < Size; ++compNdx)
	{
		if (!isEqualRelEpsilon(a[compNdx], b[compNdx], epsilon))
		{
			return false;
		}
	}

	return true;
}

template <int Size>
bool isInRange (const tcu::Vector<float, Size>& v, const tcu::Vector<float, Size>& min, const tcu::Vector<float, Size>& max)
{
	for (int compNdx = 0; compNdx < Size; ++compNdx)
	{
		if (v[compNdx] < min[compNdx] || v[compNdx] > max[compNdx])
		{
			return false;
		}
	}

	return true;
}

template <int Size>
tcu::Vector<float, Size> floor (const tcu::Vector<float, Size>& v)
{
	tcu::Vector<float, Size> result;

	for (int compNdx = 0; compNdx < Size; ++compNdx)
	{
		result[compNdx] = (float)deFloor(v[compNdx]);
	}

	return result;
}

template <int Size>
tcu::Vector<float, Size> ceil (const tcu::Vector<float, Size>& v)
{
	tcu::Vector<float, Size> result;

	for (int compNdx = 0; compNdx < Size; ++compNdx)
	{
		result[compNdx] = (float)deCeil(v[compNdx]);
	}

	return result;
}

template <int Size>
tcu::Vector<float, Size> abs (const tcu::Vector<float, Size>& v)
{
	tcu::Vector<float, Size> result;

	for (int compNdx = 0; compNdx < Size; ++compNdx)
	{
		result[compNdx] = de::abs(v[compNdx]);
	}

	return result;
}

} // util
} // texture
} // vkt

#endif // _VKTSAMPLEVERIFIERUTIL_HPP
