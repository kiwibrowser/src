#ifndef _VKMEMUTIL_HPP
#define _VKMEMUTIL_HPP
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

#include "vkDefs.hpp"
#include "deUniquePtr.hpp"

namespace vk
{

/*--------------------------------------------------------------------*//*!
 * \brief Memory allocation interface
 *
 * Allocation represents block of device memory and is allocated by
 * Allocator implementation. Test code should use Allocator for allocating
 * memory, unless there is a reason not to (for example testing vkAllocMemory).
 *
 * Allocation doesn't necessarily correspond to a whole VkDeviceMemory, but
 * instead it may represent sub-allocation. Thus whenever VkDeviceMemory
 * (getMemory()) managed by Allocation is passed to Vulkan API calls,
 * offset given by getOffset() must be used.
 *
 * If host-visible memory was requested, host pointer to the memory can
 * be queried with getHostPtr(). No offset is needed when accessing host
 * pointer, i.e. the pointer is already adjusted in case of sub-allocation.
 *
 * Memory mappings are managed solely by Allocation, i.e. unmapping or
 * re-mapping VkDeviceMemory owned by Allocation is not allowed.
 *//*--------------------------------------------------------------------*/
class Allocation
{
public:
	virtual					~Allocation		(void);

	//! Get VkDeviceMemory backing this allocation
	VkDeviceMemory			getMemory		(void) const { return m_memory;							}

	//! Get offset in VkDeviceMemory for this allocation
	VkDeviceSize			getOffset		(void) const { return m_offset;							}

	//! Get host pointer for this allocation. Only available for host-visible allocations
	void*					getHostPtr		(void) const { DE_ASSERT(m_hostPtr); return m_hostPtr;	}

protected:
							Allocation		(VkDeviceMemory memory, VkDeviceSize offset, void* hostPtr);

private:
	const VkDeviceMemory	m_memory;
	const VkDeviceSize		m_offset;
	void* const				m_hostPtr;
};

//! Memory allocation requirements
class MemoryRequirement
{
public:
	static const MemoryRequirement	Any;
	static const MemoryRequirement	HostVisible;
	static const MemoryRequirement	Coherent;
	static const MemoryRequirement	LazilyAllocated;

	inline MemoryRequirement		operator|			(MemoryRequirement requirement) const
	{
		return MemoryRequirement(m_flags | requirement.m_flags);
	}

	inline MemoryRequirement		operator&			(MemoryRequirement requirement) const
	{
		return MemoryRequirement(m_flags & requirement.m_flags);
	}

	bool							matchesHeap			(VkMemoryPropertyFlags heapFlags) const;

	inline operator					bool				(void) const { return m_flags != 0u; }

private:
	explicit						MemoryRequirement	(deUint32 flags);

	const deUint32					m_flags;

	enum Flags
	{
		FLAG_HOST_VISIBLE		= 1u << 0u,
		FLAG_COHERENT			= 1u << 1u,
		FLAG_LAZY_ALLOCATION	= 1u << 2u,
	};
};

//! Memory allocator interface
class Allocator
{
public:
									Allocator	(void) {}
	virtual							~Allocator	(void) {}

	virtual de::MovePtr<Allocation>	allocate	(const VkMemoryAllocateInfo& allocInfo, VkDeviceSize alignment) = 0;
	virtual de::MovePtr<Allocation>	allocate	(const VkMemoryRequirements& memRequirements, MemoryRequirement requirement) = 0;
};

//! Allocator that backs every allocation with its own VkDeviceMemory
class SimpleAllocator : public Allocator
{
public:
											SimpleAllocator	(const DeviceInterface& vk, VkDevice device, const VkPhysicalDeviceMemoryProperties& deviceMemProps);

	de::MovePtr<Allocation>					allocate		(const VkMemoryAllocateInfo& allocInfo, VkDeviceSize alignment);
	de::MovePtr<Allocation>					allocate		(const VkMemoryRequirements& memRequirements, MemoryRequirement requirement);

private:
	const DeviceInterface&					m_vk;
	const VkDevice							m_device;
	const VkPhysicalDeviceMemoryProperties	m_memProps;
};

de::MovePtr<Allocation>	allocateDedicated			(const InstanceInterface& vki, const DeviceInterface& vkd, const VkPhysicalDevice& physDevice, const VkDevice device, const VkBuffer buffer, MemoryRequirement requirement);
de::MovePtr<Allocation>	allocateDedicated			(const InstanceInterface& vki, const DeviceInterface& vkd, const VkPhysicalDevice& physDevice, const VkDevice device, const VkImage image, MemoryRequirement requirement);

void*					mapMemory					(const DeviceInterface& vkd, VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags);
void					flushMappedMemoryRange		(const DeviceInterface& vkd, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size);
void					invalidateMappedMemoryRange	(const DeviceInterface& vkd, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size);
deUint32				getCompatibleMemoryTypes	(const VkPhysicalDeviceMemoryProperties& deviceMemProps, MemoryRequirement requirement);
void					bindImagePlaneMemory		(const DeviceInterface&	vkd, VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset, VkImageAspectFlagBits planeAspect);

} // vk

#endif // _VKMEMUTIL_HPP
