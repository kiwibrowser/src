/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Functional rasterization tests.
 *//*--------------------------------------------------------------------*/

#include "vktRasterizationTests.hpp"
#include "tcuRasterizationVerifier.hpp"
#include "tcuSurface.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuResultCollector.hpp"
#include "vkImageUtil.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vkPrograms.hpp"
#include "vkMemUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"

#include <vector>

using namespace vk;

namespace vkt
{
namespace rasterization
{
namespace
{

using tcu::RasterizationArguments;
using tcu::TriangleSceneSpec;
using tcu::PointSceneSpec;
using tcu::LineSceneSpec;
using tcu::LineInterpolationMethod;

static const char* const s_shaderVertexTemplate =	"#version 310 es\n"
													"layout(location = 0) in highp vec4 a_position;\n"
													"layout(location = 1) in highp vec4 a_color;\n"
													"layout(location = 0) ${INTERPOLATION}out highp vec4 v_color;\n"
													"layout (set=0, binding=0) uniform PointSize {\n"
													"	highp float u_pointSize;\n"
													"};\n"
													"void main ()\n"
													"{\n"
													"	gl_Position = a_position;\n"
													"	gl_PointSize = u_pointSize;\n"
													"	v_color = a_color;\n"
													"}\n";

static const char* const s_shaderFragmentTemplate =	"#version 310 es\n"
													"layout(location = 0) out highp vec4 fragColor;\n"
													"layout(location = 0) ${INTERPOLATION}in highp vec4 v_color;\n"
													"void main ()\n"
													"{\n"
													"	fragColor = v_color;\n"
													"}\n";
enum InterpolationCaseFlags
{
	INTERPOLATIONFLAGS_NONE = 0,
	INTERPOLATIONFLAGS_PROJECTED = (1 << 1),
	INTERPOLATIONFLAGS_FLATSHADE = (1 << 2),
};

enum PrimitiveWideness
{
	PRIMITIVEWIDENESS_NARROW = 0,
	PRIMITIVEWIDENESS_WIDE,

	PRIMITIVEWIDENESS_LAST
};

class BaseRenderingTestCase : public TestCase
{
public:
								BaseRenderingTestCase	(tcu::TestContext& context, const std::string& name, const std::string& description, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, deBool flatshade = DE_FALSE);
	virtual						~BaseRenderingTestCase	(void);

	virtual void				initPrograms			(vk::SourceCollections& programCollection) const;

protected:
	const VkSampleCountFlagBits	m_sampleCount;
	const deBool				m_flatshade;
};

BaseRenderingTestCase::BaseRenderingTestCase (tcu::TestContext& context, const std::string& name, const std::string& description, VkSampleCountFlagBits sampleCount, deBool flatshade)
	: TestCase(context, name, description)
	, m_sampleCount	(sampleCount)
	, m_flatshade	(flatshade)
{
}

void BaseRenderingTestCase::initPrograms (vk::SourceCollections& programCollection) const
{
	tcu::StringTemplate					vertexSource	(s_shaderVertexTemplate);
	tcu::StringTemplate					fragmentSource	(s_shaderFragmentTemplate);
	std::map<std::string, std::string>	params;

	params["INTERPOLATION"] = (m_flatshade) ? ("flat ") : ("");

	programCollection.glslSources.add("vertext_shader") << glu::VertexSource(vertexSource.specialize(params));
	programCollection.glslSources.add("fragment_shader") << glu::FragmentSource(fragmentSource.specialize(params));
}

BaseRenderingTestCase::~BaseRenderingTestCase (void)
{
}

class BaseRenderingTestInstance : public TestInstance
{
public:
	enum {
		DEFAULT_RENDER_SIZE = 256
	};

													BaseRenderingTestInstance		(Context& context, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, deUint32 renderSize = DEFAULT_RENDER_SIZE);
													~BaseRenderingTestInstance		(void);

protected:
	void											addImageTransitionBarrier		(VkCommandBuffer commandBuffer, VkImage image, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout) const;
	void											drawPrimitives					(tcu::Surface& result, const std::vector<tcu::Vec4>& vertexData, VkPrimitiveTopology primitiveTopology);
	void											drawPrimitives					(tcu::Surface& result, const std::vector<tcu::Vec4>& vertexData, const std::vector<tcu::Vec4>& coloDrata, VkPrimitiveTopology primitiveTopology);
	virtual float									getLineWidth					(void) const;
	virtual float									getPointSize					(void) const;

	virtual
	const VkPipelineRasterizationStateCreateInfo*	getRasterizationStateCreateInfo	(void) const;

	virtual
	const VkPipelineColorBlendStateCreateInfo*		getColorBlendStateCreateInfo	(void) const;

	const tcu::TextureFormat&						getTextureFormat				(void) const;

	const deUint32									m_renderSize;
	const VkSampleCountFlagBits						m_sampleCount;
	const deUint32									m_subpixelBits;
	const deBool									m_multisampling;

	const VkFormat									m_imageFormat;
	const tcu::TextureFormat						m_textureFormat;
	Move<VkCommandPool>								m_commandPool;

	Move<VkImage>									m_image;
	de::MovePtr<Allocation>							m_imageMemory;
	Move<VkImageView>								m_imageView;

	Move<VkImage>									m_resolvedImage;
	de::MovePtr<Allocation>							m_resolvedImageMemory;
	Move<VkImageView>								m_resolvedImageView;

	Move<VkRenderPass>								m_renderPass;
	Move<VkFramebuffer>								m_frameBuffer;

	Move<VkDescriptorPool>							m_descriptorPool;
	Move<VkDescriptorSet>							m_descriptorSet;
	Move<VkDescriptorSetLayout>						m_descriptorSetLayout;

	Move<VkBuffer>									m_uniformBuffer;
	de::MovePtr<Allocation>							m_uniformBufferMemory;
	const VkDeviceSize								m_uniformBufferSize;

	Move<VkPipelineLayout>							m_pipelineLayout;

	Move<VkShaderModule>							m_vertexShaderModule;
	Move<VkShaderModule>							m_fragmentShaderModule;

	Move<VkFence>									m_fence;

	Move<VkBuffer>									m_resultBuffer;
	de::MovePtr<Allocation>							m_resultBufferMemory;
	const VkDeviceSize								m_resultBufferSize;
};

BaseRenderingTestInstance::BaseRenderingTestInstance (Context& context, VkSampleCountFlagBits sampleCount, deUint32 renderSize)
	: TestInstance			(context)
	, m_renderSize			(renderSize)
	, m_sampleCount			(sampleCount)
	, m_subpixelBits		(context.getDeviceProperties().limits.subPixelPrecisionBits)
	, m_multisampling		(m_sampleCount != VK_SAMPLE_COUNT_1_BIT)
	, m_imageFormat			(VK_FORMAT_R8G8B8A8_UNORM)
	, m_textureFormat		(vk::mapVkFormat(m_imageFormat))
	, m_uniformBufferSize	(sizeof(float))
	, m_resultBufferSize	(renderSize * renderSize * m_textureFormat.getPixelSize())
{
	const DeviceInterface&						vkd						= m_context.getDeviceInterface();
	const VkDevice								vkDevice				= m_context.getDevice();
	const deUint32								queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();
	Allocator&									allocator				= m_context.getDefaultAllocator();
	DescriptorPoolBuilder						descriptorPoolBuilder;
	DescriptorSetLayoutBuilder					descriptorSetLayoutBuilder;

	// Command Pool
	m_commandPool = createCommandPool(vkd, vkDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex);

	// Image
	{
		const VkImageUsageFlags	imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		VkImageFormatProperties	properties;

		if ((m_context.getInstanceInterface().getPhysicalDeviceImageFormatProperties(m_context.getPhysicalDevice(),
																					 m_imageFormat,
																					 VK_IMAGE_TYPE_2D,
																					 VK_IMAGE_TILING_OPTIMAL,
																					 imageUsage,
																					 0,
																					 &properties) == VK_ERROR_FORMAT_NOT_SUPPORTED))
		{
			TCU_THROW(NotSupportedError, "Format not supported");
		}

		if ((properties.sampleCounts & m_sampleCount) != m_sampleCount)
		{
			TCU_THROW(NotSupportedError, "Format not supported");
		}

		const VkImageCreateInfo					imageCreateInfo			=
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,		// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			0u,											// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,							// VkImageType				imageType;
			m_imageFormat,								// VkFormat					format;
			{ m_renderSize,	m_renderSize, 1u },			// VkExtent3D				extent;
			1u,											// deUint32					mipLevels;
			1u,											// deUint32					arrayLayers;
			m_sampleCount,								// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,					// VkImageTiling			tiling;
			imageUsage,									// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode			sharingMode;
			1u,											// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,							// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED					// VkImageLayout			initialLayout;
		};

		m_image = vk::createImage(vkd, vkDevice, &imageCreateInfo, DE_NULL);

		m_imageMemory	= allocator.allocate(getImageMemoryRequirements(vkd, vkDevice, *m_image), MemoryRequirement::Any);
		VK_CHECK(vkd.bindImageMemory(vkDevice, *m_image, m_imageMemory->getMemory(), m_imageMemory->getOffset()));
	}

	// Image View
	{
		const VkImageViewCreateInfo				imageViewCreateInfo		=
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType				sType;
			DE_NULL,									// const void*					pNext;
			0u,											// VkImageViewCreateFlags		flags;
			*m_image,									// VkImage						image;
			VK_IMAGE_VIEW_TYPE_2D,						// VkImageViewType				viewType;
			m_imageFormat,								// VkFormat						format;
			makeComponentMappingRGBA(),					// VkComponentMapping			components;
			{
				VK_IMAGE_ASPECT_COLOR_BIT,					// VkImageAspectFlags			aspectMask;
				0u,											// deUint32						baseMipLevel;
				1u,											// deUint32						mipLevels;
				0u,											// deUint32						baseArrayLayer;
				1u,											// deUint32						arraySize;
			},											// VkImageSubresourceRange		subresourceRange;
		};

		m_imageView = vk::createImageView(vkd, vkDevice, &imageViewCreateInfo, DE_NULL);
	}

	if (m_multisampling)
	{
		{
			// Resolved Image
			const VkImageUsageFlags	imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			VkImageFormatProperties	properties;

			if ((m_context.getInstanceInterface().getPhysicalDeviceImageFormatProperties(m_context.getPhysicalDevice(),
																						 m_imageFormat,
																						 VK_IMAGE_TYPE_2D,
																						 VK_IMAGE_TILING_OPTIMAL,
																						 imageUsage,
																						 0,
																						 &properties) == VK_ERROR_FORMAT_NOT_SUPPORTED))
			{
				TCU_THROW(NotSupportedError, "Format not supported");
			}

			const VkImageCreateInfo					imageCreateInfo			=
			{
				VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,		// VkStructureType			sType;
				DE_NULL,									// const void*				pNext;
				0u,											// VkImageCreateFlags		flags;
				VK_IMAGE_TYPE_2D,							// VkImageType				imageType;
				m_imageFormat,								// VkFormat					format;
				{ m_renderSize,	m_renderSize, 1u },			// VkExtent3D				extent;
				1u,											// deUint32					mipLevels;
				1u,											// deUint32					arrayLayers;
				VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits	samples;
				VK_IMAGE_TILING_OPTIMAL,					// VkImageTiling			tiling;
				imageUsage,									// VkImageUsageFlags		usage;
				VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode			sharingMode;
				1u,											// deUint32					queueFamilyIndexCount;
				&queueFamilyIndex,							// const deUint32*			pQueueFamilyIndices;
				VK_IMAGE_LAYOUT_UNDEFINED					// VkImageLayout			initialLayout;
			};

			m_resolvedImage			= vk::createImage(vkd, vkDevice, &imageCreateInfo, DE_NULL);
			m_resolvedImageMemory	= allocator.allocate(getImageMemoryRequirements(vkd, vkDevice, *m_resolvedImage), MemoryRequirement::Any);
			VK_CHECK(vkd.bindImageMemory(vkDevice, *m_resolvedImage, m_resolvedImageMemory->getMemory(), m_resolvedImageMemory->getOffset()));
		}

		// Resolved Image View
		{
			const VkImageViewCreateInfo				imageViewCreateInfo		=
			{
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType				sType;
				DE_NULL,									// const void*					pNext;
				0u,											// VkImageViewCreateFlags		flags;
				*m_resolvedImage,							// VkImage						image;
				VK_IMAGE_VIEW_TYPE_2D,						// VkImageViewType				viewType;
				m_imageFormat,								// VkFormat						format;
				makeComponentMappingRGBA(),					// VkComponentMapping			components;
				{
					VK_IMAGE_ASPECT_COLOR_BIT,					// VkImageAspectFlags			aspectMask;
					0u,											// deUint32						baseMipLevel;
					1u,											// deUint32						mipLevels;
					0u,											// deUint32						baseArrayLayer;
					1u,											// deUint32						arraySize;
				},											// VkImageSubresourceRange		subresourceRange;
			};

			m_resolvedImageView = vk::createImageView(vkd, vkDevice, &imageViewCreateInfo, DE_NULL);
		}

	}

	// Render Pass
	{
		const VkImageLayout						imageLayout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		const VkAttachmentDescription			attachmentDesc[]		=
		{
			{
				0u,													// VkAttachmentDescriptionFlags		flags;
				m_imageFormat,										// VkFormat							format;
				m_sampleCount,										// VkSampleCountFlagBits			samples;
				VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
				VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
				VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
				imageLayout,										// VkImageLayout					initialLayout;
				imageLayout,										// VkImageLayout					finalLayout;
			},
			{
				0u,													// VkAttachmentDescriptionFlags		flags;
				m_imageFormat,										// VkFormat							format;
				VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				loadOp;
				VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
				VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
				imageLayout,										// VkImageLayout					initialLayout;
				imageLayout,										// VkImageLayout					finalLayout;
			}
		};

		const VkAttachmentReference				attachmentRef			=
		{
			0u,													// deUint32							attachment;
			imageLayout,										// VkImageLayout					layout;
		};

		const VkAttachmentReference				resolveAttachmentRef	=
		{
			1u,													// deUint32							attachment;
			imageLayout,										// VkImageLayout					layout;
		};

		const VkSubpassDescription				subpassDesc				=
		{
			0u,													// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint				pipelineBindPoint;
			0u,													// deUint32							inputAttachmentCount;
			DE_NULL,											// const VkAttachmentReference*		pInputAttachments;
			1u,													// deUint32							colorAttachmentCount;
			&attachmentRef,										// const VkAttachmentReference*		pColorAttachments;
			m_multisampling ? &resolveAttachmentRef : DE_NULL,	// const VkAttachmentReference*		pResolveAttachments;
			DE_NULL,											// const VkAttachmentReference*		pDepthStencilAttachment;
			0u,													// deUint32							preserveAttachmentCount;
			DE_NULL,											// const VkAttachmentReference*		pPreserveAttachments;
		};

		const VkRenderPassCreateInfo			renderPassCreateInfo	=
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkRenderPassCreateFlags			flags;
			m_multisampling ? 2u : 1u,							// deUint32							attachmentCount;
			attachmentDesc,										// const VkAttachmentDescription*	pAttachments;
			1u,													// deUint32							subpassCount;
			&subpassDesc,										// const VkSubpassDescription*		pSubpasses;
			0u,													// deUint32							dependencyCount;
			DE_NULL,											// const VkSubpassDependency*		pDependencies;
		};

		m_renderPass =  createRenderPass(vkd, vkDevice, &renderPassCreateInfo, DE_NULL);
	}

	// FrameBuffer
	{
		const VkImageView						attachments[]			=
		{
			*m_imageView,
			*m_resolvedImageView
		};

		const VkFramebufferCreateInfo			framebufferCreateInfo	=
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			0u,											// VkFramebufferCreateFlags	flags;
			*m_renderPass,								// VkRenderPass				renderPass;
			m_multisampling ? 2u : 1u,					// deUint32					attachmentCount;
			attachments,								// const VkImageView*		pAttachments;
			m_renderSize,								// deUint32					width;
			m_renderSize,								// deUint32					height;
			1u,											// deUint32					layers;
		};

		m_frameBuffer = createFramebuffer(vkd, vkDevice, &framebufferCreateInfo, DE_NULL);
	}

	// Uniform Buffer
	{
		const VkBufferCreateInfo				bufferCreateInfo		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_uniformBufferSize,						// VkDeviceSize			size;
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_uniformBuffer			= createBuffer(vkd, vkDevice, &bufferCreateInfo);
		m_uniformBufferMemory	= allocator.allocate(getBufferMemoryRequirements(vkd, vkDevice, *m_uniformBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vkd.bindBufferMemory(vkDevice, *m_uniformBuffer, m_uniformBufferMemory->getMemory(), m_uniformBufferMemory->getOffset()));
	}

	// Descriptors
	{
		descriptorPoolBuilder.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_descriptorPool = descriptorPoolBuilder.build(vkd, vkDevice, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

		descriptorSetLayoutBuilder.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);
		m_descriptorSetLayout = descriptorSetLayoutBuilder.build(vkd, vkDevice);

		const VkDescriptorSetAllocateInfo		descriptorSetParams		=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,
			*m_descriptorPool,
			1u,
			&m_descriptorSetLayout.get(),
		};

		m_descriptorSet = allocateDescriptorSet(vkd, vkDevice, &descriptorSetParams);

		const VkDescriptorBufferInfo			descriptorBufferInfo	=
		{
			*m_uniformBuffer,							// VkBuffer		buffer;
			0u,											// VkDeviceSize	offset;
			VK_WHOLE_SIZE								// VkDeviceSize	range;
		};

		const VkWriteDescriptorSet				writeDescritporSet		=
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,		// VkStructureType					sType;
			DE_NULL,									// const void*						pNext;
			*m_descriptorSet,							// VkDescriptorSet					destSet;
			0,											// deUint32							destBinding;
			0,											// deUint32							destArrayElement;
			1u,											// deUint32							count;
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			// VkDescriptorType					descriptorType;
			DE_NULL,									// const VkDescriptorImageInfo*		pImageInfo;
			&descriptorBufferInfo,						// const VkDescriptorBufferInfo*	pBufferInfo;
			DE_NULL										// const VkBufferView*				pTexelBufferView;
		};

		vkd.updateDescriptorSets(vkDevice, 1u, &writeDescritporSet, 0u, DE_NULL);
	}

	// Pipeline Layout
	{
		const VkPipelineLayoutCreateInfo		pipelineLayoutCreateInfo	=
		{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType				sType;
			DE_NULL,											// const void*					pNext;
			0u,													// VkPipelineLayoutCreateFlags	flags;
			1u,													// deUint32						descriptorSetCount;
			&m_descriptorSetLayout.get(),						// const VkDescriptorSetLayout*	pSetLayouts;
			0u,													// deUint32						pushConstantRangeCount;
			DE_NULL												// const VkPushConstantRange*	pPushConstantRanges;
		};

		m_pipelineLayout = createPipelineLayout(vkd, vkDevice, &pipelineLayoutCreateInfo);
	}

	// Shaders
	{
		m_vertexShaderModule	= createShaderModule(vkd, vkDevice, m_context.getBinaryCollection().get("vertext_shader"), 0);
		m_fragmentShaderModule	= createShaderModule(vkd, vkDevice, m_context.getBinaryCollection().get("fragment_shader"), 0);
	}

	// Fence
	m_fence = createFence(vkd, vkDevice);

	// Result Buffer
	{
		const VkBufferCreateInfo				bufferCreateInfo		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_resultBufferSize,							// VkDeviceSize			size;
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_resultBuffer			= createBuffer(vkd, vkDevice, &bufferCreateInfo);
		m_resultBufferMemory	= allocator.allocate(getBufferMemoryRequirements(vkd, vkDevice, *m_resultBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vkd.bindBufferMemory(vkDevice, *m_resultBuffer, m_resultBufferMemory->getMemory(), m_resultBufferMemory->getOffset()));
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Sample count = " << getSampleCountFlagsStr(m_sampleCount) << tcu::TestLog::EndMessage;
	m_context.getTestContext().getLog() << tcu::TestLog::Message << "SUBPIXEL_BITS = " << m_subpixelBits << tcu::TestLog::EndMessage;
}

BaseRenderingTestInstance::~BaseRenderingTestInstance (void)
{
}


void BaseRenderingTestInstance::addImageTransitionBarrier(VkCommandBuffer commandBuffer, VkImage image, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout) const
{

	const DeviceInterface&			vkd					= m_context.getDeviceInterface();

	const VkImageSubresourceRange	subResourcerange	=
	{
		VK_IMAGE_ASPECT_COLOR_BIT,		// VkImageAspectFlags	aspectMask;
		0,								// deUint32				baseMipLevel;
		1,								// deUint32				levelCount;
		0,								// deUint32				baseArrayLayer;
		1								// deUint32				layerCount;
	};

	const VkImageMemoryBarrier		imageBarrier		=
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		srcAccessMask,								// VkAccessFlags			srcAccessMask;
		dstAccessMask,								// VkAccessFlags			dstAccessMask;
		oldLayout,									// VkImageLayout			oldLayout;
		newLayout,									// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					destQueueFamilyIndex;
		image,										// VkImage					image;
		subResourcerange							// VkImageSubresourceRange	subresourceRange;
	};

	vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, DE_NULL, 0, DE_NULL, 1, &imageBarrier);
}

void BaseRenderingTestInstance::drawPrimitives (tcu::Surface& result, const std::vector<tcu::Vec4>& vertexData, VkPrimitiveTopology primitiveTopology)
{
	// default to color white
	const std::vector<tcu::Vec4> colorData(vertexData.size(), tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

	drawPrimitives(result, vertexData, colorData, primitiveTopology);
}

void BaseRenderingTestInstance::drawPrimitives (tcu::Surface& result, const std::vector<tcu::Vec4>& positionData, const std::vector<tcu::Vec4>& colorData, VkPrimitiveTopology primitiveTopology)
{
	const DeviceInterface&						vkd						= m_context.getDeviceInterface();
	const VkDevice								vkDevice				= m_context.getDevice();
	const VkQueue								queue					= m_context.getUniversalQueue();
	const deUint32								queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();
	Allocator&									allocator				= m_context.getDefaultAllocator();
	const size_t								attributeBatchSize		= positionData.size() * sizeof(tcu::Vec4);

	Move<VkCommandBuffer>						commandBuffer;
	Move<VkPipeline>							graphicsPipeline;
	Move<VkBuffer>								vertexBuffer;
	de::MovePtr<Allocation>						vertexBufferMemory;
	const VkPhysicalDeviceProperties			properties				= m_context.getDeviceProperties();

	if (attributeBatchSize > properties.limits.maxVertexInputAttributeOffset)
	{
		std::stringstream message;
		message << "Larger vertex input attribute offset is needed (" << attributeBatchSize << ") than the available maximum (" << properties.limits.maxVertexInputAttributeOffset << ").";
		TCU_THROW(NotSupportedError, message.str().c_str());
	}

	// Create Graphics Pipeline
	{
		const VkPipelineShaderStageCreateInfo	shaderStageParams[2]	=
		{
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType					sType;
				DE_NULL,													// const void*						pNext;
				0,															// VkPipelineShaderStageCreateFlags flags;
				VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStage					stage;
				*m_vertexShaderModule,										// VkShader							shader;
				"main",														// const char*						pName;
				DE_NULL														// const VkSpecializationInfo*		pSpecializationInfo;
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType					sType;
				DE_NULL,													// const void*						pNext;
				0,															// VkPipelineShaderStageCreateFlags flags;
				VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStage					stage;
				*m_fragmentShaderModule,									// VkShader							shader;
				"main",														// const char*						pName;
				DE_NULL														// const VkSpecializationInfo*		pSpecializationInfo;
			}
		};

		const VkVertexInputBindingDescription	vertexInputBindingDescription =
		{
			0u,								// deUint32					binding;
			sizeof(tcu::Vec4),				// deUint32					strideInBytes;
			VK_VERTEX_INPUT_RATE_VERTEX		// VkVertexInputStepRate	stepRate;
		};

		const VkVertexInputAttributeDescription	vertexInputAttributeDescriptions[2] =
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
				(deUint32)attributeBatchSize		// deUint32	offsetInBytes;
			}
		};

		const VkPipelineVertexInputStateCreateInfo	vertexInputStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0,																// VkPipelineVertexInputStateCreateFlags	flags;
			1u,																// deUint32									bindingCount;
			&vertexInputBindingDescription,									// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
			2u,																// deUint32									attributeCount;
			vertexInputAttributeDescriptions								// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
		};

		const VkPipelineInputAssemblyStateCreateInfo	inputAssemblyStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0,																// VkPipelineInputAssemblyStateCreateFlags	flags;
			primitiveTopology,												// VkPrimitiveTopology						topology;
			false															// VkBool32									primitiveRestartEnable;
		};

		const VkViewport						viewport =
		{
			0.0f,						// float	originX;
			0.0f,						// float	originY;
			(float)m_renderSize,		// float	width;
			(float)m_renderSize,		// float	height;
			0.0f,						// float	minDepth;
			1.0f						// float	maxDepth;
		};

		const VkRect2D							scissor =
		{
			{ 0, 0 },														// VkOffset2D  offset;
			{ m_renderSize, m_renderSize }									// VkExtent2D  extent;
		};

		const VkPipelineViewportStateCreateInfo	viewportStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType						sType;
			DE_NULL,														// const void*							pNext;
			0,																// VkPipelineViewportStateCreateFlags	flags;
			1u,																// deUint32								viewportCount;
			&viewport,														// const VkViewport*					pViewports;
			1u,																// deUint32								scissorCount;
			&scissor														// const VkRect2D*						pScissors;
		};

		const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineMultisampleStateCreateFlags	flags;
			m_sampleCount,													// VkSampleCountFlagBits					rasterizationSamples;
			VK_FALSE,														// VkBool32									sampleShadingEnable;
			0.0f,															// float									minSampleShading;
			DE_NULL,														// const VkSampleMask*						pSampleMask;
			VK_FALSE,														// VkBool32									alphaToCoverageEnable;
			VK_FALSE														// VkBool32									alphaToOneEnable;
		};

		const VkGraphicsPipelineCreateInfo graphicsPipelineParams =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
			DE_NULL,											// const void*										pNext;
			0u,													// VkPipelineCreateFlags							flags;
			2u,													// deUint32											stageCount;
			shaderStageParams,									// const VkPipelineShaderStageCreateInfo*			pStages;
			&vertexInputStateParams,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
			&inputAssemblyStateParams,							// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
			DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
			&viewportStateParams,								// const VkPipelineViewportStateCreateInfo*			pViewportState;
			getRasterizationStateCreateInfo(),					// const VkPipelineRasterStateCreateInfo*			pRasterizationState;
			&multisampleStateParams,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
			DE_NULL,											// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
			getColorBlendStateCreateInfo(),						// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
			DE_NULL,											// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
			*m_pipelineLayout,									// VkPipelineLayout									layout;
			*m_renderPass,										// VkRenderPass										renderPass;
			0u,													// deUint32											subpass;
			0u,													// VkPipeline										basePipelineHandle;
			0u													// deInt32											basePipelineIndex;
		};

		graphicsPipeline		= createGraphicsPipeline(vkd, vkDevice, DE_NULL, &graphicsPipelineParams);
	}

	// Create Vertex Buffer
	{
		const VkBufferCreateInfo			vertexBufferParams		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			attributeBatchSize * 2,						// VkDeviceSize			size;
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		vertexBuffer		= createBuffer(vkd, vkDevice, &vertexBufferParams);
		vertexBufferMemory	= allocator.allocate(getBufferMemoryRequirements(vkd, vkDevice, *vertexBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vkd.bindBufferMemory(vkDevice, *vertexBuffer, vertexBufferMemory->getMemory(), vertexBufferMemory->getOffset()));

		// Load vertices into vertex buffer
		deMemcpy(vertexBufferMemory->getHostPtr(), positionData.data(), attributeBatchSize);
		deMemcpy(reinterpret_cast<deUint8*>(vertexBufferMemory->getHostPtr()) +  attributeBatchSize, colorData.data(), attributeBatchSize);
		flushMappedMemoryRange(vkd, vkDevice, vertexBufferMemory->getMemory(), vertexBufferMemory->getOffset(), vertexBufferParams.size);
	}

	// Create Command Buffer
	commandBuffer = allocateCommandBuffer(vkd, vkDevice, *m_commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	// Begin Command Buffer
	{
		const VkCommandBufferBeginInfo		cmdBufferBeginInfo		=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType							sType;
			DE_NULL,										// const void*								pNext;
			0u,												// VkCmdBufferOptimizeFlags					flags;
			DE_NULL											// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
		};

		VK_CHECK(vkd.beginCommandBuffer(*commandBuffer, &cmdBufferBeginInfo));
	}

	addImageTransitionBarrier(*commandBuffer, *m_image,
							  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,				// VkPipelineStageFlags		srcStageMask
							  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,				// VkPipelineStageFlags		dstStageMask
							  0,												// VkAccessFlags			srcAccessMask
							  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// VkAccessFlags			dstAccessMask
							  VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);		// VkImageLayout			newLayout;

	if (m_multisampling) {
		addImageTransitionBarrier(*commandBuffer, *m_resolvedImage,
								  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,				// VkPipelineStageFlags		srcStageMask
								  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,				// VkPipelineStageFlags		dstStageMask
								  0,												// VkAccessFlags			srcAccessMask
								  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// VkAccessFlags			dstAccessMask
								  VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
								  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);		// VkImageLayout			newLayout;
	}

	// Begin Render Pass
	{
		const VkClearValue					clearValue				= makeClearValueColorF32(0.0, 0.0, 0.0, 1.0);

		const VkRenderPassBeginInfo			renderPassBeginInfo		=
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,				// VkStructureType		sType;
			DE_NULL,												// const void*			pNext;
			*m_renderPass,											// VkRenderPass			renderPass;
			*m_frameBuffer,											// VkFramebuffer		framebuffer;
			{
				{ 0, 0 },
				{ m_renderSize, m_renderSize }
			},														// VkRect2D				renderArea;
			1u,														// deUint32				clearValueCount;
			&clearValue												// const VkClearValue*	pClearValues;
		};

		vkd.cmdBeginRenderPass(*commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	const VkDeviceSize						vertexBufferOffset		= 0;

	vkd.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);
	vkd.cmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipelineLayout, 0u, 1, &m_descriptorSet.get(), 0u, DE_NULL);
	vkd.cmdBindVertexBuffers(*commandBuffer, 0, 1, &vertexBuffer.get(), &vertexBufferOffset);
	vkd.cmdDraw(*commandBuffer, (deUint32)positionData.size(), 1, 0, 0);
	vkd.cmdEndRenderPass(*commandBuffer);

	// Copy Image
	{

		const VkBufferMemoryBarrier			bufferBarrier			=
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			VK_ACCESS_TRANSFER_WRITE_BIT,				// VkMemoryOutputFlags	outputMask;
			VK_ACCESS_HOST_READ_BIT,					// VkMemoryInputFlags	inputMask;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32				srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32				destQueueFamilyIndex;
			*m_resultBuffer,							// VkBuffer				buffer;
			0u,											// VkDeviceSize			offset;
			m_resultBufferSize							// VkDeviceSize			size;
		};

		const VkBufferImageCopy				copyRegion				=
		{
			0u,											// VkDeviceSize				bufferOffset;
			m_renderSize,								// deUint32					bufferRowLength;
			m_renderSize,								// deUint32					bufferImageHeight;
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u },	// VkImageSubresourceCopy	imageSubresource;
			{ 0, 0, 0 },								// VkOffset3D				imageOffset;
			{ m_renderSize, m_renderSize, 1u }			// VkExtent3D				imageExtent;
		};

		addImageTransitionBarrier(*commandBuffer,
								  m_multisampling ? *m_resolvedImage : *m_image,
								  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,		// VkPipelineStageFlags		srcStageMask
								  VK_PIPELINE_STAGE_TRANSFER_BIT,						// VkPipelineStageFlags		dstStageMask
								  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,					// VkAccessFlags			srcAccessMask
								  VK_ACCESS_TRANSFER_READ_BIT,							// VkAccessFlags			dstAccessMask
								  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,				// VkImageLayout			oldLayout;
								  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);				// VkImageLayout			newLayout;)

		if (m_multisampling)
			vkd.cmdCopyImageToBuffer(*commandBuffer, *m_resolvedImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *m_resultBuffer, 1, &copyRegion);
		else
			vkd.cmdCopyImageToBuffer(*commandBuffer, *m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *m_resultBuffer, 1, &copyRegion);

		vkd.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &bufferBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
	}

	VK_CHECK(vkd.endCommandBuffer(*commandBuffer));

	// Set Point Size
	{
		float	pointSize	= getPointSize();
		deMemcpy(m_uniformBufferMemory->getHostPtr(), &pointSize, (size_t)m_uniformBufferSize);
		flushMappedMemoryRange(vkd, vkDevice, m_uniformBufferMemory->getMemory(), m_uniformBufferMemory->getOffset(), m_uniformBufferSize);
	}

	// Submit
	{
		const VkSubmitInfo					submitInfo				=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType				sType;
			DE_NULL,								// const void*					pNext;
			0u,										// deUint32						waitSemaphoreCount;
			DE_NULL,								// const VkSemaphore*			pWaitSemaphores;
			DE_NULL,								// const VkPipelineStageFlags*	pWaitDstStageMask;
			1u,										// deUint32						commandBufferCount;
			&commandBuffer.get(),					// const VkCommandBuffer*		pCommandBuffers;
			0u,										// deUint32						signalSemaphoreCount;
			DE_NULL,								// const VkSemaphore*			pSignalSemaphores;
		};

		VK_CHECK(vkd.resetFences(vkDevice, 1, &m_fence.get()));
		VK_CHECK(vkd.queueSubmit(queue, 1, &submitInfo, *m_fence));
		VK_CHECK(vkd.waitForFences(vkDevice, 1, &m_fence.get(), true, ~(0ull) /* infinity */));
	}

	invalidateMappedMemoryRange(vkd, vkDevice, m_resultBufferMemory->getMemory(), m_resultBufferMemory->getOffset(), m_resultBufferSize);
	tcu::copy(result.getAccess(), tcu::ConstPixelBufferAccess(m_textureFormat, tcu::IVec3(m_renderSize, m_renderSize, 1), m_resultBufferMemory->getHostPtr()));
}

float BaseRenderingTestInstance::getLineWidth (void) const
{
	return 1.0f;
}

float BaseRenderingTestInstance::getPointSize (void) const
{
	return 1.0f;
}

const VkPipelineRasterizationStateCreateInfo* BaseRenderingTestInstance::getRasterizationStateCreateInfo (void) const
{
	static VkPipelineRasterizationStateCreateInfo	rasterizationStateCreateInfo	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		0,																// VkPipelineRasterizationStateCreateFlags	flags;
		false,															// VkBool32									depthClipEnable;
		false,															// VkBool32									rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,											// VkFillMode								fillMode;
		VK_CULL_MODE_NONE,												// VkCullMode								cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
		VK_FALSE,														// VkBool32									depthBiasEnable;
		0.0f,															// float									depthBias;
		0.0f,															// float									depthBiasClamp;
		0.0f,															// float									slopeScaledDepthBias;
		getLineWidth(),													// float									lineWidth;
	};

	rasterizationStateCreateInfo.lineWidth = getLineWidth();
	return &rasterizationStateCreateInfo;
}

const VkPipelineColorBlendStateCreateInfo* BaseRenderingTestInstance::getColorBlendStateCreateInfo (void) const
{
	static const VkPipelineColorBlendAttachmentState	colorBlendAttachmentState	=
	{
		false,														// VkBool32			blendEnable;
		VK_BLEND_FACTOR_ONE,										// VkBlend			srcBlendColor;
		VK_BLEND_FACTOR_ZERO,										// VkBlend			destBlendColor;
		VK_BLEND_OP_ADD,											// VkBlendOp		blendOpColor;
		VK_BLEND_FACTOR_ONE,										// VkBlend			srcBlendAlpha;
		VK_BLEND_FACTOR_ZERO,										// VkBlend			destBlendAlpha;
		VK_BLEND_OP_ADD,											// VkBlendOp		blendOpAlpha;
		(VK_COLOR_COMPONENT_R_BIT |
		 VK_COLOR_COMPONENT_G_BIT |
		 VK_COLOR_COMPONENT_B_BIT |
		 VK_COLOR_COMPONENT_A_BIT)									// VkChannelFlags	channelWriteMask;
	};

	static const VkPipelineColorBlendStateCreateInfo	colorBlendStateParams		=
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
		DE_NULL,													// const void*									pNext;
		0,															// VkPipelineColorBlendStateCreateFlags			flags;
		false,														// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
		1u,															// deUint32										attachmentCount;
		&colorBlendAttachmentState,									// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },									// float										blendConst[4];
	};

	return &colorBlendStateParams;
}

const tcu::TextureFormat& BaseRenderingTestInstance::getTextureFormat (void) const
{
	return m_textureFormat;
}

class BaseTriangleTestInstance : public BaseRenderingTestInstance
{
public:
							BaseTriangleTestInstance	(Context& context, VkPrimitiveTopology primitiveTopology, VkSampleCountFlagBits sampleCount);
	virtual tcu::TestStatus	iterate						(void);

private:
	virtual void			generateTriangles			(int iteration, std::vector<tcu::Vec4>& outData, std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles) = DE_NULL;

	int						m_iteration;
	const int				m_iterationCount;
	VkPrimitiveTopology		m_primitiveTopology;
	bool					m_allIterationsPassed;
};

BaseTriangleTestInstance::BaseTriangleTestInstance (Context& context, VkPrimitiveTopology primitiveTopology, VkSampleCountFlagBits sampleCount)
	: BaseRenderingTestInstance		(context, sampleCount)
	, m_iteration					(0)
	, m_iterationCount				(3)
	, m_primitiveTopology			(primitiveTopology)
	, m_allIterationsPassed			(true)
{
}

tcu::TestStatus BaseTriangleTestInstance::iterate (void)
{
	const std::string								iterationDescription	= "Test iteration " + de::toString(m_iteration+1) + " / " + de::toString(m_iterationCount);
	const tcu::ScopedLogSection						section					(m_context.getTestContext().getLog(), iterationDescription, iterationDescription);
	tcu::Surface									resultImage				(m_renderSize, m_renderSize);
	std::vector<tcu::Vec4>							drawBuffer;
	std::vector<TriangleSceneSpec::SceneTriangle>	triangles;

	generateTriangles(m_iteration, drawBuffer, triangles);

	// draw image
	drawPrimitives(resultImage, drawBuffer, m_primitiveTopology);

	// compare
	{
		bool					compareOk;
		RasterizationArguments	args;
		TriangleSceneSpec		scene;

		tcu::IVec4				colorBits = tcu::getTextureFormatBitDepth(getTextureFormat());

		args.numSamples		= m_multisampling ? 1 : 0;
		args.subpixelBits	= m_subpixelBits;
		args.redBits		= colorBits[0];
		args.greenBits		= colorBits[1];
		args.blueBits		= colorBits[2];

		scene.triangles.swap(triangles);

		compareOk = verifyTriangleGroupRasterization(resultImage, scene, args, m_context.getTestContext().getLog());

		if (!compareOk)
			m_allIterationsPassed = false;
	}

	// result
	if (++m_iteration == m_iterationCount)
	{
		if (m_allIterationsPassed)
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::fail("Incorrect rasterization");
	}
	else
		return tcu::TestStatus::incomplete();
}

class BaseLineTestInstance : public BaseRenderingTestInstance
{
public:
							BaseLineTestInstance	(Context& context, VkPrimitiveTopology primitiveTopology, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount);
	virtual tcu::TestStatus	iterate					(void);
	virtual float			getLineWidth			(void) const;

private:
	virtual void			generateLines			(int iteration, std::vector<tcu::Vec4>& outData, std::vector<LineSceneSpec::SceneLine>& outLines) = DE_NULL;

	int						m_iteration;
	const int				m_iterationCount;
	VkPrimitiveTopology		m_primitiveTopology;
	const PrimitiveWideness	m_primitiveWideness;
	bool					m_allIterationsPassed;
	float					m_maxLineWidth;
	std::vector<float>		m_lineWidths;
};

BaseLineTestInstance::BaseLineTestInstance (Context& context, VkPrimitiveTopology primitiveTopology, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount)
	: BaseRenderingTestInstance			(context, sampleCount)
	, m_iteration						(0)
	, m_iterationCount					(3)
	, m_primitiveTopology				(primitiveTopology)
	, m_primitiveWideness				(wideness)
	, m_allIterationsPassed				(true)
	, m_maxLineWidth					(1.0f)
{
	DE_ASSERT(m_primitiveWideness < PRIMITIVEWIDENESS_LAST);

	if (!context.getDeviceProperties().limits.strictLines)
		TCU_THROW(NotSupportedError, "Strict rasterization is not supported");

	// create line widths
	if (m_primitiveWideness == PRIMITIVEWIDENESS_NARROW)
	{
		m_lineWidths.resize(m_iterationCount, 1.0f);
	}
	else if (m_primitiveWideness == PRIMITIVEWIDENESS_WIDE)
	{
		if (!m_context.getDeviceFeatures().wideLines)
			TCU_THROW(NotSupportedError , "wide line support required");

		const float*	range = context.getDeviceProperties().limits.lineWidthRange;

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "ALIASED_LINE_WIDTH_RANGE = [" << range[0] << ", " << range[1] << "]" << tcu::TestLog::EndMessage;

		// no wide line support
		if (range[1] <= 1.0f)
			TCU_THROW(NotSupportedError, "wide line support required");

		// set hand picked sizes
		m_lineWidths.push_back(5.0f);
		m_lineWidths.push_back(10.0f);
		m_lineWidths.push_back(range[1]);
		DE_ASSERT((int)m_lineWidths.size() == m_iterationCount);

		m_maxLineWidth = range[1];
	}
	else
		DE_ASSERT(false);
}

tcu::TestStatus BaseLineTestInstance::iterate (void)
{
	const std::string						iterationDescription	= "Test iteration " + de::toString(m_iteration+1) + " / " + de::toString(m_iterationCount);
	const tcu::ScopedLogSection				section					(m_context.getTestContext().getLog(), iterationDescription, iterationDescription);
	const float								lineWidth				= getLineWidth();
	tcu::Surface							resultImage				(m_renderSize, m_renderSize);
	std::vector<tcu::Vec4>					drawBuffer;
	std::vector<LineSceneSpec::SceneLine>	lines;

	// supported?
	if (lineWidth <= m_maxLineWidth)
	{
		// gen data
		generateLines(m_iteration, drawBuffer, lines);

		// draw image
		drawPrimitives(resultImage, drawBuffer, m_primitiveTopology);

		// compare
		{
			RasterizationArguments	args;
			LineSceneSpec			scene;


			tcu::IVec4				colorBits = tcu::getTextureFormatBitDepth(getTextureFormat());

			args.numSamples		= m_multisampling ? 1 : 0;
			args.subpixelBits	= m_subpixelBits;
			args.redBits		= colorBits[0];
			args.greenBits		= colorBits[1];
			args.blueBits		= colorBits[2];

			scene.lines.swap(lines);
			scene.lineWidth = lineWidth;

			if (!verifyRelaxedLineGroupRasterization(resultImage, scene, args, m_context.getTestContext().getLog()))
				m_allIterationsPassed = false;
		}
	}
	else
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Line width " << lineWidth << " not supported, skipping iteration." << tcu::TestLog::EndMessage;

	// result
	if (++m_iteration == m_iterationCount)
	{
		if (m_allIterationsPassed)
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::fail("Incorrect rasterization");
	}
	else
		return tcu::TestStatus::incomplete();
}


float BaseLineTestInstance::getLineWidth (void) const
{
	return m_lineWidths[m_iteration];
}


class PointTestInstance : public BaseRenderingTestInstance
{
public:
							PointTestInstance		(Context& context, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount);
	virtual tcu::TestStatus	iterate					(void);
	virtual float			getPointSize			(void) const;

private:
	virtual void			generatePoints			(int iteration, std::vector<tcu::Vec4>& outData, std::vector<PointSceneSpec::ScenePoint>& outPoints);

	int						m_iteration;
	const int				m_iterationCount;
	const PrimitiveWideness	m_primitiveWideness;
	bool					m_allIterationsPassed;
	float					m_maxPointSize;
	std::vector<float>		m_pointSizes;
};

PointTestInstance::PointTestInstance (Context& context, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount)
	: BaseRenderingTestInstance	(context, sampleCount)
	, m_iteration				(0)
	, m_iterationCount			(3)
	, m_primitiveWideness		(wideness)
	, m_allIterationsPassed		(true)
	, m_maxPointSize			(1.0f)
{
	// create point sizes
	if (m_primitiveWideness == PRIMITIVEWIDENESS_NARROW)
	{
		m_pointSizes.resize(m_iterationCount, 1.0f);
	}
	else if (m_primitiveWideness == PRIMITIVEWIDENESS_WIDE)
	{
		if (!m_context.getDeviceFeatures().largePoints)
			TCU_THROW(NotSupportedError , "large point support required");

		const float*	range = context.getDeviceProperties().limits.pointSizeRange;

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_ALIASED_POINT_SIZE_RANGE = [" << range[0] << ", " << range[1] << "]" << tcu::TestLog::EndMessage;

		// no wide line support
		if (range[1] <= 1.0f)
			TCU_THROW(NotSupportedError , "wide point support required");

		// set hand picked sizes
		m_pointSizes.push_back(10.0f);
		m_pointSizes.push_back(25.0f);
		m_pointSizes.push_back(range[1]);
		DE_ASSERT((int)m_pointSizes.size() == m_iterationCount);

		m_maxPointSize = range[1];
	}
	else
		DE_ASSERT(false);
}

tcu::TestStatus PointTestInstance::iterate (void)
{
	const std::string						iterationDescription	= "Test iteration " + de::toString(m_iteration+1) + " / " + de::toString(m_iterationCount);
	const tcu::ScopedLogSection				section					(m_context.getTestContext().getLog(), iterationDescription, iterationDescription);
	const float								pointSize				= getPointSize();
	tcu::Surface							resultImage				(m_renderSize, m_renderSize);
	std::vector<tcu::Vec4>					drawBuffer;
	std::vector<PointSceneSpec::ScenePoint>	points;

	// supported?
	if (pointSize <= m_maxPointSize)
	{
		// gen data
		generatePoints(m_iteration, drawBuffer, points);

		// draw image
		drawPrimitives(resultImage, drawBuffer, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

		// compare
		{
			bool					compareOk;
			RasterizationArguments	args;
			PointSceneSpec			scene;

			tcu::IVec4				colorBits = tcu::getTextureFormatBitDepth(getTextureFormat());

			args.numSamples		= m_multisampling ? 1 : 0;
			args.subpixelBits	= m_subpixelBits;
			args.redBits		= colorBits[0];
			args.greenBits		= colorBits[1];
			args.blueBits		= colorBits[2];

			scene.points.swap(points);

			compareOk = verifyPointGroupRasterization(resultImage, scene, args, m_context.getTestContext().getLog());

			if (!compareOk)
				m_allIterationsPassed = false;
		}
	}
	else
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Point size " << pointSize << " not supported, skipping iteration." << tcu::TestLog::EndMessage;

	// result
	if (++m_iteration == m_iterationCount)
	{
		if (m_allIterationsPassed)
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::fail("Incorrect rasterization");
	}
	else
		return tcu::TestStatus::incomplete();
}

float PointTestInstance::getPointSize (void) const
{
	return m_pointSizes[m_iteration];
}

void PointTestInstance::generatePoints (int iteration, std::vector<tcu::Vec4>& outData, std::vector<PointSceneSpec::ScenePoint>& outPoints)
{
	outData.resize(6);

	switch (iteration)
	{
		case 0:
			// \note: these values are chosen arbitrarily
			outData[0] = tcu::Vec4( 0.2f,  0.8f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4( 0.5f,  0.2f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4( 0.5f,  0.3f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(-0.5f,  0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4(-0.2f, -0.4f, 0.0f, 1.0f);
			outData[5] = tcu::Vec4(-0.4f,  0.2f, 0.0f, 1.0f);
			break;

		case 1:
			outData[0] = tcu::Vec4(-0.499f, 0.128f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(-0.501f,  -0.3f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4(  0.11f,  -0.2f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(  0.11f,   0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4(  0.88f,   0.9f, 0.0f, 1.0f);
			outData[5] = tcu::Vec4(   0.4f,   1.2f, 0.0f, 1.0f);
			break;

		case 2:
			outData[0] = tcu::Vec4( -0.9f, -0.3f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(  0.3f, -0.9f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4( -0.4f, -0.1f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(-0.11f,  0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4( 0.88f,  0.7f, 0.0f, 1.0f);
			outData[5] = tcu::Vec4( -0.4f,  0.4f, 0.0f, 1.0f);
			break;
	}

	outPoints.resize(outData.size());
	for (int pointNdx = 0; pointNdx < (int)outPoints.size(); ++pointNdx)
	{
		outPoints[pointNdx].position = outData[pointNdx];
		outPoints[pointNdx].pointSize = getPointSize();
	}

	// log
	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Rendering " << outPoints.size() << " point(s): (point size = " << getPointSize() << ")" << tcu::TestLog::EndMessage;
	for (int pointNdx = 0; pointNdx < (int)outPoints.size(); ++pointNdx)
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Point " << (pointNdx+1) << ":\t" << outPoints[pointNdx].position << tcu::TestLog::EndMessage;
}

template <typename ConcreteTestInstance>
class BaseTestCase : public BaseRenderingTestCase
{
public:
							BaseTestCase	(tcu::TestContext& context, const std::string& name, const std::string& description, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT)
								: BaseRenderingTestCase(context, name, description, sampleCount)
							{}

	virtual TestInstance*	createInstance	(Context& context) const
							{
								return new ConcreteTestInstance(context, m_sampleCount);
							}
};

class TrianglesTestInstance : public BaseTriangleTestInstance
{
public:
							TrianglesTestInstance	(Context& context, VkSampleCountFlagBits sampleCount)
								: BaseTriangleTestInstance(context, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, sampleCount)
							{}

	void					generateTriangles		(int iteration, std::vector<tcu::Vec4>& outData, std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles);
};

void TrianglesTestInstance::generateTriangles (int iteration, std::vector<tcu::Vec4>& outData, std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles)
{
	outData.resize(6);

	switch (iteration)
	{
		case 0:
			// \note: these values are chosen arbitrarily
			outData[0] = tcu::Vec4( 0.2f,  0.8f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4( 0.5f,  0.2f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4( 0.5f,  0.3f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(-0.5f,  0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4(-1.5f, -0.4f, 0.0f, 1.0f);
			outData[5] = tcu::Vec4(-0.4f,  0.2f, 0.0f, 1.0f);
			break;

		case 1:
			outData[0] = tcu::Vec4(-0.499f, 0.128f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(-0.501f,  -0.3f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4(  0.11f,  -0.2f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(  0.11f,   0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4(  0.88f,   0.9f, 0.0f, 1.0f);
			outData[5] = tcu::Vec4(   0.4f,   1.2f, 0.0f, 1.0f);
			break;

		case 2:
			outData[0] = tcu::Vec4( -0.9f, -0.3f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(  1.1f, -0.9f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4( -1.1f, -0.1f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(-0.11f,  0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4( 0.88f,  0.7f, 0.0f, 1.0f);
			outData[5] = tcu::Vec4( -0.4f,  0.4f, 0.0f, 1.0f);
			break;
	}

	outTriangles.resize(2);
	outTriangles[0].positions[0] = outData[0];	outTriangles[0].sharedEdge[0] = false;
	outTriangles[0].positions[1] = outData[1];	outTriangles[0].sharedEdge[1] = false;
	outTriangles[0].positions[2] = outData[2];	outTriangles[0].sharedEdge[2] = false;

	outTriangles[1].positions[0] = outData[3];	outTriangles[1].sharedEdge[0] = false;
	outTriangles[1].positions[1] = outData[4];	outTriangles[1].sharedEdge[1] = false;
	outTriangles[1].positions[2] = outData[5];	outTriangles[1].sharedEdge[2] = false;

	// log
	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Rendering " << outTriangles.size() << " triangle(s):" << tcu::TestLog::EndMessage;
	for (int triangleNdx = 0; triangleNdx < (int)outTriangles.size(); ++triangleNdx)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Triangle " << (triangleNdx+1) << ":"
			<< "\n\t" << outTriangles[triangleNdx].positions[0]
			<< "\n\t" << outTriangles[triangleNdx].positions[1]
			<< "\n\t" << outTriangles[triangleNdx].positions[2]
			<< tcu::TestLog::EndMessage;
	}
}

class TriangleStripTestInstance : public BaseTriangleTestInstance
{
public:
				TriangleStripTestInstance		(Context& context, VkSampleCountFlagBits sampleCount)
					: BaseTriangleTestInstance(context, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, sampleCount)
				{}

	void		generateTriangles				(int iteration, std::vector<tcu::Vec4>& outData, std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles);
};

void TriangleStripTestInstance::generateTriangles (int iteration, std::vector<tcu::Vec4>& outData, std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles)
{
	outData.resize(5);

	switch (iteration)
	{
		case 0:
			// \note: these values are chosen arbitrarily
			outData[0] = tcu::Vec4(-0.504f,  0.8f,   0.0f, 1.0f);
			outData[1] = tcu::Vec4(-0.2f,   -0.2f,   0.0f, 1.0f);
			outData[2] = tcu::Vec4(-0.2f,    0.199f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4( 0.5f,    0.201f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4( 1.5f,    0.4f,   0.0f, 1.0f);
			break;

		case 1:
			outData[0] = tcu::Vec4(-0.499f, 0.129f,  0.0f, 1.0f);
			outData[1] = tcu::Vec4(-0.501f,  -0.3f,  0.0f, 1.0f);
			outData[2] = tcu::Vec4(  0.11f,  -0.2f,  0.0f, 1.0f);
			outData[3] = tcu::Vec4(  0.11f,  -0.31f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4(  0.88f,   0.9f,  0.0f, 1.0f);
			break;

		case 2:
			outData[0] = tcu::Vec4( -0.9f, -0.3f,  0.0f, 1.0f);
			outData[1] = tcu::Vec4(  1.1f, -0.9f,  0.0f, 1.0f);
			outData[2] = tcu::Vec4(-0.87f, -0.1f,  0.0f, 1.0f);
			outData[3] = tcu::Vec4(-0.11f,  0.19f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4( 0.88f,  0.7f,  0.0f, 1.0f);
			break;
	}

	outTriangles.resize(3);
	outTriangles[0].positions[0] = outData[0];	outTriangles[0].sharedEdge[0] = false;
	outTriangles[0].positions[1] = outData[1];	outTriangles[0].sharedEdge[1] = true;
	outTriangles[0].positions[2] = outData[2];	outTriangles[0].sharedEdge[2] = false;

	outTriangles[1].positions[0] = outData[2];	outTriangles[1].sharedEdge[0] = true;
	outTriangles[1].positions[1] = outData[1];	outTriangles[1].sharedEdge[1] = false;
	outTriangles[1].positions[2] = outData[3];	outTriangles[1].sharedEdge[2] = true;

	outTriangles[2].positions[0] = outData[2];	outTriangles[2].sharedEdge[0] = true;
	outTriangles[2].positions[1] = outData[3];	outTriangles[2].sharedEdge[1] = false;
	outTriangles[2].positions[2] = outData[4];	outTriangles[2].sharedEdge[2] = false;

	// log
	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Rendering triangle strip, " << outData.size() << " vertices." << tcu::TestLog::EndMessage;
	for (int vtxNdx = 0; vtxNdx < (int)outData.size(); ++vtxNdx)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "\t" << outData[vtxNdx]
			<< tcu::TestLog::EndMessage;
	}
}

class TriangleFanTestInstance : public BaseTriangleTestInstance
{
public:
				TriangleFanTestInstance			(Context& context, VkSampleCountFlagBits sampleCount)
					: BaseTriangleTestInstance(context, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, sampleCount)
				{}

	void		generateTriangles				(int iteration, std::vector<tcu::Vec4>& outData, std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles);
};

void TriangleFanTestInstance::generateTriangles (int iteration, std::vector<tcu::Vec4>& outData, std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles)
{
	outData.resize(5);

	switch (iteration)
	{
		case 0:
			// \note: these values are chosen arbitrarily
			outData[0] = tcu::Vec4( 0.01f,  0.0f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4( 0.5f,   0.2f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4( 0.46f,  0.3f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(-0.5f,   0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4(-1.5f,  -0.4f, 0.0f, 1.0f);
			break;

		case 1:
			outData[0] = tcu::Vec4(-0.499f, 0.128f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(-0.501f,  -0.3f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4(  0.11f,  -0.2f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(  0.11f,   0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4(  0.88f,   0.9f, 0.0f, 1.0f);
			break;

		case 2:
			outData[0] = tcu::Vec4( -0.9f, -0.3f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(  1.1f, -0.9f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4(  0.7f, -0.1f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4( 0.11f,  0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4( 0.88f,  0.7f, 0.0f, 1.0f);
			break;
	}

	outTriangles.resize(3);
	outTriangles[0].positions[0] = outData[0];	outTriangles[0].sharedEdge[0] = false;
	outTriangles[0].positions[1] = outData[1];	outTriangles[0].sharedEdge[1] = false;
	outTriangles[0].positions[2] = outData[2];	outTriangles[0].sharedEdge[2] = true;

	outTriangles[1].positions[0] = outData[0];	outTriangles[1].sharedEdge[0] = true;
	outTriangles[1].positions[1] = outData[2];	outTriangles[1].sharedEdge[1] = false;
	outTriangles[1].positions[2] = outData[3];	outTriangles[1].sharedEdge[2] = true;

	outTriangles[2].positions[0] = outData[0];	outTriangles[2].sharedEdge[0] = true;
	outTriangles[2].positions[1] = outData[3];	outTriangles[2].sharedEdge[1] = false;
	outTriangles[2].positions[2] = outData[4];	outTriangles[2].sharedEdge[2] = false;

	// log
	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Rendering triangle fan, " << outData.size() << " vertices." << tcu::TestLog::EndMessage;
	for (int vtxNdx = 0; vtxNdx < (int)outData.size(); ++vtxNdx)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "\t" << outData[vtxNdx]
			<< tcu::TestLog::EndMessage;
	}
}

template <typename ConcreteTestInstance>
class WidenessTestCase : public BaseRenderingTestCase
{
public:
								WidenessTestCase	(tcu::TestContext& context, const std::string& name, const std::string& description, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT)
									: BaseRenderingTestCase(context, name, description, sampleCount)
									, m_wideness(wideness)
								{}

	virtual TestInstance*		createInstance		(Context& context) const
								{
									return new ConcreteTestInstance(context, m_wideness, m_sampleCount);
								}
protected:
	const PrimitiveWideness		m_wideness;
};

class LinesTestInstance : public BaseLineTestInstance
{
public:
								LinesTestInstance	(Context& context, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount)
									: BaseLineTestInstance(context, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, wideness, sampleCount)
								{}

	virtual void				generateLines		(int iteration, std::vector<tcu::Vec4>& outData, std::vector<LineSceneSpec::SceneLine>& outLines);
};

void LinesTestInstance::generateLines (int iteration, std::vector<tcu::Vec4>& outData, std::vector<LineSceneSpec::SceneLine>& outLines)
{
	outData.resize(6);

	switch (iteration)
	{
		case 0:
			// \note: these values are chosen arbitrarily
			outData[0] = tcu::Vec4( 0.01f,  0.0f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4( 0.5f,   0.2f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4( 0.46f,  0.3f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(-0.3f,   0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4(-1.5f,  -0.4f, 0.0f, 1.0f);
			outData[5] = tcu::Vec4( 0.1f,   0.5f, 0.0f, 1.0f);
			break;

		case 1:
			outData[0] = tcu::Vec4(-0.499f, 0.128f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(-0.501f,  -0.3f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4(  0.11f,  -0.2f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(  0.11f,   0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4(  0.88f,   0.9f, 0.0f, 1.0f);
			outData[5] = tcu::Vec4(  0.18f,  -0.2f, 0.0f, 1.0f);
			break;

		case 2:
			outData[0] = tcu::Vec4( -0.9f, -0.3f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(  1.1f, -0.9f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4(  0.7f, -0.1f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4( 0.11f,  0.2f, 0.0f, 1.0f);
			outData[4] = tcu::Vec4( 0.88f,  0.7f, 0.0f, 1.0f);
			outData[5] = tcu::Vec4(  0.8f, -0.7f, 0.0f, 1.0f);
			break;
	}

	outLines.resize(3);
	outLines[0].positions[0] = outData[0];
	outLines[0].positions[1] = outData[1];
	outLines[1].positions[0] = outData[2];
	outLines[1].positions[1] = outData[3];
	outLines[2].positions[0] = outData[4];
	outLines[2].positions[1] = outData[5];

	// log
	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Rendering " << outLines.size() << " lines(s): (width = " << getLineWidth() << ")" << tcu::TestLog::EndMessage;
	for (int lineNdx = 0; lineNdx < (int)outLines.size(); ++lineNdx)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Line " << (lineNdx+1) << ":"
			<< "\n\t" << outLines[lineNdx].positions[0]
			<< "\n\t" << outLines[lineNdx].positions[1]
			<< tcu::TestLog::EndMessage;
	}
}

class LineStripTestInstance : public BaseLineTestInstance
{
public:
					LineStripTestInstance	(Context& context, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount)
						: BaseLineTestInstance(context, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, wideness, sampleCount)
					{}

	virtual void	generateLines			(int iteration, std::vector<tcu::Vec4>& outData, std::vector<LineSceneSpec::SceneLine>& outLines);
};

void LineStripTestInstance::generateLines (int iteration, std::vector<tcu::Vec4>& outData, std::vector<LineSceneSpec::SceneLine>& outLines)
{
	outData.resize(4);

	switch (iteration)
	{
		case 0:
			// \note: these values are chosen arbitrarily
			outData[0] = tcu::Vec4( 0.01f,  0.0f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4( 0.5f,   0.2f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4( 0.46f,  0.3f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(-0.5f,   0.2f, 0.0f, 1.0f);
			break;

		case 1:
			outData[0] = tcu::Vec4(-0.499f, 0.128f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(-0.501f,  -0.3f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4(  0.11f,  -0.2f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4(  0.11f,   0.2f, 0.0f, 1.0f);
			break;

		case 2:
			outData[0] = tcu::Vec4( -0.9f, -0.3f, 0.0f, 1.0f);
			outData[1] = tcu::Vec4(  1.1f, -0.9f, 0.0f, 1.0f);
			outData[2] = tcu::Vec4(  0.7f, -0.1f, 0.0f, 1.0f);
			outData[3] = tcu::Vec4( 0.11f,  0.2f, 0.0f, 1.0f);
			break;
	}

	outLines.resize(3);
	outLines[0].positions[0] = outData[0];
	outLines[0].positions[1] = outData[1];
	outLines[1].positions[0] = outData[1];
	outLines[1].positions[1] = outData[2];
	outLines[2].positions[0] = outData[2];
	outLines[2].positions[1] = outData[3];

	// log
	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Rendering line strip, width = " << getLineWidth() << ", " << outData.size() << " vertices." << tcu::TestLog::EndMessage;
	for (int vtxNdx = 0; vtxNdx < (int)outData.size(); ++vtxNdx)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "\t" << outData[vtxNdx]
			<< tcu::TestLog::EndMessage;
	}
}

class FillRuleTestInstance : public BaseRenderingTestInstance
{
public:
	enum FillRuleCaseType
	{
		FILLRULECASE_BASIC = 0,
		FILLRULECASE_REVERSED,
		FILLRULECASE_CLIPPED_FULL,
		FILLRULECASE_CLIPPED_PARTIAL,
		FILLRULECASE_PROJECTED,

		FILLRULECASE_LAST
	};
														FillRuleTestInstance			(Context& context, FillRuleCaseType type, VkSampleCountFlagBits sampleCount);
	virtual tcu::TestStatus								iterate							(void);

private:

	virtual const VkPipelineColorBlendStateCreateInfo*	getColorBlendStateCreateInfo	(void) const;
	int													getRenderSize					(FillRuleCaseType type) const;
	int													getNumIterations				(FillRuleCaseType type) const;
	void												generateTriangles				(int iteration, std::vector<tcu::Vec4>& outData) const;

	const FillRuleCaseType								m_caseType;
	int													m_iteration;
	const int											m_iterationCount;
	bool												m_allIterationsPassed;

};

FillRuleTestInstance::FillRuleTestInstance (Context& context, FillRuleCaseType type, VkSampleCountFlagBits sampleCount)
	: BaseRenderingTestInstance		(context, sampleCount, getRenderSize(type))
	, m_caseType					(type)
	, m_iteration					(0)
	, m_iterationCount				(getNumIterations(type))
	, m_allIterationsPassed			(true)
{
	DE_ASSERT(type < FILLRULECASE_LAST);
}

tcu::TestStatus FillRuleTestInstance::iterate (void)
{
	const std::string						iterationDescription	= "Test iteration " + de::toString(m_iteration+1) + " / " + de::toString(m_iterationCount);
	const tcu::ScopedLogSection				section					(m_context.getTestContext().getLog(), iterationDescription, iterationDescription);
	tcu::IVec4								colorBits				= tcu::getTextureFormatBitDepth(getTextureFormat());
	const int								thresholdRed			= 1 << (8 - colorBits[0]);
	const int								thresholdGreen			= 1 << (8 - colorBits[1]);
	const int								thresholdBlue			= 1 << (8 - colorBits[2]);
	tcu::Surface							resultImage				(m_renderSize, m_renderSize);
	std::vector<tcu::Vec4>					drawBuffer;

	generateTriangles(m_iteration, drawBuffer);

	// draw image
	{
		const std::vector<tcu::Vec4>	colorBuffer		(drawBuffer.size(), tcu::Vec4(0.5f, 0.5f, 0.5f, 1.0f));

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Drawing gray triangles with shared edges.\nEnabling additive blending to detect overlapping fragments." << tcu::TestLog::EndMessage;

		drawPrimitives(resultImage, drawBuffer, colorBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	}

	// verify no overdraw
	{
		const tcu::RGBA	triangleColor	= tcu::RGBA(127, 127, 127, 255);
		bool			overdraw		= false;

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Verifying result." << tcu::TestLog::EndMessage;

		for (int y = 0; y < resultImage.getHeight(); ++y)
		for (int x = 0; x < resultImage.getWidth();  ++x)
		{
			const tcu::RGBA color = resultImage.getPixel(x, y);

			// color values are greater than triangle color? Allow lower values for multisampled edges and background.
			if ((color.getRed()   - triangleColor.getRed())   > thresholdRed   ||
				(color.getGreen() - triangleColor.getGreen()) > thresholdGreen ||
				(color.getBlue()  - triangleColor.getBlue())  > thresholdBlue)
				overdraw = true;
		}

		// results
		if (!overdraw)
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "No overlapping fragments detected." << tcu::TestLog::EndMessage;
		else
		{
			m_context.getTestContext().getLog()	<< tcu::TestLog::Message << "Overlapping fragments detected, image is not valid." << tcu::TestLog::EndMessage;
			m_allIterationsPassed = false;
		}
	}

	// verify no missing fragments in the full viewport case
	if (m_caseType == FILLRULECASE_CLIPPED_FULL)
	{
		bool missingFragments = false;

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Searching missing fragments." << tcu::TestLog::EndMessage;

		for (int y = 0; y < resultImage.getHeight(); ++y)
		for (int x = 0; x < resultImage.getWidth();  ++x)
		{
			const tcu::RGBA color = resultImage.getPixel(x, y);

			// black? (background)
			if (color.getRed()   <= thresholdRed   ||
				color.getGreen() <= thresholdGreen ||
				color.getBlue()  <= thresholdBlue)
				missingFragments = true;
		}

		// results
		if (!missingFragments)
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "No missing fragments detected." << tcu::TestLog::EndMessage;
		else
		{
			m_context.getTestContext().getLog()	<< tcu::TestLog::Message << "Missing fragments detected, image is not valid." << tcu::TestLog::EndMessage;

			m_allIterationsPassed = false;
		}
	}

	m_context.getTestContext().getLog()	<< tcu::TestLog::ImageSet("Result of rendering", "Result of rendering")
										<< tcu::TestLog::Image("Result", "Result", resultImage)
										<< tcu::TestLog::EndImageSet;

	// result
	if (++m_iteration == m_iterationCount)
	{
		if (m_allIterationsPassed)
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::fail("Found invalid pixels");
	}
	else
		return tcu::TestStatus::incomplete();
}

int FillRuleTestInstance::getRenderSize (FillRuleCaseType type) const
{
	if (type == FILLRULECASE_CLIPPED_FULL || type == FILLRULECASE_CLIPPED_PARTIAL)
		return DEFAULT_RENDER_SIZE / 4;
	else
		return DEFAULT_RENDER_SIZE;
}

int FillRuleTestInstance::getNumIterations (FillRuleCaseType type) const
{
	if (type == FILLRULECASE_CLIPPED_FULL || type == FILLRULECASE_CLIPPED_PARTIAL)
		return 15;
	else
		return 2;
}

void FillRuleTestInstance::generateTriangles (int iteration, std::vector<tcu::Vec4>& outData) const
{
	switch (m_caseType)
	{
		case FILLRULECASE_BASIC:
		case FILLRULECASE_REVERSED:
		case FILLRULECASE_PROJECTED:
		{
			const int	numRows		= 4;
			const int	numColumns	= 4;
			const float	quadSide	= 0.15f;
			de::Random	rnd			(0xabcd);

			outData.resize(6 * numRows * numColumns);

			for (int col = 0; col < numColumns; ++col)
			for (int row = 0; row < numRows;    ++row)
			{
				const tcu::Vec2 center		= tcu::Vec2(((float)row + 0.5f) / (float)numRows * 2.0f - 1.0f, ((float)col + 0.5f) / (float)numColumns * 2.0f - 1.0f);
				const float		rotation	= (float)(iteration * numColumns * numRows + col * numRows + row) / (float)(m_iterationCount * numColumns * numRows) * DE_PI / 2.0f;
				const tcu::Vec2 sideH		= quadSide * tcu::Vec2(deFloatCos(rotation), deFloatSin(rotation));
				const tcu::Vec2 sideV		= tcu::Vec2(sideH.y(), -sideH.x());
				const tcu::Vec2 quad[4]		=
				{
					center + sideH + sideV,
					center + sideH - sideV,
					center - sideH - sideV,
					center - sideH + sideV,
				};

				if (m_caseType == FILLRULECASE_BASIC)
				{
					outData[6 * (col * numRows + row) + 0] = tcu::Vec4(quad[0].x(), quad[0].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 1] = tcu::Vec4(quad[1].x(), quad[1].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 2] = tcu::Vec4(quad[2].x(), quad[2].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 3] = tcu::Vec4(quad[2].x(), quad[2].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 4] = tcu::Vec4(quad[0].x(), quad[0].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 5] = tcu::Vec4(quad[3].x(), quad[3].y(), 0.0f, 1.0f);
				}
				else if (m_caseType == FILLRULECASE_REVERSED)
				{
					outData[6 * (col * numRows + row) + 0] = tcu::Vec4(quad[0].x(), quad[0].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 1] = tcu::Vec4(quad[1].x(), quad[1].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 2] = tcu::Vec4(quad[2].x(), quad[2].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 3] = tcu::Vec4(quad[0].x(), quad[0].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 4] = tcu::Vec4(quad[2].x(), quad[2].y(), 0.0f, 1.0f);
					outData[6 * (col * numRows + row) + 5] = tcu::Vec4(quad[3].x(), quad[3].y(), 0.0f, 1.0f);
				}
				else if (m_caseType == FILLRULECASE_PROJECTED)
				{
					const float w0 = rnd.getFloat(0.1f, 4.0f);
					const float w1 = rnd.getFloat(0.1f, 4.0f);
					const float w2 = rnd.getFloat(0.1f, 4.0f);
					const float w3 = rnd.getFloat(0.1f, 4.0f);

					outData[6 * (col * numRows + row) + 0] = tcu::Vec4(quad[0].x() * w0, quad[0].y() * w0, 0.0f, w0);
					outData[6 * (col * numRows + row) + 1] = tcu::Vec4(quad[1].x() * w1, quad[1].y() * w1, 0.0f, w1);
					outData[6 * (col * numRows + row) + 2] = tcu::Vec4(quad[2].x() * w2, quad[2].y() * w2, 0.0f, w2);
					outData[6 * (col * numRows + row) + 3] = tcu::Vec4(quad[2].x() * w2, quad[2].y() * w2, 0.0f, w2);
					outData[6 * (col * numRows + row) + 4] = tcu::Vec4(quad[0].x() * w0, quad[0].y() * w0, 0.0f, w0);
					outData[6 * (col * numRows + row) + 5] = tcu::Vec4(quad[3].x() * w3, quad[3].y() * w3, 0.0f, w3);
				}
				else
					DE_ASSERT(DE_FALSE);
			}

			break;
		}

		case FILLRULECASE_CLIPPED_PARTIAL:
		case FILLRULECASE_CLIPPED_FULL:
		{
			const float		quadSide	= (m_caseType == FILLRULECASE_CLIPPED_PARTIAL) ? (1.0f) : (2.0f);
			const tcu::Vec2 center		= (m_caseType == FILLRULECASE_CLIPPED_PARTIAL) ? (tcu::Vec2(0.5f, 0.5f)) : (tcu::Vec2(0.0f, 0.0f));
			const float		rotation	= (float)(iteration) / (float)(m_iterationCount - 1) * DE_PI / 2.0f;
			const tcu::Vec2 sideH		= quadSide * tcu::Vec2(deFloatCos(rotation), deFloatSin(rotation));
			const tcu::Vec2 sideV		= tcu::Vec2(sideH.y(), -sideH.x());
			const tcu::Vec2 quad[4]		=
			{
				center + sideH + sideV,
				center + sideH - sideV,
				center - sideH - sideV,
				center - sideH + sideV,
			};

			outData.resize(6);
			outData[0] = tcu::Vec4(quad[0].x(), quad[0].y(), 0.0f, 1.0f);
			outData[1] = tcu::Vec4(quad[1].x(), quad[1].y(), 0.0f, 1.0f);
			outData[2] = tcu::Vec4(quad[2].x(), quad[2].y(), 0.0f, 1.0f);
			outData[3] = tcu::Vec4(quad[2].x(), quad[2].y(), 0.0f, 1.0f);
			outData[4] = tcu::Vec4(quad[0].x(), quad[0].y(), 0.0f, 1.0f);
			outData[5] = tcu::Vec4(quad[3].x(), quad[3].y(), 0.0f, 1.0f);
			break;
		}

		default:
			DE_ASSERT(DE_FALSE);
	}
}

const VkPipelineColorBlendStateCreateInfo* FillRuleTestInstance::getColorBlendStateCreateInfo (void) const
{
	static const VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
	{
		true,														// VkBool32			blendEnable;
		VK_BLEND_FACTOR_ONE,										// VkBlend			srcBlendColor;
		VK_BLEND_FACTOR_ONE,										// VkBlend			destBlendColor;
		VK_BLEND_OP_ADD,											// VkBlendOp		blendOpColor;
		VK_BLEND_FACTOR_ONE,										// VkBlend			srcBlendAlpha;
		VK_BLEND_FACTOR_ONE,										// VkBlend			destBlendAlpha;
		VK_BLEND_OP_ADD,											// VkBlendOp		blendOpAlpha;
		(VK_COLOR_COMPONENT_R_BIT |
		 VK_COLOR_COMPONENT_G_BIT |
		 VK_COLOR_COMPONENT_B_BIT |
		 VK_COLOR_COMPONENT_A_BIT)									// VkChannelFlags	channelWriteMask;
	};

	static const VkPipelineColorBlendStateCreateInfo colorBlendStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
		DE_NULL,													// const void*									pNext;
		0,															// VkPipelineColorBlendStateCreateFlags			flags;
		false,														// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
		1u,															// deUint32										attachmentCount;
		&colorBlendAttachmentState,									// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },									// float										blendConst[4];
	};

	return &colorBlendStateParams;
}


class FillRuleTestCase : public BaseRenderingTestCase
{
public:
								FillRuleTestCase	(tcu::TestContext& context, const std::string& name, const std::string& description, FillRuleTestInstance::FillRuleCaseType type, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT)
									: BaseRenderingTestCase	(context, name, description, sampleCount)
									, m_type				(type)
								{}

	virtual TestInstance*		createInstance		(Context& context) const
								{
									return new FillRuleTestInstance(context, m_type, m_sampleCount);
								}
protected:
	const FillRuleTestInstance::FillRuleCaseType m_type;
};

class CullingTestInstance : public BaseRenderingTestInstance
{
public:
													CullingTestInstance				(Context& context, VkCullModeFlags cullMode, VkPrimitiveTopology primitiveTopology, VkFrontFace frontFace, VkPolygonMode polygonMode)
														: BaseRenderingTestInstance		(context, VK_SAMPLE_COUNT_1_BIT, DEFAULT_RENDER_SIZE)
														, m_cullMode					(cullMode)
														, m_primitiveTopology			(primitiveTopology)
														, m_frontFace					(frontFace)
														, m_polygonMode					(polygonMode)
														, m_multisampling				(true)
													{}
	virtual
	const VkPipelineRasterizationStateCreateInfo*	getRasterizationStateCreateInfo (void) const;

	tcu::TestStatus									iterate							(void);

private:
	void											generateVertices				(std::vector<tcu::Vec4>& outData) const;
	void											extractTriangles				(std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles, const std::vector<tcu::Vec4>& vertices) const;
	void											extractLines					(std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles, std::vector<LineSceneSpec::SceneLine>& outLines) const;
	void											extractPoints					(std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles, std::vector<PointSceneSpec::ScenePoint>& outPoints) const;
	bool											triangleOrder					(const tcu::Vec4& v0, const tcu::Vec4& v1, const tcu::Vec4& v2) const;

	const VkCullModeFlags							m_cullMode;
	const VkPrimitiveTopology						m_primitiveTopology;
	const VkFrontFace								m_frontFace;
	const VkPolygonMode								m_polygonMode;
	const bool										m_multisampling;
};


tcu::TestStatus CullingTestInstance::iterate (void)
{
	DE_ASSERT(m_polygonMode < VK_POLYGON_MODE_LAST);

	tcu::Surface									resultImage						(m_renderSize, m_renderSize);
	std::vector<tcu::Vec4>							drawBuffer;
	std::vector<TriangleSceneSpec::SceneTriangle>	triangles;
	std::vector<PointSceneSpec::ScenePoint>			points;
	std::vector<LineSceneSpec::SceneLine>			lines;

	const InstanceInterface&						vk				= m_context.getInstanceInterface();
	const VkPhysicalDevice							physicalDevice	= m_context.getPhysicalDevice();
	const VkPhysicalDeviceFeatures					deviceFeatures	= getPhysicalDeviceFeatures(vk, physicalDevice);

	if (!(deviceFeatures.fillModeNonSolid) && (m_polygonMode == VK_POLYGON_MODE_LINE || m_polygonMode == VK_POLYGON_MODE_POINT))
		TCU_THROW(NotSupportedError, "Wireframe fill modes are not supported");

	// generate scene
	generateVertices(drawBuffer);
	extractTriangles(triangles, drawBuffer);

	if (m_polygonMode == VK_POLYGON_MODE_LINE)
		extractLines(triangles ,lines);
	else if (m_polygonMode == VK_POLYGON_MODE_POINT)
		extractPoints(triangles, points);

	// draw image
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Setting front face to " << m_frontFace << tcu::TestLog::EndMessage;
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Setting cull face to " << m_cullMode << tcu::TestLog::EndMessage;
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Drawing test pattern (" << m_primitiveTopology << ")" << tcu::TestLog::EndMessage;

		drawPrimitives(resultImage, drawBuffer, m_primitiveTopology);
	}

	// compare
	{
		RasterizationArguments	args;
		tcu::IVec4				colorBits	= tcu::getTextureFormatBitDepth(getTextureFormat());
		bool					isCompareOk	= false;

		args.numSamples		= m_multisampling ? 1 : 0;
		args.subpixelBits	= m_subpixelBits;
		args.redBits		= colorBits[0];
		args.greenBits		= colorBits[1];
		args.blueBits		= colorBits[2];

		switch (m_polygonMode)
		{
			case VK_POLYGON_MODE_LINE:
			{
				LineSceneSpec scene;
				scene.lineWidth = 0;
				scene.lines.swap(lines);
				isCompareOk = verifyLineGroupRasterization(resultImage, scene, args, m_context.getTestContext().getLog());
				break;
			}
			case VK_POLYGON_MODE_POINT:
			{
				PointSceneSpec scene;
				scene.points.swap(points);
				isCompareOk = verifyPointGroupRasterization(resultImage, scene, args, m_context.getTestContext().getLog());
				break;
			}
			default:
			{
				TriangleSceneSpec scene;
				scene.triangles.swap(triangles);
				isCompareOk = verifyTriangleGroupRasterization(resultImage, scene, args, m_context.getTestContext().getLog(), tcu::VERIFICATIONMODE_WEAK);
				break;
			}
		}

		if (isCompareOk)
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::fail("Incorrect rendering");
	}
}

void CullingTestInstance::generateVertices (std::vector<tcu::Vec4>& outData) const
{
	de::Random rnd(543210);

	outData.resize(6);
	for (int vtxNdx = 0; vtxNdx < (int)outData.size(); ++vtxNdx)
	{
		outData[vtxNdx].x() = rnd.getFloat(-0.9f, 0.9f);
		outData[vtxNdx].y() = rnd.getFloat(-0.9f, 0.9f);
		outData[vtxNdx].z() = 0.0f;
		outData[vtxNdx].w() = 1.0f;
	}
}

void CullingTestInstance::extractTriangles (std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles, const std::vector<tcu::Vec4>& vertices) const
{
	const bool cullDirection = (m_cullMode == VK_CULL_MODE_FRONT_BIT) ^ (m_frontFace == VK_FRONT_FACE_COUNTER_CLOCKWISE);

	// No triangles
	if (m_cullMode == VK_CULL_MODE_FRONT_AND_BACK)
		return;

	switch (m_primitiveTopology)
	{
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		{
			for (int vtxNdx = 0; vtxNdx < (int)vertices.size() - 2; vtxNdx += 3)
			{
				const tcu::Vec4& v0 = vertices[vtxNdx + 0];
				const tcu::Vec4& v1 = vertices[vtxNdx + 1];
				const tcu::Vec4& v2 = vertices[vtxNdx + 2];

				if (triangleOrder(v0, v1, v2) != cullDirection)
				{
					TriangleSceneSpec::SceneTriangle tri;
					tri.positions[0] = v0;	tri.sharedEdge[0] = false;
					tri.positions[1] = v1;	tri.sharedEdge[1] = false;
					tri.positions[2] = v2;	tri.sharedEdge[2] = false;

					outTriangles.push_back(tri);
				}
			}
			break;
		}

		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		{
			for (int vtxNdx = 0; vtxNdx < (int)vertices.size() - 2; ++vtxNdx)
			{
				const tcu::Vec4& v0 = vertices[vtxNdx + 0];
				const tcu::Vec4& v1 = vertices[vtxNdx + 1];
				const tcu::Vec4& v2 = vertices[vtxNdx + 2];

				if (triangleOrder(v0, v1, v2) != (cullDirection ^ (vtxNdx % 2 != 0)))
				{
					TriangleSceneSpec::SceneTriangle tri;
					tri.positions[0] = v0;	tri.sharedEdge[0] = false;
					tri.positions[1] = v1;	tri.sharedEdge[1] = false;
					tri.positions[2] = v2;	tri.sharedEdge[2] = false;

					outTriangles.push_back(tri);
				}
			}
			break;
		}

		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
		{
			for (int vtxNdx = 1; vtxNdx < (int)vertices.size() - 1; ++vtxNdx)
			{
				const tcu::Vec4& v0 = vertices[0];
				const tcu::Vec4& v1 = vertices[vtxNdx + 0];
				const tcu::Vec4& v2 = vertices[vtxNdx + 1];

				if (triangleOrder(v0, v1, v2) != cullDirection)
				{
					TriangleSceneSpec::SceneTriangle tri;
					tri.positions[0] = v0;	tri.sharedEdge[0] = false;
					tri.positions[1] = v1;	tri.sharedEdge[1] = false;
					tri.positions[2] = v2;	tri.sharedEdge[2] = false;

					outTriangles.push_back(tri);
				}
			}
			break;
		}

		default:
			DE_ASSERT(false);
	}
}

void CullingTestInstance::extractLines (std::vector<TriangleSceneSpec::SceneTriangle>&	outTriangles,
										std::vector<LineSceneSpec::SceneLine>&			outLines) const
{
	for (int triNdx = 0; triNdx < (int)outTriangles.size(); ++triNdx)
	{
		for (int vrtxNdx = 0; vrtxNdx < 2; ++vrtxNdx)
		{
			LineSceneSpec::SceneLine line;
			line.positions[0] = outTriangles.at(triNdx).positions[vrtxNdx];
			line.positions[1] = outTriangles.at(triNdx).positions[vrtxNdx + 1];

			outLines.push_back(line);
		}
		LineSceneSpec::SceneLine line;
		line.positions[0] = outTriangles.at(triNdx).positions[2];
		line.positions[1] = outTriangles.at(triNdx).positions[0];
		outLines.push_back(line);
	}
}

void CullingTestInstance::extractPoints (std::vector<TriangleSceneSpec::SceneTriangle>	&outTriangles,
										std::vector<PointSceneSpec::ScenePoint>			&outPoints) const
{
	for (int triNdx = 0; triNdx < (int)outTriangles.size(); ++triNdx)
	{
		for (int vrtxNdx = 0; vrtxNdx < 3; ++vrtxNdx)
		{
			PointSceneSpec::ScenePoint point;
			point.position = outTriangles.at(triNdx).positions[vrtxNdx];
			point.pointSize = 1.0f;

			outPoints.push_back(point);
		}
	}
}

bool CullingTestInstance::triangleOrder (const tcu::Vec4& v0, const tcu::Vec4& v1, const tcu::Vec4& v2) const
{
	const tcu::Vec2 s0 = v0.swizzle(0, 1) / v0.w();
	const tcu::Vec2 s1 = v1.swizzle(0, 1) / v1.w();
	const tcu::Vec2 s2 = v2.swizzle(0, 1) / v2.w();

	// cross
	return ((s1.x() - s0.x()) * (s2.y() - s0.y()) - (s2.x() - s0.x()) * (s1.y() - s0.y())) > 0;
}


const VkPipelineRasterizationStateCreateInfo* CullingTestInstance::getRasterizationStateCreateInfo (void) const
{
	static VkPipelineRasterizationStateCreateInfo	rasterizationStateCreateInfo	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		0,																// VkPipelineRasterizationStateCreateFlags	flags;
		false,															// VkBool32									depthClipEnable;
		false,															// VkBool32									rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,											// VkFillMode								fillMode;
		VK_CULL_MODE_NONE,												// VkCullMode								cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
		VK_FALSE,														// VkBool32									depthBiasEnable;
		0.0f,															// float									depthBias;
		0.0f,															// float									depthBiasClamp;
		0.0f,															// float									slopeScaledDepthBias;
		getLineWidth(),													// float									lineWidth;
	};

	rasterizationStateCreateInfo.lineWidth		= getLineWidth();
	rasterizationStateCreateInfo.cullMode		= m_cullMode;
	rasterizationStateCreateInfo.frontFace		= m_frontFace;
	rasterizationStateCreateInfo.polygonMode	= m_polygonMode;

	return &rasterizationStateCreateInfo;
}

class CullingTestCase : public BaseRenderingTestCase
{
public:
								CullingTestCase		(tcu::TestContext& context, const std::string& name, const std::string& description, VkCullModeFlags cullMode, VkPrimitiveTopology primitiveTopology, VkFrontFace frontFace, VkPolygonMode polygonMode, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT)
									: BaseRenderingTestCase	(context, name, description, sampleCount)
									, m_cullMode			(cullMode)
									, m_primitiveTopology	(primitiveTopology)
									, m_frontFace			(frontFace)
									, m_polygonMode			(polygonMode)
								{}

	virtual TestInstance*		createInstance		(Context& context) const
								{
									return new CullingTestInstance(context, m_cullMode, m_primitiveTopology, m_frontFace, m_polygonMode);
								}
protected:
	const VkCullModeFlags		m_cullMode;
	const VkPrimitiveTopology	m_primitiveTopology;
	const VkFrontFace			m_frontFace;
	const VkPolygonMode			m_polygonMode;
};

class TriangleInterpolationTestInstance : public BaseRenderingTestInstance
{
public:

								TriangleInterpolationTestInstance	(Context& context, VkPrimitiveTopology primitiveTopology, int flags, VkSampleCountFlagBits sampleCount)
									: BaseRenderingTestInstance	(context, sampleCount, DEFAULT_RENDER_SIZE)
									, m_primitiveTopology		(primitiveTopology)
									, m_projective				((flags & INTERPOLATIONFLAGS_PROJECTED) != 0)
									, m_iterationCount			(3)
									, m_iteration				(0)
									, m_allIterationsPassed		(true)
									, m_flatshade				((flags & INTERPOLATIONFLAGS_FLATSHADE) != 0)
								{}

	tcu::TestStatus				iterate								(void);


private:
	void						generateVertices					(int iteration, std::vector<tcu::Vec4>& outVertices, std::vector<tcu::Vec4>& outColors) const;
	void						extractTriangles					(std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles, const std::vector<tcu::Vec4>& vertices, const std::vector<tcu::Vec4>& colors) const;


	VkPrimitiveTopology			m_primitiveTopology;
	const bool					m_projective;
	const int					m_iterationCount;
	int							m_iteration;
	bool						m_allIterationsPassed;
	const deBool				m_flatshade;
};

tcu::TestStatus TriangleInterpolationTestInstance::iterate (void)
{
	const std::string								iterationDescription	= "Test iteration " + de::toString(m_iteration+1) + " / " + de::toString(m_iterationCount);
	const tcu::ScopedLogSection						section					(m_context.getTestContext().getLog(), "Iteration" + de::toString(m_iteration+1), iterationDescription);
	tcu::Surface									resultImage				(m_renderSize, m_renderSize);
	std::vector<tcu::Vec4>							drawBuffer;
	std::vector<tcu::Vec4>							colorBuffer;
	std::vector<TriangleSceneSpec::SceneTriangle>	triangles;

	// generate scene
	generateVertices(m_iteration, drawBuffer, colorBuffer);
	extractTriangles(triangles, drawBuffer, colorBuffer);

	// log
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Generated vertices:" << tcu::TestLog::EndMessage;
		for (int vtxNdx = 0; vtxNdx < (int)drawBuffer.size(); ++vtxNdx)
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "\t" << drawBuffer[vtxNdx] << ",\tcolor= " << colorBuffer[vtxNdx] << tcu::TestLog::EndMessage;
	}

	// draw image
	drawPrimitives(resultImage, drawBuffer, colorBuffer, m_primitiveTopology);

	// compare
	{
		RasterizationArguments	args;
		TriangleSceneSpec		scene;
		tcu::IVec4				colorBits	= tcu::getTextureFormatBitDepth(getTextureFormat());

		args.numSamples		= m_multisampling ? 1 : 0;
		args.subpixelBits	= m_subpixelBits;
		args.redBits		= colorBits[0];
		args.greenBits		= colorBits[1];
		args.blueBits		= colorBits[2];

		scene.triangles.swap(triangles);

		if (!verifyTriangleGroupInterpolation(resultImage, scene, args, m_context.getTestContext().getLog()))
			m_allIterationsPassed = false;
	}

	// result
	if (++m_iteration == m_iterationCount)
	{
		if (m_allIterationsPassed)
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::fail("Found invalid pixel values");
	}
	else
		return tcu::TestStatus::incomplete();
}

void TriangleInterpolationTestInstance::generateVertices (int iteration, std::vector<tcu::Vec4>& outVertices, std::vector<tcu::Vec4>& outColors) const
{
	// use only red, green and blue
	const tcu::Vec4 colors[] =
	{
		tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f),
	};

	de::Random rnd(123 + iteration * 1000 + (int)m_primitiveTopology);

	outVertices.resize(6);
	outColors.resize(6);

	for (int vtxNdx = 0; vtxNdx < (int)outVertices.size(); ++vtxNdx)
	{
		outVertices[vtxNdx].x() = rnd.getFloat(-0.9f, 0.9f);
		outVertices[vtxNdx].y() = rnd.getFloat(-0.9f, 0.9f);
		outVertices[vtxNdx].z() = 0.0f;

		if (!m_projective)
			outVertices[vtxNdx].w() = 1.0f;
		else
		{
			const float w = rnd.getFloat(0.2f, 4.0f);

			outVertices[vtxNdx].x() *= w;
			outVertices[vtxNdx].y() *= w;
			outVertices[vtxNdx].z() *= w;
			outVertices[vtxNdx].w() = w;
		}

		outColors[vtxNdx] = colors[vtxNdx % DE_LENGTH_OF_ARRAY(colors)];
	}
}

void TriangleInterpolationTestInstance::extractTriangles (std::vector<TriangleSceneSpec::SceneTriangle>& outTriangles, const std::vector<tcu::Vec4>& vertices, const std::vector<tcu::Vec4>& colors) const
{
	switch (m_primitiveTopology)
	{
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		{
			for (int vtxNdx = 0; vtxNdx < (int)vertices.size() - 2; vtxNdx += 3)
			{
				TriangleSceneSpec::SceneTriangle tri;
				tri.positions[0]	= vertices[vtxNdx + 0];
				tri.positions[1]	= vertices[vtxNdx + 1];
				tri.positions[2]	= vertices[vtxNdx + 2];
				tri.sharedEdge[0]	= false;
				tri.sharedEdge[1]	= false;
				tri.sharedEdge[2]	= false;

				if (m_flatshade)
				{
					tri.colors[0] = colors[vtxNdx];
					tri.colors[1] = colors[vtxNdx];
					tri.colors[2] = colors[vtxNdx];
				}
				else
				{
					tri.colors[0] = colors[vtxNdx + 0];
					tri.colors[1] = colors[vtxNdx + 1];
					tri.colors[2] = colors[vtxNdx + 2];
				}

				outTriangles.push_back(tri);
			}
			break;
		}

		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		{
			for (int vtxNdx = 0; vtxNdx < (int)vertices.size() - 2; ++vtxNdx)
			{
				TriangleSceneSpec::SceneTriangle tri;
				tri.positions[0]	= vertices[vtxNdx + 0];
				tri.positions[1]	= vertices[vtxNdx + 1];
				tri.positions[2]	= vertices[vtxNdx + 2];
				tri.sharedEdge[0]	= false;
				tri.sharedEdge[1]	= false;
				tri.sharedEdge[2]	= false;

				if (m_flatshade)
				{
					tri.colors[0] = colors[vtxNdx];
					tri.colors[1] = colors[vtxNdx];
					tri.colors[2] = colors[vtxNdx];
				}
				else
				{
					tri.colors[0] = colors[vtxNdx + 0];
					tri.colors[1] = colors[vtxNdx + 1];
					tri.colors[2] = colors[vtxNdx + 2];
				}

				outTriangles.push_back(tri);
			}
			break;
		}

		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
		{
			for (int vtxNdx = 1; vtxNdx < (int)vertices.size() - 1; ++vtxNdx)
			{
				TriangleSceneSpec::SceneTriangle tri;
				tri.positions[0]	= vertices[0];
				tri.positions[1]	= vertices[vtxNdx + 0];
				tri.positions[2]	= vertices[vtxNdx + 1];
				tri.sharedEdge[0]	= false;
				tri.sharedEdge[1]	= false;
				tri.sharedEdge[2]	= false;

				if (m_flatshade)
				{
					tri.colors[0] = colors[vtxNdx];
					tri.colors[1] = colors[vtxNdx];
					tri.colors[2] = colors[vtxNdx];
				}
				else
				{
					tri.colors[0] = colors[0];
					tri.colors[1] = colors[vtxNdx + 0];
					tri.colors[2] = colors[vtxNdx + 1];
				}

				outTriangles.push_back(tri);
			}
			break;
		}

		default:
			DE_ASSERT(false);
	}
}

class TriangleInterpolationTestCase : public BaseRenderingTestCase
{
public:
								TriangleInterpolationTestCase	(tcu::TestContext& context, const std::string& name, const std::string& description, VkPrimitiveTopology primitiveTopology, int flags, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT)
									: BaseRenderingTestCase		(context, name, description, sampleCount, (flags & INTERPOLATIONFLAGS_FLATSHADE) != 0)
									, m_primitiveTopology		(primitiveTopology)
									, m_flags					(flags)
								{}

	virtual TestInstance*		createInstance					(Context& context) const
								{
									return new TriangleInterpolationTestInstance(context, m_primitiveTopology, m_flags, m_sampleCount);
								}
protected:
	const VkPrimitiveTopology	m_primitiveTopology;
	const int					m_flags;
};

class LineInterpolationTestInstance : public BaseRenderingTestInstance
{
public:
							LineInterpolationTestInstance	(Context& context, VkPrimitiveTopology primitiveTopology, int flags, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount);

	virtual tcu::TestStatus	iterate							(void);

private:
	void					generateVertices				(int iteration, std::vector<tcu::Vec4>& outVertices, std::vector<tcu::Vec4>& outColors) const;
	void					extractLines					(std::vector<LineSceneSpec::SceneLine>& outLines, const std::vector<tcu::Vec4>& vertices, const std::vector<tcu::Vec4>& colors) const;
	virtual float			getLineWidth					(void) const;

	VkPrimitiveTopology		m_primitiveTopology;
	const bool				m_projective;
	const int				m_iterationCount;
	const PrimitiveWideness	m_primitiveWideness;

	int						m_iteration;
	bool					m_allIterationsPassed;
	float					m_maxLineWidth;
	std::vector<float>		m_lineWidths;
	bool					m_flatshade;
};

LineInterpolationTestInstance::LineInterpolationTestInstance (Context& context, VkPrimitiveTopology primitiveTopology, int flags, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount)
	: BaseRenderingTestInstance			(context, sampleCount)
	, m_primitiveTopology				(primitiveTopology)
	, m_projective						((flags & INTERPOLATIONFLAGS_PROJECTED) != 0)
	, m_iterationCount					(3)
	, m_primitiveWideness				(wideness)
	, m_iteration						(0)
	, m_allIterationsPassed				(true)
	, m_maxLineWidth					(1.0f)
	, m_flatshade						((flags & INTERPOLATIONFLAGS_FLATSHADE) != 0)
{
	DE_ASSERT(m_primitiveWideness < PRIMITIVEWIDENESS_LAST);

	if (!context.getDeviceProperties().limits.strictLines)
		TCU_THROW(NotSupportedError, "Strict rasterization is not supported");

	// create line widths
	if (m_primitiveWideness == PRIMITIVEWIDENESS_NARROW)
	{
		m_lineWidths.resize(m_iterationCount, 1.0f);
	}
	else if (m_primitiveWideness == PRIMITIVEWIDENESS_WIDE)
	{
		if (!m_context.getDeviceFeatures().wideLines)
			TCU_THROW(NotSupportedError , "wide line support required");

		const float*	range = context.getDeviceProperties().limits.lineWidthRange;

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "ALIASED_LINE_WIDTH_RANGE = [" << range[0] << ", " << range[1] << "]" << tcu::TestLog::EndMessage;

		// no wide line support
		if (range[1] <= 1.0f)
			throw tcu::NotSupportedError("wide line support required");

		// set hand picked sizes
		m_lineWidths.push_back(5.0f);
		m_lineWidths.push_back(10.0f);
		m_lineWidths.push_back(range[1]);
		DE_ASSERT((int)m_lineWidths.size() == m_iterationCount);

		m_maxLineWidth = range[1];
	}
	else
		DE_ASSERT(false);
}

tcu::TestStatus LineInterpolationTestInstance::iterate (void)
{
	const std::string						iterationDescription	= "Test iteration " + de::toString(m_iteration+1) + " / " + de::toString(m_iterationCount);
	const tcu::ScopedLogSection				section					(m_context.getTestContext().getLog(), "Iteration" + de::toString(m_iteration+1), iterationDescription);
	const float								lineWidth				= getLineWidth();
	tcu::Surface							resultImage				(m_renderSize, m_renderSize);
	std::vector<tcu::Vec4>					drawBuffer;
	std::vector<tcu::Vec4>					colorBuffer;
	std::vector<LineSceneSpec::SceneLine>	lines;

	// supported?
	if (lineWidth <= m_maxLineWidth)
	{
		// generate scene
		generateVertices(m_iteration, drawBuffer, colorBuffer);
		extractLines(lines, drawBuffer, colorBuffer);

		// log
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Generated vertices:" << tcu::TestLog::EndMessage;
			for (int vtxNdx = 0; vtxNdx < (int)drawBuffer.size(); ++vtxNdx)
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "\t" << drawBuffer[vtxNdx] << ",\tcolor= " << colorBuffer[vtxNdx] << tcu::TestLog::EndMessage;
		}

		// draw image
		drawPrimitives(resultImage, drawBuffer, colorBuffer, m_primitiveTopology);

		// compare
		{
			RasterizationArguments	args;
			LineSceneSpec			scene;

			tcu::IVec4				colorBits = tcu::getTextureFormatBitDepth(getTextureFormat());

			args.numSamples		= m_multisampling ? 1 : 0;
			args.subpixelBits	= m_subpixelBits;
			args.redBits		= colorBits[0];
			args.greenBits		= colorBits[1];
			args.blueBits		= colorBits[2];

			scene.lines.swap(lines);
			scene.lineWidth = getLineWidth();

			if (!verifyTriangulatedLineGroupInterpolation(resultImage, scene, args, m_context.getTestContext().getLog()))
				m_allIterationsPassed = false;
		}
	}
	else
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Line width " << lineWidth << " not supported, skipping iteration." << tcu::TestLog::EndMessage;

	// result
	if (++m_iteration == m_iterationCount)
	{
		if (m_allIterationsPassed)
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::fail("Incorrect rasterization");
	}
	else
		return tcu::TestStatus::incomplete();
}

void LineInterpolationTestInstance::generateVertices (int iteration, std::vector<tcu::Vec4>& outVertices, std::vector<tcu::Vec4>& outColors) const
{
	// use only red, green and blue
	const tcu::Vec4 colors[] =
	{
		tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f),
	};

	de::Random rnd(123 + iteration * 1000 + (int)m_primitiveTopology);

	outVertices.resize(6);
	outColors.resize(6);

	for (int vtxNdx = 0; vtxNdx < (int)outVertices.size(); ++vtxNdx)
	{
		outVertices[vtxNdx].x() = rnd.getFloat(-0.9f, 0.9f);
		outVertices[vtxNdx].y() = rnd.getFloat(-0.9f, 0.9f);
		outVertices[vtxNdx].z() = 0.0f;

		if (!m_projective)
			outVertices[vtxNdx].w() = 1.0f;
		else
		{
			const float w = rnd.getFloat(0.2f, 4.0f);

			outVertices[vtxNdx].x() *= w;
			outVertices[vtxNdx].y() *= w;
			outVertices[vtxNdx].z() *= w;
			outVertices[vtxNdx].w() = w;
		}

		outColors[vtxNdx] = colors[vtxNdx % DE_LENGTH_OF_ARRAY(colors)];
	}
}

void LineInterpolationTestInstance::extractLines (std::vector<LineSceneSpec::SceneLine>& outLines, const std::vector<tcu::Vec4>& vertices, const std::vector<tcu::Vec4>& colors) const
{
	switch (m_primitiveTopology)
	{
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		{
			for (int vtxNdx = 0; vtxNdx < (int)vertices.size() - 1; vtxNdx += 2)
			{
				LineSceneSpec::SceneLine line;
				line.positions[0] = vertices[vtxNdx + 0];
				line.positions[1] = vertices[vtxNdx + 1];

				if (m_flatshade)
				{
					line.colors[0] = colors[vtxNdx];
					line.colors[1] = colors[vtxNdx];
				}
				else
				{
					line.colors[0] = colors[vtxNdx + 0];
					line.colors[1] = colors[vtxNdx + 1];
				}

				outLines.push_back(line);
			}
			break;
		}

		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		{
			for (int vtxNdx = 0; vtxNdx < (int)vertices.size() - 1; ++vtxNdx)
			{
				LineSceneSpec::SceneLine line;
				line.positions[0] = vertices[vtxNdx + 0];
				line.positions[1] = vertices[vtxNdx + 1];

				if (m_flatshade)
				{
					line.colors[0] = colors[vtxNdx];
					line.colors[1] = colors[vtxNdx];
				}
				else
				{
					line.colors[0] = colors[vtxNdx + 0];
					line.colors[1] = colors[vtxNdx + 1];
				}

				outLines.push_back(line);
			}
			break;
		}

		default:
			DE_ASSERT(false);
	}
}

float LineInterpolationTestInstance::getLineWidth (void) const
{
	return m_lineWidths[m_iteration];
}

class LineInterpolationTestCase : public BaseRenderingTestCase
{
public:
								LineInterpolationTestCase		(tcu::TestContext& context, const std::string& name, const std::string& description, VkPrimitiveTopology primitiveTopology, int flags, PrimitiveWideness wideness, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT)
									: BaseRenderingTestCase		(context, name, description, sampleCount, (flags & INTERPOLATIONFLAGS_FLATSHADE) != 0)
									, m_primitiveTopology		(primitiveTopology)
									, m_flags					(flags)
									, m_wideness				(wideness)
								{}

	virtual TestInstance*		createInstance					(Context& context) const
								{
									return new LineInterpolationTestInstance(context, m_primitiveTopology, m_flags, m_wideness, m_sampleCount);
								}
protected:
	const VkPrimitiveTopology	m_primitiveTopology;
	const int					m_flags;
	const PrimitiveWideness		m_wideness;
};

void createRasterizationTests (tcu::TestCaseGroup* rasterizationTests)
{
	tcu::TestContext&	testCtx		=	rasterizationTests->getTestContext();

	// .primitives
	{
		tcu::TestCaseGroup* const primitives = new tcu::TestCaseGroup(testCtx, "primitives", "Primitive rasterization");

		rasterizationTests->addChild(primitives);

		primitives->addChild(new BaseTestCase<TrianglesTestInstance>		(testCtx, "triangles",			"Render primitives as VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, verify rasterization result"));
		primitives->addChild(new BaseTestCase<TriangleStripTestInstance>	(testCtx, "triangle_strip",		"Render primitives as VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, verify rasterization result"));
		primitives->addChild(new BaseTestCase<TriangleFanTestInstance>		(testCtx, "triangle_fan",		"Render primitives as VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, verify rasterization result"));
		primitives->addChild(new WidenessTestCase<LinesTestInstance>		(testCtx, "lines",				"Render primitives as VK_PRIMITIVE_TOPOLOGY_LINE_LIST, verify rasterization result",						PRIMITIVEWIDENESS_NARROW));
		primitives->addChild(new WidenessTestCase<LineStripTestInstance>	(testCtx, "line_strip",			"Render primitives as VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, verify rasterization result",						PRIMITIVEWIDENESS_NARROW));
		primitives->addChild(new WidenessTestCase<LinesTestInstance>		(testCtx, "lines_wide",			"Render primitives as VK_PRIMITIVE_TOPOLOGY_LINE_LIST with wide lines, verify rasterization result",		PRIMITIVEWIDENESS_WIDE));
		primitives->addChild(new WidenessTestCase<LineStripTestInstance>	(testCtx, "line_strip_wide",	"Render primitives as VK_PRIMITIVE_TOPOLOGY_LINE_STRIP with wide lines, verify rasterization result",		PRIMITIVEWIDENESS_WIDE));
		primitives->addChild(new WidenessTestCase<PointTestInstance>		(testCtx, "points",				"Render primitives as VK_PRIMITIVE_TOPOLOGY_POINT_LIST, verify rasterization result",						PRIMITIVEWIDENESS_WIDE));
	}

	// .fill_rules
	{
		tcu::TestCaseGroup* const fillRules = new tcu::TestCaseGroup(testCtx, "fill_rules", "Primitive fill rules");

		rasterizationTests->addChild(fillRules);

		fillRules->addChild(new FillRuleTestCase(testCtx,	"basic_quad",			"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_BASIC));
		fillRules->addChild(new FillRuleTestCase(testCtx,	"basic_quad_reverse",	"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_REVERSED));
		fillRules->addChild(new FillRuleTestCase(testCtx,	"clipped_full",			"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_CLIPPED_FULL));
		fillRules->addChild(new FillRuleTestCase(testCtx,	"clipped_partly",		"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_CLIPPED_PARTIAL));
		fillRules->addChild(new FillRuleTestCase(testCtx,	"projected",			"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_PROJECTED));
	}

	// .culling
	{
		static const struct CullMode
		{
			VkCullModeFlags	mode;
			const char*		prefix;
		} cullModes[] =
		{
			{ VK_CULL_MODE_FRONT_BIT,				"front_"	},
			{ VK_CULL_MODE_BACK_BIT,				"back_"		},
			{ VK_CULL_MODE_FRONT_AND_BACK,			"both_"		},
		};
		static const struct PrimitiveType
		{
			VkPrimitiveTopology	type;
			const char*			name;
		} primitiveTypes[] =
		{
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,			"triangles"			},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,			"triangle_strip"	},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,			"triangle_fan"		},
		};
		static const struct FrontFaceOrder
		{
			VkFrontFace	mode;
			const char*	postfix;
		} frontOrders[] =
		{
			{ VK_FRONT_FACE_COUNTER_CLOCKWISE,	""			},
			{ VK_FRONT_FACE_CLOCKWISE,			"_reverse"	},
		};

		static const struct PolygonMode
		{
			VkPolygonMode	mode;
			const char*		name;
		} polygonModes[] =
		{
			{ VK_POLYGON_MODE_FILL,		""		},
			{ VK_POLYGON_MODE_LINE,		"_line"		},
			{ VK_POLYGON_MODE_POINT,	"_point"	}
		};

		tcu::TestCaseGroup* const culling = new tcu::TestCaseGroup(testCtx, "culling", "Culling");

		rasterizationTests->addChild(culling);

		for (int cullModeNdx	= 0; cullModeNdx	< DE_LENGTH_OF_ARRAY(cullModes);		++cullModeNdx)
		for (int primitiveNdx	= 0; primitiveNdx	< DE_LENGTH_OF_ARRAY(primitiveTypes);	++primitiveNdx)
		for (int frontOrderNdx	= 0; frontOrderNdx	< DE_LENGTH_OF_ARRAY(frontOrders);		++frontOrderNdx)
		for (int polygonModeNdx = 0; polygonModeNdx	< DE_LENGTH_OF_ARRAY(polygonModes);		++polygonModeNdx)
		{
			if (!(cullModes[cullModeNdx].mode == VK_CULL_MODE_FRONT_AND_BACK && polygonModes[polygonModeNdx].mode != VK_POLYGON_MODE_FILL))
			{
				const std::string name = std::string(cullModes[cullModeNdx].prefix) + primitiveTypes[primitiveNdx].name + frontOrders[frontOrderNdx].postfix + polygonModes[polygonModeNdx].name;
				culling->addChild(new CullingTestCase(testCtx, name, "Test primitive culling.", cullModes[cullModeNdx].mode, primitiveTypes[primitiveNdx].type, frontOrders[frontOrderNdx].mode, polygonModes[polygonModeNdx].mode));
			}
		}
	}

	// .interpolation
	{
		tcu::TestCaseGroup* const interpolation = new tcu::TestCaseGroup(testCtx, "interpolation", "Test interpolation");

		rasterizationTests->addChild(interpolation);

		// .basic
		{
			tcu::TestCaseGroup* const basic = new tcu::TestCaseGroup(testCtx, "basic", "Non-projective interpolation");

			interpolation->addChild(basic);

			basic->addChild(new TriangleInterpolationTestCase		(testCtx, "triangles",		"Verify triangle interpolation",		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,	INTERPOLATIONFLAGS_NONE));
			basic->addChild(new TriangleInterpolationTestCase		(testCtx, "triangle_strip",	"Verify triangle strip interpolation",	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	INTERPOLATIONFLAGS_NONE));
			basic->addChild(new TriangleInterpolationTestCase		(testCtx, "triangle_fan",	"Verify triangle fan interpolation",	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,		INTERPOLATIONFLAGS_NONE));
			basic->addChild(new LineInterpolationTestCase			(testCtx, "lines",			"Verify line interpolation",			VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		INTERPOLATIONFLAGS_NONE,	PRIMITIVEWIDENESS_NARROW));
			basic->addChild(new LineInterpolationTestCase			(testCtx, "line_strip",		"Verify line strip interpolation",		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		INTERPOLATIONFLAGS_NONE,	PRIMITIVEWIDENESS_NARROW));
			basic->addChild(new LineInterpolationTestCase			(testCtx, "lines_wide",		"Verify wide line interpolation",		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		INTERPOLATIONFLAGS_NONE,	PRIMITIVEWIDENESS_WIDE));
			basic->addChild(new LineInterpolationTestCase			(testCtx, "line_strip_wide","Verify wide line strip interpolation",	VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		INTERPOLATIONFLAGS_NONE,	PRIMITIVEWIDENESS_WIDE));
		}

		// .projected
		{
			tcu::TestCaseGroup* const projected = new tcu::TestCaseGroup(testCtx, "projected", "Projective interpolation");

			interpolation->addChild(projected);

			projected->addChild(new TriangleInterpolationTestCase	(testCtx, "triangles",		"Verify triangle interpolation",		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,	INTERPOLATIONFLAGS_PROJECTED));
			projected->addChild(new TriangleInterpolationTestCase	(testCtx, "triangle_strip",	"Verify triangle strip interpolation",	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	INTERPOLATIONFLAGS_PROJECTED));
			projected->addChild(new TriangleInterpolationTestCase	(testCtx, "triangle_fan",	"Verify triangle fan interpolation",	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,		INTERPOLATIONFLAGS_PROJECTED));
			projected->addChild(new LineInterpolationTestCase		(testCtx, "lines",			"Verify line interpolation",			VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		INTERPOLATIONFLAGS_PROJECTED,	PRIMITIVEWIDENESS_NARROW));
			projected->addChild(new LineInterpolationTestCase		(testCtx, "line_strip",		"Verify line strip interpolation",		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		INTERPOLATIONFLAGS_PROJECTED,	PRIMITIVEWIDENESS_NARROW));
			projected->addChild(new LineInterpolationTestCase		(testCtx, "lines_wide",		"Verify wide line interpolation",		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		INTERPOLATIONFLAGS_PROJECTED,	PRIMITIVEWIDENESS_WIDE));
			projected->addChild(new LineInterpolationTestCase		(testCtx, "line_strip_wide","Verify wide line strip interpolation",	VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		INTERPOLATIONFLAGS_PROJECTED,	PRIMITIVEWIDENESS_WIDE));
		}
	}

	// .flatshading
	{
		tcu::TestCaseGroup* const flatshading = new tcu::TestCaseGroup(testCtx, "flatshading", "Test flatshading");

		rasterizationTests->addChild(flatshading);

		flatshading->addChild(new TriangleInterpolationTestCase		(testCtx, "triangles",		"Verify triangle flatshading",			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,	INTERPOLATIONFLAGS_FLATSHADE));
		flatshading->addChild(new TriangleInterpolationTestCase		(testCtx, "triangle_strip",	"Verify triangle strip flatshading",	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	INTERPOLATIONFLAGS_FLATSHADE));
		flatshading->addChild(new TriangleInterpolationTestCase		(testCtx, "triangle_fan",	"Verify triangle fan flatshading",		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,		INTERPOLATIONFLAGS_FLATSHADE));
		flatshading->addChild(new LineInterpolationTestCase			(testCtx, "lines",			"Verify line flatshading",				VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		INTERPOLATIONFLAGS_FLATSHADE,	PRIMITIVEWIDENESS_NARROW));
		flatshading->addChild(new LineInterpolationTestCase			(testCtx, "line_strip",		"Verify line strip flatshading",		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		INTERPOLATIONFLAGS_FLATSHADE,	PRIMITIVEWIDENESS_NARROW));
		flatshading->addChild(new LineInterpolationTestCase			(testCtx, "lines_wide",		"Verify wide line flatshading",			VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		INTERPOLATIONFLAGS_FLATSHADE,	PRIMITIVEWIDENESS_WIDE));
		flatshading->addChild(new LineInterpolationTestCase			(testCtx, "line_strip_wide","Verify wide line strip flatshading",	VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		INTERPOLATIONFLAGS_FLATSHADE,	PRIMITIVEWIDENESS_WIDE));
	}

	const VkSampleCountFlagBits samples[] =
	{
		VK_SAMPLE_COUNT_2_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_64_BIT
	};

	for (int samplesNdx = 0; samplesNdx < DE_LENGTH_OF_ARRAY(samples); samplesNdx++)
	{
		std::ostringstream caseName;

		caseName << "_multisample_" << (2 << samplesNdx) << "_bit";

		// .primitives
		{
			tcu::TestCaseGroup* const primitives = new tcu::TestCaseGroup(testCtx, ("primitives" + caseName.str()).c_str(), "Primitive rasterization");

			rasterizationTests->addChild(primitives);

			primitives->addChild(new BaseTestCase<TrianglesTestInstance>		(testCtx, "triangles",			"Render primitives as VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, verify rasterization result",					samples[samplesNdx]));
			primitives->addChild(new WidenessTestCase<LinesTestInstance>		(testCtx, "lines",				"Render primitives as VK_PRIMITIVE_TOPOLOGY_LINE_LIST, verify rasterization result",						PRIMITIVEWIDENESS_NARROW,	samples[samplesNdx]));
			primitives->addChild(new WidenessTestCase<LinesTestInstance>		(testCtx, "lines_wide",			"Render primitives as VK_PRIMITIVE_TOPOLOGY_LINE_LIST with wide lines, verify rasterization result",		PRIMITIVEWIDENESS_WIDE,		samples[samplesNdx]));
			primitives->addChild(new WidenessTestCase<PointTestInstance>		(testCtx, "points",				"Render primitives as VK_PRIMITIVE_TOPOLOGY_POINT_LIST, verify rasterization result",						PRIMITIVEWIDENESS_WIDE,		samples[samplesNdx]));
		}

		// .fill_rules
		{
			tcu::TestCaseGroup* const fillRules = new tcu::TestCaseGroup(testCtx, ("fill_rules" + caseName.str()).c_str(), "Primitive fill rules");

			rasterizationTests->addChild(fillRules);

			fillRules->addChild(new FillRuleTestCase(testCtx,	"basic_quad",			"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_BASIC,			samples[samplesNdx]));
			fillRules->addChild(new FillRuleTestCase(testCtx,	"basic_quad_reverse",	"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_REVERSED,		samples[samplesNdx]));
			fillRules->addChild(new FillRuleTestCase(testCtx,	"clipped_full",			"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_CLIPPED_FULL,	samples[samplesNdx]));
			fillRules->addChild(new FillRuleTestCase(testCtx,	"clipped_partly",		"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_CLIPPED_PARTIAL,	samples[samplesNdx]));
			fillRules->addChild(new FillRuleTestCase(testCtx,	"projected",			"Verify fill rules",	FillRuleTestInstance::FILLRULECASE_PROJECTED,		samples[samplesNdx]));
		}

		// .interpolation
		{
			tcu::TestCaseGroup* const interpolation = new tcu::TestCaseGroup(testCtx, ("interpolation" + caseName.str()).c_str(), "Test interpolation");

			rasterizationTests->addChild(interpolation);

			interpolation->addChild(new TriangleInterpolationTestCase		(testCtx, "triangles",		"Verify triangle interpolation",		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,	INTERPOLATIONFLAGS_NONE,								samples[samplesNdx]));
			interpolation->addChild(new LineInterpolationTestCase			(testCtx, "lines",			"Verify line interpolation",			VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		INTERPOLATIONFLAGS_NONE,	PRIMITIVEWIDENESS_NARROW,	samples[samplesNdx]));
			interpolation->addChild(new LineInterpolationTestCase			(testCtx, "lines_wide",		"Verify wide line interpolation",		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		INTERPOLATIONFLAGS_NONE,	PRIMITIVEWIDENESS_WIDE,		samples[samplesNdx]));
		}
	}
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "rasterization", "Rasterization Tests", createRasterizationTests);
}

} // rasterization
} // vkt
