#ifndef _VKTSPARSERESOURCESTESTSUTIL_HPP
#define _VKTSPARSERESOURCESTESTSUTIL_HPP
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
 * \file  vktSparseResourcesTestsUtil.hpp
 * \brief Sparse Resources Tests Utility Classes
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkMemUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkImageUtil.hpp"
#include "deSharedPtr.hpp"
#include "deUniquePtr.hpp"

namespace vkt
{
namespace sparse
{

typedef de::SharedPtr<vk::Unique<vk::VkDeviceMemory> > DeviceMemorySp;

enum ImageType
{
	IMAGE_TYPE_1D = 0,
	IMAGE_TYPE_1D_ARRAY,
	IMAGE_TYPE_2D,
	IMAGE_TYPE_2D_ARRAY,
	IMAGE_TYPE_3D,
	IMAGE_TYPE_CUBE,
	IMAGE_TYPE_CUBE_ARRAY,
	IMAGE_TYPE_BUFFER,

	IMAGE_TYPE_LAST
};

enum FeatureFlagBits
{
	FEATURE_TESSELLATION_SHADER							= 1u << 0,
	FEATURE_GEOMETRY_SHADER								= 1u << 1,
	FEATURE_SHADER_FLOAT_64								= 1u << 2,
	FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS			= 1u << 3,
	FEATURE_FRAGMENT_STORES_AND_ATOMICS					= 1u << 4,
	FEATURE_SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE	= 1u << 5,
};
typedef deUint32 FeatureFlags;

enum
{
	BUFFER_IMAGE_COPY_OFFSET_GRANULARITY	= 4u,
	NO_MATCH_FOUND							= ~((deUint32)0),	//!< no matching index
};

vk::VkImageType					mapImageType						(const ImageType					imageType);

vk::VkImageViewType				mapImageViewType					(const ImageType					imageType);

std::string						getImageTypeName					(const ImageType					imageType);

std::string						getShaderImageType					(const tcu::TextureFormat&			format,
																	 const ImageType					imageType);

std::string						getShaderImageDataType				(const tcu::TextureFormat&			format);

std::string						getShaderImageFormatQualifier		(const tcu::TextureFormat&			format);

std::string						getShaderImageCoordinates			(const ImageType					imageType,
																	 const std::string&					x,
																	 const std::string&					xy,
																	 const std::string&					xyz);

//!< Size used for addresing image in a compute shader
tcu::UVec3						getShaderGridSize					(const ImageType					imageType,
																	 const tcu::UVec3&					imageSize,
																	 const deUint32						mipLevel	= 0);

//!< Size of a single image layer
tcu::UVec3						getLayerSize						(const ImageType					imageType,
																	 const tcu::UVec3&					imageSize);

//!< Number of array layers (for array and cube types)
deUint32						getNumLayers						(const ImageType					imageType,
																	 const tcu::UVec3&					imageSize);

//!< Number of texels in an image
deUint32						getNumPixels						(const ImageType					imageType,
																	 const tcu::UVec3&					imageSize);

//!< Coordinate dimension used for addressing (e.g. 3 (x,y,z) for 2d array)
deUint32						getDimensions						(const ImageType					imageType);

//!< Coordinate dimension used for addressing a single layer (e.g. 2 (x,y) for 2d array)
deUint32						getLayerDimensions					(const ImageType					imageType);

//!< Helper function for checking if requested image size does not exceed device limits
bool							isImageSizeSupported				(const vk::InstanceInterface&		instance,
																	 const vk::VkPhysicalDevice			physicalDevice,
																	 const ImageType					imageType,
																	 const tcu::UVec3&					imageSize);

vk::VkExtent3D					mipLevelExtents						(const vk::VkExtent3D&				baseExtents,
																	 const deUint32						mipLevel);

tcu::UVec3						mipLevelExtents						(const tcu::UVec3&					baseExtents,
																	 const deUint32						mipLevel);

deUint32						getImageMaxMipLevels				(const vk::VkImageFormatProperties& imageFormatProperties,
																	 const vk::VkExtent3D&				extent);

deUint32						getImageMipLevelSizeInBytes			(const vk::VkExtent3D&				baseExtents,
																	 const deUint32						layersCount,
																	 const tcu::TextureFormat&			format,
																	 const deUint32						mipmapLevel,
																	 const deUint32						mipmapMemoryAlignment	= 1u);

deUint32						getImageSizeInBytes					(const vk::VkExtent3D&				baseExtents,
																	 const deUint32						layersCount,
																	 const tcu::TextureFormat&			format,
																	 const deUint32						mipmapLevelsCount		= 1u,
																	 const deUint32						mipmapMemoryAlignment	= 1u);

vk::Move<vk::VkCommandPool>		makeCommandPool						(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const deUint32						queueFamilyIndex);

vk::Move<vk::VkPipelineLayout>	makePipelineLayout					(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const vk::VkDescriptorSetLayout	descriptorSetLayout = DE_NULL);

vk::Move<vk::VkPipeline>		makeComputePipeline					(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const vk::VkPipelineLayout			pipelineLayout,
																	 const vk::VkShaderModule			shaderModule,
																	 const vk::VkSpecializationInfo*	specializationInfo	= 0);

vk::Move<vk::VkBufferView>		makeBufferView						(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const vk::VkBuffer					buffer,
																	 const vk::VkFormat					format,
																	 const vk::VkDeviceSize				offset,
																	 const vk::VkDeviceSize				size);

vk::Move<vk::VkImageView>		makeImageView						(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const vk::VkImage					image,
																	 const vk::VkImageViewType			imageViewType,
																	 const vk::VkFormat					format,
																	 const vk::VkImageSubresourceRange	subresourceRange);

vk::Move<vk::VkDescriptorSet>	makeDescriptorSet					(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const vk::VkDescriptorPool			descriptorPool,
																	 const vk::VkDescriptorSetLayout	setLayout);

vk::Move<vk::VkFramebuffer>		makeFramebuffer						(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const vk::VkRenderPass				renderPass,
																	 const deUint32						attachmentCount,
																	 const vk::VkImageView*				pAttachments,
																	 const deUint32						width,
																	 const deUint32						height,
																	 const deUint32						layers = 1u);

de::MovePtr<vk::Allocation>		bindImage							(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 vk::Allocator&						allocator,
																	 const vk::VkImage					image,
																	 const vk::MemoryRequirement		requirement);

de::MovePtr<vk::Allocation>		bindBuffer							(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 vk::Allocator&						allocator,
																	 const vk::VkBuffer					buffer,
																	 const vk::MemoryRequirement		requirement);

vk::VkBufferCreateInfo			makeBufferCreateInfo				(const vk::VkDeviceSize				bufferSize,
																	 const vk::VkBufferUsageFlags		usage);

vk::VkBufferImageCopy			makeBufferImageCopy					(const vk::VkExtent3D				extent,
																	 const deUint32						layersCount,
																	 const deUint32						mipmapLevel		= 0u,
																	 const vk::VkDeviceSize				bufferOffset	= 0ull);

vk::VkBufferMemoryBarrier		makeBufferMemoryBarrier				(const vk::VkAccessFlags			srcAccessMask,
																	 const vk::VkAccessFlags			dstAccessMask,
																	 const vk::VkBuffer					buffer,
																	 const vk::VkDeviceSize				offset,
																	 const vk::VkDeviceSize				bufferSizeBytes);

vk::VkImageMemoryBarrier		makeImageMemoryBarrier				(const vk::VkAccessFlags			srcAccessMask,
																	 const vk::VkAccessFlags			dstAccessMask,
																	 const vk::VkImageLayout			oldLayout,
																	 const vk::VkImageLayout			newLayout,
																	 const vk::VkImage					image,
																	 const vk::VkImageSubresourceRange	subresourceRange);

vk::VkImageMemoryBarrier		makeImageMemoryBarrier				(const vk::VkAccessFlags			srcAccessMask,
																	 const vk::VkAccessFlags			dstAccessMask,
																	 const vk::VkImageLayout			oldLayout,
																	 const vk::VkImageLayout			newLayout,
																	 const deUint32						srcQueueFamilyIndex,
																	 const deUint32						destQueueFamilyIndex,
																	 const vk::VkImage					image,
																	 const vk::VkImageSubresourceRange	subresourceRange);

vk::VkMemoryBarrier				makeMemoryBarrier					(const vk::VkAccessFlags			srcAccessMask,
																	 const vk::VkAccessFlags			dstAccessMask);

vk::VkSparseImageMemoryBind		makeSparseImageMemoryBind			(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const vk::VkDeviceSize				allocationSize,
																	 const deUint32						memoryType,
																	 const vk::VkImageSubresource&		subresource,
																	 const vk::VkOffset3D&				offset,
																	 const vk::VkExtent3D&				extent);

vk::VkSparseMemoryBind			makeSparseMemoryBind				(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const vk::VkDeviceSize				allocationSize,
																	 const deUint32						memoryType,
																	 const vk::VkDeviceSize				resourceOffset,
																	 const vk::VkSparseMemoryBindFlags	flags			= 0u);

void							beginCommandBuffer					(const vk::DeviceInterface&			vk,
																	 const vk::VkCommandBuffer			cmdBuffer);

void							endCommandBuffer					(const vk::DeviceInterface&			vk,
																	 const vk::VkCommandBuffer			cmdBuffer);

void							submitCommands						(const vk::DeviceInterface&			vk,
																	 const vk::VkQueue					queue,
																	 const vk::VkCommandBuffer			cmdBuffer,
																	 const deUint32						waitSemaphoreCount		= 0,
																	 const vk::VkSemaphore*				pWaitSemaphores			= DE_NULL,
																	 const vk::VkPipelineStageFlags*	pWaitDstStageMask		= DE_NULL,
																	 const deUint32						signalSemaphoreCount	= 0,
																	 const vk::VkSemaphore*				pSignalSemaphores		= DE_NULL);

void							submitCommandsAndWait				(const vk::DeviceInterface&			vk,
																	 const vk::VkDevice					device,
																	 const vk::VkQueue					queue,
																	 const vk::VkCommandBuffer			cmdBuffer,
																	 const deUint32						waitSemaphoreCount		= 0,
																	 const vk::VkSemaphore*				pWaitSemaphores			= DE_NULL,
																	 const vk::VkPipelineStageFlags*	pWaitDstStageMask		= DE_NULL,
																	 const deUint32						signalSemaphoreCount	= 0,
																	 const vk::VkSemaphore*				pSignalSemaphores		= DE_NULL);

void							requireFeatures						(const vk::InstanceInterface&		vki,
																	 const vk::VkPhysicalDevice			physicalDevice,
																	 const FeatureFlags					flags);

deUint32						findMatchingMemoryType				(const vk::InstanceInterface&		instance,
																	 const vk::VkPhysicalDevice			physicalDevice,
																	 const vk::VkMemoryRequirements&	objectMemoryRequirements,
																	 const vk::MemoryRequirement&		memoryRequirement);

bool							checkSparseSupportForImageType		(const vk::InstanceInterface&		instance,
																	 const vk::VkPhysicalDevice			physicalDevice,
																	 const ImageType					imageType);

bool							checkSparseSupportForImageFormat	(const vk::InstanceInterface&		instance,
																	 const vk::VkPhysicalDevice			physicalDevice,
																	 const vk::VkImageCreateInfo&		imageInfo);

bool							checkImageFormatFeatureSupport		(const vk::InstanceInterface&		instance,
																	 const vk::VkPhysicalDevice			physicalDevice,
																	 const vk::VkFormat					format,
																	 const vk::VkFormatFeatureFlags		featureFlags);

deUint32						getSparseAspectRequirementsIndex	(const std::vector<vk::VkSparseImageMemoryRequirements>&	requirements,
																	 const vk::VkImageAspectFlags								aspectFlags);

inline vk::Move<vk::VkBuffer> makeBuffer (const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkBufferCreateInfo& createInfo)
{
	return createBuffer(vk, device, &createInfo);
}

inline vk::Move<vk::VkImage> makeImage (const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkImageCreateInfo& createInfo)
{
	return createImage(vk, device, &createInfo);
}

template<typename T>
inline de::SharedPtr<vk::Unique<T> > makeVkSharedPtr (vk::Move<T> vkMove)
{
	return de::SharedPtr<vk::Unique<T> >(new vk::Unique<T>(vkMove));
}

template<typename T>
inline de::SharedPtr<de::UniquePtr<T> > makeDeSharedPtr (de::MovePtr<T> deMove)
{
	return de::SharedPtr<de::UniquePtr<T> >(new de::UniquePtr<T>(deMove));
}

template<typename T>
inline std::size_t sizeInBytes (const std::vector<T>& vec)
{
	return vec.size() * sizeof(vec[0]);
}

template<typename T>
inline const T* getDataOrNullptr (const std::vector<T>& vec, const std::size_t index = 0u)
{
	return (index < vec.size() ? &vec[index] : DE_NULL);
}

} // sparse
} // vkt

#endif // _VKTSPARSERESOURCESTESTSUTIL_HPP
