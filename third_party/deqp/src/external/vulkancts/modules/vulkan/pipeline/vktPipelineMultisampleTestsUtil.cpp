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
 * \file  vktPipelineMultisampleTestsUtil.cpp
 * \brief Multisample Tests Utility Classes
 *//*--------------------------------------------------------------------*/

#include "vktPipelineMultisampleTestsUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"
#include "tcuTextureUtil.hpp"

#include <deMath.h>

using namespace vk;

namespace vkt
{
namespace pipeline
{
namespace multisample
{

tcu::UVec3 getShaderGridSize (const ImageType imageType, const tcu::UVec3& imageSize, const deUint32 mipLevel)
{
	const deUint32 mipLevelX = std::max(imageSize.x() >> mipLevel, 1u);
	const deUint32 mipLevelY = std::max(imageSize.y() >> mipLevel, 1u);
	const deUint32 mipLevelZ = std::max(imageSize.z() >> mipLevel, 1u);

	switch (imageType)
	{
	case IMAGE_TYPE_1D:
		return tcu::UVec3(mipLevelX, 1u, 1u);

	case IMAGE_TYPE_BUFFER:
		return tcu::UVec3(imageSize.x(), 1u, 1u);

	case IMAGE_TYPE_1D_ARRAY:
		return tcu::UVec3(mipLevelX, imageSize.z(), 1u);

	case IMAGE_TYPE_2D:
		return tcu::UVec3(mipLevelX, mipLevelY, 1u);

	case IMAGE_TYPE_2D_ARRAY:
		return tcu::UVec3(mipLevelX, mipLevelY, imageSize.z());

	case IMAGE_TYPE_3D:
		return tcu::UVec3(mipLevelX, mipLevelY, mipLevelZ);

	case IMAGE_TYPE_CUBE:
		return tcu::UVec3(mipLevelX, mipLevelY, 6u);

	case IMAGE_TYPE_CUBE_ARRAY:
		return tcu::UVec3(mipLevelX, mipLevelY, 6u * imageSize.z());

	default:
		DE_FATAL("Unknown image type");
		return tcu::UVec3(1u, 1u, 1u);
	}
}

tcu::UVec3 getLayerSize (const ImageType imageType, const tcu::UVec3& imageSize)
{
	switch (imageType)
	{
	case IMAGE_TYPE_1D:
	case IMAGE_TYPE_1D_ARRAY:
	case IMAGE_TYPE_BUFFER:
		return tcu::UVec3(imageSize.x(), 1u, 1u);

	case IMAGE_TYPE_2D:
	case IMAGE_TYPE_2D_ARRAY:
	case IMAGE_TYPE_CUBE:
	case IMAGE_TYPE_CUBE_ARRAY:
		return tcu::UVec3(imageSize.x(), imageSize.y(), 1u);

	case IMAGE_TYPE_3D:
		return tcu::UVec3(imageSize.x(), imageSize.y(), imageSize.z());

	default:
		DE_FATAL("Unknown image type");
		return tcu::UVec3(1u, 1u, 1u);
	}
}

deUint32 getNumLayers (const ImageType imageType, const tcu::UVec3& imageSize)
{
	switch (imageType)
	{
	case IMAGE_TYPE_1D:
	case IMAGE_TYPE_2D:
	case IMAGE_TYPE_3D:
	case IMAGE_TYPE_BUFFER:
		return 1u;

	case IMAGE_TYPE_1D_ARRAY:
	case IMAGE_TYPE_2D_ARRAY:
		return imageSize.z();

	case IMAGE_TYPE_CUBE:
		return 6u;

	case IMAGE_TYPE_CUBE_ARRAY:
		return imageSize.z() * 6u;

	default:
		DE_FATAL("Unknown image type");
		return 0u;
	}
}

deUint32 getNumPixels (const ImageType imageType, const tcu::UVec3& imageSize)
{
	const tcu::UVec3 gridSize = getShaderGridSize(imageType, imageSize);

	return gridSize.x() * gridSize.y() * gridSize.z();
}

deUint32 getDimensions (const ImageType imageType)
{
	switch (imageType)
	{
	case IMAGE_TYPE_1D:
	case IMAGE_TYPE_BUFFER:
		return 1u;

	case IMAGE_TYPE_1D_ARRAY:
	case IMAGE_TYPE_2D:
		return 2u;

	case IMAGE_TYPE_2D_ARRAY:
	case IMAGE_TYPE_CUBE:
	case IMAGE_TYPE_CUBE_ARRAY:
	case IMAGE_TYPE_3D:
		return 3u;

	default:
		DE_FATAL("Unknown image type");
		return 0u;
	}
}

deUint32 getLayerDimensions (const ImageType imageType)
{
	switch (imageType)
	{
	case IMAGE_TYPE_1D:
	case IMAGE_TYPE_BUFFER:
	case IMAGE_TYPE_1D_ARRAY:
		return 1u;

	case IMAGE_TYPE_2D:
	case IMAGE_TYPE_2D_ARRAY:
	case IMAGE_TYPE_CUBE:
	case IMAGE_TYPE_CUBE_ARRAY:
		return 2u;

	case IMAGE_TYPE_3D:
		return 3u;

	default:
		DE_FATAL("Unknown image type");
		return 0u;
	}
}

VkImageType	mapImageType (const ImageType imageType)
{
	switch (imageType)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_1D_ARRAY:
		case IMAGE_TYPE_BUFFER:
			return VK_IMAGE_TYPE_1D;

		case IMAGE_TYPE_2D:
		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
			return VK_IMAGE_TYPE_2D;

		case IMAGE_TYPE_3D:
			return VK_IMAGE_TYPE_3D;

		default:
			DE_ASSERT(false);
			return VK_IMAGE_TYPE_LAST;
	}
}

VkImageViewType	mapImageViewType (const ImageType imageType)
{
	switch (imageType)
	{
		case IMAGE_TYPE_1D:			return VK_IMAGE_VIEW_TYPE_1D;
		case IMAGE_TYPE_1D_ARRAY:	return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
		case IMAGE_TYPE_2D:			return VK_IMAGE_VIEW_TYPE_2D;
		case IMAGE_TYPE_2D_ARRAY:	return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		case IMAGE_TYPE_3D:			return VK_IMAGE_VIEW_TYPE_3D;
		case IMAGE_TYPE_CUBE:		return VK_IMAGE_VIEW_TYPE_CUBE;
		case IMAGE_TYPE_CUBE_ARRAY:	return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

		default:
			DE_ASSERT(false);
			return VK_IMAGE_VIEW_TYPE_LAST;
	}
}

std::string getImageTypeName (const ImageType imageType)
{
	switch (imageType)
	{
		case IMAGE_TYPE_1D:			return "1d";
		case IMAGE_TYPE_1D_ARRAY:	return "1d_array";
		case IMAGE_TYPE_2D:			return "2d";
		case IMAGE_TYPE_2D_ARRAY:	return "2d_array";
		case IMAGE_TYPE_3D:			return "3d";
		case IMAGE_TYPE_CUBE:		return "cube";
		case IMAGE_TYPE_CUBE_ARRAY:	return "cube_array";
		case IMAGE_TYPE_BUFFER:		return "buffer";

		default:
			DE_ASSERT(false);
			return "";
	}
}

std::string getShaderImageType (const tcu::TextureFormat& format, const ImageType imageType)
{
	std::string formatPart = tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER ? "u" :
							 tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER   ? "i" : "";

	std::string imageTypePart;
	switch (imageType)
	{
		case IMAGE_TYPE_1D:			imageTypePart = "1D";			break;
		case IMAGE_TYPE_1D_ARRAY:	imageTypePart = "1DArray";		break;
		case IMAGE_TYPE_2D:			imageTypePart = "2D";			break;
		case IMAGE_TYPE_2D_ARRAY:	imageTypePart = "2DArray";		break;
		case IMAGE_TYPE_3D:			imageTypePart = "3D";			break;
		case IMAGE_TYPE_CUBE:		imageTypePart = "Cube";			break;
		case IMAGE_TYPE_CUBE_ARRAY:	imageTypePart = "CubeArray";	break;
		case IMAGE_TYPE_BUFFER:		imageTypePart = "Buffer";		break;

		default:
			DE_ASSERT(false);
	}

	return formatPart + "image" + imageTypePart;
}


std::string getShaderImageDataType (const tcu::TextureFormat& format)
{
	switch (tcu::getTextureChannelClass(format.type))
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			return "uvec4";
		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			return "ivec4";
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
			return "vec4";
		default:
			DE_ASSERT(false);
			return "";
	}
}

std::string getShaderImageFormatQualifier (const tcu::TextureFormat& format)
{
	const char* orderPart;
	const char* typePart;

	switch (format.order)
	{
		case tcu::TextureFormat::R:		orderPart = "r";	break;
		case tcu::TextureFormat::RG:	orderPart = "rg";	break;
		case tcu::TextureFormat::RGB:	orderPart = "rgb";	break;
		case tcu::TextureFormat::RGBA:	orderPart = "rgba";	break;

		default:
			DE_ASSERT(false);
			orderPart = DE_NULL;
	}

	switch (format.type)
	{
		case tcu::TextureFormat::FLOAT:				typePart = "32f";		break;
		case tcu::TextureFormat::HALF_FLOAT:		typePart = "16f";		break;

		case tcu::TextureFormat::UNSIGNED_INT32:	typePart = "32ui";		break;
		case tcu::TextureFormat::UNSIGNED_INT16:	typePart = "16ui";		break;
		case tcu::TextureFormat::UNSIGNED_INT8:		typePart = "8ui";		break;

		case tcu::TextureFormat::SIGNED_INT32:		typePart = "32i";		break;
		case tcu::TextureFormat::SIGNED_INT16:		typePart = "16i";		break;
		case tcu::TextureFormat::SIGNED_INT8:		typePart = "8i";		break;

		case tcu::TextureFormat::UNORM_INT16:		typePart = "16";		break;
		case tcu::TextureFormat::UNORM_INT8:		typePart = "8";			break;

		case tcu::TextureFormat::SNORM_INT16:		typePart = "16_snorm";	break;
		case tcu::TextureFormat::SNORM_INT8:		typePart = "8_snorm";	break;

		default:
			DE_ASSERT(false);
			typePart = DE_NULL;
	}

	return std::string() + orderPart + typePart;
}

std::string getShaderImageCoordinates	(const ImageType	imageType,
										 const std::string&	x,
										 const std::string&	xy,
										 const std::string&	xyz)
{
	switch (imageType)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_BUFFER:
			return x;

		case IMAGE_TYPE_1D_ARRAY:
		case IMAGE_TYPE_2D:
			return xy;

		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_3D:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
			return xyz;

		default:
			DE_ASSERT(0);
			return "";
	}
}

VkExtent3D mipLevelExtents (const VkExtent3D& baseExtents, const deUint32 mipLevel)
{
	VkExtent3D result;

	result.width	= std::max(baseExtents.width  >> mipLevel, 1u);
	result.height	= std::max(baseExtents.height >> mipLevel, 1u);
	result.depth	= std::max(baseExtents.depth  >> mipLevel, 1u);

	return result;
}

deUint32 getImageMaxMipLevels (const VkImageFormatProperties& imageFormatProperties, const VkExtent3D& extent)
{
	const deUint32 widestEdge = std::max(std::max(extent.width, extent.height), extent.depth);

	return std::min(static_cast<deUint32>(deFloatLog2(static_cast<float>(widestEdge))) + 1u, imageFormatProperties.maxMipLevels);
}

deUint32 getImageMipLevelSizeInBytes (const VkExtent3D& baseExtents, const deUint32 layersCount, const tcu::TextureFormat& format, const deUint32 mipmapLevel, const deUint32 numSamples)
{
	const VkExtent3D extents = mipLevelExtents(baseExtents, mipmapLevel);

	return extents.width * extents.height * extents.depth * layersCount * numSamples * tcu::getPixelSize(format);
}

deUint32 getImageSizeInBytes (const VkExtent3D& baseExtents, const deUint32 layersCount, const tcu::TextureFormat& format, const deUint32 mipmapLevelsCount, const deUint32 numSamples)
{
	deUint32 imageSizeInBytes = 0;
	for (deUint32 mipmapLevel = 0; mipmapLevel < mipmapLevelsCount; ++mipmapLevel)
	{
		imageSizeInBytes += getImageMipLevelSizeInBytes(baseExtents, layersCount, format, mipmapLevel, numSamples);
	}

	return imageSizeInBytes;
}

void requireFeatures (const InstanceInterface& instanceInterface, const VkPhysicalDevice physicalDevice, const FeatureFlags flags)
{
	const VkPhysicalDeviceFeatures features = getPhysicalDeviceFeatures(instanceInterface, physicalDevice);

	if (((flags & FEATURE_TESSELLATION_SHADER) != 0) && !features.tessellationShader)
		throw tcu::NotSupportedError("Tessellation shader not supported");

	if (((flags & FEATURE_GEOMETRY_SHADER) != 0) && !features.geometryShader)
		throw tcu::NotSupportedError("Geometry shader not supported");

	if (((flags & FEATURE_SHADER_FLOAT_64) != 0) && !features.shaderFloat64)
		throw tcu::NotSupportedError("Double-precision floats not supported");

	if (((flags & FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS) != 0) && !features.vertexPipelineStoresAndAtomics)
		throw tcu::NotSupportedError("SSBO and image writes not supported in vertex pipeline");

	if (((flags & FEATURE_FRAGMENT_STORES_AND_ATOMICS) != 0) && !features.fragmentStoresAndAtomics)
		throw tcu::NotSupportedError("SSBO and image writes not supported in fragment shader");

	if (((flags & FEATURE_SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE) != 0) && !features.shaderTessellationAndGeometryPointSize)
		throw tcu::NotSupportedError("Tessellation and geometry shaders don't support PointSize built-in");
}

} // multisample
} // pipeline
} // vkt
