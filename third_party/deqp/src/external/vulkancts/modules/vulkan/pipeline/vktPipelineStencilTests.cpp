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
 * \brief Stencil Tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineStencilTests.hpp"
#include "vktPipelineClearUtil.hpp"
#include "vktPipelineImageUtil.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "vktPipelineReferenceRenderer.hpp"
#include "vktPipelineUniqueRandomIterator.hpp"
#include "vktTestCase.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "tcuImageCompare.hpp"
#include "deMemory.h"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

namespace vkt
{
namespace pipeline
{

using namespace vk;

namespace
{

bool isSupportedDepthStencilFormat (const InstanceInterface& instanceInterface, VkPhysicalDevice device, VkFormat format)
{
	VkFormatProperties formatProps;

	instanceInterface.getPhysicalDeviceFormatProperties(device, format, &formatProps);

	return (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
}

class StencilOpStateUniqueRandomIterator : public UniqueRandomIterator<VkStencilOpState>
{
public:
								StencilOpStateUniqueRandomIterator	(int seed);
	virtual						~StencilOpStateUniqueRandomIterator	(void) {}
	virtual VkStencilOpState	getIndexedValue						(deUint32 index);

private:

	// Pre-calculated constants
	const static deUint32		s_stencilOpsLength;
	const static deUint32		s_stencilOpsLength2;
	const static deUint32		s_stencilOpsLength3;
	const static deUint32		s_compareOpsLength;

	// Total number of cross-combinations of (stencilFailOp x stencilPassOp x stencilDepthFailOp x stencilCompareOp)
	const static deUint32		s_totalStencilOpStates;
};


class StencilTest : public vkt::TestCase
{
public:
	enum
	{
		QUAD_COUNT = 4
	};

	struct StencilStateConfig
	{
		deUint32	frontReadMask;
		deUint32	frontWriteMask;
		deUint32	frontRef;

		deUint32	backReadMask;
		deUint32	backWriteMask;
		deUint32	backRef;
	};

	const static StencilStateConfig			s_stencilStateConfigs[QUAD_COUNT];
	const static float						s_quadDepths[QUAD_COUNT];


											StencilTest				(tcu::TestContext&			testContext,
																	 const std::string&			name,
																	 const std::string&			description,
																	 VkFormat					stencilFormat,
																	 const VkStencilOpState&	stencilOpStateFront,
																	 const VkStencilOpState&	stencilOpStateBack);
	virtual									~StencilTest			(void);
	virtual void							initPrograms			(SourceCollections& sourceCollections) const;
	virtual TestInstance*					createInstance			(Context& context) const;

private:
	VkFormat								m_stencilFormat;
	const VkStencilOpState					m_stencilOpStateFront;
	const VkStencilOpState					m_stencilOpStateBack;
};

class StencilTestInstance : public vkt::TestInstance
{
public:
										StencilTestInstance		(Context&					context,
																 VkFormat					stencilFormat,
																 const VkStencilOpState&	stencilOpStatesFront,
																 const VkStencilOpState&	stencilOpStatesBack);
	virtual								~StencilTestInstance	(void);
	virtual tcu::TestStatus				iterate					(void);

private:
	tcu::TestStatus						verifyImage				(void);

	VkStencilOpState					m_stencilOpStateFront;
	VkStencilOpState					m_stencilOpStateBack;
	const tcu::UVec2					m_renderSize;
	const VkFormat						m_colorFormat;
	const VkFormat						m_stencilFormat;
	VkImageSubresourceRange				m_stencilImageSubresourceRange;

	VkImageCreateInfo					m_colorImageCreateInfo;
	Move<VkImage>						m_colorImage;
	de::MovePtr<Allocation>				m_colorImageAlloc;
	Move<VkImage>						m_stencilImage;
	de::MovePtr<Allocation>				m_stencilImageAlloc;
	Move<VkImageView>					m_colorAttachmentView;
	Move<VkImageView>					m_stencilAttachmentView;
	Move<VkRenderPass>					m_renderPass;
	Move<VkFramebuffer>					m_framebuffer;

	Move<VkShaderModule>				m_vertexShaderModule;
	Move<VkShaderModule>				m_fragmentShaderModule;

	Move<VkBuffer>						m_vertexBuffer;
	std::vector<Vertex4RGBA>			m_vertices;
	de::MovePtr<Allocation>				m_vertexBufferAlloc;

	Move<VkPipelineLayout>				m_pipelineLayout;
	Move<VkPipeline>					m_graphicsPipelines[StencilTest::QUAD_COUNT];

	Move<VkCommandPool>					m_cmdPool;
	Move<VkCommandBuffer>				m_cmdBuffer;

	Move<VkFence>						m_fence;
};

const VkStencilOp stencilOps[] =
{
	VK_STENCIL_OP_KEEP,
	VK_STENCIL_OP_ZERO,
	VK_STENCIL_OP_REPLACE,
	VK_STENCIL_OP_INCREMENT_AND_CLAMP,
	VK_STENCIL_OP_DECREMENT_AND_CLAMP,
	VK_STENCIL_OP_INVERT,
	VK_STENCIL_OP_INCREMENT_AND_WRAP,
	VK_STENCIL_OP_DECREMENT_AND_WRAP
};

const VkCompareOp compareOps[] =
{
	VK_COMPARE_OP_NEVER,
	VK_COMPARE_OP_LESS,
	VK_COMPARE_OP_EQUAL,
	VK_COMPARE_OP_LESS_OR_EQUAL,
	VK_COMPARE_OP_GREATER,
	VK_COMPARE_OP_NOT_EQUAL,
	VK_COMPARE_OP_GREATER_OR_EQUAL,
	VK_COMPARE_OP_ALWAYS
};

// StencilOpStateUniqueRandomIterator

const deUint32 StencilOpStateUniqueRandomIterator::s_stencilOpsLength		= DE_LENGTH_OF_ARRAY(stencilOps);
const deUint32 StencilOpStateUniqueRandomIterator::s_stencilOpsLength2		= s_stencilOpsLength * s_stencilOpsLength;
const deUint32 StencilOpStateUniqueRandomIterator::s_stencilOpsLength3		= s_stencilOpsLength2 * s_stencilOpsLength;
const deUint32 StencilOpStateUniqueRandomIterator::s_compareOpsLength		= DE_LENGTH_OF_ARRAY(compareOps);
const deUint32 StencilOpStateUniqueRandomIterator::s_totalStencilOpStates	= s_stencilOpsLength3 * s_compareOpsLength;

StencilOpStateUniqueRandomIterator::StencilOpStateUniqueRandomIterator (int seed)
	: UniqueRandomIterator<VkStencilOpState>(s_totalStencilOpStates, s_totalStencilOpStates, seed)
{
}

VkStencilOpState StencilOpStateUniqueRandomIterator::getIndexedValue (deUint32 index)
{
	const deUint32 stencilCompareOpIndex = index / s_stencilOpsLength3;
	const deUint32 stencilCompareOpSeqIndex = stencilCompareOpIndex * s_stencilOpsLength3;

	const deUint32 stencilDepthFailOpIndex = (index - stencilCompareOpSeqIndex) / s_stencilOpsLength2;
	const deUint32 stencilDepthFailOpSeqIndex = stencilDepthFailOpIndex * s_stencilOpsLength2;

	const deUint32 stencilPassOpIndex = (index - stencilCompareOpSeqIndex - stencilDepthFailOpSeqIndex) / s_stencilOpsLength;
	const deUint32 stencilPassOpSeqIndex = stencilPassOpIndex * s_stencilOpsLength;

	const deUint32 stencilFailOpIndex = index - stencilCompareOpSeqIndex - stencilDepthFailOpSeqIndex - stencilPassOpSeqIndex;

	const VkStencilOpState stencilOpState =
	{
		stencilOps[stencilFailOpIndex],			// VkStencilOp	failOp;
		stencilOps[stencilPassOpIndex],			// VkStencilOp	passOp;
		stencilOps[stencilDepthFailOpIndex],	// VkStencilOp	depthFailOp;
		compareOps[stencilCompareOpIndex],		// VkCompareOp	compareOp;
		0x0,									// deUint32		compareMask;
		0x0,									// deUint32		writeMask;
		0x0										// deUint32		reference;
	};

	return stencilOpState;
}


// StencilTest

const StencilTest::StencilStateConfig StencilTest::s_stencilStateConfigs[QUAD_COUNT] =
{
	//	frontReadMask	frontWriteMask		frontRef		backReadMask	backWriteMask	backRef
	{	0xFF,			0xFF,				0xAB,			0xF0,			0xFF,			0xFF	},
	{	0xFF,			0xF0,				0xCD,			0xF0,			0xF0,			0xEF	},
	{	0xF0,			0x0F,				0xEF,			0xFF,			0x0F,			0xCD	},
	{	0xF0,			0x01,				0xFF,			0xFF,			0x01,			0xAB	}
};

const float StencilTest::s_quadDepths[QUAD_COUNT] =
{
	0.1f,
	0.0f,
	0.3f,
	0.2f
};

StencilTest::StencilTest (tcu::TestContext&			testContext,
						  const std::string&		name,
						  const std::string&		description,
						  VkFormat					stencilFormat,
						  const VkStencilOpState&	stencilOpStateFront,
						  const VkStencilOpState&	stencilOpStateBack)
	: vkt::TestCase			(testContext, name, description)
	, m_stencilFormat		(stencilFormat)
	, m_stencilOpStateFront	(stencilOpStateFront)
	, m_stencilOpStateBack	(stencilOpStateBack)
{
}

StencilTest::~StencilTest (void)
{
}

TestInstance* StencilTest::createInstance (Context& context) const
{
	return new StencilTestInstance(context, m_stencilFormat, m_stencilOpStateFront, m_stencilOpStateBack);
}

void StencilTest::initPrograms (SourceCollections& sourceCollections) const
{
	sourceCollections.glslSources.add("color_vert") << glu::VertexSource(
		"#version 310 es\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec4 color;\n"
		"layout(location = 0) out highp vec4 vtxColor;\n"
		"void main (void)\n"
		"{\n"
		"	gl_Position = position;\n"
		"	vtxColor = color;\n"
		"}\n");

	sourceCollections.glslSources.add("color_frag") << glu::FragmentSource(
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 vtxColor;\n"
		"layout(location = 0) out highp vec4 fragColor;\n"
		"void main (void)\n"
		"{\n"
		"	fragColor = vtxColor;\n"
		"}\n");
}


// StencilTestInstance

StencilTestInstance::StencilTestInstance (Context&					context,
										  VkFormat					stencilFormat,
										  const VkStencilOpState&	stencilOpStateFront,
										  const VkStencilOpState&	stencilOpStateBack)
	: vkt::TestInstance		(context)
	, m_stencilOpStateFront	(stencilOpStateFront)
	, m_stencilOpStateBack	(stencilOpStateBack)
	, m_renderSize			(32, 32)
	, m_colorFormat			(VK_FORMAT_R8G8B8A8_UNORM)
	, m_stencilFormat		(stencilFormat)
{
	const DeviceInterface&		vk						= context.getDeviceInterface();
	const VkDevice				vkDevice				= context.getDevice();
	const deUint32				queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	SimpleAllocator				memAlloc				(vk, vkDevice, getPhysicalDeviceMemoryProperties(context.getInstanceInterface(), context.getPhysicalDevice()));
	const VkComponentMapping	componentMappingRGBA	= { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	// Create color image
	{
		const VkImageCreateInfo colorImageParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,										// VkStructureType			sType;
			DE_NULL,																	// const void*				pNext;
			0u,																			// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,															// VkImageType				imageType;
			m_colorFormat,																// VkFormat					format;
			{ m_renderSize.x(), m_renderSize.y(), 1u },									// VkExtent3D				extent;
			1u,																			// deUint32					mipLevels;
			1u,																			// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,														// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,													// VkImageTiling			tiling;
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,		// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,													// VkSharingMode			sharingMode;
			1u,																			// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,															// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED													// VkImageLayout			initialLayout;
		};

		m_colorImageCreateInfo	= colorImageParams;
		m_colorImage			= createImage(vk, vkDevice, &m_colorImageCreateInfo);

		// Allocate and bind color image memory
		m_colorImageAlloc		= memAlloc.allocate(getImageMemoryRequirements(vk, vkDevice, *m_colorImage), MemoryRequirement::Any);
		VK_CHECK(vk.bindImageMemory(vkDevice, *m_colorImage, m_colorImageAlloc->getMemory(), m_colorImageAlloc->getOffset()));
	}

	// Create stencil image
	{
		// Check format support
		if (!isSupportedDepthStencilFormat(context.getInstanceInterface(), context.getPhysicalDevice(), m_stencilFormat))
			throw tcu::NotSupportedError(std::string("Unsupported depth/stencil format: ") + getFormatName(m_stencilFormat));

		const VkImageCreateInfo stencilImageParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,			// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			0u,												// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,								// VkImageType				imageType;
			m_stencilFormat,								// VkFormat					format;
			{ m_renderSize.x(), m_renderSize.y(), 1u },		// VkExtent3D				extent;
			1u,												// deUint32					mipLevels;
			1u,												// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,							// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,						// VkImageTiling			tiling;
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,	// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,						// VkSharingMode			sharingMode;
			1u,												// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,								// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED						// VkImageLayout			initialLayout;
		};

		m_stencilImage = createImage(vk, vkDevice, &stencilImageParams);

		// Allocate and bind stencil image memory
		m_stencilImageAlloc = memAlloc.allocate(getImageMemoryRequirements(vk, vkDevice, *m_stencilImage), MemoryRequirement::Any);
		VK_CHECK(vk.bindImageMemory(vkDevice, *m_stencilImage, m_stencilImageAlloc->getMemory(), m_stencilImageAlloc->getOffset()));

		const VkImageAspectFlags aspect = (mapVkFormat(m_stencilFormat).order == tcu::TextureFormat::DS ? VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT
																										: VK_IMAGE_ASPECT_STENCIL_BIT);
		m_stencilImageSubresourceRange  = makeImageSubresourceRange(aspect, 0u, stencilImageParams.mipLevels, 0u, stencilImageParams.arrayLayers);
	}

	// Create color attachment view
	{
		const VkImageViewCreateInfo colorAttachmentViewParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,			// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkImageViewCreateFlags	flags;
			*m_colorImage,										// VkImage					image;
			VK_IMAGE_VIEW_TYPE_2D,								// VkImageViewType			viewType;
			m_colorFormat,										// VkFormat					format;
			componentMappingRGBA,								// VkComponentMapping		components;
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }		// VkImageSubresourceRange	subresourceRange;
		};

		m_colorAttachmentView = createImageView(vk, vkDevice, &colorAttachmentViewParams);
	}

	// Create stencil attachment view
	{
		const VkImageViewCreateInfo stencilAttachmentViewParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,			// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkImageViewCreateFlags	flags;
			*m_stencilImage,									// VkImage					image;
			VK_IMAGE_VIEW_TYPE_2D,								// VkImageViewType			viewType;
			m_stencilFormat,									// VkFormat					format;
			componentMappingRGBA,								// VkComponentMapping		components;
			m_stencilImageSubresourceRange,						// VkImageSubresourceRange	subresourceRange;
		};

		m_stencilAttachmentView = createImageView(vk, vkDevice, &stencilAttachmentViewParams);
	}

	// Create render pass
	{
		const VkAttachmentDescription colorAttachmentDescription =
		{
			0u,													// VkAttachmentDescriptionFlags	flags;
			m_colorFormat,										// VkFormat						format;
			VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits		samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp			loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp			storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp			stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp			stencilStoreOp;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout				initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout				finalLayout;
		};

		const VkAttachmentDescription stencilAttachmentDescription =
		{
			0u,													// VkAttachmentDescriptionFlags	flags;
			m_stencilFormat,									// VkFormat						format;
			VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits		samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp			loadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp			storeOp;
			VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp			stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp			stencilStoreOp;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	// VkImageLayout				initialLayout;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	// VkImageLayout				finalLayout;
		};

		const VkAttachmentDescription attachments[2] =
		{
			colorAttachmentDescription,
			stencilAttachmentDescription
		};

		const VkAttachmentReference colorAttachmentReference =
		{
			0u,													// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
		};

		const VkAttachmentReference stencilAttachmentReference =
		{
			1u,													// deUint32			attachment;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
		};

		const VkSubpassDescription subpassDescription =
		{
			0u,													// VkSubpassDescriptionFlags	flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint			pipelineBindPoint;
			0u,													// deUint32						inputAttachmentCount;
			DE_NULL,											// const VkAttachmentReference*	pInputAttachments;
			1u,													// deUint32						colorAttachmentCount;
			&colorAttachmentReference,							// const VkAttachmentReference*	pColorAttachments;
			DE_NULL,											// const VkAttachmentReference*	pResolveAttachments;
			&stencilAttachmentReference,						// const VkAttachmentReference*	pDepthStencilAttachment;
			0u,													// deUint32						preserveAttachmentCount;
			DE_NULL												// const VkAttachmentReference*	pPreserveAttachments;
		};

		const VkRenderPassCreateInfo renderPassParams =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkRenderPassCreateFlags			flags;
			2u,													// deUint32							attachmentCount;
			attachments,										// const VkAttachmentDescription*	pAttachments;
			1u,													// deUint32							subpassCount;
			&subpassDescription,								// const VkSubpassDescription*		pSubpasses;
			0u,													// deUint32							dependencyCount;
			DE_NULL												// const VkSubpassDependency*		pDependencies;
		};

		m_renderPass = createRenderPass(vk, vkDevice, &renderPassParams);
	}

	// Create framebuffer
	{
		const VkImageView attachmentBindInfos[2] = { *m_colorAttachmentView, *m_stencilAttachmentView };

		const VkFramebufferCreateInfo framebufferParams =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,			// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkFramebufferCreateFlags	flags;
			*m_renderPass,										// VkRenderPass				renderPass;
			2u,													// deUint32					attachmentCount;
			attachmentBindInfos,								// const VkImageView*		pAttachments;
			(deUint32)m_renderSize.x(),							// deUint32					width;
			(deUint32)m_renderSize.y(),							// deUint32					height;
			1u													// deUint32					layers;
		};

		m_framebuffer = createFramebuffer(vk, vkDevice, &framebufferParams);
	}

	// Create pipeline layout
	{
		const VkPipelineLayoutCreateInfo pipelineLayoutParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType				sType;
			DE_NULL,											// const void*					pNext;
			0u,													// VkPipelineLayoutCreateFlags	flags;
			0u,													// deUint32						setLayoutCount;
			DE_NULL,											// const VkDescriptorSetLayout*	pSetLayouts;
			0u,													// deUint32						pushConstantRangeCount;
			DE_NULL												// const VkPushConstantRange*	pPushConstantRanges;
		};

		m_pipelineLayout = createPipelineLayout(vk, vkDevice, &pipelineLayoutParams);
	}

	m_vertexShaderModule	= createShaderModule(vk, vkDevice, m_context.getBinaryCollection().get("color_vert"), 0);
	m_fragmentShaderModule	= createShaderModule(vk, vkDevice, m_context.getBinaryCollection().get("color_frag"), 0);

	// Create pipeline
	{
		const VkPipelineShaderStageCreateInfo shaderStages[2] =
		{
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				0u,															// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStageFlagBits				stage;
				*m_vertexShaderModule,										// VkShaderModule						module;
				"main",														// const char*							pName;
				DE_NULL														// const VkSpecializationInfo*			pSpecializationInfo;
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				0u,															// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlagBits				stage;
				*m_fragmentShaderModule,									// VkShaderModule						module;
				"main",														// const char*							pName;
				DE_NULL														// const VkSpecializationInfo*			pSpecializationInfo;
			}
		};

		const VkVertexInputBindingDescription vertexInputBindingDescription =
		{
			0u,										// deUint32					binding;
			sizeof(Vertex4RGBA),					// deUint32					strideInBytes;
			VK_VERTEX_INPUT_RATE_VERTEX				// VkVertexInputStepRate	inputRate;
		};

		const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] =
		{
			{
				0u,									// deUint32	location;
				0u,									// deUint32	binding;
				VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat	format;
				0u									// deUint32	offsetInBytes;
			},
			{
				1u,									// deUint32	location;
				0u,									// deUint32	binding;
				VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat	format;
				DE_OFFSET_OF(Vertex4RGBA, color),	// deUint32	offsetInBytes;
			}
		};

		const VkPipelineVertexInputStateCreateInfo vertexInputStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineVertexInputStateCreateFlags	flags;
			1u,																// deUint32									vertexBindingDescriptionCount;
			&vertexInputBindingDescription,									// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
			2u,																// deUint32									vertexAttributeDescriptionCount;
			vertexInputAttributeDescriptions								// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
		};

		const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineInputAssemblyStateCreateFlags	flags;
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,							// VkPrimitiveTopology						topology;
			false															// VkBool32									primitiveRestartEnable;
		};

		const VkViewport viewport =
		{
			0.0f,						// float	x;
			0.0f,						// float	y;
			(float)m_renderSize.x(),	// float	width;
			(float)m_renderSize.y(),	// float	height;
			0.0f,						// float	minDepth;
			1.0f						// float	maxDepth;
		};

		const VkRect2D scissor = { { 0, 0 }, { m_renderSize.x(), m_renderSize.y() } };

		const VkPipelineViewportStateCreateInfo viewportStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType						sType;
			DE_NULL,														// const void*							pNext;
			0u,																// VkPipelineViewportStateCreateFlags	flags;
			1u,																// deUint32								viewportCount;
			&viewport,														// const VkViewport*					pViewports;
			1u,																// deUint32								scissorCount;
			&scissor,														// const VkRect2D*						pScissors;
		};

		const VkPipelineRasterizationStateCreateInfo rasterStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineRasterizationStateCreateFlags	flags;
			false,															// VkBool32									depthClampEnable;
			false,															// VkBool32									rasterizerDiscardEnable;
			VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
			VK_CULL_MODE_NONE,												// VkCullModeFlags							cullMode;
			VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
			false,															// VkBool32									depthBiasEnable;
			0.0f,															// float									depthBiasConstantFactor;
			0.0f,															// float									depthBiasClamp;
			0.0f,															// float									depthBiasSlopeFactor;
			1.0f															// float									lineWidth;
		};

		const VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
		{
			false,															// VkBool32					blendEnable;
			VK_BLEND_FACTOR_ONE,											// VkBlendFactor			srcColorBlendFactor;
			VK_BLEND_FACTOR_ZERO,											// VkBlendFactor			dstColorBlendFactor;
			VK_BLEND_OP_ADD,												// VkBlendOp				colorBlendOp;
			VK_BLEND_FACTOR_ONE,											// VKBLENDFACTOR			SRCALPHABLENDFACTOR;
			VK_BLEND_FACTOR_ZERO,											// VkBlendFactor			dstAlphaBlendFactor;
			VK_BLEND_OP_ADD,												// VkBlendOp				alphaBlendOp;
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |			// VkColorComponentFlags	colorWriteMask;
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		const VkPipelineColorBlendStateCreateInfo colorBlendStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
			DE_NULL,													// const void*									pNext;
			0u,															// VkPipelineColorBlendStateCreateFlags			flags;
			false,														// VkBool32										logicOpEnable;
			VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
			1u,															// deUint32										attachmentCount;
			&colorBlendAttachmentState,									// const VkPipelineColorBlendAttachmentState*	pAttachments;
			{ 0.0f, 0.0f, 0.0f, 0.0f }									// float										blendConstants[4];
		};

		const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			0u,															// VkPipelineMultisampleStateCreateFlags	flags;
			VK_SAMPLE_COUNT_1_BIT,										// VkSampleCountFlagBits					rasterizationSamples;
			false,														// VkBool32									sampleShadingEnable;
			0.0f,														// float									minSampleShading;
			DE_NULL,													// const VkSampleMask*						pSampleMask;
			false,														// VkBool32									alphaToCoverageEnable;
			false														// VkBool32									alphaToOneEnable;
		};

		const bool isDepthEnabled = (vk::mapVkFormat(m_stencilFormat).order != tcu::TextureFormat::S);

		VkPipelineDepthStencilStateCreateInfo depthStencilStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			0u,															// VkPipelineDepthStencilStateCreateFlags	flags;
			isDepthEnabled,												// VkBool32									depthTestEnable;
			isDepthEnabled,												// VkBool32									depthWriteEnable;
			VK_COMPARE_OP_LESS,											// VkCompareOp								depthCompareOp;
			false,														// VkBool32									depthBoundsTestEnable;
			true,														// VkBool32									stencilTestEnable;
			m_stencilOpStateFront,										// VkStencilOpState							front;
			m_stencilOpStateBack,										// VkStencilOpState							back;
			0.0f,														// float									minDepthBounds;
			1.0f														// float									maxDepthBounds;
		};

		const VkGraphicsPipelineCreateInfo graphicsPipelineParams =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
			DE_NULL,											// const void*										pNext;
			0u,													// VkPipelineCreateFlags							flags;
			2u,													// deUint32											stageCount;
			shaderStages,										// const VkPipelineShaderStageCreateInfo*			pStages;
			&vertexInputStateParams,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
			&inputAssemblyStateParams,							// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
			DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
			&viewportStateParams,								// const VkPipelineViewportStateCreateInfo*			pViewportState;
			&rasterStateParams,									// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
			&multisampleStateParams,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
			&depthStencilStateParams,							// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
			&colorBlendStateParams,								// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
			(const VkPipelineDynamicStateCreateInfo*)DE_NULL,	// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
			*m_pipelineLayout,									// VkPipelineLayout									layout;
			*m_renderPass,										// VkRenderPass										renderPass;
			0u,													// deUint32											subpass;
			0u,													// VkPipeline										basePipelineHandle;
			0u													// deInt32											basePipelineIndex;
		};

		// Setup different stencil masks and refs in each quad
		for (int quadNdx = 0; quadNdx < StencilTest::QUAD_COUNT; quadNdx++)
		{
			const StencilTest::StencilStateConfig&	config	= StencilTest::s_stencilStateConfigs[quadNdx];
			VkStencilOpState&						front	= depthStencilStateParams.front;
			VkStencilOpState&						back	= depthStencilStateParams.back;

			front.compareMask	= config.frontReadMask;
			front.writeMask		= config.frontWriteMask;
			front.reference		= config.frontRef;

			back.compareMask	= config.backReadMask;
			back.writeMask		= config.backWriteMask;
			back.reference		= config.backRef;

			m_graphicsPipelines[quadNdx] = createGraphicsPipeline(vk, vkDevice, DE_NULL, &graphicsPipelineParams);
		}
	}


	// Create vertex buffer
	{
		const VkBufferCreateInfo vertexBufferParams =
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			1024u,										// VkDeviceSize			size;
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_vertices			= createOverlappingQuads();
		m_vertexBuffer		= createBuffer(vk, vkDevice, &vertexBufferParams);
		m_vertexBufferAlloc	= memAlloc.allocate(getBufferMemoryRequirements(vk, vkDevice, *m_vertexBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vk.bindBufferMemory(vkDevice, *m_vertexBuffer, m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset()));

		// Adjust depths
		for (int quadNdx = 0; quadNdx < 4; quadNdx++)
			for (int vertexNdx = 0; vertexNdx < 6; vertexNdx++)
				m_vertices[quadNdx * 6 + vertexNdx].position.z() = StencilTest::s_quadDepths[quadNdx];

		// Load vertices into vertex buffer
		deMemcpy(m_vertexBufferAlloc->getHostPtr(), m_vertices.data(), m_vertices.size() * sizeof(Vertex4RGBA));

		const VkMappedMemoryRange flushRange =
		{
				VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,	// VkStructureType	sType;
				DE_NULL,								// const void*		pNext;
				m_vertexBufferAlloc->getMemory(),		// VkDeviceMemory	memory;
				m_vertexBufferAlloc->getOffset(),		// VkDeviceSize		offset;
				vertexBufferParams.size					// VkDeviceSize		size;
		};

		vk.flushMappedMemoryRanges(vkDevice, 1, &flushRange);
	}

	// Create command pool
	m_cmdPool = createCommandPool(vk, vkDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndex);

	// Create command buffer
	{
		const VkCommandBufferBeginInfo cmdBufferBeginInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType					sType;
			DE_NULL,										// const void*						pNext;
			0u,												// VkCommandBufferUsageFlags		flags;
			(const VkCommandBufferInheritanceInfo*)DE_NULL,
		};

		const VkClearValue attachmentClearValues[2] =
		{
			defaultClearValue(m_colorFormat),
			defaultClearValue(m_stencilFormat)
		};

		const VkRenderPassBeginInfo renderPassBeginInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,				// VkStructureType		sType;
			DE_NULL,												// const void*			pNext;
			*m_renderPass,											// VkRenderPass			renderPass;
			*m_framebuffer,											// VkFramebuffer		framebuffer;
			{ { 0, 0 } , { m_renderSize.x(), m_renderSize.y() } },	// VkRect2D				renderArea;
			2,														// deUint32				clearValueCount;
			attachmentClearValues									// const VkClearValue*	pClearValues;
		};

		const VkImageMemoryBarrier imageLayoutBarriers[] =
		{
			// color image layout transition
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,									// VkStructureType            sType;
				DE_NULL,																// const void*                pNext;
				(VkAccessFlags)0,														// VkAccessFlags              srcAccessMask;
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,									// VkAccessFlags              dstAccessMask;
				VK_IMAGE_LAYOUT_UNDEFINED,												// VkImageLayout              oldLayout;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,								// VkImageLayout              newLayout;
				VK_QUEUE_FAMILY_IGNORED,												// uint32_t                   srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,												// uint32_t                   dstQueueFamilyIndex;
				*m_colorImage,															// VkImage                    image;
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }							// VkImageSubresourceRange    subresourceRange;
			},
			// stencil image layout transition
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,									// VkStructureType            sType;
				DE_NULL,																// const void*                pNext;
				(VkAccessFlags)0,														// VkAccessFlags              srcAccessMask;
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,							// VkAccessFlags              dstAccessMask;
				VK_IMAGE_LAYOUT_UNDEFINED,												// VkImageLayout              oldLayout;
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,						// VkImageLayout              newLayout;
				VK_QUEUE_FAMILY_IGNORED,												// uint32_t                   srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,												// uint32_t                   dstQueueFamilyIndex;
				*m_stencilImage,														// VkImage                    image;
				m_stencilImageSubresourceRange,											// VkImageSubresourceRange    subresourceRange;
			},
		};

		m_cmdBuffer = allocateCommandBuffer(vk, vkDevice, *m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));

		vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0,
			0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(imageLayoutBarriers), imageLayoutBarriers);

		vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		const VkDeviceSize		quadOffset		= (m_vertices.size() / StencilTest::QUAD_COUNT) * sizeof(Vertex4RGBA);

		for (int quadNdx = 0; quadNdx < StencilTest::QUAD_COUNT; quadNdx++)
		{
			VkDeviceSize vertexBufferOffset = quadOffset * quadNdx;

			vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_graphicsPipelines[quadNdx]);
			vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &m_vertexBuffer.get(), &vertexBufferOffset);
			vk.cmdDraw(*m_cmdBuffer, (deUint32)(m_vertices.size() / StencilTest::QUAD_COUNT), 1, 0, 0);
		}

		vk.cmdEndRenderPass(*m_cmdBuffer);
		VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
	}

	// Create fence
	m_fence = createFence(vk, vkDevice);
}

StencilTestInstance::~StencilTestInstance (void)
{
}

tcu::TestStatus StencilTestInstance::iterate (void)
{
	const DeviceInterface&		vk			= m_context.getDeviceInterface();
	const VkDevice				vkDevice	= m_context.getDevice();
	const VkQueue				queue		= m_context.getUniversalQueue();
	const VkSubmitInfo			submitInfo	=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
		DE_NULL,						// const void*				pNext;
		0u,								// deUint32					waitSemaphoreCount;
		DE_NULL,						// const VkSemaphore*		pWaitSemaphores;
		(const VkPipelineStageFlags*)DE_NULL,
		1u,								// deUint32					commandBufferCount;
		&m_cmdBuffer.get(),				// const VkCommandBuffer*	pCommandBuffers;
		0u,								// deUint32					signalSemaphoreCount;
		DE_NULL							// const VkSemaphore*		pSignalSemaphores;
	};

	VK_CHECK(vk.resetFences(vkDevice, 1, &m_fence.get()));
	VK_CHECK(vk.queueSubmit(queue, 1, &submitInfo, *m_fence));
	VK_CHECK(vk.waitForFences(vkDevice, 1, &m_fence.get(), true, ~(0ull) /* infinity*/));

	return verifyImage();
}

tcu::TestStatus StencilTestInstance::verifyImage (void)
{
	const tcu::TextureFormat	tcuColorFormat		= mapVkFormat(m_colorFormat);
	const tcu::TextureFormat	tcuStencilFormat	= mapVkFormat(m_stencilFormat);
	const ColorVertexShader		vertexShader;
	const ColorFragmentShader	fragmentShader		(tcuColorFormat, tcuStencilFormat);
	const rr::Program			program				(&vertexShader, &fragmentShader);
	ReferenceRenderer			refRenderer			(m_renderSize.x(), m_renderSize.y(), 1, tcuColorFormat, tcuStencilFormat, &program);
	bool						compareOk			= false;

	// Render reference image
	{
		// Set depth state
		rr::RenderState renderState(refRenderer.getViewportState());

		renderState.fragOps.depthTestEnabled	= true;
		renderState.fragOps.depthFunc			= mapVkCompareOp(VK_COMPARE_OP_LESS);
		renderState.fragOps.stencilTestEnabled	= true;

		rr::StencilState& refStencilFront	= renderState.fragOps.stencilStates[rr::FACETYPE_FRONT];
		rr::StencilState& refStencilBack	= renderState.fragOps.stencilStates[rr::FACETYPE_BACK];

		refStencilFront.sFail		= mapVkStencilOp(m_stencilOpStateFront.failOp);
		refStencilFront.dpFail		= mapVkStencilOp(m_stencilOpStateFront.depthFailOp);
		refStencilFront.dpPass		= mapVkStencilOp(m_stencilOpStateFront.passOp);
		refStencilFront.func		= mapVkCompareOp(m_stencilOpStateFront.compareOp);

		refStencilBack.sFail		= mapVkStencilOp(m_stencilOpStateBack.failOp);
		refStencilBack.dpPass		= mapVkStencilOp(m_stencilOpStateBack.passOp);
		refStencilBack.dpFail		= mapVkStencilOp(m_stencilOpStateBack.depthFailOp);
		refStencilBack.func			= mapVkCompareOp(m_stencilOpStateBack.compareOp);

		// Reverse winding of vertices, as Vulkan screen coordinates start at upper left
		std::vector<Vertex4RGBA> cwVertices(m_vertices);
		for (size_t vertexNdx = 0; vertexNdx < cwVertices.size() - 2; vertexNdx += 3)
		{
			const Vertex4RGBA cwVertex1	= cwVertices[vertexNdx + 1];

			cwVertices[vertexNdx + 1]	= cwVertices[vertexNdx + 2];
			cwVertices[vertexNdx + 2]	= cwVertex1;
		}

		for (int quadNdx = 0; quadNdx < StencilTest::QUAD_COUNT; quadNdx++)
		{
			refStencilFront.ref			= (int)StencilTest::s_stencilStateConfigs[quadNdx].frontRef;
			refStencilFront.compMask	= StencilTest::s_stencilStateConfigs[quadNdx].frontReadMask;
			refStencilFront.writeMask	= StencilTest::s_stencilStateConfigs[quadNdx].frontWriteMask;

			refStencilBack.ref			= (int)StencilTest::s_stencilStateConfigs[quadNdx].backRef;
			refStencilBack.compMask		= StencilTest::s_stencilStateConfigs[quadNdx].backReadMask;
			refStencilBack.writeMask	= StencilTest::s_stencilStateConfigs[quadNdx].backWriteMask;

			refRenderer.draw(renderState,
							 rr::PRIMITIVETYPE_TRIANGLES,
							 std::vector<Vertex4RGBA>(cwVertices.begin() + quadNdx * 6,
													  cwVertices.begin() + (quadNdx + 1) * 6));
		}
	}

	// Compare result with reference image
	{
		const DeviceInterface&				vk					= m_context.getDeviceInterface();
		const VkDevice						vkDevice			= m_context.getDevice();
		const VkQueue						queue				= m_context.getUniversalQueue();
		const deUint32						queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
		SimpleAllocator						allocator			(vk, vkDevice, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));
		de::UniquePtr<tcu::TextureLevel>	result				(readColorAttachment(vk, vkDevice, queue, queueFamilyIndex, allocator, *m_colorImage, m_colorFormat, m_renderSize).release());

		compareOk = tcu::intThresholdPositionDeviationCompare(m_context.getTestContext().getLog(),
															  "IntImageCompare",
															  "Image comparison",
															  refRenderer.getAccess(),
															  result->getAccess(),
															  tcu::UVec4(2, 2, 2, 2),
															  tcu::IVec3(1, 1, 0),
															  true,
															  tcu::COMPARE_LOG_RESULT);
	}

	if (compareOk)
		return tcu::TestStatus::pass("Result image matches reference");
	else
		return tcu::TestStatus::fail("Image mismatch");
}


// Utilities for test names

std::string getShortName (VkCompareOp compareOp)
{
	const std::string  fullName = getCompareOpName(compareOp);

	DE_ASSERT(de::beginsWith(fullName, "VK_COMPARE_OP_"));

	return de::toLower(fullName.substr(14));
}

const char* getShortName (VkStencilOp stencilOp)
{
	switch (stencilOp)
	{
		case VK_STENCIL_OP_KEEP:					return "keep";
		case VK_STENCIL_OP_ZERO:					return "zero";
		case VK_STENCIL_OP_REPLACE:					return "repl";
		case VK_STENCIL_OP_INCREMENT_AND_CLAMP:		return "incc";
		case VK_STENCIL_OP_DECREMENT_AND_CLAMP:		return "decc";
		case VK_STENCIL_OP_INVERT:					return "inv";
		case VK_STENCIL_OP_INCREMENT_AND_WRAP:		return "wrap";
		case VK_STENCIL_OP_DECREMENT_AND_WRAP:		return "decw";

		default:
			DE_FATAL("Invalid VkStencilOpState value");
	}
	return DE_NULL;
}

std::string getStencilStateSetDescription(const VkStencilOpState& stencilOpStateFront,
										  const VkStencilOpState& stencilOpStateBack)
{
	std::ostringstream desc;

	desc << "\nFront faces:\n" << stencilOpStateFront;
	desc << "Back faces:\n" << stencilOpStateBack;

	return desc.str();
}

std::string getFormatCaseName (VkFormat format)
{
	const std::string fullName = getFormatName(format);

	DE_ASSERT(de::beginsWith(fullName, "VK_FORMAT_"));

	return de::toLower(fullName.substr(10));
}

} // anonymous

tcu::TestCaseGroup* createStencilTests (tcu::TestContext& testCtx)
{
	const VkFormat stencilFormats[] =
	{
		VK_FORMAT_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT
	};

	de::MovePtr<tcu::TestCaseGroup>		stencilTests	(new tcu::TestCaseGroup(testCtx, "stencil", "Stencil tests"));
	de::MovePtr<tcu::TestCaseGroup>		formatTests		(new tcu::TestCaseGroup(testCtx, "format", "Uses different stencil formats"));
	StencilOpStateUniqueRandomIterator	stencilOpItr	(123);

	for (size_t formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(stencilFormats); formatNdx++)
	{
		const VkFormat					stencilFormat	= stencilFormats[formatNdx];
		de::MovePtr<tcu::TestCaseGroup>	formatTest		(new tcu::TestCaseGroup(testCtx,
																				getFormatCaseName(stencilFormat).c_str(),
																				(std::string("Uses format ") + getFormatName(stencilFormat)).c_str()));

		de::MovePtr<tcu::TestCaseGroup>	stencilStateTests;
		{
			std::ostringstream desc;
			desc << "Draws 4 quads with the following depths and dynamic stencil states: ";
			for (int quadNdx = 0; quadNdx < StencilTest::QUAD_COUNT; quadNdx++)
			{
				const StencilTest::StencilStateConfig& stencilConfig = StencilTest::s_stencilStateConfigs[quadNdx];

				desc << "(" << quadNdx << ") "
					 << "z = " << StencilTest::s_quadDepths[quadNdx] << ", "
					 << "frontReadMask = " << stencilConfig.frontReadMask << ", "
					 << "frontWriteMask = " << stencilConfig.frontWriteMask << ", "
					 << "frontRef = " << stencilConfig.frontRef << ", "
					 << "backReadMask = " << stencilConfig.backReadMask << ", "
					 << "backWriteMask = " << stencilConfig.backWriteMask << ", "
					 << "backRef = " << stencilConfig.backRef;
			}

			stencilStateTests = de::MovePtr<tcu::TestCaseGroup>(new tcu::TestCaseGroup(testCtx, "states", desc.str().c_str()));
		}

		stencilOpItr.reset();

		for (deUint32 failOpNdx = 0u; failOpNdx < DE_LENGTH_OF_ARRAY(stencilOps); failOpNdx++)
		{
			const std::string				failOpName	= std::string("fail_") + getShortName(stencilOps[failOpNdx]);
			de::MovePtr<tcu::TestCaseGroup>	failOpTest	(new tcu::TestCaseGroup(testCtx, failOpName.c_str(), ""));

			for (deUint32 passOpNdx = 0u; passOpNdx < DE_LENGTH_OF_ARRAY(stencilOps); passOpNdx++)
			{
				const std::string				passOpName	= std::string("pass_") + getShortName(stencilOps[passOpNdx]);
				de::MovePtr<tcu::TestCaseGroup>	passOpTest	(new tcu::TestCaseGroup(testCtx, passOpName.c_str(), ""));

				for (deUint32 dFailOpNdx = 0u; dFailOpNdx < DE_LENGTH_OF_ARRAY(stencilOps); dFailOpNdx++)
				{
					const std::string				dFailOpName	= std::string("dfail_") + getShortName(stencilOps[dFailOpNdx]);
					de::MovePtr<tcu::TestCaseGroup>	dFailOpTest	(new tcu::TestCaseGroup(testCtx, dFailOpName.c_str(), ""));

					for (deUint32 compareOpNdx = 0u; compareOpNdx < DE_LENGTH_OF_ARRAY(compareOps); compareOpNdx++)
					{
						// Iterate front set of stencil state in ascending order
						const VkStencilOpState	stencilStateFront	=
						{
							stencilOps[failOpNdx],		// failOp
							stencilOps[passOpNdx],		// passOp
							stencilOps[dFailOpNdx],		// depthFailOp
							compareOps[compareOpNdx],	// compareOp
							0x0,						// compareMask
							0x0,						// writeMask
							0x0							// reference
						};

						// Iterate back set of stencil state in random order
						const VkStencilOpState	stencilStateBack	= stencilOpItr.next();
						const std::string		caseName			= std::string("comp_") + getShortName(compareOps[compareOpNdx]);
						const std::string		caseDesc			= getStencilStateSetDescription(stencilStateFront, stencilStateBack);

						dFailOpTest->addChild(new StencilTest(testCtx, caseName, caseDesc, stencilFormat, stencilStateFront, stencilStateBack));
					}
					passOpTest->addChild(dFailOpTest.release());
				}
				failOpTest->addChild(passOpTest.release());
			}
			stencilStateTests->addChild(failOpTest.release());
		}

		formatTest->addChild(stencilStateTests.release());
		formatTests->addChild(formatTest.release());
	}
	stencilTests->addChild(formatTests.release());

	return stencilTests.release();
}

} // pipeline
} // vkt
