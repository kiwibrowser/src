#ifndef _VKTDYNAMICSTATEBASECLASS_HPP
#define _VKTDYNAMICSTATEBASECLASS_HPP
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
 * \brief Dynamic State Tests - Base Class
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vktTestCase.hpp"

#include "vktDynamicStateTestCaseUtil.hpp"
#include "vktDrawImageObjectUtil.hpp"
#include "vktDrawBufferObjectUtil.hpp"
#include "vktDrawCreateInfoUtil.hpp"

namespace vkt
{
namespace DynamicState
{

class DynamicStateBaseClass : public TestInstance
{
public:
	DynamicStateBaseClass (Context& context, const char* vertexShaderName, const char* fragmentShaderName);

protected:
	void					initialize						(void);

	virtual void			initRenderPass					(const vk::VkDevice				device);
	virtual void			initFramebuffer					(const vk::VkDevice				device);
	virtual void			initPipeline					(const vk::VkDevice				device);

	virtual tcu::TestStatus iterate							(void);

	void					beginRenderPass					(void);

	void					beginRenderPassWithClearColor	(const vk::VkClearColorValue&	clearColor,
															 const bool						skipBeginCmdBuffer	= false);

	void					setDynamicViewportState			(const deUint32					width,
															const deUint32					height);

	void					setDynamicViewportState			(deUint32						viewportCount,
															 const vk::VkViewport*			pViewports,
															 const vk::VkRect2D*			pScissors);

	void					setDynamicRasterizationState	(const float					lineWidth = 1.0f,
															 const float					depthBiasConstantFactor = 0.0f,
															 const float					depthBiasClamp = 0.0f,
															 const float					depthBiasSlopeFactor = 0.0f);

	void					setDynamicBlendState			(const float					const1 = 0.0f, const float const2 = 0.0f,
															 const float					const3 = 0.0f, const float const4 = 0.0f);

	void					setDynamicDepthStencilState		(const float					minDepthBounds = -1.0f,
															 const float					maxDepthBounds = 1.0f,
															 const deUint32					stencilFrontCompareMask = 0xffffffffu,
															 const deUint32					stencilFrontWriteMask = 0xffffffffu,
															 const deUint32					stencilFrontReference = 0,
															 const deUint32					stencilBackCompareMask = 0xffffffffu,
															 const deUint32					stencilBackWriteMask = 0xffffffffu,
															 const deUint32					stencilBackReference = 0);
	enum
	{
		WIDTH       = 128,
		HEIGHT      = 128
	};

	vk::VkFormat									m_colorAttachmentFormat;

	vk::VkPrimitiveTopology							m_topology;

	const vk::DeviceInterface&						m_vk;

	vk::Move<vk::VkPipeline>						m_pipeline;
	vk::Move<vk::VkPipelineLayout>					m_pipelineLayout;

	de::SharedPtr<Draw::Image>						m_colorTargetImage;
	vk::Move<vk::VkImageView>						m_colorTargetView;

	Draw::PipelineCreateInfo::VertexInputState		m_vertexInputState;
	de::SharedPtr<Draw::Buffer>						m_vertexBuffer;

	vk::Move<vk::VkCommandPool>						m_cmdPool;
	vk::Move<vk::VkCommandBuffer>					m_cmdBuffer;

	vk::Move<vk::VkFramebuffer>						m_framebuffer;
	vk::Move<vk::VkRenderPass>						m_renderPass;

	const std::string								m_vertexShaderName;
	const std::string								m_fragmentShaderName;
	std::vector<PositionColorVertex>				m_data;
};

} // DynamicState
} // vkt

#endif // _VKTDYNAMICSTATEBASECLASS_HPP
