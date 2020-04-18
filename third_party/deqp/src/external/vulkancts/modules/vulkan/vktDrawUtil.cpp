/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Google Inc.
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
 * \brief Utility for generating simple work
 *//*--------------------------------------------------------------------*/

#include "vktDrawUtil.hpp"
#include "rrMultisamplePixelBufferAccess.hpp"
#include "vkBufferWithMemory.hpp"
#include "vkImageWithMemory.hpp"
#include "vkTypeUtil.hpp"
#include "rrRenderer.hpp"
#include "rrRenderState.hpp"
#include "rrPrimitiveTypes.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuTestLog.hpp"
#include "deArrayUtil.hpp"
#include "vkBuilderUtil.hpp"

namespace vkt
{
namespace drawutil
{

using namespace de;
using namespace tcu;
using namespace vk;

static VkCompareOp mapCompareOp (rr::TestFunc compareFunc)
{
	switch (compareFunc)
	{
		case rr::TESTFUNC_NEVER:				return VK_COMPARE_OP_NEVER;
		case rr::TESTFUNC_LESS:					return VK_COMPARE_OP_LESS;
		case rr::TESTFUNC_EQUAL:				return VK_COMPARE_OP_EQUAL;
		case rr::TESTFUNC_LEQUAL:				return VK_COMPARE_OP_LESS_OR_EQUAL;
		case rr::TESTFUNC_GREATER:				return VK_COMPARE_OP_GREATER;
		case rr::TESTFUNC_NOTEQUAL:				return VK_COMPARE_OP_NOT_EQUAL;
		case rr::TESTFUNC_GEQUAL:				return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case rr::TESTFUNC_ALWAYS:				return VK_COMPARE_OP_ALWAYS;
		default:
			DE_ASSERT(false);
	}
	return VK_COMPARE_OP_LAST;
}

rr::PrimitiveType mapVkPrimitiveToRRPrimitive(const vk::VkPrimitiveTopology& primitiveTopology)
{
	static const rr::PrimitiveType primitiveTypeTable[] =
	{
		rr::PRIMITIVETYPE_POINTS,
		rr::PRIMITIVETYPE_LINES,
		rr::PRIMITIVETYPE_LINE_STRIP,
		rr::PRIMITIVETYPE_TRIANGLES,
		rr::PRIMITIVETYPE_TRIANGLE_STRIP,
		rr::PRIMITIVETYPE_TRIANGLE_FAN,
		rr::PRIMITIVETYPE_LINES_ADJACENCY,
		rr::PRIMITIVETYPE_LINE_STRIP_ADJACENCY,
		rr::PRIMITIVETYPE_TRIANGLES_ADJACENCY,
		rr::PRIMITIVETYPE_TRIANGLE_STRIP_ADJACENCY
	};

	return de::getSizedArrayElement<vk::VK_PRIMITIVE_TOPOLOGY_PATCH_LIST>(primitiveTypeTable, primitiveTopology);
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

Move<VkCommandPool> makeCommandPool (const DeviceInterface& vk, const VkDevice device, const deUint32 queueFamilyIndex)
{
	const VkCommandPoolCreateInfo info =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,			// VkStructureType			sType;
		DE_NULL,											// const void*				pNext;
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,	// VkCommandPoolCreateFlags	flags;
		queueFamilyIndex,									// deUint32					queueFamilyIndex;
	};
	return createCommandPool(vk, device, &info);
}

Move<VkCommandBuffer> makeCommandBuffer (const DeviceInterface& vk, const VkDevice device, const VkCommandPool commandPool)
{
	const VkCommandBufferAllocateInfo info =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,		// VkStructureType		sType;
		DE_NULL,											// const void*			pNext;
		commandPool,										// VkCommandPool		commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,					// VkCommandBufferLevel	level;
		1u,													// deUint32				commandBufferCount;
	};
	return allocateCommandBuffer(vk, device, &info);
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

Move<VkPipelineLayout> makePipelineLayout (const DeviceInterface&		vk,
										   const VkDevice				device,
										   const VkDescriptorSetLayout	descriptorSetLayout)
{
	const VkPipelineLayoutCreateInfo info =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType				sType;
		DE_NULL,											// const void*					pNext;
		(VkPipelineLayoutCreateFlags)0,						// VkPipelineLayoutCreateFlags	flags;
		1u,													// deUint32						setLayoutCount;
		&descriptorSetLayout,								// const VkDescriptorSetLayout*	pSetLayouts;
		0u,													// deUint32						pushConstantRangeCount;
		DE_NULL,											// const VkPushConstantRange*	pPushConstantRanges;
	};
	return createPipelineLayout(vk, device, &info);
}

Move<VkPipelineLayout> makePipelineLayoutWithoutDescriptors (const DeviceInterface&		vk,
															 const VkDevice				device)
{
	const VkPipelineLayoutCreateInfo info =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType				sType;
		DE_NULL,											// const void*					pNext;
		(VkPipelineLayoutCreateFlags)0,						// VkPipelineLayoutCreateFlags	flags;
		0u,													// deUint32						setLayoutCount;
		DE_NULL,											// const VkDescriptorSetLayout*	pSetLayouts;
		0u,													// deUint32						pushConstantRangeCount;
		DE_NULL,											// const VkPushConstantRange*	pPushConstantRanges;
	};
	return createPipelineLayout(vk, device, &info);
}

Move<VkImageView> makeImageView (const DeviceInterface&			vk,
								 const VkDevice					device,
								 const VkImage					image,
								 const VkImageViewType			viewType,
								 const VkFormat					format,
								 const VkImageSubresourceRange	subresourceRange)
{
	const VkImageViewCreateInfo imageViewParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		(VkImageViewCreateFlags)0,						// VkImageViewCreateFlags	flags;
		image,											// VkImage					image;
		viewType,										// VkImageViewType			viewType;
		format,											// VkFormat					format;
		makeComponentMappingRGBA(),						// VkComponentMapping		components;
		subresourceRange,								// VkImageSubresourceRange	subresourceRange;
	};
	return createImageView(vk, device, &imageViewParams);
}

VkBufferImageCopy makeBufferImageCopy (const VkImageSubresourceLayers	subresourceLayers,
									   const VkExtent3D					extent)
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

void beginCommandBuffer (const DeviceInterface& vk, const VkCommandBuffer commandBuffer)
{
	const VkCommandBufferBeginInfo info =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType							sType;
		DE_NULL,										// const void*								pNext;
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,	// VkCommandBufferUsageFlags				flags;
		DE_NULL,										// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
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
	const VkFenceCreateInfo fenceInfo =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		(VkFenceCreateFlags)0,					// VkFenceCreateFlags	flags;
	};
	const Unique<VkFence> fence(createFence(vk, device, &fenceInfo));

	const VkSubmitInfo submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType					sType;
		DE_NULL,							// const void*						pNext;
		0u,									// uint32_t							waitSemaphoreCount;
		DE_NULL,							// const VkSemaphore*				pWaitSemaphores;
		DE_NULL,							// const VkPipelineStageFlags*		pWaitDstStageMask;
		1u,									// uint32_t							commandBufferCount;
		&commandBuffer,						// const VkCommandBuffer*			pCommandBuffers;
		0u,									// uint32_t							signalSemaphoreCount;
		DE_NULL,							// const VkSemaphore*				pSignalSemaphores;
	};
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
	VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, ~0ull));
}

std::string getPrimitiveTopologyShortName (const VkPrimitiveTopology topology)
{
	std::string name(getPrimitiveTopologyName(topology));
	return de::toLower(name.substr(22));
}

DrawState::DrawState(const vk::VkPrimitiveTopology topology_, deUint32 renderWidth_, deUint32 renderHeight_)
	: topology				(topology_)
	, colorFormat			(VK_FORMAT_R8G8B8A8_UNORM)
	, renderSize			(tcu::UVec2(renderWidth_, renderHeight_))
	, depthClampEnable		(false)
	, depthTestEnable		(false)
	, depthWriteEnable		(false)
	, compareOp				(rr::TESTFUNC_LESS)
	, depthBoundsTestEnable	(false)
	, blendEnable			(false)
	, lineWidth				(1.0)
	, numPatchControlPoints	(0)
	, numSamples			(VK_SAMPLE_COUNT_1_BIT)
	, sampleShadingEnable	(false)
{
	DE_ASSERT(renderSize.x() != 0 && renderSize.y() != 0);
}

ReferenceDrawContext::~ReferenceDrawContext (void)
{
}

void ReferenceDrawContext::draw (void)
{
	m_refImage.setStorage(vk::mapVkFormat(m_drawState.colorFormat), m_drawState.renderSize.x(), m_drawState.renderSize.y());
	tcu::clear(m_refImage.getAccess(), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	{
		const rr::Program						program(&m_vertexShader, &m_fragmentShader);
		const rr::MultisamplePixelBufferAccess	referenceColorBuffer = rr::MultisamplePixelBufferAccess::fromSinglesampleAccess(m_refImage.getAccess());
		const rr::RenderTarget					renderTarget(referenceColorBuffer);
		const rr::RenderState					renderState((rr::ViewportState(referenceColorBuffer)), rr::VIEWPORTORIENTATION_UPPER_LEFT);
		const rr::Renderer						renderer;
		const rr::VertexAttrib					vertexAttrib[] =
		{
			rr::VertexAttrib(rr::VERTEXATTRIBTYPE_FLOAT, 4, sizeof(tcu::Vec4), 0, &m_drawCallData.vertices[0])
		};

		renderer.draw(rr::DrawCommand(	renderState,
										renderTarget,
										program,
										DE_LENGTH_OF_ARRAY(vertexAttrib),
										&vertexAttrib[0],
										rr::PrimitiveList(mapVkPrimitiveToRRPrimitive(m_drawState.topology), (int)m_drawCallData.vertices.size(), 0)));

	}

}

tcu::ConstPixelBufferAccess ReferenceDrawContext::getColorPixels (void) const
{
	return tcu::ConstPixelBufferAccess( m_refImage.getAccess().getFormat(),
										m_refImage.getAccess().getWidth(),
										m_refImage.getAccess().getHeight(),
										m_refImage.getAccess().getDepth(),
										m_refImage.getAccess().getDataPtr());
}

VulkanDrawContext::VulkanDrawContext ( Context&				context,
									  const DrawState&		drawState,
									  const DrawCallData&	drawCallData,
									  const VulkanProgram&	vulkanProgram)
	: DrawContext	(drawState, drawCallData)
	, m_context		(context)
	, m_program		(vulkanProgram)
{
	const DeviceInterface&	vk						= m_context.getDeviceInterface();
	const VkDevice			device					= m_context.getDevice();
	Allocator&				allocator				= m_context.getDefaultAllocator();
	VkImageSubresourceRange	colorSubresourceRange;
	Move<VkSampler>			sampler;

	// Command buffer
	{
		m_cmdPool			= makeCommandPool(vk, device, m_context.getUniversalQueueFamilyIndex());
		m_cmdBuffer			= makeCommandBuffer(vk, device, *m_cmdPool);
	}

	// Color attachment image
	{
		const VkImageUsageFlags usage			= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		colorSubresourceRange					= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
		const VkImageCreateInfo	imageCreateInfo	=
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,										// VkStructureType			sType;
			DE_NULL,																	// const void*				pNext;
			(VkImageCreateFlags)0,														// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,															// VkImageType				imageType;
			m_drawState.colorFormat,													// VkFormat					format;
			makeExtent3D(m_drawState.renderSize.x(), m_drawState.renderSize.y(), 1u),	// VkExtent3D				extent;
			1u,																			// uint32_t					mipLevels;
			1u,																			// uint32_t					arrayLayers;
			(VkSampleCountFlagBits)m_drawState.numSamples,								// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,													// VkImageTiling			tiling;
			usage,																		// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,													// VkSharingMode			sharingMode;
			0u,																			// uint32_t					queueFamilyIndexCount;
			DE_NULL,																	// const uint32_t*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED,													// VkImageLayout			initialLayout;
		};

		m_colorImage = MovePtr<ImageWithMemory>(new ImageWithMemory(vk, device, allocator, imageCreateInfo, MemoryRequirement::Any));
		m_colorImageView = makeImageView(vk, device, **m_colorImage, VK_IMAGE_VIEW_TYPE_2D, m_drawState.colorFormat, colorSubresourceRange);

		// Buffer to copy attachment data after rendering

		const VkDeviceSize bitmapSize = tcu::getPixelSize(mapVkFormat(m_drawState.colorFormat)) * m_drawState.renderSize.x() * m_drawState.renderSize.y();
		m_colorAttachmentBuffer = MovePtr<BufferWithMemory>(new BufferWithMemory(
			vk, device, allocator, makeBufferCreateInfo(bitmapSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible));

		{
			const Allocation& alloc = m_colorAttachmentBuffer->getAllocation();
			deMemset(alloc.getHostPtr(), 0, (size_t)bitmapSize);
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), bitmapSize);
		}
	}

	// Vertex buffer
	{
		const VkDeviceSize bufferSize = m_drawCallData.vertices.size() * sizeof(m_drawCallData.vertices[0]);
		m_vertexBuffer = MovePtr<BufferWithMemory>(new BufferWithMemory(
			vk, device, allocator, makeBufferCreateInfo(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible));

		const Allocation& alloc = m_vertexBuffer->getAllocation();
		deMemcpy(alloc.getHostPtr(), &m_drawCallData.vertices[0], (size_t)bufferSize);
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), bufferSize);
	}

	// bind descriptor sets
	{
		if (!vulkanProgram.descriptorSetLayout)
			m_pipelineLayout = makePipelineLayoutWithoutDescriptors(vk, device);
		else
			m_pipelineLayout = makePipelineLayout(vk, device, vulkanProgram.descriptorSetLayout);
	}

	// Renderpass
	{
		std::vector<VkAttachmentDescription> attachmentDescriptions;
		const VkAttachmentDescription attachDescriptors[] =
		{
			{
				(VkAttachmentDescriptionFlags)0,					// VkAttachmentDescriptionFlags		flags;
				m_drawState.colorFormat,							// VkFormat							format;
				(VkSampleCountFlagBits)m_drawState.numSamples,		// VkSampleCountFlagBits			samples;
				VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
				VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
				VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
				VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout					initialLayout;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					finalLayout;
			},
			{
				(VkAttachmentDescriptionFlags)0,					// VkAttachmentDescriptionFlags		flags
				m_drawState.depthFormat,							// VkFormat							format
				(VkSampleCountFlagBits)m_drawState.numSamples,		// VkSampleCountFlagBits			samples
				VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp
				VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp
				VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp
				VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout					initialLayout
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	// VkImageLayout					finalLayout

			}
		};

		const VkAttachmentReference attachmentReferences[] =
		{
			{
				0u,													// uint32_t			attachment
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout
			},
			{
				1u,													// uint32_t			attachment
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// VkImageLayout	layout
			},
			{
				VK_ATTACHMENT_UNUSED,								// deUint32         attachment;
				VK_IMAGE_LAYOUT_UNDEFINED							// VkImageLayout    layout;
			}
		};

		attachmentDescriptions.push_back(attachDescriptors[0]);
		if (!!vulkanProgram.depthImageView)
			attachmentDescriptions.push_back(attachDescriptors[1]);

		deUint32 depthReferenceNdx = !!vulkanProgram.depthImageView ? 1 : 2;
		const VkSubpassDescription subpassDescription =
		{
			(VkSubpassDescriptionFlags)0,						// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint				pipelineBindPoint;
			0u,													// deUint32							inputAttachmentCount;
			DE_NULL,											// const VkAttachmentReference*		pInputAttachments;
			1u,													// deUint32							colorAttachmentCount;
			&attachmentReferences[0],							// const VkAttachmentReference*		pColorAttachments;
			DE_NULL,											// const VkAttachmentReference*		pResolveAttachments;
			&attachmentReferences[depthReferenceNdx],			// const VkAttachmentReference*		pDepthStencilAttachment;
			0u,													// deUint32							preserveAttachmentCount;
			DE_NULL												// const deUint32*					pPreserveAttachments;
		};

		const VkRenderPassCreateInfo renderPassInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			(VkRenderPassCreateFlags)0,							// VkRenderPassCreateFlags			flags;
			(deUint32)attachmentDescriptions.size(),			// deUint32							attachmentCount;
			&attachmentDescriptions[0],							// const VkAttachmentDescription*	pAttachments;
			1u,													// deUint32							subpassCount;
			&subpassDescription,								// const VkSubpassDescription*		pSubpasses;
			0u,													// deUint32							dependencyCount;
			DE_NULL												// const VkSubpassDependency*		pDependencies;
		};

		m_renderPass = createRenderPass(vk, device, &renderPassInfo);
	}

	// Framebuffer
	{
		std::vector<VkImageView>	attachmentBindInfos;
		deUint32					numAttachments;
		attachmentBindInfos.push_back(*m_colorImageView);
		if (!!vulkanProgram.depthImageView)
			attachmentBindInfos.push_back(vulkanProgram.depthImageView);

		numAttachments = (deUint32)(attachmentBindInfos.size());
		const VkFramebufferCreateInfo framebufferInfo = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,		// VkStructureType						sType;
			DE_NULL,										// const void*							pNext;
			(VkFramebufferCreateFlags)0,					// VkFramebufferCreateFlags				flags;
			*m_renderPass,									// VkRenderPass							renderPass;
			numAttachments,									// uint32_t								attachmentCount;
			&attachmentBindInfos[0],						// const VkImageView*					pAttachments;
			m_drawState.renderSize.x(),						// uint32_t								width;
			m_drawState.renderSize.y(),						// uint32_t								height;
			1u,												// uint32_t								layers;
		};

		m_framebuffer = createFramebuffer(vk, device, &framebufferInfo);
	}

	// Graphics pipeline
	{
		const deUint32	vertexStride	= sizeof(Vec4);
		const VkFormat	vertexFormat	= VK_FORMAT_R32G32B32A32_SFLOAT;

		DE_ASSERT(m_drawState.topology != VK_PRIMITIVE_TOPOLOGY_PATCH_LIST || m_drawState.numPatchControlPoints > 0);

		const VkVertexInputBindingDescription bindingDesc =
		{
			0u,									// uint32_t				binding;
			vertexStride,						// uint32_t				stride;
			VK_VERTEX_INPUT_RATE_VERTEX,		// VkVertexInputRate	inputRate;
		};
		const VkVertexInputAttributeDescription attributeDesc =
		{
			0u,									// uint32_t			location;
			0u,									// uint32_t			binding;
			vertexFormat,						// VkFormat			format;
			0u,									// uint32_t			offset;
		};

		const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType								sType;
			DE_NULL,														// const void*									pNext;
			(VkPipelineVertexInputStateCreateFlags)0,						// VkPipelineVertexInputStateCreateFlags		flags;
			1u,																// uint32_t										vertexBindingDescriptionCount;
			&bindingDesc,													// const VkVertexInputBindingDescription*		pVertexBindingDescriptions;
			1u,																// uint32_t										vertexAttributeDescriptionCount;
			&attributeDesc,													// const VkVertexInputAttributeDescription*		pVertexAttributeDescriptions;
		};

		const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType								sType;
			DE_NULL,														// const void*									pNext;
			(VkPipelineInputAssemblyStateCreateFlags)0,						// VkPipelineInputAssemblyStateCreateFlags		flags;
			m_drawState.topology,											// VkPrimitiveTopology							topology;
			VK_FALSE,														// VkBool32										primitiveRestartEnable;
		};

		const VkPipelineTessellationStateCreateInfo pipelineTessellationStateInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,		// VkStructureType								sType;
			DE_NULL,														// const void*									pNext;
			(VkPipelineTessellationStateCreateFlags)0,						// VkPipelineTessellationStateCreateFlags		flags;
			m_drawState.numPatchControlPoints,								// uint32_t										patchControlPoints;
		};

		const VkViewport viewport = makeViewport(
			0.0f, 0.0f,
			static_cast<float>(m_drawState.renderSize.x()), static_cast<float>(m_drawState.renderSize.y()),
			0.0f, 1.0f);

		const VkRect2D scissor = {
			makeOffset2D(0, 0),
			makeExtent2D(m_drawState.renderSize.x(), m_drawState.renderSize.y()),
		};

		const VkPipelineViewportStateCreateInfo pipelineViewportStateInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,	// VkStructureType									sType;
			DE_NULL,												// const void*										pNext;
			(VkPipelineViewportStateCreateFlags)0,					// VkPipelineViewportStateCreateFlags				flags;
			1u,														// uint32_t											viewportCount;
			&viewport,												// const VkViewport*								pViewports;
			1u,														// uint32_t											scissorCount;
			&scissor,												// const VkRect2D*									pScissors;
		};

		const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			(VkPipelineRasterizationStateCreateFlags)0,						// VkPipelineRasterizationStateCreateFlags	flags;
			m_drawState.depthClampEnable,									// VkBool32									depthClampEnable;
			VK_FALSE,														// VkBool32									rasterizerDiscardEnable;
			VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
			VK_CULL_MODE_NONE,												// VkCullModeFlags							cullMode;
			VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
			VK_FALSE,														// VkBool32									depthBiasEnable;
			0.0f,															// float									depthBiasConstantFactor;
			0.0f,															// float									depthBiasClamp;
			0.0f,															// float									depthBiasSlopeFactor;
			m_drawState.lineWidth,											// float									lineWidth;
		};

		const VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			(VkPipelineMultisampleStateCreateFlags)0,					// VkPipelineMultisampleStateCreateFlags	flags;
			(VkSampleCountFlagBits)m_drawState.numSamples,				// VkSampleCountFlagBits					rasterizationSamples;
			m_drawState.sampleShadingEnable ? VK_TRUE : VK_FALSE,		// VkBool32									sampleShadingEnable;
			m_drawState.sampleShadingEnable ? 1.0f : 0.0f,				// float									minSampleShading;
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

		if (m_drawState.depthBoundsTestEnable && !context.getDeviceFeatures().depthBounds)
			TCU_THROW(NotSupportedError, "depthBounds not supported");

		const VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo =
		{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			(VkPipelineDepthStencilStateCreateFlags)0,					// VkPipelineDepthStencilStateCreateFlags	flags;
			m_drawState.depthTestEnable,								// VkBool32									depthTestEnable;
			m_drawState.depthWriteEnable,								// VkBool32									depthWriteEnable;
			mapCompareOp(m_drawState.compareOp),						// VkCompareOp								depthCompareOp;
			m_drawState.depthBoundsTestEnable,							// VkBool32									depthBoundsTestEnable
			VK_FALSE,													// VkBool32									stencilTestEnable;
			stencilOpState,												// VkStencilOpState							front;
			stencilOpState,												// VkStencilOpState							back;
			0.0f,														// float									minDepthBounds;
			1.0f,														// float									maxDepthBounds;
		};

		const VkColorComponentFlags colorComponentsAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		const VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState =
		{
			m_drawState.blendEnable,			// VkBool32					blendEnable;
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

		// Create shader stages

		std::vector<VkPipelineShaderStageCreateInfo>	shaderStages;
		VkShaderStageFlags								stageFlags = (VkShaderStageFlags)0;

		DE_ASSERT(m_program.shaders.size() <= MAX_NUM_SHADER_MODULES);
		for (deUint32 shaderNdx = 0; shaderNdx < m_program.shaders.size(); ++shaderNdx)
		{
			m_shaderModules[shaderNdx] = createShaderModule(vk, device, *m_program.shaders[shaderNdx].binary, (VkShaderModuleCreateFlags)0);

			const VkPipelineShaderStageCreateInfo pipelineShaderStageInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
				DE_NULL,												// const void*							pNext;
				(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags		flags;
				m_program.shaders[shaderNdx].stage,						// VkShaderStageFlagBits				stage;
				*m_shaderModules[shaderNdx],							// VkShaderModule						module;
				"main",													// const char*							pName;
				DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
			};

			shaderStages.push_back(pipelineShaderStageInfo);
			stageFlags |= m_program.shaders[shaderNdx].stage;
		}

		DE_ASSERT(
			(m_drawState.topology != VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ||
			(stageFlags & (VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)));

		const bool tessellationEnabled = (m_drawState.topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
		const VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,						// VkStructureType									sType;
			DE_NULL,																// const void*										pNext;
			(VkPipelineCreateFlags)0,												// VkPipelineCreateFlags							flags;
			static_cast<deUint32>(shaderStages.size()),								// deUint32											stageCount;
			&shaderStages[0],														// const VkPipelineShaderStageCreateInfo*			pStages;
			&vertexInputStateInfo,													// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
			&pipelineInputAssemblyStateInfo,										// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
			(tessellationEnabled ? &pipelineTessellationStateInfo : DE_NULL),		// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
			&pipelineViewportStateInfo,												// const VkPipelineViewportStateCreateInfo*			pViewportState;
			&pipelineRasterizationStateInfo,										// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
			&pipelineMultisampleStateInfo,											// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
			&pipelineDepthStencilStateInfo,											// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
			&pipelineColorBlendStateInfo,											// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
			DE_NULL,																// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
			*m_pipelineLayout,														// VkPipelineLayout									layout;
			*m_renderPass,															// VkRenderPass										renderPass;
			0u,																		// deUint32											subpass;
			DE_NULL,																// VkPipeline										basePipelineHandle;
			0,																		// deInt32											basePipelineIndex;
		};

		m_pipeline = createGraphicsPipeline(vk, device, DE_NULL, &graphicsPipelineInfo);
	}

	// Record commands
	{
		const VkDeviceSize zeroOffset = 0ull;

		beginCommandBuffer(vk, *m_cmdBuffer);
		if (!!vulkanProgram.descriptorSet)
			vk.cmdBindDescriptorSets(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipelineLayout, 0u, 1u, &vulkanProgram.descriptorSet, 0u, DE_NULL);

		// Begin render pass
		{
			std::vector<VkClearValue> clearValues;

			clearValues.push_back(makeClearValueColor(Vec4(0.0f, 0.0f, 0.0f, 1.0f)));
			if (!!vulkanProgram.depthImageView)
				clearValues.push_back(makeClearValueDepthStencil(0.0, 0));

			const VkRect2D		renderArea =
			{
				makeOffset2D(0, 0),
				makeExtent2D(m_drawState.renderSize.x(), m_drawState.renderSize.y())
			};

			const VkRenderPassBeginInfo renderPassBeginInfo = {
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,							// VkStructureType								sType;
				DE_NULL,															// const void*									pNext;
				*m_renderPass,														// VkRenderPass									renderPass;
				*m_framebuffer,														// VkFramebuffer								framebuffer;
				renderArea,															// VkRect2D										renderArea;
				static_cast<deUint32>(clearValues.size()),							// uint32_t										clearValueCount;
				&clearValues[0],													// const VkClearValue*							pClearValues;
			};

			vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}

		vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		vk.cmdBindVertexBuffers(*m_cmdBuffer, 0u, 1u, &(**m_vertexBuffer), &zeroOffset);

		vk.cmdDraw(*m_cmdBuffer, static_cast<deUint32>(m_drawCallData.vertices.size()), 1u, 0u, 0u);
		vk.cmdEndRenderPass(*m_cmdBuffer);

		// Barrier: draw -> copy from image
		{
			const VkImageMemoryBarrier barrier = makeImageMemoryBarrier(
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				**m_colorImage, colorSubresourceRange);

			vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0,
				0u, DE_NULL, 0u, DE_NULL, 1u, &barrier);
		}

		// Resolve multisample image
		{
			if (m_drawState.numSamples != VK_SAMPLE_COUNT_1_BIT)
			{
				const VkImageResolve imageResolve =
				{
					makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u),
					{ 0, 0, 0},
					makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u),
					{ 0, 0, 0},
					makeExtent3D(m_drawState.renderSize.x(), m_drawState.renderSize.y(), 1u)
				};

				const VkImageCreateInfo resolveImageCreateInfo =
				{
					VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,				// VkStructureType			sType
					DE_NULL,											// const void*				pNext
					(VkImageCreateFlags)0,								// VkImageCreateFlags		flags
					VK_IMAGE_TYPE_2D,									// VkImageType				imageType
					m_drawState.colorFormat,							// VkFormat					format
					makeExtent3D(m_drawState.renderSize.x(),			// VkExtent3D				extent;
							m_drawState.renderSize.y(), 1u),
					1u,													// uint32_t					mipLevels
					1u,													// uint32_t					arrayLayers
					VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits	samples
					VK_IMAGE_TILING_OPTIMAL,							// VkImaageTiling			tiling
					VK_IMAGE_USAGE_TRANSFER_DST_BIT |					// VkImageUsageFlags		usage
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
					VK_SHARING_MODE_EXCLUSIVE,							// VkSharingModeExclusive	sharingMode
					0u,													// uint32_t					queueFamilyIndexCount
					DE_NULL,											// const uint32_t*			pQueueFamilyIndices
					VK_IMAGE_LAYOUT_UNDEFINED							// VkImageLayout			initialLayout
				};

				m_resolveImage = MovePtr<ImageWithMemory>(new ImageWithMemory(vk, device, allocator, resolveImageCreateInfo, MemoryRequirement::Any));

				const VkImageMemoryBarrier resolveBarrier = makeImageMemoryBarrier(
						0u, VK_ACCESS_TRANSFER_READ_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						**m_resolveImage, colorSubresourceRange);

				vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0,
						0u, DE_NULL, 0u, DE_NULL, 1u, &resolveBarrier);

				vk.cmdResolveImage(*m_cmdBuffer, **m_colorImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						**m_resolveImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &imageResolve);

				const VkImageMemoryBarrier barrier = makeImageMemoryBarrier(
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					**m_resolveImage, colorSubresourceRange);

				vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0,
					0u, DE_NULL, 0u, DE_NULL, 1u, &barrier);
			}
			else
				m_resolveImage = m_colorImage;

			const VkBufferImageCopy copyRegion = makeBufferImageCopy(makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u),
					makeExtent3D(m_drawState.renderSize.x(), m_drawState.renderSize.y(), 1u));
			vk.cmdCopyImageToBuffer(*m_cmdBuffer, **m_resolveImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, **m_colorAttachmentBuffer, 1u, &copyRegion);
		}

		// Barrier: copy to buffer -> host read
		{
			const VkBufferMemoryBarrier barrier = makeBufferMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
				**m_colorAttachmentBuffer, 0ull, VK_WHOLE_SIZE);

			vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0,
				0u, DE_NULL, 1u, &barrier, 0u, DE_NULL);
		}

		endCommandBuffer(vk, *m_cmdBuffer);
	}
}

VulkanDrawContext::~VulkanDrawContext (void)
{
}

void VulkanDrawContext::draw (void)
{
	const DeviceInterface&	vk			= m_context.getDeviceInterface();
	const VkDevice			device		= m_context.getDevice();
	const VkQueue			queue		= m_context.getUniversalQueue();
	tcu::TestLog&			log			= m_context.getTestContext().getLog();

	submitCommandsAndWait(vk, device, queue, *m_cmdBuffer);

	log << tcu::LogImageSet("attachments", "") << tcu::LogImage("color0", "", getColorPixels()) << tcu::TestLog::EndImageSet;
}

tcu::ConstPixelBufferAccess VulkanDrawContext::getColorPixels (void) const
{
	const DeviceInterface&	vk			= m_context.getDeviceInterface();
	const VkDevice			device		= m_context.getDevice();

	const Allocation& alloc = m_colorAttachmentBuffer->getAllocation();
	invalidateMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), VK_WHOLE_SIZE);

	return tcu::ConstPixelBufferAccess(mapVkFormat(m_drawState.colorFormat), m_drawState.renderSize.x(), m_drawState.renderSize.y(), 1u, alloc.getHostPtr());
}
} // drawutil
} // vkt
