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

#include "vktYCbCrUtil.hpp"

#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"

#include "tcuTextureUtil.hpp"

#include "deSTLUtil.hpp"
#include "deUniquePtr.hpp"

namespace vkt
{
namespace ycbcr
{

using namespace vk;

using de::MovePtr;
using tcu::UVec2;
using std::vector;
using std::string;

typedef de::SharedPtr<Allocation>				AllocationSp;
typedef de::SharedPtr<vk::Unique<VkBuffer> >	VkBufferSp;

// MultiPlaneImageData

MultiPlaneImageData::MultiPlaneImageData (VkFormat format, const UVec2& size)
	: m_format		(format)
	, m_description	(getPlanarFormatDescription(format))
	, m_size		(size)
{
	for (deUint32 planeNdx = 0; planeNdx < m_description.numPlanes; ++planeNdx)
	{
		const deUint32	planeW		= size.x() / m_description.planes[planeNdx].widthDivisor;
		const deUint32	planeH		= size.y() / m_description.planes[planeNdx].heightDivisor;
		const deUint32	planeSize	= m_description.planes[planeNdx].elementSizeBytes * planeW * planeH;

		m_planeData[planeNdx].resize(planeSize);
	}
}

MultiPlaneImageData::MultiPlaneImageData (const MultiPlaneImageData& other)
	: m_format		(other.m_format)
	, m_description	(other.m_description)
	, m_size		(other.m_size)
{
	for (deUint32 planeNdx = 0; planeNdx < m_description.numPlanes; ++planeNdx)
		m_planeData[planeNdx] = other.m_planeData[planeNdx];
}

MultiPlaneImageData::~MultiPlaneImageData (void)
{
}

tcu::PixelBufferAccess MultiPlaneImageData::getChannelAccess (deUint32 channelNdx)
{
	void*		planePtrs[PlanarFormatDescription::MAX_PLANES];
	deUint32	planeRowPitches[PlanarFormatDescription::MAX_PLANES];

	for (deUint32 planeNdx = 0; planeNdx < m_description.numPlanes; ++planeNdx)
	{
		const deUint32	planeW		= m_size.x() / m_description.planes[planeNdx].widthDivisor;

		planeRowPitches[planeNdx]	= m_description.planes[planeNdx].elementSizeBytes * planeW;
		planePtrs[planeNdx]			= &m_planeData[planeNdx][0];
	}

	return vk::getChannelAccess(m_description,
								m_size,
								planeRowPitches,
								planePtrs,
								channelNdx);
}

tcu::ConstPixelBufferAccess MultiPlaneImageData::getChannelAccess (deUint32 channelNdx) const
{
	const void*	planePtrs[PlanarFormatDescription::MAX_PLANES];
	deUint32	planeRowPitches[PlanarFormatDescription::MAX_PLANES];

	for (deUint32 planeNdx = 0; planeNdx < m_description.numPlanes; ++planeNdx)
	{
		const deUint32	planeW		= m_size.x() / m_description.planes[planeNdx].widthDivisor;

		planeRowPitches[planeNdx]	= m_description.planes[planeNdx].elementSizeBytes * planeW;
		planePtrs[planeNdx]			= &m_planeData[planeNdx][0];
	}

	return vk::getChannelAccess(m_description,
								m_size,
								planeRowPitches,
								planePtrs,
								channelNdx);
}

// Misc utilities

namespace
{

void allocateStagingBuffers (const DeviceInterface&			vkd,
							 VkDevice						device,
							 Allocator&						allocator,
							 const MultiPlaneImageData&		imageData,
							 vector<VkBufferSp>*			buffers,
							 vector<AllocationSp>*			allocations)
{
	for (deUint32 planeNdx = 0; planeNdx < imageData.getDescription().numPlanes; ++planeNdx)
	{
		const VkBufferCreateInfo	bufferInfo	=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			DE_NULL,
			(VkBufferCreateFlags)0u,
			(VkDeviceSize)imageData.getPlaneSize(planeNdx),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0u,
			(const deUint32*)DE_NULL,
		};
		Move<VkBuffer>				buffer		(createBuffer(vkd, device, &bufferInfo));
		MovePtr<Allocation>			allocation	(allocator.allocate(getBufferMemoryRequirements(vkd, device, *buffer),
																	MemoryRequirement::HostVisible|MemoryRequirement::Any));

		VK_CHECK(vkd.bindBufferMemory(device, *buffer, allocation->getMemory(), allocation->getOffset()));

		buffers->push_back(VkBufferSp(new Unique<VkBuffer>(buffer)));
		allocations->push_back(AllocationSp(allocation.release()));
	}
}

void allocateAndWriteStagingBuffers (const DeviceInterface&		vkd,
									  VkDevice						device,
									  Allocator&					allocator,
									  const MultiPlaneImageData&	imageData,
									  vector<VkBufferSp>*			buffers,
									  vector<AllocationSp>*			allocations)
{
	allocateStagingBuffers(vkd, device, allocator, imageData, buffers, allocations);

	for (deUint32 planeNdx = 0; planeNdx < imageData.getDescription().numPlanes; ++planeNdx)
	{
		deMemcpy((*allocations)[planeNdx]->getHostPtr(), imageData.getPlanePtr(planeNdx), imageData.getPlaneSize(planeNdx));
		flushMappedMemoryRange(vkd, device, (*allocations)[planeNdx]->getMemory(), 0u, VK_WHOLE_SIZE);
	}
}

void readStagingBuffers (MultiPlaneImageData*			imageData,
						 const DeviceInterface&			vkd,
						 VkDevice						device,
						 const vector<AllocationSp>&	allocations)
{
	for (deUint32 planeNdx = 0; planeNdx < imageData->getDescription().numPlanes; ++planeNdx)
	{
		invalidateMappedMemoryRange(vkd, device, allocations[planeNdx]->getMemory(), 0u, VK_WHOLE_SIZE);
		deMemcpy(imageData->getPlanePtr(planeNdx), allocations[planeNdx]->getHostPtr(), imageData->getPlaneSize(planeNdx));
	}
}

} // anonymous

void checkImageSupport (Context& context, VkFormat format, VkImageCreateFlags createFlags, VkImageTiling tiling)
{
	const bool													disjoint	= (createFlags & VK_IMAGE_CREATE_DISJOINT_BIT_KHR) != 0;
	const VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR*	features	= findStructure<VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR>(context.getDeviceFeatures2().pNext);
	vector<string>												reqExts;

	reqExts.push_back("VK_KHR_sampler_ycbcr_conversion");

	if (disjoint)
	{
		reqExts.push_back("VK_KHR_bind_memory2");
		reqExts.push_back("VK_KHR_get_memory_requirements2");
	}

	for (vector<string>::const_iterator extIter = reqExts.begin(); extIter != reqExts.end(); ++extIter)
	{
		if (!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), *extIter))
			TCU_THROW(NotSupportedError, (*extIter + " is not supported").c_str());
	}

	if (!features || features->samplerYcbcrConversion == VK_FALSE)
		TCU_THROW(NotSupportedError, "samplerYcbcrConversion is not supported");

	{
		const VkFormatProperties	formatProperties	= getPhysicalDeviceFormatProperties(context.getInstanceInterface(),
																							context.getPhysicalDevice(),
																							format);
		const VkFormatFeatureFlags	featureFlags		= tiling == VK_IMAGE_TILING_OPTIMAL
														? formatProperties.optimalTilingFeatures
														: formatProperties.linearTilingFeatures;

		if ((featureFlags & (VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT_KHR | VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT_KHR)) == 0)
			TCU_THROW(NotSupportedError, "YCbCr conversion is not supported for format");

		if (disjoint && ((featureFlags & VK_FORMAT_FEATURE_DISJOINT_BIT_KHR) == 0))
			TCU_THROW(NotSupportedError, "Disjoint planes are not supported for format");
	}
}

void fillRandom (de::Random* randomGen, MultiPlaneImageData* imageData)
{
	// \todo [pyry] Optimize, take into account bits that must be 0

	for (deUint32 planeNdx = 0; planeNdx < imageData->getDescription().numPlanes; ++planeNdx)
	{
		const size_t	planeSize	= imageData->getPlaneSize(planeNdx);
		deUint8* const	planePtr	= (deUint8*)imageData->getPlanePtr(planeNdx);

		for (size_t ndx = 0; ndx < planeSize; ++ndx)
			planePtr[ndx] = randomGen->getUint8();
	}
}

void fillGradient (MultiPlaneImageData* imageData, const tcu::Vec4& minVal, const tcu::Vec4& maxVal)
{
	const PlanarFormatDescription&	formatInfo	= imageData->getDescription();

	// \todo [pyry] Optimize: no point in re-rendering source gradient for each channel.

	for (deUint32 channelNdx = 0; channelNdx < 4; channelNdx++)
	{
		if (formatInfo.hasChannelNdx(channelNdx))
		{
			const tcu::PixelBufferAccess		channelAccess	= imageData->getChannelAccess(channelNdx);
			tcu::TextureLevel					tmpTexture		(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT),  channelAccess.getWidth(), channelAccess.getHeight());
			const tcu::ConstPixelBufferAccess	tmpAccess		= tmpTexture.getAccess();

			tcu::fillWithComponentGradients(tmpTexture, minVal, maxVal);

			for (int y = 0; y < channelAccess.getHeight(); ++y)
			for (int x = 0; x < channelAccess.getWidth(); ++x)
			{
				channelAccess.setPixel(tcu::Vec4(tmpAccess.getPixel(x, y)[channelNdx]), x, y);
			}
		}
	}
}

vector<AllocationSp> allocateAndBindImageMemory (const DeviceInterface&	vkd,
												 VkDevice				device,
												 Allocator&				allocator,
												 VkImage				image,
												 VkFormat				format,
												 VkImageCreateFlags		createFlags,
												 vk::MemoryRequirement	requirement)
{
	vector<AllocationSp> allocations;

	if ((createFlags & VK_IMAGE_CREATE_DISJOINT_BIT_KHR) != 0)
	{
		const deUint32	numPlanes	= getPlaneCount(format);

		for (deUint32 planeNdx = 0; planeNdx < numPlanes; ++planeNdx)
		{
			const VkImageAspectFlagBits	planeAspect	= getPlaneAspect(planeNdx);
			const VkMemoryRequirements	reqs		= getImagePlaneMemoryRequirements(vkd, device, image, planeAspect);

			allocations.push_back(AllocationSp(allocator.allocate(reqs, requirement).release()));

			bindImagePlaneMemory(vkd, device, image, allocations.back()->getMemory(), allocations.back()->getOffset(), planeAspect);
		}
	}
	else
	{
		const VkMemoryRequirements	reqs	= getImageMemoryRequirements(vkd, device, image);

		allocations.push_back(AllocationSp(allocator.allocate(reqs, requirement).release()));

		VK_CHECK(vkd.bindImageMemory(device, image, allocations.back()->getMemory(), allocations.back()->getOffset()));
	}

	return allocations;
}

void uploadImage (const DeviceInterface&		vkd,
				  VkDevice						device,
				  deUint32						queueFamilyNdx,
				  Allocator&					allocator,
				  VkImage						image,
				  const MultiPlaneImageData&	imageData,
				  VkAccessFlags					nextAccess,
				  VkImageLayout					finalLayout)
{
	const VkQueue					queue			= getDeviceQueue(vkd, device, queueFamilyNdx, 0u);
	const Unique<VkCommandPool>		cmdPool			(createCommandPool(vkd, device, (VkCommandPoolCreateFlags)0, queueFamilyNdx));
	const Unique<VkCommandBuffer>	cmdBuffer		(allocateCommandBuffer(vkd, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	vector<VkBufferSp>				stagingBuffers;
	vector<AllocationSp>			stagingMemory;

	const PlanarFormatDescription&	formatDesc		= imageData.getDescription();

	allocateAndWriteStagingBuffers(vkd, device, allocator, imageData, &stagingBuffers, &stagingMemory);

	{
		const VkCommandBufferBeginInfo	beginInfo		=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			DE_NULL,
			(VkCommandBufferUsageFlags)VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			(const VkCommandBufferInheritanceInfo*)DE_NULL
		};

		VK_CHECK(vkd.beginCommandBuffer(*cmdBuffer, &beginInfo));
	}

	{
		const VkImageMemoryBarrier		preCopyBarrier	=
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,
			(VkAccessFlags)0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }
		};

		vkd.cmdPipelineBarrier(*cmdBuffer,
								(VkPipelineStageFlags)VK_PIPELINE_STAGE_HOST_BIT,
								(VkPipelineStageFlags)VK_PIPELINE_STAGE_TRANSFER_BIT,
								(VkDependencyFlags)0u,
								0u,
								(const VkMemoryBarrier*)DE_NULL,
								0u,
								(const VkBufferMemoryBarrier*)DE_NULL,
								1u,
								&preCopyBarrier);
	}

	for (deUint32 planeNdx = 0; planeNdx < imageData.getDescription().numPlanes; ++planeNdx)
	{
		const VkImageAspectFlagBits	aspect	= (formatDesc.numPlanes > 1)
											? getPlaneAspect(planeNdx)
											: VK_IMAGE_ASPECT_COLOR_BIT;
		const deUint32				planeW	= (formatDesc.numPlanes > 1)
											? imageData.getSize().x() / formatDesc.planes[planeNdx].widthDivisor
											: imageData.getSize().x();
		const deUint32				planeH	= (formatDesc.numPlanes > 1)
											? imageData.getSize().y() / formatDesc.planes[planeNdx].heightDivisor
											: imageData.getSize().y();
		const VkBufferImageCopy		copy	=
		{
			0u,		// bufferOffset
			0u,		// bufferRowLength
			0u,		// bufferImageHeight
			{ (VkImageAspectFlags)aspect, 0u, 0u, 1u },
			makeOffset3D(0u, 0u, 0u),
			makeExtent3D(planeW, planeH, 1u),
		};

		vkd.cmdCopyBufferToImage(*cmdBuffer, **stagingBuffers[planeNdx], image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copy);
	}

	{
		const VkImageMemoryBarrier		postCopyBarrier	=
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			nextAccess,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			finalLayout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }
		};

		vkd.cmdPipelineBarrier(*cmdBuffer,
								(VkPipelineStageFlags)VK_PIPELINE_STAGE_TRANSFER_BIT,
								(VkPipelineStageFlags)VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
								(VkDependencyFlags)0u,
								0u,
								(const VkMemoryBarrier*)DE_NULL,
								0u,
								(const VkBufferMemoryBarrier*)DE_NULL,
								1u,
								&postCopyBarrier);
	}

	VK_CHECK(vkd.endCommandBuffer(*cmdBuffer));

	{
		const Unique<VkFence>	fence		(createFence(vkd, device));
		const VkSubmitInfo		submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			DE_NULL,
			0u,
			(const VkSemaphore*)DE_NULL,
			(const VkPipelineStageFlags*)DE_NULL,
			1u,
			&*cmdBuffer,
			0u,
			(const VkSemaphore*)DE_NULL,
		};

		VK_CHECK(vkd.queueSubmit(queue, 1u, &submitInfo, *fence));
		VK_CHECK(vkd.waitForFences(device, 1u, &*fence, VK_TRUE, ~0ull));
	}
}

void fillImageMemory (const vk::DeviceInterface&							vkd,
					  vk::VkDevice											device,
					  deUint32												queueFamilyNdx,
					  vk::VkImage											image,
					  const std::vector<de::SharedPtr<vk::Allocation> >&	allocations,
					  const MultiPlaneImageData&							imageData,
					  vk::VkAccessFlags										nextAccess,
					  vk::VkImageLayout										finalLayout)
{
	const VkQueue					queue			= getDeviceQueue(vkd, device, queueFamilyNdx, 0u);
	const Unique<VkCommandPool>		cmdPool			(createCommandPool(vkd, device, (VkCommandPoolCreateFlags)0, queueFamilyNdx));
	const Unique<VkCommandBuffer>	cmdBuffer		(allocateCommandBuffer(vkd, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const PlanarFormatDescription&	formatDesc		= imageData.getDescription();

	for (deUint32 planeNdx = 0; planeNdx < formatDesc.numPlanes; ++planeNdx)
	{
		const VkImageAspectFlagBits			aspect		= (formatDesc.numPlanes > 1)
														? getPlaneAspect(planeNdx)
														: VK_IMAGE_ASPECT_COLOR_BIT;
		const de::SharedPtr<Allocation>&	allocation	= allocations.size() > 1
														? allocations[planeNdx]
														: allocations[0];
		const size_t						planeSize	= imageData.getPlaneSize(planeNdx);
		const deUint32						planeH		= imageData.getSize().y() / formatDesc.planes[planeNdx].heightDivisor;
		const VkImageSubresource			subresource	=
		{
			aspect,
			0u,
			0u,
		};
		VkSubresourceLayout			layout;

		vkd.getImageSubresourceLayout(device, image, &subresource, &layout);

		for (deUint32 row = 0; row < planeH; ++row)
		{
			const size_t		rowSize		= planeSize / planeH;
			void* const			dstPtr		= ((deUint8*)allocation->getHostPtr()) + layout.offset + layout.rowPitch * row;
			const void* const	srcPtr		= ((const deUint8*)imageData.getPlanePtr(planeNdx)) + row * rowSize;

			deMemcpy(dstPtr, srcPtr, rowSize);
		}
		flushMappedMemoryRange(vkd, device, allocation->getMemory(), 0u, VK_WHOLE_SIZE);
	}

	{
		const VkCommandBufferBeginInfo	beginInfo		=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			DE_NULL,
			(VkCommandBufferUsageFlags)VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			(const VkCommandBufferInheritanceInfo*)DE_NULL
		};

		VK_CHECK(vkd.beginCommandBuffer(*cmdBuffer, &beginInfo));
	}


	{
		const VkImageMemoryBarrier		postCopyBarrier	=
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,
			0u,
			nextAccess,
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			finalLayout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }
		};

		vkd.cmdPipelineBarrier(*cmdBuffer,
								(VkPipelineStageFlags)VK_PIPELINE_STAGE_HOST_BIT,
								(VkPipelineStageFlags)VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
								(VkDependencyFlags)0u,
								0u,
								(const VkMemoryBarrier*)DE_NULL,
								0u,
								(const VkBufferMemoryBarrier*)DE_NULL,
								1u,
								&postCopyBarrier);
	}

	VK_CHECK(vkd.endCommandBuffer(*cmdBuffer));

	{
		const Unique<VkFence>	fence		(createFence(vkd, device));
		const VkSubmitInfo		submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			DE_NULL,
			0u,
			(const VkSemaphore*)DE_NULL,
			(const VkPipelineStageFlags*)DE_NULL,
			1u,
			&*cmdBuffer,
			0u,
			(const VkSemaphore*)DE_NULL,
		};

		VK_CHECK(vkd.queueSubmit(queue, 1u, &submitInfo, *fence));
		VK_CHECK(vkd.waitForFences(device, 1u, &*fence, VK_TRUE, ~0ull));
	}
}

void downloadImage (const DeviceInterface&	vkd,
					VkDevice				device,
					deUint32				queueFamilyNdx,
					Allocator&				allocator,
					VkImage					image,
					MultiPlaneImageData*	imageData,
					VkAccessFlags			prevAccess,
					VkImageLayout			initialLayout)
{
	const VkQueue					queue			= getDeviceQueue(vkd, device, queueFamilyNdx, 0u);
	const Unique<VkCommandPool>		cmdPool			(createCommandPool(vkd, device, (VkCommandPoolCreateFlags)0, queueFamilyNdx));
	const Unique<VkCommandBuffer>	cmdBuffer		(allocateCommandBuffer(vkd, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	vector<VkBufferSp>				stagingBuffers;
	vector<AllocationSp>			stagingMemory;

	const PlanarFormatDescription&	formatDesc		= imageData->getDescription();

	allocateStagingBuffers(vkd, device, allocator, *imageData, &stagingBuffers, &stagingMemory);

	{
		const VkCommandBufferBeginInfo	beginInfo		=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			DE_NULL,
			(VkCommandBufferUsageFlags)VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			(const VkCommandBufferInheritanceInfo*)DE_NULL
		};

		VK_CHECK(vkd.beginCommandBuffer(*cmdBuffer, &beginInfo));
	}

	for (deUint32 planeNdx = 0; planeNdx < imageData->getDescription().numPlanes; ++planeNdx)
	{
		const VkImageAspectFlagBits	aspect	= (formatDesc.numPlanes > 1)
											? getPlaneAspect(planeNdx)
											: VK_IMAGE_ASPECT_COLOR_BIT;
		{
			const VkImageMemoryBarrier		preCopyBarrier	=
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				DE_NULL,
				prevAccess,
				VK_ACCESS_TRANSFER_READ_BIT,
				initialLayout,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				image,
				{
					aspect,
					0u,
					1u,
					0u,
					1u
				}
			};

			vkd.cmdPipelineBarrier(*cmdBuffer,
									(VkPipelineStageFlags)VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
									(VkPipelineStageFlags)VK_PIPELINE_STAGE_TRANSFER_BIT,
									(VkDependencyFlags)0u,
									0u,
									(const VkMemoryBarrier*)DE_NULL,
									0u,
									(const VkBufferMemoryBarrier*)DE_NULL,
									1u,
									&preCopyBarrier);
		}
		{
			const deUint32				planeW	= (formatDesc.numPlanes > 1)
												? imageData->getSize().x() / formatDesc.planes[planeNdx].widthDivisor
												: imageData->getSize().x();
			const deUint32				planeH	= (formatDesc.numPlanes > 1)
												? imageData->getSize().y() / formatDesc.planes[planeNdx].heightDivisor
												: imageData->getSize().y();
			const VkBufferImageCopy		copy	=
			{
				0u,		// bufferOffset
				0u,		// bufferRowLength
				0u,		// bufferImageHeight
				{ (VkImageAspectFlags)aspect, 0u, 0u, 1u },
				makeOffset3D(0u, 0u, 0u),
				makeExtent3D(planeW, planeH, 1u),
			};

			vkd.cmdCopyImageToBuffer(*cmdBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, **stagingBuffers[planeNdx], 1u, &copy);
		}
		{
			const VkBufferMemoryBarrier		postCopyBarrier	=
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				DE_NULL,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_HOST_READ_BIT,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				**stagingBuffers[planeNdx],
				0u,
				VK_WHOLE_SIZE
			};

			vkd.cmdPipelineBarrier(*cmdBuffer,
									(VkPipelineStageFlags)VK_PIPELINE_STAGE_TRANSFER_BIT,
									(VkPipelineStageFlags)VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
									(VkDependencyFlags)0u,
									0u,
									(const VkMemoryBarrier*)DE_NULL,
									1u,
									&postCopyBarrier,
									0u,
									(const VkImageMemoryBarrier*)DE_NULL);
		}
	}

	VK_CHECK(vkd.endCommandBuffer(*cmdBuffer));

	{
		const Unique<VkFence>	fence		(createFence(vkd, device));
		const VkSubmitInfo		submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			DE_NULL,
			0u,
			(const VkSemaphore*)DE_NULL,
			(const VkPipelineStageFlags*)DE_NULL,
			1u,
			&*cmdBuffer,
			0u,
			(const VkSemaphore*)DE_NULL,
		};

		VK_CHECK(vkd.queueSubmit(queue, 1u, &submitInfo, *fence));
		VK_CHECK(vkd.waitForFences(device, 1u, &*fence, VK_TRUE, ~0ull));
	}

	readStagingBuffers(imageData, vkd, device, stagingMemory);
}

void readImageMemory (const vk::DeviceInterface&							vkd,
					  vk::VkDevice											device,
					  deUint32												queueFamilyNdx,
					  vk::VkImage											image,
					  const std::vector<de::SharedPtr<vk::Allocation> >&	allocations,
					  MultiPlaneImageData*									imageData,
					  vk::VkAccessFlags										prevAccess,
					  vk::VkImageLayout										initialLayout)
{
	const VkQueue					queue			= getDeviceQueue(vkd, device, queueFamilyNdx, 0u);
	const Unique<VkCommandPool>		cmdPool			(createCommandPool(vkd, device, (VkCommandPoolCreateFlags)0, queueFamilyNdx));
	const Unique<VkCommandBuffer>	cmdBuffer		(allocateCommandBuffer(vkd, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const PlanarFormatDescription&	formatDesc		= imageData->getDescription();

	{
		const VkCommandBufferBeginInfo	beginInfo		=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			DE_NULL,
			(VkCommandBufferUsageFlags)VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			(const VkCommandBufferInheritanceInfo*)DE_NULL
		};

		VK_CHECK(vkd.beginCommandBuffer(*cmdBuffer, &beginInfo));
	}

	{
		const VkImageMemoryBarrier		preCopyBarrier	=
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,
			prevAccess,
			vk::VK_ACCESS_HOST_READ_BIT,
			initialLayout,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }
		};

		vkd.cmdPipelineBarrier(*cmdBuffer,
								(VkPipelineStageFlags)VK_PIPELINE_STAGE_HOST_BIT,
								(VkPipelineStageFlags)VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
								(VkDependencyFlags)0u,
								0u,
								(const VkMemoryBarrier*)DE_NULL,
								0u,
								(const VkBufferMemoryBarrier*)DE_NULL,
								1u,
								&preCopyBarrier);
	}

	VK_CHECK(vkd.endCommandBuffer(*cmdBuffer));

	{
		const Unique<VkFence>	fence		(createFence(vkd, device));
		const VkSubmitInfo		submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			DE_NULL,
			0u,
			(const VkSemaphore*)DE_NULL,
			(const VkPipelineStageFlags*)DE_NULL,
			1u,
			&*cmdBuffer,
			0u,
			(const VkSemaphore*)DE_NULL,
		};

		VK_CHECK(vkd.queueSubmit(queue, 1u, &submitInfo, *fence));
		VK_CHECK(vkd.waitForFences(device, 1u, &*fence, VK_TRUE, ~0ull));
	}

	for (deUint32 planeNdx = 0; planeNdx < formatDesc.numPlanes; ++planeNdx)
	{
		const VkImageAspectFlagBits			aspect		= (formatDesc.numPlanes > 1)
														? getPlaneAspect(planeNdx)
														: VK_IMAGE_ASPECT_COLOR_BIT;
		const de::SharedPtr<Allocation>&	allocation	= allocations.size() > 1
														? allocations[planeNdx]
														: allocations[0];
		const size_t						planeSize	= imageData->getPlaneSize(planeNdx);
		const deUint32						planeH		= imageData->getSize().y() / formatDesc.planes[planeNdx].heightDivisor;
		const VkImageSubresource			subresource	=
		{
			aspect,
			0u,
			0u,
		};
		VkSubresourceLayout			layout;

		vkd.getImageSubresourceLayout(device, image, &subresource, &layout);

		invalidateMappedMemoryRange(vkd, device, allocation->getMemory(), 0u, VK_WHOLE_SIZE);

		for (deUint32 row = 0; row < planeH; ++row)
		{
			const size_t		rowSize	= planeSize / planeH;
			const void* const	srcPtr	= ((const deUint8*)allocation->getHostPtr()) + layout.offset + layout.rowPitch * row;
			void* const			dstPtr	= ((deUint8*)imageData->getPlanePtr(planeNdx)) + row * rowSize;

			deMemcpy(dstPtr, srcPtr, rowSize);
		}
	}
}

} // ycbcr
} // vkt
