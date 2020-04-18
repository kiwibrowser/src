#ifndef _VKTSYNCHRONIZATIONOPERATIONRESOURCES_HPP
#define _VKTSYNCHRONIZATIONOPERATIONRESOURCES_HPP
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
 * \brief Synchronization operation static data
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vkDefs.hpp"
#include "tcuVector.hpp"
#include "vktSynchronizationOperation.hpp"

namespace vkt
{
namespace synchronization
{

static const ResourceDescription s_resources[] =
{
	{ RESOURCE_TYPE_BUFFER,	tcu::IVec4( 0x4000, 0, 0, 0),	vk::VK_IMAGE_TYPE_LAST,	vk::VK_FORMAT_UNDEFINED,		(vk::VkImageAspectFlags)0	  },	// 16 KiB (min max UBO range)
	{ RESOURCE_TYPE_BUFFER,	tcu::IVec4(0x40000, 0, 0, 0),	vk::VK_IMAGE_TYPE_LAST,	vk::VK_FORMAT_UNDEFINED,		(vk::VkImageAspectFlags)0	  },	// 256 KiB

	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 0, 0, 0),		vk::VK_IMAGE_TYPE_1D,	vk::VK_FORMAT_R32_UINT,				vk::VK_IMAGE_ASPECT_COLOR_BIT },

	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R8_UNORM,				vk::VK_IMAGE_ASPECT_COLOR_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R16_UINT,				vk::VK_IMAGE_ASPECT_COLOR_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R8G8B8A8_UNORM,		vk::VK_IMAGE_ASPECT_COLOR_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R16G16B16A16_UINT,	vk::VK_IMAGE_ASPECT_COLOR_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R32G32B32A32_SFLOAT,	vk::VK_IMAGE_ASPECT_COLOR_BIT },

	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(64, 64, 8, 0),		vk::VK_IMAGE_TYPE_3D,	vk::VK_FORMAT_R32_SFLOAT,			vk::VK_IMAGE_ASPECT_COLOR_BIT },

	// \note Mixed depth/stencil formats add complexity in image<->buffer transfers (packing), so we just avoid them here
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_D16_UNORM,			vk::VK_IMAGE_ASPECT_DEPTH_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_D32_SFLOAT,			vk::VK_IMAGE_ASPECT_DEPTH_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_S8_UINT,				vk::VK_IMAGE_ASPECT_STENCIL_BIT },

	// \note Special resources, when test case isn't strictly a copy and comparison of some data
	{ RESOURCE_TYPE_INDIRECT_BUFFER_DRAW,			tcu::IVec4(sizeof(vk::VkDrawIndirectCommand),        0, 0, 0),	vk::VK_IMAGE_TYPE_LAST, vk::VK_FORMAT_UNDEFINED, (vk::VkImageAspectFlags)0	},
	{ RESOURCE_TYPE_INDIRECT_BUFFER_DRAW_INDEXED,	tcu::IVec4(sizeof(vk::VkDrawIndexedIndirectCommand), 0, 0, 0),	vk::VK_IMAGE_TYPE_LAST, vk::VK_FORMAT_UNDEFINED, (vk::VkImageAspectFlags)0	},
	{ RESOURCE_TYPE_INDIRECT_BUFFER_DISPATCH,		tcu::IVec4(sizeof(vk::VkDispatchIndirectCommand),    0, 0, 0),	vk::VK_IMAGE_TYPE_LAST, vk::VK_FORMAT_UNDEFINED, (vk::VkImageAspectFlags)0	},
};

} // synchronization
} // vkt

#endif // _VKTSYNCHRONIZATIONOPERATIONRESOURCES_HPP
