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

#include "vktSampleVerifierUtil.hpp"

#include "deMath.h"
#include "tcuDefs.hpp"
#include "tcuFloat.hpp"
#include "tcuFloatFormat.hpp"
#include "tcuInterval.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"

namespace vkt
{
namespace texture
{
namespace util
{

using namespace tcu;
using namespace vk;

deInt32 mod (const deInt32 a, const deInt32 n)
{
	const deInt32 result = a % n;

	return (result < 0) ? result + n : result;
}

deInt32 mirror (const deInt32 n)
{
	if (n >= 0)
	{
		return n;
	}
	else
	{
		return -(1 + n);
	}
}

UVec2 calcLevelBounds (const Vec2&			lodBounds,
					   const int			levelCount,
					   VkSamplerMipmapMode	mipmapFilter)
{
	DE_ASSERT(lodBounds[0] <= lodBounds[1]);
	DE_ASSERT(levelCount > 0);

	const float q = (float) (levelCount - 1);

	UVec2 levelBounds;

	if (mipmapFilter == VK_SAMPLER_MIPMAP_MODE_NEAREST)
	{
		if (lodBounds[0] <= 0.5f)
		{
			levelBounds[0] = 0;
		}
		else if (lodBounds[0] < q + 0.5f)
		{
			levelBounds[0] = deCeilFloatToInt32(lodBounds[0] + 0.5f) - 1;
		}
		else
		{
			levelBounds[0] = deRoundFloatToInt32(q);
		}

		if (lodBounds[1] < 0.5f)
		{
			levelBounds[1] = 0;
		}
		else if (lodBounds[1] < q + 0.5f)
		{
			levelBounds[1] = deFloorFloatToInt32(lodBounds[1] + 0.5f);
		}
		else
		{
			levelBounds[1] = deRoundFloatToInt32(q);
		}
	}
	else
	{
		for (int ndx = 0; ndx < 2; ++ndx)
		{
			if (lodBounds[ndx] >= q)
			{
				levelBounds[ndx] = deRoundFloatToInt32(q);
			}
			else
			{
				levelBounds[ndx] = lodBounds[ndx] < 0.0f ? 0 : deFloorFloatToInt32(lodBounds[ndx]);
			}
		}
	}

	return levelBounds;
}

Vec2 calcLevelLodBounds (const Vec2& lodBounds, int level)
{
	Vec2 levelLodBounds;

	if (lodBounds[0] <= 0.0f)
	{
		levelLodBounds[0] = lodBounds[0];
	}
	else
	{
		levelLodBounds[0] = de::max(lodBounds[0], (float) level);
	}

	levelLodBounds[1] = de::min(lodBounds[1], (float) level + 1.0f);

	return levelLodBounds;
}

float addUlp (float num, deInt32 ulp)
{
	// Note: adding positive ulp always moves float away from zero

	const tcu::Float32 f(num);

	DE_ASSERT(!f.isNaN() && !f.isInf());
	DE_ASSERT(num > FLT_MIN * (float) ulp || num < FLT_MIN * (float) ulp);

	return tcu::Float32(f.bits() + ulp).asFloat();
}

void wrapTexelGridCoordLinear (IVec3&		baseTexel,
							   IVec3&		texelGridOffset,
							   const int	coordBits,
							   const ImgDim dim)
{
	const int subdivisions = 1 << coordBits;

	int numComp;

	switch (dim)
	{
		case IMG_DIM_1D:
			numComp = 1;
			break;

		case IMG_DIM_2D:
			numComp = 2;
			break;

		case IMG_DIM_CUBE:
			numComp = 2;
			break;

		case IMG_DIM_3D:
			numComp = 3;
			break;

		default:
			numComp = 0;
			break;
	}

	for (int compNdx = 0; compNdx < numComp; ++compNdx)
	{
		texelGridOffset[compNdx] -= subdivisions / (int) 2;

		if (texelGridOffset[compNdx] < 0)
		{
			baseTexel      [compNdx] -= 1;
			texelGridOffset[compNdx] += (deInt32) subdivisions;
		}
	}
}

void calcTexelBaseOffset (const IVec3&	gridCoord,
						  const int		coordBits,
						  IVec3&		baseTexel,
						  IVec3&		texelGridOffset)
{
	const int subdivisions = (int) 1 << coordBits;

	for (int compNdx = 0; compNdx < 3; ++compNdx)
	{
		// \todo [2016-07-22 collinbaker] Do floor division to properly handle negative coords
		baseTexel[compNdx]		 = gridCoord[compNdx] / (deInt32) subdivisions;
		texelGridOffset[compNdx] = gridCoord[compNdx] % (deInt32) subdivisions;
	}
}

void calcTexelGridCoordRange (const Vec3&	unnormalizedCoordMin,
							  const Vec3&	unnormalizedCoordMax,
							  const int		coordBits,
							  IVec3&		gridCoordMin,
							  IVec3&		gridCoordMax)
{
	const int subdivisions = 1 << coordBits;

	for (int compNdx = 0; compNdx < 3; ++compNdx)
	{
		const float comp[2] = {unnormalizedCoordMin[compNdx],
							   unnormalizedCoordMax[compNdx]};

		float	fracPart[2];
		double	intPart[2];

		for (int ndx = 0; ndx < 2; ++ndx)
		{
			fracPart[ndx] = (float) deModf(comp[ndx], &intPart[ndx]);

			if (comp[ndx] < 0.0f)
			{
				intPart [ndx] -= 1.0;
				fracPart[ndx] += 1.0f;
			}
		}

		const deInt32	nearestTexelGridOffsetMin = (deInt32) deFloor(intPart[0]);
		const deInt32	nearestTexelGridOffsetMax = (deInt32) deFloor(intPart[1]);

		const deInt32	subTexelGridCoordMin	  = de::max((deInt32) deFloor(fracPart[0] * (float) subdivisions), (deInt32) 0);
		const deInt32	subTexelGridCoordMax	  = de::min((deInt32) deCeil (fracPart[1] * (float) subdivisions), (deInt32) (subdivisions - 1));

	    gridCoordMin[compNdx] = nearestTexelGridOffsetMin * (deInt32) subdivisions + subTexelGridCoordMin;
	    gridCoordMax[compNdx] = nearestTexelGridOffsetMax * (deInt32) subdivisions + subTexelGridCoordMax;
	}
}

void calcUnnormalizedCoordRange (const Vec4&		coord,
								 const IVec3&		levelSize,
								 const FloatFormat& internalFormat,
								 Vec3&				unnormalizedCoordMin,
								 Vec3&				unnormalizedCoordMax)
{
    for (int compNdx = 0; compNdx < 3; ++compNdx)
	{
		const int size = levelSize[compNdx];

		Interval coordInterval = Interval(coord[compNdx]);
		coordInterval = internalFormat.roundOut(coordInterval, false);

		Interval unnormalizedCoordInterval = coordInterval * Interval((double) size);
		unnormalizedCoordInterval = internalFormat.roundOut(unnormalizedCoordInterval, false);

		unnormalizedCoordMin[compNdx] = (float)unnormalizedCoordInterval.lo();
		unnormalizedCoordMax[compNdx] = (float)unnormalizedCoordInterval.hi();
	}
}

Vec2 calcLodBounds (const Vec3& dPdx,
					const Vec3& dPdy,
					const IVec3 size,
					const float lodBias,
					const float lodMin,
					const float lodMax)
{
	Vec2 lodBounds;

	const Vec3 mx = abs(dPdx) * size.asFloat();
	const Vec3 my = abs(dPdy) * size.asFloat();

	Vec2 scaleXBounds;
	Vec2 scaleYBounds;

	scaleXBounds[0] = de::max(de::abs(mx[0]), de::max(de::abs(mx[1]), de::abs(mx[2])));
	scaleYBounds[0] = de::max(de::abs(my[0]), de::max(de::abs(my[1]), de::abs(my[2])));

	scaleXBounds[1] = de::abs(mx[0]) + de::abs(mx[1]) + de::abs(mx[2]);
	scaleYBounds[1] = de::abs(my[0]) + de::abs(my[1]) + de::abs(my[2]);

	Vec2 scaleMaxBounds;

	for (int compNdx = 0; compNdx < 2; ++compNdx)
	{
		scaleMaxBounds[compNdx] = de::max(scaleXBounds[compNdx], scaleYBounds[compNdx]);
	}

	for (int ndx = 0; ndx < 2; ++ndx)
	{
		lodBounds[ndx] = deFloatLog2(scaleMaxBounds[ndx]);
		lodBounds[ndx] += lodBias;
		lodBounds[ndx] = de::clamp(lodBounds[ndx], lodMin, lodMax);
	}

	return lodBounds;
}

void calcCubemapFaceCoords (const Vec3& r,
							const Vec3& drdx,
							const Vec3& drdy,
							const int	faceNdx,
							Vec2&		coordFace,
							Vec2&		dPdxFace,
							Vec2&		dPdyFace)
{
	DE_ASSERT(faceNdx >= 0 && faceNdx < 6);

	static const int compMap[6][3] =
	{
		{2, 1, 0},
		{2, 1, 0},
		{0, 2, 1},
		{0, 2, 1},
		{0, 1, 2},
		{0, 1, 2}
	};

	static const int signMap[6][3] =
	{
		{-1, -1, +1},
		{+1, -1, -1},
		{+1, +1, +1},
		{+1, -1, -1},
		{+1, -1, +1},
		{-1, -1, -1}
	};

	Vec3 coordC;
	Vec3 dPcdx;
	Vec3 dPcdy;

	for (int compNdx = 0; compNdx < 3; ++compNdx)
	{
		const int	mappedComp = compMap[faceNdx][compNdx];
		const int	mappedSign = signMap[faceNdx][compNdx];

		coordC[compNdx] = r   [mappedComp]	* (float)mappedSign;
		dPcdx [compNdx]	= drdx[mappedComp]	* (float)mappedSign;
		dPcdy [compNdx]	= drdy[mappedComp]	* (float)mappedSign;
	}

	DE_ASSERT(coordC[2] != 0.0f);
	coordC[2] = de::abs(coordC[2]);

	for (int compNdx = 0; compNdx < 2; ++compNdx)
	{
		coordFace[compNdx] = 0.5f * coordC[compNdx] / de::abs(coordC[2]) + 0.5f;

		dPdxFace [compNdx] = 0.5f * (de::abs(coordC[2]) * dPcdx[compNdx] - coordC[compNdx] * dPcdx[2]) / (coordC[2] * coordC[2]);
		dPdyFace [compNdx] = 0.5f * (de::abs(coordC[2]) * dPcdy[compNdx] - coordC[compNdx] * dPcdy[2]) / (coordC[2] * coordC[2]);
	}
}

int calcCandidateCubemapFaces (const Vec3& r)
{
	deUint8 faceBitmap = 0;
	float	rMax	   = de::abs(r[0]);

	for (int compNdx = 1; compNdx < 3; ++compNdx)
	{
		rMax = de::max(rMax, de::abs(r[compNdx]));
	}

	for (int compNdx = 0; compNdx < 3; ++compNdx)
	{
		if (de::abs(r[compNdx]) == rMax)
		{
			const int faceNdx = 2 * compNdx + (r[compNdx] < 0.0f ? 1 : 0);

			DE_ASSERT(faceNdx < 6);

			faceBitmap = (deUint8)(faceBitmap | (deUint8) (1U << faceNdx));
		}
	}

	DE_ASSERT(faceBitmap != 0U);

	return faceBitmap;
}

deInt32 wrapTexelCoord (const deInt32 coord,
						const int size,
						const VkSamplerAddressMode wrap)
{
	deInt32 wrappedCoord = 0;

	switch (wrap)
	{
		case VK_SAMPLER_ADDRESS_MODE_REPEAT:
			wrappedCoord = mod(coord, size);
			break;

		case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
			wrappedCoord = (size - 1) - mirror(mod(coord, 2 * size) - size);
			break;

		case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
			wrappedCoord = de::clamp(coord, 0, (deInt32) size - 1);
			break;

		case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
			wrappedCoord = de::clamp(coord, -1, (deInt32) size);
			break;

		case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
			wrappedCoord = de::clamp(mirror(coord), 0, (deInt32) size - 1);
			break;

		default:
			DE_FATAL("Invalid VkSamplerAddressMode");
			break;
	}

	return wrappedCoord;
}

namespace
{

// Cube map adjacent faces ordered clockwise from top
// \todo [2016-07-07 collinbaker] Verify these are correct
static const int adjacentFaces[6][4] =
{
	{3, 5, 2, 4},
	{3, 4, 2, 5},
	{4, 0, 5, 1},
	{5, 0, 4, 1},
	{3, 0, 2, 1},
	{3, 1, 2, 0}
};

static const int adjacentEdges[6][4] =
{
	{1, 3, 1, 1},
	{3, 3, 3, 1},
	{2, 2, 2, 2},
	{0, 0, 0, 0},
	{2, 3, 0, 1},
	{0, 3, 2, 1}
};

static const int adjacentEdgeDirs[6][4] =
{
	{-1, +1, +1, +1},
	{+1, +1, -1, +1},
	{+1, +1, -1, -1},
	{-1, -1, +1, +1},
	{+1, +1, +1, +1},
	{-1, +1, -1, +1}
};

static const int edgeComponent[4] = {0, 1, 0, 1};

static const int edgeFactors[4][2] =
{
	{0, 0},
	{1, 0},
	{0, 1},
	{0, 0}
};

} // anonymous

void wrapCubemapEdge (const IVec2&	coord,
					  const IVec2&	size,
					  const int		faceNdx,
					  IVec2&		newCoord,
					  int&			newFaceNdx)
{
	int edgeNdx = -1;

	if (coord[1] < 0)
	{
		edgeNdx = 0;
	}
	else if (coord[0] > 0)
	{
		edgeNdx = 1;
	}
	else if (coord[1] > 0)
	{
		edgeNdx = 2;
	}
	else
	{
		edgeNdx = 3;
	}

	const int		adjacentEdgeNdx = adjacentEdges[faceNdx][edgeNdx];
	const IVec2		edgeFactor		= IVec2(edgeFactors[adjacentEdgeNdx][0],
											edgeFactors[adjacentEdgeNdx][1]);
	const IVec2		edgeOffset		= edgeFactor * (size - IVec2(1));

	if (adjacentEdgeDirs[faceNdx][edgeNdx] > 0)
	{
		newCoord[edgeComponent[adjacentEdgeNdx]] = coord[edgeComponent[edgeNdx]];
	}
	else
	{
		newCoord[edgeComponent[adjacentEdgeNdx]] =
		    size[edgeComponent[edgeNdx]] - coord[edgeComponent[edgeNdx]] - 1;
	}

	newCoord[1 - edgeComponent[adjacentEdgeNdx]] = 0;
	newCoord += edgeOffset;

	newFaceNdx = adjacentFaces[faceNdx][edgeNdx];
}

void wrapCubemapCorner (const IVec2&	coord,
						const IVec2&	size,
						const int		faceNdx,
						int&			adjacentFace1,
						int&			adjacentFace2,
						IVec2&			cornerCoord0,
						IVec2&			cornerCoord1,
						IVec2&			cornerCoord2)
{
	int cornerNdx = -1;

	if (coord[0] < 0 && coord[1] < 0)
	{
		cornerNdx = 0;
	}
	else if (coord[0] > 0 && coord[1] < 0)
	{
		cornerNdx = 1;
	}
	else if (coord[0] > 0 && coord[1] > 0)
	{
		cornerNdx = 2;
	}
	else
	{
		cornerNdx = 3;
	}

	const int cornerEdges[2] = {cornerNdx, (int) ((cornerNdx + 3) % 4)};

	int		  faceCorners[3] = {cornerNdx, 0, 0};

	for (int edgeNdx = 0; edgeNdx < 2; ++edgeNdx)
	{
		const int faceEdge = adjacentEdges[faceNdx][cornerEdges[edgeNdx]];

		bool isFlipped = (adjacentEdgeDirs[faceNdx][cornerEdges[edgeNdx]] == -1);

		if ((cornerEdges[edgeNdx] > 1) != (faceEdge > 1))
		{
			isFlipped = !isFlipped;
		}

		if (isFlipped)
		{
			faceCorners[edgeNdx + 1] = (faceEdge + 1) % 4;
		}
		else
		{
			faceCorners[edgeNdx + 1] = faceEdge;
		}
	}

	adjacentFace1 = adjacentFaces[faceNdx][cornerEdges[0]];
	adjacentFace2 = adjacentFaces[faceNdx][cornerEdges[1]];

	IVec2* cornerCoords[3] = {&cornerCoord0, &cornerCoord1, &cornerCoord2};

	for (int ndx = 0; ndx < 3; ++ndx)
	{
		IVec2 cornerFactor;

		switch (faceCorners[faceNdx])
		{
			case 0:
				cornerFactor = IVec2(0, 0);
				break;

			case 1:
				cornerFactor = IVec2(1, 0);
				break;

			case 2:
				cornerFactor = IVec2(1, 1);
				break;

			case 3:
				cornerFactor = IVec2(0, 1);
				break;

			default:
				break;
		}

	    *cornerCoords[ndx] = cornerFactor * (size - IVec2(1));
	}
}

namespace
{

deInt64 signExtend (deUint64 src, int bits)
{
	const deUint64 signBit = 1ull << (bits-1);

	src |= ~((src & signBit) - 1);

	return (deInt64) src;
}

void convertFP16 (const void*	fp16Ptr,
				  FloatFormat	internalFormat,
				  float&		resultMin,
				  float&		resultMax)
{
	const Float16  fp16(*(const deUint16*) fp16Ptr);
	const Interval fpInterval = internalFormat.roundOut(Interval(fp16.asDouble()), false);

	resultMin = (float) fpInterval.lo();
	resultMax = (float) fpInterval.hi();
}

void convertNormalizedInt (deInt64		num,
						   int			numBits,
						   bool			isSigned,
						   FloatFormat	internalFormat,
						   float&		resultMin,
						   float&		resultMax)
{
	DE_ASSERT(numBits > 0);

	const double	c	 = (double) num;
	deUint64		exp	 = numBits;

	if (isSigned)
		--exp;

	const double div = (double) (((deUint64) 1 << exp) - 1);

	Interval resultInterval(de::max(c / div, -1.0));
	resultInterval = internalFormat.roundOut(resultInterval, false);

	resultMin = (float) resultInterval.lo();
	resultMax = (float) resultInterval.hi();
}

bool isPackedType (const TextureFormat::ChannelType type)
{
	DE_STATIC_ASSERT(TextureFormat::CHANNELTYPE_LAST == 40);

	switch (type)
	{
		case TextureFormat::UNORM_BYTE_44:
		case TextureFormat::UNORM_SHORT_565:
		case TextureFormat::UNORM_SHORT_555:
		case TextureFormat::UNORM_SHORT_4444:
		case TextureFormat::UNORM_SHORT_5551:
		case TextureFormat::UNORM_SHORT_1555:
		case TextureFormat::UNORM_INT_101010:
		case TextureFormat::SNORM_INT_1010102_REV:
		case TextureFormat::UNORM_INT_1010102_REV:
			return true;

		default:
			return false;
	}
}

void getPackInfo (const TextureFormat texFormat,
				  IVec4& bitSizes,
				  IVec4& bitOffsets,
				  int& baseTypeBytes)
{
	DE_STATIC_ASSERT(TextureFormat::CHANNELTYPE_LAST == 40);

	switch (texFormat.type)
	{
		case TextureFormat::UNORM_BYTE_44:
			bitSizes = IVec4(4, 4, 0, 0);
			bitOffsets = IVec4(0, 4, 0, 0);
			baseTypeBytes = 1;
			break;

		case TextureFormat::UNORM_SHORT_565:
			bitSizes = IVec4(5, 6, 5, 0);
			bitOffsets = IVec4(0, 5, 11, 0);
			baseTypeBytes = 2;
			break;

		case TextureFormat::UNORM_SHORT_555:
			bitSizes = IVec4(5, 5, 5, 0);
			bitOffsets = IVec4(0, 5, 10, 0);
			baseTypeBytes = 2;
			break;

		case TextureFormat::UNORM_SHORT_4444:
			bitSizes = IVec4(4, 4, 4, 4);
			bitOffsets = IVec4(0, 4, 8, 12);
			baseTypeBytes = 2;
			break;

		case TextureFormat::UNORM_SHORT_5551:
			bitSizes = IVec4(5, 5, 5, 1);
			bitOffsets = IVec4(0, 5, 10, 15);
			baseTypeBytes = 2;
			break;

		case TextureFormat::UNORM_SHORT_1555:
			bitSizes = IVec4(1, 5, 5, 5);
			bitOffsets = IVec4(0, 1, 6, 11);
			baseTypeBytes = 2;
			break;

		case TextureFormat::UNORM_INT_101010:
			bitSizes = IVec4(10, 10, 10, 0);
			bitOffsets = IVec4(0, 10, 20, 0);
			baseTypeBytes = 4;
			break;

		case TextureFormat::SNORM_INT_1010102_REV:
			bitSizes = IVec4(2, 10, 10, 10);
			bitOffsets = IVec4(0, 2, 12, 22);
			baseTypeBytes = 4;
			break;

		case TextureFormat::UNORM_INT_1010102_REV:
			bitSizes = IVec4(2, 10, 10, 10);
			bitOffsets = IVec4(0, 2, 12, 22);
			baseTypeBytes = 4;
			break;

		default:
			DE_FATAL("Invalid texture channel type");
			return;
	}
}

template <typename BaseType>
deUint64 unpackBits (const BaseType pack,
					 const int		bitOffset,
					 const int		numBits)
{
	DE_ASSERT(bitOffset + numBits <= 8 * (int) sizeof(BaseType));

	const BaseType mask = (BaseType) (((BaseType) 1 << (BaseType) numBits) - (BaseType) 1);

	return mask & (pack >> (BaseType) (8 * (int) sizeof(BaseType) - bitOffset - numBits));
}

deUint64 readChannel (const void* ptr,
					  const int byteOffset,
					  const int numBytes)
{
	const deUint8*	cPtr   = (const deUint8*) ptr + byteOffset;
	deUint64		result = 0;

	for (int byteNdx = 0; byteNdx < numBytes; ++byteNdx)
	{
		result = (result << 8U) | (deUint64) (cPtr[numBytes - byteNdx - 1]);
	}

	return result;
}

void convertNormalizedFormat (const void*	pixelPtr,
							  TextureFormat	texFormat,
							  FloatFormat	internalFormat,
							  Vec4&			resultMin,
							  Vec4&			resultMax)
{
    TextureSwizzle				readSwizzle	= getChannelReadSwizzle(texFormat.order);
	const TextureChannelClass	chanClass	= getTextureChannelClass(texFormat.type);

	DE_ASSERT(getTextureChannelClass(texFormat.type) < 2);

	// Information for non-packed types
	int chanSize = -1;

	// Information for packed types
	IVec4 bitOffsets;
	IVec4 bitSizes;
	int baseTypeBytes = -1;

	const bool isPacked = isPackedType(texFormat.type);

	if (isPacked)
	{
		getPackInfo(texFormat, bitSizes, bitOffsets, baseTypeBytes);

		// Kludge to work around deficiency in framework

		if (texFormat.type == TextureFormat::UNORM_INT_1010102_REV ||
			texFormat.type == TextureFormat::SNORM_INT_1010102_REV)
		{
			for (int ndx = 0; ndx < 2; ++ndx)
			{
				std::swap(readSwizzle.components[ndx], readSwizzle.components[3 - ndx]);
			}
		}

		DE_ASSERT(baseTypeBytes == 1 || baseTypeBytes == 2 || baseTypeBytes == 4);
	}
	else
	{
		chanSize = getChannelSize(texFormat.type);
	}

	const bool	isSigned = (chanClass == TEXTURECHANNELCLASS_SIGNED_FIXED_POINT);
	const bool	isSrgb	 = isSRGB(texFormat);

	// \todo [2016-08-01 collinbaker] Handle sRGB with correct rounding
	DE_ASSERT(!isSrgb);
	DE_UNREF(isSrgb);

	for (int compNdx = 0; compNdx < 4; ++compNdx)
	{
		const TextureSwizzle::Channel chan = readSwizzle.components[compNdx];

		if (chan == TextureSwizzle::CHANNEL_ZERO)
		{
			resultMin[compNdx] = 0.0f;
			resultMax[compNdx] = 0.0f;
		}
		else if (chan == TextureSwizzle::CHANNEL_ONE)
		{
			resultMin[compNdx] = 1.0f;
			resultMax[compNdx] = 1.0f;
		}
		else
		{
			deUint64 chanUVal = 0;
			int chanBits = 0;

			if (isPacked)
			{
				deUint64 pack = readChannel(pixelPtr, 0, baseTypeBytes);
				chanBits = bitSizes[chan];

				switch (baseTypeBytes)
				{
					case 1:
						chanUVal = unpackBits<deUint8>((deUint8)pack, bitOffsets[chan], bitSizes[chan]);
						break;

					case 2:
						chanUVal = unpackBits<deUint16>((deUint16)pack, bitOffsets[chan], bitSizes[chan]);
						break;

					case 4:
						chanUVal = unpackBits<deUint32>((deUint32)pack, bitOffsets[chan], bitSizes[chan]);
						break;

					default:
						break;
				}
			}
			else
			{
			    chanUVal = readChannel(pixelPtr, chan * chanSize, chanSize);
				chanBits = 8 * chanSize;
			}

			deInt64 chanVal = 0;

			if (isSigned)
			{
				chanVal = signExtend(chanUVal, chanBits);
			}
			else
			{
				chanVal = (deInt64) chanUVal;
			}

			convertNormalizedInt(chanVal, chanBits, isSigned, internalFormat, resultMin[compNdx], resultMax[compNdx]);
		}
	}
}

void convertFloatFormat (const void*	pixelPtr,
						 TextureFormat	texFormat,
						 FloatFormat	internalFormat,
						 Vec4&			resultMin,
						 Vec4&			resultMax)
{
	DE_ASSERT(getTextureChannelClass(texFormat.type) == TEXTURECHANNELCLASS_FLOATING_POINT);

	const TextureSwizzle readSwizzle = getChannelReadSwizzle(texFormat.order);

	for (int compNdx = 0; compNdx < 4; ++compNdx)
	{
		const TextureSwizzle::Channel chan = readSwizzle.components[compNdx];

		if (chan == TextureSwizzle::CHANNEL_ZERO)
		{
			resultMin[compNdx] = 0.0f;
			resultMax[compNdx] = 0.0f;
		}
		else if (chan == TextureSwizzle::CHANNEL_ONE)
		{
			resultMin[compNdx] = 1.0f;
			resultMax[compNdx] = 1.0f;
		}
		else if (texFormat.type == TextureFormat::FLOAT)
		{
			resultMin[compNdx] = resultMax[compNdx] = *((const float*)pixelPtr + chan);
		}
		else if (texFormat.type == TextureFormat::HALF_FLOAT)
		{
			convertFP16((const deUint16*) pixelPtr + chan, internalFormat, resultMin[compNdx], resultMax[compNdx]);
		}
		else
		{
			DE_FATAL("Unsupported floating point format");
		}
	}
}

} // anonymous

void convertFormat (const void*		pixelPtr,
					TextureFormat	texFormat,
					FloatFormat		internalFormat,
					Vec4&			resultMin,
					Vec4&			resultMax)
{
	const TextureChannelClass	chanClass	 = getTextureChannelClass(texFormat.type);

	// \todo [2016-08-01 collinbaker] Handle float and shared exponent formats
	if (chanClass == TEXTURECHANNELCLASS_SIGNED_FIXED_POINT || chanClass == TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT)
	{
		convertNormalizedFormat(pixelPtr, texFormat, internalFormat, resultMin, resultMax);
	}
	else if (chanClass == TEXTURECHANNELCLASS_FLOATING_POINT)
	{
		convertFloatFormat(pixelPtr, texFormat, internalFormat, resultMin, resultMax);
	}
	else
	{
		DE_FATAL("Unimplemented");
	}
}

} // util
} // texture
} // vkt
