/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Image load/store utilities
 *//*--------------------------------------------------------------------*/

#include "vktImageLoadStoreUtil.hpp"
#include "vkQueryUtil.hpp"

using namespace vk;

namespace vkt
{
namespace image
{

float computeStoreColorScale (const vk::VkFormat format, const tcu::IVec3 imageSize)
{
	const int maxImageDimension = de::max(imageSize.x(), de::max(imageSize.y(), imageSize.z()));
	const float div = static_cast<float>(maxImageDimension - 1);

	if (isUnormFormat(format))
		return 1.0f / div;
	else if (isSnormFormat(format))
		return 2.0f / div;
	else
		return 1.0f;
}

ImageType getImageTypeForSingleLayer (const ImageType imageType)
{
	switch (imageType)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_1D_ARRAY:
			return IMAGE_TYPE_1D;

		case IMAGE_TYPE_2D:
		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
			// A single layer for cube is a 2d face
			return IMAGE_TYPE_2D;

		case IMAGE_TYPE_3D:
			return IMAGE_TYPE_3D;

		case IMAGE_TYPE_BUFFER:
			return IMAGE_TYPE_BUFFER;

		default:
			DE_FATAL("Internal test error");
			return IMAGE_TYPE_LAST;
	}
}

VkImageCreateInfo makeImageCreateInfo (const Texture& texture, const VkFormat format, const VkImageUsageFlags usage, const VkImageCreateFlags flags)
{
	const VkSampleCountFlagBits samples = static_cast<VkSampleCountFlagBits>(texture.numSamples());	// integer and bit mask are aligned, so we can cast like this

	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,														// VkStructureType			sType;
		DE_NULL,																					// const void*				pNext;
		(isCube(texture) ? (VkImageCreateFlags)VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u) | flags,	// VkImageCreateFlags		flags;
		mapImageType(texture.type()),																// VkImageType				imageType;
		format,																						// VkFormat					format;
		makeExtent3D(texture.layerSize()),															// VkExtent3D				extent;
		1u,																							// deUint32					mipLevels;
		(deUint32)texture.numLayers(),																// deUint32					arrayLayers;
		samples,																					// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,																	// VkImageTiling			tiling;
		usage,																						// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,																	// VkSharingMode			sharingMode;
		0u,																							// deUint32					queueFamilyIndexCount;
		DE_NULL,																					// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,																	// VkImageLayout			initialLayout;
	};
	return imageParams;
}


//! Minimum chunk size is determined by the offset alignment requirements.
VkDeviceSize getOptimalUniformBufferChunkSize (const InstanceInterface& vki, const VkPhysicalDevice physDevice, VkDeviceSize minimumRequiredChunkSizeBytes)
{
	const VkPhysicalDeviceProperties properties = getPhysicalDeviceProperties(vki, physDevice);
	const VkDeviceSize alignment = properties.limits.minUniformBufferOffsetAlignment;

	if (minimumRequiredChunkSizeBytes > alignment)
		return alignment + (minimumRequiredChunkSizeBytes / alignment) * alignment;
	else
		return alignment;
}

bool isStorageImageExtendedFormat (const vk::VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_UINT:
			return true;

		default:
			return false;
	}
}

} // image
} // vkt
