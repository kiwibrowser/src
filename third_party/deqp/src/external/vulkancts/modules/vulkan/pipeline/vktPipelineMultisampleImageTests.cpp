/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Multisample image Tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineMultisampleImageTests.hpp"
#include "vktPipelineMakeUtil.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "vktTestGroupUtil.hpp"

#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkPrograms.hpp"
#include "vkImageUtil.hpp"

#include "tcuTextureUtil.hpp"
#include "tcuTestLog.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"

#include <string>

namespace vkt
{
namespace pipeline
{
namespace
{
using namespace vk;
using de::UniquePtr;
using de::MovePtr;
using de::SharedPtr;
using tcu::IVec2;
using tcu::Vec4;

typedef SharedPtr<Unique<VkImageView> >	ImageViewSp;
typedef SharedPtr<Unique<VkPipeline> >	PipelineSp;

//! Test case parameters
struct CaseDef
{
	IVec2					renderSize;
	int						numLayers;
	VkFormat				colorFormat;
	VkSampleCountFlagBits	numSamples;
};

template<typename T>
inline SharedPtr<Unique<T> > makeSharedPtr (Move<T> move)
{
	return SharedPtr<Unique<T> >(new Unique<T>(move));
}

template<typename T>
inline VkDeviceSize sizeInBytes(const std::vector<T>& vec)
{
	return vec.size() * sizeof(vec[0]);
}

//! Create a vector of derived pipelines, each with an increasing subpass index
std::vector<PipelineSp> makeGraphicsPipelines (const DeviceInterface&		vk,
											   const VkDevice				device,
											   const deUint32				numSubpasses,
											   const VkPipelineLayout		pipelineLayout,
											   const VkRenderPass			renderPass,
											   const VkShaderModule			vertexModule,
											   const VkShaderModule			fragmentModule,
											   const IVec2					renderSize,
											   const VkSampleCountFlagBits	numSamples,
											   const VkPrimitiveTopology	topology)
{
	const VkVertexInputBindingDescription vertexInputBindingDescription =
	{
		0u,								// uint32_t				binding;
		sizeof(Vertex4RGBA),			// uint32_t				stride;
		VK_VERTEX_INPUT_RATE_VERTEX,	// VkVertexInputRate	inputRate;
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
	{
		{
			0u,									// uint32_t			location;
			0u,									// uint32_t			binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat			format;
			0u,									// uint32_t			offset;
		},
		{
			1u,									// uint32_t			location;
			0u,									// uint32_t			binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat			format;
			sizeof(Vec4),						// uint32_t			offset;
		},
	};

	const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineVertexInputStateCreateFlags)0,						// VkPipelineVertexInputStateCreateFlags       flags;
		1u,																// uint32_t                                    vertexBindingDescriptionCount;
		&vertexInputBindingDescription,									// const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
		DE_LENGTH_OF_ARRAY(vertexInputAttributeDescriptions),			// uint32_t                                    vertexAttributeDescriptionCount;
		vertexInputAttributeDescriptions,								// const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
	};

	const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0,						// VkPipelineInputAssemblyStateCreateFlags     flags;
		topology,														// VkPrimitiveTopology                         topology;
		VK_FALSE,														// VkBool32                                    primitiveRestartEnable;
	};

	const VkViewport viewport = makeViewport(
		0.0f, 0.0f,
		static_cast<float>(renderSize.x()), static_cast<float>(renderSize.y()),
		0.0f, 1.0f);

	const VkRect2D scissor = {
		makeOffset2D(0, 0),
		makeExtent2D(renderSize.x(), renderSize.y()),
	};

	const VkPipelineViewportStateCreateInfo pipelineViewportStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineViewportStateCreateFlags)0,							// VkPipelineViewportStateCreateFlags          flags;
		1u,																// uint32_t                                    viewportCount;
		&viewport,														// const VkViewport*                           pViewports;
		1u,																// uint32_t                                    scissorCount;
		&scissor,														// const VkRect2D*                             pScissors;
	};

	const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType                          sType;
		DE_NULL,														// const void*                              pNext;
		(VkPipelineRasterizationStateCreateFlags)0,						// VkPipelineRasterizationStateCreateFlags  flags;
		VK_FALSE,														// VkBool32                                 depthClampEnable;
		VK_FALSE,														// VkBool32                                 rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
		VK_CULL_MODE_NONE,												// VkCullModeFlags							cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
		VK_FALSE,														// VkBool32									depthBiasEnable;
		0.0f,															// float									depthBiasConstantFactor;
		0.0f,															// float									depthBiasClamp;
		0.0f,															// float									depthBiasSlopeFactor;
		1.0f,															// float									lineWidth;
	};

	const VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineMultisampleStateCreateFlags)0,						// VkPipelineMultisampleStateCreateFlags	flags;
		numSamples,														// VkSampleCountFlagBits					rasterizationSamples;
		VK_FALSE,														// VkBool32									sampleShadingEnable;
		0.0f,															// float									minSampleShading;
		DE_NULL,														// const VkSampleMask*						pSampleMask;
		VK_FALSE,														// VkBool32									alphaToCoverageEnable;
		VK_FALSE														// VkBool32									alphaToOneEnable;
	};

	const VkStencilOpState stencilOpState = makeStencilOpState(
		VK_STENCIL_OP_KEEP,				// stencil fail
		VK_STENCIL_OP_KEEP,				// depth & stencil pass
		VK_STENCIL_OP_KEEP,				// depth only fail
		VK_COMPARE_OP_ALWAYS,			// compare op
		0u,								// compare mask
		0u,								// write mask
		0u);							// reference

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineDepthStencilStateCreateFlags)0,						// VkPipelineDepthStencilStateCreateFlags	flags;
		VK_FALSE,														// VkBool32									depthTestEnable;
		VK_FALSE,														// VkBool32									depthWriteEnable;
		VK_COMPARE_OP_LESS,												// VkCompareOp								depthCompareOp;
		VK_FALSE,														// VkBool32									depthBoundsTestEnable;
		VK_FALSE,														// VkBool32									stencilTestEnable;
		stencilOpState,													// VkStencilOpState							front;
		stencilOpState,													// VkStencilOpState							back;
		0.0f,															// float									minDepthBounds;
		1.0f,															// float									maxDepthBounds;
	};

	const VkColorComponentFlags colorComponentsAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// Number of blend attachments must equal the number of color attachments during any subpass.
	const VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState =
	{
		VK_FALSE,						// VkBool32					blendEnable;
		VK_BLEND_FACTOR_ONE,			// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,			// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,				// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_ONE,			// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,			// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,				// VkBlendOp				alphaBlendOp;
		colorComponentsAll,				// VkColorComponentFlags	colorWriteMask;
	};

	const VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,		// VkStructureType								sType;
		DE_NULL,														// const void*									pNext;
		(VkPipelineColorBlendStateCreateFlags)0,						// VkPipelineColorBlendStateCreateFlags			flags;
		VK_FALSE,														// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,												// VkLogicOp									logicOp;
		1u,																// deUint32										attachmentCount;
		&pipelineColorBlendAttachmentState,								// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },										// float										blendConstants[4];
	};

	const VkPipelineShaderStageCreateInfo pShaderStages[] =
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
			DE_NULL,													// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,						// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStageFlagBits				stage;
			vertexModule,												// VkShaderModule						module;
			"main",														// const char*							pName;
			DE_NULL,													// const VkSpecializationInfo*			pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
			DE_NULL,													// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,						// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlagBits				stage;
			fragmentModule,												// VkShaderModule						module;
			"main",														// const char*							pName;
			DE_NULL,													// const VkSpecializationInfo*			pSpecializationInfo;
		}
	};

	DE_ASSERT(numSubpasses > 0u);

	std::vector<VkGraphicsPipelineCreateInfo>	graphicsPipelineInfos	(0);
	std::vector<VkPipeline>						rawPipelines			(numSubpasses, DE_NULL);

	{
		const VkPipelineCreateFlags firstPipelineFlags = (numSubpasses > 1u ? VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT
																			: (VkPipelineCreateFlagBits)0);

		VkGraphicsPipelineCreateInfo createInfo =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
			DE_NULL,											// const void*										pNext;
			firstPipelineFlags,									// VkPipelineCreateFlags							flags;
			DE_LENGTH_OF_ARRAY(pShaderStages),					// deUint32											stageCount;
			pShaderStages,										// const VkPipelineShaderStageCreateInfo*			pStages;
			&vertexInputStateInfo,								// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
			&pipelineInputAssemblyStateInfo,					// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
			DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
			&pipelineViewportStateInfo,							// const VkPipelineViewportStateCreateInfo*			pViewportState;
			&pipelineRasterizationStateInfo,					// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
			&pipelineMultisampleStateInfo,						// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
			&pipelineDepthStencilStateInfo,						// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
			&pipelineColorBlendStateInfo,						// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
			DE_NULL,											// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
			pipelineLayout,										// VkPipelineLayout									layout;
			renderPass,											// VkRenderPass										renderPass;
			0u,													// deUint32											subpass;
			DE_NULL,											// VkPipeline										basePipelineHandle;
			-1,													// deInt32											basePipelineIndex;
		};

		graphicsPipelineInfos.push_back(createInfo);

		createInfo.flags				= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
		createInfo.basePipelineIndex	= 0;

		for (deUint32 subpassNdx = 1u; subpassNdx < numSubpasses; ++subpassNdx)
		{
			createInfo.subpass = subpassNdx;
			graphicsPipelineInfos.push_back(createInfo);
		}
	}

	VK_CHECK(vk.createGraphicsPipelines(device, DE_NULL, static_cast<deUint32>(graphicsPipelineInfos.size()), &graphicsPipelineInfos[0], DE_NULL, &rawPipelines[0]));

	std::vector<PipelineSp>	pipelines;

	for (std::vector<VkPipeline>::const_iterator it = rawPipelines.begin(); it != rawPipelines.end(); ++it)
		pipelines.push_back(makeSharedPtr(Move<VkPipeline>(check<VkPipeline>(*it), Deleter<VkPipeline>(vk, device, DE_NULL))));

	return pipelines;
}

//! Make a render pass with one subpass per color attachment and one attachment per image layer.
Move<VkRenderPass> makeMultisampleRenderPass (const DeviceInterface&		vk,
											  const VkDevice				device,
											  const VkFormat				colorFormat,
											  const VkSampleCountFlagBits	numSamples,
											  const deUint32				numLayers)
{
	const VkAttachmentDescription colorAttachmentDescription =
	{
		(VkAttachmentDescriptionFlags)0,					// VkAttachmentDescriptionFlags		flags;
		colorFormat,										// VkFormat							format;
		numSamples,											// VkSampleCountFlagBits			samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
		VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					finalLayout;
	};
	const std::vector<VkAttachmentDescription> attachmentDescriptions(numLayers, colorAttachmentDescription);

	// Create a subpass for each attachment (each attachement is a layer of an arrayed image).

	std::vector<VkAttachmentReference>	colorAttachmentReferences(numLayers);
	std::vector<VkSubpassDescription>	subpasses;

	for (deUint32 i = 0; i < numLayers; ++i)
	{
		const VkAttachmentReference attachmentRef =
		{
			i,												// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL		// VkImageLayout	layout;
		};
		colorAttachmentReferences[i] = attachmentRef;

		const VkSubpassDescription subpassDescription =
		{
			(VkSubpassDescriptionFlags)0,					// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,				// VkPipelineBindPoint				pipelineBindPoint;
			0u,												// deUint32							inputAttachmentCount;
			DE_NULL,										// const VkAttachmentReference*		pInputAttachments;
			1u,												// deUint32							colorAttachmentCount;
			&colorAttachmentReferences[i],					// const VkAttachmentReference*		pColorAttachments;
			DE_NULL,										// const VkAttachmentReference*		pResolveAttachments;
			DE_NULL,										// const VkAttachmentReference*		pDepthStencilAttachment;
			0u,												// deUint32							preserveAttachmentCount;
			DE_NULL											// const deUint32*					pPreserveAttachments;
		};
		subpasses.push_back(subpassDescription);
	}

	const VkRenderPassCreateInfo renderPassInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,				// VkStructureType					sType;
		DE_NULL,												// const void*						pNext;
		(VkRenderPassCreateFlags)0,								// VkRenderPassCreateFlags			flags;
		static_cast<deUint32>(attachmentDescriptions.size()),	// deUint32							attachmentCount;
		&attachmentDescriptions[0],								// const VkAttachmentDescription*	pAttachments;
		static_cast<deUint32>(subpasses.size()),				// deUint32							subpassCount;
		&subpasses[0],											// const VkSubpassDescription*		pSubpasses;
		0u,														// deUint32							dependencyCount;
		DE_NULL													// const VkSubpassDependency*		pDependencies;
	};

	return createRenderPass(vk, device, &renderPassInfo);
}

//! A single-attachment, single-subpass render pass.
Move<VkRenderPass> makeSimpleRenderPass (const DeviceInterface&	vk,
									     const VkDevice			device,
										 const VkFormat			colorFormat)
{
	const VkAttachmentDescription colorAttachmentDescription =
	{
		(VkAttachmentDescriptionFlags)0,					// VkAttachmentDescriptionFlags		flags;
		colorFormat,										// VkFormat							format;
		VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
		VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					finalLayout;
	};

	const VkAttachmentReference colorAttachmentRef =
	{
		0u,													// deUint32			attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
	};

	const VkSubpassDescription subpassDescription =
	{
		(VkSubpassDescriptionFlags)0,						// VkSubpassDescriptionFlags		flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint				pipelineBindPoint;
		0u,													// deUint32							inputAttachmentCount;
		DE_NULL,											// const VkAttachmentReference*		pInputAttachments;
		1u,													// deUint32							colorAttachmentCount;
		&colorAttachmentRef,								// const VkAttachmentReference*		pColorAttachments;
		DE_NULL,											// const VkAttachmentReference*		pResolveAttachments;
		DE_NULL,											// const VkAttachmentReference*		pDepthStencilAttachment;
		0u,													// deUint32							preserveAttachmentCount;
		DE_NULL												// const deUint32*					pPreserveAttachments;
	};

	const VkRenderPassCreateInfo renderPassInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		(VkRenderPassCreateFlags)0,							// VkRenderPassCreateFlags			flags;
		1u,													// deUint32							attachmentCount;
		&colorAttachmentDescription,						// const VkAttachmentDescription*	pAttachments;
		1u,													// deUint32							subpassCount;
		&subpassDescription,								// const VkSubpassDescription*		pSubpasses;
		0u,													// deUint32							dependencyCount;
		DE_NULL												// const VkSubpassDependency*		pDependencies;
	};

	return createRenderPass(vk, device, &renderPassInfo);
}

Move<VkImage> makeImage (const DeviceInterface& vk, const VkDevice device, const VkFormat format, const IVec2& size, const deUint32 numLayers, const VkSampleCountFlagBits samples, const VkImageUsageFlags usage)
{
	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		(VkImageCreateFlags)0,							// VkImageCreateFlags		flags;
		VK_IMAGE_TYPE_2D,								// VkImageType				imageType;
		format,											// VkFormat					format;
		makeExtent3D(size.x(), size.y(), 1),			// VkExtent3D				extent;
		1u,												// deUint32					mipLevels;
		numLayers,										// deUint32					arrayLayers;
		samples,										// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,						// VkImageTiling			tiling;
		usage,											// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,						// VkSharingMode			sharingMode;
		0u,												// deUint32					queueFamilyIndexCount;
		DE_NULL,										// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			initialLayout;
	};
	return createImage(vk, device, &imageParams);
}

//! Make a simplest sampler.
Move<VkSampler> makeSampler (const DeviceInterface& vk, const VkDevice device)
{
	const VkSamplerCreateInfo samplerParams =
	{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,			// VkStructureType         sType;
		DE_NULL,										// const void*             pNext;
		(VkSamplerCreateFlags)0,						// VkSamplerCreateFlags    flags;
		VK_FILTER_NEAREST,								// VkFilter                magFilter;
		VK_FILTER_NEAREST,								// VkFilter                minFilter;
		VK_SAMPLER_MIPMAP_MODE_NEAREST,					// VkSamplerMipmapMode     mipmapMode;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,			// VkSamplerAddressMode    addressModeU;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,			// VkSamplerAddressMode    addressModeV;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,			// VkSamplerAddressMode    addressModeW;
		0.0f,											// float                   mipLodBias;
		VK_FALSE,										// VkBool32                anisotropyEnable;
		1.0f,											// float                   maxAnisotropy;
		VK_FALSE,										// VkBool32                compareEnable;
		VK_COMPARE_OP_ALWAYS,							// VkCompareOp             compareOp;
		0.0f,											// float                   minLod;
		0.0f,											// float                   maxLod;
		VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,		// VkBorderColor           borderColor;
		VK_FALSE,										// VkBool32                unnormalizedCoordinates;
	};
	return createSampler(vk, device, &samplerParams);
}

inline Move<VkBuffer> makeBuffer (const DeviceInterface& vk, const VkDevice device, const VkDeviceSize bufferSize, const VkBufferUsageFlags usage)
{
	const VkBufferCreateInfo bufferCreateInfo = makeBufferCreateInfo(bufferSize, usage);
	return createBuffer(vk, device, &bufferCreateInfo);
}

inline VkImageSubresourceRange makeColorSubresourceRange (const int baseArrayLayer, const int layerCount)
{
	return makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, static_cast<deUint32>(baseArrayLayer), static_cast<deUint32>(layerCount));
}

inline VkImageSubresourceLayers makeColorSubresourceLayers (const int baseArrayLayer, const int layerCount)
{
	return makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, static_cast<deUint32>(baseArrayLayer), static_cast<deUint32>(layerCount));
}

void checkImageFormatRequirements (const InstanceInterface&		vki,
								   const VkPhysicalDevice		physDevice,
								   const VkSampleCountFlagBits	sampleCount,
								   const VkFormat				format,
								   const VkImageUsageFlags		usage)
{
	VkPhysicalDeviceFeatures	features;
	vki.getPhysicalDeviceFeatures(physDevice, &features);

	if (((usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0) && !features.shaderStorageImageMultisample)
		TCU_THROW(NotSupportedError, "Multisampled storage images are not supported");

	VkImageFormatProperties		imageFormatProperties;
	const VkResult				imageFormatResult		= vki.getPhysicalDeviceImageFormatProperties(
		physDevice, format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, (VkImageCreateFlags)0, &imageFormatProperties);

	if (imageFormatResult == VK_ERROR_FORMAT_NOT_SUPPORTED)
		TCU_THROW(NotSupportedError, "Image format is not supported");

	if ((imageFormatProperties.sampleCounts & sampleCount) != sampleCount)
		TCU_THROW(NotSupportedError, "Requested sample count is not supported");
}

void zeroBuffer (const DeviceInterface& vk, const VkDevice device, const Allocation& alloc, const VkDeviceSize bufferSize)
{
	deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(bufferSize));
	flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), bufferSize);
}

//! The default foreground color.
inline Vec4 getPrimitiveColor (void)
{
	return Vec4(1.0f, 0.0f, 0.0f, 1.0f);
}

//! Get a reference clear value based on color format.
VkClearValue getClearValue (const VkFormat format)
{
	if (isUintFormat(format) || isIntFormat(format))
		return makeClearValueColorU32(16, 32, 64, 96);
	else
		return makeClearValueColorF32(0.0f, 0.0f, 1.0f, 1.0f);
}

std::string getColorFormatStr (const int numComponents, const bool isUint, const bool isSint)
{
	std::ostringstream str;
	if (numComponents == 1)
		str << (isUint ? "uint" : isSint ? "int" : "float");
	else
		str << (isUint ? "u" : isSint ? "i" : "") << "vec" << numComponents;

	return str.str();
}

std::string getSamplerTypeStr (const int numLayers, const bool isUint, const bool isSint)
{
	std::ostringstream str;
	str << (isUint ? "u" : isSint ? "i" : "") << "sampler2DMS" << (numLayers > 1 ? "Array" : "");
	return str.str();
}

//! Generate a gvec4 color literal.
template<typename T>
std::string getColorStr (const T* data, int numComponents, const bool isUint, const bool isSint)
{
	const int maxIndex = 3;  // 4 components max

	std::ostringstream str;
	str << (isUint ? "u" : isSint ? "i" : "") << "vec4(";

	for (int i = 0; i < numComponents; ++i)
	{
		str << data[i]
			<< (i < maxIndex ? ", " : "");
	}

	for (int i = numComponents; i < maxIndex + 1; ++i)
	{
		str << (i == maxIndex ? 1 : 0)
			<< (i <  maxIndex ? ", " : "");
	}

	str << ")";
	return str.str();
}

//! Clear color literal value used by the sampling shader.
std::string getReferenceClearColorStr (const VkFormat format, const int numComponents, const bool isUint, const bool isSint)
{
	const VkClearColorValue clearColor = getClearValue(format).color;
	if (isUint)
		return getColorStr(clearColor.uint32, numComponents, isUint, isSint);
	else if (isSint)
		return getColorStr(clearColor.int32, numComponents, isUint, isSint);
	else
		return getColorStr(clearColor.float32, numComponents, isUint, isSint);
}

//! Primitive color literal value used by the sampling shader.
std::string getReferencePrimitiveColorStr (int numComponents, const bool isUint, const bool isSint)
{
	const Vec4 color = getPrimitiveColor();
	return getColorStr(color.getPtr(), numComponents, isUint, isSint);
}

inline int getNumSamples (const VkSampleCountFlagBits samples)
{
	return static_cast<int>(samples);	// enum bitmask actually matches the number of samples
}

//! A flat-colored shape with sharp angles to make antialiasing visible.
std::vector<Vertex4RGBA> genTriangleVertices (void)
{
	static const Vertex4RGBA data[] =
	{
		{
			Vec4(-1.0f, 0.0f, 0.0f, 1.0f),
			getPrimitiveColor(),
		},
		{
			Vec4(0.8f, 0.2f, 0.0f, 1.0f),
			getPrimitiveColor(),
		},
		{
			Vec4(0.8f, -0.2f, 0.0f, 1.0f),
			getPrimitiveColor(),
		},
	};
	return std::vector<Vertex4RGBA>(data, data + DE_LENGTH_OF_ARRAY(data));
}

//! A full-viewport quad. Use with TRIANGLE_STRIP topology.
std::vector<Vertex4RGBA> genFullQuadVertices (void)
{
	static const Vertex4RGBA data[] =
	{
		{
			Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
			Vec4(), // unused
		},
		{
			Vec4(-1.0f, 1.0f, 0.0f, 1.0f),
			Vec4(), // unused
		},
		{
			Vec4(1.0f, -1.0f, 0.0f, 1.0f),
			Vec4(), // unused
		},
		{
			Vec4(1.0f, 1.0f, 0.0f, 1.0f),
			Vec4(), // unused
		},
	};
	return std::vector<Vertex4RGBA>(data, data + DE_LENGTH_OF_ARRAY(data));
}

std::string getShaderImageFormatQualifier (const tcu::TextureFormat& format)
{
	const char* orderPart;
	const char* typePart;

	switch (format.order)
	{
		case tcu::TextureFormat::R:		orderPart = "r";	break;
		case tcu::TextureFormat::RG:	orderPart = "rg";	break;
		case tcu::TextureFormat::RGB:	orderPart = "rgb";	break;
		case tcu::TextureFormat::RGBA:	orderPart = "rgba";	break;

		default:
			DE_ASSERT(false);
			orderPart = DE_NULL;
	}

	switch (format.type)
	{
		case tcu::TextureFormat::FLOAT:				typePart = "32f";		break;
		case tcu::TextureFormat::HALF_FLOAT:		typePart = "16f";		break;

		case tcu::TextureFormat::UNSIGNED_INT32:	typePart = "32ui";		break;
		case tcu::TextureFormat::UNSIGNED_INT16:	typePart = "16ui";		break;
		case tcu::TextureFormat::UNSIGNED_INT8:		typePart = "8ui";		break;

		case tcu::TextureFormat::SIGNED_INT32:		typePart = "32i";		break;
		case tcu::TextureFormat::SIGNED_INT16:		typePart = "16i";		break;
		case tcu::TextureFormat::SIGNED_INT8:		typePart = "8i";		break;

		case tcu::TextureFormat::UNORM_INT16:		typePart = "16";		break;
		case tcu::TextureFormat::UNORM_INT8:		typePart = "8";			break;

		case tcu::TextureFormat::SNORM_INT16:		typePart = "16_snorm";	break;
		case tcu::TextureFormat::SNORM_INT8:		typePart = "8_snorm";	break;

		default:
			DE_ASSERT(false);
			typePart = DE_NULL;
	}

	return std::string() + orderPart + typePart;
}

std::string getShaderMultisampledImageType (const tcu::TextureFormat& format, const int numLayers)
{
	const std::string formatPart = tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER ? "u" :
								   tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER   ? "i" : "";

	std::ostringstream str;
	str << formatPart << "image2DMS" << (numLayers > 1 ? "Array" : "");

	return str.str();
}

void addSimpleVertexAndFragmentPrograms (SourceCollections& programCollection, const CaseDef caseDef)
{
	const int	numComponents	= tcu::getNumUsedChannels(mapVkFormat(caseDef.colorFormat).order);
	const bool	isUint			= isUintFormat(caseDef.colorFormat);
	const bool	isSint			= isIntFormat(caseDef.colorFormat);

	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in  vec4 in_position;\n"
			<< "layout(location = 1) in  vec4 in_color;\n"
			<< "layout(location = 0) out vec4 o_color;\n"
			<< "\n"
			<< "out gl_PerVertex {\n"
			<< "    vec4 gl_Position;\n"
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    gl_Position = in_position;\n"
			<< "    o_color     = in_color;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		const std::string colorFormat = getColorFormatStr(numComponents, isUint, isSint);

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in  vec4 in_color;\n"
			<< "layout(location = 0) out " << colorFormat << " o_color;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    o_color = " << colorFormat << "("		// float color will be converted to int/uint here if needed
			<< (numComponents == 1 ? "in_color.r"   :
				numComponents == 2 ? "in_color.rg"  :
				numComponents == 3 ? "in_color.rgb" : "in_color") << ");\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

//! Synchronously render to a multisampled color image.
void renderMultisampledImage (Context& context, const CaseDef& caseDef, const VkImage colorImage)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&						allocator			= context.getDefaultAllocator();

	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));

	const VkRect2D renderArea = {
		makeOffset2D(0, 0),
		makeExtent2D(caseDef.renderSize.x(), caseDef.renderSize.y()),
	};

	{
		// Create an image view (attachment) for each layer of the image
		std::vector<ImageViewSp>	colorAttachments;
		std::vector<VkImageView>	attachmentHandles;
		for (int i = 0; i < caseDef.numLayers; ++i)
		{
			colorAttachments.push_back(makeSharedPtr(makeImageView(
				vk, device, colorImage, VK_IMAGE_VIEW_TYPE_2D, caseDef.colorFormat, makeColorSubresourceRange(i, 1))));
			attachmentHandles.push_back(**colorAttachments.back());
		}

		// Vertex buffer
		const std::vector<Vertex4RGBA>	vertices			= genTriangleVertices();
		const VkDeviceSize				vertexBufferSize	= sizeInBytes(vertices);
		const Unique<VkBuffer>			vertexBuffer		(makeBuffer(vk, device, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
		const UniquePtr<Allocation>		vertexBufferAlloc	(bindBuffer(vk, device, allocator, *vertexBuffer, MemoryRequirement::HostVisible));

		{
			deMemcpy(vertexBufferAlloc->getHostPtr(), &vertices[0], static_cast<std::size_t>(vertexBufferSize));
			flushMappedMemoryRange(vk, device, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset(), vertexBufferSize);
		}

		const Unique<VkShaderModule>	vertexModule	(createShaderModule			(vk, device, context.getBinaryCollection().get("vert"), 0u));
		const Unique<VkShaderModule>	fragmentModule	(createShaderModule			(vk, device, context.getBinaryCollection().get("frag"), 0u));
		const Unique<VkRenderPass>		renderPass		(makeMultisampleRenderPass	(vk, device, caseDef.colorFormat, caseDef.numSamples, caseDef.numLayers));
		const Unique<VkFramebuffer>		framebuffer		(makeFramebuffer			(vk, device, *renderPass, caseDef.numLayers, &attachmentHandles[0],
																					 static_cast<deUint32>(caseDef.renderSize.x()),  static_cast<deUint32>(caseDef.renderSize.y())));
		const Unique<VkPipelineLayout>	pipelineLayout	(makePipelineLayout			(vk, device));
		const std::vector<PipelineSp>	pipelines		(makeGraphicsPipelines		(vk, device, caseDef.numLayers, *pipelineLayout, *renderPass, *vertexModule, *fragmentModule,
																					 caseDef.renderSize, caseDef.numSamples, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST));

		beginCommandBuffer(vk, *cmdBuffer);

		const std::vector<VkClearValue> clearValues(caseDef.numLayers, getClearValue(caseDef.colorFormat));

		const VkRenderPassBeginInfo renderPassBeginInfo = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,		// VkStructureType         sType;
			DE_NULL,										// const void*             pNext;
			*renderPass,									// VkRenderPass            renderPass;
			*framebuffer,									// VkFramebuffer           framebuffer;
			renderArea,										// VkRect2D                renderArea;
			static_cast<deUint32>(clearValues.size()),		// uint32_t                clearValueCount;
			&clearValues[0],								// const VkClearValue*     pClearValues;
		};
		vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		{
			const VkDeviceSize vertexBufferOffset = 0ull;
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
		}

		for (int layerNdx = 0; layerNdx < caseDef.numLayers; ++layerNdx)
		{
			if (layerNdx != 0)
				vk.cmdNextSubpass(*cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **pipelines[layerNdx]);

			vk.cmdDraw(*cmdBuffer, static_cast<deUint32>(vertices.size()), 1u, 0u, 0u);
		}

		vk.cmdEndRenderPass(*cmdBuffer);

		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}
}

namespace SampledImage
{

void initPrograms (SourceCollections& programCollection, const CaseDef caseDef)
{
	// Pass 1: Render to texture

	addSimpleVertexAndFragmentPrograms(programCollection, caseDef);

	// Pass 2: Sample texture

	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in  vec4  in_position;\n"
			<< "\n"
			<< "out gl_PerVertex {\n"
			<< "    vec4 gl_Position;\n"
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    gl_Position = in_position;\n"
			<< "}\n";

		programCollection.glslSources.add("sample_vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		const int			numComponents		= tcu::getNumUsedChannels(mapVkFormat(caseDef.colorFormat).order);
		const bool			isUint				= isUintFormat(caseDef.colorFormat);
		const bool			isSint				= isIntFormat(caseDef.colorFormat);
		const std::string	texelFormatStr		= (isUint ? "uvec4" : isSint ? "ivec4" : "vec4");
		const std::string	refClearColor		= getReferenceClearColorStr(caseDef.colorFormat, numComponents, isUint, isSint);
		const std::string	refPrimitiveColor	= getReferencePrimitiveColorStr(numComponents, isUint, isSint);
		const std::string	samplerTypeStr		= getSamplerTypeStr(caseDef.numLayers, isUint, isSint);

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) out int o_status;\n"
			<< "\n"
			<< "layout(set = 0, binding = 0) uniform " << samplerTypeStr << " colorTexture;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    int checksum = 0;\n"
			<< "\n";

		if (caseDef.numLayers == 1)
			src << "    for (int sampleNdx = 0; sampleNdx < " << caseDef.numSamples << "; ++sampleNdx) {\n"
				<< "        " << texelFormatStr << " color = texelFetch(colorTexture, ivec2(gl_FragCoord.xy), sampleNdx);\n"
				<< "        if (color == " << refClearColor << " || color == " << refPrimitiveColor << ")\n"
				<< "            ++checksum;\n"
				<< "    }\n";
		else
			src << "    for (int layerNdx = 0; layerNdx < " << caseDef.numLayers << "; ++layerNdx)\n"
				<< "    for (int sampleNdx = 0; sampleNdx < " << caseDef.numSamples << "; ++sampleNdx) {\n"
				<< "        " << texelFormatStr << " color = texelFetch(colorTexture, ivec3(gl_FragCoord.xy, layerNdx), sampleNdx);\n"
				<< "        if (color == " << refClearColor << " || color == " << refPrimitiveColor << ")\n"
				<< "            ++checksum;\n"
				<< "    }\n";

		src << "\n"
			<< "    o_status = checksum;\n"
			<< "}\n";

		programCollection.glslSources.add("sample_frag") << glu::FragmentSource(src.str());
	}
}

tcu::TestStatus test (Context& context, const CaseDef caseDef)
{
	const DeviceInterface&		vk					= context.getDeviceInterface();
	const InstanceInterface&	vki					= context.getInstanceInterface();
	const VkDevice				device				= context.getDevice();
	const VkPhysicalDevice		physDevice			= context.getPhysicalDevice();
	const VkQueue				queue				= context.getUniversalQueue();
	const deUint32				queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&					allocator			= context.getDefaultAllocator();

	const VkImageUsageFlags		colorImageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	checkImageFormatRequirements(vki, physDevice, caseDef.numSamples, caseDef.colorFormat, colorImageUsage);

	{
		tcu::TestLog& log = context.getTestContext().getLog();
		log << tcu::LogSection("Description", "")
			<< tcu::TestLog::Message << "Rendering to a multisampled image. Expecting all samples to be either a clear color or a primitive color." << tcu::TestLog::EndMessage
			<< tcu::TestLog::Message << "Sampling from the texture with texelFetch (OpImageFetch)." << tcu::TestLog::EndMessage
			<< tcu::TestLog::EndSection;
	}

	// Multisampled color image
	const Unique<VkImage>			colorImage		(makeImage(vk, device, caseDef.colorFormat, caseDef.renderSize, caseDef.numLayers, caseDef.numSamples, colorImageUsage));
	const UniquePtr<Allocation>		colorImageAlloc	(bindImage(vk, device, allocator, *colorImage, MemoryRequirement::Any));

	const Unique<VkCommandPool>		cmdPool			(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer		(makeCommandBuffer(vk, device, *cmdPool));

	const VkRect2D renderArea = {
		makeOffset2D(0, 0),
		makeExtent2D(caseDef.renderSize.x(), caseDef.renderSize.y()),
	};

	// Step 1: Render to texture
	{
		renderMultisampledImage(context, caseDef, *colorImage);
	}

	// Step 2: Sample texture
	{
		// Color image view
		const VkImageViewType			colorImageViewType	= (caseDef.numLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY);
		const Unique<VkImageView>		colorImageView		(makeImageView(vk, device, *colorImage, colorImageViewType, caseDef.colorFormat, makeColorSubresourceRange(0, caseDef.numLayers)));
		const Unique<VkSampler>			colorSampler		(makeSampler(vk, device));

		// Checksum image
		const VkFormat					checksumFormat		= VK_FORMAT_R32_SINT;
		const Unique<VkImage>			checksumImage		(makeImage(vk, device, checksumFormat, caseDef.renderSize, 1u, VK_SAMPLE_COUNT_1_BIT,
																	   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
		const UniquePtr<Allocation>		checksumImageAlloc	(bindImage(vk, device, allocator, *checksumImage, MemoryRequirement::Any));
		const Unique<VkImageView>		checksumImageView	(makeImageView(vk, device, *checksumImage, VK_IMAGE_VIEW_TYPE_2D, checksumFormat, makeColorSubresourceRange(0, 1)));

		// Checksum buffer (for host reading)
		const VkDeviceSize				checksumBufferSize	= caseDef.renderSize.x() * caseDef.renderSize.y() * tcu::getPixelSize(mapVkFormat(checksumFormat));
		const Unique<VkBuffer>			checksumBuffer		(makeBuffer(vk, device, checksumBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
		const UniquePtr<Allocation>		checksumBufferAlloc	(bindBuffer(vk, device, allocator, *checksumBuffer, MemoryRequirement::HostVisible));

		zeroBuffer(vk, device, *checksumBufferAlloc, checksumBufferSize);

		// Vertex buffer
		const std::vector<Vertex4RGBA>	vertices			= genFullQuadVertices();
		const VkDeviceSize				vertexBufferSize	= sizeInBytes(vertices);
		const Unique<VkBuffer>			vertexBuffer		(makeBuffer(vk, device, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
		const UniquePtr<Allocation>		vertexBufferAlloc	(bindBuffer(vk, device, allocator, *vertexBuffer, MemoryRequirement::HostVisible));

		{
			deMemcpy(vertexBufferAlloc->getHostPtr(), &vertices[0], static_cast<std::size_t>(vertexBufferSize));
			flushMappedMemoryRange(vk, device, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset(), vertexBufferSize);
		}

		// Descriptors
		// \note OpImageFetch doesn't use a sampler, but in GLSL texelFetch needs a sampler2D which translates to a combined image sampler in Vulkan.

		const Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
			.addSingleSamplerBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &colorSampler.get())
			.build(vk, device));

		const Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
			.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

		const Unique<VkDescriptorSet>	descriptorSet		(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));
		const VkDescriptorImageInfo		imageDescriptorInfo	= makeDescriptorImageInfo(DE_NULL, *colorImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		DescriptorSetUpdateBuilder()
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageDescriptorInfo)
			.update(vk, device);

		const Unique<VkShaderModule>	vertexModule	(createShaderModule		(vk, device, context.getBinaryCollection().get("sample_vert"), 0u));
		const Unique<VkShaderModule>	fragmentModule	(createShaderModule		(vk, device, context.getBinaryCollection().get("sample_frag"), 0u));
		const Unique<VkRenderPass>		renderPass		(makeSimpleRenderPass	(vk, device, checksumFormat));
		const Unique<VkFramebuffer>		framebuffer		(makeFramebuffer		(vk, device, *renderPass, 1u, &checksumImageView.get(),
																				 static_cast<deUint32>(caseDef.renderSize.x()),  static_cast<deUint32>(caseDef.renderSize.y())));
		const Unique<VkPipelineLayout>	pipelineLayout	(makePipelineLayout		(vk, device, *descriptorSetLayout));
		const std::vector<PipelineSp>	pipelines		(makeGraphicsPipelines	(vk, device, 1u, *pipelineLayout, *renderPass, *vertexModule, *fragmentModule,
																				 caseDef.renderSize, VK_SAMPLE_COUNT_1_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP));

		beginCommandBuffer(vk, *cmdBuffer);

		// Prepare for sampling in the fragment shader
		{
			const VkImageMemoryBarrier barriers[] =
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,						// VkStructureType			sType;
					DE_NULL,													// const void*				pNext;
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,						// VkAccessFlags			outputMask;
					VK_ACCESS_SHADER_READ_BIT,									// VkAccessFlags			inputMask;
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,					// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,					// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					destQueueFamilyIndex;
					*colorImage,												// VkImage					image;
					makeColorSubresourceRange(0, caseDef.numLayers),			// VkImageSubresourceRange	subresourceRange;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
		}

		const VkClearValue clearValue = makeClearValueColorU32(0u, 0u, 0u, 0u);

		const VkRenderPassBeginInfo renderPassBeginInfo = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,							// VkStructureType         sType;
			DE_NULL,															// const void*             pNext;
			*renderPass,														// VkRenderPass            renderPass;
			*framebuffer,														// VkFramebuffer           framebuffer;
			renderArea,															// VkRect2D                renderArea;
			1u,																	// uint32_t                clearValueCount;
			&clearValue,														// const VkClearValue*     pClearValues;
		};
		vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **pipelines.back());
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
		{
			const VkDeviceSize vertexBufferOffset = 0ull;
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
		}

		vk.cmdDraw(*cmdBuffer, static_cast<deUint32>(vertices.size()), 1u, 0u, 0u);
		vk.cmdEndRenderPass(*cmdBuffer);

		// Prepare checksum image for copy
		{
			const VkImageMemoryBarrier barriers[] =
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,						// VkStructureType			sType;
					DE_NULL,													// const void*				pNext;
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,						// VkAccessFlags			outputMask;
					VK_ACCESS_TRANSFER_READ_BIT,								// VkAccessFlags			inputMask;
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,					// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,						// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					destQueueFamilyIndex;
					*checksumImage,												// VkImage					image;
					makeColorSubresourceRange(0, 1),							// VkImageSubresourceRange	subresourceRange;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
		}
		// Checksum image -> host buffer
		{
			const VkBufferImageCopy region =
			{
				0ull,																		// VkDeviceSize                bufferOffset;
				0u,																			// uint32_t                    bufferRowLength;
				0u,																			// uint32_t                    bufferImageHeight;
				makeColorSubresourceLayers(0, 1),											// VkImageSubresourceLayers    imageSubresource;
				makeOffset3D(0, 0, 0),														// VkOffset3D                  imageOffset;
				makeExtent3D(caseDef.renderSize.x(), caseDef.renderSize.y(), 1u),			// VkExtent3D                  imageExtent;
			};

			vk.cmdCopyImageToBuffer(*cmdBuffer, *checksumImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *checksumBuffer, 1u, &region);
		}
		// Buffer write barrier
		{
			const VkBufferMemoryBarrier barriers[] =
			{
				{
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,		// VkStructureType    sType;
					DE_NULL,										// const void*        pNext;
					VK_ACCESS_TRANSFER_WRITE_BIT,					// VkAccessFlags      srcAccessMask;
					VK_ACCESS_HOST_READ_BIT,						// VkAccessFlags      dstAccessMask;
					VK_QUEUE_FAMILY_IGNORED,						// uint32_t           srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,						// uint32_t           dstQueueFamilyIndex;
					*checksumBuffer,								// VkBuffer           buffer;
					0ull,											// VkDeviceSize       offset;
					checksumBufferSize,								// VkDeviceSize       size;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
				0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers, DE_NULL, 0u);
		}

		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);

		// Verify result

		{
			invalidateMappedMemoryRange(vk, device, checksumBufferAlloc->getMemory(), 0ull, checksumBufferSize);

			const tcu::ConstPixelBufferAccess access(mapVkFormat(checksumFormat), caseDef.renderSize.x(), caseDef.renderSize.y(), 1, checksumBufferAlloc->getHostPtr());
			const int numExpectedChecksum = getNumSamples(caseDef.numSamples) * caseDef.numLayers;

			for (int y = 0; y < caseDef.renderSize.y(); ++y)
			for (int x = 0; x < caseDef.renderSize.x(); ++x)
			{
				if (access.getPixelInt(x, y).x() != numExpectedChecksum)
					return tcu::TestStatus::fail("Some samples have incorrect color");
			}
		}
	}

	return tcu::TestStatus::pass("OK");
}

} // SampledImage ns

namespace StorageImage
{

void initPrograms (SourceCollections& programCollection, const CaseDef caseDef)
{
	// Vertex & fragment

	addSimpleVertexAndFragmentPrograms(programCollection, caseDef);

	// Compute
	{
		const std::string	imageTypeStr		= getShaderMultisampledImageType(mapVkFormat(caseDef.colorFormat), caseDef.numLayers);
		const std::string	formatQualifierStr	= getShaderImageFormatQualifier(mapVkFormat(caseDef.colorFormat));
		const std::string	signednessPrefix	= isUintFormat(caseDef.colorFormat) ? "u" : isIntFormat(caseDef.colorFormat) ? "i" : "";
		const std::string	gvec4Expr			= signednessPrefix + "vec4";
		const std::string	texelCoordStr		= (caseDef.numLayers == 1 ? "ivec2(gx, gy)" : "ivec3(gx, gy, gz)");

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "layout(local_size_x = 1) in;\n"
			<< "layout(set = 0, binding = 0, " << formatQualifierStr << ") uniform " << imageTypeStr << " u_msImage;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    int gx = int(gl_GlobalInvocationID.x);\n"
			<< "    int gy = int(gl_GlobalInvocationID.y);\n"
			<< "    int gz = int(gl_GlobalInvocationID.z);\n"
			<< "\n"
			<< "    " << gvec4Expr << " prevColor = imageLoad(u_msImage, " << texelCoordStr << ", 0);\n"
			<< "    for (int sampleNdx = 1; sampleNdx < " << caseDef.numSamples << "; ++sampleNdx) {\n"
			<< "        " << gvec4Expr << " color = imageLoad(u_msImage, " << texelCoordStr << ", sampleNdx);\n"
			<< "        imageStore(u_msImage, " << texelCoordStr <<", sampleNdx, prevColor);\n"
			<< "        prevColor = color;\n"
			<< "    }\n"
			<< "    imageStore(u_msImage, " << texelCoordStr <<", 0, prevColor);\n"
			<< "}\n";

		programCollection.glslSources.add("comp") << glu::ComputeSource(src.str());
	}
}

//! Render a MS image, resolve it, and copy result to resolveBuffer.
void renderAndResolve (Context& context, const CaseDef& caseDef, const VkBuffer resolveBuffer, const bool useComputePass)
{
	const DeviceInterface&		vk					= context.getDeviceInterface();
	const VkDevice				device				= context.getDevice();
	const VkQueue				queue				= context.getUniversalQueue();
	const deUint32				queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&					allocator			= context.getDefaultAllocator();

	// Multisampled color image
	const Unique<VkImage>			colorImage			(makeImage(vk, device, caseDef.colorFormat, caseDef.renderSize, caseDef.numLayers, caseDef.numSamples,
																   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
	const UniquePtr<Allocation>		colorImageAlloc		(bindImage(vk, device, allocator, *colorImage, MemoryRequirement::Any));

	const Unique<VkImage>			resolveImage		(makeImage(vk, device, caseDef.colorFormat, caseDef.renderSize, caseDef.numLayers, VK_SAMPLE_COUNT_1_BIT,
																   VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
	const UniquePtr<Allocation>		resolveImageAlloc	(bindImage(vk, device, allocator, *resolveImage, MemoryRequirement::Any));

	const Unique<VkCommandPool>		cmdPool				(createCommandPool  (vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));

	// Working image barrier, we change it based on which rendering stages were executed so far.
	VkImageMemoryBarrier colorImageBarrier =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,						// VkStructureType			sType;
		DE_NULL,													// const void*				pNext;
		(VkAccessFlags)0,											// VkAccessFlags			outputMask;
		(VkAccessFlags)0,											// VkAccessFlags			inputMask;
		VK_IMAGE_LAYOUT_UNDEFINED,									// VkImageLayout			oldLayout;
		VK_IMAGE_LAYOUT_UNDEFINED,									// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,									// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,									// deUint32					destQueueFamilyIndex;
		*colorImage,												// VkImage					image;
		makeColorSubresourceRange(0, caseDef.numLayers),			// VkImageSubresourceRange	subresourceRange;
	};

	// Pass 1: Render an image
	{
		renderMultisampledImage(context, caseDef, *colorImage);

		colorImageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		colorImageBarrier.oldLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	// Pass 2: Compute shader
	if (useComputePass)
	{
		// Descriptors

		Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
			.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.build(vk, device));

		Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
			.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u)
			.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

		const Unique<VkImageView>		colorImageView		(makeImageView(vk, device, *colorImage,
																			(caseDef.numLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY),
																			caseDef.colorFormat, makeColorSubresourceRange(0, caseDef.numLayers)));
		const Unique<VkDescriptorSet>	descriptorSet		(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));
		const VkDescriptorImageInfo		descriptorImageInfo	= makeDescriptorImageInfo(DE_NULL, *colorImageView, VK_IMAGE_LAYOUT_GENERAL);

		DescriptorSetUpdateBuilder()
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorImageInfo)
			.update(vk, device);

		const Unique<VkPipelineLayout>	pipelineLayout	(makePipelineLayout	(vk, device, *descriptorSetLayout));
		const Unique<VkShaderModule>	shaderModule	(createShaderModule	(vk, device, context.getBinaryCollection().get("comp"), 0));
		const Unique<VkPipeline>		pipeline		(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule, DE_NULL));

		beginCommandBuffer(vk, *cmdBuffer);

		// Image layout for load/stores
		{
			colorImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			colorImageBarrier.newLayout		= VK_IMAGE_LAYOUT_GENERAL;

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, 1u, &colorImageBarrier);

			colorImageBarrier.srcAccessMask = colorImageBarrier.dstAccessMask;
			colorImageBarrier.oldLayout		= colorImageBarrier.newLayout;
		}
		// Dispatch
		{
			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
			vk.cmdDispatch(*cmdBuffer, caseDef.renderSize.x(), caseDef.renderSize.y(), caseDef.numLayers);
		}

		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Resolve and verify the image
	{
		beginCommandBuffer(vk, *cmdBuffer);

		// Prepare for resolve
		{
			colorImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			colorImageBarrier.newLayout		= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

			const VkImageMemoryBarrier barriers[] =
			{
				colorImageBarrier,
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,						// VkStructureType			sType;
					DE_NULL,													// const void*				pNext;
					(VkAccessFlags)0,											// VkAccessFlags			outputMask;
					VK_ACCESS_TRANSFER_WRITE_BIT,								// VkAccessFlags			inputMask;
					VK_IMAGE_LAYOUT_UNDEFINED,									// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,						// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					destQueueFamilyIndex;
					*resolveImage,												// VkImage					image;
					makeColorSubresourceRange(0, caseDef.numLayers),			// VkImageSubresourceRange	subresourceRange;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);

			colorImageBarrier.srcAccessMask = colorImageBarrier.dstAccessMask;
			colorImageBarrier.oldLayout		= colorImageBarrier.newLayout;
		}
		// Resolve the image
		{
			const VkImageResolve resolveRegion =
			{
				makeColorSubresourceLayers(0, caseDef.numLayers),					// VkImageSubresourceLayers    srcSubresource;
				makeOffset3D(0, 0, 0),												// VkOffset3D                  srcOffset;
				makeColorSubresourceLayers(0, caseDef.numLayers),					// VkImageSubresourceLayers    dstSubresource;
				makeOffset3D(0, 0, 0),												// VkOffset3D                  dstOffset;
				makeExtent3D(caseDef.renderSize.x(), caseDef.renderSize.y(), 1u),	// VkExtent3D                  extent;
			};

			vk.cmdResolveImage(*cmdBuffer, *colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *resolveImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &resolveRegion);
		}
		// Prepare resolve image for copy
		{
			const VkImageMemoryBarrier barriers[] =
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,						// VkStructureType			sType;
					DE_NULL,													// const void*				pNext;
					VK_ACCESS_TRANSFER_WRITE_BIT,								// VkAccessFlags			outputMask;
					VK_ACCESS_TRANSFER_READ_BIT,								// VkAccessFlags			inputMask;
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,						// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,						// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					destQueueFamilyIndex;
					*resolveImage,												// VkImage					image;
					makeColorSubresourceRange(0, caseDef.numLayers),			// VkImageSubresourceRange	subresourceRange;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
		}
		// Copy resolved image to host-readable buffer
		{
			const VkBufferImageCopy copyRegion =
			{
				0ull,																// VkDeviceSize                bufferOffset;
				0u,																	// uint32_t                    bufferRowLength;
				0u,																	// uint32_t                    bufferImageHeight;
				makeColorSubresourceLayers(0, caseDef.numLayers),					// VkImageSubresourceLayers    imageSubresource;
				makeOffset3D(0, 0, 0),												// VkOffset3D                  imageOffset;
				makeExtent3D(caseDef.renderSize.x(), caseDef.renderSize.y(), 1u),	// VkExtent3D                  imageExtent;
			};

			vk.cmdCopyImageToBuffer(*cmdBuffer, *resolveImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, resolveBuffer, 1u, &copyRegion);
		}

		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}
}

//! Exact image compare, but allow for some error when color format is integer.
bool compareImages (tcu::TestLog& log, const CaseDef& caseDef, const tcu::ConstPixelBufferAccess layeredReferenceImage, const tcu::ConstPixelBufferAccess layeredActualImage)
{
	DE_ASSERT(caseDef.numSamples > 1);

	const Vec4	goodColor			= Vec4(0.0f, 1.0f, 0.0f, 1.0f);
	const Vec4	badColor			= Vec4(1.0f, 0.0f, 0.0f, 1.0f);
	const bool	isAnyIntFormat		= isIntFormat(caseDef.colorFormat) || isUintFormat(caseDef.colorFormat);

	// There should be no mismatched pixels for non-integer formats. Otherwise we may get a wrong color in a location where sample coverage isn't exactly 0 or 1.
	const int	badPixelTolerance	= (isAnyIntFormat ? 2 * caseDef.renderSize.x() : 0);
	int			goodLayers			= 0;

	for (int layerNdx = 0; layerNdx < caseDef.numLayers; ++layerNdx)
	{
		const tcu::ConstPixelBufferAccess	referenceImage	= tcu::getSubregion(layeredReferenceImage, 0, 0, layerNdx, caseDef.renderSize.x(), caseDef.renderSize.y(), 1);
		const tcu::ConstPixelBufferAccess	actualImage		= tcu::getSubregion(layeredActualImage, 0, 0, layerNdx, caseDef.renderSize.x(), caseDef.renderSize.y(), 1);
		const std::string					imageName		= "color layer " + de::toString(layerNdx);

		tcu::TextureLevel		errorMaskStorage	(tcu::TextureFormat(tcu::TextureFormat::RGB, tcu::TextureFormat::UNORM_INT8), caseDef.renderSize.x(), caseDef.renderSize.y());
		tcu::PixelBufferAccess	errorMask			= errorMaskStorage.getAccess();
		int						numBadPixels		= 0;

		for (int y = 0; y < caseDef.renderSize.y(); ++y)
		for (int x = 0; x < caseDef.renderSize.x(); ++x)
		{
			if (isAnyIntFormat && (referenceImage.getPixelInt(x, y) == actualImage.getPixelInt(x, y)))
				errorMask.setPixel(goodColor, x, y);
			else if (referenceImage.getPixel(x, y) == actualImage.getPixel(x, y))
				errorMask.setPixel(goodColor, x, y);
			else
			{
				++numBadPixels;
				errorMask.setPixel(badColor, x, y);
			}
		}

		if (numBadPixels <= badPixelTolerance)
		{
			++goodLayers;

			log << tcu::TestLog::ImageSet(imageName, imageName)
				<< tcu::TestLog::Image("Result",	"Result",		actualImage)
				<< tcu::TestLog::EndImageSet;
		}
		else
		{
			log << tcu::TestLog::ImageSet(imageName, imageName)
				<< tcu::TestLog::Image("Result",	"Result",		actualImage)
				<< tcu::TestLog::Image("Reference",	"Reference",	referenceImage)
				<< tcu::TestLog::Image("ErrorMask",	"Error mask",	errorMask)
				<< tcu::TestLog::EndImageSet;
		}
	}

	if (goodLayers == caseDef.numLayers)
	{
		log << tcu::TestLog::Message << "All rendered images are correct." << tcu::TestLog::EndMessage;
		return true;
	}
	else
	{
		log << tcu::TestLog::Message << "FAILED: Some rendered images were incorrect." << tcu::TestLog::EndMessage;
		return false;
	}
}

tcu::TestStatus test (Context& context, const CaseDef caseDef)
{
	const DeviceInterface&		vk					= context.getDeviceInterface();
	const InstanceInterface&	vki					= context.getInstanceInterface();
	const VkDevice				device				= context.getDevice();
	const VkPhysicalDevice		physDevice			= context.getPhysicalDevice();
	Allocator&					allocator			= context.getDefaultAllocator();

	checkImageFormatRequirements(vki, physDevice, caseDef.numSamples, caseDef.colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);

	{
		tcu::TestLog& log = context.getTestContext().getLog();
		log << tcu::LogSection("Description", "")
			<< tcu::TestLog::Message << "Rendering to a multisampled image. Image will be processed with a compute shader using OpImageRead and OpImageWrite." << tcu::TestLog::EndMessage
			<< tcu::TestLog::Message << "Expecting the processed image to be roughly the same as the input image (deviation may occur for integer formats)." << tcu::TestLog::EndMessage
			<< tcu::TestLog::EndSection;
	}

	// Host-readable buffer
	const VkDeviceSize				resolveBufferSize			= caseDef.renderSize.x() * caseDef.renderSize.y() * caseDef.numLayers * tcu::getPixelSize(mapVkFormat(caseDef.colorFormat));
	const Unique<VkBuffer>			resolveImageOneBuffer		(makeBuffer(vk, device, resolveBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	const UniquePtr<Allocation>		resolveImageOneBufferAlloc	(bindBuffer(vk, device, allocator, *resolveImageOneBuffer, MemoryRequirement::HostVisible));
	const Unique<VkBuffer>			resolveImageTwoBuffer		(makeBuffer(vk, device, resolveBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	const UniquePtr<Allocation>		resolveImageTwoBufferAlloc	(bindBuffer(vk, device, allocator, *resolveImageTwoBuffer, MemoryRequirement::HostVisible));

	zeroBuffer(vk, device, *resolveImageOneBufferAlloc, resolveBufferSize);
	zeroBuffer(vk, device, *resolveImageTwoBufferAlloc, resolveBufferSize);

	// Render: repeat the same rendering twice to avoid non-essential API calls and layout transitions (e.g. copy).
	{
		renderAndResolve(context, caseDef, *resolveImageOneBuffer, false);	// Pass 1: render a basic multisampled image
		renderAndResolve(context, caseDef, *resolveImageTwoBuffer, true);	// Pass 2: the same but altered with a compute shader
	}

	// Verify
	{
		invalidateMappedMemoryRange(vk, device, resolveImageOneBufferAlloc->getMemory(), resolveImageOneBufferAlloc->getOffset(), resolveBufferSize);
		invalidateMappedMemoryRange(vk, device, resolveImageTwoBufferAlloc->getMemory(), resolveImageTwoBufferAlloc->getOffset(), resolveBufferSize);

		const tcu::PixelBufferAccess		layeredImageOne	(mapVkFormat(caseDef.colorFormat), caseDef.renderSize.x(), caseDef.renderSize.y(), caseDef.numLayers, resolveImageOneBufferAlloc->getHostPtr());
		const tcu::ConstPixelBufferAccess	layeredImageTwo	(mapVkFormat(caseDef.colorFormat), caseDef.renderSize.x(), caseDef.renderSize.y(), caseDef.numLayers, resolveImageTwoBufferAlloc->getHostPtr());

		// Check all layers
		if (!compareImages(context.getTestContext().getLog(), caseDef, layeredImageOne, layeredImageTwo))
			return tcu::TestStatus::fail("Rendered images are not correct");
	}

	return tcu::TestStatus::pass("OK");
}

} // StorageImage ns

std::string getSizeLayerString (const IVec2& size, const int numLayers)
{
	std::ostringstream str;
	str << size.x() << "x" << size.y() << "_" << numLayers;
	return str.str();
}

std::string getFormatString (const VkFormat format)
{
	std::string name(getFormatName(format));
	return de::toLower(name.substr(10));
}

void addTestCasesWithFunctions (tcu::TestCaseGroup*						group,
								FunctionPrograms1<CaseDef>::Function	initPrograms,
								FunctionInstance1<CaseDef>::Function	testFunc)
{
	const IVec2 size[] =
	{
		IVec2(64, 64),
		IVec2(79, 31),
	};
	const int numLayers[] =
	{
		1, 4
	};
	const VkSampleCountFlagBits samples[] =
	{
		VK_SAMPLE_COUNT_2_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_64_BIT,
	};
	const VkFormat format[] =
	{
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
	};

	for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(size); ++sizeNdx)
	for (int layerNdx = 0; layerNdx < DE_LENGTH_OF_ARRAY(numLayers); ++layerNdx)
	{
		MovePtr<tcu::TestCaseGroup>	sizeLayerGroup(new tcu::TestCaseGroup(group->getTestContext(), getSizeLayerString(size[sizeNdx], numLayers[layerNdx]).c_str(), ""));
		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(format); ++formatNdx)
		{
			MovePtr<tcu::TestCaseGroup>	formatGroup(new tcu::TestCaseGroup(group->getTestContext(), getFormatString(format[formatNdx]).c_str(), ""));
			for (int samplesNdx = 0; samplesNdx < DE_LENGTH_OF_ARRAY(samples); ++samplesNdx)
			{
				std::ostringstream caseName;
				caseName << "samples_" << getNumSamples(samples[samplesNdx]);

				const CaseDef caseDef =
				{
					size[sizeNdx],			// IVec2					renderSize;
					numLayers[layerNdx],	// int						numLayers;
					format[formatNdx],		// VkFormat					colorFormat;
					samples[samplesNdx],	// VkSampleCountFlagBits	numSamples;
				};

				addFunctionCaseWithPrograms(formatGroup.get(), caseName.str(), "", initPrograms, testFunc, caseDef);
			}
			sizeLayerGroup->addChild(formatGroup.release());
		}
		group->addChild(sizeLayerGroup.release());
	}
}

void createSampledImageTestsInGroup (tcu::TestCaseGroup* group)
{
	addTestCasesWithFunctions(group, SampledImage::initPrograms, SampledImage::test);
}

void createStorageImageTestsInGroup (tcu::TestCaseGroup* group)
{
	addTestCasesWithFunctions(group, StorageImage::initPrograms, StorageImage::test);
}

} // anonymous ns

//! Render to a multisampled image and sample from it in a fragment shader.
tcu::TestCaseGroup* createMultisampleSampledImageTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "sampled_image", "Multisampled image direct sample access", createSampledImageTestsInGroup);
}

//! Render to a multisampled image and access it with load/stores in a compute shader.
tcu::TestCaseGroup* createMultisampleStorageImageTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "storage_image", "Multisampled image draw and read/write in compute shader", createStorageImageTestsInGroup);
}

} // pipeline
} // vkt
