#ifndef _VKTDRAWCREATEINFOUTIL_HPP
#define _VKTDRAWCREATEINFOUTIL_HPP
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
 * \brief CreateInfo utilities
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "tcuVector.hpp"
#include "deSharedPtr.hpp"
#include <vector>

namespace vkt
{
namespace Draw
{

class ImageSubresourceRange : public vk::VkImageSubresourceRange
{
public:
	ImageSubresourceRange		(vk::VkImageAspectFlags	aspectMask,
								 deUint32				baseMipLevel	= 0,
								 deUint32				levelCount		= 1,
								 deUint32				baseArrayLayer	= 0,
								 deUint32				layerCount		= 1);
};

class ComponentMapping : public vk::VkComponentMapping
{
public:
	ComponentMapping			(vk::VkComponentSwizzle r = vk::VK_COMPONENT_SWIZZLE_R,
								 vk::VkComponentSwizzle g = vk::VK_COMPONENT_SWIZZLE_G,
								 vk::VkComponentSwizzle b = vk::VK_COMPONENT_SWIZZLE_B,
								 vk::VkComponentSwizzle a = vk::VK_COMPONENT_SWIZZLE_A);
};

class ImageViewCreateInfo : public vk::VkImageViewCreateInfo
{
public:
	ImageViewCreateInfo			(vk::VkImage						image,
								 vk::VkImageViewType				viewType,
								 vk::VkFormat						format,
								 const vk::VkImageSubresourceRange&	subresourceRange,
								 const vk::VkComponentMapping&		components			= ComponentMapping(),
								 vk::VkImageViewCreateFlags			flags				= 0);

	ImageViewCreateInfo			(vk::VkImage						image,
								 vk::VkImageViewType				viewType,
								 vk::VkFormat						format,
								 const vk::VkComponentMapping&		components			= ComponentMapping(),
								 vk::VkImageViewCreateFlags			flags				= 0);
};

class BufferViewCreateInfo : public vk::VkBufferViewCreateInfo
{
public:
	BufferViewCreateInfo		 (vk::VkBuffer		buffer,
								  vk::VkFormat		format,
								  vk::VkDeviceSize	offset,
								  vk::VkDeviceSize	range);
};

class BufferCreateInfo : public vk::VkBufferCreateInfo
{
public:
	BufferCreateInfo			(vk::VkDeviceSize			size,
								 vk::VkBufferCreateFlags	usage,
								 vk::VkSharingMode			sharingMode				= vk::VK_SHARING_MODE_EXCLUSIVE,
								 deUint32					queueFamilyIndexCount	= 0,
								 const deUint32*			pQueueFamilyIndices		= DE_NULL,
								 vk::VkBufferCreateFlags	flags					= 0);

	BufferCreateInfo			(const BufferCreateInfo&	other);
	BufferCreateInfo& operator=	(const BufferCreateInfo&	other);

private:
	std::vector<deUint32> m_queueFamilyIndices;
};

class ImageCreateInfo : public vk::VkImageCreateInfo
{
public:
	ImageCreateInfo				(vk::VkImageType			imageType,
								 vk::VkFormat				format,
								 vk::VkExtent3D				extent,
								 deUint32					mipLevels,
								 deUint32					arrayLayers,
								 vk::VkSampleCountFlagBits	samples,
								 vk::VkImageTiling			tiling,
								 vk::VkImageUsageFlags		usage,
								 vk::VkSharingMode			sharingMode				= vk::VK_SHARING_MODE_EXCLUSIVE,
								 deUint32					queueFamilyIndexCount	= 0,
								 const deUint32*			pQueueFamilyIndices		= DE_NULL,
								 vk::VkImageCreateFlags		flags					= 0,
								 vk::VkImageLayout			initialLayout			= vk::VK_IMAGE_LAYOUT_UNDEFINED);

private:
	ImageCreateInfo				(const ImageCreateInfo&		other);
	ImageCreateInfo& operator=	(const ImageCreateInfo&		other);

	std::vector<deUint32> m_queueFamilyIndices;
};

class FramebufferCreateInfo : public vk::VkFramebufferCreateInfo
{
public:
	FramebufferCreateInfo		(vk::VkRenderPass						renderPass,
								 const std::vector<vk::VkImageView>&	attachments,
								 deUint32								width,
								 deUint32								height,
								 deUint32								layers);
};

class AttachmentDescription : public vk::VkAttachmentDescription
{
public:
	AttachmentDescription	(vk::VkFormat				format,
							 vk::VkSampleCountFlagBits	samples,
							 vk::VkAttachmentLoadOp		loadOp,
							 vk::VkAttachmentStoreOp	storeOp,
							 vk::VkAttachmentLoadOp		stencilLoadOp,
							 vk::VkAttachmentStoreOp	stencilStoreOp,
							 vk::VkImageLayout			initialLayout,
							 vk::VkImageLayout			finalLayout);

	AttachmentDescription	(const vk::VkAttachmentDescription &);
};

class AttachmentReference : public vk::VkAttachmentReference
{
public:
	AttachmentReference		(deUint32 attachment, vk::VkImageLayout layout);
	AttachmentReference		(void);
};

class SubpassDescription : public vk::VkSubpassDescription
{
public:
	SubpassDescription				(vk::VkPipelineBindPoint			pipelineBindPoint,
									 vk::VkSubpassDescriptionFlags		flags,
									 deUint32							inputAttachmentCount,
									 const vk::VkAttachmentReference*	inputAttachments,
									 deUint32							colorAttachmentCount,
									 const vk::VkAttachmentReference*	colorAttachments,
									 const vk::VkAttachmentReference*	resolveAttachments,
									 vk::VkAttachmentReference			depthStencilAttachment,
									 deUint32							preserveAttachmentCount,
									 const deUint32*					preserveAttachments);

	SubpassDescription				(const vk::VkSubpassDescription&	other);
	SubpassDescription				(const SubpassDescription&			other);
	SubpassDescription& operator=	(const SubpassDescription&			other);

private:
	std::vector<vk::VkAttachmentReference>	m_inputAttachments;
	std::vector<vk::VkAttachmentReference>	m_colorAttachments;
	std::vector<vk::VkAttachmentReference>	m_resolveAttachments;
	std::vector<deUint32>					m_preserveAttachments;

	vk::VkAttachmentReference				m_depthStencilAttachment;
};

class SubpassDependency : public vk::VkSubpassDependency
{
public:
	SubpassDependency (	deUint32					srcSubpass,
						deUint32					dstSubpass,
						vk::VkPipelineStageFlags	srcStageMask,
						vk::VkPipelineStageFlags	dstStageMask,
						vk::VkAccessFlags			srcAccessMask,
						vk::VkAccessFlags			dstAccessMask,
						vk::VkDependencyFlags		dependencyFlags);

	SubpassDependency (const vk::VkSubpassDependency& other);
};

class RenderPassCreateInfo : public vk::VkRenderPassCreateInfo
{
public:
	RenderPassCreateInfo (const std::vector<vk::VkAttachmentDescription>&	attachments,
						  const std::vector<vk::VkSubpassDescription>&		subpasses,
						  const std::vector<vk::VkSubpassDependency>&		dependiences		= std::vector<vk::VkSubpassDependency>());

	RenderPassCreateInfo (deUint32											attachmentCount	= 0,
						  const vk::VkAttachmentDescription*				pAttachments	= DE_NULL,
						  deUint32											subpassCount	= 0,
						  const vk::VkSubpassDescription*					pSubpasses		= DE_NULL,
						  deUint32											dependencyCount	= 0,
						  const vk::VkSubpassDependency*					pDependiences	= DE_NULL);

	void addAttachment	(vk::VkAttachmentDescription						attachment);
	void addSubpass		(vk::VkSubpassDescription							subpass);
	void addDependency	(vk::VkSubpassDependency							dependency);

private:
	std::vector<AttachmentDescription>			m_attachments;
	std::vector<SubpassDescription>				m_subpasses;
	std::vector<SubpassDependency>				m_dependiences;

	std::vector<vk::VkAttachmentDescription>	m_attachmentsStructs;
	std::vector<vk::VkSubpassDescription>		m_subpassesStructs;
	std::vector<vk::VkSubpassDependency>		m_dependiencesStructs;

	RenderPassCreateInfo			(const RenderPassCreateInfo &other); //Not allowed!
	RenderPassCreateInfo& operator= (const RenderPassCreateInfo &other); //Not allowed!
};

class RenderPassBeginInfo : public vk::VkRenderPassBeginInfo
{
public:
	RenderPassBeginInfo (vk::VkRenderPass						renderPass,
						 vk::VkFramebuffer						framebuffer,
						 vk::VkRect2D							renderArea,
						 const std::vector<vk::VkClearValue>&	clearValues = std::vector<vk::VkClearValue>());

private:
	std::vector<vk::VkClearValue> m_clearValues;

	RenderPassBeginInfo				(const RenderPassBeginInfo&	other); //Not allowed!
	RenderPassBeginInfo& operator=	(const RenderPassBeginInfo&	other); //Not allowed!
};

class CmdPoolCreateInfo : public vk::VkCommandPoolCreateInfo
{
public:
	CmdPoolCreateInfo (deUint32						queueFamilyIndex,
					   vk::VkCommandPoolCreateFlags flags				= vk::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
};

class CmdBufferBeginInfo : public vk::VkCommandBufferBeginInfo
{
public:
	CmdBufferBeginInfo (vk::VkCommandBufferUsageFlags		flags					= 0);
};

class DescriptorPoolSize : public vk::VkDescriptorPoolSize
{
public:
	DescriptorPoolSize (vk::VkDescriptorType _type, deUint32 _descriptorCount)
	{
		type			= _type;
		descriptorCount = _descriptorCount;
	}
};

class DescriptorPoolCreateInfo : public vk::VkDescriptorPoolCreateInfo
{
public:
	DescriptorPoolCreateInfo (const std::vector<vk::VkDescriptorPoolSize>&	poolSizeCounts,
							  vk::VkDescriptorPoolCreateFlags				flags,
							  deUint32										maxSets);

	DescriptorPoolCreateInfo& addDescriptors (vk::VkDescriptorType type, deUint32 count);

private:
	std::vector<vk::VkDescriptorPoolSize> m_poolSizeCounts;
};

class DescriptorSetLayoutCreateInfo : public vk::VkDescriptorSetLayoutCreateInfo
{
public:
	DescriptorSetLayoutCreateInfo (deUint32 bindingCount, const vk::VkDescriptorSetLayoutBinding* pBindings);
};

class PipelineLayoutCreateInfo : public vk::VkPipelineLayoutCreateInfo
{
public:
	PipelineLayoutCreateInfo (deUint32										descriptorSetCount,
							  const vk::VkDescriptorSetLayout*				pSetLayouts,
							  deUint32										pushConstantRangeCount	= 0,
							  const vk::VkPushConstantRange*				pPushConstantRanges		= DE_NULL);

	PipelineLayoutCreateInfo (const std::vector<vk::VkDescriptorSetLayout>&	setLayouts				= std::vector<vk::VkDescriptorSetLayout>(),
							  deUint32										pushConstantRangeCount	= 0,
							  const vk::VkPushConstantRange*				pPushConstantRanges		= DE_NULL);

private:
	std::vector<vk::VkDescriptorSetLayout>	m_setLayouts;
	std::vector<vk::VkPushConstantRange>	m_pushConstantRanges;
};

class PipelineCreateInfo : public vk::VkGraphicsPipelineCreateInfo
{
public:
	class VertexInputState : public vk::VkPipelineVertexInputStateCreateInfo
	{
	public:
		VertexInputState (deUint32										vertexBindingDescriptionCount	= 0,
						  const vk::VkVertexInputBindingDescription*	pVertexBindingDescriptions		= NULL,
						  deUint32										vertexAttributeDescriptionCount	= 0,
						  const vk::VkVertexInputAttributeDescription*	pVertexAttributeDescriptions	= NULL);
	};

	class InputAssemblerState : public vk::VkPipelineInputAssemblyStateCreateInfo
	{
	public:
		InputAssemblerState (vk::VkPrimitiveTopology topology, vk::VkBool32 primitiveRestartEnable = false);
	};

	class TessellationState : public vk::VkPipelineTessellationStateCreateInfo
	{
	public:
		TessellationState (deUint32 patchControlPoints = 0);
	};

	class ViewportState : public vk::VkPipelineViewportStateCreateInfo
	{
	public:
		ViewportState				(deUint32						viewportCount,
									 std::vector<vk::VkViewport>	viewports		= std::vector<vk::VkViewport>(0),
									 std::vector<vk::VkRect2D>		scissors		= std::vector<vk::VkRect2D>(0));

		ViewportState				(const ViewportState&			other);
		ViewportState& operator=	(const ViewportState&			other);

		std::vector<vk::VkViewport> m_viewports;
		std::vector<vk::VkRect2D>	m_scissors;
	};

	class RasterizerState : public vk::VkPipelineRasterizationStateCreateInfo
	{
	public:
		RasterizerState (vk::VkBool32			depthClampEnable		= false,
						 vk::VkBool32			rasterizerDiscardEnable = false,
						 vk::VkPolygonMode		polygonMode				= vk::VK_POLYGON_MODE_FILL,
						 vk::VkCullModeFlags	cullMode				= vk::VK_CULL_MODE_NONE,
						 vk::VkFrontFace		frontFace				= vk::VK_FRONT_FACE_CLOCKWISE,
						 vk::VkBool32			depthBiasEnable			= true,
						 float					depthBiasConstantFactor	= 0.0f,
						 float					depthBiasClamp			= 0.0f,
						 float					depthBiasSlopeFactor	= 0.0f,
						 float					lineWidth				= 1.0f);
	};

	class MultiSampleState : public vk::VkPipelineMultisampleStateCreateInfo
	{
	public:
		MultiSampleState			(vk::VkSampleCountFlagBits				rasterizationSamples		= vk::VK_SAMPLE_COUNT_1_BIT,
									 vk::VkBool32							sampleShadingEnable			= false,
									 float									minSampleShading			= 0.0f,
									 const std::vector<vk::VkSampleMask>&	sampleMask					= std::vector<vk::VkSampleMask>(1, 0xffffffffu),
									 bool									alphaToCoverageEnable		= false,
									 bool									alphaToOneEnable			= false);

		MultiSampleState			(const MultiSampleState&				other);
		MultiSampleState& operator= (const MultiSampleState&				other);

	private:
		std::vector<vk::VkSampleMask> m_sampleMask;
	};

	class ColorBlendState : public vk::VkPipelineColorBlendStateCreateInfo
	{
	public:
		class Attachment : public vk::VkPipelineColorBlendAttachmentState
		{
		public:
			Attachment (vk::VkBool32				blendEnable			= false,
						vk::VkBlendFactor			srcColorBlendFactor	= vk::VK_BLEND_FACTOR_SRC_COLOR,
						vk::VkBlendFactor			dstColorBlendFactor	= vk::VK_BLEND_FACTOR_DST_COLOR,
						vk::VkBlendOp				colorBlendOp		= vk::VK_BLEND_OP_ADD,
						vk::VkBlendFactor			srcAlphaBlendFactor	= vk::VK_BLEND_FACTOR_SRC_COLOR,
						vk::VkBlendFactor			dstAlphaBlendFactor	= vk::VK_BLEND_FACTOR_DST_COLOR,
						vk::VkBlendOp				alphaBlendOp		= vk::VK_BLEND_OP_ADD,
						vk::VkColorComponentFlags	colorWriteMask		= vk::VK_COLOR_COMPONENT_R_BIT|
																		  vk::VK_COLOR_COMPONENT_G_BIT|
																		  vk::VK_COLOR_COMPONENT_B_BIT|
																		  vk::VK_COLOR_COMPONENT_A_BIT);
		};

		ColorBlendState (const std::vector<vk::VkPipelineColorBlendAttachmentState>&	attachments,
						 vk::VkBool32													alphaToCoverageEnable	= false,
						 vk::VkLogicOp													logicOp					= vk::VK_LOGIC_OP_COPY);

		ColorBlendState (deUint32														attachmentCount,
						 const vk::VkPipelineColorBlendAttachmentState*					attachments,
						 vk::VkBool32													logicOpEnable			= false,
						 vk::VkLogicOp													logicOp					= vk::VK_LOGIC_OP_COPY);

		ColorBlendState (const vk::VkPipelineColorBlendStateCreateInfo&					createInfo);
		ColorBlendState (const ColorBlendState&											createInfo,
						 std::vector<float>												blendConstants			= std::vector<float>(4));

	private:
		std::vector<vk::VkPipelineColorBlendAttachmentState> m_attachments;
	};

	class DepthStencilState : public vk::VkPipelineDepthStencilStateCreateInfo
	{
	public:
		class StencilOpState : public vk::VkStencilOpState
		{
		public:
			StencilOpState (vk::VkStencilOp failOp					= vk::VK_STENCIL_OP_REPLACE,
							vk::VkStencilOp passOp					= vk::VK_STENCIL_OP_REPLACE,
							vk::VkStencilOp depthFailOp				= vk::VK_STENCIL_OP_REPLACE,
							vk::VkCompareOp compareOp				= vk::VK_COMPARE_OP_ALWAYS,
							deUint32		compareMask				= 0xffffffffu,
							deUint32		writeMask				= 0xffffffffu,
							deUint32		reference				= 0u);
		};

		DepthStencilState (vk::VkBool32		depthTestEnable			= false,
						   vk::VkBool32		depthWriteEnable		= false,
						   vk::VkCompareOp	depthCompareOp			= vk::VK_COMPARE_OP_ALWAYS,
						   vk::VkBool32		depthBoundsTestEnable	= false,
						   vk::VkBool32		stencilTestEnable		= false,
						   StencilOpState	front					= StencilOpState(),
						   StencilOpState	back					= StencilOpState(),
						   float			minDepthBounds			= 0.0f,
						   float			maxDepthBounds			= 1.0f);
	};

	class PipelineShaderStage : public vk::VkPipelineShaderStageCreateInfo
	{
	public:
		PipelineShaderStage (vk::VkShaderModule shaderModule, const char* pName, vk::VkShaderStageFlagBits stage);
	};

	class DynamicState : public vk::VkPipelineDynamicStateCreateInfo
	{
	public:
		DynamicState			(const std::vector<vk::VkDynamicState>& dynamicStates = std::vector<vk::VkDynamicState>(0));

		DynamicState			(const DynamicState& other);
		DynamicState& operator= (const DynamicState& other);

		std::vector<vk::VkDynamicState> m_dynamicStates;
	};

	PipelineCreateInfo				(vk::VkPipelineLayout								layout,
								     vk::VkRenderPass									renderPass,
									 int												subpass,
									 vk::VkPipelineCreateFlags							flags);

	PipelineCreateInfo& addShader	(const vk::VkPipelineShaderStageCreateInfo&			shader);

	PipelineCreateInfo& addState	(const vk::VkPipelineVertexInputStateCreateInfo&	state);
	PipelineCreateInfo& addState	(const vk::VkPipelineInputAssemblyStateCreateInfo&	state);
	PipelineCreateInfo& addState	(const vk::VkPipelineColorBlendStateCreateInfo&		state);
	PipelineCreateInfo& addState	(const vk::VkPipelineViewportStateCreateInfo&		state);
	PipelineCreateInfo& addState	(const vk::VkPipelineDepthStencilStateCreateInfo&	state);
	PipelineCreateInfo& addState	(const vk::VkPipelineTessellationStateCreateInfo&	state);
	PipelineCreateInfo& addState	(const vk::VkPipelineRasterizationStateCreateInfo&	state);
	PipelineCreateInfo& addState	(const vk::VkPipelineMultisampleStateCreateInfo&	state);
	PipelineCreateInfo& addState	(const vk::VkPipelineDynamicStateCreateInfo&		state);

private:
	std::vector<vk::VkPipelineShaderStageCreateInfo>		m_shaders;

	vk::VkPipelineVertexInputStateCreateInfo				m_vertexInputState;
	vk::VkPipelineInputAssemblyStateCreateInfo				m_inputAssemblyState;
	std::vector<vk::VkPipelineColorBlendAttachmentState>	m_colorBlendStateAttachments;
	vk::VkPipelineColorBlendStateCreateInfo					m_colorBlendState;
	vk::VkPipelineViewportStateCreateInfo					m_viewportState;
	vk::VkPipelineDepthStencilStateCreateInfo				m_dynamicDepthStencilState;
	vk::VkPipelineTessellationStateCreateInfo				m_tessState;
	vk::VkPipelineRasterizationStateCreateInfo				m_rasterState;
	vk::VkPipelineMultisampleStateCreateInfo				m_multisampleState;
	vk::VkPipelineDynamicStateCreateInfo					m_dynamicState;

	std::vector<vk::VkDynamicState>							m_dynamicStates;
	std::vector<vk::VkViewport>								m_viewports;
	std::vector<vk::VkRect2D>								m_scissors;
	std::vector<vk::VkSampleMask>							m_multisampleStateSampleMask;
};

class SamplerCreateInfo : public vk::VkSamplerCreateInfo
{
public:
	SamplerCreateInfo (vk::VkFilter				magFilter				= vk::VK_FILTER_NEAREST,
					   vk::VkFilter				minFilter				= vk::VK_FILTER_NEAREST,
					   vk::VkSamplerMipmapMode	mipmapMode				= vk::VK_SAMPLER_MIPMAP_MODE_NEAREST,
					   vk::VkSamplerAddressMode	addressU				= vk::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
					   vk::VkSamplerAddressMode	addressV				= vk::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
					   vk::VkSamplerAddressMode	addressW				= vk::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
					   float					mipLodBias				= 0.0f,
					   vk::VkBool32				anisotropyEnable		= VK_FALSE,
					   float					maxAnisotropy			= 1.0f,
					   vk::VkBool32				compareEnable			= false,
					   vk::VkCompareOp			compareOp				= vk::VK_COMPARE_OP_ALWAYS,
					   float					minLod					= 0.0f,
					   float					maxLod					= 16.0f,
					   vk::VkBorderColor		borderColor				= vk::VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
					   vk::VkBool32				unnormalizedCoordinates	= false);
};

} // Draw
} // vkt

#endif // _VKTDRAWCREATEINFOUTIL_HPP
