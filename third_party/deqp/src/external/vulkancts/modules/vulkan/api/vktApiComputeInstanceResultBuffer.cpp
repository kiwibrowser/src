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

#include "vktApiComputeInstanceResultBuffer.hpp"
#include "vktApiBufferComputeInstance.hpp"
#include "vkRefUtil.hpp"

namespace vkt
{
namespace api
{

using namespace vk;

ComputeInstanceResultBuffer::ComputeInstanceResultBuffer (const DeviceInterface &vki,
																	  VkDevice device,
																	  Allocator &allocator,
																	  float initValue)
		: m_vki(vki),
		m_device(device),
		m_bufferMem(DE_NULL),
		m_buffer(createResultBuffer(m_vki, m_device, allocator, &m_bufferMem, initValue)),
		m_bufferBarrier(createResultBufferBarrier(*m_buffer))
{
}

void ComputeInstanceResultBuffer::readResultContentsTo(tcu::Vec4 (*results)[4]) const
{
	invalidateMappedMemoryRange(m_vki, m_device, m_bufferMem->getMemory(), m_bufferMem->getOffset(), sizeof(*results));
	deMemcpy(*results, m_bufferMem->getHostPtr(), sizeof(*results));
}

void ComputeInstanceResultBuffer::readResultContentsTo(deUint32 *result) const
{
	invalidateMappedMemoryRange(m_vki, m_device, m_bufferMem->getMemory(), m_bufferMem->getOffset(), sizeof(*result));
	deMemcpy(result, m_bufferMem->getHostPtr(), sizeof(*result));
}

Move<VkBuffer> ComputeInstanceResultBuffer::createResultBuffer(const DeviceInterface &vki,
																	 VkDevice device,
																	 Allocator &allocator,
																	 de::MovePtr<Allocation> *outAllocation,
																	 float initValue)
{
	const VkBufferCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		DE_NULL,
		0u,															// flags
		(VkDeviceSize) DATA_SIZE,									// size
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,							// usage
		VK_SHARING_MODE_EXCLUSIVE,									// sharingMode
		0u,															// queueFamilyCount
		DE_NULL,													// pQueueFamilyIndices
	};

	Move<VkBuffer> buffer(createBuffer(vki, device, &createInfo));

	const VkMemoryRequirements				requirements			= getBufferMemoryRequirements(vki, device, *buffer);
	de::MovePtr<Allocation>					allocation				= allocator.allocate(requirements, MemoryRequirement::HostVisible);

	VK_CHECK(vki.bindBufferMemory(device, *buffer, allocation->getMemory(), allocation->getOffset()));

	const float								clearValue				= initValue;
	void*									mapPtr					= allocation->getHostPtr();

	for (size_t offset = 0; offset < DATA_SIZE; offset += sizeof(float))
		deMemcpy(((deUint8 *) mapPtr) + offset, &clearValue, sizeof(float));

	flushMappedMemoryRange(vki, device, allocation->getMemory(), allocation->getOffset(), (VkDeviceSize) DATA_SIZE);

	*outAllocation = allocation;
	return buffer;
}

VkBufferMemoryBarrier ComputeInstanceResultBuffer::createResultBufferBarrier(VkBuffer buffer)
{
	const VkBufferMemoryBarrier bufferBarrier =
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		DE_NULL,
		VK_ACCESS_SHADER_WRITE_BIT,									// srcAccessMask
		VK_ACCESS_HOST_READ_BIT,									// dstAccessMask
		VK_QUEUE_FAMILY_IGNORED,									// srcQueueFamilyIndex
		VK_QUEUE_FAMILY_IGNORED,									// destQueueFamilyIndex
		buffer,														// buffer
		(VkDeviceSize) 0u,											// offset
		DATA_SIZE,													// size
	};

	return bufferBarrier;
}

} // api
} // vkt
