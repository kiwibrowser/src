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
*//*
* \file vktPipelineMultisampleBaseResolve.cpp
* \brief Base class for tests that check results of multisample resolve
*//*--------------------------------------------------------------------*/

#include "vktPipelineMultisampleBaseResolve.hpp"
#include "vktPipelineMakeUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkQueryUtil.hpp"
#include "tcuTestLog.hpp"
#include <vector>

namespace vkt
{
namespace pipeline
{
namespace multisample
{

using namespace vk;

tcu::TestStatus MSInstanceBaseResolve::iterate (void)
{
	const InstanceInterface&		instance			= m_context.getInstanceInterface();
	const DeviceInterface&			deviceInterface		= m_context.getDeviceInterface();
	const VkDevice					device				= m_context.getDevice();
	const VkPhysicalDevice			physicalDevice		= m_context.getPhysicalDevice();
	const VkPhysicalDeviceFeatures&	features			= m_context.getDeviceFeatures();
	Allocator&						allocator			= m_context.getDefaultAllocator();
	const VkQueue					queue				= m_context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();

	VkImageCreateInfo				imageMSInfo;
	VkImageCreateInfo				imageRSInfo;

	// Check if image size does not exceed device limits
	validateImageSize(instance, physicalDevice, m_imageType, m_imageMSParams.imageSize);

	// Check if device supports image format as color attachment
	validateImageFeatureFlags(instance, physicalDevice, mapTextureFormat(m_imageFormat), VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);

	imageMSInfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageMSInfo.pNext					= DE_NULL;
	imageMSInfo.flags					= 0u;
	imageMSInfo.imageType				= mapImageType(m_imageType);
	imageMSInfo.format					= mapTextureFormat(m_imageFormat);
	imageMSInfo.extent					= makeExtent3D(getLayerSize(m_imageType, m_imageMSParams.imageSize));
	imageMSInfo.arrayLayers				= getNumLayers(m_imageType, m_imageMSParams.imageSize);
	imageMSInfo.mipLevels				= 1u;
	imageMSInfo.samples					= m_imageMSParams.numSamples;
	imageMSInfo.tiling					= VK_IMAGE_TILING_OPTIMAL;
	imageMSInfo.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imageMSInfo.usage					= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageMSInfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imageMSInfo.queueFamilyIndexCount	= 0u;
	imageMSInfo.pQueueFamilyIndices		= DE_NULL;

	if (m_imageType == IMAGE_TYPE_CUBE || m_imageType == IMAGE_TYPE_CUBE_ARRAY)
	{
		imageMSInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	validateImageInfo(instance, physicalDevice, imageMSInfo);

	const de::UniquePtr<Image> imageMS(new Image(deviceInterface, device, allocator, imageMSInfo, MemoryRequirement::Any));

	imageRSInfo			= imageMSInfo;
	imageRSInfo.samples	= VK_SAMPLE_COUNT_1_BIT;

	validateImageInfo(instance, physicalDevice, imageRSInfo);

	const de::UniquePtr<Image> imageRS(new Image(deviceInterface, device, allocator, imageRSInfo, MemoryRequirement::Any));

	// Create render pass
	const VkAttachmentDescription attachmentMSDesc =
	{
		(VkAttachmentDescriptionFlags)0u,			// VkAttachmentDescriptionFlags		flags;
		imageMSInfo.format,							// VkFormat							format;
		imageMSInfo.samples,						// VkSampleCountFlagBits			samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,				// VkAttachmentLoadOp				loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,				// VkAttachmentStoreOp				storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,			// VkAttachmentLoadOp				stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,			// VkAttachmentStoreOp				stencilStoreOp;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout					finalLayout;
	};

	const VkAttachmentDescription attachmentRSDesc =
	{
		(VkAttachmentDescriptionFlags)0u,			// VkAttachmentDescriptionFlags		flags;
		imageRSInfo.format,							// VkFormat							format;
		imageRSInfo.samples,						// VkSampleCountFlagBits			samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,			// VkAttachmentLoadOp				loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,				// VkAttachmentStoreOp				storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,			// VkAttachmentLoadOp				stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,			// VkAttachmentStoreOp				stencilStoreOp;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout					finalLayout;
	};

	const VkAttachmentDescription attachments[] = { attachmentMSDesc, attachmentRSDesc };

	const VkAttachmentReference attachmentMSRef =
	{
		0u,											// deUint32			attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
	};

	const VkAttachmentReference attachmentRSRef =
	{
		1u,											// deUint32			attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
	};

	const VkAttachmentReference* resolveAttachment = m_imageMSParams.numSamples == VK_SAMPLE_COUNT_1_BIT ? DE_NULL : &attachmentRSRef;

	const VkSubpassDescription subpassDescription =
	{
		(VkSubpassDescriptionFlags)0u,						// VkSubpassDescriptionFlags		flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint				pipelineBindPoint;
		0u,													// deUint32							inputAttachmentCount;
		DE_NULL,											// const VkAttachmentReference*		pInputAttachments;
		1u,													// deUint32							colorAttachmentCount;
		&attachmentMSRef,									// const VkAttachmentReference*		pColorAttachments;
		resolveAttachment,								// const VkAttachmentReference*		pResolveAttachments;
		DE_NULL,											// const VkAttachmentReference*		pDepthStencilAttachment;
		0u,													// deUint32							preserveAttachmentCount;
		DE_NULL												// const deUint32*					pPreserveAttachments;
	};

	const VkRenderPassCreateInfo renderPassInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		(VkRenderPassCreateFlags)0u,						// VkRenderPassCreateFlags			flags;
		2u,													// deUint32							attachmentCount;
		attachments,										// const VkAttachmentDescription*	pAttachments;
		1u,													// deUint32							subpassCount;
		&subpassDescription,								// const VkSubpassDescription*		pSubpasses;
		0u,													// deUint32							dependencyCount;
		DE_NULL												// const VkSubpassDependency*		pDependencies;
	};

	const Unique<VkRenderPass> renderPass(createRenderPass(deviceInterface, device, &renderPassInfo));

	const VkImageSubresourceRange fullImageRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, imageMSInfo.mipLevels, 0u, imageMSInfo.arrayLayers);

	// Create color attachments image views
	const Unique<VkImageView> imageMSView(makeImageView(deviceInterface, device, **imageMS, mapImageViewType(m_imageType), imageMSInfo.format, fullImageRange));
	const Unique<VkImageView> imageRSView(makeImageView(deviceInterface, device, **imageRS, mapImageViewType(m_imageType), imageMSInfo.format, fullImageRange));

	const VkImageView attachmentsViews[] = { *imageMSView, *imageRSView };

	// Create framebuffer
	const VkFramebufferCreateInfo framebufferInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// VkStructureType                             sType;
		DE_NULL,									// const void*                                 pNext;
		(VkFramebufferCreateFlags)0u,				// VkFramebufferCreateFlags                    flags;
		*renderPass,								// VkRenderPass                                renderPass;
		2u,											// uint32_t                                    attachmentCount;
		attachmentsViews,							// const VkImageView*                          pAttachments;
		imageMSInfo.extent.width,					// uint32_t                                    width;
		imageMSInfo.extent.height,					// uint32_t                                    height;
		imageMSInfo.arrayLayers,					// uint32_t                                    layers;
	};

	const Unique<VkFramebuffer> framebuffer(createFramebuffer(deviceInterface, device, &framebufferInfo));

	// Create pipeline layout
	const VkPipelineLayoutCreateInfo pipelineLayoutParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		(VkPipelineLayoutCreateFlags)0u,					// VkPipelineLayoutCreateFlags		flags;
		0u,													// deUint32							setLayoutCount;
		DE_NULL,											// const VkDescriptorSetLayout*		pSetLayouts;
		0u,													// deUint32							pushConstantRangeCount;
		DE_NULL,											// const VkPushConstantRange*		pPushConstantRanges;
	};

	const Unique<VkPipelineLayout> pipelineLayout(createPipelineLayout(deviceInterface, device, &pipelineLayoutParams));

	// Create vertex attributes data
	const VertexDataDesc vertexDataDesc = getVertexDataDescripton();

	de::SharedPtr<Buffer> vertexBuffer = de::SharedPtr<Buffer>(new Buffer(deviceInterface, device, allocator, makeBufferCreateInfo(vertexDataDesc.dataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible));
	const Allocation& vertexBufferAllocation = vertexBuffer->getAllocation();

	uploadVertexData(vertexBufferAllocation, vertexDataDesc);

	flushMappedMemoryRange(deviceInterface, device, vertexBufferAllocation.getMemory(), vertexBufferAllocation.getOffset(), VK_WHOLE_SIZE);

	const VkVertexInputBindingDescription vertexBinding =
	{
		0u,							// deUint32				binding;
		vertexDataDesc.dataStride,	// deUint32				stride;
		VK_VERTEX_INPUT_RATE_VERTEX	// VkVertexInputRate	inputRate;
	};

	const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,			// VkStructureType                             sType;
		DE_NULL,															// const void*                                 pNext;
		(VkPipelineVertexInputStateCreateFlags)0u,							// VkPipelineVertexInputStateCreateFlags       flags;
		1u,																	// uint32_t                                    vertexBindingDescriptionCount;
		&vertexBinding,														// const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
		static_cast<deUint32>(vertexDataDesc.vertexAttribDescVec.size()),	// uint32_t                                    vertexAttributeDescriptionCount;
		dataPointer(vertexDataDesc.vertexAttribDescVec),					// const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
	};

	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0u,					// VkPipelineInputAssemblyStateCreateFlags     flags;
		vertexDataDesc.primitiveTopology,								// VkPrimitiveTopology                         topology;
		VK_FALSE,														// VkBool32                                    primitiveRestartEnable;
	};

	const VkViewport viewport =
	{
		0.0f, 0.0f,
		static_cast<float>(imageMSInfo.extent.width), static_cast<float>(imageMSInfo.extent.height),
		0.0f, 1.0f
	};

	const VkRect2D scissor =
	{
		makeOffset2D(0, 0),
		makeExtent2D(imageMSInfo.extent.width, imageMSInfo.extent.height),
	};

	const VkPipelineViewportStateCreateInfo viewportStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineViewportStateCreateFlags)0u,							// VkPipelineViewportStateCreateFlags          flags;
		1u,																// uint32_t                                    viewportCount;
		&viewport,														// const VkViewport*                           pViewports;
		1u,																// uint32_t                                    scissorCount;
		&scissor,														// const VkRect2D*                             pScissors;
	};

	const VkPipelineRasterizationStateCreateInfo rasterizationStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType                          sType;
		DE_NULL,														// const void*                              pNext;
		(VkPipelineRasterizationStateCreateFlags)0u,					// VkPipelineRasterizationStateCreateFlags  flags;
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

	const VkPipelineMultisampleStateCreateInfo multisampleStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineMultisampleStateCreateFlags)0u,						// VkPipelineMultisampleStateCreateFlags	flags;
		imageMSInfo.samples,											// VkSampleCountFlagBits					rasterizationSamples;
		features.sampleRateShading,										// VkBool32									sampleShadingEnable;
		1.0f,															// float									minSampleShading;
		DE_NULL,														// const VkSampleMask*						pSampleMask;
		VK_FALSE,														// VkBool32									alphaToCoverageEnable;
		VK_FALSE,														// VkBool32									alphaToOneEnable;
	};

	const VkStencilOpState stencilOpState = makeStencilOpState
	(
		VK_STENCIL_OP_KEEP,		// stencil fail
		VK_STENCIL_OP_KEEP,		// depth & stencil pass
		VK_STENCIL_OP_KEEP,		// depth only fail
		VK_COMPARE_OP_ALWAYS,	// compare op
		0u,						// compare mask
		0u,						// write mask
		0u						// reference
	);

	const VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineDepthStencilStateCreateFlags)0u,						// VkPipelineDepthStencilStateCreateFlags	flags;
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

	const VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
	{
		VK_FALSE,														// VkBool32					blendEnable;
		VK_BLEND_FACTOR_ONE,											// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,											// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,												// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_ONE,											// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,											// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,												// VkBlendOp				alphaBlendOp;
		colorComponentsAll,												// VkColorComponentFlags	colorWriteMask;
	};

	const VkPipelineColorBlendStateCreateInfo colorBlendStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,		// VkStructureType								sType;
		DE_NULL,														// const void*									pNext;
		(VkPipelineColorBlendStateCreateFlags)0u,						// VkPipelineColorBlendStateCreateFlags			flags;
		VK_FALSE,														// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,												// VkLogicOp									logicOp;
		1u,																// deUint32										attachmentCount;
		&colorBlendAttachmentState,										// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },										// float										blendConstants[4];
	};

	const Unique<VkShaderModule> vsModule(createShaderModule(deviceInterface, device, m_context.getBinaryCollection().get("vertex_shader"), (VkShaderModuleCreateFlags)0));

	const VkPipelineShaderStageCreateInfo vsShaderStageInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,			// VkStructureType						sType;
		DE_NULL,														// const void*							pNext;
		(VkPipelineShaderStageCreateFlags)0u,							// VkPipelineShaderStageCreateFlags		flags;
		VK_SHADER_STAGE_VERTEX_BIT,										// VkShaderStageFlagBits				stage;
		*vsModule,														// VkShaderModule						module;
		"main",															// const char*							pName;
		DE_NULL,														// const VkSpecializationInfo*			pSpecializationInfo;
	};

	const Unique<VkShaderModule> fsModule(createShaderModule(deviceInterface, device, m_context.getBinaryCollection().get("fragment_shader"), (VkShaderModuleCreateFlags)0));

	const VkPipelineShaderStageCreateInfo fsShaderStageInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,			// VkStructureType						sType;
		DE_NULL,														// const void*							pNext;
		(VkPipelineShaderStageCreateFlags)0u,							// VkPipelineShaderStageCreateFlags		flags;
		VK_SHADER_STAGE_FRAGMENT_BIT,									// VkShaderStageFlagBits				stage;
		*fsModule,														// VkShaderModule						module;
		"main",															// const char*							pName;
		DE_NULL,														// const VkSpecializationInfo*			pSpecializationInfo;
	};

	const VkPipelineShaderStageCreateInfo shaderStageInfos[] = { vsShaderStageInfo, fsShaderStageInfo };

	const VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,				// VkStructureType									sType;
		DE_NULL,														// const void*										pNext;
		(VkPipelineCreateFlags)0,										// VkPipelineCreateFlags							flags;
		2u,																// deUint32											stageCount;
		shaderStageInfos,												// const VkPipelineShaderStageCreateInfo*			pStages;
		&vertexInputStateInfo,											// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
		&inputAssemblyStateInfo,										// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
		DE_NULL,														// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
		&viewportStateInfo,												// const VkPipelineViewportStateCreateInfo*			pViewportState;
		&rasterizationStateInfo,										// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
		&multisampleStateInfo,											// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
		&depthStencilStateInfo,											// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
		&colorBlendStateInfo,											// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
		DE_NULL,														// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
		*pipelineLayout,												// VkPipelineLayout									layout;
		*renderPass,													// VkRenderPass										renderPass;
		0u,																// deUint32											subpass;
		DE_NULL,														// VkPipeline										basePipelineHandle;
		0u,																// deInt32											basePipelineIndex;
	};

	// Create graphics pipeline
	const Unique<VkPipeline> graphicsPipeline(createGraphicsPipeline(deviceInterface, device, DE_NULL, &graphicsPipelineInfo));

	// Create command buffer for compute and transfer oparations
	const Unique<VkCommandPool>	  commandPool(createCommandPool(deviceInterface, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,  queueFamilyIndex));
	const Unique<VkCommandBuffer> commandBuffer(makeCommandBuffer(deviceInterface, device, *commandPool));

	// Start recording commands
	beginCommandBuffer(deviceInterface, *commandBuffer);

	{
		VkImageMemoryBarrier imageOutputAttachmentBarriers[2];

		imageOutputAttachmentBarriers[0] = makeImageMemoryBarrier
		(
			0u,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			**imageMS,
			fullImageRange
		);

		imageOutputAttachmentBarriers[1] = makeImageMemoryBarrier
		(
			0u,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			**imageRS,
			fullImageRange
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 2u, imageOutputAttachmentBarriers);
	}

	{
		const VkDeviceSize vertexStartOffset = 0u;

		std::vector<VkClearValue> clearValues;
		clearValues.push_back(makeClearValueColor(tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		clearValues.push_back(makeClearValueColor(tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f)));

		const vk::VkRect2D renderArea =
		{
			makeOffset2D(0u, 0u),
			makeExtent2D(imageMSInfo.extent.width, imageMSInfo.extent.height),
		};

		// Begin render pass
		const VkRenderPassBeginInfo renderPassBeginInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,		// VkStructureType         sType;
			DE_NULL,										// const void*             pNext;
			*renderPass,										// VkRenderPass            renderPass;
			*framebuffer,									// VkFramebuffer           framebuffer;
			renderArea,										// VkRect2D                renderArea;
			static_cast<deUint32>(clearValues.size()),		// deUint32                clearValueCount;
			&clearValues[0],								// const VkClearValue*     pClearValues;
		};

		deviceInterface.cmdBeginRenderPass(*commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind graphics pipeline
		deviceInterface.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);

		// Bind vertex buffer
		deviceInterface.cmdBindVertexBuffers(*commandBuffer, 0u, 1u, &vertexBuffer->get(), &vertexStartOffset);

		// Draw full screen quad
		deviceInterface.cmdDraw(*commandBuffer, vertexDataDesc.verticesCount, 1u, 0u, 0u);

		// End render pass
		deviceInterface.cmdEndRenderPass(*commandBuffer);
	}

	const VkImage sourceImage = m_imageMSParams.numSamples == VK_SAMPLE_COUNT_1_BIT ? **imageMS : **imageRS;

	{
		const VkImageMemoryBarrier imageTransferSrcBarrier = makeImageMemoryBarrier
		(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			sourceImage,
			fullImageRange
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imageTransferSrcBarrier);
	}

	// Copy data from resolve image to buffer
	const deUint32				imageRSSizeInBytes = getImageSizeInBytes(imageRSInfo.extent, imageRSInfo.arrayLayers, m_imageFormat, imageRSInfo.mipLevels);

	const VkBufferCreateInfo	bufferRSInfo = makeBufferCreateInfo(imageRSSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	const de::UniquePtr<Buffer>	bufferRS(new Buffer(deviceInterface, device, allocator, bufferRSInfo, MemoryRequirement::HostVisible));

	{
		const VkBufferImageCopy bufferImageCopy =
		{
			0u,																						//	VkDeviceSize				bufferOffset;
			0u,																						//	deUint32					bufferRowLength;
			0u,																						//	deUint32					bufferImageHeight;
			makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, imageRSInfo.arrayLayers),	//	VkImageSubresourceLayers	imageSubresource;
			makeOffset3D(0, 0, 0),																	//	VkOffset3D					imageOffset;
			imageRSInfo.extent,																		//	VkExtent3D					imageExtent;
		};

		deviceInterface.cmdCopyImageToBuffer(*commandBuffer, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, bufferRS->get(), 1u, &bufferImageCopy);
	}

	{
		const VkBufferMemoryBarrier bufferRSHostReadBarrier = makeBufferMemoryBarrier
		(
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_HOST_READ_BIT,
			bufferRS->get(),
			0u,
			imageRSSizeInBytes
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL, 1u, &bufferRSHostReadBarrier, 0u, DE_NULL);
	}

	// End recording commands
	VK_CHECK(deviceInterface.endCommandBuffer(*commandBuffer));

	// Submit commands for execution and wait for completion
	submitCommandsAndWait(deviceInterface, device, queue, *commandBuffer);

	// Retrieve data from buffer to host memory
	const Allocation& bufferRSAllocation = bufferRS->getAllocation();

	invalidateMappedMemoryRange(deviceInterface, device, bufferRSAllocation.getMemory(), bufferRSAllocation.getOffset(), VK_WHOLE_SIZE);

	const tcu::ConstPixelBufferAccess bufferRSData (m_imageFormat,
													imageRSInfo.extent.width,
													imageRSInfo.extent.height,
													imageRSInfo.extent.depth * imageRSInfo.arrayLayers,
													bufferRSAllocation.getHostPtr());

	std::stringstream imageName;
	imageName << getImageTypeName(m_imageType) << "_" << bufferRSData.getWidth() << "_" << bufferRSData.getHeight() << "_" << bufferRSData.getDepth() << std::endl;

	m_context.getTestContext().getLog()
		<< tcu::TestLog::Section(imageName.str(), imageName.str())
		<< tcu::LogImage("image", "", bufferRSData)
		<< tcu::TestLog::EndSection;

	return verifyImageData(imageRSInfo, bufferRSData);
}

} // multisample
} // pipeline
} // vkt
