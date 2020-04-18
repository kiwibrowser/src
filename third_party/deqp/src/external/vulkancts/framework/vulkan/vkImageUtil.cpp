/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Imagination Technologies Ltd.
 * Copyright (c) 2015 Google Inc.
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
 * \brief Utilities for images.
 *//*--------------------------------------------------------------------*/

#include "vkImageUtil.hpp"
#include "tcuTextureUtil.hpp"

namespace vk
{

bool isFloatFormat (VkFormat format)
{
	return tcu::getTextureChannelClass(mapVkFormat(format).type) == tcu::TEXTURECHANNELCLASS_FLOATING_POINT;
}

bool isUnormFormat (VkFormat format)
{
	return tcu::getTextureChannelClass(mapVkFormat(format).type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
}

bool isSnormFormat (VkFormat format)
{
	return tcu::getTextureChannelClass(mapVkFormat(format).type) == tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT;
}

bool isIntFormat (VkFormat format)
{
	return tcu::getTextureChannelClass(mapVkFormat(format).type) == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER;
}

bool isUintFormat (VkFormat format)
{
	return tcu::getTextureChannelClass(mapVkFormat(format).type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER;
}

bool isDepthStencilFormat (VkFormat format)
{
	if (isCompressedFormat(format))
		return false;

	const tcu::TextureFormat tcuFormat = mapVkFormat(format);
	return tcuFormat.order == tcu::TextureFormat::D || tcuFormat.order == tcu::TextureFormat::S || tcuFormat.order == tcu::TextureFormat::DS;
}

bool isSrgbFormat (VkFormat format)
{
	switch (mapVkFormat(format).order)
	{
		case tcu::TextureFormat::sR:
		case tcu::TextureFormat::sRG:
		case tcu::TextureFormat::sRGB:
		case tcu::TextureFormat::sRGBA:
		case tcu::TextureFormat::sBGR:
		case tcu::TextureFormat::sBGRA:
			return true;

		default:
			return false;
	}
}

bool isCompressedFormat (VkFormat format)
{
	// update this mapping if VkFormat changes
	DE_STATIC_ASSERT(VK_CORE_FORMAT_LAST == 185);

	switch (format)
	{
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		case VK_FORMAT_BC2_UNORM_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
		case VK_FORMAT_BC4_UNORM_BLOCK:
		case VK_FORMAT_BC4_SNORM_BLOCK:
		case VK_FORMAT_BC5_UNORM_BLOCK:
		case VK_FORMAT_BC5_SNORM_BLOCK:
		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
		case VK_FORMAT_BC6H_SFLOAT_BLOCK:
		case VK_FORMAT_BC7_UNORM_BLOCK:
		case VK_FORMAT_BC7_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
		case VK_FORMAT_EAC_R11_UNORM_BLOCK:
		case VK_FORMAT_EAC_R11_SNORM_BLOCK:
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
			return true;

		default:
			return false;
	}
}

bool isYCbCrFormat (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
		case VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
		case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
		case VK_FORMAT_R10X6_UNORM_PACK16_KHR:
		case VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR:
		case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR:
		case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
		case VK_FORMAT_R12X4_UNORM_PACK16_KHR:
		case VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR:
		case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR:
		case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
		case VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
		case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
		case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
		case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
			return true;

		default:
			return false;
	}
}

const PlanarFormatDescription& getYCbCrPlanarFormatDescription (VkFormat format)
{
	using tcu::TextureFormat;

	const deUint32	chanR			= PlanarFormatDescription::CHANNEL_R;
	const deUint32	chanG			= PlanarFormatDescription::CHANNEL_G;
	const deUint32	chanB			= PlanarFormatDescription::CHANNEL_B;
	const deUint32	chanA			= PlanarFormatDescription::CHANNEL_A;

	const deUint8	unorm			= (deUint8)tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;

	static const PlanarFormatDescription s_formatInfo[] =
	{
		// VK_FORMAT_G8B8G8R8_422_UNORM_KHR
		{
			1, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	4,		2,		1	},
				{	0,		0,		0	},
				{	0,		0,		0	},
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	24,		8,		4 },	// R
				{	0,		unorm,	0,		8,		2 },	// G
				{	0,		unorm,	8,		8,		4 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_B8G8R8G8_422_UNORM_KHR
		{
			1, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	4,		2,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	16,		8,		4 },	// R
				{	0,		unorm,	8,		8,		2 },	// G
				{	0,		unorm,	0,		8,		4 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{  1, 1, 1 },
				{  1, 2, 2 },
				{  1, 2, 2 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	0,		8,		1 },	// R
				{	0,		unorm,	0,		8,		1 },	// G
				{	1,		unorm,	0,		8,		1 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR
		{
			2, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{  1, 1, 1 },
				{  2, 2, 2 },
				{  0, 0, 0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	1,		unorm,	8,		8,		2 },	// R
				{	0,		unorm,	0,		8,		1 },	// G
				{	1,		unorm,	0,		8,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{  1, 1, 1 },
				{  1, 2, 1 },
				{  1, 2, 1 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	0,		8,		1 },	// R
				{	0,		unorm,	0,		8,		1 },	// G
				{	1,		unorm,	0,		8,		1 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR
		{
			2, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{  1, 1, 1 },
				{  2, 2, 1 },
				{  0, 0, 0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	1,		unorm,	8,		8,		2 },	// R
				{	0,		unorm,	0,		8,		1 },	// G
				{	1,		unorm,	0,		8,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{  1, 1, 1 },
				{  1, 1, 1 },
				{  1, 1, 1 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	0,		8,		1 },	// R
				{	0,		unorm,	0,		8,		1 },	// G
				{	1,		unorm,	0,		8,		1 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_R10X6_UNORM_PACK16_KHR
		{
			1, // planes
			chanR,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	6,		10,		2 },	// R
				{ 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0 },
			}
		},
		// VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR
		{
			1, // planes
			chanR|chanG,
			{
			//		Size	WDiv	HDiv
				{	4,		1,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	6,		10,		4 },	// R
				{	0,		unorm,	22,		10,		4 },	// G
				{ 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0 },
			}
		},
		// VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR
		{
			1, // planes
			chanR|chanG|chanB|chanA,
			{
			//		Size	WDiv	HDiv
				{	8,		1,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	6,		10,		8 },	// R
				{	0,		unorm,	22,		10,		8 },	// G
				{	0,		unorm,	38,		10,		8 },	// B
				{	0,		unorm,	54,		10,		8 },	// A
			}
		},
		// VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR
		{
			1, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	8,		2,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	54,		10,		8 },	// R
				{	0,		unorm,	6,		10,		4 },	// G
				{	0,		unorm,	22,		10,		8 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR
		{
			1, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	8,		2,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	38,		10,		8 },	// R
				{	0,		unorm,	22,		10,		4 },	// G
				{	0,		unorm,	6,		10,		8 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	2,		2,		2 },
				{	2,		2,		2 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	6,		10,		2 },	// R
				{	0,		unorm,	6,		10,		2 },	// G
				{	1,		unorm,	6,		10,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR
		{
			2, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	4,		2,		2 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	1,		unorm,	22,		10,		4 },	// R
				{	0,		unorm,	6,		10,		2 },	// G
				{	1,		unorm,	6,		10,		4 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	2,		2,		1 },
				{	2,		2,		1 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	6,		10,		2 },	// R
				{	0,		unorm,	6,		10,		2 },	// G
				{	1,		unorm,	6,		10,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR
		{
			2, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	4,		2,		1 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	1,		unorm,	22,		10,		4 },	// R
				{	0,		unorm,	6,		10,		2 },	// G
				{	1,		unorm,	6,		10,		4 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	2,		1,		1 },
				{	2,		1,		1 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	6,		10,		2 },	// R
				{	0,		unorm,	6,		10,		2 },	// G
				{	1,		unorm,	6,		10,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_R12X4_UNORM_PACK16_KHR
		{
			1, // planes
			chanR,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	4,		12,		2 },	// R
				{ 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0 },
			}
		},
		// VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR
		{
			1, // planes
			chanR|chanG,
			{
			//		Size	WDiv	HDiv
				{	4,		1,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	4,		12,		4 },	// R
				{	0,		unorm,	20,		12,		4 },	// G
				{ 0, 0, 0, 0, 0 },
				{ 0, 0, 0, 0, 0 },
			}
		},
		// VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR
		{
			1, // planes
			chanR|chanG|chanB|chanA,
			{
			//		Size	WDiv	HDiv
				{	8,		1,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	4,		12,		8 },	// R
				{	0,		unorm,	20,		12,		8 },	// G
				{	0,		unorm,	36,		12,		8 },	// B
				{	0,		unorm,	52,		12,		8 },	// A
			}
		},
		// VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR
		{
			1, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	8,		2,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	52,		12,		8 },	// R
				{	0,		unorm,	4,		12,		4 },	// G
				{	0,		unorm,	20,		12,		8 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR
		{
			1, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	8,		2,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	36,		12,		8 },	// R
				{	0,		unorm,	20,		12,		4 },	// G
				{	0,		unorm,	4,		12,		8 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	2,		2,		2 },
				{	2,		2,		2 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	4,		12,		2 },	// R
				{	0,		unorm,	4,		12,		2 },	// G
				{	1,		unorm,	4,		12,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR
		{
			2, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	4,		2,		2 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	1,		unorm,	20,		12,		4 },	// R
				{	0,		unorm,	4,		12,		2 },	// G
				{	1,		unorm,	4,		12,		4 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	2,		2,		1 },
				{	2,		2,		1 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	4,		12,		2 },	// R
				{	0,		unorm,	4,		12,		2 },	// G
				{	1,		unorm,	4,		12,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR
		{
			2, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	4,		2,		1 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	1,		unorm,	20,		12,		4 },	// R
				{	0,		unorm,	4,		12,		2 },	// G
				{	1,		unorm,	4,		12,		4 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	2,		1,		1 },
				{	2,		1,		1 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	4,		12,		2 },	// R
				{	0,		unorm,	4,		12,		2 },	// G
				{	1,		unorm,	4,		12,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G16B16G16R16_422_UNORM_KHR
		{
			1, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	8,		2,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	48,		16,		8 },	// R
				{	0,		unorm,	0,		16,		4 },	// G
				{	0,		unorm,	16,		16,		8 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_B16G16R16G16_422_UNORM_KHR
		{
			1, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	8,		2,		1 },
				{	0,		0,		0 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	0,		unorm,	32,		16,		8 },	// R
				{	0,		unorm,	16,		16,		4 },	// G
				{	0,		unorm,	0,		16,		8 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	2,		2,		2 },
				{	2,		2,		2 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	0,		16,		2 },	// R
				{	0,		unorm,	0,		16,		2 },	// G
				{	1,		unorm,	0,		16,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR
		{
			2, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	4,		2,		2 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	1,		unorm,	16,		16,		4 },	// R
				{	0,		unorm,	0,		16,		2 },	// G
				{	1,		unorm,	0,		16,		4 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	2,		2,		1 },
				{	2,		2,		1 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	0,		16,		2 },	// R
				{	0,		unorm,	0,		16,		2 },	// G
				{	1,		unorm,	0,		16,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR
		{
			2, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	4,		2,		1 },
				{	0,		0,		0 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	1,		unorm,	16,		16,		4 },	// R
				{	0,		unorm,	0,		16,		2 },	// G
				{	1,		unorm,	0,		16,		4 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
		// VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR
		{
			3, // planes
			chanR|chanG|chanB,
			{
			//		Size	WDiv	HDiv
				{	2,		1,		1 },
				{	2,		1,		1 },
				{	2,		1,		1 },
			},
			{
			//		Plane	Type	Offs	Size	Stride
				{	2,		unorm,	0,		16,		2 },	// R
				{	0,		unorm,	0,		16,		2 },	// G
				{	1,		unorm,	0,		16,		2 },	// B
				{ 0, 0, 0, 0, 0 }
			}
		},
	};

	const size_t	offset	= (size_t)VK_FORMAT_G8B8G8R8_422_UNORM_KHR;

	DE_ASSERT(de::inBounds<size_t>((size_t)format, offset, offset+(size_t)DE_LENGTH_OF_ARRAY(s_formatInfo)));

	return s_formatInfo[(size_t)format-offset];
}

PlanarFormatDescription getCorePlanarFormatDescription (VkFormat format)
{
	const deUint8			unorm	= (deUint8)tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;

	const deUint8			chanR	= (deUint8)PlanarFormatDescription::CHANNEL_R;
	const deUint8			chanG	= (deUint8)PlanarFormatDescription::CHANNEL_G;
	const deUint8			chanB	= (deUint8)PlanarFormatDescription::CHANNEL_B;
	const deUint8			chanA	= (deUint8)PlanarFormatDescription::CHANNEL_A;

	DE_ASSERT(de::inBounds<deUint32>(format, VK_FORMAT_UNDEFINED+1, VK_CORE_FORMAT_LAST));

#if (DE_ENDIANNESS != DE_LITTLE_ENDIAN)
#	error "Big-endian is not supported"
#endif

	switch (format)
	{
		case VK_FORMAT_R8_UNORM:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR,
				{
				//		Size	WDiv	HDiv
					{	1,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		8,		1 },	// R
					{	0,		0,		0,		0,		0 },	// G
					{	0,		0,		0,		0,		0 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R8G8_UNORM:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG,
				{
				//		Size	WDiv	HDiv
					{	2,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		8,		2 },	// R
					{	0,		unorm,	8,		8,		2 },	// G
					{	0,		0,		0,		0,		0 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R16_UNORM:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR,
				{
				//		Size	WDiv	HDiv
					{	2,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		16,		2 },	// R
					{	0,		0,		0,		0,		0 },	// G
					{	0,		0,		0,		0,		0 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R16G16_UNORM:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG,
				{
				//		Size	WDiv	HDiv
					{	4,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		16,		4 },	// R
					{	0,		unorm,	16,		16,		4 },	// G
					{	0,		0,		0,		0,		0 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB,
				{
				//		Size	WDiv	HDiv
					{	4,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		11,		4 },	// R
					{	0,		unorm,	11,		11,		4 },	// G
					{	0,		unorm,	22,		10,		4 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R4G4_UNORM_PACK8:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG,
				{
				//		Size	WDiv	HDiv
					{	1,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	4,		4,		1 },	// R
					{	0,		unorm,	0,		4,		1 },	// G
					{	0,		0,		0,		0,		0 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	2,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	12,		4,		2 },	// R
					{	0,		unorm,	8,		4,		2 },	// G
					{	0,		unorm,	4,		4,		2 },	// B
					{	0,		unorm,	0,		4,		2 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	2,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	4,		4,		2 },	// R
					{	0,		unorm,	8,		4,		2 },	// G
					{	0,		unorm,	12,		4,		2 },	// B
					{	0,		unorm,	0,		4,		2 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB,
				{
				//		Size	WDiv	HDiv
					{	2,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	11,		5,		2 },	// R
					{	0,		unorm,	5,		6,		2 },	// G
					{	0,		unorm,	0,		5,		2 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_B5G6R5_UNORM_PACK16:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB,
				{
				//		Size	WDiv	HDiv
					{	2,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		5,		2 },	// R
					{	0,		unorm,	5,		6,		2 },	// G
					{	0,		unorm,	11,		5,		2 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	2,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	11,		5,		2 },	// R
					{	0,		unorm,	6,		5,		2 },	// G
					{	0,		unorm,	1,		5,		2 },	// B
					{	0,		unorm,	0,		1,		2 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	2,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	1,		5,		2 },	// R
					{	0,		unorm,	6,		5,		2 },	// G
					{	0,		unorm,	11,		5,		2 },	// B
					{	0,		unorm,	0,		1,		2 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	2,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	10,		5,		2 },	// R
					{	0,		unorm,	5,		5,		2 },	// G
					{	0,		unorm,	0,		5,		2 },	// B
					{	0,		unorm,	15,		1,		2 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R8G8B8_UNORM:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB,
				{
				//		Size	WDiv	HDiv
					{	3,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		8,		3 },	// R
					{	0,		unorm,	8,		8,		3 },	// G
					{	0,		unorm,	16,		8,		3 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_B8G8R8_UNORM:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB,
				{
				//		Size	WDiv	HDiv
					{	3,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	16,		8,		3 },	// R
					{	0,		unorm,	8,		8,		3 },	// G
					{	0,		unorm,	0,		8,		3 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	4,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		8,		4 },	// R
					{	0,		unorm,	8,		8,		4 },	// G
					{	0,		unorm,	16,		8,		4 },	// B
					{	0,		unorm,	24,		8,		4 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_B8G8R8A8_UNORM:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	4,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	16,		8,		4 },	// R
					{	0,		unorm,	8,		8,		4 },	// G
					{	0,		unorm,	0,		8,		4 },	// B
					{	0,		unorm,	24,		8,		4 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	4,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	20,		10,		4 },	// R
					{	0,		unorm,	10,		10,		4 },	// G
					{	0,		unorm,	0,		10,		4 },	// B
					{	0,		unorm,	30,		2,		4 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	4,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		10,		4 },	// R
					{	0,		unorm,	10,		10,		4 },	// G
					{	0,		unorm,	20,		10,		4 },	// B
					{	0,		unorm,	30,		2,		4 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R16G16B16_UNORM:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB,
				{
				//		Size	WDiv	HDiv
					{	6,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		16,		6 },	// R
					{	0,		unorm,	16,		16,		6 },	// G
					{	0,		unorm,	32,		16,		6 },	// B
					{	0,		0,		0,		0,		0 }		// A
				}
			};
			return desc;
		}

		case VK_FORMAT_R16G16B16A16_UNORM:
		{
			const PlanarFormatDescription	desc	=
			{
				1, // planes
				chanR|chanG|chanB|chanA,
				{
				//		Size	WDiv	HDiv
					{	16,		1,		1 },
					{	0,		0,		0 },
					{	0,		0,		0 },
				},
				{
				//		Plane	Type	Offs	Size	Stride
					{	0,		unorm,	0,		16,		8 },	// R
					{	0,		unorm,	16,		16,		8 },	// G
					{	0,		unorm,	32,		16,		8 },	// B
					{	0,		unorm,	48,		16,		8 }		// A
				}
			};
			return desc;
		}

		default:
			TCU_THROW(InternalError, "Not implemented");
	}
}

PlanarFormatDescription getPlanarFormatDescription (VkFormat format)
{
	if (isYCbCrFormat(format))
		return getYCbCrPlanarFormatDescription(format);
	else
		return getCorePlanarFormatDescription(format);
}

int getPlaneCount (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
		case VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case VK_FORMAT_R10X6_UNORM_PACK16_KHR:
		case VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR:
		case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR:
		case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_R12X4_UNORM_PACK16_KHR:
		case VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR:
		case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR:
		case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
			return 1;

		case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
			return 2;

		case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
		case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
		case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
		case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
			return 3;

		default:
			DE_FATAL("Not YCbCr format");
			return 0;
	}
}

VkImageAspectFlagBits getPlaneAspect (deUint32 planeNdx)
{
	DE_ASSERT(de::inBounds(planeNdx, 0u, 3u));
	return (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_PLANE_0_BIT_KHR << planeNdx);
}

deUint32 getAspectPlaneNdx (VkImageAspectFlagBits flags)
{
	switch (flags)
	{
		case VK_IMAGE_ASPECT_PLANE_0_BIT_KHR:	return 0;
		case VK_IMAGE_ASPECT_PLANE_1_BIT_KHR:	return 1;
		case VK_IMAGE_ASPECT_PLANE_2_BIT_KHR:	return 2;
		default:
			DE_FATAL("Invalid plane aspect");
			return 0;
	}
}

bool isChromaSubsampled (VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
		case VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
		case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
		case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
		case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
		case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
		case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
		case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
		case VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
		case VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
		case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
		case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
		case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
		case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
		case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
			return true;

		default:
			return false;
	}
}

bool isSupportedByFramework (VkFormat format)
{
	if (format == VK_FORMAT_UNDEFINED || format > VK_CORE_FORMAT_LAST)
		return false;

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
			// \todo [2016-12-01 pyry] Support 64-bit channel types
			return false;

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		case VK_FORMAT_BC2_UNORM_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
		case VK_FORMAT_BC4_UNORM_BLOCK:
		case VK_FORMAT_BC4_SNORM_BLOCK:
		case VK_FORMAT_BC5_UNORM_BLOCK:
		case VK_FORMAT_BC5_SNORM_BLOCK:
		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
		case VK_FORMAT_BC6H_SFLOAT_BLOCK:
		case VK_FORMAT_BC7_UNORM_BLOCK:
		case VK_FORMAT_BC7_SRGB_BLOCK:
			return false;

		default:
			return true;
	}
}

VkFormat mapTextureFormat (const tcu::TextureFormat& format)
{
	DE_STATIC_ASSERT(tcu::TextureFormat::CHANNELORDER_LAST < (1<<16));
	DE_STATIC_ASSERT(tcu::TextureFormat::CHANNELTYPE_LAST < (1<<16));

#define PACK_FMT(ORDER, TYPE) ((int(ORDER) << 16) | int(TYPE))
#define FMT_CASE(ORDER, TYPE) PACK_FMT(tcu::TextureFormat::ORDER, tcu::TextureFormat::TYPE)

	// update this mapping if VkFormat changes
	DE_STATIC_ASSERT(VK_CORE_FORMAT_LAST == 185);

	switch (PACK_FMT(format.order, format.type))
	{
		case FMT_CASE(RG, UNORM_BYTE_44):					return VK_FORMAT_R4G4_UNORM_PACK8;
		case FMT_CASE(RGB, UNORM_SHORT_565):				return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case FMT_CASE(RGBA, UNORM_SHORT_4444):				return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
		case FMT_CASE(RGBA, UNORM_SHORT_5551):				return VK_FORMAT_R5G5B5A1_UNORM_PACK16;

		case FMT_CASE(BGR, UNORM_SHORT_565):				return VK_FORMAT_B5G6R5_UNORM_PACK16;
		case FMT_CASE(BGRA, UNORM_SHORT_4444):				return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
		case FMT_CASE(BGRA, UNORM_SHORT_5551):				return VK_FORMAT_B5G5R5A1_UNORM_PACK16;

		case FMT_CASE(ARGB, UNORM_SHORT_1555):				return VK_FORMAT_A1R5G5B5_UNORM_PACK16;

		case FMT_CASE(R, UNORM_INT8):						return VK_FORMAT_R8_UNORM;
		case FMT_CASE(R, SNORM_INT8):						return VK_FORMAT_R8_SNORM;
		case FMT_CASE(R, UNSIGNED_INT8):					return VK_FORMAT_R8_UINT;
		case FMT_CASE(R, SIGNED_INT8):						return VK_FORMAT_R8_SINT;
		case FMT_CASE(sR, UNORM_INT8):						return VK_FORMAT_R8_SRGB;

		case FMT_CASE(RG, UNORM_INT8):						return VK_FORMAT_R8G8_UNORM;
		case FMT_CASE(RG, SNORM_INT8):						return VK_FORMAT_R8G8_SNORM;
		case FMT_CASE(RG, UNSIGNED_INT8):					return VK_FORMAT_R8G8_UINT;
		case FMT_CASE(RG, SIGNED_INT8):						return VK_FORMAT_R8G8_SINT;
		case FMT_CASE(sRG, UNORM_INT8):						return VK_FORMAT_R8G8_SRGB;

		case FMT_CASE(RGB, UNORM_INT8):						return VK_FORMAT_R8G8B8_UNORM;
		case FMT_CASE(RGB, SNORM_INT8):						return VK_FORMAT_R8G8B8_SNORM;
		case FMT_CASE(RGB, UNSIGNED_INT8):					return VK_FORMAT_R8G8B8_UINT;
		case FMT_CASE(RGB, SIGNED_INT8):					return VK_FORMAT_R8G8B8_SINT;
		case FMT_CASE(sRGB, UNORM_INT8):					return VK_FORMAT_R8G8B8_SRGB;

		case FMT_CASE(RGBA, UNORM_INT8):					return VK_FORMAT_R8G8B8A8_UNORM;
		case FMT_CASE(RGBA, SNORM_INT8):					return VK_FORMAT_R8G8B8A8_SNORM;
		case FMT_CASE(RGBA, UNSIGNED_INT8):					return VK_FORMAT_R8G8B8A8_UINT;
		case FMT_CASE(RGBA, SIGNED_INT8):					return VK_FORMAT_R8G8B8A8_SINT;
		case FMT_CASE(sRGBA, UNORM_INT8):					return VK_FORMAT_R8G8B8A8_SRGB;

		case FMT_CASE(RGBA, UNORM_INT_1010102_REV):			return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		case FMT_CASE(RGBA, SNORM_INT_1010102_REV):			return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
		case FMT_CASE(RGBA, UNSIGNED_INT_1010102_REV):		return VK_FORMAT_A2B10G10R10_UINT_PACK32;
		case FMT_CASE(RGBA, SIGNED_INT_1010102_REV):		return VK_FORMAT_A2B10G10R10_SINT_PACK32;

		case FMT_CASE(R, UNORM_INT16):						return VK_FORMAT_R16_UNORM;
		case FMT_CASE(R, SNORM_INT16):						return VK_FORMAT_R16_SNORM;
		case FMT_CASE(R, UNSIGNED_INT16):					return VK_FORMAT_R16_UINT;
		case FMT_CASE(R, SIGNED_INT16):						return VK_FORMAT_R16_SINT;
		case FMT_CASE(R, HALF_FLOAT):						return VK_FORMAT_R16_SFLOAT;

		case FMT_CASE(RG, UNORM_INT16):						return VK_FORMAT_R16G16_UNORM;
		case FMT_CASE(RG, SNORM_INT16):						return VK_FORMAT_R16G16_SNORM;
		case FMT_CASE(RG, UNSIGNED_INT16):					return VK_FORMAT_R16G16_UINT;
		case FMT_CASE(RG, SIGNED_INT16):					return VK_FORMAT_R16G16_SINT;
		case FMT_CASE(RG, HALF_FLOAT):						return VK_FORMAT_R16G16_SFLOAT;

		case FMT_CASE(RGB, UNORM_INT16):					return VK_FORMAT_R16G16B16_UNORM;
		case FMT_CASE(RGB, SNORM_INT16):					return VK_FORMAT_R16G16B16_SNORM;
		case FMT_CASE(RGB, UNSIGNED_INT16):					return VK_FORMAT_R16G16B16_UINT;
		case FMT_CASE(RGB, SIGNED_INT16):					return VK_FORMAT_R16G16B16_SINT;
		case FMT_CASE(RGB, HALF_FLOAT):						return VK_FORMAT_R16G16B16_SFLOAT;

		case FMT_CASE(RGBA, UNORM_INT16):					return VK_FORMAT_R16G16B16A16_UNORM;
		case FMT_CASE(RGBA, SNORM_INT16):					return VK_FORMAT_R16G16B16A16_SNORM;
		case FMT_CASE(RGBA, UNSIGNED_INT16):				return VK_FORMAT_R16G16B16A16_UINT;
		case FMT_CASE(RGBA, SIGNED_INT16):					return VK_FORMAT_R16G16B16A16_SINT;
		case FMT_CASE(RGBA, HALF_FLOAT):					return VK_FORMAT_R16G16B16A16_SFLOAT;

		case FMT_CASE(R, UNSIGNED_INT32):					return VK_FORMAT_R32_UINT;
		case FMT_CASE(R, SIGNED_INT32):						return VK_FORMAT_R32_SINT;
		case FMT_CASE(R, FLOAT):							return VK_FORMAT_R32_SFLOAT;

		case FMT_CASE(RG, UNSIGNED_INT32):					return VK_FORMAT_R32G32_UINT;
		case FMT_CASE(RG, SIGNED_INT32):					return VK_FORMAT_R32G32_SINT;
		case FMT_CASE(RG, FLOAT):							return VK_FORMAT_R32G32_SFLOAT;

		case FMT_CASE(RGB, UNSIGNED_INT32):					return VK_FORMAT_R32G32B32_UINT;
		case FMT_CASE(RGB, SIGNED_INT32):					return VK_FORMAT_R32G32B32_SINT;
		case FMT_CASE(RGB, FLOAT):							return VK_FORMAT_R32G32B32_SFLOAT;

		case FMT_CASE(RGBA, UNSIGNED_INT32):				return VK_FORMAT_R32G32B32A32_UINT;
		case FMT_CASE(RGBA, SIGNED_INT32):					return VK_FORMAT_R32G32B32A32_SINT;
		case FMT_CASE(RGBA, FLOAT):							return VK_FORMAT_R32G32B32A32_SFLOAT;

		case FMT_CASE(R, FLOAT64):							return VK_FORMAT_R64_SFLOAT;
		case FMT_CASE(RG, FLOAT64):							return VK_FORMAT_R64G64_SFLOAT;
		case FMT_CASE(RGB, FLOAT64):						return VK_FORMAT_R64G64B64_SFLOAT;
		case FMT_CASE(RGBA, FLOAT64):						return VK_FORMAT_R64G64B64A64_SFLOAT;

		case FMT_CASE(RGB, UNSIGNED_INT_11F_11F_10F_REV):	return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case FMT_CASE(RGB, UNSIGNED_INT_999_E5_REV):		return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;

		case FMT_CASE(BGR, UNORM_INT8):						return VK_FORMAT_B8G8R8_UNORM;
		case FMT_CASE(BGR, SNORM_INT8):						return VK_FORMAT_B8G8R8_SNORM;
		case FMT_CASE(BGR, UNSIGNED_INT8):					return VK_FORMAT_B8G8R8_UINT;
		case FMT_CASE(BGR, SIGNED_INT8):					return VK_FORMAT_B8G8R8_SINT;
		case FMT_CASE(sBGR, UNORM_INT8):					return VK_FORMAT_B8G8R8_SRGB;

		case FMT_CASE(BGRA, UNORM_INT8):					return VK_FORMAT_B8G8R8A8_UNORM;
		case FMT_CASE(BGRA, SNORM_INT8):					return VK_FORMAT_B8G8R8A8_SNORM;
		case FMT_CASE(BGRA, UNSIGNED_INT8):					return VK_FORMAT_B8G8R8A8_UINT;
		case FMT_CASE(BGRA, SIGNED_INT8):					return VK_FORMAT_B8G8R8A8_SINT;
		case FMT_CASE(sBGRA, UNORM_INT8):					return VK_FORMAT_B8G8R8A8_SRGB;

		case FMT_CASE(BGRA, UNORM_INT_1010102_REV):			return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case FMT_CASE(BGRA, SNORM_INT_1010102_REV):			return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
		case FMT_CASE(BGRA, UNSIGNED_INT_1010102_REV):		return VK_FORMAT_A2R10G10B10_UINT_PACK32;
		case FMT_CASE(BGRA, SIGNED_INT_1010102_REV):		return VK_FORMAT_A2R10G10B10_SINT_PACK32;

		case FMT_CASE(D, UNORM_INT16):						return VK_FORMAT_D16_UNORM;
		case FMT_CASE(D, UNSIGNED_INT_24_8_REV):			return VK_FORMAT_X8_D24_UNORM_PACK32;
		case FMT_CASE(D, FLOAT):							return VK_FORMAT_D32_SFLOAT;

		case FMT_CASE(S, UNSIGNED_INT8):					return VK_FORMAT_S8_UINT;

		case FMT_CASE(DS, UNSIGNED_INT_16_8_8):				return VK_FORMAT_D16_UNORM_S8_UINT;
		case FMT_CASE(DS, UNSIGNED_INT_24_8_REV):			return VK_FORMAT_D24_UNORM_S8_UINT;
		case FMT_CASE(DS, FLOAT_UNSIGNED_INT_24_8_REV):		return VK_FORMAT_D32_SFLOAT_S8_UINT;


		case FMT_CASE(R,	UNORM_SHORT_10):				return VK_FORMAT_R10X6_UNORM_PACK16_KHR;
		case FMT_CASE(RG,	UNORM_SHORT_10):				return VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR;
		case FMT_CASE(RGBA,	UNORM_SHORT_10):				return VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR;

		case FMT_CASE(R,	UNORM_SHORT_12):				return VK_FORMAT_R12X4_UNORM_PACK16_KHR;
		case FMT_CASE(RG,	UNORM_SHORT_12):				return VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR;
		case FMT_CASE(RGBA,	UNORM_SHORT_12):				return VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR;

		default:
			TCU_THROW(InternalError, "Unknown texture format");
	}

#undef PACK_FMT
#undef FMT_CASE
}

VkFormat mapCompressedTextureFormat (const tcu::CompressedTexFormat format)
{
	// update this mapping if CompressedTexFormat changes
	DE_STATIC_ASSERT(tcu::COMPRESSEDTEXFORMAT_LAST == 39);

	switch (format)
	{
		case tcu::COMPRESSEDTEXFORMAT_ETC2_RGB8:						return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ETC2_SRGB8:						return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:	return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:	return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ETC2_EAC_RGBA8:					return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ETC2_EAC_SRGB8_ALPHA8:			return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;

		case tcu::COMPRESSEDTEXFORMAT_EAC_R11:							return VK_FORMAT_EAC_R11_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_EAC_SIGNED_R11:					return VK_FORMAT_EAC_R11_SNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_EAC_RG11:							return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_EAC_SIGNED_RG11:					return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;

		case tcu::COMPRESSEDTEXFORMAT_ASTC_4x4_RGBA:					return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_4x4_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_5x4_RGBA:					return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_5x4_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_5x5_RGBA:					return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_5x5_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_6x5_RGBA:					return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_6x5_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_6x6_RGBA:					return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_6x6_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_8x5_RGBA:					return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_8x5_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_8x6_RGBA:					return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_8x6_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_8x8_RGBA:					return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_8x8_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_10x5_RGBA:					return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_10x5_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_10x6_RGBA:					return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_10x6_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_10x8_RGBA:					return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_10x8_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_10x10_RGBA:					return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_10x10_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_12x10_RGBA:					return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_12x10_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_12x12_RGBA:					return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
		case tcu::COMPRESSEDTEXFORMAT_ASTC_12x12_SRGB8_ALPHA8:			return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;

		default:
			TCU_THROW(InternalError, "Unknown texture format");
			return VK_FORMAT_UNDEFINED;
	}
}

tcu::TextureFormat mapVkFormat (VkFormat format)
{
	using tcu::TextureFormat;

	// update this mapping if VkFormat changes
	DE_STATIC_ASSERT(VK_CORE_FORMAT_LAST == 185);

	switch (format)
	{
		case VK_FORMAT_R4G4_UNORM_PACK8:		return TextureFormat(TextureFormat::RG,		TextureFormat::UNORM_BYTE_44);
		case VK_FORMAT_R5G6B5_UNORM_PACK16:		return TextureFormat(TextureFormat::RGB,	TextureFormat::UNORM_SHORT_565);
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_SHORT_4444);
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_SHORT_5551);

		case VK_FORMAT_B5G6R5_UNORM_PACK16:		return TextureFormat(TextureFormat::BGR,	TextureFormat::UNORM_SHORT_565);
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:	return TextureFormat(TextureFormat::BGRA,	TextureFormat::UNORM_SHORT_4444);
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:	return TextureFormat(TextureFormat::BGRA,	TextureFormat::UNORM_SHORT_5551);

		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:	return TextureFormat(TextureFormat::ARGB,	TextureFormat::UNORM_SHORT_1555);

		case VK_FORMAT_R8_UNORM:				return TextureFormat(TextureFormat::R,		TextureFormat::UNORM_INT8);
		case VK_FORMAT_R8_SNORM:				return TextureFormat(TextureFormat::R,		TextureFormat::SNORM_INT8);
		case VK_FORMAT_R8_USCALED:				return TextureFormat(TextureFormat::R,		TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_R8_SSCALED:				return TextureFormat(TextureFormat::R,		TextureFormat::SIGNED_INT8);
		case VK_FORMAT_R8_UINT:					return TextureFormat(TextureFormat::R,		TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_R8_SINT:					return TextureFormat(TextureFormat::R,		TextureFormat::SIGNED_INT8);
		case VK_FORMAT_R8_SRGB:					return TextureFormat(TextureFormat::sR,		TextureFormat::UNORM_INT8);

		case VK_FORMAT_R8G8_UNORM:				return TextureFormat(TextureFormat::RG,		TextureFormat::UNORM_INT8);
		case VK_FORMAT_R8G8_SNORM:				return TextureFormat(TextureFormat::RG,		TextureFormat::SNORM_INT8);
		case VK_FORMAT_R8G8_USCALED:			return TextureFormat(TextureFormat::RG,		TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_R8G8_SSCALED:			return TextureFormat(TextureFormat::RG,		TextureFormat::SIGNED_INT8);
		case VK_FORMAT_R8G8_UINT:				return TextureFormat(TextureFormat::RG,		TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_R8G8_SINT:				return TextureFormat(TextureFormat::RG,		TextureFormat::SIGNED_INT8);
		case VK_FORMAT_R8G8_SRGB:				return TextureFormat(TextureFormat::sRG,	TextureFormat::UNORM_INT8);

		case VK_FORMAT_R8G8B8_UNORM:			return TextureFormat(TextureFormat::RGB,	TextureFormat::UNORM_INT8);
		case VK_FORMAT_R8G8B8_SNORM:			return TextureFormat(TextureFormat::RGB,	TextureFormat::SNORM_INT8);
		case VK_FORMAT_R8G8B8_USCALED:			return TextureFormat(TextureFormat::RGB,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_R8G8B8_SSCALED:			return TextureFormat(TextureFormat::RGB,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_R8G8B8_UINT:				return TextureFormat(TextureFormat::RGB,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_R8G8B8_SINT:				return TextureFormat(TextureFormat::RGB,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_R8G8B8_SRGB:				return TextureFormat(TextureFormat::sRGB,	TextureFormat::UNORM_INT8);

		case VK_FORMAT_R8G8B8A8_UNORM:			return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_INT8);
		case VK_FORMAT_R8G8B8A8_SNORM:			return TextureFormat(TextureFormat::RGBA,	TextureFormat::SNORM_INT8);
		case VK_FORMAT_R8G8B8A8_USCALED:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_R8G8B8A8_SSCALED:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_R8G8B8A8_UINT:			return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_R8G8B8A8_SINT:			return TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_R8G8B8A8_SRGB:			return TextureFormat(TextureFormat::sRGBA,	TextureFormat::UNORM_INT8);

		case VK_FORMAT_R16_UNORM:				return TextureFormat(TextureFormat::R,		TextureFormat::UNORM_INT16);
		case VK_FORMAT_R16_SNORM:				return TextureFormat(TextureFormat::R,		TextureFormat::SNORM_INT16);
		case VK_FORMAT_R16_USCALED:				return TextureFormat(TextureFormat::R,		TextureFormat::UNSIGNED_INT16);
		case VK_FORMAT_R16_SSCALED:				return TextureFormat(TextureFormat::R,		TextureFormat::SIGNED_INT16);
		case VK_FORMAT_R16_UINT:				return TextureFormat(TextureFormat::R,		TextureFormat::UNSIGNED_INT16);
		case VK_FORMAT_R16_SINT:				return TextureFormat(TextureFormat::R,		TextureFormat::SIGNED_INT16);
		case VK_FORMAT_R16_SFLOAT:				return TextureFormat(TextureFormat::R,		TextureFormat::HALF_FLOAT);

		case VK_FORMAT_R16G16_UNORM:			return TextureFormat(TextureFormat::RG,		TextureFormat::UNORM_INT16);
		case VK_FORMAT_R16G16_SNORM:			return TextureFormat(TextureFormat::RG,		TextureFormat::SNORM_INT16);
		case VK_FORMAT_R16G16_USCALED:			return TextureFormat(TextureFormat::RG,		TextureFormat::UNSIGNED_INT16);
		case VK_FORMAT_R16G16_SSCALED:			return TextureFormat(TextureFormat::RG,		TextureFormat::SIGNED_INT16);
		case VK_FORMAT_R16G16_UINT:				return TextureFormat(TextureFormat::RG,		TextureFormat::UNSIGNED_INT16);
		case VK_FORMAT_R16G16_SINT:				return TextureFormat(TextureFormat::RG,		TextureFormat::SIGNED_INT16);
		case VK_FORMAT_R16G16_SFLOAT:			return TextureFormat(TextureFormat::RG,		TextureFormat::HALF_FLOAT);

		case VK_FORMAT_R16G16B16_UNORM:			return TextureFormat(TextureFormat::RGB,	TextureFormat::UNORM_INT16);
		case VK_FORMAT_R16G16B16_SNORM:			return TextureFormat(TextureFormat::RGB,	TextureFormat::SNORM_INT16);
		case VK_FORMAT_R16G16B16_USCALED:		return TextureFormat(TextureFormat::RGB,	TextureFormat::UNSIGNED_INT16);
		case VK_FORMAT_R16G16B16_SSCALED:		return TextureFormat(TextureFormat::RGB,	TextureFormat::SIGNED_INT16);
		case VK_FORMAT_R16G16B16_UINT:			return TextureFormat(TextureFormat::RGB,	TextureFormat::UNSIGNED_INT16);
		case VK_FORMAT_R16G16B16_SINT:			return TextureFormat(TextureFormat::RGB,	TextureFormat::SIGNED_INT16);
		case VK_FORMAT_R16G16B16_SFLOAT:		return TextureFormat(TextureFormat::RGB,	TextureFormat::HALF_FLOAT);

		case VK_FORMAT_R16G16B16A16_UNORM:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_INT16);
		case VK_FORMAT_R16G16B16A16_SNORM:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::SNORM_INT16);
		case VK_FORMAT_R16G16B16A16_USCALED:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT16);
		case VK_FORMAT_R16G16B16A16_SSCALED:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT16);
		case VK_FORMAT_R16G16B16A16_UINT:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT16);
		case VK_FORMAT_R16G16B16A16_SINT:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT16);
		case VK_FORMAT_R16G16B16A16_SFLOAT:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::HALF_FLOAT);

		case VK_FORMAT_R32_UINT:				return TextureFormat(TextureFormat::R,		TextureFormat::UNSIGNED_INT32);
		case VK_FORMAT_R32_SINT:				return TextureFormat(TextureFormat::R,		TextureFormat::SIGNED_INT32);
		case VK_FORMAT_R32_SFLOAT:				return TextureFormat(TextureFormat::R,		TextureFormat::FLOAT);

		case VK_FORMAT_R32G32_UINT:				return TextureFormat(TextureFormat::RG,		TextureFormat::UNSIGNED_INT32);
		case VK_FORMAT_R32G32_SINT:				return TextureFormat(TextureFormat::RG,		TextureFormat::SIGNED_INT32);
		case VK_FORMAT_R32G32_SFLOAT:			return TextureFormat(TextureFormat::RG,		TextureFormat::FLOAT);

		case VK_FORMAT_R32G32B32_UINT:			return TextureFormat(TextureFormat::RGB,	TextureFormat::UNSIGNED_INT32);
		case VK_FORMAT_R32G32B32_SINT:			return TextureFormat(TextureFormat::RGB,	TextureFormat::SIGNED_INT32);
		case VK_FORMAT_R32G32B32_SFLOAT:		return TextureFormat(TextureFormat::RGB,	TextureFormat::FLOAT);

		case VK_FORMAT_R32G32B32A32_UINT:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT32);
		case VK_FORMAT_R32G32B32A32_SINT:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT32);
		case VK_FORMAT_R32G32B32A32_SFLOAT:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::FLOAT);

		case VK_FORMAT_R64_SFLOAT:				return TextureFormat(TextureFormat::R,		TextureFormat::FLOAT64);
		case VK_FORMAT_R64G64_SFLOAT:			return TextureFormat(TextureFormat::RG,		TextureFormat::FLOAT64);
		case VK_FORMAT_R64G64B64_SFLOAT:		return TextureFormat(TextureFormat::RGB,	TextureFormat::FLOAT64);
		case VK_FORMAT_R64G64B64A64_SFLOAT:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::FLOAT64);

		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:	return TextureFormat(TextureFormat::RGB,	TextureFormat::UNSIGNED_INT_11F_11F_10F_REV);
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:	return TextureFormat(TextureFormat::RGB,	TextureFormat::UNSIGNED_INT_999_E5_REV);

		case VK_FORMAT_B8G8R8_UNORM:			return TextureFormat(TextureFormat::BGR,	TextureFormat::UNORM_INT8);
		case VK_FORMAT_B8G8R8_SNORM:			return TextureFormat(TextureFormat::BGR,	TextureFormat::SNORM_INT8);
		case VK_FORMAT_B8G8R8_USCALED:			return TextureFormat(TextureFormat::BGR,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_B8G8R8_SSCALED:			return TextureFormat(TextureFormat::BGR,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_B8G8R8_UINT:				return TextureFormat(TextureFormat::BGR,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_B8G8R8_SINT:				return TextureFormat(TextureFormat::BGR,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_B8G8R8_SRGB:				return TextureFormat(TextureFormat::sBGR,	TextureFormat::UNORM_INT8);

		case VK_FORMAT_B8G8R8A8_UNORM:			return TextureFormat(TextureFormat::BGRA,	TextureFormat::UNORM_INT8);
		case VK_FORMAT_B8G8R8A8_SNORM:			return TextureFormat(TextureFormat::BGRA,	TextureFormat::SNORM_INT8);
		case VK_FORMAT_B8G8R8A8_USCALED:		return TextureFormat(TextureFormat::BGRA,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_B8G8R8A8_SSCALED:		return TextureFormat(TextureFormat::BGRA,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_B8G8R8A8_UINT:			return TextureFormat(TextureFormat::BGRA,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_B8G8R8A8_SINT:			return TextureFormat(TextureFormat::BGRA,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_B8G8R8A8_SRGB:			return TextureFormat(TextureFormat::sBGRA,	TextureFormat::UNORM_INT8);

		case VK_FORMAT_D16_UNORM:				return TextureFormat(TextureFormat::D,		TextureFormat::UNORM_INT16);
		case VK_FORMAT_X8_D24_UNORM_PACK32:		return TextureFormat(TextureFormat::D,		TextureFormat::UNSIGNED_INT_24_8_REV);
		case VK_FORMAT_D32_SFLOAT:				return TextureFormat(TextureFormat::D,		TextureFormat::FLOAT);

		case VK_FORMAT_S8_UINT:					return TextureFormat(TextureFormat::S,		TextureFormat::UNSIGNED_INT8);

		// \note There is no standard interleaved memory layout for DS formats; buffer-image copies
		//		 will always operate on either D or S aspect only. See Khronos bug 12998
		case VK_FORMAT_D16_UNORM_S8_UINT:		return TextureFormat(TextureFormat::DS,		TextureFormat::UNSIGNED_INT_16_8_8);
		case VK_FORMAT_D24_UNORM_S8_UINT:		return TextureFormat(TextureFormat::DS,		TextureFormat::UNSIGNED_INT_24_8_REV);
		case VK_FORMAT_D32_SFLOAT_S8_UINT:		return TextureFormat(TextureFormat::DS,		TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV);

#if (DE_ENDIANNESS == DE_LITTLE_ENDIAN)
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_INT8);
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::SNORM_INT8);
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT8);
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT8);
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:	return TextureFormat(TextureFormat::sRGBA,	TextureFormat::UNORM_INT8);
#else
#	error "Big-endian not supported"
#endif

		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:	return TextureFormat(TextureFormat::BGRA,	TextureFormat::UNORM_INT_1010102_REV);
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:	return TextureFormat(TextureFormat::BGRA,	TextureFormat::SNORM_INT_1010102_REV);
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:	return TextureFormat(TextureFormat::BGRA,	TextureFormat::UNSIGNED_INT_1010102_REV);
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:	return TextureFormat(TextureFormat::BGRA,	TextureFormat::SIGNED_INT_1010102_REV);
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:		return TextureFormat(TextureFormat::BGRA,	TextureFormat::UNSIGNED_INT_1010102_REV);
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:		return TextureFormat(TextureFormat::BGRA,	TextureFormat::SIGNED_INT_1010102_REV);

		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_INT_1010102_REV);
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::SNORM_INT_1010102_REV);
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT_1010102_REV);
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT_1010102_REV);
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNSIGNED_INT_1010102_REV);
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::SIGNED_INT_1010102_REV);

		// YCbCr formats that can be mapped
		case VK_FORMAT_R10X6_UNORM_PACK16_KHR:					return TextureFormat(TextureFormat::R,		TextureFormat::UNORM_SHORT_10);
		case VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR:			return TextureFormat(TextureFormat::RG,		TextureFormat::UNORM_SHORT_10);
		case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_SHORT_10);

		case VK_FORMAT_R12X4_UNORM_PACK16_KHR:					return TextureFormat(TextureFormat::R,		TextureFormat::UNORM_SHORT_12);
		case VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR:			return TextureFormat(TextureFormat::RG,		TextureFormat::UNORM_SHORT_12);
		case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR:	return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_SHORT_12);

		default:
			TCU_THROW(InternalError, "Unknown image format");
	}
}

tcu::CompressedTexFormat mapVkCompressedFormat (VkFormat format)
{
	// update this mapping if VkFormat changes
	DE_STATIC_ASSERT(VK_CORE_FORMAT_LAST == 185);

	switch (format)
	{
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ETC2_RGB8;
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ETC2_SRGB8;
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:	return tcu::COMPRESSEDTEXFORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1;
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:	return tcu::COMPRESSEDTEXFORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:	return tcu::COMPRESSEDTEXFORMAT_ETC2_EAC_RGBA8;
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:	return tcu::COMPRESSEDTEXFORMAT_ETC2_EAC_SRGB8_ALPHA8;

		case VK_FORMAT_EAC_R11_UNORM_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_EAC_R11;
		case VK_FORMAT_EAC_R11_SNORM_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_EAC_SIGNED_R11;
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_EAC_RG11;
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_EAC_SIGNED_RG11;

		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_4x4_RGBA;
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_ASTC_4x4_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_5x4_RGBA;
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_ASTC_5x4_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_5x5_RGBA;
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_ASTC_5x5_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_6x5_RGBA;
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_ASTC_6x5_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_6x6_RGBA;
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_ASTC_6x6_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_8x5_RGBA;
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_ASTC_8x5_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_8x6_RGBA;
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_ASTC_8x6_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_8x8_RGBA;
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:			return tcu::COMPRESSEDTEXFORMAT_ASTC_8x8_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_10x5_RGBA;
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_10x5_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_10x6_RGBA;
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_10x6_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_10x8_RGBA;
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_10x8_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_10x10_RGBA;
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_10x10_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_12x10_RGBA;
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_12x10_SRGB8_ALPHA8;
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_12x12_RGBA;
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:		return tcu::COMPRESSEDTEXFORMAT_ASTC_12x12_SRGB8_ALPHA8;

		default:
			TCU_THROW(InternalError, "Unknown image format");
			return tcu::COMPRESSEDTEXFORMAT_LAST;
	}
}

static bool isScaledFormat (VkFormat format)
{
	// update this mapping if VkFormat changes
	DE_STATIC_ASSERT(VK_CORE_FORMAT_LAST == 185);

	switch (format)
	{
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
			return true;

		default:
			return false;
	}
}

static bool fullTextureFormatRoundTripSupported (VkFormat format)
{
	if (isScaledFormat(format))
	{
		// *SCALED formats get mapped to correspoding (u)int formats since
		// accessing them through (float) getPixel/setPixel has same behavior
		// as in shader access in Vulkan.
		// Unfortunately full round-trip between tcu::TextureFormat and VkFormat
		// for most SCALED formats is not supported though.

		const tcu::TextureFormat	tcuFormat	= mapVkFormat(format);

		switch (tcuFormat.type)
		{
			case tcu::TextureFormat::UNSIGNED_INT8:
			case tcu::TextureFormat::UNSIGNED_INT16:
			case tcu::TextureFormat::UNSIGNED_INT32:
			case tcu::TextureFormat::SIGNED_INT8:
			case tcu::TextureFormat::SIGNED_INT16:
			case tcu::TextureFormat::SIGNED_INT32:
			case tcu::TextureFormat::UNSIGNED_INT_1010102_REV:
			case tcu::TextureFormat::SIGNED_INT_1010102_REV:
				return false;

			default:
				return true;
		}
	}
	else
	{
		switch (format)
		{
			case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
			case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
			case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
			case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
			case VK_FORMAT_A8B8G8R8_UINT_PACK32:
			case VK_FORMAT_A8B8G8R8_SINT_PACK32:
			case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
				return false; // These map to regular byte array formats

			default:
				break;
		}

		return (format != VK_FORMAT_UNDEFINED);
	}
}

tcu::TextureFormat getChannelAccessFormat (tcu::TextureChannelClass	type,
										   deUint32					offsetBits,
										   deUint32					sizeBits)
{
	using tcu::TextureFormat;

	if (offsetBits == 0)
	{
		static const TextureFormat::ChannelType	s_size8[tcu::TEXTURECHANNELCLASS_LAST] =
		{
			TextureFormat::SNORM_INT8,			// snorm
			TextureFormat::UNORM_INT8,			// unorm
			TextureFormat::SIGNED_INT8,			// sint
			TextureFormat::UNSIGNED_INT8,		// uint
			TextureFormat::CHANNELTYPE_LAST,	// float
		};
		static const TextureFormat::ChannelType	s_size16[tcu::TEXTURECHANNELCLASS_LAST] =
		{
			TextureFormat::SNORM_INT16,			// snorm
			TextureFormat::UNORM_INT16,			// unorm
			TextureFormat::SIGNED_INT16,		// sint
			TextureFormat::UNSIGNED_INT16,		// uint
			TextureFormat::HALF_FLOAT,			// float
		};
		static const TextureFormat::ChannelType	s_size32[tcu::TEXTURECHANNELCLASS_LAST] =
		{
			TextureFormat::SNORM_INT32,			// snorm
			TextureFormat::UNORM_INT32,			// unorm
			TextureFormat::SIGNED_INT32,		// sint
			TextureFormat::UNSIGNED_INT32,		// uint
			TextureFormat::FLOAT,				// float
		};

		TextureFormat::ChannelType	chnType		= TextureFormat::CHANNELTYPE_LAST;

		if (sizeBits == 8)
			chnType = s_size8[type];
		else if (sizeBits == 16)
			chnType = s_size16[type];
		else if (sizeBits == 32)
			chnType = s_size32[type];

		if (chnType != TextureFormat::CHANNELTYPE_LAST)
			return TextureFormat(TextureFormat::R, chnType);
	}
	else
	{
		if (type		== tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT	&&
			offsetBits	== 6												&&
			sizeBits	== 10)
			return TextureFormat(TextureFormat::R, TextureFormat::UNORM_SHORT_10);
		else if (type		== tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT	&&
				 offsetBits	== 4												&&
				 sizeBits	== 12)
			return TextureFormat(TextureFormat::R, TextureFormat::UNORM_SHORT_12);
	}

	TCU_THROW(InternalError, "Channel access format is not supported");
}

tcu::PixelBufferAccess getChannelAccess (const PlanarFormatDescription&	formatInfo,
										 const tcu::UVec2&				size,
										 const deUint32*				planeRowPitches,
										 void* const*					planePtrs,
										 deUint32						channelNdx)
{
	DE_ASSERT(formatInfo.hasChannelNdx(channelNdx));

	const deUint32	planeNdx			= formatInfo.channels[channelNdx].planeNdx;
	const deUint32	planeOffsetBytes	= formatInfo.channels[channelNdx].offsetBits / 8;
	const deUint32	valueOffsetBits		= formatInfo.channels[channelNdx].offsetBits % 8;
	const deUint32	pixelStrideBytes	= formatInfo.channels[channelNdx].strideBytes;

	DE_ASSERT(size.x() % formatInfo.planes[planeNdx].widthDivisor == 0);
	DE_ASSERT(size.y() % formatInfo.planes[planeNdx].heightDivisor == 0);

	deUint32		accessWidth			= size.x() / formatInfo.planes[planeNdx].widthDivisor;
	const deUint32	accessHeight		= size.y() / formatInfo.planes[planeNdx].heightDivisor;
	const deUint32	elementSizeBytes	= formatInfo.planes[planeNdx].elementSizeBytes;

	const deUint32	rowPitch			= planeRowPitches[planeNdx];

	if (pixelStrideBytes != elementSizeBytes)
	{
		DE_ASSERT(elementSizeBytes % pixelStrideBytes == 0);
		accessWidth *= elementSizeBytes/pixelStrideBytes;
	}

	return tcu::PixelBufferAccess(getChannelAccessFormat((tcu::TextureChannelClass)formatInfo.channels[channelNdx].type,
														 valueOffsetBits,
														 formatInfo.channels[channelNdx].sizeBits),
								  tcu::IVec3((int)accessWidth, (int)accessHeight, 1),
								  tcu::IVec3((int)pixelStrideBytes, (int)rowPitch, 0),
								  (deUint8*)planePtrs[planeNdx] + planeOffsetBytes);
}


tcu::ConstPixelBufferAccess getChannelAccess (const PlanarFormatDescription&	formatInfo,
											  const tcu::UVec2&					size,
											  const deUint32*					planeRowPitches,
											  const void* const*				planePtrs,
											  deUint32							channelNdx)
{
	return getChannelAccess(formatInfo, size, planeRowPitches, const_cast<void* const*>(planePtrs), channelNdx);
}

void imageUtilSelfTest (void)
{
	for (int formatNdx = 0; formatNdx < VK_CORE_FORMAT_LAST; formatNdx++)
	{
		const VkFormat	format	= (VkFormat)formatNdx;

		if (format == VK_FORMAT_R64_UINT			||
			format == VK_FORMAT_R64_SINT			||
			format == VK_FORMAT_R64G64_UINT			||
			format == VK_FORMAT_R64G64_SINT			||
			format == VK_FORMAT_R64G64B64_UINT		||
			format == VK_FORMAT_R64G64B64_SINT		||
			format == VK_FORMAT_R64G64B64A64_UINT	||
			format == VK_FORMAT_R64G64B64A64_SINT)
			continue; // \todo [2015-12-05 pyry] Add framework support for (u)int64 channel type

		if (format != VK_FORMAT_UNDEFINED && !isCompressedFormat(format))
		{
			const tcu::TextureFormat	tcuFormat		= mapVkFormat(format);
			const VkFormat				remappedFormat	= mapTextureFormat(tcuFormat);

			DE_TEST_ASSERT(isValid(tcuFormat));

			if (fullTextureFormatRoundTripSupported(format))
				DE_TEST_ASSERT(format == remappedFormat);
		}
	}

	for (int formatNdx = VK_FORMAT_G8B8G8R8_422_UNORM_KHR; formatNdx <= VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR; formatNdx++)
	{
		const VkFormat					format	= (VkFormat)formatNdx;
		const PlanarFormatDescription&	info	= getPlanarFormatDescription(format);

		DE_TEST_ASSERT(isYCbCrFormat(format));
		DE_TEST_ASSERT(de::inRange<deUint8>(info.numPlanes, 1u, 3u));
		DE_TEST_ASSERT(info.numPlanes == getPlaneCount(format));
	}
}

VkFilter mapFilterMode (tcu::Sampler::FilterMode filterMode)
{
	DE_STATIC_ASSERT(tcu::Sampler::FILTERMODE_LAST == 6);

	switch (filterMode)
	{
		case tcu::Sampler::NEAREST:					return VK_FILTER_NEAREST;
		case tcu::Sampler::LINEAR:					return VK_FILTER_LINEAR;
		case tcu::Sampler::NEAREST_MIPMAP_NEAREST:	return VK_FILTER_NEAREST;
		case tcu::Sampler::NEAREST_MIPMAP_LINEAR:	return VK_FILTER_NEAREST;
		case tcu::Sampler::LINEAR_MIPMAP_NEAREST:	return VK_FILTER_LINEAR;
		case tcu::Sampler::LINEAR_MIPMAP_LINEAR:	return VK_FILTER_LINEAR;
		default:
			DE_FATAL("Illegal filter mode");
			return (VkFilter)0;
	}
}

VkSamplerMipmapMode mapMipmapMode (tcu::Sampler::FilterMode filterMode)
{
	DE_STATIC_ASSERT(tcu::Sampler::FILTERMODE_LAST == 6);

	// \note VkSamplerCreateInfo doesn't have a flag for disabling mipmapping. Instead
	//		 minLod = 0 and maxLod = 0.25 should be used to match OpenGL NEAREST and LINEAR
	//		 filtering mode behavior.

	switch (filterMode)
	{
		case tcu::Sampler::NEAREST:					return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case tcu::Sampler::LINEAR:					return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case tcu::Sampler::NEAREST_MIPMAP_NEAREST:	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case tcu::Sampler::NEAREST_MIPMAP_LINEAR:	return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		case tcu::Sampler::LINEAR_MIPMAP_NEAREST:	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case tcu::Sampler::LINEAR_MIPMAP_LINEAR:	return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default:
			DE_FATAL("Illegal filter mode");
			return (VkSamplerMipmapMode)0;
	}
}

VkSamplerAddressMode mapWrapMode (tcu::Sampler::WrapMode wrapMode)
{
	switch (wrapMode)
	{
		case tcu::Sampler::CLAMP_TO_EDGE:		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case tcu::Sampler::CLAMP_TO_BORDER:		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case tcu::Sampler::REPEAT_GL:			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case tcu::Sampler::MIRRORED_REPEAT_GL:	return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case tcu::Sampler::MIRRORED_ONCE:		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		default:
			DE_FATAL("Wrap mode can't be mapped to Vulkan");
			return (vk::VkSamplerAddressMode)0;
	}
}

vk::VkCompareOp mapCompareMode (tcu::Sampler::CompareMode mode)
{
	switch (mode)
	{
		case tcu::Sampler::COMPAREMODE_NONE:				return vk::VK_COMPARE_OP_NEVER;
		case tcu::Sampler::COMPAREMODE_LESS:				return vk::VK_COMPARE_OP_LESS;
		case tcu::Sampler::COMPAREMODE_LESS_OR_EQUAL:		return vk::VK_COMPARE_OP_LESS_OR_EQUAL;
		case tcu::Sampler::COMPAREMODE_GREATER:				return vk::VK_COMPARE_OP_GREATER;
		case tcu::Sampler::COMPAREMODE_GREATER_OR_EQUAL:	return vk::VK_COMPARE_OP_GREATER_OR_EQUAL;
		case tcu::Sampler::COMPAREMODE_EQUAL:				return vk::VK_COMPARE_OP_EQUAL;
		case tcu::Sampler::COMPAREMODE_NOT_EQUAL:			return vk::VK_COMPARE_OP_NOT_EQUAL;
		case tcu::Sampler::COMPAREMODE_ALWAYS:				return vk::VK_COMPARE_OP_ALWAYS;
		case tcu::Sampler::COMPAREMODE_NEVER:				return vk::VK_COMPARE_OP_NEVER;
		default:
			DE_FATAL("Illegal compare mode");
			return (vk::VkCompareOp)0;
	}
}

static VkBorderColor mapBorderColor (tcu::TextureChannelClass channelClass, const rr::GenericVec4& color)
{
	if (channelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER)
	{
		const tcu::UVec4	uColor	= color.get<deUint32>();

		if (uColor		== tcu::UVec4(0, 0, 0, 0)) return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		else if (uColor	== tcu::UVec4(0, 0, 0, 1)) return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		else if (uColor == tcu::UVec4(1, 1, 1, 1)) return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	}
	else if (channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)
	{
		const tcu::IVec4	sColor	= color.get<deInt32>();

		if (sColor		== tcu::IVec4(0, 0, 0, 0)) return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		else if (sColor	== tcu::IVec4(0, 0, 0, 1)) return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		else if (sColor == tcu::IVec4(1, 1, 1, 1)) return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	}
	else
	{
		const tcu::Vec4		fColor	= color.get<float>();

		if (fColor		== tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f)) return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		else if (fColor == tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f)) return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		else if (fColor == tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f)) return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	}

	DE_FATAL("Unsupported border color");
	return VK_BORDER_COLOR_LAST;
}

VkSamplerCreateInfo mapSampler (const tcu::Sampler& sampler, const tcu::TextureFormat& format, float minLod, float maxLod)
{
	const bool					compareEnabled	= (sampler.compare != tcu::Sampler::COMPAREMODE_NONE);
	const VkCompareOp			compareOp		= (compareEnabled) ? (mapCompareMode(sampler.compare)) : (VK_COMPARE_OP_ALWAYS);
	const VkBorderColor			borderColor		= mapBorderColor(getTextureChannelClass(format.type), sampler.borderColor);
	const bool					isMipmapEnabled	= (sampler.minFilter != tcu::Sampler::NEAREST && sampler.minFilter != tcu::Sampler::LINEAR);

	const VkSamplerCreateInfo	createInfo		=
	{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		DE_NULL,
		(VkSamplerCreateFlags)0,
		mapFilterMode(sampler.magFilter),							// magFilter
		mapFilterMode(sampler.minFilter),							// minFilter
		mapMipmapMode(sampler.minFilter),							// mipMode
		mapWrapMode(sampler.wrapS),									// addressU
		mapWrapMode(sampler.wrapT),									// addressV
		mapWrapMode(sampler.wrapR),									// addressW
		0.0f,														// mipLodBias
		VK_FALSE,													// anisotropyEnable
		1.0f,														// maxAnisotropy
		(VkBool32)(compareEnabled ? VK_TRUE : VK_FALSE),			// compareEnable
		compareOp,													// compareOp
		(isMipmapEnabled ? minLod : 0.0f),							// minLod
		(isMipmapEnabled ? maxLod : 0.25f),							// maxLod
		borderColor,												// borderColor
		(VkBool32)(sampler.normalizedCoords ? VK_FALSE : VK_TRUE),	// unnormalizedCoords
	};

	return createInfo;
}

tcu::Sampler mapVkSampler (const VkSamplerCreateInfo& samplerCreateInfo)
{
	// \note minLod & maxLod are not supported by tcu::Sampler. LOD must be clamped
	//       before passing it to tcu::Texture*::sample*()

	tcu::Sampler::ReductionMode reductionMode = tcu::Sampler::WEIGHTED_AVERAGE;

	void const *pNext = samplerCreateInfo.pNext;
	while (pNext != DE_NULL)
	{
		const VkStructureType nextType = *reinterpret_cast<const VkStructureType*>(pNext);
		switch (nextType)
		{
			case VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT:
			{
				const VkSamplerReductionModeCreateInfoEXT reductionModeCreateInfo = *reinterpret_cast<const VkSamplerReductionModeCreateInfoEXT*>(pNext);
				reductionMode = mapVkSamplerReductionMode(reductionModeCreateInfo.reductionMode);
				pNext = reinterpret_cast<const VkSamplerReductionModeCreateInfoEXT*>(pNext)->pNext;
				break;
			}
			case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO_KHR:
				pNext = reinterpret_cast<const VkSamplerYcbcrConversionInfoKHR*>(pNext)->pNext;
				break;
			default:
				TCU_FAIL("Unrecognized sType in chained sampler create info");
		}
	}



	tcu::Sampler sampler(mapVkSamplerAddressMode(samplerCreateInfo.addressModeU),
						 mapVkSamplerAddressMode(samplerCreateInfo.addressModeV),
						 mapVkSamplerAddressMode(samplerCreateInfo.addressModeW),
						 mapVkMinTexFilter(samplerCreateInfo.minFilter, samplerCreateInfo.mipmapMode),
						 mapVkMagTexFilter(samplerCreateInfo.magFilter),
						 0.0f,
						 !samplerCreateInfo.unnormalizedCoordinates,
						 samplerCreateInfo.compareEnable ? mapVkSamplerCompareOp(samplerCreateInfo.compareOp)
														 : tcu::Sampler::COMPAREMODE_NONE,
						 0,
						 tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f),
						 true,
						 tcu::Sampler::MODE_DEPTH,
						 reductionMode);

	if (samplerCreateInfo.anisotropyEnable)
		TCU_THROW(InternalError, "Anisotropic filtering is not supported by tcu::Sampler");

	switch (samplerCreateInfo.borderColor)
	{
		case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
			sampler.borderColor = tcu::UVec4(0,0,0,1);
			break;
		case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
			sampler.borderColor = tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
			break;
		case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
			sampler.borderColor = tcu::UVec4(1, 1, 1, 1);
			break;
		case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
			sampler.borderColor = tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
			break;
		case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
			sampler.borderColor = tcu::UVec4(0,0,0,0);
			break;
		case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
			sampler.borderColor = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	return sampler;
}

tcu::Sampler::CompareMode mapVkSamplerCompareOp (VkCompareOp compareOp)
{
	switch (compareOp)
	{
		case VK_COMPARE_OP_NEVER:				return tcu::Sampler::COMPAREMODE_NEVER;
		case VK_COMPARE_OP_LESS:				return tcu::Sampler::COMPAREMODE_LESS;
		case VK_COMPARE_OP_EQUAL:				return tcu::Sampler::COMPAREMODE_EQUAL;
		case VK_COMPARE_OP_LESS_OR_EQUAL:		return tcu::Sampler::COMPAREMODE_LESS_OR_EQUAL;
		case VK_COMPARE_OP_GREATER:				return tcu::Sampler::COMPAREMODE_GREATER;
		case VK_COMPARE_OP_NOT_EQUAL:			return tcu::Sampler::COMPAREMODE_NOT_EQUAL;
		case VK_COMPARE_OP_GREATER_OR_EQUAL:	return tcu::Sampler::COMPAREMODE_GREATER_OR_EQUAL;
		case VK_COMPARE_OP_ALWAYS:				return tcu::Sampler::COMPAREMODE_ALWAYS;
		default:
			break;
	}

	DE_ASSERT(false);
	return tcu::Sampler::COMPAREMODE_LAST;
}

tcu::Sampler::WrapMode mapVkSamplerAddressMode (VkSamplerAddressMode addressMode)
{
	switch (addressMode)
	{
		case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:			return tcu::Sampler::CLAMP_TO_EDGE;
		case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:		return tcu::Sampler::CLAMP_TO_BORDER;
		case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:		return tcu::Sampler::MIRRORED_REPEAT_GL;
		case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:	return tcu::Sampler::MIRRORED_ONCE;
		case VK_SAMPLER_ADDRESS_MODE_REPEAT:				return tcu::Sampler::REPEAT_GL;
		default:
			break;
	}

	DE_ASSERT(false);
	return tcu::Sampler::WRAPMODE_LAST;
}

tcu::Sampler::ReductionMode mapVkSamplerReductionMode (VkSamplerReductionModeEXT reductionMode)
{
	switch (reductionMode)
	{
		case VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT:	return tcu::Sampler::WEIGHTED_AVERAGE;
		case VK_SAMPLER_REDUCTION_MODE_MIN_EXT:					return tcu::Sampler::MIN;
		case VK_SAMPLER_REDUCTION_MODE_MAX_EXT:					return tcu::Sampler::MAX;
		default:
			break;
	}

	DE_ASSERT(false);
	return tcu::Sampler::REDUCTIONMODE_LAST;
}

tcu::Sampler::FilterMode mapVkMinTexFilter (VkFilter filter, VkSamplerMipmapMode mipMode)
{
	switch (filter)
	{
		case VK_FILTER_LINEAR:
			switch (mipMode)
			{
				case VK_SAMPLER_MIPMAP_MODE_LINEAR:		return tcu::Sampler::LINEAR_MIPMAP_LINEAR;
				case VK_SAMPLER_MIPMAP_MODE_NEAREST:	return tcu::Sampler::LINEAR_MIPMAP_NEAREST;
				default:
					break;
			}
			break;

		case VK_FILTER_NEAREST:
			switch (mipMode)
			{
				case VK_SAMPLER_MIPMAP_MODE_LINEAR:		return tcu::Sampler::NEAREST_MIPMAP_LINEAR;
				case VK_SAMPLER_MIPMAP_MODE_NEAREST:	return tcu::Sampler::NEAREST_MIPMAP_NEAREST;
				default:
					break;
			}
			break;

		default:
			break;
	}

	DE_ASSERT(false);
	return tcu::Sampler::FILTERMODE_LAST;
}

tcu::Sampler::FilterMode mapVkMagTexFilter (VkFilter filter)
{
	switch (filter)
	{
		case VK_FILTER_LINEAR:		return tcu::Sampler::LINEAR;
		case VK_FILTER_NEAREST:		return tcu::Sampler::NEAREST;
		default:
			break;
	}

	DE_ASSERT(false);
	return tcu::Sampler::FILTERMODE_LAST;
}

//! Get a format the matches the layout in buffer memory used for a
//! buffer<->image copy on a depth/stencil format.
tcu::TextureFormat getDepthCopyFormat (VkFormat combinedFormat)
{
	switch (combinedFormat)
	{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D32_SFLOAT:
			return mapVkFormat(combinedFormat);

		case VK_FORMAT_D16_UNORM_S8_UINT:
			return mapVkFormat(VK_FORMAT_D16_UNORM);
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return mapVkFormat(VK_FORMAT_X8_D24_UNORM_PACK32);
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return mapVkFormat(VK_FORMAT_D32_SFLOAT);

		case VK_FORMAT_S8_UINT:
		default:
			DE_FATAL("Unexpected depth/stencil format");
			return tcu::TextureFormat();
	}
}

//! Get a format the matches the layout in buffer memory used for a
//! buffer<->image copy on a depth/stencil format.
tcu::TextureFormat getStencilCopyFormat (VkFormat combinedFormat)
{
	switch (combinedFormat)
	{
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		case VK_FORMAT_S8_UINT:
			return mapVkFormat(VK_FORMAT_S8_UINT);

		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D32_SFLOAT:
		default:
			DE_FATAL("Unexpected depth/stencil format");
			return tcu::TextureFormat();
	}
}

} // vk
