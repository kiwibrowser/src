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
 * \file  vktSparseResourcesTestsUtil.cpp
 * \brief Sparse Resources Tests Utility Classes
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesTestsUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"
#include "tcuTextureUtil.hpp"

#include <deMath.h>

using namespace vk;

namespace vkt
{
namespace sparse
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

bool isImageSizeSupported (const InstanceInterface& instance, const VkPhysicalDevice physicalDevice, const ImageType imageType, const tcu::UVec3& imageSize)
{
	const VkPhysicalDeviceProperties deviceProperties = getPhysicalDeviceProperties(instance, physicalDevice);

	switch (imageType)
	{
		case IMAGE_TYPE_1D:
			return	imageSize.x() <= deviceProperties.limits.maxImageDimension1D;
		case IMAGE_TYPE_1D_ARRAY:
			return	imageSize.x() <= deviceProperties.limits.maxImageDimension1D &&
					imageSize.z() <= deviceProperties.limits.maxImageArrayLayers;
		case IMAGE_TYPE_2D:
			return	imageSize.x() <= deviceProperties.limits.maxImageDimension2D &&
					imageSize.y() <= deviceProperties.limits.maxImageDimension2D;
		case IMAGE_TYPE_2D_ARRAY:
			return	imageSize.x() <= deviceProperties.limits.maxImageDimension2D &&
					imageSize.y() <= deviceProperties.limits.maxImageDimension2D &&
					imageSize.z() <= deviceProperties.limits.maxImageArrayLayers;
		case IMAGE_TYPE_CUBE:
			return	imageSize.x() <= deviceProperties.limits.maxImageDimensionCube &&
					imageSize.y() <= deviceProperties.limits.maxImageDimensionCube;
		case IMAGE_TYPE_CUBE_ARRAY:
			return	imageSize.x() <= deviceProperties.limits.maxImageDimensionCube &&
					imageSize.y() <= deviceProperties.limits.maxImageDimensionCube &&
					imageSize.z() <= deviceProperties.limits.maxImageArrayLayers;
		case IMAGE_TYPE_3D:
			return	imageSize.x() <= deviceProperties.limits.maxImageDimension3D &&
					imageSize.y() <= deviceProperties.limits.maxImageDimension3D &&
					imageSize.z() <= deviceProperties.limits.maxImageDimension3D;
		case IMAGE_TYPE_BUFFER:
			return true;
		default:
			DE_FATAL("Unknown image type");
			return false;
	}
}

VkBufferCreateInfo makeBufferCreateInfo (const VkDeviceSize			bufferSize,
										 const VkBufferUsageFlags	usage)
{
	const VkBufferCreateInfo bufferCreateInfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		0u,										// VkBufferCreateFlags	flags;
		bufferSize,								// VkDeviceSize			size;
		usage,									// VkBufferUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
		0u,										// deUint32				queueFamilyIndexCount;
		DE_NULL,								// const deUint32*		pQueueFamilyIndices;
	};
	return bufferCreateInfo;
}

VkBufferImageCopy makeBufferImageCopy (const VkExtent3D		extent,
									   const deUint32		layerCount,
									   const deUint32		mipmapLevel,
									   const VkDeviceSize	bufferOffset)
{
	const VkBufferImageCopy copyParams =
	{
		bufferOffset,																		//	VkDeviceSize				bufferOffset;
		0u,																					//	deUint32					bufferRowLength;
		0u,																					//	deUint32					bufferImageHeight;
		makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, mipmapLevel, 0u, layerCount),	//	VkImageSubresourceLayers	imageSubresource;
		makeOffset3D(0, 0, 0),																//	VkOffset3D					imageOffset;
		extent,																				//	VkExtent3D					imageExtent;
	};
	return copyParams;
}

Move<VkCommandPool> makeCommandPool (const DeviceInterface& vk, const VkDevice device, const deUint32 queueFamilyIndex)
{
	const VkCommandPoolCreateInfo commandPoolParams =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,			// VkStructureType			sType;
		DE_NULL,											// const void*				pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,	// VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,									// deUint32					queueFamilyIndex;
	};
	return createCommandPool(vk, device, &commandPoolParams);
}

Move<VkPipelineLayout> makePipelineLayout (const DeviceInterface&		vk,
										   const VkDevice				device,
										   const VkDescriptorSetLayout	descriptorSetLayout)
{
	const VkPipelineLayoutCreateInfo pipelineLayoutParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,						// VkStructureType					sType;
		DE_NULL,															// const void*						pNext;
		0u,																	// VkPipelineLayoutCreateFlags		flags;
		(descriptorSetLayout != DE_NULL ? 1u : 0u),							// deUint32							setLayoutCount;
		(descriptorSetLayout != DE_NULL ? &descriptorSetLayout : DE_NULL),	// const VkDescriptorSetLayout*		pSetLayouts;
		0u,																	// deUint32							pushConstantRangeCount;
		DE_NULL,															// const VkPushConstantRange*		pPushConstantRanges;
	};
	return createPipelineLayout(vk, device, &pipelineLayoutParams);
}

Move<VkPipeline> makeComputePipeline (const DeviceInterface&		vk,
									  const VkDevice				device,
									  const VkPipelineLayout		pipelineLayout,
									  const VkShaderModule			shaderModule,
									  const VkSpecializationInfo*	specializationInfo)
{
	const VkPipelineShaderStageCreateInfo pipelineShaderStageParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
		DE_NULL,												// const void*							pNext;
		0u,														// VkPipelineShaderStageCreateFlags		flags;
		VK_SHADER_STAGE_COMPUTE_BIT,							// VkShaderStageFlagBits				stage;
		shaderModule,											// VkShaderModule						module;
		"main",													// const char*							pName;
		specializationInfo,										// const VkSpecializationInfo*			pSpecializationInfo;
	};
	const VkComputePipelineCreateInfo pipelineCreateInfo =
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,		// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		0u,													// VkPipelineCreateFlags			flags;
		pipelineShaderStageParams,							// VkPipelineShaderStageCreateInfo	stage;
		pipelineLayout,										// VkPipelineLayout					layout;
		DE_NULL,											// VkPipeline						basePipelineHandle;
		0,													// deInt32							basePipelineIndex;
	};
	return createComputePipeline(vk, device, DE_NULL , &pipelineCreateInfo);
}

Move<VkBufferView> makeBufferView (const DeviceInterface&	vk,
								   const VkDevice			vkDevice,
								   const VkBuffer			buffer,
								   const VkFormat			format,
								   const VkDeviceSize		offset,
								   const VkDeviceSize		size)
{
	const VkBufferViewCreateInfo bufferViewParams =
	{
		VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,	// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		0u,											// VkBufferViewCreateFlags	flags;
		buffer,										// VkBuffer					buffer;
		format,										// VkFormat					format;
		offset,										// VkDeviceSize				offset;
		size,										// VkDeviceSize				range;
	};
	return createBufferView(vk, vkDevice, &bufferViewParams);
}

Move<VkImageView> makeImageView (const DeviceInterface&			vk,
								 const VkDevice					vkDevice,
								 const VkImage					image,
								 const VkImageViewType			imageViewType,
								 const VkFormat					format,
								 const VkImageSubresourceRange	subresourceRange)
{
	const VkImageViewCreateInfo imageViewParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		0u,												// VkImageViewCreateFlags	flags;
		image,											// VkImage					image;
		imageViewType,									// VkImageViewType			viewType;
		format,											// VkFormat					format;
		makeComponentMappingRGBA(),						// VkComponentMapping		components;
		subresourceRange,								// VkImageSubresourceRange	subresourceRange;
	};
	return createImageView(vk, vkDevice, &imageViewParams);
}

Move<VkDescriptorSet> makeDescriptorSet (const DeviceInterface&			vk,
										 const VkDevice					device,
										 const VkDescriptorPool			descriptorPool,
										 const VkDescriptorSetLayout	setLayout)
{
	const VkDescriptorSetAllocateInfo allocateParams =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,		// VkStructureType				sType;
		DE_NULL,											// const void*					pNext;
		descriptorPool,										// VkDescriptorPool				descriptorPool;
		1u,													// deUint32						setLayoutCount;
		&setLayout,											// const VkDescriptorSetLayout*	pSetLayouts;
	};
	return allocateDescriptorSet(vk, device, &allocateParams);
}

Move<VkFramebuffer> makeFramebuffer (const DeviceInterface&		vk,
									 const VkDevice				device,
									 const VkRenderPass			renderPass,
									 const deUint32				attachmentCount,
									 const VkImageView*			pAttachments,
									 const deUint32				width,
									 const deUint32				height,
									 const deUint32				layers)
{
	const VkFramebufferCreateInfo framebufferInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,		// VkStructureType                             sType;
		DE_NULL,										// const void*                                 pNext;
		(VkFramebufferCreateFlags)0,					// VkFramebufferCreateFlags                    flags;
		renderPass,										// VkRenderPass                                renderPass;
		attachmentCount,								// uint32_t                                    attachmentCount;
		pAttachments,									// const VkImageView*                          pAttachments;
		width,											// uint32_t                                    width;
		height,											// uint32_t                                    height;
		layers,											// uint32_t                                    layers;
	};

	return createFramebuffer(vk, device, &framebufferInfo);
}

VkBufferMemoryBarrier makeBufferMemoryBarrier (const VkAccessFlags	srcAccessMask,
											   const VkAccessFlags	dstAccessMask,
											   const VkBuffer		buffer,
											   const VkDeviceSize	offset,
											   const VkDeviceSize	bufferSizeBytes)
{
	const VkBufferMemoryBarrier barrier =
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
		DE_NULL,									// const void*		pNext;
		srcAccessMask,								// VkAccessFlags	srcAccessMask;
		dstAccessMask,								// VkAccessFlags	dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			destQueueFamilyIndex;
		buffer,										// VkBuffer			buffer;
		offset,										// VkDeviceSize		offset;
		bufferSizeBytes,							// VkDeviceSize		size;
	};
	return barrier;
}

VkImageMemoryBarrier makeImageMemoryBarrier	(const VkAccessFlags			srcAccessMask,
											 const VkAccessFlags			dstAccessMask,
											 const VkImageLayout			oldLayout,
											 const VkImageLayout			newLayout,
											 const VkImage					image,
											 const VkImageSubresourceRange	subresourceRange)
{
	const VkImageMemoryBarrier barrier =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// VkStructureType			sType;
		DE_NULL,								// const void*				pNext;
		srcAccessMask,							// VkAccessFlags			outputMask;
		dstAccessMask,							// VkAccessFlags			inputMask;
		oldLayout,								// VkImageLayout			oldLayout;
		newLayout,								// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,				// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,				// deUint32					destQueueFamilyIndex;
		image,									// VkImage					image;
		subresourceRange,						// VkImageSubresourceRange	subresourceRange;
	};
	return barrier;
}

VkImageMemoryBarrier makeImageMemoryBarrier (const VkAccessFlags			srcAccessMask,
											 const VkAccessFlags			dstAccessMask,
											 const VkImageLayout			oldLayout,
											 const VkImageLayout			newLayout,
											 const deUint32					srcQueueFamilyIndex,
											 const deUint32					destQueueFamilyIndex,
											 const VkImage					image,
											 const VkImageSubresourceRange	subresourceRange)
{
	const VkImageMemoryBarrier barrier =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// VkStructureType			sType;
		DE_NULL,								// const void*				pNext;
		srcAccessMask,							// VkAccessFlags			outputMask;
		dstAccessMask,							// VkAccessFlags			inputMask;
		oldLayout,								// VkImageLayout			oldLayout;
		newLayout,								// VkImageLayout			newLayout;
		srcQueueFamilyIndex,					// deUint32					srcQueueFamilyIndex;
		destQueueFamilyIndex,					// deUint32					destQueueFamilyIndex;
		image,									// VkImage					image;
		subresourceRange,						// VkImageSubresourceRange	subresourceRange;
	};
	return barrier;
}

VkMemoryBarrier makeMemoryBarrier (const VkAccessFlags	srcAccessMask,
									   const VkAccessFlags	dstAccessMask)
{
	const VkMemoryBarrier barrier =
	{
		VK_STRUCTURE_TYPE_MEMORY_BARRIER,	// VkStructureType			sType;
		DE_NULL,							// const void*				pNext;
		srcAccessMask,						// VkAccessFlags			outputMask;
		dstAccessMask,						// VkAccessFlags			inputMask;
	};
	return barrier;
}

de::MovePtr<Allocation> bindImage (const DeviceInterface& vk, const VkDevice device, Allocator& allocator, const VkImage image, const MemoryRequirement requirement)
{
	de::MovePtr<Allocation> alloc = allocator.allocate(getImageMemoryRequirements(vk, device, image), requirement);
	VK_CHECK(vk.bindImageMemory(device, image, alloc->getMemory(), alloc->getOffset()));
	return alloc;
}

de::MovePtr<Allocation> bindBuffer (const DeviceInterface& vk, const VkDevice device, Allocator& allocator, const VkBuffer buffer, const MemoryRequirement requirement)
{
	de::MovePtr<Allocation> alloc(allocator.allocate(getBufferMemoryRequirements(vk, device, buffer), requirement));
	VK_CHECK(vk.bindBufferMemory(device, buffer, alloc->getMemory(), alloc->getOffset()));
	return alloc;
}

void beginCommandBuffer (const DeviceInterface& vk, const VkCommandBuffer commandBuffer)
{
	const VkCommandBufferBeginInfo commandBufBeginParams =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType					sType;
		DE_NULL,										// const void*						pNext;
		0u,												// VkCommandBufferUsageFlags		flags;
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};
	VK_CHECK(vk.beginCommandBuffer(commandBuffer, &commandBufBeginParams));
}

void endCommandBuffer (const DeviceInterface& vk, const VkCommandBuffer commandBuffer)
{
	VK_CHECK(vk.endCommandBuffer(commandBuffer));
}

void submitCommands (const DeviceInterface&			vk,
					 const VkQueue					queue,
					 const VkCommandBuffer			commandBuffer,
					 const deUint32					waitSemaphoreCount,
					 const VkSemaphore*				pWaitSemaphores,
					 const VkPipelineStageFlags*	pWaitDstStageMask,
					 const deUint32					signalSemaphoreCount,
					 const VkSemaphore*				pSignalSemaphores)
{
	const VkSubmitInfo submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
		DE_NULL,						// const void*					pNext;
		waitSemaphoreCount,				// deUint32						waitSemaphoreCount;
		pWaitSemaphores,				// const VkSemaphore*			pWaitSemaphores;
		pWaitDstStageMask,				// const VkPipelineStageFlags*	pWaitDstStageMask;
		1u,								// deUint32						commandBufferCount;
		&commandBuffer,					// const VkCommandBuffer*		pCommandBuffers;
		signalSemaphoreCount,			// deUint32						signalSemaphoreCount;
		pSignalSemaphores,				// const VkSemaphore*			pSignalSemaphores;
	};

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, DE_NULL));
}

void submitCommandsAndWait (const DeviceInterface&		vk,
							const VkDevice				device,
							const VkQueue				queue,
							const VkCommandBuffer		commandBuffer,
							const deUint32				waitSemaphoreCount,
							const VkSemaphore*			pWaitSemaphores,
							const VkPipelineStageFlags*	pWaitDstStageMask,
							const deUint32				signalSemaphoreCount,
							const VkSemaphore*			pSignalSemaphores)
{
	const VkFenceCreateInfo	fenceParams =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		0u,										// VkFenceCreateFlags	flags;
	};
	const Unique<VkFence> fence(createFence(vk, device, &fenceParams));

	const VkSubmitInfo submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType				sType;
		DE_NULL,							// const void*					pNext;
		waitSemaphoreCount,					// deUint32						waitSemaphoreCount;
		pWaitSemaphores,					// const VkSemaphore*			pWaitSemaphores;
		pWaitDstStageMask,					// const VkPipelineStageFlags*	pWaitDstStageMask;
		1u,									// deUint32						commandBufferCount;
		&commandBuffer,						// const VkCommandBuffer*		pCommandBuffers;
		signalSemaphoreCount,				// deUint32						signalSemaphoreCount;
		pSignalSemaphores,					// const VkSemaphore*			pSignalSemaphores;
	};

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
	VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, ~0ull));
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


std::string getShaderImageDataType(const tcu::TextureFormat& format)
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

deUint32 getImageMipLevelSizeInBytes(const VkExtent3D& baseExtents, const deUint32 layersCount, const tcu::TextureFormat& format, const deUint32 mipmapLevel, const deUint32 mipmapMemoryAlignment)
{
	const VkExtent3D extents = mipLevelExtents(baseExtents, mipmapLevel);

	return deAlign32(extents.width * extents.height * extents.depth * layersCount * tcu::getPixelSize(format), mipmapMemoryAlignment);
}

deUint32 getImageSizeInBytes(const VkExtent3D& baseExtents, const deUint32 layersCount, const tcu::TextureFormat& format, const deUint32 mipmapLevelsCount, const deUint32 mipmapMemoryAlignment)
{
	deUint32 imageSizeInBytes = 0;
	for (deUint32 mipmapLevel = 0; mipmapLevel < mipmapLevelsCount; ++mipmapLevel)
		imageSizeInBytes += getImageMipLevelSizeInBytes(baseExtents, layersCount, format, mipmapLevel, mipmapMemoryAlignment);

	return imageSizeInBytes;
}

VkSparseImageMemoryBind	makeSparseImageMemoryBind  (const DeviceInterface&			vk,
													const VkDevice					device,
													const VkDeviceSize				allocationSize,
													const deUint32					memoryType,
													const VkImageSubresource&		subresource,
													const VkOffset3D&				offset,
													const VkExtent3D&				extent)
{
	const VkMemoryAllocateInfo	allocInfo =
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	//	VkStructureType			sType;
		DE_NULL,								//	const void*				pNext;
		allocationSize,							//	VkDeviceSize			allocationSize;
		memoryType,								//	deUint32				memoryTypeIndex;
	};

	VkDeviceMemory deviceMemory = 0;
	VK_CHECK(vk.allocateMemory(device, &allocInfo, DE_NULL, &deviceMemory));

	VkSparseImageMemoryBind imageMemoryBind;

	imageMemoryBind.subresource		= subresource;
	imageMemoryBind.memory			= deviceMemory;
	imageMemoryBind.memoryOffset	= 0u;
	imageMemoryBind.flags			= 0u;
	imageMemoryBind.offset			= offset;
	imageMemoryBind.extent			= extent;

	return imageMemoryBind;
}

VkSparseMemoryBind makeSparseMemoryBind	(const DeviceInterface&			vk,
										 const VkDevice					device,
										 const VkDeviceSize				allocationSize,
										 const deUint32					memoryType,
										 const VkDeviceSize				resourceOffset,
										 const VkSparseMemoryBindFlags	flags)
{
	const VkMemoryAllocateInfo allocInfo =
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	//	VkStructureType	sType;
		DE_NULL,								//	const void*		pNext;
		allocationSize,							//	VkDeviceSize	allocationSize;
		memoryType,								//	deUint32		memoryTypeIndex;
	};

	VkDeviceMemory deviceMemory = 0;
	VK_CHECK(vk.allocateMemory(device, &allocInfo, DE_NULL, &deviceMemory));

	VkSparseMemoryBind memoryBind;

	memoryBind.resourceOffset	= resourceOffset;
	memoryBind.size				= allocationSize;
	memoryBind.memory			= deviceMemory;
	memoryBind.memoryOffset		= 0u;
	memoryBind.flags			= flags;

	return memoryBind;
}

void requireFeatures (const InstanceInterface& vki, const VkPhysicalDevice physDevice, const FeatureFlags flags)
{
	const VkPhysicalDeviceFeatures features = getPhysicalDeviceFeatures(vki, physDevice);

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

deUint32 findMatchingMemoryType (const InstanceInterface&		instance,
								 const VkPhysicalDevice			physicalDevice,
								 const VkMemoryRequirements&	objectMemoryRequirements,
								 const MemoryRequirement&		memoryRequirement)
{
	const VkPhysicalDeviceMemoryProperties deviceMemoryProperties = getPhysicalDeviceMemoryProperties(instance, physicalDevice);

	for (deUint32 memoryTypeNdx = 0; memoryTypeNdx < deviceMemoryProperties.memoryTypeCount; ++memoryTypeNdx)
	{
		if ((objectMemoryRequirements.memoryTypeBits & (1u << memoryTypeNdx)) != 0 &&
			memoryRequirement.matchesHeap(deviceMemoryProperties.memoryTypes[memoryTypeNdx].propertyFlags))
		{
			return memoryTypeNdx;
		}
	}

	return NO_MATCH_FOUND;
}

bool checkSparseSupportForImageType (const InstanceInterface&	instance,
									 const VkPhysicalDevice		physicalDevice,
									 const ImageType			imageType)
{
	const VkPhysicalDeviceFeatures deviceFeatures = getPhysicalDeviceFeatures(instance, physicalDevice);

	if (!deviceFeatures.sparseBinding)
		return false;

	switch (mapImageType(imageType))
	{
		case VK_IMAGE_TYPE_2D:
			return deviceFeatures.sparseResidencyImage2D == VK_TRUE;
		case VK_IMAGE_TYPE_3D:
			return deviceFeatures.sparseResidencyImage3D == VK_TRUE;
		default:
			DE_ASSERT(0);
			return false;
	};
}

bool checkSparseSupportForImageFormat (const InstanceInterface&	instance,
									   const VkPhysicalDevice	physicalDevice,
									   const VkImageCreateInfo&	imageInfo)
{
	const std::vector<VkSparseImageFormatProperties> sparseImageFormatPropVec = getPhysicalDeviceSparseImageFormatProperties(
		instance, physicalDevice, imageInfo.format, imageInfo.imageType, imageInfo.samples, imageInfo.usage, imageInfo.tiling);

	return sparseImageFormatPropVec.size() > 0u;
}

bool checkImageFormatFeatureSupport (const InstanceInterface&	instance,
									 const VkPhysicalDevice		physicalDevice,
									 const VkFormat				format,
									 const VkFormatFeatureFlags	featureFlags)
{
	const VkFormatProperties formatProperties = getPhysicalDeviceFormatProperties(instance, physicalDevice, format);

	return (formatProperties.optimalTilingFeatures & featureFlags) == featureFlags;
}

deUint32 getSparseAspectRequirementsIndex (const std::vector<VkSparseImageMemoryRequirements>&	requirements,
										   const VkImageAspectFlags								aspectFlags)
{
	for (deUint32 memoryReqNdx = 0; memoryReqNdx < requirements.size(); ++memoryReqNdx)
	{
		if (requirements[memoryReqNdx].formatProperties.aspectMask & aspectFlags)
			return memoryReqNdx;
	}

	return NO_MATCH_FOUND;
}

} // sparse
} // vkt
