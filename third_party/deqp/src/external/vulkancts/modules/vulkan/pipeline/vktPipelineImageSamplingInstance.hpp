#ifndef _VKTPIPELINEIMAGESAMPLINGINSTANCE_HPP
#define _VKTPIPELINEIMAGESAMPLINGINSTANCE_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Imagination Technologies Ltd.
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
 * \brief Image sampling case
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"

#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktPipelineImageUtil.hpp"
#include "vktPipelineReferenceRenderer.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "deSharedPtr.hpp"

namespace vkt
{
namespace pipeline
{

enum AllocationKind
{
	ALLOCATION_KIND_SUBALLOCATED,
	ALLOCATION_KIND_DEDICATED,
};

class ImageSamplingInstance : public vkt::TestInstance
{
public:
												ImageSamplingInstance	(Context&							context,
																		 const tcu::UVec2&					renderSize,
																		 vk::VkImageViewType				imageViewType,
																		 vk::VkFormat						imageFormat,
																		 const tcu::IVec3&					imageSize,
																		 int								layerCount,
																		 const vk::VkComponentMapping&		componentMapping,
																		 const vk::VkImageSubresourceRange&	subresourceRange,
																		 const vk::VkSamplerCreateInfo&		samplerParams,
																		 float								samplerLod,
																		 const std::vector<Vertex4Tex4>&	vertices,
																		 vk::VkDescriptorType				samplingType = vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
																		 int								imageCount = 1,
																		 AllocationKind						allocationKind = ALLOCATION_KIND_SUBALLOCATED);

	virtual										~ImageSamplingInstance	(void);

	virtual tcu::TestStatus						iterate					(void);

protected:
	tcu::TestStatus								verifyImage				(void);

private:
	typedef	vk::Unique<vk::VkImage>				UniqueImage;
	typedef	vk::Unique<vk::VkImageView>			UniqueImageView;
	typedef	de::UniquePtr<vk::Allocation>		UniqueAlloc;
	typedef	de::SharedPtr<UniqueImage>			SharedImagePtr;
	typedef	de::SharedPtr<UniqueImageView>		SharedImageViewPtr;
	typedef	de::SharedPtr<UniqueAlloc>			SharedAllocPtr;

	const AllocationKind						m_allocationKind;
	const vk::VkDescriptorType					m_samplingType;
	const vk::VkImageViewType					m_imageViewType;
	const vk::VkFormat							m_imageFormat;
	const tcu::IVec3							m_imageSize;
	const int									m_layerCount;
	const int									m_imageCount;

	const vk::VkComponentMapping				m_componentMapping;
	tcu::BVec4									m_componentMask;
	const vk::VkImageSubresourceRange			m_subresourceRange;
	const vk::VkSamplerCreateInfo				m_samplerParams;
	const float									m_samplerLod;

	std::vector<SharedImagePtr>					m_images;
	std::vector<SharedAllocPtr>					m_imageAllocs;
	std::vector<SharedImageViewPtr>				m_imageViews;
	vk::Move<vk::VkSampler>						m_sampler;
	de::MovePtr<TestTexture>					m_texture;

	const tcu::UVec2							m_renderSize;
	const vk::VkFormat							m_colorFormat;

	vk::Move<vk::VkDescriptorPool>				m_descriptorPool;
	vk::Move<vk::VkDescriptorSetLayout>			m_descriptorSetLayout;
	vk::Move<vk::VkDescriptorSet>				m_descriptorSet;

	std::vector<SharedImagePtr>					m_colorImages;
	std::vector<SharedAllocPtr>					m_colorImageAllocs;
	std::vector<SharedImageViewPtr>				m_colorAttachmentViews;
	vk::Move<vk::VkRenderPass>					m_renderPass;
	vk::Move<vk::VkFramebuffer>					m_framebuffer;

	vk::Move<vk::VkShaderModule>				m_vertexShaderModule;
	vk::Move<vk::VkShaderModule>				m_fragmentShaderModule;

	vk::Move<vk::VkBuffer>						m_vertexBuffer;
	std::vector<Vertex4Tex4>					m_vertices;
	de::MovePtr<vk::Allocation>					m_vertexBufferAlloc;

	vk::Move<vk::VkPipelineLayout>				m_pipelineLayout;
	vk::Move<vk::VkPipeline>					m_graphicsPipeline;

	vk::Move<vk::VkCommandPool>					m_cmdPool;
	vk::Move<vk::VkCommandBuffer>				m_cmdBuffer;

	vk::Move<vk::VkFence>						m_fence;
};

} // pipeline
} // vkt

#endif // _VKTPIPELINEIMAGESAMPLINGINSTANCE_HPP
