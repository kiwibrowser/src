/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Geometry shader instanced rendering tests
 *//*--------------------------------------------------------------------*/

#include "vktGeometryInstancedRenderingTests.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktGeometryTestsUtil.hpp"

#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkImageUtil.hpp"

#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"

#include "deRandom.hpp"
#include "deMath.h"

namespace vkt
{
namespace geometry
{
namespace
{
using namespace vk;
using de::MovePtr;
using de::UniquePtr;
using tcu::Vec4;
using tcu::UVec2;

struct TestParams
{
	int	numDrawInstances;
	int	numInvocations;
};

VkImageCreateInfo makeImageCreateInfo (const VkFormat format, const VkExtent3D size, const VkImageUsageFlags usage)
{
	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		(VkImageCreateFlags)0,							// VkImageCreateFlags		flags;
		VK_IMAGE_TYPE_2D,								// VkImageType				imageType;
		format,											// VkFormat					format;
		size,											// VkExtent3D				extent;
		1u,												// deUint32					mipLevels;
		1u,												// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,							// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,						// VkImageTiling			tiling;
		usage,											// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,						// VkSharingMode			sharingMode;
		0u,												// deUint32					queueFamilyIndexCount;
		DE_NULL,										// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			initialLayout;
	};
	return imageParams;
}

Move<VkRenderPass> makeRenderPass (const DeviceInterface&	vk,
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

Move<VkPipeline> makeGraphicsPipeline (const DeviceInterface&		vk,
									   const VkDevice				device,
									   const VkPipelineLayout		pipelineLayout,
									   const VkRenderPass			renderPass,
									   const VkShaderModule			vertexModule,
									   const VkShaderModule			geometryModule,
									   const VkShaderModule			fragmentModule,
									   const VkExtent2D				renderSize)
{
	const VkVertexInputBindingDescription vertexInputBindingDescription =
	{
		0u,											// uint32_t             binding;
		sizeof(Vec4),								// uint32_t             stride;
		VK_VERTEX_INPUT_RATE_INSTANCE,				// VkVertexInputRate    inputRate;
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescription =
	{
		0u,											// uint32_t    location;
		0u,											// uint32_t    binding;
		VK_FORMAT_R32G32B32A32_SFLOAT,				// VkFormat    format;
		0u,											// uint32_t    offset;
	};

	const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineVertexInputStateCreateFlags)0,						// VkPipelineVertexInputStateCreateFlags       flags;
		1u,																// uint32_t                                    vertexBindingDescriptionCount;
		&vertexInputBindingDescription,									// const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
		1u,																// uint32_t                                    vertexAttributeDescriptionCount;
		&vertexInputAttributeDescription,								// const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
	};

	const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0,						// VkPipelineInputAssemblyStateCreateFlags     flags;
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST,								// VkPrimitiveTopology                         topology;
		VK_FALSE,														// VkBool32                                    primitiveRestartEnable;
	};

	const VkViewport viewport = makeViewport(
		0.0f, 0.0f,
		static_cast<float>(renderSize.width), static_cast<float>(renderSize.height),
		0.0f, 1.0f);
	const VkRect2D scissor =
	{
		makeOffset2D(0, 0),
		makeExtent2D(renderSize.width, renderSize.height),
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
		VK_SAMPLE_COUNT_1_BIT,											// VkSampleCountFlagBits					rasterizationSamples;
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

	const VkColorComponentFlags					colorComponentsAll					= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	const VkPipelineColorBlendAttachmentState	pipelineColorBlendAttachmentState	=
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
			VK_SHADER_STAGE_GEOMETRY_BIT,								// VkShaderStageFlagBits				stage;
			geometryModule,												// VkShaderModule						module;
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
		},
	};

	const VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
		DE_NULL,											// const void*										pNext;
		(VkPipelineCreateFlags)0,							// VkPipelineCreateFlags							flags;
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
		0,													// deInt32											basePipelineIndex;
	};

	return createGraphicsPipeline(vk, device, DE_NULL, &graphicsPipelineInfo);
}

void draw (Context&					context,
		   const UVec2&				renderSize,
		   const VkFormat			colorFormat,
		   const Vec4&				clearColor,
		   const VkBuffer			colorBuffer,
		   const int				numDrawInstances,
		   const std::vector<Vec4>& perInstanceAttribute)
{
	const DeviceInterface&			vk						= context.getDeviceInterface();
	const VkDevice					device					= context.getDevice();
	const deUint32					queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	const VkQueue					queue					= context.getUniversalQueue();
	Allocator&						allocator				= context.getDefaultAllocator();

	const VkImageSubresourceRange	colorSubresourceRange	(makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u));
	const VkExtent3D				colorImageExtent		(makeExtent3D(renderSize.x(), renderSize.y(), 1u));
	const VkExtent2D				renderExtent			(makeExtent2D(renderSize.x(), renderSize.y()));

	const Unique<VkImage>			colorImage				(makeImage		(vk, device, makeImageCreateInfo(colorFormat, colorImageExtent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)));
	const UniquePtr<Allocation>		colorImageAlloc			(bindImage		(vk, device, allocator, *colorImage, MemoryRequirement::Any));
	const Unique<VkImageView>		colorAttachment			(makeImageView	(vk, device, *colorImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorSubresourceRange));

	const VkDeviceSize				vertexBufferSize		= sizeInBytes(perInstanceAttribute);
	const Unique<VkBuffer>			vertexBuffer			(makeBuffer(vk, device, makeBufferCreateInfo(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)));
	const UniquePtr<Allocation>		vertexBufferAlloc		(bindBuffer(vk, device, allocator, *vertexBuffer, MemoryRequirement::HostVisible));

	const Unique<VkShaderModule>	vertexModule			(createShaderModule	(vk, device, context.getBinaryCollection().get("vert"), 0u));
	const Unique<VkShaderModule>	geometryModule			(createShaderModule	(vk, device, context.getBinaryCollection().get("geom"), 0u));
	const Unique<VkShaderModule>	fragmentModule			(createShaderModule	(vk, device, context.getBinaryCollection().get("frag"), 0u));

	const Unique<VkRenderPass>		renderPass				(makeRenderPass			(vk, device, colorFormat));
	const Unique<VkFramebuffer>		framebuffer				(makeFramebuffer		(vk, device, *renderPass, *colorAttachment, renderSize.x(), renderSize.y(), 1u));
	const Unique<VkPipelineLayout>	pipelineLayout			(makePipelineLayout		(vk, device));
	const Unique<VkPipeline>		pipeline				(makeGraphicsPipeline	(vk, device, *pipelineLayout, *renderPass, *vertexModule, *geometryModule, *fragmentModule, renderExtent));

	const Unique<VkCommandPool>		cmdPool					(createCommandPool		(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer				(allocateCommandBuffer	(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Initialize vertex data
	{
		deMemcpy(vertexBufferAlloc->getHostPtr(), &perInstanceAttribute[0], (size_t)vertexBufferSize);
		flushMappedMemoryRange(vk, device, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset(), vertexBufferSize);
	}

	beginCommandBuffer(vk, *cmdBuffer);

	const VkClearValue			clearValue	= makeClearValueColor(clearColor);
	const VkRect2D				renderArea	=
	{
		makeOffset2D(0, 0),
		renderExtent,
	};
	const VkRenderPassBeginInfo renderPassBeginInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,		// VkStructureType         sType;
		DE_NULL,										// const void*             pNext;
		*renderPass,									// VkRenderPass            renderPass;
		*framebuffer,									// VkFramebuffer           framebuffer;
		renderArea,										// VkRect2D                renderArea;
		1u,												// uint32_t                clearValueCount;
		&clearValue,									// const VkClearValue*     pClearValues;
	};
	vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
	{
		const VkDeviceSize offset = 0ull;
		vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &offset);
	}
	vk.cmdDraw(*cmdBuffer, 1u, static_cast<deUint32>(numDrawInstances), 0u, 0u);
	vk.cmdEndRenderPass(*cmdBuffer);

	// Prepare color image for copy
	{
		const VkImageMemoryBarrier barriers[] =
		{
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
				DE_NULL,									// const void*				pNext;
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,		// VkAccessFlags			outputMask;
				VK_ACCESS_TRANSFER_READ_BIT,				// VkAccessFlags			inputMask;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout			oldLayout;
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		// VkImageLayout			newLayout;
				VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,					// deUint32					destQueueFamilyIndex;
				*colorImage,								// VkImage					image;
				colorSubresourceRange,						// VkImageSubresourceRange	subresourceRange;
			},
		};

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
			0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
	}
	// Color image -> host buffer
	{
		const VkBufferImageCopy region =
		{
			0ull,																	// VkDeviceSize                bufferOffset;
			0u,																		// uint32_t                    bufferRowLength;
			0u,																		// uint32_t                    bufferImageHeight;
			makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u),		// VkImageSubresourceLayers    imageSubresource;
			makeOffset3D(0, 0, 0),													// VkOffset3D                  imageOffset;
			colorImageExtent,														// VkExtent3D                  imageExtent;
		};

		vk.cmdCopyImageToBuffer(*cmdBuffer, *colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, colorBuffer, 1u, &region);
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
				colorBuffer,									// VkBuffer           buffer;
				0ull,											// VkDeviceSize       offset;
				VK_WHOLE_SIZE,									// VkDeviceSize       size;
			},
		};

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
			0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers, DE_NULL, 0u);
	}

	VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);
}

std::vector<Vec4> generatePerInstancePosition (const int numInstances)
{
	de::Random			rng(1234);
	std::vector<Vec4>	positions;

	for (int i = 0; i < numInstances; ++i)
	{
		const float flipX	= rng.getBool() ? 1.0f : -1.0f;
		const float flipY	= rng.getBool() ? 1.0f : -1.0f;
		const float x		= flipX * rng.getFloat(0.1f, 0.9f);	// x mustn't be 0.0, because we are using sign() in the shader
		const float y		= flipY * rng.getFloat(0.0f, 0.7f);

		positions.push_back(Vec4(x, y, 0.0f, 1.0f));
	}

	return positions;
}

//! Get a rectangle region of an image, using NDC coordinates (i.e. [-1, 1] range).
//! Result rect is cropped in either dimension to be inside the bounds of the image.
tcu::PixelBufferAccess getSubregion (tcu::PixelBufferAccess image, const float x, const float y, const float size)
{
	const float w	= static_cast<float>(image.getWidth());
	const float h	= static_cast<float>(image.getHeight());
	const float x1	= w * (x + 1.0f) * 0.5f;
	const float y1	= h * (y + 1.0f) * 0.5f;
	const float sx	= w * size * 0.5f;
	const float sy	= h * size * 0.5f;
	const float x2	= x1 + sx;
	const float y2	= y1 + sy;

	// Round and clamp only after all of the above.
	const int	ix1	= std::max(deRoundFloatToInt32(x1), 0);
	const int	ix2	= std::min(deRoundFloatToInt32(x2), image.getWidth());
	const int	iy1	= std::max(deRoundFloatToInt32(y1), 0);
	const int	iy2	= std::min(deRoundFloatToInt32(y2), image.getHeight());

	return tcu::getSubregion(image, ix1, iy1, ix2 - ix1, iy2 - iy1);
}

//! Must be in sync with the geometry shader code.
void generateReferenceImage(tcu::PixelBufferAccess image, const Vec4& clearColor, const std::vector<Vec4>& perInstancePosition, const int numInvocations)
{
	tcu::clear(image, clearColor);

	for (std::vector<Vec4>::const_iterator iterPosition = perInstancePosition.begin(); iterPosition != perInstancePosition.end(); ++iterPosition)
	for (int invocationNdx = 0; invocationNdx < numInvocations; ++invocationNdx)
	{
		const float x			= iterPosition->x();
		const float y			= iterPosition->y();
		const float	modifier	= (numInvocations > 1 ? static_cast<float>(invocationNdx) / static_cast<float>(numInvocations - 1) : 0.0f);
		const Vec4	color		(deFloatAbs(x), deFloatAbs(y), 0.2f + 0.8f * modifier, 1.0f);
		const float size		= 0.05f + 0.03f * modifier;
		const float dx			= (deFloatSign(-x) - x) / static_cast<float>(numInvocations);
		const float xOffset		= static_cast<float>(invocationNdx) * dx;
		const float yOffset		= 0.3f * deFloatSin(12.0f * modifier);

		tcu::PixelBufferAccess rect = getSubregion(image, x + xOffset - size, y + yOffset - size, size + size);
		tcu::clear(rect, color);
	}
}

void initPrograms (SourceCollections& programCollection, const TestParams params)
{
	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in vec4 in_position;\n"
			<< "\n"
			<< "out gl_PerVertex {\n"
			<< "    vec4 gl_Position;\n"
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    gl_Position = in_position;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Geometry shader
	{
		// The shader must be in sync with reference image rendering routine.

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(points, invocations = " << params.numInvocations << ") in;\n"
			<< "layout(triangle_strip, max_vertices = 4) out;\n"
			<< "\n"
			<< "layout(location = 0) out vec4 out_color;\n"
			<< "\n"
			<< "in gl_PerVertex {\n"
			<< "    vec4 gl_Position;\n"
			<< "} gl_in[];\n"
			<< "\n"
			<< "out gl_PerVertex {\n"
			<< "    vec4 gl_Position;\n"
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    const vec4  pos       = gl_in[0].gl_Position;\n"
			<< "    const float modifier  = " << (params.numInvocations > 1 ? "float(gl_InvocationID) / float(" + de::toString(params.numInvocations - 1) + ")" : "0.0") << ";\n"
			<< "    const vec4  color     = vec4(abs(pos.x), abs(pos.y), 0.2 + 0.8 * modifier, 1.0);\n"
			<< "    const float size      = 0.05 + 0.03 * modifier;\n"
			<< "    const float dx        = (sign(-pos.x) - pos.x) / float(" << params.numInvocations << ");\n"
			<< "    const vec4  offsetPos = pos + vec4(float(gl_InvocationID) * dx,\n"
			<< "                                       0.3 * sin(12.0 * modifier),\n"
			<< "                                       0.0,\n"
			<< "                                       0.0);\n"
			<< "\n"
			<< "    gl_Position = offsetPos + vec4(-size, -size, 0.0, 0.0);\n"
			<< "    out_color   = color;\n"
			<< "    EmitVertex();\n"
			<< "\n"
			<< "    gl_Position = offsetPos + vec4(-size,  size, 0.0, 0.0);\n"
			<< "    out_color   = color;\n"
			<< "    EmitVertex();\n"
			<< "\n"
			<< "    gl_Position = offsetPos + vec4( size, -size, 0.0, 0.0);\n"
			<< "    out_color   = color;\n"
			<< "    EmitVertex();\n"
			<< "\n"
			<< "    gl_Position = offsetPos + vec4( size,  size, 0.0, 0.0);\n"
			<< "    out_color   = color;\n"
			<< "    EmitVertex();\n"
			<<	"}\n";

		programCollection.glslSources.add("geom") << glu::GeometrySource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in  vec4 in_color;\n"
			<< "layout(location = 0) out vec4 o_color;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    o_color = in_color;\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

tcu::TestStatus test (Context& context, const TestParams params)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const InstanceInterface&		vki					= context.getInstanceInterface();
	const VkDevice					device				= context.getDevice();
	const VkPhysicalDevice			physDevice			= context.getPhysicalDevice();
	Allocator&						allocator			= context.getDefaultAllocator();

	checkGeometryShaderSupport(vki, physDevice, params.numInvocations);

	const UVec2						renderSize			(128u, 128u);
	const VkFormat					colorFormat			= VK_FORMAT_R8G8B8A8_UNORM;
	const Vec4						clearColor			= Vec4(0.0f, 0.0f, 0.0f, 1.0f);

	const VkDeviceSize				colorBufferSize		= renderSize.x() * renderSize.y() * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Unique<VkBuffer>			colorBuffer			(makeBuffer(vk, device, makeBufferCreateInfo(colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT)));
	const UniquePtr<Allocation>		colorBufferAlloc	(bindBuffer(vk, device, allocator, *colorBuffer, MemoryRequirement::HostVisible));

	const std::vector<Vec4>			perInstancePosition	= generatePerInstancePosition(params.numDrawInstances);

	{
		context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Rendering " << params.numDrawInstances << " instance(s) of colorful quads." << tcu::TestLog::EndMessage
			<< tcu::TestLog::Message << "Drawing " << params.numInvocations << " quad(s), each drawn by a geometry shader invocation." << tcu::TestLog::EndMessage;
	}

	zeroBuffer(vk, device, *colorBufferAlloc, colorBufferSize);
	draw(context, renderSize, colorFormat, clearColor, *colorBuffer, params.numDrawInstances, perInstancePosition);

	// Compare result
	{
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), colorBufferSize);
		const tcu::ConstPixelBufferAccess result(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1u, colorBufferAlloc->getHostPtr());

		tcu::TextureLevel reference(mapVkFormat(colorFormat), renderSize.x(), renderSize.y());
		generateReferenceImage(reference.getAccess(), clearColor, perInstancePosition, params.numInvocations);

		if (!tcu::fuzzyCompare(context.getTestContext().getLog(), "Image Compare", "Image Compare", reference.getAccess(), result, 0.01f, tcu::COMPARE_LOG_RESULT))
			return tcu::TestStatus::fail("Rendered image is incorrect");
		else
			return tcu::TestStatus::pass("OK");
	}
}

} // anonymous

//! \note CTS requires shaders to be known ahead of time (some platforms use precompiled shaders), so we can't query a limit at runtime and generate
//!       a shader based on that. This applies to number of GS invocations which can't be injected into the shader.
tcu::TestCaseGroup* createInstancedRenderingTests (tcu::TestContext& testCtx)
{
	MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(testCtx, "instanced", "Instanced rendering tests."));

	const int drawInstanceCases[]	=
	{
		1, 2, 4, 8,
	};
	const int invocationCases[]		=
	{
		1, 2, 8, 32,	// required by the Vulkan spec
		64, 127,		// larger than the minimum, but perhaps some implementations support it, so we'll try
	};

	for (const int* pNumDrawInstances = drawInstanceCases; pNumDrawInstances != drawInstanceCases + DE_LENGTH_OF_ARRAY(drawInstanceCases); ++pNumDrawInstances)
	for (const int* pNumInvocations   = invocationCases;   pNumInvocations   != invocationCases   + DE_LENGTH_OF_ARRAY(invocationCases);   ++pNumInvocations)
	{
		std::ostringstream caseName;
		caseName << "draw_" << *pNumDrawInstances << "_instances_" << *pNumInvocations << "_geometry_invocations";

		const TestParams params =
		{
			*pNumDrawInstances,
			*pNumInvocations,
		};

		addFunctionCaseWithPrograms(group.get(), caseName.str(), "", initPrograms, test, params);
	}

	return group.release();
}

} // geometry
} // vkt
