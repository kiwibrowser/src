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
 * \brief Sparse buffer tests
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesBufferTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktSparseResourcesTestsUtil.hpp"
#include "vktSparseResourcesBase.hpp"
#include "vktSparseResourcesBufferSparseBinding.hpp"
#include "vktSparseResourcesBufferSparseResidency.hpp"
#include "vktSparseResourcesBufferMemoryAliasing.hpp"

#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkMemUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"

#include "tcuTestLog.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"
#include "deMath.h"

#include <string>
#include <vector>
#include <map>

using namespace vk;
using de::MovePtr;
using de::UniquePtr;
using de::SharedPtr;
using tcu::Vec4;
using tcu::IVec2;
using tcu::IVec4;

namespace vkt
{
namespace sparse
{
namespace
{

typedef SharedPtr<UniquePtr<Allocation> > AllocationSp;

enum
{
	RENDER_SIZE	= 128,				//!< framebuffer size in pixels
	GRID_SIZE	= RENDER_SIZE / 8,	//!< number of grid tiles in a row
};

enum TestFlagBits
{
												//   sparseBinding is implied
	TEST_FLAG_ALIASED				= 1u << 0,	//!< sparseResidencyAliased
	TEST_FLAG_RESIDENCY				= 1u << 1,	//!< sparseResidencyBuffer
	TEST_FLAG_NON_RESIDENT_STRICT	= 1u << 2,	//!< residencyNonResidentStrict
};
typedef deUint32 TestFlags;

//! SparseAllocationBuilder output. Owns the allocated memory.
struct SparseAllocation
{
	deUint32							numResourceChunks;
	VkDeviceSize						resourceSize;		//!< buffer size in bytes
	std::vector<AllocationSp>			allocations;		//!< actual allocated memory
	std::vector<VkSparseMemoryBind>		memoryBinds;		//!< memory binds backing the resource
};

//! Utility to lay out memory allocations for a sparse buffer, including holes and aliased regions.
//! Will allocate memory upon building.
class SparseAllocationBuilder
{
public:
								SparseAllocationBuilder	(void);

	// \note "chunk" is the smallest (due to alignment) bindable amount of memory

	SparseAllocationBuilder&	addMemoryHole			(const deUint32 numChunks = 1u);
	SparseAllocationBuilder&	addResourceHole			(const deUint32 numChunks = 1u);
	SparseAllocationBuilder&	addMemoryBind			(const deUint32 numChunks = 1u);
	SparseAllocationBuilder&	addAliasedMemoryBind	(const deUint32 allocationNdx, const deUint32 chunkOffset, const deUint32 numChunks = 1u);
	SparseAllocationBuilder&	addMemoryAllocation		(void);

	MovePtr<SparseAllocation>	build					(const DeviceInterface&		vk,
														 const VkDevice				device,
														 Allocator&					allocator,
														 VkBufferCreateInfo			referenceCreateInfo,		//!< buffer size is ignored in this info
														 const VkDeviceSize			minChunkSize = 0ull) const;	//!< make sure chunks are at least this big

private:
	struct MemoryBind
	{
		deUint32	allocationNdx;
		deUint32	resourceChunkNdx;
		deUint32	memoryChunkNdx;
		deUint32	numChunks;
	};

	deUint32					m_allocationNdx;
	deUint32					m_resourceChunkNdx;
	deUint32					m_memoryChunkNdx;
	std::vector<MemoryBind>		m_memoryBinds;
	std::vector<deUint32>		m_chunksPerAllocation;

};

SparseAllocationBuilder::SparseAllocationBuilder (void)
	: m_allocationNdx		(0)
	, m_resourceChunkNdx	(0)
	, m_memoryChunkNdx		(0)
{
	m_chunksPerAllocation.push_back(0);
}

SparseAllocationBuilder& SparseAllocationBuilder::addMemoryHole (const deUint32 numChunks)
{
	m_memoryChunkNdx						+= numChunks;
	m_chunksPerAllocation[m_allocationNdx]	+= numChunks;

	return *this;
}

SparseAllocationBuilder& SparseAllocationBuilder::addResourceHole (const deUint32 numChunks)
{
	m_resourceChunkNdx += numChunks;

	return *this;
}

SparseAllocationBuilder& SparseAllocationBuilder::addMemoryAllocation (void)
{
	DE_ASSERT(m_memoryChunkNdx != 0);	// doesn't make sense to have an empty allocation

	m_allocationNdx  += 1;
	m_memoryChunkNdx  = 0;
	m_chunksPerAllocation.push_back(0);

	return *this;
}

SparseAllocationBuilder& SparseAllocationBuilder::addMemoryBind (const deUint32 numChunks)
{
	const MemoryBind memoryBind =
	{
		m_allocationNdx,
		m_resourceChunkNdx,
		m_memoryChunkNdx,
		numChunks
	};
	m_memoryBinds.push_back(memoryBind);

	m_resourceChunkNdx						+= numChunks;
	m_memoryChunkNdx						+= numChunks;
	m_chunksPerAllocation[m_allocationNdx]	+= numChunks;

	return *this;
}

SparseAllocationBuilder& SparseAllocationBuilder::addAliasedMemoryBind	(const deUint32 allocationNdx, const deUint32 chunkOffset, const deUint32 numChunks)
{
	DE_ASSERT(allocationNdx <= m_allocationNdx);

	const MemoryBind memoryBind =
	{
		allocationNdx,
		m_resourceChunkNdx,
		chunkOffset,
		numChunks
	};
	m_memoryBinds.push_back(memoryBind);

	m_resourceChunkNdx += numChunks;

	return *this;
}

inline VkMemoryRequirements requirementsWithSize (VkMemoryRequirements requirements, const VkDeviceSize size)
{
	requirements.size = size;
	return requirements;
}

MovePtr<SparseAllocation> SparseAllocationBuilder::build (const DeviceInterface&	vk,
														  const VkDevice			device,
														  Allocator&				allocator,
														  VkBufferCreateInfo		referenceCreateInfo,
														  const VkDeviceSize		minChunkSize) const
{

	MovePtr<SparseAllocation>	sparseAllocation			(new SparseAllocation());

								referenceCreateInfo.size	= sizeof(deUint32);
	const Unique<VkBuffer>		refBuffer					(createBuffer(vk, device, &referenceCreateInfo));
	const VkMemoryRequirements	memoryRequirements			= getBufferMemoryRequirements(vk, device, *refBuffer);
	const VkDeviceSize			chunkSize					= std::max(memoryRequirements.alignment, static_cast<VkDeviceSize>(deAlign64(minChunkSize, memoryRequirements.alignment)));

	for (std::vector<deUint32>::const_iterator numChunksIter = m_chunksPerAllocation.begin(); numChunksIter != m_chunksPerAllocation.end(); ++numChunksIter)
	{
		sparseAllocation->allocations.push_back(makeDeSharedPtr(
			allocator.allocate(requirementsWithSize(memoryRequirements, *numChunksIter * chunkSize), MemoryRequirement::Any)));
	}

	for (std::vector<MemoryBind>::const_iterator memBindIter = m_memoryBinds.begin(); memBindIter != m_memoryBinds.end(); ++memBindIter)
	{
		const Allocation&			alloc	= **sparseAllocation->allocations[memBindIter->allocationNdx];
		const VkSparseMemoryBind	bind	=
		{
			memBindIter->resourceChunkNdx * chunkSize,							// VkDeviceSize               resourceOffset;
			memBindIter->numChunks * chunkSize,									// VkDeviceSize               size;
			alloc.getMemory(),													// VkDeviceMemory             memory;
			alloc.getOffset() + memBindIter->memoryChunkNdx * chunkSize,		// VkDeviceSize               memoryOffset;
			(VkSparseMemoryBindFlags)0,											// VkSparseMemoryBindFlags    flags;
		};
		sparseAllocation->memoryBinds.push_back(bind);
		referenceCreateInfo.size = std::max(referenceCreateInfo.size, bind.resourceOffset + bind.size);
	}

	sparseAllocation->resourceSize		= referenceCreateInfo.size;
	sparseAllocation->numResourceChunks = m_resourceChunkNdx;

	return sparseAllocation;
}

VkImageCreateInfo makeImageCreateInfo (const VkFormat format, const IVec2& size, const VkImageUsageFlags usage)
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

Move<VkPipeline> makeGraphicsPipeline (const DeviceInterface&					vk,
									   const VkDevice							device,
									   const VkPipelineLayout					pipelineLayout,
									   const VkRenderPass						renderPass,
									   const IVec2								renderSize,
									   const VkPrimitiveTopology				topology,
									   const deUint32							stageCount,
									   const VkPipelineShaderStageCreateInfo*	pStages)
{
	const VkVertexInputBindingDescription vertexInputBindingDescription =
	{
		0u,								// uint32_t				binding;
		sizeof(Vec4),					// uint32_t				stride;
		VK_VERTEX_INPUT_RATE_VERTEX,	// VkVertexInputRate	inputRate;
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescription =
	{
		0u,									// uint32_t			location;
		0u,									// uint32_t			binding;
		VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat			format;
		0u,									// uint32_t			offset;
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
		topology,														// VkPrimitiveTopology                         topology;
		VK_FALSE,														// VkBool32                                    primitiveRestartEnable;
	};

	const VkViewport viewport = makeViewport(
		0.0f, 0.0f,
		static_cast<float>(renderSize.x()), static_cast<float>(renderSize.y()),
		0.0f, 1.0f);

	const VkRect2D scissor = {
		makeOffset2D(0, 0),
		makeExtent2D(static_cast<deUint32>(renderSize.x()), static_cast<deUint32>(renderSize.y())),
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

	const VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
		DE_NULL,											// const void*										pNext;
		(VkPipelineCreateFlags)0,							// VkPipelineCreateFlags							flags;
		stageCount,											// deUint32											stageCount;
		pStages,											// const VkPipelineShaderStageCreateInfo*			pStages;
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

//! Return true if there are any red (or all zero) pixels in the image
bool imageHasErrorPixels (const tcu::ConstPixelBufferAccess image)
{
	const Vec4 errorColor	= Vec4(1.0f, 0.0f, 0.0f, 1.0f);
	const Vec4 blankColor	= Vec4();

	for (int y = 0; y < image.getHeight(); ++y)
	for (int x = 0; x < image.getWidth(); ++x)
	{
		const Vec4 color = image.getPixel(x, y);
		if (color == errorColor || color == blankColor)
			return true;
	}

	return false;
}

class Renderer
{
public:
	typedef std::map<VkShaderStageFlagBits, const VkSpecializationInfo*>	SpecializationMap;

	//! Use the delegate to bind descriptor sets, vertex buffers, etc. and make a draw call
	struct Delegate
	{
		virtual			~Delegate		(void) {}
		virtual void	rendererDraw	(const VkPipelineLayout pipelineLayout, const VkCommandBuffer cmdBuffer) const = 0;
	};

	Renderer (const DeviceInterface&					vk,
			  const VkDevice							device,
			  Allocator&								allocator,
			  const deUint32							queueFamilyIndex,
			  const VkDescriptorSetLayout				descriptorSetLayout,	//!< may be NULL, if no descriptors are used
			  ProgramCollection<vk::ProgramBinary>&		binaryCollection,
			  const std::string&						vertexName,
			  const std::string&						fragmentName,
			  const VkBuffer							colorBuffer,
			  const IVec2&								renderSize,
			  const VkFormat							colorFormat,
			  const Vec4&								clearColor,
			  const VkPrimitiveTopology					topology,
			  SpecializationMap							specMap = SpecializationMap())
		: m_colorBuffer				(colorBuffer)
		, m_renderSize				(renderSize)
		, m_colorFormat				(colorFormat)
		, m_colorSubresourceRange	(makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u))
		, m_clearColor				(clearColor)
		, m_topology				(topology)
		, m_descriptorSetLayout		(descriptorSetLayout)
	{
		m_colorImage		= makeImage		(vk, device, makeImageCreateInfo(m_colorFormat, m_renderSize, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
		m_colorImageAlloc	= bindImage		(vk, device, allocator, *m_colorImage, MemoryRequirement::Any);
		m_colorAttachment	= makeImageView	(vk, device, *m_colorImage, VK_IMAGE_VIEW_TYPE_2D, m_colorFormat, m_colorSubresourceRange);

		m_vertexModule		= createShaderModule	(vk, device, binaryCollection.get(vertexName), 0u);
		m_fragmentModule	= createShaderModule	(vk, device, binaryCollection.get(fragmentName), 0u);

		const VkPipelineShaderStageCreateInfo pShaderStages[] =
		{
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				(VkPipelineShaderStageCreateFlags)0,						// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStageFlagBits				stage;
				*m_vertexModule,											// VkShaderModule						module;
				"main",														// const char*							pName;
				specMap[VK_SHADER_STAGE_VERTEX_BIT],						// const VkSpecializationInfo*			pSpecializationInfo;
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				(VkPipelineShaderStageCreateFlags)0,						// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlagBits				stage;
				*m_fragmentModule,											// VkShaderModule						module;
				"main",														// const char*							pName;
				specMap[VK_SHADER_STAGE_FRAGMENT_BIT],						// const VkSpecializationInfo*			pSpecializationInfo;
			}
		};

		m_renderPass		= makeRenderPass		(vk, device, m_colorFormat);
		m_framebuffer		= makeFramebuffer		(vk, device, *m_renderPass, 1u, &m_colorAttachment.get(),
													 static_cast<deUint32>(m_renderSize.x()), static_cast<deUint32>(m_renderSize.y()));
		m_pipelineLayout	= makePipelineLayout	(vk, device, m_descriptorSetLayout);
		m_pipeline			= makeGraphicsPipeline	(vk, device, *m_pipelineLayout, *m_renderPass, m_renderSize, m_topology, DE_LENGTH_OF_ARRAY(pShaderStages), pShaderStages);
		m_cmdPool			= makeCommandPool		(vk, device, queueFamilyIndex);
		m_cmdBuffer			= allocateCommandBuffer	(vk, device, *m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	}

	void draw (const DeviceInterface&	vk,
			   const VkDevice			device,
			   const VkQueue			queue,
			   const Delegate&			drawDelegate) const
	{
		beginCommandBuffer(vk, *m_cmdBuffer);

		const VkClearValue			clearValue	= makeClearValueColor(m_clearColor);
		const VkRect2D				renderArea	=
		{
			makeOffset2D(0, 0),
			makeExtent2D(m_renderSize.x(), m_renderSize.y()),
		};
		const VkRenderPassBeginInfo renderPassBeginInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,		// VkStructureType         sType;
			DE_NULL,										// const void*             pNext;
			*m_renderPass,									// VkRenderPass            renderPass;
			*m_framebuffer,									// VkFramebuffer           framebuffer;
			renderArea,										// VkRect2D                renderArea;
			1u,												// uint32_t                clearValueCount;
			&clearValue,									// const VkClearValue*     pClearValues;
		};
		vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		drawDelegate.rendererDraw(*m_pipelineLayout, *m_cmdBuffer);

		vk.cmdEndRenderPass(*m_cmdBuffer);

		// Prepare color image for copy
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
					*m_colorImage,												// VkImage					image;
					m_colorSubresourceRange,									// VkImageSubresourceRange	subresourceRange;
				},
			};

			vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
		}
		// Color image -> host buffer
		{
			const VkBufferImageCopy region =
			{
				0ull,																		// VkDeviceSize                bufferOffset;
				0u,																			// uint32_t                    bufferRowLength;
				0u,																			// uint32_t                    bufferImageHeight;
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u),			// VkImageSubresourceLayers    imageSubresource;
				makeOffset3D(0, 0, 0),														// VkOffset3D                  imageOffset;
				makeExtent3D(m_renderSize.x(), m_renderSize.y(), 1u),						// VkExtent3D                  imageExtent;
			};

			vk.cmdCopyImageToBuffer(*m_cmdBuffer, *m_colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_colorBuffer, 1u, &region);
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
					m_colorBuffer,									// VkBuffer           buffer;
					0ull,											// VkDeviceSize       offset;
					VK_WHOLE_SIZE,									// VkDeviceSize       size;
				},
			};

			vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
				0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers, DE_NULL, 0u);
		}

		VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *m_cmdBuffer);
	}

private:
	const VkBuffer					m_colorBuffer;
	const IVec2						m_renderSize;
	const VkFormat					m_colorFormat;
	const VkImageSubresourceRange	m_colorSubresourceRange;
	const Vec4						m_clearColor;
	const VkPrimitiveTopology		m_topology;
	const VkDescriptorSetLayout		m_descriptorSetLayout;

	Move<VkImage>					m_colorImage;
	MovePtr<Allocation>				m_colorImageAlloc;
	Move<VkImageView>				m_colorAttachment;
	Move<VkShaderModule>			m_vertexModule;
	Move<VkShaderModule>			m_fragmentModule;
	Move<VkRenderPass>				m_renderPass;
	Move<VkFramebuffer>				m_framebuffer;
	Move<VkPipelineLayout>			m_pipelineLayout;
	Move<VkPipeline>				m_pipeline;
	Move<VkCommandPool>				m_cmdPool;
	Move<VkCommandBuffer>			m_cmdBuffer;

	// "deleted"
				Renderer	(const Renderer&);
	Renderer&	operator=	(const Renderer&);
};

void bindSparseBuffer (const DeviceInterface& vk, const VkDevice device, const VkQueue sparseQueue, const VkBuffer buffer, const SparseAllocation& sparseAllocation)
{
	const VkSparseBufferMemoryBindInfo sparseBufferMemoryBindInfo =
	{
		buffer,														// VkBuffer                     buffer;
		static_cast<deUint32>(sparseAllocation.memoryBinds.size()),	// uint32_t                     bindCount;
		&sparseAllocation.memoryBinds[0],							// const VkSparseMemoryBind*    pBinds;
	};

	const VkBindSparseInfo bindInfo =
	{
		VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,					// VkStructureType                             sType;
		DE_NULL,											// const void*                                 pNext;
		0u,													// uint32_t                                    waitSemaphoreCount;
		DE_NULL,											// const VkSemaphore*                          pWaitSemaphores;
		1u,													// uint32_t                                    bufferBindCount;
		&sparseBufferMemoryBindInfo,						// const VkSparseBufferMemoryBindInfo*         pBufferBinds;
		0u,													// uint32_t                                    imageOpaqueBindCount;
		DE_NULL,											// const VkSparseImageOpaqueMemoryBindInfo*    pImageOpaqueBinds;
		0u,													// uint32_t                                    imageBindCount;
		DE_NULL,											// const VkSparseImageMemoryBindInfo*          pImageBinds;
		0u,													// uint32_t                                    signalSemaphoreCount;
		DE_NULL,											// const VkSemaphore*                          pSignalSemaphores;
	};

	const Unique<VkFence> fence(createFence(vk, device));

	VK_CHECK(vk.queueBindSparse(sparseQueue, 1u, &bindInfo, *fence));
	VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), VK_TRUE, ~0ull));
}

class SparseBufferTestInstance : public SparseResourcesBaseInstance, Renderer::Delegate
{
public:
	SparseBufferTestInstance (Context& context, const TestFlags flags)
		: SparseResourcesBaseInstance	(context)
		, m_aliased						((flags & TEST_FLAG_ALIASED)   != 0)
		, m_residency					((flags & TEST_FLAG_RESIDENCY) != 0)
		, m_nonResidentStrict			((flags & TEST_FLAG_NON_RESIDENT_STRICT) != 0)
		, m_renderSize					(RENDER_SIZE, RENDER_SIZE)
		, m_colorFormat					(VK_FORMAT_R8G8B8A8_UNORM)
		, m_colorBufferSize				(m_renderSize.x() * m_renderSize.y() * tcu::getPixelSize(mapVkFormat(m_colorFormat)))
	{
		const VkPhysicalDeviceFeatures	features	= getPhysicalDeviceFeatures(m_context.getInstanceInterface(), m_context.getPhysicalDevice());

		if (!features.sparseBinding)
			TCU_THROW(NotSupportedError, "Missing feature: sparseBinding");

		if (m_residency && !features.sparseResidencyBuffer)
			TCU_THROW(NotSupportedError, "Missing feature: sparseResidencyBuffer");

		if (m_aliased && !features.sparseResidencyAliased)
			TCU_THROW(NotSupportedError, "Missing feature: sparseResidencyAliased");

		if (m_nonResidentStrict && !m_context.getDeviceProperties().sparseProperties.residencyNonResidentStrict)
			TCU_THROW(NotSupportedError, "Missing sparse property: residencyNonResidentStrict");

		{
			QueueRequirementsVec requirements;
			requirements.push_back(QueueRequirements(VK_QUEUE_SPARSE_BINDING_BIT, 1u));
			requirements.push_back(QueueRequirements(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 1u));

			createDeviceSupportingQueues(requirements);
		}

		const DeviceInterface& vk		= getDeviceInterface();
		m_sparseQueue					= getQueue(VK_QUEUE_SPARSE_BINDING_BIT, 0u);
		m_universalQueue				= getQueue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0u);

		m_sharedQueueFamilyIndices[0]	= m_sparseQueue.queueFamilyIndex;
		m_sharedQueueFamilyIndices[1]	= m_universalQueue.queueFamilyIndex;

		m_colorBuffer					= makeBuffer(vk, getDevice(), makeBufferCreateInfo(m_colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
		m_colorBufferAlloc				= bindBuffer(vk, getDevice(), getAllocator(), *m_colorBuffer, MemoryRequirement::HostVisible);

		deMemset(m_colorBufferAlloc->getHostPtr(), 0, static_cast<std::size_t>(m_colorBufferSize));
		flushMappedMemoryRange(vk, getDevice(), m_colorBufferAlloc->getMemory(), m_colorBufferAlloc->getOffset(), m_colorBufferSize);
	}

protected:
	VkBufferCreateInfo getSparseBufferCreateInfo (const VkBufferUsageFlags usage) const
	{
		VkBufferCreateFlags	flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
		if (m_residency)
			flags |= VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
		if (m_aliased)
			flags |= VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;

		VkBufferCreateInfo referenceBufferCreateInfo =
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,				// VkStructureType        sType;
			DE_NULL,											// const void*            pNext;
			flags,												// VkBufferCreateFlags    flags;
			0u,	// override later								// VkDeviceSize           size;
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,			// VkBufferUsageFlags     usage;
			VK_SHARING_MODE_EXCLUSIVE,							// VkSharingMode          sharingMode;
			0u,													// uint32_t               queueFamilyIndexCount;
			DE_NULL,											// const uint32_t*        pQueueFamilyIndices;
		};

		if (m_sparseQueue.queueFamilyIndex != m_universalQueue.queueFamilyIndex)
		{
			referenceBufferCreateInfo.sharingMode			= VK_SHARING_MODE_CONCURRENT;
			referenceBufferCreateInfo.queueFamilyIndexCount	= DE_LENGTH_OF_ARRAY(m_sharedQueueFamilyIndices);
			referenceBufferCreateInfo.pQueueFamilyIndices	= m_sharedQueueFamilyIndices;
		}

		return referenceBufferCreateInfo;
	}

	void draw (const VkPrimitiveTopology	topology,
			   const VkDescriptorSetLayout	descriptorSetLayout	= DE_NULL,
			   Renderer::SpecializationMap	specMap				= Renderer::SpecializationMap())
	{
		const UniquePtr<Renderer> renderer(new Renderer(
			getDeviceInterface(), getDevice(), getAllocator(), m_universalQueue.queueFamilyIndex, descriptorSetLayout,
			m_context.getBinaryCollection(), "vert", "frag", *m_colorBuffer, m_renderSize, m_colorFormat, Vec4(1.0f, 0.0f, 0.0f, 1.0f), topology, specMap));

		renderer->draw(getDeviceInterface(), getDevice(), m_universalQueue.queueHandle, *this);
	}

	tcu::TestStatus verifyDrawResult (void) const
	{
		invalidateMappedMemoryRange(getDeviceInterface(), getDevice(), m_colorBufferAlloc->getMemory(), 0ull, m_colorBufferSize);

		const tcu::ConstPixelBufferAccess resultImage (mapVkFormat(m_colorFormat), m_renderSize.x(), m_renderSize.y(), 1u, m_colorBufferAlloc->getHostPtr());

		m_context.getTestContext().getLog()
			<< tcu::LogImageSet("Result", "Result") << tcu::LogImage("color0", "", resultImage) << tcu::TestLog::EndImageSet;

		if (imageHasErrorPixels(resultImage))
			return tcu::TestStatus::fail("Some buffer values were incorrect");
		else
			return tcu::TestStatus::pass("Pass");
	}

	const bool							m_aliased;
	const bool							m_residency;
	const bool							m_nonResidentStrict;

	Queue								m_sparseQueue;
	Queue								m_universalQueue;

private:
	const IVec2							m_renderSize;
	const VkFormat						m_colorFormat;
	const VkDeviceSize					m_colorBufferSize;

	Move<VkBuffer>						m_colorBuffer;
	MovePtr<Allocation>					m_colorBufferAlloc;

	deUint32							m_sharedQueueFamilyIndices[2];
};

void initProgramsDrawWithUBO (vk::SourceCollections& programCollection, const TestFlags flags)
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

	// Fragment shader
	{
		const bool			aliased				= (flags & TEST_FLAG_ALIASED) != 0;
		const bool			residency			= (flags & TEST_FLAG_RESIDENCY) != 0;
		const bool			nonResidentStrict	= (flags & TEST_FLAG_NON_RESIDENT_STRICT) != 0;
		const std::string	valueExpr			= (aliased ? "ivec4(3*(ndx % nonAliasedSize) ^ 127, 0, 0, 0)" : "ivec4(3*ndx ^ 127, 0, 0, 0)");

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) out vec4 o_color;\n"
			<< "\n"
			<< "layout(constant_id = 1) const int dataSize  = 1;\n"
			<< "layout(constant_id = 2) const int chunkSize = 1;\n"
			<< "\n"
			<< "layout(set = 0, binding = 0, std140) uniform SparseBuffer {\n"
			<< "    ivec4 data[dataSize];\n"
			<< "} ubo;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    const int fragNdx        = int(gl_FragCoord.x) + " << RENDER_SIZE << " * int(gl_FragCoord.y);\n"
			<< "    const int pageSize       = " << RENDER_SIZE << " * " << RENDER_SIZE << ";\n"
			<< "    const int numChunks      = dataSize / chunkSize;\n";

		if (aliased)
			src << "    const int nonAliasedSize = (numChunks > 1 ? dataSize - chunkSize : dataSize);\n";

		src << "    bool      ok             = true;\n"
			<< "\n"
			<< "    for (int ndx = fragNdx; ndx < dataSize; ndx += pageSize)\n"
			<< "    {\n";

		if (residency && nonResidentStrict)
		{
			src << "        if (ndx >= chunkSize && ndx < 2*chunkSize)\n"
				<< "            ok = ok && (ubo.data[ndx] == ivec4(0));\n"
				<< "        else\n"
				<< "            ok = ok && (ubo.data[ndx] == " + valueExpr + ");\n";
		}
		else if (residency)
		{
			src << "        if (ndx >= chunkSize && ndx < 2*chunkSize)\n"
				<< "            continue;\n"
				<< "        ok = ok && (ubo.data[ndx] == " << valueExpr << ");\n";
		}
		else
			src << "        ok = ok && (ubo.data[ndx] == " << valueExpr << ");\n";

		src << "    }\n"
			<< "\n"
			<< "    if (ok)\n"
			<< "        o_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
			<< "    else\n"
			<< "        o_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

//! Sparse buffer backing a UBO
class UBOTestInstance : public SparseBufferTestInstance
{
public:
	UBOTestInstance (Context& context, const TestFlags flags)
		: SparseBufferTestInstance	(context, flags)
	{
	}

	void rendererDraw (const VkPipelineLayout pipelineLayout, const VkCommandBuffer cmdBuffer) const
	{
		const DeviceInterface&	vk				= getDeviceInterface();
		const VkDeviceSize		vertexOffset	= 0ull;

		vk.cmdBindVertexBuffers	(cmdBuffer, 0u, 1u, &m_vertexBuffer.get(), &vertexOffset);
		vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0u, 1u, &m_descriptorSet.get(), 0u, DE_NULL);
		vk.cmdDraw				(cmdBuffer, 4u, 1u, 0u, 0u);
	}

	tcu::TestStatus iterate (void)
	{
		const DeviceInterface&		vk					= getDeviceInterface();
		MovePtr<SparseAllocation>	sparseAllocation;
		Move<VkBuffer>				sparseBuffer;
		Move<VkBuffer>				sparseBufferAliased;

		// Set up the sparse buffer
		{
			VkBufferCreateInfo	referenceBufferCreateInfo	= getSparseBufferCreateInfo(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			const VkDeviceSize	minChunkSize				= 512u;	// make sure the smallest allocation is at least this big
			deUint32			numMaxChunks				= 0u;

			// Check how many chunks we can allocate given the alignment and size requirements of UBOs
			{
				const UniquePtr<SparseAllocation> minAllocation(SparseAllocationBuilder()
					.addMemoryBind()
					.build(vk, getDevice(), getAllocator(), referenceBufferCreateInfo, minChunkSize));

				numMaxChunks = deMaxu32(static_cast<deUint32>(m_context.getDeviceProperties().limits.maxUniformBufferRange / minAllocation->resourceSize), 1u);
			}

			if (numMaxChunks < 4)
			{
				sparseAllocation = SparseAllocationBuilder()
					.addMemoryBind()
					.build(vk, getDevice(), getAllocator(), referenceBufferCreateInfo, minChunkSize);
			}
			else
			{
				// Try to use a non-trivial memory allocation scheme to make it different from a non-sparse binding
				SparseAllocationBuilder builder;
				builder.addMemoryBind();

				if (m_residency)
					builder.addResourceHole();

				builder
					.addMemoryAllocation()
					.addMemoryHole()
					.addMemoryBind();

				if (m_aliased)
					builder.addAliasedMemoryBind(0u, 0u);

				sparseAllocation = builder.build(vk, getDevice(), getAllocator(), referenceBufferCreateInfo, minChunkSize);
				DE_ASSERT(sparseAllocation->resourceSize <= m_context.getDeviceProperties().limits.maxUniformBufferRange);
			}

			// Create the buffer
			referenceBufferCreateInfo.size	= sparseAllocation->resourceSize;
			sparseBuffer					= makeBuffer(vk, getDevice(), referenceBufferCreateInfo);
			bindSparseBuffer(vk, getDevice(), m_sparseQueue.queueHandle, *sparseBuffer, *sparseAllocation);

			if (m_aliased)
			{
				sparseBufferAliased = makeBuffer(vk, getDevice(), referenceBufferCreateInfo);
				bindSparseBuffer(vk, getDevice(), m_sparseQueue.queueHandle, *sparseBufferAliased, *sparseAllocation);
			}
		}

		// Set uniform data
		{
			const bool					hasAliasedChunk		= (m_aliased && sparseAllocation->memoryBinds.size() > 1u);
			const VkDeviceSize			chunkSize			= sparseAllocation->resourceSize / sparseAllocation->numResourceChunks;
			const VkDeviceSize			stagingBufferSize	= sparseAllocation->resourceSize - (hasAliasedChunk ? chunkSize : 0);
			const deUint32				numBufferEntries	= static_cast<deUint32>(stagingBufferSize / sizeof(IVec4));

			const Unique<VkBuffer>		stagingBuffer		(makeBuffer(vk, getDevice(), makeBufferCreateInfo(stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)));
			const UniquePtr<Allocation>	stagingBufferAlloc	(bindBuffer(vk, getDevice(), getAllocator(), *stagingBuffer, MemoryRequirement::HostVisible));

			{
				// If aliased chunk is used, the staging buffer is smaller than the sparse buffer and we don't overwrite the last chunk
				IVec4* const pData = static_cast<IVec4*>(stagingBufferAlloc->getHostPtr());
				for (deUint32 i = 0; i < numBufferEntries; ++i)
					pData[i] = IVec4(3*i ^ 127, 0, 0, 0);

				flushMappedMemoryRange(vk, getDevice(), stagingBufferAlloc->getMemory(), stagingBufferAlloc->getOffset(), stagingBufferSize);

				const VkBufferCopy copyRegion =
				{
					0ull,						// VkDeviceSize    srcOffset;
					0ull,						// VkDeviceSize    dstOffset;
					stagingBufferSize,			// VkDeviceSize    size;
				};

				const Unique<VkCommandPool>		cmdPool		(makeCommandPool(vk, getDevice(), m_universalQueue.queueFamilyIndex));
				const Unique<VkCommandBuffer>	cmdBuffer	(allocateCommandBuffer(vk, getDevice(), *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

				beginCommandBuffer	(vk, *cmdBuffer);
				vk.cmdCopyBuffer	(*cmdBuffer, *stagingBuffer, *sparseBuffer, 1u, &copyRegion);
				endCommandBuffer	(vk, *cmdBuffer);

				submitCommandsAndWait(vk, getDevice(), m_universalQueue.queueHandle, *cmdBuffer);
				// Once the fence is signaled, the write is also available to the aliasing buffer.
			}
		}

		// Make sure that we don't try to access a larger range than is allowed. This only applies to a single chunk case.
		const deUint32 maxBufferRange = deMinu32(static_cast<deUint32>(sparseAllocation->resourceSize), m_context.getDeviceProperties().limits.maxUniformBufferRange);

		// Descriptor sets
		{
			m_descriptorSetLayout = DescriptorSetLayoutBuilder()
				.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.build(vk, getDevice());

			m_descriptorPool = DescriptorPoolBuilder()
				.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				.build(vk, getDevice(), VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

			m_descriptorSet = makeDescriptorSet(vk, getDevice(), *m_descriptorPool, *m_descriptorSetLayout);

			const VkBuffer					buffer				= (m_aliased ? *sparseBufferAliased : *sparseBuffer);
			const VkDescriptorBufferInfo	sparseBufferInfo	= makeDescriptorBufferInfo(buffer, 0ull, maxBufferRange);

			DescriptorSetUpdateBuilder()
				.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &sparseBufferInfo)
				.update(vk, getDevice());
		}

		// Vertex data
		{
			const Vec4 vertexData[] =
			{
				Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
				Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
				Vec4( 1.0f, -1.0f, 0.0f, 1.0f),
				Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
			};

			const VkDeviceSize	vertexBufferSize	= sizeof(vertexData);

			m_vertexBuffer		= makeBuffer(vk, getDevice(), makeBufferCreateInfo(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
			m_vertexBufferAlloc	= bindBuffer(vk, getDevice(), getAllocator(), *m_vertexBuffer, MemoryRequirement::HostVisible);

			deMemcpy(m_vertexBufferAlloc->getHostPtr(), &vertexData[0], vertexBufferSize);
			flushMappedMemoryRange(vk, getDevice(), m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset(), vertexBufferSize);
		}

		// Draw
		{
			std::vector<deInt32> specializationData;
			{
				const deUint32	numBufferEntries	= maxBufferRange / static_cast<deUint32>(sizeof(IVec4));
				const deUint32	numEntriesPerChunk	= numBufferEntries / sparseAllocation->numResourceChunks;

				specializationData.push_back(numBufferEntries);
				specializationData.push_back(numEntriesPerChunk);
			}

			const VkSpecializationMapEntry	specMapEntries[] =
			{
				{
					1u,					// uint32_t    constantID;
					0u,					// uint32_t    offset;
					sizeof(deInt32),	// size_t      size;
				},
				{
					2u,					// uint32_t    constantID;
					sizeof(deInt32),	// uint32_t    offset;
					sizeof(deInt32),	// size_t      size;
				},
			};

			const VkSpecializationInfo specInfo =
			{
				DE_LENGTH_OF_ARRAY(specMapEntries),		// uint32_t                           mapEntryCount;
				specMapEntries,							// const VkSpecializationMapEntry*    pMapEntries;
				sizeInBytes(specializationData),		// size_t                             dataSize;
				getDataOrNullptr(specializationData),	// const void*                        pData;
			};

			Renderer::SpecializationMap	specMap;
			specMap[VK_SHADER_STAGE_FRAGMENT_BIT] = &specInfo;

			draw(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, *m_descriptorSetLayout, specMap);
		}

		return verifyDrawResult();
	}

private:
	Move<VkBuffer>					m_vertexBuffer;
	MovePtr<Allocation>				m_vertexBufferAlloc;

	Move<VkDescriptorSetLayout>		m_descriptorSetLayout;
	Move<VkDescriptorPool>			m_descriptorPool;
	Move<VkDescriptorSet>			m_descriptorSet;
};

void initProgramsDrawGrid (vk::SourceCollections& programCollection, const TestFlags flags)
{
	DE_UNREF(flags);

	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in  vec4 in_position;\n"
			<< "layout(location = 0) out int  out_ndx;\n"
			<< "\n"
			<< "out gl_PerVertex {\n"
			<< "    vec4 gl_Position;\n"
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    gl_Position = in_position;\n"
			<< "    out_ndx     = gl_VertexIndex;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) flat in  int  in_ndx;\n"
			<< "layout(location = 0)      out vec4 o_color;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    if (in_ndx % 2 == 0)\n"
			<< "        o_color = vec4(vec3(1.0), 1.0);\n"
			<< "    else\n"
			<< "        o_color = vec4(vec3(0.75), 1.0);\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

//! Generate vertex positions for a grid of tiles composed of two triangles each (6 vertices)
void generateGrid (void* pRawData, const float step, const float ox, const float oy, const deUint32 numX, const deUint32 numY, const float z = 0.0f)
{
	typedef Vec4 (*TilePtr)[6];

	TilePtr const pData = static_cast<TilePtr>(pRawData);
	{
		for (deUint32 iy = 0; iy < numY; ++iy)
		for (deUint32 ix = 0; ix < numX; ++ix)
		{
			const deUint32	ndx	= ix + numX * iy;
			const float		x	= ox + step * static_cast<float>(ix);
			const float		y	= oy + step * static_cast<float>(iy);

			pData[ndx][0] = Vec4(x + step,	y,			z, 1.0f);
			pData[ndx][1] = Vec4(x,			y,			z, 1.0f);
			pData[ndx][2] = Vec4(x,			y + step,	z, 1.0f);

			pData[ndx][3] = Vec4(x,			y + step,	z, 1.0f);
			pData[ndx][4] = Vec4(x + step,	y + step,	z, 1.0f);
			pData[ndx][5] = Vec4(x + step,	y,			z, 1.0f);
		}
	}
}

//! Base test for a sparse buffer backing a vertex/index buffer
class DrawGridTestInstance : public SparseBufferTestInstance
{
public:
	DrawGridTestInstance (Context& context, const TestFlags flags, const VkBufferUsageFlags usage, const VkDeviceSize minChunkSize)
		: SparseBufferTestInstance	(context, flags)
	{
		const DeviceInterface&	vk							= getDeviceInterface();
		VkBufferCreateInfo		referenceBufferCreateInfo	= getSparseBufferCreateInfo(usage);

		{
			// Allocate two chunks, each covering half of the viewport
			SparseAllocationBuilder builder;
			builder.addMemoryBind();

			if (m_residency)
				builder.addResourceHole();

			builder
				.addMemoryAllocation()
				.addMemoryHole()
				.addMemoryBind();

			if (m_aliased)
				builder.addAliasedMemoryBind(0u, 0u);

			m_sparseAllocation	= builder.build(vk, getDevice(), getAllocator(), referenceBufferCreateInfo, minChunkSize);
		}

		// Create the buffer
		referenceBufferCreateInfo.size	= m_sparseAllocation->resourceSize;
		m_sparseBuffer					= makeBuffer(vk, getDevice(), referenceBufferCreateInfo);

		// Bind the memory
		bindSparseBuffer(vk, getDevice(), m_sparseQueue.queueHandle, *m_sparseBuffer, *m_sparseAllocation);

		m_perDrawBufferOffset	= m_sparseAllocation->resourceSize / m_sparseAllocation->numResourceChunks;
		m_stagingBufferSize		= 2 * m_perDrawBufferOffset;
		m_stagingBuffer			= makeBuffer(vk, getDevice(), makeBufferCreateInfo(m_stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
		m_stagingBufferAlloc	= bindBuffer(vk, getDevice(), getAllocator(), *m_stagingBuffer, MemoryRequirement::HostVisible);
	}

	tcu::TestStatus iterate (void)
	{
		initializeBuffers();

		const DeviceInterface&	vk	= getDeviceInterface();

		// Upload to the sparse buffer
		{
			flushMappedMemoryRange(vk, getDevice(), m_stagingBufferAlloc->getMemory(), m_stagingBufferAlloc->getOffset(), m_stagingBufferSize);

			VkDeviceSize	firstChunkOffset	= 0ull;
			VkDeviceSize	secondChunkOffset	= m_perDrawBufferOffset;

			if (m_residency)
				secondChunkOffset += m_perDrawBufferOffset;

			if (m_aliased)
				firstChunkOffset = secondChunkOffset + m_perDrawBufferOffset;

			const VkBufferCopy copyRegions[] =
			{
				{
					0ull,						// VkDeviceSize    srcOffset;
					firstChunkOffset,			// VkDeviceSize    dstOffset;
					m_perDrawBufferOffset,		// VkDeviceSize    size;
				},
				{
					m_perDrawBufferOffset,		// VkDeviceSize    srcOffset;
					secondChunkOffset,			// VkDeviceSize    dstOffset;
					m_perDrawBufferOffset,		// VkDeviceSize    size;
				},
			};

			const Unique<VkCommandPool>		cmdPool		(makeCommandPool(vk, getDevice(), m_universalQueue.queueFamilyIndex));
			const Unique<VkCommandBuffer>	cmdBuffer	(allocateCommandBuffer(vk, getDevice(), *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

			beginCommandBuffer	(vk, *cmdBuffer);
			vk.cmdCopyBuffer	(*cmdBuffer, *m_stagingBuffer, *m_sparseBuffer, DE_LENGTH_OF_ARRAY(copyRegions), copyRegions);
			endCommandBuffer	(vk, *cmdBuffer);

			submitCommandsAndWait(vk, getDevice(), m_universalQueue.queueHandle, *cmdBuffer);
		}

		draw(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		return verifyDrawResult();
	}

protected:
	virtual void				initializeBuffers		(void) = 0;

	VkDeviceSize				m_perDrawBufferOffset;

	VkDeviceSize				m_stagingBufferSize;
	Move<VkBuffer>				m_stagingBuffer;
	MovePtr<Allocation>			m_stagingBufferAlloc;

	MovePtr<SparseAllocation>	m_sparseAllocation;
	Move<VkBuffer>				m_sparseBuffer;
};

//! Sparse buffer backing a vertex input buffer
class VertexBufferTestInstance : public DrawGridTestInstance
{
public:
	VertexBufferTestInstance (Context& context, const TestFlags flags)
		: DrawGridTestInstance	(context,
								 flags,
								 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
								 GRID_SIZE * GRID_SIZE * 6 * sizeof(Vec4))
	{
	}

	void rendererDraw (const VkPipelineLayout pipelineLayout, const VkCommandBuffer cmdBuffer) const
	{
		DE_UNREF(pipelineLayout);

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Drawing a grid of triangles backed by a sparse vertex buffer. There should be no red pixels visible." << tcu::TestLog::EndMessage;

		const DeviceInterface&	vk				= getDeviceInterface();
		const deUint32			vertexCount		= 6 * (GRID_SIZE * GRID_SIZE) / 2;
		VkDeviceSize			vertexOffset	= 0ull;

		vk.cmdBindVertexBuffers	(cmdBuffer, 0u, 1u, &m_sparseBuffer.get(), &vertexOffset);
		vk.cmdDraw				(cmdBuffer, vertexCount, 1u, 0u, 0u);

		vertexOffset += m_perDrawBufferOffset * (m_residency ? 2 : 1);

		vk.cmdBindVertexBuffers	(cmdBuffer, 0u, 1u, &m_sparseBuffer.get(), &vertexOffset);
		vk.cmdDraw				(cmdBuffer, vertexCount, 1u, 0u, 0u);
	}

	void initializeBuffers (void)
	{
		deUint8*	pData	= static_cast<deUint8*>(m_stagingBufferAlloc->getHostPtr());
		const float	step	= 2.0f / static_cast<float>(GRID_SIZE);

		// Prepare data for two draw calls
		generateGrid(pData,							step, -1.0f, -1.0f, GRID_SIZE, GRID_SIZE/2);
		generateGrid(pData + m_perDrawBufferOffset,	step, -1.0f,  0.0f, GRID_SIZE, GRID_SIZE/2);
	}
};

//! Sparse buffer backing an index buffer
class IndexBufferTestInstance : public DrawGridTestInstance
{
public:
	IndexBufferTestInstance (Context& context, const TestFlags flags)
		: DrawGridTestInstance	(context,
								 flags,
								 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
								 GRID_SIZE * GRID_SIZE * 6 * sizeof(deUint32))
		, m_halfVertexCount		(6 * (GRID_SIZE * GRID_SIZE) / 2)
	{
	}

	void rendererDraw (const VkPipelineLayout pipelineLayout, const VkCommandBuffer cmdBuffer) const
	{
		DE_UNREF(pipelineLayout);

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Drawing a grid of triangles from a sparse index buffer. There should be no red pixels visible." << tcu::TestLog::EndMessage;

		const DeviceInterface&	vk				= getDeviceInterface();
		const VkDeviceSize		vertexOffset	= 0ull;
		VkDeviceSize			indexOffset		= 0ull;

		vk.cmdBindVertexBuffers	(cmdBuffer, 0u, 1u, &m_vertexBuffer.get(), &vertexOffset);

		vk.cmdBindIndexBuffer	(cmdBuffer, *m_sparseBuffer, indexOffset, VK_INDEX_TYPE_UINT32);
		vk.cmdDrawIndexed		(cmdBuffer, m_halfVertexCount, 1u, 0u, 0, 0u);

		indexOffset += m_perDrawBufferOffset * (m_residency ? 2 : 1);

		vk.cmdBindIndexBuffer	(cmdBuffer, *m_sparseBuffer, indexOffset, VK_INDEX_TYPE_UINT32);
		vk.cmdDrawIndexed		(cmdBuffer, m_halfVertexCount, 1u, 0u, 0, 0u);
	}

	void initializeBuffers (void)
	{
		// Vertex buffer
		const DeviceInterface&	vk					= getDeviceInterface();
		const VkDeviceSize		vertexBufferSize	= 2 * m_halfVertexCount * sizeof(Vec4);
								m_vertexBuffer		= makeBuffer(vk, getDevice(), makeBufferCreateInfo(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
								m_vertexBufferAlloc	= bindBuffer(vk, getDevice(), getAllocator(), *m_vertexBuffer, MemoryRequirement::HostVisible);

		{
			const float	step = 2.0f / static_cast<float>(GRID_SIZE);

			generateGrid(m_vertexBufferAlloc->getHostPtr(), step, -1.0f, -1.0f, GRID_SIZE, GRID_SIZE);

			flushMappedMemoryRange(vk, getDevice(), m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset(), vertexBufferSize);
		}

		// Sparse index buffer
		for (deUint32 chunkNdx = 0u; chunkNdx < 2; ++chunkNdx)
		{
			deUint8* const	pData		= static_cast<deUint8*>(m_stagingBufferAlloc->getHostPtr()) + chunkNdx * m_perDrawBufferOffset;
			deUint32* const	pIndexData	= reinterpret_cast<deUint32*>(pData);
			const deUint32	ndxBase		= chunkNdx * m_halfVertexCount;

			for (deUint32 i = 0u; i < m_halfVertexCount; ++i)
				pIndexData[i] = ndxBase + i;
		}
	}

private:
	const deUint32			m_halfVertexCount;
	Move<VkBuffer>			m_vertexBuffer;
	MovePtr<Allocation>		m_vertexBufferAlloc;
};

//! Draw from a sparse indirect buffer
class IndirectBufferTestInstance : public DrawGridTestInstance
{
public:
	IndirectBufferTestInstance (Context& context, const TestFlags flags)
		: DrawGridTestInstance	(context,
								 flags,
								 VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
								 sizeof(VkDrawIndirectCommand))
	{
	}

	void rendererDraw (const VkPipelineLayout pipelineLayout, const VkCommandBuffer cmdBuffer) const
	{
		DE_UNREF(pipelineLayout);

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Drawing two triangles covering the whole viewport. There should be no red pixels visible." << tcu::TestLog::EndMessage;

		const DeviceInterface&	vk				= getDeviceInterface();
		const VkDeviceSize		vertexOffset	= 0ull;
		VkDeviceSize			indirectOffset	= 0ull;

		vk.cmdBindVertexBuffers	(cmdBuffer, 0u, 1u, &m_vertexBuffer.get(), &vertexOffset);
		vk.cmdDrawIndirect		(cmdBuffer, *m_sparseBuffer, indirectOffset, 1u, 0u);

		indirectOffset += m_perDrawBufferOffset * (m_residency ? 2 : 1);

		vk.cmdDrawIndirect		(cmdBuffer, *m_sparseBuffer, indirectOffset, 1u, 0u);
	}

	void initializeBuffers (void)
	{
		// Vertex buffer
		const DeviceInterface&	vk					= getDeviceInterface();
		const VkDeviceSize		vertexBufferSize	= 2 * 3 * sizeof(Vec4);
								m_vertexBuffer		= makeBuffer(vk, getDevice(), makeBufferCreateInfo(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
								m_vertexBufferAlloc	= bindBuffer(vk, getDevice(), getAllocator(), *m_vertexBuffer, MemoryRequirement::HostVisible);

		{
			generateGrid(m_vertexBufferAlloc->getHostPtr(), 2.0f, -1.0f, -1.0f, 1, 1);
			flushMappedMemoryRange(vk, getDevice(), m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset(), vertexBufferSize);
		}

		// Indirect buffer
		for (deUint32 chunkNdx = 0u; chunkNdx < 2; ++chunkNdx)
		{
			deUint8* const					pData		= static_cast<deUint8*>(m_stagingBufferAlloc->getHostPtr()) + chunkNdx * m_perDrawBufferOffset;
			VkDrawIndirectCommand* const	pCmdData	= reinterpret_cast<VkDrawIndirectCommand*>(pData);

			pCmdData->firstVertex	= 3u * chunkNdx;
			pCmdData->firstInstance	= 0u;
			pCmdData->vertexCount	= 3u;
			pCmdData->instanceCount	= 1u;
		}
	}

private:
	Move<VkBuffer>			m_vertexBuffer;
	MovePtr<Allocation>		m_vertexBufferAlloc;
};

//! Similar to the class in vktTestCaseUtil.hpp, but uses Arg0 directly rather than through a InstanceFunction1
template<typename Arg0>
class FunctionProgramsSimple1
{
public:
	typedef void	(*Function)				(vk::SourceCollections& dst, Arg0 arg0);
					FunctionProgramsSimple1	(Function func) : m_func(func)							{}
	void			init					(vk::SourceCollections& dst, const Arg0& arg0) const	{ m_func(dst, arg0); }

private:
	const Function	m_func;
};

//! Convenience function to create a TestCase based on a freestanding initPrograms and a TestInstance implementation
template<typename TestInstanceT, typename Arg0>
TestCase* createTestInstanceWithPrograms (tcu::TestContext&									testCtx,
										  const std::string&								name,
										  const std::string&								desc,
										  typename FunctionProgramsSimple1<Arg0>::Function	initPrograms,
										  Arg0												arg0)
{
	return new InstanceFactory1<TestInstanceT, Arg0, FunctionProgramsSimple1<Arg0> >(
		testCtx, tcu::NODETYPE_SELF_VALIDATE, name, desc, FunctionProgramsSimple1<Arg0>(initPrograms), arg0);
}

void populateTestGroup (tcu::TestCaseGroup* parentGroup)
{
	const struct
	{
		std::string		name;
		TestFlags		flags;
	} groups[] =
	{
		{ "sparse_binding",							0u														},
		{ "sparse_binding_aliased",					TEST_FLAG_ALIASED,										},
		{ "sparse_residency",						TEST_FLAG_RESIDENCY,									},
		{ "sparse_residency_aliased",				TEST_FLAG_RESIDENCY | TEST_FLAG_ALIASED,				},
		{ "sparse_residency_non_resident_strict",	TEST_FLAG_RESIDENCY | TEST_FLAG_NON_RESIDENT_STRICT,	},
	};

	const int numGroupsIncludingNonResidentStrict	= DE_LENGTH_OF_ARRAY(groups);
	const int numGroupsDefaultList					= numGroupsIncludingNonResidentStrict - 1;

	// Transfer
	{
		MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(parentGroup->getTestContext(), "transfer", ""));
		{
			MovePtr<tcu::TestCaseGroup> subGroup(new tcu::TestCaseGroup(parentGroup->getTestContext(), "sparse_binding", ""));
			addBufferSparseBindingTests(subGroup.get());
			group->addChild(subGroup.release());
		}
		parentGroup->addChild(group.release());
	}

	// SSBO
	{
		MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(parentGroup->getTestContext(), "ssbo", ""));
		{
			MovePtr<tcu::TestCaseGroup> subGroup(new tcu::TestCaseGroup(parentGroup->getTestContext(), "sparse_binding_aliased", ""));
			addBufferSparseMemoryAliasingTests(subGroup.get());
			group->addChild(subGroup.release());
		}
		{
			MovePtr<tcu::TestCaseGroup> subGroup(new tcu::TestCaseGroup(parentGroup->getTestContext(), "sparse_residency", ""));
			addBufferSparseResidencyTests(subGroup.get());
			group->addChild(subGroup.release());
		}
		parentGroup->addChild(group.release());
	}

	// UBO
	{
		MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(parentGroup->getTestContext(), "ubo", ""));

		for (int groupNdx = 0u; groupNdx < numGroupsIncludingNonResidentStrict; ++groupNdx)
			group->addChild(createTestInstanceWithPrograms<UBOTestInstance>(group->getTestContext(), groups[groupNdx].name.c_str(), "", initProgramsDrawWithUBO, groups[groupNdx].flags));

		parentGroup->addChild(group.release());
	}

	// Vertex buffer
	{
		MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(parentGroup->getTestContext(), "vertex_buffer", ""));

		for (int groupNdx = 0u; groupNdx < numGroupsDefaultList; ++groupNdx)
			group->addChild(createTestInstanceWithPrograms<VertexBufferTestInstance>(group->getTestContext(), groups[groupNdx].name.c_str(), "", initProgramsDrawGrid, groups[groupNdx].flags));

		parentGroup->addChild(group.release());
	}

	// Index buffer
	{
		MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(parentGroup->getTestContext(), "index_buffer", ""));

		for (int groupNdx = 0u; groupNdx < numGroupsDefaultList; ++groupNdx)
			group->addChild(createTestInstanceWithPrograms<IndexBufferTestInstance>(group->getTestContext(), groups[groupNdx].name.c_str(), "", initProgramsDrawGrid, groups[groupNdx].flags));

		parentGroup->addChild(group.release());
	}

	// Indirect buffer
	{
		MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(parentGroup->getTestContext(), "indirect_buffer", ""));

		for (int groupNdx = 0u; groupNdx < numGroupsDefaultList; ++groupNdx)
			group->addChild(createTestInstanceWithPrograms<IndirectBufferTestInstance>(group->getTestContext(), groups[groupNdx].name.c_str(), "", initProgramsDrawGrid, groups[groupNdx].flags));

		parentGroup->addChild(group.release());
	}
}

} // anonymous ns

tcu::TestCaseGroup* createSparseBufferTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "buffer", "Sparse buffer usage tests", populateTestGroup);
}

} // sparse
} // vkt
