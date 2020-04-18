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
 * \file  vktSparseResourcesMipmapSparseResidency.cpp
 * \brief Sparse partially resident images with mipmaps tests
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesMipmapSparseResidency.hpp"
#include "vktSparseResourcesTestsUtil.hpp"
#include "vktSparseResourcesBase.hpp"
#include "vktTestCaseUtil.hpp"

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkMemUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"

#include <string>
#include <vector>

using namespace vk;

namespace vkt
{
namespace sparse
{
namespace
{

tcu::UVec3 alignedDivide (const VkExtent3D& extent, const VkExtent3D& divisor)
{
	tcu::UVec3 result;

	result.x() = extent.width  / divisor.width  + ((extent.width  % divisor.width)  ? 1u : 0u);
	result.y() = extent.height / divisor.height + ((extent.height % divisor.height) ? 1u : 0u);
	result.z() = extent.depth  / divisor.depth  + ((extent.depth  % divisor.depth)  ? 1u : 0u);

	return result;
}

class MipmapSparseResidencyCase : public TestCase
{
public:
					MipmapSparseResidencyCase	(tcu::TestContext&			testCtx,
												 const std::string&			name,
												 const std::string&			description,
												 const ImageType			imageType,
												 const tcu::UVec3&			imageSize,
												 const tcu::TextureFormat&	format);

	TestInstance*	createInstance				(Context&					context) const;

private:
	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const tcu::TextureFormat	m_format;
};

MipmapSparseResidencyCase::MipmapSparseResidencyCase (tcu::TestContext&			testCtx,
													  const std::string&		name,
													  const std::string&		description,
													  const ImageType			imageType,
													  const tcu::UVec3&			imageSize,
													  const tcu::TextureFormat&	format)
	: TestCase				(testCtx, name, description)
	, m_imageType			(imageType)
	, m_imageSize			(imageSize)
	, m_format				(format)
{
}

class MipmapSparseResidencyInstance : public SparseResourcesBaseInstance
{
public:
					MipmapSparseResidencyInstance	(Context&									 context,
													 const ImageType							 imageType,
													 const tcu::UVec3&							 imageSize,
													 const tcu::TextureFormat&					 format);

	tcu::TestStatus	iterate							(void);

private:

	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const tcu::TextureFormat	m_format;
};

MipmapSparseResidencyInstance::MipmapSparseResidencyInstance (Context&					context,
															  const ImageType			imageType,
															  const tcu::UVec3&			imageSize,
															  const tcu::TextureFormat&	format)
	: SparseResourcesBaseInstance	(context)
	, m_imageType					(imageType)
	, m_imageSize					(imageSize)
	, m_format						(format)
{
}

tcu::TestStatus MipmapSparseResidencyInstance::iterate (void)
{
	const InstanceInterface&	instance		= m_context.getInstanceInterface();
	const VkPhysicalDevice		physicalDevice	= m_context.getPhysicalDevice();
	VkImageCreateInfo			imageSparseInfo;
	std::vector<DeviceMemorySp>	deviceMemUniquePtrVec;

	// Check if image size does not exceed device limits
	if (!isImageSizeSupported(instance, physicalDevice, m_imageType, m_imageSize))
		TCU_THROW(NotSupportedError, "Image size not supported for device");

	// Check if device supports sparse operations for image type
	if (!checkSparseSupportForImageType(instance, physicalDevice, m_imageType))
		TCU_THROW(NotSupportedError, "Sparse residency for image type is not supported");

	imageSparseInfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageSparseInfo.pNext					= DE_NULL;
	imageSparseInfo.flags					= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
	imageSparseInfo.imageType				= mapImageType(m_imageType);
	imageSparseInfo.format					= mapTextureFormat(m_format);
	imageSparseInfo.extent					= makeExtent3D(getLayerSize(m_imageType, m_imageSize));
	imageSparseInfo.arrayLayers				= getNumLayers(m_imageType, m_imageSize);
	imageSparseInfo.samples					= VK_SAMPLE_COUNT_1_BIT;
	imageSparseInfo.tiling					= VK_IMAGE_TILING_OPTIMAL;
	imageSparseInfo.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imageSparseInfo.usage					= VK_IMAGE_USAGE_TRANSFER_DST_BIT |
											  VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageSparseInfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imageSparseInfo.queueFamilyIndexCount	= 0u;
	imageSparseInfo.pQueueFamilyIndices		= DE_NULL;

	if (m_imageType == IMAGE_TYPE_CUBE || m_imageType == IMAGE_TYPE_CUBE_ARRAY)
	{
		imageSparseInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	{
		VkImageFormatProperties imageFormatProperties;
		instance.getPhysicalDeviceImageFormatProperties(physicalDevice,
			imageSparseInfo.format,
			imageSparseInfo.imageType,
			imageSparseInfo.tiling,
			imageSparseInfo.usage,
			imageSparseInfo.flags,
			&imageFormatProperties);

		imageSparseInfo.mipLevels = getImageMaxMipLevels(imageFormatProperties, imageSparseInfo.extent);
	}

	// Check if device supports sparse operations for image format
	if (!checkSparseSupportForImageFormat(instance, physicalDevice, imageSparseInfo))
		TCU_THROW(NotSupportedError, "The image format does not support sparse operations");

	{
		// Create logical device supporting both sparse and compute operations
		QueueRequirementsVec queueRequirements;
		queueRequirements.push_back(QueueRequirements(VK_QUEUE_SPARSE_BINDING_BIT, 1u));
		queueRequirements.push_back(QueueRequirements(VK_QUEUE_COMPUTE_BIT, 1u));

		createDeviceSupportingQueues(queueRequirements);
	}

	const DeviceInterface&	deviceInterface	= getDeviceInterface();
	const Queue&			sparseQueue		= getQueue(VK_QUEUE_SPARSE_BINDING_BIT, 0);
	const Queue&			computeQueue	= getQueue(VK_QUEUE_COMPUTE_BIT, 0);

	// Create sparse image
	const Unique<VkImage> imageSparse(createImage(deviceInterface, getDevice(), &imageSparseInfo));

	// Create sparse image memory bind semaphore
	const Unique<VkSemaphore> imageMemoryBindSemaphore(createSemaphore(deviceInterface, getDevice()));

	{
		// Get sparse image general memory requirements
		const VkMemoryRequirements imageMemoryRequirements = getImageMemoryRequirements(deviceInterface, getDevice(), *imageSparse);

		// Check if required image memory size does not exceed device limits
		if (imageMemoryRequirements.size > getPhysicalDeviceProperties(instance, physicalDevice).limits.sparseAddressSpaceSize)
			TCU_THROW(NotSupportedError, "Required memory size for sparse resource exceeds device limits");

		DE_ASSERT((imageMemoryRequirements.size % imageMemoryRequirements.alignment) == 0);

		// Get sparse image sparse memory requirements
		const std::vector<VkSparseImageMemoryRequirements> sparseMemoryRequirements = getImageSparseMemoryRequirements(deviceInterface, getDevice(), *imageSparse);

		DE_ASSERT(sparseMemoryRequirements.size() != 0);

		const deUint32 colorAspectIndex		= getSparseAspectRequirementsIndex(sparseMemoryRequirements, VK_IMAGE_ASPECT_COLOR_BIT);
		const deUint32 metadataAspectIndex	= getSparseAspectRequirementsIndex(sparseMemoryRequirements, VK_IMAGE_ASPECT_METADATA_BIT);

		if (colorAspectIndex == NO_MATCH_FOUND)
			TCU_THROW(NotSupportedError, "Not supported image aspect - the test supports currently only VK_IMAGE_ASPECT_COLOR_BIT");

		const VkSparseImageMemoryRequirements	aspectRequirements	= sparseMemoryRequirements[colorAspectIndex];
		const VkImageAspectFlags				aspectMask			= aspectRequirements.formatProperties.aspectMask;
		const VkExtent3D						imageGranularity	= aspectRequirements.formatProperties.imageGranularity;

		DE_ASSERT((aspectRequirements.imageMipTailSize % imageMemoryRequirements.alignment) == 0);

		std::vector<VkSparseImageMemoryBind>	imageResidencyMemoryBinds;
		std::vector<VkSparseMemoryBind>			imageMipTailMemoryBinds;

		const deUint32							memoryType = findMatchingMemoryType(instance, physicalDevice, imageMemoryRequirements, MemoryRequirement::Any);

		if (memoryType == NO_MATCH_FOUND)
			return tcu::TestStatus::fail("No matching memory type found");

		// Bind memory for each layer
		for (deUint32 layerNdx = 0; layerNdx < imageSparseInfo.arrayLayers; ++layerNdx)
		{
			for (deUint32 mipLevelNdx = 0; mipLevelNdx < aspectRequirements.imageMipTailFirstLod; ++mipLevelNdx)
			{
				const VkExtent3D			mipExtent			= mipLevelExtents(imageSparseInfo.extent, mipLevelNdx);
				const tcu::UVec3			sparseBlocks		= alignedDivide(mipExtent, imageGranularity);
				const deUint32				numSparseBlocks		= sparseBlocks.x() * sparseBlocks.y() * sparseBlocks.z();
				const VkImageSubresource	subresource			= { aspectMask, mipLevelNdx, layerNdx };

				const VkSparseImageMemoryBind imageMemoryBind = makeSparseImageMemoryBind(deviceInterface, getDevice(),
					imageMemoryRequirements.alignment * numSparseBlocks, memoryType, subresource, makeOffset3D(0u, 0u, 0u), mipExtent);

				deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

				imageResidencyMemoryBinds.push_back(imageMemoryBind);
			}

			if (!(aspectRequirements.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) && aspectRequirements.imageMipTailFirstLod < imageSparseInfo.mipLevels)
			{
				const VkSparseMemoryBind imageMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
					aspectRequirements.imageMipTailSize, memoryType, aspectRequirements.imageMipTailOffset + layerNdx * aspectRequirements.imageMipTailStride);

				deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

				imageMipTailMemoryBinds.push_back(imageMipTailMemoryBind);
			}

			// Metadata
			if (metadataAspectIndex != NO_MATCH_FOUND)
			{
				const VkSparseImageMemoryRequirements metadataAspectRequirements = sparseMemoryRequirements[metadataAspectIndex];

				if (!(metadataAspectRequirements.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT))
				{
					const VkSparseMemoryBind imageMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
						metadataAspectRequirements.imageMipTailSize, memoryType,
						metadataAspectRequirements.imageMipTailOffset + layerNdx * metadataAspectRequirements.imageMipTailStride,
						VK_SPARSE_MEMORY_BIND_METADATA_BIT);

					deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

					imageMipTailMemoryBinds.push_back(imageMipTailMemoryBind);
				}
			}
		}

		if ((aspectRequirements.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) && aspectRequirements.imageMipTailFirstLod < imageSparseInfo.mipLevels)
		{
			const VkSparseMemoryBind imageMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
				aspectRequirements.imageMipTailSize, memoryType, aspectRequirements.imageMipTailOffset);

			deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

			imageMipTailMemoryBinds.push_back(imageMipTailMemoryBind);
		}

		// Metadata
		if (metadataAspectIndex != NO_MATCH_FOUND)
		{
			const VkSparseImageMemoryRequirements metadataAspectRequirements = sparseMemoryRequirements[metadataAspectIndex];

			if (metadataAspectRequirements.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT)
			{
				const VkSparseMemoryBind imageMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
					metadataAspectRequirements.imageMipTailSize, memoryType, metadataAspectRequirements.imageMipTailOffset,
					VK_SPARSE_MEMORY_BIND_METADATA_BIT);

				deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

				imageMipTailMemoryBinds.push_back(imageMipTailMemoryBind);
			}
		}

		VkBindSparseInfo bindSparseInfo =
		{
			VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,			//VkStructureType							sType;
			DE_NULL,									//const void*								pNext;
			0u,											//deUint32									waitSemaphoreCount;
			DE_NULL,									//const VkSemaphore*						pWaitSemaphores;
			0u,											//deUint32									bufferBindCount;
			DE_NULL,									//const VkSparseBufferMemoryBindInfo*		pBufferBinds;
			0u,											//deUint32									imageOpaqueBindCount;
			DE_NULL,									//const VkSparseImageOpaqueMemoryBindInfo*	pImageOpaqueBinds;
			0u,											//deUint32									imageBindCount;
			DE_NULL,									//const VkSparseImageMemoryBindInfo*		pImageBinds;
			1u,											//deUint32									signalSemaphoreCount;
			&imageMemoryBindSemaphore.get()				//const VkSemaphore*						pSignalSemaphores;
		};

		VkSparseImageMemoryBindInfo			imageResidencyBindInfo;
		VkSparseImageOpaqueMemoryBindInfo	imageMipTailBindInfo;

		if (imageResidencyMemoryBinds.size() > 0)
		{
			imageResidencyBindInfo.image		= *imageSparse;
			imageResidencyBindInfo.bindCount	= static_cast<deUint32>(imageResidencyMemoryBinds.size());
			imageResidencyBindInfo.pBinds		= &imageResidencyMemoryBinds[0];

			bindSparseInfo.imageBindCount		= 1u;
			bindSparseInfo.pImageBinds			= &imageResidencyBindInfo;
		}

		if (imageMipTailMemoryBinds.size() > 0)
		{
			imageMipTailBindInfo.image			= *imageSparse;
			imageMipTailBindInfo.bindCount		= static_cast<deUint32>(imageMipTailMemoryBinds.size());
			imageMipTailBindInfo.pBinds			= &imageMipTailMemoryBinds[0];

			bindSparseInfo.imageOpaqueBindCount	= 1u;
			bindSparseInfo.pImageOpaqueBinds	= &imageMipTailBindInfo;
		}

		// Submit sparse bind commands for execution
		VK_CHECK(deviceInterface.queueBindSparse(sparseQueue.queueHandle, 1u, &bindSparseInfo, DE_NULL));
	}

	// Create command buffer for compute and transfer oparations
	const Unique<VkCommandPool>	  commandPool(makeCommandPool(deviceInterface, getDevice(), computeQueue.queueFamilyIndex));
	const Unique<VkCommandBuffer> commandBuffer(allocateCommandBuffer(deviceInterface, getDevice(), *commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	std::vector <VkBufferImageCopy> bufferImageCopy(imageSparseInfo.mipLevels);

	{
		deUint32 bufferOffset = 0;
		for (deUint32 mipmapNdx = 0; mipmapNdx < imageSparseInfo.mipLevels; mipmapNdx++)
		{
			bufferImageCopy[mipmapNdx] = makeBufferImageCopy(mipLevelExtents(imageSparseInfo.extent, mipmapNdx), imageSparseInfo.arrayLayers, mipmapNdx, static_cast<VkDeviceSize>(bufferOffset));
			bufferOffset += getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, mipmapNdx, BUFFER_IMAGE_COPY_OFFSET_GRANULARITY);
		}
	}

	// Start recording commands
	beginCommandBuffer(deviceInterface, *commandBuffer);

	const deUint32					imageSizeInBytes		= getImageSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, imageSparseInfo.mipLevels, BUFFER_IMAGE_COPY_OFFSET_GRANULARITY);
	const VkBufferCreateInfo		inputBufferCreateInfo	= makeBufferCreateInfo(imageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	const Unique<VkBuffer>			inputBuffer				(createBuffer(deviceInterface, getDevice(), &inputBufferCreateInfo));
	const de::UniquePtr<Allocation>	inputBufferAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *inputBuffer, MemoryRequirement::HostVisible));

	std::vector<deUint8> referenceData(imageSizeInBytes);

	const VkMemoryRequirements imageMemoryRequirements = getImageMemoryRequirements(deviceInterface, getDevice(), *imageSparse);

	for (deUint32 valueNdx = 0; valueNdx < imageSizeInBytes; ++valueNdx)
	{
		referenceData[valueNdx] = static_cast<deUint8>((valueNdx % imageMemoryRequirements.alignment) + 1u);
	}

	deMemcpy(inputBufferAlloc->getHostPtr(), &referenceData[0], imageSizeInBytes);

	flushMappedMemoryRange(deviceInterface, getDevice(), inputBufferAlloc->getMemory(), inputBufferAlloc->getOffset(), imageSizeInBytes);

	{
		const VkBufferMemoryBarrier inputBufferBarrier = makeBufferMemoryBarrier
		(
			VK_ACCESS_HOST_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			*inputBuffer,
			0u,
			imageSizeInBytes
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 1u, &inputBufferBarrier, 0u, DE_NULL);
	}

	{
		const VkImageMemoryBarrier imageSparseTransferDstBarrier = makeImageMemoryBarrier
		(
			0u,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			sparseQueue.queueFamilyIndex != computeQueue.queueFamilyIndex ? sparseQueue.queueFamilyIndex : VK_QUEUE_FAMILY_IGNORED,
			sparseQueue.queueFamilyIndex != computeQueue.queueFamilyIndex ? computeQueue.queueFamilyIndex : VK_QUEUE_FAMILY_IGNORED,
			*imageSparse,
			makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, imageSparseInfo.mipLevels, 0u, imageSparseInfo.arrayLayers)
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imageSparseTransferDstBarrier);
	}

	deviceInterface.cmdCopyBufferToImage(*commandBuffer, *inputBuffer, *imageSparse, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<deUint32>(bufferImageCopy.size()), &bufferImageCopy[0]);

	{
		const VkImageMemoryBarrier imageSparseTransferSrcBarrier = makeImageMemoryBarrier
		(
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*imageSparse,
			makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, imageSparseInfo.mipLevels, 0u, imageSparseInfo.arrayLayers)
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imageSparseTransferSrcBarrier);
	}

	const VkBufferCreateInfo		outputBufferCreateInfo	= makeBufferCreateInfo(imageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	const Unique<VkBuffer>			outputBuffer			(createBuffer(deviceInterface, getDevice(), &outputBufferCreateInfo));
	const de::UniquePtr<Allocation>	outputBufferAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *outputBuffer, MemoryRequirement::HostVisible));

	deviceInterface.cmdCopyImageToBuffer(*commandBuffer, *imageSparse, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *outputBuffer, static_cast<deUint32>(bufferImageCopy.size()), &bufferImageCopy[0]);

	{
		const VkBufferMemoryBarrier outputBufferBarrier = makeBufferMemoryBarrier
		(
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_HOST_READ_BIT,
			*outputBuffer,
			0u,
			imageSizeInBytes
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL, 1u, &outputBufferBarrier, 0u, DE_NULL);
	}

	// End recording commands
	endCommandBuffer(deviceInterface, *commandBuffer);

	const VkPipelineStageFlags stageBits[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	// Submit commands for execution and wait for completion
	submitCommandsAndWait(deviceInterface, getDevice(), computeQueue.queueHandle, *commandBuffer, 1u, &imageMemoryBindSemaphore.get(), stageBits);

	// Retrieve data from buffer to host memory
	invalidateMappedMemoryRange(deviceInterface, getDevice(), outputBufferAlloc->getMemory(), outputBufferAlloc->getOffset(), imageSizeInBytes);

	const deUint8* outputData = static_cast<const deUint8*>(outputBufferAlloc->getHostPtr());

	// Wait for sparse queue to become idle
	deviceInterface.queueWaitIdle(sparseQueue.queueHandle);

	for (deUint32 mipmapNdx = 0; mipmapNdx < imageSparseInfo.mipLevels; ++mipmapNdx)
	{
		const deUint32 mipLevelSizeInBytes	= getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, mipmapNdx);
		const deUint32 bufferOffset			= static_cast<deUint32>(bufferImageCopy[mipmapNdx].bufferOffset);

		if (deMemCmp(outputData + bufferOffset, &referenceData[bufferOffset], mipLevelSizeInBytes) != 0)
			return tcu::TestStatus::fail("Failed");
	}

	return tcu::TestStatus::pass("Passed");
}

TestInstance* MipmapSparseResidencyCase::createInstance (Context& context) const
{
	return new MipmapSparseResidencyInstance(context, m_imageType, m_imageSize, m_format);
}

} // anonymous ns

tcu::TestCaseGroup* createMipmapSparseResidencyTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "mipmap_sparse_residency", "Mipmap Sparse Residency"));

	static const deUint32 sizeCountPerImageType = 3u;

	struct ImageParameters
	{
		ImageType	imageType;
		tcu::UVec3	imageSizes[sizeCountPerImageType];
	};

	static const ImageParameters imageParametersArray[] =
	{
		{ IMAGE_TYPE_2D,		 { tcu::UVec3(512u, 256u, 1u),  tcu::UVec3(1024u, 128u, 1u), tcu::UVec3(11u,  137u, 1u) } },
		{ IMAGE_TYPE_2D_ARRAY,	 { tcu::UVec3(512u, 256u, 6u),	tcu::UVec3(1024u, 128u, 8u), tcu::UVec3(11u,  137u, 3u) } },
		{ IMAGE_TYPE_CUBE,		 { tcu::UVec3(256u, 256u, 1u),	tcu::UVec3(128u,  128u, 1u), tcu::UVec3(137u, 137u, 1u) } },
		{ IMAGE_TYPE_CUBE_ARRAY, { tcu::UVec3(256u, 256u, 6u),	tcu::UVec3(128u,  128u, 8u), tcu::UVec3(137u, 137u, 3u) } },
		{ IMAGE_TYPE_3D,		 { tcu::UVec3(256u, 256u, 16u), tcu::UVec3(1024u, 128u, 8u), tcu::UVec3(11u,  137u, 3u) } }
	};

	static const tcu::TextureFormat formats[] =
	{
		tcu::TextureFormat(tcu::TextureFormat::R,	 tcu::TextureFormat::SIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::R,	 tcu::TextureFormat::SIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::R,	 tcu::TextureFormat::SIGNED_INT8),
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT16),
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT8)
	};

	for (deInt32 imageTypeNdx = 0; imageTypeNdx < DE_LENGTH_OF_ARRAY(imageParametersArray); ++imageTypeNdx)
	{
		const ImageType					imageType = imageParametersArray[imageTypeNdx].imageType;
		de::MovePtr<tcu::TestCaseGroup> imageTypeGroup(new tcu::TestCaseGroup(testCtx, getImageTypeName(imageType).c_str(), ""));

		for (deInt32 formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
		{
			const tcu::TextureFormat&		format = formats[formatNdx];
			de::MovePtr<tcu::TestCaseGroup> formatGroup(new tcu::TestCaseGroup(testCtx, getShaderImageFormatQualifier(format).c_str(), ""));

			for (deInt32 imageSizeNdx = 0; imageSizeNdx < DE_LENGTH_OF_ARRAY(imageParametersArray[imageTypeNdx].imageSizes); ++imageSizeNdx)
			{
				const tcu::UVec3 imageSize = imageParametersArray[imageTypeNdx].imageSizes[imageSizeNdx];

				std::ostringstream stream;
				stream << imageSize.x() << "_" << imageSize.y() << "_" << imageSize.z();

				formatGroup->addChild(new MipmapSparseResidencyCase(testCtx, stream.str(), "", imageType, imageSize, format));
			}
			imageTypeGroup->addChild(formatGroup.release());
		}
		testGroup->addChild(imageTypeGroup.release());
	}

	return testGroup.release();
}

} // sparse
} // vkt
