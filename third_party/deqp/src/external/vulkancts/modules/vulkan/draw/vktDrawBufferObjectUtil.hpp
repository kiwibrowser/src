#ifndef _VKTDRAWBUFFEROBJECTUTIL_HPP
#define _VKTDRAWBUFFEROBJECTUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Intel Corporation
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
 * \brief Buffer Object Util
 *//*--------------------------------------------------------------------*/

#include "vkMemUtil.hpp"
#include "vkRefUtil.hpp"

#include "deSharedPtr.hpp"

namespace vkt
{
namespace Draw
{

class Buffer
{
public:

	static de::SharedPtr<Buffer> create			(const vk::DeviceInterface& vk, vk::VkDevice device, const vk::VkBufferCreateInfo &createInfo);

	static de::SharedPtr<Buffer> createAndAlloc (const vk::DeviceInterface&		vk,
												 vk::VkDevice					device,
												 const vk::VkBufferCreateInfo&	createInfo,
												 vk::Allocator&					allocator,
												 vk::MemoryRequirement			allocationMemoryProperties = vk::MemoryRequirement::Any);

								Buffer			(const vk::DeviceInterface &vk, vk::VkDevice device, vk::Move<vk::VkBuffer> object);

	void						bindMemory		(de::MovePtr<vk::Allocation> allocation);

	vk::VkBuffer				object			(void) const								{ return *m_object;		}
	vk::Allocation				getBoundMemory	(void) const								{ return *m_allocation;	}

private:

	Buffer										(const Buffer& other);	// Not allowed!
	Buffer&						operator=		(const Buffer& other);	// Not allowed!

	de::MovePtr<vk::Allocation>		m_allocation;
	vk::Unique<vk::VkBuffer>		m_object;

	const vk::DeviceInterface&		m_vk;
	vk::VkDevice					m_device;
};

} // Draw
} // vkt

#endif // _VKTDRAWBUFFEROBJECTUTIL_HPP
