/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Imagination Technologies Ltd.
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
 * \brief Utilities for vertex buffers.
 *//*--------------------------------------------------------------------*/

#include "vktPipelineVertexUtil.hpp"
#include "vkStrUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "deStringUtil.hpp"

namespace vkt
{
namespace pipeline
{

using namespace vk;

deUint32 getVertexFormatSize (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_R4G4_UNORM_PACK8:
			return 1;

		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
			return 2;

		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
			return 3;

		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
			return 4;

		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16_SFLOAT:
			return 6;

		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R64_SFLOAT:
			return 8;

		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32_SFLOAT:
			return 12;

		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		case VK_FORMAT_R64G64_SFLOAT:
			return 16;

		case VK_FORMAT_R64G64B64_SFLOAT:
			return 24;

		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return 32;

		default:
			break;
	}

	DE_ASSERT(false);
	return 0;
}

deUint32 getVertexFormatComponentCount (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R64_SFLOAT:
			return 1;

		case VK_FORMAT_R4G4_UNORM_PACK8:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R64G64_SFLOAT:
			return 2;

		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16_SFLOAT:
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32_SFLOAT:
		case VK_FORMAT_R64G64B64_SFLOAT:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
			return 3;

		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
			return 4;

		default:
			break;
	}

	DE_ASSERT(false);
	return 0;
}

deUint32 getVertexFormatComponentSize (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
			return 1;

		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16_SFLOAT:
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return 2;

		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32_SFLOAT:
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 4;

		case VK_FORMAT_R64_SFLOAT:
		case VK_FORMAT_R64G64_SFLOAT:
		case VK_FORMAT_R64G64B64_SFLOAT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return 8;

		default:
			break;
	}

	DE_ASSERT(false);
	return 0;
}

bool isVertexFormatComponentOrderBGR (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
			return true;

		default:
			break;
	}
	return false;
}

bool isVertexFormatSint (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32A32_SINT:
			return true;

		default:
			break;
	}

	return false;
}

bool isVertexFormatUint (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32A32_UINT:
			return true;

		default:
			break;
	}

	return false;

}

bool isVertexFormatSfloat (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R16G16B16_SFLOAT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R64_SFLOAT:
		case VK_FORMAT_R32G32B32_SFLOAT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		case VK_FORMAT_R64G64_SFLOAT:
		case VK_FORMAT_R64G64B64_SFLOAT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return true;

		default:
			break;
	}

	return false;

}

bool isVertexFormatUfloat (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			return true;

		default:
			break;
	}

	return false;

}

bool isVertexFormatUnorm (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R4G4_UNORM_PACK8:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16A16_UNORM:
			return true;

		default:
			break;
	}

	return false;

}

bool isVertexFormatSnorm (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
			return true;

		default:
			break;
	}

	return false;

}

bool isVertexFormatSRGB (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_SRGB:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_SRGB:
			return true;

		default:
			break;
	}

	return false;

}

bool isVertexFormatSscaled (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
			return true;

		default:
			break;
	}

	return false;

}

bool isVertexFormatUscaled (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16A16_USCALED:
			return true;

		default:
			break;
	}

	return false;

}

bool isVertexFormatDouble (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R64_UINT:
		case VK_FORMAT_R64_SINT:
		case VK_FORMAT_R64_SFLOAT:
		case VK_FORMAT_R64G64_UINT:
		case VK_FORMAT_R64G64_SINT:
		case VK_FORMAT_R64G64_SFLOAT:
		case VK_FORMAT_R64G64B64_UINT:
		case VK_FORMAT_R64G64B64_SINT:
		case VK_FORMAT_R64G64B64_SFLOAT:
		case VK_FORMAT_R64G64B64A64_UINT:
		case VK_FORMAT_R64G64B64A64_SINT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return true;

		default:
			break;
	}
	return false;
}

std::vector<Vertex4RGBA> createOverlappingQuads (void)
{
	using tcu::Vec2;
	using tcu::Vec4;

	std::vector<Vertex4RGBA> vertices;

	const Vec2 translations[4] =
	{
		Vec2(-0.25f, -0.25f),
		Vec2(-1.0f, -0.25f),
		Vec2(-1.0f, -1.0f),
		Vec2(-0.25f, -1.0f)
	};

	const Vec4 quadColors[4] =
	{
		Vec4(1.0f, 0.0f, 0.0f, 1.0),
		Vec4(0.0f, 1.0f, 0.0f, 1.0),
		Vec4(0.0f, 0.0f, 1.0f, 1.0),
		Vec4(1.0f, 0.0f, 1.0f, 1.0)
	};

	const float quadSize = 1.25f;

	for (int quadNdx = 0; quadNdx < 4; quadNdx++)
	{
		const Vec2&	translation	= translations[quadNdx];
		const Vec4&	color		= quadColors[quadNdx];

		const Vertex4RGBA lowerLeftVertex =
		{
			Vec4(translation.x(), translation.y(), 0.0f, 1.0f),
			color
		};
		const Vertex4RGBA upperLeftVertex =
		{
			Vec4(translation.x(), translation.y() + quadSize, 0.0f, 1.0f),
			color
		};
		const Vertex4RGBA lowerRightVertex =
		{
			Vec4(translation.x() + quadSize, translation.y(), 0.0f, 1.0f),
			color
		};
		const Vertex4RGBA upperRightVertex =
		{
			Vec4(translation.x() + quadSize, translation.y() + quadSize, 0.0f, 1.0f),
			color
		};

		// Triangle 1, CCW
		vertices.push_back(lowerLeftVertex);
		vertices.push_back(lowerRightVertex);
		vertices.push_back(upperLeftVertex);

		// Triangle 2, CW
		vertices.push_back(lowerRightVertex);
		vertices.push_back(upperLeftVertex);
		vertices.push_back(upperRightVertex);
	}

	return vertices;
}

std::vector<Vertex4Tex4> createFullscreenQuad (void)
{
	using tcu::Vec4;

	const Vertex4Tex4 lowerLeftVertex =
	{
		Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		Vec4(0.0f, 0.0f, 0.0f, 0.0f)
	};
	const Vertex4Tex4 upperLeftVertex =
	{
		Vec4(-1.0f, 1.0f, 0.0f, 1.0f),
		Vec4(0.0f, 1.0f, 0.0f, 0.0f)
	};
	const Vertex4Tex4 lowerRightVertex =
	{
		Vec4(1.0f, -1.0f, 0.0f, 1.0f),
		Vec4(1.0f, 0.0f, 0.0f, 0.0f)
	};
	const Vertex4Tex4 upperRightVertex =
	{
		Vec4(1.0f, 1.0f, 0.0f, 1.0f),
		Vec4(1.0f, 1.0f, 0.0f, 0.0f)
	};

	const Vertex4Tex4 vertices[6] =
	{
		lowerLeftVertex,
		lowerRightVertex,
		upperLeftVertex,

		upperLeftVertex,
		lowerRightVertex,
		upperRightVertex
	};

	return std::vector<Vertex4Tex4>(vertices, vertices + DE_LENGTH_OF_ARRAY(vertices));
}

std::vector<Vertex4Tex4> createQuadMosaic (int rows, int columns)
{
	using tcu::Vec4;

	DE_ASSERT(rows >= 1);
	DE_ASSERT(columns >= 1);

	std::vector<Vertex4Tex4>	vertices;
	const float					rowSize		= 2.0f / (float)rows;
	const float					columnSize	= 2.0f / (float)columns;
	int							arrayIndex	= 0;

	for (int rowNdx = 0; rowNdx < rows; rowNdx++)
	{
		for (int columnNdx = 0; columnNdx < columns; columnNdx++)
		{
			const Vertex4Tex4 lowerLeftVertex =
			{
				Vec4(-1.0f + (float)columnNdx * columnSize, -1.0f + (float)rowNdx * rowSize, 0.0f, 1.0f),
				Vec4(0.0f, 0.0f, (float)arrayIndex, 0.0f)
			};
			const Vertex4Tex4 upperLeftVertex =
			{
				Vec4(lowerLeftVertex.position.x(), lowerLeftVertex.position.y() + rowSize, 0.0f, 1.0f),
				Vec4(0.0f, 1.0f, (float)arrayIndex, 0.0f)
			};
			const Vertex4Tex4 lowerRightVertex =
			{
				Vec4(lowerLeftVertex.position.x() + columnSize, lowerLeftVertex.position.y(), 0.0f, 1.0f),
				Vec4(1.0f, 0.0f, (float)arrayIndex, 0.0f)
			};
			const Vertex4Tex4 upperRightVertex =
			{
				Vec4(lowerLeftVertex.position.x() + columnSize, lowerLeftVertex.position.y() + rowSize, 0.0f, 1.0f),
				Vec4(1.0f, 1.0f, (float)arrayIndex, 0.0f)
			};

			vertices.push_back(lowerLeftVertex);
			vertices.push_back(lowerRightVertex);
			vertices.push_back(upperLeftVertex);
			vertices.push_back(upperLeftVertex);
			vertices.push_back(lowerRightVertex);
			vertices.push_back(upperRightVertex);

			arrayIndex++;
		}
	}

	return vertices;
}

std::vector<Vertex4Tex4> createQuadMosaicCube (void)
{
	using tcu::Vec3;

	static const Vec3 texCoordsCube[8] =
	{
		Vec3(-1.0f, -1.0f, -1.0f),	// 0: -X, -Y, -Z
		Vec3(1.0f, -1.0f, -1.0f),	// 1:  X, -Y, -Z
		Vec3(1.0f, -1.0f, 1.0f),	// 2:  X, -Y,  Z
		Vec3(-1.0f, -1.0f, 1.0f),	// 3: -X, -Y,  Z

		Vec3(-1.0f, 1.0f, -1.0f),	// 4: -X,  Y, -Z
		Vec3(1.0f, 1.0f, -1.0f),	// 5:  X,  Y, -Z
		Vec3(1.0f, 1.0f, 1.0f),		// 6:  X,  Y,  Z
		Vec3(-1.0f, 1.0f, 1.0f),	// 7: -X,  Y,  Z
	};

	static const int texCoordCubeIndices[6][6] =
	{
		{ 6, 5, 2, 2, 5, 1 },		// +X face
		{ 3, 0, 7, 7, 0, 4 },		// -X face
		{ 4, 5, 7, 7, 5, 6 },		// +Y face
		{ 3, 2, 0, 0, 2, 1 },		// -Y face
		{ 2, 3, 6, 6, 3, 7 },		// +Z face
		{ 0, 1, 4, 4, 1, 5 }		// -Z face
	};

	// Create 6 quads and set appropriate texture coordinates for cube mapping

	std::vector<Vertex4Tex4>			vertices	= createQuadMosaic(2, 3);
	std::vector<Vertex4Tex4>::iterator	vertexItr	= vertices.begin();

	for (int quadNdx = 0; quadNdx < 6; quadNdx++)
	{
		for (int vertexNdx = 0; vertexNdx < 6; vertexNdx++)
		{
			vertexItr->texCoord.xyz() = texCoordsCube[texCoordCubeIndices[quadNdx][vertexNdx]];
			vertexItr++;
		}
	}

	return vertices;
}

std::vector<Vertex4Tex4> createQuadMosaicCubeArray (int faceArrayIndices[6])
{
	std::vector<Vertex4Tex4>			vertices	= createQuadMosaicCube();
	std::vector<Vertex4Tex4>::iterator	vertexItr	= vertices.begin();

	for (int quadNdx = 0; quadNdx < 6; quadNdx++)
	{
		for (int vertexNdx = 0; vertexNdx < 6; vertexNdx++)
		{
			vertexItr->texCoord.w() = (float)faceArrayIndices[quadNdx];
			vertexItr++;
		}
	}

	return vertices;
}

std::vector<Vertex4Tex4> createTestQuadMosaic (vk::VkImageViewType viewType)
{
	std::vector<Vertex4Tex4> vertices;

	switch (viewType)
	{
		case vk::VK_IMAGE_VIEW_TYPE_1D:
		case vk::VK_IMAGE_VIEW_TYPE_2D:
			vertices = createFullscreenQuad();
			break;

		case vk::VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			vertices = createQuadMosaic(2, 3);

			// Set up array indices
			for (size_t quadNdx = 0; quadNdx < 6; quadNdx++)
				for (size_t vertexNdx = 0; vertexNdx < 6; vertexNdx++)
					vertices[quadNdx * 6 + vertexNdx].texCoord.y() = (float)quadNdx;

			break;

		case vk::VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			vertices = createQuadMosaic(2, 3);
			break;

		case vk::VK_IMAGE_VIEW_TYPE_3D:
			vertices = createQuadMosaic(2, 3);

			// Use z between 0.0 and 1.0.
			for (size_t vertexNdx = 0; vertexNdx < vertices.size(); vertexNdx++)
			{
				vertices[vertexNdx].texCoord.z() /= 5.0f;
				vertices[vertexNdx].texCoord.z() -= 0.001f; // Substract small value to correct floating-point errors at the boundaries between slices
			}

			break;

		case vk::VK_IMAGE_VIEW_TYPE_CUBE:
			vertices = createQuadMosaicCube();
			break;

		case vk::VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			{
				int faceArrayIndices[6] = { 0, 1, 2, 3, 4, 5 };
				vertices = createQuadMosaicCubeArray(faceArrayIndices);
			}
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	return vertices;
}

} // pipeline
} // vkt
