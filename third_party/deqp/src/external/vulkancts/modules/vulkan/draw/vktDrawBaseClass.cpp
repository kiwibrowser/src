/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Intel Corporation
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
 * \brief Command draw Tests - Base Class
 *//*--------------------------------------------------------------------*/

#include "vktDrawBaseClass.hpp"

namespace vkt
{
namespace Draw
{

DrawTestsBaseClass::DrawTestsBaseClass (Context& context, const char* vertexShaderName, const char* fragmentShaderName, vk::VkPrimitiveTopology topology)
	: TestInstance				(context)
	, m_colorAttachmentFormat	(vk::VK_FORMAT_R8G8B8A8_UNORM)
	, m_topology				(topology)
	, m_vk						(context.getDeviceInterface())
	, m_vertexShaderName		(vertexShaderName)
	, m_fragmentShaderName		(fragmentShaderName)
{
}

void DrawTestsBaseClass::initialize (void)
{
	const vk::VkDevice device				= m_context.getDevice();
	const deUint32 queueFamilyIndex			= m_context.getUniversalQueueFamilyIndex();

	const PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	m_pipelineLayout						= vk::createPipelineLayout(m_vk, device, &pipelineLayoutCreateInfo);

	const vk::VkExtent3D targetImageExtent	= { WIDTH, HEIGHT, 1 };
	const ImageCreateInfo targetImageCreateInfo(vk::VK_IMAGE_TYPE_2D, m_colorAttachmentFormat, targetImageExtent, 1, 1, vk::VK_SAMPLE_COUNT_1_BIT,
		vk::VK_IMAGE_TILING_OPTIMAL, vk::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT | vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	m_colorTargetImage						= Image::createAndAlloc(m_vk, device, targetImageCreateInfo, m_context.getDefaultAllocator(), m_context.getUniversalQueueFamilyIndex());

	const ImageViewCreateInfo colorTargetViewInfo(m_colorTargetImage->object(), vk::VK_IMAGE_VIEW_TYPE_2D, m_colorAttachmentFormat);
	m_colorTargetView						= vk::createImageView(m_vk, device, &colorTargetViewInfo);

	RenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.addAttachment(AttachmentDescription(m_colorAttachmentFormat,
															 vk::VK_SAMPLE_COUNT_1_BIT,
															 vk::VK_ATTACHMENT_LOAD_OP_LOAD,
															 vk::VK_ATTACHMENT_STORE_OP_STORE,
															 vk::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
															 vk::VK_ATTACHMENT_STORE_OP_STORE,
															 vk::VK_IMAGE_LAYOUT_GENERAL,
															 vk::VK_IMAGE_LAYOUT_GENERAL));


	const vk::VkAttachmentReference colorAttachmentReference =
	{
		0,
		vk::VK_IMAGE_LAYOUT_GENERAL
	};

	renderPassCreateInfo.addSubpass(SubpassDescription(vk::VK_PIPELINE_BIND_POINT_GRAPHICS,
													   0,
													   0,
													   DE_NULL,
													   1,
													   &colorAttachmentReference,
													   DE_NULL,
													   AttachmentReference(),
													   0,
													   DE_NULL));

	m_renderPass		= vk::createRenderPass(m_vk, device, &renderPassCreateInfo);

	std::vector<vk::VkImageView> colorAttachments(1);
	colorAttachments[0] = *m_colorTargetView;

	const FramebufferCreateInfo framebufferCreateInfo(*m_renderPass, colorAttachments, WIDTH, HEIGHT, 1);

	m_framebuffer		= vk::createFramebuffer(m_vk, device, &framebufferCreateInfo);

	const vk::VkVertexInputBindingDescription vertexInputBindingDescription =
	{
		0,
		sizeof(VertexElementData),
		vk::VK_VERTEX_INPUT_RATE_VERTEX,
	};

	const vk::VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
	{
		{
			0u,
			0u,
			vk::VK_FORMAT_R32G32B32A32_SFLOAT,
			0u
		},	// VertexElementData::position
		{
			1u,
			0u,
			vk::VK_FORMAT_R32G32B32A32_SFLOAT,
			static_cast<deUint32>(sizeof(tcu::Vec4))
		},  // VertexElementData::color
		{
			2u,
			0u,
			vk::VK_FORMAT_R32_SINT,
			static_cast<deUint32>(sizeof(tcu::Vec4)) * 2
		}   // VertexElementData::refVertexIndex
	};

	m_vertexInputState = PipelineCreateInfo::VertexInputState(1,
															  &vertexInputBindingDescription,
															  DE_LENGTH_OF_ARRAY(vertexInputAttributeDescriptions),
															  vertexInputAttributeDescriptions);

	const vk::VkDeviceSize dataSize = m_data.size() * sizeof(VertexElementData);
	m_vertexBuffer = Buffer::createAndAlloc(m_vk, device, BufferCreateInfo(dataSize,
		vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), m_context.getDefaultAllocator(), vk::MemoryRequirement::HostVisible);

	deUint8* ptr = reinterpret_cast<deUint8*>(m_vertexBuffer->getBoundMemory().getHostPtr());
	deMemcpy(ptr, &m_data[0], static_cast<size_t>(dataSize));

	vk::flushMappedMemoryRange(m_vk,
							   device,
							   m_vertexBuffer->getBoundMemory().getMemory(),
							   m_vertexBuffer->getBoundMemory().getOffset(),
							   dataSize);

	const CmdPoolCreateInfo cmdPoolCreateInfo(queueFamilyIndex);
	m_cmdPool	= vk::createCommandPool(m_vk, device, &cmdPoolCreateInfo);
	m_cmdBuffer	= vk::allocateCommandBuffer(m_vk, device, *m_cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	initPipeline(device);
}

void DrawTestsBaseClass::initPipeline (const vk::VkDevice device)
{
	const vk::Unique<vk::VkShaderModule> vs(createShaderModule(m_vk, device, m_context.getBinaryCollection().get(m_vertexShaderName), 0));
	const vk::Unique<vk::VkShaderModule> fs(createShaderModule(m_vk, device, m_context.getBinaryCollection().get(m_fragmentShaderName), 0));

	const PipelineCreateInfo::ColorBlendState::Attachment vkCbAttachmentState;

	vk::VkViewport viewport;
	viewport.x				= 0;
	viewport.y				= 0;
	viewport.width			= static_cast<float>(WIDTH);
	viewport.height			= static_cast<float>(HEIGHT);
	viewport.minDepth		= 0.0f;
	viewport.maxDepth		= 1.0f;

	vk::VkRect2D scissor;
	scissor.offset.x		= 0;
	scissor.offset.y		= 0;
	scissor.extent.width	= WIDTH;
	scissor.extent.height	= HEIGHT;

	PipelineCreateInfo pipelineCreateInfo(*m_pipelineLayout, *m_renderPass, 0, 0);
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*vs, "main", vk::VK_SHADER_STAGE_VERTEX_BIT));
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*fs, "main", vk::VK_SHADER_STAGE_FRAGMENT_BIT));
	pipelineCreateInfo.addState(PipelineCreateInfo::VertexInputState(m_vertexInputState));
	pipelineCreateInfo.addState(PipelineCreateInfo::InputAssemblerState(m_topology));
	pipelineCreateInfo.addState(PipelineCreateInfo::ColorBlendState(1, &vkCbAttachmentState));
	pipelineCreateInfo.addState(PipelineCreateInfo::ViewportState(1, std::vector<vk::VkViewport>(1, viewport), std::vector<vk::VkRect2D>(1, scissor)));
	pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState());
	pipelineCreateInfo.addState(PipelineCreateInfo::RasterizerState());
	pipelineCreateInfo.addState(PipelineCreateInfo::MultiSampleState());

	m_pipeline = vk::createGraphicsPipeline(m_vk, device, DE_NULL, &pipelineCreateInfo);
}

void DrawTestsBaseClass::beginRenderPass (void)
{
	const vk::VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	const CmdBufferBeginInfo beginInfo;

	m_vk.beginCommandBuffer(*m_cmdBuffer, &beginInfo);

	initialTransitionColor2DImage(m_vk, *m_cmdBuffer, m_colorTargetImage->object(), vk::VK_IMAGE_LAYOUT_GENERAL,
								  vk::VK_ACCESS_TRANSFER_WRITE_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT);

	const ImageSubresourceRange subresourceRange(vk::VK_IMAGE_ASPECT_COLOR_BIT);
	m_vk.cmdClearColorImage(*m_cmdBuffer, m_colorTargetImage->object(),
		vk::VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);

	const vk::VkMemoryBarrier memBarrier =
	{
		vk::VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		DE_NULL,
		vk::VK_ACCESS_TRANSFER_WRITE_BIT,
		vk::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};

	m_vk.cmdPipelineBarrier(*m_cmdBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT,
		vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, 1, &memBarrier, 0, DE_NULL, 0, DE_NULL);

	const vk::VkRect2D renderArea = { { 0, 0 }, { WIDTH, HEIGHT } };
	const RenderPassBeginInfo renderPassBegin(*m_renderPass, *m_framebuffer, renderArea);

	m_vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBegin, vk::VK_SUBPASS_CONTENTS_INLINE);
}

}	// Draw
}	// vkt
