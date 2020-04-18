#ifndef _VKTGEOMETRYTESTSUTIL_HPP
#define _VKTGEOMETRYTESTSUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Geometry Utilities
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkMemUtil.hpp"
#include "vkRef.hpp"
#include "vkPrograms.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vktTestCase.hpp"

#include "tcuVector.hpp"
#include "tcuTexture.hpp"

#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

namespace vkt
{
namespace geometry
{

struct PrimitiveTestSpec
{
	vk::VkPrimitiveTopology		primitiveType;
	const char*					name;
	vk::VkPrimitiveTopology		outputType;
};

class Buffer
{
public:
										Buffer			(const vk::DeviceInterface&		vk,
														 const vk::VkDevice				device,
														 vk::Allocator&					allocator,
														 const vk::VkBufferCreateInfo&	bufferCreateInfo,
														 const vk::MemoryRequirement	memoryRequirement)

											: m_buffer		(createBuffer(vk, device, &bufferCreateInfo))
											, m_allocation	(allocator.allocate(getBufferMemoryRequirements(vk, device, *m_buffer), memoryRequirement))
										{
											VK_CHECK(vk.bindBufferMemory(device, *m_buffer, m_allocation->getMemory(), m_allocation->getOffset()));
										}

	const vk::VkBuffer&					get				(void) const { return *m_buffer; }
	const vk::VkBuffer&					operator*		(void) const { return get(); }
	vk::Allocation&						getAllocation	(void) const { return *m_allocation; }

private:
	const vk::Unique<vk::VkBuffer>		m_buffer;
	const de::UniquePtr<vk::Allocation>	m_allocation;

	// "deleted"
										Buffer			(const Buffer&);
	Buffer&								operator=		(const Buffer&);
};

class Image
{
public:
										Image			(const vk::DeviceInterface&		vk,
														 const vk::VkDevice				device,
														 vk::Allocator&					allocator,
														 const vk::VkImageCreateInfo&	imageCreateInfo,
														 const vk::MemoryRequirement	memoryRequirement)

											: m_image		(createImage(vk, device, &imageCreateInfo))
											, m_allocation	(allocator.allocate(getImageMemoryRequirements(vk, device, *m_image), memoryRequirement))
										{
											VK_CHECK(vk.bindImageMemory(device, *m_image, m_allocation->getMemory(), m_allocation->getOffset()));
										}

	const vk::VkImage&					get				(void) const { return *m_image; }
	const vk::VkImage&					operator*		(void) const { return get(); }
	vk::Allocation&						getAllocation	(void) const { return *m_allocation; }

private:
	const vk::Unique<vk::VkImage>		m_image;
	const de::UniquePtr<vk::Allocation>	m_allocation;

	// "deleted"
										Image			(const Image&);
	Image&								operator=		(const Image&);
};

class GraphicsPipelineBuilder
{
public:
								GraphicsPipelineBuilder	(void) : m_renderSize			(0, 0)
															   , m_shaderStageFlags		(0u)
															   , m_cullModeFlags		(vk::VK_CULL_MODE_NONE)
															   , m_frontFace			(vk::VK_FRONT_FACE_COUNTER_CLOCKWISE)
															   , m_patchControlPoints	(1u)
															   , m_blendEnable			(false)
															   , m_primitiveTopology	(vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) {}

	GraphicsPipelineBuilder&	setRenderSize					(const tcu::IVec2& size) { m_renderSize = size; return *this; }
	GraphicsPipelineBuilder&	setShader						(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkShaderStageFlagBits stage, const vk::ProgramBinary& binary, const vk::VkSpecializationInfo* specInfo);
	GraphicsPipelineBuilder&	setPatchControlPoints			(const deUint32 controlPoints) { m_patchControlPoints = controlPoints; return *this; }
	GraphicsPipelineBuilder&	setCullModeFlags				(const vk::VkCullModeFlags cullModeFlags) { m_cullModeFlags = cullModeFlags; return *this; }
	GraphicsPipelineBuilder&	setFrontFace					(const vk::VkFrontFace frontFace) { m_frontFace = frontFace; return *this; }
	GraphicsPipelineBuilder&	setBlend						(const bool enable) { m_blendEnable = enable; return *this; }

	//! Applies only to pipelines without tessellation shaders.
	GraphicsPipelineBuilder&	setPrimitiveTopology			(const vk::VkPrimitiveTopology topology) { m_primitiveTopology = topology; return *this; }

	GraphicsPipelineBuilder&	addVertexBinding				(const vk::VkVertexInputBindingDescription vertexBinding) { m_vertexInputBindings.push_back(vertexBinding); return *this; }
	GraphicsPipelineBuilder&	addVertexAttribute				(const vk::VkVertexInputAttributeDescription vertexAttribute) { m_vertexInputAttributes.push_back(vertexAttribute); return *this; }

	//! Basic vertex input configuration (uses biding 0, location 0, etc.)
	GraphicsPipelineBuilder&	setVertexInputSingleAttribute	(const vk::VkFormat vertexFormat, const deUint32 stride);

	vk::Move<vk::VkPipeline>	build							(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkPipelineLayout pipelineLayout, const vk::VkRenderPass renderPass);

private:
	tcu::IVec2											m_renderSize;
	vk::Move<vk::VkShaderModule>						m_vertexShaderModule;
	vk::Move<vk::VkShaderModule>						m_fragmentShaderModule;
	vk::Move<vk::VkShaderModule>						m_geometryShaderModule;
	vk::Move<vk::VkShaderModule>						m_tessControlShaderModule;
	vk::Move<vk::VkShaderModule>						m_tessEvaluationShaderModule;
	std::vector<vk::VkPipelineShaderStageCreateInfo>	m_shaderStages;
	std::vector<vk::VkVertexInputBindingDescription>	m_vertexInputBindings;
	std::vector<vk::VkVertexInputAttributeDescription>	m_vertexInputAttributes;
	vk::VkShaderStageFlags								m_shaderStageFlags;
	vk::VkCullModeFlags									m_cullModeFlags;
	vk::VkFrontFace										m_frontFace;
	deUint32											m_patchControlPoints;
	bool												m_blendEnable;
	vk::VkPrimitiveTopology								m_primitiveTopology;

	GraphicsPipelineBuilder (const GraphicsPipelineBuilder&); // "deleted"
	GraphicsPipelineBuilder& operator= (const GraphicsPipelineBuilder&);
};

template<typename T>
inline std::size_t sizeInBytes (const std::vector<T>& vec)
{
	return vec.size() * sizeof(vec[0]);
}

std::string						inputTypeToGLString			(const vk::VkPrimitiveTopology& inputType);
std::string						outputTypeToGLString		(const vk::VkPrimitiveTopology& outputType);
std::size_t						calcOutputVertices			(const vk::VkPrimitiveTopology& inputType);

vk::VkBufferCreateInfo			makeBufferCreateInfo		(const vk::VkDeviceSize bufferSize, const vk::VkBufferUsageFlags usage);
vk::VkImageCreateInfo			makeImageCreateInfo			(const tcu::IVec2& size, const vk::VkFormat format, const vk::VkImageUsageFlags usage, const deUint32 numArrayLayers = 1u);
vk::Move<vk::VkDescriptorSet>	makeDescriptorSet			(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkDescriptorPool descriptorPool, const vk::VkDescriptorSetLayout setLayout);
vk::Move<vk::VkRenderPass>		makeRenderPass				(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkFormat colorFormat);
vk::Move<vk::VkImageView>		makeImageView				(const vk::DeviceInterface& vk, const vk::VkDevice vkDevice, const vk::VkImage image, const vk::VkImageViewType viewType, const vk::VkFormat format, const vk::VkImageSubresourceRange subresourceRange);
vk::VkBufferImageCopy			makeBufferImageCopy			(const vk::VkExtent3D extent, const vk::VkImageSubresourceLayers subresourceLayers);
vk::Move<vk::VkPipelineLayout>	makePipelineLayout			(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkDescriptorSetLayout descriptorSetLayout = DE_NULL);
vk::Move<vk::VkFramebuffer>		makeFramebuffer				(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkRenderPass renderPass, const vk::VkImageView colorAttachment, const deUint32 width, const deUint32 height, const deUint32 layers);
vk::VkImageMemoryBarrier		makeImageMemoryBarrier		(const vk::VkAccessFlags srcAccessMask, const vk::VkAccessFlags dstAccessMask, const vk::VkImageLayout oldLayout, const vk::VkImageLayout newLayout, const vk::VkImage image, const vk::VkImageSubresourceRange subresourceRange);
vk::VkBufferMemoryBarrier		makeBufferMemoryBarrier		(const vk::VkAccessFlags srcAccessMask, const vk::VkAccessFlags dstAccessMask, const vk::VkBuffer buffer, const vk::VkDeviceSize offset, const vk::VkDeviceSize bufferSizeBytes);
de::MovePtr<vk::Allocation>		bindImage					(const vk::DeviceInterface& vk, const vk::VkDevice device, vk::Allocator& allocator, const vk::VkImage image, const vk::MemoryRequirement requirement);
de::MovePtr<vk::Allocation>		bindBuffer					(const vk::DeviceInterface& vk, const vk::VkDevice device, vk::Allocator& allocator, const vk::VkBuffer buffer, const vk::MemoryRequirement requirement);

void							beginRenderPass				(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer, const vk::VkRenderPass renderPass, const vk::VkFramebuffer framebuffer, const vk::VkRect2D& renderArea, const tcu::Vec4& clearColor);
void							endRenderPass				(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer);
void							beginCommandBuffer			(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer);
void							endCommandBuffer			(const vk::DeviceInterface& vk, const vk::VkCommandBuffer commandBuffer);
void							submitCommandsAndWait		(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkQueue queue, const vk::VkCommandBuffer commandBuffer);

bool							compareWithFileImage		(Context& context, const tcu::ConstPixelBufferAccess& resultImage, std::string name);

void							zeroBuffer					(const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::Allocation& alloc, const vk::VkDeviceSize size);

void							checkGeometryShaderSupport	(const vk::InstanceInterface& vki, const vk::VkPhysicalDevice physDevice, const int numGeometryShaderInvocations = 0);
vk::VkBool32					checkPointSize				(const vk::InstanceInterface& vki, const vk::VkPhysicalDevice physDevice);

inline vk::Move<vk::VkBuffer> makeBuffer (const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkBufferCreateInfo& createInfo)
{
	return createBuffer(vk, device, &createInfo);
}

inline vk::Move<vk::VkImage> makeImage (const vk::DeviceInterface& vk, const vk::VkDevice device, const vk::VkImageCreateInfo& createInfo)
{
	return createImage(vk, device, &createInfo);
}

} //vkt
} //geometry

#endif // _VKTGEOMETRYTESTSUTIL_HPP
