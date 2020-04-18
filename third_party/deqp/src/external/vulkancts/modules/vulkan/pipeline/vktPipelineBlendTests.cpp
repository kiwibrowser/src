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
 * \brief Blend Tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineBlendTests.hpp"
#include "vktPipelineClearUtil.hpp"
#include "vktPipelineImageUtil.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "vktPipelineUniqueRandomIterator.hpp"
#include "vktPipelineReferenceRenderer.hpp"
#include "vktTestCase.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuPlatform.hpp"
#include "tcuTextureUtil.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include <cstring>
#include <set>
#include <sstream>
#include <vector>

namespace vkt
{
namespace pipeline
{

using namespace vk;

namespace
{

bool isSupportedBlendFormat (const InstanceInterface& instanceInterface, VkPhysicalDevice device, VkFormat format)
{
	VkFormatProperties formatProps;

	instanceInterface.getPhysicalDeviceFormatProperties(device, format, &formatProps);

	return (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) &&
		   (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT);
}

class BlendStateUniqueRandomIterator : public UniqueRandomIterator<VkPipelineColorBlendAttachmentState>
{
public:
											BlendStateUniqueRandomIterator		(deUint32 numberOfCombinations, int seed);
	virtual									~BlendStateUniqueRandomIterator		(void) {}
	VkPipelineColorBlendAttachmentState		getIndexedValue	(deUint32 index);

private:
	const static VkBlendFactor				m_blendFactors[];
	const static VkBlendOp					m_blendOps[];

	// Pre-calculated constants
	const static deUint32					m_blendFactorsLength;
	const static deUint32					m_blendFactorsLength2;
	const static deUint32					m_blendFactorsLength3;
	const static deUint32					m_blendFactorsLength4;
	const static deUint32					m_blendOpsLength;

	// Total number of cross-combinations of (srcBlendColor x destBlendColor x blendOpColor x srcBlendAlpha x destBlendAlpha x blendOpAlpha)
	const static deUint32					m_totalBlendStates;
};

class BlendTest : public vkt::TestCase
{
public:
	enum
	{
		QUAD_COUNT = 4
	};

	const static VkColorComponentFlags	s_colorWriteMasks[QUAD_COUNT];
	const static tcu::Vec4				s_blendConst;

										BlendTest				(tcu::TestContext&							testContext,
																 const std::string&							name,
																 const std::string&							description,
																 const VkFormat								colorFormat,
																 const VkPipelineColorBlendAttachmentState	blendStates[QUAD_COUNT]);
	virtual								~BlendTest				(void);
	virtual void						initPrograms			(SourceCollections& sourceCollections) const;
	virtual TestInstance*				createInstance			(Context& context) const;

private:
	const VkFormat						m_colorFormat;
	VkPipelineColorBlendAttachmentState	m_blendStates[QUAD_COUNT];
};

class BlendTestInstance : public vkt::TestInstance
{
public:
										BlendTestInstance		(Context& context, const VkFormat colorFormat, const VkPipelineColorBlendAttachmentState blendStates[BlendTest::QUAD_COUNT]);
	virtual								~BlendTestInstance		(void);
	virtual tcu::TestStatus				iterate					(void);

private:
	static float						getNormChannelThreshold	(const tcu::TextureFormat& format, int numBits);
	static tcu::Vec4					getFormatThreshold		(const tcu::TextureFormat& format);
	tcu::TestStatus						verifyImage				(void);

	VkPipelineColorBlendAttachmentState	m_blendStates[BlendTest::QUAD_COUNT];

	const tcu::UVec2					m_renderSize;
	const VkFormat						m_colorFormat;

	VkImageCreateInfo					m_colorImageCreateInfo;
	Move<VkImage>						m_colorImage;
	de::MovePtr<Allocation>				m_colorImageAlloc;
	Move<VkImageView>					m_colorAttachmentView;
	Move<VkRenderPass>					m_renderPass;
	Move<VkFramebuffer>					m_framebuffer;

	Move<VkShaderModule>				m_vertexShaderModule;
	Move<VkShaderModule>				m_fragmentShaderModule;

	Move<VkBuffer>						m_vertexBuffer;
	std::vector<Vertex4RGBA>			m_vertices;
	de::MovePtr<Allocation>				m_vertexBufferAlloc;

	Move<VkPipelineLayout>				m_pipelineLayout;
	Move<VkPipeline>					m_graphicsPipelines[BlendTest::QUAD_COUNT];

	Move<VkCommandPool>					m_cmdPool;
	Move<VkCommandBuffer>				m_cmdBuffer;

	Move<VkFence>						m_fence;
};


// BlendStateUniqueRandomIterator

const VkBlendFactor BlendStateUniqueRandomIterator::m_blendFactors[] =
{
	VK_BLEND_FACTOR_ZERO,
	VK_BLEND_FACTOR_ONE,
	VK_BLEND_FACTOR_SRC_COLOR,
	VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	VK_BLEND_FACTOR_DST_COLOR,
	VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	VK_BLEND_FACTOR_SRC_ALPHA,
	VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	VK_BLEND_FACTOR_DST_ALPHA,
	VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
	VK_BLEND_FACTOR_CONSTANT_COLOR,
	VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
	VK_BLEND_FACTOR_CONSTANT_ALPHA,
	VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
	VK_BLEND_FACTOR_SRC_ALPHA_SATURATE
};

const VkBlendOp BlendStateUniqueRandomIterator::m_blendOps[] =
{
	VK_BLEND_OP_ADD,
	VK_BLEND_OP_SUBTRACT,
	VK_BLEND_OP_REVERSE_SUBTRACT,
	VK_BLEND_OP_MIN,
	VK_BLEND_OP_MAX
};

const deUint32 BlendStateUniqueRandomIterator::m_blendFactorsLength		= DE_LENGTH_OF_ARRAY(m_blendFactors);
const deUint32 BlendStateUniqueRandomIterator::m_blendFactorsLength2	= m_blendFactorsLength * m_blendFactorsLength;
const deUint32 BlendStateUniqueRandomIterator::m_blendFactorsLength3	= m_blendFactorsLength2 * m_blendFactorsLength;
const deUint32 BlendStateUniqueRandomIterator::m_blendFactorsLength4	= m_blendFactorsLength3 * m_blendFactorsLength;
const deUint32 BlendStateUniqueRandomIterator::m_blendOpsLength			= DE_LENGTH_OF_ARRAY(m_blendOps);
const deUint32 BlendStateUniqueRandomIterator::m_totalBlendStates		= m_blendFactorsLength4 * m_blendOpsLength * m_blendOpsLength;


BlendStateUniqueRandomIterator::BlendStateUniqueRandomIterator (deUint32 numberOfCombinations, int seed)
	: UniqueRandomIterator<VkPipelineColorBlendAttachmentState>(numberOfCombinations, m_totalBlendStates, seed)
{
}

VkPipelineColorBlendAttachmentState BlendStateUniqueRandomIterator::getIndexedValue (deUint32 index)
{
	const deUint32		blendOpAlphaIndex			= index / (m_blendFactorsLength4 * m_blendOpsLength);
	const deUint32		blendOpAlphaSeqIndex		= blendOpAlphaIndex * (m_blendFactorsLength4 * m_blendOpsLength);

	const deUint32		destBlendAlphaIndex			= (index - blendOpAlphaSeqIndex) / (m_blendFactorsLength3 * m_blendOpsLength);
	const deUint32		destBlendAlphaSeqIndex		= destBlendAlphaIndex * (m_blendFactorsLength3 * m_blendOpsLength);

	const deUint32		srcBlendAlphaIndex			= (index - blendOpAlphaSeqIndex - destBlendAlphaSeqIndex) / (m_blendFactorsLength2 * m_blendOpsLength);
	const deUint32		srcBlendAlphaSeqIndex		= srcBlendAlphaIndex * (m_blendFactorsLength2 * m_blendOpsLength);

	const deUint32		blendOpColorIndex			= (index - blendOpAlphaSeqIndex - destBlendAlphaSeqIndex - srcBlendAlphaSeqIndex) / m_blendFactorsLength2;
	const deUint32		blendOpColorSeqIndex		= blendOpColorIndex * m_blendFactorsLength2;

	const deUint32		destBlendColorIndex			= (index - blendOpAlphaSeqIndex - destBlendAlphaSeqIndex - srcBlendAlphaSeqIndex - blendOpColorSeqIndex) / m_blendFactorsLength;
	const deUint32		destBlendColorSeqIndex		= destBlendColorIndex * m_blendFactorsLength;

	const deUint32		srcBlendColorIndex			= index - blendOpAlphaSeqIndex - destBlendAlphaSeqIndex - srcBlendAlphaSeqIndex - blendOpColorSeqIndex - destBlendColorSeqIndex;

	const VkPipelineColorBlendAttachmentState blendAttachmentState =
	{
		true,														// VkBool32					blendEnable;
		m_blendFactors[srcBlendColorIndex],							// VkBlendFactor			srcColorBlendFactor;
		m_blendFactors[destBlendColorIndex],						// VkBlendFactor			dstColorBlendFactor;
		m_blendOps[blendOpColorIndex],								// VkBlendOp				colorBlendOp;
		m_blendFactors[srcBlendAlphaIndex],							// VkBlendFactor			srcAlphaBlendFactor;
		m_blendFactors[destBlendAlphaIndex],						// VkBlendFactor			dstAlphaBlendFactor;
		m_blendOps[blendOpAlphaIndex],								// VkBlendOp				alphaBlendOp;
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |		// VkColorComponentFlags	colorWriteMask;
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	return blendAttachmentState;
}


// BlendTest

const VkColorComponentFlags BlendTest::s_colorWriteMasks[BlendTest::QUAD_COUNT] = { VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT,	// Pair of channels: R & G
																					VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,	// Pair of channels: G & B
																					VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,	// Pair of channels: B & A
																					VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };	// All channels

const tcu::Vec4 BlendTest::s_blendConst = tcu::Vec4(0.1f, 0.2f, 0.3f, 0.4f);

BlendTest::BlendTest (tcu::TestContext&								testContext,
					  const std::string&							name,
					  const std::string&							description,
					  const VkFormat								colorFormat,
					  const VkPipelineColorBlendAttachmentState		blendStates[QUAD_COUNT])
	: vkt::TestCase	(testContext, name, description)
	, m_colorFormat(colorFormat)
{
	deMemcpy(m_blendStates, blendStates, sizeof(VkPipelineColorBlendAttachmentState) * QUAD_COUNT);
}

BlendTest::~BlendTest (void)
{
}

TestInstance* BlendTest::createInstance(Context& context) const
{
	return new BlendTestInstance(context, m_colorFormat, m_blendStates);
}

void BlendTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream fragmentSource;

	sourceCollections.glslSources.add("color_vert") << glu::VertexSource(
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 position;\n"
		"layout(location = 1) in highp vec4 color;\n"
		"layout(location = 0) out highp vec4 vtxColor;\n"
		"void main (void)\n"
		"{\n"
		"	gl_Position = position;\n"
		"	vtxColor = color;\n"
		"}\n");

	fragmentSource << "#version 310 es\n"
		"layout(location = 0) in highp vec4 vtxColor;\n"
		"layout(location = 0) out highp vec4 fragColor;\n"
		"void main (void)\n"
		"{\n"
		"	fragColor = vtxColor;\n"
		"}\n";

	sourceCollections.glslSources.add("color_frag") << glu::FragmentSource(fragmentSource.str());
}


// BlendTestInstance

BlendTestInstance::BlendTestInstance (Context&									context,
									  const VkFormat							colorFormat,
									  const VkPipelineColorBlendAttachmentState	blendStates[BlendTest::QUAD_COUNT])
	: vkt::TestInstance	(context)
	, m_renderSize		(32, 32)
	, m_colorFormat		(colorFormat)
{
	const DeviceInterface&		vk					= m_context.getDeviceInterface();
	const VkDevice				vkDevice			= m_context.getDevice();
	const deUint32				queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	SimpleAllocator				memAlloc			(vk, vkDevice, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));

	// Copy depth operators
	deMemcpy(m_blendStates, blendStates, sizeof(VkPipelineColorBlendAttachmentState) * BlendTest::QUAD_COUNT);

	// Create color image
	{
		if (!isSupportedBlendFormat(context.getInstanceInterface(), context.getPhysicalDevice(), m_colorFormat))
			throw tcu::NotSupportedError(std::string("Unsupported color blending format: ") + getFormatName(m_colorFormat));

		const VkImageCreateInfo	colorImageParams =
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
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }		// VkImageSubresourceRange	subresourceRange;
		};

		m_colorAttachmentView = createImageView(vk, vkDevice, &colorAttachmentViewParams);
	}

	// Create render pass
	{
		const VkAttachmentDescription colorAttachmentDescription =
		{
			0u,													// VkAttachmentDescriptionFlags		flags;
			m_colorFormat,										// VkFormat							format;
			VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					finalLayout;
		};

		const VkAttachmentReference colorAttachmentReference =
		{
			0u,													// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
		};

		const VkSubpassDescription subpassDescription =
		{
			0u,													// VkSubpassDescriptionFlag		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint			pipelineBindPoint;
			0u,													// deUint32						inputAttachmentCount;
			DE_NULL,											// const VkAttachmentReference*	pInputAttachments;
			1u,													// deUint32						colorAttachmentCount;
			&colorAttachmentReference,							// const VkAttachmentReference*	pColorAttachments;
			DE_NULL,											// const VkAttachmentReference*	pResolveAttachments;
			DE_NULL,											// const VkAttachmentReference*	pDepthStencilAttachment;
			0u,													// deUint32						preserveAttachmentCount;
			DE_NULL												// const VkAttachmentReference*	pPreserveAttachments;
		};

		const VkRenderPassCreateInfo renderPassParams =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkRenderPassCreateFlags			flags;
			1u,													// deUint32							attachmentCount;
			&colorAttachmentDescription,						// const VkAttachmentDescription*	pAttachments;
			1u,													// deUint32							subpassCount;
			&subpassDescription,								// const VkSubpassDescription*		pSubpasses;
			0u,													// deUint32							dependencyCount;
			DE_NULL												// const VkSubpassDependency*		pDependencies;
		};

		m_renderPass = createRenderPass(vk, vkDevice, &renderPassParams);
	}

	// Create framebuffer
	{
		const VkFramebufferCreateInfo framebufferParams =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,			// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkFramebufferCreateFlags	flags;
			*m_renderPass,										// VkRenderPass				renderPass;
			1u,													// deUint32					attachmentCount;
			&m_colorAttachmentView.get(),						// const VkImageView*		pAttachments;
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
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkPipelineLayoutCreateFlags		flags;
			0u,													// deUint32							setLayoutCount;
			DE_NULL,											// const VkDescriptorSetLayout*		pSetLayouts;
			0u,													// deUint32							pushConstantRangeCount;
			DE_NULL												// const VkPushConstantRange*		pPushConstantRanges;
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
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType					sType;
				DE_NULL,												// const void*						pNext;
				0u,														// VkPipelineShaderStageCreateFlags	flags;
				VK_SHADER_STAGE_VERTEX_BIT,								// VkShaderStageFlagBits			stage;
				*m_vertexShaderModule,									// VkShaderModule					module;
				"main",													// const char*						pName;
				DE_NULL													// const VkSpecializationInfo*		pSpecializationInfo;
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType					sType;
				DE_NULL,												// const void*						pNext;
				0u,														// VkPipelineShaderStageCreateFlags	flags;
				VK_SHADER_STAGE_FRAGMENT_BIT,							// VkShaderStageFlagBits			stage;
				*m_fragmentShaderModule,								// VkShaderModule					module;
				"main",													// const char*						pName;
				DE_NULL													// const VkSpecializationInfo*		pSpecializationInfo;
			}
		};

		const VkVertexInputBindingDescription vertexInputBindingDescription =
		{
			0u,									// deUint32					binding;
			sizeof(Vertex4RGBA),				// deUint32					strideInBytes;
			VK_VERTEX_INPUT_RATE_VERTEX			// VkVertexInputStepRate	inputRate;
		};

		const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] =
		{
			{
				0u,								// deUint32	location;
				0u,								// deUint32	binding;
				VK_FORMAT_R32G32B32A32_SFLOAT,	// VkFormat	format;
				0u								// deUint32	offset;
			},
			{
				1u,								// deUint32	location;
				0u,								// deUint32	binding;
				VK_FORMAT_R32G32B32A32_SFLOAT,	// VkFormat	format;
				(deUint32)(sizeof(float) * 4),	// deUint32	offset;
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
			&scissor														// const VkRect2D*						pScissors;
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

		const VkPipelineDepthStencilStateCreateInfo depthStencilStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			0u,															// VkPipelineDepthStencilStateCreateFlags	flags;
			false,														// VkBool32									depthTestEnable;
			false,														// VkBool32									depthWriteEnable;
			VK_COMPARE_OP_LESS,											// VkCompareOp								depthCompareOp;
			false,														// VkBool32									depthBoundsTestEnable;
			false,														// VkBool32									stencilTestEnable;
			// VkStencilOpState	front;
			{
				VK_STENCIL_OP_KEEP,		// VkStencilOp	failOp;
				VK_STENCIL_OP_KEEP,		// VkStencilOp	passOp;
				VK_STENCIL_OP_KEEP,		// VkStencilOp	depthFailOp;
				VK_COMPARE_OP_NEVER,	// VkCompareOp	compareOp;
				0u,						// deUint32		compareMask;
				0u,						// deUint32		writeMask;
				0u						// deUint32		reference;
			},
			// VkStencilOpState	back;
			{
				VK_STENCIL_OP_KEEP,		// VkStencilOp	failOp;
				VK_STENCIL_OP_KEEP,		// VkStencilOp	passOp;
				VK_STENCIL_OP_KEEP,		// VkStencilOp	depthFailOp;
				VK_COMPARE_OP_NEVER,	// VkCompareOp	compareOp;
				0u,						// deUint32		compareMask;
				0u,						// deUint32		writeMask;
				0u						// deUint32		reference;
			},
			0.0f,														// float			minDepthBounds;
			1.0f														// float			maxDepthBounds;
		};

		// The color blend attachment will be set up before creating the graphics pipeline.
		VkPipelineColorBlendStateCreateInfo colorBlendStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
			DE_NULL,													// const void*									pNext;
			0u,															// VkPipelineColorBlendStateCreateFlags			flags;
			false,														// VkBool32										logicOpEnable;
			VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
			0u,															// deUint32										attachmentCount;
			DE_NULL,													// const VkPipelineColorBlendAttachmentState*	pAttachments;
			{															// float										blendConstants[4];
				BlendTest::s_blendConst.x(),
				BlendTest::s_blendConst.y(),
				BlendTest::s_blendConst.z(),
				BlendTest::s_blendConst.w()
			}
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

		for (int quadNdx = 0; quadNdx < BlendTest::QUAD_COUNT; quadNdx++)
		{
			colorBlendStateParams.attachmentCount	= 1u;
			colorBlendStateParams.pAttachments		= &m_blendStates[quadNdx];
			m_graphicsPipelines[quadNdx]			= createGraphicsPipeline(vk, vkDevice, DE_NULL, &graphicsPipelineParams);
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

		// Adjust vertex colors
		if (!isFloatFormat(m_colorFormat))
		{
			const tcu::TextureFormatInfo formatInfo = tcu::getTextureFormatInfo(mapVkFormat(m_colorFormat));
			for (size_t vertexNdx = 0; vertexNdx < m_vertices.size(); vertexNdx++)
				m_vertices[vertexNdx].color = (m_vertices[vertexNdx].color - formatInfo.lookupBias) / formatInfo.lookupScale;
		}

		// Upload vertex data
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

		const VkClearValue attachmentClearValue = defaultClearValue(m_colorFormat);

		const VkRenderPassBeginInfo renderPassBeginInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,				// VkStructureType		sType;
			DE_NULL,												// const void*			pNext;
			*m_renderPass,											// VkRenderPass			renderPass;
			*m_framebuffer,											// VkFramebuffer		framebuffer;
			{ { 0, 0 }, { m_renderSize.x(), m_renderSize.y() } },	// VkRect2D				renderArea;
			1,														// deUint32				clearValueCount;
			&attachmentClearValue									// const VkClearValue*	pClearValues;
		};

		// Color image layout transition
		const VkImageMemoryBarrier imageLayoutBarrier =
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
		};

		m_cmdBuffer = allocateCommandBuffer(vk, vkDevice, *m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));

		vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0,
			0u, DE_NULL, 0u, DE_NULL, 1u, &imageLayoutBarrier);

		vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		const VkDeviceSize quadOffset = (m_vertices.size() / BlendTest::QUAD_COUNT) * sizeof(Vertex4RGBA);

		for (int quadNdx = 0; quadNdx < BlendTest::QUAD_COUNT; quadNdx++)
		{
			VkDeviceSize vertexBufferOffset = quadOffset * quadNdx;

			vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_graphicsPipelines[quadNdx]);
			vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &m_vertexBuffer.get(), &vertexBufferOffset);
			vk.cmdDraw(*m_cmdBuffer, (deUint32)(m_vertices.size() / BlendTest::QUAD_COUNT), 1, 0, 0);
		}

		vk.cmdEndRenderPass(*m_cmdBuffer);
		VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
	}

	// Create fence
	m_fence = createFence(vk, vkDevice);
}

BlendTestInstance::~BlendTestInstance (void)
{
}

tcu::TestStatus BlendTestInstance::iterate (void)
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
	VK_CHECK(vk.waitForFences(vkDevice, 1, &m_fence.get(), true, ~(0ull) /* infinity */));

	return verifyImage();
}

float BlendTestInstance::getNormChannelThreshold (const tcu::TextureFormat& format, int numBits)
{
	switch (tcu::getTextureChannelClass(format.type))
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:	return BlendTest::QUAD_COUNT / static_cast<float>((1 << numBits) - 1);
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:	return BlendTest::QUAD_COUNT / static_cast<float>((1 << (numBits - 1)) - 1);
		default:
			break;
	}

	DE_ASSERT(false);
	return 0.0f;
}

tcu::Vec4 BlendTestInstance::getFormatThreshold (const tcu::TextureFormat& format)
{
	using tcu::Vec4;
	using tcu::TextureFormat;

	Vec4 threshold(0.01f);

	switch (format.type)
	{
		case TextureFormat::UNORM_BYTE_44:
			threshold = Vec4(getNormChannelThreshold(format, 4), getNormChannelThreshold(format, 4), 1.0f, 1.0f);
			break;

		case TextureFormat::UNORM_SHORT_565:
			threshold = Vec4(getNormChannelThreshold(format, 5), getNormChannelThreshold(format, 6), getNormChannelThreshold(format, 5), 1.0f);
			break;

		case TextureFormat::UNORM_SHORT_555:
			threshold = Vec4(getNormChannelThreshold(format, 5), getNormChannelThreshold(format, 5), getNormChannelThreshold(format, 5), 1.0f);
			break;

		case TextureFormat::UNORM_SHORT_4444:
			threshold = Vec4(getNormChannelThreshold(format, 4));
			break;

		case TextureFormat::UNORM_SHORT_5551:
			threshold = Vec4(getNormChannelThreshold(format, 5), getNormChannelThreshold(format, 5), getNormChannelThreshold(format, 5), 0.1f);
			break;

		case TextureFormat::UNORM_INT_1010102_REV:
		case TextureFormat::SNORM_INT_1010102_REV:
			threshold = Vec4(getNormChannelThreshold(format, 10), getNormChannelThreshold(format, 10), getNormChannelThreshold(format, 10), 0.34f);
			break;

		case TextureFormat::UNORM_INT8:
		case TextureFormat::SNORM_INT8:
			threshold = Vec4(getNormChannelThreshold(format, 8));
			break;

		case TextureFormat::UNORM_INT16:
		case TextureFormat::SNORM_INT16:
			threshold = Vec4(getNormChannelThreshold(format, 16));
			break;

		case TextureFormat::UNORM_INT32:
		case TextureFormat::SNORM_INT32:
			threshold = Vec4(getNormChannelThreshold(format, 32));
			break;

		case TextureFormat::HALF_FLOAT:
			threshold = Vec4(0.005f);
			break;

		case TextureFormat::FLOAT:
			threshold = Vec4(0.00001f);
			break;

		case TextureFormat::UNSIGNED_INT_11F_11F_10F_REV:
			threshold = Vec4(0.02f, 0.02f, 0.0625f, 1.0f);
			break;

		case TextureFormat::UNSIGNED_INT_999_E5_REV:
			threshold = Vec4(0.05f, 0.05f, 0.05f, 1.0f);
			break;

		default:
			DE_ASSERT(false);
	}

	// Return value matching the channel order specified by the format
	if (format.order == tcu::TextureFormat::BGR || format.order == tcu::TextureFormat::BGRA)
		return threshold.swizzle(2, 1, 0, 3);
	else
		return threshold;
}

bool isLegalExpandableFormat (tcu::TextureFormat::ChannelType channeltype)
{
	using tcu::TextureFormat;

	switch (channeltype)
	{
		case TextureFormat::UNORM_INT24:
		case TextureFormat::UNORM_BYTE_44:
		case TextureFormat::UNORM_SHORT_565:
		case TextureFormat::UNORM_SHORT_555:
		case TextureFormat::UNORM_SHORT_4444:
		case TextureFormat::UNORM_SHORT_5551:
		case TextureFormat::UNORM_SHORT_1555:
		case TextureFormat::UNORM_INT_101010:
		case TextureFormat::SNORM_INT_1010102_REV:
		case TextureFormat::UNORM_INT_1010102_REV:
		case TextureFormat::UNSIGNED_BYTE_44:
		case TextureFormat::UNSIGNED_SHORT_565:
		case TextureFormat::UNSIGNED_SHORT_4444:
		case TextureFormat::UNSIGNED_SHORT_5551:
		case TextureFormat::SIGNED_INT_1010102_REV:
		case TextureFormat::UNSIGNED_INT_1010102_REV:
		case TextureFormat::UNSIGNED_INT_11F_11F_10F_REV:
		case TextureFormat::UNSIGNED_INT_999_E5_REV:
		case TextureFormat::UNSIGNED_INT_24_8:
		case TextureFormat::UNSIGNED_INT_24_8_REV:
		case TextureFormat::UNSIGNED_INT24:
		case TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV:
			return true;

		case TextureFormat::SNORM_INT8:
		case TextureFormat::SNORM_INT16:
		case TextureFormat::SNORM_INT32:
		case TextureFormat::UNORM_INT8:
		case TextureFormat::UNORM_INT16:
		case TextureFormat::UNORM_INT32:
		case TextureFormat::UNSIGNED_INT_16_8_8:
		case TextureFormat::SIGNED_INT8:
		case TextureFormat::SIGNED_INT16:
		case TextureFormat::SIGNED_INT32:
		case TextureFormat::UNSIGNED_INT8:
		case TextureFormat::UNSIGNED_INT16:
		case TextureFormat::UNSIGNED_INT32:
		case TextureFormat::HALF_FLOAT:
		case TextureFormat::FLOAT:
		case TextureFormat::FLOAT64:
			return false;

		default:
			DE_FATAL("Unknown texture format");
	}
	return false;
}

bool isSmallerThan8BitFormat (tcu::TextureFormat::ChannelType channeltype)
{
	using tcu::TextureFormat;

	// Note: only checks the legal expandable formats
	// (i.e, formats that have channels that fall outside
	// the 8, 16 and 32 bit width)
	switch (channeltype)
	{
		case TextureFormat::UNORM_BYTE_44:
		case TextureFormat::UNORM_SHORT_565:
		case TextureFormat::UNORM_SHORT_555:
		case TextureFormat::UNORM_SHORT_4444:
		case TextureFormat::UNORM_SHORT_5551:
		case TextureFormat::UNORM_SHORT_1555:
		case TextureFormat::UNSIGNED_BYTE_44:
		case TextureFormat::UNSIGNED_SHORT_565:
		case TextureFormat::UNSIGNED_SHORT_4444:
		case TextureFormat::UNSIGNED_SHORT_5551:
			return true;

		case TextureFormat::UNORM_INT24:
		case TextureFormat::UNORM_INT_101010:
		case TextureFormat::SNORM_INT_1010102_REV:
		case TextureFormat::UNORM_INT_1010102_REV:
		case TextureFormat::SIGNED_INT_1010102_REV:
		case TextureFormat::UNSIGNED_INT_1010102_REV:
		case TextureFormat::UNSIGNED_INT_11F_11F_10F_REV:
		case TextureFormat::UNSIGNED_INT_999_E5_REV:
		case TextureFormat::UNSIGNED_INT_24_8:
		case TextureFormat::UNSIGNED_INT_24_8_REV:
		case TextureFormat::UNSIGNED_INT24:
		case TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV:
			return false;

		default:
			DE_FATAL("Unknown texture format");
	}

	return false;
}

tcu::TestStatus BlendTestInstance::verifyImage (void)
{
	const tcu::TextureFormat	tcuColorFormat		= mapVkFormat(m_colorFormat);
	const tcu::TextureFormat	tcuColorFormat64	= mapVkFormat(VK_FORMAT_R64G64B64A64_SFLOAT);
	const tcu::TextureFormat	tcuColorFormat8		= mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM);
	const tcu::TextureFormat	tcuDepthFormat		= tcu::TextureFormat(); // Undefined depth/stencil format
	const ColorVertexShader		vertexShader;
	const ColorFragmentShader	fragmentShader		(tcuColorFormat, tcuDepthFormat);
	const rr::Program			program				(&vertexShader, &fragmentShader);
	ReferenceRenderer			refRenderer			(m_renderSize.x(), m_renderSize.y(), 1, tcuColorFormat, tcuDepthFormat, &program);
	ReferenceRenderer			refRenderer64		(m_renderSize.x(), m_renderSize.y(), 1, tcuColorFormat64, tcuDepthFormat, &program);
	ReferenceRenderer			refRenderer8		(m_renderSize.x(), m_renderSize.y(), 1, tcuColorFormat8, tcuDepthFormat, &program);
	bool						compareOk			= false;

	// Render reference image
	{
		for (int quadNdx = 0; quadNdx < BlendTest::QUAD_COUNT; quadNdx++)
		{
			const VkPipelineColorBlendAttachmentState& blendState = m_blendStates[quadNdx];

			// Set blend state
			rr::RenderState renderState					(refRenderer.getViewportState());
			renderState.fragOps.blendMode				= rr::BLENDMODE_STANDARD;
			renderState.fragOps.blendRGBState.srcFunc	= mapVkBlendFactor(blendState.srcColorBlendFactor);
			renderState.fragOps.blendRGBState.dstFunc	= mapVkBlendFactor(blendState.dstColorBlendFactor);
			renderState.fragOps.blendRGBState.equation	= mapVkBlendOp(blendState.colorBlendOp);
			renderState.fragOps.blendAState.srcFunc		= mapVkBlendFactor(blendState.srcAlphaBlendFactor);
			renderState.fragOps.blendAState.dstFunc		= mapVkBlendFactor(blendState.dstAlphaBlendFactor);
			renderState.fragOps.blendAState.equation	= mapVkBlendOp(blendState.alphaBlendOp);
			renderState.fragOps.blendColor				= BlendTest::s_blendConst;
			renderState.fragOps.colorMask				= mapVkColorComponentFlags(BlendTest::s_colorWriteMasks[quadNdx]);

			refRenderer.draw(renderState,
							rr::PRIMITIVETYPE_TRIANGLES,
							std::vector<Vertex4RGBA>(m_vertices.begin() + quadNdx * 6,
													 m_vertices.begin() + (quadNdx + 1) * 6));

			if (isLegalExpandableFormat(tcuColorFormat.type))
			{
				refRenderer64.draw(renderState,
								   rr::PRIMITIVETYPE_TRIANGLES,
								   std::vector<Vertex4RGBA>(m_vertices.begin() + quadNdx * 6,
								   m_vertices.begin() + (quadNdx + 1) * 6));

				if (isSmallerThan8BitFormat(tcuColorFormat.type))
					refRenderer8.draw(renderState,
									  rr::PRIMITIVETYPE_TRIANGLES,
									  std::vector<Vertex4RGBA>(m_vertices.begin() + quadNdx * 6,
									  m_vertices.begin() + (quadNdx + 1) * 6));
			}
		}
	}

	// Compare result with reference image
	{
		const DeviceInterface&				vk							= m_context.getDeviceInterface();
		const VkDevice						vkDevice					= m_context.getDevice();
		const VkQueue						queue						= m_context.getUniversalQueue();
		const deUint32						queueFamilyIndex			= m_context.getUniversalQueueFamilyIndex();
		SimpleAllocator						allocator					(vk, vkDevice, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));
		de::UniquePtr<tcu::TextureLevel>	result						(readColorAttachment(vk, vkDevice, queue, queueFamilyIndex, allocator, *m_colorImage, m_colorFormat, m_renderSize).release());
		const tcu::Vec4						threshold					(getFormatThreshold(tcuColorFormat));
		tcu::TextureLevel					refLevel;

		refLevel.setStorage(tcuColorFormat, m_renderSize.x(), m_renderSize.y(), 1);

		compareOk = tcu::floatThresholdCompare(m_context.getTestContext().getLog(),
											   "FloatImageCompare",
											   "Image comparison",
											   refRenderer.getAccess(),
											   result->getAccess(),
											   threshold,
											   tcu::COMPARE_LOG_RESULT);

		if (isLegalExpandableFormat(tcuColorFormat.type))
		{
			if (!compareOk && isSmallerThan8BitFormat(tcuColorFormat.type))
			{
				// Convert to target format
				tcu::copy(refLevel.getAccess(), refRenderer8.getAccess());

				compareOk = tcu::floatThresholdCompare(m_context.getTestContext().getLog(),
													   "FloatImageCompare",
													   "Image comparison, 8 bit intermediate format",
													   refLevel.getAccess(),
													   result->getAccess(),
													   threshold,
													   tcu::COMPARE_LOG_RESULT);
			}

			if (!compareOk)
			{
				// Convert to target format
				tcu::copy(refLevel.getAccess(), refRenderer64.getAccess());

				compareOk = tcu::floatThresholdCompare(m_context.getTestContext().getLog(),
													   "FloatImageCompare",
													   "Image comparison, 64 bit intermediate format",
													   refLevel.getAccess(),
													   result->getAccess(),
													   threshold,
													   tcu::COMPARE_LOG_RESULT);
			}
		}
	}

	if (compareOk)
		return tcu::TestStatus::pass("Result image matches reference");
	else
		return tcu::TestStatus::fail("Image mismatch");
}

} // anonymous

std::string getBlendStateName (const VkPipelineColorBlendAttachmentState& blendState)
{
	const char* shortBlendFactorNames[] =
	{
		"z",		// VK_BLEND_ZERO
		"o",		// VK_BLEND_ONE
		"sc",		// VK_BLEND_SRC_COLOR
		"1msc",		// VK_BLEND_ONE_MINUS_SRC_COLOR
		"dc",		// VK_BLEND_DEST_COLOR
		"1mdc",		// VK_BLEND_ONE_MINUS_DEST_COLOR
		"sa",		// VK_BLEND_SRC_ALPHA
		"1msa",		// VK_BLEND_ONE_MINUS_SRC_ALPHA
		"da",		// VK_BLEND_DEST_ALPHA
		"1mda",		// VK_BLEND_ONE_MINUS_DEST_ALPHA
		"cc",		// VK_BLEND_CONSTANT_COLOR
		"1mcc",		// VK_BLEND_ONE_MINUS_CONSTANT_COLOR
		"ca",		// VK_BLEND_CONSTANT_ALPHA
		"1mca",		// VK_BLEND_ONE_MINUS_CONSTANT_ALPHA
		"sas"		// VK_BLEND_SRC_ALPHA_SATURATE
	};

	const char* blendOpNames[] =
	{
		"add",		// VK_BLEND_OP_ADD
		"sub",		// VK_BLEND_OP_SUBTRACT
		"rsub",		// VK_BLEND_OP_REVERSE_SUBTRACT
		"min",		// VK_BLEND_OP_MIN
		"max",		// VK_BLEND_OP_MAX
	};

	std::ostringstream shortName;

	shortName << "color_" << shortBlendFactorNames[blendState.srcColorBlendFactor] << "_" << shortBlendFactorNames[blendState.dstColorBlendFactor] << "_" << blendOpNames[blendState.colorBlendOp];
	shortName << "_alpha_" << shortBlendFactorNames[blendState.srcAlphaBlendFactor] << "_" << shortBlendFactorNames[blendState.dstAlphaBlendFactor] << "_" << blendOpNames[blendState.alphaBlendOp];

	return shortName.str();
}

std::string getBlendStateSetName (const VkPipelineColorBlendAttachmentState blendStates[BlendTest::QUAD_COUNT])
{
	std::ostringstream name;

	for (int quadNdx = 0; quadNdx < BlendTest::QUAD_COUNT; quadNdx++)
	{
		name << getBlendStateName(blendStates[quadNdx]);

		if (quadNdx < BlendTest::QUAD_COUNT - 1)
			name << "-";
	}

	return name.str();
}

std::string getBlendStateSetDescription (const VkPipelineColorBlendAttachmentState blendStates[BlendTest::QUAD_COUNT])
{
	std::ostringstream description;

	description << "Draws " << BlendTest::QUAD_COUNT << " quads with the following blend states:\n";

	for (int quadNdx = 0; quadNdx < BlendTest::QUAD_COUNT; quadNdx++)
		description << blendStates[quadNdx] << "\n";

	return description.str();
}

std::string getFormatCaseName (VkFormat format)
{
	const std::string fullName = getFormatName(format);

	DE_ASSERT(de::beginsWith(fullName, "VK_FORMAT_"));

	return de::toLower(fullName.substr(10));
}

tcu::TestCaseGroup* createBlendTests (tcu::TestContext& testCtx)
{
	const deUint32 blendStatesPerFormat = 100 * BlendTest::QUAD_COUNT;

	// Formats that are dEQP-compatible, non-integer and uncompressed
	const VkFormat blendFormats[] =
	{
		VK_FORMAT_R4G4_UNORM_PACK8,
		VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_SRGB,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_SRGB,
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8_SNORM,
		VK_FORMAT_R8G8B8_SRGB,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16_SNORM,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		VK_FORMAT_B5G5R5A1_UNORM_PACK16,
	};

	de::MovePtr<tcu::TestCaseGroup>		blendTests		(new tcu::TestCaseGroup(testCtx, "blend", "Blend tests"));
	de::MovePtr<tcu::TestCaseGroup>		formatTests		(new tcu::TestCaseGroup(testCtx, "format", "Uses different blend formats"));
	BlendStateUniqueRandomIterator		blendStateItr	(blendStatesPerFormat, 123);

	for (size_t formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(blendFormats); formatNdx++)
	{
		const VkFormat					format			= blendFormats[formatNdx];
		de::MovePtr<tcu::TestCaseGroup>	formatTest		(new tcu::TestCaseGroup(testCtx,
																				getFormatCaseName(format).c_str(),
																				(std::string("Uses format ") + getFormatName(format)).c_str()));
		de::MovePtr<tcu::TestCaseGroup>	blendStateTests;
		{
			std::ostringstream blendStateDescription;
			blendStateDescription << "Combines blend factors, operators and channel write masks. The constant color used in all tests is " << BlendTest::s_blendConst;
			blendStateTests = de::MovePtr<tcu::TestCaseGroup>(new tcu::TestCaseGroup(testCtx, "states", blendStateDescription.str().c_str()));
		}

		blendStateItr.reset();

		while (blendStateItr.hasNext())
		{
			VkPipelineColorBlendAttachmentState quadBlendConfigs[BlendTest::QUAD_COUNT];

			for (int quadNdx = 0; quadNdx < BlendTest::QUAD_COUNT; quadNdx++)
			{
				quadBlendConfigs[quadNdx]					= blendStateItr.next();
				quadBlendConfigs[quadNdx].colorWriteMask	= BlendTest::s_colorWriteMasks[quadNdx];
			}

			blendStateTests->addChild(new BlendTest(testCtx,
													getBlendStateSetName(quadBlendConfigs),
													getBlendStateSetDescription(quadBlendConfigs),
													format,
													quadBlendConfigs));
		}
		formatTest->addChild(blendStateTests.release());
		formatTests->addChild(formatTest.release());
	}
	blendTests->addChild(formatTests.release());

	return blendTests.release();
}

} // pipeline
} // vkt
