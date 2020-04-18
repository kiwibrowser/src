/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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
 * \brief Vulkan Get Render Area Granularity Tests
 *//*--------------------------------------------------------------------*/

#include "vktApiGranularityTests.hpp"

#include "deRandom.hpp"
#include "deSharedPtr.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"
#include "vktTestCase.hpp"

#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"

#include <string>

namespace vkt
{

namespace api
{

using namespace vk;

namespace
{

struct AttachmentInfo
{
	AttachmentInfo (const VkFormat	vkFormat,
					const deUint32	width,
					const deUint32	height,
					const deUint32	depth)
		: format	(vkFormat)
	{
		extent.width	= width;
		extent.height	= height;
		extent.depth	= depth;
	}

	~AttachmentInfo (void)
	{}

	VkFormat	format;
	VkExtent3D	extent;
};

typedef de::SharedPtr<Allocation>			AllocationSp;
typedef de::SharedPtr<Unique<VkImage> >		VkImageSp;
typedef de::SharedPtr<Unique<VkImageView> >	VkImageViewSp;

class GranularityInstance : public vkt::TestInstance
{
public:
											GranularityInstance			(Context&							context,
																		 const std::vector<AttachmentInfo>&	attachments,
																		 const bool							useRenderPass);
	virtual									~GranularityInstance		(void);
	void									checkFormatSupport			(const VkFormat format);
	void									initAttachmentDescriptions	(void);
	void									initImages					(void);
	void									initRenderPass				(void);
	void									beginRenderPass				(void);
	void									endRenderPass				(void);
	virtual	tcu::TestStatus					iterate						(void);
private:
	const std::vector<AttachmentInfo>		m_attachments;
	const bool								m_useRenderPass;

	Move<VkRenderPass>						m_renderPass;
	Move<VkFramebuffer>						m_frameBuffer;
	Move<VkCommandPool>						m_cmdPool;
	Move<VkCommandBuffer>					m_cmdBuffer;
	std::vector<VkAttachmentDescription>	m_attachmentDescriptions;
	std::vector<VkImageSp>					m_images;
	std::vector<AllocationSp>				m_imageAllocs;
	std::vector<VkImageViewSp>				m_imageViews;
};

GranularityInstance::GranularityInstance (Context&								context,
										  const std::vector<AttachmentInfo>&	attachments,
										  const bool							useRenderPass)
	: vkt::TestInstance	(context)
	, m_attachments		(attachments)
	, m_useRenderPass	(useRenderPass)
{
	initAttachmentDescriptions();
}

GranularityInstance::~GranularityInstance (void)
{
}

void GranularityInstance::checkFormatSupport (const VkFormat format)
{
	VkImageFormatProperties	properties;

	VkResult result = m_context.getInstanceInterface().getPhysicalDeviceImageFormatProperties(m_context.getPhysicalDevice(),
																							  format, VK_IMAGE_TYPE_2D,
																							  VK_IMAGE_TILING_OPTIMAL,
																							  VK_IMAGE_USAGE_SAMPLED_BIT,
																							  0,
																							  &properties);

	if (result == VK_ERROR_FORMAT_NOT_SUPPORTED)
		TCU_THROW(NotSupportedError, "Format not supported");
}

void GranularityInstance::initAttachmentDescriptions (void)
{
	VkAttachmentDescription	attachmentDescription	=
	{
		0u,									// VkAttachmentDescriptionFlags	flags;
		VK_FORMAT_UNDEFINED,				// VkFormat						format;
		VK_SAMPLE_COUNT_1_BIT,				// VkSampleCountFlagBits		samples;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,	// VkAttachmentLoadOp			loadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,	// VkAttachmentStoreOp			storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,	// VkAttachmentLoadOp			stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,	// VkAttachmentStoreOp			stencilStoreOp;
		VK_IMAGE_LAYOUT_UNDEFINED,			// VkImageLayout				initialLayout;
		VK_IMAGE_LAYOUT_UNDEFINED,			// VkImageLayout				finalLayout;
	};

	for (std::vector<AttachmentInfo>::const_iterator it = m_attachments.begin(); it != m_attachments.end(); ++it)
	{
		checkFormatSupport(it->format);
		attachmentDescription.format = it->format;
		m_attachmentDescriptions.push_back(attachmentDescription);
	}
}

void GranularityInstance::initImages (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	SimpleAllocator			memAlloc			(vk, device, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));

	for (std::vector<AttachmentInfo>::const_iterator it = m_attachments.begin(); it != m_attachments.end(); ++it)
	{
		const VkImageCreateInfo		imageInfo	=
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,	// VkStructureType		sType;
			DE_NULL,								// const void*			pNext;
			0u,										// VkImageCreateFlags	flags;
			VK_IMAGE_TYPE_2D,						// VkImageType			imageType;
			it->format,								// VkFormat				format;
			it->extent,								// VkExtent3D			extent;
			1u,										// deUint32				mipLevels;
			1u,										// deUint32				arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,					// deUint32				samples;
			VK_IMAGE_TILING_OPTIMAL,				// VkImageTiling		tiling;
			VK_IMAGE_USAGE_SAMPLED_BIT,				// VkImageUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
			1u,										// deUint32				queueFamilyCount;
			&queueFamilyIndex,						// const deUint32*		pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED,				// VkImageLayout		initialLayout;
		};

		// Create the image
		Move<VkImage>			image		= createImage(vk, device, &imageInfo);
		de::MovePtr<Allocation>	imageAlloc	= memAlloc.allocate(getImageMemoryRequirements(vk, device, *image), MemoryRequirement::Any);
		VK_CHECK(vk.bindImageMemory(device, *image, imageAlloc->getMemory(), imageAlloc->getOffset()));

		VkImageAspectFlags			aspectFlags = 0;
		const tcu::TextureFormat	tcuFormat	= mapVkFormat(it->format);

		if (tcu::hasDepthComponent(tcuFormat.order))
			aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;

		if (tcu::hasStencilComponent(tcuFormat.order))
			aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;

		if (!aspectFlags)
			aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

		VkFormatProperties formatProperties;
		m_context.getInstanceInterface().getPhysicalDeviceFormatProperties(m_context.getPhysicalDevice(),
										   it->format, &formatProperties);

		if ((formatProperties.optimalTilingFeatures & (VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
							       VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) == 0)
			throw tcu::NotSupportedError("Format not supported as attachment");

		const VkImageViewCreateInfo		createInfo	=
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			0,												// VkImageViewCreateFlags	flags;
			*image,											// VkImage					image;
			VK_IMAGE_VIEW_TYPE_2D,							// VkImageViewType			viewType;
			it->format,										// VkFormat					format;
			{
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			},												// VkComponentMapping		components;
			{ aspectFlags, 0u, 1u, 0u, 1u	}	// VkImageSubresourceRange	subresourceRange;
		};

		// Create the Image View
		Move<VkImageView>	imageView		= createImageView(vk, device, &createInfo);

		// To prevent object free
		m_images.push_back(VkImageSp(new Unique<VkImage>(image)));
		m_imageAllocs.push_back(AllocationSp(imageAlloc.release()));
		m_imageViews.push_back(VkImageViewSp(new Unique<VkImageView>(imageView)));
	}
}

void GranularityInstance::initRenderPass (void)
{
	const DeviceInterface&		vk					= m_context.getDeviceInterface();
	const VkDevice				device				= m_context.getDevice();
	const deUint32				queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();

	{	// Create RenderPass
		const VkSubpassDescription	subpassDesc =
		{
			(VkSubpassDescriptionFlags)0u,		// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,	// VkPipelineBindPoint				pipelineBindPoint;
			0u,									// deUint32							inputCount;
			DE_NULL,							// const VkAttachmentReference*		pInputAttachments;
			0u,									// deUint32							colorCount;
			DE_NULL,							// const VkAttachmentReference*		pColorAttachments;
			DE_NULL,							// const VkAttachmentReference*		pResolveAttachments;
			DE_NULL,							// VkAttachmentReference			depthStencilAttachment;
			0u,									// deUint32							preserveCount;
			DE_NULL								// const VkAttachmentReference*		pPreserveAttachments;
		};

		const VkRenderPassCreateInfo	renderPassParams =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// VkStructureType					sType;
			DE_NULL,									// const void*						pNext;
			(VkRenderPassCreateFlags)0,					// VkRenderPassCreateFlags			flags;
			(deUint32)m_attachmentDescriptions.size(),	// deUint32							attachmentCount;
			&m_attachmentDescriptions[0],				// const VkAttachmentDescription*	pAttachments;
			1u,											// deUint32							subpassCount;
			&subpassDesc,								// const VkSubpassDescription*		pSubpasses;
			0u,											// deUint32							dependencyCount;
			DE_NULL										// const VkSubpassDependency*		pDependencies;
		};

		m_renderPass	= createRenderPass(vk, device, &renderPassParams);
	}

	initImages();

	{	// Create Framebuffer
		std::vector<VkImageView>	imageViews;

		for (std::vector<VkImageViewSp>::const_iterator it = m_imageViews.begin(); it != m_imageViews.end(); ++it)
			imageViews.push_back(it->get()->get());

		const VkFramebufferCreateInfo	framebufferParams	=
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			(VkFramebufferCreateFlags)0,				// VkFramebufferCreateFlags	flags;
			*m_renderPass,								// VkRenderPass				renderPass;
			(deUint32)imageViews.size(),				// deUint32					attachmentCount;
			&imageViews[0],								// const VkImageView*		pAttachments;
			1,											// deUint32					width;
			1,											// deUint32					height;
			1											// deUint32					layers;
		};

		m_frameBuffer	= createFramebuffer(vk, device, &framebufferParams);
	}

	m_cmdPool	= createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndex);

	// Create CommandBuffer
	m_cmdBuffer	= allocateCommandBuffer(vk, device, *m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	{	// Begin CommandBuffer
		const VkCommandBufferBeginInfo	cmdBufferBeginInfo	=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType							sType;
			DE_NULL,										// const void*								pNext;
			0u,												// VkCmdBufferOptimizeFlags					flags;
			DE_NULL,										// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
		};

		VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));
	}
}

void GranularityInstance::beginRenderPass (void)
{
	const DeviceInterface&	vk	= m_context.getDeviceInterface();

	const VkRect2D	renderArea	=
	{
		{ 0, 0 },	// VkOffset2D    offset;
		{ 1, 1 }	// VkExtent2D	extent;
	};

	const VkRenderPassBeginInfo		renderPassBeginInfo	=
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// VkStructureType		sType;
		DE_NULL,									// const void*			pNext;
		*m_renderPass,								// VkRenderPass			renderPass;
		*m_frameBuffer,								// VkFramebuffer		framebuffer;
		renderArea,									// VkRect2D				renderArea;
		0u,											// uint32_t				clearValueCount;
		DE_NULL										// const VkClearValue*	pClearValues;
	};

	vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void GranularityInstance::endRenderPass (void)
{
	const DeviceInterface&	vk	= m_context.getDeviceInterface();

	vk.cmdEndRenderPass(*m_cmdBuffer);
	VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
}

tcu::TestStatus GranularityInstance::iterate (void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();
	tcu::TestLog&			log		= m_context.getTestContext().getLog();

	initRenderPass();

	VkExtent2D prePassGranularity = { ~0u, ~0u };
	vk.getRenderAreaGranularity(device, *m_renderPass, &prePassGranularity);

	if(m_useRenderPass)
		beginRenderPass();

	VkExtent2D	granularity = { 0u, 0u };
	vk.getRenderAreaGranularity(device, *m_renderPass, &granularity);
	TCU_CHECK(granularity.width >= 1 && granularity.height >= 1);
	TCU_CHECK(prePassGranularity.width == granularity.width && prePassGranularity.height == granularity.height);
	TCU_CHECK(granularity.width <= m_context.getDeviceProperties().limits.maxFramebufferWidth && granularity.height <= m_context.getDeviceProperties().limits.maxFramebufferHeight);

	if(m_useRenderPass)
		endRenderPass();

	log << tcu::TestLog::Message << "Horizontal granularity: " << granularity.width << " Vertical granularity: " << granularity.height << tcu::TestLog::EndMessage;
	return tcu::TestStatus::pass("Granularity test");
}

class GranularityCase : public vkt::TestCase
{
public:
										GranularityCase		(tcu::TestContext&					testCtx,
															 const std::string&					name,
															 const std::string&					description,
															 const std::vector<AttachmentInfo>&	attachments,
															 const bool							useRenderPass);
	virtual								~GranularityCase	(void);

	virtual TestInstance*				createInstance		(Context&	context) const;
private:
	const std::vector<AttachmentInfo>	m_attachments;
	const bool							m_useRenderPass;
};

GranularityCase::GranularityCase (tcu::TestContext&						testCtx,
								  const std::string&					name,
								  const std::string&					description,
								  const std::vector<AttachmentInfo>&	attachments,
								  const bool							useRenderPass = false)
	: vkt::TestCase		(testCtx, name, description)
	, m_attachments		(attachments)
	, m_useRenderPass	(useRenderPass)
{
}

GranularityCase::~GranularityCase (void)
{
}

TestInstance* GranularityCase::createInstance (Context& context) const
{
	return new GranularityInstance(context, m_attachments, m_useRenderPass);
}

} // anonymous

tcu::TestCaseGroup* createGranularityQueryTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "granularity", "Granularity query tests"));
	// Subgroups
	de::MovePtr<tcu::TestCaseGroup>	single			(new tcu::TestCaseGroup(testCtx, "single", "Single texture granularity tests."));
	de::MovePtr<tcu::TestCaseGroup>	multi			(new tcu::TestCaseGroup(testCtx, "multi", "Multiple textures with same format granularity tests."));
	de::MovePtr<tcu::TestCaseGroup>	random			(new tcu::TestCaseGroup(testCtx, "random", "Multiple textures with a guaranteed format occurence."));
	de::MovePtr<tcu::TestCaseGroup>	inRenderPass	(new tcu::TestCaseGroup(testCtx, "in_render_pass", "Single texture granularity tests, inside render pass"));

	de::Random	rnd(215);
	const char*	description	= "Granularity case.";

	const VkFormat mandatoryFormats[] =
	{
		VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_A1R5G5B5_UNORM_PACK16,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_A8B8G8R8_UNORM_PACK32,
		VK_FORMAT_A8B8G8R8_SNORM_PACK32,
		VK_FORMAT_A8B8G8R8_UINT_PACK32,
		VK_FORMAT_A8B8G8R8_SINT_PACK32,
		VK_FORMAT_A8B8G8R8_SRGB_PACK32,
		VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_UINT_PACK32,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_D32_SFLOAT,
	};

	const deUint32	maxDimension	= 500;
	const deUint32	minIteration	= 2;
	const deUint32	maxIteration	= 10;

	for (deUint32 formatIdx = 1; formatIdx <= VK_FORMAT_D32_SFLOAT_S8_UINT; ++formatIdx)
	{
		VkFormat	format		= VkFormat(formatIdx);
		std::string name		= de::toLower(getFormatName(format)).substr(10);

		{
			std::vector<AttachmentInfo>	attachments;
			attachments.push_back(AttachmentInfo(format, rnd.getInt(1, maxDimension), rnd.getInt(1, maxDimension), 1));
			single->addChild(new GranularityCase(testCtx, name.c_str(), description, attachments));
		}

		{
			std::vector<AttachmentInfo>	attachments;
			deUint32	iterations	= rnd.getInt(minIteration, maxIteration);
			for (deUint32 idx = 0; idx < iterations; ++idx)
				attachments.push_back(AttachmentInfo(VkFormat(formatIdx), rnd.getInt(1, maxDimension), rnd.getInt(1, maxDimension), 1));
			multi->addChild(new GranularityCase(testCtx, name.c_str(), description, attachments));
		}

		{
			std::vector<AttachmentInfo>	attachments;
			deUint32	iterations	= rnd.getInt(minIteration, maxIteration);
			attachments.push_back(AttachmentInfo(VkFormat(formatIdx), rnd.getInt(1, maxDimension), rnd.getInt(1, maxDimension), 1));
			for (deUint32 idx = 0; idx < iterations; ++idx)
				attachments.push_back(AttachmentInfo(mandatoryFormats[rnd.getInt(0, DE_LENGTH_OF_ARRAY(mandatoryFormats) - 1)], rnd.getInt(1, maxDimension), rnd.getInt(1, maxDimension), 1));
			random->addChild(new GranularityCase(testCtx, name.c_str(), description, attachments));
		}

		{
			std::vector<AttachmentInfo>	attachments;
			attachments.push_back(AttachmentInfo(format, rnd.getInt(1, maxDimension), rnd.getInt(1, maxDimension), 1));
			inRenderPass->addChild(new GranularityCase(testCtx, name.c_str(), description, attachments, true));
		}
	}

	group->addChild(single.release());
	group->addChild(multi.release());
	group->addChild(random.release());
	group->addChild(inRenderPass.release());

	return group.release();
}

} // api
} // vkt
