#ifndef _VKIMAGEWITHMEMORY_HPP
#define _VKIMAGEWITHMEMORY_HPP
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
 * \brief Image backed with memory
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"

namespace vk
{
class ImageWithMemory
{
public:
										ImageWithMemory	(const vk::DeviceInterface&		vk,
														 const vk::VkDevice				device,
														 vk::Allocator&					allocator,
														 const vk::VkImageCreateInfo&	imageCreateInfo,
														 const vk::MemoryRequirement	memoryRequirement)

											: m_image		(createImage(vk, device, &imageCreateInfo))
											, m_allocation	(allocator.allocate(getImageMemoryRequirements(vk, device, *m_image), memoryRequirement))
										{
											VK_CHECK(vk.bindImageMemory(device, *m_image, m_allocation->getMemory(), m_allocation->getOffset()));
										}

	const vk::VkImage&					get				(void) const { return *m_image; }
	const vk::VkImage&					operator*		(void) const { return get(); }
	vk::Allocation&						getAllocation	(void) const { return *m_allocation; }

private:
	const vk::Unique<vk::VkImage>		m_image;
	const de::UniquePtr<vk::Allocation>	m_allocation;

	// "deleted"
										ImageWithMemory	(const ImageWithMemory&);
	ImageWithMemory&					operator=		(const ImageWithMemory&);
};
} // vk

#endif // _VKIMAGEWITHMEMORY_HPP
