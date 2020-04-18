#ifndef _VKTFRAGMENTOPERATIONSMAKEUTIL_HPP
#define _VKTFRAGMENTOPERATIONSMAKEUTIL_HPP
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

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkMemUtil.hpp"
#include "deUniquePtr.hpp"
#include "tcuVector.hpp"

namespace vkt
{
namespace FragmentOperations
{

vk::VkBufferCreateInfo			makeBufferCreateInfo	(const vk::VkDeviceSize bufferSize, const vk::VkBufferUsageFlags usage);
vk::Move<vk::VkDescriptorSet>	makeDescriptorSet		(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkDescriptorPool descriptorPool, const vk::VkDescriptorSetLayout setLayout);
vk::Move<vk::VkPipelineLayout>	makePipelineLayout		(const vk::DeviceInterface& vk, const vk::VkDevice device);
vk::Move<vk::VkPipelineLayout>	makePipelineLayout		(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkDescriptorSetLayout descriptorSetLayout);
vk::Move<vk::VkPipeline>		makeComputePipeline		(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkPipelineLayout pipelineLayout, const vk::VkShaderModule shaderModule, const vk::VkSpecializationInfo* specInfo);
vk::Move<vk::VkFramebuffer>		makeFramebuffer			(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkRenderPass renderPass, const deUint32 attachmentCount, const vk::VkImageView* pAttachments, const deUint32 width, const deUint32 height, const deUint32 layers = 1u);
vk::Move<vk::VkImageView>		makeImageView			(const vk::DeviceInterface& vk, const vk::VkDevice vkDevice, const vk::VkImage image, const vk::VkImageViewType viewType, const vk::VkFormat format, const vk::VkImageSubresourceRange subresourceRange);
vk::VkBufferMemoryBarrier		makeBufferMemoryBarrier	(const vk::VkAccessFlags srcAccessMask, const vk::VkAccessFlags dstAccessMask, const vk::VkBuffer buffer, const vk::VkDeviceSize offset, const vk::VkDeviceSize bufferSizeBytes);
vk::VkImageMemoryBarrier		makeImageMemoryBarrier	(const vk::VkAccessFlags srcAccessMask, const vk::VkAccessFlags dstAccessMask, const vk::VkImageLayout oldLayout, const vk::VkImageLayout newLayout, const vk::VkImage image, const vk::VkImageSubresourceRange subresourceRange);
de::MovePtr<vk::Allocation>		bindImage				(const vk::DeviceInterface& vk, const vk::VkDevice device, vk::Allocator& allocator, const vk::VkImage image, const vk::MemoryRequirement requirement);
de::MovePtr<vk::Allocation>		bindBuffer				(const vk::DeviceInterface& vk, const vk::VkDevice device, vk::Allocator& allocator, const vk::VkBuffer buffer, const vk::MemoryRequirement requirement);
void							beginCommandBuffer		(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer);
void							submitCommandsAndWait	(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkQueue queue, const vk::VkCommandBuffer commandBuffer);

inline vk::Move<vk::VkBuffer> makeBuffer (const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkBufferCreateInfo& createInfo)
{
	return createBuffer(vk, device, &createInfo);
}

inline vk::Move<vk::VkImage> makeImage (const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkImageCreateInfo& createInfo)
{
	return createImage(vk, device, &createInfo);
}

} // FragmentOperations
} // vkt

#endif // _VKTFRAGMENTOPERATIONSMAKEUTIL_HPP
