/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Geometry Utilities
 *//*--------------------------------------------------------------------*/

#include "vktGeometryTestsUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkDefs.hpp"
#include "tcuImageCompare.hpp"

#include "tcuImageIO.hpp"

#include "deMath.h"

using namespace vk;

namespace vkt
{
namespace geometry
{

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setShader (const DeviceInterface&			vk,
															 const VkDevice					device,
															 const VkShaderStageFlagBits	stage,
															 const ProgramBinary&			binary,
															 const VkSpecializationInfo*	specInfo)
{
	VkShaderModule module;
	switch (stage)
	{
		case (VK_SHADER_STAGE_VERTEX_BIT):
			DE_ASSERT(m_vertexShaderModule.get() == DE_NULL);
			m_vertexShaderModule = createShaderModule(vk, device, binary, (VkShaderModuleCreateFlags)0);
			module = *m_vertexShaderModule;
			break;

		case (VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT):
			DE_ASSERT(m_tessControlShaderModule.get() == DE_NULL);
			m_tessControlShaderModule = createShaderModule(vk, device, binary, (VkShaderModuleCreateFlags)0);
			module = *m_tessControlShaderModule;
			break;

		case (VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT):
			DE_ASSERT(m_tessEvaluationShaderModule.get() == DE_NULL);
			m_tessEvaluationShaderModule = createShaderModule(vk, device, binary, (VkShaderModuleCreateFlags)0);
			module = *m_tessEvaluationShaderModule;
			break;

		case (VK_SHADER_STAGE_GEOMETRY_BIT):
			DE_ASSERT(m_geometryShaderModule.get() == DE_NULL);
			m_geometryShaderModule = createShaderModule(vk, device, binary, (VkShaderModuleCreateFlags)0);
			module = *m_geometryShaderModule;
			break;

		case (VK_SHADER_STAGE_FRAGMENT_BIT):
			DE_ASSERT(m_fragmentShaderModule.get() == DE_NULL);
			m_fragmentShaderModule = createShaderModule(vk, device, binary, (VkShaderModuleCreateFlags)0);
			module = *m_fragmentShaderModule;
			break;

		default:
			DE_FATAL("Invalid shader stage");
			return *this;
	}

	const VkPipelineShaderStageCreateInfo pipelineShaderStageInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
		DE_NULL,												// const void*							pNext;
		(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags		flags;
		stage,													// VkShaderStageFlagBits				stage;
		module,													// VkShaderModule						module;
		"main",													// const char*							pName;
		specInfo,												// const VkSpecializationInfo*			pSpecializationInfo;
	};

	m_shaderStageFlags |= stage;
	m_shaderStages.push_back(pipelineShaderStageInfo);

	return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexInputSingleAttribute (const VkFormat vertexFormat, const deUint32 stride)
{
	const VkVertexInputBindingDescription bindingDesc =
	{
		0u,									// uint32_t				binding;
		stride,								// uint32_t				stride;
		VK_VERTEX_INPUT_RATE_VERTEX,		// VkVertexInputRate	inputRate;
	};
	const VkVertexInputAttributeDescription attributeDesc =
	{
		0u,									// uint32_t			location;
		0u,									// uint32_t			binding;
		vertexFormat,						// VkFormat			format;
		0u,									// uint32_t			offset;
	};

	m_vertexInputBindings.clear();
	m_vertexInputBindings.push_back(bindingDesc);

	m_vertexInputAttributes.clear();
	m_vertexInputAttributes.push_back(attributeDesc);

	return *this;
}

template<typename T>
inline const T* dataPointer (const std::vector<T>& vec)
{
	return (vec.size() != 0 ? &vec[0] : DE_NULL);
}

Move<VkPipeline> GraphicsPipelineBuilder::build (const DeviceInterface&	vk,
												 const VkDevice			device,
												 const VkPipelineLayout	pipelineLayout,
												 const VkRenderPass		renderPass)
{
	const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineVertexInputStateCreateFlags)0,						// VkPipelineVertexInputStateCreateFlags       flags;
		static_cast<deUint32>(m_vertexInputBindings.size()),			// uint32_t                                    vertexBindingDescriptionCount;
		dataPointer(m_vertexInputBindings),								// const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
		static_cast<deUint32>(m_vertexInputAttributes.size()),			// uint32_t                                    vertexAttributeDescriptionCount;
		dataPointer(m_vertexInputAttributes),							// const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
	};

	const VkPrimitiveTopology topology = (m_shaderStageFlags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) ? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
																										 : m_primitiveTopology;

	VkBool32	primitiveRestartEnable = VK_TRUE;
	switch(m_primitiveTopology)
	{
		case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
			primitiveRestartEnable = VK_FALSE;
			break;
		default:
			break;
	};

	const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0,						// VkPipelineInputAssemblyStateCreateFlags     flags;
		topology,														// VkPrimitiveTopology                         topology;
		primitiveRestartEnable,											// VkBool32                                    primitiveRestartEnable;
	};

	const VkPipelineTessellationStateCreateInfo pipelineTessellationStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,		// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineTessellationStateCreateFlags)0,						// VkPipelineTessellationStateCreateFlags      flags;
		m_patchControlPoints,											// uint32_t                                    patchControlPoints;
	};

	const VkViewport viewport = makeViewport(
		0.0f, 0.0f,
		static_cast<float>(m_renderSize.x()), static_cast<float>(m_renderSize.y()),
		0.0f, 1.0f);

	const VkRect2D scissor = {
		makeOffset2D(0, 0),
		makeExtent2D(m_renderSize.x(), m_renderSize.y()),
	};

	const VkPipelineViewportStateCreateInfo pipelineViewportStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,	// VkStructureType                             sType;
		DE_NULL,												// const void*                                 pNext;
		(VkPipelineViewportStateCreateFlags)0,					// VkPipelineViewportStateCreateFlags          flags;
		1u,														// uint32_t                                    viewportCount;
		&viewport,												// const VkViewport*                           pViewports;
		1u,														// uint32_t                                    scissorCount;
		&scissor,												// const VkRect2D*                             pScissors;
	};

	const bool isRasterizationDisabled = ((m_shaderStageFlags & VK_SHADER_STAGE_FRAGMENT_BIT) == 0);
	const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType                          sType;
		DE_NULL,														// const void*                              pNext;
		(VkPipelineRasterizationStateCreateFlags)0,						// VkPipelineRasterizationStateCreateFlags  flags;
		VK_FALSE,														// VkBool32                                 depthClampEnable;
		isRasterizationDisabled,										// VkBool32                                 rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
		m_cullModeFlags,												// VkCullModeFlags							cullMode;
		m_frontFace,													// VkFrontFace								frontFace;
		VK_FALSE,														// VkBool32									depthBiasEnable;
		0.0f,															// float									depthBiasConstantFactor;
		0.0f,															// float									depthBiasClamp;
		0.0f,															// float									depthBiasSlopeFactor;
		1.0f,															// float									lineWidth;
	};

	const VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineMultisampleStateCreateFlags)0,					// VkPipelineMultisampleStateCreateFlags	flags;
		VK_SAMPLE_COUNT_1_BIT,										// VkSampleCountFlagBits					rasterizationSamples;
		VK_FALSE,													// VkBool32									sampleShadingEnable;
		0.0f,														// float									minSampleShading;
		DE_NULL,													// const VkSampleMask*						pSampleMask;
		VK_FALSE,													// VkBool32									alphaToCoverageEnable;
		VK_FALSE													// VkBool32									alphaToOneEnable;
	};

	const VkStencilOpState stencilOpState = makeStencilOpState(
		VK_STENCIL_OP_KEEP,		// stencil fail
		VK_STENCIL_OP_KEEP,		// depth & stencil pass
		VK_STENCIL_OP_KEEP,		// depth only fail
		VK_COMPARE_OP_NEVER,	// compare op
		0u,						// compare mask
		0u,						// write mask
		0u);					// reference

	const VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineDepthStencilStateCreateFlags)0,					// VkPipelineDepthStencilStateCreateFlags	flags;
		VK_FALSE,													// VkBool32									depthTestEnable;
		VK_FALSE,													// VkBool32									depthWriteEnable;
		VK_COMPARE_OP_LESS,											// VkCompareOp								depthCompareOp;
		VK_FALSE,													// VkBool32									depthBoundsTestEnable;
		VK_FALSE,													// VkBool32									stencilTestEnable;
		stencilOpState,												// VkStencilOpState							front;
		stencilOpState,												// VkStencilOpState							back;
		0.0f,														// float									minDepthBounds;
		1.0f,														// float									maxDepthBounds;
	};

	const VkColorComponentFlags colorComponentsAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	const VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState =
	{
		m_blendEnable,						// VkBool32					blendEnable;
		VK_BLEND_FACTOR_SRC_ALPHA,			// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ONE,				// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,					// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_SRC_ALPHA,			// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ONE,				// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,					// VkBlendOp				alphaBlendOp;
		colorComponentsAll,					// VkColorComponentFlags	colorWriteMask;
	};

	const VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
		DE_NULL,													// const void*									pNext;
		(VkPipelineColorBlendStateCreateFlags)0,					// VkPipelineColorBlendStateCreateFlags			flags;
		VK_FALSE,													// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
		1u,															// deUint32										attachmentCount;
		&pipelineColorBlendAttachmentState,							// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },									// float										blendConstants[4];
	};

	const VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,						// VkStructureType									sType;
		DE_NULL,																// const void*										pNext;
		(VkPipelineCreateFlags)0,												// VkPipelineCreateFlags							flags;
		static_cast<deUint32>(m_shaderStages.size()),							// deUint32											stageCount;
		&m_shaderStages[0],														// const VkPipelineShaderStageCreateInfo*			pStages;
		&vertexInputStateInfo,													// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
		&pipelineInputAssemblyStateInfo,										// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
		(m_shaderStageFlags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT ? &pipelineTessellationStateInfo : DE_NULL), // const VkPipelineTessellationStateCreateInfo*		pTessellationState;
		(isRasterizationDisabled ? DE_NULL : &pipelineViewportStateInfo),		// const VkPipelineViewportStateCreateInfo*			pViewportState;
		&pipelineRasterizationStateInfo,										// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
		(isRasterizationDisabled ? DE_NULL : &pipelineMultisampleStateInfo),	// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
		(isRasterizationDisabled ? DE_NULL : &pipelineDepthStencilStateInfo),	// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
		(isRasterizationDisabled ? DE_NULL : &pipelineColorBlendStateInfo),		// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
		DE_NULL,																// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
		pipelineLayout,															// VkPipelineLayout									layout;
		renderPass,																// VkRenderPass										renderPass;
		0u,																		// deUint32											subpass;
		DE_NULL,																// VkPipeline										basePipelineHandle;
		0,																		// deInt32											basePipelineIndex;
	};

	return createGraphicsPipeline(vk, device, DE_NULL, &graphicsPipelineInfo);
}

std::string inputTypeToGLString (const VkPrimitiveTopology& inputType)
{
	switch (inputType)
	{
		case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
			return "points";
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
			return "lines";
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
			return "lines_adjacency";
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
			return "triangles";
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
			return "triangles_adjacency";
		default:
			DE_ASSERT(DE_FALSE);
			return "error";
	}
}

std::string outputTypeToGLString (const VkPrimitiveTopology& outputType)
{
	switch (outputType)
	{
		case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
			return "points";
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
				return "line_strip";
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
			return "triangle_strip";
		default:
			DE_ASSERT(DE_FALSE);
			return "error";
	}
}

size_t calcOutputVertices (const VkPrimitiveTopology&  inputType)
{
	switch (inputType)
	{
		case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
			return 1 * 3;
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
			return 2 * 3;
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
			return 4 * 3;
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
			return 3 * 3;
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
			return 6 * 3;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

VkBufferCreateInfo makeBufferCreateInfo (const VkDeviceSize			bufferSize,
										 const VkBufferUsageFlags	usage)
{
	const VkBufferCreateInfo bufferCreateInfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		(VkBufferCreateFlags)0,					// VkBufferCreateFlags	flags;
		bufferSize,								// VkDeviceSize			size;
		usage,									// VkBufferUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
		0u,										// deUint32				queueFamilyIndexCount;
		DE_NULL,								// const deUint32*		pQueueFamilyIndices;
	};
	return bufferCreateInfo;
}

VkImageCreateInfo makeImageCreateInfo (const tcu::IVec2& size, const VkFormat format, const VkImageUsageFlags usage, const deUint32 numArrayLayers)
{
	const VkImageCreateInfo imageInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,		// VkStructureType          sType;
		DE_NULL,									// const void*              pNext;
		(VkImageCreateFlags)0,						// VkImageCreateFlags       flags;
		VK_IMAGE_TYPE_2D,							// VkImageType              imageType;
		format,										// VkFormat                 format;
		makeExtent3D(size.x(), size.y(), 1),		// VkExtent3D               extent;
		1u,											// uint32_t                 mipLevels;
		numArrayLayers,								// uint32_t                 arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits    samples;
		VK_IMAGE_TILING_OPTIMAL,					// VkImageTiling            tiling;
		usage,										// VkImageUsageFlags        usage;
		VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode            sharingMode;
		0u,											// uint32_t                 queueFamilyIndexCount;
		DE_NULL,									// const uint32_t*          pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,					// VkImageLayout            initialLayout;
	};
	return imageInfo;
}

Move<VkDescriptorSet> makeDescriptorSet (const DeviceInterface&			vk,
										 const VkDevice					device,
										 const VkDescriptorPool			descriptorPool,
										 const VkDescriptorSetLayout	setLayout)
{
	const VkDescriptorSetAllocateInfo info =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,		// VkStructureType				sType;
		DE_NULL,											// const void*					pNext;
		descriptorPool,										// VkDescriptorPool				descriptorPool;
		1u,													// deUint32						descriptorSetCount;
		&setLayout,											// const VkDescriptorSetLayout*	pSetLayouts;
	};
	return allocateDescriptorSet(vk, device, &info);
}

Move<VkRenderPass> makeRenderPass (const DeviceInterface& vk, const VkDevice device, const VkFormat colorFormat)
{
	const VkAttachmentDescription colorAttachmentDescription =
	{
		(VkAttachmentDescriptionFlags)0,				// VkAttachmentDescriptionFlags		flags;
		colorFormat,									// VkFormat							format;
		VK_SAMPLE_COUNT_1_BIT,							// VkSampleCountFlagBits			samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,					// VkAttachmentLoadOp				loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,					// VkAttachmentStoreOp				storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,				// VkAttachmentLoadOp				stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,				// VkAttachmentStoreOp				stencilStoreOp;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL		// VkImageLayout					finalLayout;
	};

	const VkAttachmentReference colorAttachmentReference =
	{
		0u,												// deUint32			attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL		// VkImageLayout	layout;
	};

	const VkAttachmentReference depthAttachmentReference =
	{
		VK_ATTACHMENT_UNUSED,							// deUint32			attachment;
		VK_IMAGE_LAYOUT_UNDEFINED						// VkImageLayout	layout;
	};

	const VkSubpassDescription subpassDescription =
	{
		(VkSubpassDescriptionFlags)0,					// VkSubpassDescriptionFlags		flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,				// VkPipelineBindPoint				pipelineBindPoint;
		0u,												// deUint32							inputAttachmentCount;
		DE_NULL,										// const VkAttachmentReference*		pInputAttachments;
		1u,												// deUint32							colorAttachmentCount;
		&colorAttachmentReference,						// const VkAttachmentReference*		pColorAttachments;
		DE_NULL,										// const VkAttachmentReference*		pResolveAttachments;
		&depthAttachmentReference,						// const VkAttachmentReference*		pDepthStencilAttachment;
		0u,												// deUint32							preserveAttachmentCount;
		DE_NULL											// const deUint32*					pPreserveAttachments;
	};

	const VkRenderPassCreateInfo renderPassInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,		// VkStructureType					sType;
		DE_NULL,										// const void*						pNext;
		(VkRenderPassCreateFlags)0,						// VkRenderPassCreateFlags			flags;
		1u,												// deUint32							attachmentCount;
		&colorAttachmentDescription,					// const VkAttachmentDescription*	pAttachments;
		1u,												// deUint32							subpassCount;
		&subpassDescription,							// const VkSubpassDescription*		pSubpasses;
		0u,												// deUint32							dependencyCount;
		DE_NULL											// const VkSubpassDependency*		pDependencies;
	};

	return createRenderPass(vk, device, &renderPassInfo);
}

Move<VkImageView> makeImageView (const DeviceInterface&			vk,
								 const VkDevice					vkDevice,
								 const VkImage					image,
								 const VkImageViewType			viewType,
								 const VkFormat					format,
								 const VkImageSubresourceRange	subresourceRange)
{
	const VkImageViewCreateInfo imageViewParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		(VkImageViewCreateFlags)0,					// VkImageViewCreateFlags	flags;
		image,										// VkImage					image;
		viewType,									// VkImageViewType			viewType;
		format,										// VkFormat					format;
		makeComponentMappingRGBA(),					// VkComponentMapping		components;
		subresourceRange,							// VkImageSubresourceRange	subresourceRange;
	};
	return createImageView(vk, vkDevice, &imageViewParams);
}

VkBufferImageCopy makeBufferImageCopy (const VkExtent3D					extent,
									   const VkImageSubresourceLayers	subresourceLayers)
{
	const VkBufferImageCopy copyParams =
	{
		0ull,										//	VkDeviceSize				bufferOffset;
		0u,											//	deUint32					bufferRowLength;
		0u,											//	deUint32					bufferImageHeight;
		subresourceLayers,							//	VkImageSubresourceLayers	imageSubresource;
		makeOffset3D(0, 0, 0),						//	VkOffset3D					imageOffset;
		extent,										//	VkExtent3D					imageExtent;
	};
	return copyParams;
}

Move<VkPipelineLayout> makePipelineLayout (const DeviceInterface&		vk,
										   const VkDevice				device,
										   const VkDescriptorSetLayout	descriptorSetLayout)
{
	const VkPipelineLayoutCreateInfo info =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,							// VkStructureType				sType;
		DE_NULL,																// const void*					pNext;
		(VkPipelineLayoutCreateFlags)0,											// VkPipelineLayoutCreateFlags	flags;
		(descriptorSetLayout != DE_NULL ? 1u : 0u),								// deUint32						setLayoutCount;
		(descriptorSetLayout != DE_NULL ? &descriptorSetLayout : DE_NULL),		// const VkDescriptorSetLayout*	pSetLayouts;
		0u,																		// deUint32						pushConstantRangeCount;
		DE_NULL,																// const VkPushConstantRange*	pPushConstantRanges;
	};
	return createPipelineLayout(vk, device, &info);
}

Move<VkFramebuffer> makeFramebuffer (const DeviceInterface&	vk,
									 const VkDevice			device,
									 const VkRenderPass		renderPass,
									 const VkImageView		colorAttachment,
									 const deUint32			width,
									 const deUint32			height,
									 const deUint32			layers)
{
	const VkFramebufferCreateInfo framebufferInfo = {
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,		// VkStructureType                             sType;
		DE_NULL,										// const void*                                 pNext;
		(VkFramebufferCreateFlags)0,					// VkFramebufferCreateFlags                    flags;
		renderPass,										// VkRenderPass                                renderPass;
		1u,												// uint32_t                                    attachmentCount;
		&colorAttachment,								// const VkImageView*                          pAttachments;
		width,											// uint32_t                                    width;
		height,											// uint32_t                                    height;
		layers,											// uint32_t                                    layers;
	};

	return createFramebuffer(vk, device, &framebufferInfo);
}

VkImageMemoryBarrier makeImageMemoryBarrier	(const VkAccessFlags			srcAccessMask,
											 const VkAccessFlags			dstAccessMask,
											 const VkImageLayout			oldLayout,
											 const VkImageLayout			newLayout,
											 const VkImage					image,
											 const VkImageSubresourceRange	subresourceRange)
{
	const VkImageMemoryBarrier barrier =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		srcAccessMask,									// VkAccessFlags			outputMask;
		dstAccessMask,									// VkAccessFlags			inputMask;
		oldLayout,										// VkImageLayout			oldLayout;
		newLayout,										// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
		image,											// VkImage					image;
		subresourceRange,								// VkImageSubresourceRange	subresourceRange;
	};
	return barrier;
}

VkBufferMemoryBarrier makeBufferMemoryBarrier (const VkAccessFlags	srcAccessMask,
											   const VkAccessFlags	dstAccessMask,
											   const VkBuffer		buffer,
											   const VkDeviceSize	offset,
											   const VkDeviceSize	bufferSizeBytes)
{
	const VkBufferMemoryBarrier barrier =
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
		DE_NULL,									// const void*		pNext;
		srcAccessMask,								// VkAccessFlags	srcAccessMask;
		dstAccessMask,								// VkAccessFlags	dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			destQueueFamilyIndex;
		buffer,										// VkBuffer			buffer;
		offset,										// VkDeviceSize		offset;
		bufferSizeBytes,							// VkDeviceSize		size;
	};
	return barrier;
}

void beginRenderPass (const DeviceInterface&	vk,
					  const VkCommandBuffer		commandBuffer,
					  const VkRenderPass		renderPass,
					  const VkFramebuffer		framebuffer,
					  const VkRect2D&			renderArea,
					  const tcu::Vec4&			clearColor)
{
	const VkClearValue clearValue = makeClearValueColor(clearColor);

	const VkRenderPassBeginInfo renderPassBeginInfo = {
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,		// VkStructureType         sType;
		DE_NULL,										// const void*             pNext;
		renderPass,										// VkRenderPass            renderPass;
		framebuffer,									// VkFramebuffer           framebuffer;
		renderArea,										// VkRect2D                renderArea;
		1u,												// uint32_t                clearValueCount;
		&clearValue,									// const VkClearValue*     pClearValues;
	};

	vk.cmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void endRenderPass (const DeviceInterface&	vk,
					const VkCommandBuffer	commandBuffer)
{
	vk.cmdEndRenderPass(commandBuffer);
}

void beginCommandBuffer (const DeviceInterface& vk, const VkCommandBuffer commandBuffer)
{
	const VkCommandBufferBeginInfo info =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType                          sType;
		DE_NULL,										// const void*                              pNext;
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,	// VkCommandBufferUsageFlags                flags;
		DE_NULL,										// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
	};
	VK_CHECK(vk.beginCommandBuffer(commandBuffer, &info));
}

void endCommandBuffer (const DeviceInterface& vk, const VkCommandBuffer commandBuffer)
{
	VK_CHECK(vk.endCommandBuffer(commandBuffer));
}

void submitCommandsAndWait (const DeviceInterface&	vk,
							const VkDevice			device,
							const VkQueue			queue,
							const VkCommandBuffer	commandBuffer)
{
	const Unique<VkFence> fence(createFence(vk, device));

	const VkSubmitInfo submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType                sType;
		DE_NULL,							// const void*                    pNext;
		0u,									// uint32_t                       waitSemaphoreCount;
		DE_NULL,							// const VkSemaphore*             pWaitSemaphores;
		DE_NULL,							// const VkPipelineStageFlags*    pWaitDstStageMask;
		1u,									// uint32_t                       commandBufferCount;
		&commandBuffer,						// const VkCommandBuffer*         pCommandBuffers;
		0u,									// uint32_t                       signalSemaphoreCount;
		DE_NULL,							// const VkSemaphore*             pSignalSemaphores;
	};
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
	VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, ~0ull));
}

bool compareWithFileImage (Context& context, const tcu::ConstPixelBufferAccess& resultImage, std::string testName)
{
	tcu::TextureLevel referenceImage;
	std::string fileName="vulkan/data/geometry/"+testName+".png";
	tcu::ImageIO::loadPNG(referenceImage, context.getTestContext().getArchive(), fileName.c_str());

	if (tcu::fuzzyCompare(context.getTestContext().getLog(), "ImageComparison", "Image Comparison",
								referenceImage.getAccess(), resultImage, 0.001f, tcu::COMPARE_LOG_RESULT))
		return tcu::intThresholdPositionDeviationCompare(context.getTestContext().getLog(), "ImageComparison", "Image Comparison",
														referenceImage.getAccess(), resultImage, tcu::UVec4(1u, 1u, 1u, 1u), tcu::IVec3(2,2,2), false, tcu::COMPARE_LOG_RESULT);
	else
		return false;
}

de::MovePtr<Allocation> bindImage (const DeviceInterface& vk, const VkDevice device, Allocator& allocator, const VkImage image, const MemoryRequirement requirement)
{
	de::MovePtr<Allocation> alloc = allocator.allocate(getImageMemoryRequirements(vk, device, image), requirement);
	VK_CHECK(vk.bindImageMemory(device, image, alloc->getMemory(), alloc->getOffset()));
	return alloc;
}

de::MovePtr<Allocation> bindBuffer (const DeviceInterface& vk, const VkDevice device, Allocator& allocator, const VkBuffer buffer, const MemoryRequirement requirement)
{
	de::MovePtr<Allocation> alloc(allocator.allocate(getBufferMemoryRequirements(vk, device, buffer), requirement));
	VK_CHECK(vk.bindBufferMemory(device, buffer, alloc->getMemory(), alloc->getOffset()));
	return alloc;
}

void zeroBuffer (const DeviceInterface& vk, const VkDevice device, const Allocation& alloc, const VkDeviceSize size)
{
	deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(size));
	flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), size);
}

VkBool32 checkPointSize (const InstanceInterface& vki, const VkPhysicalDevice physDevice)
{
	const VkPhysicalDeviceFeatures	features	= getPhysicalDeviceFeatures  (vki, physDevice);
	return features.shaderTessellationAndGeometryPointSize;
}

void checkGeometryShaderSupport (const InstanceInterface& vki, const VkPhysicalDevice physDevice, const int numGeometryShaderInvocations)
{
	const VkPhysicalDeviceFeatures	features	= getPhysicalDeviceFeatures  (vki, physDevice);
	const VkPhysicalDeviceLimits	limits		= getPhysicalDeviceProperties(vki, physDevice).limits;

	if (!features.geometryShader)
		TCU_THROW(NotSupportedError, "Missing feature: geometryShader");

	if (numGeometryShaderInvocations != 0 && limits.maxGeometryShaderInvocations < static_cast<deUint32>(numGeometryShaderInvocations))
		TCU_THROW(NotSupportedError, ("Unsupported limit: maxGeometryShaderInvocations < " + de::toString(numGeometryShaderInvocations)).c_str());
}

} //geometry
} //vkt
