/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
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
 * \brief Platform Synchronization tests
 *//*--------------------------------------------------------------------*/

#include "vktSynchronizationSmokeTests.hpp"

#include "vktTestCaseUtil.hpp"

#include "vkPlatform.hpp"
#include "vkStrUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkDeviceUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"

#include "deUniquePtr.hpp"
#include "deThread.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkPrograms.hpp"
#include "vkTypeUtil.hpp"

#include <limits>

namespace vkt
{
namespace synchronization
{

using namespace vk;
using namespace tcu;

namespace
{

using std::vector;
using std::string;
using tcu::TestLog;
using de::UniquePtr;
using de::MovePtr;

static const deUint64 DEFAULT_TIMEOUT = 2ull*1000*1000*1000; //!< 2 seconds in nanoseconds

void buildShaders (SourceCollections& shaderCollection)
{
	shaderCollection.glslSources.add("glslvert") <<
		glu::VertexSource(
				"#version 310 es\n"
				"precision mediump float;\n"
				"layout (location = 0) in vec4 vertexPosition;\n"
				"void main()\n"
				"{\n"
				"	gl_Position = vertexPosition;\n"
				"}\n");

	shaderCollection.glslSources.add("glslfrag") <<
		glu::FragmentSource(
				"#version 310 es\n"
				"precision mediump float;\n"
				"layout (location = 0) out vec4 outputColor;\n"
				"void main()\n"
				"{\n"
				"	outputColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"}\n");
}

Move<VkDevice> createTestDevice (const InstanceInterface& vki, VkPhysicalDevice physicalDevice, deUint32 *outQueueFamilyIndex)
{
	VkDeviceQueueCreateInfo		queueInfo;
	VkDeviceCreateInfo			deviceInfo;
	size_t						queueNdx;
	const deUint32				queueCount					= 2u;
	const float					queuePriority[queueCount]	= { 1.0f, 1.0f };

	const vector<VkQueueFamilyProperties>	queueProps				= getPhysicalDeviceQueueFamilyProperties(vki, physicalDevice);
	const VkPhysicalDeviceFeatures			physicalDeviceFeatures	= getPhysicalDeviceFeatures(vki, physicalDevice);

	for (queueNdx = 0; queueNdx < queueProps.size(); queueNdx++)
	{
		if ((queueProps[queueNdx].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT && (queueProps[queueNdx].queueCount >= queueCount))
			break;
	}

	if (queueNdx >= queueProps.size())
	{
		// No queue family index found
		std::ostringstream msg;
		msg << "Cannot create device with " << queueCount << " graphics queues";

		throw tcu::NotSupportedError(msg.str());
	}

	deMemset(&queueInfo,	0, sizeof(queueInfo));
	deMemset(&deviceInfo,	0, sizeof(deviceInfo));

	deMemset(&queueInfo, 0xcd, sizeof(queueInfo));
	queueInfo.sType							= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.pNext							= DE_NULL;
	queueInfo.flags							= (VkDeviceQueueCreateFlags)0u;
	queueInfo.queueFamilyIndex				= (deUint32)queueNdx;
	queueInfo.queueCount					= queueCount;
	queueInfo.pQueuePriorities				= queuePriority;

	deMemset(&deviceInfo, 0xcd, sizeof(deviceInfo));
	deviceInfo.sType						= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext						= DE_NULL;
	deviceInfo.flags						= (VkDeviceCreateFlags)0u;
	deviceInfo.queueCreateInfoCount			= 1u;
	deviceInfo.pQueueCreateInfos			= &queueInfo;
	deviceInfo.enabledExtensionCount		= 0u;
	deviceInfo.ppEnabledExtensionNames		= DE_NULL;
	deviceInfo.enabledLayerCount			= 0u;
	deviceInfo.ppEnabledLayerNames			= DE_NULL;
	deviceInfo.pEnabledFeatures				= &physicalDeviceFeatures;

	*outQueueFamilyIndex					= queueInfo.queueFamilyIndex;

	return createDevice(vki, physicalDevice, &deviceInfo);
};

struct BufferParameters
{
	const void*						memory;
	VkDeviceSize					size;
	VkBufferUsageFlags				usage;
	VkSharingMode					sharingMode;
	deUint32						queueFamilyCount;
	const deUint32*					queueFamilyIndex;
	VkAccessFlags					inputBarrierFlags;
};

struct Buffer
{
	MovePtr<Allocation>				allocation;
	vector<VkMemoryBarrier>			memoryBarrier;
	vk::Move<VkBuffer>				buffer;
};

void createVulkanBuffer (const DeviceInterface& vkd, VkDevice device, Allocator& allocator, const BufferParameters& bufferParameters, Buffer& buffer, MemoryRequirement visibility)
{
	VkBufferCreateInfo	bufferCreateParams;

	deMemset(&bufferCreateParams, 0xcd, sizeof(bufferCreateParams));
	bufferCreateParams.sType					= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateParams.pNext					= DE_NULL;
	bufferCreateParams.flags					= 0;
	bufferCreateParams.size						= bufferParameters.size;
	bufferCreateParams.usage					= bufferParameters.usage;
	bufferCreateParams.sharingMode				= bufferParameters.sharingMode;
	bufferCreateParams.queueFamilyIndexCount	= bufferParameters.queueFamilyCount;
	bufferCreateParams.pQueueFamilyIndices		= bufferParameters.queueFamilyIndex;

	buffer.buffer		= createBuffer(vkd, device, &bufferCreateParams);
	buffer.allocation	= allocator.allocate(getBufferMemoryRequirements(vkd, device, *buffer.buffer), visibility);

	VK_CHECK(vkd.bindBufferMemory(device, *buffer.buffer, buffer.allocation->getMemory(), buffer.allocation->getOffset()));

	// If caller provides a host memory buffer for the allocation, then go
	// ahead and copy the provided data into the allocation and update the
	// barrier list with the associated access
	if (bufferParameters.memory != DE_NULL)
	{
		VkMemoryBarrier				barrier;
		VkMappedMemoryRange			range;

		deMemset(&range, 0xcd, sizeof(range));
		range.sType		= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.pNext		= DE_NULL;
		range.memory	= buffer.allocation->getMemory();
		range.offset	= buffer.allocation->getOffset();
		range.size		= bufferParameters.size;

		deMemcpy(buffer.allocation->getHostPtr(), bufferParameters.memory, (size_t)bufferParameters.size);
		VK_CHECK(vkd.flushMappedMemoryRanges(device, 1, &range));

		deMemset(&barrier, 0xcd, sizeof(barrier));
		barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.pNext			= DE_NULL;
		barrier.srcAccessMask	= VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask	= bufferParameters.inputBarrierFlags;

		buffer.memoryBarrier.push_back(barrier);
	}
}

struct ImageParameters
{
	VkImageType							imageType;
	VkFormat							format;
	VkExtent3D							extent3D;
	deUint32							mipLevels;
	VkSampleCountFlagBits				samples;
	VkImageTiling						tiling;
	VkBufferUsageFlags					usage;
	VkSharingMode						sharingMode;
	deUint32							queueFamilyCount;
	const deUint32*						queueFamilyNdxList;
	VkImageLayout						initialLayout;
	VkImageLayout						finalLayout;
	VkAccessFlags						barrierInputMask;
};

struct Image
{
	vk::Move<VkImage>					image;
	vk::Move<VkImageView>				imageView;
	MovePtr<Allocation>					allocation;
	vector<VkImageMemoryBarrier>		imageMemoryBarrier;
};

void createVulkanImage (const DeviceInterface& vkd, VkDevice device, Allocator& allocator, const ImageParameters& imageParameters, Image& image, MemoryRequirement visibility)
{
	VkComponentMapping			componentMap;
	VkImageSubresourceRange		subresourceRange;
	VkImageViewCreateInfo		imageViewCreateInfo;
	VkImageCreateInfo			imageCreateParams;
	VkImageMemoryBarrier		imageBarrier;

	deMemset(&imageCreateParams, 0xcd, sizeof(imageCreateParams));
	imageCreateParams.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateParams.pNext					= DE_NULL;
	imageCreateParams.flags					= 0;
	imageCreateParams.imageType				= imageParameters.imageType;
	imageCreateParams.format				= imageParameters.format;
	imageCreateParams.extent				= imageParameters.extent3D;
	imageCreateParams.mipLevels				= imageParameters.mipLevels;
	imageCreateParams.arrayLayers			= 1;
	imageCreateParams.samples				= imageParameters.samples;
	imageCreateParams.tiling				= imageParameters.tiling;
	imageCreateParams.usage					= imageParameters.usage;
	imageCreateParams.sharingMode			= imageParameters.sharingMode;
	imageCreateParams.queueFamilyIndexCount	= imageParameters.queueFamilyCount;
	imageCreateParams.pQueueFamilyIndices	= imageParameters.queueFamilyNdxList;
	imageCreateParams.initialLayout			= imageParameters.initialLayout;

	image.image			= createImage(vkd, device, &imageCreateParams);
	image.allocation	= allocator.allocate(getImageMemoryRequirements(vkd, device, *image.image), visibility);

	VK_CHECK(vkd.bindImageMemory(device, *image.image, image.allocation->getMemory(), image.allocation->getOffset()));

	componentMap.r							= VK_COMPONENT_SWIZZLE_R;
	componentMap.g							= VK_COMPONENT_SWIZZLE_G;
	componentMap.b							= VK_COMPONENT_SWIZZLE_B;
	componentMap.a							= VK_COMPONENT_SWIZZLE_A;

	subresourceRange.aspectMask				= VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel			= 0;
	subresourceRange.levelCount				= imageParameters.mipLevels;
	subresourceRange.baseArrayLayer			= 0;
	subresourceRange.layerCount				= 1;

	deMemset(&imageViewCreateInfo, 0xcd, sizeof(imageViewCreateInfo));
	imageViewCreateInfo.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext				= DE_NULL;
	imageViewCreateInfo.flags				= 0;
	imageViewCreateInfo.image				= image.image.get();
	imageViewCreateInfo.viewType			= VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format				= imageParameters.format;
	imageViewCreateInfo.components			= componentMap;
	imageViewCreateInfo.subresourceRange	= subresourceRange;

	image.imageView	= createImageView(vkd, device, &imageViewCreateInfo);

	deMemset(&imageBarrier, 0xcd, sizeof(imageBarrier));
	imageBarrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.pNext					= DE_NULL;
	imageBarrier.srcAccessMask			= 0;
	imageBarrier.dstAccessMask			= imageParameters.barrierInputMask;
	imageBarrier.oldLayout				= imageParameters.initialLayout;
	imageBarrier.newLayout				= imageParameters.finalLayout;
	imageBarrier.srcQueueFamilyIndex	= imageParameters.queueFamilyNdxList[0];
	imageBarrier.dstQueueFamilyIndex	= imageParameters.queueFamilyNdxList[imageParameters.queueFamilyCount-1];
	imageBarrier.image					= image.image.get();
	imageBarrier.subresourceRange		= subresourceRange;

	image.imageMemoryBarrier.push_back(imageBarrier);
}

struct RenderPassParameters
{
	VkFormat				colorFormat;
	VkSampleCountFlagBits	colorSamples;
};

void  createColorOnlyRenderPass (const DeviceInterface& vkd, VkDevice device, const RenderPassParameters& renderPassParameters, vk::Move<VkRenderPass>& renderPass)
{
	VkAttachmentDescription				colorAttachmentDesc;
	VkAttachmentReference				colorAttachmentRef;
	VkAttachmentReference				stencilAttachmentRef;
	VkSubpassDescription				subpassDesc;
	VkRenderPassCreateInfo				renderPassParams;
	VkRenderPass						newRenderPass;

	colorAttachmentDesc.flags			= 0;
	colorAttachmentDesc.format			= renderPassParameters.colorFormat;
	colorAttachmentDesc.samples			= renderPassParameters.colorSamples;
	colorAttachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDesc.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentDesc.finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	colorAttachmentRef.attachment		= 0;
	colorAttachmentRef.layout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	stencilAttachmentRef.attachment		= VK_ATTACHMENT_UNUSED;
	stencilAttachmentRef.layout			= VK_IMAGE_LAYOUT_UNDEFINED;

	subpassDesc.flags					= 0;
	subpassDesc.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.inputAttachmentCount	= 0;
	subpassDesc.pInputAttachments		= DE_NULL;
	subpassDesc.colorAttachmentCount	= 1;
	subpassDesc.pColorAttachments		= &colorAttachmentRef;
	subpassDesc.pResolveAttachments		= DE_NULL;
	subpassDesc.pDepthStencilAttachment	= &stencilAttachmentRef;
	subpassDesc.preserveAttachmentCount	= 0;
	subpassDesc.pPreserveAttachments	= DE_NULL;

	deMemset(&renderPassParams, 0xcd, sizeof(renderPassParams));
	renderPassParams.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassParams.pNext				= DE_NULL;
	renderPassParams.flags				= 0;
	renderPassParams.attachmentCount	= 1;
	renderPassParams.pAttachments		= &colorAttachmentDesc;
	renderPassParams.subpassCount		= 1;
	renderPassParams.pSubpasses			= &subpassDesc;
	renderPassParams.dependencyCount	= 0;
	renderPassParams.pDependencies		= DE_NULL;

	renderPass = createRenderPass(vkd, device, &renderPassParams);
}

struct ShaderDescParams
{
	VkShaderModule			shaderModule;
	VkShaderStageFlagBits	stage;
};

struct VertexDesc
{
	deUint32	location;
	VkFormat	format;
	deUint32	stride;
	deUint32	offset;
};

void createVertexInfo (const vector<VertexDesc>& vertexDesc, vector<VkVertexInputBindingDescription>& bindingList, vector<VkVertexInputAttributeDescription>& attrList, VkPipelineVertexInputStateCreateInfo& vertexInputState)
{
	for (vector<VertexDesc>::const_iterator vertDescIter = vertexDesc.begin(); vertDescIter != vertexDesc.end(); vertDescIter++)
	{
		deUint32							bindingId = 0;
		VkVertexInputBindingDescription		bindingDesc;
		VkVertexInputAttributeDescription	attrDesc;

		bindingDesc.binding		= bindingId;
		bindingDesc.stride		= vertDescIter->stride;
		bindingDesc.inputRate	= VK_VERTEX_INPUT_RATE_VERTEX;
		bindingList.push_back(bindingDesc);

		attrDesc.location		= vertDescIter->location;
		attrDesc.binding		= bindingId;
		attrDesc.format			= vertDescIter->format;
		attrDesc.offset			= vertDescIter->offset;
		attrList.push_back(attrDesc);

		bindingId++;
	}

	deMemset(&vertexInputState, 0xcd, sizeof(vertexInputState));
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.pNext = DE_NULL;
	vertexInputState.flags = 0u;
	vertexInputState.vertexBindingDescriptionCount = (deUint32)bindingList.size();
	vertexInputState.pVertexBindingDescriptions = &bindingList[0];
	vertexInputState.vertexAttributeDescriptionCount = (deUint32)attrList.size();
	vertexInputState.pVertexAttributeDescriptions = &attrList[0];
}

void createCommandBuffer (const DeviceInterface& deviceInterface, const VkDevice device, const deUint32 queueFamilyNdx, vk::Move<VkCommandBuffer>* commandBufferRef, vk::Move<VkCommandPool>* commandPoolRef)
{
	vk::Move<VkCommandPool>		commandPool;
	VkCommandBufferAllocateInfo	commandBufferInfo;
	VkCommandBuffer				commandBuffer;

	commandPool = createCommandPool(deviceInterface, device, 0u, queueFamilyNdx);

	deMemset(&commandBufferInfo, 0xcd, sizeof(commandBufferInfo));
	commandBufferInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferInfo.pNext					= DE_NULL;
	commandBufferInfo.commandPool			= commandPool.get();
	commandBufferInfo.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferInfo.commandBufferCount	= 1;

	VK_CHECK(deviceInterface.allocateCommandBuffers(device, &commandBufferInfo, &commandBuffer));
	*commandBufferRef = vk::Move<VkCommandBuffer>(vk::check<VkCommandBuffer>(commandBuffer), Deleter<VkCommandBuffer>(deviceInterface, device, commandPool.get()));
	*commandPoolRef = commandPool;
}

void createFences (const DeviceInterface& deviceInterface, VkDevice device, bool signaled, deUint32 numFences, VkFence* fence)
{
	VkFenceCreateInfo		fenceState;
	VkFenceCreateFlags		signalFlag = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	deMemset(&fenceState, 0xcd, sizeof(fenceState));
	fenceState.sType		= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceState.pNext		= DE_NULL;
	fenceState.flags		= signalFlag;

	for (deUint32 ndx = 0; ndx < numFences; ndx++)
		VK_CHECK(deviceInterface.createFence(device, &fenceState, DE_NULL, &fence[ndx]));
}

void destroyFences (const DeviceInterface& deviceInterface, VkDevice device, deUint32 numFences, VkFence* fence)
{
	for (deUint32 ndx = 0; ndx < numFences; ndx++)
		deviceInterface.destroyFence(device, fence[ndx], DE_NULL);
}

struct RenderInfo
{
	deInt32							width;
	deInt32							height;
	deUint32						vertexBufferSize;
	VkBuffer						vertexBuffer;
	VkImage							image;
	VkCommandBuffer					commandBuffer;
	VkRenderPass					renderPass;
	VkFramebuffer					framebuffer;
	VkPipeline						pipeline;
	deUint32						mipLevels;
	const deUint32*					queueFamilyNdxList;
	deUint32						queueFamilyNdxCount;
	bool							waitEvent;
	VkEvent							event;
	vector<VkImageMemoryBarrier>*	barriers;
};

void  recordRenderPass (const DeviceInterface& deviceInterface, const RenderInfo& renderInfo)
{
	const VkDeviceSize					bindingOffset			= 0;
	const VkClearValue					clearValue				= makeClearValueColorF32(0.0, 0.0, 1.0, 1.0);
	VkRenderPassBeginInfo				renderPassBeginState;
	VkImageMemoryBarrier				renderBarrier;

	deMemset(&renderPassBeginState, 0xcd, sizeof(renderPassBeginState));
	renderPassBeginState.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginState.pNext						= DE_NULL;
	renderPassBeginState.renderPass					= renderInfo.renderPass;
	renderPassBeginState.framebuffer				= renderInfo.framebuffer;
	renderPassBeginState.renderArea.offset.x		= 0;
	renderPassBeginState.renderArea.offset.y		= 0;
	renderPassBeginState.renderArea.extent.width	= renderInfo.width;
	renderPassBeginState.renderArea.extent.height	= renderInfo.height;
	renderPassBeginState.clearValueCount			= 1;
	renderPassBeginState.pClearValues				= &clearValue;

	deviceInterface.cmdBeginRenderPass(renderInfo.commandBuffer, &renderPassBeginState, VK_SUBPASS_CONTENTS_INLINE);
	if (renderInfo.waitEvent)
		deviceInterface.cmdWaitEvents(renderInfo.commandBuffer, 1, &renderInfo.event, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, DE_NULL, 0, DE_NULL, 0, DE_NULL);
	deviceInterface.cmdBindPipeline(renderInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderInfo.pipeline);
	deviceInterface.cmdBindVertexBuffers(renderInfo.commandBuffer, 0u, 1u, &renderInfo.vertexBuffer, &bindingOffset);
	deviceInterface.cmdDraw(renderInfo.commandBuffer, renderInfo.vertexBufferSize, 1, 0, 0);
	deviceInterface.cmdEndRenderPass(renderInfo.commandBuffer);

	deMemset(&renderBarrier, 0xcd, sizeof(renderBarrier));
	renderBarrier.sType								= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	renderBarrier.pNext								= DE_NULL;
	renderBarrier.srcAccessMask						= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	renderBarrier.dstAccessMask						= VK_ACCESS_TRANSFER_READ_BIT;
	renderBarrier.oldLayout							= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	renderBarrier.newLayout							= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	renderBarrier.srcQueueFamilyIndex				= renderInfo.queueFamilyNdxList[0];
	renderBarrier.dstQueueFamilyIndex				= renderInfo.queueFamilyNdxList[renderInfo.queueFamilyNdxCount-1];
	renderBarrier.image								= renderInfo.image;
	renderBarrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	renderBarrier.subresourceRange.baseMipLevel		= 0;
	renderBarrier.subresourceRange.levelCount		= renderInfo.mipLevels;
	renderBarrier.subresourceRange.baseArrayLayer	= 0;
	renderBarrier.subresourceRange.layerCount		= 1;
	renderInfo.barriers->push_back(renderBarrier);
}

struct TransferInfo
{
	VkCommandBuffer					commandBuffer;
	deUint32						width;
	deUint32						height;
	VkImage							image;
	VkBuffer						buffer;
	VkDeviceSize					size;
	deUint32						mipLevel;
	VkOffset3D						imageOffset;
	vector<VkBufferMemoryBarrier>*	barriers;
};

void copyToCPU (const DeviceInterface& vkd, TransferInfo* transferInfo)
{
	VkBufferImageCopy	copyState;

	copyState.bufferOffset						= 0;
	copyState.bufferRowLength					= transferInfo->width;
	copyState.bufferImageHeight					= transferInfo->height;
	copyState.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	copyState.imageSubresource.mipLevel			= transferInfo->mipLevel;
	copyState.imageSubresource.baseArrayLayer	= 0;
	copyState.imageSubresource.layerCount		= 1;
	copyState.imageOffset						= transferInfo->imageOffset;
	copyState.imageExtent.width					= (deInt32)(transferInfo->width);
	copyState.imageExtent.height				= (deInt32)(transferInfo->height);
	copyState.imageExtent.depth					= 1;

	vkd.cmdCopyImageToBuffer(transferInfo->commandBuffer, transferInfo->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, transferInfo->buffer, 1, &copyState);

	{
		VkBufferMemoryBarrier	bufferBarrier;
		deMemset(&bufferBarrier, 0xcd, sizeof(bufferBarrier));
		bufferBarrier.sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.pNext					= DE_NULL;
		bufferBarrier.srcAccessMask			= VK_ACCESS_TRANSFER_WRITE_BIT;
		bufferBarrier.dstAccessMask			= VK_ACCESS_HOST_READ_BIT;
		bufferBarrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.buffer				= transferInfo->buffer;
		bufferBarrier.offset				= 0;
		bufferBarrier.size					= transferInfo->size;
		transferInfo->barriers->push_back(bufferBarrier);
	}
}

struct TestContext
{
	const DeviceInterface&		vkd;
	const VkDevice				device;
	const deUint32				queueFamilyIndex;
	const BinaryCollection&		binaryCollection;
	Allocator&					allocator;

	const tcu::Vec4*			vertices;
	deUint32					numVertices;
	tcu::IVec2					renderDimension;
	VkFence						fences[2];
	VkDeviceSize				renderSize;
	MovePtr<Allocation>			renderReadBuffer;
	MovePtr<Allocation>			vertexBufferAllocation;
	vk::Move<VkBuffer>			vertexBuffer;
	vk::Move<VkBuffer>			renderBuffer;
	bool						waitEvent;
	VkEvent						event;
	vk::Move<VkImage>			image;
	vk::Move<VkImageView>		imageView;
	vk::Move<VkFramebuffer>		framebuffer;
	vk::Move<VkCommandPool>		commandPool;
	vk::Move<VkCommandBuffer>	cmdBuffer;
	vk::Move<VkRenderPass>		renderPass;
	vk::Move<VkPipelineCache>	pipelineCache;
	vk::Move<VkPipeline>		pipeline;
	MovePtr<Allocation>			imageAllocation;

	TestContext (const DeviceInterface&		vkd_,
				 const VkDevice				device_,
				 deUint32					queueFamilyIndex_,
				 const BinaryCollection&	binaryCollection_,
				 Allocator&					allocator_)
		: vkd				(vkd_)
		, device			(device_)
		, queueFamilyIndex	(queueFamilyIndex_)
		, binaryCollection	(binaryCollection_)
		, allocator			(allocator_)
		, numVertices		(0)
		, waitEvent			(false)
	{
		createFences(vkd, device, false, DE_LENGTH_OF_ARRAY(fences), fences);
	}

	~TestContext()
	{
		destroyFences(vkd, device, DE_LENGTH_OF_ARRAY(fences), fences);
	}
};

void generateWork (TestContext& testContext)
{
	const DeviceInterface&						deviceInterface		= testContext.vkd;
	const deUint32								queueFamilyNdx		= testContext.queueFamilyIndex;

	// \note VkShaderModule is consumed by vkCreate*Pipelines() so it can be deleted
	//       as pipeline has been constructed.
	const vk::Unique<VkShaderModule>			vertShaderModule	(createShaderModule(deviceInterface,
																						testContext.device,
																						testContext.binaryCollection.get("glslvert"),
																						(VkShaderModuleCreateFlags)0));

	const vk::Unique<VkShaderModule>			fragShaderModule	(createShaderModule(deviceInterface,
																						testContext.device,
																						testContext.binaryCollection.get("glslfrag"),
																						(VkShaderModuleCreateFlags)0));
	const VkPipelineShaderStageCreateInfo		shaderStageParams[]	=
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			DE_NULL,
			(VkPipelineShaderStageCreateFlags)0,
			VK_SHADER_STAGE_VERTEX_BIT,
			*vertShaderModule,
			"main",
			(const VkSpecializationInfo*)DE_NULL,
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			DE_NULL,
			(VkPipelineShaderStageCreateFlags)0,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			*fragShaderModule,
			"main",
			(const VkSpecializationInfo*)DE_NULL,
		}
	};

	vk::Move<VkPipelineLayout>					layout;
	vector<ShaderDescParams>					shaderDescParams;
	VertexDesc									vertexDesc;
	vector<VertexDesc>							vertexDescList;
	vector<VkVertexInputAttributeDescription>	attrList;
	vector<VkBufferMemoryBarrier>				bufferMemoryBarrier;
	deUint32									memoryBarrierNdx;
	deUint32									bufferMemoryBarrierNdx;
	deUint32									imageMemoryBarrierNdx;
	vector<VkVertexInputBindingDescription>		bindingList;
	VkPipelineVertexInputStateCreateInfo		vertexInputState;
	VkPipelineInputAssemblyStateCreateInfo		inputAssemblyState;
	VkPipelineDepthStencilStateCreateInfo		depthStencilState;
	VkPipelineColorBlendAttachmentState			blendAttachment;
	VkPipelineColorBlendStateCreateInfo			blendState;
	VkPipelineLayoutCreateInfo					pipelineLayoutState;
	VkGraphicsPipelineCreateInfo				pipelineState;
	VkPipelineCacheCreateInfo					cacheState;
	VkViewport									viewport;
	VkPipelineViewportStateCreateInfo			viewportInfo;
	VkRect2D									scissor;
	BufferParameters							bufferParameters;
	Buffer										buffer;
	RenderInfo									renderInfo;
	ImageParameters								imageParameters;
	Image										image;
	VkPipelineRasterizationStateCreateInfo		rasterState;
	VkPipelineMultisampleStateCreateInfo		multisampleState;
	VkFramebufferCreateInfo						fbState;
	VkCommandBufferBeginInfo					commandBufRecordState;
	VkCommandBufferInheritanceInfo				inheritanceInfo;
	RenderPassParameters						renderPassParameters;
	TransferInfo								transferInfo;
	vector<void*>								barrierList;
	VkExtent3D									extent;
	vector<VkMemoryBarrier>						memoryBarriers;
	vector<VkBufferMemoryBarrier>				bufferBarriers;
	vector<VkImageMemoryBarrier>				imageBarriers;

	memoryBarrierNdx			= 0;
	bufferMemoryBarrierNdx		= 0;
	imageMemoryBarrierNdx		= 0;
	buffer.memoryBarrier.resize(memoryBarrierNdx);
	bufferMemoryBarrier.resize(bufferMemoryBarrierNdx);
	image.imageMemoryBarrier.resize(imageMemoryBarrierNdx);

	memoryBarriers.resize(0);
	bufferBarriers.resize(0);
	imageBarriers.resize(0);

	bufferParameters.memory					= testContext.vertices;
	bufferParameters.size					= testContext.numVertices * sizeof(tcu::Vec4);
	bufferParameters.usage					= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferParameters.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	bufferParameters.queueFamilyCount		= 1;
	bufferParameters.queueFamilyIndex		= &queueFamilyNdx;
	bufferParameters.inputBarrierFlags		= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	createVulkanBuffer(deviceInterface, testContext.device, testContext.allocator, bufferParameters, buffer, MemoryRequirement::HostVisible);
	testContext.vertexBufferAllocation		= buffer.allocation;
	testContext.vertexBuffer				= buffer.buffer;

	bufferParameters.memory					= DE_NULL;
	bufferParameters.size					= testContext.renderSize;
	bufferParameters.usage					= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferParameters.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	bufferParameters.queueFamilyCount		= 1;
	bufferParameters.queueFamilyIndex		= &queueFamilyNdx;
	bufferParameters.inputBarrierFlags		= 0;
	createVulkanBuffer(deviceInterface, testContext.device, testContext.allocator, bufferParameters, buffer, MemoryRequirement::HostVisible);
	testContext.renderReadBuffer			= buffer.allocation;
	testContext.renderBuffer				= buffer.buffer;

	extent.width							= testContext.renderDimension.x();
	extent.height							= testContext.renderDimension.y();
	extent.depth							= 1;

	imageParameters.imageType				= VK_IMAGE_TYPE_2D;
	imageParameters.format					= VK_FORMAT_R8G8B8A8_UNORM;
	imageParameters.extent3D				= extent;
	imageParameters.mipLevels				= 1;
	imageParameters.samples					= VK_SAMPLE_COUNT_1_BIT;
	imageParameters.tiling					= VK_IMAGE_TILING_OPTIMAL;
	imageParameters.usage					= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageParameters.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imageParameters.queueFamilyCount		= 1;
	imageParameters.queueFamilyNdxList		= &queueFamilyNdx;
	imageParameters.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imageParameters.finalLayout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageParameters.barrierInputMask		= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	createVulkanImage(deviceInterface, testContext.device, testContext.allocator, imageParameters, image, MemoryRequirement::Any);
	testContext.imageAllocation				= image.allocation;
	testContext.image						= image.image;

	for (size_t ndx = 0; ndx < image.imageMemoryBarrier.size(); ++ndx)
		imageBarriers.push_back(image.imageMemoryBarrier[ndx]);

	renderPassParameters.colorFormat		= VK_FORMAT_R8G8B8A8_UNORM;
	renderPassParameters.colorSamples		= VK_SAMPLE_COUNT_1_BIT;
	createColorOnlyRenderPass(deviceInterface, testContext.device, renderPassParameters, testContext.renderPass);

	vertexDesc.location = 0;
	vertexDesc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertexDesc.stride = sizeof(tcu::Vec4);
	vertexDesc.offset = 0;
	vertexDescList.push_back(vertexDesc);

	createVertexInfo(vertexDescList, bindingList, attrList, vertexInputState);

	deMemset(&inputAssemblyState, 0xcd, sizeof(inputAssemblyState));
	inputAssemblyState.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.pNext					= DE_NULL;
	inputAssemblyState.flags					= 0u;
	inputAssemblyState.topology					= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.primitiveRestartEnable	= false;

	viewport.x									= 0;
	viewport.y									= 0;
	viewport.width								= (float)testContext.renderDimension.x();
	viewport.height								= (float)testContext.renderDimension.y();
	viewport.minDepth							= 0;
	viewport.maxDepth							= 1;

	scissor.offset.x							= 0;
	scissor.offset.y							= 0;
	scissor.extent.width						= testContext.renderDimension.x();
	scissor.extent.height						= testContext.renderDimension.y();

	deMemset(&viewportInfo, 0xcd, sizeof(viewportInfo));
	viewportInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.pNext							= DE_NULL;
	viewportInfo.flags							= 0;
	viewportInfo.viewportCount					= 1;
	viewportInfo.pViewports						= &viewport;
	viewportInfo.scissorCount					= 1;
	viewportInfo.pScissors						= &scissor;

	deMemset(&rasterState, 0xcd, sizeof(rasterState));
	rasterState.sType							= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterState.pNext							= DE_NULL;
	rasterState.flags							= 0;
	rasterState.depthClampEnable				= VK_TRUE;
	rasterState.rasterizerDiscardEnable			= VK_FALSE;
	rasterState.polygonMode						= VK_POLYGON_MODE_FILL;
	rasterState.cullMode						= VK_CULL_MODE_NONE;
	rasterState.frontFace						= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterState.depthBiasEnable					= VK_FALSE;
	rasterState.lineWidth						= 1;

	deMemset(&multisampleState, 0xcd, sizeof(multisampleState));
	multisampleState.sType						= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pNext						= DE_NULL;
	multisampleState.flags						= 0;
	multisampleState.rasterizationSamples		= VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable		= VK_FALSE;
	multisampleState.pSampleMask				= DE_NULL;
	multisampleState.alphaToCoverageEnable		= VK_FALSE;
	multisampleState.alphaToOneEnable			= VK_FALSE;

	deMemset(&depthStencilState, 0xcd, sizeof(depthStencilState));
	depthStencilState.sType						= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.pNext						= DE_NULL;
	depthStencilState.flags						= 0;
	depthStencilState.depthTestEnable			= VK_FALSE;
	depthStencilState.depthWriteEnable			= VK_FALSE;
	depthStencilState.depthCompareOp			= VK_COMPARE_OP_ALWAYS;
	depthStencilState.depthBoundsTestEnable		= VK_FALSE;
	depthStencilState.stencilTestEnable			= VK_FALSE;
	depthStencilState.front.failOp				= VK_STENCIL_OP_KEEP;
	depthStencilState.front.passOp				= VK_STENCIL_OP_KEEP;
	depthStencilState.front.depthFailOp			= VK_STENCIL_OP_KEEP;
	depthStencilState.front.compareOp			= VK_COMPARE_OP_ALWAYS;
	depthStencilState.front.compareMask			= 0u;
	depthStencilState.front.writeMask			= 0u;
	depthStencilState.front.reference			= 0u;
	depthStencilState.back						= depthStencilState.front;

	deMemset(&blendAttachment, 0xcd, sizeof(blendAttachment));
	blendAttachment.blendEnable					= VK_FALSE;
	blendAttachment.srcColorBlendFactor			= VK_BLEND_FACTOR_ZERO;
	blendAttachment.srcAlphaBlendFactor			= VK_BLEND_FACTOR_ZERO;
	blendAttachment.dstColorBlendFactor			= VK_BLEND_FACTOR_ZERO;
	blendAttachment.dstAlphaBlendFactor			= VK_BLEND_FACTOR_ZERO;
	blendAttachment.colorBlendOp				= VK_BLEND_OP_ADD;
	blendAttachment.alphaBlendOp				= VK_BLEND_OP_ADD;
	blendAttachment.colorWriteMask				= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	deMemset(&blendState, 0xcd, sizeof(blendState));
	blendState.sType							= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.pNext							= DE_NULL;
	blendState.flags							= 0;
	blendState.logicOpEnable					= VK_FALSE;
	blendState.logicOp							= VK_LOGIC_OP_COPY;
	blendState.attachmentCount					= 1;
	blendState.pAttachments						= &blendAttachment;

	deMemset(&pipelineLayoutState, 0xcd, sizeof(pipelineLayoutState));
	pipelineLayoutState.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutState.pNext					= DE_NULL;
	pipelineLayoutState.flags					= 0;
	pipelineLayoutState.setLayoutCount			= 0;
	pipelineLayoutState.pSetLayouts				= DE_NULL;
	pipelineLayoutState.pushConstantRangeCount	= 0;
	pipelineLayoutState.pPushConstantRanges		= DE_NULL;
	layout = createPipelineLayout(deviceInterface, testContext.device, &pipelineLayoutState, DE_NULL);

	deMemset(&pipelineState, 0xcd, sizeof(pipelineState));
	pipelineState.sType							= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineState.pNext							= DE_NULL;
	pipelineState.flags							= 0;
	pipelineState.stageCount					= DE_LENGTH_OF_ARRAY(shaderStageParams);
	pipelineState.pStages						= &shaderStageParams[0];
	pipelineState.pVertexInputState				= &vertexInputState;
	pipelineState.pInputAssemblyState			= &inputAssemblyState;
	pipelineState.pTessellationState			= DE_NULL;
	pipelineState.pViewportState				= &viewportInfo;
	pipelineState.pRasterizationState			= &rasterState;
	pipelineState.pMultisampleState				= &multisampleState;
	pipelineState.pDepthStencilState			= &depthStencilState;
	pipelineState.pColorBlendState				= &blendState;
	pipelineState.pDynamicState					= (const VkPipelineDynamicStateCreateInfo*)DE_NULL;
	pipelineState.layout						= layout.get();
	pipelineState.renderPass					= testContext.renderPass.get();
	pipelineState.subpass						= 0;
	pipelineState.basePipelineHandle			= DE_NULL;
	pipelineState.basePipelineIndex				= 0;

	deMemset(&cacheState, 0xcd, sizeof(cacheState));
	cacheState.sType							= VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheState.pNext							= DE_NULL;
	cacheState.flags							= 0;
	cacheState.initialDataSize					= 0;
	cacheState.pInitialData						= DE_NULL;

	testContext.pipelineCache	= createPipelineCache(deviceInterface, testContext.device, &cacheState);
	testContext.pipeline		= createGraphicsPipeline(deviceInterface, testContext.device, testContext.pipelineCache.get(), &pipelineState);

	deMemset(&fbState, 0xcd, sizeof(fbState));
	fbState.sType								= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbState.pNext								= DE_NULL;
	fbState.flags								= 0;
	fbState.renderPass							= testContext.renderPass.get();
	fbState.attachmentCount						= 1;
	fbState.pAttachments						= &image.imageView.get();
	fbState.width								= (deUint32)testContext.renderDimension.x();
	fbState.height								= (deUint32)testContext.renderDimension.y();
	fbState.layers								= 1;

	testContext.framebuffer	= createFramebuffer(deviceInterface, testContext.device, &fbState);
	testContext.imageView	= image.imageView;

	deMemset(&inheritanceInfo, 0xcd, sizeof(inheritanceInfo));
	inheritanceInfo.sType						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext						= DE_NULL;
	inheritanceInfo.renderPass					= testContext.renderPass.get();
	inheritanceInfo.subpass						= 0;
	inheritanceInfo.framebuffer					= *testContext.framebuffer;
	inheritanceInfo.occlusionQueryEnable		= VK_FALSE;
	inheritanceInfo.queryFlags					= 0u;
	inheritanceInfo.pipelineStatistics			= 0u;

	deMemset(&commandBufRecordState, 0xcd, sizeof(commandBufRecordState));
	commandBufRecordState.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufRecordState.pNext					= DE_NULL;
	commandBufRecordState.flags					= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufRecordState.pInheritanceInfo		= &inheritanceInfo;
	VK_CHECK(deviceInterface.beginCommandBuffer(testContext.cmdBuffer.get(), &commandBufRecordState));

	deviceInterface.cmdPipelineBarrier( testContext.cmdBuffer.get(),
										VK_PIPELINE_STAGE_HOST_BIT,
										VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
										false,
										(deUint32)memoryBarriers.size(), (memoryBarriers.empty() ? DE_NULL : &memoryBarriers[0]),
										(deUint32)bufferBarriers.size(), (bufferBarriers.empty() ? DE_NULL : &bufferBarriers[0]),
										(deUint32)imageBarriers.size(), (imageBarriers.empty() ? DE_NULL : &imageBarriers[0]));

	memoryBarriers.resize(0);
	bufferBarriers.resize(0);
	imageBarriers.resize(0);

	renderInfo.width				= testContext.renderDimension.x();
	renderInfo.height				= testContext.renderDimension.y();
	renderInfo.vertexBufferSize		= testContext.numVertices;
	renderInfo.vertexBuffer			= testContext.vertexBuffer.get();
	renderInfo.image				= testContext.image.get();
	renderInfo.commandBuffer		= testContext.cmdBuffer.get();
	renderInfo.renderPass			= testContext.renderPass.get();
	renderInfo.framebuffer			= *testContext.framebuffer;
	renderInfo.pipeline				= *testContext.pipeline;
	renderInfo.mipLevels			= 1;
	renderInfo.queueFamilyNdxList	= &queueFamilyNdx;
	renderInfo.queueFamilyNdxCount	= 1;
	renderInfo.waitEvent			= testContext.waitEvent;
	renderInfo.event				= testContext.event;
	renderInfo.barriers				= &imageBarriers;
	recordRenderPass(deviceInterface, renderInfo);

	deviceInterface.cmdPipelineBarrier(	renderInfo.commandBuffer,
										VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
										VK_PIPELINE_STAGE_TRANSFER_BIT,
										false,
										(deUint32)memoryBarriers.size(), (memoryBarriers.empty() ? DE_NULL : &memoryBarriers[0]),
										(deUint32)bufferBarriers.size(), (bufferBarriers.empty() ? DE_NULL : &bufferBarriers[0]),
										(deUint32)imageBarriers.size(), (imageBarriers.empty() ? DE_NULL : &imageBarriers[0]));

	memoryBarriers.resize(0);
	bufferBarriers.resize(0);
	imageBarriers.resize(0);

	transferInfo.commandBuffer		= renderInfo.commandBuffer;
	transferInfo.width				= testContext.renderDimension.x();
	transferInfo.height				= testContext.renderDimension.y();
	transferInfo.image				= renderInfo.image;
	transferInfo.buffer				= testContext.renderBuffer.get();
	transferInfo.size				= testContext.renderSize;
	transferInfo.mipLevel			= 0;
	transferInfo.imageOffset.x		= 0;
	transferInfo.imageOffset.y		= 0;
	transferInfo.imageOffset.z		= 0;
	transferInfo.barriers			= &bufferBarriers;
	copyToCPU(deviceInterface, &transferInfo);

	deviceInterface.cmdPipelineBarrier(	transferInfo.commandBuffer,
										VK_PIPELINE_STAGE_TRANSFER_BIT,
										VK_PIPELINE_STAGE_HOST_BIT,
										false,
										(deUint32)memoryBarriers.size(), (memoryBarriers.empty() ? DE_NULL : &memoryBarriers[0]),
										(deUint32)bufferBarriers.size(), (bufferBarriers.empty() ? DE_NULL : &bufferBarriers[0]),
										(deUint32)imageBarriers.size(), (imageBarriers.empty() ? DE_NULL : &imageBarriers[0]));

	memoryBarriers.resize(0);
	bufferBarriers.resize(0);
	imageBarriers.resize(0);

	VK_CHECK(deviceInterface.endCommandBuffer(transferInfo.commandBuffer));
}

static void initSubmitInfo (VkSubmitInfo* submitInfo, deUint32 submitInfoCount)
{
	for (deUint32 ndx = 0; ndx < submitInfoCount; ndx++)
	{
		submitInfo[ndx].sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo[ndx].pNext					= DE_NULL;
		submitInfo[ndx].waitSemaphoreCount		= 0;
		submitInfo[ndx].pWaitSemaphores			= DE_NULL;
		submitInfo[ndx].pWaitDstStageMask		= DE_NULL;
		submitInfo[ndx].commandBufferCount		= 1;
		submitInfo[ndx].signalSemaphoreCount	= 0;
		submitInfo[ndx].pSignalSemaphores		= DE_NULL;
	}
}

tcu::TestStatus testFences (Context& context)
{
	TestLog&					log					= context.getTestContext().getLog();
	const DeviceInterface&		deviceInterface		= context.getDeviceInterface();
	const VkQueue				queue				= context.getUniversalQueue();
	const deUint32				queueFamilyIdx		= context.getUniversalQueueFamilyIndex();
	VkDevice					device				= context.getDevice();
	VkResult					waitStatus;
	VkResult					fenceStatus;
	TestContext					testContext			(deviceInterface, device, queueFamilyIdx, context.getBinaryCollection(), context.getDefaultAllocator());
	VkSubmitInfo				submitInfo;
	VkMappedMemoryRange			range;
	void*						resultImage;

	const tcu::Vec4				vertices[]			=
	{
		tcu::Vec4( 0.5f,  0.5f, 0.0f, 1.0f),
		tcu::Vec4(-0.5f,  0.5f, 0.0f, 1.0f),
		tcu::Vec4( 0.0f, -0.5f, 0.0f, 1.0f)
	};

	testContext.vertices = vertices;
	testContext.numVertices = DE_LENGTH_OF_ARRAY(vertices);
	testContext.renderDimension = tcu::IVec2(256, 256);
	testContext.renderSize = sizeof(deUint32) * testContext.renderDimension.x() * testContext.renderDimension.y();

	createCommandBuffer(deviceInterface, device, queueFamilyIdx, &testContext.cmdBuffer, &testContext.commandPool);
	generateWork(testContext);

	initSubmitInfo(&submitInfo, 1);
	submitInfo.pCommandBuffers		= &testContext.cmdBuffer.get();

	// Default status is unsignaled
	fenceStatus = deviceInterface.getFenceStatus(device, testContext.fences[0]);
	if (fenceStatus != VK_NOT_READY)
	{
		log << TestLog::Message << "testSynchronizationPrimitives fence 0 should be reset but status is " << getResultName(fenceStatus) << TestLog::EndMessage;
		return tcu::TestStatus::fail("Fence in incorrect state");
	}
	fenceStatus = deviceInterface.getFenceStatus(device, testContext.fences[1]);
	if (fenceStatus != VK_NOT_READY)
	{
		log << TestLog::Message << "testSynchronizationPrimitives fence 1 should be reset but status is " << getResultName(fenceStatus) << TestLog::EndMessage;
		return tcu::TestStatus::fail("Fence in incorrect state");
	}

	VK_CHECK(deviceInterface.queueSubmit(queue, 1, &submitInfo, testContext.fences[0]));

	// Wait with timeout = 0
	waitStatus = deviceInterface.waitForFences(device, 1, &testContext.fences[0], true, 0u);
	if (waitStatus != VK_SUCCESS && waitStatus != VK_TIMEOUT)
	{
		// Will most likely end with VK_TIMEOUT
		log << TestLog::Message << "testSynchPrimitives failed to wait for a single fence" << TestLog::EndMessage;
		return tcu::TestStatus::fail("Failed to wait for a single fence");
	}

	// Wait with a reasonable timeout
	waitStatus = deviceInterface.waitForFences(device, 1, &testContext.fences[0], true, DEFAULT_TIMEOUT);
	if (waitStatus != VK_SUCCESS && waitStatus != VK_TIMEOUT)
	{
		// \note Wait can end with a timeout if DEFAULT_TIMEOUT is not sufficient
		log << TestLog::Message << "testSynchPrimitives failed to wait for a single fence" << TestLog::EndMessage;
		return tcu::TestStatus::fail("Failed to wait for a single fence");
	}

	// Wait for work on fences[0] to actually complete
	waitStatus = deviceInterface.waitForFences(device, 1, &testContext.fences[0], true, std::numeric_limits<deUint64>::max());
	if (waitStatus != VK_SUCCESS)
	{
		log << TestLog::Message << "testSynchPrimitives failed to wait for a fence" << TestLog::EndMessage;
		return tcu::TestStatus::fail("failed to wait for a fence");
	}

	// Wait until timeout on a fence that has not been submitted
	waitStatus = deviceInterface.waitForFences(device, 1, &testContext.fences[1], true, 1);
	if (waitStatus != VK_TIMEOUT)
	{
		log << TestLog::Message << "testSyncPrimitives failed to timeout on wait for single fence" << TestLog::EndMessage;
		return tcu::TestStatus::fail("failed to timeout on wait for single fence");
	}

	// Check that the fence is signaled after the wait
	fenceStatus = deviceInterface.getFenceStatus(device, testContext.fences[0]);
	if (fenceStatus != VK_SUCCESS)
	{
		log << TestLog::Message << "testSynchronizationPrimitives fence should be signaled but status is " << getResultName(fenceStatus) << TestLog::EndMessage;
		return tcu::TestStatus::fail("Fence in incorrect state");
	}

	range.sType			= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.pNext			= DE_NULL;
	range.memory		= testContext.renderReadBuffer->getMemory();
	range.offset		= 0;
	range.size			= testContext.renderSize;
	VK_CHECK(deviceInterface.invalidateMappedMemoryRanges(device, 1, &range));
	resultImage = testContext.renderReadBuffer->getHostPtr();

	log << TestLog::Image(	"result",
							"result",
							tcu::ConstPixelBufferAccess(tcu::TextureFormat(
									tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8),
									testContext.renderDimension.x(),
									testContext.renderDimension.y(),
									1,
									resultImage));

	return TestStatus::pass("synchronization-fences passed");
}

tcu::TestStatus testSemaphores (Context& context)
{
	TestLog&					log					= context.getTestContext().getLog();
	const InstanceInterface&	instanceInterface	= context.getInstanceInterface();
	const VkPhysicalDevice		physicalDevice		= context.getPhysicalDevice();
	deUint32					queueFamilyIdx;
	vk::Move<VkDevice>			device				= createTestDevice(instanceInterface, physicalDevice, &queueFamilyIdx);
	const DeviceDriver			deviceInterface		(instanceInterface, *device);
	SimpleAllocator				allocator			(deviceInterface,
													 *device,
													 getPhysicalDeviceMemoryProperties(instanceInterface, physicalDevice));
	const VkQueue				queue[2]			=
	{
		getDeviceQueue(deviceInterface, *device, queueFamilyIdx, 0),
		getDeviceQueue(deviceInterface, *device, queueFamilyIdx, 1)
	};
	VkResult					testStatus;
	TestContext					testContext1		(deviceInterface, device.get(), queueFamilyIdx, context.getBinaryCollection(), allocator);
	TestContext					testContext2		(deviceInterface, device.get(), queueFamilyIdx, context.getBinaryCollection(), allocator);
	Unique<VkSemaphore>			semaphore			(createSemaphore(deviceInterface, *device));
	VkSubmitInfo				submitInfo[2];
	VkMappedMemoryRange			range;
	void*						resultImage;
	const VkPipelineStageFlags	waitDstStageMask	= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	const tcu::Vec4		vertices1[]			=
	{
		tcu::Vec4( 0.5f,  0.5f, 0.0f, 1.0f),
		tcu::Vec4(-0.5f,  0.5f, 0.0f, 1.0f),
		tcu::Vec4( 0.0f, -0.5f, 0.0f, 1.0f)
	};

	const tcu::Vec4		vertices2[]			=
	{
		tcu::Vec4(-0.5f, -0.5f, 0.0f, 1.0f),
		tcu::Vec4(+0.5f, -0.5f, 0.0f, 1.0f),
		tcu::Vec4( 0.0f, +0.5f, 0.0f, 1.0f)
	};

	testContext1.vertices			= vertices1;
	testContext1.numVertices		= DE_LENGTH_OF_ARRAY(vertices1);
	testContext1.renderDimension	= tcu::IVec2(256, 256);
	testContext1.renderSize			= sizeof(deUint32) * testContext1.renderDimension.x() * testContext1.renderDimension.y();

	testContext2.vertices			= vertices2;
	testContext2.numVertices		= DE_LENGTH_OF_ARRAY(vertices2);
	testContext2.renderDimension	= tcu::IVec2(256, 256);
	testContext2.renderSize			= sizeof(deUint32) * testContext2.renderDimension.x() * testContext2.renderDimension.y();

	createCommandBuffer(deviceInterface, device.get(), queueFamilyIdx, &testContext1.cmdBuffer, &testContext1.commandPool);
	generateWork(testContext1);

	createCommandBuffer(deviceInterface, device.get(), queueFamilyIdx, &testContext2.cmdBuffer, &testContext2.commandPool);
	generateWork(testContext2);

	initSubmitInfo(submitInfo, DE_LENGTH_OF_ARRAY(submitInfo));

	// The difference between the two submit infos is that each will use a unique cmd buffer,
	// and one will signal a semaphore but not wait on a semaphore, the other will wait on the
	// semaphore but not signal a semaphore
	submitInfo[0].pCommandBuffers		= &testContext1.cmdBuffer.get();
	submitInfo[1].pCommandBuffers		= &testContext2.cmdBuffer.get();

	submitInfo[0].signalSemaphoreCount	= 1;
	submitInfo[0].pSignalSemaphores		= &semaphore.get();
	submitInfo[1].waitSemaphoreCount	= 1;
	submitInfo[1].pWaitSemaphores		= &semaphore.get();
	submitInfo[1].pWaitDstStageMask		= &waitDstStageMask;

	VK_CHECK(deviceInterface.queueSubmit(queue[0], 1, &submitInfo[0], testContext1.fences[0]));

	testStatus  = deviceInterface.waitForFences(device.get(), 1, &testContext1.fences[0], true, std::numeric_limits<deUint64>::max());
	if (testStatus != VK_SUCCESS)
	{
		log << TestLog::Message << "testSynchPrimitives failed to wait for a set fence" << TestLog::EndMessage;
		return tcu::TestStatus::fail("failed to wait for a set fence");
	}

	range.sType			= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.pNext			= DE_NULL;
	range.memory		= testContext1.renderReadBuffer->getMemory();
	range.offset		= 0;
	range.size			= testContext1.renderSize;
	VK_CHECK(deviceInterface.invalidateMappedMemoryRanges(device.get(), 1, &range));
	resultImage = testContext1.renderReadBuffer->getHostPtr();

	log << TestLog::Image(	"result",
							"result",
							tcu::ConstPixelBufferAccess(tcu::TextureFormat(
									tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8),
									testContext1.renderDimension.x(),
									testContext1.renderDimension.y(),
									1,
									resultImage));

	VK_CHECK(deviceInterface.queueSubmit(queue[1], 1, &submitInfo[1], testContext2.fences[0]));

	testStatus  = deviceInterface.waitForFences(device.get(), 1, &testContext2.fences[0], true, std::numeric_limits<deUint64>::max());
	if (testStatus != VK_SUCCESS)
	{
		log << TestLog::Message << "testSynchPrimitives failed to wait for a set fence" << TestLog::EndMessage;
		return tcu::TestStatus::fail("failed to wait for a set fence");
	}

	range.sType			= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.pNext			= DE_NULL;
	range.memory		= testContext2.renderReadBuffer->getMemory();
	range.offset		= 0;
	range.size			= testContext2.renderSize;
	VK_CHECK(deviceInterface.invalidateMappedMemoryRanges(device.get(), 1, &range));
	resultImage = testContext2.renderReadBuffer->getHostPtr();

	log << TestLog::Image(	"result",
							"result",
							tcu::ConstPixelBufferAccess(tcu::TextureFormat(
									tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8),
									testContext2.renderDimension.x(),
									testContext2.renderDimension.y(),
									1,
									resultImage));

	return tcu::TestStatus::pass("synchronization-semaphores passed");
}

tcu::TestStatus testEvents (Context& context)
{
	TestLog&					log					= context.getTestContext().getLog();
	const DeviceInterface&		deviceInterface		= context.getDeviceInterface();
	VkDevice					device				= context.getDevice();
	const deUint32				queueFamilyIdx		= context.getUniversalQueueFamilyIndex();
	Allocator&					allocator			= context.getDefaultAllocator();
	VkQueue						queue				= context.getUniversalQueue();
	VkResult					testStatus;
	VkResult					eventStatus;
	TestContext					testContext			(deviceInterface, device, queueFamilyIdx, context.getBinaryCollection(), allocator);
	Unique<VkEvent>				event				(createEvent(deviceInterface, device));
	VkSubmitInfo				submitInfo;
	VkMappedMemoryRange			range;
	void*						resultImage;

	const tcu::Vec4		vertices1[]			=
	{
		tcu::Vec4( 0.5f,  0.5f, 0.0f, 1.0f),
		tcu::Vec4(-0.5f,  0.5f, 0.0f, 1.0f),
		tcu::Vec4( 0.0f, -0.5f, 0.0f, 1.0f)
	};

	testContext.vertices = vertices1;
	testContext.numVertices = DE_LENGTH_OF_ARRAY(vertices1);
	testContext.renderDimension = tcu::IVec2(256, 256);
	testContext.waitEvent = true;
	testContext.event = event.get();
	testContext.renderSize = sizeof(deUint32) * testContext.renderDimension.x() * testContext.renderDimension.y();

	createCommandBuffer(deviceInterface, device, queueFamilyIdx, &testContext.cmdBuffer, &testContext.commandPool);
	generateWork(testContext);

	initSubmitInfo(&submitInfo, 1);
	submitInfo.pCommandBuffers = &testContext.cmdBuffer.get();

	// 6.3 An event is initially in the unsignaled state
	eventStatus = deviceInterface.getEventStatus(device, event.get());
	if (eventStatus != VK_EVENT_RESET)
	{
		log << TestLog::Message << "testSynchronizationPrimitives event should be reset but status is " << getResultName(eventStatus) << TestLog::EndMessage;
		return tcu::TestStatus::fail("Event in incorrect status");
	}

	// The recorded command buffer should wait at the top of the graphics pipe for an event signaled by the host and so should not
	// make forward progress as long as the event is not signaled
	VK_CHECK(deviceInterface.queueSubmit(queue, 1, &submitInfo, testContext.fences[0]));

	testStatus  = deviceInterface.waitForFences(device, 1, &testContext.fences[0], true, 1);
	if (testStatus != VK_TIMEOUT)
	{
		log << TestLog::Message << "testSynchronizationPrimitives failed to wait for set event from host." << TestLog::EndMessage;
		return tcu::TestStatus::fail("failed to wait for event set from host");
	}

	// Should allow the recorded command buffer to finally make progress
	VK_CHECK(deviceInterface.setEvent(device, event.get()));
	eventStatus = deviceInterface.getEventStatus(device, event.get());
	if (eventStatus != VK_EVENT_SET)
	{
		log << TestLog::Message << "testEvents failed to transition event to signaled state via setEvent call from host" << TestLog::EndMessage;
		return tcu::TestStatus::fail("failed to signal event from host");
	}

	testStatus  = deviceInterface.waitForFences(device, 1, &testContext.fences[0], true, ~(0ull));
	if (testStatus != VK_SUCCESS)
	{
		log << TestLog::Message << "testSynchronizationPrimitives failed to proceed after set event from host." << TestLog::EndMessage;
		return tcu::TestStatus::fail("failed to proceed after event set from host");
	}

	range.sType			= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.pNext			= DE_NULL;
	range.memory		= testContext.renderReadBuffer->getMemory();
	range.offset		= 0;
	range.size			= testContext.renderSize;
	VK_CHECK(deviceInterface.invalidateMappedMemoryRanges(device, 1, &range));
	resultImage = testContext.renderReadBuffer->getHostPtr();

	log << TestLog::Image(	"result",
							"result",
							tcu::ConstPixelBufferAccess(tcu::TextureFormat(
									tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8),
									testContext.renderDimension.x(),
									testContext.renderDimension.y(),
									1,
									resultImage));

	return tcu::TestStatus::pass("synchronization-events passed");
}

} // anonymous

tcu::TestCaseGroup* createSmokeTests (tcu::TestContext& textCtx)
{
	de::MovePtr<tcu::TestCaseGroup> synchTests  (new tcu::TestCaseGroup(textCtx, "smoke", "Synchronization smoke tests"));

	addFunctionCaseWithPrograms(synchTests.get(), "fences", "", buildShaders, testFences);
	addFunctionCaseWithPrograms(synchTests.get(), "semaphores", "", buildShaders, testSemaphores);
	addFunctionCaseWithPrograms(synchTests.get(), "events", "", buildShaders, testEvents);

	return synchTests.release();
}

} // synchronization
} // vkt
