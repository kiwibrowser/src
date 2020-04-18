/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
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
 *//*!
 * \file
 * \brief Memory management utilities.
 *//*--------------------------------------------------------------------*/

#include "vkMemUtil.hpp"
#include "vkStrUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "deInt32.h"

#include <sstream>

namespace vk
{

using de::UniquePtr;
using de::MovePtr;

namespace
{

class HostPtr
{
public:
								HostPtr		(const DeviceInterface& vkd, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags);
								~HostPtr	(void);

	void*						get			(void) const { return m_ptr; }

private:
	const DeviceInterface&		m_vkd;
	const VkDevice				m_device;
	const VkDeviceMemory		m_memory;
	void* const					m_ptr;
};

HostPtr::HostPtr (const DeviceInterface& vkd, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags)
	: m_vkd		(vkd)
	, m_device	(device)
	, m_memory	(memory)
	, m_ptr		(mapMemory(vkd, device, memory, offset, size, flags))
{
}

HostPtr::~HostPtr (void)
{
	m_vkd.unmapMemory(m_device, m_memory);
}

deUint32 selectMatchingMemoryType (const VkPhysicalDeviceMemoryProperties& deviceMemProps, deUint32 allowedMemTypeBits, MemoryRequirement requirement)
{
	const deUint32	compatibleTypes	= getCompatibleMemoryTypes(deviceMemProps, requirement);
	const deUint32	candidates		= allowedMemTypeBits & compatibleTypes;

	if (candidates == 0)
		TCU_THROW(NotSupportedError, "No compatible memory type found");

	return (deUint32)deCtz32(candidates);
}

bool isHostVisibleMemory (const VkPhysicalDeviceMemoryProperties& deviceMemProps, deUint32 memoryTypeNdx)
{
	DE_ASSERT(memoryTypeNdx < deviceMemProps.memoryTypeCount);
	return (deviceMemProps.memoryTypes[memoryTypeNdx].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0u;
}

} // anonymous

// Allocation

Allocation::Allocation (VkDeviceMemory memory, VkDeviceSize offset, void* hostPtr)
	: m_memory	(memory)
	, m_offset	(offset)
	, m_hostPtr	(hostPtr)
{
}

Allocation::~Allocation (void)
{
}

// MemoryRequirement

const MemoryRequirement MemoryRequirement::Any				= MemoryRequirement(0x0u);
const MemoryRequirement MemoryRequirement::HostVisible		= MemoryRequirement(MemoryRequirement::FLAG_HOST_VISIBLE);
const MemoryRequirement MemoryRequirement::Coherent			= MemoryRequirement(MemoryRequirement::FLAG_COHERENT);
const MemoryRequirement MemoryRequirement::LazilyAllocated	= MemoryRequirement(MemoryRequirement::FLAG_LAZY_ALLOCATION);

bool MemoryRequirement::matchesHeap (VkMemoryPropertyFlags heapFlags) const
{
	// sanity check
	if ((m_flags & FLAG_COHERENT) && !(m_flags & FLAG_HOST_VISIBLE))
		DE_FATAL("Coherent memory must be host-visible");
	if ((m_flags & FLAG_HOST_VISIBLE) && (m_flags & FLAG_LAZY_ALLOCATION))
		DE_FATAL("Lazily allocated memory cannot be mappable");

	// host-visible
	if ((m_flags & FLAG_HOST_VISIBLE) && !(heapFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		return false;

	// coherent
	if ((m_flags & FLAG_COHERENT) && !(heapFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		return false;

	// lazy
	if ((m_flags & FLAG_LAZY_ALLOCATION) && !(heapFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT))
		return false;

	return true;
}

MemoryRequirement::MemoryRequirement (deUint32 flags)
	: m_flags(flags)
{
}

// SimpleAllocator

class SimpleAllocation : public Allocation
{
public:
									SimpleAllocation	(Move<VkDeviceMemory> mem, MovePtr<HostPtr> hostPtr);
	virtual							~SimpleAllocation	(void);

private:
	const Unique<VkDeviceMemory>	m_memHolder;
	const UniquePtr<HostPtr>		m_hostPtr;
};

SimpleAllocation::SimpleAllocation (Move<VkDeviceMemory> mem, MovePtr<HostPtr> hostPtr)
	: Allocation	(*mem, (VkDeviceSize)0, hostPtr ? hostPtr->get() : DE_NULL)
	, m_memHolder	(mem)
	, m_hostPtr		(hostPtr)
{
}

SimpleAllocation::~SimpleAllocation (void)
{
}

SimpleAllocator::SimpleAllocator (const DeviceInterface& vk, VkDevice device, const VkPhysicalDeviceMemoryProperties& deviceMemProps)
	: m_vk		(vk)
	, m_device	(device)
	, m_memProps(deviceMemProps)
{
}

MovePtr<Allocation> SimpleAllocator::allocate (const VkMemoryAllocateInfo& allocInfo, VkDeviceSize alignment)
{
	DE_UNREF(alignment);

	Move<VkDeviceMemory>	mem		= allocateMemory(m_vk, m_device, &allocInfo);
	MovePtr<HostPtr>		hostPtr;

	if (isHostVisibleMemory(m_memProps, allocInfo.memoryTypeIndex))
		hostPtr = MovePtr<HostPtr>(new HostPtr(m_vk, m_device, *mem, 0u, allocInfo.allocationSize, 0u));

	return MovePtr<Allocation>(new SimpleAllocation(mem, hostPtr));
}

MovePtr<Allocation> SimpleAllocator::allocate (const VkMemoryRequirements& memReqs, MemoryRequirement requirement)
{
	const deUint32				memoryTypeNdx	= selectMatchingMemoryType(m_memProps, memReqs.memoryTypeBits, requirement);
	const VkMemoryAllocateInfo	allocInfo		=
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	//	VkStructureType			sType;
		DE_NULL,								//	const void*				pNext;
		memReqs.size,							//	VkDeviceSize			allocationSize;
		memoryTypeNdx,							//	deUint32				memoryTypeIndex;
	};

	Move<VkDeviceMemory>		mem				= allocateMemory(m_vk, m_device, &allocInfo);
	MovePtr<HostPtr>			hostPtr;

	if (requirement & MemoryRequirement::HostVisible)
	{
		DE_ASSERT(isHostVisibleMemory(m_memProps, allocInfo.memoryTypeIndex));
		hostPtr = MovePtr<HostPtr>(new HostPtr(m_vk, m_device, *mem, 0u, allocInfo.allocationSize, 0u));
	}

	return MovePtr<Allocation>(new SimpleAllocation(mem, hostPtr));
}

static MovePtr<Allocation> allocateDedicated (const InstanceInterface&		vki,
											  const DeviceInterface&		vkd,
											  const VkPhysicalDevice&		physDevice,
											  const VkDevice				device,
											  const VkMemoryRequirements&	memReqs,
											  const MemoryRequirement		requirement,
											  const void*					pNext)
{
	const VkPhysicalDeviceMemoryProperties	memoryProperties	= getPhysicalDeviceMemoryProperties(vki, physDevice);
	const deUint32							memoryTypeNdx		= selectMatchingMemoryType(memoryProperties, memReqs.memoryTypeBits, requirement);
	const VkMemoryAllocateInfo				allocInfo			=
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	//	VkStructureType	sType
		pNext,									//	const void*		pNext
		memReqs.size,							//	VkDeviceSize	allocationSize
		memoryTypeNdx,							//	deUint32		memoryTypeIndex
	};
	Move<VkDeviceMemory>					mem					= allocateMemory(vkd, device, &allocInfo);
	MovePtr<HostPtr>						hostPtr;

	if (requirement & MemoryRequirement::HostVisible)
	{
		DE_ASSERT(isHostVisibleMemory(memoryProperties, allocInfo.memoryTypeIndex));
		hostPtr = MovePtr<HostPtr>(new HostPtr(vkd, device, *mem, 0u, allocInfo.allocationSize, 0u));
	}

	return MovePtr<Allocation>(new SimpleAllocation(mem, hostPtr));
}

de::MovePtr<Allocation> allocateDedicated (const InstanceInterface&	vki,
										   const DeviceInterface&	vkd,
										   const VkPhysicalDevice&	physDevice,
										   const VkDevice			device,
										   const VkBuffer			buffer,
										   MemoryRequirement		requirement)
{
	const VkMemoryRequirements					memoryRequirements		= getBufferMemoryRequirements(vkd, device, buffer);
	const VkMemoryDedicatedAllocateInfoKHR		dedicatedAllocationInfo	=
	{
		VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,				// VkStructureType		sType
		DE_NULL,															// const void*			pNext
		DE_NULL,															// VkImage				image
		buffer																// VkBuffer				buffer
	};

	return allocateDedicated(vki, vkd, physDevice, device, memoryRequirements, requirement, &dedicatedAllocationInfo);
}

de::MovePtr<Allocation> allocateDedicated (const InstanceInterface&	vki,
										   const DeviceInterface&	vkd,
										   const VkPhysicalDevice&	physDevice,
										   const VkDevice			device,
										   const VkImage			image,
										   MemoryRequirement		requirement)
{
	const VkMemoryRequirements				memoryRequirements		= getImageMemoryRequirements(vkd, device, image);
	const VkMemoryDedicatedAllocateInfoKHR	dedicatedAllocationInfo	=
	{
		VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,			// VkStructureType		sType
		DE_NULL,														// const void*			pNext
		image,															// VkImage				image
		DE_NULL															// VkBuffer				buffer
	};

	return allocateDedicated(vki, vkd, physDevice, device, memoryRequirements, requirement, &dedicatedAllocationInfo);
}

void* mapMemory (const DeviceInterface& vkd, VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags)
{
	void* hostPtr = DE_NULL;
	VK_CHECK(vkd.mapMemory(device, mem, offset, size, flags, &hostPtr));
	TCU_CHECK(hostPtr);
	return hostPtr;
}

void flushMappedMemoryRange (const DeviceInterface& vkd, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size)
{
	const VkMappedMemoryRange	range	=
	{
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		DE_NULL,
		memory,
		offset,
		size
	};

	VK_CHECK(vkd.flushMappedMemoryRanges(device, 1u, &range));
}

void invalidateMappedMemoryRange (const DeviceInterface& vkd, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size)
{
	const VkMappedMemoryRange	range	=
	{
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		DE_NULL,
		memory,
		offset,
		size
	};

	VK_CHECK(vkd.invalidateMappedMemoryRanges(device, 1u, &range));
}

deUint32 getCompatibleMemoryTypes (const VkPhysicalDeviceMemoryProperties& deviceMemProps, MemoryRequirement requirement)
{
	deUint32	compatibleTypes	= 0u;

	for (deUint32 memoryTypeNdx = 0; memoryTypeNdx < deviceMemProps.memoryTypeCount; memoryTypeNdx++)
	{
		if (requirement.matchesHeap(deviceMemProps.memoryTypes[memoryTypeNdx].propertyFlags))
			compatibleTypes |= (1u << memoryTypeNdx);
	}

	return compatibleTypes;
}

void bindImagePlaneMemory (const DeviceInterface&	vkd,
						   VkDevice					device,
						   VkImage					image,
						   VkDeviceMemory			memory,
						   VkDeviceSize				memoryOffset,
						   VkImageAspectFlagBits	planeAspect)
{
	const VkBindImagePlaneMemoryInfoKHR	planeInfo	=
	{
		VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO_KHR,
		DE_NULL,
		planeAspect
	};
	const VkBindImageMemoryInfoKHR		coreInfo	=
	{
		VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO_KHR,
		&planeInfo,
		image,
		memory,
		memoryOffset,
	};

	VK_CHECK(vkd.bindImageMemory2KHR(device, 1u, &coreInfo));
}

} // vk
