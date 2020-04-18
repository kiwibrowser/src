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

#include "vktApiBufferComputeInstance.hpp"
#include "vktApiComputeInstanceResultBuffer.hpp"
#include "vkRefUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"

namespace vkt
{
namespace api
{

using namespace vk;

Move<VkBuffer> createDataBuffer (vkt::Context&				context,
								 deUint32					offset,
								 deUint32					bufferSize,
								 deUint32					initData,
								 deUint32					initDataSize,
								 deUint32					uninitData,
								 de::MovePtr<Allocation>*	outAllocation)
{
	const DeviceInterface&					vki             = context.getDeviceInterface();
	const VkDevice							device          = context.getDevice();
	Allocator&								allocator       = context.getDefaultAllocator();

	DE_ASSERT(offset + initDataSize <= bufferSize);

	const VkBufferUsageFlags				usageFlags      = (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	const VkBufferCreateInfo				createInfo      =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		DE_NULL,
		0u,															// flags
		(VkDeviceSize)bufferSize,									// size
		usageFlags,													// usage
		VK_SHARING_MODE_EXCLUSIVE,									// sharingMode
		0u,															// queueFamilyCount
		DE_NULL,													// pQueueFamilyIndices
	};
	Move<VkBuffer>							buffer(createBuffer(vki, device, &createInfo));

	const VkMemoryRequirements				requirements    = getBufferMemoryRequirements(vki, device, *buffer);
	de::MovePtr<Allocation>					allocation      = allocator.allocate(requirements, MemoryRequirement::HostVisible);

	VK_CHECK(vki.bindBufferMemory(device, *buffer, allocation->getMemory(), allocation->getOffset()));

	void* const								mapPtr          = allocation->getHostPtr();

	if (offset)
		deMemset(mapPtr, uninitData, (size_t)offset);

	deMemset((deUint8 *)mapPtr + offset, initData, initDataSize);
	deMemset((deUint8 *)mapPtr + offset + initDataSize, uninitData,
		(size_t)bufferSize - (size_t)offset - initDataSize);

	flushMappedMemoryRange(vki, device, allocation->getMemory(), allocation->getOffset(), bufferSize);

	*outAllocation = allocation;
	return buffer;
}

Move<VkBuffer> createColorDataBuffer (deUint32 offset,
									  deUint32 bufferSize,
									  const tcu::Vec4& color1,
									  const tcu::Vec4& color2,
									  de::MovePtr<Allocation>* outAllocation,
									  vkt::Context& context)
{
	const DeviceInterface&					vki						= context.getDeviceInterface();
	const VkDevice							device					= context.getDevice();
	Allocator&								allocator				= context.getDefaultAllocator();

	DE_ASSERT(offset + sizeof(tcu::Vec4[2]) <= bufferSize);

	const VkBufferUsageFlags				usageFlags				= (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	const VkBufferCreateInfo				createInfo				=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		DE_NULL,
		0u,															// flags
		(VkDeviceSize) bufferSize,									// size
		usageFlags,													// usage
		VK_SHARING_MODE_EXCLUSIVE,									// sharingMode
		0u,															// queueFamilyCount
		DE_NULL,													// pQueueFamilyIndices
	};
	Move<VkBuffer> buffer(createBuffer(vki, device, &createInfo));

	const VkMemoryRequirements				requirements			= getBufferMemoryRequirements(vki, device, *buffer);
	de::MovePtr<Allocation>					allocation				= allocator.allocate(requirements, MemoryRequirement::HostVisible);

	VK_CHECK(vki.bindBufferMemory(device, *buffer, allocation->getMemory(), allocation->getOffset()));

	void*									mapPtr					= allocation->getHostPtr();

	if (offset)
		deMemset(mapPtr, 0x5A, (size_t) offset);

	deMemcpy((deUint8 *) mapPtr + offset, color1.getPtr(), sizeof(tcu::Vec4));
	deMemcpy((deUint8 *) mapPtr + offset + sizeof(tcu::Vec4), color2.getPtr(), sizeof(tcu::Vec4));
	deMemset((deUint8 *) mapPtr + offset + 2 * sizeof(tcu::Vec4), 0x5A,
			 (size_t) bufferSize - (size_t) offset - 2 * sizeof(tcu::Vec4));

	flushMappedMemoryRange(vki, device, allocation->getMemory(), allocation->getOffset(), bufferSize);

	*outAllocation = allocation;
	return buffer;
}

Move<VkDescriptorSetLayout> createDescriptorSetLayout (vkt::Context& context)
{

	const DeviceInterface&					vki						= context.getDeviceInterface();
	const VkDevice							device					= context.getDevice();

	DescriptorSetLayoutBuilder				builder;

	builder.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
	builder.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

	return builder.build(vki, device);
}

Move<VkDescriptorPool> createDescriptorPool (vkt::Context& context)
{
	const DeviceInterface&					vki						= context.getDeviceInterface();
	const VkDevice							device					= context.getDevice();

	return vk::DescriptorPoolBuilder()
		.addType(vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1u)
		.build(vki, device, vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1);
}

Move<VkDescriptorSet> createDescriptorSet (vkt::Context& context,
											VkDescriptorPool pool,
											VkDescriptorSetLayout layout,
											VkBuffer buffer,
											deUint32 offset,
											VkBuffer resBuf)
{
	const DeviceInterface&					vki             = context.getDeviceInterface();
	const VkDevice							device          = context.getDevice();

	const vk::VkDescriptorBufferInfo		resultInfo      = makeDescriptorBufferInfo(resBuf, 0u, (vk::VkDeviceSize) ComputeInstanceResultBuffer::DATA_SIZE);
	const vk::VkDescriptorBufferInfo		bufferInfo      = makeDescriptorBufferInfo(buffer, (vk::VkDeviceSize)offset, (vk::VkDeviceSize)sizeof(tcu::Vec4[2]));

	const vk::VkDescriptorSetAllocateInfo	allocInfo       =
	{
		vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		DE_NULL,
		pool,
		1u,
		&layout
	};
	vk::Move<vk::VkDescriptorSet>			descriptorSet   = allocateDescriptorSet(vki, device, &allocInfo);

	DescriptorSetUpdateBuilder builder;

	// result
	builder.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultInfo);

	// buffer
	builder.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);

	builder.update(vki, device);
	return descriptorSet;
}

Move<VkDescriptorSet> createDescriptorSet (VkDescriptorPool pool,
										   VkDescriptorSetLayout layout,
										   VkBuffer viewA,
										   deUint32 offsetA,
										   VkBuffer viewB,
										   deUint32 offsetB,
										   VkBuffer resBuf,
										   vkt::Context& context)
{
	const DeviceInterface&					vki						= context.getDeviceInterface();
	const VkDevice							device					= context.getDevice();

	const vk::VkDescriptorBufferInfo		resultInfo				= makeDescriptorBufferInfo(resBuf, 0u, (vk::VkDeviceSize) ComputeInstanceResultBuffer::DATA_SIZE);
	const vk::VkDescriptorBufferInfo		bufferInfos[2]			=
	{
		vk::makeDescriptorBufferInfo(viewA, (vk::VkDeviceSize)offsetA, (vk::VkDeviceSize)sizeof(tcu::Vec4[2])),
		vk::makeDescriptorBufferInfo(viewB, (vk::VkDeviceSize)offsetB, (vk::VkDeviceSize)sizeof(tcu::Vec4[2])),
	};

	const vk::VkDescriptorSetAllocateInfo	allocInfo				=
	{
		vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		DE_NULL,
		pool,
		1u,
		&layout
	};
	vk::Move<vk::VkDescriptorSet>			descriptorSet			= allocateDescriptorSet(vki, device, &allocInfo);

	DescriptorSetUpdateBuilder builder;

	// result
	builder.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultInfo);

	// buffers
	builder.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfos[0]);

	builder.update(vki, device);
	return descriptorSet;
}

} // api
} // vkt
