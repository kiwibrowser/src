#ifndef _VKTDRAWUTIL_HPP
#define _VKTDRAWUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Google Inc.
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
 * \brief Utility for generating simple work
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"

#include "deUniquePtr.hpp"
#include "vkBufferWithMemory.hpp"
#include "vkImageWithMemory.hpp"
#include "vkImageUtil.hpp"
#include "vkPrograms.hpp"
#include "vktTestCase.hpp"
#include "vkTypeUtil.hpp"
#include "rrRenderer.hpp"

namespace vkt
{
namespace drawutil
{

struct DrawState
{
	vk::VkPrimitiveTopology			topology;
	vk::VkFormat					colorFormat;
	vk::VkFormat					depthFormat;
	tcu::UVec2						renderSize;
	bool							depthClampEnable;
	bool							depthTestEnable;
	bool							depthWriteEnable;
	rr::TestFunc					compareOp;
	bool							depthBoundsTestEnable;
	bool							blendEnable;
	float							lineWidth;
	deUint32						numPatchControlPoints;
	deUint32						numSamples;
	bool							sampleShadingEnable;

	DrawState (const vk::VkPrimitiveTopology topology_, deUint32 renderWidth_, deUint32 renderHeight_);
};

struct DrawCallData
{
	const std::vector<tcu::Vec4>&	vertices;

	DrawCallData		(const std::vector<tcu::Vec4>&	vertices_)
		: vertices		(vertices_)
	{
	}
};

//! Sets up a graphics pipeline and enables simple draw calls to predefined attachments.
//! Clip volume uses wc = 1.0, which gives clip coord ranges: x = [-1, 1], y = [-1, 1], z = [0, 1]
//! Clip coords (-1,-1) map to viewport coords (0, 0).
class DrawContext
{
public:
											DrawContext				(const DrawState&		drawState,
																	 const DrawCallData&	drawCallData)
		: m_drawState						(drawState)
		, m_drawCallData					(drawCallData)
	{
	}
	virtual									~DrawContext			(void)
	{
	}

	virtual void							draw					(void) = 0;
	virtual tcu::ConstPixelBufferAccess		getColorPixels			(void) const = 0;
protected:
	const DrawState&						m_drawState;
	const DrawCallData&						m_drawCallData;
};

class ReferenceDrawContext : public DrawContext
{
public:
											ReferenceDrawContext	(const DrawState&			drawState,
																	 const DrawCallData&		drawCallData,
																	 const rr::VertexShader&	vertexShader,
																	 const rr::FragmentShader&	fragmentShader)
		: DrawContext						(drawState, drawCallData)
		, m_vertexShader					(vertexShader)
		, m_fragmentShader					(fragmentShader)
	{
	}
	virtual									~ReferenceDrawContext	(void);
	virtual void							draw					(void);
	virtual tcu::ConstPixelBufferAccess		getColorPixels			(void) const;
private:
	const rr::VertexShader&					m_vertexShader;
	const rr::FragmentShader&				m_fragmentShader;
	tcu::TextureLevel						m_refImage;
};

struct VulkanShader
{
	vk::VkShaderStageFlagBits	stage;
	const vk::ProgramBinary*	binary;

	VulkanShader (const vk::VkShaderStageFlagBits stage_, const vk::ProgramBinary& binary_)
		: stage		(stage_)
		, binary	(&binary_)
	{
	}
};

struct VulkanProgram
{
	std::vector<VulkanShader>	shaders;
	vk::VkImageView				depthImageView;		// \todo [2017-06-06 pyry] This shouldn't be here? Doesn't logically belong to program
	vk::VkDescriptorSetLayout	descriptorSetLayout;
	vk::VkDescriptorSet			descriptorSet;

	VulkanProgram (const std::vector<VulkanShader>& shaders_)
		: shaders				(shaders_)
		, depthImageView		(0)
		, descriptorSetLayout	(0)
		, descriptorSet			(0)
	{}

	VulkanProgram (void)
		: depthImageView		(0)
		, descriptorSetLayout	(0)
		, descriptorSet			(0)
	{}
};

class VulkanDrawContext : public DrawContext
{
public:
											VulkanDrawContext	(Context&				context,
																 const DrawState&		drawState,
																 const DrawCallData&	drawCallData,
																 const VulkanProgram&	vulkanProgram);
	virtual									~VulkanDrawContext	(void);
	virtual void							draw				(void);
	virtual tcu::ConstPixelBufferAccess		getColorPixels		(void) const;
private:
	enum VulkanContants
	{
		MAX_NUM_SHADER_MODULES					= 5,
	};
	Context&									m_context;
	const VulkanProgram&						m_program;
	de::MovePtr<vk::ImageWithMemory>			m_colorImage;
	de::MovePtr<vk::ImageWithMemory>			m_resolveImage;
	de::MovePtr<vk::BufferWithMemory>			m_colorAttachmentBuffer;
	vk::refdetails::Move<vk::VkImageView>		m_colorImageView;
	vk::refdetails::Move<vk::VkRenderPass>		m_renderPass;
	vk::refdetails::Move<vk::VkFramebuffer>		m_framebuffer;
	vk::refdetails::Move<vk::VkPipelineLayout>	m_pipelineLayout;
	vk::refdetails::Move<vk::VkPipeline>		m_pipeline;
	vk::refdetails::Move<vk::VkCommandPool>		m_cmdPool;
	vk::refdetails::Move<vk::VkCommandBuffer>	m_cmdBuffer;
	vk::refdetails::Move<vk::VkShaderModule>	m_shaderModules[MAX_NUM_SHADER_MODULES];
	de::MovePtr<vk::BufferWithMemory>			m_vertexBuffer;
};

std::string getPrimitiveTopologyShortName (const vk::VkPrimitiveTopology topology);

} // drwwutil
} // vkt

#endif // _VKTDRAWUTIL_HPP
