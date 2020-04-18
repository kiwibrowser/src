#ifndef _VKTPIPELINEMULTISAMPLETESTSUTIL_HPP
#define _VKTPIPELINEMULTISAMPLETESTSUTIL_HPP
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
 * \file  vktPipelineMultisampleTestsUtil.hpp
 * \brief Multisample Tests Utility Classes
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkMemUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkPrograms.hpp"
#include "vkTypeUtil.hpp"
#include "vkImageUtil.hpp"
#include "deSharedPtr.hpp"

namespace vkt
{
namespace pipeline
{
namespace multisample
{

enum ImageType
{
	IMAGE_TYPE_1D			= 0u,
	IMAGE_TYPE_1D_ARRAY,
	IMAGE_TYPE_2D,
	IMAGE_TYPE_2D_ARRAY,
	IMAGE_TYPE_3D,
	IMAGE_TYPE_CUBE,
	IMAGE_TYPE_CUBE_ARRAY,
	IMAGE_TYPE_BUFFER,

	IMAGE_TYPE_LAST
};

// Image helper functions
vk::VkImageType		mapImageType					(const ImageType imageType);
vk::VkImageViewType	mapImageViewType				(const ImageType imageType);
std::string			getImageTypeName				(const ImageType imageType);
std::string			getShaderImageType				(const tcu::TextureFormat& format, const ImageType imageType);
std::string			getShaderImageDataType			(const tcu::TextureFormat& format);
std::string			getShaderImageFormatQualifier	(const tcu::TextureFormat& format);
std::string			getShaderImageCoordinates		(const ImageType imageType, const std::string& x, const std::string& xy, const std::string&	xyz);
//!< Size used for addresing image in a compute shader
tcu::UVec3			getShaderGridSize				(const ImageType imageType, const tcu::UVec3& imageSize, const deUint32	mipLevel = 0);
//!< Size of a single image layer
tcu::UVec3			getLayerSize					(const ImageType imageType, const tcu::UVec3& imageSize);
//!< Number of array layers (for array and cube types)
deUint32			getNumLayers					(const ImageType imageType, const tcu::UVec3& imageSize);
//!< Number of texels in an image
deUint32			getNumPixels					(const ImageType imageType, const tcu::UVec3& imageSize);
//!< Coordinate dimension used for addressing (e.g. 3 (x,y,z) for 2d array)
deUint32			getDimensions					(const ImageType imageType);
//!< Coordinate dimension used for addressing a single layer (e.g. 2 (x,y) for 2d array)
deUint32			getLayerDimensions				(const ImageType imageType);
vk::VkExtent3D		mipLevelExtents					(const vk::VkExtent3D& baseExtents, const deUint32 mipLevel);
tcu::UVec3			mipLevelExtents					(const tcu::UVec3&	   baseExtents, const deUint32 mipLevel);
deUint32			getImageMipLevelSizeInBytes		(const vk::VkExtent3D& baseExtents, const deUint32 layersCount, const tcu::TextureFormat& format, const deUint32 mipmapLevel, const deUint32 numSamples = 1u);
deUint32			getImageSizeInBytes				(const vk::VkExtent3D& baseExtents, const deUint32 layersCount, const tcu::TextureFormat& format, const deUint32 mipmapLevelsCount = 1u, const deUint32 numSamples = 1u);
deUint32			getImageMaxMipLevels			(const vk::VkImageFormatProperties& imageFormatProperties, const vk::VkExtent3D& extent);

enum FeatureFlagBits
{
	FEATURE_TESSELLATION_SHADER = 1u << 0,
	FEATURE_GEOMETRY_SHADER = 1u << 1,
	FEATURE_SHADER_FLOAT_64 = 1u << 2,
	FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS = 1u << 3,
	FEATURE_FRAGMENT_STORES_AND_ATOMICS = 1u << 4,
	FEATURE_SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE = 1u << 5,
};
typedef deUint32 FeatureFlags;

void requireFeatures(const vk::InstanceInterface& instanceInterface, const vk::VkPhysicalDevice physicalDevice, const FeatureFlags flags);

template<typename T>
inline de::SharedPtr<vk::Unique<T> > makeVkSharedPtr(vk::Move<T> vkMove)
{
	return de::SharedPtr<vk::Unique<T> >(new vk::Unique<T>(vkMove));
}

template<typename T>
inline std::size_t sizeInBytes(const std::vector<T>& vec)
{
	return vec.size() * sizeof(vec[0]);
}

template<typename T>
inline const T* dataPointer(const std::vector<T>& vec)
{
	return (vec.size() != 0 ? &vec[0] : DE_NULL);
}

} // multisample
} // pipeline
} // vkt

#endif // _VKTPIPELINEMULTISAMPLETESTSUTIL_HPP
