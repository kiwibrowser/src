/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Texture utility class
 *//*--------------------------------------------------------------------*/

#include "vktImageTexture.hpp"

namespace vkt
{
namespace image
{

void Texture::checkInvariants (void) const
{
	DE_ASSERT((m_numSamples == 1)  || (m_numSamples == 2)  || (m_numSamples == 4) || (m_numSamples == 8) ||
			  (m_numSamples == 16) || (m_numSamples == 32) || (m_numSamples == 64));
	DE_ASSERT(m_numLayers >= 1);
	DE_ASSERT(m_layerSize.x() >= 1 && m_layerSize.y() >= 1 && m_layerSize.z() >= 1);

	switch (m_type)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_BUFFER:
			DE_ASSERT(m_numLayers == 1);
			DE_ASSERT(m_numSamples == 1);
			DE_ASSERT(m_layerSize.y() == 1 && m_layerSize.z() == 1);
			break;

		case IMAGE_TYPE_1D_ARRAY:
			DE_ASSERT(m_numSamples == 1);
			DE_ASSERT(m_layerSize.y() == 1 && m_layerSize.z() == 1);
			break;

		case IMAGE_TYPE_2D:
			DE_ASSERT(m_numLayers == 1);
			DE_ASSERT(m_layerSize.z() == 1);
			break;

		case IMAGE_TYPE_2D_ARRAY:
			DE_ASSERT(m_layerSize.z() == 1);
			break;

		case IMAGE_TYPE_CUBE:
			DE_ASSERT(m_numSamples == 1);
			DE_ASSERT(m_numLayers == 6);
			DE_ASSERT(m_layerSize.z() == 1);
			break;

		case IMAGE_TYPE_CUBE_ARRAY:
			DE_ASSERT(m_numSamples == 1);
			DE_ASSERT(m_numLayers >= 6 && m_numLayers % 6 == 0);
			DE_ASSERT(m_layerSize.z() == 1);
			break;

		case IMAGE_TYPE_3D:
			DE_ASSERT(m_numSamples == 1);
			DE_ASSERT(m_numLayers == 1);
			break;

		default:
			DE_FATAL("Internal error");
			break;
	}
}

Texture::Texture (const ImageType imageType, const tcu::IVec3& imageLayerSize, const int layers, const int samples)
	: m_layerSize	(imageLayerSize)
	, m_type		(imageType)
	, m_numLayers	(layers)
	, m_numSamples	(samples)
{
	checkInvariants();
}

Texture::Texture (const Texture& other, const int samples)
	: m_layerSize	(other.m_layerSize)
	, m_type		(other.m_type)
	, m_numLayers	(other.m_numLayers)
	, m_numSamples	(samples)
{
	checkInvariants();
}

tcu::IVec3 Texture::size (void) const
{
	switch (m_type)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_BUFFER:
		case IMAGE_TYPE_2D:
		case IMAGE_TYPE_3D:
			return m_layerSize;

		case IMAGE_TYPE_1D_ARRAY:
			return tcu::IVec3(m_layerSize.x(), m_numLayers, 1);

		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
			return tcu::IVec3(m_layerSize.x(), m_layerSize.y(), m_numLayers);

		default:
			DE_FATAL("Internal error");
			return tcu::IVec3();
	}
}

int Texture::dimension (void) const
{
	switch (m_type)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_BUFFER:
			return 1;

		case IMAGE_TYPE_1D_ARRAY:
		case IMAGE_TYPE_2D:
			return 2;

		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
		case IMAGE_TYPE_3D:
			return 3;

		default:
			DE_FATAL("Internal error");
			return 0;
	}
}

int Texture::layerDimension (void) const
{
	switch (m_type)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_BUFFER:
		case IMAGE_TYPE_1D_ARRAY:
			return 1;

		case IMAGE_TYPE_2D:
		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
			return 2;

		case IMAGE_TYPE_3D:
			return 3;

		default:
			DE_FATAL("Internal error");
			return 0;
	}
}

} // image
} // vkt
