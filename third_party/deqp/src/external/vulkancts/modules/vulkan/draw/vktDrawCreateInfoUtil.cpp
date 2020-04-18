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

#include "vktDrawCreateInfoUtil.hpp"

#include "vkImageUtil.hpp"

namespace vkt
{
namespace Draw
{

ImageSubresourceRange::ImageSubresourceRange (vk::VkImageAspectFlags	_aspectMask,
											  deUint32					_baseMipLevel,
											  deUint32					_levelCount,
											  deUint32					_baseArrayLayer,
											  deUint32					_layerCount)
{
	aspectMask		= _aspectMask;
	baseMipLevel	= _baseMipLevel;
	levelCount		= _levelCount;
	baseArrayLayer	= _baseArrayLayer;
	layerCount		= _layerCount;
}

ComponentMapping::ComponentMapping (vk::VkComponentSwizzle _r,
									vk::VkComponentSwizzle _g,
									vk::VkComponentSwizzle _b,
									vk::VkComponentSwizzle _a)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;
}

ImageViewCreateInfo::ImageViewCreateInfo (vk::VkImage							_image,
										  vk::VkImageViewType					_viewType,
										  vk::VkFormat							_format,
										  const vk::VkImageSubresourceRange&	_subresourceRange,
										  const vk::VkComponentMapping&			_components,
										  vk::VkImageViewCreateFlags			_flags)
{
	sType = vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	pNext = DE_NULL;
	flags				= 0u;
	image				= _image;
	viewType			= _viewType;
	format				= _format;
	components.r		= _components.r;
	components.g		= _components.g;
	components.b		= _components.b;
	components.a		= _components.a;
	subresourceRange	= _subresourceRange;
	flags				= _flags;
}

ImageViewCreateInfo::ImageViewCreateInfo (vk::VkImage					_image,
										  vk::VkImageViewType			_viewType,
										  vk::VkFormat					_format,
										  const vk::VkComponentMapping&	_components,
										  vk::VkImageViewCreateFlags	_flags)
{
	sType = vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	pNext = DE_NULL;
	flags			= 0u;
	image			= _image;
	viewType		= _viewType;
	format			= _format;
	components.r	= _components.r;
	components.g	= _components.g;
	components.b	= _components.b;
	components.a	= _components.a;

	vk::VkImageAspectFlags aspectFlags;
	const tcu::TextureFormat tcuFormat = vk::mapVkFormat(_format);

	switch (tcuFormat.order)
	{
		case tcu::TextureFormat::D:
			aspectFlags = vk::VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		case tcu::TextureFormat::S:
			aspectFlags = vk::VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		case tcu::TextureFormat::DS:
			aspectFlags = vk::VK_IMAGE_ASPECT_STENCIL_BIT | vk::VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		default:
			aspectFlags = vk::VK_IMAGE_ASPECT_COLOR_BIT;
			break;
	}

	subresourceRange = ImageSubresourceRange(aspectFlags);;
	flags = _flags;
}

BufferViewCreateInfo::BufferViewCreateInfo (vk::VkBuffer	_buffer,
											vk::VkFormat		_format,
											vk::VkDeviceSize _offset,
											vk::VkDeviceSize _range)
{
	sType = vk::VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	pNext = DE_NULL;

	flags	= 0;
	buffer	= _buffer;
	format	= _format;
	offset	= _offset;
	range	= _range;
}

BufferCreateInfo::BufferCreateInfo (vk::VkDeviceSize		_size,
									vk::VkBufferUsageFlags	_usage,
									vk::VkSharingMode		_sharingMode,
									deUint32				_queueFamilyIndexCount,
									const deUint32*			_pQueueFamilyIndices,
									vk::VkBufferCreateFlags _flags)
{
	sType = vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	pNext = DE_NULL;
	size					= _size;
	usage					= _usage;
	flags					= _flags;
	sharingMode				= _sharingMode;
	queueFamilyIndexCount	= _queueFamilyIndexCount;

	if (_queueFamilyIndexCount)
	{
		m_queueFamilyIndices = std::vector<deUint32>(
			_pQueueFamilyIndices, _pQueueFamilyIndices + _queueFamilyIndexCount);
		pQueueFamilyIndices = &m_queueFamilyIndices[0];
	}
	else
	{
		pQueueFamilyIndices = _pQueueFamilyIndices;
	}
}

BufferCreateInfo::BufferCreateInfo (const BufferCreateInfo &other)
{
	sType					= other.sType;
	pNext					= other.pNext;
	size					= other.size;
	usage					= other.usage;
	flags					= other.flags;
	sharingMode				= other.sharingMode;
	queueFamilyIndexCount	= other.queueFamilyIndexCount;

	m_queueFamilyIndices	= other.m_queueFamilyIndices;
	DE_ASSERT(m_queueFamilyIndices.size() == queueFamilyIndexCount);

	if (m_queueFamilyIndices.size())
	{
		pQueueFamilyIndices = &m_queueFamilyIndices[0];
	}
	else
	{
		pQueueFamilyIndices = DE_NULL;
	}
}

BufferCreateInfo & BufferCreateInfo::operator= (const BufferCreateInfo &other)
{
	sType						= other.sType;
	pNext						= other.pNext;
	size						= other.size;
	usage						= other.usage;
	flags						= other.flags;
	sharingMode					= other.sharingMode;
	queueFamilyIndexCount		= other.queueFamilyIndexCount;

	m_queueFamilyIndices		= other.m_queueFamilyIndices;

	DE_ASSERT(m_queueFamilyIndices.size() == queueFamilyIndexCount);

	if (m_queueFamilyIndices.size())
	{
		pQueueFamilyIndices = &m_queueFamilyIndices[0];
	}
	else
	{
		pQueueFamilyIndices = DE_NULL;
	}

	return *this;
}

ImageCreateInfo::ImageCreateInfo (vk::VkImageType			_imageType,
								  vk::VkFormat				_format,
								  vk::VkExtent3D			_extent,
								  deUint32					_mipLevels,
								  deUint32					_arrayLayers,
								  vk::VkSampleCountFlagBits	_samples,
								  vk::VkImageTiling			_tiling,
								  vk::VkImageUsageFlags		_usage,
								  vk::VkSharingMode			_sharingMode,
								  deUint32					_queueFamilyIndexCount,
								  const deUint32*			_pQueueFamilyIndices,
								  vk::VkImageCreateFlags	_flags,
								  vk::VkImageLayout			_initialLayout)
{
	if (_queueFamilyIndexCount)
	{
		m_queueFamilyIndices = std::vector<deUint32>(_pQueueFamilyIndices, _pQueueFamilyIndices + _queueFamilyIndexCount);
	}

	sType = vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	pNext = DE_NULL;
	flags					= _flags;
	imageType				= _imageType;
	format					= _format;
	extent					= _extent;
	mipLevels				= _mipLevels;
	arrayLayers				= _arrayLayers;
	samples					= _samples;
	tiling					= _tiling;
	usage					= _usage;
	sharingMode				= _sharingMode;
	queueFamilyIndexCount	= _queueFamilyIndexCount;

	if (m_queueFamilyIndices.size())
	{
		pQueueFamilyIndices = &m_queueFamilyIndices[0];
	}
	else
	{
		pQueueFamilyIndices = DE_NULL;
	}
	initialLayout	= _initialLayout;
}

FramebufferCreateInfo::FramebufferCreateInfo (vk::VkRenderPass						_renderPass,
											  const std::vector<vk::VkImageView>&	atachments,
											  deUint32								_width,
											  deUint32								_height,
											  deUint32								_layers)
{
	sType = vk::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	pNext = DE_NULL;
	flags = 0u;

	renderPass		= _renderPass;
	attachmentCount	= static_cast<deUint32>(atachments.size());

	if (attachmentCount)
	{
		pAttachments = const_cast<vk::VkImageView *>(&atachments[0]);
	}

	width	= _width;
	height	= _height;
	layers	= _layers;
}

RenderPassCreateInfo::RenderPassCreateInfo (const std::vector<vk::VkAttachmentDescription>&	attachments,
											const std::vector<vk::VkSubpassDescription>&	subpasses,
											const std::vector<vk::VkSubpassDependency>&		dependiences)

	: m_attachments			(attachments.begin(), attachments.end())
	, m_subpasses			(subpasses.begin(), subpasses.end())
	, m_dependiences		(dependiences.begin(), dependiences.end())
	, m_attachmentsStructs	(m_attachments.begin(), m_attachments.end())
	, m_subpassesStructs	(m_subpasses.begin(), m_subpasses.end())
	, m_dependiencesStructs	(m_dependiences.begin(), m_dependiences.end())
{
	sType = vk::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	pNext = DE_NULL;
	flags = 0;

	attachmentCount = static_cast<deUint32>(m_attachments.size());
	pAttachments	= &m_attachmentsStructs[0];
	subpassCount	= static_cast<deUint32>(m_subpasses.size());
	pSubpasses		= &m_subpassesStructs[0];
	dependencyCount = static_cast<deUint32>(m_dependiences.size());
	pDependencies	= &m_dependiencesStructs[0];
}

RenderPassCreateInfo::RenderPassCreateInfo (deUint32							_attachmentCount,
											const vk::VkAttachmentDescription*	_pAttachments,
											deUint32							_subpassCount,
											const vk::VkSubpassDescription*		_pSubpasses,
											deUint32							_dependencyCount,
											const vk::VkSubpassDependency*		_pDependiences)
{

	m_attachments	= std::vector<AttachmentDescription>(_pAttachments, _pAttachments + _attachmentCount);
	m_subpasses		= std::vector<SubpassDescription>(_pSubpasses, _pSubpasses + _subpassCount);
	m_dependiences	= std::vector<SubpassDependency>(_pDependiences, _pDependiences + _dependencyCount);

	m_attachmentsStructs	= std::vector<vk::VkAttachmentDescription>	(m_attachments.begin(),		m_attachments.end());
	m_subpassesStructs		= std::vector<vk::VkSubpassDescription>		(m_subpasses.begin(),		m_subpasses.end());
	m_dependiencesStructs	= std::vector<vk::VkSubpassDependency>		(m_dependiences.begin(),	m_dependiences.end());

	sType = vk::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	pNext = DE_NULL;
	flags = 0;

	attachmentCount = static_cast<deUint32>(m_attachments.size());

	if (attachmentCount) {
		pAttachments = &m_attachmentsStructs[0];
	}
	else
	{
		pAttachments = DE_NULL;
	}

	subpassCount = static_cast<deUint32>(m_subpasses.size());

	if (subpassCount) {
		pSubpasses = &m_subpassesStructs[0];
	}
	else
	{
		pSubpasses = DE_NULL;
	}

	dependencyCount = static_cast<deUint32>(m_dependiences.size());

	if (dependencyCount) {
		pDependencies = &m_dependiencesStructs[0];
	}
	else
	{
		pDependencies = DE_NULL;
	}
}

void
RenderPassCreateInfo::addAttachment (vk::VkAttachmentDescription attachment)
{

	m_attachments.push_back(attachment);
	m_attachmentsStructs	= std::vector<vk::VkAttachmentDescription>(m_attachments.begin(), m_attachments.end());
	attachmentCount			= static_cast<deUint32>(m_attachments.size());
	pAttachments			= &m_attachmentsStructs[0];
}

void
RenderPassCreateInfo::addSubpass (vk::VkSubpassDescription subpass)
{

	m_subpasses.push_back(subpass);
	m_subpassesStructs	= std::vector<vk::VkSubpassDescription>(m_subpasses.begin(), m_subpasses.end());
	subpassCount		= static_cast<deUint32>(m_subpasses.size());
	pSubpasses			= &m_subpassesStructs[0];
}

void
RenderPassCreateInfo::addDependency (vk::VkSubpassDependency dependency)
{

	m_dependiences.push_back(dependency);
	m_dependiencesStructs	= std::vector<vk::VkSubpassDependency>(m_dependiences.begin(), m_dependiences.end());

	dependencyCount			= static_cast<deUint32>(m_dependiences.size());
	pDependencies			= &m_dependiencesStructs[0];
}

RenderPassBeginInfo::RenderPassBeginInfo (vk::VkRenderPass						_renderPass,
										  vk::VkFramebuffer						_framebuffer,
										  vk::VkRect2D							_renderArea,
										  const std::vector<vk::VkClearValue>&	_clearValues)
{

	m_clearValues	= _clearValues;

	sType			= vk::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pNext			= DE_NULL;
	renderPass		= _renderPass;
	framebuffer		= _framebuffer;
	renderArea		= _renderArea;
	clearValueCount = static_cast<deUint32>(m_clearValues.size());
	pClearValues	= m_clearValues.size() ? &m_clearValues[0] : DE_NULL;
}

CmdPoolCreateInfo::CmdPoolCreateInfo (deUint32 _queueFamilyIndex, unsigned int _flags)
{
	sType = vk::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pNext = DE_NULL;

	queueFamilyIndex = _queueFamilyIndex;
	flags				= _flags;
}

AttachmentDescription::AttachmentDescription (vk::VkFormat				_format,
											  vk::VkSampleCountFlagBits	_samples,
											  vk::VkAttachmentLoadOp	_loadOp,
											  vk::VkAttachmentStoreOp	_storeOp,
											  vk::VkAttachmentLoadOp	_stencilLoadOp,
											  vk::VkAttachmentStoreOp	_stencilStoreOp,
											  vk::VkImageLayout			_initialLayout,
											  vk::VkImageLayout			_finalLayout)
{
	flags = 0;
	format			= _format;
	samples			= _samples;
	loadOp			= _loadOp;
	storeOp			= _storeOp;
	stencilLoadOp	= _stencilLoadOp;
	stencilStoreOp	= _stencilStoreOp;
	initialLayout	= _initialLayout;
	finalLayout		= _finalLayout;
}

AttachmentDescription::AttachmentDescription (const vk::VkAttachmentDescription& rhs)
{
	flags			= rhs.flags;
	format			= rhs.format;
	samples			= rhs.samples;
	loadOp			= rhs.loadOp;
	storeOp			= rhs.storeOp;
	stencilLoadOp	= rhs.stencilLoadOp;
	stencilStoreOp	= rhs.stencilStoreOp;
	initialLayout	= rhs.initialLayout;
	finalLayout		= rhs.finalLayout;
}

AttachmentReference::AttachmentReference (deUint32 _attachment, vk::VkImageLayout _layout)
{
	attachment	= _attachment;
	layout		= _layout;
}

AttachmentReference::AttachmentReference (void)
{
	attachment = VK_ATTACHMENT_UNUSED;
	layout = vk::VK_IMAGE_LAYOUT_UNDEFINED;
}

SubpassDescription::SubpassDescription (vk::VkPipelineBindPoint				_pipelineBindPoint,
										vk::VkSubpassDescriptionFlags		_flags,
										deUint32							_inputAttachmentCount,
										const vk::VkAttachmentReference*	_inputAttachments,
										deUint32							_colorAttachmentCount,
										const vk::VkAttachmentReference*	_colorAttachments,
										const vk::VkAttachmentReference*	_resolveAttachments,
										vk::VkAttachmentReference			depthStencilAttachment,
										deUint32							_preserveAttachmentCount,
										const deUint32*						_preserveAttachments)
{
	m_inputAttachments = std::vector<vk::VkAttachmentReference>(_inputAttachments, _inputAttachments + _inputAttachmentCount);
	m_colorAttachments = std::vector<vk::VkAttachmentReference>(_colorAttachments, _colorAttachments + _colorAttachmentCount);

	if (_resolveAttachments)
		m_resolveAttachments = std::vector<vk::VkAttachmentReference>(_resolveAttachments, _resolveAttachments + _colorAttachmentCount);

	m_preserveAttachments = std::vector<deUint32>(_preserveAttachments, _preserveAttachments + _preserveAttachmentCount);

	m_depthStencilAttachment = depthStencilAttachment;

	flags					= _flags;
	pipelineBindPoint		= _pipelineBindPoint;
	inputAttachmentCount	= _inputAttachmentCount;
	pInputAttachments		= DE_NULL;
	colorAttachmentCount	= _colorAttachmentCount;
	pColorAttachments		= DE_NULL;
	pResolveAttachments		= DE_NULL;
	pDepthStencilAttachment	= &m_depthStencilAttachment;
	pPreserveAttachments	= DE_NULL;
	preserveAttachmentCount	= _preserveAttachmentCount;

	if (!m_inputAttachments.empty())
		pInputAttachments = &m_inputAttachments[0];

	if (!m_colorAttachments.empty())
		pColorAttachments = &m_colorAttachments[0];

	if (!m_resolveAttachments.empty())
		pResolveAttachments = &m_resolveAttachments[0];

	if (!m_preserveAttachments.empty())
		pPreserveAttachments = &m_preserveAttachments[0];
}

SubpassDescription::SubpassDescription (const vk::VkSubpassDescription& rhs)
{
	*static_cast<vk::VkSubpassDescription*>(this) = rhs;

	m_inputAttachments = std::vector<vk::VkAttachmentReference>(
		rhs.pInputAttachments, rhs.pInputAttachments + rhs.inputAttachmentCount);

	m_colorAttachments = std::vector<vk::VkAttachmentReference>(
		rhs.pColorAttachments, rhs.pColorAttachments + rhs.colorAttachmentCount);

	if (rhs.pResolveAttachments)
		m_resolveAttachments = std::vector<vk::VkAttachmentReference>(
			rhs.pResolveAttachments, rhs.pResolveAttachments + rhs.colorAttachmentCount);

	m_preserveAttachments = std::vector<deUint32>(
		rhs.pPreserveAttachments, rhs.pPreserveAttachments + rhs.preserveAttachmentCount);

	if (rhs.pDepthStencilAttachment)
		m_depthStencilAttachment = *rhs.pDepthStencilAttachment;

	if (!m_inputAttachments.empty())
		pInputAttachments = &m_inputAttachments[0];

	if (!m_colorAttachments.empty())
		pColorAttachments = &m_colorAttachments[0];

	if (!m_resolveAttachments.empty())
		pResolveAttachments = &m_resolveAttachments[0];

	pDepthStencilAttachment = &m_depthStencilAttachment;

	if (!m_preserveAttachments.empty())
		pPreserveAttachments = &m_preserveAttachments[0];
}

SubpassDescription::SubpassDescription (const SubpassDescription& rhs) {
	*this = rhs;
}

SubpassDescription& SubpassDescription::operator= (const SubpassDescription& rhs)
{
	*static_cast<vk::VkSubpassDescription*>(this) = rhs;

	m_inputAttachments		= rhs.m_inputAttachments;
	m_colorAttachments		= rhs.m_colorAttachments;
	m_resolveAttachments	= rhs.m_resolveAttachments;
	m_preserveAttachments	= rhs.m_preserveAttachments;
	m_depthStencilAttachment = rhs.m_depthStencilAttachment;

	if (!m_inputAttachments.empty())
		pInputAttachments = &m_inputAttachments[0];

	if (!m_colorAttachments.empty())
		pColorAttachments = &m_colorAttachments[0];

	if (!m_resolveAttachments.empty())
		pResolveAttachments = &m_resolveAttachments[0];

	pDepthStencilAttachment = &m_depthStencilAttachment;

	if (!m_preserveAttachments.empty())
		pPreserveAttachments = &m_preserveAttachments[0];

	return *this;
}

SubpassDependency::SubpassDependency (deUint32					_srcSubpass,
									  deUint32					_dstSubpass,
									  vk::VkPipelineStageFlags	_srcStageMask,
									  vk::VkPipelineStageFlags	_dstStageMask,
									  vk::VkAccessFlags			_srcAccessMask,
									  vk::VkAccessFlags			_dstAccessMask,
									  vk::VkDependencyFlags		_dependencyFlags)
{
	srcSubpass		= _srcSubpass;
	dstSubpass		= _dstSubpass;
	srcStageMask	= _srcStageMask;
	dstStageMask	= _dstStageMask;
	srcAccessMask	= _srcAccessMask;
	dstAccessMask	= _dstAccessMask;
	dependencyFlags	= _dependencyFlags;
}

SubpassDependency::SubpassDependency (const vk::VkSubpassDependency& rhs)
{
	srcSubpass		= rhs.srcSubpass;
	dstSubpass		= rhs.dstSubpass;
	srcStageMask	= rhs.srcStageMask;
	dstStageMask	= rhs.dstStageMask;
	srcAccessMask	= rhs.srcAccessMask;
	dstAccessMask	= rhs.dstAccessMask;
	dependencyFlags	= rhs.dependencyFlags;
}

CmdBufferBeginInfo::CmdBufferBeginInfo (vk::VkCommandBufferUsageFlags _flags)
{
	sType				= vk::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	pNext				= DE_NULL;
	flags				= _flags;
	pInheritanceInfo	= DE_NULL;
}

DescriptorPoolCreateInfo::DescriptorPoolCreateInfo (const std::vector<vk::VkDescriptorPoolSize>&	poolSizeCounts,
													vk::VkDescriptorPoolCreateFlags					_flags,
													deUint32										_maxSets)
	: m_poolSizeCounts(poolSizeCounts)
{
	sType = vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pNext = DE_NULL;
	flags			= _flags;
	maxSets			= _maxSets;
	poolSizeCount	= static_cast<deUint32>(m_poolSizeCounts.size());
	pPoolSizes		= &m_poolSizeCounts[0];
}

DescriptorPoolCreateInfo& DescriptorPoolCreateInfo::addDescriptors (vk::VkDescriptorType type, deUint32 count)
{
	vk::VkDescriptorPoolSize descriptorTypeCount = { type, count };
	m_poolSizeCounts.push_back(descriptorTypeCount);

	poolSizeCount	= static_cast<deUint32>(m_poolSizeCounts.size());
	pPoolSizes		= &m_poolSizeCounts[0];

	return *this;
}

DescriptorSetLayoutCreateInfo::DescriptorSetLayoutCreateInfo (deUint32 _bindingCount, const vk::VkDescriptorSetLayoutBinding* _pBindings)
{
	sType = vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	pNext = DE_NULL;
	flags = 0;
	bindingCount = _bindingCount;
	pBindings	 = _pBindings;
}

PipelineLayoutCreateInfo::PipelineLayoutCreateInfo (deUint32							_descriptorSetCount,
													const vk::VkDescriptorSetLayout*	_pSetLayouts,
													deUint32							_pushConstantRangeCount,
													const vk::VkPushConstantRange*		_pPushConstantRanges)
	: m_pushConstantRanges(_pPushConstantRanges, _pPushConstantRanges + _pushConstantRangeCount)
{
	for (unsigned int i = 0; i < _descriptorSetCount; i++)
	{
		m_setLayouts.push_back(_pSetLayouts[i]);
	}

	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pNext = DE_NULL;
	flags					= 0;
	setLayoutCount			= static_cast<deUint32>(m_setLayouts.size());
	pSetLayouts				= setLayoutCount > 0 ? &m_setLayouts[0] : DE_NULL;
	pushConstantRangeCount	= static_cast<deUint32>(m_pushConstantRanges.size());

	if (m_pushConstantRanges.size()) {
		pPushConstantRanges = &m_pushConstantRanges[0];
	}
	else
	{
		pPushConstantRanges = DE_NULL;
	}
}

PipelineLayoutCreateInfo::PipelineLayoutCreateInfo (const std::vector<vk::VkDescriptorSetLayout>&	setLayouts,
													deUint32										_pushConstantRangeCount,
													const vk::VkPushConstantRange*					_pPushConstantRanges)
	: m_setLayouts			(setLayouts)
	, m_pushConstantRanges	(_pPushConstantRanges, _pPushConstantRanges + _pushConstantRangeCount)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pNext = DE_NULL;

	flags			= 0;
	setLayoutCount	= static_cast<deUint32>(m_setLayouts.size());

	if (setLayoutCount)
	{
		pSetLayouts = &m_setLayouts[0];
	}
	else
	{
		pSetLayouts = DE_NULL;
	}

	pushConstantRangeCount = static_cast<deUint32>(m_pushConstantRanges.size());
	if (pushConstantRangeCount) {
		pPushConstantRanges = &m_pushConstantRanges[0];
	}
	else
	{
		pPushConstantRanges = DE_NULL;
	}
}

PipelineCreateInfo::PipelineShaderStage::PipelineShaderStage (vk::VkShaderModule _module, const char* _pName, vk::VkShaderStageFlagBits _stage)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pNext = DE_NULL;
	flags = 0u;
	stage				= _stage;
	module				= _module;
	pName				= _pName;
	pSpecializationInfo = DE_NULL;
}

PipelineCreateInfo::VertexInputState::VertexInputState (deUint32										_vertexBindingDescriptionCount,
														const vk::VkVertexInputBindingDescription*		_pVertexBindingDescriptions,
														deUint32										_vertexAttributeDescriptionCount,
														const vk::VkVertexInputAttributeDescription*	_pVertexAttributeDescriptions)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pNext = DE_NULL;
	flags							= 0u;
	vertexBindingDescriptionCount	= _vertexBindingDescriptionCount;
	pVertexBindingDescriptions		= _pVertexBindingDescriptions;
	vertexAttributeDescriptionCount	= _vertexAttributeDescriptionCount;
	pVertexAttributeDescriptions	= _pVertexAttributeDescriptions;
}

PipelineCreateInfo::InputAssemblerState::InputAssemblerState (vk::VkPrimitiveTopology	_topology,
															  vk::VkBool32				_primitiveRestartEnable)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pNext = DE_NULL;
	flags					= 0u;
	topology				= _topology;
	primitiveRestartEnable	= _primitiveRestartEnable;
}

PipelineCreateInfo::TessellationState::TessellationState (deUint32 _patchControlPoints)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	pNext = DE_NULL;
	flags				= 0;
	patchControlPoints	= _patchControlPoints;
}

PipelineCreateInfo::ViewportState::ViewportState (deUint32						_viewportCount,
												  std::vector<vk::VkViewport>	_viewports,
												  std::vector<vk::VkRect2D>		_scissors)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pNext = DE_NULL;
	flags			= 0u;
	viewportCount	= _viewportCount;
	scissorCount	= _viewportCount;

	if (!_viewports.size())
	{
		m_viewports.resize(viewportCount);
		deMemset(&m_viewports[0], 0, sizeof(m_viewports[0]) * m_viewports.size());
	}
	else
	{
		m_viewports = _viewports;
	}

	if (!_scissors.size())
	{
		m_scissors.resize(scissorCount);
		deMemset(&m_scissors[0], 0, sizeof(m_scissors[0]) * m_scissors.size());
	}
	else
	{
		m_scissors = _scissors;
	}

	pViewports	= &m_viewports[0];
	pScissors	= &m_scissors[0];
}

PipelineCreateInfo::ViewportState::ViewportState (const ViewportState& other)
{
	sType			= other.sType;
	pNext			= other.pNext;
	flags			= other.flags;
	viewportCount	= other.viewportCount;
	scissorCount	= other.scissorCount;

	m_viewports = std::vector<vk::VkViewport>(other.pViewports, other.pViewports + viewportCount);
	m_scissors	= std::vector<vk::VkRect2D>(other.pScissors, other.pScissors + scissorCount);

	pViewports	= &m_viewports[0];
	pScissors	= &m_scissors[0];
}

PipelineCreateInfo::ViewportState& PipelineCreateInfo::ViewportState::operator= (const ViewportState& other)
{
	sType			= other.sType;
	pNext			= other.pNext;
	flags			= other.flags;
	viewportCount	= other.viewportCount;
	scissorCount	= other.scissorCount;

	m_viewports		= std::vector<vk::VkViewport>(other.pViewports, other.pViewports + scissorCount);
	m_scissors		= std::vector<vk::VkRect2D>(other.pScissors, other.pScissors + scissorCount);

	pViewports		= &m_viewports[0];
	pScissors		= &m_scissors[0];
	return *this;
}

PipelineCreateInfo::RasterizerState::RasterizerState (vk::VkBool32			_depthClampEnable,
													  vk::VkBool32			_rasterizerDiscardEnable,
													  vk::VkPolygonMode		_polygonMode,
													  vk::VkCullModeFlags	_cullMode,
													  vk::VkFrontFace		_frontFace,
													  vk::VkBool32			_depthBiasEnable,
													  float					_depthBiasConstantFactor,
													  float					_depthBiasClamp,
													  float					_depthBiasSlopeFactor,
													  float					_lineWidth)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pNext = DE_NULL;
	flags					= 0u;
	depthClampEnable		= _depthClampEnable;
	rasterizerDiscardEnable = _rasterizerDiscardEnable;
	polygonMode				= _polygonMode;
	cullMode				= _cullMode;
	frontFace				= _frontFace;

	depthBiasEnable			= _depthBiasEnable;
	depthBiasConstantFactor	= _depthBiasConstantFactor;
	depthBiasClamp			= _depthBiasClamp;
	depthBiasSlopeFactor	= _depthBiasSlopeFactor;
	lineWidth				= _lineWidth;
}

PipelineCreateInfo::MultiSampleState::MultiSampleState (vk::VkSampleCountFlagBits				_rasterizationSamples,
														vk::VkBool32							_sampleShadingEnable,
														float									_minSampleShading,
														const std::vector<vk::VkSampleMask>&	_sampleMask,
														bool									_alphaToCoverageEnable,
														bool									_alphaToOneEnable)
	: m_sampleMask(_sampleMask)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pNext = DE_NULL;
	flags					= 0u;
	rasterizationSamples	= _rasterizationSamples;
	sampleShadingEnable		= _sampleShadingEnable;
	minSampleShading		= _minSampleShading;
	pSampleMask				= &m_sampleMask[0];
	alphaToCoverageEnable   = _alphaToCoverageEnable;
	alphaToOneEnable		= _alphaToOneEnable;
}

PipelineCreateInfo::MultiSampleState::MultiSampleState (const MultiSampleState& other)
{
	sType					= other.sType;
	pNext					= other.pNext;
	flags					= other.flags;
	rasterizationSamples	= other.rasterizationSamples;
	sampleShadingEnable		= other.sampleShadingEnable;
	minSampleShading		= other.minSampleShading;

	const size_t sampleMaskArrayLen = (sizeof(vk::VkSampleMask) * 8 + other.rasterizationSamples) / (sizeof(vk::VkSampleMask) * 8);

	m_sampleMask	= std::vector<vk::VkSampleMask>(other.pSampleMask, other.pSampleMask + sampleMaskArrayLen);
	pSampleMask		= &m_sampleMask[0];
}

PipelineCreateInfo::MultiSampleState& PipelineCreateInfo::MultiSampleState::operator= (const MultiSampleState& other)
{
	sType = other.sType;
	pNext = other.pNext;
	flags					= other.flags;
	rasterizationSamples	= other.rasterizationSamples;
	sampleShadingEnable		= other.sampleShadingEnable;
	minSampleShading		= other.minSampleShading;

	const size_t sampleMaskArrayLen = (sizeof(vk::VkSampleMask) * 8 + other.rasterizationSamples) / (sizeof(vk::VkSampleMask) * 8);

	m_sampleMask	= std::vector<vk::VkSampleMask>(other.pSampleMask, other.pSampleMask + sampleMaskArrayLen);
	pSampleMask		= &m_sampleMask[0];

	return *this;
}

PipelineCreateInfo::ColorBlendState::ColorBlendState (const std::vector<vk::VkPipelineColorBlendAttachmentState>&	_attachments,
													  vk::VkBool32													_logicOpEnable,
													  vk::VkLogicOp													_logicOp)
	: m_attachments(_attachments)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pNext = DE_NULL;
	flags					= 0u;
	logicOpEnable			= _logicOpEnable;
	logicOp					= _logicOp;
	attachmentCount			= static_cast<deUint32>(m_attachments.size());
	pAttachments			= &m_attachments[0];
}

PipelineCreateInfo::ColorBlendState::ColorBlendState (deUint32											_attachmentCount,
													  const vk::VkPipelineColorBlendAttachmentState*	_attachments,
													  vk::VkBool32										_logicOpEnable,
													  vk::VkLogicOp										_logicOp)
	: m_attachments(_attachments, _attachments + _attachmentCount)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pNext	= DE_NULL;
	flags					= 0;
	logicOpEnable			= _logicOpEnable;
	logicOp					= _logicOp;
	attachmentCount			= static_cast<deUint32>(m_attachments.size());
	pAttachments			= &m_attachments[0];
}

PipelineCreateInfo::ColorBlendState::ColorBlendState (const vk::VkPipelineColorBlendStateCreateInfo& createInfo)
	: m_attachments (createInfo.pAttachments, createInfo.pAttachments + createInfo.attachmentCount)
{
	sType = createInfo.sType;
	pNext = createInfo.pNext;
	flags					= createInfo.flags;
	logicOpEnable			= createInfo.logicOpEnable;
	logicOp					= createInfo.logicOp;
	attachmentCount			= static_cast<deUint32>(m_attachments.size());
	pAttachments			= &m_attachments[0];
}

PipelineCreateInfo::ColorBlendState::ColorBlendState (const ColorBlendState& createInfo, std::vector<float> _blendConstants)
	: m_attachments (createInfo.pAttachments, createInfo.pAttachments + createInfo.attachmentCount)
{
	sType = createInfo.sType;
	pNext = createInfo.pNext;
	flags					= createInfo.flags;
	logicOpEnable			= createInfo.logicOpEnable;
	logicOp					= createInfo.logicOp;
	attachmentCount			= static_cast<deUint32>(m_attachments.size());
	pAttachments			= &m_attachments[0];
	deMemcpy(blendConstants, &_blendConstants[0], 4 * sizeof(float));
}

PipelineCreateInfo::ColorBlendState::Attachment::Attachment (vk::VkBool32				_blendEnable,
															 vk::VkBlendFactor			_srcColorBlendFactor,
															 vk::VkBlendFactor			_dstColorBlendFactor,
															 vk::VkBlendOp				_colorBlendOp,
															 vk::VkBlendFactor			_srcAlphaBlendFactor,
															 vk::VkBlendFactor			_dstAlphaBlendFactor,
															 vk::VkBlendOp				_alphaBlendOp,
															 vk::VkColorComponentFlags	_colorWriteMask)
{
	blendEnable			= _blendEnable;
	srcColorBlendFactor	= _srcColorBlendFactor;
	dstColorBlendFactor	= _dstColorBlendFactor;
	colorBlendOp		= _colorBlendOp;
	srcAlphaBlendFactor	= _srcAlphaBlendFactor;
	dstAlphaBlendFactor	= _dstAlphaBlendFactor;
	alphaBlendOp		= _alphaBlendOp;
	colorWriteMask		= _colorWriteMask;
}

PipelineCreateInfo::DepthStencilState::StencilOpState::StencilOpState (vk::VkStencilOp	_failOp,
																	   vk::VkStencilOp	_passOp,
																	   vk::VkStencilOp	_depthFailOp,
																	   vk::VkCompareOp	_compareOp,
																	   deUint32			_compareMask,
																	   deUint32			_writeMask,
																	   deUint32			_reference)
{
	failOp		= _failOp;
	passOp		= _passOp;
	depthFailOp	= _depthFailOp;
	compareOp	= _compareOp;

	compareMask	= _compareMask;
	writeMask	= _writeMask;
	reference	= _reference;
}

PipelineCreateInfo::DepthStencilState::DepthStencilState (vk::VkBool32		_depthTestEnable,
														  vk::VkBool32		_depthWriteEnable,
														  vk::VkCompareOp	_depthCompareOp,
														  vk::VkBool32		_depthBoundsTestEnable,
														  vk::VkBool32		_stencilTestEnable,
														  StencilOpState	_front,
														  StencilOpState	_back,
														  float				_minDepthBounds,
														  float				_maxDepthBounds)
{
	sType = vk::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pNext = DE_NULL;
	flags					= 0u;
	depthTestEnable			= _depthTestEnable;
	depthWriteEnable		= _depthWriteEnable;
	depthCompareOp			= _depthCompareOp;
	depthBoundsTestEnable	= _depthBoundsTestEnable;
	stencilTestEnable		= _stencilTestEnable;
	front	= _front;
	back	= _back;

	minDepthBounds = _minDepthBounds;
	maxDepthBounds = _maxDepthBounds;
}

PipelineCreateInfo::DynamicState::DynamicState (const std::vector<vk::VkDynamicState>& _dynamicStates)
{
	sType	= vk::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pNext	= DE_NULL;
	flags	= 0;

	if (!_dynamicStates.size())
	{
		for (size_t i = 0; i < vk::VK_DYNAMIC_STATE_LAST; ++i)
		{
			m_dynamicStates.push_back(static_cast<vk::VkDynamicState>(i));
		}
	}
	else
		m_dynamicStates = _dynamicStates;

	dynamicStateCount = static_cast<deUint32>(m_dynamicStates.size());
	pDynamicStates = &m_dynamicStates[0];
}

PipelineCreateInfo::DynamicState::DynamicState (const DynamicState &other)
{
	sType = other.sType;
	pNext = other.pNext;

	flags				= other.flags;
	dynamicStateCount	= other.dynamicStateCount;
	m_dynamicStates		= std::vector<vk::VkDynamicState>(other.pDynamicStates, other.pDynamicStates + dynamicStateCount);
	pDynamicStates		= &m_dynamicStates[0];
}

PipelineCreateInfo::DynamicState& PipelineCreateInfo::DynamicState::operator= (const DynamicState& other)
{
	sType = other.sType;
	pNext = other.pNext;

	flags				= other.flags;
	dynamicStateCount	= other.dynamicStateCount;
	m_dynamicStates		= std::vector<vk::VkDynamicState>(other.pDynamicStates, other.pDynamicStates + dynamicStateCount);
	pDynamicStates		= &m_dynamicStates[0];

	return *this;
}

PipelineCreateInfo::PipelineCreateInfo (vk::VkPipelineLayout		_layout,
										vk::VkRenderPass			_renderPass,
										int							_subpass,
										vk::VkPipelineCreateFlags	_flags)
{
	deMemset(static_cast<vk::VkGraphicsPipelineCreateInfo *>(this), 0,
		sizeof(vk::VkGraphicsPipelineCreateInfo));

	sType = vk::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pNext = DE_NULL;
	flags				= _flags;
	renderPass			= _renderPass;
	subpass				= _subpass;
	layout				= _layout;
	basePipelineHandle	= DE_NULL;
	basePipelineIndex	= 0;
	pDynamicState		= DE_NULL;
}

PipelineCreateInfo& PipelineCreateInfo::addShader (const vk::VkPipelineShaderStageCreateInfo& shader)
{
	m_shaders.push_back(shader);

	stageCount	= static_cast<deUint32>(m_shaders.size());
	pStages		= &m_shaders[0];

	return *this;
}

PipelineCreateInfo& PipelineCreateInfo::addState (const vk::VkPipelineVertexInputStateCreateInfo& state)
{
	m_vertexInputState	= state;
	pVertexInputState	= &m_vertexInputState;

	return *this;
}

PipelineCreateInfo& PipelineCreateInfo::addState (const vk::VkPipelineInputAssemblyStateCreateInfo& state)
{
	m_inputAssemblyState = state;
	pInputAssemblyState = &m_inputAssemblyState;

	return *this;
}

PipelineCreateInfo& PipelineCreateInfo::addState (const vk::VkPipelineColorBlendStateCreateInfo& state)
{
	m_colorBlendStateAttachments	= std::vector<vk::VkPipelineColorBlendAttachmentState>(state.pAttachments, state.pAttachments + state.attachmentCount);
	m_colorBlendState				= state;
	m_colorBlendState.pAttachments	= &m_colorBlendStateAttachments[0];
	pColorBlendState				= &m_colorBlendState;

	return *this;
}

PipelineCreateInfo& PipelineCreateInfo::addState (const vk::VkPipelineViewportStateCreateInfo& state)
{
	m_viewports					= std::vector<vk::VkViewport>(state.pViewports, state.pViewports + state.viewportCount);
	m_scissors					= std::vector<vk::VkRect2D>(state.pScissors, state.pScissors + state.scissorCount);
	m_viewportState				= state;
	m_viewportState.pViewports	= &m_viewports[0];
	m_viewportState.pScissors	= &m_scissors[0];
	pViewportState				= &m_viewportState;

	return *this;
}

PipelineCreateInfo& PipelineCreateInfo::addState (const vk::VkPipelineDepthStencilStateCreateInfo& state)
{
	m_dynamicDepthStencilState	= state;
	pDepthStencilState			= &m_dynamicDepthStencilState;
	return *this;
}

PipelineCreateInfo& PipelineCreateInfo::addState (const vk::VkPipelineTessellationStateCreateInfo& state)
{
	m_tessState			= state;
	pTessellationState	= &m_tessState;

	return *this;
}

PipelineCreateInfo& PipelineCreateInfo::addState (const vk::VkPipelineRasterizationStateCreateInfo& state)
{
	m_rasterState		= state;
	pRasterizationState	= &m_rasterState;

	return *this;
}

PipelineCreateInfo& PipelineCreateInfo::addState (const vk::VkPipelineMultisampleStateCreateInfo& state)
{

	const size_t sampleMaskArrayLen = (sizeof(vk::VkSampleMask) * 8 + state.rasterizationSamples) / ( sizeof(vk::VkSampleMask) * 8 );
	m_multisampleStateSampleMask	= std::vector<vk::VkSampleMask>(state.pSampleMask, state.pSampleMask + sampleMaskArrayLen);
	m_multisampleState				= state;
	m_multisampleState.pSampleMask	= &m_multisampleStateSampleMask[0];
	pMultisampleState				= &m_multisampleState;

	return *this;
}
PipelineCreateInfo& PipelineCreateInfo::addState (const vk::VkPipelineDynamicStateCreateInfo& state)
{
	m_dynamicStates					= std::vector<vk::VkDynamicState>(state.pDynamicStates, state.pDynamicStates + state.dynamicStateCount);
	m_dynamicState					= state;
	m_dynamicState.pDynamicStates	= &m_dynamicStates[0];
	pDynamicState					= &m_dynamicState;

	return *this;
}

SamplerCreateInfo::SamplerCreateInfo (vk::VkFilter				_magFilter,
									  vk::VkFilter				_minFilter,
									  vk::VkSamplerMipmapMode	_mipmapMode,
									  vk::VkSamplerAddressMode	_addressModeU,
									  vk::VkSamplerAddressMode	_addressModeV,
									  vk::VkSamplerAddressMode	_addressModeW,
									  float						_mipLodBias,
									  vk::VkBool32				_anisotropyEnable,
									  float						_maxAnisotropy,
									  vk::VkBool32				_compareEnable,
									  vk::VkCompareOp			_compareOp,
									  float						_minLod,
									  float						_maxLod,
									  vk::VkBorderColor			_borderColor,
									  vk::VkBool32				_unnormalizedCoordinates)
{
	sType					= vk::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	pNext					= DE_NULL;
	flags					= 0u;
	magFilter				= _magFilter;
	minFilter				= _minFilter;
	mipmapMode				= _mipmapMode;
	addressModeU			= _addressModeU;
	addressModeV			= _addressModeV;
	addressModeW			= _addressModeW;
	mipLodBias				= _mipLodBias;
	anisotropyEnable		= _anisotropyEnable;
	maxAnisotropy			= _maxAnisotropy;
	compareEnable			= _compareEnable;
	compareOp				= _compareOp;
	minLod					= _minLod;
	maxLod					= _maxLod;
	borderColor				= _borderColor;
	unnormalizedCoordinates = _unnormalizedCoordinates;
}
} // Draw
} // vkt
