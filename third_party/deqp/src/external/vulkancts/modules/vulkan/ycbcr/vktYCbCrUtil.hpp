#ifndef _VKTYCBCRUTIL_HPP
#define _VKTYCBCRUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 Google Inc.
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
 * \brief YCbCr Test Utilities
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"

#include "vktTestCase.hpp"

#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkRef.hpp"

#include "deSharedPtr.hpp"
#include "deRandom.hpp"

namespace vkt
{
namespace ycbcr
{

#define VK_YCBCR_FORMAT_FIRST	VK_FORMAT_G8B8G8R8_422_UNORM_KHR
#define VK_YCBCR_FORMAT_LAST	((vk::VkFormat)(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR+1))

class MultiPlaneImageData
{
public:
										MultiPlaneImageData		(vk::VkFormat format, const tcu::UVec2& size);
										MultiPlaneImageData		(const MultiPlaneImageData&);
										~MultiPlaneImageData	(void);

	vk::VkFormat						getFormat				(void) const				{ return m_format;						}
	const vk::PlanarFormatDescription&	getDescription			(void) const				{ return m_description;					}
	const tcu::UVec2&					getSize					(void) const				{ return m_size;						}

	size_t								getPlaneSize			(deUint32 planeNdx) const	{ return m_planeData[planeNdx].size();	}
	void*								getPlanePtr				(deUint32 planeNdx)			{ return &m_planeData[planeNdx][0];		}
	const void*							getPlanePtr				(deUint32 planeNdx) const	{ return &m_planeData[planeNdx][0];		}

	tcu::PixelBufferAccess				getChannelAccess		(deUint32 channelNdx);
	tcu::ConstPixelBufferAccess			getChannelAccess		(deUint32 channelNdx) const;

private:
	MultiPlaneImageData&				operator=				(const MultiPlaneImageData&);

	const vk::VkFormat					m_format;
	const vk::PlanarFormatDescription	m_description;
	const tcu::UVec2					m_size;

	std::vector<deUint8>				m_planeData[vk::PlanarFormatDescription::MAX_PLANES];
};

void										checkImageSupport			(Context& context, vk::VkFormat format, vk::VkImageCreateFlags createFlags, vk::VkImageTiling tiling = vk::VK_IMAGE_TILING_OPTIMAL);

void										fillRandom					(de::Random* randomGen, MultiPlaneImageData* imageData);
void										fillGradient				(MultiPlaneImageData* imageData, const tcu::Vec4& minVal, const tcu::Vec4& maxVal);

std::vector<de::SharedPtr<vk::Allocation> >	allocateAndBindImageMemory	(const vk::DeviceInterface&	vkd,
																		 vk::VkDevice				device,
																		 vk::Allocator&				allocator,
																		 vk::VkImage				image,
																		 vk::VkFormat				format,
																		 vk::VkImageCreateFlags		createFlags,
																		 vk::MemoryRequirement		requirement = vk::MemoryRequirement::Any);

void										uploadImage					(const vk::DeviceInterface&	vkd,
																		 vk::VkDevice				device,
																		 deUint32					queueFamilyNdx,
																		 vk::Allocator&				allocator,
																		 vk::VkImage				image,
																		 const MultiPlaneImageData&	imageData,
																		 vk::VkAccessFlags			nextAccess,
																		 vk::VkImageLayout			finalLayout);

void										fillImageMemory				(const vk::DeviceInterface&							vkd,
																		 vk::VkDevice										device,
																		 deUint32											queueFamilyNdx,
																		 vk::VkImage										image,
																		 const std::vector<de::SharedPtr<vk::Allocation> >&	memory,
																		 const MultiPlaneImageData&							imageData,
																		 vk::VkAccessFlags									nextAccess,
																		 vk::VkImageLayout									finalLayout);

void										downloadImage				(const vk::DeviceInterface&	vkd,
																		 vk::VkDevice				device,
																		 deUint32					queueFamilyNdx,
																		 vk::Allocator&				allocator,
																		 vk::VkImage				image,
																		 MultiPlaneImageData*		imageData,
																		 vk::VkAccessFlags			prevAccess,
																		 vk::VkImageLayout			initialLayout);

void										readImageMemory				(const vk::DeviceInterface&							vkd,
																		 vk::VkDevice										device,
																		 deUint32											queueFamilyNdx,
																		 vk::VkImage										image,
																		 const std::vector<de::SharedPtr<vk::Allocation> >&	memory,
																		 MultiPlaneImageData*								imageData,
																		 vk::VkAccessFlags									prevAccess,
																		 vk::VkImageLayout									initialLayout);

} // ycbcr
} // vkt

#endif // _VKTYCBCRUTIL_HPP
