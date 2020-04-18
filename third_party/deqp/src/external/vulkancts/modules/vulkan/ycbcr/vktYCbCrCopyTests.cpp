/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 Google Inc.
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
 * \brief YCbCr format copy tests
 *//*--------------------------------------------------------------------*/

#include "vktYCbCrCopyTests.hpp"

#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktYCbCrUtil.hpp"

#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"

#include "tcuSeedBuilder.hpp"
#include "tcuTestLog.hpp"
#include "tcuVector.hpp"
#include "tcuVectorUtil.hpp"

#include "deRandom.hpp"
#include "deSTLUtil.hpp"
#include "deStringUtil.hpp"

#include <string>
#include <utility>
#include <vector>

using tcu::UVec2;
using tcu::Vec4;
using tcu::TestLog;

using std::string;
using std::vector;
using std::pair;

namespace vkt
{
namespace ycbcr
{
namespace
{
typedef de::SharedPtr<vk::Allocation> AllocationSp;

struct ImageConfig
{
						ImageConfig	(vk::VkFormat		format_,
									 vk::VkImageTiling	tiling_,
									 bool				disjoint_,
									 const UVec2&		size_)
		: format	(format_)
		, tiling	(tiling_)
		, disjoint	(disjoint_)
		, size		(size_)
	{
	}

	vk::VkFormat		format;
	vk::VkImageTiling	tiling;
	bool				disjoint;
	UVec2				size;
};

struct TestConfig
{
				TestConfig		(const ImageConfig&	src_,
								 const ImageConfig&	dst_)
		: src	(src_)
		, dst	(dst_)
	{
	}

	ImageConfig	src;
	ImageConfig	dst;
};

void checkSupport (Context& context, const TestConfig& config)
{
	if (!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), string("VK_KHR_sampler_ycbcr_conversion")))
		TCU_THROW(NotSupportedError, "Extension VK_KHR_sampler_ycbcr_conversion not supported");

	try
	{
		const vk::VkFormatProperties	properties	(vk::getPhysicalDeviceFormatProperties(context.getInstanceInterface(), context.getPhysicalDevice(), config.src.format));
		const vk::VkFormatFeatureFlags	features	(config.src.tiling == vk::VK_IMAGE_TILING_OPTIMAL
													? properties.optimalTilingFeatures
													: properties.linearTilingFeatures);

		if ((features & vk::VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR) == 0
			&& (features & vk::VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR) == 0)
		{
			TCU_THROW(NotSupportedError, "Source format doesn't support copies");
		}

		if (config.src.disjoint && ((features & vk::VK_FORMAT_FEATURE_DISJOINT_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn'tsupport  disjoint planes");
	}
	catch (const vk::Error& err)
	{
		if (err.getError() == vk::VK_ERROR_FORMAT_NOT_SUPPORTED)
			TCU_THROW(NotSupportedError, "Format not supported");

		throw;
	}

	try
	{
		const vk::VkFormatProperties	properties	(vk::getPhysicalDeviceFormatProperties(context.getInstanceInterface(), context.getPhysicalDevice(), config.dst.format));
		const vk::VkFormatFeatureFlags	features	(config.dst.tiling == vk::VK_IMAGE_TILING_OPTIMAL
													? properties.optimalTilingFeatures
													: properties.linearTilingFeatures);

		if ((features & vk::VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR) == 0
			&& (features & vk::VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR) == 0)
		{
			TCU_THROW(NotSupportedError, "Source format doesn't support copies");
		}

		if (config.dst.disjoint && ((features & vk::VK_FORMAT_FEATURE_DISJOINT_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Format doesn't disjoint planes");
	}
	catch (const vk::Error& err)
	{
		if (err.getError() == vk::VK_ERROR_FORMAT_NOT_SUPPORTED)
			TCU_THROW(NotSupportedError, "Format not supported");

		throw;
	}
}

vk::Move<vk::VkImage> createImage (const vk::DeviceInterface&	vkd,
								   vk::VkDevice					device,
								   vk::VkFormat					format,
								   const UVec2&					size,
								   bool							disjoint,
								   vk::VkImageTiling			tiling)
{
	const vk::VkImageCreateInfo createInfo =
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		DE_NULL,
		disjoint ? (vk::VkImageCreateFlags)vk::VK_IMAGE_CREATE_DISJOINT_BIT_KHR : (vk::VkImageCreateFlags)0u,

		vk::VK_IMAGE_TYPE_2D,
		format,
		vk::makeExtent3D(size.x(), size.y(), 1u),
		1u,
		1u,
		vk::VK_SAMPLE_COUNT_1_BIT,
		tiling,
		vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT | vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		vk::VK_SHARING_MODE_EXCLUSIVE,
		0u,
		(const deUint32*)DE_NULL,
		vk::VK_IMAGE_LAYOUT_UNDEFINED,
	};

	return vk::createImage(vkd, device, &createInfo);
}

vk::VkFormat getPlaneCompatibleFormat (vk::VkFormat format, deUint32 planeNdx)
{
	DE_ASSERT(planeNdx < 3);

	switch (format)
	{
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
			return vk::VK_FORMAT_R8_UNORM;

		case vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		{
			DE_ASSERT(planeNdx < 2);

			if (planeNdx == 0)
				return vk::VK_FORMAT_R8_UNORM;
			else
				return vk::VK_FORMAT_R8G8_UNORM;
		}

		case vk::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
			return vk::VK_FORMAT_R8_UNORM;

		case vk::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		{
			DE_ASSERT(planeNdx < 2);

			if (planeNdx == 0)
				return vk::VK_FORMAT_R8_UNORM;
			else
				return vk::VK_FORMAT_R8G8_UNORM;
		}

		case vk::VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
			return vk::VK_FORMAT_R8_UNORM;

		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
			return vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR;

		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		{
			DE_ASSERT(planeNdx < 2);

			if (planeNdx == 0)
				return vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR;
			else
				return vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR;
		}

		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
			return vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR;

		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		{
			DE_ASSERT(planeNdx < 2);

			if (planeNdx == 0)
				return vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR;
			else
				return vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR;
		}

		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
			return vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR;

		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
			return vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR;

		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		{
			DE_ASSERT(planeNdx < 2);

			if (planeNdx == 0)
				return vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR;
			else
				return vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR;
		}

		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
			return vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR;

		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		{
			DE_ASSERT(planeNdx < 2);

			if (planeNdx == 0)
				return vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR;
			else
				return vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR;
		}

		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
			return vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR;

		case vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
			return vk::VK_FORMAT_R16_UNORM;

		case vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		{
			DE_ASSERT(planeNdx < 2);

			if (planeNdx == 0)
				return vk::VK_FORMAT_R16_UNORM;
			else
				return vk::VK_FORMAT_R16G16_UNORM;
		}

		case vk::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
			return vk::VK_FORMAT_R16_UNORM;

		case vk::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
		{
			DE_ASSERT(planeNdx < 2);

			if (planeNdx == 0)
				return vk::VK_FORMAT_R16_UNORM;
			else
				return vk::VK_FORMAT_R16G16_UNORM;
		}

		case vk::VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
			return vk::VK_FORMAT_R16_UNORM;

		default:
			DE_ASSERT(planeNdx == 0);
			return format;
	}
}

bool isCompatible (vk::VkFormat	srcFormat,
				   vk::VkFormat	dstFormat)
{
	if (srcFormat == dstFormat)
		return true;
	else
	{
		const vk::VkFormat class8Bit[] =
		{
			vk::VK_FORMAT_R4G4_UNORM_PACK8,
			vk::VK_FORMAT_R8_UNORM,
			vk::VK_FORMAT_R8_SNORM,
			vk::VK_FORMAT_R8_USCALED,
			vk::VK_FORMAT_R8_SSCALED,
			vk::VK_FORMAT_R8_UINT,
			vk::VK_FORMAT_R8_SINT,
			vk::VK_FORMAT_R8_SRGB
		};
		const vk::VkFormat class16Bit[] =
		{
			vk::VK_FORMAT_R4G4B4A4_UNORM_PACK16,
			vk::VK_FORMAT_B4G4R4A4_UNORM_PACK16,
			vk::VK_FORMAT_R5G6B5_UNORM_PACK16,
			vk::VK_FORMAT_B5G6R5_UNORM_PACK16,
			vk::VK_FORMAT_R5G5B5A1_UNORM_PACK16,
			vk::VK_FORMAT_B5G5R5A1_UNORM_PACK16,
			vk::VK_FORMAT_A1R5G5B5_UNORM_PACK16,
			vk::VK_FORMAT_R8G8_UNORM,
			vk::VK_FORMAT_R8G8_SNORM,
			vk::VK_FORMAT_R8G8_USCALED,
			vk::VK_FORMAT_R8G8_SSCALED,
			vk::VK_FORMAT_R8G8_UINT,
			vk::VK_FORMAT_R8G8_SINT,
			vk::VK_FORMAT_R8G8_SRGB,
			vk::VK_FORMAT_R16_UNORM,
			vk::VK_FORMAT_R16_SNORM,
			vk::VK_FORMAT_R16_USCALED,
			vk::VK_FORMAT_R16_SSCALED,
			vk::VK_FORMAT_R16_UINT,
			vk::VK_FORMAT_R16_SINT,
			vk::VK_FORMAT_R16_SFLOAT,
			vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR,
			vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR
		};
		const vk::VkFormat class24Bit[] =
		{
			vk::VK_FORMAT_R8G8B8_UNORM,
			vk::VK_FORMAT_R8G8B8_SNORM,
			vk::VK_FORMAT_R8G8B8_USCALED,
			vk::VK_FORMAT_R8G8B8_SSCALED,
			vk::VK_FORMAT_R8G8B8_UINT,
			vk::VK_FORMAT_R8G8B8_SINT,
			vk::VK_FORMAT_R8G8B8_SRGB,
			vk::VK_FORMAT_B8G8R8_UNORM,
			vk::VK_FORMAT_B8G8R8_SNORM,
			vk::VK_FORMAT_B8G8R8_USCALED,
			vk::VK_FORMAT_B8G8R8_SSCALED,
			vk::VK_FORMAT_B8G8R8_UINT,
			vk::VK_FORMAT_B8G8R8_SINT,
			vk::VK_FORMAT_B8G8R8_SRGB
		};
		const vk::VkFormat class32Bit[] =
		{
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			vk::VK_FORMAT_R8G8B8A8_SNORM,
			vk::VK_FORMAT_R8G8B8A8_USCALED,
			vk::VK_FORMAT_R8G8B8A8_SSCALED,
			vk::VK_FORMAT_R8G8B8A8_UINT,
			vk::VK_FORMAT_R8G8B8A8_SINT,
			vk::VK_FORMAT_R8G8B8A8_SRGB,
			vk::VK_FORMAT_B8G8R8A8_UNORM,
			vk::VK_FORMAT_B8G8R8A8_SNORM,
			vk::VK_FORMAT_B8G8R8A8_USCALED,
			vk::VK_FORMAT_B8G8R8A8_SSCALED,
			vk::VK_FORMAT_B8G8R8A8_UINT,
			vk::VK_FORMAT_B8G8R8A8_SINT,
			vk::VK_FORMAT_B8G8R8A8_SRGB,
			vk::VK_FORMAT_A8B8G8R8_UNORM_PACK32,
			vk::VK_FORMAT_A8B8G8R8_SNORM_PACK32,
			vk::VK_FORMAT_A8B8G8R8_USCALED_PACK32,
			vk::VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
			vk::VK_FORMAT_A8B8G8R8_UINT_PACK32,
			vk::VK_FORMAT_A8B8G8R8_SINT_PACK32,
			vk::VK_FORMAT_A8B8G8R8_SRGB_PACK32,
			vk::VK_FORMAT_A2R10G10B10_UNORM_PACK32,
			vk::VK_FORMAT_A2R10G10B10_SNORM_PACK32,
			vk::VK_FORMAT_A2R10G10B10_USCALED_PACK32,
			vk::VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
			vk::VK_FORMAT_A2R10G10B10_UINT_PACK32,
			vk::VK_FORMAT_A2R10G10B10_SINT_PACK32,
			vk::VK_FORMAT_A2B10G10R10_UNORM_PACK32,
			vk::VK_FORMAT_A2B10G10R10_SNORM_PACK32,
			vk::VK_FORMAT_A2B10G10R10_USCALED_PACK32,
			vk::VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
			vk::VK_FORMAT_A2B10G10R10_UINT_PACK32,
			vk::VK_FORMAT_A2B10G10R10_SINT_PACK32,
			vk::VK_FORMAT_R16G16_UNORM,
			vk::VK_FORMAT_R16G16_SNORM,
			vk::VK_FORMAT_R16G16_USCALED,
			vk::VK_FORMAT_R16G16_SSCALED,
			vk::VK_FORMAT_R16G16_UINT,
			vk::VK_FORMAT_R16G16_SINT,
			vk::VK_FORMAT_R16G16_SFLOAT,
			vk::VK_FORMAT_R32_UINT,
			vk::VK_FORMAT_R32_SINT,
			vk::VK_FORMAT_R32_SFLOAT,
			vk::VK_FORMAT_B10G11R11_UFLOAT_PACK32,
			vk::VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
			vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR,
			vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR
		};
		const vk::VkFormat class48Bit[] =
		{
			vk::VK_FORMAT_R16G16B16_UNORM,
			vk::VK_FORMAT_R16G16B16_SNORM,
			vk::VK_FORMAT_R16G16B16_USCALED,
			vk::VK_FORMAT_R16G16B16_SSCALED,
			vk::VK_FORMAT_R16G16B16_UINT,
			vk::VK_FORMAT_R16G16B16_SINT,
			vk::VK_FORMAT_R16G16B16_SFLOAT
		};
		const vk::VkFormat class64Bit[] =
		{
			vk::VK_FORMAT_R16G16B16A16_UNORM,
			vk::VK_FORMAT_R16G16B16A16_SNORM,
			vk::VK_FORMAT_R16G16B16A16_USCALED,
			vk::VK_FORMAT_R16G16B16A16_SSCALED,
			vk::VK_FORMAT_R16G16B16A16_UINT,
			vk::VK_FORMAT_R16G16B16A16_SINT,
			vk::VK_FORMAT_R16G16B16A16_SFLOAT,
			vk::VK_FORMAT_R32G32_UINT,
			vk::VK_FORMAT_R32G32_SINT,
			vk::VK_FORMAT_R32G32_SFLOAT,
			vk::VK_FORMAT_R64_UINT,
			vk::VK_FORMAT_R64_SINT,
			vk::VK_FORMAT_R64_SFLOAT
		};
		const vk::VkFormat class96Bit[] =
		{
			vk::VK_FORMAT_R32G32B32_UINT,
			vk::VK_FORMAT_R32G32B32_SINT,
			vk::VK_FORMAT_R32G32B32_SFLOAT
		};
		const vk::VkFormat class128Bit[] =
		{
			vk::VK_FORMAT_R32G32B32A32_UINT,
			vk::VK_FORMAT_R32G32B32A32_SINT,
			vk::VK_FORMAT_R32G32B32A32_SFLOAT,
			vk::VK_FORMAT_R64G64_UINT,
			vk::VK_FORMAT_R64G64_SINT,
			vk::VK_FORMAT_R64G64_SFLOAT
		};
		const vk::VkFormat class192Bit[] =
		{
			vk::VK_FORMAT_R64G64B64_UINT,
			vk::VK_FORMAT_R64G64B64_SINT,
			vk::VK_FORMAT_R64G64B64_SFLOAT
		};
		const vk::VkFormat class256Bit[] =
		{
			vk::VK_FORMAT_R64G64B64A64_UINT,
			vk::VK_FORMAT_R64G64B64A64_SINT,
			vk::VK_FORMAT_R64G64B64A64_SFLOAT
		};

		if (de::contains(DE_ARRAY_BEGIN(class8Bit), DE_ARRAY_END(class8Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class8Bit), DE_ARRAY_END(class8Bit), dstFormat))
			return true;

		if (de::contains(DE_ARRAY_BEGIN(class16Bit), DE_ARRAY_END(class16Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class16Bit), DE_ARRAY_END(class16Bit), dstFormat))
			return true;

		if (de::contains(DE_ARRAY_BEGIN(class24Bit), DE_ARRAY_END(class24Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class24Bit), DE_ARRAY_END(class24Bit), dstFormat))
			return true;

		if (de::contains(DE_ARRAY_BEGIN(class32Bit), DE_ARRAY_END(class32Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class32Bit), DE_ARRAY_END(class32Bit), dstFormat))
			return true;

		if (de::contains(DE_ARRAY_BEGIN(class48Bit), DE_ARRAY_END(class48Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class48Bit), DE_ARRAY_END(class48Bit), dstFormat))
			return true;

		if (de::contains(DE_ARRAY_BEGIN(class64Bit), DE_ARRAY_END(class64Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class64Bit), DE_ARRAY_END(class64Bit), dstFormat))
			return true;

		if (de::contains(DE_ARRAY_BEGIN(class96Bit), DE_ARRAY_END(class96Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class96Bit), DE_ARRAY_END(class96Bit), dstFormat))
			return true;

		if (de::contains(DE_ARRAY_BEGIN(class128Bit), DE_ARRAY_END(class128Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class128Bit), DE_ARRAY_END(class128Bit), dstFormat))
			return true;

		if (de::contains(DE_ARRAY_BEGIN(class192Bit), DE_ARRAY_END(class192Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class192Bit), DE_ARRAY_END(class192Bit), dstFormat))
			return true;

		if (de::contains(DE_ARRAY_BEGIN(class256Bit), DE_ARRAY_END(class256Bit), srcFormat)
			&& de::contains(DE_ARRAY_BEGIN(class256Bit), DE_ARRAY_END(class256Bit), dstFormat))
			return true;

		return false;
	}
}

UVec2 getBlockSize (vk::VkFormat format)
{
	switch (format)
	{
		case vk::VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
		case vk::VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case vk::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case vk::VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
			return UVec2(2, 1);

		default:
			return UVec2(1u, 1u);
	}
}

deUint32 getBlockByteSize (vk::VkFormat format)
{
	switch (format)
	{
		case vk::VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case vk::VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
			return 4u;

		case vk::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
		case vk::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case vk::VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR:
		case vk::VK_FORMAT_R16G16B16A16_UNORM:
			return 4u * 2u;

		case vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR:
		case vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR:
			return 2u;

		case vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR:
		case vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR:
			return 2u * 2u;

		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
			return 3u * 2u;

		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
		case vk::VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
			DE_FATAL("Plane formats not supported");
			return ~0u;

		default:
			return (deUint32)vk::mapVkFormat(format).getPixelSize();
	}
}

UVec2 getPlaneSize (const vk::PlanarFormatDescription&	info,
					deUint32							planeNdx,
					const UVec2&						size)
{
	if (info.numPlanes > 1)
		return UVec2(size.x() / info.planes[planeNdx].widthDivisor, size.y() / info.planes[planeNdx].heightDivisor);
	else
		return size;
}

UVec2 randomUVec2 (de::Random&	rng,
				   const UVec2&	min,
				   const UVec2&	max)
{
	UVec2 result;

	result[0] = min[0] + (rng.getUint32() % (1 + max[0] - min[0]));
	result[1] = min[1] + (rng.getUint32() % (1 + max[1] - min[1]));

	return result;
}

void genCopies (de::Random&					rng,
				size_t						copyCount,
				vk::VkFormat				srcFormat,
				const UVec2&				srcSize,
				vk::VkFormat				dstFormat,
				const UVec2&				dstSize,
				vector<vk::VkImageCopy>*	copies)
{
	vector<pair<deUint32, deUint32> >	pairs;
	const vk::PlanarFormatDescription	srcPlaneInfo	(vk::getPlanarFormatDescription(srcFormat));
	const vk::PlanarFormatDescription	dstPlaneInfo	(vk::getPlanarFormatDescription(dstFormat));

	for (deUint32 srcPlaneNdx = 0; srcPlaneNdx < srcPlaneInfo.numPlanes; srcPlaneNdx++)
	{
		for (deUint32 dstPlaneNdx = 0; dstPlaneNdx < dstPlaneInfo.numPlanes; dstPlaneNdx++)
		{
			const vk::VkFormat srcPlaneFormat (getPlaneCompatibleFormat(srcFormat, srcPlaneNdx));
			const vk::VkFormat dstPlaneFormat (getPlaneCompatibleFormat(dstFormat, dstPlaneNdx));

			if (isCompatible(srcPlaneFormat, dstPlaneFormat))
				pairs.push_back(std::make_pair(srcPlaneNdx, dstPlaneNdx));
		}
	}

	DE_ASSERT(!pairs.empty());

	copies->reserve(copyCount);

	for (size_t copyNdx = 0; copyNdx < copyCount; copyNdx++)
	{
		const pair<deUint32, deUint32>	planes			(rng.choose<pair<deUint32, deUint32> >(pairs.begin(), pairs.end()));

		const deUint32					srcPlaneNdx			(planes.first);
		const vk::VkFormat				srcPlaneFormat		(getPlaneCompatibleFormat(srcFormat, srcPlaneNdx));
		const UVec2						srcBlockSize		(getBlockSize(srcPlaneFormat));
		const UVec2						srcPlaneSize		(getPlaneSize(srcPlaneInfo, srcPlaneNdx, srcSize));
		const UVec2						srcPlaneBlockSize	(srcPlaneSize / srcBlockSize);

		const deUint32					dstPlaneNdx			(planes.second);
		const vk::VkFormat				dstPlaneFormat		(getPlaneCompatibleFormat(dstFormat, dstPlaneNdx));
		const UVec2						dstBlockSize		(getBlockSize(dstPlaneFormat));
		const UVec2						dstPlaneSize		(getPlaneSize(dstPlaneInfo, dstPlaneNdx, dstSize));
		const UVec2						dstPlaneBlockSize	(dstPlaneSize / dstBlockSize);

		const UVec2						copyBlockSize		(randomUVec2(rng, UVec2(1u, 1u), tcu::min(srcPlaneBlockSize, dstPlaneBlockSize)));
		const UVec2						srcOffset			(srcBlockSize * randomUVec2(rng, UVec2(0u, 0u), srcPlaneBlockSize - copyBlockSize));
		const UVec2						dstOffset			(dstBlockSize * randomUVec2(rng, UVec2(0u, 0u), dstPlaneBlockSize - copyBlockSize));
		const UVec2						copySize			(copyBlockSize * srcBlockSize);
		const vk::VkImageCopy			copy				=
		{
			// src
			{
				srcPlaneInfo.numPlanes > 1 ?  vk::getPlaneAspect(srcPlaneNdx) : vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0u,
				0u,
				1u
			},
			{
				(deInt32)srcOffset.x(),
				(deInt32)srcOffset.y(),
				0,
			},
			// dst
			{
				dstPlaneInfo.numPlanes > 1 ?  vk::getPlaneAspect(dstPlaneNdx) : vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0u,
				0u,
				1u
			},
			{
				(deInt32)dstOffset.x(),
				(deInt32)dstOffset.y(),
				0,
			},
			// size
			{
				copySize.x(),
				copySize.y(),
				1u
			}
		};

		copies->push_back(copy);
	}
}

tcu::SeedBuilder& operator<< (tcu::SeedBuilder& builder, const ImageConfig& config)
{

	builder << (deUint32)config.format << (deUint32)config.tiling << config.disjoint << config.size[0] << config.size[1];
	return builder;
}

deUint32 buildSeed (const TestConfig& config)
{
	tcu::SeedBuilder builder;

	builder << 6792903u << config.src << config.dst;

	return builder.get();
}

void logImageInfo (TestLog&				log,
				   const ImageConfig&	config)
{
	log << TestLog::Message << "Format: " << config.format << TestLog::EndMessage;
	log << TestLog::Message << "Tiling: " << config.tiling << TestLog::EndMessage;
	log << TestLog::Message << "Size: " << config.size << TestLog::EndMessage;
	log << TestLog::Message << "Disjoint: " << (config.disjoint ? "true" : "false") << TestLog::EndMessage;
}
void logTestCaseInfo (TestLog&							log,
					  const TestConfig&					config,
					  const vector<vk::VkImageCopy>&	copies)
{
	{
		const tcu::ScopedLogSection section (log, "SourceImage", "SourceImage");
		logImageInfo(log, config.src);
	}

	{
		const tcu::ScopedLogSection section (log, "DestinationImage", "DestinationImage");
		logImageInfo(log, config.dst);
	}
	{
		const tcu::ScopedLogSection section (log, "Copies", "Copies");

		for (size_t copyNdx = 0; copyNdx < copies.size(); copyNdx++)
			log << TestLog::Message << copies[copyNdx] << TestLog::EndMessage;
	}
}

tcu::TestStatus imageCopyTest (Context& context, const TestConfig config)
{
	checkSupport(context, config);

	{
		const size_t			copyCount	= 10;
		TestLog&				log			(context.getTestContext().getLog());

		MultiPlaneImageData		srcData		(config.src.format, config.src.size);
		MultiPlaneImageData		dstData		(config.dst.format, config.dst.size);
		MultiPlaneImageData		result		(config.dst.format, config.dst.size);
		vector<vk::VkImageCopy>	copies;

		de::Random				rng			(buildSeed(config));

		genCopies(rng, copyCount, config.src.format, config.src.size, config.dst.format, config.dst.size, &copies);

		logTestCaseInfo(log, config, copies);

		fillRandom(&rng, &srcData);
		fillRandom(&rng, &dstData);

		{
			const vk::DeviceInterface&		vkd						(context.getDeviceInterface());
			const vk::VkDevice				device					(context.getDevice());

			const vk::Unique<vk::VkImage>	srcImage				(createImage(vkd, device, config.src.format, config.src.size, config.src.disjoint, config.src.tiling));
			const vk::MemoryRequirement		srcMemoryRequirement	(config.src.tiling == vk::VK_IMAGE_TILING_OPTIMAL
																	? vk::MemoryRequirement::Any
																	: vk::MemoryRequirement::HostVisible);
			const vk::VkImageCreateFlags	srcCreateFlags			(config.src.disjoint ? vk::VK_IMAGE_CREATE_DISJOINT_BIT_KHR : (vk::VkImageCreateFlagBits)0u);
			const vector<AllocationSp>		srcImageMemory			(allocateAndBindImageMemory(vkd, device, context.getDefaultAllocator(), *srcImage, config.src.format, srcCreateFlags, srcMemoryRequirement));

			const vk::Unique<vk::VkImage>	dstImage				(createImage(vkd, device, config.dst.format, config.dst.size, config.dst.disjoint, config.dst.tiling));
			const vk::MemoryRequirement		dstMemoryRequirement	(config.dst.tiling == vk::VK_IMAGE_TILING_OPTIMAL
																	? vk::MemoryRequirement::Any
																	: vk::MemoryRequirement::HostVisible);
			const vk::VkImageCreateFlags	dstCreateFlags			(config.dst.disjoint ? vk::VK_IMAGE_CREATE_DISJOINT_BIT_KHR : (vk::VkImageCreateFlagBits)0u);
			const vector<AllocationSp>		dstImageMemory			(allocateAndBindImageMemory(vkd, device, context.getDefaultAllocator(), *dstImage, config.dst.format, dstCreateFlags, dstMemoryRequirement));

			if (config.src.tiling == vk::VK_IMAGE_TILING_OPTIMAL)
				uploadImage(vkd, device, context.getUniversalQueueFamilyIndex(), context.getDefaultAllocator(), *srcImage, srcData, vk::VK_ACCESS_TRANSFER_READ_BIT, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			else
				fillImageMemory(vkd, device, context.getUniversalQueueFamilyIndex(), *srcImage, srcImageMemory, srcData, vk::VK_ACCESS_TRANSFER_READ_BIT, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			if (config.dst.tiling == vk::VK_IMAGE_TILING_OPTIMAL)
				uploadImage(vkd, device, context.getUniversalQueueFamilyIndex(), context.getDefaultAllocator(), *dstImage, dstData, vk::VK_ACCESS_TRANSFER_WRITE_BIT, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			else
				fillImageMemory(vkd, device, context.getUniversalQueueFamilyIndex(), *dstImage, dstImageMemory, dstData, vk::VK_ACCESS_TRANSFER_WRITE_BIT, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			{
				const deUint32							queueFamilyNdx	(context.getUniversalQueueFamilyIndex());
				const vk::VkQueue						queue			(context.getUniversalQueue());
				const vk::Unique<vk::VkCommandPool>		cmdPool			(createCommandPool(vkd, device, (vk::VkCommandPoolCreateFlags)0, queueFamilyNdx));
				const vk::Unique<vk::VkCommandBuffer>	cmdBuffer		(allocateCommandBuffer(vkd, device, *cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
				{
					const vk::VkCommandBufferBeginInfo	beginInfo		=
					{
						vk::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
						DE_NULL,
						(vk::VkCommandBufferUsageFlags)vk::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
						(const vk::VkCommandBufferInheritanceInfo*)DE_NULL
					};

					VK_CHECK(vkd.beginCommandBuffer(*cmdBuffer, &beginInfo));
				}

				for (size_t i = 0; i < copies.size(); i++)
				{
					vkd.cmdCopyImage(*cmdBuffer, *srcImage, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *dstImage, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copies[i]);

					const vk::VkImageMemoryBarrier preCopyBarrier =
					{
						vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						DE_NULL,
						vk::VK_ACCESS_TRANSFER_WRITE_BIT,
						vk::VK_ACCESS_TRANSFER_WRITE_BIT,
						vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_QUEUE_FAMILY_IGNORED,
						VK_QUEUE_FAMILY_IGNORED,
						*dstImage,
						{ vk::VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }
					};

					vkd.cmdPipelineBarrier(*cmdBuffer,
											(vk::VkPipelineStageFlags)vk::VK_PIPELINE_STAGE_TRANSFER_BIT,
											(vk::VkPipelineStageFlags)vk::VK_PIPELINE_STAGE_TRANSFER_BIT,
											(vk::VkDependencyFlags)0u,
											0u,
											(const vk::VkMemoryBarrier*)DE_NULL,
											0u,
											(const vk::VkBufferMemoryBarrier*)DE_NULL,
											1u,
											&preCopyBarrier);
				}

				VK_CHECK(vkd.endCommandBuffer(*cmdBuffer));

				{
					const vk::Unique<vk::VkFence>	fence		(createFence(vkd, device));
					const vk::VkSubmitInfo			submitInfo	=
					{
						vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,
						DE_NULL,
						0u,
						(const vk::VkSemaphore*)DE_NULL,
						(const vk::VkPipelineStageFlags*)DE_NULL,
						1u,
						&*cmdBuffer,
						0u,
						(const vk::VkSemaphore*)DE_NULL,
					};

					VK_CHECK(vkd.queueSubmit(queue, 1u, &submitInfo, *fence));
					VK_CHECK(vkd.waitForFences(device, 1u, &*fence, VK_TRUE, ~0ull));
				}
			}

			if (config.dst.tiling == vk::VK_IMAGE_TILING_OPTIMAL)
				downloadImage(vkd, device, context.getUniversalQueueFamilyIndex(), context.getDefaultAllocator(), *dstImage, &result, vk::VK_ACCESS_TRANSFER_WRITE_BIT, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			else
				readImageMemory(vkd, device, context.getUniversalQueueFamilyIndex(), *dstImage, dstImageMemory, &result, vk::VK_ACCESS_TRANSFER_WRITE_BIT, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		}

		{
			MultiPlaneImageData	reference		(dstData);
			const size_t		maxErrorCount	= 30;
			size_t				errorCount		= 0;

			for (size_t copyNdx = 0; copyNdx < copies.size(); copyNdx++)
			{
				const vk::VkImageCopy&	copy				(copies[copyNdx]);

				const deUint32			srcPlaneNdx			(copy.srcSubresource.aspectMask != vk::VK_IMAGE_ASPECT_COLOR_BIT
															? vk::getAspectPlaneNdx((vk::VkImageAspectFlagBits)copy.srcSubresource.aspectMask)
															: 0u);
				const UVec2				srcPlaneSize		(getPlaneSize(srcData.getDescription(), srcPlaneNdx, config.src.size));

				const vk::VkFormat		srcPlaneFormat		(getPlaneCompatibleFormat(config.src.format, srcPlaneNdx));
				const UVec2				srcBlockSize		(getBlockSize(srcPlaneFormat));

				const deUint32			blockSizeBytes		(getBlockByteSize(srcPlaneFormat));

				const UVec2				srcPlaneBlockSize	(srcPlaneSize / srcBlockSize);
				const UVec2				srcBlockOffset		(copy.srcOffset.x / srcBlockSize.x(), copy.srcOffset.y / srcBlockSize.y());
				const UVec2				srcBlockPitch		(blockSizeBytes, blockSizeBytes * srcPlaneBlockSize.x());

				const deUint32			dstPlaneNdx			(copy.dstSubresource.aspectMask != vk::VK_IMAGE_ASPECT_COLOR_BIT
															? vk::getAspectPlaneNdx((vk::VkImageAspectFlagBits)copy.dstSubresource.aspectMask)
															: 0u);
				const UVec2				dstPlaneSize		(getPlaneSize(dstData.getDescription(), dstPlaneNdx, config.dst.size));

				const vk::VkFormat		dstPlaneFormat		(getPlaneCompatibleFormat(config.dst.format, dstPlaneNdx));
				const UVec2				dstBlockSize		(getBlockSize(dstPlaneFormat));

				const UVec2				dstPlaneBlockSize	(dstPlaneSize / dstBlockSize);
				const UVec2				dstBlockOffset		(copy.dstOffset.x / dstBlockSize.x(), copy.dstOffset.y / dstBlockSize.y());
				const UVec2				dstBlockPitch		(blockSizeBytes, blockSizeBytes * dstPlaneBlockSize.x());

				const UVec2				blockSize			(copy.extent.width / srcBlockSize.x(), copy.extent.height / srcBlockSize.y());

				DE_ASSERT(blockSizeBytes == getBlockByteSize(dstPlaneFormat));

				for (deUint32 y = 0; y < blockSize.y(); y++)
				{
					const deUint32	size	= blockSize.x() * blockSizeBytes;
					const deUint32	srcPos	= tcu::dot(srcBlockPitch, UVec2(srcBlockOffset.x(), srcBlockOffset.y() + y));
					const deUint32	dstPos	= tcu::dot(dstBlockPitch, UVec2(dstBlockOffset.x(), dstBlockOffset.y() + y));

					deMemcpy(((deUint8*)reference.getPlanePtr(dstPlaneNdx)) + dstPos,
							 ((const deUint8*)srcData.getPlanePtr(srcPlaneNdx)) + srcPos, size);
				}
			}

			for (deUint32 planeNdx = 0; planeNdx < result.getDescription().numPlanes; ++planeNdx)
			{
				for (size_t byteNdx = 0; byteNdx < result.getPlaneSize(planeNdx); byteNdx++)
				{
					const deUint8	res	= ((const deUint8*)result.getPlanePtr(planeNdx))[byteNdx];
					const deUint8	ref	= ((const deUint8*)reference.getPlanePtr(planeNdx))[byteNdx];

					if (res != ref)
					{
						log << TestLog::Message << "Plane: " << planeNdx << ", Offset: " << byteNdx << ", Expected: " << (deUint32)ref << ", Got: " << (deUint32)res << TestLog::EndMessage;
						errorCount++;

						if (errorCount > maxErrorCount)
							break;
					}
				}

				if (errorCount > maxErrorCount)
					break;
			}

			if (errorCount > 0)
				return tcu::TestStatus::fail("Failed, found " + (errorCount > maxErrorCount ?  de::toString(maxErrorCount) + "+" : de::toString(errorCount))  + " incorrect bytes");
			else
				return tcu::TestStatus::pass("Pass");
		}
	}
}

bool isCopyCompatible (vk::VkFormat srcFormat, vk::VkFormat dstFormat)
{
	if (isYCbCrFormat(srcFormat) && isYCbCrFormat(dstFormat))
	{
		const vk::PlanarFormatDescription	srcPlaneInfo	(vk::getPlanarFormatDescription(srcFormat));
		const vk::PlanarFormatDescription	dstPlaneInfo	(vk::getPlanarFormatDescription(dstFormat));

		for (deUint32 srcPlaneNdx = 0; srcPlaneNdx < srcPlaneInfo.numPlanes; srcPlaneNdx++)
		{
			for (deUint32 dstPlaneNdx = 0; dstPlaneNdx < dstPlaneInfo.numPlanes; dstPlaneNdx++)
			{
				const vk::VkFormat srcPlaneFormat (getPlaneCompatibleFormat(srcFormat, srcPlaneNdx));
				const vk::VkFormat dstPlaneFormat (getPlaneCompatibleFormat(dstFormat, dstPlaneNdx));

				if (isCompatible(srcPlaneFormat, dstPlaneFormat))
					return true;
			}
		}
	}
	else if (isYCbCrFormat(srcFormat))
	{
		const vk::PlanarFormatDescription	srcPlaneInfo	(vk::getPlanarFormatDescription(srcFormat));

		for (deUint32 srcPlaneNdx = 0; srcPlaneNdx < srcPlaneInfo.numPlanes; srcPlaneNdx++)
		{
			const vk::VkFormat srcPlaneFormat (getPlaneCompatibleFormat(srcFormat, srcPlaneNdx));

			if (isCompatible(srcPlaneFormat, dstFormat))
				return true;
		}
	}
	else if (isYCbCrFormat(dstFormat))
	{
		const vk::PlanarFormatDescription	dstPlaneInfo	(vk::getPlanarFormatDescription(dstFormat));

		for (deUint32 dstPlaneNdx = 0; dstPlaneNdx < dstPlaneInfo.numPlanes; dstPlaneNdx++)
		{
			const vk::VkFormat dstPlaneFormat (getPlaneCompatibleFormat(dstFormat, dstPlaneNdx));

			if (isCompatible(dstPlaneFormat, srcFormat))
				return true;
		}
	}
	else
		return isCompatible(srcFormat, dstFormat);

	return false;
}

void initTests (tcu::TestCaseGroup* testGroup)
{
	const vk::VkFormat ycbcrFormats[] =
	{
		vk::VK_FORMAT_R4G4_UNORM_PACK8,
		vk::VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		vk::VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		vk::VK_FORMAT_R5G6B5_UNORM_PACK16,
		vk::VK_FORMAT_B5G6R5_UNORM_PACK16,
		vk::VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		vk::VK_FORMAT_B5G5R5A1_UNORM_PACK16,
		vk::VK_FORMAT_A1R5G5B5_UNORM_PACK16,
		vk::VK_FORMAT_R8_UNORM,
		vk::VK_FORMAT_R8G8_UNORM,
		vk::VK_FORMAT_R8G8B8_UNORM,
		vk::VK_FORMAT_B8G8R8_UNORM,
		vk::VK_FORMAT_R8G8B8A8_UNORM,
		vk::VK_FORMAT_B8G8R8A8_UNORM,
		vk::VK_FORMAT_A8B8G8R8_UNORM_PACK32,
		vk::VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		vk::VK_FORMAT_R16_UNORM,
		vk::VK_FORMAT_R16G16_UNORM,
		vk::VK_FORMAT_R16G16B16_UNORM,
		vk::VK_FORMAT_R16G16B16A16_UNORM,
		vk::VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		vk::VK_FORMAT_G8B8G8R8_422_UNORM_KHR,
		vk::VK_FORMAT_B8G8R8G8_422_UNORM_KHR,
		vk::VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR,
		vk::VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR,
		vk::VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR,
		vk::VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR,
		vk::VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR,
		vk::VK_FORMAT_R10X6_UNORM_PACK16_KHR,
		vk::VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR,
		vk::VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_R12X4_UNORM_PACK16_KHR,
		vk::VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR,
		vk::VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR,
		vk::VK_FORMAT_G16B16G16R16_422_UNORM_KHR,
		vk::VK_FORMAT_B16G16R16G16_422_UNORM_KHR,
		vk::VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR,
		vk::VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR,
		vk::VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR,
		vk::VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR,
		vk::VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR
	};
	const struct
	{
		const char*			name;
		vk::VkImageTiling	value;
	} imageTilings[] =
	{
		{ "linear",	vk::VK_IMAGE_TILING_LINEAR },
		{ "optimal",	vk::VK_IMAGE_TILING_OPTIMAL }
	};
	tcu::TestContext&	testCtx	= testGroup->getTestContext();

	for (size_t srcFormatNdx = 0; srcFormatNdx < DE_LENGTH_OF_ARRAY(ycbcrFormats); srcFormatNdx++)
	{
		const vk::VkFormat				srcFormat		(ycbcrFormats[srcFormatNdx]);
		const UVec2						srcSize			(isYCbCrFormat(srcFormat) ? UVec2(24u, 16u) : UVec2(23u, 17u));
		const string					srcFormatName	(de::toLower(std::string(getFormatName(srcFormat)).substr(10)));
		de::MovePtr<tcu::TestCaseGroup>	srcFormatGroup	(new tcu::TestCaseGroup(testCtx, srcFormatName.c_str(), ("Tests for copies using format " + srcFormatName).c_str()));

		for (size_t dstFormatNdx = 0; dstFormatNdx < DE_LENGTH_OF_ARRAY(ycbcrFormats); dstFormatNdx++)
		{
			const vk::VkFormat				dstFormat		(ycbcrFormats[dstFormatNdx]);
			const UVec2						dstSize			(isYCbCrFormat(dstFormat) ? UVec2(24u, 16u) : UVec2(23u, 17u));
			const string					dstFormatName	(de::toLower(std::string(getFormatName(dstFormat)).substr(10)));

			if ((!vk::isYCbCrFormat(srcFormat) && !vk::isYCbCrFormat(dstFormat))
					|| !isCopyCompatible(srcFormat, dstFormat))
				continue;

			de::MovePtr<tcu::TestCaseGroup>	dstFormatGroup	(new tcu::TestCaseGroup(testCtx, dstFormatName.c_str(), ("Tests for copies using format " + dstFormatName).c_str()));

			for (size_t srcTilingNdx = 0; srcTilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); srcTilingNdx++)
			{
				const vk::VkImageTiling	srcTiling		= imageTilings[srcTilingNdx].value;
				const char* const		srcTilingName	= imageTilings[srcTilingNdx].name;

				for (size_t dstTilingNdx = 0; dstTilingNdx < DE_LENGTH_OF_ARRAY(imageTilings); dstTilingNdx++)
				{
					const vk::VkImageTiling	dstTiling		= imageTilings[dstTilingNdx].value;
					const char* const		dstTilingName	= imageTilings[dstTilingNdx].name;

					for (size_t srcDisjointNdx = 0; srcDisjointNdx < 2; srcDisjointNdx++)
					for (size_t dstDisjointNdx = 0; dstDisjointNdx < 2; dstDisjointNdx++)
					{
						const bool			srcDisjoint	= srcDisjointNdx == 1;
						const bool			dstDisjoint	= dstDisjointNdx == 1;
						const TestConfig	config		(ImageConfig(srcFormat, srcTiling, srcDisjoint, srcSize), ImageConfig(dstFormat, dstTiling, dstDisjoint, dstSize));

						addFunctionCase(dstFormatGroup.get(), string(srcTilingName) + (srcDisjoint ? "_disjoint_" : "_") + string(dstTilingName) + (dstDisjoint ? "_disjoint" : ""), "", imageCopyTest, config);
					}
				}
			}

			srcFormatGroup->addChild(dstFormatGroup.release());
		}

		testGroup->addChild(srcFormatGroup.release());
	}
}

} // anonymous

tcu::TestCaseGroup* createCopyTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "copy", "YCbCr Format Copy Tests", initTests);
}

} // ycbcr
} // vkt
