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
 * \brief Object creation utilities
 *//*--------------------------------------------------------------------*/

#include "vktFragmentOperationsMakeUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include <vector>

namespace vkt
{
namespace FragmentOperations
{
using namespace vk;
using de::MovePtr;

VkBufferCreateInfo makeBufferCreateInfo (const VkDeviceSize			bufferSize,
										 const VkBufferUsageFlags	usage)
{
	const VkBufferCreateInfo bufferCreateInfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		(VkBufferCreateFlags)0,					// VkBufferCreateFlags	flags;
		bufferSize,								// VkDeviceSize			size;
		usage,									// VkBufferUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
		0u,										// deUint32				queueFamilyIndexCount;
		DE_NULL,								// const deUint32*		pQueueFamilyIndices;
	};
	return bufferCreateInfo;
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
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		srcAccessMask,									// VkAccessFlags			outputMask;
		dstAccessMask,									// VkAccessFlags			inputMask;
		oldLayout,										// VkImageLayout			oldLayout;
		newLayout,										// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
		image,											// VkImage					image;
		subresourceRange,								// VkImageSubresourceRange	subresourceRange;
	};
	return barrier;
}

Move<VkDescriptorSet> makeDescriptorSet (const DeviceInterface&			vk,
										 const VkDevice					device,
										 const VkDescriptorPool			descriptorPool,
										 const VkDescriptorSetLayout	setLayout)
{
	const VkDescriptorSetAllocateInfo info =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,		// VkStructureType				sType;
		DE_NULL,											// const void*					pNext;
		descriptorPool,										// VkDescriptorPool				descriptorPool;
		1u,													// deUint32						descriptorSetCount;
		&setLayout,											// const VkDescriptorSetLayout*	pSetLayouts;
	};
	return allocateDescriptorSet(vk, device, &info);
}

Move<VkPipelineLayout> makePipelineLayout (const DeviceInterface&		vk,
										   const VkDevice				device)
{
	const VkPipelineLayoutCreateInfo info =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType				sType;
		DE_NULL,											// const void*					pNext;
		(VkPipelineLayoutCreateFlags)0,						// VkPipelineLayoutCreateFlags	flags;
		0u,													// deUint32						setLayoutCount;
		DE_NULL,											// const VkDescriptorSetLayout*	pSetLayouts;
		0u,													// deUint32						pushConstantRangeCount;
		DE_NULL,											// const VkPushConstantRange*	pPushConstantRanges;
	};
	return createPipelineLayout(vk, device, &info);
}

Move<VkPipelineLayout> makePipelineLayout (const DeviceInterface&		vk,
										   const VkDevice				device,
										   const VkDescriptorSetLayout	descriptorSetLayout)
{
	const VkPipelineLayoutCreateInfo info =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType				sType;
		DE_NULL,											// const void*					pNext;
		(VkPipelineLayoutCreateFlags)0,						// VkPipelineLayoutCreateFlags	flags;
		1u,													// deUint32						setLayoutCount;
		&descriptorSetLayout,								// const VkDescriptorSetLayout*	pSetLayouts;
		0u,													// deUint32						pushConstantRangeCount;
		DE_NULL,											// const VkPushConstantRange*	pPushConstantRanges;
	};
	return createPipelineLayout(vk, device, &info);
}

Move<VkPipeline> makeComputePipeline (const DeviceInterface&		vk,
									  const VkDevice				device,
									  const VkPipelineLayout		pipelineLayout,
									  const VkShaderModule			shaderModule,
									  const VkSpecializationInfo*	specInfo)
{
	const VkPipelineShaderStageCreateInfo shaderStageInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType					sType;
		DE_NULL,												// const void*						pNext;
		(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags	flags;
		VK_SHADER_STAGE_COMPUTE_BIT,							// VkShaderStageFlagBits			stage;
		shaderModule,											// VkShaderModule					module;
		"main",													// const char*						pName;
		specInfo,												// const VkSpecializationInfo*		pSpecializationInfo;
	};
	const VkComputePipelineCreateInfo pipelineInfo =
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,		// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		(VkPipelineCreateFlags)0,							// VkPipelineCreateFlags			flags;
		shaderStageInfo,									// VkPipelineShaderStageCreateInfo	stage;
		pipelineLayout,										// VkPipelineLayout					layout;
		DE_NULL,											// VkPipeline						basePipelineHandle;
		0,													// deInt32							basePipelineIndex;
	};
	return createComputePipeline(vk, device, DE_NULL , &pipelineInfo);
}

Move<VkImageView> makeImageView (const DeviceInterface&			vk,
								 const VkDevice					vkDevice,
								 const VkImage					image,
								 const VkImageViewType			viewType,
								 const VkFormat					format,
								 const VkImageSubresourceRange	subresourceRange)
{
	const VkImageViewCreateInfo imageViewParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		(VkImageViewCreateFlags)0,						// VkImageViewCreateFlags	flags;
		image,											// VkImage					image;
		viewType,										// VkImageViewType			viewType;
		format,											// VkFormat					format;
		makeComponentMappingRGBA(),						// VkComponentMapping		components;
		subresourceRange,								// VkImageSubresourceRange	subresourceRange;
	};
	return createImageView(vk, vkDevice, &imageViewParams);
}

void beginCommandBuffer (const DeviceInterface& vk, const VkCommandBuffer commandBuffer)
{
	const VkCommandBufferBeginInfo info =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType                          sType;
		DE_NULL,										// const void*                              pNext;
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,	// VkCommandBufferUsageFlags                flags;
		DE_NULL,										// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
	};
	VK_CHECK(vk.beginCommandBuffer(commandBuffer, &info));
}

void submitCommandsAndWait (const DeviceInterface&	vk,
							const VkDevice			device,
							const VkQueue			queue,
							const VkCommandBuffer	commandBuffer)
{
	const Unique<VkFence> fence(createFence(vk, device));

	const VkSubmitInfo submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType                sType;
		DE_NULL,							// const void*                    pNext;
		0u,									// uint32_t                       waitSemaphoreCount;
		DE_NULL,							// const VkSemaphore*             pWaitSemaphores;
		DE_NULL,							// const VkPipelineStageFlags*    pWaitDstStageMask;
		1u,									// uint32_t                       commandBufferCount;
		&commandBuffer,						// const VkCommandBuffer*         pCommandBuffers;
		0u,									// uint32_t                       signalSemaphoreCount;
		DE_NULL,							// const VkSemaphore*             pSignalSemaphores;
	};
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
	VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, ~0ull));
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
	const VkFramebufferCreateInfo framebufferInfo = {
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

MovePtr<Allocation> bindImage (const DeviceInterface& vk, const VkDevice device, Allocator& allocator, const VkImage image, const MemoryRequirement requirement)
{
	MovePtr<Allocation> alloc = allocator.allocate(getImageMemoryRequirements(vk, device, image), requirement);
	VK_CHECK(vk.bindImageMemory(device, image, alloc->getMemory(), alloc->getOffset()));
	return alloc;
}

MovePtr<Allocation> bindBuffer (const DeviceInterface& vk, const VkDevice device, Allocator& allocator, const VkBuffer buffer, const MemoryRequirement requirement)
{
	MovePtr<Allocation> alloc(allocator.allocate(getBufferMemoryRequirements(vk, device, buffer), requirement));
	VK_CHECK(vk.bindBufferMemory(device, buffer, alloc->getMemory(), alloc->getOffset()));
	return alloc;
}

} // FragmentOperations
} // vkt
