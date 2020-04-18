#ifndef _VKTSYNCHRONIZATIONOPERATION_HPP
#define _VKTSYNCHRONIZATIONOPERATION_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Synchronization operation abstraction
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vkDefs.hpp"
#include "vkPrograms.hpp"
#include "vktTestCase.hpp"
#include "vktSynchronizationUtil.hpp"
#include "tcuVector.hpp"
#include "deUniquePtr.hpp"
#include <string>

namespace vkt
{
namespace synchronization
{

enum OperationName
{
	// Write operations
	OPERATION_NAME_WRITE_FILL_BUFFER,
	OPERATION_NAME_WRITE_UPDATE_BUFFER,
	OPERATION_NAME_WRITE_COPY_BUFFER,
	OPERATION_NAME_WRITE_COPY_BUFFER_TO_IMAGE,
	OPERATION_NAME_WRITE_COPY_IMAGE_TO_BUFFER,
	OPERATION_NAME_WRITE_COPY_IMAGE,
	OPERATION_NAME_WRITE_BLIT_IMAGE,
	OPERATION_NAME_WRITE_SSBO_VERTEX,
	OPERATION_NAME_WRITE_SSBO_TESSELLATION_CONTROL,
	OPERATION_NAME_WRITE_SSBO_TESSELLATION_EVALUATION,
	OPERATION_NAME_WRITE_SSBO_GEOMETRY,
	OPERATION_NAME_WRITE_SSBO_FRAGMENT,
	OPERATION_NAME_WRITE_SSBO_COMPUTE,
	OPERATION_NAME_WRITE_SSBO_COMPUTE_INDIRECT,
	OPERATION_NAME_WRITE_IMAGE_VERTEX,
	OPERATION_NAME_WRITE_IMAGE_TESSELLATION_CONTROL,
	OPERATION_NAME_WRITE_IMAGE_TESSELLATION_EVALUATION,
	OPERATION_NAME_WRITE_IMAGE_GEOMETRY,
	OPERATION_NAME_WRITE_IMAGE_FRAGMENT,
	OPERATION_NAME_WRITE_IMAGE_COMPUTE,
	OPERATION_NAME_WRITE_IMAGE_COMPUTE_INDIRECT,
	OPERATION_NAME_WRITE_CLEAR_COLOR_IMAGE,
	OPERATION_NAME_WRITE_CLEAR_DEPTH_STENCIL_IMAGE,
	OPERATION_NAME_WRITE_DRAW,
	OPERATION_NAME_WRITE_DRAW_INDEXED,
	OPERATION_NAME_WRITE_DRAW_INDIRECT,
	OPERATION_NAME_WRITE_DRAW_INDEXED_INDIRECT,
	OPERATION_NAME_WRITE_CLEAR_ATTACHMENTS,
	OPERATION_NAME_WRITE_INDIRECT_BUFFER_DRAW,
	OPERATION_NAME_WRITE_INDIRECT_BUFFER_DRAW_INDEXED,
	OPERATION_NAME_WRITE_INDIRECT_BUFFER_DISPATCH,

	// Read operations
	OPERATION_NAME_READ_COPY_BUFFER,
	OPERATION_NAME_READ_COPY_BUFFER_TO_IMAGE,
	OPERATION_NAME_READ_COPY_IMAGE_TO_BUFFER,
	OPERATION_NAME_READ_COPY_IMAGE,
	OPERATION_NAME_READ_BLIT_IMAGE,
	OPERATION_NAME_READ_UBO_VERTEX,
	OPERATION_NAME_READ_UBO_TESSELLATION_CONTROL,
	OPERATION_NAME_READ_UBO_TESSELLATION_EVALUATION,
	OPERATION_NAME_READ_UBO_GEOMETRY,
	OPERATION_NAME_READ_UBO_FRAGMENT,
	OPERATION_NAME_READ_UBO_COMPUTE,
	OPERATION_NAME_READ_UBO_COMPUTE_INDIRECT,
	OPERATION_NAME_READ_SSBO_VERTEX,
	OPERATION_NAME_READ_SSBO_TESSELLATION_CONTROL,
	OPERATION_NAME_READ_SSBO_TESSELLATION_EVALUATION,
	OPERATION_NAME_READ_SSBO_GEOMETRY,
	OPERATION_NAME_READ_SSBO_FRAGMENT,
	OPERATION_NAME_READ_SSBO_COMPUTE,
	OPERATION_NAME_READ_SSBO_COMPUTE_INDIRECT,
	OPERATION_NAME_READ_IMAGE_VERTEX,
	OPERATION_NAME_READ_IMAGE_TESSELLATION_CONTROL,
	OPERATION_NAME_READ_IMAGE_TESSELLATION_EVALUATION,
	OPERATION_NAME_READ_IMAGE_GEOMETRY,
	OPERATION_NAME_READ_IMAGE_FRAGMENT,
	OPERATION_NAME_READ_IMAGE_COMPUTE,
	OPERATION_NAME_READ_IMAGE_COMPUTE_INDIRECT,
	OPERATION_NAME_READ_INDIRECT_BUFFER_DRAW,
	OPERATION_NAME_READ_INDIRECT_BUFFER_DRAW_INDEXED,
	OPERATION_NAME_READ_INDIRECT_BUFFER_DISPATCH,
	OPERATION_NAME_READ_VERTEX_INPUT,
};

// Similar to Context, but allows test instance to decide which resources are used by the operation.
// E.g. this is needed when we want operation to work on a particular queue instead of the universal queue.
class OperationContext
{
public:
												OperationContext		(Context&			context,
																		 PipelineCacheData&	pipelineCacheData);

												OperationContext		(Context&					context,
																		 PipelineCacheData&			pipelineCacheData,
																		 const vk::DeviceInterface&	vk,
																		 const vk::VkDevice			device,
																		 vk::Allocator&				allocator);

												OperationContext		(const vk::InstanceInterface&				vki,
																		 const vk::DeviceInterface&					vkd,
																		 vk::VkPhysicalDevice						physicalDevice,
																		 vk::VkDevice								device,
																		 vk::Allocator&								allocator,
																		 const std::vector<std::string>&			deviceExtensions,
																		 vk::ProgramCollection<vk::ProgramBinary>&	programCollection,
																		 PipelineCacheData&							pipelineCacheData);

	const vk::InstanceInterface&				getInstanceInterface	(void) const { return m_vki; }
	const vk::DeviceInterface&					getDeviceInterface		(void) const { return m_vk; }
	vk::VkPhysicalDevice						getPhysicalDevice		(void) const { return m_physicalDevice; }
	vk::VkDevice								getDevice				(void) const { return m_device; }
	vk::Allocator&								getAllocator			(void) const { return m_allocator; }
	vk::ProgramCollection<vk::ProgramBinary>&	getBinaryCollection		(void) const { return m_progCollection; }
	PipelineCacheData&							getPipelineCacheData	(void) const { return m_pipelineCacheData; }
	const std::vector<std::string>&				getDeviceExtensions		(void) const { return m_deviceExtensions;}

private:
	const vk::InstanceInterface&				m_vki;
	const vk::DeviceInterface&					m_vk;
	const vk::VkPhysicalDevice					m_physicalDevice;
	const vk::VkDevice							m_device;
	vk::Allocator&								m_allocator;
	vk::ProgramCollection<vk::ProgramBinary>&	m_progCollection;
	PipelineCacheData&							m_pipelineCacheData;
	const std::vector<std::string>&				m_deviceExtensions;

	// Disabled
												OperationContext		(const OperationContext&);
	OperationContext&							operator=				(const OperationContext&);
};

// Common interface to images and buffers used by operations.
class Resource
{
public:
							Resource	(OperationContext&				context,
										 const ResourceDescription&		desc,
										 const deUint32					usage,
										 const vk::VkSharingMode		sharingMode = vk::VK_SHARING_MODE_EXCLUSIVE,
										 const std::vector<deUint32>&	queueFamilyIndex = std::vector<deUint32>());

							Resource	(ResourceType					type,
										 vk::Move<vk::VkBuffer>			buffer,
										 de::MovePtr<vk::Allocation>	allocation,
										 vk::VkDeviceSize				offset,
										 vk::VkDeviceSize				size);

							Resource	(vk::Move<vk::VkImage>			image,
										 de::MovePtr<vk::Allocation>	allocation,
										 const vk::VkExtent3D&			extent,
										 vk::VkImageType				imageType,
										 vk::VkFormat					format,
										 vk::VkImageSubresourceRange	subresourceRange,
										 vk::VkImageSubresourceLayers	subresourceLayers);

	ResourceType			getType		(void) const { return m_type; }
	const BufferResource&	getBuffer	(void) const { return m_bufferData; }
	const ImageResource&	getImage	(void) const { return m_imageData; }

	vk::VkDeviceMemory		getMemory	(void) const;

private:
	const ResourceType		m_type;
	de::MovePtr<Buffer>		m_buffer;
	BufferResource			m_bufferData;
	de::MovePtr<Image>		m_image;
	ImageResource			m_imageData;
};

// \note Meaning of image layout is different for read and write types of operations:
//       read  - the layout image must be in before being passed to the read operation
//       write - the layout image will be in after the write operation has finished
struct SyncInfo
{
	vk::VkPipelineStageFlags	stageMask;		// pipeline stage where read/write takes place
	vk::VkAccessFlags			accessMask;		// type of access that is performed
	vk::VkImageLayout			imageLayout;	// src (for reads) or dst (for writes) image layout
};

struct Data
{
	std::size_t					size;
	const deUint8*				data;
};

// Abstract operation on a resource
// \note Meaning of getData is different for read and write operations:
//       read  - data actually read by the operation
//       write - expected data that operation was supposed to write
// \note It's assumed that recordCommands is called only once (i.e. no multiple command buffers are using these commands).
class Operation
{
public:
						Operation		(void) {}
	virtual				~Operation		(void) {}

	virtual void		recordCommands	(const vk::VkCommandBuffer cmdBuffer) = 0;	// commands that carry out this operation
	virtual SyncInfo	getSyncInfo		(void) const = 0;							// data required to properly synchronize this operation
	virtual Data		getData			(void) const = 0;							// get raw data that was written to or read from actual resource

private:
						Operation		(const Operation&);
	Operation&			operator=		(const Operation&);
};

// A helper class to init programs and create the operation when context becomes available.
// Throws OperationInvalidResourceError when resource and operation combination is not possible (e.g. buffer-specific op on an image).
class OperationSupport
{
public:
									OperationSupport		(void) {}
	virtual							~OperationSupport		(void) {}

	virtual deUint32				getResourceUsageFlags	(void) const = 0;
	virtual vk::VkQueueFlags		getQueueFlags			(const OperationContext& context) const = 0;
	virtual void					initPrograms			(vk::SourceCollections&) const {}	//!< empty by default

	virtual de::MovePtr<Operation>	build					(OperationContext& context, Resource& resource) const = 0;

private:
									OperationSupport		(const OperationSupport&);
	OperationSupport&				operator=				(const OperationSupport&);
};

bool							isResourceSupported		(const OperationName opName, const ResourceDescription& resourceDesc);
de::MovePtr<OperationSupport>	makeOperationSupport	(const OperationName opName, const ResourceDescription& resourceDesc);
std::string						getOperationName		(const OperationName opName);

} // synchronization
} // vkt

#endif // _VKTSYNCHRONIZATIONOPERATION_HPP
