/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
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
 * \brief Vulkan object reference holder utilities.
 *//*--------------------------------------------------------------------*/

#include "vkRefUtil.hpp"

namespace vk
{

#include "vkRefUtilImpl.inl"

Move<VkPipeline> createGraphicsPipeline (const DeviceInterface&					vk,
										 VkDevice								device,
										 VkPipelineCache						pipelineCache,
										 const VkGraphicsPipelineCreateInfo*	pCreateInfo,
										 const VkAllocationCallbacks*			pAllocator)
{
	VkPipeline object = 0;
	VK_CHECK(vk.createGraphicsPipelines(device, pipelineCache, 1u, pCreateInfo, pAllocator, &object));
	return Move<VkPipeline>(check<VkPipeline>(object), Deleter<VkPipeline>(vk, device, pAllocator));
}

Move<VkPipeline> createComputePipeline (const DeviceInterface&				vk,
										VkDevice							device,
										VkPipelineCache						pipelineCache,
										const VkComputePipelineCreateInfo*	pCreateInfo,
										const VkAllocationCallbacks*		pAllocator)
{
	VkPipeline object = 0;
	VK_CHECK(vk.createComputePipelines(device, pipelineCache, 1u, pCreateInfo, pAllocator, &object));
	return Move<VkPipeline>(check<VkPipeline>(object), Deleter<VkPipeline>(vk, device, pAllocator));
}

Move<VkCommandBuffer> allocateCommandBuffer (const DeviceInterface& vk, VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo)
{
	VkCommandBuffer object = 0;
	DE_ASSERT(pAllocateInfo->commandBufferCount == 1u);
	VK_CHECK(vk.allocateCommandBuffers(device, pAllocateInfo, &object));
	return Move<VkCommandBuffer>(check<VkCommandBuffer>(object), Deleter<VkCommandBuffer>(vk, device, pAllocateInfo->commandPool));
}

Move<VkDescriptorSet> allocateDescriptorSet (const DeviceInterface& vk, VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo)
{
	VkDescriptorSet object = 0;
	DE_ASSERT(pAllocateInfo->descriptorSetCount == 1u);
	VK_CHECK(vk.allocateDescriptorSets(device, pAllocateInfo, &object));
	return Move<VkDescriptorSet>(check<VkDescriptorSet>(object), Deleter<VkDescriptorSet>(vk, device, pAllocateInfo->descriptorPool));
}

Move<VkSemaphore> createSemaphore (const DeviceInterface&		vk,
								   VkDevice						device,
								   VkSemaphoreCreateFlags		flags,
								   const VkAllocationCallbacks*	pAllocator)
{
	const VkSemaphoreCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		DE_NULL,

		flags
	};

	return createSemaphore(vk, device, &createInfo, pAllocator);
}

Move<VkFence> createFence (const DeviceInterface&		vk,
						   VkDevice						device,
						   VkFenceCreateFlags			flags,
						   const VkAllocationCallbacks*	pAllocator)
{
	const VkFenceCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		DE_NULL,

		flags
	};

	return createFence(vk, device, &createInfo, pAllocator);
}

Move<VkCommandPool> createCommandPool (const DeviceInterface&		vk,
									   VkDevice						device,
									   VkCommandPoolCreateFlags		flags,
									   deUint32						queueFamilyIndex,
									   const VkAllocationCallbacks*	pAllocator)
{
	const VkCommandPoolCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		DE_NULL,

		flags,
		queueFamilyIndex
	};

	return createCommandPool(vk, device, &createInfo, pAllocator);
}

Move<VkCommandBuffer> allocateCommandBuffer (const DeviceInterface&	vk,
											 VkDevice				device,
											 VkCommandPool			commandPool,
											 VkCommandBufferLevel	level)
{
	const VkCommandBufferAllocateInfo allocInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		DE_NULL,

		commandPool,
		level,
		1
	};

	return allocateCommandBuffer(vk, device, &allocInfo);
}

Move<VkEvent> createEvent (const DeviceInterface&		vk,
						   VkDevice						device,
						   VkEventCreateFlags			flags,
						   const VkAllocationCallbacks*	pAllocateInfo)
{
	const VkEventCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
		DE_NULL,

		flags
	};

	return createEvent(vk, device, &createInfo, pAllocateInfo);
}

} // vk
