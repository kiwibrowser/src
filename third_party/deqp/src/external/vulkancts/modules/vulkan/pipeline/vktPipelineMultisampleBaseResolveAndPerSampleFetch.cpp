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
* \file vktPipelineMultisampleBaseResolveAndPerSampleFetch.cpp
* \brief Base class for tests that check results of multisample resolve
*		  and/or values of individual samples
*//*--------------------------------------------------------------------*/

#include "vktPipelineMultisampleBaseResolveAndPerSampleFetch.hpp"
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

void MSCaseBaseResolveAndPerSampleFetch::initPrograms (vk::SourceCollections& programCollection) const
{
	// Create vertex shader
	std::ostringstream vs;

	vs << "#version 440\n"
		<< "layout(location = 0) in vec4 vs_in_position_ndc;\n"
		<< "\n"
		<< "out gl_PerVertex {\n"
		<< "	vec4  gl_Position;\n"
		<< "};\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position	= vs_in_position_ndc;\n"
		<< "}\n";

	programCollection.glslSources.add("per_sample_fetch_vs") << glu::VertexSource(vs.str());

	// Create fragment shader
	std::ostringstream fs;

	fs << "#version 440\n"
		<< "\n"
		<< "layout(location = 0) out vec4 fs_out_color;\n"
		<< "\n"
		<< "layout(set = 0, binding = 0, input_attachment_index = 0) uniform subpassInputMS imageMS;\n"
		<< "\n"
		<< "layout(set = 0, binding = 1, std140) uniform SampleBlock {\n"
		<< "    int sampleNdx;\n"
		<< "};\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	fs_out_color = subpassLoad(imageMS, sampleNdx);\n"
		<< "}\n";

	programCollection.glslSources.add("per_sample_fetch_fs") << glu::FragmentSource(fs.str());
}

VkPipelineMultisampleStateCreateInfo MSInstanceBaseResolveAndPerSampleFetch::getMSStateCreateInfo (const ImageMSParams& imageMSParams) const
{
	const VkPipelineMultisampleStateCreateInfo multisampleStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineMultisampleStateCreateFlags)0u,						// VkPipelineMultisampleStateCreateFlags	flags;
		imageMSParams.numSamples,										// VkSampleCountFlagBits					rasterizationSamples;
		VK_TRUE,														// VkBool32									sampleShadingEnable;
		1.0f,															// float									minSampleShading;
		DE_NULL,														// const VkSampleMask*						pSampleMask;
		VK_FALSE,														// VkBool32									alphaToCoverageEnable;
		VK_FALSE,														// VkBool32									alphaToOneEnable;
	};

	return multisampleStateInfo;
}

const VkDescriptorSetLayout* MSInstanceBaseResolveAndPerSampleFetch::createMSPassDescSetLayout(const ImageMSParams& imageMSParams)
{
	DE_UNREF(imageMSParams);

	return DE_NULL;
}

const VkDescriptorSet* MSInstanceBaseResolveAndPerSampleFetch::createMSPassDescSet(const ImageMSParams& imageMSParams, const VkDescriptorSetLayout* descSetLayout)
{
	DE_UNREF(imageMSParams);
	DE_UNREF(descSetLayout);

	return DE_NULL;
}

tcu::TestStatus MSInstanceBaseResolveAndPerSampleFetch::iterate (void)
{
	const InstanceInterface&	instance			= m_context.getInstanceInterface();
	const DeviceInterface&		deviceInterface		= m_context.getDeviceInterface();
	const VkDevice				device				= m_context.getDevice();
	const VkPhysicalDevice		physicalDevice		= m_context.getPhysicalDevice();
	Allocator&					allocator			= m_context.getDefaultAllocator();
	const VkQueue				queue				= m_context.getUniversalQueue();
	const deUint32				queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();

	VkImageCreateInfo			imageMSInfo;
	VkImageCreateInfo			imageRSInfo;
	const deUint32				firstSubpassAttachmentsCount = 2u;

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
	imageMSInfo.usage					= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
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
	imageRSInfo.usage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	validateImageInfo(instance, physicalDevice, imageRSInfo);

	const de::UniquePtr<Image> imageRS(new Image(deviceInterface, device, allocator, imageRSInfo, MemoryRequirement::Any));

	const deUint32 numSamples = static_cast<deUint32>(imageMSInfo.samples);

	std::vector<de::SharedPtr<Image> > imagesPerSampleVec(numSamples);

	for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
	{
		imagesPerSampleVec[sampleNdx] = de::SharedPtr<Image>(new Image(deviceInterface, device, allocator, imageRSInfo, MemoryRequirement::Any));
	}

	// Create render pass
	std::vector<VkAttachmentDescription> attachments(firstSubpassAttachmentsCount + numSamples);

	{
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
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL	// VkImageLayout					finalLayout;
		};

		attachments[0] = attachmentMSDesc;

		const VkAttachmentDescription attachmentRSDesc =
		{
			(VkAttachmentDescriptionFlags)0u,			// VkAttachmentDescriptionFlags		flags;
			imageRSInfo.format,							// VkFormat							format;
			imageRSInfo.samples,						// VkSampleCountFlagBits			samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,				// VkAttachmentLoadOp				loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,				// VkAttachmentStoreOp				storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,			// VkAttachmentLoadOp				stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,			// VkAttachmentStoreOp				stencilStoreOp;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout					initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout					finalLayout;
		};

		attachments[1] = attachmentRSDesc;

		for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
		{
			attachments[firstSubpassAttachmentsCount + sampleNdx] = attachmentRSDesc;
		}
	}

	const VkAttachmentReference attachmentMSColorRef =
	{
		0u,											// deUint32			attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
	};

	const VkAttachmentReference attachmentMSInputRef =
	{
		0u,											// deUint32			attachment;
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL	// VkImageLayout	layout;
	};

	const VkAttachmentReference attachmentRSColorRef =
	{
		1u,											// deUint32			attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
	};

	std::vector<VkAttachmentReference> perSampleAttachmentRef(numSamples);

	for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
	{
		const VkAttachmentReference attachmentRef =
		{
			firstSubpassAttachmentsCount + sampleNdx,	// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
		};

		perSampleAttachmentRef[sampleNdx] = attachmentRef;
	}

	std::vector<deUint32> preserveAttachments(1u + numSamples);

	for (deUint32 attachNdx = 0u; attachNdx < 1u + numSamples; ++attachNdx)
	{
		preserveAttachments[attachNdx] = 1u + attachNdx;
	}

	std::vector<VkSubpassDescription> subpasses(1u + numSamples);
	std::vector<VkSubpassDependency>  subpassDependencies(numSamples);

	const VkSubpassDescription firstSubpassDesc =
	{
		(VkSubpassDescriptionFlags)0u,		// VkSubpassDescriptionFlags		flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,	// VkPipelineBindPoint				pipelineBindPoint;
		0u,									// deUint32							inputAttachmentCount;
		DE_NULL,							// const VkAttachmentReference*		pInputAttachments;
		1u,									// deUint32							colorAttachmentCount;
		&attachmentMSColorRef,				// const VkAttachmentReference*		pColorAttachments;
		&attachmentRSColorRef,				// const VkAttachmentReference*		pResolveAttachments;
		DE_NULL,							// const VkAttachmentReference*		pDepthStencilAttachment;
		0u,									// deUint32							preserveAttachmentCount;
		DE_NULL								// const deUint32*					pPreserveAttachments;
	};

	subpasses[0] = firstSubpassDesc;

	for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
	{
		const VkSubpassDescription subpassDesc =
		{
			(VkSubpassDescriptionFlags)0u,			// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,		// VkPipelineBindPoint				pipelineBindPoint;
			1u,										// deUint32							inputAttachmentCount;
			&attachmentMSInputRef,					// const VkAttachmentReference*		pInputAttachments;
			1u,										// deUint32							colorAttachmentCount;
			&perSampleAttachmentRef[sampleNdx],		// const VkAttachmentReference*		pColorAttachments;
			DE_NULL,								// const VkAttachmentReference*		pResolveAttachments;
			DE_NULL,								// const VkAttachmentReference*		pDepthStencilAttachment;
			1u + sampleNdx,							// deUint32							preserveAttachmentCount;
			dataPointer(preserveAttachments)		// const deUint32*					pPreserveAttachments;
		};

		subpasses[1u + sampleNdx] = subpassDesc;

		const VkSubpassDependency subpassDependency =
		{
			0u,												// uint32_t                srcSubpass;
			1u + sampleNdx,									// uint32_t                dstSubpass;
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // VkPipelineStageFlags    srcStageMask;
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,			// VkPipelineStageFlags    dstStageMask;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			// VkAccessFlags           srcAccessMask;
			VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,			// VkAccessFlags           dstAccessMask;
			0u,												// VkDependencyFlags       dependencyFlags;
		};

		subpassDependencies[sampleNdx] = subpassDependency;
	}

	const VkRenderPassCreateInfo renderPassInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		(VkRenderPassCreateFlags)0u,						// VkRenderPassCreateFlags			flags;
		static_cast<deUint32>(attachments.size()),			// deUint32							attachmentCount;
		dataPointer(attachments),							// const VkAttachmentDescription*	pAttachments;
		static_cast<deUint32>(subpasses.size()),			// deUint32							subpassCount;
		dataPointer(subpasses),								// const VkSubpassDescription*		pSubpasses;
		static_cast<deUint32>(subpassDependencies.size()),	// deUint32							dependencyCount;
		dataPointer(subpassDependencies)					// const VkSubpassDependency*		pDependencies;
	};

	const Unique<VkRenderPass> renderPass(createRenderPass(deviceInterface, device, &renderPassInfo));

	const VkImageSubresourceRange fullImageRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, imageMSInfo.mipLevels, 0u, imageMSInfo.arrayLayers);

	// Create color attachments image views
	typedef de::SharedPtr<Unique<VkImageView> > VkImageViewSp;
	std::vector<VkImageViewSp>	imageViewsShPtrs(firstSubpassAttachmentsCount + numSamples);
	std::vector<VkImageView>	imageViews(firstSubpassAttachmentsCount + numSamples);

	imageViewsShPtrs[0] = makeVkSharedPtr(makeImageView(deviceInterface, device, **imageMS, mapImageViewType(m_imageType), imageMSInfo.format, fullImageRange));
	imageViewsShPtrs[1] = makeVkSharedPtr(makeImageView(deviceInterface, device, **imageRS, mapImageViewType(m_imageType), imageRSInfo.format, fullImageRange));

	imageViews[0] = **imageViewsShPtrs[0];
	imageViews[1] = **imageViewsShPtrs[1];

	for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
	{
		imageViewsShPtrs[firstSubpassAttachmentsCount + sampleNdx] = makeVkSharedPtr(makeImageView(deviceInterface, device, **imagesPerSampleVec[sampleNdx], mapImageViewType(m_imageType), imageRSInfo.format, fullImageRange));
		imageViews[firstSubpassAttachmentsCount + sampleNdx] = **imageViewsShPtrs[firstSubpassAttachmentsCount + sampleNdx];
	}

	// Create framebuffer
	const VkFramebufferCreateInfo framebufferInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// VkStructureType							   sType;
		DE_NULL,									// const void*                                 pNext;
		(VkFramebufferCreateFlags)0u,				// VkFramebufferCreateFlags                    flags;
		*renderPass,								// VkRenderPass                                renderPass;
		static_cast<deUint32>(imageViews.size()),	// uint32_t                                    attachmentCount;
		dataPointer(imageViews),					// const VkImageView*                          pAttachments;
		imageMSInfo.extent.width,					// uint32_t                                    width;
		imageMSInfo.extent.height,					// uint32_t                                    height;
		imageMSInfo.arrayLayers,					// uint32_t                                    layers;
	};

	const Unique<VkFramebuffer> framebuffer(createFramebuffer(deviceInterface, device, &framebufferInfo));

	const VkDescriptorSetLayout* descriptorSetLayoutMSPass = createMSPassDescSetLayout(m_imageMSParams);

	// Create pipeline layout
	const VkPipelineLayoutCreateInfo pipelineLayoutMSPassParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,	// VkStructureType					sType;
		DE_NULL,										// const void*						pNext;
		(VkPipelineLayoutCreateFlags)0u,				// VkPipelineLayoutCreateFlags		flags;
		descriptorSetLayoutMSPass ? 1u : 0u,			// deUint32							setLayoutCount;
		descriptorSetLayoutMSPass,						// const VkDescriptorSetLayout*		pSetLayouts;
		0u,												// deUint32							pushConstantRangeCount;
		DE_NULL,										// const VkPushConstantRange*		pPushConstantRanges;
	};

	const Unique<VkPipelineLayout> pipelineLayoutMSPass(createPipelineLayout(deviceInterface, device, &pipelineLayoutMSPassParams));

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

	const VkPipelineMultisampleStateCreateInfo multisampleStateInfo = getMSStateCreateInfo(m_imageMSParams);

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

	// Create graphics pipeline for multisample pass
	const Unique<VkShaderModule> vsMSPassModule(createShaderModule(deviceInterface, device, m_context.getBinaryCollection().get("vertex_shader"), (VkShaderModuleCreateFlags)0u));

	const VkPipelineShaderStageCreateInfo vsMSPassShaderStageInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,			// VkStructureType						sType;
		DE_NULL,														// const void*							pNext;
		(VkPipelineShaderStageCreateFlags)0u,							// VkPipelineShaderStageCreateFlags		flags;
		VK_SHADER_STAGE_VERTEX_BIT,										// VkShaderStageFlagBits				stage;
		*vsMSPassModule,												// VkShaderModule						module;
		"main",															// const char*							pName;
		DE_NULL,														// const VkSpecializationInfo*			pSpecializationInfo;
	};

	const Unique<VkShaderModule> fsMSPassModule(createShaderModule(deviceInterface, device, m_context.getBinaryCollection().get("fragment_shader"), (VkShaderModuleCreateFlags)0u));

	const VkPipelineShaderStageCreateInfo fsMSPassShaderStageInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,			// VkStructureType						sType;
		DE_NULL,														// const void*							pNext;
		(VkPipelineShaderStageCreateFlags)0u,							// VkPipelineShaderStageCreateFlags		flags;
		VK_SHADER_STAGE_FRAGMENT_BIT,									// VkShaderStageFlagBits				stage;
		*fsMSPassModule,												// VkShaderModule						module;
		"main",															// const char*							pName;
		DE_NULL,														// const VkSpecializationInfo*			pSpecializationInfo;
	};

	const VkPipelineShaderStageCreateInfo shaderStageInfosMSPass[] = { vsMSPassShaderStageInfo, fsMSPassShaderStageInfo };

	const VkGraphicsPipelineCreateInfo graphicsPipelineInfoMSPass =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,				// VkStructureType									sType;
		DE_NULL,														// const void*										pNext;
		(VkPipelineCreateFlags)0u,										// VkPipelineCreateFlags							flags;
		2u,																// deUint32											stageCount;
		shaderStageInfosMSPass,											// const VkPipelineShaderStageCreateInfo*			pStages;
		&vertexInputStateInfo,											// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
		&inputAssemblyStateInfo,										// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
		DE_NULL,														// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
		&viewportStateInfo,												// const VkPipelineViewportStateCreateInfo*			pViewportState;
		&rasterizationStateInfo,										// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
		&multisampleStateInfo,											// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
		&depthStencilStateInfo,											// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
		&colorBlendStateInfo,											// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
		DE_NULL,														// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
		*pipelineLayoutMSPass,											// VkPipelineLayout									layout;
		*renderPass,													// VkRenderPass										renderPass;
		0u,																// deUint32											subpass;
		DE_NULL,														// VkPipeline										basePipelineHandle;
		0u,																// deInt32											basePipelineIndex;
	};

	const Unique<VkPipeline> graphicsPipelineMSPass(createGraphicsPipeline(deviceInterface, device, DE_NULL, &graphicsPipelineInfoMSPass));

	typedef de::SharedPtr<Unique<VkPipeline> > VkPipelineSp;
	std::vector<VkPipelineSp> graphicsPipelinesPerSampleFetch(numSamples);

	// Create descriptor set layout
	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(deviceInterface, device));

	const Unique<VkPipelineLayout> pipelineLayoutPerSampleFetchPass(makePipelineLayout(deviceInterface, device, *descriptorSetLayout));

	const deUint32 bufferPerSampleFetchPassSize = 4u * (deUint32)sizeof(tcu::Vec4);

	de::SharedPtr<Buffer> vertexBufferPerSampleFetchPass = de::SharedPtr<Buffer>(new Buffer(deviceInterface, device, allocator, makeBufferCreateInfo(bufferPerSampleFetchPassSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible));

	// Create graphics pipelines for per sample texel fetch passes
	{
		const Unique<VkShaderModule> vsPerSampleFetchPassModule(createShaderModule(deviceInterface, device, m_context.getBinaryCollection().get("per_sample_fetch_vs"), (VkShaderModuleCreateFlags)0u));

		const VkPipelineShaderStageCreateInfo vsPerSampleFetchPassShaderStageInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,			// VkStructureType						sType;
			DE_NULL,														// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0u,							// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_VERTEX_BIT,										// VkShaderStageFlagBits				stage;
			*vsPerSampleFetchPassModule,									// VkShaderModule						module;
			"main",															// const char*							pName;
			DE_NULL,														// const VkSpecializationInfo*			pSpecializationInfo;
		};

		const Unique<VkShaderModule> fsPerSampleFetchPassModule(createShaderModule(deviceInterface, device, m_context.getBinaryCollection().get("per_sample_fetch_fs"), (VkShaderModuleCreateFlags)0u));

		const VkPipelineShaderStageCreateInfo fsPerSampleFetchPassShaderStageInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,			// VkStructureType						sType;
			DE_NULL,														// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0u,							// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,									// VkShaderStageFlagBits				stage;
			*fsPerSampleFetchPassModule,									// VkShaderModule						module;
			"main",															// const char*							pName;
			DE_NULL,														// const VkSpecializationInfo*			pSpecializationInfo;
		};

		const VkPipelineShaderStageCreateInfo shaderStageInfosPerSampleFetchPass[] = { vsPerSampleFetchPassShaderStageInfo, fsPerSampleFetchPassShaderStageInfo };

		std::vector<tcu::Vec4> vertices;

		vertices.push_back(tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f));
		vertices.push_back(tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f));
		vertices.push_back(tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f));
		vertices.push_back(tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f));

		const Allocation& vertexAllocPerSampleFetchPass = vertexBufferPerSampleFetchPass->getAllocation();

		deMemcpy(vertexAllocPerSampleFetchPass.getHostPtr(), dataPointer(vertices), static_cast<std::size_t>(bufferPerSampleFetchPassSize));

		flushMappedMemoryRange(deviceInterface, device, vertexAllocPerSampleFetchPass.getMemory(), vertexAllocPerSampleFetchPass.getOffset(), VK_WHOLE_SIZE);

		const VkVertexInputBindingDescription vertexBindingPerSampleFetchPass =
		{
			0u,							// deUint32				binding;
			sizeof(tcu::Vec4),			// deUint32				stride;
			VK_VERTEX_INPUT_RATE_VERTEX	// VkVertexInputRate	inputRate;
		};

		const VkVertexInputAttributeDescription vertexAttribPositionNdc =
		{
			0u,											// deUint32	location;
			0u,											// deUint32	binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,				// VkFormat	format;
			0u,											// deUint32	offset;
		};

		const VkPipelineVertexInputStateCreateInfo vertexInputStatePerSampleFetchPass =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,			// VkStructureType                             sType;
			DE_NULL,															// const void*                                 pNext;
			(VkPipelineVertexInputStateCreateFlags)0u,							// VkPipelineVertexInputStateCreateFlags       flags;
			1u,																	// uint32_t                                    vertexBindingDescriptionCount;
			&vertexBindingPerSampleFetchPass,									// const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
			1u,																	// uint32_t                                    vertexAttributeDescriptionCount;
			&vertexAttribPositionNdc,											// const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
		};

		const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStatePerSampleFetchPass =
		{
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType                             sType;
			DE_NULL,														// const void*                                 pNext;
			(VkPipelineInputAssemblyStateCreateFlags)0u,					// VkPipelineInputAssemblyStateCreateFlags     flags;
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,								// VkPrimitiveTopology                         topology;
			VK_FALSE,														// VkBool32                                    primitiveRestartEnable;
		};

		const VkPipelineMultisampleStateCreateInfo multisampleStatePerSampleFetchPass =
		{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			(VkPipelineMultisampleStateCreateFlags)0u,						// VkPipelineMultisampleStateCreateFlags	flags;
			VK_SAMPLE_COUNT_1_BIT,											// VkSampleCountFlagBits					rasterizationSamples;
			VK_FALSE,														// VkBool32									sampleShadingEnable;
			0.0f,															// float									minSampleShading;
			DE_NULL,														// const VkSampleMask*						pSampleMask;
			VK_FALSE,														// VkBool32									alphaToCoverageEnable;
			VK_FALSE,														// VkBool32									alphaToOneEnable;
		};

		for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
		{
			const VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
			{
				VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,				// VkStructureType									sType;
				DE_NULL,														// const void*										pNext;
				(VkPipelineCreateFlags)0u,										// VkPipelineCreateFlags							flags;
				2u,																// deUint32											stageCount;
				shaderStageInfosPerSampleFetchPass,								// const VkPipelineShaderStageCreateInfo*			pStages;
				&vertexInputStatePerSampleFetchPass,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
				&inputAssemblyStatePerSampleFetchPass,							// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
				DE_NULL,														// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
				&viewportStateInfo,												// const VkPipelineViewportStateCreateInfo*			pViewportState;
				&rasterizationStateInfo,										// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
				&multisampleStatePerSampleFetchPass,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
				&depthStencilStateInfo,											// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
				&colorBlendStateInfo,											// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
				DE_NULL,														// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
				*pipelineLayoutPerSampleFetchPass,								// VkPipelineLayout									layout;
				*renderPass,													// VkRenderPass										renderPass;
				1u + sampleNdx,													// deUint32											subpass;
				DE_NULL,														// VkPipeline										basePipelineHandle;
				0u,																// deInt32											basePipelineIndex;
			};

			graphicsPipelinesPerSampleFetch[sampleNdx] = makeVkSharedPtr(createGraphicsPipeline(deviceInterface, device, DE_NULL, &graphicsPipelineInfo));
		}
	}

	// Create descriptor pool
	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1u)
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1u)
		.build(deviceInterface, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	// Create descriptor set
	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(deviceInterface, device, *descriptorPool, *descriptorSetLayout));

	const VkPhysicalDeviceLimits deviceLimits = getPhysicalDeviceProperties(instance, physicalDevice).limits;

	VkDeviceSize uboOffsetAlignment = sizeof(deInt32) < deviceLimits.minUniformBufferOffsetAlignment ? deviceLimits.minUniformBufferOffsetAlignment : sizeof(deInt32);

	uboOffsetAlignment += (deviceLimits.minUniformBufferOffsetAlignment - uboOffsetAlignment % deviceLimits.minUniformBufferOffsetAlignment) % deviceLimits.minUniformBufferOffsetAlignment;

	const VkBufferCreateInfo	bufferSampleIDInfo = makeBufferCreateInfo(uboOffsetAlignment * numSamples, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	const de::UniquePtr<Buffer>	bufferSampleID(new Buffer(deviceInterface, device, allocator, bufferSampleIDInfo, MemoryRequirement::HostVisible));

	std::vector<deUint32> sampleIDsOffsets(numSamples);

	{
		deInt8* sampleIDs = new deInt8[uboOffsetAlignment * numSamples];

		for (deInt32 sampleNdx = 0u; sampleNdx < static_cast<deInt32>(numSamples); ++sampleNdx)
		{
			sampleIDsOffsets[sampleNdx] = static_cast<deUint32>(sampleNdx * uboOffsetAlignment);
			deInt8* samplePtr = sampleIDs + sampleIDsOffsets[sampleNdx];

			deMemcpy(samplePtr, &sampleNdx, sizeof(deInt32));
		}

		deMemcpy(bufferSampleID->getAllocation().getHostPtr(), sampleIDs, static_cast<deUint32>(uboOffsetAlignment * numSamples));

		flushMappedMemoryRange(deviceInterface, device, bufferSampleID->getAllocation().getMemory(), bufferSampleID->getAllocation().getOffset(), VK_WHOLE_SIZE);

		delete[] sampleIDs;
	}

	{
		const VkDescriptorImageInfo	 descImageInfo  = makeDescriptorImageInfo(DE_NULL, imageViews[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		const VkDescriptorBufferInfo descBufferInfo	= makeDescriptorBufferInfo(**bufferSampleID, 0u, sizeof(deInt32));

		DescriptorSetUpdateBuilder()
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &descImageInfo)
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, &descBufferInfo)
			.update(deviceInterface, device);
	}

	// Create command buffer for compute and transfer oparations
	const Unique<VkCommandPool>	  commandPool(createCommandPool(deviceInterface, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer> commandBuffer(makeCommandBuffer(deviceInterface, device, *commandPool));

	// Start recording commands
	beginCommandBuffer(deviceInterface, *commandBuffer);

	{
		std::vector<VkImageMemoryBarrier> imageOutputAttachmentBarriers(firstSubpassAttachmentsCount + numSamples);

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

		for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
		{
			imageOutputAttachmentBarriers[firstSubpassAttachmentsCount + sampleNdx] = makeImageMemoryBarrier
			(
				0u,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				**imagesPerSampleVec[sampleNdx],
				fullImageRange
			);
		}

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL,
			static_cast<deUint32>(imageOutputAttachmentBarriers.size()), dataPointer(imageOutputAttachmentBarriers));
	}

	{
		const VkDeviceSize vertexStartOffset = 0u;

		std::vector<VkClearValue> clearValues(firstSubpassAttachmentsCount + numSamples);
		for (deUint32 attachmentNdx = 0u; attachmentNdx < firstSubpassAttachmentsCount + numSamples; ++attachmentNdx)
		{
			clearValues[attachmentNdx] = makeClearValueColor(tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
		}

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
			*renderPass,									// VkRenderPass            renderPass;
			*framebuffer,									// VkFramebuffer           framebuffer;
			renderArea,										// VkRect2D                renderArea;
			static_cast<deUint32>(clearValues.size()),		// deUint32                clearValueCount;
			dataPointer(clearValues),						// const VkClearValue*     pClearValues;
		};

		deviceInterface.cmdBeginRenderPass(*commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind graphics pipeline
		deviceInterface.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipelineMSPass);

		const VkDescriptorSet* descriptorSetMSPass = createMSPassDescSet(m_imageMSParams, descriptorSetLayoutMSPass);

		if (descriptorSetMSPass)
		{
			// Bind descriptor set
			deviceInterface.cmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayoutMSPass, 0u, 1u, descriptorSetMSPass, 0u, DE_NULL);
		}

		// Bind vertex buffer
		deviceInterface.cmdBindVertexBuffers(*commandBuffer, 0u, 1u, &vertexBuffer->get(), &vertexStartOffset);

		// Perform a draw
		deviceInterface.cmdDraw(*commandBuffer, vertexDataDesc.verticesCount, 1u, 0u, 0u);

		for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
		{
			deviceInterface.cmdNextSubpass(*commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

			// Bind graphics pipeline
			deviceInterface.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **graphicsPipelinesPerSampleFetch[sampleNdx]);

			// Bind descriptor set
			deviceInterface.cmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayoutPerSampleFetchPass, 0u, 1u, &descriptorSet.get(), 1u, &sampleIDsOffsets[sampleNdx]);

			// Bind vertex buffer
			deviceInterface.cmdBindVertexBuffers(*commandBuffer, 0u, 1u, &vertexBufferPerSampleFetchPass->get(), &vertexStartOffset);

			// Perform a draw
			deviceInterface.cmdDraw(*commandBuffer, 4u, 1u, 0u, 0u);
		}

		// End render pass
		deviceInterface.cmdEndRenderPass(*commandBuffer);
	}

	{
		const VkImageMemoryBarrier imageRSTransferBarrier = makeImageMemoryBarrier
		(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			**imageRS,
			fullImageRange
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imageRSTransferBarrier);
	}

	// Copy data from imageRS to buffer
	const deUint32				imageRSSizeInBytes = getImageSizeInBytes(imageRSInfo.extent, imageRSInfo.arrayLayers, m_imageFormat, imageRSInfo.mipLevels, 1u);

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

		deviceInterface.cmdCopyImageToBuffer(*commandBuffer, **imageRS, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, bufferRS->get(), 1u, &bufferImageCopy);
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

	// Copy data from per sample images to buffers
	std::vector<VkImageMemoryBarrier> imagesPerSampleTransferBarriers(numSamples);

	for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
	{
		imagesPerSampleTransferBarriers[sampleNdx] = makeImageMemoryBarrier
		(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			**imagesPerSampleVec[sampleNdx],
			fullImageRange
		);
	}

	deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL,
		static_cast<deUint32>(imagesPerSampleTransferBarriers.size()), dataPointer(imagesPerSampleTransferBarriers));

	std::vector<de::SharedPtr<Buffer> > buffersPerSample(numSamples);

	for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
	{
		buffersPerSample[sampleNdx] = de::SharedPtr<Buffer>(new Buffer(deviceInterface, device, allocator, bufferRSInfo, MemoryRequirement::HostVisible));

		const VkBufferImageCopy bufferImageCopy =
		{
			0u,																						//	VkDeviceSize				bufferOffset;
			0u,																						//	deUint32					bufferRowLength;
			0u,																						//	deUint32					bufferImageHeight;
			makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, imageRSInfo.arrayLayers),	//	VkImageSubresourceLayers	imageSubresource;
			makeOffset3D(0, 0, 0),																	//	VkOffset3D					imageOffset;
			imageRSInfo.extent,																		//	VkExtent3D					imageExtent;
		};

		deviceInterface.cmdCopyImageToBuffer(*commandBuffer, **imagesPerSampleVec[sampleNdx], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, **buffersPerSample[sampleNdx], 1u, &bufferImageCopy);
	}

	std::vector<VkBufferMemoryBarrier> buffersPerSampleHostReadBarriers(numSamples);

	for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
	{
		buffersPerSampleHostReadBarriers[sampleNdx] = makeBufferMemoryBarrier
		(
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_HOST_READ_BIT,
			**buffersPerSample[sampleNdx],
			0u,
			imageRSSizeInBytes
		);
	}

	deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL,
		static_cast<deUint32>(buffersPerSampleHostReadBarriers.size()), dataPointer(buffersPerSampleHostReadBarriers), 0u, DE_NULL);

	// End recording commands
	VK_CHECK(deviceInterface.endCommandBuffer(*commandBuffer));

	// Submit commands for execution and wait for completion
	submitCommandsAndWait(deviceInterface, device, queue, *commandBuffer);

	// Retrieve data from bufferRS to host memory
	const Allocation& bufferRSAlloc = bufferRS->getAllocation();

	invalidateMappedMemoryRange(deviceInterface, device, bufferRSAlloc.getMemory(), bufferRSAlloc.getOffset(), VK_WHOLE_SIZE);

	const tcu::ConstPixelBufferAccess bufferRSData (m_imageFormat,
													imageRSInfo.extent.width,
													imageRSInfo.extent.height,
													imageRSInfo.extent.depth * imageRSInfo.arrayLayers,
													bufferRSAlloc.getHostPtr());

	std::stringstream resolveName;
	resolveName << "Resolve image " << getImageTypeName(m_imageType) << "_" << bufferRSData.getWidth() << "_" << bufferRSData.getHeight() << "_" << bufferRSData.getDepth() << std::endl;

	m_context.getTestContext().getLog()
		<< tcu::TestLog::Section(resolveName.str(), resolveName.str())
		<< tcu::LogImage("resolve", "", bufferRSData)
		<< tcu::TestLog::EndSection;

	std::vector<tcu::ConstPixelBufferAccess> buffersPerSampleData(numSamples);

	// Retrieve data from per sample buffers to host memory
	for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
	{
		const Allocation& bufferAlloc = buffersPerSample[sampleNdx]->getAllocation();

		invalidateMappedMemoryRange(deviceInterface, device, bufferAlloc.getMemory(), bufferAlloc.getOffset(), VK_WHOLE_SIZE);

		buffersPerSampleData[sampleNdx] = tcu::ConstPixelBufferAccess
		(
			m_imageFormat,
			imageRSInfo.extent.width,
			imageRSInfo.extent.height,
			imageRSInfo.extent.depth * imageRSInfo.arrayLayers,
			bufferAlloc.getHostPtr()
		);

		std::stringstream sampleName;
		sampleName << "Sample " << sampleNdx << " image" << std::endl;

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Section(sampleName.str(), sampleName.str())
			<< tcu::LogImage("sample", "", buffersPerSampleData[sampleNdx])
			<< tcu::TestLog::EndSection;
	}

	return verifyImageData(imageMSInfo, imageRSInfo, buffersPerSampleData, bufferRSData);
}

} // multisample
} // pipeline
} // vkt
