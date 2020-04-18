/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
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
 * \brief Pipeline barrier tests
 *//*--------------------------------------------------------------------*/

#include "vktMemoryPipelineBarrierTests.hpp"

#include "vktTestCaseUtil.hpp"

#include "vkDefs.hpp"
#include "vkPlatform.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkPrograms.hpp"

#include "tcuMaybe.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuTestLog.hpp"
#include "tcuResultCollector.hpp"
#include "tcuTexture.hpp"
#include "tcuImageCompare.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"

#include "deInt32.h"
#include "deMath.h"
#include "deMemory.h"

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using tcu::TestLog;
using tcu::Maybe;

using de::MovePtr;

using std::string;
using std::vector;
using std::map;
using std::set;
using std::pair;

using tcu::IVec2;
using tcu::UVec2;
using tcu::UVec4;
using tcu::Vec4;
using tcu::ConstPixelBufferAccess;
using tcu::PixelBufferAccess;
using tcu::TextureFormat;
using tcu::TextureLevel;

namespace vkt
{
namespace memory
{
namespace
{
enum
{
	MAX_UNIFORM_BUFFER_SIZE = 1024,
	MAX_STORAGE_BUFFER_SIZE = (1<<28),
	MAX_SIZE = (128 * 1024)
};

// \todo [mika] Add to utilities
template<typename T>
T divRoundUp (const T& a, const T& b)
{
	return (a / b) + (a % b == 0 ? 0 : 1);
}

enum
{
	ALL_PIPELINE_STAGES = vk::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
						| vk::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
						| vk::VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
						| vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
						| vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
						| vk::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
						| vk::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
						| vk::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT
						| vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
						| vk::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
						| vk::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
						| vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
						| vk::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
						| vk::VK_PIPELINE_STAGE_TRANSFER_BIT
						| vk::VK_PIPELINE_STAGE_HOST_BIT
};

enum
{
	ALL_ACCESSES = vk::VK_ACCESS_INDIRECT_COMMAND_READ_BIT
				 | vk::VK_ACCESS_INDEX_READ_BIT
				 | vk::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
				 | vk::VK_ACCESS_UNIFORM_READ_BIT
				 | vk::VK_ACCESS_INPUT_ATTACHMENT_READ_BIT
				 | vk::VK_ACCESS_SHADER_READ_BIT
				 | vk::VK_ACCESS_SHADER_WRITE_BIT
				 | vk::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				 | vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
				 | vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
				 | vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
				 | vk::VK_ACCESS_TRANSFER_READ_BIT
				 | vk::VK_ACCESS_TRANSFER_WRITE_BIT
				 | vk::VK_ACCESS_HOST_READ_BIT
				 | vk::VK_ACCESS_HOST_WRITE_BIT
				 | vk::VK_ACCESS_MEMORY_READ_BIT
				 | vk::VK_ACCESS_MEMORY_WRITE_BIT
};

enum Usage
{
	// Mapped host read and write
	USAGE_HOST_READ = (0x1u<<0),
	USAGE_HOST_WRITE = (0x1u<<1),

	// Copy and other transfer operations
	USAGE_TRANSFER_SRC = (0x1u<<2),
	USAGE_TRANSFER_DST = (0x1u<<3),

	// Buffer usage flags
	USAGE_INDEX_BUFFER = (0x1u<<4),
	USAGE_VERTEX_BUFFER = (0x1u<<5),

	USAGE_UNIFORM_BUFFER = (0x1u<<6),
	USAGE_STORAGE_BUFFER = (0x1u<<7),

	USAGE_UNIFORM_TEXEL_BUFFER = (0x1u<<8),
	USAGE_STORAGE_TEXEL_BUFFER = (0x1u<<9),

	// \todo [2016-03-09 mika] This is probably almost impossible to do
	USAGE_INDIRECT_BUFFER = (0x1u<<10),

	// Texture usage flags
	USAGE_SAMPLED_IMAGE = (0x1u<<11),
	USAGE_STORAGE_IMAGE = (0x1u<<12),
	USAGE_COLOR_ATTACHMENT = (0x1u<<13),
	USAGE_INPUT_ATTACHMENT = (0x1u<<14),
	USAGE_DEPTH_STENCIL_ATTACHMENT = (0x1u<<15),
};

bool supportsDeviceBufferWrites (Usage usage)
{
	if (usage & USAGE_TRANSFER_DST)
		return true;

	if (usage & USAGE_STORAGE_BUFFER)
		return true;

	if (usage & USAGE_STORAGE_TEXEL_BUFFER)
		return true;

	return false;
}

bool supportsDeviceImageWrites (Usage usage)
{
	if (usage & USAGE_TRANSFER_DST)
		return true;

	if (usage & USAGE_STORAGE_IMAGE)
		return true;

	if (usage & USAGE_COLOR_ATTACHMENT)
		return true;

	return false;
}

// Sequential access enums
enum Access
{
	ACCESS_INDIRECT_COMMAND_READ_BIT = 0,
	ACCESS_INDEX_READ_BIT,
	ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
	ACCESS_UNIFORM_READ_BIT,
	ACCESS_INPUT_ATTACHMENT_READ_BIT,
	ACCESS_SHADER_READ_BIT,
	ACCESS_SHADER_WRITE_BIT,
	ACCESS_COLOR_ATTACHMENT_READ_BIT,
	ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
	ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	ACCESS_TRANSFER_READ_BIT,
	ACCESS_TRANSFER_WRITE_BIT,
	ACCESS_HOST_READ_BIT,
	ACCESS_HOST_WRITE_BIT,
	ACCESS_MEMORY_READ_BIT,
	ACCESS_MEMORY_WRITE_BIT,

	ACCESS_LAST
};

// Sequential stage enums
enum PipelineStage
{
	PIPELINESTAGE_TOP_OF_PIPE_BIT = 0,
	PIPELINESTAGE_BOTTOM_OF_PIPE_BIT,
	PIPELINESTAGE_DRAW_INDIRECT_BIT,
	PIPELINESTAGE_VERTEX_INPUT_BIT,
	PIPELINESTAGE_VERTEX_SHADER_BIT,
	PIPELINESTAGE_TESSELLATION_CONTROL_SHADER_BIT,
	PIPELINESTAGE_TESSELLATION_EVALUATION_SHADER_BIT,
	PIPELINESTAGE_GEOMETRY_SHADER_BIT,
	PIPELINESTAGE_FRAGMENT_SHADER_BIT,
	PIPELINESTAGE_EARLY_FRAGMENT_TESTS_BIT,
	PIPELINESTAGE_LATE_FRAGMENT_TESTS_BIT,
	PIPELINESTAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	PIPELINESTAGE_COMPUTE_SHADER_BIT,
	PIPELINESTAGE_TRANSFER_BIT,
	PIPELINESTAGE_HOST_BIT,

	PIPELINESTAGE_LAST
};

PipelineStage pipelineStageFlagToPipelineStage (vk::VkPipelineStageFlagBits flags)
{
	switch (flags)
	{
		case vk::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:						return PIPELINESTAGE_TOP_OF_PIPE_BIT;
		case vk::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT:					return PIPELINESTAGE_BOTTOM_OF_PIPE_BIT;
		case vk::VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT:					return PIPELINESTAGE_DRAW_INDIRECT_BIT;
		case vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT:					return PIPELINESTAGE_VERTEX_INPUT_BIT;
		case vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT:					return PIPELINESTAGE_VERTEX_SHADER_BIT;
		case vk::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT:		return PIPELINESTAGE_TESSELLATION_CONTROL_SHADER_BIT;
		case vk::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT:	return PIPELINESTAGE_TESSELLATION_EVALUATION_SHADER_BIT;
		case vk::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:					return PIPELINESTAGE_GEOMETRY_SHADER_BIT;
		case vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:					return PIPELINESTAGE_FRAGMENT_SHADER_BIT;
		case vk::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT:			return PIPELINESTAGE_EARLY_FRAGMENT_TESTS_BIT;
		case vk::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT:				return PIPELINESTAGE_LATE_FRAGMENT_TESTS_BIT;
		case vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:			return PIPELINESTAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		case vk::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT:					return PIPELINESTAGE_COMPUTE_SHADER_BIT;
		case vk::VK_PIPELINE_STAGE_TRANSFER_BIT:						return PIPELINESTAGE_TRANSFER_BIT;
		case vk::VK_PIPELINE_STAGE_HOST_BIT:							return PIPELINESTAGE_HOST_BIT;

		default:
			DE_FATAL("Unknown pipeline stage flags");
			return PIPELINESTAGE_LAST;
	}
}

Usage operator| (Usage a, Usage b)
{
	return (Usage)((deUint32)a | (deUint32)b);
}

Usage operator& (Usage a, Usage b)
{
	return (Usage)((deUint32)a & (deUint32)b);
}

string usageToName (Usage usage)
{
	const struct
	{
		Usage				usage;
		const char* const	name;
	} usageNames[] =
	{
		{ USAGE_HOST_READ,					"host_read" },
		{ USAGE_HOST_WRITE,					"host_write" },

		{ USAGE_TRANSFER_SRC,				"transfer_src" },
		{ USAGE_TRANSFER_DST,				"transfer_dst" },

		{ USAGE_INDEX_BUFFER,				"index_buffer" },
		{ USAGE_VERTEX_BUFFER,				"vertex_buffer" },
		{ USAGE_UNIFORM_BUFFER,				"uniform_buffer" },
		{ USAGE_STORAGE_BUFFER,				"storage_buffer" },
		{ USAGE_UNIFORM_TEXEL_BUFFER,		"uniform_texel_buffer" },
		{ USAGE_STORAGE_TEXEL_BUFFER,		"storage_texel_buffer" },
		{ USAGE_INDIRECT_BUFFER,			"indirect_buffer" },
		{ USAGE_SAMPLED_IMAGE,				"image_sampled" },
		{ USAGE_STORAGE_IMAGE,				"storage_image" },
		{ USAGE_COLOR_ATTACHMENT,			"color_attachment" },
		{ USAGE_INPUT_ATTACHMENT,			"input_attachment" },
		{ USAGE_DEPTH_STENCIL_ATTACHMENT,	"depth_stencil_attachment" },
	};

	std::ostringstream	stream;
	bool				first = true;

	for (size_t usageNdx = 0; usageNdx < DE_LENGTH_OF_ARRAY(usageNames); usageNdx++)
	{
		if (usage & usageNames[usageNdx].usage)
		{
			if (!first)
				stream << "_";
			else
				first = false;

			stream << usageNames[usageNdx].name;
		}
	}

	return stream.str();
}

vk::VkBufferUsageFlags usageToBufferUsageFlags (Usage usage)
{
	vk::VkBufferUsageFlags flags = 0;

	if (usage & USAGE_TRANSFER_SRC)
		flags |= vk::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	if (usage & USAGE_TRANSFER_DST)
		flags |= vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (usage & USAGE_INDEX_BUFFER)
		flags |= vk::VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	if (usage & USAGE_VERTEX_BUFFER)
		flags |= vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	if (usage & USAGE_INDIRECT_BUFFER)
		flags |= vk::VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

	if (usage & USAGE_UNIFORM_BUFFER)
		flags |= vk::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	if (usage & USAGE_STORAGE_BUFFER)
		flags |= vk::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	if (usage & USAGE_UNIFORM_TEXEL_BUFFER)
		flags |= vk::VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;

	if (usage & USAGE_STORAGE_TEXEL_BUFFER)
		flags |= vk::VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

	return flags;
}

vk::VkImageUsageFlags usageToImageUsageFlags (Usage usage)
{
	vk::VkImageUsageFlags flags = 0;

	if (usage & USAGE_TRANSFER_SRC)
		flags |= vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (usage & USAGE_TRANSFER_DST)
		flags |= vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (usage & USAGE_SAMPLED_IMAGE)
		flags |= vk::VK_IMAGE_USAGE_SAMPLED_BIT;

	if (usage & USAGE_STORAGE_IMAGE)
		flags |= vk::VK_IMAGE_USAGE_STORAGE_BIT;

	if (usage & USAGE_COLOR_ATTACHMENT)
		flags |= vk::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (usage & USAGE_INPUT_ATTACHMENT)
		flags |= vk::VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

	if (usage & USAGE_DEPTH_STENCIL_ATTACHMENT)
		flags |= vk::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	return flags;
}

vk::VkPipelineStageFlags usageToStageFlags (Usage usage)
{
	vk::VkPipelineStageFlags flags = 0;

	if (usage & (USAGE_HOST_READ|USAGE_HOST_WRITE))
		flags |= vk::VK_PIPELINE_STAGE_HOST_BIT;

	if (usage & (USAGE_TRANSFER_SRC|USAGE_TRANSFER_DST))
		flags |= vk::VK_PIPELINE_STAGE_TRANSFER_BIT;

	if (usage & (USAGE_VERTEX_BUFFER|USAGE_INDEX_BUFFER))
		flags |= vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

	if (usage & USAGE_INDIRECT_BUFFER)
		flags |= vk::VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

	if (usage &
			(USAGE_UNIFORM_BUFFER
			| USAGE_STORAGE_BUFFER
			| USAGE_UNIFORM_TEXEL_BUFFER
			| USAGE_STORAGE_TEXEL_BUFFER
			| USAGE_SAMPLED_IMAGE
			| USAGE_STORAGE_IMAGE))
	{
		flags |= (vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
				| vk::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
				| vk::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
				| vk::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT
				| vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				| vk::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}

	if (usage & USAGE_INPUT_ATTACHMENT)
		flags |= vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	if (usage & USAGE_COLOR_ATTACHMENT)
		flags |= vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	if (usage & USAGE_DEPTH_STENCIL_ATTACHMENT)
	{
		flags |= vk::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
				| vk::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}

	return flags;
}

vk::VkAccessFlags usageToAccessFlags (Usage usage)
{
	vk::VkAccessFlags flags = 0;

	if (usage & USAGE_HOST_READ)
		flags |= vk::VK_ACCESS_HOST_READ_BIT;

	if (usage & USAGE_HOST_WRITE)
		flags |= vk::VK_ACCESS_HOST_WRITE_BIT;

	if (usage & USAGE_TRANSFER_SRC)
		flags |= vk::VK_ACCESS_TRANSFER_READ_BIT;

	if (usage & USAGE_TRANSFER_DST)
		flags |= vk::VK_ACCESS_TRANSFER_WRITE_BIT;

	if (usage & USAGE_INDEX_BUFFER)
		flags |= vk::VK_ACCESS_INDEX_READ_BIT;

	if (usage & USAGE_VERTEX_BUFFER)
		flags |= vk::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

	if (usage & (USAGE_UNIFORM_BUFFER | USAGE_UNIFORM_TEXEL_BUFFER))
		flags |= vk::VK_ACCESS_UNIFORM_READ_BIT;

	if (usage & USAGE_SAMPLED_IMAGE)
		flags |= vk::VK_ACCESS_SHADER_READ_BIT;

	if (usage & (USAGE_STORAGE_BUFFER
				| USAGE_STORAGE_TEXEL_BUFFER
				| USAGE_STORAGE_IMAGE))
		flags |= vk::VK_ACCESS_SHADER_READ_BIT | vk::VK_ACCESS_SHADER_WRITE_BIT;

	if (usage & USAGE_INDIRECT_BUFFER)
		flags |= vk::VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

	if (usage & USAGE_COLOR_ATTACHMENT)
		flags |= vk::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	if (usage & USAGE_INPUT_ATTACHMENT)
		flags |= vk::VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

	if (usage & USAGE_DEPTH_STENCIL_ATTACHMENT)
		flags |= vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
			| vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	return flags;
}

struct TestConfig
{
	Usage				usage;
	vk::VkDeviceSize	size;
	vk::VkSharingMode	sharing;
};

vk::Move<vk::VkCommandBuffer> createBeginCommandBuffer (const vk::DeviceInterface&	vkd,
														vk::VkDevice				device,
														vk::VkCommandPool			pool,
														vk::VkCommandBufferLevel	level)
{
	const vk::VkCommandBufferInheritanceInfo	inheritInfo	=
	{
		vk::VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		0,
		0,
		0,
		VK_FALSE,
		0u,
		0u
	};
	const vk::VkCommandBufferBeginInfo			beginInfo =
	{
		vk::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		DE_NULL,
		0u,
		(level == vk::VK_COMMAND_BUFFER_LEVEL_SECONDARY ? &inheritInfo : (const vk::VkCommandBufferInheritanceInfo*)DE_NULL),
	};

	vk::Move<vk::VkCommandBuffer> commandBuffer (allocateCommandBuffer(vkd, device, pool, level));

	vkd.beginCommandBuffer(*commandBuffer, &beginInfo);

	return commandBuffer;
}

vk::Move<vk::VkBuffer> createBuffer (const vk::DeviceInterface&	vkd,
									 vk::VkDevice				device,
									 vk::VkDeviceSize			size,
									 vk::VkBufferUsageFlags		usage,
									 vk::VkSharingMode			sharingMode,
									 const vector<deUint32>&	queueFamilies)
{
	const vk::VkBufferCreateInfo	createInfo =
	{
		vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		DE_NULL,

		0,	// flags
		size,
		usage,
		sharingMode,
		(deUint32)queueFamilies.size(),
		&queueFamilies[0]
	};

	return vk::createBuffer(vkd, device, &createInfo);
}

vk::Move<vk::VkDeviceMemory> allocMemory (const vk::DeviceInterface&	vkd,
										  vk::VkDevice					device,
										  vk::VkDeviceSize				size,
										  deUint32						memoryTypeIndex)
{
	const vk::VkMemoryAllocateInfo alloc =
	{
		vk::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,	// sType
		DE_NULL,									// pNext

		size,
		memoryTypeIndex
	};

	return vk::allocateMemory(vkd, device, &alloc);
}

vk::Move<vk::VkDeviceMemory> bindBufferMemory (const vk::InstanceInterface&	vki,
											   const vk::DeviceInterface&	vkd,
											   vk::VkPhysicalDevice			physicalDevice,
											   vk::VkDevice					device,
											   vk::VkBuffer					buffer,
											   vk::VkMemoryPropertyFlags	properties)
{
	const vk::VkMemoryRequirements				memoryRequirements	= vk::getBufferMemoryRequirements(vkd, device, buffer);
	const vk::VkPhysicalDeviceMemoryProperties	memoryProperties	= vk::getPhysicalDeviceMemoryProperties(vki, physicalDevice);
	deUint32									memoryTypeIndex;

	for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; memoryTypeIndex++)
	{
		if ((memoryRequirements.memoryTypeBits & (0x1u << memoryTypeIndex))
			&& (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & properties) == properties)
		{
			try
			{
				const vk::VkMemoryAllocateInfo	allocationInfo	=
				{
					vk::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					DE_NULL,
					memoryRequirements.size,
					memoryTypeIndex
				};
				vk::Move<vk::VkDeviceMemory>	memory			(vk::allocateMemory(vkd, device, &allocationInfo));

				VK_CHECK(vkd.bindBufferMemory(device, buffer, *memory, 0));

				return memory;
			}
			catch (const vk::Error& error)
			{
				if (error.getError() == vk::VK_ERROR_OUT_OF_DEVICE_MEMORY
					|| error.getError() == vk::VK_ERROR_OUT_OF_HOST_MEMORY)
				{
					// Try next memory type/heap if out of memory
				}
				else
				{
					// Throw all other errors forward
					throw;
				}
			}
		}
	}

	TCU_FAIL("Failed to allocate memory for buffer");
}

vk::Move<vk::VkDeviceMemory> bindImageMemory (const vk::InstanceInterface&	vki,
											   const vk::DeviceInterface&	vkd,
											   vk::VkPhysicalDevice			physicalDevice,
											   vk::VkDevice					device,
											   vk::VkImage					image,
											   vk::VkMemoryPropertyFlags	properties)
{
	const vk::VkMemoryRequirements				memoryRequirements	= vk::getImageMemoryRequirements(vkd, device, image);
	const vk::VkPhysicalDeviceMemoryProperties	memoryProperties	= vk::getPhysicalDeviceMemoryProperties(vki, physicalDevice);
	deUint32									memoryTypeIndex;

	for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; memoryTypeIndex++)
	{
		if ((memoryRequirements.memoryTypeBits & (0x1u << memoryTypeIndex))
			&& (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & properties) == properties)
		{
			try
			{
				const vk::VkMemoryAllocateInfo	allocationInfo	=
				{
					vk::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					DE_NULL,
					memoryRequirements.size,
					memoryTypeIndex
				};
				vk::Move<vk::VkDeviceMemory>	memory			(vk::allocateMemory(vkd, device, &allocationInfo));

				VK_CHECK(vkd.bindImageMemory(device, image, *memory, 0));

				return memory;
			}
			catch (const vk::Error& error)
			{
				if (error.getError() == vk::VK_ERROR_OUT_OF_DEVICE_MEMORY
					|| error.getError() == vk::VK_ERROR_OUT_OF_HOST_MEMORY)
				{
					// Try next memory type/heap if out of memory
				}
				else
				{
					// Throw all other errors forward
					throw;
				}
			}
		}
	}

	TCU_FAIL("Failed to allocate memory for image");
}

void queueRun (const vk::DeviceInterface&	vkd,
			   vk::VkQueue					queue,
			   vk::VkCommandBuffer			commandBuffer)
{
	const vk::VkSubmitInfo	submitInfo	=
	{
		vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,
		DE_NULL,

		0,
		DE_NULL,
		(const vk::VkPipelineStageFlags*)DE_NULL,

		1,
		&commandBuffer,

		0,
		DE_NULL
	};

	VK_CHECK(vkd.queueSubmit(queue, 1, &submitInfo, 0));
	VK_CHECK(vkd.queueWaitIdle(queue));
}

void* mapMemory (const vk::DeviceInterface&	vkd,
				 vk::VkDevice				device,
				 vk::VkDeviceMemory			memory,
				 vk::VkDeviceSize			size)
{
	void* ptr;

	VK_CHECK(vkd.mapMemory(device, memory, 0, size, 0, &ptr));

	return ptr;
}

class ReferenceMemory
{
public:
			ReferenceMemory	(size_t size);

	void	set				(size_t pos, deUint8 val);
	deUint8	get				(size_t pos) const;
	bool	isDefined		(size_t pos) const;

	void	setDefined		(size_t offset, size_t size, const void* data);
	void	setUndefined	(size_t offset, size_t size);
	void	setData			(size_t offset, size_t size, const void* data);

	size_t	getSize			(void) const { return m_data.size(); }

private:
	vector<deUint8>		m_data;
	vector<deUint64>	m_defined;
};

ReferenceMemory::ReferenceMemory (size_t size)
	: m_data	(size, 0)
	, m_defined	(size / 64 + (size % 64 == 0 ? 0 : 1), 0ull)
{
}

void ReferenceMemory::set (size_t pos, deUint8 val)
{
	DE_ASSERT(pos < m_data.size());

	m_data[pos] = val;
	m_defined[pos / 64] |= 0x1ull << (pos % 64);
}

void ReferenceMemory::setData (size_t offset, size_t size, const void* data_)
{
	const deUint8* data = (const deUint8*)data_;

	DE_ASSERT(offset < m_data.size());
	DE_ASSERT(offset + size <= m_data.size());

	// \todo [2016-03-09 mika] Optimize
	for (size_t pos = 0; pos < size; pos++)
	{
		m_data[offset + pos] = data[pos];
		m_defined[(offset + pos) / 64] |= 0x1ull << ((offset + pos) % 64);
	}
}

void ReferenceMemory::setUndefined	(size_t offset, size_t size)
{
	// \todo [2016-03-09 mika] Optimize
	for (size_t pos = 0; pos < size; pos++)
		m_defined[(offset + pos) / 64] |= 0x1ull << ((offset + pos) % 64);
}

deUint8 ReferenceMemory::get (size_t pos) const
{
	DE_ASSERT(pos < m_data.size());
	DE_ASSERT(isDefined(pos));
	return m_data[pos];
}

bool ReferenceMemory::isDefined (size_t pos) const
{
	DE_ASSERT(pos < m_data.size());

	return (m_defined[pos / 64] & (0x1ull << (pos % 64))) != 0;
}

class Memory
{
public:
							Memory				(const vk::InstanceInterface&	vki,
												 const vk::DeviceInterface&		vkd,
												 vk::VkPhysicalDevice			physicalDevice,
												 vk::VkDevice					device,
												 vk::VkDeviceSize				size,
												 deUint32						memoryTypeIndex,
												 vk::VkDeviceSize				maxBufferSize,
												 deInt32						maxImageWidth,
												 deInt32						maxImageHeight);

	vk::VkDeviceSize		getSize				(void) const { return m_size; }
	vk::VkDeviceSize		getMaxBufferSize	(void) const { return m_maxBufferSize; }
	bool					getSupportBuffers	(void) const { return m_maxBufferSize > 0; }

	deInt32					getMaxImageWidth	(void) const { return m_maxImageWidth; }
	deInt32					getMaxImageHeight	(void) const { return m_maxImageHeight; }
	bool					getSupportImages	(void) const { return m_maxImageWidth > 0; }

	const vk::VkMemoryType&	getMemoryType		(void) const { return m_memoryType; }
	deUint32				getMemoryTypeIndex	(void) const { return m_memoryTypeIndex; }
	vk::VkDeviceMemory		getMemory			(void) const { return *m_memory; }

private:
	const vk::VkDeviceSize					m_size;
	const deUint32							m_memoryTypeIndex;
	const vk::VkMemoryType					m_memoryType;
	const vk::Unique<vk::VkDeviceMemory>	m_memory;
	const vk::VkDeviceSize					m_maxBufferSize;
	const deInt32							m_maxImageWidth;
	const deInt32							m_maxImageHeight;
};

vk::VkMemoryType getMemoryTypeInfo (const vk::InstanceInterface&	vki,
									vk::VkPhysicalDevice			device,
									deUint32						memoryTypeIndex)
{
	const vk::VkPhysicalDeviceMemoryProperties memoryProperties = vk::getPhysicalDeviceMemoryProperties(vki, device);

	DE_ASSERT(memoryTypeIndex < memoryProperties.memoryTypeCount);

	return memoryProperties.memoryTypes[memoryTypeIndex];
}

vk::VkDeviceSize findMaxBufferSize (const vk::DeviceInterface&		vkd,
									vk::VkDevice					device,

									vk::VkBufferUsageFlags			usage,
									vk::VkSharingMode				sharingMode,
									const vector<deUint32>&			queueFamilies,

									vk::VkDeviceSize				memorySize,
									deUint32						memoryTypeIndex)
{
	vk::VkDeviceSize	lastSuccess	= 0;
	vk::VkDeviceSize	currentSize	= memorySize / 2;

	{
		const vk::Unique<vk::VkBuffer>  buffer			(createBuffer(vkd, device, memorySize, usage, sharingMode, queueFamilies));
		const vk::VkMemoryRequirements  requirements	(vk::getBufferMemoryRequirements(vkd, device, *buffer));

		if (requirements.size == memorySize && requirements.memoryTypeBits & (0x1u << memoryTypeIndex))
			return memorySize;
	}

	for (vk::VkDeviceSize stepSize = memorySize / 4; currentSize > 0; stepSize /= 2)
	{
		const vk::Unique<vk::VkBuffer>	buffer			(createBuffer(vkd, device, currentSize, usage, sharingMode, queueFamilies));
		const vk::VkMemoryRequirements	requirements	(vk::getBufferMemoryRequirements(vkd, device, *buffer));

		if (requirements.size <= memorySize && requirements.memoryTypeBits & (0x1u << memoryTypeIndex))
		{
			lastSuccess = currentSize;
			currentSize += stepSize;
		}
		else
			currentSize -= stepSize;

		if (stepSize == 0)
			break;
	}

	return lastSuccess;
}

// Round size down maximum W * H * 4, where W and H < 4096
vk::VkDeviceSize roundBufferSizeToWxHx4 (vk::VkDeviceSize size)
{
	const vk::VkDeviceSize	maxTextureSize	= 4096;
	vk::VkDeviceSize		maxTexelCount	= size / 4;
	vk::VkDeviceSize		bestW			= de::max(maxTexelCount, maxTextureSize);
	vk::VkDeviceSize		bestH			= maxTexelCount / bestW;

	// \todo [2016-03-09 mika] Could probably be faster?
	for (vk::VkDeviceSize w = 1; w * w < maxTexelCount && w < maxTextureSize && bestW * bestH * 4 < size; w++)
	{
		const vk::VkDeviceSize h = maxTexelCount / w;

		if (bestW * bestH < w * h)
		{
			bestW = w;
			bestH = h;
		}
	}

	return bestW * bestH * 4;
}

// Find RGBA8 image size that has exactly "size" of number of bytes.
// "size" must be W * H * 4 where W and H < 4096
IVec2 findImageSizeWxHx4 (vk::VkDeviceSize size)
{
	const vk::VkDeviceSize	maxTextureSize	= 4096;
	vk::VkDeviceSize		texelCount		= size / 4;

	DE_ASSERT((size % 4) == 0);

	// \todo [2016-03-09 mika] Could probably be faster?
	for (vk::VkDeviceSize w = 1; w < maxTextureSize && w < texelCount; w++)
	{
		const vk::VkDeviceSize	h	= texelCount / w;

		if ((texelCount  % w) == 0 && h < maxTextureSize)
			return IVec2((int)w, (int)h);
	}

	DE_FATAL("Invalid size");
	return IVec2(-1, -1);
}

IVec2 findMaxRGBA8ImageSize (const vk::DeviceInterface&	vkd,
							 vk::VkDevice				device,

							 vk::VkImageUsageFlags		usage,
							 vk::VkSharingMode			sharingMode,
							 const vector<deUint32>&	queueFamilies,

							 vk::VkDeviceSize			memorySize,
							 deUint32					memoryTypeIndex)
{
	IVec2		lastSuccess		(0);
	IVec2		currentSize;

	{
		const deUint32	texelCount	= (deUint32)(memorySize / 4);
		const deUint32	width		= (deUint32)deFloatSqrt((float)texelCount);
		const deUint32	height		= texelCount / width;

		currentSize[0] = deMaxu32(width, height);
		currentSize[1] = deMinu32(width, height);
	}

	for (deInt32 stepSize = currentSize[0] / 2; currentSize[0] > 0; stepSize /= 2)
	{
		const vk::VkImageCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			DE_NULL,

			0u,
			vk::VK_IMAGE_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{
				(deUint32)currentSize[0],
				(deUint32)currentSize[1],
				1u,
			},
			1u, 1u,
			vk::VK_SAMPLE_COUNT_1_BIT,
			vk::VK_IMAGE_TILING_OPTIMAL,
			usage,
			sharingMode,
			(deUint32)queueFamilies.size(),
			&queueFamilies[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};
		const vk::Unique<vk::VkImage>	image			(vk::createImage(vkd, device, &createInfo));
		const vk::VkMemoryRequirements	requirements	(vk::getImageMemoryRequirements(vkd, device, *image));

		if (requirements.size <= memorySize && requirements.memoryTypeBits & (0x1u << memoryTypeIndex))
		{
			lastSuccess = currentSize;
			currentSize[0] += stepSize;
			currentSize[1] += stepSize;
		}
		else
		{
			currentSize[0] -= stepSize;
			currentSize[1] -= stepSize;
		}

		if (stepSize == 0)
			break;
	}

	return lastSuccess;
}

Memory::Memory (const vk::InstanceInterface&	vki,
				const vk::DeviceInterface&		vkd,
				vk::VkPhysicalDevice			physicalDevice,
				vk::VkDevice					device,
				vk::VkDeviceSize				size,
				deUint32						memoryTypeIndex,
				vk::VkDeviceSize				maxBufferSize,
				deInt32							maxImageWidth,
				deInt32							maxImageHeight)
	: m_size			(size)
	, m_memoryTypeIndex	(memoryTypeIndex)
	, m_memoryType		(getMemoryTypeInfo(vki, physicalDevice, memoryTypeIndex))
	, m_memory			(allocMemory(vkd, device, size, memoryTypeIndex))
	, m_maxBufferSize	(maxBufferSize)
	, m_maxImageWidth	(maxImageWidth)
	, m_maxImageHeight	(maxImageHeight)
{
}

class Context
{
public:
													Context					(const vk::InstanceInterface&						vki,
																			 const vk::DeviceInterface&							vkd,
																			 vk::VkPhysicalDevice								physicalDevice,
																			 vk::VkDevice										device,
																			 vk::VkQueue										queue,
																			 deUint32											queueFamilyIndex,
																			 const vector<pair<deUint32, vk::VkQueue> >&		queues,
																			 const vk::ProgramCollection<vk::ProgramBinary>&	binaryCollection)
		: m_vki					(vki)
		, m_vkd					(vkd)
		, m_physicalDevice		(physicalDevice)
		, m_device				(device)
		, m_queue				(queue)
		, m_queueFamilyIndex	(queueFamilyIndex)
		, m_queues				(queues)
		, m_commandPool			(createCommandPool(vkd, device, vk::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex))
		, m_binaryCollection	(binaryCollection)
	{
		for (size_t queueNdx = 0; queueNdx < m_queues.size(); queueNdx++)
			m_queueFamilies.push_back(m_queues[queueNdx].first);
	}

	const vk::InstanceInterface&					getInstanceInterface	(void) const { return m_vki; }
	vk::VkPhysicalDevice							getPhysicalDevice		(void) const { return m_physicalDevice; }
	vk::VkDevice									getDevice				(void) const { return m_device; }
	const vk::DeviceInterface&						getDeviceInterface		(void) const { return m_vkd; }
	vk::VkQueue										getQueue				(void) const { return m_queue; }
	deUint32										getQueueFamily			(void) const { return m_queueFamilyIndex; }
	const vector<pair<deUint32, vk::VkQueue> >&		getQueues				(void) const { return m_queues; }
	const vector<deUint32>							getQueueFamilies		(void) const { return m_queueFamilies; }
	vk::VkCommandPool								getCommandPool			(void) const { return *m_commandPool; }
	const vk::ProgramCollection<vk::ProgramBinary>&	getBinaryCollection		(void) const { return m_binaryCollection; }

private:
	const vk::InstanceInterface&					m_vki;
	const vk::DeviceInterface&						m_vkd;
	const vk::VkPhysicalDevice						m_physicalDevice;
	const vk::VkDevice								m_device;
	const vk::VkQueue								m_queue;
	const deUint32									m_queueFamilyIndex;
	const vector<pair<deUint32, vk::VkQueue> >		m_queues;
	const vk::Unique<vk::VkCommandPool>				m_commandPool;
	const vk::ProgramCollection<vk::ProgramBinary>&	m_binaryCollection;
	vector<deUint32>								m_queueFamilies;
};

class PrepareContext
{
public:
													PrepareContext			(const Context&	context,
																			 const Memory&	memory)
		: m_context	(context)
		, m_memory	(memory)
	{
	}

	const Memory&									getMemory				(void) const { return m_memory; }
	const Context&									getContext				(void) const { return m_context; }
	const vk::ProgramCollection<vk::ProgramBinary>&	getBinaryCollection		(void) const { return m_context.getBinaryCollection(); }

	void				setBuffer		(vk::Move<vk::VkBuffer>	buffer,
										 vk::VkDeviceSize		size)
	{
		DE_ASSERT(!m_currentImage);
		DE_ASSERT(!m_currentBuffer);

		m_currentBuffer		= buffer;
		m_currentBufferSize	= size;
	}

	vk::VkBuffer		getBuffer		(void) const { return *m_currentBuffer; }
	vk::VkDeviceSize	getBufferSize	(void) const
	{
		DE_ASSERT(m_currentBuffer);
		return m_currentBufferSize;
	}

	void				releaseBuffer	(void) { m_currentBuffer.disown(); }

	void				setImage		(vk::Move<vk::VkImage>	image,
										 vk::VkImageLayout		layout,
										 vk::VkDeviceSize		memorySize,
										 deInt32				width,
										 deInt32				height)
	{
		DE_ASSERT(!m_currentImage);
		DE_ASSERT(!m_currentBuffer);

		m_currentImage				= image;
		m_currentImageMemorySize	= memorySize;
		m_currentImageLayout		= layout;
		m_currentImageWidth			= width;
		m_currentImageHeight		= height;
	}

	void				setImageLayout	(vk::VkImageLayout layout)
	{
		DE_ASSERT(m_currentImage);
		m_currentImageLayout = layout;
	}

	vk::VkImage			getImage		(void) const { return *m_currentImage; }
	deInt32				getImageWidth	(void) const
	{
		DE_ASSERT(m_currentImage);
		return m_currentImageWidth;
	}
	deInt32				getImageHeight	(void) const
	{
		DE_ASSERT(m_currentImage);
		return m_currentImageHeight;
	}
	vk::VkDeviceSize	getImageMemorySize	(void) const
	{
		DE_ASSERT(m_currentImage);
		return m_currentImageMemorySize;
	}

	void				releaseImage	(void) { m_currentImage.disown(); }

	vk::VkImageLayout	getImageLayout	(void) const
	{
		DE_ASSERT(m_currentImage);
		return m_currentImageLayout;
	}

private:
	const Context&			m_context;
	const Memory&			m_memory;

	vk::Move<vk::VkBuffer>	m_currentBuffer;
	vk::VkDeviceSize		m_currentBufferSize;

	vk::Move<vk::VkImage>	m_currentImage;
	vk::VkDeviceSize		m_currentImageMemorySize;
	vk::VkImageLayout		m_currentImageLayout;
	deInt32					m_currentImageWidth;
	deInt32					m_currentImageHeight;
};

class ExecuteContext
{
public:
					ExecuteContext	(const Context&	context)
		: m_context	(context)
	{
	}

	const Context&	getContext		(void) const { return m_context; }
	void			setMapping		(void* ptr) { m_mapping = ptr; }
	void*			getMapping		(void) const { return m_mapping; }

private:
	const Context&	m_context;
	void*			m_mapping;
};

class VerifyContext
{
public:
							VerifyContext		(TestLog&				log,
												 tcu::ResultCollector&	resultCollector,
												 const Context&			context,
												 vk::VkDeviceSize		size)
		: m_log				(log)
		, m_resultCollector	(resultCollector)
		, m_context			(context)
		, m_reference		((size_t)size)
	{
	}

	const Context&			getContext			(void) const { return m_context; }
	TestLog&				getLog				(void) const { return m_log; }
	tcu::ResultCollector&	getResultCollector	(void) const { return m_resultCollector; }

	ReferenceMemory&		getReference		(void) { return m_reference; }
	TextureLevel&			getReferenceImage	(void) { return m_referenceImage;}

private:
	TestLog&				m_log;
	tcu::ResultCollector&	m_resultCollector;
	const Context&			m_context;
	ReferenceMemory			m_reference;
	TextureLevel			m_referenceImage;
};

class Command
{
public:
	// Constructor should allocate all non-vulkan resources.
	virtual				~Command	(void) {}

	// Get name of the command
	virtual const char*	getName		(void) const = 0;

	// Log prepare operations
	virtual void		logPrepare	(TestLog&, size_t) const {}
	// Log executed operations
	virtual void		logExecute	(TestLog&, size_t) const {}

	// Prepare should allocate all vulkan resources and resources that require
	// that buffer or memory has been already allocated. This should build all
	// command buffers etc.
	virtual void		prepare		(PrepareContext&) {}

	// Execute command. Write or read mapped memory, submit commands to queue
	// etc.
	virtual void		execute		(ExecuteContext&) {}

	// Verify that results are correct.
	virtual void		verify		(VerifyContext&, size_t) {}

protected:
	// Allow only inheritance
						Command		(void) {}

private:
	// Disallow copying
						Command		(const Command&);
	Command&			operator&	(const Command&);
};

class Map : public Command
{
public:
						Map			(void) {}
						~Map		(void) {}
	const char*			getName		(void) const { return "Map"; }


	void				logExecute	(TestLog& log, size_t commandIndex) const
	{
		log << TestLog::Message << commandIndex << ":" << getName() << " Map memory" << TestLog::EndMessage;
	}

	void				prepare		(PrepareContext& context)
	{
		m_memory	= context.getMemory().getMemory();
		m_size		= context.getMemory().getSize();
	}

	void				execute		(ExecuteContext& context)
	{
		const vk::DeviceInterface&	vkd		= context.getContext().getDeviceInterface();
		const vk::VkDevice			device	= context.getContext().getDevice();

		context.setMapping(mapMemory(vkd, device, m_memory, m_size));
	}

private:
	vk::VkDeviceMemory	m_memory;
	vk::VkDeviceSize	m_size;
};

class UnMap : public Command
{
public:
						UnMap		(void) {}
						~UnMap		(void) {}
	const char*			getName		(void) const { return "UnMap"; }

	void				logExecute	(TestLog& log, size_t commandIndex) const
	{
		log << TestLog::Message << commandIndex << ": Unmap memory" << TestLog::EndMessage;
	}

	void				prepare		(PrepareContext& context)
	{
		m_memory	= context.getMemory().getMemory();
	}

	void				execute		(ExecuteContext& context)
	{
		const vk::DeviceInterface&	vkd		= context.getContext().getDeviceInterface();
		const vk::VkDevice			device	= context.getContext().getDevice();

		vkd.unmapMemory(device, m_memory);
		context.setMapping(DE_NULL);
	}

private:
	vk::VkDeviceMemory	m_memory;
};

class Invalidate : public Command
{
public:
						Invalidate	(void) {}
						~Invalidate	(void) {}
	const char*			getName		(void) const { return "Invalidate"; }

	void				logExecute	(TestLog& log, size_t commandIndex) const
	{
		log << TestLog::Message << commandIndex << ": Invalidate mapped memory" << TestLog::EndMessage;
	}

	void				prepare		(PrepareContext& context)
	{
		m_memory	= context.getMemory().getMemory();
		m_size		= context.getMemory().getSize();
	}

	void				execute		(ExecuteContext& context)
	{
		const vk::DeviceInterface&	vkd		= context.getContext().getDeviceInterface();
		const vk::VkDevice			device	= context.getContext().getDevice();

		vk::invalidateMappedMemoryRange(vkd, device, m_memory, 0, m_size);
	}

private:
	vk::VkDeviceMemory	m_memory;
	vk::VkDeviceSize	m_size;
};

class Flush : public Command
{
public:
						Flush		(void) {}
						~Flush		(void) {}
	const char*			getName		(void) const { return "Flush"; }

	void				logExecute	(TestLog& log, size_t commandIndex) const
	{
		log << TestLog::Message << commandIndex << ": Flush mapped memory" << TestLog::EndMessage;
	}

	void				prepare		(PrepareContext& context)
	{
		m_memory	= context.getMemory().getMemory();
		m_size		= context.getMemory().getSize();
	}

	void				execute		(ExecuteContext& context)
	{
		const vk::DeviceInterface&	vkd		= context.getContext().getDeviceInterface();
		const vk::VkDevice			device	= context.getContext().getDevice();

		vk::flushMappedMemoryRange(vkd, device, m_memory, 0, m_size);
	}

private:
	vk::VkDeviceMemory	m_memory;
	vk::VkDeviceSize	m_size;
};

// Host memory reads and writes
class HostMemoryAccess : public Command
{
public:
					HostMemoryAccess	(bool read, bool write, deUint32 seed);
					~HostMemoryAccess	(void) {}
	const char*		getName				(void) const { return "HostMemoryAccess"; }

	void			logExecute			(TestLog& log, size_t commandIndex) const;
	void			prepare				(PrepareContext& context);
	void			execute				(ExecuteContext& context);
	void			verify				(VerifyContext& context, size_t commandIndex);

private:
	const bool		m_read;
	const bool		m_write;
	const deUint32	m_seed;

	size_t			m_size;
	vector<deUint8>	m_readData;
};

HostMemoryAccess::HostMemoryAccess (bool read, bool write, deUint32 seed)
	: m_read	(read)
	, m_write	(write)
	, m_seed	(seed)
{
}

void HostMemoryAccess::logExecute (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ": Host memory access:" << (m_read ? " read" : "") << (m_write ? " write" : "")  << ", seed: " << m_seed << TestLog::EndMessage;
}

void HostMemoryAccess::prepare (PrepareContext& context)
{
	m_size = (size_t)context.getMemory().getSize();

	if (m_read)
		m_readData.resize(m_size, 0);
}

void HostMemoryAccess::execute (ExecuteContext& context)
{
	de::Random		rng	(m_seed);
	deUint8* const	ptr	= (deUint8*)context.getMapping();

	if (m_read && m_write)
	{
		for (size_t pos = 0; pos < m_size; pos++)
		{
			const deUint8	mask	= rng.getUint8();
			const deUint8	value	= ptr[pos];

			m_readData[pos] = value;
			ptr[pos] = value ^ mask;
		}
	}
	else if (m_read)
	{
		for (size_t pos = 0; pos < m_size; pos++)
		{
			const deUint8	value	= ptr[pos];

			m_readData[pos] = value;
		}
	}
	else if (m_write)
	{
		for (size_t pos = 0; pos < m_size; pos++)
		{
			const deUint8	value	= rng.getUint8();

			ptr[pos] = value;
		}
	}
	else
		DE_FATAL("Host memory access without read or write.");
}

void HostMemoryAccess::verify (VerifyContext& context, size_t commandIndex)
{
	tcu::ResultCollector&	resultCollector	= context.getResultCollector();
	ReferenceMemory&		reference		= context.getReference();
	de::Random				rng				(m_seed);

	if (m_read && m_write)
	{
		for (size_t pos = 0; pos < m_size; pos++)
		{
			const deUint8	mask	= rng.getUint8();
			const deUint8	value	= m_readData[pos];

			if (reference.isDefined(pos))
			{
				if (value != reference.get(pos))
				{
					resultCollector.fail(
							de::toString(commandIndex) + ":" + getName()
							+ " Result differs from reference, Expected: "
							+ de::toString(tcu::toHex<8>(reference.get(pos)))
							+ ", Got: "
							+ de::toString(tcu::toHex<8>(value))
							+ ", At offset: "
							+ de::toString(pos));
					break;
				}

				reference.set(pos, reference.get(pos) ^ mask);
			}
		}
	}
	else if (m_read)
	{
		for (size_t pos = 0; pos < m_size; pos++)
		{
			const deUint8	value	= m_readData[pos];

			if (reference.isDefined(pos))
			{
				if (value != reference.get(pos))
				{
					resultCollector.fail(
							de::toString(commandIndex) + ":" + getName()
							+ " Result differs from reference, Expected: "
							+ de::toString(tcu::toHex<8>(reference.get(pos)))
							+ ", Got: "
							+ de::toString(tcu::toHex<8>(value))
							+ ", At offset: "
							+ de::toString(pos));
					break;
				}
			}
		}
	}
	else if (m_write)
	{
		for (size_t pos = 0; pos < m_size; pos++)
		{
			const deUint8	value	= rng.getUint8();

			reference.set(pos, value);
		}
	}
	else
		DE_FATAL("Host memory access without read or write.");
}

class CreateBuffer : public Command
{
public:
									CreateBuffer	(vk::VkBufferUsageFlags	usage,
													 vk::VkSharingMode		sharing);
									~CreateBuffer	(void) {}
	const char*						getName			(void) const { return "CreateBuffer"; }

	void							logPrepare		(TestLog& log, size_t commandIndex) const;
	void							prepare			(PrepareContext& context);

private:
	const vk::VkBufferUsageFlags	m_usage;
	const vk::VkSharingMode			m_sharing;
};

CreateBuffer::CreateBuffer (vk::VkBufferUsageFlags	usage,
							vk::VkSharingMode		sharing)
	: m_usage	(usage)
	, m_sharing	(sharing)
{
}

void CreateBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create buffer, Sharing mode: " << m_sharing << ", Usage: " << vk::getBufferUsageFlagsStr(m_usage) << TestLog::EndMessage;
}

void CreateBuffer::prepare (PrepareContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkDevice			device			= context.getContext().getDevice();
	const vk::VkDeviceSize		bufferSize		= context.getMemory().getMaxBufferSize();
	const vector<deUint32>&		queueFamilies	= context.getContext().getQueueFamilies();

	context.setBuffer(createBuffer(vkd, device, bufferSize, m_usage, m_sharing, queueFamilies), bufferSize);
}

class DestroyBuffer : public Command
{
public:
							DestroyBuffer	(void);
							~DestroyBuffer	(void) {}
	const char*				getName			(void) const { return "DestroyBuffer"; }

	void					logExecute		(TestLog& log, size_t commandIndex) const;
	void					prepare			(PrepareContext& context);
	void					execute			(ExecuteContext& context);

private:
	vk::Move<vk::VkBuffer>	m_buffer;
};

DestroyBuffer::DestroyBuffer (void)
{
}

void DestroyBuffer::prepare (PrepareContext& context)
{
	m_buffer = vk::Move<vk::VkBuffer>(vk::check(context.getBuffer()), vk::Deleter<vk::VkBuffer>(context.getContext().getDeviceInterface(), context.getContext().getDevice(), DE_NULL));
	context.releaseBuffer();
}

void DestroyBuffer::logExecute (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Destroy buffer" << TestLog::EndMessage;
}

void DestroyBuffer::execute (ExecuteContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkDevice			device			= context.getContext().getDevice();

	vkd.destroyBuffer(device, m_buffer.disown(), DE_NULL);
}

class BindBufferMemory : public Command
{
public:
				BindBufferMemory	(void) {}
				~BindBufferMemory	(void) {}
	const char*	getName				(void) const { return "BindBufferMemory"; }

	void		logPrepare			(TestLog& log, size_t commandIndex) const;
	void		prepare				(PrepareContext& context);
};

void BindBufferMemory::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Bind memory to buffer" << TestLog::EndMessage;
}

void BindBufferMemory::prepare (PrepareContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkDevice			device			= context.getContext().getDevice();

	VK_CHECK(vkd.bindBufferMemory(device, context.getBuffer(), context.getMemory().getMemory(), 0));
}

class CreateImage : public Command
{
public:
									CreateImage		(vk::VkImageUsageFlags	usage,
													 vk::VkSharingMode		sharing);
									~CreateImage	(void) {}
	const char*						getName			(void) const { return "CreateImage"; }

	void							logPrepare		(TestLog& log, size_t commandIndex) const;
	void							prepare			(PrepareContext& context);
	void							verify			(VerifyContext& context, size_t commandIndex);

private:
	const vk::VkImageUsageFlags	m_usage;
	const vk::VkSharingMode		m_sharing;
	deInt32						m_imageWidth;
	deInt32						m_imageHeight;
};

CreateImage::CreateImage (vk::VkImageUsageFlags	usage,
						  vk::VkSharingMode		sharing)
	: m_usage	(usage)
	, m_sharing	(sharing)
{
}

void CreateImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create image, sharing: " << m_sharing << ", usage: " << vk::getImageUsageFlagsStr(m_usage)  << TestLog::EndMessage;
}

void CreateImage::prepare (PrepareContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkDevice			device			= context.getContext().getDevice();
	const vector<deUint32>&		queueFamilies	= context.getContext().getQueueFamilies();

	m_imageWidth	= context.getMemory().getMaxImageWidth();
	m_imageHeight	= context.getMemory().getMaxImageHeight();

	{
		const vk::VkImageCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			DE_NULL,

			0u,
			vk::VK_IMAGE_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{
				(deUint32)m_imageWidth,
				(deUint32)m_imageHeight,
				1u,
			},
			1u, 1u,
			vk::VK_SAMPLE_COUNT_1_BIT,
			vk::VK_IMAGE_TILING_OPTIMAL,
			m_usage,
			m_sharing,
			(deUint32)queueFamilies.size(),
			&queueFamilies[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};
		vk::Move<vk::VkImage>			image			(createImage(vkd, device, &createInfo));
		const vk::VkMemoryRequirements	requirements	= vk::getImageMemoryRequirements(vkd, device, *image);

		context.setImage(image, vk::VK_IMAGE_LAYOUT_UNDEFINED, requirements.size, m_imageWidth, m_imageHeight);
	}
}

void CreateImage::verify (VerifyContext& context, size_t)
{
	context.getReferenceImage() = TextureLevel(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_imageWidth, m_imageHeight);
}

class DestroyImage : public Command
{
public:
							DestroyImage	(void);
							~DestroyImage	(void) {}
	const char*				getName			(void) const { return "DestroyImage"; }

	void					logExecute		(TestLog& log, size_t commandIndex) const;
	void					prepare			(PrepareContext& context);
	void					execute			(ExecuteContext& context);

private:
	vk::Move<vk::VkImage>	m_image;
};

DestroyImage::DestroyImage (void)
{
}

void DestroyImage::prepare (PrepareContext& context)
{
	m_image = vk::Move<vk::VkImage>(vk::check(context.getImage()), vk::Deleter<vk::VkImage>(context.getContext().getDeviceInterface(), context.getContext().getDevice(), DE_NULL));
	context.releaseImage();
}


void DestroyImage::logExecute (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Destroy image" << TestLog::EndMessage;
}

void DestroyImage::execute (ExecuteContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkDevice			device			= context.getContext().getDevice();

	vkd.destroyImage(device, m_image.disown(), DE_NULL);
}

class BindImageMemory : public Command
{
public:
				BindImageMemory		(void) {}
				~BindImageMemory	(void) {}
	const char*	getName				(void) const { return "BindImageMemory"; }

	void		logPrepare			(TestLog& log, size_t commandIndex) const;
	void		prepare				(PrepareContext& context);
};

void BindImageMemory::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Bind memory to image" << TestLog::EndMessage;
}

void BindImageMemory::prepare (PrepareContext& context)
{
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkDevice				device			= context.getContext().getDevice();

	VK_CHECK(vkd.bindImageMemory(device, context.getImage(), context.getMemory().getMemory(), 0));
}

class QueueWaitIdle : public Command
{
public:
				QueueWaitIdle	(void) {}
				~QueueWaitIdle	(void) {}
	const char*	getName			(void) const { return "QueuetWaitIdle"; }

	void		logExecute		(TestLog& log, size_t commandIndex) const;
	void		execute			(ExecuteContext& context);
};

void QueueWaitIdle::logExecute (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Queue wait idle" << TestLog::EndMessage;
}

void QueueWaitIdle::execute (ExecuteContext& context)
{
	const vk::DeviceInterface&	vkd		= context.getContext().getDeviceInterface();
	const vk::VkQueue			queue	= context.getContext().getQueue();

	VK_CHECK(vkd.queueWaitIdle(queue));
}

class DeviceWaitIdle : public Command
{
public:
				DeviceWaitIdle	(void) {}
				~DeviceWaitIdle	(void) {}
	const char*	getName			(void) const { return "DeviceWaitIdle"; }

	void		logExecute		(TestLog& log, size_t commandIndex) const;
	void		execute			(ExecuteContext& context);
};

void DeviceWaitIdle::logExecute (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Device wait idle" << TestLog::EndMessage;
}

void DeviceWaitIdle::execute (ExecuteContext& context)
{
	const vk::DeviceInterface&	vkd		= context.getContext().getDeviceInterface();
	const vk::VkDevice			device	= context.getContext().getDevice();

	VK_CHECK(vkd.deviceWaitIdle(device));
}

class SubmitContext
{
public:
								SubmitContext		(const PrepareContext&		context,
													 const vk::VkCommandBuffer	commandBuffer)
		: m_context			(context)
		, m_commandBuffer	(commandBuffer)
	{
	}

	const Memory&				getMemory			(void) const { return m_context.getMemory(); }
	const Context&				getContext			(void) const { return m_context.getContext(); }
	vk::VkCommandBuffer			getCommandBuffer	(void) const { return m_commandBuffer; }

	vk::VkBuffer				getBuffer			(void) const { return m_context.getBuffer(); }
	vk::VkDeviceSize			getBufferSize		(void) const { return m_context.getBufferSize(); }

	vk::VkImage					getImage			(void) const { return m_context.getImage(); }
	deInt32						getImageWidth		(void) const { return m_context.getImageWidth(); }
	deInt32						getImageHeight		(void) const { return m_context.getImageHeight(); }

private:
	const PrepareContext&		m_context;
	const vk::VkCommandBuffer	m_commandBuffer;
};

class CmdCommand
{
public:
	virtual				~CmdCommand	(void) {}
	virtual const char*	getName		(void) const = 0;

	// Log things that are done during prepare
	virtual void		logPrepare	(TestLog&, size_t) const {}
	// Log submitted calls etc.
	virtual void		logSubmit	(TestLog&, size_t) const {}

	// Allocate vulkan resources and prepare for submit.
	virtual void		prepare		(PrepareContext&) {}

	// Submit commands to command buffer.
	virtual void		submit		(SubmitContext&) {}

	// Verify results
	virtual void		verify		(VerifyContext&, size_t) {}
};

class SubmitCommandBuffer : public Command
{
public:
					SubmitCommandBuffer		(const vector<CmdCommand*>& commands);
					~SubmitCommandBuffer	(void);

	const char*		getName					(void) const { return "SubmitCommandBuffer"; }
	void			logExecute				(TestLog& log, size_t commandIndex) const;
	void			logPrepare				(TestLog& log, size_t commandIndex) const;

	// Allocate command buffer and submit commands to command buffer
	void			prepare					(PrepareContext& context);
	void			execute					(ExecuteContext& context);

	// Verify that results are correct.
	void			verify					(VerifyContext& context, size_t commandIndex);

private:
	vector<CmdCommand*>				m_commands;
	vk::Move<vk::VkCommandBuffer>	m_commandBuffer;
};

SubmitCommandBuffer::SubmitCommandBuffer (const vector<CmdCommand*>& commands)
	: m_commands	(commands)
{
}

SubmitCommandBuffer::~SubmitCommandBuffer (void)
{
	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
		delete m_commands[cmdNdx];
}

void SubmitCommandBuffer::prepare (PrepareContext& context)
{
	const vk::DeviceInterface&	vkd			= context.getContext().getDeviceInterface();
	const vk::VkDevice			device		= context.getContext().getDevice();
	const vk::VkCommandPool		commandPool	= context.getContext().getCommandPool();

	m_commandBuffer = createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
	{
		CmdCommand& command = *m_commands[cmdNdx];

		command.prepare(context);
	}

	{
		SubmitContext submitContext (context, *m_commandBuffer);

		for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
		{
			CmdCommand& command = *m_commands[cmdNdx];

			command.submit(submitContext);
		}

		VK_CHECK(vkd.endCommandBuffer(*m_commandBuffer));
	}
}

void SubmitCommandBuffer::execute (ExecuteContext& context)
{
	const vk::DeviceInterface&	vkd		= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	cmd		= *m_commandBuffer;
	const vk::VkQueue			queue	= context.getContext().getQueue();
	const vk::VkSubmitInfo		submit	=
	{
		vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,
		DE_NULL,

		0,
		DE_NULL,
		(const vk::VkPipelineStageFlags*)DE_NULL,

		1,
		&cmd,

		0,
		DE_NULL
	};

	vkd.queueSubmit(queue, 1, &submit, 0);
}

void SubmitCommandBuffer::verify (VerifyContext& context, size_t commandIndex)
{
	const string				sectionName	(de::toString(commandIndex) + ":" + getName());
	const tcu::ScopedLogSection	section		(context.getLog(), sectionName, sectionName);

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
		m_commands[cmdNdx]->verify(context, cmdNdx);
}

void SubmitCommandBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	const string				sectionName	(de::toString(commandIndex) + ":" + getName());
	const tcu::ScopedLogSection	section		(log, sectionName, sectionName);

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
		m_commands[cmdNdx]->logPrepare(log, cmdNdx);
}

void SubmitCommandBuffer::logExecute (TestLog& log, size_t commandIndex) const
{
	const string				sectionName	(de::toString(commandIndex) + ":" + getName());
	const tcu::ScopedLogSection	section		(log, sectionName, sectionName);

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
		m_commands[cmdNdx]->logSubmit(log, cmdNdx);
}

class PipelineBarrier : public CmdCommand
{
public:
	enum Type
	{
		TYPE_GLOBAL = 0,
		TYPE_BUFFER,
		TYPE_IMAGE,
		TYPE_LAST
	};
									PipelineBarrier		(const vk::VkPipelineStageFlags			srcStages,
														 const vk::VkAccessFlags				srcAccesses,
														 const vk::VkPipelineStageFlags			dstStages,
														 const vk::VkAccessFlags				dstAccesses,
														 Type									type,
														 const tcu::Maybe<vk::VkImageLayout>	imageLayout);
									~PipelineBarrier	(void) {}
	const char*						getName				(void) const { return "PipelineBarrier"; }

	void							logSubmit			(TestLog& log, size_t commandIndex) const;
	void							submit				(SubmitContext& context);

private:
	const vk::VkPipelineStageFlags		m_srcStages;
	const vk::VkAccessFlags				m_srcAccesses;
	const vk::VkPipelineStageFlags		m_dstStages;
	const vk::VkAccessFlags				m_dstAccesses;
	const Type							m_type;
	const tcu::Maybe<vk::VkImageLayout>	m_imageLayout;
};

PipelineBarrier::PipelineBarrier (const vk::VkPipelineStageFlags		srcStages,
								  const vk::VkAccessFlags				srcAccesses,
								  const vk::VkPipelineStageFlags		dstStages,
								  const vk::VkAccessFlags				dstAccesses,
								  Type									type,
								  const tcu::Maybe<vk::VkImageLayout>	imageLayout)
	: m_srcStages	(srcStages)
	, m_srcAccesses	(srcAccesses)
	, m_dstStages	(dstStages)
	, m_dstAccesses	(dstAccesses)
	, m_type		(type)
	, m_imageLayout	(imageLayout)
{
}

void PipelineBarrier::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName()
		<< " " << (m_type == TYPE_GLOBAL ? "Global pipeline barrier"
					: m_type == TYPE_BUFFER ? "Buffer pipeline barrier"
					: "Image pipeline barrier")
		<< ", srcStages: " << vk::getPipelineStageFlagsStr(m_srcStages) << ", srcAccesses: " << vk::getAccessFlagsStr(m_srcAccesses)
		<< ", dstStages: " << vk::getPipelineStageFlagsStr(m_dstStages) << ", dstAccesses: " << vk::getAccessFlagsStr(m_dstAccesses) << TestLog::EndMessage;
}

void PipelineBarrier::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd	= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	cmd	= context.getCommandBuffer();

	switch (m_type)
	{
		case TYPE_GLOBAL:
		{
			const vk::VkMemoryBarrier	barrier		=
			{
				vk::VK_STRUCTURE_TYPE_MEMORY_BARRIER,
				DE_NULL,

				m_srcAccesses,
				m_dstAccesses
			};

			vkd.cmdPipelineBarrier(cmd, m_srcStages, m_dstStages, (vk::VkDependencyFlags)0, 1, &barrier, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 0, (const vk::VkImageMemoryBarrier*)DE_NULL);
			break;
		}

		case TYPE_BUFFER:
		{
			const vk::VkBufferMemoryBarrier	barrier		=
			{
				vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				DE_NULL,

				m_srcAccesses,
				m_dstAccesses,

				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,

				context.getBuffer(),
				0,
				VK_WHOLE_SIZE
			};

			vkd.cmdPipelineBarrier(cmd, m_srcStages, m_dstStages, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 1, &barrier, 0, (const vk::VkImageMemoryBarrier*)DE_NULL);
			break;
		}

		case TYPE_IMAGE:
		{
			const vk::VkImageMemoryBarrier	barrier		=
			{
				vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				DE_NULL,

				m_srcAccesses,
				m_dstAccesses,

				*m_imageLayout,
				*m_imageLayout,

				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,

				context.getImage(),
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,
					0, 1,
					0, 1
				}
			};

			vkd.cmdPipelineBarrier(cmd, m_srcStages, m_dstStages, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &barrier);
			break;
		}

		default:
			DE_FATAL("Unknown pipeline barrier type");
	}
}

class ImageTransition : public CmdCommand
{
public:
						ImageTransition		(vk::VkPipelineStageFlags	srcStages,
											 vk::VkAccessFlags			srcAccesses,

											 vk::VkPipelineStageFlags	dstStages,
											 vk::VkAccessFlags			dstAccesses,

											 vk::VkImageLayout			srcLayout,
											 vk::VkImageLayout			dstLayout);

						~ImageTransition	(void) {}
	const char*			getName				(void) const { return "ImageTransition"; }

	void				prepare				(PrepareContext& context);
	void				logSubmit			(TestLog& log, size_t commandIndex) const;
	void				submit				(SubmitContext& context);
	void				verify				(VerifyContext& context, size_t);

private:
	const vk::VkPipelineStageFlags	m_srcStages;
	const vk::VkAccessFlags			m_srcAccesses;
	const vk::VkPipelineStageFlags	m_dstStages;
	const vk::VkAccessFlags			m_dstAccesses;
	const vk::VkImageLayout			m_srcLayout;
	const vk::VkImageLayout			m_dstLayout;

	vk::VkDeviceSize				m_imageMemorySize;
};

ImageTransition::ImageTransition (vk::VkPipelineStageFlags	srcStages,
								  vk::VkAccessFlags			srcAccesses,

								  vk::VkPipelineStageFlags	dstStages,
								  vk::VkAccessFlags			dstAccesses,

								  vk::VkImageLayout			srcLayout,
								  vk::VkImageLayout			dstLayout)
	: m_srcStages		(srcStages)
	, m_srcAccesses		(srcAccesses)
	, m_dstStages		(dstStages)
	, m_dstAccesses		(dstAccesses)
	, m_srcLayout		(srcLayout)
	, m_dstLayout		(dstLayout)
{
}

void ImageTransition::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName()
		<< " Image transition pipeline barrier"
		<< ", srcStages: " << vk::getPipelineStageFlagsStr(m_srcStages) << ", srcAccesses: " << vk::getAccessFlagsStr(m_srcAccesses)
		<< ", dstStages: " << vk::getPipelineStageFlagsStr(m_dstStages) << ", dstAccesses: " << vk::getAccessFlagsStr(m_dstAccesses)
		<< ", srcLayout: " << m_srcLayout << ", dstLayout: " << m_dstLayout << TestLog::EndMessage;
}

void ImageTransition::prepare (PrepareContext& context)
{
	DE_ASSERT(context.getImageLayout() == vk::VK_IMAGE_LAYOUT_UNDEFINED || m_srcLayout == vk::VK_IMAGE_LAYOUT_UNDEFINED || context.getImageLayout() == m_srcLayout);

	context.setImageLayout(m_dstLayout);
	m_imageMemorySize = context.getImageMemorySize();
}

void ImageTransition::submit (SubmitContext& context)
{
	const vk::DeviceInterface&		vkd			= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer		cmd			= context.getCommandBuffer();
	const vk::VkImageMemoryBarrier	barrier		=
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		DE_NULL,

		m_srcAccesses,
		m_dstAccesses,

		m_srcLayout,
		m_dstLayout,

		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,

		context.getImage(),
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0u, 1u,
			0u, 1u
		}
	};

	vkd.cmdPipelineBarrier(cmd, m_srcStages, m_dstStages, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &barrier);
}

void ImageTransition::verify (VerifyContext& context, size_t)
{
	context.getReference().setUndefined(0, (size_t)m_imageMemorySize);
}

class FillBuffer : public CmdCommand
{
public:
						FillBuffer	(deUint32 value) : m_value(value) {}
						~FillBuffer	(void) {}
	const char*			getName		(void) const { return "FillBuffer"; }

	void				logSubmit	(TestLog& log, size_t commandIndex) const;
	void				submit		(SubmitContext& context);
	void				verify		(VerifyContext& context, size_t commandIndex);

private:
	const deUint32		m_value;
	vk::VkDeviceSize	m_bufferSize;
};

void FillBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Fill value: " << m_value << TestLog::EndMessage;
}

void FillBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd			= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	cmd			= context.getCommandBuffer();
	const vk::VkBuffer			buffer		= context.getBuffer();
	const vk::VkDeviceSize		sizeMask	= ~(0x3ull); // \note Round down to multiple of 4

	m_bufferSize = sizeMask & context.getBufferSize();
	vkd.cmdFillBuffer(cmd, buffer, 0, m_bufferSize, m_value);
}

void FillBuffer::verify (VerifyContext& context, size_t)
{
	ReferenceMemory&	reference	= context.getReference();

	for (size_t ndx = 0; ndx < m_bufferSize; ndx++)
	{
#if (DE_ENDIANNESS == DE_LITTLE_ENDIAN)
		reference.set(ndx, (deUint8)(0xffu & (m_value >> (8*(ndx % 4)))));
#else
		reference.set(ndx, (deUint8)(0xffu & (m_value >> (8*(3 - (ndx % 4))))));
#endif
	}
}

class UpdateBuffer : public CmdCommand
{
public:
						UpdateBuffer	(deUint32 seed) : m_seed(seed) {}
						~UpdateBuffer	(void) {}
	const char*			getName			(void) const { return "UpdateBuffer"; }

	void				logSubmit		(TestLog& log, size_t commandIndex) const;
	void				submit			(SubmitContext& context);
	void				verify			(VerifyContext& context, size_t commandIndex);

private:
	const deUint32		m_seed;
	vk::VkDeviceSize	m_bufferSize;
};

void UpdateBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Update buffer, seed: " << m_seed << TestLog::EndMessage;
}

void UpdateBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd			= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	cmd			= context.getCommandBuffer();
	const vk::VkBuffer			buffer		= context.getBuffer();
	const size_t				blockSize	= 65536;
	std::vector<deUint8>		data		(blockSize, 0);
	de::Random					rng			(m_seed);

	m_bufferSize = context.getBufferSize();

	for (size_t updated = 0; updated < m_bufferSize; updated += blockSize)
	{
		for (size_t ndx = 0; ndx < data.size(); ndx++)
			data[ndx] = rng.getUint8();

		if (m_bufferSize - updated > blockSize)
			vkd.cmdUpdateBuffer(cmd, buffer, updated, blockSize, (const deUint32*)(&data[0]));
		else
			vkd.cmdUpdateBuffer(cmd, buffer, updated, m_bufferSize - updated, (const deUint32*)(&data[0]));
	}
}

void UpdateBuffer::verify (VerifyContext& context, size_t)
{
	ReferenceMemory&	reference	= context.getReference();
	const size_t		blockSize	= 65536;
	vector<deUint8>		data		(blockSize, 0);
	de::Random			rng			(m_seed);

	for (size_t updated = 0; updated < m_bufferSize; updated += blockSize)
	{
		for (size_t ndx = 0; ndx < data.size(); ndx++)
			data[ndx] = rng.getUint8();

		if (m_bufferSize - updated > blockSize)
			reference.setData(updated, blockSize, &data[0]);
		else
			reference.setData(updated, (size_t)(m_bufferSize - updated), &data[0]);
	}
}

class BufferCopyToBuffer : public CmdCommand
{
public:
									BufferCopyToBuffer	(void) {}
									~BufferCopyToBuffer	(void) {}
	const char*						getName				(void) const { return "BufferCopyToBuffer"; }

	void							logPrepare			(TestLog& log, size_t commandIndex) const;
	void							prepare				(PrepareContext& context);
	void							logSubmit			(TestLog& log, size_t commandIndex) const;
	void							submit				(SubmitContext& context);
	void							verify				(VerifyContext& context, size_t commandIndex);

private:
	vk::VkDeviceSize				m_bufferSize;
	vk::Move<vk::VkBuffer>			m_dstBuffer;
	vk::Move<vk::VkDeviceMemory>	m_memory;
};

void BufferCopyToBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate destination buffer for buffer to buffer copy." << TestLog::EndMessage;
}

void BufferCopyToBuffer::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();

	m_bufferSize = context.getBufferSize();

	m_dstBuffer	= createBuffer(vkd, device, m_bufferSize, vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies);
	m_memory	= bindBufferMemory(vki, vkd, physicalDevice, device, *m_dstBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
}

void BufferCopyToBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Copy buffer to another buffer" << TestLog::EndMessage;
}

void BufferCopyToBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkBufferCopy		range			=
	{
		0, 0, // Offsets
		m_bufferSize
	};

	vkd.cmdCopyBuffer(commandBuffer, context.getBuffer(), *m_dstBuffer, 1, &range);
}

void BufferCopyToBuffer::verify (VerifyContext& context, size_t commandIndex)
{
	tcu::ResultCollector&					resultCollector	(context.getResultCollector());
	ReferenceMemory&						reference		(context.getReference());
	const vk::DeviceInterface&				vkd				= context.getContext().getDeviceInterface();
	const vk::VkDevice						device			= context.getContext().getDevice();
	const vk::VkQueue						queue			= context.getContext().getQueue();
	const vk::VkCommandPool					commandPool		= context.getContext().getCommandPool();
	const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const vk::VkBufferMemoryBarrier			barrier			=
	{
		vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		DE_NULL,

		vk::VK_ACCESS_TRANSFER_WRITE_BIT,
		vk::VK_ACCESS_HOST_READ_BIT,

		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		*m_dstBuffer,
		0,
		VK_WHOLE_SIZE
	};

	vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_HOST_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 1, &barrier, 0, (const vk::VkImageMemoryBarrier*)DE_NULL);

	VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
	queueRun(vkd, queue, *commandBuffer);

	{
		void* const	ptr		= mapMemory(vkd, device, *m_memory, m_bufferSize);
		bool		isOk	= true;

		vk::invalidateMappedMemoryRange(vkd, device, *m_memory, 0, m_bufferSize);

		{
			const deUint8* const data = (const deUint8*)ptr;

			for (size_t pos = 0; pos < (size_t)m_bufferSize; pos++)
			{
				if (reference.isDefined(pos))
				{
					if (data[pos] != reference.get(pos))
					{
						resultCollector.fail(
								de::toString(commandIndex) + ":" + getName()
								+ " Result differs from reference, Expected: "
								+ de::toString(tcu::toHex<8>(reference.get(pos)))
								+ ", Got: "
								+ de::toString(tcu::toHex<8>(data[pos]))
								+ ", At offset: "
								+ de::toString(pos));
						break;
					}
				}
			}
		}

		vkd.unmapMemory(device, *m_memory);

		if (!isOk)
			context.getLog() << TestLog::Message << commandIndex << ": Buffer copy to buffer verification failed" << TestLog::EndMessage;
	}
}

class BufferCopyFromBuffer : public CmdCommand
{
public:
									BufferCopyFromBuffer	(deUint32 seed) : m_seed(seed) {}
									~BufferCopyFromBuffer	(void) {}
	const char*						getName					(void) const { return "BufferCopyFromBuffer"; }

	void							logPrepare				(TestLog& log, size_t commandIndex) const;
	void							prepare					(PrepareContext& context);
	void							logSubmit				(TestLog& log, size_t commandIndex) const;
	void							submit					(SubmitContext& context);
	void							verify					(VerifyContext& context, size_t commandIndex);

private:
	const deUint32					m_seed;
	vk::VkDeviceSize				m_bufferSize;
	vk::Move<vk::VkBuffer>			m_srcBuffer;
	vk::Move<vk::VkDeviceMemory>	m_memory;
};

void BufferCopyFromBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate source buffer for buffer to buffer copy. Seed: " << m_seed << TestLog::EndMessage;
}

void BufferCopyFromBuffer::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();

	m_bufferSize	= context.getBufferSize();
	m_srcBuffer		= createBuffer(vkd, device, m_bufferSize, vk::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies);
	m_memory		= bindBufferMemory(vki, vkd, physicalDevice, device, *m_srcBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	{
		void* const	ptr	= mapMemory(vkd, device, *m_memory, m_bufferSize);
		de::Random	rng	(m_seed);

		{
			deUint8* const	data = (deUint8*)ptr;

			for (size_t ndx = 0; ndx < (size_t)m_bufferSize; ndx++)
				data[ndx] = rng.getUint8();
		}

		vk::flushMappedMemoryRange(vkd, device, *m_memory, 0, m_bufferSize);
		vkd.unmapMemory(device, *m_memory);
	}
}

void BufferCopyFromBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Copy buffer data from another buffer" << TestLog::EndMessage;
}

void BufferCopyFromBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkBufferCopy		range			=
	{
		0, 0, // Offsets
		m_bufferSize
	};

	vkd.cmdCopyBuffer(commandBuffer, *m_srcBuffer, context.getBuffer(), 1, &range);
}

void BufferCopyFromBuffer::verify (VerifyContext& context, size_t)
{
	ReferenceMemory&	reference	(context.getReference());
	de::Random			rng			(m_seed);

	for (size_t ndx = 0; ndx < (size_t)m_bufferSize; ndx++)
		reference.set(ndx, rng.getUint8());
}

class BufferCopyToImage : public CmdCommand
{
public:
									BufferCopyToImage	(void) {}
									~BufferCopyToImage	(void) {}
	const char*						getName				(void) const { return "BufferCopyToImage"; }

	void							logPrepare			(TestLog& log, size_t commandIndex) const;
	void							prepare				(PrepareContext& context);
	void							logSubmit			(TestLog& log, size_t commandIndex) const;
	void							submit				(SubmitContext& context);
	void							verify				(VerifyContext& context, size_t commandIndex);

private:
	deInt32							m_imageWidth;
	deInt32							m_imageHeight;
	vk::Move<vk::VkImage>			m_dstImage;
	vk::Move<vk::VkDeviceMemory>	m_memory;
};

void BufferCopyToImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate destination image for buffer to image copy." << TestLog::EndMessage;
}

void BufferCopyToImage::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vk::VkQueue				queue			= context.getContext().getQueue();
	const vk::VkCommandPool			commandPool		= context.getContext().getCommandPool();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();
	const IVec2						imageSize		= findImageSizeWxHx4(context.getBufferSize());

	m_imageWidth	= imageSize[0];
	m_imageHeight	= imageSize[1];

	{
		const vk::VkImageCreateInfo	createInfo =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			DE_NULL,

			0,
			vk::VK_IMAGE_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{
				(deUint32)m_imageWidth,
				(deUint32)m_imageHeight,
				1u,
			},
			1, 1, // mipLevels, arrayLayers
			vk::VK_SAMPLE_COUNT_1_BIT,

			vk::VK_IMAGE_TILING_OPTIMAL,
			vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT|vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			vk::VK_SHARING_MODE_EXCLUSIVE,

			(deUint32)queueFamilies.size(),
			&queueFamilies[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		m_dstImage = vk::createImage(vkd, device, &createInfo);
	}

	m_memory = bindImageMemory(vki, vkd, physicalDevice, device, *m_dstImage, 0);

	{
		const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const vk::VkImageMemoryBarrier			barrier			=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			0,
			vk::VK_ACCESS_TRANSFER_WRITE_BIT,

			vk::VK_IMAGE_LAYOUT_UNDEFINED,
			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_dstImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};

		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &barrier);

		VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
		queueRun(vkd, queue, *commandBuffer);
	}
}

void BufferCopyToImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Copy buffer to image" << TestLog::EndMessage;
}

void BufferCopyToImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkBufferImageCopy	region			=
	{
		0,
		0, 0,
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{ 0, 0, 0 },
		{
			(deUint32)m_imageWidth,
			(deUint32)m_imageHeight,
			1u
		}
	};

	vkd.cmdCopyBufferToImage(commandBuffer, context.getBuffer(), *m_dstImage, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void BufferCopyToImage::verify (VerifyContext& context, size_t commandIndex)
{
	tcu::ResultCollector&					resultCollector	(context.getResultCollector());
	ReferenceMemory&						reference		(context.getReference());
	const vk::InstanceInterface&			vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&				vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice				physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice						device			= context.getContext().getDevice();
	const vk::VkQueue						queue			= context.getContext().getQueue();
	const vk::VkCommandPool					commandPool		= context.getContext().getCommandPool();
	const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const vector<deUint32>&					queueFamilies	= context.getContext().getQueueFamilies();
	const vk::Unique<vk::VkBuffer>			dstBuffer		(createBuffer(vkd, device, 4 * m_imageWidth * m_imageHeight, vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies));
	const vk::Unique<vk::VkDeviceMemory>	memory			(bindBufferMemory(vki, vkd, physicalDevice, device, *dstBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
	{
		const vk::VkImageMemoryBarrier		imageBarrier	=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			vk::VK_ACCESS_TRANSFER_WRITE_BIT,
			vk::VK_ACCESS_TRANSFER_READ_BIT,

			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_dstImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};
		const vk::VkBufferMemoryBarrier bufferBarrier =
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,

			vk::VK_ACCESS_TRANSFER_WRITE_BIT,
			vk::VK_ACCESS_HOST_READ_BIT,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			*dstBuffer,
			0,
			VK_WHOLE_SIZE
		};

		const vk::VkBufferImageCopy	region =
		{
			0,
			0, 0,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// mipLevel
				0,	// arrayLayer
				1	// layerCount
			},
			{ 0, 0, 0 },
			{
				(deUint32)m_imageWidth,
				(deUint32)m_imageHeight,
				1u
			}
		};

		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &imageBarrier);
		vkd.cmdCopyImageToBuffer(*commandBuffer, *m_dstImage, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *dstBuffer, 1, &region);
		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_HOST_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 1, &bufferBarrier, 0, (const vk::VkImageMemoryBarrier*)DE_NULL);
	}

	VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
	queueRun(vkd, queue, *commandBuffer);

	{
		void* const	ptr		= mapMemory(vkd, device, *memory, 4 * m_imageWidth * m_imageHeight);

		vk::invalidateMappedMemoryRange(vkd, device, *memory, 0,  4 * m_imageWidth * m_imageHeight);

		{
			const deUint8* const	data = (const deUint8*)ptr;

			for (size_t pos = 0; pos < (size_t)( 4 * m_imageWidth * m_imageHeight); pos++)
			{
				if (reference.isDefined(pos))
				{
					if (data[pos] != reference.get(pos))
					{
						resultCollector.fail(
								de::toString(commandIndex) + ":" + getName()
								+ " Result differs from reference, Expected: "
								+ de::toString(tcu::toHex<8>(reference.get(pos)))
								+ ", Got: "
								+ de::toString(tcu::toHex<8>(data[pos]))
								+ ", At offset: "
								+ de::toString(pos));
						break;
					}
				}
			}
		}

		vkd.unmapMemory(device, *memory);
	}
}

class BufferCopyFromImage : public CmdCommand
{
public:
									BufferCopyFromImage		(deUint32 seed) : m_seed(seed) {}
									~BufferCopyFromImage	(void) {}
	const char*						getName					(void) const { return "BufferCopyFromImage"; }

	void							logPrepare				(TestLog& log, size_t commandIndex) const;
	void							prepare					(PrepareContext& context);
	void							logSubmit				(TestLog& log, size_t commandIndex) const;
	void							submit					(SubmitContext& context);
	void							verify					(VerifyContext& context, size_t commandIndex);

private:
	const deUint32					m_seed;
	deInt32							m_imageWidth;
	deInt32							m_imageHeight;
	vk::Move<vk::VkImage>			m_srcImage;
	vk::Move<vk::VkDeviceMemory>	m_memory;
};

void BufferCopyFromImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate source image for image to buffer copy." << TestLog::EndMessage;
}

void BufferCopyFromImage::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vk::VkQueue				queue			= context.getContext().getQueue();
	const vk::VkCommandPool			commandPool		= context.getContext().getCommandPool();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();
	const IVec2						imageSize		= findImageSizeWxHx4(context.getBufferSize());

	m_imageWidth	= imageSize[0];
	m_imageHeight	= imageSize[1];

	{
		const vk::VkImageCreateInfo	createInfo =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			DE_NULL,

			0,
			vk::VK_IMAGE_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{
				(deUint32)m_imageWidth,
				(deUint32)m_imageHeight,
				1u,
			},
			1, 1, // mipLevels, arrayLayers
			vk::VK_SAMPLE_COUNT_1_BIT,

			vk::VK_IMAGE_TILING_OPTIMAL,
			vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT|vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			vk::VK_SHARING_MODE_EXCLUSIVE,

			(deUint32)queueFamilies.size(),
			&queueFamilies[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		m_srcImage = vk::createImage(vkd, device, &createInfo);
	}

	m_memory = bindImageMemory(vki, vkd, physicalDevice, device, *m_srcImage, 0);

	{
		const vk::Unique<vk::VkBuffer>			srcBuffer		(createBuffer(vkd, device, 4 * m_imageWidth * m_imageHeight, vk::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies));
		const vk::Unique<vk::VkDeviceMemory>	memory			(bindBufferMemory(vki, vkd, physicalDevice, device, *srcBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
		const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const vk::VkImageMemoryBarrier			preImageBarrier	=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			0,
			vk::VK_ACCESS_TRANSFER_WRITE_BIT,

			vk::VK_IMAGE_LAYOUT_UNDEFINED,
			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_srcImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};
		const vk::VkImageMemoryBarrier			postImageBarrier =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			vk::VK_ACCESS_TRANSFER_WRITE_BIT,
			0,

			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_srcImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};
		const vk::VkBufferImageCopy				region				=
		{
			0,
			0, 0,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// mipLevel
				0,	// arrayLayer
				1	// layerCount
			},
			{ 0, 0, 0 },
			{
				(deUint32)m_imageWidth,
				(deUint32)m_imageHeight,
				1u
			}
		};

		{
			void* const	ptr	= mapMemory(vkd, device, *memory, 4 * m_imageWidth * m_imageHeight);
			de::Random	rng	(m_seed);

			{
				deUint8* const	data = (deUint8*)ptr;

				for (size_t ndx = 0; ndx < (size_t)(4 * m_imageWidth * m_imageHeight); ndx++)
					data[ndx] = rng.getUint8();
			}

			vk::flushMappedMemoryRange(vkd, device, *memory, 0, 4 * m_imageWidth * m_imageHeight);
			vkd.unmapMemory(device, *memory);
		}

		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &preImageBarrier);
		vkd.cmdCopyBufferToImage(*commandBuffer, *srcBuffer, *m_srcImage, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &postImageBarrier);

		VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
		queueRun(vkd, queue, *commandBuffer);
	}
}

void BufferCopyFromImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Copy buffer data from image" << TestLog::EndMessage;
}

void BufferCopyFromImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkBufferImageCopy	region			=
	{
		0,
		0, 0,
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{ 0, 0, 0 },
		{
			(deUint32)m_imageWidth,
			(deUint32)m_imageHeight,
			1u
		}
	};

	vkd.cmdCopyImageToBuffer(commandBuffer, *m_srcImage, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, context.getBuffer(), 1, &region);
}

void BufferCopyFromImage::verify (VerifyContext& context, size_t)
{
	ReferenceMemory&	reference		(context.getReference());
	de::Random			rng	(m_seed);

	for (size_t ndx = 0; ndx < (size_t)(4 * m_imageWidth * m_imageHeight); ndx++)
		reference.set(ndx, rng.getUint8());
}

class ImageCopyToBuffer : public CmdCommand
{
public:
									ImageCopyToBuffer	(vk::VkImageLayout imageLayout) : m_imageLayout (imageLayout) {}
									~ImageCopyToBuffer	(void) {}
	const char*						getName				(void) const { return "BufferCopyToImage"; }

	void							logPrepare			(TestLog& log, size_t commandIndex) const;
	void							prepare				(PrepareContext& context);
	void							logSubmit			(TestLog& log, size_t commandIndex) const;
	void							submit				(SubmitContext& context);
	void							verify				(VerifyContext& context, size_t commandIndex);

private:
	vk::VkImageLayout				m_imageLayout;
	vk::VkDeviceSize				m_bufferSize;
	vk::Move<vk::VkBuffer>			m_dstBuffer;
	vk::Move<vk::VkDeviceMemory>	m_memory;
	vk::VkDeviceSize				m_imageMemorySize;
	deInt32							m_imageWidth;
	deInt32							m_imageHeight;
};

void ImageCopyToBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate destination buffer for image to buffer copy." << TestLog::EndMessage;
}

void ImageCopyToBuffer::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();

	m_imageWidth		= context.getImageWidth();
	m_imageHeight		= context.getImageHeight();
	m_bufferSize		= 4 * m_imageWidth * m_imageHeight;
	m_imageMemorySize	= context.getImageMemorySize();
	m_dstBuffer			= createBuffer(vkd, device, m_bufferSize, vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies);
	m_memory			= bindBufferMemory(vki, vkd, physicalDevice, device, *m_dstBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
}

void ImageCopyToBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Copy image to buffer" << TestLog::EndMessage;
}

void ImageCopyToBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkBufferImageCopy	region			=
	{
		0,
		0, 0,
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{ 0, 0, 0 },
		{
			(deUint32)m_imageWidth,
			(deUint32)m_imageHeight,
			1u
		}
	};

	vkd.cmdCopyImageToBuffer(commandBuffer, context.getImage(), m_imageLayout, *m_dstBuffer, 1, &region);
}

void ImageCopyToBuffer::verify (VerifyContext& context, size_t commandIndex)
{
	tcu::ResultCollector&					resultCollector	(context.getResultCollector());
	ReferenceMemory&						reference		(context.getReference());
	const vk::DeviceInterface&				vkd				= context.getContext().getDeviceInterface();
	const vk::VkDevice						device			= context.getContext().getDevice();
	const vk::VkQueue						queue			= context.getContext().getQueue();
	const vk::VkCommandPool					commandPool		= context.getContext().getCommandPool();
	const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const vk::VkBufferMemoryBarrier			barrier			=
	{
		vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		DE_NULL,

		vk::VK_ACCESS_TRANSFER_WRITE_BIT,
		vk::VK_ACCESS_HOST_READ_BIT,

		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		*m_dstBuffer,
		0,
		VK_WHOLE_SIZE
	};

	vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_HOST_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 1, &barrier, 0, (const vk::VkImageMemoryBarrier*)DE_NULL);

	VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
	queueRun(vkd, queue, *commandBuffer);

	reference.setUndefined(0, (size_t)m_imageMemorySize);
	{
		void* const						ptr				= mapMemory(vkd, device, *m_memory, m_bufferSize);
		const ConstPixelBufferAccess	referenceImage	(context.getReferenceImage().getAccess());
		const ConstPixelBufferAccess	resultImage		(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_imageWidth, m_imageHeight, 1, ptr);

		vk::invalidateMappedMemoryRange(vkd, device, *m_memory, 0, m_bufferSize);

		if (!tcu::intThresholdCompare(context.getLog(), (de::toString(commandIndex) + ":" + getName()).c_str(), (de::toString(commandIndex) + ":" + getName()).c_str(), referenceImage, resultImage, UVec4(0), tcu::COMPARE_LOG_ON_ERROR))
			resultCollector.fail(de::toString(commandIndex) + ":" + getName() + " Image comparison failed");

		vkd.unmapMemory(device, *m_memory);
	}
}

class ImageCopyFromBuffer : public CmdCommand
{
public:
									ImageCopyFromBuffer		(deUint32 seed, vk::VkImageLayout imageLayout) : m_seed(seed), m_imageLayout(imageLayout) {}
									~ImageCopyFromBuffer	(void) {}
	const char*						getName					(void) const { return "ImageCopyFromBuffer"; }

	void							logPrepare				(TestLog& log, size_t commandIndex) const;
	void							prepare					(PrepareContext& context);
	void							logSubmit				(TestLog& log, size_t commandIndex) const;
	void							submit					(SubmitContext& context);
	void							verify					(VerifyContext& context, size_t commandIndex);

private:
	const deUint32					m_seed;
	const vk::VkImageLayout			m_imageLayout;
	deInt32							m_imageWidth;
	deInt32							m_imageHeight;
	vk::VkDeviceSize				m_imageMemorySize;
	vk::VkDeviceSize				m_bufferSize;
	vk::Move<vk::VkBuffer>			m_srcBuffer;
	vk::Move<vk::VkDeviceMemory>	m_memory;
};

void ImageCopyFromBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate source buffer for buffer to image copy. Seed: " << m_seed << TestLog::EndMessage;
}

void ImageCopyFromBuffer::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();

	m_imageWidth		= context.getImageHeight();
	m_imageHeight		= context.getImageWidth();
	m_imageMemorySize	= context.getImageMemorySize();
	m_bufferSize		= m_imageWidth * m_imageHeight * 4;
	m_srcBuffer			= createBuffer(vkd, device, m_bufferSize, vk::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies);
	m_memory			= bindBufferMemory(vki, vkd, physicalDevice, device, *m_srcBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	{
		void* const	ptr	= mapMemory(vkd, device, *m_memory, m_bufferSize);
		de::Random	rng	(m_seed);

		{
			deUint8* const	data = (deUint8*)ptr;

			for (size_t ndx = 0; ndx < (size_t)m_bufferSize; ndx++)
				data[ndx] = rng.getUint8();
		}

		vk::flushMappedMemoryRange(vkd, device, *m_memory, 0, m_bufferSize);
		vkd.unmapMemory(device, *m_memory);
	}
}

void ImageCopyFromBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Copy image data from buffer" << TestLog::EndMessage;
}

void ImageCopyFromBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkBufferImageCopy	region			=
	{
		0,
		0, 0,
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{ 0, 0, 0 },
		{
			(deUint32)m_imageWidth,
			(deUint32)m_imageHeight,
			1u
		}
	};

	vkd.cmdCopyBufferToImage(commandBuffer, *m_srcBuffer, context.getImage(), m_imageLayout, 1, &region);
}

void ImageCopyFromBuffer::verify (VerifyContext& context, size_t)
{
	ReferenceMemory&	reference	(context.getReference());
	de::Random			rng			(m_seed);

	reference.setUndefined(0, (size_t)m_imageMemorySize);

	{
		const PixelBufferAccess&	refAccess	(context.getReferenceImage().getAccess());

		for (deInt32 y = 0; y < m_imageHeight; y++)
		for (deInt32 x = 0; x < m_imageWidth; x++)
		{
			const deUint8 r8 = rng.getUint8();
			const deUint8 g8 = rng.getUint8();
			const deUint8 b8 = rng.getUint8();
			const deUint8 a8 = rng.getUint8();

			refAccess.setPixel(UVec4(r8, g8, b8, a8), x, y);
		}
	}
}

class ImageCopyFromImage : public CmdCommand
{
public:
									ImageCopyFromImage	(deUint32 seed, vk::VkImageLayout imageLayout) : m_seed(seed), m_imageLayout(imageLayout) {}
									~ImageCopyFromImage	(void) {}
	const char*						getName				(void) const { return "ImageCopyFromImage"; }

	void							logPrepare			(TestLog& log, size_t commandIndex) const;
	void							prepare				(PrepareContext& context);
	void							logSubmit			(TestLog& log, size_t commandIndex) const;
	void							submit				(SubmitContext& context);
	void							verify				(VerifyContext& context, size_t commandIndex);

private:
	const deUint32					m_seed;
	const vk::VkImageLayout			m_imageLayout;
	deInt32							m_imageWidth;
	deInt32							m_imageHeight;
	vk::VkDeviceSize				m_imageMemorySize;
	vk::Move<vk::VkImage>			m_srcImage;
	vk::Move<vk::VkDeviceMemory>	m_memory;
};

void ImageCopyFromImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate source image for image to image copy." << TestLog::EndMessage;
}

void ImageCopyFromImage::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vk::VkQueue				queue			= context.getContext().getQueue();
	const vk::VkCommandPool			commandPool		= context.getContext().getCommandPool();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();

	m_imageWidth		= context.getImageWidth();
	m_imageHeight		= context.getImageHeight();
	m_imageMemorySize	= context.getImageMemorySize();

	{
		const vk::VkImageCreateInfo	createInfo =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			DE_NULL,

			0,
			vk::VK_IMAGE_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{
				(deUint32)m_imageWidth,
				(deUint32)m_imageHeight,
				1u,
			},
			1, 1, // mipLevels, arrayLayers
			vk::VK_SAMPLE_COUNT_1_BIT,

			vk::VK_IMAGE_TILING_OPTIMAL,
			vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT|vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			vk::VK_SHARING_MODE_EXCLUSIVE,

			(deUint32)queueFamilies.size(),
			&queueFamilies[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		m_srcImage = vk::createImage(vkd, device, &createInfo);
	}

	m_memory = bindImageMemory(vki, vkd, physicalDevice, device, *m_srcImage, 0);

	{
		const vk::Unique<vk::VkBuffer>			srcBuffer		(createBuffer(vkd, device, 4 * m_imageWidth * m_imageHeight, vk::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies));
		const vk::Unique<vk::VkDeviceMemory>	memory			(bindBufferMemory(vki, vkd, physicalDevice, device, *srcBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
		const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const vk::VkImageMemoryBarrier			preImageBarrier	=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			0,
			vk::VK_ACCESS_TRANSFER_WRITE_BIT,

			vk::VK_IMAGE_LAYOUT_UNDEFINED,
			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_srcImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};
		const vk::VkImageMemoryBarrier			postImageBarrier =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			vk::VK_ACCESS_TRANSFER_WRITE_BIT,
			0,

			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_srcImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};
		const vk::VkBufferImageCopy				region				=
		{
			0,
			0, 0,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// mipLevel
				0,	// arrayLayer
				1	// layerCount
			},
			{ 0, 0, 0 },
			{
				(deUint32)m_imageWidth,
				(deUint32)m_imageHeight,
				1u
			}
		};

		{
			void* const	ptr	= mapMemory(vkd, device, *memory, 4 * m_imageWidth * m_imageHeight);
			de::Random	rng	(m_seed);

			{
				deUint8* const	data = (deUint8*)ptr;

				for (size_t ndx = 0; ndx < (size_t)(4 * m_imageWidth * m_imageHeight); ndx++)
					data[ndx] = rng.getUint8();
			}

			vk::flushMappedMemoryRange(vkd, device, *memory, 0, 4 * m_imageWidth * m_imageHeight);
			vkd.unmapMemory(device, *memory);
		}

		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &preImageBarrier);
		vkd.cmdCopyBufferToImage(*commandBuffer, *srcBuffer, *m_srcImage, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &postImageBarrier);

		VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
		queueRun(vkd, queue, *commandBuffer);
	}
}

void ImageCopyFromImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Copy image data from another image" << TestLog::EndMessage;
}

void ImageCopyFromImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkImageCopy		region			=
	{
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{ 0, 0, 0 },

		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{ 0, 0, 0 },
		{
			(deUint32)m_imageWidth,
			(deUint32)m_imageHeight,
			1u
		}
	};

	vkd.cmdCopyImage(commandBuffer, *m_srcImage, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, context.getImage(), m_imageLayout, 1, &region);
}

void ImageCopyFromImage::verify (VerifyContext& context, size_t)
{
	ReferenceMemory&	reference	(context.getReference());
	de::Random			rng			(m_seed);

	reference.setUndefined(0, (size_t)m_imageMemorySize);

	{
		const PixelBufferAccess&	refAccess	(context.getReferenceImage().getAccess());

		for (deInt32 y = 0; y < m_imageHeight; y++)
		for (deInt32 x = 0; x < m_imageWidth; x++)
		{
			const deUint8 r8 = rng.getUint8();
			const deUint8 g8 = rng.getUint8();
			const deUint8 b8 = rng.getUint8();
			const deUint8 a8 = rng.getUint8();

			refAccess.setPixel(UVec4(r8, g8, b8, a8), x, y);
		}
	}
}

class ImageCopyToImage : public CmdCommand
{
public:
									ImageCopyToImage	(vk::VkImageLayout imageLayout) : m_imageLayout(imageLayout) {}
									~ImageCopyToImage	(void) {}
	const char*						getName				(void) const { return "ImageCopyToImage"; }

	void							logPrepare			(TestLog& log, size_t commandIndex) const;
	void							prepare				(PrepareContext& context);
	void							logSubmit			(TestLog& log, size_t commandIndex) const;
	void							submit				(SubmitContext& context);
	void							verify				(VerifyContext& context, size_t commandIndex);

private:
	const vk::VkImageLayout			m_imageLayout;
	deInt32							m_imageWidth;
	deInt32							m_imageHeight;
	vk::VkDeviceSize				m_imageMemorySize;
	vk::Move<vk::VkImage>			m_dstImage;
	vk::Move<vk::VkDeviceMemory>	m_memory;
};

void ImageCopyToImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate destination image for image to image copy." << TestLog::EndMessage;
}

void ImageCopyToImage::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vk::VkQueue				queue			= context.getContext().getQueue();
	const vk::VkCommandPool			commandPool		= context.getContext().getCommandPool();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();

	m_imageWidth		= context.getImageWidth();
	m_imageHeight		= context.getImageHeight();
	m_imageMemorySize	= context.getImageMemorySize();

	{
		const vk::VkImageCreateInfo	createInfo =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			DE_NULL,

			0,
			vk::VK_IMAGE_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{
				(deUint32)m_imageWidth,
				(deUint32)m_imageHeight,
				1u,
			},
			1, 1, // mipLevels, arrayLayers
			vk::VK_SAMPLE_COUNT_1_BIT,

			vk::VK_IMAGE_TILING_OPTIMAL,
			vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT|vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			vk::VK_SHARING_MODE_EXCLUSIVE,

			(deUint32)queueFamilies.size(),
			&queueFamilies[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		m_dstImage = vk::createImage(vkd, device, &createInfo);
	}

	m_memory = bindImageMemory(vki, vkd, physicalDevice, device, *m_dstImage, 0);

	{
		const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const vk::VkImageMemoryBarrier			barrier			=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			0,
			vk::VK_ACCESS_TRANSFER_WRITE_BIT,

			vk::VK_IMAGE_LAYOUT_UNDEFINED,
			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_dstImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};

		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &barrier);

		VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
		queueRun(vkd, queue, *commandBuffer);
	}
}

void ImageCopyToImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Copy image to another image" << TestLog::EndMessage;
}

void ImageCopyToImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkImageCopy		region			=
	{
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{ 0, 0, 0 },

		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{ 0, 0, 0 },
		{
			(deUint32)m_imageWidth,
			(deUint32)m_imageHeight,
			1u
		}
	};

	vkd.cmdCopyImage(commandBuffer, context.getImage(), m_imageLayout, *m_dstImage, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void ImageCopyToImage::verify (VerifyContext& context, size_t commandIndex)
{
	tcu::ResultCollector&					resultCollector	(context.getResultCollector());
	const vk::InstanceInterface&			vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&				vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice				physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice						device			= context.getContext().getDevice();
	const vk::VkQueue						queue			= context.getContext().getQueue();
	const vk::VkCommandPool					commandPool		= context.getContext().getCommandPool();
	const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const vector<deUint32>&					queueFamilies	= context.getContext().getQueueFamilies();
	const vk::Unique<vk::VkBuffer>			dstBuffer		(createBuffer(vkd, device, 4 * m_imageWidth * m_imageHeight, vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies));
	const vk::Unique<vk::VkDeviceMemory>	memory			(bindBufferMemory(vki, vkd, physicalDevice, device, *dstBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
	{
		const vk::VkImageMemoryBarrier		imageBarrier	=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			vk::VK_ACCESS_TRANSFER_WRITE_BIT,
			vk::VK_ACCESS_TRANSFER_READ_BIT,

			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_dstImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};
		const vk::VkBufferMemoryBarrier bufferBarrier =
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,

			vk::VK_ACCESS_TRANSFER_WRITE_BIT,
			vk::VK_ACCESS_HOST_READ_BIT,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			*dstBuffer,
			0,
			VK_WHOLE_SIZE
		};
		const vk::VkBufferImageCopy	region =
		{
			0,
			0, 0,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// mipLevel
				0,	// arrayLayer
				1	// layerCount
			},
			{ 0, 0, 0 },
			{
				(deUint32)m_imageWidth,
				(deUint32)m_imageHeight,
				1u
			}
		};

		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &imageBarrier);
		vkd.cmdCopyImageToBuffer(*commandBuffer, *m_dstImage, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *dstBuffer, 1, &region);
		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_HOST_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 1, &bufferBarrier, 0, (const vk::VkImageMemoryBarrier*)DE_NULL);
	}

	VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
	queueRun(vkd, queue, *commandBuffer);

	{
		void* const	ptr		= mapMemory(vkd, device, *memory, 4 * m_imageWidth * m_imageHeight);

		vk::invalidateMappedMemoryRange(vkd, device, *memory, 0,  4 * m_imageWidth * m_imageHeight);

		{
			const deUint8* const			data		= (const deUint8*)ptr;
			const ConstPixelBufferAccess	resAccess	(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_imageWidth, m_imageHeight, 1, data);
			const ConstPixelBufferAccess&	refAccess	(context.getReferenceImage().getAccess());

			if (!tcu::intThresholdCompare(context.getLog(), (de::toString(commandIndex) + ":" + getName()).c_str(), (de::toString(commandIndex) + ":" + getName()).c_str(), refAccess, resAccess, UVec4(0), tcu::COMPARE_LOG_ON_ERROR))
				resultCollector.fail(de::toString(commandIndex) + ":" + getName() + " Image comparison failed");
		}

		vkd.unmapMemory(device, *memory);
	}
}

enum BlitScale
{
	BLIT_SCALE_20,
	BLIT_SCALE_10,
};

class ImageBlitFromImage : public CmdCommand
{
public:
									ImageBlitFromImage	(deUint32 seed, BlitScale scale, vk::VkImageLayout imageLayout) : m_seed(seed), m_scale(scale), m_imageLayout(imageLayout) {}
									~ImageBlitFromImage	(void) {}
	const char*						getName				(void) const { return "ImageBlitFromImage"; }

	void							logPrepare			(TestLog& log, size_t commandIndex) const;
	void							prepare				(PrepareContext& context);
	void							logSubmit			(TestLog& log, size_t commandIndex) const;
	void							submit				(SubmitContext& context);
	void							verify				(VerifyContext& context, size_t commandIndex);

private:
	const deUint32					m_seed;
	const BlitScale					m_scale;
	const vk::VkImageLayout			m_imageLayout;
	deInt32							m_imageWidth;
	deInt32							m_imageHeight;
	vk::VkDeviceSize				m_imageMemorySize;
	deInt32							m_srcImageWidth;
	deInt32							m_srcImageHeight;
	vk::Move<vk::VkImage>			m_srcImage;
	vk::Move<vk::VkDeviceMemory>	m_memory;
};

void ImageBlitFromImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate source image for image to image blit." << TestLog::EndMessage;
}

void ImageBlitFromImage::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vk::VkQueue				queue			= context.getContext().getQueue();
	const vk::VkCommandPool			commandPool		= context.getContext().getCommandPool();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();

	m_imageWidth		= context.getImageWidth();
	m_imageHeight		= context.getImageHeight();
	m_imageMemorySize	= context.getImageMemorySize();

	if (m_scale == BLIT_SCALE_10)
	{
		m_srcImageWidth			= m_imageWidth;
		m_srcImageHeight		= m_imageHeight;
	}
	else if (m_scale == BLIT_SCALE_20)
	{
		m_srcImageWidth			= m_imageWidth / 2;
		m_srcImageHeight		= m_imageHeight / 2;
	}
	else
		DE_FATAL("Unsupported scale");

	{
		const vk::VkImageCreateInfo	createInfo =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			DE_NULL,

			0,
			vk::VK_IMAGE_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{
				(deUint32)m_srcImageWidth,
				(deUint32)m_srcImageHeight,
				1u,
			},
			1, 1, // mipLevels, arrayLayers
			vk::VK_SAMPLE_COUNT_1_BIT,

			vk::VK_IMAGE_TILING_OPTIMAL,
			vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT|vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			vk::VK_SHARING_MODE_EXCLUSIVE,

			(deUint32)queueFamilies.size(),
			&queueFamilies[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		m_srcImage = vk::createImage(vkd, device, &createInfo);
	}

	m_memory = bindImageMemory(vki, vkd, physicalDevice, device, *m_srcImage, 0);

	{
		const vk::Unique<vk::VkBuffer>			srcBuffer		(createBuffer(vkd, device, 4 * m_srcImageWidth * m_srcImageHeight, vk::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies));
		const vk::Unique<vk::VkDeviceMemory>	memory			(bindBufferMemory(vki, vkd, physicalDevice, device, *srcBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
		const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const vk::VkImageMemoryBarrier			preImageBarrier	=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			0,
			vk::VK_ACCESS_TRANSFER_WRITE_BIT,

			vk::VK_IMAGE_LAYOUT_UNDEFINED,
			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_srcImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};
		const vk::VkImageMemoryBarrier			postImageBarrier =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			vk::VK_ACCESS_TRANSFER_WRITE_BIT,
			0,

			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_srcImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};
		const vk::VkBufferImageCopy				region				=
		{
			0,
			0, 0,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// mipLevel
				0,	// arrayLayer
				1	// layerCount
			},
			{ 0, 0, 0 },
			{
				(deUint32)m_srcImageWidth,
				(deUint32)m_srcImageHeight,
				1u
			}
		};

		{
			void* const	ptr	= mapMemory(vkd, device, *memory, 4 * m_srcImageWidth * m_srcImageHeight);
			de::Random	rng	(m_seed);

			{
				deUint8* const	data = (deUint8*)ptr;

				for (size_t ndx = 0; ndx < (size_t)(4 * m_srcImageWidth * m_srcImageHeight); ndx++)
					data[ndx] = rng.getUint8();
			}

			vk::flushMappedMemoryRange(vkd, device, *memory, 0, 4 * m_srcImageWidth * m_srcImageHeight);
			vkd.unmapMemory(device, *memory);
		}

		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &preImageBarrier);
		vkd.cmdCopyBufferToImage(*commandBuffer, *srcBuffer, *m_srcImage, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &postImageBarrier);

		VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
		queueRun(vkd, queue, *commandBuffer);
	}
}

void ImageBlitFromImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Blit from another image" << (m_scale == BLIT_SCALE_20 ? " scale 2x" : "")  << TestLog::EndMessage;
}

void ImageBlitFromImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkImageBlit		region			=
	{
		// Src
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{
			{ 0, 0, 0 },
			{
				m_srcImageWidth,
				m_srcImageHeight,
				1
			},
		},

		// Dst
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{
			{ 0, 0, 0 },
			{
				m_imageWidth,
				m_imageHeight,
				1u
			}
		}
	};
	vkd.cmdBlitImage(commandBuffer, *m_srcImage, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, context.getImage(), m_imageLayout, 1, &region, vk::VK_FILTER_NEAREST);
}

void ImageBlitFromImage::verify (VerifyContext& context, size_t)
{
	ReferenceMemory&	reference	(context.getReference());
	de::Random			rng			(m_seed);

	reference.setUndefined(0, (size_t)m_imageMemorySize);

	{
		const PixelBufferAccess&	refAccess	(context.getReferenceImage().getAccess());

		if (m_scale == BLIT_SCALE_10)
		{
			for (deInt32 y = 0; y < m_imageHeight; y++)
			for (deInt32 x = 0; x < m_imageWidth; x++)
			{
				const deUint8 r8 = rng.getUint8();
				const deUint8 g8 = rng.getUint8();
				const deUint8 b8 = rng.getUint8();
				const deUint8 a8 = rng.getUint8();

				refAccess.setPixel(UVec4(r8, g8, b8, a8), x, y);
			}
		}
		else if (m_scale == BLIT_SCALE_20)
		{
			tcu::TextureLevel	source	(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_srcImageWidth, m_srcImageHeight);
			const float			xscale	= ((float)m_srcImageWidth)  / (float)m_imageWidth;
			const float			yscale	= ((float)m_srcImageHeight) / (float)m_imageHeight;

			for (deInt32 y = 0; y < m_srcImageHeight; y++)
			for (deInt32 x = 0; x < m_srcImageWidth; x++)
			{
				const deUint8 r8 = rng.getUint8();
				const deUint8 g8 = rng.getUint8();
				const deUint8 b8 = rng.getUint8();
				const deUint8 a8 = rng.getUint8();

				source.getAccess().setPixel(UVec4(r8, g8, b8, a8), x, y);
			}

			for (deInt32 y = 0; y < m_imageHeight; y++)
			for (deInt32 x = 0; x < m_imageWidth; x++)
				refAccess.setPixel(source.getAccess().getPixelUint(int((float(x) + 0.5f) * xscale), int((float(y) + 0.5f) * yscale)), x, y);
		}
		else
			DE_FATAL("Unsupported scale");
	}
}

class ImageBlitToImage : public CmdCommand
{
public:
									ImageBlitToImage	(BlitScale scale, vk::VkImageLayout imageLayout) : m_scale(scale), m_imageLayout(imageLayout) {}
									~ImageBlitToImage	(void) {}
	const char*						getName				(void) const { return "ImageBlitToImage"; }

	void							logPrepare			(TestLog& log, size_t commandIndex) const;
	void							prepare				(PrepareContext& context);
	void							logSubmit			(TestLog& log, size_t commandIndex) const;
	void							submit				(SubmitContext& context);
	void							verify				(VerifyContext& context, size_t commandIndex);

private:
	const BlitScale					m_scale;
	const vk::VkImageLayout			m_imageLayout;
	deInt32							m_imageWidth;
	deInt32							m_imageHeight;
	vk::VkDeviceSize				m_imageMemorySize;
	deInt32							m_dstImageWidth;
	deInt32							m_dstImageHeight;
	vk::Move<vk::VkImage>			m_dstImage;
	vk::Move<vk::VkDeviceMemory>	m_memory;
};

void ImageBlitToImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Allocate destination image for image to image blit." << TestLog::EndMessage;
}

void ImageBlitToImage::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&	vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice		physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice				device			= context.getContext().getDevice();
	const vk::VkQueue				queue			= context.getContext().getQueue();
	const vk::VkCommandPool			commandPool		= context.getContext().getCommandPool();
	const vector<deUint32>&			queueFamilies	= context.getContext().getQueueFamilies();

	m_imageWidth		= context.getImageWidth();
	m_imageHeight		= context.getImageHeight();
	m_imageMemorySize	= context.getImageMemorySize();

	if (m_scale == BLIT_SCALE_10)
	{
		m_dstImageWidth		= context.getImageWidth();
		m_dstImageHeight	= context.getImageHeight();
	}
	else if (m_scale == BLIT_SCALE_20)
	{
		m_dstImageWidth		= context.getImageWidth() * 2;
		m_dstImageHeight	= context.getImageHeight() * 2;
	}
	else
		DE_FATAL("Unsupportd blit scale");

	{
		const vk::VkImageCreateInfo	createInfo =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			DE_NULL,

			0,
			vk::VK_IMAGE_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{
				(deUint32)m_dstImageWidth,
				(deUint32)m_dstImageHeight,
				1u,
			},
			1, 1, // mipLevels, arrayLayers
			vk::VK_SAMPLE_COUNT_1_BIT,

			vk::VK_IMAGE_TILING_OPTIMAL,
			vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT|vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			vk::VK_SHARING_MODE_EXCLUSIVE,

			(deUint32)queueFamilies.size(),
			&queueFamilies[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		m_dstImage = vk::createImage(vkd, device, &createInfo);
	}

	m_memory = bindImageMemory(vki, vkd, physicalDevice, device, *m_dstImage, 0);

	{
		const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const vk::VkImageMemoryBarrier			barrier			=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			0,
			vk::VK_ACCESS_TRANSFER_WRITE_BIT,

			vk::VK_IMAGE_LAYOUT_UNDEFINED,
			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_dstImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};

		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &barrier);

		VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
		queueRun(vkd, queue, *commandBuffer);
	}
}

void ImageBlitToImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Blit image to another image" << (m_scale == BLIT_SCALE_20 ? " scale 2x" : "")  << TestLog::EndMessage;
}

void ImageBlitToImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkImageBlit		region			=
	{
		// Src
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{
			{ 0, 0, 0 },
			{
				m_imageWidth,
				m_imageHeight,
				1
			},
		},

		// Dst
		{
			vk::VK_IMAGE_ASPECT_COLOR_BIT,
			0,	// mipLevel
			0,	// arrayLayer
			1	// layerCount
		},
		{
			{ 0, 0, 0 },
			{
				m_dstImageWidth,
				m_dstImageHeight,
				1u
			}
		}
	};
	vkd.cmdBlitImage(commandBuffer, context.getImage(), m_imageLayout, *m_dstImage, vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, vk::VK_FILTER_NEAREST);
}

void ImageBlitToImage::verify (VerifyContext& context, size_t commandIndex)
{
	tcu::ResultCollector&					resultCollector	(context.getResultCollector());
	const vk::InstanceInterface&			vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&				vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice				physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice						device			= context.getContext().getDevice();
	const vk::VkQueue						queue			= context.getContext().getQueue();
	const vk::VkCommandPool					commandPool		= context.getContext().getCommandPool();
	const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const vector<deUint32>&					queueFamilies	= context.getContext().getQueueFamilies();
	const vk::Unique<vk::VkBuffer>			dstBuffer		(createBuffer(vkd, device, 4 * m_dstImageWidth * m_dstImageHeight, vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies));
	const vk::Unique<vk::VkDeviceMemory>	memory			(bindBufferMemory(vki, vkd, physicalDevice, device, *dstBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
	{
		const vk::VkImageMemoryBarrier		imageBarrier	=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			vk::VK_ACCESS_TRANSFER_WRITE_BIT,
			vk::VK_ACCESS_TRANSFER_READ_BIT,

			vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_dstImage,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// Mip level
				1,	// Mip level count
				0,	// Layer
				1	// Layer count
			}
		};
		const vk::VkBufferMemoryBarrier bufferBarrier =
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,

			vk::VK_ACCESS_TRANSFER_WRITE_BIT,
			vk::VK_ACCESS_HOST_READ_BIT,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			*dstBuffer,
			0,
			VK_WHOLE_SIZE
		};
		const vk::VkBufferImageCopy	region =
		{
			0,
			0, 0,
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0,	// mipLevel
				0,	// arrayLayer
				1	// layerCount
			},
			{ 0, 0, 0 },
			{
				(deUint32)m_dstImageWidth,
				(deUint32)m_dstImageHeight,
				1
			}
		};

		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &imageBarrier);
		vkd.cmdCopyImageToBuffer(*commandBuffer, *m_dstImage, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *dstBuffer, 1, &region);
		vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_HOST_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 1, &bufferBarrier, 0, (const vk::VkImageMemoryBarrier*)DE_NULL);
	}

	VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
	queueRun(vkd, queue, *commandBuffer);

	{
		void* const	ptr		= mapMemory(vkd, device, *memory, 4 * m_dstImageWidth * m_dstImageHeight);

		vk::invalidateMappedMemoryRange(vkd, device, *memory, 0,  4 * m_dstImageWidth * m_dstImageHeight);

		if (m_scale == BLIT_SCALE_10)
		{
			const deUint8* const			data		= (const deUint8*)ptr;
			const ConstPixelBufferAccess	resAccess	(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_dstImageWidth, m_dstImageHeight, 1, data);
			const ConstPixelBufferAccess&	refAccess	(context.getReferenceImage().getAccess());

			if (!tcu::intThresholdCompare(context.getLog(), (de::toString(commandIndex) + ":" + getName()).c_str(), (de::toString(commandIndex) + ":" + getName()).c_str(), refAccess, resAccess, UVec4(0), tcu::COMPARE_LOG_ON_ERROR))
				resultCollector.fail(de::toString(commandIndex) + ":" + getName() + " Image comparison failed");
		}
		else if (m_scale == BLIT_SCALE_20)
		{
			const deUint8* const			data		= (const deUint8*)ptr;
			const ConstPixelBufferAccess	resAccess	(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_dstImageWidth, m_dstImageHeight, 1, data);
			tcu::TextureLevel				reference	(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_dstImageWidth, m_dstImageHeight, 1);

			{
				const ConstPixelBufferAccess&	refAccess	(context.getReferenceImage().getAccess());

				for (deInt32 y = 0; y < m_dstImageHeight; y++)
				for (deInt32 x = 0; x < m_dstImageWidth; x++)
				{
					reference.getAccess().setPixel(refAccess.getPixel(x/2, y/2), x, y);
				}
			}

			if (!tcu::intThresholdCompare(context.getLog(), (de::toString(commandIndex) + ":" + getName()).c_str(), (de::toString(commandIndex) + ":" + getName()).c_str(), reference.getAccess(), resAccess, UVec4(0), tcu::COMPARE_LOG_ON_ERROR))
				resultCollector.fail(de::toString(commandIndex) + ":" + getName() + " Image comparison failed");
		}
		else
			DE_FATAL("Unknown scale");

		vkd.unmapMemory(device, *memory);
	}
}

class PrepareRenderPassContext
{
public:
								PrepareRenderPassContext	(PrepareContext&	context,
															 vk::VkRenderPass	renderPass,
															 vk::VkFramebuffer	framebuffer,
															 deInt32			targetWidth,
															 deInt32			targetHeight)
		: m_context			(context)
		, m_renderPass		(renderPass)
		, m_framebuffer		(framebuffer)
		, m_targetWidth		(targetWidth)
		, m_targetHeight	(targetHeight)
	{
	}

	const Memory&									getMemory					(void) const { return m_context.getMemory(); }
	const Context&									getContext					(void) const { return m_context.getContext(); }
	const vk::ProgramCollection<vk::ProgramBinary>&	getBinaryCollection			(void) const { return m_context.getBinaryCollection(); }

	vk::VkBuffer				getBuffer					(void) const { return m_context.getBuffer(); }
	vk::VkDeviceSize			getBufferSize				(void) const { return m_context.getBufferSize(); }

	vk::VkImage					getImage					(void) const { return m_context.getImage(); }
	deInt32						getImageWidth				(void) const { return m_context.getImageWidth(); }
	deInt32						getImageHeight				(void) const { return m_context.getImageHeight(); }
	vk::VkImageLayout			getImageLayout				(void) const { return m_context.getImageLayout(); }

	deInt32						getTargetWidth				(void) const { return m_targetWidth; }
	deInt32						getTargetHeight				(void) const { return m_targetHeight; }

	vk::VkRenderPass			getRenderPass				(void) const { return m_renderPass; }

private:
	PrepareContext&				m_context;
	const vk::VkRenderPass		m_renderPass;
	const vk::VkFramebuffer		m_framebuffer;
	const deInt32				m_targetWidth;
	const deInt32				m_targetHeight;
};

class VerifyRenderPassContext
{
public:
							VerifyRenderPassContext		(VerifyContext&			context,
														 deInt32				targetWidth,
														 deInt32				targetHeight)
		: m_context			(context)
		, m_referenceTarget	(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), targetWidth, targetHeight)
	{
	}

	const Context&			getContext			(void) const { return m_context.getContext(); }
	TestLog&				getLog				(void) const { return m_context.getLog(); }
	tcu::ResultCollector&	getResultCollector	(void) const { return m_context.getResultCollector(); }

	TextureLevel&			getReferenceTarget	(void) { return m_referenceTarget; }

	ReferenceMemory&		getReference		(void) { return m_context.getReference(); }
	TextureLevel&			getReferenceImage	(void) { return m_context.getReferenceImage();}

private:
	VerifyContext&	m_context;
	TextureLevel	m_referenceTarget;
};

class RenderPassCommand
{
public:
	virtual				~RenderPassCommand	(void) {}
	virtual const char*	getName				(void) const = 0;

	// Log things that are done during prepare
	virtual void		logPrepare			(TestLog&, size_t) const {}
	// Log submitted calls etc.
	virtual void		logSubmit			(TestLog&, size_t) const {}

	// Allocate vulkan resources and prepare for submit.
	virtual void		prepare				(PrepareRenderPassContext&) {}

	// Submit commands to command buffer.
	virtual void		submit				(SubmitContext&) {}

	// Verify results
	virtual void		verify				(VerifyRenderPassContext&, size_t) {}
};

class SubmitRenderPass : public CmdCommand
{
public:
				SubmitRenderPass	(const vector<RenderPassCommand*>& commands);
				~SubmitRenderPass	(void);
	const char*	getName				(void) const { return "SubmitRenderPass"; }

	void		logPrepare			(TestLog&, size_t) const;
	void		logSubmit			(TestLog&, size_t) const;

	void		prepare				(PrepareContext&);
	void		submit				(SubmitContext&);

	void		verify				(VerifyContext&, size_t);

private:
	const deInt32					m_targetWidth;
	const deInt32					m_targetHeight;
	vk::Move<vk::VkRenderPass>		m_renderPass;
	vk::Move<vk::VkDeviceMemory>	m_colorTargetMemory;
	de::MovePtr<vk::Allocation>		m_colorTargetMemory2;
	vk::Move<vk::VkImage>			m_colorTarget;
	vk::Move<vk::VkImageView>		m_colorTargetView;
	vk::Move<vk::VkFramebuffer>		m_framebuffer;
	vector<RenderPassCommand*>		m_commands;
};

SubmitRenderPass::SubmitRenderPass (const vector<RenderPassCommand*>& commands)
	: m_targetWidth		(256)
	, m_targetHeight	(256)
	, m_commands		(commands)
{
}

SubmitRenderPass::~SubmitRenderPass()
{
	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
		delete m_commands[cmdNdx];
}

void SubmitRenderPass::logPrepare (TestLog& log, size_t commandIndex) const
{
	const string				sectionName	(de::toString(commandIndex) + ":" + getName());
	const tcu::ScopedLogSection	section		(log, sectionName, sectionName);

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
	{
		RenderPassCommand& command = *m_commands[cmdNdx];
		command.logPrepare(log, cmdNdx);
	}
}

void SubmitRenderPass::logSubmit (TestLog& log, size_t commandIndex) const
{
	const string				sectionName	(de::toString(commandIndex) + ":" + getName());
	const tcu::ScopedLogSection	section		(log, sectionName, sectionName);

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
	{
		RenderPassCommand& command = *m_commands[cmdNdx];
		command.logSubmit(log, cmdNdx);
	}
}

void SubmitRenderPass::prepare (PrepareContext& context)
{
	const vk::InstanceInterface&			vki				= context.getContext().getInstanceInterface();
	const vk::DeviceInterface&				vkd				= context.getContext().getDeviceInterface();
	const vk::VkPhysicalDevice				physicalDevice	= context.getContext().getPhysicalDevice();
	const vk::VkDevice						device			= context.getContext().getDevice();
	const vector<deUint32>&					queueFamilies	= context.getContext().getQueueFamilies();

	const vk::VkAttachmentReference	colorAttachments[]	=
	{
		{ 0, vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
	};
	const vk::VkSubpassDescription	subpass				=
	{
		0u,
		vk::VK_PIPELINE_BIND_POINT_GRAPHICS,

		0u,
		DE_NULL,

		DE_LENGTH_OF_ARRAY(colorAttachments),
		colorAttachments,
		DE_NULL,
		DE_NULL,
		0u,
		DE_NULL
	};
	const vk::VkAttachmentDescription attachment =
	{
		0u,
		vk::VK_FORMAT_R8G8B8A8_UNORM,
		vk::VK_SAMPLE_COUNT_1_BIT,

		vk::VK_ATTACHMENT_LOAD_OP_CLEAR,
		vk::VK_ATTACHMENT_STORE_OP_STORE,

		vk::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		vk::VK_ATTACHMENT_STORE_OP_DONT_CARE,

		vk::VK_IMAGE_LAYOUT_UNDEFINED,
		vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	};
	{
		const vk::VkImageCreateInfo createInfo =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			DE_NULL,
			0u,

			vk::VK_IMAGE_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{ (deUint32)m_targetWidth, (deUint32)m_targetHeight, 1u },
			1u,
			1u,
			vk::VK_SAMPLE_COUNT_1_BIT,
			vk::VK_IMAGE_TILING_OPTIMAL,
			vk::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			vk::VK_SHARING_MODE_EXCLUSIVE,
			(deUint32)queueFamilies.size(),
			&queueFamilies[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		m_colorTarget = vk::createImage(vkd, device, &createInfo);
	}

	m_colorTargetMemory = bindImageMemory(vki, vkd, physicalDevice, device, *m_colorTarget, 0);

	{
		const vk::VkImageViewCreateInfo createInfo =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			DE_NULL,

			0u,
			*m_colorTarget,
			vk::VK_IMAGE_VIEW_TYPE_2D,
			vk::VK_FORMAT_R8G8B8A8_UNORM,
			{
				vk::VK_COMPONENT_SWIZZLE_R,
				vk::VK_COMPONENT_SWIZZLE_G,
				vk::VK_COMPONENT_SWIZZLE_B,
				vk::VK_COMPONENT_SWIZZLE_A
			},
			{
				vk::VK_IMAGE_ASPECT_COLOR_BIT,
				0u,
				1u,
				0u,
				1u
			}
		};

		m_colorTargetView = vk::createImageView(vkd, device, &createInfo);
	}
	{
		const vk::VkRenderPassCreateInfo createInfo =
		{
			vk::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			DE_NULL,
			0u,

			1u,
			&attachment,

			1u,
			&subpass,

			0,
			DE_NULL
		};

		m_renderPass = vk::createRenderPass(vkd, device, &createInfo);
	}

	{
		const vk::VkImageView				imageViews[]	=
		{
			*m_colorTargetView
		};
		const vk::VkFramebufferCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			DE_NULL,
			0u,

			*m_renderPass,
			DE_LENGTH_OF_ARRAY(imageViews),
			imageViews,
			(deUint32)m_targetWidth,
			(deUint32)m_targetHeight,
			1u
		};

		m_framebuffer = vk::createFramebuffer(vkd, device, &createInfo);
	}

	{
		PrepareRenderPassContext renderpassContext (context, *m_renderPass, *m_framebuffer, m_targetWidth, m_targetHeight);

		for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
		{
			RenderPassCommand& command = *m_commands[cmdNdx];
			command.prepare(renderpassContext);
		}
	}
}

void SubmitRenderPass::submit (SubmitContext& context)
{
	const vk::DeviceInterface&		vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer		commandBuffer	= context.getCommandBuffer();
	const vk::VkClearValue			clearValue		= vk::makeClearValueColorF32(0.0f, 0.0f, 0.0f, 1.0f);

	const vk::VkRenderPassBeginInfo	beginInfo		=
	{
		vk::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		DE_NULL,

		*m_renderPass,
		*m_framebuffer,

		{ { 0, 0 },  { (deUint32)m_targetWidth, (deUint32)m_targetHeight } },
		1u,
		&clearValue
	};

	vkd.cmdBeginRenderPass(commandBuffer, &beginInfo, vk::VK_SUBPASS_CONTENTS_INLINE);

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
	{
		RenderPassCommand& command = *m_commands[cmdNdx];

		command.submit(context);
	}

	vkd.cmdEndRenderPass(commandBuffer);
}

void SubmitRenderPass::verify (VerifyContext& context, size_t commandIndex)
{
	TestLog&					log				(context.getLog());
	tcu::ResultCollector&		resultCollector	(context.getResultCollector());
	const string				sectionName		(de::toString(commandIndex) + ":" + getName());
	const tcu::ScopedLogSection	section			(log, sectionName, sectionName);
	VerifyRenderPassContext		verifyContext	(context, m_targetWidth, m_targetHeight);

	tcu::clear(verifyContext.getReferenceTarget().getAccess(), Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
	{
		RenderPassCommand& command = *m_commands[cmdNdx];
		command.verify(verifyContext, cmdNdx);
	}

	{
		const vk::InstanceInterface&			vki				= context.getContext().getInstanceInterface();
		const vk::DeviceInterface&				vkd				= context.getContext().getDeviceInterface();
		const vk::VkPhysicalDevice				physicalDevice	= context.getContext().getPhysicalDevice();
		const vk::VkDevice						device			= context.getContext().getDevice();
		const vk::VkQueue						queue			= context.getContext().getQueue();
		const vk::VkCommandPool					commandPool		= context.getContext().getCommandPool();
		const vk::Unique<vk::VkCommandBuffer>	commandBuffer	(createBeginCommandBuffer(vkd, device, commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const vector<deUint32>&					queueFamilies	= context.getContext().getQueueFamilies();
		const vk::Unique<vk::VkBuffer>			dstBuffer		(createBuffer(vkd, device, 4 * m_targetWidth * m_targetHeight, vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT, vk::VK_SHARING_MODE_EXCLUSIVE, queueFamilies));
		const vk::Unique<vk::VkDeviceMemory>	memory			(bindBufferMemory(vki, vkd, physicalDevice, device, *dstBuffer, vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
		{
			const vk::VkImageMemoryBarrier		imageBarrier	=
			{
				vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				DE_NULL,

				vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				vk::VK_ACCESS_TRANSFER_READ_BIT,

				vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,

				*m_colorTarget,
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,
					0,	// Mip level
					1,	// Mip level count
					0,	// Layer
					1	// Layer count
				}
			};
			const vk::VkBufferMemoryBarrier bufferBarrier =
			{
				vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				DE_NULL,

				vk::VK_ACCESS_TRANSFER_WRITE_BIT,
				vk::VK_ACCESS_HOST_READ_BIT,

				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				*dstBuffer,
				0,
				VK_WHOLE_SIZE
			};
			const vk::VkBufferImageCopy	region =
			{
				0,
				0, 0,
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,
					0,	// mipLevel
					0,	// arrayLayer
					1	// layerCount
				},
				{ 0, 0, 0 },
				{
					(deUint32)m_targetWidth,
					(deUint32)m_targetHeight,
					1u
				}
			};

			vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 0, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1, &imageBarrier);
			vkd.cmdCopyImageToBuffer(*commandBuffer, *m_colorTarget, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *dstBuffer, 1, &region);
			vkd.cmdPipelineBarrier(*commandBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_PIPELINE_STAGE_HOST_BIT, (vk::VkDependencyFlags)0, 0, (const vk::VkMemoryBarrier*)DE_NULL, 1, &bufferBarrier, 0, (const vk::VkImageMemoryBarrier*)DE_NULL);
		}

		VK_CHECK(vkd.endCommandBuffer(*commandBuffer));
		queueRun(vkd, queue, *commandBuffer);

		{
			void* const	ptr		= mapMemory(vkd, device, *memory, 4 * m_targetWidth * m_targetHeight);

			vk::invalidateMappedMemoryRange(vkd, device, *memory, 0,  4 * m_targetWidth * m_targetHeight);

			{
				const deUint8* const			data		= (const deUint8*)ptr;
				const ConstPixelBufferAccess	resAccess	(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_targetWidth, m_targetHeight, 1, data);
				const ConstPixelBufferAccess&	refAccess	(verifyContext.getReferenceTarget().getAccess());

				if (!tcu::intThresholdCompare(context.getLog(), (de::toString(commandIndex) + ":" + getName()).c_str(), (de::toString(commandIndex) + ":" + getName()).c_str(), refAccess, resAccess, UVec4(0), tcu::COMPARE_LOG_ON_ERROR))
					resultCollector.fail(de::toString(commandIndex) + ":" + getName() + " Image comparison failed");
			}

			vkd.unmapMemory(device, *memory);
		}
	}
}

struct PipelineResources
{
	vk::Move<vk::VkPipeline>			pipeline;
	vk::Move<vk::VkDescriptorSetLayout>	descriptorSetLayout;
	vk::Move<vk::VkPipelineLayout>		pipelineLayout;
};

void createPipelineWithResources (const vk::DeviceInterface&							vkd,
								  const vk::VkDevice									device,
								  const vk::VkRenderPass								renderPass,
								  const deUint32										subpass,
								  const vk::VkShaderModule&								vertexShaderModule,
								  const vk::VkShaderModule&								fragmentShaderModule,
								  const deUint32										viewPortWidth,
								  const deUint32										viewPortHeight,
								  const vector<vk::VkVertexInputBindingDescription>&	vertexBindingDescriptions,
								  const vector<vk::VkVertexInputAttributeDescription>&	vertexAttributeDescriptions,
								  const vector<vk::VkDescriptorSetLayoutBinding>&		bindings,
								  const vk::VkPrimitiveTopology							topology,
								  deUint32												pushConstantRangeCount,
								  const vk::VkPushConstantRange*						pushConstantRanges,
								  PipelineResources&									resources)
{
	if (!bindings.empty())
	{
		const vk::VkDescriptorSetLayoutCreateInfo createInfo =
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			DE_NULL,

			0u,
			(deUint32)bindings.size(),
			bindings.empty() ? DE_NULL : &bindings[0]
		};

		resources.descriptorSetLayout = vk::createDescriptorSetLayout(vkd, device, &createInfo);
	}

	{
		const vk::VkDescriptorSetLayout			descriptorSetLayout_	= *resources.descriptorSetLayout;
		const vk::VkPipelineLayoutCreateInfo	createInfo				=
		{
			vk::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			DE_NULL,
			0,

			resources.descriptorSetLayout ? 1u : 0u,
			resources.descriptorSetLayout ? &descriptorSetLayout_ : DE_NULL,

			pushConstantRangeCount,
			pushConstantRanges
		};

		resources.pipelineLayout = vk::createPipelineLayout(vkd, device, &createInfo);
	}

	{
		const vk::VkPipelineShaderStageCreateInfo			shaderStages[]					=
		{
			{
				vk::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				DE_NULL,
				0,
				vk::VK_SHADER_STAGE_VERTEX_BIT,
				vertexShaderModule,
				"main",
				DE_NULL
			},
			{
				vk::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				DE_NULL,
				0,
				vk::VK_SHADER_STAGE_FRAGMENT_BIT,
				fragmentShaderModule,
				"main",
				DE_NULL
			}
		};
		const vk::VkPipelineDepthStencilStateCreateInfo		depthStencilState				=
		{
			vk::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			DE_NULL,
			0u,
			DE_FALSE,
			DE_FALSE,
			vk::VK_COMPARE_OP_ALWAYS,
			DE_FALSE,
			DE_FALSE,
			{
				vk::VK_STENCIL_OP_KEEP,
				vk::VK_STENCIL_OP_KEEP,
				vk::VK_STENCIL_OP_KEEP,
				vk::VK_COMPARE_OP_ALWAYS,
				0u,
				0u,
				0u,
			},
			{
				vk::VK_STENCIL_OP_KEEP,
				vk::VK_STENCIL_OP_KEEP,
				vk::VK_STENCIL_OP_KEEP,
				vk::VK_COMPARE_OP_ALWAYS,
				0u,
				0u,
				0u,
			},
			-1.0f,
			+1.0f
		};
		const vk::VkPipelineVertexInputStateCreateInfo		vertexInputState				=
		{
			vk::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			DE_NULL,
			0u,

			(deUint32)vertexBindingDescriptions.size(),
			vertexBindingDescriptions.empty() ? DE_NULL : &vertexBindingDescriptions[0],

			(deUint32)vertexAttributeDescriptions.size(),
			vertexAttributeDescriptions.empty() ? DE_NULL : &vertexAttributeDescriptions[0]
		};
		const vk::VkPipelineInputAssemblyStateCreateInfo	inputAssemblyState				=
		{
			vk::VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			DE_NULL,
			0,
			topology,
			VK_FALSE
		};
		const vk::VkViewport								viewports[]						=
		{
			{ 0.0f, 0.0f, (float)viewPortWidth, (float)viewPortHeight, 0.0f, 1.0f }
		};
		const vk::VkRect2D									scissors[]						=
		{
			{ { 0, 0 }, { (deUint32)viewPortWidth, (deUint32)viewPortHeight } }
		};
		const vk::VkPipelineViewportStateCreateInfo			viewportState					=
		{
			vk::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			DE_NULL,
			0,
			DE_LENGTH_OF_ARRAY(viewports),
			viewports,
			DE_LENGTH_OF_ARRAY(scissors),
			scissors
		};
		const vk::VkPipelineRasterizationStateCreateInfo	rasterState						=
		{
			vk::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			DE_NULL,
			0,

			VK_TRUE,
			VK_FALSE,
			vk::VK_POLYGON_MODE_FILL,
			vk::VK_CULL_MODE_NONE,
			vk::VK_FRONT_FACE_COUNTER_CLOCKWISE,
			VK_FALSE,
			0.0f,
			0.0f,
			0.0f,
			1.0f
		};
		const vk::VkSampleMask								sampleMask						= ~0u;
		const vk::VkPipelineMultisampleStateCreateInfo		multisampleState				=
		{
			vk::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			DE_NULL,
			0,

			vk::VK_SAMPLE_COUNT_1_BIT,
			VK_FALSE,
			0.0f,
			&sampleMask,
			VK_FALSE,
			VK_FALSE
		};
		const vk::VkPipelineColorBlendAttachmentState		attachments[]					=
		{
			{
				VK_FALSE,
				vk::VK_BLEND_FACTOR_ONE,
				vk::VK_BLEND_FACTOR_ZERO,
				vk::VK_BLEND_OP_ADD,
				vk::VK_BLEND_FACTOR_ONE,
				vk::VK_BLEND_FACTOR_ZERO,
				vk::VK_BLEND_OP_ADD,
				(vk::VK_COLOR_COMPONENT_R_BIT|
				 vk::VK_COLOR_COMPONENT_G_BIT|
				 vk::VK_COLOR_COMPONENT_B_BIT|
				 vk::VK_COLOR_COMPONENT_A_BIT)
			}
		};
		const vk::VkPipelineColorBlendStateCreateInfo		colorBlendState					=
		{
			vk::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			DE_NULL,
			0,

			VK_FALSE,
			vk::VK_LOGIC_OP_COPY,
			DE_LENGTH_OF_ARRAY(attachments),
			attachments,
			{ 0.0f, 0.0f, 0.0f, 0.0f }
		};
		const vk::VkGraphicsPipelineCreateInfo				createInfo						=
		{
			vk::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			DE_NULL,
			0u,

			DE_LENGTH_OF_ARRAY(shaderStages),
			shaderStages,

			&vertexInputState,
			&inputAssemblyState,
			DE_NULL,
			&viewportState,
			&rasterState,
			&multisampleState,
			&depthStencilState,
			&colorBlendState,
			DE_NULL,
			*resources.pipelineLayout,
			renderPass,
			subpass,
			0,
			0
		};

		resources.pipeline = vk::createGraphicsPipeline(vkd, device, 0, &createInfo);
	}
}

class RenderIndexBuffer : public RenderPassCommand
{
public:
				RenderIndexBuffer	(void) {}
				~RenderIndexBuffer	(void) {}

	const char*	getName				(void) const { return "RenderIndexBuffer"; }
	void		logPrepare			(TestLog&, size_t) const;
	void		logSubmit			(TestLog&, size_t) const;
	void		prepare				(PrepareRenderPassContext&);
	void		submit				(SubmitContext& context);
	void		verify				(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::VkDeviceSize				m_bufferSize;
};

void RenderIndexBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render buffer as index buffer." << TestLog::EndMessage;
}

void RenderIndexBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as index buffer." << TestLog::EndMessage;
}

void RenderIndexBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&				vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice						device					= context.getContext().getDevice();
	const vk::VkRenderPass					renderPass				= context.getRenderPass();
	const deUint32							subpass					= 0;
	const vk::Unique<vk::VkShaderModule>	vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("index-buffer.vert"), 0));
	const vk::Unique<vk::VkShaderModule>	fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-white.frag"), 0));

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), vector<vk::VkDescriptorSetLayoutBinding>(), vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0u, DE_NULL, m_resources);
	m_bufferSize = context.getBufferSize();
}

void RenderIndexBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);
	vkd.cmdBindIndexBuffer(commandBuffer, context.getBuffer(), 0, vk::VK_INDEX_TYPE_UINT16);
	vkd.cmdDrawIndexed(commandBuffer, (deUint32)(context.getBufferSize() / 2), 1, 0, 0, 0);
}

void RenderIndexBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	for (size_t pos = 0; pos < (size_t)m_bufferSize / 2; pos++)
	{
		const deUint8 x  = context.getReference().get(pos * 2);
		const deUint8 y  = context.getReference().get((pos * 2) + 1);

		context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), x, y);
	}
}

class RenderVertexBuffer : public RenderPassCommand
{
public:
				RenderVertexBuffer	(void) {}
				~RenderVertexBuffer	(void) {}

	const char*	getName				(void) const { return "RenderVertexBuffer"; }
	void		logPrepare			(TestLog&, size_t) const;
	void		logSubmit			(TestLog&, size_t) const;
	void		prepare				(PrepareRenderPassContext&);
	void		submit				(SubmitContext& context);
	void		verify				(VerifyRenderPassContext&, size_t);

private:
	PipelineResources	m_resources;
	vk::VkDeviceSize	m_bufferSize;
};

void RenderVertexBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render buffer as vertex buffer." << TestLog::EndMessage;
}

void RenderVertexBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as vertex buffer." << TestLog::EndMessage;
}

void RenderVertexBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&						vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice								device					= context.getContext().getDevice();
	const vk::VkRenderPass							renderPass				= context.getRenderPass();
	const deUint32									subpass					= 0;
	const vk::Unique<vk::VkShaderModule>			vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("vertex-buffer.vert"), 0));
	const vk::Unique<vk::VkShaderModule>			fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-white.frag"), 0));

	vector<vk::VkVertexInputAttributeDescription>	vertexAttributeDescriptions;
	vector<vk::VkVertexInputBindingDescription>		vertexBindingDescriptions;

	{
		const vk::VkVertexInputBindingDescription vertexBindingDescription =
			{
				0,
				2,
				vk::VK_VERTEX_INPUT_RATE_VERTEX
			};

		vertexBindingDescriptions.push_back(vertexBindingDescription);
	}
	{
		const vk::VkVertexInputAttributeDescription vertexAttributeDescription =
		{
			0,
			0,
			vk::VK_FORMAT_R8G8_UNORM,
			0
		};

		vertexAttributeDescriptions.push_back(vertexAttributeDescription);
	}
	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vertexBindingDescriptions, vertexAttributeDescriptions, vector<vk::VkDescriptorSetLayoutBinding>(), vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0u, DE_NULL, m_resources);

	m_bufferSize = context.getBufferSize();
}

void RenderVertexBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();
	const vk::VkDeviceSize		offset			= 0;
	const vk::VkBuffer			buffer			= context.getBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);
	vkd.cmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, &offset);
	vkd.cmdDraw(commandBuffer, (deUint32)(context.getBufferSize() / 2), 1, 0, 0);
}

void RenderVertexBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	for (size_t pos = 0; pos < (size_t)m_bufferSize / 2; pos++)
	{
		const deUint8 x  = context.getReference().get(pos * 2);
		const deUint8 y  = context.getReference().get((pos * 2) + 1);

		context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), x, y);
	}
}

class RenderVertexUniformBuffer : public RenderPassCommand
{
public:
									RenderVertexUniformBuffer	(void) {}
									~RenderVertexUniformBuffer	(void);

	const char*						getName						(void) const { return "RenderVertexUniformBuffer"; }
	void							logPrepare					(TestLog&, size_t) const;
	void							logSubmit					(TestLog&, size_t) const;
	void							prepare						(PrepareRenderPassContext&);
	void							submit						(SubmitContext& context);
	void							verify						(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vector<vk::VkDescriptorSet>		m_descriptorSets;

	vk::VkDeviceSize				m_bufferSize;
};

RenderVertexUniformBuffer::~RenderVertexUniformBuffer (void)
{
}

void RenderVertexUniformBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render buffer as uniform buffer." << TestLog::EndMessage;
}

void RenderVertexUniformBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as uniform buffer." << TestLog::EndMessage;
}

void RenderVertexUniformBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("uniform-buffer.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-white.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	m_bufferSize = context.getBufferSize();

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			vk::VK_SHADER_STAGE_VERTEX_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0u, DE_NULL, m_resources);

	{
		const deUint32							descriptorCount	= (deUint32)(divRoundUp(m_bufferSize, (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE));
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			descriptorCount
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			descriptorCount,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
		m_descriptorSets.resize(descriptorCount);
	}

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSets[descriptorSetNdx] = vk::allocateDescriptorSet(vkd, device, &allocateInfo).disown();

		{
			const vk::VkDescriptorBufferInfo		bufferInfo	=
			{
				context.getBuffer(),
				(vk::VkDeviceSize)(descriptorSetNdx * (size_t)MAX_UNIFORM_BUFFER_SIZE),
				m_bufferSize < (descriptorSetNdx + 1) * (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
					? m_bufferSize - descriptorSetNdx * (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
					: (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
			};
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				m_descriptorSets[descriptorSetNdx],
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				DE_NULL,
				&bufferInfo,
				DE_NULL,
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderVertexUniformBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const size_t	size	= (size_t)(m_bufferSize < (descriptorSetNdx + 1) * (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
								? m_bufferSize - descriptorSetNdx * (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
								: (size_t)MAX_UNIFORM_BUFFER_SIZE);
		const deUint32	count	= (deUint32)(size / 2);

		vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &m_descriptorSets[descriptorSetNdx], 0u, DE_NULL);
		vkd.cmdDraw(commandBuffer, count, 1, 0, 0);
	}
}

void RenderVertexUniformBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const size_t	offset	= descriptorSetNdx * MAX_UNIFORM_BUFFER_SIZE;
		const size_t	size	= (size_t)(m_bufferSize < (descriptorSetNdx + 1) * (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
								? m_bufferSize - descriptorSetNdx * (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
								: (size_t)MAX_UNIFORM_BUFFER_SIZE);
		const size_t	count	= size / 2;

		for (size_t pos = 0; pos < count; pos++)
		{
			const deUint8 x  = context.getReference().get(offset + pos * 2);
			const deUint8 y  = context.getReference().get(offset + (pos * 2) + 1);

			context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), x, y);
		}
	}
}

class RenderVertexUniformTexelBuffer : public RenderPassCommand
{
public:
				RenderVertexUniformTexelBuffer	(void) {}
				~RenderVertexUniformTexelBuffer	(void);

	const char*	getName							(void) const { return "RenderVertexUniformTexelBuffer"; }
	void		logPrepare						(TestLog&, size_t) const;
	void		logSubmit						(TestLog&, size_t) const;
	void		prepare							(PrepareRenderPassContext&);
	void		submit							(SubmitContext& context);
	void		verify							(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vector<vk::VkDescriptorSet>		m_descriptorSets;
	vector<vk::VkBufferView>		m_bufferViews;

	const vk::DeviceInterface*		m_vkd;
	vk::VkDevice					m_device;
	vk::VkDeviceSize				m_bufferSize;
	deUint32						m_maxUniformTexelCount;
};

RenderVertexUniformTexelBuffer::~RenderVertexUniformTexelBuffer (void)
{
	for (size_t bufferViewNdx = 0; bufferViewNdx < m_bufferViews.size(); bufferViewNdx++)
	{
		if (!!m_bufferViews[bufferViewNdx])
		{
			m_vkd->destroyBufferView(m_device, m_bufferViews[bufferViewNdx], DE_NULL);
			m_bufferViews[bufferViewNdx] = (vk::VkBufferView)0;
		}
	}
}

void RenderVertexUniformTexelBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render buffer as uniform buffer." << TestLog::EndMessage;
}

void RenderVertexUniformTexelBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as uniform buffer." << TestLog::EndMessage;
}

void RenderVertexUniformTexelBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::InstanceInterface&				vki						= context.getContext().getInstanceInterface();
	const vk::VkPhysicalDevice					physicalDevice			= context.getContext().getPhysicalDevice();
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("uniform-texel-buffer.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-white.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	m_device				= device;
	m_vkd					= &vkd;
	m_bufferSize			= context.getBufferSize();
	m_maxUniformTexelCount	= vk::getPhysicalDeviceProperties(vki, physicalDevice).limits.maxTexelBufferElements;

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			1,
			vk::VK_SHADER_STAGE_VERTEX_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0u, DE_NULL, m_resources);

	{
		const deUint32							descriptorCount	= (deUint32)(divRoundUp(m_bufferSize, (vk::VkDeviceSize)m_maxUniformTexelCount * 2));
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			descriptorCount
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			descriptorCount,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
		m_descriptorSets.resize(descriptorCount, (vk::VkDescriptorSet)0);
		m_bufferViews.resize(descriptorCount, (vk::VkBufferView)0);
	}

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const deUint32							count			= (deUint32)(m_bufferSize < (descriptorSetNdx + 1) * m_maxUniformTexelCount * 2
																? m_bufferSize - descriptorSetNdx * m_maxUniformTexelCount * 2
																: m_maxUniformTexelCount * 2) / 2;
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSets[descriptorSetNdx] = vk::allocateDescriptorSet(vkd, device, &allocateInfo).disown();

		{
			const vk::VkBufferViewCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
				DE_NULL,
				0u,

				context.getBuffer(),
				vk::VK_FORMAT_R16_UINT,
				descriptorSetNdx * m_maxUniformTexelCount * 2,
				count * 2
			};

			VK_CHECK(vkd.createBufferView(device, &createInfo, DE_NULL, &m_bufferViews[descriptorSetNdx]));
		}

		{
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				m_descriptorSets[descriptorSetNdx],
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
				DE_NULL,
				DE_NULL,
				&m_bufferViews[descriptorSetNdx]
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderVertexUniformTexelBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const deUint32 count	= (deUint32)(m_bufferSize < (descriptorSetNdx + 1) * m_maxUniformTexelCount * 2
								? m_bufferSize - descriptorSetNdx * m_maxUniformTexelCount * 2
								: m_maxUniformTexelCount * 2) / 2;

		vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &m_descriptorSets[descriptorSetNdx], 0u, DE_NULL);
		vkd.cmdDraw(commandBuffer, count, 1, 0, 0);
	}
}

void RenderVertexUniformTexelBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const size_t	offset	= descriptorSetNdx * m_maxUniformTexelCount * 2;
		const deUint32	count	= (deUint32)(m_bufferSize < (descriptorSetNdx + 1) * m_maxUniformTexelCount * 2
								? m_bufferSize - descriptorSetNdx * m_maxUniformTexelCount * 2
								: m_maxUniformTexelCount * 2) / 2;

		for (size_t pos = 0; pos < (size_t)count; pos++)
		{
			const deUint8 x  = context.getReference().get(offset + pos * 2);
			const deUint8 y  = context.getReference().get(offset + (pos * 2) + 1);

			context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), x, y);
		}
	}
}

class RenderVertexStorageBuffer : public RenderPassCommand
{
public:
				RenderVertexStorageBuffer	(void) {}
				~RenderVertexStorageBuffer	(void);

	const char*	getName						(void) const { return "RenderVertexStorageBuffer"; }
	void		logPrepare					(TestLog&, size_t) const;
	void		logSubmit					(TestLog&, size_t) const;
	void		prepare						(PrepareRenderPassContext&);
	void		submit						(SubmitContext& context);
	void		verify						(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vector<vk::VkDescriptorSet>		m_descriptorSets;

	vk::VkDeviceSize				m_bufferSize;
};

RenderVertexStorageBuffer::~RenderVertexStorageBuffer (void)
{
}

void RenderVertexStorageBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render buffer as storage buffer." << TestLog::EndMessage;
}

void RenderVertexStorageBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as storage buffer." << TestLog::EndMessage;
}

void RenderVertexStorageBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("storage-buffer.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-white.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	m_bufferSize = context.getBufferSize();

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1,
			vk::VK_SHADER_STAGE_VERTEX_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0u, DE_NULL, m_resources);

	{
		const deUint32							descriptorCount	= (deUint32)(divRoundUp(m_bufferSize, (vk::VkDeviceSize)MAX_STORAGE_BUFFER_SIZE));
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			descriptorCount
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			descriptorCount,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
		m_descriptorSets.resize(descriptorCount);
	}

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSets[descriptorSetNdx] = vk::allocateDescriptorSet(vkd, device, &allocateInfo).disown();

		{
			const vk::VkDescriptorBufferInfo		bufferInfo	=
			{
				context.getBuffer(),
				descriptorSetNdx * MAX_STORAGE_BUFFER_SIZE,
				de::min(m_bufferSize - descriptorSetNdx * MAX_STORAGE_BUFFER_SIZE,  (vk::VkDeviceSize)MAX_STORAGE_BUFFER_SIZE)
			};
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				m_descriptorSets[descriptorSetNdx],
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				DE_NULL,
				&bufferInfo,
				DE_NULL,
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderVertexStorageBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const size_t size	= m_bufferSize < (descriptorSetNdx + 1) * MAX_STORAGE_BUFFER_SIZE
							? (size_t)(m_bufferSize - descriptorSetNdx * MAX_STORAGE_BUFFER_SIZE)
							: (size_t)(MAX_STORAGE_BUFFER_SIZE);

		vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &m_descriptorSets[descriptorSetNdx], 0u, DE_NULL);
		vkd.cmdDraw(commandBuffer, (deUint32)(size / 2), 1, 0, 0);
	}
}

void RenderVertexStorageBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const size_t offset	= descriptorSetNdx * MAX_STORAGE_BUFFER_SIZE;
		const size_t size	= m_bufferSize < (descriptorSetNdx + 1) * MAX_STORAGE_BUFFER_SIZE
							? (size_t)(m_bufferSize - descriptorSetNdx * MAX_STORAGE_BUFFER_SIZE)
							: (size_t)(MAX_STORAGE_BUFFER_SIZE);

		for (size_t pos = 0; pos < size / 2; pos++)
		{
			const deUint8 x  = context.getReference().get(offset + pos * 2);
			const deUint8 y  = context.getReference().get(offset + (pos * 2) + 1);

			context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), x, y);
		}
	}
}

class RenderVertexStorageTexelBuffer : public RenderPassCommand
{
public:
				RenderVertexStorageTexelBuffer	(void) {}
				~RenderVertexStorageTexelBuffer	(void);

	const char*	getName							(void) const { return "RenderVertexStorageTexelBuffer"; }
	void		logPrepare						(TestLog&, size_t) const;
	void		logSubmit						(TestLog&, size_t) const;
	void		prepare							(PrepareRenderPassContext&);
	void		submit							(SubmitContext& context);
	void		verify							(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vector<vk::VkDescriptorSet>		m_descriptorSets;
	vector<vk::VkBufferView>		m_bufferViews;

	const vk::DeviceInterface*		m_vkd;
	vk::VkDevice					m_device;
	vk::VkDeviceSize				m_bufferSize;
	deUint32						m_maxStorageTexelCount;
};

RenderVertexStorageTexelBuffer::~RenderVertexStorageTexelBuffer (void)
{
	for (size_t bufferViewNdx = 0; bufferViewNdx < m_bufferViews.size(); bufferViewNdx++)
	{
		if (!!m_bufferViews[bufferViewNdx])
		{
			m_vkd->destroyBufferView(m_device, m_bufferViews[bufferViewNdx], DE_NULL);
			m_bufferViews[bufferViewNdx] = (vk::VkBufferView)0;
		}
	}
}

void RenderVertexStorageTexelBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render buffer as storage buffer." << TestLog::EndMessage;
}

void RenderVertexStorageTexelBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as storage buffer." << TestLog::EndMessage;
}

void RenderVertexStorageTexelBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::InstanceInterface&				vki						= context.getContext().getInstanceInterface();
	const vk::VkPhysicalDevice					physicalDevice			= context.getContext().getPhysicalDevice();
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("storage-texel-buffer.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-white.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	m_device				= device;
	m_vkd					= &vkd;
	m_bufferSize			= context.getBufferSize();
	m_maxStorageTexelCount	= vk::getPhysicalDeviceProperties(vki, physicalDevice).limits.maxTexelBufferElements;

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			1,
			vk::VK_SHADER_STAGE_VERTEX_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0u, DE_NULL, m_resources);

	{
		const deUint32							descriptorCount	= (deUint32)(divRoundUp(m_bufferSize, (vk::VkDeviceSize)m_maxStorageTexelCount * 4));
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			descriptorCount
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			descriptorCount,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
		m_descriptorSets.resize(descriptorCount, (vk::VkDescriptorSet)0);
		m_bufferViews.resize(descriptorCount, (vk::VkBufferView)0);
	}

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSets[descriptorSetNdx] = vk::allocateDescriptorSet(vkd, device, &allocateInfo).disown();

		{
			const vk::VkBufferViewCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
				DE_NULL,
				0u,

				context.getBuffer(),
				vk::VK_FORMAT_R32_UINT,
				descriptorSetNdx * m_maxStorageTexelCount * 4,
				(deUint32)de::min<vk::VkDeviceSize>(m_maxStorageTexelCount * 4, m_bufferSize - descriptorSetNdx * m_maxStorageTexelCount * 4)
			};

			VK_CHECK(vkd.createBufferView(device, &createInfo, DE_NULL, &m_bufferViews[descriptorSetNdx]));
		}

		{
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				m_descriptorSets[descriptorSetNdx],
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
				DE_NULL,
				DE_NULL,
				&m_bufferViews[descriptorSetNdx]
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderVertexStorageTexelBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const deUint32 count	= (deUint32)(m_bufferSize < (descriptorSetNdx + 1) * m_maxStorageTexelCount * 4
								? m_bufferSize - descriptorSetNdx * m_maxStorageTexelCount * 4
								: m_maxStorageTexelCount * 4) / 2;

		vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &m_descriptorSets[descriptorSetNdx], 0u, DE_NULL);
		vkd.cmdDraw(commandBuffer, count, 1, 0, 0);
	}
}

void RenderVertexStorageTexelBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const size_t	offset	= descriptorSetNdx * m_maxStorageTexelCount * 4;
		const deUint32	count	= (deUint32)(m_bufferSize < (descriptorSetNdx + 1) * m_maxStorageTexelCount * 4
								? m_bufferSize - descriptorSetNdx * m_maxStorageTexelCount * 4
								: m_maxStorageTexelCount * 4) / 2;

		DE_ASSERT(context.getReference().getSize() <= 4 * m_maxStorageTexelCount * m_descriptorSets.size());
		DE_ASSERT(context.getReference().getSize() > offset);
		DE_ASSERT(offset + count * 2 <= context.getReference().getSize());

		for (size_t pos = 0; pos < (size_t)count; pos++)
		{
			const deUint8 x = context.getReference().get(offset + pos * 2);
			const deUint8 y = context.getReference().get(offset + (pos * 2) + 1);

			context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), x, y);
		}
	}
}

class RenderVertexStorageImage : public RenderPassCommand
{
public:
				RenderVertexStorageImage	(void) {}
				~RenderVertexStorageImage	(void);

	const char*	getName						(void) const { return "RenderVertexStorageImage"; }
	void		logPrepare					(TestLog&, size_t) const;
	void		logSubmit					(TestLog&, size_t) const;
	void		prepare						(PrepareRenderPassContext&);
	void		submit						(SubmitContext& context);
	void		verify						(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vk::Move<vk::VkDescriptorSet>	m_descriptorSet;
	vk::Move<vk::VkImageView>		m_imageView;
};

RenderVertexStorageImage::~RenderVertexStorageImage (void)
{
}

void RenderVertexStorageImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render storage image." << TestLog::EndMessage;
}

void RenderVertexStorageImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using storage image." << TestLog::EndMessage;
}

void RenderVertexStorageImage::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("storage-image.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-white.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1,
			vk::VK_SHADER_STAGE_VERTEX_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0u, DE_NULL, m_resources);

	{
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			1u,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
	}

	{
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSet = vk::allocateDescriptorSet(vkd, device, &allocateInfo);

		{
			const vk::VkImageViewCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				DE_NULL,
				0u,

				context.getImage(),
				vk::VK_IMAGE_VIEW_TYPE_2D,
				vk::VK_FORMAT_R8G8B8A8_UNORM,
				vk::makeComponentMappingRGBA(),
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,
					0u,
					1u,
					0u,
					1u
				}
			};

			m_imageView = vk::createImageView(vkd, device, &createInfo);
		}

		{
			const vk::VkDescriptorImageInfo			imageInfo	=
			{
				0,
				*m_imageView,
				context.getImageLayout()
			};
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				*m_descriptorSet,
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				&imageInfo,
				DE_NULL,
				DE_NULL,
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderVertexStorageImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &(*m_descriptorSet), 0u, DE_NULL);
	vkd.cmdDraw(commandBuffer, context.getImageWidth() * context.getImageHeight() * 2, 1, 0, 0);
}

void RenderVertexStorageImage::verify (VerifyRenderPassContext& context, size_t)
{
	for (int pos = 0; pos < (int)(context.getReferenceImage().getWidth() * context.getReferenceImage().getHeight() * 2); pos++)
	{
		const tcu::IVec3		size	= context.getReferenceImage().getAccess().getSize();
		const tcu::UVec4		pixel	= context.getReferenceImage().getAccess().getPixelUint((pos / 2) / size.x(), (pos / 2) % size.x());

		if (pos % 2 == 0)
			context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), pixel.x(), pixel.y());
		else
			context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), pixel.z(), pixel.w());
	}
}

class RenderVertexSampledImage : public RenderPassCommand
{
public:
				RenderVertexSampledImage	(void) {}
				~RenderVertexSampledImage	(void);

	const char*	getName						(void) const { return "RenderVertexSampledImage"; }
	void		logPrepare					(TestLog&, size_t) const;
	void		logSubmit					(TestLog&, size_t) const;
	void		prepare						(PrepareRenderPassContext&);
	void		submit						(SubmitContext& context);
	void		verify						(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vk::Move<vk::VkDescriptorSet>	m_descriptorSet;
	vk::Move<vk::VkImageView>		m_imageView;
	vk::Move<vk::VkSampler>			m_sampler;
};

RenderVertexSampledImage::~RenderVertexSampledImage (void)
{
}

void RenderVertexSampledImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render sampled image." << TestLog::EndMessage;
}

void RenderVertexSampledImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using sampled image." << TestLog::EndMessage;
}

void RenderVertexSampledImage::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("sampled-image.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-white.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			vk::VK_SHADER_STAGE_VERTEX_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0u, DE_NULL, m_resources);

	{
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			1u,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
	}

	{
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSet = vk::allocateDescriptorSet(vkd, device, &allocateInfo);

		{
			const vk::VkImageViewCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				DE_NULL,
				0u,

				context.getImage(),
				vk::VK_IMAGE_VIEW_TYPE_2D,
				vk::VK_FORMAT_R8G8B8A8_UNORM,
				vk::makeComponentMappingRGBA(),
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,
					0u,
					1u,
					0u,
					1u
				}
			};

			m_imageView = vk::createImageView(vkd, device, &createInfo);
		}

		{
			const vk::VkSamplerCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				DE_NULL,
				0u,

				vk::VK_FILTER_NEAREST,
				vk::VK_FILTER_NEAREST,

				vk::VK_SAMPLER_MIPMAP_MODE_LINEAR,
				vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				0.0f,
				VK_FALSE,
				1.0f,
				VK_FALSE,
				vk::VK_COMPARE_OP_ALWAYS,
				0.0f,
				0.0f,
				vk::VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
				VK_FALSE
			};

			m_sampler = vk::createSampler(vkd, device, &createInfo);
		}

		{
			const vk::VkDescriptorImageInfo			imageInfo	=
			{
				*m_sampler,
				*m_imageView,
				context.getImageLayout()
			};
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				*m_descriptorSet,
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&imageInfo,
				DE_NULL,
				DE_NULL,
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderVertexSampledImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &(*m_descriptorSet), 0u, DE_NULL);
	vkd.cmdDraw(commandBuffer, context.getImageWidth() * context.getImageHeight() * 2, 1, 0, 0);
}

void RenderVertexSampledImage::verify (VerifyRenderPassContext& context, size_t)
{
	for (int pos = 0; pos < (int)(context.getReferenceImage().getWidth() * context.getReferenceImage().getHeight() * 2); pos++)
	{
		const tcu::IVec3	size	= context.getReferenceImage().getAccess().getSize();
		const tcu::UVec4	pixel	= context.getReferenceImage().getAccess().getPixelUint((pos / 2) / size.x(), (pos / 2) % size.x());

		if (pos % 2 == 0)
			context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), pixel.x(), pixel.y());
		else
			context.getReferenceTarget().getAccess().setPixel(Vec4(1.0f, 1.0f, 1.0f, 1.0f), pixel.z(), pixel.w());
	}
}

class RenderFragmentUniformBuffer : public RenderPassCommand
{
public:
									RenderFragmentUniformBuffer		(void) {}
									~RenderFragmentUniformBuffer	(void);

	const char*						getName							(void) const { return "RenderFragmentUniformBuffer"; }
	void							logPrepare						(TestLog&, size_t) const;
	void							logSubmit						(TestLog&, size_t) const;
	void							prepare							(PrepareRenderPassContext&);
	void							submit							(SubmitContext& context);
	void							verify							(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vector<vk::VkDescriptorSet>		m_descriptorSets;

	vk::VkDeviceSize				m_bufferSize;
	size_t							m_targetWidth;
	size_t							m_targetHeight;
};

RenderFragmentUniformBuffer::~RenderFragmentUniformBuffer (void)
{
}

void RenderFragmentUniformBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render buffer as uniform buffer." << TestLog::EndMessage;
}

void RenderFragmentUniformBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as uniform buffer." << TestLog::EndMessage;
}

void RenderFragmentUniformBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-quad.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("uniform-buffer.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	m_bufferSize	= de::min(context.getBufferSize(), (vk::VkDeviceSize)MAX_SIZE);
	m_targetWidth	= context.getTargetWidth();
	m_targetHeight	= context.getTargetHeight();

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			vk::VK_SHADER_STAGE_FRAGMENT_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}
	const vk::VkPushConstantRange pushConstantRange =
	{
		vk::VK_SHADER_STAGE_FRAGMENT_BIT,
		0u,
		8u
	};

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 1u, &pushConstantRange, m_resources);

	{
		const deUint32							descriptorCount	= (deUint32)(divRoundUp(m_bufferSize, (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE));
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			descriptorCount
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			descriptorCount,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
		m_descriptorSets.resize(descriptorCount);
	}

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSets[descriptorSetNdx] = vk::allocateDescriptorSet(vkd, device, &allocateInfo).disown();

		{
			const vk::VkDescriptorBufferInfo		bufferInfo	=
			{
				context.getBuffer(),
				(vk::VkDeviceSize)(descriptorSetNdx * (size_t)MAX_UNIFORM_BUFFER_SIZE),
				m_bufferSize < (descriptorSetNdx + 1) * (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
					? m_bufferSize - descriptorSetNdx * (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
					: (vk::VkDeviceSize)MAX_UNIFORM_BUFFER_SIZE
			};
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				m_descriptorSets[descriptorSetNdx],
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				DE_NULL,
				&bufferInfo,
				DE_NULL,
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderFragmentUniformBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const struct
		{
			const deUint32	callId;
			const deUint32	valuesPerPixel;
		} callParams =
		{
			(deUint32)descriptorSetNdx,
			(deUint32)divRoundUp<size_t>(m_descriptorSets.size() * (MAX_UNIFORM_BUFFER_SIZE / 4), m_targetWidth * m_targetHeight)
		};

		vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &m_descriptorSets[descriptorSetNdx], 0u, DE_NULL);
		vkd.cmdPushConstants(commandBuffer, *m_resources.pipelineLayout, vk::VK_SHADER_STAGE_FRAGMENT_BIT, 0u, (deUint32)sizeof(callParams), &callParams);
		vkd.cmdDraw(commandBuffer, 6, 1, 0, 0);
	}
}

void RenderFragmentUniformBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	const deUint32	valuesPerPixel	= (deUint32)divRoundUp<size_t>(m_descriptorSets.size() * (MAX_UNIFORM_BUFFER_SIZE / 4), m_targetWidth * m_targetHeight);
	const size_t	arraySize		= MAX_UNIFORM_BUFFER_SIZE / (sizeof(deUint32) * 4);
	const size_t	arrayIntSize	= arraySize * 4;

	for (int y = 0; y < context.getReferenceTarget().getSize().y(); y++)
	for (int x = 0; x < context.getReferenceTarget().getSize().x(); x++)
	{
		const size_t firstDescriptorSetNdx = de::min<size_t>((y * 256u + x) / (arrayIntSize / valuesPerPixel), m_descriptorSets.size() - 1);

		for (size_t descriptorSetNdx = firstDescriptorSetNdx; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
		{
			const size_t	offset	= descriptorSetNdx * MAX_UNIFORM_BUFFER_SIZE;
			const deUint32	callId	= (deUint32)descriptorSetNdx;

			const deUint32	id		= callId * ((deUint32)arrayIntSize / valuesPerPixel) + (deUint32)y * 256u + (deUint32)x;

			if (y * 256u + x < callId * (arrayIntSize / valuesPerPixel))
				continue;
			else
			{
				deUint32 value = id;

				for (deUint32 i = 0; i < valuesPerPixel; i++)
				{
					value	= ((deUint32)context.getReference().get(offset + (value % (MAX_UNIFORM_BUFFER_SIZE / sizeof(deUint32))) * 4 + 0))
							| (((deUint32)context.getReference().get(offset + (value % (MAX_UNIFORM_BUFFER_SIZE / sizeof(deUint32))) * 4 + 1)) << 8u)
							| (((deUint32)context.getReference().get(offset + (value % (MAX_UNIFORM_BUFFER_SIZE / sizeof(deUint32))) * 4 + 2)) << 16u)
							| (((deUint32)context.getReference().get(offset + (value % (MAX_UNIFORM_BUFFER_SIZE / sizeof(deUint32))) * 4 + 3)) << 24u);

				}
				const UVec4	vec	((value >>  0u) & 0xFFu,
								 (value >>  8u) & 0xFFu,
								 (value >> 16u) & 0xFFu,
								 (value >> 24u) & 0xFFu);

				context.getReferenceTarget().getAccess().setPixel(vec.asFloat() / Vec4(255.0f), x, y);
			}
		}
	}
}

class RenderFragmentStorageBuffer : public RenderPassCommand
{
public:
									RenderFragmentStorageBuffer		(void) {}
									~RenderFragmentStorageBuffer	(void);

	const char*						getName							(void) const { return "RenderFragmentStorageBuffer"; }
	void							logPrepare						(TestLog&, size_t) const;
	void							logSubmit						(TestLog&, size_t) const;
	void							prepare							(PrepareRenderPassContext&);
	void							submit							(SubmitContext& context);
	void							verify							(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vk::Move<vk::VkDescriptorSet>	m_descriptorSet;

	vk::VkDeviceSize				m_bufferSize;
	size_t							m_targetWidth;
	size_t							m_targetHeight;
};

RenderFragmentStorageBuffer::~RenderFragmentStorageBuffer (void)
{
}

void RenderFragmentStorageBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline to render buffer as storage buffer." << TestLog::EndMessage;
}

void RenderFragmentStorageBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as storage buffer." << TestLog::EndMessage;
}

void RenderFragmentStorageBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-quad.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("storage-buffer.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	m_bufferSize	= context.getBufferSize();
	m_targetWidth	= context.getTargetWidth();
	m_targetHeight	= context.getTargetHeight();

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1,
			vk::VK_SHADER_STAGE_FRAGMENT_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}
	const vk::VkPushConstantRange pushConstantRange =
	{
		vk::VK_SHADER_STAGE_FRAGMENT_BIT,
		0u,
		12u
	};

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 1u, &pushConstantRange, m_resources);

	{
		const deUint32							descriptorCount	= 1;
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			descriptorCount
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			descriptorCount,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
	}

	{
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSet = vk::allocateDescriptorSet(vkd, device, &allocateInfo);

		{
			const vk::VkDescriptorBufferInfo	bufferInfo	=
			{
				context.getBuffer(),
				0u,
				m_bufferSize
			};
			const vk::VkWriteDescriptorSet		write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				m_descriptorSet.get(),
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				DE_NULL,
				&bufferInfo,
				DE_NULL,
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderFragmentStorageBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	const struct
	{
		const deUint32	valuesPerPixel;
		const deUint32	bufferSize;
	} callParams =
	{
		(deUint32)divRoundUp<vk::VkDeviceSize>(m_bufferSize / 4, m_targetWidth * m_targetHeight),
		(deUint32)m_bufferSize
	};

	vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &m_descriptorSet.get(), 0u, DE_NULL);
	vkd.cmdPushConstants(commandBuffer, *m_resources.pipelineLayout, vk::VK_SHADER_STAGE_FRAGMENT_BIT, 0u, (deUint32)sizeof(callParams), &callParams);
	vkd.cmdDraw(commandBuffer, 6, 1, 0, 0);
}

void RenderFragmentStorageBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	const deUint32	valuesPerPixel	= (deUint32)divRoundUp<vk::VkDeviceSize>(m_bufferSize / 4, m_targetWidth * m_targetHeight);

	for (int y = 0; y < context.getReferenceTarget().getSize().y(); y++)
	for (int x = 0; x < context.getReferenceTarget().getSize().x(); x++)
	{
		const deUint32	id		= (deUint32)y * 256u + (deUint32)x;

		deUint32 value = id;

		for (deUint32 i = 0; i < valuesPerPixel; i++)
		{
			value	= (((deUint32)context.getReference().get((size_t)(value % (m_bufferSize / sizeof(deUint32))) * 4 + 0)) << 0u)
					| (((deUint32)context.getReference().get((size_t)(value % (m_bufferSize / sizeof(deUint32))) * 4 + 1)) << 8u)
					| (((deUint32)context.getReference().get((size_t)(value % (m_bufferSize / sizeof(deUint32))) * 4 + 2)) << 16u)
					| (((deUint32)context.getReference().get((size_t)(value % (m_bufferSize / sizeof(deUint32))) * 4 + 3)) << 24u);

		}
		const UVec4	vec	((value >>  0u) & 0xFFu,
						 (value >>  8u) & 0xFFu,
						 (value >> 16u) & 0xFFu,
						 (value >> 24u) & 0xFFu);

		context.getReferenceTarget().getAccess().setPixel(vec.asFloat() / Vec4(255.0f), x, y);
	}
}

class RenderFragmentUniformTexelBuffer : public RenderPassCommand
{
public:
									RenderFragmentUniformTexelBuffer	(void) {}
									~RenderFragmentUniformTexelBuffer	(void);

	const char*						getName								(void) const { return "RenderFragmentUniformTexelBuffer"; }
	void							logPrepare							(TestLog&, size_t) const;
	void							logSubmit							(TestLog&, size_t) const;
	void							prepare								(PrepareRenderPassContext&);
	void							submit								(SubmitContext& context);
	void							verify								(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vector<vk::VkDescriptorSet>		m_descriptorSets;
	vector<vk::VkBufferView>		m_bufferViews;

	const vk::DeviceInterface*		m_vkd;
	vk::VkDevice					m_device;
	vk::VkDeviceSize				m_bufferSize;
	deUint32						m_maxUniformTexelCount;
	size_t							m_targetWidth;
	size_t							m_targetHeight;
};

RenderFragmentUniformTexelBuffer::~RenderFragmentUniformTexelBuffer (void)
{
	for (size_t bufferViewNdx = 0; bufferViewNdx < m_bufferViews.size(); bufferViewNdx++)
	{
		if (!!m_bufferViews[bufferViewNdx])
		{
			m_vkd->destroyBufferView(m_device, m_bufferViews[bufferViewNdx], DE_NULL);
			m_bufferViews[bufferViewNdx] = (vk::VkBufferView)0;
		}
	}
}

void RenderFragmentUniformTexelBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render buffer as uniform buffer." << TestLog::EndMessage;
}

void RenderFragmentUniformTexelBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as uniform buffer." << TestLog::EndMessage;
}

void RenderFragmentUniformTexelBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::InstanceInterface&				vki						= context.getContext().getInstanceInterface();
	const vk::VkPhysicalDevice					physicalDevice			= context.getContext().getPhysicalDevice();
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-quad.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("uniform-texel-buffer.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	m_device				= device;
	m_vkd					= &vkd;
	m_bufferSize			= context.getBufferSize();
	m_maxUniformTexelCount	= vk::getPhysicalDeviceProperties(vki, physicalDevice).limits.maxTexelBufferElements;
	m_targetWidth			= context.getTargetWidth();
	m_targetHeight			= context.getTargetHeight();

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			1,
			vk::VK_SHADER_STAGE_FRAGMENT_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}
	const vk::VkPushConstantRange pushConstantRange =
	{
		vk::VK_SHADER_STAGE_FRAGMENT_BIT,
		0u,
		12u
	};

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 1u, &pushConstantRange, m_resources);

	{
		const deUint32							descriptorCount	= (deUint32)(divRoundUp(m_bufferSize, (vk::VkDeviceSize)m_maxUniformTexelCount * 4));
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			descriptorCount
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			descriptorCount,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
		m_descriptorSets.resize(descriptorCount, (vk::VkDescriptorSet)0);
		m_bufferViews.resize(descriptorCount, (vk::VkBufferView)0);
	}

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const deUint32							count			= (deUint32)(m_bufferSize < (descriptorSetNdx + 1) * m_maxUniformTexelCount * 4
																? m_bufferSize - descriptorSetNdx * m_maxUniformTexelCount * 4
																: m_maxUniformTexelCount * 4) / 4;
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSets[descriptorSetNdx] = vk::allocateDescriptorSet(vkd, device, &allocateInfo).disown();

		{
			const vk::VkBufferViewCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
				DE_NULL,
				0u,

				context.getBuffer(),
				vk::VK_FORMAT_R32_UINT,
				descriptorSetNdx * m_maxUniformTexelCount * 4,
				count * 4
			};

			VK_CHECK(vkd.createBufferView(device, &createInfo, DE_NULL, &m_bufferViews[descriptorSetNdx]));
		}

		{
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				m_descriptorSets[descriptorSetNdx],
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
				DE_NULL,
				DE_NULL,
				&m_bufferViews[descriptorSetNdx]
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderFragmentUniformTexelBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const struct
		{
			const deUint32	callId;
			const deUint32	valuesPerPixel;
			const deUint32	maxUniformTexelCount;
		} callParams =
		{
			(deUint32)descriptorSetNdx,
			(deUint32)divRoundUp<size_t>(m_descriptorSets.size() * de::min<size_t>((size_t)m_bufferSize / 4, m_maxUniformTexelCount), m_targetWidth * m_targetHeight),
			m_maxUniformTexelCount
		};

		vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &m_descriptorSets[descriptorSetNdx], 0u, DE_NULL);
		vkd.cmdPushConstants(commandBuffer, *m_resources.pipelineLayout, vk::VK_SHADER_STAGE_FRAGMENT_BIT, 0u, (deUint32)sizeof(callParams), &callParams);
		vkd.cmdDraw(commandBuffer, 6, 1, 0, 0);
	}
}

void RenderFragmentUniformTexelBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	const deUint32	valuesPerPixel	= (deUint32)divRoundUp<size_t>(m_descriptorSets.size() * de::min<size_t>((size_t)m_bufferSize / 4, m_maxUniformTexelCount), m_targetWidth * m_targetHeight);

	for (int y = 0; y < context.getReferenceTarget().getSize().y(); y++)
	for (int x = 0; x < context.getReferenceTarget().getSize().x(); x++)
	{
		const size_t firstDescriptorSetNdx = de::min<size_t>((y * 256u + x) / (m_maxUniformTexelCount / valuesPerPixel), m_descriptorSets.size() - 1);

		for (size_t descriptorSetNdx = firstDescriptorSetNdx; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
		{
			const size_t	offset	= descriptorSetNdx * m_maxUniformTexelCount * 4;
			const deUint32	callId	= (deUint32)descriptorSetNdx;

			const deUint32	id		= (deUint32)y * 256u + (deUint32)x;
			const deUint32	count	= (deUint32)(m_bufferSize < (descriptorSetNdx + 1) * m_maxUniformTexelCount * 4
									? m_bufferSize - descriptorSetNdx * m_maxUniformTexelCount * 4
									: m_maxUniformTexelCount * 4) / 4;

			if (y * 256u + x < callId * (m_maxUniformTexelCount / valuesPerPixel))
				continue;
			else
			{
				deUint32 value = id;

				for (deUint32 i = 0; i < valuesPerPixel; i++)
				{
					value	= ((deUint32)context.getReference().get( offset + (value % count) * 4 + 0))
							| (((deUint32)context.getReference().get(offset + (value % count) * 4 + 1)) << 8u)
							| (((deUint32)context.getReference().get(offset + (value % count) * 4 + 2)) << 16u)
							| (((deUint32)context.getReference().get(offset + (value % count) * 4 + 3)) << 24u);

				}
				const UVec4	vec	((value >>  0u) & 0xFFu,
								 (value >>  8u) & 0xFFu,
								 (value >> 16u) & 0xFFu,
								 (value >> 24u) & 0xFFu);

				context.getReferenceTarget().getAccess().setPixel(vec.asFloat() / Vec4(255.0f), x, y);
			}
		}
	}
}

class RenderFragmentStorageTexelBuffer : public RenderPassCommand
{
public:
									RenderFragmentStorageTexelBuffer	(void) {}
									~RenderFragmentStorageTexelBuffer	(void);

	const char*						getName								(void) const { return "RenderFragmentStorageTexelBuffer"; }
	void							logPrepare							(TestLog&, size_t) const;
	void							logSubmit							(TestLog&, size_t) const;
	void							prepare								(PrepareRenderPassContext&);
	void							submit								(SubmitContext& context);
	void							verify								(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vector<vk::VkDescriptorSet>		m_descriptorSets;
	vector<vk::VkBufferView>		m_bufferViews;

	const vk::DeviceInterface*		m_vkd;
	vk::VkDevice					m_device;
	vk::VkDeviceSize				m_bufferSize;
	deUint32						m_maxStorageTexelCount;
	size_t							m_targetWidth;
	size_t							m_targetHeight;
};

RenderFragmentStorageTexelBuffer::~RenderFragmentStorageTexelBuffer (void)
{
	for (size_t bufferViewNdx = 0; bufferViewNdx < m_bufferViews.size(); bufferViewNdx++)
	{
		if (!!m_bufferViews[bufferViewNdx])
		{
			m_vkd->destroyBufferView(m_device, m_bufferViews[bufferViewNdx], DE_NULL);
			m_bufferViews[bufferViewNdx] = (vk::VkBufferView)0;
		}
	}
}

void RenderFragmentStorageTexelBuffer::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render buffer as storage buffer." << TestLog::EndMessage;
}

void RenderFragmentStorageTexelBuffer::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using buffer as storage buffer." << TestLog::EndMessage;
}

void RenderFragmentStorageTexelBuffer::prepare (PrepareRenderPassContext& context)
{
	const vk::InstanceInterface&				vki						= context.getContext().getInstanceInterface();
	const vk::VkPhysicalDevice					physicalDevice			= context.getContext().getPhysicalDevice();
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-quad.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("storage-texel-buffer.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	m_device				= device;
	m_vkd					= &vkd;
	m_bufferSize			= context.getBufferSize();
	m_maxStorageTexelCount	= vk::getPhysicalDeviceProperties(vki, physicalDevice).limits.maxTexelBufferElements;
	m_targetWidth			= context.getTargetWidth();
	m_targetHeight			= context.getTargetHeight();

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			1,
			vk::VK_SHADER_STAGE_FRAGMENT_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}
	const vk::VkPushConstantRange pushConstantRange =
	{
		vk::VK_SHADER_STAGE_FRAGMENT_BIT,
		0u,
		16u
	};

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 1u, &pushConstantRange, m_resources);

	{
		const deUint32							descriptorCount	= (deUint32)(divRoundUp(m_bufferSize, (vk::VkDeviceSize)m_maxStorageTexelCount * 4));
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			descriptorCount
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			descriptorCount,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
		m_descriptorSets.resize(descriptorCount, (vk::VkDescriptorSet)0);
		m_bufferViews.resize(descriptorCount, (vk::VkBufferView)0);
	}

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const deUint32							count			= (deUint32)(m_bufferSize < (descriptorSetNdx + 1) * m_maxStorageTexelCount * 4
																? m_bufferSize - descriptorSetNdx * m_maxStorageTexelCount * 4
																: m_maxStorageTexelCount * 4) / 4;
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSets[descriptorSetNdx] = vk::allocateDescriptorSet(vkd, device, &allocateInfo).disown();

		{
			const vk::VkBufferViewCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
				DE_NULL,
				0u,

				context.getBuffer(),
				vk::VK_FORMAT_R32_UINT,
				descriptorSetNdx * m_maxStorageTexelCount * 4,
				count * 4
			};

			VK_CHECK(vkd.createBufferView(device, &createInfo, DE_NULL, &m_bufferViews[descriptorSetNdx]));
		}

		{
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				m_descriptorSets[descriptorSetNdx],
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
				DE_NULL,
				DE_NULL,
				&m_bufferViews[descriptorSetNdx]
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderFragmentStorageTexelBuffer::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	for (size_t descriptorSetNdx = 0; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
	{
		const struct
		{
			const deUint32	callId;
			const deUint32	valuesPerPixel;
			const deUint32	maxStorageTexelCount;
			const deUint32	width;
		} callParams =
		{
			(deUint32)descriptorSetNdx,
			(deUint32)divRoundUp<size_t>(m_descriptorSets.size() * de::min<size_t>(m_maxStorageTexelCount, (size_t)m_bufferSize / 4), m_targetWidth * m_targetHeight),
			m_maxStorageTexelCount,
			(deUint32)(m_bufferSize < (descriptorSetNdx + 1u) * m_maxStorageTexelCount * 4u
								? m_bufferSize - descriptorSetNdx * m_maxStorageTexelCount * 4u
								: m_maxStorageTexelCount * 4u) / 4u
		};

		vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &m_descriptorSets[descriptorSetNdx], 0u, DE_NULL);
		vkd.cmdPushConstants(commandBuffer, *m_resources.pipelineLayout, vk::VK_SHADER_STAGE_FRAGMENT_BIT, 0u, (deUint32)sizeof(callParams), &callParams);
		vkd.cmdDraw(commandBuffer, 6, 1, 0, 0);
	}
}

void RenderFragmentStorageTexelBuffer::verify (VerifyRenderPassContext& context, size_t)
{
	const deUint32	valuesPerPixel	= (deUint32)divRoundUp<size_t>(m_descriptorSets.size() * de::min<size_t>(m_maxStorageTexelCount, (size_t)m_bufferSize / 4), m_targetWidth * m_targetHeight);

	for (int y = 0; y < context.getReferenceTarget().getSize().y(); y++)
	for (int x = 0; x < context.getReferenceTarget().getSize().x(); x++)
	{
		const size_t firstDescriptorSetNdx = de::min<size_t>((y * 256u + x) / (m_maxStorageTexelCount / valuesPerPixel), m_descriptorSets.size() - 1);

		for (size_t descriptorSetNdx = firstDescriptorSetNdx; descriptorSetNdx < m_descriptorSets.size(); descriptorSetNdx++)
		{
			const size_t	offset	= descriptorSetNdx * m_maxStorageTexelCount * 4;
			const deUint32	callId	= (deUint32)descriptorSetNdx;

			const deUint32	id		= (deUint32)y * 256u + (deUint32)x;
			const deUint32	count	= (deUint32)(m_bufferSize < (descriptorSetNdx + 1) * m_maxStorageTexelCount * 4
									? m_bufferSize - descriptorSetNdx * m_maxStorageTexelCount * 4
									: m_maxStorageTexelCount * 4) / 4;

			if (y * 256u + x < callId * (m_maxStorageTexelCount / valuesPerPixel))
				continue;
			else
			{
				deUint32 value = id;

				for (deUint32 i = 0; i < valuesPerPixel; i++)
				{
					value	= ((deUint32)context.getReference().get( offset + (value % count) * 4 + 0))
							| (((deUint32)context.getReference().get(offset + (value % count) * 4 + 1)) << 8u)
							| (((deUint32)context.getReference().get(offset + (value % count) * 4 + 2)) << 16u)
							| (((deUint32)context.getReference().get(offset + (value % count) * 4 + 3)) << 24u);

				}
				const UVec4	vec	((value >>  0u) & 0xFFu,
								 (value >>  8u) & 0xFFu,
								 (value >> 16u) & 0xFFu,
								 (value >> 24u) & 0xFFu);

				context.getReferenceTarget().getAccess().setPixel(vec.asFloat() / Vec4(255.0f), x, y);
			}
		}
	}
}

class RenderFragmentStorageImage : public RenderPassCommand
{
public:
									RenderFragmentStorageImage	(void) {}
									~RenderFragmentStorageImage	(void);

	const char*						getName						(void) const { return "RenderFragmentStorageImage"; }
	void							logPrepare					(TestLog&, size_t) const;
	void							logSubmit					(TestLog&, size_t) const;
	void							prepare						(PrepareRenderPassContext&);
	void							submit						(SubmitContext& context);
	void							verify						(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vk::Move<vk::VkDescriptorSet>	m_descriptorSet;
	vk::Move<vk::VkImageView>		m_imageView;
};

RenderFragmentStorageImage::~RenderFragmentStorageImage (void)
{
}

void RenderFragmentStorageImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render storage image." << TestLog::EndMessage;
}

void RenderFragmentStorageImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using storage image." << TestLog::EndMessage;
}

void RenderFragmentStorageImage::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-quad.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("storage-image.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1,
			vk::VK_SHADER_STAGE_FRAGMENT_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0u, DE_NULL, m_resources);

	{
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			1u,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
	}

	{
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSet = vk::allocateDescriptorSet(vkd, device, &allocateInfo);

		{
			const vk::VkImageViewCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				DE_NULL,
				0u,

				context.getImage(),
				vk::VK_IMAGE_VIEW_TYPE_2D,
				vk::VK_FORMAT_R8G8B8A8_UNORM,
				vk::makeComponentMappingRGBA(),
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,
					0u,
					1u,
					0u,
					1u
				}
			};

			m_imageView = vk::createImageView(vkd, device, &createInfo);
		}

		{
			const vk::VkDescriptorImageInfo			imageInfo	=
			{
				0,
				*m_imageView,
				context.getImageLayout()
			};
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				*m_descriptorSet,
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				&imageInfo,
				DE_NULL,
				DE_NULL,
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderFragmentStorageImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &(*m_descriptorSet), 0u, DE_NULL);
	vkd.cmdDraw(commandBuffer, 6, 1, 0, 0);
}

void RenderFragmentStorageImage::verify (VerifyRenderPassContext& context, size_t)
{
	const UVec2		size			= UVec2(context.getReferenceImage().getWidth(), context.getReferenceImage().getHeight());
	const deUint32	valuesPerPixel	= de::max<deUint32>(1u, (size.x() * size.y()) / (256u * 256u));

	for (int y = 0; y < context.getReferenceTarget().getSize().y(); y++)
	for (int x = 0; x < context.getReferenceTarget().getSize().x(); x++)
	{
		UVec4	value	= UVec4(x, y, 0u, 0u);

		for (deUint32 i = 0; i < valuesPerPixel; i++)
		{
			const UVec2	pos			= UVec2(value.z() * 256u + (value.x() ^ value.z()), value.w() * 256u + (value.y() ^ value.w()));
			const Vec4	floatValue	= context.getReferenceImage().getAccess().getPixel(pos.x() % size.x(), pos.y() % size.y());

			value = UVec4((deUint32)(floatValue.x() * 255.0f),
						  (deUint32)(floatValue.y() * 255.0f),
						  (deUint32)(floatValue.z() * 255.0f),
						  (deUint32)(floatValue.w() * 255.0f));

		}
		context.getReferenceTarget().getAccess().setPixel(value.asFloat() / Vec4(255.0f), x, y);
	}
}

class RenderFragmentSampledImage : public RenderPassCommand
{
public:
				RenderFragmentSampledImage	(void) {}
				~RenderFragmentSampledImage	(void);

	const char*	getName						(void) const { return "RenderFragmentSampledImage"; }
	void		logPrepare					(TestLog&, size_t) const;
	void		logSubmit					(TestLog&, size_t) const;
	void		prepare						(PrepareRenderPassContext&);
	void		submit						(SubmitContext& context);
	void		verify						(VerifyRenderPassContext&, size_t);

private:
	PipelineResources				m_resources;
	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vk::Move<vk::VkDescriptorSet>	m_descriptorSet;
	vk::Move<vk::VkImageView>		m_imageView;
	vk::Move<vk::VkSampler>			m_sampler;
};

RenderFragmentSampledImage::~RenderFragmentSampledImage (void)
{
}

void RenderFragmentSampledImage::logPrepare (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Create pipeline for render storage image." << TestLog::EndMessage;
}

void RenderFragmentSampledImage::logSubmit (TestLog& log, size_t commandIndex) const
{
	log << TestLog::Message << commandIndex << ":" << getName() << " Render using storage image." << TestLog::EndMessage;
}

void RenderFragmentSampledImage::prepare (PrepareRenderPassContext& context)
{
	const vk::DeviceInterface&					vkd						= context.getContext().getDeviceInterface();
	const vk::VkDevice							device					= context.getContext().getDevice();
	const vk::VkRenderPass						renderPass				= context.getRenderPass();
	const deUint32								subpass					= 0;
	const vk::Unique<vk::VkShaderModule>		vertexShaderModule		(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("render-quad.vert"), 0));
	const vk::Unique<vk::VkShaderModule>		fragmentShaderModule	(vk::createShaderModule(vkd, device, context.getBinaryCollection().get("sampled-image.frag"), 0));
	vector<vk::VkDescriptorSetLayoutBinding>	bindings;

	{
		const vk::VkDescriptorSetLayoutBinding binding =
		{
			0u,
			vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			vk::VK_SHADER_STAGE_FRAGMENT_BIT,
			DE_NULL
		};

		bindings.push_back(binding);
	}

	createPipelineWithResources(vkd, device, renderPass, subpass, *vertexShaderModule, *fragmentShaderModule, context.getTargetWidth(), context.getTargetHeight(),
								vector<vk::VkVertexInputBindingDescription>(), vector<vk::VkVertexInputAttributeDescription>(), bindings, vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0u, DE_NULL, m_resources);

	{
		const vk::VkDescriptorPoolSize			poolSizes		=
		{
			vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1
		};
		const vk::VkDescriptorPoolCreateInfo	createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			DE_NULL,
			vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

			1u,
			1u,
			&poolSizes,
		};

		m_descriptorPool = vk::createDescriptorPool(vkd, device, &createInfo);
	}

	{
		const vk::VkDescriptorSetLayout			layout			= *m_resources.descriptorSetLayout;
		const vk::VkDescriptorSetAllocateInfo	allocateInfo	=
		{
			vk::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,

			*m_descriptorPool,
			1,
			&layout
		};

		m_descriptorSet = vk::allocateDescriptorSet(vkd, device, &allocateInfo);

		{
			const vk::VkImageViewCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				DE_NULL,
				0u,

				context.getImage(),
				vk::VK_IMAGE_VIEW_TYPE_2D,
				vk::VK_FORMAT_R8G8B8A8_UNORM,
				vk::makeComponentMappingRGBA(),
				{
					vk::VK_IMAGE_ASPECT_COLOR_BIT,
					0u,
					1u,
					0u,
					1u
				}
			};

			m_imageView = vk::createImageView(vkd, device, &createInfo);
		}

		{
			const vk::VkSamplerCreateInfo createInfo =
			{
				vk::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				DE_NULL,
				0u,

				vk::VK_FILTER_NEAREST,
				vk::VK_FILTER_NEAREST,

				vk::VK_SAMPLER_MIPMAP_MODE_LINEAR,
				vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				vk::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				0.0f,
				VK_FALSE,
				1.0f,
				VK_FALSE,
				vk::VK_COMPARE_OP_ALWAYS,
				0.0f,
				0.0f,
				vk::VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
				VK_FALSE
			};

			m_sampler = vk::createSampler(vkd, device, &createInfo);
		}

		{
			const vk::VkDescriptorImageInfo			imageInfo	=
			{
				*m_sampler,
				*m_imageView,
				context.getImageLayout()
			};
			const vk::VkWriteDescriptorSet			write		=
			{
				vk::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,
				*m_descriptorSet,
				0u,
				0u,
				1u,
				vk::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				&imageInfo,
				DE_NULL,
				DE_NULL,
			};

			vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
		}
	}
}

void RenderFragmentSampledImage::submit (SubmitContext& context)
{
	const vk::DeviceInterface&	vkd				= context.getContext().getDeviceInterface();
	const vk::VkCommandBuffer	commandBuffer	= context.getCommandBuffer();

	vkd.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipeline);

	vkd.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_resources.pipelineLayout, 0u, 1u, &(*m_descriptorSet), 0u, DE_NULL);
	vkd.cmdDraw(commandBuffer, 6u, 1u, 0u, 0u);
}

void RenderFragmentSampledImage::verify (VerifyRenderPassContext& context, size_t)
{
	const UVec2		size			= UVec2(context.getReferenceImage().getWidth(), context.getReferenceImage().getHeight());
	const deUint32	valuesPerPixel	= de::max<deUint32>(1u, (size.x() * size.y()) / (256u * 256u));

	for (int y = 0; y < context.getReferenceTarget().getSize().y(); y++)
	for (int x = 0; x < context.getReferenceTarget().getSize().x(); x++)
	{
		UVec4	value	= UVec4(x, y, 0u, 0u);

		for (deUint32 i = 0; i < valuesPerPixel; i++)
		{
			const UVec2	pos			= UVec2(value.z() * 256u + (value.x() ^ value.z()), value.w() * 256u + (value.y() ^ value.w()));
			const Vec4	floatValue	= context.getReferenceImage().getAccess().getPixel(pos.x() % size.x(), pos.y() % size.y());

			value = UVec4((deUint32)(floatValue.x() * 255.0f),
						  (deUint32)(floatValue.y() * 255.0f),
						  (deUint32)(floatValue.z() * 255.0f),
						  (deUint32)(floatValue.w() * 255.0f));

		}

		context.getReferenceTarget().getAccess().setPixel(value.asFloat() / Vec4(255.0f), x, y);
	}
}

enum Op
{
	OP_MAP,
	OP_UNMAP,

	OP_MAP_FLUSH,
	OP_MAP_INVALIDATE,

	OP_MAP_READ,
	OP_MAP_WRITE,
	OP_MAP_MODIFY,

	OP_BUFFER_CREATE,
	OP_BUFFER_DESTROY,
	OP_BUFFER_BINDMEMORY,

	OP_QUEUE_WAIT_FOR_IDLE,
	OP_DEVICE_WAIT_FOR_IDLE,

	OP_COMMAND_BUFFER_BEGIN,
	OP_COMMAND_BUFFER_END,

	// Buffer transfer operations
	OP_BUFFER_FILL,
	OP_BUFFER_UPDATE,

	OP_BUFFER_COPY_TO_BUFFER,
	OP_BUFFER_COPY_FROM_BUFFER,

	OP_BUFFER_COPY_TO_IMAGE,
	OP_BUFFER_COPY_FROM_IMAGE,

	OP_IMAGE_CREATE,
	OP_IMAGE_DESTROY,
	OP_IMAGE_BINDMEMORY,

	OP_IMAGE_TRANSITION_LAYOUT,

	OP_IMAGE_COPY_TO_BUFFER,
	OP_IMAGE_COPY_FROM_BUFFER,

	OP_IMAGE_COPY_TO_IMAGE,
	OP_IMAGE_COPY_FROM_IMAGE,

	OP_IMAGE_BLIT_TO_IMAGE,
	OP_IMAGE_BLIT_FROM_IMAGE,

	OP_IMAGE_RESOLVE,

	OP_PIPELINE_BARRIER_GLOBAL,
	OP_PIPELINE_BARRIER_BUFFER,
	OP_PIPELINE_BARRIER_IMAGE,

	// Renderpass operations
	OP_RENDERPASS_BEGIN,
	OP_RENDERPASS_END,

	// Commands inside render pass
	OP_RENDER_VERTEX_BUFFER,
	OP_RENDER_INDEX_BUFFER,

	OP_RENDER_VERTEX_UNIFORM_BUFFER,
	OP_RENDER_FRAGMENT_UNIFORM_BUFFER,

	OP_RENDER_VERTEX_UNIFORM_TEXEL_BUFFER,
	OP_RENDER_FRAGMENT_UNIFORM_TEXEL_BUFFER,

	OP_RENDER_VERTEX_STORAGE_BUFFER,
	OP_RENDER_FRAGMENT_STORAGE_BUFFER,

	OP_RENDER_VERTEX_STORAGE_TEXEL_BUFFER,
	OP_RENDER_FRAGMENT_STORAGE_TEXEL_BUFFER,

	OP_RENDER_VERTEX_STORAGE_IMAGE,
	OP_RENDER_FRAGMENT_STORAGE_IMAGE,

	OP_RENDER_VERTEX_SAMPLED_IMAGE,
	OP_RENDER_FRAGMENT_SAMPLED_IMAGE,
};

enum Stage
{
	STAGE_HOST,
	STAGE_COMMAND_BUFFER,

	STAGE_RENDER_PASS
};

vk::VkAccessFlags getWriteAccessFlags (void)
{
	return vk::VK_ACCESS_SHADER_WRITE_BIT
		| vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		| vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
		| vk::VK_ACCESS_TRANSFER_WRITE_BIT
		| vk::VK_ACCESS_HOST_WRITE_BIT
		| vk::VK_ACCESS_MEMORY_WRITE_BIT;
}

bool isWriteAccess (vk::VkAccessFlagBits access)
{
	return (getWriteAccessFlags() & access) != 0;
}

class CacheState
{
public:
									CacheState				(vk::VkPipelineStageFlags allowedStages, vk::VkAccessFlags allowedAccesses);

	bool							isValid					(vk::VkPipelineStageFlagBits	stage,
															 vk::VkAccessFlagBits			access) const;

	void							perform					(vk::VkPipelineStageFlagBits	stage,
															 vk::VkAccessFlagBits			access);

	void							submitCommandBuffer		(void);
	void							waitForIdle				(void);

	void							getFullBarrier			(vk::VkPipelineStageFlags&	srcStages,
															 vk::VkAccessFlags&			srcAccesses,
															 vk::VkPipelineStageFlags&	dstStages,
															 vk::VkAccessFlags&			dstAccesses) const;

	void							barrier					(vk::VkPipelineStageFlags	srcStages,
															 vk::VkAccessFlags			srcAccesses,
															 vk::VkPipelineStageFlags	dstStages,
															 vk::VkAccessFlags			dstAccesses);

	void							imageLayoutBarrier		(vk::VkPipelineStageFlags	srcStages,
															 vk::VkAccessFlags			srcAccesses,
															 vk::VkPipelineStageFlags	dstStages,
															 vk::VkAccessFlags			dstAccesses);

	void							checkImageLayoutBarrier	(vk::VkPipelineStageFlags	srcStages,
															 vk::VkAccessFlags			srcAccesses,
															 vk::VkPipelineStageFlags	dstStages,
															 vk::VkAccessFlags			dstAccesses);

	// Everything is clean and there is no need for barriers
	bool							isClean					(void) const;

	vk::VkPipelineStageFlags		getAllowedStages		(void) const { return m_allowedStages; }
	vk::VkAccessFlags				getAllowedAcceses		(void) const { return m_allowedAccesses; }
private:
	// Limit which stages and accesses are used by the CacheState tracker
	const vk::VkPipelineStageFlags	m_allowedStages;
	const vk::VkAccessFlags			m_allowedAccesses;

	// [dstStage][srcStage] = srcAccesses
	// In stage dstStage write srcAccesses from srcStage are not yet available
	vk::VkAccessFlags				m_unavailableWriteOperations[PIPELINESTAGE_LAST][PIPELINESTAGE_LAST];
	// Latest pipeline transition is not available in stage
	bool							m_unavailableLayoutTransition[PIPELINESTAGE_LAST];
	// [dstStage] = dstAccesses
	// In stage dstStage ops with dstAccesses are not yet visible
	vk::VkAccessFlags				m_invisibleOperations[PIPELINESTAGE_LAST];

	// [dstStage] = srcStage
	// Memory operation in srcStage have not completed before dstStage
	vk::VkPipelineStageFlags		m_incompleteOperations[PIPELINESTAGE_LAST];
};

CacheState::CacheState (vk::VkPipelineStageFlags allowedStages, vk::VkAccessFlags allowedAccesses)
	: m_allowedStages	(allowedStages)
	, m_allowedAccesses	(allowedAccesses)
{
	for (vk::VkPipelineStageFlags dstStage_ = 1; dstStage_ <= m_allowedStages; dstStage_ <<= 1)
	{
		const PipelineStage dstStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)dstStage_);

		if ((dstStage_ & m_allowedStages) == 0)
			continue;

		// All operations are initially visible
		m_invisibleOperations[dstStage] = 0;

		// There are no incomplete read operations initially
		m_incompleteOperations[dstStage] = 0;

		// There are no incomplete layout transitions
		m_unavailableLayoutTransition[dstStage] = false;

		for (vk::VkPipelineStageFlags srcStage_ = 1; srcStage_ <= m_allowedStages; srcStage_ <<= 1)
		{
			const PipelineStage srcStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)srcStage_);

			if ((srcStage_ & m_allowedStages) == 0)
				continue;

			// There are no write operations that are not yet available
			// initially.
			m_unavailableWriteOperations[dstStage][srcStage] = 0;
		}
	}
}

bool CacheState::isValid (vk::VkPipelineStageFlagBits	stage,
						  vk::VkAccessFlagBits			access) const
{
	DE_ASSERT((access & (~m_allowedAccesses)) == 0);
	DE_ASSERT((stage & (~m_allowedStages)) == 0);

	const PipelineStage	dstStage	= pipelineStageFlagToPipelineStage(stage);

	// Previous operations are not visible to access on stage
	if (m_unavailableLayoutTransition[dstStage] || (m_invisibleOperations[dstStage] & access) != 0)
		return false;

	if (isWriteAccess(access))
	{
		// Memory operations from other stages have not completed before
		// dstStage
		if (m_incompleteOperations[dstStage] != 0)
			return false;
	}

	return true;
}

void CacheState::perform (vk::VkPipelineStageFlagBits	stage,
						  vk::VkAccessFlagBits			access)
{
	DE_ASSERT((access & (~m_allowedAccesses)) == 0);
	DE_ASSERT((stage & (~m_allowedStages)) == 0);

	const PipelineStage srcStage = pipelineStageFlagToPipelineStage(stage);

	for (vk::VkPipelineStageFlags dstStage_ = 1; dstStage_ <= m_allowedStages; dstStage_ <<= 1)
	{
		const PipelineStage dstStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)dstStage_);

		if ((dstStage_ & m_allowedStages) == 0)
			continue;

		// Mark stage as incomplete for all stages
		m_incompleteOperations[dstStage] |= stage;

		if (isWriteAccess(access))
		{
			// Mark all accesses from all stages invisible
			m_invisibleOperations[dstStage] |= m_allowedAccesses;

			// Mark write access from srcStage unavailable to all stages
			m_unavailableWriteOperations[dstStage][srcStage] |= access;
		}
	}
}

void CacheState::submitCommandBuffer (void)
{
	// Flush all host writes and reads
	barrier(m_allowedStages & vk::VK_PIPELINE_STAGE_HOST_BIT,
			m_allowedAccesses & (vk::VK_ACCESS_HOST_READ_BIT | vk::VK_ACCESS_HOST_WRITE_BIT),
			m_allowedStages,
			m_allowedAccesses);
}

void CacheState::waitForIdle (void)
{
	// Make all writes available
	barrier(m_allowedStages,
			m_allowedAccesses & getWriteAccessFlags(),
			m_allowedStages,
			0);

	// Make all writes visible on device side
	barrier(m_allowedStages,
			0,
			m_allowedStages & (~vk::VK_PIPELINE_STAGE_HOST_BIT),
			m_allowedAccesses);
}

void CacheState::getFullBarrier (vk::VkPipelineStageFlags&	srcStages,
								 vk::VkAccessFlags&			srcAccesses,
								 vk::VkPipelineStageFlags&	dstStages,
								 vk::VkAccessFlags&			dstAccesses) const
{
	srcStages	= 0;
	srcAccesses	= 0;
	dstStages	= 0;
	dstAccesses	= 0;

	for (vk::VkPipelineStageFlags dstStage_ = 1; dstStage_ <= m_allowedStages; dstStage_ <<= 1)
	{
		const PipelineStage dstStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)dstStage_);

		if ((dstStage_ & m_allowedStages) == 0)
			continue;

		// Make sure all previous operation are complete in all stages
		if (m_incompleteOperations[dstStage])
		{
			dstStages |= dstStage_;
			srcStages |= m_incompleteOperations[dstStage];
		}

		// Make sure all read operations are visible in dstStage
		if (m_invisibleOperations[dstStage])
		{
			dstStages |= dstStage_;
			dstAccesses |= m_invisibleOperations[dstStage];
		}

		// Make sure all write operations fro mall stages are available
		for (vk::VkPipelineStageFlags srcStage_ = 1; srcStage_ <= m_allowedStages; srcStage_ <<= 1)
		{
			const PipelineStage srcStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)srcStage_);

			if ((srcStage_ & m_allowedStages) == 0)
				continue;

			if (m_unavailableWriteOperations[dstStage][srcStage])
			{
				dstStages |= dstStage_;
				srcStages |= dstStage_;
				srcAccesses |= m_unavailableWriteOperations[dstStage][srcStage];
			}

			if (m_unavailableLayoutTransition[dstStage] && !m_unavailableLayoutTransition[srcStage])
			{
				// Add dependency between srcStage and dstStage if layout transition has not completed in dstStage,
				// but has completed in srcStage.
				dstStages |= dstStage_;
				srcStages |= dstStage_;
			}
		}
	}

	DE_ASSERT((srcStages & (~m_allowedStages)) == 0);
	DE_ASSERT((srcAccesses & (~m_allowedAccesses)) == 0);
	DE_ASSERT((dstStages & (~m_allowedStages)) == 0);
	DE_ASSERT((dstAccesses & (~m_allowedAccesses)) == 0);
}

void CacheState::checkImageLayoutBarrier (vk::VkPipelineStageFlags	srcStages,
										 vk::VkAccessFlags			srcAccesses,
										 vk::VkPipelineStageFlags	dstStages,
										 vk::VkAccessFlags			dstAccesses)
{
	DE_ASSERT((srcStages & (~m_allowedStages)) == 0);
	DE_ASSERT((srcAccesses & (~m_allowedAccesses)) == 0);
	DE_ASSERT((dstStages & (~m_allowedStages)) == 0);
	DE_ASSERT((dstAccesses & (~m_allowedAccesses)) == 0);

	DE_UNREF(srcStages);
	DE_UNREF(srcAccesses);

	DE_UNREF(dstStages);
	DE_UNREF(dstAccesses);

#if defined(DE_DEBUG)
	// Check that all stages have completed before srcStages or are in srcStages.
	{
		vk::VkPipelineStageFlags completedStages = srcStages;

		for (vk::VkPipelineStageFlags srcStage_ = 1; srcStage_ <= srcStages; srcStage_ <<= 1)
		{
			const PipelineStage srcStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)srcStage_);

			if ((srcStage_ & srcStages) == 0)
				continue;

			completedStages |= (~m_incompleteOperations[srcStage]);
		}

		DE_ASSERT((completedStages & m_allowedStages) == m_allowedStages);
	}

	// Check that any write is available at least in one stage. Since all stages are complete even single flush is enough.
	if ((getWriteAccessFlags() & m_allowedAccesses) != 0 && (srcAccesses & getWriteAccessFlags()) == 0)
	{
		bool anyWriteAvailable = false;

		for (vk::VkPipelineStageFlags dstStage_ = 1; dstStage_ <= m_allowedStages; dstStage_ <<= 1)
		{
			const PipelineStage dstStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)dstStage_);

			if ((dstStage_ & m_allowedStages) == 0)
				continue;

			for (vk::VkPipelineStageFlags srcStage_ = 1; srcStage_ <= m_allowedStages; srcStage_ <<= 1)
			{
				const PipelineStage srcStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)srcStage_);

				if ((srcStage_ & m_allowedStages) == 0)
					continue;

				if (m_unavailableWriteOperations[dstStage][srcStage] != (getWriteAccessFlags() & m_allowedAccesses))
				{
					anyWriteAvailable = true;
					break;
				}
			}
		}

		DE_ASSERT(anyWriteAvailable);
	}
#endif
}

void CacheState::imageLayoutBarrier (vk::VkPipelineStageFlags	srcStages,
									 vk::VkAccessFlags			srcAccesses,
									 vk::VkPipelineStageFlags	dstStages,
									 vk::VkAccessFlags			dstAccesses)
{
	checkImageLayoutBarrier(srcStages, srcAccesses, dstStages, dstAccesses);

	for (vk::VkPipelineStageFlags dstStage_ = 1; dstStage_ <= m_allowedStages; dstStage_ <<= 1)
	{
		const PipelineStage dstStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)dstStage_);

		if ((dstStage_ & m_allowedStages) == 0)
			continue;

		// All stages are incomplete after the barrier except each dstStage in it self.
		m_incompleteOperations[dstStage] = m_allowedStages & (~dstStage_);

		// All memory operations are invisible unless they are listed in dstAccess
		m_invisibleOperations[dstStage] = m_allowedAccesses & (~dstAccesses);

		// Layout transition is unavailable in stage unless it was listed in dstStages
		m_unavailableLayoutTransition[dstStage]= (dstStage_ & dstStages) == 0;

		for (vk::VkPipelineStageFlags srcStage_ = 1; srcStage_ <= m_allowedStages; srcStage_ <<= 1)
		{
			const PipelineStage srcStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)srcStage_);

			if ((srcStage_ & m_allowedStages) == 0)
				continue;

			// All write operations are available after layout transition
			m_unavailableWriteOperations[dstStage][srcStage] = 0;
		}
	}
}

void CacheState::barrier (vk::VkPipelineStageFlags	srcStages,
						  vk::VkAccessFlags			srcAccesses,
						  vk::VkPipelineStageFlags	dstStages,
						  vk::VkAccessFlags			dstAccesses)
{
	DE_ASSERT((srcStages & (~m_allowedStages)) == 0);
	DE_ASSERT((srcAccesses & (~m_allowedAccesses)) == 0);
	DE_ASSERT((dstStages & (~m_allowedStages)) == 0);
	DE_ASSERT((dstAccesses & (~m_allowedAccesses)) == 0);

	// Transitivity
	{
		vk::VkPipelineStageFlags		oldIncompleteOperations[PIPELINESTAGE_LAST];
		vk::VkAccessFlags				oldUnavailableWriteOperations[PIPELINESTAGE_LAST][PIPELINESTAGE_LAST];
		bool							oldUnavailableLayoutTransition[PIPELINESTAGE_LAST];

		deMemcpy(oldIncompleteOperations, m_incompleteOperations, sizeof(oldIncompleteOperations));
		deMemcpy(oldUnavailableWriteOperations, m_unavailableWriteOperations, sizeof(oldUnavailableWriteOperations));
		deMemcpy(oldUnavailableLayoutTransition, m_unavailableLayoutTransition, sizeof(oldUnavailableLayoutTransition));

		for (vk::VkPipelineStageFlags srcStage_ = 1; srcStage_ <= srcStages; srcStage_ <<= 1)
		{
			const PipelineStage srcStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)srcStage_);

			if ((srcStage_ & srcStages) == 0)
				continue;

			for (vk::VkPipelineStageFlags dstStage_ = 1; dstStage_ <= dstStages; dstStage_ <<= 1)
			{
				const PipelineStage	dstStage			= pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)dstStage_);

				if ((dstStage_ & dstStages) == 0)
					continue;

				// Stages that have completed before srcStage have also completed before dstStage
				m_incompleteOperations[dstStage] &= oldIncompleteOperations[srcStage];

				// Image layout transition in srcStage are now available in dstStage
				m_unavailableLayoutTransition[dstStage] &= oldUnavailableLayoutTransition[srcStage];

				for (vk::VkPipelineStageFlags sharedStage_ = 1; sharedStage_ <= m_allowedStages; sharedStage_ <<= 1)
				{
					const PipelineStage	sharedStage			= pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)sharedStage_);

					if ((sharedStage_ & m_allowedStages) == 0)
						continue;

					// Writes that are available in srcStage are also available in dstStage
					m_unavailableWriteOperations[dstStage][sharedStage] &= oldUnavailableWriteOperations[srcStage][sharedStage];
				}
			}
		}
	}

	// Barrier
	for (vk::VkPipelineStageFlags dstStage_ = 1; dstStage_ <= dstStages; dstStage_ <<= 1)
	{
		const PipelineStage	dstStage			= pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)dstStage_);
		bool				allWritesAvailable	= true;

		if ((dstStage_ & dstStages) == 0)
			continue;

		// Operations in srcStages have completed before any stage in dstStages
		m_incompleteOperations[dstStage] &= ~srcStages;

		for (vk::VkPipelineStageFlags srcStage_ = 1; srcStage_ <= m_allowedStages; srcStage_ <<= 1)
		{
			const PipelineStage srcStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)srcStage_);

			if ((srcStage_ & m_allowedStages) == 0)
				continue;

			// Make srcAccesses from srcStage available in dstStage
			if ((srcStage_ & srcStages) != 0)
				m_unavailableWriteOperations[dstStage][srcStage] &= ~srcAccesses;

			if (m_unavailableWriteOperations[dstStage][srcStage] != 0)
				allWritesAvailable = false;
		}

		// If all writes are available in dstStage make dstAccesses also visible
		if (allWritesAvailable)
			m_invisibleOperations[dstStage] &= ~dstAccesses;
	}
}

bool CacheState::isClean (void) const
{
	for (vk::VkPipelineStageFlags dstStage_ = 1; dstStage_ <= m_allowedStages; dstStage_ <<= 1)
	{
		const PipelineStage dstStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)dstStage_);

		if ((dstStage_ & m_allowedStages) == 0)
			continue;

		// Some operations are not visible to some stages
		if (m_invisibleOperations[dstStage] != 0)
			return false;

		// There are operation that have not completed yet
		if (m_incompleteOperations[dstStage] != 0)
			return false;

		// Layout transition has not completed yet
		if (m_unavailableLayoutTransition[dstStage])
			return false;

		for (vk::VkPipelineStageFlags srcStage_ = 1; srcStage_ <= m_allowedStages; srcStage_ <<= 1)
		{
			const PipelineStage srcStage = pipelineStageFlagToPipelineStage((vk::VkPipelineStageFlagBits)srcStage_);

			if ((srcStage_ & m_allowedStages) == 0)
				continue;

			// Some write operations are not available yet
			if (m_unavailableWriteOperations[dstStage][srcStage] != 0)
				return false;
		}
	}

	return true;
}

bool layoutSupportedByUsage (Usage usage, vk::VkImageLayout layout)
{
	switch (layout)
	{
		case vk::VK_IMAGE_LAYOUT_GENERAL:
			return true;

		case vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return (usage & USAGE_COLOR_ATTACHMENT) != 0;

		case vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			return (usage & USAGE_DEPTH_STENCIL_ATTACHMENT) != 0;

		case vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			return (usage & USAGE_DEPTH_STENCIL_ATTACHMENT) != 0;

		case vk::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// \todo [2016-03-09 mika] Should include input attachment
			return (usage & USAGE_SAMPLED_IMAGE) != 0;

		case vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return (usage & USAGE_TRANSFER_SRC) != 0;

		case vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			return (usage & USAGE_TRANSFER_DST) != 0;

		case vk::VK_IMAGE_LAYOUT_PREINITIALIZED:
			return true;

		default:
			DE_FATAL("Unknown layout");
			return false;
	}
}

size_t getNumberOfSupportedLayouts (Usage usage)
{
	const vk::VkImageLayout layouts[] =
	{
		vk::VK_IMAGE_LAYOUT_GENERAL,
		vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	};
	size_t supportedLayoutCount = 0;

	for (size_t layoutNdx = 0; layoutNdx < DE_LENGTH_OF_ARRAY(layouts); layoutNdx++)
	{
		const vk::VkImageLayout layout = layouts[layoutNdx];

		if (layoutSupportedByUsage(usage, layout))
			supportedLayoutCount++;
	}

	return supportedLayoutCount;
}

vk::VkImageLayout getRandomNextLayout (de::Random&			rng,
									   Usage				usage,
									   vk::VkImageLayout	previousLayout)
{
	const vk::VkImageLayout	layouts[] =
	{
		vk::VK_IMAGE_LAYOUT_GENERAL,
		vk::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	};
	const size_t			supportedLayoutCount = getNumberOfSupportedLayouts(usage);

	DE_ASSERT(supportedLayoutCount > 0);

	size_t nextLayoutNdx = ((size_t)rng.getUint64()) % (previousLayout == vk::VK_IMAGE_LAYOUT_UNDEFINED
														? supportedLayoutCount
														: supportedLayoutCount - 1);

	for (size_t layoutNdx = 0; layoutNdx < DE_LENGTH_OF_ARRAY(layouts); layoutNdx++)
	{
		const vk::VkImageLayout layout = layouts[layoutNdx];

		if (layoutSupportedByUsage(usage, layout) && layout != previousLayout)
		{
			if (nextLayoutNdx == 0)
				return layout;
			else
				nextLayoutNdx--;
		}
	}

	DE_FATAL("Unreachable");
	return vk::VK_IMAGE_LAYOUT_UNDEFINED;
}

struct State
{
	State (Usage usage, deUint32 seed)
		: stage					(STAGE_HOST)
		, cache					(usageToStageFlags(usage), usageToAccessFlags(usage))
		, rng					(seed)
		, mapped				(false)
		, hostInvalidated		(true)
		, hostFlushed			(true)
		, memoryDefined			(false)
		, hasBuffer				(false)
		, hasBoundBufferMemory	(false)
		, hasImage				(false)
		, hasBoundImageMemory	(false)
		, imageLayout			(vk::VK_IMAGE_LAYOUT_UNDEFINED)
		, imageDefined			(false)
		, queueIdle				(true)
		, deviceIdle			(true)
		, commandBufferIsEmpty	(true)
		, renderPassIsEmpty		(true)
	{
	}

	Stage				stage;
	CacheState			cache;
	de::Random			rng;

	bool				mapped;
	bool				hostInvalidated;
	bool				hostFlushed;
	bool				memoryDefined;

	bool				hasBuffer;
	bool				hasBoundBufferMemory;

	bool				hasImage;
	bool				hasBoundImageMemory;
	vk::VkImageLayout	imageLayout;
	bool				imageDefined;

	bool				queueIdle;
	bool				deviceIdle;

	bool				commandBufferIsEmpty;
	bool				renderPassIsEmpty;
};

void getAvailableOps (const State& state, bool supportsBuffers, bool supportsImages, Usage usage, vector<Op>& ops)
{
	if (state.stage == STAGE_HOST)
	{
		if (usage & (USAGE_HOST_READ | USAGE_HOST_WRITE))
		{
			// Host memory operations
			if (state.mapped)
			{
				ops.push_back(OP_UNMAP);

				// Avoid flush and finish if they are not needed
				if (!state.hostFlushed)
					ops.push_back(OP_MAP_FLUSH);

				if (!state.hostInvalidated
					&& state.queueIdle
					&& ((usage & USAGE_HOST_READ) == 0
						|| state.cache.isValid(vk::VK_PIPELINE_STAGE_HOST_BIT, vk::VK_ACCESS_HOST_READ_BIT))
					&& ((usage & USAGE_HOST_WRITE) == 0
						|| state.cache.isValid(vk::VK_PIPELINE_STAGE_HOST_BIT, vk::VK_ACCESS_HOST_WRITE_BIT)))
				{
					ops.push_back(OP_MAP_INVALIDATE);
				}

				if (usage & USAGE_HOST_READ
					&& usage & USAGE_HOST_WRITE
					&& state.memoryDefined
					&& state.hostInvalidated
					&& state.queueIdle
					&& state.cache.isValid(vk::VK_PIPELINE_STAGE_HOST_BIT, vk::VK_ACCESS_HOST_WRITE_BIT)
					&& state.cache.isValid(vk::VK_PIPELINE_STAGE_HOST_BIT, vk::VK_ACCESS_HOST_READ_BIT))
				{
					ops.push_back(OP_MAP_MODIFY);
				}

				if (usage & USAGE_HOST_READ
					&& state.memoryDefined
					&& state.hostInvalidated
					&& state.queueIdle
					&& state.cache.isValid(vk::VK_PIPELINE_STAGE_HOST_BIT, vk::VK_ACCESS_HOST_READ_BIT))
				{
					ops.push_back(OP_MAP_READ);
				}

				if (usage & USAGE_HOST_WRITE
					&& state.hostInvalidated
					&& state.queueIdle
					&& state.cache.isValid(vk::VK_PIPELINE_STAGE_HOST_BIT, vk::VK_ACCESS_HOST_WRITE_BIT))
				{
					ops.push_back(OP_MAP_WRITE);
				}
			}
			else
				ops.push_back(OP_MAP);
		}

		if (state.hasBoundBufferMemory && state.queueIdle)
		{
			// \note Destroy only buffers after they have been bound
			ops.push_back(OP_BUFFER_DESTROY);
		}
		else
		{
			if (state.hasBuffer)
			{
				if (!state.hasBoundBufferMemory)
					ops.push_back(OP_BUFFER_BINDMEMORY);
			}
			else if (!state.hasImage && supportsBuffers)	// Avoid creating buffer if there is already image
				ops.push_back(OP_BUFFER_CREATE);
		}

		if (state.hasBoundImageMemory && state.queueIdle)
		{
			// \note Destroy only image after they have been bound
			ops.push_back(OP_IMAGE_DESTROY);
		}
		else
		{
			if (state.hasImage)
			{
				if (!state.hasBoundImageMemory)
					ops.push_back(OP_IMAGE_BINDMEMORY);
			}
			else if (!state.hasBuffer && supportsImages)	// Avoid creating image if there is already buffer
				ops.push_back(OP_IMAGE_CREATE);
		}

		// Host writes must be flushed before GPU commands and there must be
		// buffer or image for GPU commands
		if (state.hostFlushed
			&& (state.memoryDefined || supportsDeviceBufferWrites(usage) || state.imageDefined || supportsDeviceImageWrites(usage))
			&& (state.hasBoundBufferMemory || state.hasBoundImageMemory) // Avoid command buffers if there is no object to use
			&& (usageToStageFlags(usage) & (~vk::VK_PIPELINE_STAGE_HOST_BIT)) != 0) // Don't start command buffer if there are no ways to use memory from gpu
		{
			ops.push_back(OP_COMMAND_BUFFER_BEGIN);
		}

		if (!state.deviceIdle)
			ops.push_back(OP_DEVICE_WAIT_FOR_IDLE);

		if (!state.queueIdle)
			ops.push_back(OP_QUEUE_WAIT_FOR_IDLE);
	}
	else if (state.stage == STAGE_COMMAND_BUFFER)
	{
		if (!state.cache.isClean())
		{
			ops.push_back(OP_PIPELINE_BARRIER_GLOBAL);

			if (state.hasImage)
				ops.push_back(OP_PIPELINE_BARRIER_IMAGE);

			if (state.hasBuffer)
				ops.push_back(OP_PIPELINE_BARRIER_BUFFER);
		}

		if (state.hasBoundBufferMemory)
		{
			if (usage & USAGE_TRANSFER_DST
				&& state.cache.isValid(vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_ACCESS_TRANSFER_WRITE_BIT))
			{
				ops.push_back(OP_BUFFER_FILL);
				ops.push_back(OP_BUFFER_UPDATE);
				ops.push_back(OP_BUFFER_COPY_FROM_BUFFER);
				ops.push_back(OP_BUFFER_COPY_FROM_IMAGE);
			}

			if (usage & USAGE_TRANSFER_SRC
				&& state.memoryDefined
				&& state.cache.isValid(vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_ACCESS_TRANSFER_READ_BIT))
			{
				ops.push_back(OP_BUFFER_COPY_TO_BUFFER);
				ops.push_back(OP_BUFFER_COPY_TO_IMAGE);
			}
		}

		if (state.hasBoundImageMemory
			&& (state.imageLayout == vk::VK_IMAGE_LAYOUT_UNDEFINED
				|| getNumberOfSupportedLayouts(usage) > 1))
		{
			ops.push_back(OP_IMAGE_TRANSITION_LAYOUT);

			{
				if (usage & USAGE_TRANSFER_DST
					&& (state.imageLayout == vk::VK_IMAGE_LAYOUT_GENERAL
						|| state.imageLayout == vk::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
					&& state.cache.isValid(vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_ACCESS_TRANSFER_WRITE_BIT))
				{
					ops.push_back(OP_IMAGE_COPY_FROM_BUFFER);
					ops.push_back(OP_IMAGE_COPY_FROM_IMAGE);
					ops.push_back(OP_IMAGE_BLIT_FROM_IMAGE);
				}

				if (usage & USAGE_TRANSFER_SRC
					&& (state.imageLayout == vk::VK_IMAGE_LAYOUT_GENERAL
						|| state.imageLayout == vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
					&& state.imageDefined
					&& state.cache.isValid(vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_ACCESS_TRANSFER_READ_BIT))
				{
					ops.push_back(OP_IMAGE_COPY_TO_BUFFER);
					ops.push_back(OP_IMAGE_COPY_TO_IMAGE);
					ops.push_back(OP_IMAGE_BLIT_TO_IMAGE);
				}
			}
		}

		// \todo [2016-03-09 mika] Add other usages?
		if ((state.memoryDefined
				&& state.hasBoundBufferMemory
				&& (((usage & USAGE_VERTEX_BUFFER)
					&& state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, vk::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT))
				|| ((usage & USAGE_INDEX_BUFFER)
					&& state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, vk::VK_ACCESS_INDEX_READ_BIT))
				|| ((usage & USAGE_UNIFORM_BUFFER)
					&& (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT)
						|| state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT)))
				|| ((usage & USAGE_UNIFORM_TEXEL_BUFFER)
					&& (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT)
						|| state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT)))
				|| ((usage & USAGE_STORAGE_BUFFER)
					&& (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT)
						|| state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT)))
				|| ((usage & USAGE_STORAGE_TEXEL_BUFFER)
					&& state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))))
			|| (state.imageDefined
				&& state.hasBoundImageMemory
				&& (((usage & USAGE_STORAGE_IMAGE)
						&& state.imageLayout == vk::VK_IMAGE_LAYOUT_GENERAL
						&& (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT)
							|| state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT)))
					|| ((usage & USAGE_SAMPLED_IMAGE)
						&& (state.imageLayout == vk::VK_IMAGE_LAYOUT_GENERAL
							|| state.imageLayout == vk::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
						&& (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT)
							|| state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))))))
		{
			ops.push_back(OP_RENDERPASS_BEGIN);
		}

		// \note This depends on previous operations and has to be always the
		// last command buffer operation check
		if (ops.empty() || !state.commandBufferIsEmpty)
			ops.push_back(OP_COMMAND_BUFFER_END);
	}
	else if (state.stage == STAGE_RENDER_PASS)
	{
		if ((usage & USAGE_VERTEX_BUFFER) != 0
			&& state.memoryDefined
			&& state.hasBoundBufferMemory
			&& state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, vk::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT))
		{
			ops.push_back(OP_RENDER_VERTEX_BUFFER);
		}

		if ((usage & USAGE_INDEX_BUFFER) != 0
			&& state.memoryDefined
			&& state.hasBoundBufferMemory
			&& state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, vk::VK_ACCESS_INDEX_READ_BIT))
		{
			ops.push_back(OP_RENDER_INDEX_BUFFER);
		}

		if ((usage & USAGE_UNIFORM_BUFFER) != 0
			&& state.memoryDefined
			&& state.hasBoundBufferMemory)
		{
			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT))
				ops.push_back(OP_RENDER_VERTEX_UNIFORM_BUFFER);

			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT))
				ops.push_back(OP_RENDER_FRAGMENT_UNIFORM_BUFFER);
		}

		if ((usage & USAGE_UNIFORM_TEXEL_BUFFER) != 0
			&& state.memoryDefined
			&& state.hasBoundBufferMemory)
		{
			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT))
				ops.push_back(OP_RENDER_VERTEX_UNIFORM_TEXEL_BUFFER);

			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT))
				ops.push_back(OP_RENDER_FRAGMENT_UNIFORM_TEXEL_BUFFER);
		}

		if ((usage & USAGE_STORAGE_BUFFER) != 0
			&& state.memoryDefined
			&& state.hasBoundBufferMemory)
		{
			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))
				ops.push_back(OP_RENDER_VERTEX_STORAGE_BUFFER);

			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))
				ops.push_back(OP_RENDER_FRAGMENT_STORAGE_BUFFER);
		}

		if ((usage & USAGE_STORAGE_TEXEL_BUFFER) != 0
			&& state.memoryDefined
			&& state.hasBoundBufferMemory)
		{
			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))
				ops.push_back(OP_RENDER_VERTEX_STORAGE_TEXEL_BUFFER);

			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))
				ops.push_back(OP_RENDER_FRAGMENT_STORAGE_TEXEL_BUFFER);
		}

		if ((usage & USAGE_STORAGE_IMAGE) != 0
			&& state.imageDefined
			&& state.hasBoundImageMemory
			&& (state.imageLayout == vk::VK_IMAGE_LAYOUT_GENERAL))
		{
			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))
				ops.push_back(OP_RENDER_VERTEX_STORAGE_IMAGE);

			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))
				ops.push_back(OP_RENDER_FRAGMENT_STORAGE_IMAGE);
		}

		if ((usage & USAGE_SAMPLED_IMAGE) != 0
			&& state.imageDefined
			&& state.hasBoundImageMemory
			&& (state.imageLayout == vk::VK_IMAGE_LAYOUT_GENERAL
				|| state.imageLayout == vk::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
		{
			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))
				ops.push_back(OP_RENDER_VERTEX_SAMPLED_IMAGE);

			if (state.cache.isValid(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT))
				ops.push_back(OP_RENDER_FRAGMENT_SAMPLED_IMAGE);
		}

		if (!state.renderPassIsEmpty)
			ops.push_back(OP_RENDERPASS_END);
	}
	else
		DE_FATAL("Unknown stage");
}

void removeIllegalAccessFlags (vk::VkAccessFlags& accessflags, vk::VkPipelineStageFlags stageflags)
{
	if (!(stageflags & vk::VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT))
		accessflags &= ~vk::VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

	if (!(stageflags & vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT))
		accessflags &= ~vk::VK_ACCESS_INDEX_READ_BIT;

	if (!(stageflags & vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT))
		accessflags &= ~vk::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

	if (!(stageflags & (vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)))
		accessflags &= ~vk::VK_ACCESS_UNIFORM_READ_BIT;

	if (!(stageflags & vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT))
		accessflags &= ~vk::VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

	if (!(stageflags & (vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)))
		accessflags &= ~vk::VK_ACCESS_SHADER_READ_BIT;

	if (!(stageflags & (vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
						vk::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)))
		accessflags &= ~vk::VK_ACCESS_SHADER_WRITE_BIT;

	if (!(stageflags & vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT))
		accessflags &= ~vk::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

	if (!(stageflags & vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT))
		accessflags &= ~vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	if (!(stageflags & (vk::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
						vk::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT)))
		accessflags &= ~vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	if (!(stageflags & (vk::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
						vk::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT)))
		accessflags &= ~vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	if (!(stageflags & vk::VK_PIPELINE_STAGE_TRANSFER_BIT))
		accessflags &= ~vk::VK_ACCESS_TRANSFER_READ_BIT;

	if (!(stageflags & vk::VK_PIPELINE_STAGE_TRANSFER_BIT))
		accessflags &= ~vk::VK_ACCESS_TRANSFER_WRITE_BIT;

	if (!(stageflags & vk::VK_PIPELINE_STAGE_HOST_BIT))
		accessflags &= ~vk::VK_ACCESS_HOST_READ_BIT;

	if (!(stageflags & vk::VK_PIPELINE_STAGE_HOST_BIT))
		accessflags &= ~vk::VK_ACCESS_HOST_WRITE_BIT;
}

void applyOp (State& state, const Memory& memory, Op op, Usage usage)
{
	switch (op)
	{
		case OP_MAP:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(!state.mapped);
			state.mapped = true;
			break;

		case OP_UNMAP:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(state.mapped);
			state.mapped = false;
			break;

		case OP_MAP_FLUSH:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(!state.hostFlushed);
			state.hostFlushed = true;
			break;

		case OP_MAP_INVALIDATE:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(!state.hostInvalidated);
			state.hostInvalidated = true;
			break;

		case OP_MAP_READ:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(state.hostInvalidated);
			state.rng.getUint32();
			break;

		case OP_MAP_WRITE:
			DE_ASSERT(state.stage == STAGE_HOST);
			if ((memory.getMemoryType().propertyFlags & vk::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
				state.hostFlushed = false;

			state.memoryDefined = true;
			state.imageDefined = false;
			state.imageLayout = vk::VK_IMAGE_LAYOUT_UNDEFINED;
			state.rng.getUint32();
			break;

		case OP_MAP_MODIFY:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(state.hostInvalidated);

			if ((memory.getMemoryType().propertyFlags & vk::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
				state.hostFlushed = false;

			state.rng.getUint32();
			break;

		case OP_BUFFER_CREATE:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(!state.hasBuffer);

			state.hasBuffer = true;
			break;

		case OP_BUFFER_DESTROY:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(state.hasBuffer);
			DE_ASSERT(state.hasBoundBufferMemory);

			state.hasBuffer = false;
			state.hasBoundBufferMemory = false;
			break;

		case OP_BUFFER_BINDMEMORY:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(state.hasBuffer);
			DE_ASSERT(!state.hasBoundBufferMemory);

			state.hasBoundBufferMemory = true;
			break;

		case OP_IMAGE_CREATE:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(!state.hasImage);
			DE_ASSERT(!state.hasBuffer);

			state.hasImage = true;
			break;

		case OP_IMAGE_DESTROY:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(state.hasImage);
			DE_ASSERT(state.hasBoundImageMemory);

			state.hasImage = false;
			state.hasBoundImageMemory = false;
			state.imageLayout = vk::VK_IMAGE_LAYOUT_UNDEFINED;
			state.imageDefined = false;
			break;

		case OP_IMAGE_BINDMEMORY:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(state.hasImage);
			DE_ASSERT(!state.hasBoundImageMemory);

			state.hasBoundImageMemory = true;
			break;

		case OP_IMAGE_TRANSITION_LAYOUT:
		{
			DE_ASSERT(state.stage == STAGE_COMMAND_BUFFER);
			DE_ASSERT(state.hasImage);
			DE_ASSERT(state.hasBoundImageMemory);

			// \todo [2016-03-09 mika] Support linear tiling and predefined data
			const vk::VkImageLayout		srcLayout	= state.rng.getFloat() < 0.9f ? state.imageLayout : vk::VK_IMAGE_LAYOUT_UNDEFINED;
			const vk::VkImageLayout		dstLayout	= getRandomNextLayout(state.rng, usage, srcLayout);

			vk::VkPipelineStageFlags	dirtySrcStages;
			vk::VkAccessFlags			dirtySrcAccesses;
			vk::VkPipelineStageFlags	dirtyDstStages;
			vk::VkAccessFlags			dirtyDstAccesses;

			vk::VkPipelineStageFlags	srcStages;
			vk::VkAccessFlags			srcAccesses;
			vk::VkPipelineStageFlags	dstStages;
			vk::VkAccessFlags			dstAccesses;

			state.cache.getFullBarrier(dirtySrcStages, dirtySrcAccesses, dirtyDstStages, dirtyDstAccesses);

			// Try masking some random bits
			srcStages	= dirtySrcStages;
			srcAccesses	= dirtySrcAccesses;

			dstStages	= state.cache.getAllowedStages() & state.rng.getUint32();
			dstAccesses	= state.cache.getAllowedAcceses() & state.rng.getUint32();

			// If there are no bits in dst stage mask use all stages
			dstStages	= dstStages ? dstStages : state.cache.getAllowedStages();

			if (!srcStages)
				srcStages = dstStages;

			removeIllegalAccessFlags(dstAccesses, dstStages);
			removeIllegalAccessFlags(srcAccesses, srcStages);

			if (srcLayout == vk::VK_IMAGE_LAYOUT_UNDEFINED)
				state.imageDefined = false;

			state.commandBufferIsEmpty = false;
			state.imageLayout = dstLayout;
			state.memoryDefined = false;
			state.cache.imageLayoutBarrier(srcStages, srcAccesses, dstStages, dstAccesses);
			break;
		}

		case OP_QUEUE_WAIT_FOR_IDLE:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(!state.queueIdle);

			state.queueIdle = true;

			state.cache.waitForIdle();
			break;

		case OP_DEVICE_WAIT_FOR_IDLE:
			DE_ASSERT(state.stage == STAGE_HOST);
			DE_ASSERT(!state.deviceIdle);

			state.queueIdle = true;
			state.deviceIdle = true;

			state.cache.waitForIdle();
			break;

		case OP_COMMAND_BUFFER_BEGIN:
			DE_ASSERT(state.stage == STAGE_HOST);
			state.stage = STAGE_COMMAND_BUFFER;
			state.commandBufferIsEmpty = true;
			// Makes host writes visible to command buffer
			state.cache.submitCommandBuffer();
			break;

		case OP_COMMAND_BUFFER_END:
			DE_ASSERT(state.stage == STAGE_COMMAND_BUFFER);
			state.stage = STAGE_HOST;
			state.queueIdle = false;
			state.deviceIdle = false;
			break;

		case OP_BUFFER_COPY_FROM_BUFFER:
		case OP_BUFFER_COPY_FROM_IMAGE:
		case OP_BUFFER_UPDATE:
		case OP_BUFFER_FILL:
			state.rng.getUint32();
			DE_ASSERT(state.stage == STAGE_COMMAND_BUFFER);

			if ((memory.getMemoryType().propertyFlags & vk::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
				state.hostInvalidated = false;

			state.commandBufferIsEmpty = false;
			state.memoryDefined = true;
			state.imageDefined = false;
			state.imageLayout = vk::VK_IMAGE_LAYOUT_UNDEFINED;
			state.cache.perform(vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_ACCESS_TRANSFER_WRITE_BIT);
			break;

		case OP_BUFFER_COPY_TO_BUFFER:
		case OP_BUFFER_COPY_TO_IMAGE:
			DE_ASSERT(state.stage == STAGE_COMMAND_BUFFER);

			state.commandBufferIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_ACCESS_TRANSFER_READ_BIT);
			break;

		case OP_IMAGE_BLIT_FROM_IMAGE:
			state.rng.getBool();
			// Fall through
		case OP_IMAGE_COPY_FROM_BUFFER:
		case OP_IMAGE_COPY_FROM_IMAGE:
			state.rng.getUint32();
			DE_ASSERT(state.stage == STAGE_COMMAND_BUFFER);

			if ((memory.getMemoryType().propertyFlags & vk::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
				state.hostInvalidated = false;

			state.commandBufferIsEmpty = false;
			state.memoryDefined = false;
			state.imageDefined = true;
			state.cache.perform(vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_ACCESS_TRANSFER_WRITE_BIT);
			break;

		case OP_IMAGE_BLIT_TO_IMAGE:
			state.rng.getBool();
			// Fall through
		case OP_IMAGE_COPY_TO_BUFFER:
		case OP_IMAGE_COPY_TO_IMAGE:
			DE_ASSERT(state.stage == STAGE_COMMAND_BUFFER);

			state.commandBufferIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_TRANSFER_BIT, vk::VK_ACCESS_TRANSFER_READ_BIT);
			break;

		case OP_PIPELINE_BARRIER_GLOBAL:
		case OP_PIPELINE_BARRIER_BUFFER:
		case OP_PIPELINE_BARRIER_IMAGE:
		{
			DE_ASSERT(state.stage == STAGE_COMMAND_BUFFER);

			vk::VkPipelineStageFlags	dirtySrcStages;
			vk::VkAccessFlags			dirtySrcAccesses;
			vk::VkPipelineStageFlags	dirtyDstStages;
			vk::VkAccessFlags			dirtyDstAccesses;

			vk::VkPipelineStageFlags	srcStages;
			vk::VkAccessFlags			srcAccesses;
			vk::VkPipelineStageFlags	dstStages;
			vk::VkAccessFlags			dstAccesses;

			state.cache.getFullBarrier(dirtySrcStages, dirtySrcAccesses, dirtyDstStages, dirtyDstAccesses);

			// Try masking some random bits
			srcStages	= dirtySrcStages & state.rng.getUint32();
			srcAccesses	= dirtySrcAccesses & state.rng.getUint32();

			dstStages	= dirtyDstStages & state.rng.getUint32();
			dstAccesses	= dirtyDstAccesses & state.rng.getUint32();

			// If there are no bits in stage mask use the original dirty stages
			srcStages	= srcStages ? srcStages : dirtySrcStages;
			dstStages	= dstStages ? dstStages : dirtyDstStages;

			if (!srcStages)
				srcStages = dstStages;

			removeIllegalAccessFlags(dstAccesses, dstStages);
			removeIllegalAccessFlags(srcAccesses, srcStages);

			state.commandBufferIsEmpty = false;
			state.cache.barrier(srcStages, srcAccesses, dstStages, dstAccesses);
			break;
		}

		case OP_RENDERPASS_BEGIN:
		{
			DE_ASSERT(state.stage == STAGE_COMMAND_BUFFER);

			state.renderPassIsEmpty	= true;
			state.stage				= STAGE_RENDER_PASS;
			break;
		}

		case OP_RENDERPASS_END:
		{
			DE_ASSERT(state.stage == STAGE_RENDER_PASS);

			state.renderPassIsEmpty	= true;
			state.stage				= STAGE_COMMAND_BUFFER;
			break;
		}

		case OP_RENDER_VERTEX_BUFFER:
		{
			DE_ASSERT(state.stage == STAGE_RENDER_PASS);

			state.renderPassIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, vk::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
			break;
		}

		case OP_RENDER_INDEX_BUFFER:
		{
			DE_ASSERT(state.stage == STAGE_RENDER_PASS);

			state.renderPassIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, vk::VK_ACCESS_INDEX_READ_BIT);
			break;
		}

		case OP_RENDER_VERTEX_UNIFORM_BUFFER:
		case OP_RENDER_VERTEX_UNIFORM_TEXEL_BUFFER:
		{
			DE_ASSERT(state.stage == STAGE_RENDER_PASS);

			state.renderPassIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT);
			break;
		}

		case OP_RENDER_FRAGMENT_UNIFORM_BUFFER:
		case OP_RENDER_FRAGMENT_UNIFORM_TEXEL_BUFFER:
		{
			DE_ASSERT(state.stage == STAGE_RENDER_PASS);

			state.renderPassIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_UNIFORM_READ_BIT);
			break;
		}

		case OP_RENDER_VERTEX_STORAGE_BUFFER:
		case OP_RENDER_VERTEX_STORAGE_TEXEL_BUFFER:
		{
			DE_ASSERT(state.stage == STAGE_RENDER_PASS);

			state.renderPassIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT);
			break;
		}

		case OP_RENDER_FRAGMENT_STORAGE_BUFFER:
		case OP_RENDER_FRAGMENT_STORAGE_TEXEL_BUFFER:
		{
			DE_ASSERT(state.stage == STAGE_RENDER_PASS);

			state.renderPassIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT);
			break;
		}

		case OP_RENDER_FRAGMENT_STORAGE_IMAGE:
		case OP_RENDER_FRAGMENT_SAMPLED_IMAGE:
		{
			DE_ASSERT(state.stage == STAGE_RENDER_PASS);

			state.renderPassIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT);
			break;
		}

		case OP_RENDER_VERTEX_STORAGE_IMAGE:
		case OP_RENDER_VERTEX_SAMPLED_IMAGE:
		{
			DE_ASSERT(state.stage == STAGE_RENDER_PASS);

			state.renderPassIsEmpty = false;
			state.cache.perform(vk::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, vk::VK_ACCESS_SHADER_READ_BIT);
			break;
		}

		default:
			DE_FATAL("Unknown op");
	}
}

de::MovePtr<Command> createHostCommand (Op					op,
										de::Random&			rng,
										Usage				usage,
										vk::VkSharingMode	sharing)
{
	switch (op)
	{
		case OP_MAP:					return de::MovePtr<Command>(new Map());
		case OP_UNMAP:					return de::MovePtr<Command>(new UnMap());

		case OP_MAP_FLUSH:				return de::MovePtr<Command>(new Flush());
		case OP_MAP_INVALIDATE:			return de::MovePtr<Command>(new Invalidate());

		case OP_MAP_READ:				return de::MovePtr<Command>(new HostMemoryAccess(true, false, rng.getUint32()));
		case OP_MAP_WRITE:				return de::MovePtr<Command>(new HostMemoryAccess(false, true, rng.getUint32()));
		case OP_MAP_MODIFY:				return de::MovePtr<Command>(new HostMemoryAccess(true, true, rng.getUint32()));

		case OP_BUFFER_CREATE:			return de::MovePtr<Command>(new CreateBuffer(usageToBufferUsageFlags(usage), sharing));
		case OP_BUFFER_DESTROY:			return de::MovePtr<Command>(new DestroyBuffer());
		case OP_BUFFER_BINDMEMORY:		return de::MovePtr<Command>(new BindBufferMemory());

		case OP_IMAGE_CREATE:			return de::MovePtr<Command>(new CreateImage(usageToImageUsageFlags(usage), sharing));
		case OP_IMAGE_DESTROY:			return de::MovePtr<Command>(new DestroyImage());
		case OP_IMAGE_BINDMEMORY:		return de::MovePtr<Command>(new BindImageMemory());

		case OP_QUEUE_WAIT_FOR_IDLE:	return de::MovePtr<Command>(new QueueWaitIdle());
		case OP_DEVICE_WAIT_FOR_IDLE:	return de::MovePtr<Command>(new DeviceWaitIdle());

		default:
			DE_FATAL("Unknown op");
			return de::MovePtr<Command>(DE_NULL);
	}
}

de::MovePtr<CmdCommand> createCmdCommand (de::Random&	rng,
										  const State&	state,
										  Op			op,
										  Usage			usage)
{
	switch (op)
	{
		case OP_BUFFER_FILL:					return de::MovePtr<CmdCommand>(new FillBuffer(rng.getUint32()));
		case OP_BUFFER_UPDATE:					return de::MovePtr<CmdCommand>(new UpdateBuffer(rng.getUint32()));
		case OP_BUFFER_COPY_TO_BUFFER:			return de::MovePtr<CmdCommand>(new BufferCopyToBuffer());
		case OP_BUFFER_COPY_FROM_BUFFER:		return de::MovePtr<CmdCommand>(new BufferCopyFromBuffer(rng.getUint32()));

		case OP_BUFFER_COPY_TO_IMAGE:			return de::MovePtr<CmdCommand>(new BufferCopyToImage());
		case OP_BUFFER_COPY_FROM_IMAGE:			return de::MovePtr<CmdCommand>(new BufferCopyFromImage(rng.getUint32()));

		case OP_IMAGE_TRANSITION_LAYOUT:
		{
			DE_ASSERT(state.stage == STAGE_COMMAND_BUFFER);
			DE_ASSERT(state.hasImage);
			DE_ASSERT(state.hasBoundImageMemory);

			const vk::VkImageLayout		srcLayout	= rng.getFloat() < 0.9f ? state.imageLayout : vk::VK_IMAGE_LAYOUT_UNDEFINED;
			const vk::VkImageLayout		dstLayout	= getRandomNextLayout(rng, usage, srcLayout);

			vk::VkPipelineStageFlags	dirtySrcStages;
			vk::VkAccessFlags			dirtySrcAccesses;
			vk::VkPipelineStageFlags	dirtyDstStages;
			vk::VkAccessFlags			dirtyDstAccesses;

			vk::VkPipelineStageFlags	srcStages;
			vk::VkAccessFlags			srcAccesses;
			vk::VkPipelineStageFlags	dstStages;
			vk::VkAccessFlags			dstAccesses;

			state.cache.getFullBarrier(dirtySrcStages, dirtySrcAccesses, dirtyDstStages, dirtyDstAccesses);

			// Try masking some random bits
			srcStages	= dirtySrcStages;
			srcAccesses	= dirtySrcAccesses;

			dstStages	= state.cache.getAllowedStages() & rng.getUint32();
			dstAccesses	= state.cache.getAllowedAcceses() & rng.getUint32();

			// If there are no bits in dst stage mask use all stages
			dstStages	= dstStages ? dstStages : state.cache.getAllowedStages();

			if (!srcStages)
				srcStages = dstStages;

			removeIllegalAccessFlags(dstAccesses, dstStages);
			removeIllegalAccessFlags(srcAccesses, srcStages);

			return de::MovePtr<CmdCommand>(new ImageTransition(srcStages, srcAccesses, dstStages, dstAccesses, srcLayout, dstLayout));
		}

		case OP_IMAGE_COPY_TO_BUFFER:			return de::MovePtr<CmdCommand>(new ImageCopyToBuffer(state.imageLayout));
		case OP_IMAGE_COPY_FROM_BUFFER:			return de::MovePtr<CmdCommand>(new ImageCopyFromBuffer(rng.getUint32(), state.imageLayout));
		case OP_IMAGE_COPY_TO_IMAGE:			return de::MovePtr<CmdCommand>(new ImageCopyToImage(state.imageLayout));
		case OP_IMAGE_COPY_FROM_IMAGE:			return de::MovePtr<CmdCommand>(new ImageCopyFromImage(rng.getUint32(), state.imageLayout));
		case OP_IMAGE_BLIT_TO_IMAGE:
		{
			const BlitScale scale = rng.getBool() ? BLIT_SCALE_20 : BLIT_SCALE_10;
			return de::MovePtr<CmdCommand>(new ImageBlitToImage(scale, state.imageLayout));
		}

		case OP_IMAGE_BLIT_FROM_IMAGE:
		{
			const BlitScale scale = rng.getBool() ? BLIT_SCALE_20 : BLIT_SCALE_10;
			return de::MovePtr<CmdCommand>(new ImageBlitFromImage(rng.getUint32(), scale, state.imageLayout));
		}

		case OP_PIPELINE_BARRIER_GLOBAL:
		case OP_PIPELINE_BARRIER_BUFFER:
		case OP_PIPELINE_BARRIER_IMAGE:
		{
			vk::VkPipelineStageFlags	dirtySrcStages;
			vk::VkAccessFlags			dirtySrcAccesses;
			vk::VkPipelineStageFlags	dirtyDstStages;
			vk::VkAccessFlags			dirtyDstAccesses;

			vk::VkPipelineStageFlags	srcStages;
			vk::VkAccessFlags			srcAccesses;
			vk::VkPipelineStageFlags	dstStages;
			vk::VkAccessFlags			dstAccesses;

			state.cache.getFullBarrier(dirtySrcStages, dirtySrcAccesses, dirtyDstStages, dirtyDstAccesses);

			// Try masking some random bits
			srcStages	= dirtySrcStages & rng.getUint32();
			srcAccesses	= dirtySrcAccesses & rng.getUint32();

			dstStages	= dirtyDstStages & rng.getUint32();
			dstAccesses	= dirtyDstAccesses & rng.getUint32();

			// If there are no bits in stage mask use the original dirty stages
			srcStages	= srcStages ? srcStages : dirtySrcStages;
			dstStages	= dstStages ? dstStages : dirtyDstStages;

			if (!srcStages)
				srcStages = dstStages;

			removeIllegalAccessFlags(dstAccesses, dstStages);
			removeIllegalAccessFlags(srcAccesses, srcStages);

			PipelineBarrier::Type type;

			if (op == OP_PIPELINE_BARRIER_IMAGE)
				type = PipelineBarrier::TYPE_IMAGE;
			else if (op == OP_PIPELINE_BARRIER_BUFFER)
				type = PipelineBarrier::TYPE_BUFFER;
			else if (op == OP_PIPELINE_BARRIER_GLOBAL)
				type = PipelineBarrier::TYPE_GLOBAL;
			else
			{
				type = PipelineBarrier::TYPE_LAST;
				DE_FATAL("Unknown op");
			}

			if (type == PipelineBarrier::TYPE_IMAGE)
				return de::MovePtr<CmdCommand>(new PipelineBarrier(srcStages, srcAccesses, dstStages, dstAccesses, type, tcu::just(state.imageLayout)));
			else
				return de::MovePtr<CmdCommand>(new PipelineBarrier(srcStages, srcAccesses, dstStages, dstAccesses, type, tcu::nothing<vk::VkImageLayout>()));
		}

		default:
			DE_FATAL("Unknown op");
			return de::MovePtr<CmdCommand>(DE_NULL);
	}
}

de::MovePtr<RenderPassCommand> createRenderPassCommand (de::Random&,
														const State&,
														Op				op)
{
	switch (op)
	{
		case OP_RENDER_VERTEX_BUFFER:					return de::MovePtr<RenderPassCommand>(new RenderVertexBuffer());
		case OP_RENDER_INDEX_BUFFER:					return de::MovePtr<RenderPassCommand>(new RenderIndexBuffer());

		case OP_RENDER_VERTEX_UNIFORM_BUFFER:			return de::MovePtr<RenderPassCommand>(new RenderVertexUniformBuffer());
		case OP_RENDER_FRAGMENT_UNIFORM_BUFFER:			return de::MovePtr<RenderPassCommand>(new RenderFragmentUniformBuffer());

		case OP_RENDER_VERTEX_UNIFORM_TEXEL_BUFFER:		return de::MovePtr<RenderPassCommand>(new RenderVertexUniformTexelBuffer());
		case OP_RENDER_FRAGMENT_UNIFORM_TEXEL_BUFFER:	return de::MovePtr<RenderPassCommand>(new RenderFragmentUniformTexelBuffer());

		case OP_RENDER_VERTEX_STORAGE_BUFFER:			return de::MovePtr<RenderPassCommand>(new RenderVertexStorageBuffer());
		case OP_RENDER_FRAGMENT_STORAGE_BUFFER:			return de::MovePtr<RenderPassCommand>(new RenderFragmentStorageBuffer());

		case OP_RENDER_VERTEX_STORAGE_TEXEL_BUFFER:		return de::MovePtr<RenderPassCommand>(new RenderVertexStorageTexelBuffer());
		case OP_RENDER_FRAGMENT_STORAGE_TEXEL_BUFFER:	return de::MovePtr<RenderPassCommand>(new RenderFragmentStorageTexelBuffer());

		case OP_RENDER_VERTEX_STORAGE_IMAGE:			return de::MovePtr<RenderPassCommand>(new RenderVertexStorageImage());
		case OP_RENDER_FRAGMENT_STORAGE_IMAGE:			return de::MovePtr<RenderPassCommand>(new RenderFragmentStorageImage());

		case OP_RENDER_VERTEX_SAMPLED_IMAGE:			return de::MovePtr<RenderPassCommand>(new RenderVertexSampledImage());
		case OP_RENDER_FRAGMENT_SAMPLED_IMAGE:			return de::MovePtr<RenderPassCommand>(new RenderFragmentSampledImage());

		default:
			DE_FATAL("Unknown op");
			return de::MovePtr<RenderPassCommand>(DE_NULL);
	}
}

de::MovePtr<CmdCommand> createRenderPassCommands (const Memory&	memory,
												  de::Random&	nextOpRng,
												  State&		state,
												  Usage			usage,
												  size_t&		opNdx,
												  size_t		opCount)
{
	vector<RenderPassCommand*>	commands;

	try
	{
		for (; opNdx < opCount; opNdx++)
		{
			vector<Op>	ops;

			getAvailableOps(state, memory.getSupportBuffers(), memory.getSupportImages(), usage, ops);

			DE_ASSERT(!ops.empty());

			{
				const Op op = nextOpRng.choose<Op>(ops.begin(), ops.end());

				if (op == OP_RENDERPASS_END)
				{
					break;
				}
				else
				{
					de::Random	rng	(state.rng);

					commands.push_back(createRenderPassCommand(rng, state, op).release());
					applyOp(state, memory, op, usage);

					DE_ASSERT(state.rng == rng);
				}
			}
		}

		applyOp(state, memory, OP_RENDERPASS_END, usage);
		return de::MovePtr<CmdCommand>(new SubmitRenderPass(commands));
	}
	catch (...)
	{
		for (size_t commandNdx = 0; commandNdx < commands.size(); commandNdx++)
			delete commands[commandNdx];

		throw;
	}
}

de::MovePtr<Command> createCmdCommands (const Memory&	memory,
										de::Random&		nextOpRng,
										State&			state,
										Usage			usage,
										size_t&			opNdx,
										size_t			opCount)
{
	vector<CmdCommand*>	commands;

	try
	{
		for (; opNdx < opCount; opNdx++)
		{
			vector<Op>	ops;

			getAvailableOps(state, memory.getSupportBuffers(), memory.getSupportImages(), usage, ops);

			DE_ASSERT(!ops.empty());

			{
				const Op op = nextOpRng.choose<Op>(ops.begin(), ops.end());

				if (op == OP_COMMAND_BUFFER_END)
				{
					break;
				}
				else
				{
					// \note Command needs to known the state before the operation
					if (op == OP_RENDERPASS_BEGIN)
					{
						applyOp(state, memory, op, usage);
						commands.push_back(createRenderPassCommands(memory, nextOpRng, state, usage, opNdx, opCount).release());
					}
					else
					{
						de::Random	rng	(state.rng);

						commands.push_back(createCmdCommand(rng, state, op, usage).release());
						applyOp(state, memory, op, usage);

						DE_ASSERT(state.rng == rng);
					}

				}
			}
		}

		applyOp(state, memory, OP_COMMAND_BUFFER_END, usage);
		return de::MovePtr<Command>(new SubmitCommandBuffer(commands));
	}
	catch (...)
	{
		for (size_t commandNdx = 0; commandNdx < commands.size(); commandNdx++)
			delete commands[commandNdx];

		throw;
	}
}

void createCommands (vector<Command*>&	commands,
					 deUint32			seed,
					 const Memory&		memory,
					 Usage				usage,
					 vk::VkSharingMode	sharingMode,
					 size_t				opCount)
{
	State			state		(usage, seed);
	// Used to select next operation only
	de::Random		nextOpRng	(seed ^ 12930809);

	commands.reserve(opCount);

	for (size_t opNdx = 0; opNdx < opCount; opNdx++)
	{
		vector<Op>	ops;

		getAvailableOps(state, memory.getSupportBuffers(), memory.getSupportImages(), usage, ops);

		DE_ASSERT(!ops.empty());

		{
			const Op	op	= nextOpRng.choose<Op>(ops.begin(), ops.end());

			if (op == OP_COMMAND_BUFFER_BEGIN)
			{
				applyOp(state, memory, op, usage);
				commands.push_back(createCmdCommands(memory, nextOpRng, state, usage, opNdx, opCount).release());
			}
			else
			{
				de::Random	rng	(state.rng);

				commands.push_back(createHostCommand(op, rng, usage, sharingMode).release());
				applyOp(state, memory, op, usage);

				// Make sure that random generator is in sync
				DE_ASSERT(state.rng == rng);
			}
		}
	}

	// Clean up resources
	if (state.hasBuffer && state.hasImage)
	{
		if (!state.queueIdle)
			commands.push_back(new QueueWaitIdle());

		if (state.hasBuffer)
			commands.push_back(new DestroyBuffer());

		if (state.hasImage)
			commands.push_back(new DestroyImage());
	}
}

class MemoryTestInstance : public TestInstance
{
public:

	typedef bool(MemoryTestInstance::*StageFunc)(void);

												MemoryTestInstance				(::vkt::Context& context, const TestConfig& config);
												~MemoryTestInstance				(void);

	tcu::TestStatus								iterate							(void);

private:
	const TestConfig							m_config;
	const size_t								m_iterationCount;
	const size_t								m_opCount;
	const vk::VkPhysicalDeviceMemoryProperties	m_memoryProperties;
	deUint32									m_memoryTypeNdx;
	size_t										m_iteration;
	StageFunc									m_stage;
	tcu::ResultCollector						m_resultCollector;

	vector<Command*>							m_commands;
	MovePtr<Memory>								m_memory;
	MovePtr<Context>							m_renderContext;
	MovePtr<PrepareContext>						m_prepareContext;

	bool										nextIteration					(void);
	bool										nextMemoryType					(void);

	bool										createCommandsAndAllocateMemory	(void);
	bool										prepare							(void);
	bool										execute							(void);
	bool										verify							(void);
	void										resetResources					(void);
};

void MemoryTestInstance::resetResources (void)
{
	const vk::DeviceInterface&	vkd		= m_context.getDeviceInterface();
	const vk::VkDevice			device	= m_context.getDevice();

	VK_CHECK(vkd.deviceWaitIdle(device));

	for (size_t commandNdx = 0; commandNdx < m_commands.size(); commandNdx++)
	{
		delete m_commands[commandNdx];
		m_commands[commandNdx] = DE_NULL;
	}

	m_commands.clear();
	m_prepareContext.clear();
	m_memory.clear();
}

bool MemoryTestInstance::nextIteration (void)
{
	m_iteration++;

	if (m_iteration < m_iterationCount)
	{
		resetResources();
		m_stage = &MemoryTestInstance::createCommandsAndAllocateMemory;
		return true;
	}
	else
		return nextMemoryType();
}

bool MemoryTestInstance::nextMemoryType (void)
{
	resetResources();

	DE_ASSERT(m_commands.empty());

	m_memoryTypeNdx++;

	if (m_memoryTypeNdx < m_memoryProperties.memoryTypeCount)
	{
		m_iteration	= 0;
		m_stage		= &MemoryTestInstance::createCommandsAndAllocateMemory;

		return true;
	}
	else
	{
		m_stage = DE_NULL;
		return false;
	}
}

MemoryTestInstance::MemoryTestInstance (::vkt::Context& context, const TestConfig& config)
	: TestInstance			(context)
	, m_config				(config)
	, m_iterationCount		(5)
	, m_opCount				(50)
	, m_memoryProperties	(vk::getPhysicalDeviceMemoryProperties(context.getInstanceInterface(), context.getPhysicalDevice()))
	, m_memoryTypeNdx		(0)
	, m_iteration			(0)
	, m_stage				(&MemoryTestInstance::createCommandsAndAllocateMemory)
	, m_resultCollector		(context.getTestContext().getLog())

	, m_memory				(DE_NULL)
{
	TestLog&	log	= context.getTestContext().getLog();
	{
		const tcu::ScopedLogSection section (log, "TestCaseInfo", "Test Case Info");

		log << TestLog::Message << "Buffer size: " << config.size << TestLog::EndMessage;
		log << TestLog::Message << "Sharing: " << config.sharing << TestLog::EndMessage;
		log << TestLog::Message << "Access: " << config.usage << TestLog::EndMessage;
	}

	{
		const tcu::ScopedLogSection section (log, "MemoryProperties", "Memory Properties");

		for (deUint32 heapNdx = 0; heapNdx < m_memoryProperties.memoryHeapCount; heapNdx++)
		{
			const tcu::ScopedLogSection heapSection (log, "Heap" + de::toString(heapNdx), "Heap " + de::toString(heapNdx));

			log << TestLog::Message << "Size: " << m_memoryProperties.memoryHeaps[heapNdx].size << TestLog::EndMessage;
			log << TestLog::Message << "Flags: " << m_memoryProperties.memoryHeaps[heapNdx].flags << TestLog::EndMessage;
		}

		for (deUint32 memoryTypeNdx = 0; memoryTypeNdx < m_memoryProperties.memoryTypeCount; memoryTypeNdx++)
		{
			const tcu::ScopedLogSection memoryTypeSection (log, "MemoryType" + de::toString(memoryTypeNdx), "Memory type " + de::toString(memoryTypeNdx));

			log << TestLog::Message << "Properties: " << m_memoryProperties.memoryTypes[memoryTypeNdx].propertyFlags << TestLog::EndMessage;
			log << TestLog::Message << "Heap: " << m_memoryProperties.memoryTypes[memoryTypeNdx].heapIndex << TestLog::EndMessage;
		}
	}

	{
		const vk::InstanceInterface&			vki					= context.getInstanceInterface();
		const vk::VkPhysicalDevice				physicalDevice		= context.getPhysicalDevice();
		const vk::DeviceInterface&				vkd					= context.getDeviceInterface();
		const vk::VkDevice						device				= context.getDevice();
		const vk::VkQueue						queue				= context.getUniversalQueue();
		const deUint32							queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
		vector<pair<deUint32, vk::VkQueue> >	queues;

		queues.push_back(std::make_pair(queueFamilyIndex, queue));

		m_renderContext = MovePtr<Context>(new Context(vki, vkd, physicalDevice, device, queue, queueFamilyIndex, queues, context.getBinaryCollection()));
	}
}

MemoryTestInstance::~MemoryTestInstance (void)
{
	resetResources();
}

bool MemoryTestInstance::createCommandsAndAllocateMemory (void)
{
	const vk::VkDevice							device				= m_context.getDevice();
	TestLog&									log					= m_context.getTestContext().getLog();
	const vk::InstanceInterface&				vki					= m_context.getInstanceInterface();
	const vk::VkPhysicalDevice					physicalDevice		= m_context.getPhysicalDevice();
	const vk::DeviceInterface&					vkd					= m_context.getDeviceInterface();
	const vk::VkPhysicalDeviceMemoryProperties	memoryProperties	= vk::getPhysicalDeviceMemoryProperties(vki, physicalDevice);
	const tcu::ScopedLogSection					section				(log, "MemoryType" + de::toString(m_memoryTypeNdx) + "CreateCommands" + de::toString(m_iteration),
																		  "Memory type " + de::toString(m_memoryTypeNdx) + " create commands iteration " + de::toString(m_iteration));
	const vector<deUint32>&						queues				= m_renderContext->getQueueFamilies();

	DE_ASSERT(m_commands.empty());

	if (m_config.usage & (USAGE_HOST_READ | USAGE_HOST_WRITE)
		&& !(memoryProperties.memoryTypes[m_memoryTypeNdx].propertyFlags & vk::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
	{
		log << TestLog::Message << "Memory type not supported" << TestLog::EndMessage;

		return nextMemoryType();
	}
	else
	{
		try
		{
			const vk::VkBufferUsageFlags	bufferUsage		= usageToBufferUsageFlags(m_config.usage);
			const vk::VkImageUsageFlags		imageUsage		= usageToImageUsageFlags(m_config.usage);
			const vk::VkDeviceSize			maxBufferSize	= bufferUsage != 0
															? roundBufferSizeToWxHx4(findMaxBufferSize(vkd, device, bufferUsage, m_config.sharing, queues, m_config.size, m_memoryTypeNdx))
															: 0;
			const IVec2						maxImageSize	= imageUsage != 0
															? findMaxRGBA8ImageSize(vkd, device, imageUsage, m_config.sharing, queues, m_config.size, m_memoryTypeNdx)
															: IVec2(0, 0);

			log << TestLog::Message << "Max buffer size: " << maxBufferSize << TestLog::EndMessage;
			log << TestLog::Message << "Max RGBA8 image size: " << maxImageSize << TestLog::EndMessage;

			// Skip tests if there are no supported operations
			if (maxBufferSize == 0
				&& maxImageSize[0] == 0
				&& (m_config.usage & (USAGE_HOST_READ|USAGE_HOST_WRITE)) == 0)
			{
				log << TestLog::Message << "Skipping memory type. None of the usages are supported." << TestLog::EndMessage;

				return nextMemoryType();
			}
			else
			{
				const deUint32	seed	= 2830980989u ^ deUint32Hash((deUint32)(m_iteration) * m_memoryProperties.memoryTypeCount +  m_memoryTypeNdx);

				m_memory	= MovePtr<Memory>(new Memory(vki, vkd, physicalDevice, device, m_config.size, m_memoryTypeNdx, maxBufferSize, maxImageSize[0], maxImageSize[1]));

				log << TestLog::Message << "Create commands" << TestLog::EndMessage;
				createCommands(m_commands, seed, *m_memory, m_config.usage, m_config.sharing, m_opCount);

				m_stage = &MemoryTestInstance::prepare;
				return true;
			}
		}
		catch (const tcu::TestError& e)
		{
			m_resultCollector.fail("Failed, got exception: " + string(e.getMessage()));
			return nextMemoryType();
		}
	}
}

bool MemoryTestInstance::prepare (void)
{
	TestLog&					log		= m_context.getTestContext().getLog();
	const tcu::ScopedLogSection	section	(log, "MemoryType" + de::toString(m_memoryTypeNdx) + "Prepare" + de::toString(m_iteration),
											  "Memory type " + de::toString(m_memoryTypeNdx) + " prepare iteration" + de::toString(m_iteration));

	m_prepareContext = MovePtr<PrepareContext>(new PrepareContext(*m_renderContext, *m_memory));

	DE_ASSERT(!m_commands.empty());

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
	{
		Command& command = *m_commands[cmdNdx];

		try
		{
			command.prepare(*m_prepareContext);
		}
		catch (const tcu::TestError& e)
		{
			m_resultCollector.fail(de::toString(cmdNdx) + ":" + command.getName() + " failed to prepare, got exception: " + string(e.getMessage()));
			return nextMemoryType();
		}
	}

	m_stage = &MemoryTestInstance::execute;
	return true;
}

bool MemoryTestInstance::execute (void)
{
	TestLog&					log				= m_context.getTestContext().getLog();
	const tcu::ScopedLogSection	section			(log, "MemoryType" + de::toString(m_memoryTypeNdx) + "Execute" + de::toString(m_iteration),
													  "Memory type " + de::toString(m_memoryTypeNdx) + " execute iteration " + de::toString(m_iteration));
	ExecuteContext				executeContext	(*m_renderContext);
	const vk::VkDevice			device			= m_context.getDevice();
	const vk::DeviceInterface&	vkd				= m_context.getDeviceInterface();

	DE_ASSERT(!m_commands.empty());

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
	{
		Command& command = *m_commands[cmdNdx];

		try
		{
			command.execute(executeContext);
		}
		catch (const tcu::TestError& e)
		{
			m_resultCollector.fail(de::toString(cmdNdx) + ":" + command.getName() + " failed to execute, got exception: " + string(e.getMessage()));
			return nextIteration();
		}
	}

	VK_CHECK(vkd.deviceWaitIdle(device));

	m_stage = &MemoryTestInstance::verify;
	return true;
}

bool MemoryTestInstance::verify (void)
{
	DE_ASSERT(!m_commands.empty());

	TestLog&					log				= m_context.getTestContext().getLog();
	const tcu::ScopedLogSection	section			(log, "MemoryType" + de::toString(m_memoryTypeNdx) + "Verify" + de::toString(m_iteration),
													  "Memory type " + de::toString(m_memoryTypeNdx) + " verify iteration " + de::toString(m_iteration));
	VerifyContext				verifyContext	(log, m_resultCollector, *m_renderContext, m_config.size);

	log << TestLog::Message << "Begin verify" << TestLog::EndMessage;

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
	{
		Command& command = *m_commands[cmdNdx];

		try
		{
			command.verify(verifyContext, cmdNdx);
		}
		catch (const tcu::TestError& e)
		{
			m_resultCollector.fail(de::toString(cmdNdx) + ":" + command.getName() + " failed to verify, got exception: " + string(e.getMessage()));
			return nextIteration();
		}
	}

	return nextIteration();
}

tcu::TestStatus MemoryTestInstance::iterate (void)
{
	if ((this->*m_stage)())
		return tcu::TestStatus::incomplete();
	else
		return tcu::TestStatus(m_resultCollector.getResult(), m_resultCollector.getMessage());
}

struct AddPrograms
{
	void init (vk::SourceCollections& sources, TestConfig config) const
	{
		// Vertex buffer rendering
		if (config.usage & USAGE_VERTEX_BUFFER)
		{
			const char* const vertexShader =
				"#version 310 es\n"
				"layout(location = 0) in highp vec2 a_position;\n"
				"void main (void) {\n"
				"\tgl_PointSize = 1.0;\n"
				"\tgl_Position = vec4(1.998 * a_position - vec2(0.999), 0.0, 1.0);\n"
				"}\n";

			sources.glslSources.add("vertex-buffer.vert")
				<< glu::VertexSource(vertexShader);
		}

		// Index buffer rendering
		if (config.usage & USAGE_INDEX_BUFFER)
		{
			const char* const vertexShader =
				"#version 310 es\n"
				"precision highp float;\n"
				"void main (void) {\n"
				"\tgl_PointSize = 1.0;\n"
				"\thighp vec2 pos = vec2(gl_VertexIndex % 256, gl_VertexIndex / 256) / vec2(255.0);\n"
				"\tgl_Position = vec4(1.998 * pos - vec2(0.999), 0.0, 1.0);\n"
				"}\n";

			sources.glslSources.add("index-buffer.vert")
				<< glu::VertexSource(vertexShader);
		}

		if (config.usage & USAGE_UNIFORM_BUFFER)
		{
			{
				std::ostringstream vertexShader;

				vertexShader <<
					"#version 310 es\n"
					"precision highp float;\n"
					"layout(set=0, binding=0) uniform Block\n"
					"{\n"
					"\thighp uvec4 values[" << de::toString<size_t>(MAX_UNIFORM_BUFFER_SIZE / (sizeof(deUint32) * 4)) << "];\n"
					"} block;\n"
					"void main (void) {\n"
					"\tgl_PointSize = 1.0;\n"
					"\thighp uvec4 vecVal = block.values[gl_VertexIndex / 8];\n"
					"\thighp uint val;\n"
					"\tif (((gl_VertexIndex / 2) % 4 == 0))\n"
					"\t\tval = vecVal.x;\n"
					"\telse if (((gl_VertexIndex / 2) % 4 == 1))\n"
					"\t\tval = vecVal.y;\n"
					"\telse if (((gl_VertexIndex / 2) % 4 == 2))\n"
					"\t\tval = vecVal.z;\n"
					"\telse if (((gl_VertexIndex / 2) % 4 == 3))\n"
					"\t\tval = vecVal.w;\n"
					"\tif ((gl_VertexIndex % 2) == 0)\n"
					"\t\tval = val & 0xFFFFu;\n"
					"\telse\n"
					"\t\tval = val >> 16u;\n"
					"\thighp vec2 pos = vec2(val & 0xFFu, val >> 8u) / vec2(255.0);\n"
					"\tgl_Position = vec4(1.998 * pos - vec2(0.999), 0.0, 1.0);\n"
					"}\n";

				sources.glslSources.add("uniform-buffer.vert")
					<< glu::VertexSource(vertexShader.str());
			}

			{
				const size_t		arraySize		= MAX_UNIFORM_BUFFER_SIZE / (sizeof(deUint32) * 4);
				const size_t		arrayIntSize	= arraySize * 4;
				std::ostringstream	fragmentShader;

				fragmentShader <<
					"#version 310 es\n"
					"precision highp float;\n"
					"precision highp int;\n"
					"layout(location = 0) out highp vec4 o_color;\n"
					"layout(set=0, binding=0) uniform Block\n"
					"{\n"
					"\thighp uvec4 values[" << arraySize << "];\n"
					"} block;\n"
					"layout(push_constant) uniform PushC\n"
					"{\n"
					"\tuint callId;\n"
					"\tuint valuesPerPixel;\n"
					"} pushC;\n"
					"void main (void) {\n"
					"\thighp uint id = pushC.callId * (" << arrayIntSize << "u / pushC.valuesPerPixel) + uint(gl_FragCoord.y) * 256u + uint(gl_FragCoord.x);\n"
					"\tif (uint(gl_FragCoord.y) * 256u + uint(gl_FragCoord.x) < pushC.callId * (" << arrayIntSize  << "u / pushC.valuesPerPixel))\n"
					"\t\tdiscard;\n"
					"\thighp uint value = id;\n"
					"\tfor (uint i = 0u; i < pushC.valuesPerPixel; i++)\n"
					"\t{\n"
					"\t\thighp uvec4 vecVal = block.values[(value / 4u) % " << arraySize << "u];\n"
					"\t\tif ((value % 4u) == 0u)\n"
					"\t\t\tvalue = vecVal.x;\n"
					"\t\telse if ((value % 4u) == 1u)\n"
					"\t\t\tvalue = vecVal.y;\n"
					"\t\telse if ((value % 4u) == 2u)\n"
					"\t\t\tvalue = vecVal.z;\n"
					"\t\telse if ((value % 4u) == 3u)\n"
					"\t\t\tvalue = vecVal.w;\n"
					"\t}\n"
					"\tuvec4 valueOut = uvec4(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu, (value >> 24u) & 0xFFu);\n"
					"\to_color = vec4(valueOut) / vec4(255.0);\n"
					"}\n";

				sources.glslSources.add("uniform-buffer.frag")
					<< glu::FragmentSource(fragmentShader.str());
			}
		}

		if (config.usage & USAGE_STORAGE_BUFFER)
		{
			{
				// Vertex storage buffer rendering
				const char* const vertexShader =
					"#version 310 es\n"
					"precision highp float;\n"
					"layout(set=0, binding=0) buffer Block\n"
					"{\n"
					"\thighp uvec4 values[];\n"
					"} block;\n"
					"void main (void) {\n"
					"\tgl_PointSize = 1.0;\n"
					"\thighp uvec4 vecVal = block.values[gl_VertexIndex / 8];\n"
					"\thighp uint val;\n"
					"\tif (((gl_VertexIndex / 2) % 4 == 0))\n"
					"\t\tval = vecVal.x;\n"
					"\telse if (((gl_VertexIndex / 2) % 4 == 1))\n"
					"\t\tval = vecVal.y;\n"
					"\telse if (((gl_VertexIndex / 2) % 4 == 2))\n"
					"\t\tval = vecVal.z;\n"
					"\telse if (((gl_VertexIndex / 2) % 4 == 3))\n"
					"\t\tval = vecVal.w;\n"
					"\tif ((gl_VertexIndex % 2) == 0)\n"
					"\t\tval = val & 0xFFFFu;\n"
					"\telse\n"
					"\t\tval = val >> 16u;\n"
					"\thighp vec2 pos = vec2(val & 0xFFu, val >> 8u) / vec2(255.0);\n"
					"\tgl_Position = vec4(1.998 * pos - vec2(0.999), 0.0, 1.0);\n"
					"}\n";

				sources.glslSources.add("storage-buffer.vert")
					<< glu::VertexSource(vertexShader);
			}

			{
				std::ostringstream	fragmentShader;

				fragmentShader <<
					"#version 310 es\n"
					"precision highp float;\n"
					"precision highp int;\n"
					"layout(location = 0) out highp vec4 o_color;\n"
					"layout(set=0, binding=0) buffer Block\n"
					"{\n"
					"\thighp uvec4 values[];\n"
					"} block;\n"
					"layout(push_constant) uniform PushC\n"
					"{\n"
					"\tuint valuesPerPixel;\n"
					"\tuint bufferSize;\n"
					"} pushC;\n"
					"void main (void) {\n"
					"\thighp uint arrayIntSize = pushC.bufferSize / 4u;\n"
					"\thighp uint id = uint(gl_FragCoord.y) * 256u + uint(gl_FragCoord.x);\n"
					"\thighp uint value = id;\n"
					"\tfor (uint i = 0u; i < pushC.valuesPerPixel; i++)\n"
					"\t{\n"
					"\t\thighp uvec4 vecVal = block.values[(value / 4u) % (arrayIntSize / 4u)];\n"
					"\t\tif ((value % 4u) == 0u)\n"
					"\t\t\tvalue = vecVal.x;\n"
					"\t\telse if ((value % 4u) == 1u)\n"
					"\t\t\tvalue = vecVal.y;\n"
					"\t\telse if ((value % 4u) == 2u)\n"
					"\t\t\tvalue = vecVal.z;\n"
					"\t\telse if ((value % 4u) == 3u)\n"
					"\t\t\tvalue = vecVal.w;\n"
					"\t}\n"
					"\tuvec4 valueOut = uvec4(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu, (value >> 24u) & 0xFFu);\n"
					"\to_color = vec4(valueOut) / vec4(255.0);\n"
					"}\n";

				sources.glslSources.add("storage-buffer.frag")
					<< glu::FragmentSource(fragmentShader.str());
			}
		}

		if (config.usage & USAGE_UNIFORM_TEXEL_BUFFER)
		{
			{
				// Vertex uniform texel buffer rendering
				const char* const vertexShader =
					"#version 310 es\n"
					"#extension GL_EXT_texture_buffer : require\n"
					"precision highp float;\n"
					"layout(set=0, binding=0) uniform highp usamplerBuffer u_sampler;\n"
					"void main (void) {\n"
					"\tgl_PointSize = 1.0;\n"
					"\thighp uint val = texelFetch(u_sampler, gl_VertexIndex).x;\n"
					"\thighp vec2 pos = vec2(val & 0xFFu, val >> 8u) / vec2(255.0);\n"
					"\tgl_Position = vec4(1.998 * pos - vec2(0.999), 0.0, 1.0);\n"
					"}\n";

				sources.glslSources.add("uniform-texel-buffer.vert")
					<< glu::VertexSource(vertexShader);
			}

			{
				// Fragment uniform texel buffer rendering
				const char* const fragmentShader =
					"#version 310 es\n"
					"#extension GL_EXT_texture_buffer : require\n"
					"precision highp float;\n"
					"precision highp int;\n"
					"layout(set=0, binding=0) uniform highp usamplerBuffer u_sampler;\n"
					"layout(location = 0) out highp vec4 o_color;\n"
					"layout(push_constant) uniform PushC\n"
					"{\n"
					"\tuint callId;\n"
					"\tuint valuesPerPixel;\n"
					"\tuint maxTexelCount;\n"
					"} pushC;\n"
					"void main (void) {\n"
					"\thighp uint id = uint(gl_FragCoord.y) * 256u + uint(gl_FragCoord.x);\n"
					"\thighp uint value = id;\n"
					"\tif (uint(gl_FragCoord.y) * 256u + uint(gl_FragCoord.x) < pushC.callId * (pushC.maxTexelCount / pushC.valuesPerPixel))\n"
					"\t\tdiscard;\n"
					"\tfor (uint i = 0u; i < pushC.valuesPerPixel; i++)\n"
					"\t{\n"
					"\t\tvalue = texelFetch(u_sampler, int(value % uint(textureSize(u_sampler)))).x;\n"
					"\t}\n"
					"\tuvec4 valueOut = uvec4(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu, (value >> 24u) & 0xFFu);\n"
					"\to_color = vec4(valueOut) / vec4(255.0);\n"
					"}\n";

				sources.glslSources.add("uniform-texel-buffer.frag")
					<< glu::FragmentSource(fragmentShader);
			}
		}

		if (config.usage & USAGE_STORAGE_TEXEL_BUFFER)
		{
			{
				// Vertex storage texel buffer rendering
				const char* const vertexShader =
					"#version 450\n"
					"#extension GL_EXT_texture_buffer : require\n"
					"precision highp float;\n"
					"layout(set=0, binding=0, r32ui) uniform readonly highp uimageBuffer u_sampler;\n"
					"out gl_PerVertex {\n"
					"\tvec4 gl_Position;\n"
					"\tfloat gl_PointSize;\n"
					"};\n"
					"void main (void) {\n"
					"\tgl_PointSize = 1.0;\n"
					"\thighp uint val = imageLoad(u_sampler, gl_VertexIndex / 2).x;\n"
					"\tif (gl_VertexIndex % 2 == 0)\n"
					"\t\tval = val & 0xFFFFu;\n"
					"\telse\n"
					"\t\tval = val >> 16;\n"
					"\thighp vec2 pos = vec2(val & 0xFFu, val >> 8u) / vec2(255.0);\n"
					"\tgl_Position = vec4(1.998 * pos - vec2(0.999), 0.0, 1.0);\n"
					"}\n";

				sources.glslSources.add("storage-texel-buffer.vert")
					<< glu::VertexSource(vertexShader);
			}
			{
				// Fragment storage texel buffer rendering
				const char* const fragmentShader =
					"#version 310 es\n"
					"#extension GL_EXT_texture_buffer : require\n"
					"precision highp float;\n"
					"precision highp int;\n"
					"layout(set=0, binding=0, r32ui) uniform readonly highp uimageBuffer u_sampler;\n"
					"layout(location = 0) out highp vec4 o_color;\n"
					"layout(push_constant) uniform PushC\n"
					"{\n"
					"\tuint callId;\n"
					"\tuint valuesPerPixel;\n"
					"\tuint maxTexelCount;\n"
					"\tuint width;\n"
					"} pushC;\n"
					"void main (void) {\n"
					"\thighp uint id = uint(gl_FragCoord.y) * 256u + uint(gl_FragCoord.x);\n"
					"\thighp uint value = id;\n"
					"\tif (uint(gl_FragCoord.y) * 256u + uint(gl_FragCoord.x) < pushC.callId * (pushC.maxTexelCount / pushC.valuesPerPixel))\n"
					"\t\tdiscard;\n"
					"\tfor (uint i = 0u; i < pushC.valuesPerPixel; i++)\n"
					"\t{\n"
					"\t\tvalue = imageLoad(u_sampler, int(value % pushC.width)).x;\n"
					"\t}\n"
					"\tuvec4 valueOut = uvec4(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu, (value >> 24u) & 0xFFu);\n"
					"\to_color = vec4(valueOut) / vec4(255.0);\n"
					"}\n";

				sources.glslSources.add("storage-texel-buffer.frag")
					<< glu::FragmentSource(fragmentShader);
			}
		}

		if (config.usage & USAGE_STORAGE_IMAGE)
		{
			{
				// Vertex storage image
				const char* const vertexShader =
					"#version 450\n"
					"precision highp float;\n"
					"layout(set=0, binding=0, rgba8) uniform image2D u_image;\n"
					"out gl_PerVertex {\n"
					"\tvec4 gl_Position;\n"
					"\tfloat gl_PointSize;\n"
					"};\n"
					"void main (void) {\n"
					"\tgl_PointSize = 1.0;\n"
					"\thighp vec4 val = imageLoad(u_image, ivec2((gl_VertexIndex / 2) / imageSize(u_image).x, (gl_VertexIndex / 2) % imageSize(u_image).x));\n"
					"\thighp vec2 pos;\n"
					"\tif (gl_VertexIndex % 2 == 0)\n"
					"\t\tpos = val.xy;\n"
					"\telse\n"
					"\t\tpos = val.zw;\n"
					"\tgl_Position = vec4(1.998 * pos - vec2(0.999), 0.0, 1.0);\n"
					"}\n";

				sources.glslSources.add("storage-image.vert")
					<< glu::VertexSource(vertexShader);
			}
			{
				// Fragment storage image
				const char* const fragmentShader =
					"#version 450\n"
					"#extension GL_EXT_texture_buffer : require\n"
					"precision highp float;\n"
					"layout(set=0, binding=0, rgba8) uniform image2D u_image;\n"
					"layout(location = 0) out highp vec4 o_color;\n"
					"void main (void) {\n"
					"\thighp uvec2 size = uvec2(imageSize(u_image).x, imageSize(u_image).y);\n"
					"\thighp uint valuesPerPixel = max(1u, (size.x * size.y) / (256u * 256u));\n"
					"\thighp uvec4 value = uvec4(uint(gl_FragCoord.x), uint(gl_FragCoord.y), 0u, 0u);\n"
					"\tfor (uint i = 0u; i < valuesPerPixel; i++)\n"
					"\t{\n"
					"\t\thighp vec4 floatValue = imageLoad(u_image, ivec2(int((value.z *  256u + (value.x ^ value.z)) % size.x), int((value.w * 256u + (value.y ^ value.w)) % size.y)));\n"
					"\t\tvalue = uvec4(uint(floatValue.x * 255.0), uint(floatValue.y * 255.0), uint(floatValue.z * 255.0), uint(floatValue.w * 255.0));\n"
					"\t}\n"
					"\to_color = vec4(value) / vec4(255.0);\n"
					"}\n";

				sources.glslSources.add("storage-image.frag")
					<< glu::FragmentSource(fragmentShader);
			}
		}

		if (config.usage & USAGE_SAMPLED_IMAGE)
		{
			{
				// Vertex storage image
				const char* const vertexShader =
					"#version 450\n"
					"precision highp float;\n"
					"layout(set=0, binding=0) uniform sampler2D u_sampler;\n"
					"out gl_PerVertex {\n"
					"\tvec4 gl_Position;\n"
					"\tfloat gl_PointSize;\n"
					"};\n"
					"void main (void) {\n"
					"\tgl_PointSize = 1.0;\n"
					"\thighp vec4 val = texelFetch(u_sampler, ivec2((gl_VertexIndex / 2) / textureSize(u_sampler, 0).x, (gl_VertexIndex / 2) % textureSize(u_sampler, 0).x), 0);\n"
					"\thighp vec2 pos;\n"
					"\tif (gl_VertexIndex % 2 == 0)\n"
					"\t\tpos = val.xy;\n"
					"\telse\n"
					"\t\tpos = val.zw;\n"
					"\tgl_Position = vec4(1.998 * pos - vec2(0.999), 0.0, 1.0);\n"
					"}\n";

				sources.glslSources.add("sampled-image.vert")
					<< glu::VertexSource(vertexShader);
			}
			{
				// Fragment storage image
				const char* const fragmentShader =
					"#version 450\n"
					"#extension GL_EXT_texture_buffer : require\n"
					"precision highp float;\n"
					"layout(set=0, binding=0) uniform sampler2D u_sampler;\n"
					"layout(location = 0) out highp vec4 o_color;\n"
					"void main (void) {\n"
					"\thighp uvec2 size = uvec2(textureSize(u_sampler, 0).x, textureSize(u_sampler, 0).y);\n"
					"\thighp uint valuesPerPixel = max(1u, (size.x * size.y) / (256u * 256u));\n"
					"\thighp uvec4 value = uvec4(uint(gl_FragCoord.x), uint(gl_FragCoord.y), 0u, 0u);\n"
					"\tfor (uint i = 0u; i < valuesPerPixel; i++)\n"
					"\t{\n"
					"\t\thighp vec4 floatValue = texelFetch(u_sampler, ivec2(int((value.z *  256u + (value.x ^ value.z)) % size.x), int((value.w * 256u + (value.y ^ value.w)) % size.y)), 0);\n"
					"\t\tvalue = uvec4(uint(floatValue.x * 255.0), uint(floatValue.y * 255.0), uint(floatValue.z * 255.0), uint(floatValue.w * 255.0));\n"
					"\t}\n"
					"\to_color = vec4(value) / vec4(255.0);\n"
					"}\n";

				sources.glslSources.add("sampled-image.frag")
					<< glu::FragmentSource(fragmentShader);
			}
		}

		{
			const char* const vertexShader =
				"#version 450\n"
				"out gl_PerVertex {\n"
				"\tvec4 gl_Position;\n"
				"};\n"
				"precision highp float;\n"
				"void main (void) {\n"
				"\tgl_Position = vec4(((gl_VertexIndex + 2) / 3) % 2 == 0 ? -1.0 : 1.0,\n"
				"\t                   ((gl_VertexIndex + 1) / 3) % 2 == 0 ? -1.0 : 1.0, 0.0, 1.0);\n"
				"}\n";

			sources.glslSources.add("render-quad.vert")
				<< glu::VertexSource(vertexShader);
		}

		{
			const char* const fragmentShader =
				"#version 310 es\n"
				"layout(location = 0) out highp vec4 o_color;\n"
				"void main (void) {\n"
				"\to_color = vec4(1.0);\n"
				"}\n";

			sources.glslSources.add("render-white.frag")
				<< glu::FragmentSource(fragmentShader);
		}
	}
};

} // anonymous

tcu::TestCaseGroup* createPipelineBarrierTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "pipeline_barrier", "Pipeline barrier tests."));
	const vk::VkDeviceSize			sizes[]			=
	{
		1024,		// 1K
		8*1024,		// 8K
		64*1024,	// 64K
		1024*1024,	// 1M
	};
	const Usage						usages[]		=
	{
		USAGE_HOST_READ,
		USAGE_HOST_WRITE,
		USAGE_TRANSFER_SRC,
		USAGE_TRANSFER_DST,
		USAGE_VERTEX_BUFFER,
		USAGE_INDEX_BUFFER,
		USAGE_UNIFORM_BUFFER,
		USAGE_UNIFORM_TEXEL_BUFFER,
		USAGE_STORAGE_BUFFER,
		USAGE_STORAGE_TEXEL_BUFFER,
		USAGE_STORAGE_IMAGE,
		USAGE_SAMPLED_IMAGE
	};
	const Usage						readUsages[]		=
	{
		USAGE_HOST_READ,
		USAGE_TRANSFER_SRC,
		USAGE_VERTEX_BUFFER,
		USAGE_INDEX_BUFFER,
		USAGE_UNIFORM_BUFFER,
		USAGE_UNIFORM_TEXEL_BUFFER,
		USAGE_STORAGE_BUFFER,
		USAGE_STORAGE_TEXEL_BUFFER,
		USAGE_STORAGE_IMAGE,
		USAGE_SAMPLED_IMAGE
	};

	const Usage						writeUsages[]	=
	{
		USAGE_HOST_WRITE,
		USAGE_TRANSFER_DST
	};

	for (size_t writeUsageNdx = 0; writeUsageNdx < DE_LENGTH_OF_ARRAY(writeUsages); writeUsageNdx++)
	{
		const Usage	writeUsage	= writeUsages[writeUsageNdx];

		for (size_t readUsageNdx = 0; readUsageNdx < DE_LENGTH_OF_ARRAY(readUsages); readUsageNdx++)
		{
			const Usage						readUsage		= readUsages[readUsageNdx];
			const Usage						usage			= writeUsage | readUsage;
			const string					usageGroupName	(usageToName(usage));
			de::MovePtr<tcu::TestCaseGroup>	usageGroup		(new tcu::TestCaseGroup(testCtx, usageGroupName.c_str(), usageGroupName.c_str()));

			for (size_t sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes); sizeNdx++)
			{
				const vk::VkDeviceSize	size		= sizes[sizeNdx];
				const string			testName	(de::toString((deUint64)(size)));
				const TestConfig		config		=
				{
					usage,
					size,
					vk::VK_SHARING_MODE_EXCLUSIVE
				};

				usageGroup->addChild(new InstanceFactory1<MemoryTestInstance, TestConfig, AddPrograms>(testCtx,tcu::NODETYPE_SELF_VALIDATE,  testName, testName, AddPrograms(), config));
			}

			group->addChild(usageGroup.get());
			usageGroup.release();
		}
	}

	{
		Usage all = (Usage)0;

		for (size_t usageNdx = 0; usageNdx < DE_LENGTH_OF_ARRAY(usages); usageNdx++)
			all = all | usages[usageNdx];

		{
			const string					usageGroupName	("all");
			de::MovePtr<tcu::TestCaseGroup>	usageGroup		(new tcu::TestCaseGroup(testCtx, usageGroupName.c_str(), usageGroupName.c_str()));

			for (size_t sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes); sizeNdx++)
			{
				const vk::VkDeviceSize	size		= sizes[sizeNdx];
				const string			testName	(de::toString((deUint64)(size)));
				const TestConfig		config		=
				{
					all,
					size,
					vk::VK_SHARING_MODE_EXCLUSIVE
				};

				usageGroup->addChild(new InstanceFactory1<MemoryTestInstance, TestConfig, AddPrograms>(testCtx,tcu::NODETYPE_SELF_VALIDATE,  testName, testName, AddPrograms(), config));
			}

			group->addChild(usageGroup.get());
			usageGroup.release();
		}

		{
			const string					usageGroupName	("all_device");
			de::MovePtr<tcu::TestCaseGroup>	usageGroup		(new tcu::TestCaseGroup(testCtx, usageGroupName.c_str(), usageGroupName.c_str()));

			for (size_t sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes); sizeNdx++)
			{
				const vk::VkDeviceSize	size		= sizes[sizeNdx];
				const string			testName	(de::toString((deUint64)(size)));
				const TestConfig		config		=
				{
					(Usage)(all & (~(USAGE_HOST_READ|USAGE_HOST_WRITE))),
					size,
					vk::VK_SHARING_MODE_EXCLUSIVE
				};

				usageGroup->addChild(new InstanceFactory1<MemoryTestInstance, TestConfig, AddPrograms>(testCtx,tcu::NODETYPE_SELF_VALIDATE,  testName, testName, AddPrograms(), config));
			}

			group->addChild(usageGroup.get());
			usageGroup.release();
		}
	}

	return group.release();
}

} // memory
} // vkt
