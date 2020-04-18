#ifndef _VKTAPICOMPUTEINSTANCERESULTBUFFER_HPP
#define _VKTAPICOMPUTEINSTANCERESULTBUFFER_HPP
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

#include "tcuDefs.hpp"
#include "tcuTestLog.hpp"
#include "deUniquePtr.hpp"
#include "vkRef.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"

namespace vkt
{

namespace api
{

class ComputeInstanceResultBuffer
{
public:
	enum
	{
		DATA_SIZE = sizeof(tcu::Vec4[4])
	};

											ComputeInstanceResultBuffer (const vk::DeviceInterface &vki,
																				vk::VkDevice device,
																				vk::Allocator &allocator,
																				float initValue = -1.0f);

	void									readResultContentsTo(tcu::Vec4 (* results)[4]) const;

	void									readResultContentsTo(deUint32* result) const;

	inline vk::VkBuffer						getBuffer(void) const { return *m_buffer; }

	inline const vk::VkBufferMemoryBarrier*	getResultReadBarrier(void) const { return &m_bufferBarrier; }

private:
	static vk::Move<vk::VkBuffer>			createResultBuffer(const vk::DeviceInterface &vki,
														vk::VkDevice device,
														vk::Allocator &allocator,
														de::MovePtr<vk::Allocation>* outAllocation,
														float initValue = -1.0f);

	static vk::VkBufferMemoryBarrier		createResultBufferBarrier(vk::VkBuffer buffer);

	const vk::DeviceInterface &				m_vki;
	const vk::VkDevice						m_device;

	de::MovePtr<vk::Allocation>				m_bufferMem;
	const vk::Unique<vk::VkBuffer>			m_buffer;
	const vk::VkBufferMemoryBarrier			m_bufferBarrier;
};

} // api
} // vkt

#endif // _VKTAPICOMPUTEINSTANCERESULTBUFFER_HPP
