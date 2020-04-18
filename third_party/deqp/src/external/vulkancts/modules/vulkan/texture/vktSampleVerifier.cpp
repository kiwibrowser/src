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
#include "vktSampleVerifierUtil.hpp"

#include "deMath.h"
#include "tcuFloat.hpp"
#include "tcuTextureUtil.hpp"
#include "vkImageUtil.hpp"

#include <fstream>
#include <sstream>

namespace vkt
{
namespace texture
{

using namespace vk;
using namespace tcu;
using namespace util;

namespace
{

int calcUnnormalizedDim (const ImgDim dim)
{
	if (dim == IMG_DIM_1D)
	{
	    return 1;
	}
	else if (dim == IMG_DIM_2D || dim == IMG_DIM_CUBE)
	{
	    return 2;
	}
	else
	{
	    return 3;
	}
}

} // anonymous

SampleVerifier::SampleVerifier (const ImageViewParameters&						imParams,
								const SamplerParameters&						samplerParams,
								const SampleLookupSettings&						sampleLookupSettings,
								int												coordBits,
								int												mipmapBits,
								const tcu::FloatFormat&							conversionPrecision,
								const tcu::FloatFormat&							filteringPrecision,
								const std::vector<tcu::ConstPixelBufferAccess>&	levels)
	: m_imParams				(imParams)
	, m_samplerParams			(samplerParams)
	, m_sampleLookupSettings	(sampleLookupSettings)
	, m_coordBits				(coordBits)
	, m_mipmapBits				(mipmapBits)
	, m_conversionPrecision		(conversionPrecision)
	, m_filteringPrecision		(filteringPrecision)
	, m_unnormalizedDim			(calcUnnormalizedDim(imParams.dim))
	, m_levels					(levels)
{

}

bool SampleVerifier::coordOutOfRange (const IVec3& coord, int compNdx, int level) const
{
	DE_ASSERT(compNdx >= 0 && compNdx < 3);

	return coord[compNdx] < 0 || coord[compNdx] >= m_levels[level].getSize()[compNdx];
}

void SampleVerifier::fetchTexelWrapped (const IVec3&	coord,
										int				layer,
										int				level,
										Vec4&			resultMin,
										Vec4&			resultMax) const
{
    const void* pixelPtr = DE_NULL;

	if (m_imParams.dim == IMG_DIM_1D)
	{
	    pixelPtr = m_levels[level].getPixelPtr(coord[0], layer, 0);
	}
	else if (m_imParams.dim == IMG_DIM_2D || m_imParams.dim == IMG_DIM_CUBE)
	{
		pixelPtr = m_levels[level].getPixelPtr(coord[0], coord[1], layer);
	}
	else
	{
		pixelPtr = m_levels[level].getPixelPtr(coord[0], coord[1], coord[2]);
	}

	convertFormat(pixelPtr, mapVkFormat(m_imParams.format), m_conversionPrecision, resultMin, resultMax);

#if defined(DE_DEBUG)
	// Make sure tcuTexture agrees
	const tcu::ConstPixelBufferAccess&	levelAccess	= m_levels[level];
	const tcu::Vec4						refPix		= (m_imParams.dim == IMG_DIM_1D) ? levelAccess.getPixel(coord[0], layer, 0)
													: (m_imParams.dim == IMG_DIM_2D || m_imParams.dim == IMG_DIM_CUBE) ? levelAccess.getPixel(coord[0], coord[1], layer)
													: levelAccess.getPixel(coord[0], coord[1], coord[2]);

	for (int c = 0; c < 4; c++)
		DE_ASSERT(de::inRange(refPix[c], resultMin[c], resultMax[c]));
#endif
}

void SampleVerifier::fetchTexel (const IVec3&	coordIn,
								 int			layer,
								 int			level,
								 VkFilter		filter,
								 Vec4&			resultMin,
								 Vec4&			resultMax) const
{
	IVec3 coord = coordIn;

	VkSamplerAddressMode wrappingModes[] =
	{
		m_samplerParams.wrappingModeU,
		m_samplerParams.wrappingModeV,
		m_samplerParams.wrappingModeW
	};

	const bool isSrgb = isSrgbFormat(m_imParams.format);

	// Wrapping operations


	if (m_imParams.dim == IMG_DIM_CUBE && filter == VK_FILTER_LINEAR)
	{
		// If the image is a cubemap and we are using linear filtering, we do edge or corner wrapping

		const int	arrayLayer = layer / 6;
		int			arrayFace  = layer % 6;

		if (coordOutOfRange(coord, 0, level) != coordOutOfRange(coord, 1, level))
		{
			// Wrap around edge

			IVec2	newCoord(0);
			int		newFace = 0;

			wrapCubemapEdge(coord.swizzle(0, 1),
							m_levels[level].getSize().swizzle(0, 1),
							arrayFace,
							newCoord,
							newFace);

			coord.xy()	= newCoord;
			layer		= arrayLayer * 6 + newFace;
		}
		else if (coordOutOfRange(coord, 0, level) && coordOutOfRange(coord, 1, level))
		{
			// Wrap corner

			int   faces[3] = {arrayFace, 0, 0};
			IVec2 cornerCoords[3];

			wrapCubemapCorner(coord.swizzle(0, 1),
							  m_levels[level].getSize().swizzle(0, 1),
							  arrayFace,
							  faces[1],
							  faces[2],
							  cornerCoords[0],
							  cornerCoords[1],
							  cornerCoords[2]);

			// \todo [2016-08-01 collinbaker] Call into fetchTexelWrapped instead

			Vec4 cornerTexels[3];

			for (int ndx = 0; ndx < 3; ++ndx)
			{
				int cornerLayer = faces[ndx] + arrayLayer * 6;

				if (isSrgb)
				{
				    cornerTexels[ndx] += sRGBToLinear(m_levels[level].getPixel(cornerCoords[ndx][0], cornerCoords[ndx][1], cornerLayer));
				}
				else
				{
					cornerTexels[ndx] += m_levels[level].getPixel(cornerCoords[ndx][0], cornerCoords[ndx][1], cornerLayer);
				}
			}

			for (int compNdx = 0; compNdx < 4; ++compNdx)
			{
				float compMin = cornerTexels[0][compNdx];
				float compMax = cornerTexels[0][compNdx];

				for (int ndx = 1; ndx < 3; ++ndx)
				{
					const float comp = cornerTexels[ndx][compNdx];

					compMin = de::min(comp, compMin);
					compMax = de::max(comp, compMax);
				}

				resultMin[compNdx] = compMin;
				resultMax[compNdx] = compMax;
			}

			return;
		}
		else
		{
			// If no wrapping is necessary, just do nothing
		}
	}
	else
	{
		// Otherwise, we do normal wrapping

		if (m_imParams.dim == IMG_DIM_CUBE)
		{
			wrappingModes[0] = wrappingModes[1] = wrappingModes[2] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}

		for (int compNdx = 0; compNdx < 3; ++compNdx)
		{
			const int size = m_levels[level].getSize()[compNdx];

			coord[compNdx] = wrapTexelCoord(coord[compNdx], size, wrappingModes[compNdx]);
		}
	}

	if (coordOutOfRange(coord, 0, level) ||
		coordOutOfRange(coord, 1, level) ||
		coordOutOfRange(coord, 2, level))
	{
		// If after wrapping coordinates are still out of range, perform texel replacement

		switch (m_samplerParams.borderColor)
		{
			case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
			{
				resultMin = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
				resultMax = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
				return;
			}
			case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
			{
				resultMin = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
				resultMax = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
				return;
			}
			case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
			{
				resultMin = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
				resultMax = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
				return;
			}
			default:
			{
				// \\ [2016-07-07 collinbaker] Handle
				// VK_BORDER_COLOR_INT_* borders
				DE_FATAL("Not implemented");
				break;
			}
		}
	}
	else
	{
		// Otherwise, actually fetch a texel

	    fetchTexelWrapped(coord, layer, level, resultMin, resultMax);
	}
}

void SampleVerifier::getFilteredSample1D (const IVec3&	texelBase,
										  float			weight,
										  int			layer,
										  int			level,
										  Vec4&			resultMin,
										  Vec4&			resultMax) const
{
	Vec4 texelsMin[2];
	Vec4 texelsMax[2];

	for (int i = 0; i < 2; ++i)
	{
	    fetchTexel(texelBase + IVec3(i, 0, 0), layer, level, VK_FILTER_LINEAR, texelsMin[i], texelsMax[i]);
	}

	Interval resultIntervals[4];

	for (int ndx = 0; ndx < 4; ++ndx)
	{
		resultIntervals[ndx] = Interval(0.0);
	}

    for (int i = 0; i < 2; ++i)
	{
		const Interval weightInterval = m_filteringPrecision.roundOut(Interval(i == 0 ? 1.0f - weight : weight), false);

		for (int compNdx = 0; compNdx < 4; ++compNdx)
		{
			const Interval texelInterval(false, texelsMin[i][compNdx], texelsMax[i][compNdx]);

			resultIntervals[compNdx] = m_filteringPrecision.roundOut(resultIntervals[compNdx] + weightInterval * texelInterval, false);
		}
	}

	for (int compNdx = 0; compNdx < 4; ++compNdx)
	{
		resultMin[compNdx] = (float)resultIntervals[compNdx].lo();
		resultMax[compNdx] = (float)resultIntervals[compNdx].hi();
	}
}


void SampleVerifier::getFilteredSample2D (const IVec3&	texelBase,
										  const Vec2&	weights,
										  int			layer,
										  int			level,
										  Vec4&			resultMin,
										  Vec4&			resultMax) const
{
	Vec4 texelsMin[4];
	Vec4 texelsMax[4];

	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
		    fetchTexel(texelBase + IVec3(i, j, 0), layer, level, VK_FILTER_LINEAR, texelsMin[2 * j + i], texelsMax[2 * j + i]);
		}
	}

	Interval resultIntervals[4];

	for (int ndx = 0; ndx < 4; ++ndx)
	{
		resultIntervals[ndx] = Interval(0.0);
	}

	for (int i = 0; i < 2; ++i)
	{
		const Interval iWeightInterval = m_filteringPrecision.roundOut(Interval(i == 0 ? 1.0f - weights[1] : weights[1]), false);

		for (int j = 0; j < 2; ++j)
		{
		    const Interval jWeightInterval = m_filteringPrecision.roundOut(iWeightInterval * Interval(j == 0 ? 1.0f - weights[0] : weights[0]), false);

			for (int compNdx = 0; compNdx < 4; ++compNdx)
			{
				const Interval texelInterval(false, texelsMin[2 * i + j][compNdx], texelsMax[2 * i + j][compNdx]);

				resultIntervals[compNdx] = m_filteringPrecision.roundOut(resultIntervals[compNdx] + jWeightInterval * texelInterval, false);
			}
		}
	}

	for (int compNdx = 0; compNdx < 4; ++compNdx)
	{
		resultMin[compNdx] = (float)resultIntervals[compNdx].lo();
		resultMax[compNdx] = (float)resultIntervals[compNdx].hi();
	}
}

void SampleVerifier::getFilteredSample3D (const IVec3&	texelBase,
										  const Vec3&	weights,
										  int			layer,
										  int			level,
										  Vec4&			resultMin,
										  Vec4&			resultMax) const
{
	Vec4 texelsMin[8];
	Vec4 texelsMax[8];

	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			for (int k = 0; k < 2; ++k)
			{
			    fetchTexel(texelBase + IVec3(i, j, k), layer, level, VK_FILTER_LINEAR, texelsMin[4 * k + 2 * j + i], texelsMax[4 * k + 2 * j + i]);
			}
		}
	}

    Interval resultIntervals[4];

	for (int ndx = 0; ndx < 4; ++ndx)
	{
		resultIntervals[ndx] = Interval(0.0);
	}

	for (int i = 0; i < 2; ++i)
	{
	    const Interval iWeightInterval = m_filteringPrecision.roundOut(Interval(i == 0 ? 1.0f - weights[2] : weights[2]), false);

		for (int j = 0; j < 2; ++j)
		{
		    const Interval jWeightInterval = m_filteringPrecision.roundOut(iWeightInterval * Interval(j == 0 ? 1.0f - weights[1] : weights[1]), false);

			for (int k = 0; k < 2; ++k)
			{
			    const Interval kWeightInterval = m_filteringPrecision.roundOut(jWeightInterval * Interval(k == 0 ? 1.0f - weights[0] : weights[0]), false);

				for (int compNdx = 0; compNdx < 4; ++compNdx)
				{
					const Interval texelInterval(false, texelsMin[4 * i + 2 * j + k][compNdx], texelsMax[4 * i + 2 * j + k][compNdx]);

					resultIntervals[compNdx] = m_filteringPrecision.roundOut(resultIntervals[compNdx] + kWeightInterval * texelInterval, false);
				}
			}
		}
	}

	for (int compNdx = 0; compNdx < 4; ++compNdx)
	{
		resultMin[compNdx] = (float)resultIntervals[compNdx].lo();
		resultMax[compNdx] = (float)resultIntervals[compNdx].hi();
	}
}

void SampleVerifier::getFilteredSample (const IVec3&	texelBase,
										const Vec3&		weights,
										int				layer,
										int				level,
										Vec4&			resultMin,
										Vec4&			resultMax) const
{
	DE_ASSERT(layer < m_imParams.arrayLayers);
	DE_ASSERT(level < m_imParams.levels);

	if (m_imParams.dim == IMG_DIM_1D)
	{
		getFilteredSample1D(texelBase, weights.x(), layer, level, resultMin, resultMax);
	}
	else if (m_imParams.dim == IMG_DIM_2D || m_imParams.dim == IMG_DIM_CUBE)
	{
		getFilteredSample2D(texelBase, weights.swizzle(0, 1), layer, level, resultMin, resultMax);
	}
	else
	{
		getFilteredSample3D(texelBase, weights, layer, level, resultMin, resultMax);
	}
}

void SampleVerifier::getMipmapStepBounds (const Vec2&	lodFracBounds,
										  deInt32&		stepMin,
										  deInt32&		stepMax) const
{
	DE_ASSERT(m_mipmapBits < 32);
	const int mipmapSteps = ((int)1) << m_mipmapBits;

	stepMin = deFloorFloatToInt32(lodFracBounds[0] * (float)mipmapSteps);
	stepMax = deCeilFloatToInt32 (lodFracBounds[1] * (float)mipmapSteps);

	stepMin = de::max(stepMin, (deInt32)0);
	stepMax = de::min(stepMax, (deInt32)mipmapSteps);
}

bool SampleVerifier::verifySampleFiltered (const Vec4&			result,
										   const IVec3&			baseTexelHiIn,
										   const IVec3&			baseTexelLoIn,
										   const IVec3&			texelGridOffsetHiIn,
										   const IVec3&			texelGridOffsetLoIn,
										   int					layer,
										   int					levelHi,
										   const Vec2&			lodFracBounds,
										   VkFilter				filter,
										   VkSamplerMipmapMode	mipmapFilter,
										   std::ostream&		report) const
{
	DE_ASSERT(layer < m_imParams.arrayLayers);
	DE_ASSERT(levelHi < m_imParams.levels);

	const int	coordSteps			= 1 << m_coordBits;
	const int	lodSteps			= 1 << m_mipmapBits;
	const int	levelLo				= (levelHi < m_imParams.levels - 1) ? levelHi + 1 : levelHi;

	IVec3		baseTexelHi			= baseTexelHiIn;
	IVec3		baseTexelLo			= baseTexelLoIn;
	IVec3		texelGridOffsetHi	= texelGridOffsetHiIn;
	IVec3		texelGridOffsetLo	= texelGridOffsetLoIn;
	deInt32		lodStepsMin			= 0;
	deInt32		lodStepsMax			= 0;

	getMipmapStepBounds(lodFracBounds, lodStepsMin, lodStepsMax);

	report << "Testing at base texel " << baseTexelHi << ", " << baseTexelLo << " offset " << texelGridOffsetHi << ", " << texelGridOffsetLo << "\n";

	Vec4 idealSampleHiMin;
	Vec4 idealSampleHiMax;
	Vec4 idealSampleLoMin;
	Vec4 idealSampleLoMax;

	// Get ideal samples at steps at each mipmap level

	if (filter == VK_FILTER_LINEAR)
	{
		// Adjust texel grid coordinates for linear filtering
		wrapTexelGridCoordLinear(baseTexelHi, texelGridOffsetHi, m_coordBits, m_imParams.dim);

		if (mipmapFilter == VK_SAMPLER_MIPMAP_MODE_LINEAR)
		{
			wrapTexelGridCoordLinear(baseTexelLo, texelGridOffsetLo, m_coordBits, m_imParams.dim);
		}

		const Vec3 roundedWeightsHi = texelGridOffsetHi.asFloat() / (float)coordSteps;
		const Vec3 roundedWeightsLo = texelGridOffsetLo.asFloat() / (float)coordSteps;

		report << "Computed weights: " << roundedWeightsHi << ", " << roundedWeightsLo << "\n";

	    getFilteredSample(baseTexelHi, roundedWeightsHi, layer, levelHi, idealSampleHiMin, idealSampleHiMax);

		report << "Ideal hi sample: " << idealSampleHiMin << " through " << idealSampleHiMax << "\n";

		if (mipmapFilter == VK_SAMPLER_MIPMAP_MODE_LINEAR)
		{
		    getFilteredSample(baseTexelLo, roundedWeightsLo, layer, levelLo, idealSampleLoMin, idealSampleLoMax);

			report << "Ideal lo sample: " << idealSampleLoMin << " through " << idealSampleLoMax << "\n";
		}
	}
	else
	{
	    fetchTexel(baseTexelHi, layer, levelHi, VK_FILTER_NEAREST, idealSampleHiMin, idealSampleHiMax);

		report << "Ideal hi sample: " << idealSampleHiMin << " through " << idealSampleHiMax << "\n";

		if (mipmapFilter == VK_SAMPLER_MIPMAP_MODE_LINEAR)
		{
		    fetchTexel(baseTexelLo, layer, levelLo, VK_FILTER_NEAREST, idealSampleLoMin, idealSampleLoMax);

			report << "Ideal lo sample: " << idealSampleLoMin << " through " << idealSampleLoMax << "\n";
		}
	}

	// Test ideal samples based on mipmap filtering mode

	if (mipmapFilter == VK_SAMPLER_MIPMAP_MODE_LINEAR)
	{
		for (deInt32 lodStep = lodStepsMin; lodStep <= lodStepsMax; ++lodStep)
		{
			float weight = (float)lodStep / (float)lodSteps;

			report << "Testing at mipmap weight " << weight << "\n";

			Vec4 idealSampleMin;
			Vec4 idealSampleMax;

			for (int compNdx = 0; compNdx < 4; ++compNdx)
			{
				const Interval idealSampleLo(false, idealSampleLoMin[compNdx], idealSampleLoMax[compNdx]);
				const Interval idealSampleHi(false, idealSampleHiMin[compNdx], idealSampleHiMax[compNdx]);

				const Interval idealSample
					= m_filteringPrecision.roundOut(Interval(weight) * idealSampleLo + Interval(1.0f - weight) * idealSampleHi, false);

				idealSampleMin[compNdx] = (float)idealSample.lo();
				idealSampleMax[compNdx] = (float)idealSample.hi();
			}

			report << "Ideal sample: " << idealSampleMin << " through " << idealSampleMax << "\n";

			if (isInRange(result, idealSampleMin, idealSampleMax))
			{
				return true;
			}
			else
			{
				report << "Failed comparison\n";
			}
		}
	}
	else
	{
		if (isInRange(result, idealSampleHiMin, idealSampleHiMax))
		{
			return true;
		}
		else
		{
			report << "Failed comparison\n";
		}
	}

	return false;
}

bool SampleVerifier::verifySampleTexelGridCoords (const SampleArguments&	args,
												  const Vec4&				result,
												  const IVec3&				gridCoordHi,
												  const IVec3&				gridCoordLo,
												  const Vec2&				lodBounds,
												  int						level,
												  VkSamplerMipmapMode		mipmapFilter,
												  std::ostream&				report) const
{
	const int	layer		 = m_imParams.isArrayed ? (int)deRoundEven(args.layer) : 0U;
	const IVec3 gridCoord[2] = {gridCoordHi, gridCoordLo};

	IVec3 baseTexel[2];
	IVec3 texelGridOffset[2];

    for (int levelNdx = 0; levelNdx < 2; ++levelNdx)
	{
		calcTexelBaseOffset(gridCoord[levelNdx], m_coordBits, baseTexel[levelNdx], texelGridOffset[levelNdx]);
	}

	const bool	canBeMinified  = lodBounds[1] > 0.0f;
	const bool	canBeMagnified = lodBounds[0] <= 0.0f;

	if (canBeMagnified)
	{
		report << "Trying magnification...\n";

		if (m_samplerParams.magFilter == VK_FILTER_NEAREST)
		{
			report << "Testing against nearest texel at " << baseTexel[0] << "\n";

			Vec4 idealMin;
			Vec4 idealMax;

			fetchTexel(baseTexel[0], layer, level, VK_FILTER_NEAREST, idealMin, idealMax);

			if (isInRange(result, idealMin, idealMax))
		    {
				return true;
			}
			else
			{
				report << "Failed against " << idealMin << " through " << idealMax << "\n";
			}
		}
		else
		{
			if  (verifySampleFiltered(result, baseTexel[0], baseTexel[1], texelGridOffset[0], texelGridOffset[1], layer, level, Vec2(0.0f, 0.0f), VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, report))
				return true;
		}
	}

	if (canBeMinified)
	{
		report << "Trying minification...\n";

		if (mipmapFilter == VK_SAMPLER_MIPMAP_MODE_LINEAR)
		{
			const Vec2 lodFracBounds = lodBounds - Vec2((float)level);

			if (verifySampleFiltered(result, baseTexel[0], baseTexel[1], texelGridOffset[0], texelGridOffset[1], layer, level, lodFracBounds, m_samplerParams.minFilter, VK_SAMPLER_MIPMAP_MODE_LINEAR, report))
				return true;
		}
		else if (m_samplerParams.minFilter == VK_FILTER_LINEAR)
		{
		    if (verifySampleFiltered(result, baseTexel[0], baseTexel[1], texelGridOffset[0], texelGridOffset[1], layer, level, Vec2(0.0f, 0.0f), VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, report))
				return true;
		}
		else
		{
			report << "Testing against nearest texel at " << baseTexel[0] << "\n";

			Vec4 idealMin;
			Vec4 idealMax;

		    fetchTexel(baseTexel[0], layer, level, VK_FILTER_NEAREST, idealMin, idealMax);

			if (isInRange(result, idealMin, idealMax))
		    {
				return true;
			}
			else
			{
				report << "Failed against " << idealMin << " through " << idealMax << "\n";
			}
		}
	}

	return false;
}

bool SampleVerifier::verifySampleMipmapLevel (const SampleArguments&	args,
											  const Vec4&				result,
											  const Vec4&				coord,
											  const Vec2&				lodBounds,
											  int						level,
											  std::ostream&				report) const
{
	DE_ASSERT(level < m_imParams.levels);

	VkSamplerMipmapMode mipmapFilter = m_samplerParams.mipmapFilter;

	if (level == m_imParams.levels - 1)
	{
		mipmapFilter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}

	Vec3	unnormalizedCoordMin[2];
	Vec3	unnormalizedCoordMax[2];
	IVec3	gridCoordMin[2];
	IVec3	gridCoordMax[2];

	const FloatFormat coordFormat(-32, 32, 16, true);

	calcUnnormalizedCoordRange(coord,
							   m_levels[level].getSize(),
							   coordFormat,
							   unnormalizedCoordMin[0],
							   unnormalizedCoordMax[0]);

	calcTexelGridCoordRange(unnormalizedCoordMin[0],
							unnormalizedCoordMax[0],
							m_coordBits,
							gridCoordMin[0],
							gridCoordMax[0]);

	report << "Level " << level << " computed unnormalized coordinate range: [" << unnormalizedCoordMin[0] << ", " << unnormalizedCoordMax[0] << "]\n";
	report << "Level " << level << " computed texel grid coordinate range: [" << gridCoordMin[0] << ", " << gridCoordMax[0] << "]\n";

	if (mipmapFilter == VK_SAMPLER_MIPMAP_MODE_LINEAR)
	{
		calcUnnormalizedCoordRange(coord,
								   m_levels[level+1].getSize(),
								   coordFormat,
								   unnormalizedCoordMin[1],
								   unnormalizedCoordMax[1]);

		calcTexelGridCoordRange(unnormalizedCoordMin[1],
								unnormalizedCoordMax[1],
								m_coordBits,
								gridCoordMin[1],
								gridCoordMax[1]);


		report << "Level " << level+1 << " computed unnormalized coordinate range: [" << unnormalizedCoordMin[1] << " - " << unnormalizedCoordMax[1] << "]\n";
		report << "Level " << level+1 << " computed texel grid coordinate range: [" << gridCoordMin[1] << " - " << gridCoordMax[1] << "]\n";
	}
	else
	{
		unnormalizedCoordMin[1] = unnormalizedCoordMax[1] = Vec3(0.0f);
		gridCoordMin[1] = gridCoordMax[1] = IVec3(0);
	}

	bool done = false;

	IVec3 gridCoord[2] = {gridCoordMin[0], gridCoordMin[1]};

    while (!done)
	{
		if (verifySampleTexelGridCoords(args, result, gridCoord[0], gridCoord[1], lodBounds, level, mipmapFilter, report))
			return true;

		// Get next grid coordinate to test at

		// Represents whether the increment at a position wraps and should "carry" to the next place
		bool carry = true;

		for (int levelNdx = 0; levelNdx < 2; ++levelNdx)
		{
			for (int compNdx = 0; compNdx < 3; ++compNdx)
			{
				if (carry)
				{
					deInt32& comp = gridCoord[levelNdx][compNdx];
				    ++comp;

					if (comp > gridCoordMax[levelNdx][compNdx])
					{
						comp = gridCoordMin[levelNdx][compNdx];
					}
					else
					{
						carry = false;
					}
				}
			}
		}

		done = carry;
	}

	return false;
}

bool SampleVerifier::verifySampleCubemapFace (const SampleArguments&	args,
											  const Vec4&				result,
											  const Vec4&				coord,
											  const Vec4&				dPdx,
											  const Vec4&				dPdy,
											  int						face,
											  std::ostream&				report) const
{
	// Will use this parameter once cubemapping is implemented completely
	DE_UNREF(face);

	Vec2 lodBounds;

	if (m_sampleLookupSettings.lookupLodMode == LOOKUP_LOD_MODE_DERIVATIVES)
	{
		float lodBias = m_samplerParams.lodBias;

		if (m_sampleLookupSettings.hasLodBias)
			lodBias += args.lodBias;

		lodBounds = calcLodBounds(dPdx.swizzle(0, 1, 2),
								  dPdy.swizzle(0, 1, 2),
								  m_imParams.size,
								  lodBias,
								  m_samplerParams.minLod,
								  m_samplerParams.maxLod);
	}
	else
	{
		lodBounds[0] = lodBounds[1] = args.lod;
	}

	DE_ASSERT(lodBounds[0] <= lodBounds[1]);

    const UVec2 levelBounds = calcLevelBounds(lodBounds, m_imParams.levels, m_samplerParams.mipmapFilter);

	for (deUint32 level = levelBounds[0]; level <= levelBounds[1]; ++level)
	{
		report << "Testing at mipmap level " << level << "...\n";

		const Vec2 levelLodBounds = calcLevelLodBounds(lodBounds, level);

		if (verifySampleMipmapLevel(args, result, coord, levelLodBounds, level, report))
		{
			return true;
		}

		report << "Done testing mipmap level " << level << ".\n\n";
	}

	return false;
}

bool SampleVerifier::verifySampleImpl (const SampleArguments&	args,
									   const Vec4&				result,
									   std::ostream&			report) const
{
	// \todo [2016-07-11 collinbaker] Handle depth and stencil formats
	// \todo [2016-07-06 collinbaker] Handle dRef
	DE_ASSERT(m_samplerParams.isCompare == false);

	Vec4	coord	  = args.coord;
	int coordSize = 0;

	if (m_imParams.dim == IMG_DIM_1D)
	{
		coordSize = 1;
	}
	else if (m_imParams.dim == IMG_DIM_2D)
	{
		coordSize = 2;
	}
	else if (m_imParams.dim == IMG_DIM_3D || m_imParams.dim == IMG_DIM_CUBE)
	{
		coordSize = 3;
	}

	// 15.6.1 Project operation

	if (m_sampleLookupSettings.isProjective)
	{
		DE_ASSERT(args.coord[coordSize] != 0.0f);
		const float proj = coord[coordSize];

		coord = coord / proj;
	}

	const Vec4 dPdx = (m_sampleLookupSettings.lookupLodMode == LOOKUP_LOD_MODE_DERIVATIVES) ? args.dPdx : Vec4(0);
	const Vec4 dPdy = (m_sampleLookupSettings.lookupLodMode == LOOKUP_LOD_MODE_DERIVATIVES) ? args.dPdy : Vec4(0);

	// 15.6.3 Cube Map Face Selection and Transformations

	if (m_imParams.dim == IMG_DIM_CUBE)
	{
		const Vec3	r		   = coord.swizzle(0, 1, 2);
		const Vec3	drdx	   = dPdx.swizzle(0, 1, 2);
		const Vec3	drdy	   = dPdy.swizzle(0, 1, 2);

	    int			faceBitmap = calcCandidateCubemapFaces(r);

		// We must test every possible disambiguation order

		for (int faceNdx = 0; faceNdx < 6; ++faceNdx)
		{
			const bool isPossible = ((faceBitmap & (1U << faceNdx)) != 0);

		    if (!isPossible)
			{
				continue;
			}

			Vec2 coordFace;
			Vec2 dPdxFace;
			Vec2 dPdyFace;

			calcCubemapFaceCoords(r, drdx, drdy, faceNdx, coordFace, dPdxFace, dPdyFace);

			if (verifySampleCubemapFace(args,
										result,
										Vec4(coordFace[0], coordFace[1], 0.0f, 0.0f),
										Vec4(dPdxFace[0], dPdxFace[1], 0.0f, 0.0f),
										Vec4(dPdyFace[0], dPdyFace[1], 0.0f, 0.0f),
										faceNdx,
										report))
			{
				return true;
			}
		}

		return false;
	}
	else
	{
		return verifySampleCubemapFace(args, result, coord, dPdx, dPdy, 0, report);
	}
}

bool SampleVerifier::verifySampleReport (const SampleArguments&	args,
										 const Vec4&			result,
										 std::string&			report) const
{
	std::ostringstream reportStream;

	const bool isValid = verifySampleImpl(args, result, reportStream);

	report = reportStream.str();

    return isValid;
}

bool SampleVerifier::verifySample (const SampleArguments&	args,
								   const Vec4&				result) const
{
	// Create unopened ofstream to simulate "null" ostream
	std::ofstream nullStream;

	return verifySampleImpl(args, result, nullStream);
}

} // texture
} // vkt
