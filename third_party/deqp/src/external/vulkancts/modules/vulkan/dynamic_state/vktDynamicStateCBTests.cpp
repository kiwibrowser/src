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
 * \brief Dynamic CB State Tests
 *//*--------------------------------------------------------------------*/

#include "vktDynamicStateCBTests.hpp"

#include "vktDynamicStateBaseClass.hpp"
#include "vktDynamicStateTestCaseUtil.hpp"

#include "vkImageUtil.hpp"

#include "tcuImageCompare.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuRGBA.hpp"

namespace vkt
{
namespace DynamicState
{

using namespace Draw;

namespace
{

class BlendConstantsTestInstance : public DynamicStateBaseClass
{
public:
	BlendConstantsTestInstance (Context& context, ShaderMap shaders)
		: DynamicStateBaseClass	(context, shaders[glu::SHADERTYPE_VERTEX], shaders[glu::SHADERTYPE_FRAGMENT])
	{
		m_topology = vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

		m_data.push_back(PositionColorVertex(tcu::Vec4(-1.0f, 1.0f, 1.0f, 1.0f), tcu::RGBA::green().toVec()));
		m_data.push_back(PositionColorVertex(tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f), tcu::RGBA::green().toVec()));
		m_data.push_back(PositionColorVertex(tcu::Vec4(-1.0f, -1.0f, 1.0f, 1.0f), tcu::RGBA::green().toVec()));
		m_data.push_back(PositionColorVertex(tcu::Vec4(1.0f, -1.0f, 1.0f, 1.0f), tcu::RGBA::green().toVec()));

		DynamicStateBaseClass::initialize();
	}

	virtual void initPipeline (const vk::VkDevice device)
	{
		const vk::Unique<vk::VkShaderModule> vs (createShaderModule(m_vk, device, m_context.getBinaryCollection().get(m_vertexShaderName), 0));
		const vk::Unique<vk::VkShaderModule> fs (createShaderModule(m_vk, device, m_context.getBinaryCollection().get(m_fragmentShaderName), 0));

		const vk::VkPipelineColorBlendAttachmentState VkPipelineColorBlendAttachmentState =
			PipelineCreateInfo::ColorBlendState::Attachment(VK_TRUE,
															vk::VK_BLEND_FACTOR_SRC_ALPHA, vk::VK_BLEND_FACTOR_CONSTANT_COLOR, vk::VK_BLEND_OP_ADD,
															vk::VK_BLEND_FACTOR_SRC_ALPHA, vk::VK_BLEND_FACTOR_CONSTANT_ALPHA, vk::VK_BLEND_OP_ADD);

		PipelineCreateInfo pipelineCreateInfo(*m_pipelineLayout, *m_renderPass, 0, 0);
		pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*vs, "main", vk::VK_SHADER_STAGE_VERTEX_BIT));
		pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*fs, "main", vk::VK_SHADER_STAGE_FRAGMENT_BIT));
		pipelineCreateInfo.addState(PipelineCreateInfo::VertexInputState(m_vertexInputState));
		pipelineCreateInfo.addState(PipelineCreateInfo::InputAssemblerState(m_topology));
		pipelineCreateInfo.addState(PipelineCreateInfo::ColorBlendState(1, &VkPipelineColorBlendAttachmentState));
		pipelineCreateInfo.addState(PipelineCreateInfo::ViewportState(1));
		pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState());
		pipelineCreateInfo.addState(PipelineCreateInfo::RasterizerState());
		pipelineCreateInfo.addState(PipelineCreateInfo::MultiSampleState());
		pipelineCreateInfo.addState(PipelineCreateInfo::DynamicState());

		m_pipeline = vk::createGraphicsPipeline(m_vk, device, DE_NULL, &pipelineCreateInfo);
	}

	virtual tcu::TestStatus iterate (void)
	{
		tcu::TestLog &log = m_context.getTestContext().getLog();
		const vk::VkQueue queue = m_context.getUniversalQueue();

		const vk::VkClearColorValue clearColor = { { 1.0f, 1.0f, 1.0f, 1.0f } };
		beginRenderPassWithClearColor(clearColor);

		m_vk.cmdBindPipeline(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

		// bind states here
		setDynamicViewportState(WIDTH, HEIGHT);
		setDynamicRasterizationState();
		setDynamicDepthStencilState();
		setDynamicBlendState(0.33f, 0.1f, 0.66f, 0.5f);

		const vk::VkDeviceSize vertexBufferOffset = 0;
		const vk::VkBuffer vertexBuffer = m_vertexBuffer->object();
		m_vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);

		m_vk.cmdDraw(*m_cmdBuffer, static_cast<deUint32>(m_data.size()), 1, 0, 0);

		m_vk.cmdEndRenderPass(*m_cmdBuffer);
		m_vk.endCommandBuffer(*m_cmdBuffer);

		vk::VkSubmitInfo submitInfo =
		{
			vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
			DE_NULL,							// const void*				pNext;
			0,									// deUint32					waitSemaphoreCount;
			DE_NULL,							// const VkSemaphore*		pWaitSemaphores;
			(const vk::VkPipelineStageFlags*)DE_NULL,
			1,									// deUint32					commandBufferCount;
			&m_cmdBuffer.get(),					// const VkCommandBuffer*	pCommandBuffers;
			0,									// deUint32					signalSemaphoreCount;
			DE_NULL								// const VkSemaphore*		pSignalSemaphores;
		};
		m_vk.queueSubmit(queue, 1, &submitInfo, DE_NULL);
		VK_CHECK(m_vk.queueWaitIdle(queue));

		//validation
		{
			tcu::Texture2D referenceFrame(vk::mapVkFormat(m_colorAttachmentFormat), (int)(0.5 + WIDTH), (int)(0.5 + HEIGHT));
			referenceFrame.allocLevel(0);

			const deInt32 frameWidth = referenceFrame.getWidth();
			const deInt32 frameHeight = referenceFrame.getHeight();

			tcu::clear(referenceFrame.getLevel(0), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

			for (int y = 0; y < frameHeight; y++)
			{
				const float yCoord = (float)(y / (0.5*frameHeight)) - 1.0f;

				for (int x = 0; x < frameWidth; x++)
				{
					const float xCoord = (float)(x / (0.5*frameWidth)) - 1.0f;

					if ((yCoord >= -1.0f && yCoord <= 1.0f && xCoord >= -1.0f && xCoord <= 1.0f))
						referenceFrame.getLevel(0).setPixel(tcu::Vec4(0.33f, 1.0f, 0.66f, 1.0f), x, y);
				}
			}

			const vk::VkOffset3D zeroOffset = { 0, 0, 0 };
			const tcu::ConstPixelBufferAccess renderedFrame = m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(),
																							  vk::VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, vk::VK_IMAGE_ASPECT_COLOR_BIT);

			if (!tcu::fuzzyCompare(log, "Result", "Image comparison result",
				referenceFrame.getLevel(0), renderedFrame, 0.05f,
				tcu::COMPARE_LOG_RESULT))
			{
				return tcu::TestStatus(QP_TEST_RESULT_FAIL, "Image verification failed");
			}

			return tcu::TestStatus(QP_TEST_RESULT_PASS, "Image verification passed");
		}
	}
};

} //anonymous

DynamicStateCBTests::DynamicStateCBTests (tcu::TestContext& testCtx)
	: TestCaseGroup (testCtx, "cb_state", "Tests for color blend state")
{
	/* Left blank on purpose */
}

DynamicStateCBTests::~DynamicStateCBTests (void) {}

void DynamicStateCBTests::init (void)
{
	ShaderMap shaderPaths;
	shaderPaths[glu::SHADERTYPE_VERTEX] = "vulkan/dynamic_state/VertexFetch.vert";
	shaderPaths[glu::SHADERTYPE_FRAGMENT] = "vulkan/dynamic_state/VertexFetch.frag";
	addChild(new InstanceFactory<BlendConstantsTestInstance>(m_testCtx, "blend_constants", "Check if blend constants are working properly", shaderPaths));
}

} // DynamicState
} // vkt
