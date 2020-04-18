#ifndef _VKTAPIBUFFERANDIMAGEALLOCATIONUTIL_HPP
#define _VKTAPIBUFFERANDIMAGEALLOCATIONUTIL_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \brief Utility classes for various kinds of allocation memory for buffers and images
 *//*--------------------------------------------------------------------*/

#include "deUniquePtr.hpp"
#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vktTestCase.hpp"
#include "tcuVectorType.hpp"

namespace vk
{
class Allocator;
class MemoryRequirement;
class Allocation;
}

namespace vkt
{

namespace api
{

using namespace vk;

class IBufferAllocator
{
public:
	virtual void						createTestBuffer				(VkDeviceSize				size,
																		 VkBufferUsageFlags			usage,
																		 Context&					context,
																		 Allocator&			allocator,
																		 Move<VkBuffer>&			buffer,
																		 const MemoryRequirement&	requirement,
																		 de::MovePtr<Allocation>&	memory) const = 0;
};

class BufferSuballocation : public IBufferAllocator
{
public:
	virtual void						createTestBuffer				(VkDeviceSize				size,
																		 VkBufferUsageFlags			usage,
																		 Context&					context,
																		 Allocator&					allocator,
																		 Move<VkBuffer>&			buffer,
																		 const MemoryRequirement&	requirement,
																		 de::MovePtr<Allocation>&	memory) const; // override
};

class BufferDedicatedAllocation	: public IBufferAllocator
{
public:
	virtual void						createTestBuffer				(VkDeviceSize				size,
																		 VkBufferUsageFlags			usage,
																		 Context&					context,
																		 Allocator&					allocator,
																		 Move<VkBuffer>&			buffer,
																		 const MemoryRequirement&	requirement,
																		 de::MovePtr<Allocation>&	memory) const; // override
};

class IImageAllocator
{
public:
	virtual void						createTestImage					(tcu::IVec2					size,
																		 VkFormat					format,
																		 Context&					context,
																		 Allocator&					allocator,
																		 Move<VkImage>&				image,
																		 const MemoryRequirement&	requirement,
																		 de::MovePtr<Allocation>&	memory) const = 0;
};

class ImageSuballocation : public IImageAllocator
{
public:
	virtual void						createTestImage					(tcu::IVec2					size,
																		 VkFormat					format,
																		 Context&					context,
																		 Allocator&					allocator,
																		 Move<VkImage>&				image,
																		 const MemoryRequirement&	requirement,
																		 de::MovePtr<Allocation>&	memory) const; // override
};

class ImageDedicatedAllocation	: public IImageAllocator
{
public:
	virtual void						createTestImage					(tcu::IVec2					size,
																		 VkFormat					format,
																		 Context&					context,
																		 Allocator&					allocator,
																		 Move<VkImage>&				image,
																		 const MemoryRequirement&	requirement,
																		 de::MovePtr<Allocation>&	memory) const; // override
};

} // api
} // vkt

#endif // _VKTAPIBUFFERANDIMAGEALLOCATIONUTIL_HPP
