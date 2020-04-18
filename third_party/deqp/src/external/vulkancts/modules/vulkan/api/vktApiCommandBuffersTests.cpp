/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Copyright (c) 2015 Google Inc.
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
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkPlatform.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkDeviceUtil.hpp"
#include "tcuTextureUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkPrograms.hpp"
#include "vkTypeUtil.hpp"
#include "vkAllocationCallbackUtil.hpp"
#include "vktApiCommandBuffersTests.hpp"
#include "vktApiBufferComputeInstance.hpp"
#include "vktApiComputeInstanceResultBuffer.hpp"
#include "deSharedPtr.hpp"
#include <sstream>

namespace vkt
{
namespace api
{
namespace
{

using namespace vk;

typedef de::SharedPtr<vk::Unique<vk::VkEvent> >	VkEventSp;

// Global variables
const deUint64								INFINITE_TIMEOUT		= ~(deUint64)0u;


template <deUint32 NumBuffers>
class CommandBufferBareTestEnvironment
{
public:
											CommandBufferBareTestEnvironment	(Context&						context,
																				 VkCommandPoolCreateFlags		commandPoolCreateFlags);

	VkCommandPool							getCommandPool						(void) const					{ return *m_commandPool; }
	VkCommandBuffer							getCommandBuffer					(deUint32 bufferIndex) const;

protected:
	Context&								m_context;
	const VkDevice							m_device;
	const DeviceInterface&					m_vkd;
	const VkQueue							m_queue;
	const deUint32							m_queueFamilyIndex;
	Allocator&								m_allocator;

	// \note All VkCommandBuffers are allocated from m_commandPool so there is no need
	//       to free them separately as the auto-generated dtor will do that through
	//       destroying the pool.
	Move<VkCommandPool>						m_commandPool;
	VkCommandBuffer							m_primaryCommandBuffers[NumBuffers];
};

template <deUint32 NumBuffers>
CommandBufferBareTestEnvironment<NumBuffers>::CommandBufferBareTestEnvironment(Context& context, VkCommandPoolCreateFlags commandPoolCreateFlags)
	: m_context								(context)
	, m_device								(context.getDevice())
	, m_vkd									(context.getDeviceInterface())
	, m_queue								(context.getUniversalQueue())
	, m_queueFamilyIndex					(context.getUniversalQueueFamilyIndex())
	, m_allocator							(context.getDefaultAllocator())
{
	m_commandPool = createCommandPool(m_vkd, m_device, commandPoolCreateFlags, m_queueFamilyIndex);

	const VkCommandBufferAllocateInfo		cmdBufferAllocateInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// VkStructureType             sType;
		DE_NULL,													// const void*                 pNext;
		*m_commandPool,												// VkCommandPool               commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// VkCommandBufferLevel        level;
		NumBuffers												// deUint32                    commandBufferCount;
	};

	VK_CHECK(m_vkd.allocateCommandBuffers(m_device, &cmdBufferAllocateInfo, m_primaryCommandBuffers));
}

template <deUint32 NumBuffers>
VkCommandBuffer CommandBufferBareTestEnvironment<NumBuffers>::getCommandBuffer(deUint32 bufferIndex) const
{
	DE_ASSERT(bufferIndex < NumBuffers);
	return m_primaryCommandBuffers[bufferIndex];
}

class CommandBufferRenderPassTestEnvironment : public CommandBufferBareTestEnvironment<1>
{
public:
											CommandBufferRenderPassTestEnvironment	(Context&						context,
																					 VkCommandPoolCreateFlags		commandPoolCreateFlags);

	VkRenderPass							getRenderPass							(void) const { return *m_renderPass; }
	VkFramebuffer							getFrameBuffer							(void) const { return *m_frameBuffer; }
	VkCommandBuffer							getPrimaryCommandBuffer					(void) const { return getCommandBuffer(0); }
	VkCommandBuffer							getSecondaryCommandBuffer				(void) const { return *m_secondaryCommandBuffer; }

	void									beginPrimaryCommandBuffer				(VkCommandBufferUsageFlags usageFlags);
	void									beginSecondaryCommandBuffer				(VkCommandBufferUsageFlags usageFlags);
	void									beginRenderPass							(VkSubpassContents content);
	void									submitPrimaryCommandBuffer				(void);
	de::MovePtr<tcu::TextureLevel>			readColorAttachment						(void);

	static const VkImageType				DEFAULT_IMAGE_TYPE;
	static const VkFormat					DEFAULT_IMAGE_FORMAT;
	static const VkExtent3D					DEFAULT_IMAGE_SIZE;
	static const VkRect2D					DEFAULT_IMAGE_AREA;

protected:

	Move<VkImage>							m_colorImage;
	Move<VkImageView>						m_colorImageView;
	Move<VkRenderPass>						m_renderPass;
	Move<VkFramebuffer>						m_frameBuffer;
	de::MovePtr<Allocation>					m_colorImageMemory;
	Move<VkCommandBuffer>					m_secondaryCommandBuffer;

};

const VkImageType		CommandBufferRenderPassTestEnvironment::DEFAULT_IMAGE_TYPE		= VK_IMAGE_TYPE_2D;
const VkFormat			CommandBufferRenderPassTestEnvironment::DEFAULT_IMAGE_FORMAT	= VK_FORMAT_R8G8B8A8_UINT;
const VkExtent3D		CommandBufferRenderPassTestEnvironment::DEFAULT_IMAGE_SIZE		= {255, 255, 1};
const VkRect2D			CommandBufferRenderPassTestEnvironment::DEFAULT_IMAGE_AREA		=
{
	{ 0u, 0u, },												//	VkOffset2D	offset;
	{ DEFAULT_IMAGE_SIZE.width,	DEFAULT_IMAGE_SIZE.height },	//	VkExtent2D	extent;
};

CommandBufferRenderPassTestEnvironment::CommandBufferRenderPassTestEnvironment(Context& context, VkCommandPoolCreateFlags commandPoolCreateFlags)
	: CommandBufferBareTestEnvironment<1>		(context, commandPoolCreateFlags)
{
	{
		const VkAttachmentDescription			colorAttDesc			=
		{
			0u,													// VkAttachmentDescriptionFlags		flags;
			DEFAULT_IMAGE_FORMAT,								// VkFormat							format;
			VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
			VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout					initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					finalLayout;
		};

		const VkAttachmentDescription			attachments[1]			=
		{
			colorAttDesc
		};

		const VkAttachmentReference				colorAttRef				=
		{
			0u,													// deUint32							attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					layout;
		};

		const VkSubpassDescription				subpassDesc[1]			=
		{
			{
				0u,												// VkSubpassDescriptionFlags		flags;
				VK_PIPELINE_BIND_POINT_GRAPHICS,				// VkPipelineBindPoint				pipelineBindPoint;
				0u,												// deUint32							inputAttachmentCount;
				DE_NULL,										// const VkAttachmentReference*		pInputAttachments;
				1u,												// deUint32							colorAttachmentCount;
				&colorAttRef,									// const VkAttachmentReference*		pColorAttachments;
				DE_NULL,										// const VkAttachmentReference*		pResolveAttachments;
				DE_NULL,										// const VkAttachmentReference*		depthStencilAttachment;
				0u,												// deUint32							preserveAttachmentCount;
				DE_NULL,										// const VkAttachmentReference*		pPreserveAttachments;
			}
		};

		const VkRenderPassCreateInfo			renderPassCreateInfo	=
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkRenderPassCreateFlags			flags;
			1u,													// deUint32							attachmentCount;
			attachments,										// const VkAttachmentDescription*	pAttachments;
			1u,													// deUint32							subpassCount;
			subpassDesc,										// const VkSubpassDescription*		pSubpasses;
			0u,													// deUint32							dependencyCount;
			DE_NULL,											// const VkSubpassDependency*		pDependencies;
		};

		m_renderPass = createRenderPass(m_vkd, m_device, &renderPassCreateInfo, DE_NULL);
	}

	{
		const VkImageCreateInfo					imageCreateInfo			=
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,		// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			0u,											// VkImageCreateFlags		flags;
			DEFAULT_IMAGE_TYPE,							// VkImageType				imageType;
			DEFAULT_IMAGE_FORMAT,						// VkFormat					format;
			DEFAULT_IMAGE_SIZE,							// VkExtent3D				extent;
			1,											// deUint32					mipLevels;
			1,											// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,					// VkImageTiling			tiling;
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,			// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode			sharingMode;
			1,											// deUint32					queueFamilyIndexCount;
			&m_queueFamilyIndex,						// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED					// VkImageLayout			initialLayout;
		};

		m_colorImage = createImage(m_vkd, m_device, &imageCreateInfo, DE_NULL);
	}

	m_colorImageMemory = m_allocator.allocate(getImageMemoryRequirements(m_vkd, m_device, *m_colorImage), MemoryRequirement::Any);
	VK_CHECK(m_vkd.bindImageMemory(m_device, *m_colorImage, m_colorImageMemory->getMemory(), m_colorImageMemory->getOffset()));

	{
		const VkImageViewCreateInfo				imageViewCreateInfo		=
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType				sType;
			DE_NULL,									// const void*					pNext;
			0u,											// VkImageViewCreateFlags		flags;
			*m_colorImage,								// VkImage						image;
			VK_IMAGE_VIEW_TYPE_2D,						// VkImageViewType				viewType;
			DEFAULT_IMAGE_FORMAT,						// VkFormat						format;
			{
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			},											// VkComponentMapping			components;
			{
				VK_IMAGE_ASPECT_COLOR_BIT,					// VkImageAspectFlags			aspectMask;
				0u,											// deUint32						baseMipLevel;
				1u,											// deUint32						mipLevels;
				0u,											// deUint32						baseArrayLayer;
				1u,											// deUint32						arraySize;
			},											// VkImageSubresourceRange		subresourceRange;
		};

		m_colorImageView = createImageView(m_vkd, m_device, &imageViewCreateInfo, DE_NULL);
	}

	{
		const VkImageView						attachmentViews[1]		=
		{
			*m_colorImageView
		};

		const VkFramebufferCreateInfo			framebufferCreateInfo	=
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			0u,											// VkFramebufferCreateFlags	flags;
			*m_renderPass,								// VkRenderPass				renderPass;
			1,											// deUint32					attachmentCount;
			attachmentViews,							// const VkImageView*		pAttachments;
			DEFAULT_IMAGE_SIZE.width,					// deUint32					width;
			DEFAULT_IMAGE_SIZE.height,					// deUint32					height;
			1u,											// deUint32					layers;
		};

		m_frameBuffer = createFramebuffer(m_vkd, m_device, &framebufferCreateInfo, DE_NULL);
	}

	{
		const VkCommandBufferAllocateInfo		cmdBufferAllocateInfo	=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// VkStructureType             sType;
			DE_NULL,													// const void*                 pNext;
			*m_commandPool,												// VkCommandPool               commandPool;
			VK_COMMAND_BUFFER_LEVEL_SECONDARY,							// VkCommandBufferLevel        level;
			1u															// deUint32                    commandBufferCount;
		};

		m_secondaryCommandBuffer = allocateCommandBuffer(m_vkd, m_device, &cmdBufferAllocateInfo);

	}
}

void CommandBufferRenderPassTestEnvironment::beginRenderPass(VkSubpassContents content)
{
	const VkClearValue						clearValues[1]			=
	{
		makeClearValueColorU32(17, 59, 163, 251),
	};

	const VkRenderPassBeginInfo				renderPassBeginInfo		=
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,				// VkStructureType		sType;
		DE_NULL,												// const void*			pNext;
		*m_renderPass,											// VkRenderPass			renderPass;
		*m_frameBuffer,											// VkFramebuffer		framebuffer;
		DEFAULT_IMAGE_AREA,										// VkRect2D				renderArea;
		1u,														// deUint32				clearValueCount;
		clearValues												// const VkClearValue*	pClearValues;
	};

	m_vkd.cmdBeginRenderPass(m_primaryCommandBuffers[0], &renderPassBeginInfo, content);
}

void CommandBufferRenderPassTestEnvironment::beginPrimaryCommandBuffer(VkCommandBufferUsageFlags usageFlags)
{
	const VkCommandBufferBeginInfo			commandBufferBeginInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,			// VkStructureType                          sType;
		DE_NULL,												// const void*                              pNext;
		usageFlags,												// VkCommandBufferUsageFlags                flags;
		DE_NULL													// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
	};

	VK_CHECK(m_vkd.beginCommandBuffer(m_primaryCommandBuffers[0], &commandBufferBeginInfo));

}


void CommandBufferRenderPassTestEnvironment::beginSecondaryCommandBuffer(VkCommandBufferUsageFlags usageFlags)
{
	const VkCommandBufferInheritanceInfo	commandBufferInheritanceInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,		// VkStructureType                  sType;
		DE_NULL,												// const void*                      pNext;
		*m_renderPass,											// VkRenderPass                     renderPass;
		0u,														// deUint32                         subpass;
		*m_frameBuffer,											// VkFramebuffer                    framebuffer;
		VK_FALSE,												// VkBool32                         occlusionQueryEnable;
		0u,														// VkQueryControlFlags              queryFlags;
		0u														// VkQueryPipelineStatisticFlags    pipelineStatistics;
	};

	const VkCommandBufferBeginInfo			commandBufferBeginInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,			// VkStructureType                          sType;
		DE_NULL,												// const void*                              pNext;
		usageFlags,												// VkCommandBufferUsageFlags                flags;
		&commandBufferInheritanceInfo							// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
	};

	VK_CHECK(m_vkd.beginCommandBuffer(*m_secondaryCommandBuffer, &commandBufferBeginInfo));

}

void CommandBufferRenderPassTestEnvironment::submitPrimaryCommandBuffer(void)
{
	const Unique<VkFence>					fence					(createFence(m_vkd, m_device));
	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,							// VkStructureType                sType;
		DE_NULL,												// const void*                    pNext;
		0u,														// deUint32                       waitSemaphoreCount;
		DE_NULL,												// const VkSemaphore*             pWaitSemaphores;
		DE_NULL,												// const VkPipelineStageFlags*    pWaitDstStageMask;
		1u,														// deUint32                       commandBufferCount;
		m_primaryCommandBuffers,								// const VkCommandBuffer*         pCommandBuffers;
		0u,														// deUint32                       signalSemaphoreCount;
		DE_NULL													// const VkSemaphore*             pSignalSemaphores;
	};

	VK_CHECK(m_vkd.queueSubmit(m_queue, 1, &submitInfo, *fence));

	VK_CHECK(m_vkd.waitForFences(m_device, 1, &fence.get(), VK_TRUE, ~0ull));

}

de::MovePtr<tcu::TextureLevel> CommandBufferRenderPassTestEnvironment::readColorAttachment ()
{
	Move<VkBuffer>					buffer;
	de::MovePtr<Allocation>			bufferAlloc;
	const tcu::TextureFormat		tcuFormat		= mapVkFormat(DEFAULT_IMAGE_FORMAT);
	const VkDeviceSize				pixelDataSize	= DEFAULT_IMAGE_SIZE.height * DEFAULT_IMAGE_SIZE.height * tcuFormat.getPixelSize();
	de::MovePtr<tcu::TextureLevel>	resultLevel		(new tcu::TextureLevel(tcuFormat, DEFAULT_IMAGE_SIZE.width, DEFAULT_IMAGE_SIZE.height));

	// Create destination buffer
	{
		const VkBufferCreateInfo bufferParams =
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			pixelDataSize,								// VkDeviceSize			size;
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			0u,											// deUint32				queueFamilyIndexCount;
			DE_NULL										// const deUint32*		pQueueFamilyIndices;
		};

		buffer		= createBuffer(m_vkd, m_device, &bufferParams);
		bufferAlloc = m_allocator.allocate(getBufferMemoryRequirements(m_vkd, m_device, *buffer), MemoryRequirement::HostVisible);
		VK_CHECK(m_vkd.bindBufferMemory(m_device, *buffer, bufferAlloc->getMemory(), bufferAlloc->getOffset()));
	}

	// Barriers for copying image to buffer

	const VkImageMemoryBarrier imageBarrier =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,		// VkAccessFlags			srcAccessMask;
		VK_ACCESS_TRANSFER_READ_BIT,				// VkAccessFlags			dstAccessMask;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout			oldLayout;
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					dstQueueFamilyIndex;
		*m_colorImage,								// VkImage					image;
		{											// VkImageSubresourceRange	subresourceRange;
			VK_IMAGE_ASPECT_COLOR_BIT,	// VkImageAspectFlags	aspectMask;
			0u,							// deUint32				baseMipLevel;
			1u,							// deUint32				mipLevels;
			0u,							// deUint32				baseArraySlice;
			1u							// deUint32				arraySize;
		}
	};

	const VkBufferMemoryBarrier bufferBarrier =
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
		DE_NULL,									// const void*		pNext;
		VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags	srcAccessMask;
		VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags	dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			dstQueueFamilyIndex;
		*buffer,									// VkBuffer			buffer;
		0u,											// VkDeviceSize		offset;
		pixelDataSize								// VkDeviceSize		size;
	};

	// Copy image to buffer

	const VkBufferImageCopy copyRegion =
	{
		0u,												// VkDeviceSize				bufferOffset;
		DEFAULT_IMAGE_SIZE.width,						// deUint32					bufferRowLength;
		DEFAULT_IMAGE_SIZE.height,						// deUint32					bufferImageHeight;
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u },		// VkImageSubresourceLayers	imageSubresource;
		{ 0, 0, 0 },									// VkOffset3D				imageOffset;
		DEFAULT_IMAGE_SIZE								// VkExtent3D				imageExtent;
	};

	beginPrimaryCommandBuffer(0);
	m_vkd.cmdPipelineBarrier(m_primaryCommandBuffers[0], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &imageBarrier);
	m_vkd.cmdCopyImageToBuffer(m_primaryCommandBuffers[0], *m_colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *buffer, 1, &copyRegion);
	m_vkd.cmdPipelineBarrier(m_primaryCommandBuffers[0], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &bufferBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
	VK_CHECK(m_vkd.endCommandBuffer(m_primaryCommandBuffers[0]));

	submitPrimaryCommandBuffer();

	// Read buffer data
	invalidateMappedMemoryRange(m_vkd, m_device, bufferAlloc->getMemory(), bufferAlloc->getOffset(), pixelDataSize);
	tcu::copy(*resultLevel, tcu::ConstPixelBufferAccess(resultLevel->getFormat(), resultLevel->getSize(), bufferAlloc->getHostPtr()));

	return resultLevel;
}


// Testcases
/********* 19.1. Command Pools (5.1 in VK 1.0 Spec) ***************************/
tcu::TestStatus createPoolNullParamsTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	createCommandPool(vk, vkDevice, 0u, queueFamilyIndex);

	return tcu::TestStatus::pass("Command Pool allocated correctly.");
}

tcu::TestStatus createPoolNonNullAllocatorTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	const VkAllocationCallbacks*			allocationCallbacks		= getSystemAllocator();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		0u,															// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};

	createCommandPool(vk, vkDevice, &cmdPoolParams, allocationCallbacks);

	return tcu::TestStatus::pass("Command Pool allocated correctly.");
}

tcu::TestStatus createPoolTransientBitTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,						// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};

	createCommandPool(vk, vkDevice, &cmdPoolParams, DE_NULL);

	return tcu::TestStatus::pass("Command Pool allocated correctly.");
}

tcu::TestStatus createPoolResetBitTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};

	createCommandPool(vk, vkDevice, &cmdPoolParams, DE_NULL);

	return tcu::TestStatus::pass("Command Pool allocated correctly.");
}

tcu::TestStatus resetPoolReleaseResourcesBitTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		0u,															// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};

	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams, DE_NULL));

	VK_CHECK(vk.resetCommandPool(vkDevice, *cmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));

	return tcu::TestStatus::pass("Command Pool allocated correctly.");
}

tcu::TestStatus resetPoolNoFlagsTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		0u,															// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};

	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams, DE_NULL));

	VK_CHECK(vk.resetCommandPool(vkDevice, *cmdPool, 0u));

	return tcu::TestStatus::pass("Command Pool allocated correctly.");
}

bool executeCommandBuffer (const VkDevice			device,
						   const DeviceInterface&	vk,
						   const VkQueue			queue,
						   const VkCommandBuffer	commandBuffer,
						   const bool				exitBeforeEndCommandBuffer = false)
{
	const Unique<VkEvent>			event					(createEvent(vk, device));
	const VkCommandBufferBeginInfo	commandBufferBeginInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	//VkStructureType						sType;
		DE_NULL,										//const void*							pNext;
		0u,												//VkCommandBufferUsageFlags				flags;
		(const VkCommandBufferInheritanceInfo*)DE_NULL	//const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
	};

	VK_CHECK(vk.beginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
	{
		const VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		vk.cmdSetEvent(commandBuffer, *event, stageMask);
		if (exitBeforeEndCommandBuffer)
			return exitBeforeEndCommandBuffer;
	}
	VK_CHECK(vk.endCommandBuffer(commandBuffer));

	{
		const Unique<VkFence>					fence			(createFence(vk, device));
		const VkSubmitInfo						submitInfo		=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,			// sType
			DE_NULL,								// pNext
			0u,										// waitSemaphoreCount
			DE_NULL,								// pWaitSemaphores
			(const VkPipelineStageFlags*)DE_NULL,	// pWaitDstStageMask
			1u,										// commandBufferCount
			&commandBuffer,							// pCommandBuffers
			0u,										// signalSemaphoreCount
			DE_NULL									// pSignalSemaphores
		};

		// Submit the command buffer to the queue
		VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
		// wait for end of execution of queue
		VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), 0u, INFINITE_TIMEOUT));
	}
	// check if buffer has been executed
	const VkResult result = vk.getEventStatus(device, *event);
	return result == VK_EVENT_SET;
}

tcu::TestStatus resetPoolReuseTest (Context& context)
{
	const VkDevice						vkDevice			= context.getDevice();
	const DeviceInterface&				vk					= context.getDeviceInterface();
	const deUint32						queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const VkQueue						queue				= context.getUniversalQueue();

	const VkCommandPoolCreateInfo		cmdPoolParams		=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,	// sType;
		DE_NULL,									// pNext;
		0u,											// flags;
		queueFamilyIndex							// queueFamilyIndex;
	};
	const Unique<VkCommandPool>			cmdPool				(createCommandPool(vk, vkDevice, &cmdPoolParams, DE_NULL));
	const VkCommandBufferAllocateInfo	cmdBufParams		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// sType;
		DE_NULL,										// pNext;
		*cmdPool,										// commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,				// level;
		1u												// bufferCount;
	};
	const Move<VkCommandBuffer>			commandBuffers[]	=
	{
		allocateCommandBuffer(vk, vkDevice, &cmdBufParams),
		allocateCommandBuffer(vk, vkDevice, &cmdBufParams)
	};

	if (!executeCommandBuffer(vkDevice, vk, queue, *(commandBuffers[0])))
		return tcu::TestStatus::fail("Failed");
	if (!executeCommandBuffer(vkDevice, vk, queue, *(commandBuffers[1]), true))
		return tcu::TestStatus::fail("Failed");

	VK_CHECK(vk.resetCommandPool(vkDevice, *cmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));

	if (!executeCommandBuffer(vkDevice, vk, queue, *(commandBuffers[0])))
		return tcu::TestStatus::fail("Failed");
	if (!executeCommandBuffer(vkDevice, vk, queue, *(commandBuffers[1])))
		return tcu::TestStatus::fail("Failed");

	{
		const Unique<VkCommandBuffer> afterResetCommandBuffers(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
		if (!executeCommandBuffer(vkDevice, vk, queue, *afterResetCommandBuffers))
			return tcu::TestStatus::fail("Failed");
	}

	return tcu::TestStatus::pass("Passed");
}

/******** 19.2. Command Buffer Lifetime (5.2 in VK 1.0 Spec) ******************/
tcu::TestStatus allocatePrimaryBufferTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level;
		1u,															// bufferCount;
	};
	const Unique<VkCommandBuffer>			cmdBuf					(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	return tcu::TestStatus::pass("Buffer was created correctly.");
}

tcu::TestStatus allocateManyPrimaryBuffersTest(Context& context)
{

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// \todo Determining the minimum number of command buffers should be a function of available system memory and driver capabilities.
#if (DE_PTR_SIZE == 4)
	const unsigned minCommandBuffer = 1024;
#else
	const unsigned minCommandBuffer = 10000;
#endif

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		minCommandBuffer,											//	uint32_t					bufferCount;
	};

	// do not keep the handles to buffers, as they will be freed with command pool

	// allocate the minimum required amount of buffers
	VkCommandBuffer cmdBuffers[minCommandBuffer];
	VK_CHECK(vk.allocateCommandBuffers(vkDevice, &cmdBufParams, cmdBuffers));

	std::ostringstream out;
	out << "allocateManyPrimaryBuffersTest succeded: created " << minCommandBuffer << " command buffers";

	return tcu::TestStatus::pass(out.str());
}

tcu::TestStatus allocateSecondaryBufferTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// commandPool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							// level;
		1u,															// bufferCount;
	};
	const Unique<VkCommandBuffer>			cmdBuf					(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	return tcu::TestStatus::pass("Buffer was created correctly.");
}

tcu::TestStatus allocateManySecondaryBuffersTest(Context& context)
{

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// \todo Determining the minimum number of command buffers should be a function of available system memory and driver capabilities.
#if (DE_PTR_SIZE == 4)
	const unsigned minCommandBuffer = 1024;
#else
	const unsigned minCommandBuffer = 10000;
#endif

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							//	VkCommandBufferLevel		level;
		minCommandBuffer,											//	uint32_t					bufferCount;
	};

	// do not keep the handles to buffers, as they will be freed with command pool

	// allocate the minimum required amount of buffers
	VkCommandBuffer cmdBuffers[minCommandBuffer];
	VK_CHECK(vk.allocateCommandBuffers(vkDevice, &cmdBufParams, cmdBuffers));

	std::ostringstream out;
	out << "allocateManySecondaryBuffersTest succeded: created " << minCommandBuffer << " command buffers";

	return tcu::TestStatus::pass(out.str());
}

tcu::TestStatus executePrimaryBufferTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// record setting event
		vk.cmdSetEvent(*primCmdBuf, *event,stageMask);
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// Submit the command buffer to the queue
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// check if buffer has been executed
	VkResult result = vk.getEventStatus(vkDevice,*event);
	if (result == VK_EVENT_SET)
		return tcu::TestStatus::pass("Execute Primary Command Buffer succeeded");

	return tcu::TestStatus::fail("Execute Primary Command Buffer FAILED");
}

tcu::TestStatus executeLargePrimaryBufferTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	const deUint32							LARGE_BUFFER_SIZE		= 10000;

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	std::vector<VkEventSp>					events;
	for (deUint32 ndx = 0; ndx < LARGE_BUFFER_SIZE; ++ndx)
		events.push_back(VkEventSp(new vk::Unique<VkEvent>(createEvent(vk, vkDevice))));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// set all the events
		for (deUint32 ndx = 0; ndx < LARGE_BUFFER_SIZE; ++ndx)
		{
			vk.cmdSetEvent(*primCmdBuf, events[ndx]->get(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// Submit the command buffer to the queue
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// check if the buffer was executed correctly - all events had their status
	// changed
	tcu::TestStatus testResult = tcu::TestStatus::incomplete();

	for (deUint32 ndx = 0; ndx < LARGE_BUFFER_SIZE; ++ndx)
	{
		if (vk.getEventStatus(vkDevice, events[ndx]->get()) != VK_EVENT_SET)
		{
			testResult = tcu::TestStatus::fail("An event was not set.");
			break;
		}
	}

	if (!testResult.isComplete())
		testResult = tcu::TestStatus::pass("All events set correctly.");

	return testResult;
}

tcu::TestStatus resetBufferImplicitlyTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level;
		1u,															// bufferCount;
	};
	const Unique<VkCommandBuffer>			cmdBuf						(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	const VkCommandBufferBeginInfo			cmdBufBeginInfo			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// Put the command buffer in recording state.
	VK_CHECK(vk.beginCommandBuffer(*cmdBuf, &cmdBufBeginInfo));
	{
		// Set the event
		vk.cmdSetEvent(*cmdBuf, *event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}
	VK_CHECK(vk.endCommandBuffer(*cmdBuf));

	// We'll use a fence to wait for the execution of the queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1u,															// commandBufferCount
		&cmdBuf.get(),												// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// Submitting the command buffer that sets the event to the queue
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, fence.get()));

	// Waiting for the queue to finish executing
	VK_CHECK(vk.waitForFences(vkDevice, 1u, &fence.get(), 0u, INFINITE_TIMEOUT));
	// Reset the fence so that we can reuse it
	VK_CHECK(vk.resetFences(vkDevice, 1u, &fence.get()));

	// Check if the buffer was executed
	if (vk.getEventStatus(vkDevice, *event) != VK_EVENT_SET)
		return tcu::TestStatus::fail("Failed to set the event.");

	// Reset the event
	vk.resetEvent(vkDevice, *event);
	if(vk.getEventStatus(vkDevice, *event) != VK_EVENT_RESET)
		return tcu::TestStatus::fail("Failed to reset the event.");

	// Reset the command buffer by putting it in recording state again. This
	// should empty the command buffer.
	VK_CHECK(vk.beginCommandBuffer(*cmdBuf, &cmdBufBeginInfo));
	VK_CHECK(vk.endCommandBuffer(*cmdBuf));

	// Submit the command buffer after resetting. It should have no commands
	// recorded, so the event should remain unsignaled.
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, fence.get()));
	// Waiting for the queue to finish executing
	VK_CHECK(vk.waitForFences(vkDevice, 1u, &fence.get(), 0u, INFINITE_TIMEOUT));

	// Check if the event remained unset.
	if(vk.getEventStatus(vkDevice, *event) == VK_EVENT_RESET)
		return tcu::TestStatus::pass("Buffer was reset correctly.");
	else
		return tcu::TestStatus::fail("Buffer was not reset correctly.");
}

using  de::SharedPtr;
typedef SharedPtr<Unique<VkEvent> >			VkEventShared;

template<typename T>
inline SharedPtr<Unique<T> > makeSharedPtr (Move<T> move)
{
	return SharedPtr<Unique<T> >(new Unique<T>(move));
}

bool submitAndCheck (Context& context, std::vector<VkCommandBuffer>& cmdBuffers, std::vector <VkEventShared>& events)
{
	const VkDevice						vkDevice	= context.getDevice();
	const DeviceInterface&				vk			= context.getDeviceInterface();
	const VkQueue						queue		= context.getUniversalQueue();
	const Unique<VkFence>				fence		(createFence(vk, vkDevice));

	const VkSubmitInfo					submitInfo	=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,				// sType
		DE_NULL,									// pNext
		0u,											// waitSemaphoreCount
		DE_NULL,									// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,		// pWaitDstStageMask
		static_cast<deUint32>(cmdBuffers.size()),	// commandBufferCount
		&cmdBuffers[0],								// pCommandBuffers
		0u,											// signalSemaphoreCount
		DE_NULL,									// pSignalSemaphores
	};

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, fence.get()));
	VK_CHECK(vk.waitForFences(vkDevice, 1u, &fence.get(), 0u, INFINITE_TIMEOUT));

	for(int eventNdx = 0; eventNdx < static_cast<int>(events.size()); ++eventNdx)
	{
		if (vk.getEventStatus(vkDevice, **events[eventNdx]) != VK_EVENT_SET)
			return false;
		vk.resetEvent(vkDevice, **events[eventNdx]);
	}

	return true;
}

void createCommadBuffers (const DeviceInterface&		vk,
						  const VkDevice				vkDevice,
						  deUint32						bufferCount,
						  VkCommandPool					pool,
						  const VkCommandBufferLevel	cmdBufferLevel,
						  VkCommandBuffer*				pCommandBuffers)
{
	const VkCommandBufferAllocateInfo		cmdBufParams	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	//	VkStructureType				sType;
		DE_NULL,										//	const void*					pNext;
		pool,											//	VkCommandPool				pool;
		cmdBufferLevel,									//	VkCommandBufferLevel		level;
		bufferCount,									//	uint32_t					bufferCount;
	};
	VK_CHECK(vk.allocateCommandBuffers(vkDevice, &cmdBufParams, pCommandBuffers));
}

void addCommandsToBuffer (const DeviceInterface& vk, std::vector<VkCommandBuffer>& cmdBuffers, std::vector <VkEventShared>& events)
{
	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		(VkRenderPass)0u,								// renderPass
		0u,												// subpass
		(VkFramebuffer)0u,								// framebuffer
		VK_FALSE,										// occlusionQueryEnable
		(VkQueryControlFlags)0u,						// queryFlags
		(VkQueryPipelineStatisticFlags)0u,				// pipelineStatistics
	};

	const VkCommandBufferBeginInfo		cmdBufBeginInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType
		DE_NULL,										// pNext
		0u,												// flags
		&secCmdBufInheritInfo,							// pInheritanceInfo;
	};

	for(int bufferNdx = 0; bufferNdx < static_cast<int>(cmdBuffers.size()); ++bufferNdx)
	{
		VK_CHECK(vk.beginCommandBuffer(cmdBuffers[bufferNdx], &cmdBufBeginInfo));
		vk.cmdSetEvent(cmdBuffers[bufferNdx], **events[bufferNdx % events.size()], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		VK_CHECK(vk.endCommandBuffer(cmdBuffers[bufferNdx]));
	}
}

bool executeSecondaryCmdBuffer (Context&						context,
								VkCommandPool					pool,
								std::vector<VkCommandBuffer>&	cmdBuffersSecondary,
								std::vector <VkEventShared>&	events)
{
	const VkDevice					vkDevice		= context.getDevice();
	const DeviceInterface&			vk				= context.getDeviceInterface();
	std::vector<VkCommandBuffer>	cmdBuffer		(1);
	const VkCommandBufferBeginInfo	cmdBufBeginInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType
		DE_NULL,										// pNext
		0u,												// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,	// pInheritanceInfo;
	};

	createCommadBuffers(vk, vkDevice, 1u, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &cmdBuffer[0]);
	VK_CHECK(vk.beginCommandBuffer(cmdBuffer[0], &cmdBufBeginInfo));
	vk.cmdExecuteCommands(cmdBuffer[0], static_cast<deUint32>(cmdBuffersSecondary.size()), &cmdBuffersSecondary[0]);
	VK_CHECK(vk.endCommandBuffer(cmdBuffer[0]));

	bool returnValue = submitAndCheck(context, cmdBuffer, events);
	vk.freeCommandBuffers(vkDevice, pool, 1u, &cmdBuffer[0]);
	return returnValue;
}

tcu::TestStatus trimCommandPoolTest (Context& context, const VkCommandBufferLevel cmdBufferLevel)
{
	if (!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_maintenance1"))
		TCU_THROW(NotSupportedError, "Extension VK_KHR_maintenance1 not supported");

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	//test parameters
	const deUint32							cmdBufferIterationCount	= 300u;
	const deUint32							cmdBufferCount			= 10u;

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	std::vector <VkEventShared>				events;
	for (deUint32 ndx = 0u; ndx < cmdBufferCount; ++ndx)
		events.push_back(makeSharedPtr(createEvent(vk, vkDevice)));

	{
		std::vector<VkCommandBuffer> cmdBuffers(cmdBufferCount);
		createCommadBuffers(vk, vkDevice, cmdBufferCount, *cmdPool, cmdBufferLevel, &cmdBuffers[0]);

		for (deUint32 cmdBufferIterationrNdx = 0; cmdBufferIterationrNdx < cmdBufferIterationCount; ++cmdBufferIterationrNdx)
		{
			addCommandsToBuffer(vk, cmdBuffers, events);

			//Peak, situation when we use a lot more command buffers
			if (cmdBufferIterationrNdx % 10u == 0)
			{
				std::vector<VkCommandBuffer> cmdBuffersPeak(cmdBufferCount * 10u);
				createCommadBuffers(vk, vkDevice, static_cast<deUint32>(cmdBuffersPeak.size()), *cmdPool, cmdBufferLevel, &cmdBuffersPeak[0]);
				addCommandsToBuffer(vk, cmdBuffersPeak, events);

				switch(cmdBufferLevel)
				{
					case VK_COMMAND_BUFFER_LEVEL_PRIMARY:
						if (!submitAndCheck(context, cmdBuffersPeak, events))
							return tcu::TestStatus::fail("Fail");
						break;
					case VK_COMMAND_BUFFER_LEVEL_SECONDARY:
						if (!executeSecondaryCmdBuffer(context, *cmdPool, cmdBuffersPeak, events))
							return tcu::TestStatus::fail("Fail");
						break;
					default:
						DE_ASSERT(0);
				}
				vk.freeCommandBuffers(vkDevice, *cmdPool, static_cast<deUint32>(cmdBuffersPeak.size()), &cmdBuffersPeak[0]);
			}

			vk.trimCommandPoolKHR(vkDevice, *cmdPool, (VkCommandPoolTrimFlagsKHR)0);

			switch(cmdBufferLevel)
			{
				case VK_COMMAND_BUFFER_LEVEL_PRIMARY:
					if (!submitAndCheck(context, cmdBuffers, events))
						return tcu::TestStatus::fail("Fail");
					break;
				case VK_COMMAND_BUFFER_LEVEL_SECONDARY:
					if (!executeSecondaryCmdBuffer(context, *cmdPool, cmdBuffers, events))
						return tcu::TestStatus::fail("Fail");
					break;
				default:
					DE_ASSERT(0);
			}

			for (deUint32 bufferNdx = cmdBufferIterationrNdx % 3u; bufferNdx < cmdBufferCount; bufferNdx+=2u)
			{
				vk.freeCommandBuffers(vkDevice, *cmdPool, 1u, &cmdBuffers[bufferNdx]);
				createCommadBuffers(vk, vkDevice, 1u, *cmdPool, cmdBufferLevel, &cmdBuffers[bufferNdx]);
			}
		}
	}

	return tcu::TestStatus::pass("Pass");
}

/******** 19.3. Command Buffer Recording (5.3 in VK 1.0 Spec) *****************/
tcu::TestStatus recordSinglePrimaryBufferTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// record setting event
		vk.cmdSetEvent(*primCmdBuf, *event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	return tcu::TestStatus::pass("Primary buffer recorded successfully.");
}

tcu::TestStatus recordLargePrimaryBufferTest(Context &context)
{

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// define minimal amount of commands to accept
		const long long unsigned minNumCommands = 10000llu;

		for ( long long unsigned currentCommands = 0; currentCommands < minNumCommands / 2; ++currentCommands )
		{
			// record setting event
			vk.cmdSetEvent(*primCmdBuf, *event,stageMask);

			// record resetting event
			vk.cmdResetEvent(*primCmdBuf, *event,stageMask);
		};

	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// Submit the command buffer to the queue
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	return tcu::TestStatus::pass("hugeTest succeeded");
}

tcu::TestStatus recordSingleSecondaryBufferTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			secCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		(VkRenderPass)0u,											// renderPass
		0u,															// subpass
		(VkFramebuffer)0u,											// framebuffer
		VK_FALSE,													// occlusionQueryEnable
		(VkQueryControlFlags)0u,									// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		&secCmdBufInheritInfo,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*secCmdBuf, &secCmdBufBeginInfo));
	{
		// record setting event
		vk.cmdSetEvent(*secCmdBuf, *event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}
	VK_CHECK(vk.endCommandBuffer(*secCmdBuf));

	return tcu::TestStatus::pass("Secondary buffer recorded successfully.");
}

tcu::TestStatus recordLargeSecondaryBufferTest(Context &context)
{

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// define minimal amount of commands to accept
		const long long unsigned minNumCommands = 10000llu;

		for ( long long unsigned currentCommands = 0; currentCommands < minNumCommands / 2; ++currentCommands )
		{
			// record setting event
			vk.cmdSetEvent(*primCmdBuf, *event,stageMask);

			// record resetting event
			vk.cmdResetEvent(*primCmdBuf, *event,stageMask);
		};


	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// Submit the command buffer to the queue
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	return tcu::TestStatus::pass("hugeTest succeeded");
}

tcu::TestStatus submitPrimaryBufferTwiceTest(Context& context)
{

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// record setting event
		vk.cmdSetEvent(*primCmdBuf, *event,stageMask);
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// submit primary buffer
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));
	VK_CHECK(vk.resetFences(vkDevice, 1u, &fence.get()));
	// check if buffer has been executed
	VkResult result = vk.getEventStatus(vkDevice,*event);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("Submit Twice Test FAILED");

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// check if buffer has been executed
	result = vk.getEventStatus(vkDevice,*event);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("Submit Twice Test FAILED");
	else
		return tcu::TestStatus::pass("Submit Twice Test succeeded");
}

tcu::TestStatus submitSecondaryBufferTwiceTest(Context& context)
{

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};

	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};

	const Unique<VkCommandBuffer>			primCmdBuf1				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const Unique<VkCommandBuffer>			primCmdBuf2				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	// Secondary Command buffer
	const VkCommandBufferAllocateInfo		secCmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			secCmdBuf				(allocateCommandBuffer(vk, vkDevice, &secCmdBufParams));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		(VkRenderPass)0u,											// renderPass
		0u,															// subpass
		(VkFramebuffer)0u,											// framebuffer
		VK_FALSE,													// occlusionQueryEnable
		(VkQueryControlFlags)0u,									// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0u,															// flags
		&secCmdBufInheritInfo,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record first primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf1, &primCmdBufBeginInfo));
	{
		// record secondary command buffer
		VK_CHECK(vk.beginCommandBuffer(*secCmdBuf, &secCmdBufBeginInfo));
		{
			// allow execution of event during every stage of pipeline
			VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// record setting event
			vk.cmdSetEvent(*secCmdBuf, *event,stageMask);
		}

		// end recording of secondary buffers
		VK_CHECK(vk.endCommandBuffer(*secCmdBuf));

		// execute secondary buffer
		vk.cmdExecuteCommands(*primCmdBuf1, 1, &secCmdBuf.get());
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf1));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo1				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf1.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo1, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));
	VK_CHECK(vk.resetFences(vkDevice, 1u, &fence.get()));

	// check if secondary buffer has been executed
	VkResult result = vk.getEventStatus(vkDevice,*event);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("Submit Twice Secondary Command Buffer FAILED");

	// reset first primary buffer
	vk.resetCommandBuffer( *primCmdBuf1, 0u);

	// reset event to allow receiving it again
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record second primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf2, &primCmdBufBeginInfo));
	{
		// execute secondary buffer
		vk.cmdExecuteCommands(*primCmdBuf2, 1, &secCmdBuf.get());
	}
	// end recording
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf2));

	// submit second primary buffer, the secondary should be executed too
	const VkSubmitInfo						submitInfo2				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf2.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo2, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// check if secondary buffer has been executed
	result = vk.getEventStatus(vkDevice,*event);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("Submit Twice Secondary Command Buffer FAILED");
	else
		return tcu::TestStatus::pass("Submit Twice Secondary Command Buffer succeeded");
}

tcu::TestStatus oneTimeSubmitFlagPrimaryBufferTest(Context& context)
{

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,				// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// record setting event
		vk.cmdSetEvent(*primCmdBuf, *event,stageMask);
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// submit primary buffer
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));
	VK_CHECK(vk.resetFences(vkDevice, 1u, &fence.get()));

	// check if buffer has been executed
	VkResult result = vk.getEventStatus(vkDevice,*event);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("oneTimeSubmitFlagPrimaryBufferTest FAILED");

	// record primary command buffer again - implicit reset because of VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// record setting event
		vk.cmdSetEvent(*primCmdBuf, *event,stageMask);
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// check if buffer has been executed
	result = vk.getEventStatus(vkDevice,*event);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("oneTimeSubmitFlagPrimaryBufferTest FAILED");
	else
		return tcu::TestStatus::pass("oneTimeSubmitFlagPrimaryBufferTest succeeded");
}

tcu::TestStatus oneTimeSubmitFlagSecondaryBufferTest(Context& context)
{

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};

	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};

	const Unique<VkCommandBuffer>			primCmdBuf1				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const Unique<VkCommandBuffer>			primCmdBuf2				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	// Secondary Command buffer
	const VkCommandBufferAllocateInfo		secCmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			secCmdBuf				(allocateCommandBuffer(vk, vkDevice, &secCmdBufParams));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		(VkRenderPass)0u,											// renderPass
		0u,															// subpass
		(VkFramebuffer)0u,											// framebuffer
		VK_FALSE,													// occlusionQueryEnable
		(VkQueryControlFlags)0u,									// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,				// flags
		&secCmdBufInheritInfo,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record first primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf1, &primCmdBufBeginInfo));
	{
		// record secondary command buffer
		VK_CHECK(vk.beginCommandBuffer(*secCmdBuf, &secCmdBufBeginInfo));
		{
			// allow execution of event during every stage of pipeline
			VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// record setting event
			vk.cmdSetEvent(*secCmdBuf, *event,stageMask);
		}

		// end recording of secondary buffers
		VK_CHECK(vk.endCommandBuffer(*secCmdBuf));

		// execute secondary buffer
		vk.cmdExecuteCommands(*primCmdBuf1, 1, &secCmdBuf.get());
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf1));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo1				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf1.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo1, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));
	VK_CHECK(vk.resetFences(vkDevice, 1u, &fence.get()));

	// check if secondary buffer has been executed
	VkResult result = vk.getEventStatus(vkDevice,*event);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("Submit Twice Secondary Command Buffer FAILED");

	// reset first primary buffer
	vk.resetCommandBuffer( *primCmdBuf1, 0u);

	// reset event to allow receiving it again
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record secondary command buffer again
	VK_CHECK(vk.beginCommandBuffer(*secCmdBuf, &secCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// record setting event
		vk.cmdSetEvent(*secCmdBuf, *event,stageMask);
	}
	// end recording of secondary buffers
	VK_CHECK(vk.endCommandBuffer(*secCmdBuf));

	// record second primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf2, &primCmdBufBeginInfo));
	{
		// execute secondary buffer
		vk.cmdExecuteCommands(*primCmdBuf2, 1, &secCmdBuf.get());
	}
	// end recording
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf2));

	// submit second primary buffer, the secondary should be executed too
	const VkSubmitInfo						submitInfo2				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf2.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo2, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// check if secondary buffer has been executed
	result = vk.getEventStatus(vkDevice,*event);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("oneTimeSubmitFlagSecondaryBufferTest FAILED");
	else
		return tcu::TestStatus::pass("oneTimeSubmitFlagSecondaryBufferTest succeeded");
}

tcu::TestStatus renderPassContinueTest(Context& context)
{
	const DeviceInterface&					vkd						= context.getDeviceInterface();
	CommandBufferRenderPassTestEnvironment	env						(context, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VkCommandBuffer							primaryCommandBuffer	= env.getPrimaryCommandBuffer();
	VkCommandBuffer							secondaryCommandBuffer	= env.getSecondaryCommandBuffer();
	const deUint32							clearColor[4]			= { 2, 47, 131, 211 };

	const VkClearAttachment					clearAttachment			=
	{
		VK_IMAGE_ASPECT_COLOR_BIT,									// VkImageAspectFlags	aspectMask;
		0,															// deUint32				colorAttachment;
		makeClearValueColorU32(clearColor[0],
							   clearColor[1],
							   clearColor[2],
							   clearColor[3])						// VkClearValue			clearValue;
	};

	const VkClearRect						clearRect				=
	{
		CommandBufferRenderPassTestEnvironment::DEFAULT_IMAGE_AREA,	// VkRect2D	rect;
		0u,															// deUint32	baseArrayLayer;
		1u															// deUint32	layerCount;
	};

	env.beginSecondaryCommandBuffer(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
	vkd.cmdClearAttachments(secondaryCommandBuffer, 1, &clearAttachment, 1, &clearRect);
	VK_CHECK(vkd.endCommandBuffer(secondaryCommandBuffer));


	env.beginPrimaryCommandBuffer(0);
	env.beginRenderPass(VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	vkd.cmdExecuteCommands(primaryCommandBuffer, 1, &secondaryCommandBuffer);
	vkd.cmdEndRenderPass(primaryCommandBuffer);

	VK_CHECK(vkd.endCommandBuffer(primaryCommandBuffer));

	env.submitPrimaryCommandBuffer();

	de::MovePtr<tcu::TextureLevel>			result					= env.readColorAttachment();
	tcu::PixelBufferAccess					pixelBufferAccess		= result->getAccess();

	for (deUint32 i = 0; i < (CommandBufferRenderPassTestEnvironment::DEFAULT_IMAGE_SIZE.width * CommandBufferRenderPassTestEnvironment::DEFAULT_IMAGE_SIZE.height); ++i)
	{
		deUint8* colorData = reinterpret_cast<deUint8*>(pixelBufferAccess.getDataPtr());
		for (int colorComponent = 0; colorComponent < 4; ++colorComponent)
			if (colorData[i * 4 + colorComponent] != clearColor[colorComponent])
				return tcu::TestStatus::fail("clear value mismatch");
	}

	return tcu::TestStatus::pass("render pass continue test passed");
}

tcu::TestStatus simultaneousUsePrimaryBufferTest(Context& context)
{

	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,				// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					eventOne				(createEvent(vk, vkDevice));
	const Unique<VkEvent>					eventTwo				(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *eventOne));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// wait for event
		vk.cmdWaitEvents(*primCmdBuf, 1u, &eventOne.get(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0u, DE_NULL, 0u, DE_NULL, 0u, DE_NULL);

		// Set the second event
		vk.cmdSetEvent(*primCmdBuf, eventTwo.get(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence1					(createFence(vk, vkDevice));
	const Unique<VkFence>					fence2					(createFence(vk, vkDevice));


	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// submit first buffer
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence1));

	// submit second buffer
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence2));

	// wait for both buffer to stop at event for 100 microseconds
	vk.waitForFences(vkDevice, 1, &fence1.get(), 0u, 100000);
	vk.waitForFences(vkDevice, 1, &fence2.get(), 0u, 100000);

	// set event
	VK_CHECK(vk.setEvent(vkDevice, *eventOne));

	// wait for end of execution of the first buffer
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence1.get(), 0u, INFINITE_TIMEOUT));
	// wait for end of execution of the second buffer
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence2.get(), 0u, INFINITE_TIMEOUT));

	// TODO: this will be true if the command buffer was executed only once
	// TODO: add some test that will say if it was executed twice

	// check if buffer has been executed
	VkResult result = vk.getEventStatus(vkDevice, *eventTwo);
	if (result == VK_EVENT_SET)
		return tcu::TestStatus::pass("simultaneous use - primary buffers test succeeded");
	else
		return tcu::TestStatus::fail("simultaneous use - primary buffers test FAILED");
}

tcu::TestStatus simultaneousUseSecondaryBufferTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	// Secondary Command buffer params
	const VkCommandBufferAllocateInfo		secCmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			secCmdBuf				(allocateCommandBuffer(vk, vkDevice, &secCmdBufParams));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		(VkRenderPass)0u,											// renderPass
		0u,															// subpass
		(VkFramebuffer)0u,											// framebuffer
		VK_FALSE,													// occlusionQueryEnable
		(VkQueryControlFlags)0u,									// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,				// flags
		&secCmdBufInheritInfo,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					eventOne				(createEvent(vk, vkDevice));
	const Unique<VkEvent>					eventTwo				(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *eventOne));
	VK_CHECK(vk.resetEvent(vkDevice, *eventTwo));

	// record secondary command buffer
	VK_CHECK(vk.beginCommandBuffer(*secCmdBuf, &secCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// wait for event
		vk.cmdWaitEvents(*secCmdBuf, 1, &eventOne.get(), stageMask, stageMask, 0, DE_NULL, 0u, DE_NULL, 0u, DE_NULL);

		// reset event
		vk.cmdSetEvent(*secCmdBuf, *eventTwo, stageMask);
	}
	// end recording of secondary buffers
	VK_CHECK(vk.endCommandBuffer(*secCmdBuf));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// execute secondary buffer
		vk.cmdExecuteCommands(*primCmdBuf, 1, &secCmdBuf.get());
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// submit primary buffer, the secondary should be executed too
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for both buffers to stop at event for 100 microseconds
	vk.waitForFences(vkDevice, 1, &fence.get(), 0u, 100000);

	// set event
	VK_CHECK(vk.setEvent(vkDevice, *eventOne));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// TODO: this will be true if the command buffer was executed only once
	// TODO: add some test that will say if it was executed twice

	// check if secondary buffer has been executed
	VkResult result = vk.getEventStatus(vkDevice,*eventTwo);
	if (result == VK_EVENT_SET)
		return tcu::TestStatus::pass("Simulatous Secondary Command Buffer Execution succeeded");
	else
		return tcu::TestStatus::fail("Simulatous Secondary Command Buffer Execution FAILED");
}

tcu::TestStatus simultaneousUseSecondaryBufferOnePrimaryBufferTest(Context& context)
{
	const VkDevice							vkDevice = context.getDevice();
	const DeviceInterface&					vk = context.getDeviceInterface();
	const VkQueue							queue = context.getUniversalQueue();
	const deUint32							queueFamilyIndex = context.getUniversalQueueFamilyIndex();
	Allocator&								allocator = context.getDefaultAllocator();
	const ComputeInstanceResultBuffer		result(vk, vkDevice, allocator, 0.0f);

	const VkCommandPoolCreateInfo			cmdPoolParams =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	// Secondary Command buffer params
	const VkCommandBufferAllocateInfo		secCmdBufParams =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			secCmdBuf(allocateCommandBuffer(vk, vkDevice, &secCmdBufParams));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		(VkRenderPass)0u,
		0u,															// subpass
		(VkFramebuffer)0u,
		VK_FALSE,													// occlusionQueryEnable
		(VkQueryControlFlags)0u,
		(VkQueryPipelineStatisticFlags)0u,
	};
	const VkCommandBufferBeginInfo			secCmdBufBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,				// flags
		&secCmdBufInheritInfo,
	};

	const deUint32							offset = (0u);
	const deUint32							addressableSize = 256;
	const deUint32							dataSize = 8;
	de::MovePtr<Allocation>					bufferMem;
	const Unique<VkBuffer>					buffer(createDataBuffer(context, offset, addressableSize, 0x00, dataSize, 0x5A, &bufferMem));
	// Secondary command buffer will have a compute shader that does an atomic increment to make sure that all instances of secondary buffers execute
	const Unique<VkDescriptorSetLayout>		descriptorSetLayout(createDescriptorSetLayout(context));
	const Unique<VkDescriptorPool>			descriptorPool(createDescriptorPool(context));
	const Unique<VkDescriptorSet>			descriptorSet(createDescriptorSet(context, *descriptorPool, *descriptorSetLayout, *buffer, offset, result.getBuffer()));
	const VkDescriptorSet					descriptorSets[] = { *descriptorSet };
	const int								numDescriptorSets = DE_LENGTH_OF_ARRAY(descriptorSets);

	const VkPipelineLayoutCreateInfo layoutCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,				// sType
		DE_NULL,													// pNext
		(VkPipelineLayoutCreateFlags)0,
		numDescriptorSets,											// setLayoutCount
		&descriptorSetLayout.get(),									// pSetLayouts
		0u,															// pushConstantRangeCount
		DE_NULL,													// pPushConstantRanges
	};
	Unique<VkPipelineLayout>				pipelineLayout(createPipelineLayout(vk, vkDevice, &layoutCreateInfo));

	const Unique<VkShaderModule>			computeModule(createShaderModule(vk, vkDevice, context.getBinaryCollection().get("compute_increment"), (VkShaderModuleCreateFlags)0u));

	const VkPipelineShaderStageCreateInfo	shaderCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		DE_NULL,
		(VkPipelineShaderStageCreateFlags)0,
		VK_SHADER_STAGE_COMPUTE_BIT,								// stage
		*computeModule,												// shader
		"main",
		DE_NULL,													// pSpecializationInfo
	};

	const VkComputePipelineCreateInfo		pipelineCreateInfo =
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		DE_NULL,
		0u,															// flags
		shaderCreateInfo,											// cs
		*pipelineLayout,											// layout
		(vk::VkPipeline)0,											// basePipelineHandle
		0u,															// basePipelineIndex
	};

	const Unique<VkPipeline>				pipeline(createComputePipeline(vk, vkDevice, (VkPipelineCache)0u, &pipelineCreateInfo));

	// record secondary command buffer
	VK_CHECK(vk.beginCommandBuffer(*secCmdBuf, &secCmdBufBeginInfo));
	{
		vk.cmdBindPipeline(*secCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
		vk.cmdBindDescriptorSets(*secCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0, numDescriptorSets, descriptorSets, 0, 0);
		vk.cmdDispatch(*secCmdBuf, 1u, 1u, 1u);
	}
	// end recording of secondary buffer
	VK_CHECK(vk.endCommandBuffer(*secCmdBuf));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// execute secondary buffer twice in same primary
		vk.cmdExecuteCommands(*primCmdBuf, 1, &secCmdBuf.get());
		vk.cmdExecuteCommands(*primCmdBuf, 1, &secCmdBuf.get());
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// submit primary buffer, the secondary should be executed too
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	deUint32 resultCount;
	result.readResultContentsTo(&resultCount);
	// check if secondary buffer has been executed
	if (resultCount == 2)
		return tcu::TestStatus::pass("Simulatous Secondary Command Buffer Execution succeeded");
	else
		return tcu::TestStatus::fail("Simulatous Secondary Command Buffer Execution FAILED");
}

tcu::TestStatus simultaneousUseSecondaryBufferTwoPrimaryBuffersTest(Context& context)
{
	const VkDevice							vkDevice = context.getDevice();
	const DeviceInterface&					vk = context.getDeviceInterface();
	const VkQueue							queue = context.getUniversalQueue();
	const deUint32							queueFamilyIndex = context.getUniversalQueueFamilyIndex();
	Allocator&								allocator = context.getDefaultAllocator();
	const ComputeInstanceResultBuffer		result(vk, vkDevice, allocator, 0.0f);

	const VkCommandPoolCreateInfo			cmdPoolParams =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	// Two separate primary cmd buffers that will be executed with the same secondary cmd buffer
	const deUint32 numPrimCmdBufs = 2;
	const Unique<VkCommandBuffer>			primCmdBufOne(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const Unique<VkCommandBuffer>			primCmdBufTwo(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	VkCommandBuffer primCmdBufs[numPrimCmdBufs];
	primCmdBufs[0] = primCmdBufOne.get();
	primCmdBufs[1] = primCmdBufTwo.get();

	// Secondary Command buffer params
	const VkCommandBufferAllocateInfo		secCmdBufParams =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			secCmdBuf(allocateCommandBuffer(vk, vkDevice, &secCmdBufParams));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		(VkRenderPass)0u,											// renderPass
		0u,															// subpass
		(VkFramebuffer)0u,											// framebuffer
		VK_FALSE,													// occlusionQueryEnable
		(VkQueryControlFlags)0u,									// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secCmdBufBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,				// flags
		&secCmdBufInheritInfo,
	};

	const deUint32							offset = (0u);
	const deUint32							addressableSize = 256;
	const deUint32							dataSize = 8;
	de::MovePtr<Allocation>					bufferMem;
	const Unique<VkBuffer>					buffer(createDataBuffer(context, offset, addressableSize, 0x00, dataSize, 0x5A, &bufferMem));
	// Secondary command buffer will have a compute shader that does an atomic increment to make sure that all instances of secondary buffers execute
	const Unique<VkDescriptorSetLayout>		descriptorSetLayout(createDescriptorSetLayout(context));
	const Unique<VkDescriptorPool>			descriptorPool(createDescriptorPool(context));
	const Unique<VkDescriptorSet>			descriptorSet(createDescriptorSet(context, *descriptorPool, *descriptorSetLayout, *buffer, offset, result.getBuffer()));
	const VkDescriptorSet					descriptorSets[] = { *descriptorSet };
	const int								numDescriptorSets = DE_LENGTH_OF_ARRAY(descriptorSets);

	const VkPipelineLayoutCreateInfo layoutCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,				// sType
		DE_NULL,													// pNext
		(VkPipelineLayoutCreateFlags)0,
		numDescriptorSets,											// setLayoutCount
		&descriptorSetLayout.get(),									// pSetLayouts
		0u,															// pushConstantRangeCount
		DE_NULL,													// pPushConstantRanges
	};
	Unique<VkPipelineLayout>				pipelineLayout(createPipelineLayout(vk, vkDevice, &layoutCreateInfo));

	const Unique<VkShaderModule>			computeModule(createShaderModule(vk, vkDevice, context.getBinaryCollection().get("compute_increment"), (VkShaderModuleCreateFlags)0u));

	const VkPipelineShaderStageCreateInfo	shaderCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		DE_NULL,
		(VkPipelineShaderStageCreateFlags)0,
		VK_SHADER_STAGE_COMPUTE_BIT,								// stage
		*computeModule,												// shader
		"main",
		DE_NULL,													// pSpecializationInfo
	};

	const VkComputePipelineCreateInfo		pipelineCreateInfo =
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		DE_NULL,
		0u,															// flags
		shaderCreateInfo,											// cs
		*pipelineLayout,											// layout
		(vk::VkPipeline)0,											// basePipelineHandle
		0u,															// basePipelineIndex
	};

	const Unique<VkPipeline>				pipeline(createComputePipeline(vk, vkDevice, (VkPipelineCache)0u, &pipelineCreateInfo));

	// record secondary command buffer
	VK_CHECK(vk.beginCommandBuffer(*secCmdBuf, &secCmdBufBeginInfo));
	{
		vk.cmdBindPipeline(*secCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
		vk.cmdBindDescriptorSets(*secCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0, numDescriptorSets, descriptorSets, 0, 0);
		vk.cmdDispatch(*secCmdBuf, 1u, 1u, 1u);
	}
	// end recording of secondary buffer
	VK_CHECK(vk.endCommandBuffer(*secCmdBuf));

	// record primary command buffers
	// Insert one instance of same secondary command buffer into two separate primary command buffers
	VK_CHECK(vk.beginCommandBuffer(*primCmdBufOne, &primCmdBufBeginInfo));
	{
		vk.cmdExecuteCommands(*primCmdBufOne, 1, &secCmdBuf.get());
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBufOne));

	VK_CHECK(vk.beginCommandBuffer(*primCmdBufTwo, &primCmdBufBeginInfo));
	{
		vk.cmdExecuteCommands(*primCmdBufTwo, 1, &secCmdBuf.get());
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBufTwo));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		numPrimCmdBufs,												// commandBufferCount
		primCmdBufs,												// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// submit primary buffers, the secondary should be executed too
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	deUint32 resultCount;
	result.readResultContentsTo(&resultCount);
	// check if secondary buffer has been executed
	if (resultCount == 2)
		return tcu::TestStatus::pass("Simulatous Secondary Command Buffer Execution succeeded");
	else
		return tcu::TestStatus::fail("Simulatous Secondary Command Buffer Execution FAILED");
}

tcu::TestStatus recordBufferQueryPreciseWithFlagTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	if (!context.getDeviceFeatures().inheritedQueries)
		TCU_THROW(NotSupportedError, "Inherited queries feature is not supported");

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		primCmdBufParams		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level;
		1u,															// flags;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &primCmdBufParams));

	// Secondary Command buffer params
	const VkCommandBufferAllocateInfo		secCmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							// level;
		1u,															// flags;
	};
	const Unique<VkCommandBuffer>			secCmdBuf				(allocateCommandBuffer(vk, vkDevice, &secCmdBufParams));

	const VkCommandBufferBeginInfo			primBufferBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secBufferInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		0u,															// renderPass
		0u,															// subpass
		0u,															// framebuffer
		VK_TRUE,													// occlusionQueryEnable
		VK_QUERY_CONTROL_PRECISE_BIT,								// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secBufferBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		&secBufferInheritInfo,
	};

	const VkQueryPoolCreateInfo				queryPoolCreateInfo		=
	{
		VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,					// sType
		DE_NULL,													// pNext
		(VkQueryPoolCreateFlags)0,									// flags
		VK_QUERY_TYPE_OCCLUSION,									// queryType
		1u,															// entryCount
		0u,															// pipelineStatistics
	};
	Unique<VkQueryPool>						queryPool				(createQueryPool(vk, vkDevice, &queryPoolCreateInfo));

	VK_CHECK(vk.beginCommandBuffer(secCmdBuf.get(), &secBufferBeginInfo));
	VK_CHECK(vk.endCommandBuffer(secCmdBuf.get()));

	VK_CHECK(vk.beginCommandBuffer(primCmdBuf.get(), &primBufferBeginInfo));
	{
		vk.cmdResetQueryPool(primCmdBuf.get(), queryPool.get(), 0u, 1u);
		vk.cmdBeginQuery(primCmdBuf.get(), queryPool.get(), 0u, VK_QUERY_CONTROL_PRECISE_BIT);
		{
			vk.cmdExecuteCommands(primCmdBuf.get(), 1u, &secCmdBuf.get());
		}
		vk.cmdEndQuery(primCmdBuf.get(), queryPool.get(), 0u);
	}
	VK_CHECK(vk.endCommandBuffer(primCmdBuf.get()));

	return tcu::TestStatus::pass("Successfully recorded a secondary command buffer allowing a precise occlusion query.");
}

tcu::TestStatus recordBufferQueryImpreciseWithFlagTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	if (!context.getDeviceFeatures().inheritedQueries)
		TCU_THROW(NotSupportedError, "Inherited queries feature is not supported");

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		primCmdBufParams		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level;
		1u,															// flags;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &primCmdBufParams));

	// Secondary Command buffer params
	const VkCommandBufferAllocateInfo		secCmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							// level;
		1u,															// flags;
	};
	const Unique<VkCommandBuffer>			secCmdBuf				(allocateCommandBuffer(vk, vkDevice, &secCmdBufParams));

	const VkCommandBufferBeginInfo			primBufferBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secBufferInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		0u,															// renderPass
		0u,															// subpass
		0u,															// framebuffer
		VK_TRUE,													// occlusionQueryEnable
		VK_QUERY_CONTROL_PRECISE_BIT,								// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secBufferBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		&secBufferInheritInfo,
	};

	// Create an occlusion query with VK_QUERY_CONTROL_PRECISE_BIT set
	const VkQueryPoolCreateInfo				queryPoolCreateInfo		=
	{
		VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,					// sType
		DE_NULL,													// pNext
		0u,															// flags
		VK_QUERY_TYPE_OCCLUSION,									// queryType
		1u,															// entryCount
		0u,															// pipelineStatistics
	};
	Unique<VkQueryPool>						queryPool				(createQueryPool(vk, vkDevice, &queryPoolCreateInfo));

	VK_CHECK(vk.beginCommandBuffer(secCmdBuf.get(), &secBufferBeginInfo));
	VK_CHECK(vk.endCommandBuffer(secCmdBuf.get()));

	VK_CHECK(vk.beginCommandBuffer(primCmdBuf.get(), &primBufferBeginInfo));
	{
		vk.cmdResetQueryPool(primCmdBuf.get(), queryPool.get(), 0u, 1u);
		vk.cmdBeginQuery(primCmdBuf.get(), queryPool.get(), 0u, VK_QUERY_CONTROL_PRECISE_BIT);
		{
			vk.cmdExecuteCommands(primCmdBuf.get(), 1u, &secCmdBuf.get());
		}
		vk.cmdEndQuery(primCmdBuf.get(), queryPool.get(), 0u);
	}
	VK_CHECK(vk.endCommandBuffer(primCmdBuf.get()));

	return tcu::TestStatus::pass("Successfully recorded a secondary command buffer allowing a precise occlusion query.");
}

tcu::TestStatus recordBufferQueryImpreciseWithoutFlagTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	if (!context.getDeviceFeatures().inheritedQueries)
		TCU_THROW(NotSupportedError, "Inherited queries feature is not supported");

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		primCmdBufParams		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level;
		1u,															// flags;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &primCmdBufParams));

	// Secondary Command buffer params
	const VkCommandBufferAllocateInfo		secCmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							// level;
		1u,															// flags;
	};
	const Unique<VkCommandBuffer>			secCmdBuf				(allocateCommandBuffer(vk, vkDevice, &secCmdBufParams));

	const VkCommandBufferBeginInfo			primBufferBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secBufferInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		0u,															// renderPass
		0u,															// subpass
		0u,															// framebuffer
		VK_TRUE,													// occlusionQueryEnable
		0u,															// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secBufferBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		&secBufferInheritInfo,
	};

	// Create an occlusion query with VK_QUERY_CONTROL_PRECISE_BIT set
	const VkQueryPoolCreateInfo				queryPoolCreateInfo		=
	{
		VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,					// sType
		DE_NULL,													// pNext
		(VkQueryPoolCreateFlags)0,
		VK_QUERY_TYPE_OCCLUSION,
		1u,
		0u,
	};
	Unique<VkQueryPool>						queryPool				(createQueryPool(vk, vkDevice, &queryPoolCreateInfo));

	VK_CHECK(vk.beginCommandBuffer(secCmdBuf.get(), &secBufferBeginInfo));
	VK_CHECK(vk.endCommandBuffer(secCmdBuf.get()));

	VK_CHECK(vk.beginCommandBuffer(primCmdBuf.get(), &primBufferBeginInfo));
	{
		vk.cmdResetQueryPool(primCmdBuf.get(), queryPool.get(), 0u, 1u);
		vk.cmdBeginQuery(primCmdBuf.get(), queryPool.get(), 0u, VK_QUERY_CONTROL_PRECISE_BIT);
		{
			vk.cmdExecuteCommands(primCmdBuf.get(), 1u, &secCmdBuf.get());
		}
		vk.cmdEndQuery(primCmdBuf.get(), queryPool.get(), 0u);
	}
	VK_CHECK(vk.endCommandBuffer(primCmdBuf.get()));

	return tcu::TestStatus::pass("Successfully recorded a secondary command buffer allowing a precise occlusion query.");
}

/******** 19.4. Command Buffer Submission (5.4 in VK 1.0 Spec) ****************/
tcu::TestStatus submitBufferCountNonZero(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const deUint32							BUFFER_COUNT			= 5u;

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		0u,															// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level;
		BUFFER_COUNT,												// bufferCount;
	};
	VkCommandBuffer cmdBuffers[BUFFER_COUNT];
	VK_CHECK(vk.allocateCommandBuffers(vkDevice, &cmdBufParams, cmdBuffers));

	const VkCommandBufferBeginInfo			cmdBufBeginInfo			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	std::vector<VkEventSp>					events;
	for (deUint32 ndx = 0; ndx < BUFFER_COUNT; ++ndx)
	{
		events.push_back(VkEventSp(new vk::Unique<VkEvent>(createEvent(vk, vkDevice))));
	}

	// Record the command buffers
	for (deUint32 ndx = 0; ndx < BUFFER_COUNT; ++ndx)
	{
		VK_CHECK(vk.beginCommandBuffer(cmdBuffers[ndx], &cmdBufBeginInfo));
		{
			vk.cmdSetEvent(cmdBuffers[ndx], events[ndx]->get(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}
		VK_CHECK(vk.endCommandBuffer(cmdBuffers[ndx]));
	}

	// We'll use a fence to wait for the execution of the queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		BUFFER_COUNT,												// commandBufferCount
		cmdBuffers,													// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// Submit the alpha command buffer to the queue
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, fence.get()));
	// Wait for the queue
	VK_CHECK(vk.waitForFences(vkDevice, 1u, &fence.get(), VK_TRUE, INFINITE_TIMEOUT));

	// Check if the buffers were executed
	tcu::TestStatus testResult = tcu::TestStatus::incomplete();

	for (deUint32 ndx = 0; ndx < BUFFER_COUNT; ++ndx)
	{
		if (vk.getEventStatus(vkDevice, events[ndx]->get()) != VK_EVENT_SET)
		{
			testResult = tcu::TestStatus::fail("Failed to set the event.");
			break;
		}
	}

	if (!testResult.isComplete())
		testResult = tcu::TestStatus::pass("All buffers were submitted and executed correctly.");

	return testResult;
}

tcu::TestStatus submitBufferCountEqualZero(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const deUint32							BUFFER_COUNT			= 2u;

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		0u,															// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level;
		BUFFER_COUNT,												// bufferCount;
	};
	VkCommandBuffer cmdBuffers[BUFFER_COUNT];
	VK_CHECK(vk.allocateCommandBuffers(vkDevice, &cmdBufParams, cmdBuffers));

	const VkCommandBufferBeginInfo			cmdBufBeginInfo			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	std::vector<VkEventSp>					events;
	for (deUint32 ndx = 0; ndx < BUFFER_COUNT; ++ndx)
		events.push_back(VkEventSp(new vk::Unique<VkEvent>(createEvent(vk, vkDevice))));

	// Record the command buffers
	for (deUint32 ndx = 0; ndx < BUFFER_COUNT; ++ndx)
	{
		VK_CHECK(vk.beginCommandBuffer(cmdBuffers[ndx], &cmdBufBeginInfo));
		{
			vk.cmdSetEvent(cmdBuffers[ndx], events[ndx]->get(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}
		VK_CHECK(vk.endCommandBuffer(cmdBuffers[ndx]));
	}

	// We'll use a fence to wait for the execution of the queue
	const Unique<VkFence>					fenceZero				(createFence(vk, vkDevice));
	const Unique<VkFence>					fenceOne				(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfoCountZero		=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1u,															// commandBufferCount
		&cmdBuffers[0],												// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	const VkSubmitInfo						submitInfoCountOne		=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1u,															// commandBufferCount
		&cmdBuffers[1],												// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// Submit the command buffers to the queue
	// We're performing two submits to make sure that the first one has
	// a chance to be processed before we check the event's status
	VK_CHECK(vk.queueSubmit(queue, 0, &submitInfoCountZero, fenceZero.get()));
	VK_CHECK(vk.queueSubmit(queue, 1, &submitInfoCountOne, fenceOne.get()));

	const VkFence							fences[]				=
	{
		fenceZero.get(),
		fenceOne.get(),
	};

	// Wait for the queue
	VK_CHECK(vk.waitForFences(vkDevice, (deUint32)DE_LENGTH_OF_ARRAY(fences), fences, VK_TRUE, INFINITE_TIMEOUT));

	// Check if the first buffer was executed
	tcu::TestStatus testResult = tcu::TestStatus::incomplete();

	if (vk.getEventStatus(vkDevice, events[0]->get()) == VK_EVENT_SET)
		testResult = tcu::TestStatus::fail("The first event was signaled.");
	else
		testResult = tcu::TestStatus::pass("The first submission was ignored.");

	return testResult;
}

tcu::TestStatus submitBufferWaitSingleSemaphore(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// VkStructureType				sType;
		DE_NULL,													// const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// VkCommandPoolCreateFlags		flags;
		queueFamilyIndex,											// deUint32						queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// VkStructureType				sType;
		DE_NULL,													// const void*					pNext;
		*cmdPool,													// VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// VkCommandBufferLevel			level;
		1u,															// uint32_t						bufferCount;
	};

	// Create two command buffers
	const Unique<VkCommandBuffer>			primCmdBuf1				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const Unique<VkCommandBuffer>			primCmdBuf2				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0,															// flags
		DE_NULL														// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
	};

	// create two events that will be used to check if command buffers has been executed
	const Unique<VkEvent>					event1					(createEvent(vk, vkDevice));
	const Unique<VkEvent>					event2					(createEvent(vk, vkDevice));

	// reset events
	VK_CHECK(vk.resetEvent(vkDevice, *event1));
	VK_CHECK(vk.resetEvent(vkDevice, *event2));

	// record first command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf1, &primCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// record setting event
		vk.cmdSetEvent(*primCmdBuf1, *event1,stageMask);
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf1));

	// record second command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf2, &primCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// record setting event
		vk.cmdSetEvent(*primCmdBuf2, *event2,stageMask);
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf2));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	// create semaphore for use in this test
	const Unique <VkSemaphore>				semaphore				(createSemaphore(vk, vkDevice));

	// create submit info for first buffer - signalling semaphore
	const VkSubmitInfo						submitInfo1				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		DE_NULL,													// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf1.get(),											// pCommandBuffers
		1u,															// signalSemaphoreCount
		&semaphore.get(),											// pSignalSemaphores
	};

	// Submit the command buffer to the queue
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo1, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// check if buffer has been executed
	VkResult result = vk.getEventStatus(vkDevice,*event1);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("Submit Buffer and Wait for Single Semaphore Test FAILED");

	const VkPipelineStageFlags				waitDstStageFlags		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	// create submit info for second buffer - waiting for semaphore
	const VkSubmitInfo						submitInfo2				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		1u,															// waitSemaphoreCount
		&semaphore.get(),											// pWaitSemaphores
		&waitDstStageFlags,											// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBuf2.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// reset fence, so it can be used again
	VK_CHECK(vk.resetFences(vkDevice, 1u, &fence.get()));

	// Submit the second command buffer to the queue
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo2, *fence));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// check if second buffer has been executed
	// if it has been executed, it means that the semaphore was signalled - so test if passed
	result = vk.getEventStatus(vkDevice,*event1);
	if (result != VK_EVENT_SET)
		return tcu::TestStatus::fail("Submit Buffer and Wait for Single Semaphore Test FAILED");

	return tcu::TestStatus::pass("Submit Buffer and Wait for Single Semaphore Test succeeded");
}

tcu::TestStatus submitBufferWaitManySemaphores(Context& context)
{
	// This test will create numSemaphores semaphores, and signal them in NUM_SEMAPHORES submits to queue
	// After that the numSubmissions queue submissions will wait for each semaphore

	const deUint32							numSemaphores			= 10u;  // it must be multiply of numSubmission
	const deUint32							numSubmissions			= 2u;
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// VkStructureType				sType;
		DE_NULL,													// const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// VkCommandPoolCreateFlags		flags;
		queueFamilyIndex,											// deUint32						queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// VkStructureType				sType;
		DE_NULL,													// const void*					pNext;
		*cmdPool,													// VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// VkCommandBufferLevel			level;
		1u,															// uint32_t						bufferCount;
	};

	// Create command buffer
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0,															// flags
		DE_NULL														// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
	};

	// create event that will be used to check if command buffers has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// reset event - at creation state is undefined
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// record setting event
		vk.cmdSetEvent(*primCmdBuf, *event,stageMask);
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	// numSemaphores is declared const, so this array can be static
	// the semaphores will be destroyed automatically at end of scope
	Move <VkSemaphore>						semaphoreArray[numSemaphores];
	VkSemaphore								semaphores[numSemaphores];

	for (deUint32 idx = 0; idx < numSemaphores; ++idx) {
		// create semaphores for use in this test
		semaphoreArray[idx] = createSemaphore(vk, vkDevice);
		semaphores[idx] = semaphoreArray[idx].get();
	};

	{
		// create submit info for buffer - signal semaphores
		const VkSubmitInfo submitInfo1 =
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,							// sType
			DE_NULL,												// pNext
			0u,														// waitSemaphoreCount
			DE_NULL,												// pWaitSemaphores
			DE_NULL,												// pWaitDstStageMask
			1,														// commandBufferCount
			&primCmdBuf.get(),										// pCommandBuffers
			numSemaphores,											// signalSemaphoreCount
			semaphores												// pSignalSemaphores
		};
		// Submit the command buffer to the queue
		VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo1, *fence));

		// wait for end of execution of queue
		VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

		// check if buffer has been executed
		VkResult result = vk.getEventStatus(vkDevice,*event);
		if (result != VK_EVENT_SET)
			return tcu::TestStatus::fail("Submit Buffer and Wait for Many Semaphores Test FAILED");

		// reset event, so next buffers can set it again
		VK_CHECK(vk.resetEvent(vkDevice, *event));

		// reset fence, so it can be used again
		VK_CHECK(vk.resetFences(vkDevice, 1u, &fence.get()));
	}

	const deUint32							numberOfSemaphoresToBeWaitedByOneSubmission	= numSemaphores / numSubmissions;
	const std::vector<VkPipelineStageFlags>	waitDstStageFlags							(numberOfSemaphoresToBeWaitedByOneSubmission, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

	// the following code waits for the semaphores set above - numSubmissions queues will wait for each semaphore from above
	for (deUint32 idxSubmission = 0; idxSubmission < numSubmissions; ++idxSubmission) {

		// create submit info for buffer - waiting for semaphore
		const VkSubmitInfo				submitInfo2				=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,												// sType
			DE_NULL,																	// pNext
			numberOfSemaphoresToBeWaitedByOneSubmission,								// waitSemaphoreCount
			semaphores + (numberOfSemaphoresToBeWaitedByOneSubmission * idxSubmission),	// pWaitSemaphores
			waitDstStageFlags.data(),													// pWaitDstStageMask
			1,																			// commandBufferCount
			&primCmdBuf.get(),															// pCommandBuffers
			0u,																			// signalSemaphoreCount
			DE_NULL,																	// pSignalSemaphores
		};

		// Submit the second command buffer to the queue
		VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo2, *fence));

		// wait for 1 second.
		VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, 1000 * 1000 * 1000));

		// check if second buffer has been executed
		// if it has been executed, it means that the semaphore was signalled - so test if passed
		VkResult result = vk.getEventStatus(vkDevice,*event);
		if (result != VK_EVENT_SET)
			return tcu::TestStatus::fail("Submit Buffer and Wait for Many Semaphores Test FAILED");

		// reset fence, so it can be used again
		VK_CHECK(vk.resetFences(vkDevice, 1u, &fence.get()));

		// reset event, so next buffers can set it again
		VK_CHECK(vk.resetEvent(vkDevice, *event));
	}

	return tcu::TestStatus::pass("Submit Buffer and Wait for Many Semaphores Test succeeded");
}

tcu::TestStatus submitBufferNullFence(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const short								BUFFER_COUNT			= 2;

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		0u,															// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level;
		1u,															// bufferCount;
	};
	VkCommandBuffer cmdBuffers[BUFFER_COUNT];
	for (deUint32 ndx = 0; ndx < BUFFER_COUNT; ++ndx)
		VK_CHECK(vk.allocateCommandBuffers(vkDevice, &cmdBufParams, &cmdBuffers[ndx]));

	const VkCommandBufferBeginInfo			cmdBufBeginInfo			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	std::vector<VkEventSp>					events;
	for (deUint32 ndx = 0; ndx < BUFFER_COUNT; ++ndx)
		events.push_back(VkEventSp(new vk::Unique<VkEvent>(createEvent(vk, vkDevice))));

	// Record the command buffers
	for (deUint32 ndx = 0; ndx < BUFFER_COUNT; ++ndx)
	{
		VK_CHECK(vk.beginCommandBuffer(cmdBuffers[ndx], &cmdBufBeginInfo));
		{
			vk.cmdSetEvent(cmdBuffers[ndx], events[ndx]->get(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}
		VK_CHECK(vk.endCommandBuffer(cmdBuffers[ndx]));
	}

	// We'll use a fence to wait for the execution of the queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfoNullFence		=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1u,															// commandBufferCount
		&cmdBuffers[0],												// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	const VkSubmitInfo						submitInfoNonNullFence	=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1u,															// commandBufferCount
		&cmdBuffers[1],												// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// Perform two submissions - one with no fence, the other one with a valid
	// fence Hoping submitting the other buffer will give the first one time to
	// execute
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfoNullFence, DE_NULL));
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfoNonNullFence, fence.get()));

	// Wait for the queue
	VK_CHECK(vk.waitForFences(vkDevice, 1u, &fence.get(), VK_TRUE, INFINITE_TIMEOUT));


	tcu::TestStatus testResult = tcu::TestStatus::incomplete();

	//Fence guaranteed that all buffers submited before fence were executed
	if (vk.getEventStatus(vkDevice, events[0]->get()) != VK_EVENT_SET || vk.getEventStatus(vkDevice, events[1]->get()) != VK_EVENT_SET)
	{
		testResult = tcu::TestStatus::fail("One of the buffers was not executed.");
	}
	else
	{
		testResult = tcu::TestStatus::pass("Buffers have been submitted and executed correctly.");
	}

	vk.queueWaitIdle(queue);
	return testResult;
}

/******** 19.5. Secondary Command Buffer Execution (5.6 in VK 1.0 Spec) *******/
tcu::TestStatus executeSecondaryBufferTest(Context& context)
{
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			// flags;
		queueFamilyIndex,											// queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level;
		1u,															// bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBuf				(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	// Secondary Command buffer
	const VkCommandBufferAllocateInfo		secCmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType;
		DE_NULL,													// pNext;
		*cmdPool,													// commandPool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							// level;
		1u,															// bufferCount;
	};
	const Unique<VkCommandBuffer>			secCmdBuf				(allocateCommandBuffer(vk, vkDevice, &secCmdBufParams));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		DE_NULL,													// renderPass
		0u,															// subpass
		DE_NULL,													// framebuffer
		VK_FALSE,													// occlusionQueryEnable
		(VkQueryControlFlags)0u,									// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		&secCmdBufInheritInfo,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					event					(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *event));

	// record secondary command buffer
	VK_CHECK(vk.beginCommandBuffer(*secCmdBuf, &secCmdBufBeginInfo));
	{
		// allow execution of event during every stage of pipeline
		VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		// record setting event
		vk.cmdSetEvent(*secCmdBuf, *event, stageMask);
	}
	// end recording of the secondary buffer
	VK_CHECK(vk.endCommandBuffer(*secCmdBuf));

	// record primary command buffer
	VK_CHECK(vk.beginCommandBuffer(*primCmdBuf, &primCmdBufBeginInfo));
	{
		// execute secondary buffer
		vk.cmdExecuteCommands(*primCmdBuf, 1u, &secCmdBuf.get());
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBuf));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fence					(createFence(vk, vkDevice));
	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1u,															// commandBufferCount
		&primCmdBuf.get(),											// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// submit primary buffer, the secondary should be executed too
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, fence.get()));

	// wait for end of execution of queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fence.get(), 0u, INFINITE_TIMEOUT));

	// check if secondary buffer has been executed
	VkResult result = vk.getEventStatus(vkDevice, *event);
	if (result == VK_EVENT_SET)
		return tcu::TestStatus::pass("executeSecondaryBufferTest succeeded");

	return tcu::TestStatus::fail("executeSecondaryBufferTest FAILED");
}

tcu::TestStatus executeSecondaryBufferTwiceTest(Context& context)
{
	const deUint32							BUFFER_COUNT			= 10u;
	const VkDevice							vkDevice				= context.getDevice();
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();

	const VkCommandPoolCreateInfo			cmdPoolParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					//	VkStructureType				sType;
		DE_NULL,													//	const void*					pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,			//	VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,											//	deUint32					queueFamilyIndex;
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, vkDevice, &cmdPoolParams));

	// Command buffer
	const VkCommandBufferAllocateInfo		cmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							//	VkCommandBufferLevel		level;
		1u,															//	uint32_t					bufferCount;
	};
	const Unique<VkCommandBuffer>			primCmdBufOne			(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));
	const Unique<VkCommandBuffer>			primCmdBufTwo			(allocateCommandBuffer(vk, vkDevice, &cmdBufParams));

	// Secondary Command buffers params
	const VkCommandBufferAllocateInfo		secCmdBufParams			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		*cmdPool,													//	VkCommandPool				pool;
		VK_COMMAND_BUFFER_LEVEL_SECONDARY,							//	VkCommandBufferLevel		level;
		BUFFER_COUNT,												//	uint32_t					bufferCount;
	};
	VkCommandBuffer cmdBuffers[BUFFER_COUNT];
	VK_CHECK(vk.allocateCommandBuffers(vkDevice, &secCmdBufParams, cmdBuffers));

	const VkCommandBufferBeginInfo			primCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		(VkRenderPass)0u,											// renderPass
		0u,															// subpass
		(VkFramebuffer)0u,											// framebuffer
		VK_FALSE,													// occlusionQueryEnable
		(VkQueryControlFlags)0u,									// queryFlags
		(VkQueryPipelineStatisticFlags)0u,							// pipelineStatistics
	};
	const VkCommandBufferBeginInfo			secCmdBufBeginInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,				// flags
		&secCmdBufInheritInfo,
	};

	// create event that will be used to check if secondary command buffer has been executed
	const Unique<VkEvent>					eventOne				(createEvent(vk, vkDevice));

	// reset event
	VK_CHECK(vk.resetEvent(vkDevice, *eventOne));

	for (deUint32 ndx = 0; ndx < BUFFER_COUNT; ++ndx)
	{
		// record secondary command buffer
		VK_CHECK(vk.beginCommandBuffer(cmdBuffers[ndx], &secCmdBufBeginInfo));
		{
			// allow execution of event during every stage of pipeline
			VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			// wait for event
			vk.cmdWaitEvents(cmdBuffers[ndx], 1, &eventOne.get(), stageMask, stageMask, 0, DE_NULL, 0u, DE_NULL, 0u, DE_NULL);
		}
		// end recording of secondary buffers
		VK_CHECK(vk.endCommandBuffer(cmdBuffers[ndx]));
	};

	// record primary command buffer one
	VK_CHECK(vk.beginCommandBuffer(*primCmdBufOne, &primCmdBufBeginInfo));
	{
		// execute one secondary buffer
		vk.cmdExecuteCommands(*primCmdBufOne, 1, cmdBuffers );
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBufOne));

	// record primary command buffer two
	VK_CHECK(vk.beginCommandBuffer(*primCmdBufTwo, &primCmdBufBeginInfo));
	{
		// execute one secondary buffer with all buffers
		vk.cmdExecuteCommands(*primCmdBufTwo, BUFFER_COUNT, cmdBuffers );
	}
	VK_CHECK(vk.endCommandBuffer(*primCmdBufTwo));

	// create fence to wait for execution of queue
	const Unique<VkFence>					fenceOne				(createFence(vk, vkDevice));
	const Unique<VkFence>					fenceTwo				(createFence(vk, vkDevice));

	const VkSubmitInfo						submitInfoOne			=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBufOne.get(),										// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// submit primary buffer, the secondary should be executed too
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfoOne, *fenceOne));

	// wait for buffer to stop at event for 100 microseconds
	vk.waitForFences(vkDevice, 1, &fenceOne.get(), 0u, 100000);

	const VkSubmitInfo						submitInfoTwo			=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&primCmdBufTwo.get(),										// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};

	// submit second primary buffer, the secondary should be executed too
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfoTwo, *fenceTwo));

	// wait for all buffers to stop at event for 100 microseconds
	vk.waitForFences(vkDevice, 1, &fenceOne.get(), 0u, 100000);

	// now all buffers are waiting at eventOne
	// set event eventOne
	VK_CHECK(vk.setEvent(vkDevice, *eventOne));

	// wait for end of execution of fenceOne
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fenceOne.get(), 0u, INFINITE_TIMEOUT));

	// wait for end of execution of second queue
	VK_CHECK(vk.waitForFences(vkDevice, 1, &fenceTwo.get(), 0u, INFINITE_TIMEOUT));

	return tcu::TestStatus::pass("executeSecondaryBufferTwiceTest succeeded");
}

/******** 19.6. Commands Allowed Inside Command Buffers (? in VK 1.0 Spec) **/
tcu::TestStatus orderBindPipelineTest(Context& context)
{
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkDevice							device					= context.getDevice();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	Allocator&								allocator				= context.getDefaultAllocator();
	const ComputeInstanceResultBuffer		result					(vk, device, allocator);

	enum
	{
		ADDRESSABLE_SIZE = 256, // allocate a lot more than required
	};

	const tcu::Vec4							colorA1					= tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4							colorA2					= tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4							colorB1					= tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f);
	const tcu::Vec4							colorB2					= tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f);

	const deUint32							dataOffsetA				= (0u);
	const deUint32							dataOffsetB				= (0u);
	const deUint32							viewOffsetA				= (0u);
	const deUint32							viewOffsetB				= (0u);
	const deUint32							bufferSizeA				= dataOffsetA + ADDRESSABLE_SIZE;
	const deUint32							bufferSizeB				= dataOffsetB + ADDRESSABLE_SIZE;

	de::MovePtr<Allocation>					bufferMemA;
	const Unique<VkBuffer>					bufferA					(createColorDataBuffer(dataOffsetA, bufferSizeA, colorA1, colorA2, &bufferMemA, context));

	de::MovePtr<Allocation>					bufferMemB;
	const Unique<VkBuffer>					bufferB					(createColorDataBuffer(dataOffsetB, bufferSizeB, colorB1, colorB2, &bufferMemB, context));

	const Unique<VkDescriptorSetLayout>		descriptorSetLayout		(createDescriptorSetLayout(context));
	const Unique<VkDescriptorPool>			descriptorPool			(createDescriptorPool(context));
	const Unique<VkDescriptorSet>			descriptorSet			(createDescriptorSet(*descriptorPool, *descriptorSetLayout, *bufferA, viewOffsetA, *bufferB, viewOffsetB, result.getBuffer(), context));
	const VkDescriptorSet					descriptorSets[]		= { *descriptorSet };
	const int								numDescriptorSets		= DE_LENGTH_OF_ARRAY(descriptorSets);

	const VkPipelineLayoutCreateInfo layoutCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,				// sType
		DE_NULL,													// pNext
		(VkPipelineLayoutCreateFlags)0,
		numDescriptorSets,											// setLayoutCount
		&descriptorSetLayout.get(),									// pSetLayouts
		0u,															// pushConstantRangeCount
		DE_NULL,													// pPushConstantRanges
	};
	Unique<VkPipelineLayout>				pipelineLayout			(createPipelineLayout(vk, device, &layoutCreateInfo));

	const Unique<VkShaderModule>			computeModuleGood		(createShaderModule(vk, device, context.getBinaryCollection().get("compute_good"), (VkShaderModuleCreateFlags)0u));
	const Unique<VkShaderModule>			computeModuleBad		(createShaderModule(vk, device, context.getBinaryCollection().get("compute_bad"),  (VkShaderModuleCreateFlags)0u));

	const VkPipelineShaderStageCreateInfo	shaderCreateInfoGood	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		DE_NULL,
		(VkPipelineShaderStageCreateFlags)0,
		VK_SHADER_STAGE_COMPUTE_BIT,								// stage
		*computeModuleGood,											// shader
		"main",
		DE_NULL,													// pSpecializationInfo
	};

	const VkPipelineShaderStageCreateInfo	shaderCreateInfoBad	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		DE_NULL,
		(vk::VkPipelineShaderStageCreateFlags)0,
		vk::VK_SHADER_STAGE_COMPUTE_BIT,							// stage
		*computeModuleBad,											// shader
		"main",
		DE_NULL,													// pSpecializationInfo
	};

	const VkComputePipelineCreateInfo		createInfoGood			=
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		DE_NULL,
		0u,															// flags
		shaderCreateInfoGood,										// cs
		*pipelineLayout,											// layout
		(vk::VkPipeline)0,											// basePipelineHandle
		0u,															// basePipelineIndex
	};

	const VkComputePipelineCreateInfo		createInfoBad			=
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		DE_NULL,
		0u,															// flags
		shaderCreateInfoBad,										// cs
		*pipelineLayout,											// descriptorSetLayout.get()
		(VkPipeline)0,												// basePipelineHandle
		0u,															// basePipelineIndex
	};

	const Unique<VkPipeline>				pipelineGood			(createComputePipeline(vk, device, (VkPipelineCache)0u, &createInfoGood));
	const Unique<VkPipeline>				pipelineBad				(createComputePipeline(vk, device, (VkPipelineCache)0u, &createInfoBad));

	const VkAccessFlags						inputBit				= (VK_ACCESS_UNIFORM_READ_BIT);
	const VkBufferMemoryBarrier				bufferBarriers[]		=
	{
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,
			VK_ACCESS_HOST_WRITE_BIT,									// srcAccessMask
			inputBit,													// dstAccessMask
			VK_QUEUE_FAMILY_IGNORED,									// srcQueueFamilyIndex
			VK_QUEUE_FAMILY_IGNORED,									// destQueueFamilyIndex
			*bufferA,													// buffer
			(VkDeviceSize)0u,											// offset
			(VkDeviceSize)bufferSizeA,									// size
		},
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,
			VK_ACCESS_HOST_WRITE_BIT,									// srcAccessMask
			inputBit,													// dstAccessMask
			VK_QUEUE_FAMILY_IGNORED,									// srcQueueFamilyIndex
			VK_QUEUE_FAMILY_IGNORED,									// destQueueFamilyIndex
			*bufferB,													// buffer
			(VkDeviceSize)0u,											// offset
			(VkDeviceSize)bufferSizeB,									// size
		}
	};

	const deUint32							numSrcBuffers			= 1u;

	const deUint32* const					dynamicOffsets			= (DE_NULL);
	const deUint32							numDynamicOffsets		= (0);
	const int								numPreBarriers			= numSrcBuffers;
	const vk::VkBufferMemoryBarrier* const	postBarriers			= result.getResultReadBarrier();
	const int								numPostBarriers			= 1;
	const tcu::Vec4							refQuadrantValue14		= (colorA2);
	const tcu::Vec4							refQuadrantValue23		= (colorA1);
	const tcu::Vec4							references[4]			=
	{
		refQuadrantValue14,
		refQuadrantValue23,
		refQuadrantValue23,
		refQuadrantValue14,
	};
	tcu::Vec4								results[4];

	// submit and wait begin

	const tcu::UVec3 numWorkGroups = tcu::UVec3(4, 1u, 1);

	const VkCommandPoolCreateInfo			cmdPoolCreateInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,					// sType;
		DE_NULL,													// pNext
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,						// flags
		queueFamilyIndex,											// queueFamilyIndex
	};
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, device, &cmdPoolCreateInfo));
	const VkCommandBufferAllocateInfo		cmdBufCreateInfo		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,				// sType
		DE_NULL,													// pNext
		*cmdPool,													// commandPool
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,							// level
		1u,															// bufferCount;
	};

	const VkCommandBufferBeginInfo			cmdBufBeginInfo			=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,				// sType
		DE_NULL,													// pNext
		0u,															// flags
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const Unique<VkFence>					cmdCompleteFence		(createFence(vk, device));
	const Unique<VkCommandBuffer>			cmd						(allocateCommandBuffer(vk, device, &cmdBufCreateInfo));

	VK_CHECK(vk.beginCommandBuffer(*cmd, &cmdBufBeginInfo));

	vk.cmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineBad);
	vk.cmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineGood);
	vk.cmdBindDescriptorSets(*cmd, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0, numDescriptorSets, descriptorSets, numDynamicOffsets, dynamicOffsets);

	if (numPreBarriers)
		vk.cmdPipelineBarrier(*cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0,
							  0, (const VkMemoryBarrier*)DE_NULL,
							  numPreBarriers, bufferBarriers,
							  0, (const VkImageMemoryBarrier*)DE_NULL);

	vk.cmdDispatch(*cmd, numWorkGroups.x(), numWorkGroups.y(), numWorkGroups.z());
	vk.cmdPipelineBarrier(*cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0,
						  0, (const VkMemoryBarrier*)DE_NULL,
						  numPostBarriers, postBarriers,
						  0, (const VkImageMemoryBarrier*)DE_NULL);
	VK_CHECK(vk.endCommandBuffer(*cmd));

	// run
	// submit second primary buffer, the secondary should be executed too
	const VkSubmitInfo						submitInfo				=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,								// sType
		DE_NULL,													// pNext
		0u,															// waitSemaphoreCount
		DE_NULL,													// pWaitSemaphores
		(const VkPipelineStageFlags*)DE_NULL,						// pWaitDstStageMask
		1,															// commandBufferCount
		&cmd.get(),													// pCommandBuffers
		0u,															// signalSemaphoreCount
		DE_NULL,													// pSignalSemaphores
	};
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *cmdCompleteFence));

	VK_CHECK(vk.waitForFences(device, 1u, &cmdCompleteFence.get(), 0u, INFINITE_TIMEOUT)); // \note: timeout is failure
	VK_CHECK(vk.resetFences(device, 1u, &cmdCompleteFence.get()));

	// submit and wait end
	result.readResultContentsTo(&results);

	// verify
	if (results[0] == references[0] &&
		results[1] == references[1] &&
		results[2] == references[2] &&
		results[3] == references[3])
	{
		return tcu::TestStatus::pass("Pass");
	}
	else if (results[0] == tcu::Vec4(-1.0f) &&
			 results[1] == tcu::Vec4(-1.0f) &&
			 results[2] == tcu::Vec4(-1.0f) &&
			 results[3] == tcu::Vec4(-1.0f))
	{
		context.getTestContext().getLog()
		<< tcu::TestLog::Message
		<< "Result buffer was not written to."
		<< tcu::TestLog::EndMessage;
		return tcu::TestStatus::fail("Result buffer was not written to");
	}
	else
	{
		context.getTestContext().getLog()
		<< tcu::TestLog::Message
		<< "Error expected ["
		<< references[0] << ", "
		<< references[1] << ", "
		<< references[2] << ", "
		<< references[3] << "], got ["
		<< results[0] << ", "
		<< results[1] << ", "
		<< results[2] << ", "
		<< results[3] << "]"
		<< tcu::TestLog::EndMessage;
		return tcu::TestStatus::fail("Invalid result values");
	}
}

// Shaders
void genComputeSource (SourceCollections& programCollection)
{
	const char* const						versionDecl				= glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	std::ostringstream						bufGood;

	bufGood << versionDecl << "\n"
	<< ""
	<< "layout(local_size_x = 1u, local_size_y = 1u, local_size_z = 1u) in;\n"
	<< "layout(set = 0, binding = 1u, std140) uniform BufferName\n"
	<< "{\n"
	<< "	highp vec4 colorA;\n"
	<< "	highp vec4 colorB;\n"
	<< "} b_instance;\n"
	<< "layout(set = 0, binding = 0, std140) writeonly buffer OutBuf\n"
	<< "{\n"
	<< "	highp vec4 read_colors[4];\n"
	<< "} b_out;\n"
	<< "void main(void)\n"
	<< "{\n"
	<< "	highp int quadrant_id = int(gl_WorkGroupID.x);\n"
	<< "	highp vec4 result_color;\n"
	<< "	if (quadrant_id == 1 || quadrant_id == 2)\n"
	<< "		result_color = b_instance.colorA;\n"
	<< "	else\n"
	<< "		result_color = b_instance.colorB;\n"
	<< "	b_out.read_colors[gl_WorkGroupID.x] = result_color;\n"
	<< "}\n";

	programCollection.glslSources.add("compute_good") << glu::ComputeSource(bufGood.str());

	std::ostringstream	bufBad;

	bufBad	<< versionDecl << "\n"
	<< ""
	<< "layout(local_size_x = 1u, local_size_y = 1u, local_size_z = 1u) in;\n"
	<< "layout(set = 0, binding = 1u, std140) uniform BufferName\n"
	<< "{\n"
	<< "	highp vec4 colorA;\n"
	<< "	highp vec4 colorB;\n"
	<< "} b_instance;\n"
	<< "layout(set = 0, binding = 0, std140) writeonly buffer OutBuf\n"
	<< "{\n"
	<< "	highp vec4 read_colors[4];\n"
	<< "} b_out;\n"
	<< "void main(void)\n"
	<< "{\n"
	<< "	highp int quadrant_id = int(gl_WorkGroupID.x);\n"
	<< "	highp vec4 result_color;\n"
	<< "	if (quadrant_id == 1 || quadrant_id == 2)\n"
	<< "		result_color = b_instance.colorA;\n"
	<< "	else\n"
	<< "		result_color = b_instance.colorB;\n"
	<< "	b_out.read_colors[gl_WorkGroupID.x] = vec4(0.0, 0.0, 0.0, 0.0);\n"
	<< "}\n";

	programCollection.glslSources.add("compute_bad") << glu::ComputeSource(bufBad.str());
}

void genComputeIncrementSource (SourceCollections& programCollection)
{
	const char* const						versionDecl = glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	std::ostringstream						bufIncrement;

	bufIncrement << versionDecl << "\n"
		<< ""
		<< "layout(local_size_x = 1u, local_size_y = 1u, local_size_z = 1u) in;\n"
		<< "layout(set = 0, binding = 0, std140) buffer InOutBuf\n"
		<< "{\n"
		<< "    coherent uint count;\n"
		<< "} b_in_out;\n"
		<< "void main(void)\n"
		<< "{\n"
		<< "	atomicAdd(b_in_out.count, 1u);\n"
		<< "}\n";

	programCollection.glslSources.add("compute_increment") << glu::ComputeSource(bufIncrement.str());
}

} // anonymous

tcu::TestCaseGroup* createCommandBuffersTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	commandBuffersTests	(new tcu::TestCaseGroup(testCtx, "command_buffers", "Command Buffers Tests"));

	/* 19.1. Command Pools (5.1 in VK 1.0 Spec) */
	addFunctionCase				(commandBuffersTests.get(), "pool_create_null_params",			"",	createPoolNullParamsTest);
	addFunctionCase				(commandBuffersTests.get(), "pool_create_non_null_allocator",	"",	createPoolNonNullAllocatorTest);
	addFunctionCase				(commandBuffersTests.get(), "pool_create_transient_bit",		"",	createPoolTransientBitTest);
	addFunctionCase				(commandBuffersTests.get(), "pool_create_reset_bit",			"",	createPoolResetBitTest);
	addFunctionCase				(commandBuffersTests.get(), "pool_reset_release_res",			"",	resetPoolReleaseResourcesBitTest);
	addFunctionCase				(commandBuffersTests.get(), "pool_reset_no_flags_res",			"",	resetPoolNoFlagsTest);
	addFunctionCase				(commandBuffersTests.get(), "pool_reset_reuse",					"",	resetPoolReuseTest);
	/* 19.2. Command Buffer Lifetime (5.2 in VK 1.0 Spec) */
	addFunctionCase				(commandBuffersTests.get(), "allocate_single_primary",			"", allocatePrimaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "allocate_many_primary",			"",	allocateManyPrimaryBuffersTest);
	addFunctionCase				(commandBuffersTests.get(), "allocate_single_secondary",		"", allocateSecondaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "allocate_many_secondary",			"", allocateManySecondaryBuffersTest);
	addFunctionCase				(commandBuffersTests.get(), "execute_small_primary",			"",	executePrimaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "execute_large_primary",			"",	executeLargePrimaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "reset_implicit",					"", resetBufferImplicitlyTest);
	addFunctionCase				(commandBuffersTests.get(), "trim_command_pool",				"", trimCommandPoolTest, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	addFunctionCase				(commandBuffersTests.get(), "trim_command_pool_secondary",		"", trimCommandPoolTest, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
	/* 19.3. Command Buffer Recording (5.3 in VK 1.0 Spec) */
	addFunctionCase				(commandBuffersTests.get(), "record_single_primary",			"",	recordSinglePrimaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "record_many_primary",				"", recordLargePrimaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "record_single_secondary",			"",	recordSingleSecondaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "record_many_secondary",			"", recordLargeSecondaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "submit_twice_primary",				"",	submitPrimaryBufferTwiceTest);
	addFunctionCase				(commandBuffersTests.get(), "submit_twice_secondary",			"",	submitSecondaryBufferTwiceTest);
	addFunctionCase				(commandBuffersTests.get(), "record_one_time_submit_primary",	"",	oneTimeSubmitFlagPrimaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "record_one_time_submit_secondary",	"",	oneTimeSubmitFlagSecondaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "render_pass_continue",				"",	renderPassContinueTest);
	addFunctionCase				(commandBuffersTests.get(), "record_simul_use_primary",			"",	simultaneousUsePrimaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "record_simul_use_secondary",		"",	simultaneousUseSecondaryBufferTest);
	addFunctionCaseWithPrograms (commandBuffersTests.get(), "record_simul_use_secondary_one_primary", "", genComputeIncrementSource, simultaneousUseSecondaryBufferOnePrimaryBufferTest);
	addFunctionCaseWithPrograms (commandBuffersTests.get(), "record_simul_use_secondary_two_primary", "", genComputeIncrementSource, simultaneousUseSecondaryBufferTwoPrimaryBuffersTest);
	addFunctionCase				(commandBuffersTests.get(), "record_query_precise_w_flag",		"",	recordBufferQueryPreciseWithFlagTest);
	addFunctionCase				(commandBuffersTests.get(), "record_query_imprecise_w_flag",	"",	recordBufferQueryImpreciseWithFlagTest);
	addFunctionCase				(commandBuffersTests.get(), "record_query_imprecise_wo_flag",	"",	recordBufferQueryImpreciseWithoutFlagTest);
	/* 19.4. Command Buffer Submission (5.4 in VK 1.0 Spec) */
	addFunctionCase				(commandBuffersTests.get(), "submit_count_non_zero",			"", submitBufferCountNonZero);
	addFunctionCase				(commandBuffersTests.get(), "submit_count_equal_zero",			"", submitBufferCountEqualZero);
	addFunctionCase				(commandBuffersTests.get(), "submit_wait_single_semaphore",		"", submitBufferWaitSingleSemaphore);
	addFunctionCase				(commandBuffersTests.get(), "submit_wait_many_semaphores",		"", submitBufferWaitManySemaphores);
	addFunctionCase				(commandBuffersTests.get(), "submit_null_fence",				"", submitBufferNullFence);
	/* 19.5. Secondary Command Buffer Execution (5.6 in VK 1.0 Spec) */
	addFunctionCase				(commandBuffersTests.get(), "secondary_execute",				"",	executeSecondaryBufferTest);
	addFunctionCase				(commandBuffersTests.get(), "secondary_execute_twice",			"",	executeSecondaryBufferTwiceTest);
	/* 19.6. Commands Allowed Inside Command Buffers (? in VK 1.0 Spec) */
	addFunctionCaseWithPrograms (commandBuffersTests.get(), "order_bind_pipeline",				"", genComputeSource, orderBindPipelineTest);

	return commandBuffersTests.release();
}

} // api
} // vkt

