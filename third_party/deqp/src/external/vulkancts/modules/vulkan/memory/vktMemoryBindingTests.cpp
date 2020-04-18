/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Memory binding test excercising VK_KHR_bind_memory2 extension.
 *//*--------------------------------------------------------------------*/

#include "vktMemoryBindingTests.hpp"

#include "vktTestCase.hpp"
#include "tcuTestLog.hpp"

#include "vkPlatform.hpp"
#include "gluVarType.hpp"
#include "deStringUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"
#include "deSharedPtr.hpp"
#include "vktTestCase.hpp"
#include "vkTypeUtil.hpp"

#include <algorithm>

namespace vkt
{
namespace memory
{
namespace
{

using namespace vk;

typedef const VkMemoryDedicatedAllocateInfoKHR								ConstDedicatedInfo;
typedef de::SharedPtr<Move<VkDeviceMemory> >								MemoryRegionPtr;
typedef std::vector<MemoryRegionPtr>										MemoryRegionsList;
typedef de::SharedPtr<Move<VkBuffer> >										BufferPtr;
typedef std::vector<BufferPtr>												BuffersList;
typedef de::SharedPtr<Move<VkImage> >										ImagePtr;
typedef std::vector<ImagePtr>												ImagesList;
typedef std::vector<VkBindBufferMemoryInfoKHR>								BindBufferMemoryInfosList;
typedef std::vector<VkBindImageMemoryInfoKHR>								BindImageMemoryInfosList;

class MemoryMappingRAII
{
public:
										MemoryMappingRAII					(const DeviceInterface&	deviceInterface,
																			 const VkDevice&		device,
																			 VkDeviceMemory			deviceMemory,
																			 VkDeviceSize			offset,
																			 VkDeviceSize			size,
																			 VkMemoryMapFlags		flags)
										: vk								(deviceInterface)
										, dev								(device)
										, memory							(deviceMemory)
										, hostPtr							(DE_NULL)

	{
		vk.mapMemory(dev, memory, offset, size, flags, &hostPtr);
	}

										~MemoryMappingRAII					()
	{
		vk.unmapMemory(dev, memory);
		hostPtr = DE_NULL;
	}

	void*								ptr									()
	{
		return hostPtr;
	}

	void								flush								(VkDeviceSize			offset,
																			 VkDeviceSize			size)
	{
		const VkMappedMemoryRange		range								= makeMemoryRange(offset, size);
		VK_CHECK(vk.flushMappedMemoryRanges(dev, 1u, &range));
	}

protected:
	const DeviceInterface&				vk;
	const VkDevice&						dev;
	VkDeviceMemory						memory;
	void*								hostPtr;

	const VkMappedMemoryRange			makeMemoryRange						(VkDeviceSize			offset,
																			 VkDeviceSize			size)
	{
		const VkMappedMemoryRange		range								=
		{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			DE_NULL,
			memory,
			offset,
			size
		};
		return range;
	}
};

class SimpleRandomGenerator
{
public:
										SimpleRandomGenerator				(deUint32				seed)
										: value								(seed)
	{}
	deUint32							getNext								()
	{
		value += 1;
		value ^= (value << 21);
		value ^= (value >> 15);
		value ^= (value << 4);
		return value;
	}
protected:
	deUint32							value;
};

struct BindingCaseParameters
{
	VkBufferCreateFlags					flags;
	VkBufferUsageFlags					usage;
	VkSharingMode						sharing;
	VkDeviceSize						bufferSize;
	VkExtent3D							imageSize;
	deUint32							targetsCount;
};

BindingCaseParameters					makeBindingCaseParameters			(deUint32				targetsCount,
																			 deUint32				width,
																			 deUint32				height)
{
	BindingCaseParameters				params;
	deMemset(&params, 0, sizeof(BindingCaseParameters));
	params.imageSize.width = width;
	params.imageSize.height = height;
	params.imageSize.depth = 1;
	params.bufferSize = params.imageSize.width * params.imageSize.height * params.imageSize.depth * sizeof(deUint32);
	params.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	params.targetsCount = targetsCount;
	return params;
}

BindingCaseParameters					makeBindingCaseParameters			(deUint32				targetsCount,
																			 VkBufferUsageFlags		usage,
																			 VkSharingMode			sharing,
																			 VkDeviceSize			bufferSize)
{
	BindingCaseParameters				params								=
	{
		0,																	// VkBufferCreateFlags	flags;
		usage,																// VkBufferUsageFlags	usage;
		sharing,															// VkSharingMode		sharing;
		bufferSize,															// VkDeviceSize			bufferSize;
		{0u, 0u, 0u},														// VkExtent3D			imageSize;
		targetsCount														// deUint32				targetsCount;
	};
	return params;
}

VkImageCreateInfo						makeImageCreateInfo					(BindingCaseParameters&	params)
{
	const VkImageCreateInfo				imageParams							=
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,								// VkStructureType		sType;
		DE_NULL,															// const void*			pNext;
		0u,																	// VkImageCreateFlags	flags;
		VK_IMAGE_TYPE_2D,													// VkImageType			imageType;
		VK_FORMAT_R8G8B8A8_UINT,											// VkFormat				format;
		params.imageSize,													// VkExtent3D			extent;
		1u,																	// deUint32				mipLevels;
		1u,																	// deUint32				arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,												// VkSampleCountFlagBits samples;
		VK_IMAGE_TILING_LINEAR,												// VkImageTiling		tiling;
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,	// VkImageUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,											// VkSharingMode		sharingMode;
		0u,																	// deUint32				queueFamilyIndexCount;
		DE_NULL,															// const deUint32*		pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,											// VkImageLayout		initialLayout;
	};
	return imageParams;
}

VkBufferCreateInfo						makeBufferCreateInfo				(Context&				ctx,
																			 BindingCaseParameters&	params)
{
	const deUint32						queueFamilyIndex					= ctx.getUniversalQueueFamilyIndex();
	VkBufferCreateInfo					bufferParams						=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,								// VkStructureType		sType;
		DE_NULL,															// const void*			pNext;
		params.flags,														// VkBufferCreateFlags	flags;
		params.bufferSize,													// VkDeviceSize			size;
		params.usage,														// VkBufferUsageFlags	usage;
		params.sharing,														// VkSharingMode		sharingMode;
		1u,																	// uint32_t				queueFamilyIndexCount;
		&queueFamilyIndex,													// const uint32_t*		pQueueFamilyIndices;
	};
	return bufferParams;
}

const VkMemoryAllocateInfo				makeMemoryAllocateInfo				(VkMemoryRequirements&	memReqs,
																			 ConstDedicatedInfo*	next)
{
	const deUint32						heapTypeIndex						= (deUint32)deCtz32(memReqs.memoryTypeBits);
	const VkMemoryAllocateInfo			allocateParams						=
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,								// VkStructureType		sType;
		next,																// const void*			pNext;
		memReqs.size,														// VkDeviceSize			allocationSize;
		heapTypeIndex,														// uint32_t				memoryTypeIndex;
	};
	return allocateParams;
}

enum MemoryHostVisibility
{
	MemoryAny,
	MemoryHostVisible
};

deUint32								selectMatchingMemoryType			(Context&				ctx,
																			 VkMemoryRequirements&	memReqs,
																			 MemoryHostVisibility	memoryVisibility)
{
	const VkPhysicalDevice				vkPhysicalDevice					= ctx.getPhysicalDevice();
	const InstanceInterface&			vkInstance							= ctx.getInstanceInterface();
	VkPhysicalDeviceMemoryProperties	memoryProperties;

	vkInstance.getPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memoryProperties);
	if (memoryVisibility == MemoryHostVisible)
	{
		for (deUint32 typeNdx = 0; typeNdx < memoryProperties.memoryTypeCount; ++typeNdx)
		{
			const deBool				isInAllowed							= (memReqs.memoryTypeBits & (1u << typeNdx)) != 0u;
			const deBool				hasRightProperties					= (memoryProperties.memoryTypes[typeNdx].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0u;
			if (isInAllowed && hasRightProperties)
				return typeNdx;
		}
	}
	return (deUint32)deCtz32(memReqs.memoryTypeBits);
}

const VkMemoryAllocateInfo				makeMemoryAllocateInfo				(Context&				ctx,
																			 VkMemoryRequirements&	memReqs,
																			 MemoryHostVisibility	memoryVisibility)
{
	const deUint32						heapTypeIndex						= selectMatchingMemoryType(ctx, memReqs, memoryVisibility);
	const VkMemoryAllocateInfo			allocateParams						=
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,								// VkStructureType		sType;
		DE_NULL,															// const void*			pNext;
		memReqs.size,														// VkDeviceSize			allocationSize;
		heapTypeIndex,														// uint32_t				memoryTypeIndex;
	};
	return allocateParams;
}

ConstDedicatedInfo						makeDedicatedAllocationInfo			(VkBuffer				buffer)
{
	ConstDedicatedInfo					dedicatedAllocationInfo				=
	{
		VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,				// VkStructureType		sType
		DE_NULL,															// const void*			pNext
		DE_NULL,															// VkImage				image
		buffer																// VkBuffer				buffer
	};
	return dedicatedAllocationInfo;
}

ConstDedicatedInfo						makeDedicatedAllocationInfo			(VkImage				image)
{
	ConstDedicatedInfo					dedicatedAllocationInfo				=
	{
		VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,				// VkStructureType		sType
		DE_NULL,															// const void*			pNext
		image,																// VkImage				image
		DE_NULL																// VkBuffer				buffer
	};
	return dedicatedAllocationInfo;
}

const VkBindBufferMemoryInfoKHR			makeBufferMemoryBindingInfo			(VkBuffer				buffer,
																			 VkDeviceMemory			memory)
{
	const VkBindBufferMemoryInfoKHR		bufferMemoryBinding					=
	{
		VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO_KHR,						// VkStructureType		sType;
		DE_NULL,															// const void*			pNext;
		buffer,																// VkBuffer				buffer;
		memory,																// VkDeviceMemory		memory;
		0u,																	// VkDeviceSize			memoryOffset;
	};
	return bufferMemoryBinding;
}

const VkBindImageMemoryInfoKHR			makeImageMemoryBindingInfo			(VkImage				image,
																			 VkDeviceMemory			memory)
{
	const VkBindImageMemoryInfoKHR		imageMemoryBinding					=
	{
		VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO_KHR,						// VkStructureType		sType;
		DE_NULL,															// const void*			pNext;
		image,																// VkImage				image;
		memory,																// VkDeviceMemory		memory;
		0u,																	// VkDeviceSize			memoryOffset;
	};
	return imageMemoryBinding;
}

enum TransferDirection
{
	TransferToResource														= 0,
	TransferFromResource													= 1
};

const VkBufferMemoryBarrier				makeMemoryBarrierInfo				(VkBuffer				buffer,
																			 VkDeviceSize			size,
																			 TransferDirection		direction)
{
	const deBool fromRes													= direction == TransferFromResource;
	const VkAccessFlags					srcMask								= static_cast<VkAccessFlags>(fromRes ? VK_ACCESS_HOST_WRITE_BIT : VK_ACCESS_TRANSFER_WRITE_BIT);
	const VkAccessFlags					dstMask								= static_cast<VkAccessFlags>(fromRes ? VK_ACCESS_TRANSFER_READ_BIT : VK_ACCESS_HOST_READ_BIT);
	const VkBufferMemoryBarrier			bufferBarrier						=
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,							// VkStructureType		sType;
		DE_NULL,															// const void*			pNext;
		srcMask,															// VkAccessFlags		srcAccessMask;
		dstMask,															// VkAccessFlags		dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,											// deUint32				srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,											// deUint32				dstQueueFamilyIndex;
		buffer,																// VkBuffer				buffer;
		0u,																	// VkDeviceSize			offset;
		size																// VkDeviceSize			size;
	};
	return bufferBarrier;
}

const VkImageMemoryBarrier				makeMemoryBarrierInfo				(VkImage				image,
																			 VkAccessFlags			srcAccess,
																			 VkAccessFlags			dstAccess,
																			 VkImageLayout			oldLayout,
																			 VkImageLayout			newLayout)
{
	const VkImageAspectFlags			aspect								= VK_IMAGE_ASPECT_COLOR_BIT;
	const VkImageMemoryBarrier			imageBarrier						=
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,								// VkStructureType		sType;
		DE_NULL,															// const void*			pNext;
		srcAccess,															// VkAccessFlags		srcAccessMask;
		dstAccess,															// VkAccessFlags		dstAccessMask;
		oldLayout,															// VkImageLayout		oldLayout;
		newLayout,															// VkImageLayout		newLayout;
		VK_QUEUE_FAMILY_IGNORED,											// deUint32				srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,											// deUint32				dstQueueFamilyIndex;
		image,																// VkImage				image;
		{																	// VkImageSubresourceRange subresourceRange;
			aspect,															// VkImageAspectFlags	aspect;
			0u,																// deUint32				baseMipLevel;
			1u,																// deUint32				mipLevels;
			0u,																// deUint32				baseArraySlice;
			1u,																// deUint32				arraySize;
		}
	};
	return imageBarrier;
}

const VkCommandBufferBeginInfo			makeCommandBufferInfo				()
{
	const VkCommandBufferBeginInfo		cmdBufferBeginInfo					=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		static_cast<const VkCommandBufferInheritanceInfo*>(DE_NULL)
	};
	return cmdBufferBeginInfo;
}

const VkSubmitInfo						makeSubmitInfo						(const VkCommandBuffer&	commandBuffer)
{
	const VkSubmitInfo					submitInfo							=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,										// VkStructureType		sType;
		DE_NULL,															// const void*			pNext;
		0u,																	// deUint32				waitSemaphoreCount;
		DE_NULL,															// const VkSemaphore*	pWaitSemaphores;
		(const VkPipelineStageFlags*)DE_NULL,								// const VkPipelineStageFlags* flags;
		1u,																	// deUint32				commandBufferCount;
		&commandBuffer,														// const VkCommandBuffer* pCommandBuffers;
		0u,																	// deUint32				signalSemaphoreCount;
		DE_NULL																// const VkSemaphore*	pSignalSemaphores;
	};
	return submitInfo;
}

Move<VkCommandBuffer>					createCommandBuffer					(const DeviceInterface&	vk,
																			 VkDevice				device,
																			 VkCommandPool			commandPool)
{
	const VkCommandBufferAllocateInfo allocInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		DE_NULL,
		commandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1
	};
	return allocateCommandBuffer(vk, device, &allocInfo);
}


template<typename TTarget>
void									createBindingTargets				(std::vector<de::SharedPtr<Move<TTarget> > >&
																									targets,
																			 Context&				ctx,
																			 BindingCaseParameters	params);

template<>
void									createBindingTargets<VkBuffer>		(BuffersList&			targets,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	const deUint32						count								= params.targetsCount;
	const VkDevice						vkDevice							= ctx.getDevice();
	const DeviceInterface&				vk									= ctx.getDeviceInterface();

	targets.reserve(count);
	for (deUint32 i = 0u; i < count; ++i)
	{
		VkBufferCreateInfo				bufferParams						= makeBufferCreateInfo(ctx, params);
		targets.push_back(BufferPtr(new Move<VkBuffer>(createBuffer(vk, vkDevice, &bufferParams))));
	}
}

template<>
void									createBindingTargets<VkImage>		(ImagesList&			targets,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	const deUint32						count								= params.targetsCount;
	const VkDevice						vkDevice							= ctx.getDevice();
	const DeviceInterface&				vk									= ctx.getDeviceInterface();

	targets.reserve(count);
	for (deUint32 i = 0u; i < count; ++i)
	{
		VkImageCreateInfo				imageParams							= makeImageCreateInfo(params);
		targets.push_back(ImagePtr(new Move<VkImage>(createImage(vk, vkDevice, &imageParams))));
	}
}

template<typename TTarget, deBool TDedicated>
void									createMemory						(std::vector<de::SharedPtr<Move<TTarget> > >&
																									targets,
																			 MemoryRegionsList&		memory,
																			 Context&				ctx,
																			 BindingCaseParameters	params);

template<>
void									createMemory<VkBuffer, DE_FALSE>	(BuffersList&			targets,
																			 MemoryRegionsList&		memory,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	DE_UNREF(params);
	const deUint32						count								= static_cast<deUint32>(targets.size());
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();

	memory.reserve(count);
	for (deUint32 i = 0; i < count; ++i)
	{
		VkMemoryRequirements			memReqs;

		vk.getBufferMemoryRequirements(vkDevice, **targets[i], &memReqs);

		const VkMemoryAllocateInfo		memAlloc							= makeMemoryAllocateInfo(memReqs, DE_NULL);
		VkDeviceMemory					rawMemory							= DE_NULL;

		vk.allocateMemory(vkDevice, &memAlloc, (VkAllocationCallbacks*)DE_NULL, &rawMemory);
		memory.push_back(MemoryRegionPtr(new Move<VkDeviceMemory>(check<VkDeviceMemory>(rawMemory), Deleter<VkDeviceMemory>(vk, vkDevice, DE_NULL))));
	}
}

template<>
void									createMemory<VkImage, DE_FALSE>		(ImagesList&			targets,
																			 MemoryRegionsList&		memory,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	DE_UNREF(params);
	const deUint32						count								= static_cast<deUint32>(targets.size());
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();

	memory.reserve(count);
	for (deUint32 i = 0; i < count; ++i)
	{
		VkMemoryRequirements			memReqs;
		vk.getImageMemoryRequirements(vkDevice, **targets[i], &memReqs);

		const VkMemoryAllocateInfo		memAlloc							= makeMemoryAllocateInfo(memReqs, DE_NULL);
		VkDeviceMemory					rawMemory							= DE_NULL;

		vk.allocateMemory(vkDevice, &memAlloc, (VkAllocationCallbacks*)DE_NULL, &rawMemory);
		memory.push_back(de::SharedPtr<Move<VkDeviceMemory> >(new Move<VkDeviceMemory>(check<VkDeviceMemory>(rawMemory), Deleter<VkDeviceMemory>(vk, vkDevice, DE_NULL))));
	}
}

template<>
void									createMemory<VkBuffer, DE_TRUE>		(BuffersList&			targets,
																			 MemoryRegionsList&		memory,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	DE_UNREF(params);
	const deUint32						count								= static_cast<deUint32>(targets.size());
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();

	memory.reserve(count);
	for (deUint32 i = 0; i < count; ++i)
	{
		VkMemoryRequirements			memReqs;

		vk.getBufferMemoryRequirements(vkDevice, **targets[i], &memReqs);

		ConstDedicatedInfo				dedicatedAllocationInfo				= makeDedicatedAllocationInfo(**targets[i]);;
		const VkMemoryAllocateInfo		memAlloc							= makeMemoryAllocateInfo(memReqs, &dedicatedAllocationInfo);
		VkDeviceMemory					rawMemory							= DE_NULL;

		vk.allocateMemory(vkDevice, &memAlloc, static_cast<VkAllocationCallbacks*>(DE_NULL), &rawMemory);
		memory.push_back(MemoryRegionPtr(new Move<VkDeviceMemory>(check<VkDeviceMemory>(rawMemory), Deleter<VkDeviceMemory>(vk, vkDevice, DE_NULL))));
	}
}

template<>
void									createMemory<VkImage, DE_TRUE>		(ImagesList&			targets,
																			 MemoryRegionsList&		memory,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	DE_UNREF(params);
	const deUint32						count								= static_cast<deUint32>(targets.size());
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();

	memory.reserve(count);
	for (deUint32 i = 0; i < count; ++i)
	{
		VkMemoryRequirements			memReqs;
		vk.getImageMemoryRequirements(vkDevice, **targets[i], &memReqs);

		ConstDedicatedInfo				dedicatedAllocationInfo				= makeDedicatedAllocationInfo(**targets[i]);
		const VkMemoryAllocateInfo		memAlloc							= makeMemoryAllocateInfo(memReqs, &dedicatedAllocationInfo);
		VkDeviceMemory					rawMemory							= DE_NULL;

		vk.allocateMemory(vkDevice, &memAlloc, static_cast<VkAllocationCallbacks*>(DE_NULL), &rawMemory);
		memory.push_back(MemoryRegionPtr(new Move<VkDeviceMemory>(check<VkDeviceMemory>(rawMemory), Deleter<VkDeviceMemory>(vk, vkDevice, DE_NULL))));
	}
}

template<typename TTarget>
void									makeBinding							(std::vector<de::SharedPtr<Move<TTarget> > >&
																									targets,
																			 MemoryRegionsList&		memory,
																			 Context&				ctx,
																			 BindingCaseParameters	params);

template<>
void									makeBinding<VkBuffer>				(BuffersList&			targets,
																			 MemoryRegionsList&		memory,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	DE_UNREF(params);
	const deUint32						count								= static_cast<deUint32>(targets.size());
	const VkDevice						vkDevice							= ctx.getDevice();
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	BindBufferMemoryInfosList			bindMemoryInfos;

	for (deUint32 i = 0; i < count; ++i)
	{
		bindMemoryInfos.push_back(makeBufferMemoryBindingInfo(**targets[i], **memory[i]));
	}

	VK_CHECK(vk.bindBufferMemory2KHR(vkDevice, count, &bindMemoryInfos.front()));
}

template<>
void									makeBinding<VkImage>				(ImagesList&			targets,
																			 MemoryRegionsList&		memory,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	DE_UNREF(params);
	const deUint32						count								= static_cast<deUint32>(targets.size());
	const VkDevice						vkDevice							= ctx.getDevice();
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	BindImageMemoryInfosList			bindMemoryInfos;

	for (deUint32 i = 0; i < count; ++i)
	{
		bindMemoryInfos.push_back(makeImageMemoryBindingInfo(**targets[i], **memory[i]));
	}

	VK_CHECK(vk.bindImageMemory2KHR(vkDevice, count, &bindMemoryInfos.front()));
}

template <typename TTarget>
void									fillUpResource						(Move<VkBuffer>&		source,
																			 Move<TTarget>&			target,
																			 Context&				ctx,
																			 BindingCaseParameters	params);

template <>
void									fillUpResource<VkBuffer>			(Move<VkBuffer>&		source,
																			 Move<VkBuffer>&		target,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();
	const VkQueue						queue								= ctx.getUniversalQueue();

	const VkBufferMemoryBarrier			srcBufferBarrier					= makeMemoryBarrierInfo(*source, params.bufferSize, TransferFromResource);
	const VkBufferMemoryBarrier			dstBufferBarrier					= makeMemoryBarrierInfo(*target, params.bufferSize, TransferToResource);

	const VkCommandBufferBeginInfo		cmdBufferBeginInfo					= makeCommandBufferInfo();
	Move<VkCommandPool>					commandPool							= createCommandPool(vk, vkDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, 0);
	Move<VkCommandBuffer>				cmdBuffer							= createCommandBuffer(vk, vkDevice, *commandPool);
	VkBufferCopy						bufferCopy							= { 0u, 0u, params.bufferSize };

	VK_CHECK(vk.beginCommandBuffer(*cmdBuffer, &cmdBufferBeginInfo));
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &srcBufferBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
	vk.cmdCopyBuffer(*cmdBuffer, *source, *target, 1, &bufferCopy);
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &dstBufferBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
	VK_CHECK(vk.endCommandBuffer(*cmdBuffer));

	const VkSubmitInfo					submitInfo							= makeSubmitInfo(*cmdBuffer);
	Move<VkFence>						fence								= createFence(vk, vkDevice);

	VK_CHECK(vk.queueSubmit(queue, 1, &submitInfo, *fence));
	VK_CHECK(vk.waitForFences(vkDevice, 1, &*fence, DE_TRUE, ~(0ull)));
}

template <>
void									fillUpResource<VkImage>				(Move<VkBuffer>&		source,
																			 Move<VkImage>&			target,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();
	const VkQueue						queue								= ctx.getUniversalQueue();

	const VkBufferMemoryBarrier			srcBufferBarrier					= makeMemoryBarrierInfo(*source, params.bufferSize, TransferFromResource);
	const VkImageMemoryBarrier			preImageBarrier						= makeMemoryBarrierInfo(*target, 0u, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	const VkImageMemoryBarrier			dstImageBarrier						= makeMemoryBarrierInfo(*target, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	const VkCommandBufferBeginInfo		cmdBufferBeginInfo					= makeCommandBufferInfo();
	Move<VkCommandPool>					commandPool							= createCommandPool(vk, vkDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, 0);
	Move<VkCommandBuffer>				cmdBuffer							= createCommandBuffer(vk, vkDevice, *commandPool);

	const VkBufferImageCopy				copyRegion							=
	{
		0u,																	// VkDeviceSize			bufferOffset;
		params.imageSize.width,												// deUint32				bufferRowLength;
		params.imageSize.height,											// deUint32				bufferImageHeight;
		{
			VK_IMAGE_ASPECT_COLOR_BIT,										// VkImageAspectFlags	aspect;
			0u,																// deUint32				mipLevel;
			0u,																// deUint32				baseArrayLayer;
			1u,																// deUint32				layerCount;
		},																	// VkImageSubresourceLayers imageSubresource;
		{ 0, 0, 0 },														// VkOffset3D			imageOffset;
		params.imageSize													// VkExtent3D			imageExtent;
	};

	VK_CHECK(vk.beginCommandBuffer(*cmdBuffer, &cmdBufferBeginInfo));
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &srcBufferBarrier, 1, &preImageBarrier);
	vk.cmdCopyBufferToImage(*cmdBuffer, *source, *target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, (&copyRegion));
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &dstImageBarrier);
	VK_CHECK(vk.endCommandBuffer(*cmdBuffer));

	const VkSubmitInfo					submitInfo							= makeSubmitInfo(*cmdBuffer);
	Move<VkFence>						fence								= createFence(vk, vkDevice);

	VK_CHECK(vk.queueSubmit(queue, 1, &submitInfo, *fence));
	VK_CHECK(vk.waitForFences(vkDevice, 1, &*fence, DE_TRUE, ~(0ull)));
}

template <typename TTarget>
void									readUpResource						(Move<TTarget>&			source,
																			 Move<VkBuffer>&		target,
																			 Context&				ctx,
																			 BindingCaseParameters	params);

template <>
void									readUpResource						(Move<VkBuffer>&		source,
																			 Move<VkBuffer>&		target,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	fillUpResource(source, target, ctx, params);
}

template <>
void									readUpResource						(Move<VkImage>&			source,
																			 Move<VkBuffer>&		target,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();
	const VkQueue						queue								= ctx.getUniversalQueue();

	const VkImageMemoryBarrier			srcImageBarrier						= makeMemoryBarrierInfo(*source, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	const VkBufferMemoryBarrier			dstBufferBarrier					= makeMemoryBarrierInfo(*target, params.bufferSize, TransferToResource);
	const VkImageMemoryBarrier			postImageBarrier					= makeMemoryBarrierInfo(*source, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	const VkCommandBufferBeginInfo		cmdBufferBeginInfo					= makeCommandBufferInfo();
	Move<VkCommandPool>					commandPool							= createCommandPool(vk, vkDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, 0);
	Move<VkCommandBuffer>				cmdBuffer							= createCommandBuffer(vk, vkDevice, *commandPool);

	const VkBufferImageCopy				copyRegion							=
	{
		0u,																	// VkDeviceSize			bufferOffset;
		params.imageSize.width,												// deUint32				bufferRowLength;
		params.imageSize.height,											// deUint32				bufferImageHeight;
		{
			VK_IMAGE_ASPECT_COLOR_BIT,										// VkImageAspectFlags	aspect;
			0u,																// deUint32				mipLevel;
			0u,																// deUint32				baseArrayLayer;
			1u,																// deUint32				layerCount;
		},																	// VkImageSubresourceLayers imageSubresource;
		{ 0, 0, 0 },														// VkOffset3D			imageOffset;
		params.imageSize													// VkExtent3D			imageExtent;
	};

	VK_CHECK(vk.beginCommandBuffer(*cmdBuffer, &cmdBufferBeginInfo));
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &srcImageBarrier);
	vk.cmdCopyImageToBuffer(*cmdBuffer, *source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *target, 1, (&copyRegion));
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &dstBufferBarrier, 1, &postImageBarrier);
	VK_CHECK(vk.endCommandBuffer(*cmdBuffer));

	const VkSubmitInfo					submitInfo							= makeSubmitInfo(*cmdBuffer);
	Move<VkFence>						fence								= createFence(vk, vkDevice);

	VK_CHECK(vk.queueSubmit(queue, 1, &submitInfo, *fence));
	VK_CHECK(vk.waitForFences(vkDevice, 1, &*fence, DE_TRUE, ~(0ull)));
}

void									createBuffer						(Move<VkBuffer>&		buffer,
																			 Move<VkDeviceMemory>&	memory,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();
	VkBufferCreateInfo					bufferParams						= makeBufferCreateInfo(ctx, params);
	VkMemoryRequirements				memReqs;

	buffer = createBuffer(vk, vkDevice, &bufferParams);
	vk.getBufferMemoryRequirements(vkDevice, *buffer, &memReqs);

	const VkMemoryAllocateInfo			memAlloc							= makeMemoryAllocateInfo(ctx, memReqs, MemoryHostVisible);
	VkDeviceMemory						rawMemory							= DE_NULL;

	vk.allocateMemory(vkDevice, &memAlloc, static_cast<VkAllocationCallbacks*>(DE_NULL), &rawMemory);
	memory = Move<VkDeviceMemory>(check<VkDeviceMemory>(rawMemory), Deleter<VkDeviceMemory>(vk, vkDevice, DE_NULL));
	VK_CHECK(vk.bindBufferMemory(vkDevice, *buffer, *memory, 0u));
}

void									pushData							(VkDeviceMemory			memory,
																			 deUint32				dataSeed,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();
	MemoryMappingRAII					hostMemory							(vk, vkDevice, memory, 0u, params.bufferSize, 0u);
	deUint8*							hostBuffer							= static_cast<deUint8*>(hostMemory.ptr());
	SimpleRandomGenerator				random								(dataSeed);

	for (deUint32 i = 0u; i < params.bufferSize; ++i)
	{
		hostBuffer[i] = static_cast<deUint8>(random.getNext() & 0xFFu);
	}
	hostMemory.flush(0u, params.bufferSize);
}

deBool									checkData							(VkDeviceMemory			memory,
																			 deUint32				dataSeed,
																			 Context&				ctx,
																			 BindingCaseParameters	params)
{
	const DeviceInterface&				vk									= ctx.getDeviceInterface();
	const VkDevice						vkDevice							= ctx.getDevice();
	MemoryMappingRAII					hostMemory							(vk, vkDevice, memory, 0u, params.bufferSize, 0u);
	deUint8*							hostBuffer							= static_cast<deUint8*>(hostMemory.ptr());
	SimpleRandomGenerator				random								(dataSeed);

	for (deUint32 i = 0u; i < params.bufferSize; ++i)
	{
		if (hostBuffer[i] != static_cast<deUint8>(random.getNext() & 0xFFu) )
			return DE_FALSE;
	}
	return DE_TRUE;
}

template<typename TTarget, deBool TDedicated>
class MemoryBindingInstance : public TestInstance
{
public:
										MemoryBindingInstance				(Context&				ctx,
																			 BindingCaseParameters	params)
										: TestInstance						(ctx)
										, m_params							(params)
	{
	}

	virtual tcu::TestStatus				iterate								(void)
	{
		const std::vector<std::string>&	extensions							= m_context.getDeviceExtensions();
		const deBool					isSupported							= std::find(extensions.begin(), extensions.end(), "VK_KHR_bind_memory2") != extensions.end();
		if (!isSupported)
		{
			TCU_THROW(NotSupportedError, "Not supported");
		}

		std::vector<de::SharedPtr<Move<TTarget> > >
										targets;
		MemoryRegionsList				memory;

		createBindingTargets<TTarget>(targets, m_context, m_params);
		createMemory<TTarget, TDedicated>(targets, memory, m_context, m_params);
		makeBinding<TTarget>(targets, memory, m_context, m_params);

		Move<VkBuffer>					srcBuffer;
		Move<VkDeviceMemory>			srcMemory;

		createBuffer(srcBuffer, srcMemory, m_context, m_params);
		pushData(*srcMemory, 1, m_context, m_params);

		Move<VkBuffer>					dstBuffer;
		Move<VkDeviceMemory>			dstMemory;

		createBuffer(dstBuffer, dstMemory, m_context, m_params);

		deBool							passed								= DE_TRUE;
		for (deUint32 i = 0; passed && i < m_params.targetsCount; ++i)
		{
			fillUpResource(srcBuffer, *targets[i], m_context, m_params);
			readUpResource(*targets[i], dstBuffer, m_context, m_params);
			passed = checkData(*dstMemory, 1, m_context, m_params);
		}

		return passed ? tcu::TestStatus::pass("Pass") : tcu::TestStatus::fail("Failed");
	}
private:
	BindingCaseParameters				m_params;
};

template<typename TTarget, deBool TDedicated>
class AliasedMemoryBindingInstance : public TestInstance
{
public:
										AliasedMemoryBindingInstance		(Context&				ctx,
																			 BindingCaseParameters	params)
										: TestInstance						(ctx)
										, m_params							(params)
	{
	}

	virtual tcu::TestStatus				iterate								(void)
	{
		const std::vector<std::string>&	extensions							= m_context.getDeviceExtensions();
		const deBool					isSupported							= std::find(extensions.begin(), extensions.end(), "VK_KHR_bind_memory2") != extensions.end();
		if (!isSupported)
		{
			TCU_THROW(NotSupportedError, "Not supported");
		}

		std::vector<de::SharedPtr<Move<TTarget> > >
										targets[2];
		MemoryRegionsList				memory;

		for (deUint32 i = 0; i < DE_LENGTH_OF_ARRAY(targets); ++i)
			createBindingTargets<TTarget>(targets[i], m_context, m_params);
		createMemory<TTarget, TDedicated>(targets[0], memory, m_context, m_params);
		for (deUint32 i = 0; i < DE_LENGTH_OF_ARRAY(targets); ++i)
			makeBinding<TTarget>(targets[i], memory, m_context, m_params);

		Move<VkBuffer>					srcBuffer;
		Move<VkDeviceMemory>			srcMemory;

		createBuffer(srcBuffer, srcMemory, m_context, m_params);
		pushData(*srcMemory, 2, m_context, m_params);

		Move<VkBuffer>					dstBuffer;
		Move<VkDeviceMemory>			dstMemory;

		createBuffer(dstBuffer, dstMemory, m_context, m_params);

		deBool							passed								= DE_TRUE;
		for (deUint32 i = 0; passed && i < m_params.targetsCount; ++i)
		{
			fillUpResource(srcBuffer, *(targets[0][i]), m_context, m_params);
			readUpResource(*(targets[1][i]), dstBuffer, m_context, m_params);
			passed = checkData(*dstMemory, 2, m_context, m_params);
		}

		return passed ? tcu::TestStatus::pass("Pass") : tcu::TestStatus::fail("Failed");
	}
private:
	BindingCaseParameters				m_params;
};

template<typename TInstance>
class MemoryBindingTest : public TestCase
{
public:
										MemoryBindingTest					(tcu::TestContext&		testCtx,
																			 const std::string&		name,
																			 const std::string&		description,
																			 BindingCaseParameters	params)
										: TestCase							(testCtx, name, description)
										, m_params							(params)
	{
	}

	virtual								~MemoryBindingTest					(void)
	{
	}

	virtual TestInstance*				createInstance						(Context&				ctx) const
	{
		return new TInstance(ctx, m_params);
	}

private:
	BindingCaseParameters				m_params;
};

} // unnamed namespace

tcu::TestCaseGroup* createMemoryBindingTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>		group								(new tcu::TestCaseGroup(testCtx, "binding", "Memory binding tests."));

	de::MovePtr<tcu::TestCaseGroup>		regular								(new tcu::TestCaseGroup(testCtx, "regular", "Basic memory binding tests."));
	de::MovePtr<tcu::TestCaseGroup>		aliasing							(new tcu::TestCaseGroup(testCtx, "aliasing", "Memory binding tests with aliasing of two resources."));

	de::MovePtr<tcu::TestCaseGroup>		regular_suballocated				(new tcu::TestCaseGroup(testCtx, "suballocated", "Basic memory binding tests with suballocated memory."));
	de::MovePtr<tcu::TestCaseGroup>		regular_dedicated					(new tcu::TestCaseGroup(testCtx, "dedicated", "Basic memory binding tests with deditatedly allocated memory."));

	de::MovePtr<tcu::TestCaseGroup>		aliasing_suballocated				(new tcu::TestCaseGroup(testCtx, "suballocated", "Memory binding tests with aliasing of two resources with suballocated mamory."));

	const VkDeviceSize					allocationSizes[]					= {	33, 257, 4087, 8095, 1*1024*1024 + 1	};

	for (deUint32 sizeNdx = 0u; sizeNdx < DE_LENGTH_OF_ARRAY(allocationSizes); ++sizeNdx )
	{
		const VkDeviceSize				bufferSize							= allocationSizes[sizeNdx];
		const BindingCaseParameters		params								= makeBindingCaseParameters(10, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, bufferSize);
		std::ostringstream				testName;

		testName << "buffer_" << bufferSize;
		regular_suballocated->addChild(new MemoryBindingTest<MemoryBindingInstance<VkBuffer, DE_FALSE> >(testCtx, testName.str(), " ", params));
		regular_dedicated->addChild(new MemoryBindingTest<MemoryBindingInstance<VkBuffer, DE_TRUE> >(testCtx, testName.str(), " ", params));
		aliasing_suballocated->addChild(new MemoryBindingTest<AliasedMemoryBindingInstance<VkBuffer, DE_FALSE> >(testCtx, testName.str(), " ", params));
	}

	const deUint32						imageSizes[]						= {	8, 33, 257	};

	for (deUint32 widthNdx = 0u; widthNdx < DE_LENGTH_OF_ARRAY(imageSizes); ++widthNdx )
	for (deUint32 heightNdx = 0u; heightNdx < DE_LENGTH_OF_ARRAY(imageSizes); ++heightNdx )
	{
		const deUint32					width								= imageSizes[widthNdx];
		const deUint32					height								= imageSizes[heightNdx];
		const BindingCaseParameters		regularparams						= makeBindingCaseParameters(10, width, height);
		std::ostringstream				testName;

		testName << "image_" << width << '_' << height;
		regular_suballocated->addChild(new MemoryBindingTest<MemoryBindingInstance<VkImage, DE_FALSE> >(testCtx, testName.str(), " ", regularparams));
		regular_dedicated->addChild(new MemoryBindingTest<MemoryBindingInstance<VkImage, DE_TRUE> >(testCtx, testName.str(), "", regularparams));
		aliasing_suballocated->addChild(new MemoryBindingTest<AliasedMemoryBindingInstance<VkImage, DE_FALSE> >(testCtx, testName.str(), " ", regularparams));
	}

	regular->addChild(regular_suballocated.release());
	regular->addChild(regular_dedicated.release());

	aliasing->addChild(aliasing_suballocated.release());

	group->addChild(regular.release());
	group->addChild(aliasing.release());

	return group.release();
}

} // memory
} // vkt
