/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file vktPipelineRenderToImageTests.cpp
 * \brief Render to image tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineRenderToImageTests.hpp"
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
#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"

#include <string>
#include <vector>
#include <set>

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
using tcu::IVec3;
using tcu::Vec4;
using tcu::UVec4;
using tcu::IVec2;
using tcu::IVec4;
using tcu::BVec4;
using std::vector;

typedef SharedPtr<Unique<VkImageView> >	SharedPtrVkImageView;
typedef SharedPtr<Unique<VkPipeline> >	SharedPtrVkPipeline;

enum Constants
{
	NUM_CUBE_FACES					= 6,
	REFERENCE_COLOR_VALUE			= 125,
	REFERENCE_STENCIL_VALUE			= 42,
	MAX_SIZE						= -1,	//!< Should be queried at runtime and replaced with max possible value
	MAX_VERIFICATION_REGION_SIZE	= 32,	//!<  Limit the checked area to a small size, especially for huge images
	MAX_VERIFICATION_REGION_DEPTH	= 8,

	MASK_W					= (1 | 0 | 0 | 0),
	MASK_W_LAYERS			= (1 | 0 | 0 | 8),
	MASK_WH					= (1 | 2 | 0 | 0),
	MASK_WH_LAYERS			= (1 | 2 | 0 | 8),
	MASK_WHD				= (1 | 2 | 4 | 0),
};

enum AllocationKind
{
	ALLOCATION_KIND_SUBALLOCATED = 0,
	ALLOCATION_KIND_DEDICATED,
};

static const float	REFERENCE_DEPTH_VALUE	= 1.0f;
static const Vec4	COLOR_TABLE[]			=
{
	Vec4(0.9f, 0.0f, 0.0f, 1.0f),
	Vec4(0.6f, 1.0f, 0.0f, 1.0f),
	Vec4(0.3f, 0.0f, 1.0f, 1.0f),
	Vec4(0.1f, 1.0f, 1.0f, 1.0f),
	Vec4(0.8f, 1.0f, 0.0f, 1.0f),
	Vec4(0.5f, 0.0f, 1.0f, 1.0f),
	Vec4(0.2f, 0.0f, 0.0f, 1.0f),
	Vec4(1.0f, 1.0f, 0.0f, 1.0f),
};

struct CaseDef
{
	VkImageViewType	viewType;
	IVec4			imageSizeHint;			//!< (w, h, d, layers), a component may have a symbolic value MAX_SIZE
	VkFormat		colorFormat;
	VkFormat		depthStencilFormat;		//! A depth/stencil format, or UNDEFINED if not used
	AllocationKind	allocationKind;
};

template<typename T>
inline SharedPtr<Unique<T> > makeSharedPtr (Move<T> move)
{
	return SharedPtr<Unique<T> >(new Unique<T>(move));
}

template<typename T>
inline VkDeviceSize sizeInBytes (const vector<T>& vec)
{
	return vec.size() * sizeof(vec[0]);
}

inline bool isCube (const VkImageViewType viewType)
{
	return (viewType == VK_IMAGE_VIEW_TYPE_CUBE || viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY);
}

inline VkDeviceSize product (const IVec4& v)
{
	return ((static_cast<VkDeviceSize>(v.x()) * v.y()) * v.z()) * v.w();
}

template<typename T>
inline T sum (const vector<T>& v)
{
	T total = static_cast<T>(0);
	for (typename vector<T>::const_iterator it = v.begin(); it != v.end(); ++it)
		total += *it;
	return total;
}

template <typename T, int Size>
int findIndexOfMaxComponent (const tcu::Vector<T, Size>& vec)
{
	int index	= 0;
	T	value	= vec[0];

	for (int i = 1; i < Size; ++i)
	{
		if (vec[i] > value)
		{
			index	= i;
			value	= vec[i];
		}
	}

	return index;
}

inline int maxLayersOrDepth (const IVec4& size)
{
	// This is safe because 3D images must have layers (w) = 1
	return deMax32(size.z(), size.w());
}

de::MovePtr<Allocation> bindBuffer (const InstanceInterface&	vki,
									const DeviceInterface&		vkd,
									const VkPhysicalDevice&		physDevice,
									const VkDevice				device,
									const VkBuffer&				buffer,
									const MemoryRequirement		requirement,
									Allocator&					allocator,
									AllocationKind				allocationKind)
{
	switch (allocationKind)
	{
		case ALLOCATION_KIND_SUBALLOCATED:
		{
			return ::vkt::pipeline::bindBuffer(vkd, device, allocator, buffer, requirement);
		}

		case ALLOCATION_KIND_DEDICATED:
		{
			return bindBufferDedicated(vki, vkd, physDevice, device, buffer, requirement);
		}

		default:
		{
			TCU_THROW(InternalError, "Invalid allocation kind");
		}
	}
}

de::MovePtr<Allocation> bindImage (const InstanceInterface&		vki,
								   const DeviceInterface&		vkd,
								   const VkPhysicalDevice&		physDevice,
								   const VkDevice				device,
								   const VkImage&				image,
								   const MemoryRequirement		requirement,
								   Allocator&					allocator,
								   AllocationKind				allocationKind)
{
	switch (allocationKind)
	{
		case ALLOCATION_KIND_SUBALLOCATED:
		{
			return ::vkt::pipeline::bindImage(vkd, device, allocator, image, requirement);
		}

		case ALLOCATION_KIND_DEDICATED:
		{
			return bindImageDedicated(vki, vkd, physDevice, device, image, requirement);
		}

		default:
		{
			TCU_THROW(InternalError, "Invalid allocation kind");
		}
	}
}

// This is very test specific, so be careful if you want to reuse this code.
Move<VkPipeline> makeGraphicsPipeline (const DeviceInterface&		vk,
									   const VkDevice				device,
									   const VkPipeline				basePipeline,		// for derivatives
									   const VkPipelineLayout		pipelineLayout,
									   const VkRenderPass			renderPass,
									   const VkShaderModule			vertexModule,
									   const VkShaderModule			fragmentModule,
									   const IVec2&					renderSize,
									   const VkPrimitiveTopology	topology,
									   const deUint32				subpass,
									   const bool					useDepth,
									   const bool					useStencil)
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
			0u,								// uint32_t			location;
			0u,								// uint32_t			binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,	// VkFormat			format;
			0u,								// uint32_t			offset;
		},
		{
			1u,								// uint32_t			location;
			0u,								// uint32_t			binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,	// VkFormat			format;
			sizeof(Vec4),					// uint32_t			offset;
		}
	};

	const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineVertexInputStateCreateFlags)0,					// VkPipelineVertexInputStateCreateFlags	flags;
		1u,															// uint32_t									vertexBindingDescriptionCount;
		&vertexInputBindingDescription,								// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
		DE_LENGTH_OF_ARRAY(vertexInputAttributeDescriptions),		// uint32_t									vertexAttributeDescriptionCount;
		vertexInputAttributeDescriptions,							// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
	};

	const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0,						// VkPipelineInputAssemblyStateCreateFlags	flags;
		topology,														// VkPrimitiveTopology						topology;
		VK_FALSE,														// VkBool32									primitiveRestartEnable;
	};

	const VkViewport viewport = makeViewport(
		0.0f, 0.0f,
		static_cast<float>(renderSize.x()), static_cast<float>(renderSize.y()),
		0.0f, 1.0f);

	const VkRect2D scissor =
	{
		makeOffset2D(0, 0),
		makeExtent2D(renderSize.x(), renderSize.y()),
	};

	const VkPipelineViewportStateCreateInfo pipelineViewportStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,	// VkStructureType						sType;
		DE_NULL,												// const void*							pNext;
		(VkPipelineViewportStateCreateFlags)0,					// VkPipelineViewportStateCreateFlags	flags;
		1u,														// uint32_t								viewportCount;
		&viewport,												// const VkViewport*					pViewports;
		1u,														// uint32_t								scissorCount;
		&scissor,												// const VkRect2D*						pScissors;
	};

	const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineRasterizationStateCreateFlags)0,					// VkPipelineRasterizationStateCreateFlags	flags;
		VK_FALSE,													// VkBool32									depthClampEnable;
		VK_FALSE,													// VkBool32									rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,										// VkPolygonMode							polygonMode;
		VK_CULL_MODE_NONE,											// VkCullModeFlags							cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,							// VkFrontFace								frontFace;
		VK_FALSE,													// VkBool32									depthBiasEnable;
		0.0f,														// float									depthBiasConstantFactor;
		0.0f,														// float									depthBiasClamp;
		0.0f,														// float									depthBiasSlopeFactor;
		1.0f,														// float									lineWidth;
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
		VK_STENCIL_OP_KEEP,									// stencil fail
		VK_STENCIL_OP_KEEP,									// depth & stencil pass
		VK_STENCIL_OP_KEEP,									// depth only fail
		VK_COMPARE_OP_EQUAL,								// compare op
		~0u,												// compare mask
		~0u,												// write mask
		static_cast<deUint32>(REFERENCE_STENCIL_VALUE));	// reference

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineDepthStencilStateCreateFlags)0,					// VkPipelineDepthStencilStateCreateFlags	flags;
		useDepth,													// VkBool32									depthTestEnable;
		VK_FALSE,													// VkBool32									depthWriteEnable;
		VK_COMPARE_OP_LESS,											// VkCompareOp								depthCompareOp;
		VK_FALSE,													// VkBool32									depthBoundsTestEnable;
		useStencil,													// VkBool32									stencilTestEnable;
		stencilOpState,												// VkStencilOpState							front;
		stencilOpState,												// VkStencilOpState							back;
		0.0f,														// float									minDepthBounds;
		1.0f,														// float									maxDepthBounds;
	};

	const VkColorComponentFlags colorComponentsAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// Number of blend attachments must equal the number of color attachments during any subpass.
	const VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState =
	{
		VK_FALSE,				// VkBool32					blendEnable;
		VK_BLEND_FACTOR_ONE,	// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,	// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,		// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_ONE,	// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,	// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,		// VkBlendOp				alphaBlendOp;
		colorComponentsAll,		// VkColorComponentFlags	colorWriteMask;
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

	const VkPipelineShaderStageCreateInfo pShaderStages[] =
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_VERTEX_BIT,								// VkShaderStageFlagBits				stage;
			vertexModule,											// VkShaderModule						module;
			"main",													// const char*							pName;
			DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,							// VkShaderStageFlagBits				stage;
			fragmentModule,											// VkShaderModule						module;
			"main",													// const char*							pName;
			DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
		}
	};

	const VkPipelineCreateFlags			flags = (basePipeline == DE_NULL ? VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT
																		 : VK_PIPELINE_CREATE_DERIVATIVE_BIT);

	const VkGraphicsPipelineCreateInfo	graphicsPipelineInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
		DE_NULL,											// const void*										pNext;
		flags,												// VkPipelineCreateFlags							flags;
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
		subpass,											// deUint32											subpass;
		basePipeline,										// VkPipeline										basePipelineHandle;
		-1,													// deInt32											basePipelineIndex;
	};

	return createGraphicsPipeline(vk, device, DE_NULL, &graphicsPipelineInfo);
}

//! Make a render pass with one subpass per color attachment and depth/stencil attachment (if used).
Move<VkRenderPass> makeRenderPass (const DeviceInterface&		vk,
								   const VkDevice				device,
								   const VkFormat				colorFormat,
								   const VkFormat				depthStencilFormat,
								   const deUint32				numLayers,
								   const VkImageLayout			initialColorImageLayout			= VK_IMAGE_LAYOUT_UNDEFINED,
								   const VkImageLayout			initialDepthStencilImageLayout	= VK_IMAGE_LAYOUT_UNDEFINED)
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
		initialColorImageLayout,							// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					finalLayout;
	};
	vector<VkAttachmentDescription> attachmentDescriptions(numLayers, colorAttachmentDescription);

	const VkAttachmentDescription depthStencilAttachmentDescription =
	{
		(VkAttachmentDescriptionFlags)0,					// VkAttachmentDescriptionFlags		flags;
		depthStencilFormat,									// VkFormat							format;
		VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				storeOp;
		VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
		initialDepthStencilImageLayout,						// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	// VkImageLayout					finalLayout;
	};

	if (depthStencilFormat != VK_FORMAT_UNDEFINED)
		attachmentDescriptions.insert(attachmentDescriptions.end(), numLayers, depthStencilAttachmentDescription);

	// Create a subpass for each attachment (each attachement is a layer of an arrayed image).
	vector<VkAttachmentReference>	colorAttachmentReferences		(numLayers);
	vector<VkAttachmentReference>	depthStencilAttachmentReferences(numLayers);
	vector<VkSubpassDescription>	subpasses;

	// Ordering here must match the framebuffer attachments
	for (deUint32 i = 0; i < numLayers; ++i)
	{
		const VkAttachmentReference attachmentRef =
		{
			i,													// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
		};
		const VkAttachmentReference depthStencilAttachmentRef =
		{
			i + numLayers,										// deUint32			attachment;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
		};

		colorAttachmentReferences[i]		= attachmentRef;
		depthStencilAttachmentReferences[i]	= depthStencilAttachmentRef;

		const VkAttachmentReference*	pDepthStencilAttachment	= (depthStencilFormat != VK_FORMAT_UNDEFINED ? &depthStencilAttachmentReferences[i] : DE_NULL);
		const VkSubpassDescription		subpassDescription		=
		{
			(VkSubpassDescriptionFlags)0,					// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,				// VkPipelineBindPoint				pipelineBindPoint;
			0u,												// deUint32							inputAttachmentCount;
			DE_NULL,										// const VkAttachmentReference*		pInputAttachments;
			1u,												// deUint32							colorAttachmentCount;
			&colorAttachmentReferences[i],					// const VkAttachmentReference*		pColorAttachments;
			DE_NULL,										// const VkAttachmentReference*		pResolveAttachments;
			pDepthStencilAttachment,						// const VkAttachmentReference*		pDepthStencilAttachment;
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

Move<VkImage> makeImage (const DeviceInterface&		vk,
						 const VkDevice				device,
						 VkImageCreateFlags			flags,
						 VkImageType				imageType,
						 const VkFormat				format,
						 const IVec3&				size,
						 const deUint32				numMipLevels,
						 const deUint32				numLayers,
						 const VkImageUsageFlags	usage)
{
	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,	// VkStructureType			sType;
		DE_NULL,								// const void*				pNext;
		flags,									// VkImageCreateFlags		flags;
		imageType,								// VkImageType				imageType;
		format,									// VkFormat					format;
		makeExtent3D(size),						// VkExtent3D				extent;
		numMipLevels,							// deUint32					mipLevels;
		numLayers,								// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,					// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,				// VkImageTiling			tiling;
		usage,									// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode			sharingMode;
		0u,										// deUint32					queueFamilyIndexCount;
		DE_NULL,								// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,				// VkImageLayout			initialLayout;
	};
	return createImage(vk, device, &imageParams);
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

//! Get a reference clear value based on color format.
VkClearValue getClearValue (const VkFormat format)
{
	if (isUintFormat(format) || isIntFormat(format))
		return makeClearValueColorU32(REFERENCE_COLOR_VALUE, REFERENCE_COLOR_VALUE, REFERENCE_COLOR_VALUE, REFERENCE_COLOR_VALUE);
	else
		return makeClearValueColorF32(1.0f, 1.0f, 1.0f, 1.0f);
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

//! A half-viewport quad. Use with TRIANGLE_STRIP topology.
vector<Vertex4RGBA> genFullQuadVertices (const int subpassCount)
{
	vector<Vertex4RGBA>	vectorData;
	for (int subpassNdx = 0; subpassNdx < subpassCount; ++subpassNdx)
	{
		Vertex4RGBA data =
		{
			Vec4(0.0f, -1.0f, 0.0f, 1.0f),
			COLOR_TABLE[subpassNdx % DE_LENGTH_OF_ARRAY(COLOR_TABLE)],
		};
		vectorData.push_back(data);
		data.position	= Vec4(0.0f,  1.0f, 0.0f, 1.0f);
		vectorData.push_back(data);
		data.position	= Vec4(1.0f, -1.0f, 0.0f, 1.0f);
		vectorData.push_back(data);
		data.position	= Vec4(1.0f,  1.0f, 0.0f, 1.0f);
		vectorData.push_back(data);
	}
	return vectorData;
}

VkImageType getImageType (const VkImageViewType viewType)
{
	switch (viewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			return VK_IMAGE_TYPE_1D;

		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			return VK_IMAGE_TYPE_2D;

		case VK_IMAGE_VIEW_TYPE_3D:
			return VK_IMAGE_TYPE_3D;

		default:
			DE_ASSERT(0);
			return VK_IMAGE_TYPE_LAST;
	}
}

//! ImageViewType for accessing a single layer/slice of an image
VkImageViewType getImageViewSliceType (const VkImageViewType viewType)
{
	switch (viewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			return VK_IMAGE_VIEW_TYPE_1D;

		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
		case VK_IMAGE_VIEW_TYPE_3D:
			return VK_IMAGE_VIEW_TYPE_2D;

		default:
			DE_ASSERT(0);
			return VK_IMAGE_VIEW_TYPE_LAST;
	}
}

VkImageCreateFlags getImageCreateFlags (const VkImageViewType viewType)
{
	VkImageCreateFlags	flags	= (VkImageCreateFlags)0;

	if (viewType == VK_IMAGE_VIEW_TYPE_3D)	flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
	if (isCube(viewType))					flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	return flags;
}

void generateExpectedImage (const tcu::PixelBufferAccess& outputImage, const IVec2& renderSize, const int colorDepthOffset)
{
	const tcu::TextureChannelClass	channelClass	= tcu::getTextureChannelClass(outputImage.getFormat().type);
	const bool						isInt			= (channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER || channelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER);
	const VkClearValue				clearValue		= getClearValue(mapTextureFormat(outputImage.getFormat()));

	if (isInt)
		tcu::clear(outputImage, IVec4(clearValue.color.int32));
	else
		tcu::clear(outputImage, Vec4(clearValue.color.float32));

	for (int z = 0; z < outputImage.getDepth(); ++z)
	{
		const Vec4& setColor	= COLOR_TABLE[(z + colorDepthOffset) % DE_LENGTH_OF_ARRAY(COLOR_TABLE)];
		const IVec4 setColorInt	= (static_cast<float>(REFERENCE_COLOR_VALUE) * setColor).cast<deInt32>();

		for (int y = 0;					y < renderSize.y(); ++y)
		for (int x = renderSize.x()/2;	x < renderSize.x(); ++x)
		{
			if (isInt)
				outputImage.setPixel(setColorInt, x, y, z);
			else
				outputImage.setPixel(setColor, x, y, z);
		}
	}
}

deUint32 selectMatchingMemoryType (const VkPhysicalDeviceMemoryProperties& deviceMemProps, deUint32 allowedMemTypeBits, MemoryRequirement requirement)
{
	const deUint32	compatibleTypes	= getCompatibleMemoryTypes(deviceMemProps, requirement);
	const deUint32	candidates		= allowedMemTypeBits & compatibleTypes;

	if (candidates == 0)
		TCU_THROW(NotSupportedError, "No compatible memory type found");

	return (deUint32)deCtz32(candidates);
}

IVec4 getMaxImageSize (const VkImageViewType viewType, const IVec4& sizeHint)
{
	//Limits have been taken from the vulkan specification
	IVec4 size = IVec4(
		sizeHint.x() != MAX_SIZE ? sizeHint.x() : 4096,
		sizeHint.y() != MAX_SIZE ? sizeHint.y() : 4096,
		sizeHint.z() != MAX_SIZE ? sizeHint.z() : 256,
		sizeHint.w() != MAX_SIZE ? sizeHint.w() : 256);

	switch (viewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			size.x() = deMin32(4096, size.x());
			break;

		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			size.x() = deMin32(4096, size.x());
			size.y() = deMin32(4096, size.y());
			break;

		case VK_IMAGE_VIEW_TYPE_3D:
			size.x() = deMin32(256, size.x());
			size.y() = deMin32(256, size.y());
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			size.x() = deMin32(4096, size.x());
			size.y() = deMin32(4096, size.y());
			size.w() = deMin32(252, size.w());
			size.w() = NUM_CUBE_FACES * (size.w() / NUM_CUBE_FACES);	// round down to 6 faces
			break;

		default:
			DE_ASSERT(0);
			return IVec4();
	}

	return size;
}

deUint32 getMemoryTypeNdx (Context& context, const CaseDef& caseDef)
{
	const DeviceInterface&					vk					= context.getDeviceInterface();
	const InstanceInterface&				vki					= context.getInstanceInterface();
	const VkDevice							device				= context.getDevice();
	const VkPhysicalDevice					physDevice			= context.getPhysicalDevice();

	const VkPhysicalDeviceMemoryProperties	memoryProperties	= getPhysicalDeviceMemoryProperties(vki, physDevice);
	Move<VkImage>							colorImage;
	VkMemoryRequirements					memReqs;

	const VkImageUsageFlags					imageUsage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	const IVec4								imageSize	= getMaxImageSize(caseDef.viewType, caseDef.imageSizeHint);

	//create image, don't bind any memory to it
	colorImage	= makeImage(vk, device, getImageCreateFlags(caseDef.viewType), getImageType(caseDef.viewType), caseDef.colorFormat,
								imageSize.swizzle(0, 1, 2), 1u, imageSize.w(), imageUsage);

	vk.getImageMemoryRequirements(device, *colorImage, &memReqs);
	return selectMatchingMemoryType(memoryProperties, memReqs.memoryTypeBits, MemoryRequirement::Any);
}

VkDeviceSize getMaxDeviceHeapSize (Context& context, const CaseDef& caseDef)
{
	const InstanceInterface&				vki					= context.getInstanceInterface();
	const VkPhysicalDevice					physDevice			= context.getPhysicalDevice();
	const VkPhysicalDeviceMemoryProperties	memoryProperties	= getPhysicalDeviceMemoryProperties(vki, physDevice);
	const deUint32							memoryTypeNdx		= getMemoryTypeNdx (context, caseDef);

	return memoryProperties.memoryHeaps[memoryProperties.memoryTypes[memoryTypeNdx].heapIndex].size;
}

//! Get a smaller image size. Returns a vector of zeroes, if it can't reduce more.
IVec4 getReducedImageSize (const CaseDef& caseDef, IVec4 size)
{
	const int maxIndex		= findIndexOfMaxComponent(size);
	const int reducedSize	= size[maxIndex] >> 1;

	switch (caseDef.viewType)
	{
		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			if (maxIndex < 2)
				size.x() = size.y() = reducedSize;
			else if (maxIndex == 3 && reducedSize >= NUM_CUBE_FACES)
				size.w() = NUM_CUBE_FACES * (reducedSize / NUM_CUBE_FACES); // round down to a multiple of 6
			else
				size = IVec4(0);
			break;

		default:
			size[maxIndex] = reducedSize;
			break;
	}

	if (reducedSize == 0)
		size = IVec4(0);

	return size;
}

bool isDepthStencilFormatSupported (const InstanceInterface& vki, const VkPhysicalDevice physDevice, const VkFormat format)
{
	const VkFormatProperties properties = getPhysicalDeviceFormatProperties(vki, physDevice, format);
	return (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
}

VkImageAspectFlags getFormatAspectFlags (const VkFormat format)
{
	if (format == VK_FORMAT_UNDEFINED)
		return 0;

	const tcu::TextureFormat::ChannelOrder	order	= mapVkFormat(format).order;

	switch (order)
	{
		case tcu::TextureFormat::DS:	return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		case tcu::TextureFormat::D:		return VK_IMAGE_ASPECT_DEPTH_BIT;
		case tcu::TextureFormat::S:		return VK_IMAGE_ASPECT_STENCIL_BIT;
		default:						return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

void initPrograms (SourceCollections& programCollection, const CaseDef caseDef)
{
	const int	numComponents	= getNumUsedChannels(mapVkFormat(caseDef.colorFormat).order);
	const bool	isUint			= isUintFormat(caseDef.colorFormat);
	const bool	isSint			= isIntFormat(caseDef.colorFormat);

	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in  vec4 in_position;\n"
			<< "layout(location = 1) in  vec4 in_color;\n"
			<< "layout(location = 0) out vec4 out_color;\n"
			<< "\n"
			<< "out gl_PerVertex {\n"
			<< "	vec4 gl_Position;\n"
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "	gl_Position	= in_position;\n"
			<< "	out_color	= in_color;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream colorValue;
		colorValue << REFERENCE_COLOR_VALUE;
		const std::string colorFormat	= getColorFormatStr(numComponents, isUint, isSint);
		const std::string colorInteger	= (isUint || isSint ? " * "+colorFormat+"("+colorValue.str()+")" :"");

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in  vec4 in_color;\n"
			<< "layout(location = 0) out " << colorFormat << " o_color;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    o_color = " << colorFormat << "("
			<< (numComponents == 1 ? "in_color.r"   :
				numComponents == 2 ? "in_color.rg"  :
				numComponents == 3 ? "in_color.rgb" : "in_color")
			<< colorInteger
			<< ");\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

//! See testAttachmentSize() description
tcu::TestStatus testWithSizeReduction (Context& context, const CaseDef& caseDef)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const InstanceInterface&		vki					= context.getInstanceInterface();
	const VkDevice					device				= context.getDevice();
	const VkPhysicalDevice			physDevice			= context.getPhysicalDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&						allocator			= context.getDefaultAllocator();

	// The memory might be too small to allocate a largest possible attachment, so try to account for that.
	const bool						useDepthStencil		= (caseDef.depthStencilFormat != VK_FORMAT_UNDEFINED);

	IVec4							imageSize			= getMaxImageSize(caseDef.viewType, caseDef.imageSizeHint);
	VkDeviceSize					colorSize			= product(imageSize) * tcu::getPixelSize(mapVkFormat(caseDef.colorFormat));
	VkDeviceSize					depthStencilSize	= (useDepthStencil ? product(imageSize) * tcu::getPixelSize(mapVkFormat(caseDef.depthStencilFormat)) : 0ull);

	const VkDeviceSize				reserveForChecking	= 500ull * 1024ull;	//left 512KB
	const float						additionalMemory	= 1.15f;			//left some free memory on device (15%)
	VkDeviceSize					neededMemory		= static_cast<VkDeviceSize>(static_cast<float>(colorSize + depthStencilSize) * additionalMemory) + reserveForChecking;
	VkDeviceSize					maxMemory			= getMaxDeviceHeapSize(context, caseDef) >> 2;

	const VkDeviceSize				deviceMemoryBudget	= std::min(neededMemory, maxMemory);
	bool							allocationPossible	= false;

	// Keep reducing the size, if image size is too big
	while (neededMemory > deviceMemoryBudget)
	{
		imageSize = getReducedImageSize(caseDef, imageSize);

		if (imageSize == IVec4())
			return tcu::TestStatus::fail("Couldn't create an image with required size");

		colorSize			= product(imageSize) * tcu::getPixelSize(mapVkFormat(caseDef.colorFormat));
		depthStencilSize	= (useDepthStencil ? product(imageSize) * tcu::getPixelSize(mapVkFormat(caseDef.depthStencilFormat)) : 0ull);
		neededMemory		= static_cast<VkDeviceSize>(static_cast<double>(colorSize + depthStencilSize) * additionalMemory);
	}

	// Keep reducing the size, if allocation return out of any memory
	while (!allocationPossible)
	{
		VkDeviceMemory				object			= 0;
		const VkMemoryAllocateInfo	allocateInfo	=
		{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	//VkStructureType	sType;
			DE_NULL,								//const void*		pNext;
			neededMemory,							//VkDeviceSize		allocationSize;
			getMemoryTypeNdx(context, caseDef)		//deUint32			memoryTypeIndex;
		};

		const VkResult				result			= vk.allocateMemory(device, &allocateInfo, DE_NULL, &object);

		if (VK_ERROR_OUT_OF_DEVICE_MEMORY == result || VK_ERROR_OUT_OF_HOST_MEMORY == result)
		{
			imageSize = getReducedImageSize(caseDef, imageSize);

			if (imageSize == IVec4())
				return tcu::TestStatus::fail("Couldn't create an image with required size");

			colorSize			= product(imageSize) * tcu::getPixelSize(mapVkFormat(caseDef.colorFormat));
			depthStencilSize	= (useDepthStencil ? product(imageSize) * tcu::getPixelSize(mapVkFormat(caseDef.depthStencilFormat)) : 0ull);
			neededMemory		= static_cast<VkDeviceSize>(static_cast<double>(colorSize + depthStencilSize) * additionalMemory) + reserveForChecking;
		}
		else if (VK_SUCCESS != result)
		{
			return tcu::TestStatus::fail("Couldn't allocate memory");
		}
		else
		{
			//free memory using Move pointer
			Move<VkDeviceMemory> memoryAllocated (check<VkDeviceMemory>(object), Deleter<VkDeviceMemory>(vk, device, DE_NULL));
			allocationPossible = true;
		}
	}

	context.getTestContext().getLog()
		<< tcu::TestLog::Message << "Using an image with size (width, height, depth, layers) = " << imageSize << tcu::TestLog::EndMessage;

	// "Slices" is either the depth of a 3D image, or the number of layers of an arrayed image
	const deInt32					numSlices			= maxLayersOrDepth(imageSize);


	if (useDepthStencil && !isDepthStencilFormatSupported(vki, physDevice, caseDef.depthStencilFormat))
		TCU_THROW(NotSupportedError, "Unsupported depth/stencil format");

	// Determine the verification bounds. The checked region will be in the center of the rendered image
	const IVec4	checkSize	= tcu::min(imageSize, IVec4(MAX_VERIFICATION_REGION_SIZE,
														MAX_VERIFICATION_REGION_SIZE,
														MAX_VERIFICATION_REGION_DEPTH,
														MAX_VERIFICATION_REGION_DEPTH));
	const IVec4	checkOffset	= (imageSize - checkSize) / 2;

	// Only make enough space for the check region
	const VkDeviceSize				colorBufferSize		= product(checkSize) * tcu::getPixelSize(mapVkFormat(caseDef.colorFormat));
	const Unique<VkBuffer>			colorBuffer			(makeBuffer(vk, device, colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	const UniquePtr<Allocation>		colorBufferAlloc	(bindBuffer(vki, vk, physDevice, device, *colorBuffer, MemoryRequirement::HostVisible, allocator, caseDef.allocationKind));

	{
		deMemset(colorBufferAlloc->getHostPtr(), 0, static_cast<std::size_t>(colorBufferSize));
		flushMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), VK_WHOLE_SIZE);
	}

	const Unique<VkShaderModule>	vertexModule	(createShaderModule			(vk, device, context.getBinaryCollection().get("vert"), 0u));
	const Unique<VkShaderModule>	fragmentModule	(createShaderModule			(vk, device, context.getBinaryCollection().get("frag"), 0u));
	const Unique<VkRenderPass>		renderPass		(makeRenderPass				(vk, device, caseDef.colorFormat, caseDef.depthStencilFormat, static_cast<deUint32>(numSlices),
																				 (caseDef.viewType == VK_IMAGE_VIEW_TYPE_3D) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
																															 : VK_IMAGE_LAYOUT_UNDEFINED));
	const Unique<VkPipelineLayout>	pipelineLayout	(makePipelineLayout			(vk, device));
	vector<SharedPtrVkPipeline>		pipelines;

	Move<VkImage>					colorImage;
	MovePtr<Allocation>				colorImageAlloc;
	vector<SharedPtrVkImageView>	colorAttachments;
	Move<VkImage>					depthStencilImage;
	MovePtr<Allocation>				depthStencilImageAlloc;
	vector<SharedPtrVkImageView>	depthStencilAttachments;
	vector<VkImageView>				attachmentHandles;			// all attachments (color and d/s)
	Move<VkBuffer>					vertexBuffer;
	MovePtr<Allocation>				vertexBufferAlloc;
	Move<VkFramebuffer>				framebuffer;

	// Create a color image
	{
		const VkImageUsageFlags	imageUsage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		colorImage		= makeImage(vk, device, getImageCreateFlags(caseDef.viewType), getImageType(caseDef.viewType), caseDef.colorFormat,
									imageSize.swizzle(0, 1, 2), 1u, imageSize.w(), imageUsage);
		colorImageAlloc	= bindImage(vki, vk, physDevice, device, *colorImage, MemoryRequirement::Any, allocator, caseDef.allocationKind);
	}

	// Create a depth/stencil image (always a 2D image, optionally layered)
	if (useDepthStencil)
	{
		const VkImageUsageFlags	imageUsage	= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		depthStencilImage		= makeImage(vk, device, (VkImageCreateFlags)0, VK_IMAGE_TYPE_2D, caseDef.depthStencilFormat,
											IVec3(imageSize.x(), imageSize.y(), 1), 1u, numSlices, imageUsage);
		depthStencilImageAlloc	= bindImage(vki, vk, physDevice, device, *depthStencilImage, MemoryRequirement::Any, allocator, caseDef.allocationKind);
	}

	// Create a vertex buffer
	{
		const vector<Vertex4RGBA>	vertices			= genFullQuadVertices(numSlices);
		const VkDeviceSize			vertexBufferSize	= sizeInBytes(vertices);

		vertexBuffer		= makeBuffer(vk, device, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		vertexBufferAlloc	= bindBuffer(vki, vk, physDevice, device, *vertexBuffer, MemoryRequirement::HostVisible, allocator, caseDef.allocationKind);

		deMemcpy(vertexBufferAlloc->getHostPtr(), &vertices[0], static_cast<std::size_t>(vertexBufferSize));
		flushMappedMemoryRange(vk, device, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset(), vertexBufferSize);
	}

	// Prepare color image upfront for rendering to individual slices.  3D slices aren't separate subresources, so they shouldn't be transitioned
	// during each subpass like array layers.
	if (caseDef.viewType == VK_IMAGE_VIEW_TYPE_3D)
	{
		const Unique<VkCommandPool>		cmdPool		(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Unique<VkCommandBuffer>	cmdBuffer	(makeCommandBuffer(vk, device, *cmdPool));

		beginCommandBuffer(vk, *cmdBuffer);

		const VkImageMemoryBarrier	imageBarrier	=
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType            sType;
			DE_NULL,											// const void*                pNext;
			(VkAccessFlags)0,									// VkAccessFlags              srcAccessMask;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// VkAccessFlags              dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout              oldLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout              newLayout;
			VK_QUEUE_FAMILY_IGNORED,							// uint32_t                   srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,							// uint32_t                   dstQueueFamilyIndex;
			*colorImage,										// VkImage                    image;
			{													// VkImageSubresourceRange    subresourceRange;
				VK_IMAGE_ASPECT_COLOR_BIT,							// VkImageAspectFlags    aspectMask;
				0u,													// uint32_t              baseMipLevel;
				1u,													// uint32_t              levelCount;
				0u,													// uint32_t              baseArrayLayer;
				static_cast<deUint32>(imageSize.w()),				// uint32_t              layerCount;
			}
		};

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
								0u, DE_NULL, 0u, DE_NULL, 1u, &imageBarrier);

		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// For each image layer or slice (3D), create an attachment and a pipeline
	{
		const VkImageAspectFlags	depthStencilAspect		= getFormatAspectFlags(caseDef.depthStencilFormat);
		const bool					useDepth				= (depthStencilAspect & VK_IMAGE_ASPECT_DEPTH_BIT)   != 0;
		const bool					useStencil				= (depthStencilAspect & VK_IMAGE_ASPECT_STENCIL_BIT) != 0;
		VkPipeline					basePipeline			= DE_NULL;

		// Color attachments are first in the framebuffer
		for (int subpassNdx = 0; subpassNdx < numSlices; ++subpassNdx)
		{
			colorAttachments.push_back(makeSharedPtr(
				makeImageView(vk, device, *colorImage, getImageViewSliceType(caseDef.viewType), caseDef.colorFormat, makeColorSubresourceRange(subpassNdx, 1))));
			attachmentHandles.push_back(**colorAttachments.back());

			// We also have to create pipelines for each subpass
			pipelines.push_back(makeSharedPtr(makeGraphicsPipeline(
				vk, device, basePipeline, *pipelineLayout, *renderPass, *vertexModule, *fragmentModule, imageSize.swizzle(0, 1), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				static_cast<deUint32>(subpassNdx), useDepth, useStencil)));

			basePipeline = **pipelines.front();
		}

		// Then D/S attachments, if any
		if (useDepthStencil)
		for (int subpassNdx = 0; subpassNdx < numSlices; ++subpassNdx)
		{
			depthStencilAttachments.push_back(makeSharedPtr(
				makeImageView(vk, device, *depthStencilImage, VK_IMAGE_VIEW_TYPE_2D, caseDef.depthStencilFormat, makeImageSubresourceRange(depthStencilAspect, 0u, 1u, subpassNdx, 1u))));
			attachmentHandles.push_back(**depthStencilAttachments.back());
		}
	}

	framebuffer = makeFramebuffer(vk, device, *renderPass, static_cast<deUint32>(attachmentHandles.size()), &attachmentHandles[0], static_cast<deUint32>(imageSize.x()), static_cast<deUint32>(imageSize.y()));

	{
		const Unique<VkCommandPool>		cmdPool		(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Unique<VkCommandBuffer>	cmdBuffer	(makeCommandBuffer(vk, device, *cmdPool));

		beginCommandBuffer(vk, *cmdBuffer);
		{
			vector<VkClearValue>	clearValues	(numSlices, getClearValue(caseDef.colorFormat));

			if (useDepthStencil)
				clearValues.insert(clearValues.end(), numSlices, makeClearValueDepthStencil(REFERENCE_DEPTH_VALUE, REFERENCE_STENCIL_VALUE));

			const VkRect2D			renderArea	=
			{
				makeOffset2D(0, 0),
				makeExtent2D(imageSize.x(), imageSize.y()),
			};
			const VkRenderPassBeginInfo	renderPassBeginInfo	=
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// VkStructureType         sType;
				DE_NULL,									// const void*             pNext;
				*renderPass,								// VkRenderPass            renderPass;
				*framebuffer,								// VkFramebuffer           framebuffer;
				renderArea,									// VkRect2D                renderArea;
				static_cast<deUint32>(clearValues.size()),	// uint32_t                clearValueCount;
				&clearValues[0],							// const VkClearValue*     pClearValues;
			};
			const VkDeviceSize		vertexBufferOffset	= 0ull;

			vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
		}

		// Draw
		for (deUint32 subpassNdx = 0; subpassNdx < static_cast<deUint32>(numSlices); ++subpassNdx)
		{
			if (subpassNdx != 0)
				vk.cmdNextSubpass(*cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **pipelines[subpassNdx]);
			vk.cmdDraw(*cmdBuffer, 4u, 1u, subpassNdx*4u, 0u);
		}

		vk.cmdEndRenderPass(*cmdBuffer);

		// Copy colorImage -> host visible colorBuffer
		{
			const VkImageMemoryBarrier	imageBarriers[]	=
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
					DE_NULL,										// const void*				pNext;
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			// VkAccessFlags			outputMask;
					VK_ACCESS_TRANSFER_READ_BIT,					// VkAccessFlags			inputMask;
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,			// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
					*colorImage,									// VkImage					image;
					makeColorSubresourceRange(0, imageSize.w())		// VkImageSubresourceRange	subresourceRange;
				}
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
								  0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(imageBarriers), imageBarriers);

			// Copy the checked region rather than the whole image
			const VkImageSubresourceLayers	subresource	=
			{
				VK_IMAGE_ASPECT_COLOR_BIT,							// VkImageAspectFlags    aspectMask;
				0u,													// uint32_t              mipLevel;
				static_cast<deUint32>(checkOffset.w()),				// uint32_t              baseArrayLayer;
				static_cast<deUint32>(checkSize.w()),				// uint32_t              layerCount;
			};

			const VkBufferImageCopy			region		=
			{
				0ull,																// VkDeviceSize                bufferOffset;
				0u,																	// uint32_t                    bufferRowLength;
				0u,																	// uint32_t                    bufferImageHeight;
				subresource,														// VkImageSubresourceLayers    imageSubresource;
				makeOffset3D(checkOffset.x(), checkOffset.y(), checkOffset.z()),	// VkOffset3D                  imageOffset;
				makeExtent3D(checkSize.swizzle(0, 1, 2)),							// VkExtent3D                  imageExtent;
			};

			vk.cmdCopyImageToBuffer(*cmdBuffer, *colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, 1u, &region);

			const VkBufferMemoryBarrier	bufferBarriers[] =
			{
				{
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType    sType;
					DE_NULL,									// const void*        pNext;
					VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags      srcAccessMask;
					VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags      dstAccessMask;
					VK_QUEUE_FAMILY_IGNORED,					// uint32_t           srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,					// uint32_t           dstQueueFamilyIndex;
					*colorBuffer,								// VkBuffer           buffer;
					0ull,										// VkDeviceSize       offset;
					VK_WHOLE_SIZE,								// VkDeviceSize       size;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
								  0u, DE_NULL, DE_LENGTH_OF_ARRAY(bufferBarriers), bufferBarriers, 0u, DE_NULL);
		}

		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Verify results
	{
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), VK_WHOLE_SIZE);

		const tcu::TextureFormat			format			= mapVkFormat(caseDef.colorFormat);
		const int							checkDepth		= maxLayersOrDepth(checkSize);
		const int							depthOffset		= maxLayersOrDepth(checkOffset);
		const tcu::ConstPixelBufferAccess	resultImage		(format, checkSize.x(), checkSize.y(), checkDepth, colorBufferAlloc->getHostPtr());
		tcu::TextureLevel					textureLevel	(format, checkSize.x(), checkSize.y(), checkDepth);
		const tcu::PixelBufferAccess		expectedImage	= textureLevel.getAccess();
		bool								ok				= false;

		generateExpectedImage(expectedImage, checkSize.swizzle(0, 1), depthOffset);

		if (isFloatFormat(caseDef.colorFormat))
			ok = tcu::floatThresholdCompare(context.getTestContext().getLog(), "Image Comparison", "", expectedImage, resultImage, tcu::Vec4(0.01f), tcu::COMPARE_LOG_RESULT);
		else
			ok = tcu::intThresholdCompare(context.getTestContext().getLog(), "Image Comparison", "", expectedImage, resultImage, tcu::UVec4(2), tcu::COMPARE_LOG_RESULT);

		return ok ? tcu::TestStatus::pass("Pass") : tcu::TestStatus::fail("Fail");
	}
}

void checkImageViewTypeRequirements (Context& context, const VkImageViewType viewType)
{
	if (viewType == VK_IMAGE_VIEW_TYPE_3D &&
		!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_maintenance1"))
		TCU_THROW(NotSupportedError, "Extension VK_KHR_maintenance1 not supported");

	if (viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY && !context.getDeviceFeatures().imageCubeArray)
		TCU_THROW(NotSupportedError, "Missing feature: imageCubeArray");
}

//! A test that can exercise very big color and depth/stencil attachment sizes.
//! If the total memory consumed by images is too large, or if the implementation returns OUT_OF_MEMORY error somewhere,
//! the test can be retried with a next increment of size reduction index, making the attachments smaller.
tcu::TestStatus testAttachmentSize (Context& context, const CaseDef caseDef)
{
	checkImageViewTypeRequirements(context, caseDef.viewType);

	if (caseDef.allocationKind == ALLOCATION_KIND_DEDICATED)
	{
		const std::string extensionName("VK_KHR_dedicated_allocation");

		if (!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), extensionName))
			TCU_THROW(NotSupportedError, std::string(extensionName + " is not supported").c_str());
	}

	return testWithSizeReduction(context, caseDef);
	// Never reached
}

vector<IVec4> getMipLevelSizes (IVec4 baseSize)
{
	vector<IVec4> levels;
	levels.push_back(baseSize);

	while (baseSize.x() != 1 || baseSize.y() != 1 || baseSize.z() != 1)
	{
		baseSize.x() = deMax32(baseSize.x() >> 1, 1);
		baseSize.y() = deMax32(baseSize.y() >> 1, 1);
		baseSize.z() = deMax32(baseSize.z() >> 1, 1);
		levels.push_back(baseSize);
	}

	return levels;
}

//! Compute memory consumed by each mip level, including all layers. Sizes include a padding for alignment.
vector<VkDeviceSize> getPerMipLevelStorageSize (const vector<IVec4>& mipLevelSizes, const VkDeviceSize pixelSize)
{
	const deInt64			levelAlignment	= 16;
	vector<VkDeviceSize>	storageSizes;

	for (vector<IVec4>::const_iterator it = mipLevelSizes.begin(); it != mipLevelSizes.end(); ++it)
		storageSizes.push_back(deAlign64(pixelSize * product(*it), levelAlignment));

	return storageSizes;
}

void drawToMipLevel (const Context&				context,
					 const CaseDef&				caseDef,
					 const int					mipLevel,
					 const IVec4&				mipSize,
					 const int					numSlices,
					 const VkImage				colorImage,
					 const VkImage				depthStencilImage,
					 const VkBuffer				vertexBuffer,
					 const VkPipelineLayout		pipelineLayout,
					 const VkShaderModule		vertexModule,
					 const VkShaderModule		fragmentModule)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const VkImageAspectFlags		depthStencilAspect	= getFormatAspectFlags(caseDef.depthStencilFormat);
	const bool						useDepth			= (depthStencilAspect & VK_IMAGE_ASPECT_DEPTH_BIT)   != 0;
	const bool						useStencil			= (depthStencilAspect & VK_IMAGE_ASPECT_STENCIL_BIT) != 0;
	const Unique<VkRenderPass>		renderPass			(makeRenderPass(vk, device, caseDef.colorFormat, caseDef.depthStencilFormat, static_cast<deUint32>(numSlices),
																		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
																		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
	vector<SharedPtrVkPipeline>		pipelines;
	vector<SharedPtrVkImageView>	colorAttachments;
	vector<SharedPtrVkImageView>	depthStencilAttachments;
	vector<VkImageView>				attachmentHandles;			// all attachments (color and d/s)

	// For each image layer or slice (3D), create an attachment and a pipeline
	{
		VkPipeline					basePipeline			= DE_NULL;

		// Color attachments are first in the framebuffer
		for (int subpassNdx = 0; subpassNdx < numSlices; ++subpassNdx)
		{
			colorAttachments.push_back(makeSharedPtr(makeImageView(
				vk, device, colorImage, getImageViewSliceType(caseDef.viewType), caseDef.colorFormat,
				makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 1u, subpassNdx, 1u))));
			attachmentHandles.push_back(**colorAttachments.back());

			// We also have to create pipelines for each subpass
			pipelines.push_back(makeSharedPtr(makeGraphicsPipeline(
				vk, device, basePipeline, pipelineLayout, *renderPass, vertexModule, fragmentModule, mipSize.swizzle(0, 1), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				static_cast<deUint32>(subpassNdx), useDepth, useStencil)));

			basePipeline = **pipelines.front();
		}

		// Then D/S attachments, if any
		if (useDepth || useStencil)
		for (int subpassNdx = 0; subpassNdx < numSlices; ++subpassNdx)
		{
			depthStencilAttachments.push_back(makeSharedPtr(makeImageView(
				vk, device, depthStencilImage, VK_IMAGE_VIEW_TYPE_2D, caseDef.depthStencilFormat,
				makeImageSubresourceRange(depthStencilAspect, mipLevel, 1u, subpassNdx, 1u))));
			attachmentHandles.push_back(**depthStencilAttachments.back());
		}
	}

	const Unique<VkFramebuffer>			framebuffer (makeFramebuffer(vk, device, *renderPass, static_cast<deUint32>(attachmentHandles.size()), &attachmentHandles[0],
																	 static_cast<deUint32>(mipSize.x()), static_cast<deUint32>(mipSize.y())));

	{
		const Unique<VkCommandPool>		cmdPool		(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Unique<VkCommandBuffer>	cmdBuffer	(makeCommandBuffer(vk, device, *cmdPool));

		beginCommandBuffer(vk, *cmdBuffer);
		{
			vector<VkClearValue>	clearValues	(numSlices, getClearValue(caseDef.colorFormat));

			if (useDepth || useStencil)
				clearValues.insert(clearValues.end(), numSlices, makeClearValueDepthStencil(REFERENCE_DEPTH_VALUE, REFERENCE_STENCIL_VALUE));

			const VkRect2D			renderArea	=
			{
				makeOffset2D(0, 0),
				makeExtent2D(mipSize.x(), mipSize.y()),
			};
			const VkRenderPassBeginInfo	renderPassBeginInfo	=
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// VkStructureType         sType;
				DE_NULL,									// const void*             pNext;
				*renderPass,								// VkRenderPass            renderPass;
				*framebuffer,								// VkFramebuffer           framebuffer;
				renderArea,									// VkRect2D                renderArea;
				static_cast<deUint32>(clearValues.size()),	// uint32_t                clearValueCount;
				&clearValues[0],							// const VkClearValue*     pClearValues;
			};
			const VkDeviceSize		vertexBufferOffset	= 0ull;

			vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer, &vertexBufferOffset);
		}

		// Draw
		for (deUint32 subpassNdx = 0; subpassNdx < static_cast<deUint32>(numSlices); ++subpassNdx)
		{
			if (subpassNdx != 0)
				vk.cmdNextSubpass(*cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **pipelines[subpassNdx]);
			vk.cmdDraw(*cmdBuffer, 4u, 1u, subpassNdx*4u, 0u);
		}

		vk.cmdEndRenderPass(*cmdBuffer);

		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}
}

//! Use image mip levels as attachments
tcu::TestStatus testRenderToMipMaps (Context& context, const CaseDef caseDef)
{
	checkImageViewTypeRequirements(context, caseDef.viewType);

	const DeviceInterface&			vk					= context.getDeviceInterface();
	const InstanceInterface&		vki					= context.getInstanceInterface();
	const VkDevice					device				= context.getDevice();
	const VkPhysicalDevice			physDevice			= context.getPhysicalDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&						allocator			= context.getDefaultAllocator();

	const IVec4						imageSize				= caseDef.imageSizeHint;	// MAX_SIZE is not used in this test
	const deInt32					numSlices				= maxLayersOrDepth(imageSize);
	const vector<IVec4>				mipLevelSizes			= getMipLevelSizes(imageSize);
	const vector<VkDeviceSize>		mipLevelStorageSizes	= getPerMipLevelStorageSize(mipLevelSizes, tcu::getPixelSize(mapVkFormat(caseDef.colorFormat)));
	const int						numMipLevels			= static_cast<int>(mipLevelSizes.size());
	const bool						useDepthStencil			= (caseDef.depthStencilFormat != VK_FORMAT_UNDEFINED);

	if (caseDef.allocationKind == ALLOCATION_KIND_DEDICATED)
	{
		const std::string extensionName("VK_KHR_dedicated_allocation");

		if (!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), extensionName))
			TCU_THROW(NotSupportedError, std::string(extensionName + " is not supported").c_str());
	}

	if (useDepthStencil && !isDepthStencilFormatSupported(vki, physDevice, caseDef.depthStencilFormat))
		TCU_THROW(NotSupportedError, "Unsupported depth/stencil format");

	// Create a color buffer big enough to hold all layers and mip levels
	const VkDeviceSize				colorBufferSize		= sum(mipLevelStorageSizes);
	const Unique<VkBuffer>			colorBuffer			(makeBuffer(vk, device, colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	const UniquePtr<Allocation>		colorBufferAlloc	(bindBuffer(vki, vk, physDevice, device, *colorBuffer, MemoryRequirement::HostVisible, allocator, caseDef.allocationKind));

	{
		deMemset(colorBufferAlloc->getHostPtr(), 0, static_cast<std::size_t>(colorBufferSize));
		flushMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), VK_WHOLE_SIZE);
	}

	const Unique<VkShaderModule>	vertexModule		(createShaderModule	(vk, device, context.getBinaryCollection().get("vert"), 0u));
	const Unique<VkShaderModule>	fragmentModule		(createShaderModule	(vk, device, context.getBinaryCollection().get("frag"), 0u));
	const Unique<VkPipelineLayout>	pipelineLayout		(makePipelineLayout	(vk, device));

	Move<VkImage>					colorImage;
	MovePtr<Allocation>				colorImageAlloc;
	Move<VkImage>					depthStencilImage;
	MovePtr<Allocation>				depthStencilImageAlloc;
	Move<VkBuffer>					vertexBuffer;
	MovePtr<Allocation>				vertexBufferAlloc;

	// Create a color image
	{
		const VkImageUsageFlags	imageUsage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		colorImage		= makeImage(vk, device, getImageCreateFlags(caseDef.viewType), getImageType(caseDef.viewType), caseDef.colorFormat,
									imageSize.swizzle(0, 1, 2), numMipLevels, imageSize.w(), imageUsage);
		colorImageAlloc	= bindImage(vki, vk, physDevice, device, *colorImage, MemoryRequirement::Any, allocator, caseDef.allocationKind);
	}

	// Create a depth/stencil image (always a 2D image, optionally layered)
	if (useDepthStencil)
	{
		const VkImageUsageFlags	imageUsage	= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		depthStencilImage		= makeImage(vk, device, (VkImageCreateFlags)0, VK_IMAGE_TYPE_2D, caseDef.depthStencilFormat,
											IVec3(imageSize.x(), imageSize.y(), 1), numMipLevels, numSlices, imageUsage);
		depthStencilImageAlloc	= bindImage(vki, vk, physDevice, device, *depthStencilImage, MemoryRequirement::Any, allocator, caseDef.allocationKind);
	}

	// Create a vertex buffer
	{
		const vector<Vertex4RGBA>	vertices			= genFullQuadVertices(numSlices);
		const VkDeviceSize			vertexBufferSize	= sizeInBytes(vertices);

		vertexBuffer		= makeBuffer(vk, device, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		vertexBufferAlloc	= bindBuffer(vki, vk, physDevice, device, *vertexBuffer, MemoryRequirement::HostVisible, allocator, caseDef.allocationKind);

		deMemcpy(vertexBufferAlloc->getHostPtr(), &vertices[0], static_cast<std::size_t>(vertexBufferSize));
		flushMappedMemoryRange(vk, device, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset(), vertexBufferSize);
	}

	// Prepare images
	{
		const Unique<VkCommandPool>		cmdPool		(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Unique<VkCommandBuffer>	cmdBuffer	(makeCommandBuffer(vk, device, *cmdPool));

		beginCommandBuffer(vk, *cmdBuffer);

		const VkImageMemoryBarrier	imageBarriers[]	=
		{
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType            sType;
				DE_NULL,											// const void*                pNext;
				(VkAccessFlags)0,									// VkAccessFlags              srcAccessMask;
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// VkAccessFlags              dstAccessMask;
				VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout              oldLayout;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout              newLayout;
				VK_QUEUE_FAMILY_IGNORED,							// uint32_t                   srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,							// uint32_t                   dstQueueFamilyIndex;
				*colorImage,										// VkImage                    image;
				{													// VkImageSubresourceRange    subresourceRange;
					VK_IMAGE_ASPECT_COLOR_BIT,							// VkImageAspectFlags    aspectMask;
					0u,													// uint32_t              baseMipLevel;
					static_cast<deUint32>(numMipLevels),				// uint32_t              levelCount;
					0u,													// uint32_t              baseArrayLayer;
					static_cast<deUint32>(imageSize.w()),				// uint32_t              layerCount;
				},
			},
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType            sType;
				DE_NULL,											// const void*                pNext;
				(VkAccessFlags)0,									// VkAccessFlags              srcAccessMask;
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,		// VkAccessFlags              dstAccessMask;
				VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout              oldLayout;
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	// VkImageLayout              newLayout;
				VK_QUEUE_FAMILY_IGNORED,							// uint32_t                   srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,							// uint32_t                   dstQueueFamilyIndex;
				*depthStencilImage,									// VkImage                    image;
				{													// VkImageSubresourceRange    subresourceRange;
					getFormatAspectFlags(caseDef.depthStencilFormat),	// VkImageAspectFlags    aspectMask;
					0u,													// uint32_t              baseMipLevel;
					static_cast<deUint32>(numMipLevels),				// uint32_t              levelCount;
					0u,													// uint32_t              baseArrayLayer;
					static_cast<deUint32>(numSlices),					// uint32_t              layerCount;
				},
			}
		};

		const deUint32	numImageBarriers = static_cast<deUint32>(DE_LENGTH_OF_ARRAY(imageBarriers) - (useDepthStencil ? 0 : 1));

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
								0u, DE_NULL, 0u, DE_NULL, numImageBarriers, imageBarriers);

		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Draw
	for (int mipLevel = 0; mipLevel < numMipLevels; ++mipLevel)
	{
		const IVec4&	mipSize		= mipLevelSizes[mipLevel];
		const int		levelSlices	= maxLayersOrDepth(mipSize);

		drawToMipLevel (context, caseDef, mipLevel, mipSize, levelSlices, *colorImage, *depthStencilImage, *vertexBuffer, *pipelineLayout,
						*vertexModule, *fragmentModule);
	}

	// Copy results: colorImage -> host visible colorBuffer
	{
		const Unique<VkCommandPool>		cmdPool		(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Unique<VkCommandBuffer>	cmdBuffer	(makeCommandBuffer(vk, device, *cmdPool));

		beginCommandBuffer(vk, *cmdBuffer);

		{
			const VkImageMemoryBarrier	imageBarriers[]	=
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType            sType;
					DE_NULL,										// const void*                pNext;
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			// VkAccessFlags              srcAccessMask;
					VK_ACCESS_TRANSFER_READ_BIT,					// VkAccessFlags              dstAccessMask;
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout              oldLayout;
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,			// VkImageLayout              newLayout;
					VK_QUEUE_FAMILY_IGNORED,						// uint32_t                   srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,						// uint32_t                   dstQueueFamilyIndex;
					*colorImage,									// VkImage                    image;
					{												// VkImageSubresourceRange    subresourceRange;
						VK_IMAGE_ASPECT_COLOR_BIT,							// VkImageAspectFlags    aspectMask;
						0u,													// uint32_t              baseMipLevel;
						static_cast<deUint32>(numMipLevels),				// uint32_t              levelCount;
						0u,													// uint32_t              baseArrayLayer;
						static_cast<deUint32>(imageSize.w()),				// uint32_t              layerCount;
					},
				}
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
									0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(imageBarriers), imageBarriers);
		}
		{
			vector<VkBufferImageCopy>	regions;
			VkDeviceSize				levelOffset = 0ull;
			VkBufferImageCopy			workRegion	=
			{
				0ull,																				// VkDeviceSize                bufferOffset;
				0u,																					// uint32_t                    bufferRowLength;
				0u,																					// uint32_t                    bufferImageHeight;
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, imageSize.w()),		// VkImageSubresourceLayers    imageSubresource;
				makeOffset3D(0, 0, 0),																// VkOffset3D                  imageOffset;
				makeExtent3D(0, 0, 0),																// VkExtent3D                  imageExtent;
			};

			for (int mipLevel = 0; mipLevel < numMipLevels; ++mipLevel)
			{
				workRegion.bufferOffset					= levelOffset;
				workRegion.imageSubresource.mipLevel	= static_cast<deUint32>(mipLevel);
				workRegion.imageExtent					= makeExtent3D(mipLevelSizes[mipLevel].swizzle(0, 1, 2));

				regions.push_back(workRegion);

				levelOffset += mipLevelStorageSizes[mipLevel];
			}

			vk.cmdCopyImageToBuffer(*cmdBuffer, *colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, static_cast<deUint32>(regions.size()), &regions[0]);
		}
		{
			const VkBufferMemoryBarrier	bufferBarriers[] =
			{
				{
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType    sType;
					DE_NULL,									// const void*        pNext;
					VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags      srcAccessMask;
					VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags      dstAccessMask;
					VK_QUEUE_FAMILY_IGNORED,					// uint32_t           srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,					// uint32_t           dstQueueFamilyIndex;
					*colorBuffer,								// VkBuffer           buffer;
					0ull,										// VkDeviceSize       offset;
					VK_WHOLE_SIZE,								// VkDeviceSize       size;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
									0u, DE_NULL, DE_LENGTH_OF_ARRAY(bufferBarriers), bufferBarriers, 0u, DE_NULL);
		}

		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Verify results (per mip level)
	{
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), VK_WHOLE_SIZE);

		const tcu::TextureFormat			format			= mapVkFormat(caseDef.colorFormat);

		VkDeviceSize						levelOffset		= 0ull;
		bool								allOk			= true;

		for (int mipLevel = 0; mipLevel < numMipLevels; ++mipLevel)
		{
			const IVec4&						mipSize			= mipLevelSizes[mipLevel];
			const void*	const					pLevelData		= static_cast<const deUint8*>(colorBufferAlloc->getHostPtr()) + levelOffset;
			const int							levelDepth		= maxLayersOrDepth(mipSize);
			const tcu::ConstPixelBufferAccess	resultImage		(format, mipSize.x(), mipSize.y(), levelDepth, pLevelData);
			tcu::TextureLevel					textureLevel	(format, mipSize.x(), mipSize.y(), levelDepth);
			const tcu::PixelBufferAccess		expectedImage	= textureLevel.getAccess();
			const std::string					comparisonName	= "Mip level " + de::toString(mipLevel);
			bool								ok				= false;

			generateExpectedImage(expectedImage, mipSize.swizzle(0, 1), 0);

			if (isFloatFormat(caseDef.colorFormat))
				ok = tcu::floatThresholdCompare(context.getTestContext().getLog(), "Image Comparison", comparisonName.c_str(), expectedImage, resultImage, tcu::Vec4(0.01f), tcu::COMPARE_LOG_RESULT);
			else
				ok = tcu::intThresholdCompare(context.getTestContext().getLog(), "Image Comparison", comparisonName.c_str(), expectedImage, resultImage, tcu::UVec4(2), tcu::COMPARE_LOG_RESULT);

			allOk		=  allOk && ok;	// keep testing all levels, even if we know it's a fail overall
			levelOffset += mipLevelStorageSizes[mipLevel];
		}

		return allOk ? tcu::TestStatus::pass("Pass") : tcu::TestStatus::fail("Fail");
	}
}

std::string getSizeDescription (const IVec4& size)
{
	std::ostringstream str;

	const char* const description[4] =
	{
		"width", "height", "depth", "layers"
	};

	int numMaxComponents = 0;

	for (int i = 0; i < 4; ++i)
	{
		if (size[i] == MAX_SIZE)
		{
			if (numMaxComponents > 0)
				str << "_";

			str << description[i];
			++numMaxComponents;
		}
	}

	if (numMaxComponents == 0)
		str << "small";

	return str.str();
}

inline std::string getFormatString (const VkFormat format)
{
	std::string name(getFormatName(format));
	return de::toLower(name.substr(10));
}

std::string getFormatString (const VkFormat colorFormat, const VkFormat depthStencilFormat)
{
	std::ostringstream str;
	str << getFormatString(colorFormat);
	if (depthStencilFormat != VK_FORMAT_UNDEFINED)
		str << "_" << getFormatString(depthStencilFormat);
	return str.str();
}

std::string getShortImageViewTypeName (const VkImageViewType imageViewType)
{
	std::string s(getImageViewTypeName(imageViewType));
	return de::toLower(s.substr(19));
}

inline BVec4 bvecFromMask (deUint32 mask)
{
	return BVec4((mask >> 0) & 1,
				 (mask >> 1) & 1,
				 (mask >> 2) & 1,
				 (mask >> 3) & 1);
}

vector<IVec4> genSizeCombinations (const IVec4& baselineSize, const deUint32 sizeMask, const VkImageViewType imageViewType)
{
	vector<IVec4>		sizes;
	std::set<deUint32>	masks;

	for (deUint32 i = 0; i < (1u << 4); ++i)
	{
		// Cube images have square faces
		if (isCube(imageViewType) && ((i & MASK_WH) != 0))
			i |= MASK_WH;

		masks.insert(i & sizeMask);
	}

	for (std::set<deUint32>::const_iterator it = masks.begin(); it != masks.end(); ++it)
		sizes.push_back(tcu::select(IVec4(MAX_SIZE), baselineSize, bvecFromMask(*it)));

	return sizes;
}

void addTestCasesWithFunctions (tcu::TestCaseGroup* group, AllocationKind allocationKind)
{
	const struct
	{
		VkImageViewType		viewType;
		IVec4				baselineSize;	//!< image size: (dimX, dimY, dimZ, arraySize)
		deUint32			sizeMask;		//!< if a dimension is masked, generate a huge size case for it
	} testCase[] =
	{
		{ VK_IMAGE_VIEW_TYPE_1D,			IVec4(54,  1, 1,   1),	MASK_W			},
		{ VK_IMAGE_VIEW_TYPE_1D_ARRAY,		IVec4(54,  1, 1,   4),	MASK_W_LAYERS	},
		{ VK_IMAGE_VIEW_TYPE_2D,			IVec4(44, 23, 1,   1),	MASK_WH			},
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		IVec4(44, 23, 1,   4),	MASK_WH_LAYERS	},
		{ VK_IMAGE_VIEW_TYPE_3D,			IVec4(22, 31, 7,   1),	MASK_WHD		},
		{ VK_IMAGE_VIEW_TYPE_CUBE,			IVec4(35, 35, 1,   6),	MASK_WH			},
		{ VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,	IVec4(35, 35, 1, 2*6),	MASK_WH_LAYERS	},
	};

	const VkFormat format[]	=
	{
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
	};

	const VkFormat depthStencilFormat[] =
	{
		VK_FORMAT_UNDEFINED,			// don't use a depth/stencil attachment
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,	// one of the following mixed formats must be supported
		VK_FORMAT_D32_SFLOAT_S8_UINT,
	};

	for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(testCase); ++caseNdx)
	{
		MovePtr<tcu::TestCaseGroup>	imageGroup(new tcu::TestCaseGroup(group->getTestContext(), getShortImageViewTypeName(testCase[caseNdx].viewType).c_str(), ""));

		// Generate attachment size cases
		{
			const vector<IVec4> sizes = genSizeCombinations(testCase[caseNdx].baselineSize, testCase[caseNdx].sizeMask, testCase[caseNdx].viewType);

			MovePtr<tcu::TestCaseGroup>	smallGroup(new tcu::TestCaseGroup(group->getTestContext(), "small", ""));
			MovePtr<tcu::TestCaseGroup>	hugeGroup (new tcu::TestCaseGroup(group->getTestContext(), "huge",  ""));

			imageGroup->addChild(smallGroup.get());
			imageGroup->addChild(hugeGroup.get());

			for (vector<IVec4>::const_iterator sizeIter = sizes.begin(); sizeIter != sizes.end(); ++sizeIter)
			{
				// The first size is the baseline size, put it in a dedicated group
				if (sizeIter == sizes.begin())
				{
					for (int dsFormatNdx = 0; dsFormatNdx < DE_LENGTH_OF_ARRAY(depthStencilFormat); ++dsFormatNdx)
					for (int formatNdx   = 0; formatNdx   < DE_LENGTH_OF_ARRAY(format);             ++formatNdx)
					{
						const CaseDef caseDef =
						{
							testCase[caseNdx].viewType,			// VkImageViewType	imageType;
							*sizeIter,							// IVec4			imageSizeHint;
							format[formatNdx],					// VkFormat			colorFormat;
							depthStencilFormat[dsFormatNdx],	// VkFormat			depthStencilFormat;
							allocationKind						// AllocationKind	allocationKind;
						};
						addFunctionCaseWithPrograms(smallGroup.get(), getFormatString(format[formatNdx], depthStencilFormat[dsFormatNdx]), "", initPrograms, testAttachmentSize, caseDef);
					}
				}
				else // All huge cases go into a separate group
				{
					if (allocationKind != ALLOCATION_KIND_DEDICATED)
					{
						MovePtr<tcu::TestCaseGroup>	sizeGroup	(new tcu::TestCaseGroup(group->getTestContext(), getSizeDescription(*sizeIter).c_str(), ""));
						const VkFormat				colorFormat	= VK_FORMAT_R8G8B8A8_UNORM;

						// Use the same color format for all cases, to reduce the number of permutations
						for (int dsFormatNdx = 0; dsFormatNdx < DE_LENGTH_OF_ARRAY(depthStencilFormat); ++dsFormatNdx)
						{
							const CaseDef caseDef =
							{
								testCase[caseNdx].viewType,			// VkImageViewType	viewType;
								*sizeIter,							// IVec4			imageSizeHint;
								colorFormat,						// VkFormat			colorFormat;
								depthStencilFormat[dsFormatNdx],	// VkFormat			depthStencilFormat;
								allocationKind						// AllocationKind	allocationKind;
							};
							addFunctionCaseWithPrograms(sizeGroup.get(), getFormatString(colorFormat, depthStencilFormat[dsFormatNdx]), "", initPrograms, testAttachmentSize, caseDef);
						}
						hugeGroup->addChild(sizeGroup.release());
					}
				}
			}
			smallGroup.release();
			hugeGroup.release();
		}

		// Generate mip map cases
		{
			MovePtr<tcu::TestCaseGroup>	mipmapGroup(new tcu::TestCaseGroup(group->getTestContext(), "mipmap", ""));

			for (int dsFormatNdx = 0; dsFormatNdx < DE_LENGTH_OF_ARRAY(depthStencilFormat); ++dsFormatNdx)
			for (int formatNdx   = 0; formatNdx   < DE_LENGTH_OF_ARRAY(format);             ++formatNdx)
			{
				const CaseDef caseDef =
				{
					testCase[caseNdx].viewType,			// VkImageViewType	imageType;
					testCase[caseNdx].baselineSize,		// IVec4			imageSizeHint;
					format[formatNdx],					// VkFormat			colorFormat;
					depthStencilFormat[dsFormatNdx],	// VkFormat			depthStencilFormat;
					allocationKind						// AllocationKind	allocationKind;
				};
				addFunctionCaseWithPrograms(mipmapGroup.get(), getFormatString(format[formatNdx], depthStencilFormat[dsFormatNdx]), "", initPrograms, testRenderToMipMaps, caseDef);
			}
			imageGroup->addChild(mipmapGroup.release());
		}

		group->addChild(imageGroup.release());
	}
}

void addCoreRenderToImageTests (tcu::TestCaseGroup* group)
{
	addTestCasesWithFunctions(group, ALLOCATION_KIND_SUBALLOCATED);
}

void addDedicatedAllocationRenderToImageTests (tcu::TestCaseGroup* group)
{
	addTestCasesWithFunctions(group, ALLOCATION_KIND_DEDICATED);
}

} // anonymous ns

tcu::TestCaseGroup* createRenderToImageTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	renderToImageTests	(new tcu::TestCaseGroup(testCtx, "render_to_image", "Render to image tests"));

	renderToImageTests->addChild(createTestGroup(testCtx, "core",					"Core render to image tests",								addCoreRenderToImageTests));
	renderToImageTests->addChild(createTestGroup(testCtx, "dedicated_allocation",	"Render to image tests for dedicated memory allocation",	addDedicatedAllocationRenderToImageTests));

	return renderToImageTests.release();
}

} // pipeline
} // vkt
