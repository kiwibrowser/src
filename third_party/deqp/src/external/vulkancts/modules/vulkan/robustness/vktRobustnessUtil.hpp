#ifndef _VKTROBUSTNESSUTIL_HPP
#define _VKTROBUSTNESSUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Imagination Technologies Ltd.
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
 * \brief Robustness Utilities
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vkDefs.hpp"
#include "vkRefUtil.hpp"
#include "vktTestCase.hpp"
#include "vkMemUtil.hpp"
#include "deUniquePtr.hpp"
#include "tcuVectorUtil.hpp"

namespace vkt
{
namespace robustness
{

vk::Move<vk::VkDevice>	createRobustBufferAccessDevice		(Context& context);
bool					areEqual							(float a, float b);
bool					isValueZero							(const void* valuePtr, size_t valueSize);
bool					isValueWithinBuffer					(const void* buffer, vk::VkDeviceSize bufferSize, const void* valuePtr, size_t valueSizeInBytes);
bool					isValueWithinBufferOrZero			(const void* buffer, vk::VkDeviceSize bufferSize, const void* valuePtr, size_t valueSizeInBytes);
bool					verifyOutOfBoundsVec4				(const void* vecPtr, vk::VkFormat bufferFormat);
void					populateBufferWithTestValues		(void* buffer, vk::VkDeviceSize size, vk::VkFormat format);
void					logValue							(std::ostringstream& logMsg, const void* valuePtr, vk::VkFormat valueFormat, size_t valueSize);

class TestEnvironment
{
public:
									TestEnvironment		(Context&					context,
														 vk::VkDevice				device,
														 vk::VkDescriptorSetLayout	descriptorSetLayout,
														 vk::VkDescriptorSet		descriptorSet);

	virtual							~TestEnvironment	(void) {}

	virtual vk::VkCommandBuffer		getCommandBuffer	(void);

protected:
	Context&						m_context;
	vk::VkDevice					m_device;
	vk::VkDescriptorSetLayout		m_descriptorSetLayout;
	vk::VkDescriptorSet				m_descriptorSet;

	vk::Move<vk::VkCommandPool>		m_commandPool;
	vk::Move<vk::VkCommandBuffer>	m_commandBuffer;
};

class GraphicsEnvironment: public TestEnvironment
{
public:
	typedef std::vector<vk::VkVertexInputBindingDescription>	VertexBindings;
	typedef std::vector<vk::VkVertexInputAttributeDescription>	VertexAttributes;

	struct DrawConfig
	{
		std::vector<vk::VkBuffer>	vertexBuffers;
		deUint32					vertexCount;
		deUint32					instanceCount;

		vk::VkBuffer				indexBuffer;
		deUint32					indexCount;
	};

									GraphicsEnvironment		(Context&					context,
															 vk::VkDevice				device,
															 vk::VkDescriptorSetLayout	descriptorSetLayout,
															 vk::VkDescriptorSet		descriptorSet,
															 const VertexBindings&		vertexBindings,
															 const VertexAttributes&	vertexAttributes,
															 const DrawConfig&			drawConfig);

	virtual							~GraphicsEnvironment	(void) {}

private:
	const tcu::UVec2				m_renderSize;
	const vk::VkFormat				m_colorFormat;

	vk::Move<vk::VkImage>			m_colorImage;
	de::MovePtr<vk::Allocation>		m_colorImageAlloc;
	vk::Move<vk::VkImageView>		m_colorAttachmentView;
	vk::Move<vk::VkRenderPass>		m_renderPass;
	vk::Move<vk::VkFramebuffer>		m_framebuffer;

	vk::Move<vk::VkShaderModule>	m_vertexShaderModule;
	vk::Move<vk::VkShaderModule>	m_fragmentShaderModule;

	vk::Move<vk::VkBuffer>			m_vertexBuffer;
	de::MovePtr<vk::Allocation>		m_vertexBufferAlloc;

	vk::Move<vk::VkPipelineLayout>	m_pipelineLayout;
	vk::Move<vk::VkPipeline>		m_graphicsPipeline;
};

class ComputeEnvironment: public TestEnvironment
{
public:
									ComputeEnvironment		(Context&					context,
															 vk::VkDevice				device,
															 vk::VkDescriptorSetLayout	descriptorSetLayout,
															 vk::VkDescriptorSet		descriptorSet);

	virtual							~ComputeEnvironment		(void) {}

private:
	vk::Move<vk::VkShaderModule>	m_computeShaderModule;
	vk::Move<vk::VkPipelineLayout>	m_pipelineLayout;
	vk::Move<vk::VkPipeline>		m_computePipeline;
};

} // robustness
} // vkt

#endif // _VKTROBUSTNESSUTIL_HPP
