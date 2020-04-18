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
 *//*
 * \file  vktSparseResourcesShaderIntrinsicsBase.cpp
 * \brief Sparse Resources Shader Intrinsics Base Classes
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesShaderIntrinsicsBase.hpp"

using namespace vk;

namespace vkt
{
namespace sparse
{

tcu::UVec3 alignedDivide (const VkExtent3D& extent, const VkExtent3D& divisor)
{
	tcu::UVec3 result;

	result.x() = extent.width  / divisor.width  + ((extent.width  % divisor.width)  ? 1u : 0u);
	result.y() = extent.height / divisor.height + ((extent.height % divisor.height) ? 1u : 0u);
	result.z() = extent.depth  / divisor.depth  + ((extent.depth  % divisor.depth)  ? 1u : 0u);

	return result;
}

std::string getOpTypeImageComponent (const tcu::TextureFormat& format)
{
	switch (tcu::getTextureChannelClass(format.type))
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			return "OpTypeInt 32 0";
		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			return "OpTypeInt 32 1";
		default:
			DE_ASSERT(0);
			return "";
	}
}

std::string getImageComponentTypeName (const tcu::TextureFormat& format)
{
	switch (tcu::getTextureChannelClass(format.type))
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			return "%type_uint";
		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			return "%type_int";
		default:
			DE_ASSERT(0);
			return "";
	}
}

std::string getImageComponentVec4TypeName (const tcu::TextureFormat& format)
{
	switch (tcu::getTextureChannelClass(format.type))
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			return "%type_uvec4";
		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			return "%type_ivec4";
		default:
			DE_ASSERT(0);
			return "";
	}
}

std::string getOpTypeImageSparse (const ImageType			imageType,
								  const tcu::TextureFormat&	format,
								  const std::string&		componentType,
								  const bool				requiresSampler)
{
	std::ostringstream	src;

	src << "OpTypeImage " << componentType << " ";

	switch (imageType)
	{
		case IMAGE_TYPE_1D :
			src << "1D 0 0 0 ";
		break;
		case IMAGE_TYPE_1D_ARRAY :
			src << "1D 0 1 0 ";
		break;
		case IMAGE_TYPE_2D :
			src << "2D 0 0 0 ";
		break;
		case IMAGE_TYPE_2D_ARRAY :
			src << "2D 0 1 0 ";
		break;
		case IMAGE_TYPE_3D :
			src << "3D 0 0 0 ";
		break;
		case IMAGE_TYPE_CUBE :
			src << "Cube 0 0 0 ";
		break;
		case IMAGE_TYPE_CUBE_ARRAY :
			src << "Cube 0 1 0 ";
		break;
		default :
			DE_ASSERT(0);
		break;
	};

	if (requiresSampler)
		src << "1 ";
	else
		src << "2 ";

	switch (format.order)
	{
		case tcu::TextureFormat::R:
			src << "R";
		break;
		case tcu::TextureFormat::RG:
			src << "Rg";
			break;
		case tcu::TextureFormat::RGB:
			src << "Rgb";
			break;
		case tcu::TextureFormat::RGBA:
			src << "Rgba";
		break;
		default:
			DE_ASSERT(0);
		break;
	}

	switch (format.type)
	{
		case tcu::TextureFormat::SIGNED_INT8:
			src << "8i";
		break;
		case tcu::TextureFormat::SIGNED_INT16:
			src << "16i";
		break;
		case tcu::TextureFormat::SIGNED_INT32:
			src << "32i";
		break;
		case tcu::TextureFormat::UNSIGNED_INT8:
			src << "8ui";
		break;
		case tcu::TextureFormat::UNSIGNED_INT16:
			src << "16ui";
		break;
		case tcu::TextureFormat::UNSIGNED_INT32:
			src << "32ui";
		break;
		default:
			DE_ASSERT(0);
		break;
	};

	return src.str();
}

std::string getOpTypeImageResidency (const ImageType imageType)
{
	std::ostringstream	src;

	src << "OpTypeImage %type_uint ";

	switch (imageType)
	{
		case IMAGE_TYPE_1D :
			src << "1D 0 0 0 2 R32ui";
		break;
		case IMAGE_TYPE_1D_ARRAY :
			src << "1D 0 1 0 2 R32ui";
		break;
		case IMAGE_TYPE_2D :
			src << "2D 0 0 0 2 R32ui";
		break;
		case IMAGE_TYPE_2D_ARRAY :
			src << "2D 0 1 0 2 R32ui";
		break;
		case IMAGE_TYPE_3D :
			src << "3D 0 0 0 2 R32ui";
		break;
		case IMAGE_TYPE_CUBE :
			src << "Cube 0 0 0 2 R32ui";
		break;
		case IMAGE_TYPE_CUBE_ARRAY :
			src << "Cube 0 1 0 2 R32ui";
		break;
		default :
			DE_ASSERT(0);
		break;
	};

	return src.str();
}

tcu::TestStatus SparseShaderIntrinsicsInstanceBase::iterate (void)
{
	const InstanceInterface&			instance				= m_context.getInstanceInterface();
	const VkPhysicalDevice				physicalDevice			= m_context.getPhysicalDevice();
	VkImageCreateInfo					imageSparseInfo;
	VkImageCreateInfo					imageTexelsInfo;
	VkImageCreateInfo					imageResidencyInfo;
	VkSparseImageMemoryRequirements		aspectRequirements;
	std::vector <deUint32>				residencyReferenceData;
	std::vector<DeviceMemorySp>			deviceMemUniquePtrVec;

	// Check if image size does not exceed device limits
	if (!isImageSizeSupported(instance, physicalDevice, m_imageType, m_imageSize))
		TCU_THROW(NotSupportedError, "Image size not supported for device");

	// Check if device supports sparse operations for image type
	if (!checkSparseSupportForImageType(instance, physicalDevice, m_imageType))
		TCU_THROW(NotSupportedError, "Sparse residency for image type is not supported");

	if (!getPhysicalDeviceFeatures(instance, physicalDevice).shaderResourceResidency)
		TCU_THROW(NotSupportedError, "Sparse resource residency information not supported in shader code.");

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
	imageSparseInfo.usage					= VK_IMAGE_USAGE_TRANSFER_DST_BIT | imageSparseUsageFlags();
	imageSparseInfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imageSparseInfo.queueFamilyIndexCount	= 0u;
	imageSparseInfo.pQueueFamilyIndices		= DE_NULL;

	if (m_imageType == IMAGE_TYPE_CUBE || m_imageType == IMAGE_TYPE_CUBE_ARRAY)
	{
		imageSparseInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	{
		// Assign maximum allowed mipmap levels to image
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
		// Create logical device supporting both sparse and compute/graphics queues
		QueueRequirementsVec queueRequirements;
		queueRequirements.push_back(QueueRequirements(VK_QUEUE_SPARSE_BINDING_BIT, 1u));
		queueRequirements.push_back(QueueRequirements(getQueueFlags(), 1u));

		createDeviceSupportingQueues(queueRequirements);
	}

	const DeviceInterface&	deviceInterface	= getDeviceInterface();

	// Create queues supporting sparse binding operations and compute/graphics operations
	const Queue&			sparseQueue		= getQueue(VK_QUEUE_SPARSE_BINDING_BIT, 0);
	const Queue&			extractQueue	= getQueue(getQueueFlags(), 0);

	// Create sparse image
	const Unique<VkImage> imageSparse(createImage(deviceInterface, getDevice(), &imageSparseInfo));

	// Create sparse image memory bind semaphore
	const Unique<VkSemaphore> memoryBindSemaphore(createSemaphore(deviceInterface, getDevice()));

	const deUint32			  imageSparseSizeInBytes		= getImageSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, imageSparseInfo.mipLevels, BUFFER_IMAGE_COPY_OFFSET_GRANULARITY);
	const deUint32			  imageSizeInPixels				= getImageSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, imageSparseInfo.mipLevels) / tcu::getPixelSize(m_format);

	residencyReferenceData.assign(imageSizeInPixels, MEMORY_BLOCK_NOT_BOUND_VALUE);

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

		aspectRequirements = sparseMemoryRequirements[colorAspectIndex];

		DE_ASSERT((aspectRequirements.imageMipTailSize % imageMemoryRequirements.alignment) == 0);

		const VkImageAspectFlags aspectMask			= aspectRequirements.formatProperties.aspectMask;
		const VkExtent3D		 imageGranularity	= aspectRequirements.formatProperties.imageGranularity;
		const deUint32			 memoryType			= findMatchingMemoryType(instance, physicalDevice, imageMemoryRequirements, MemoryRequirement::Any);

		if (memoryType == NO_MATCH_FOUND)
			return tcu::TestStatus::fail("No matching memory type found");

		deUint32 pixelOffset = 0u;

		std::vector<VkSparseImageMemoryBind>  imageResidencyMemoryBinds;
		std::vector<VkSparseMemoryBind>		  imageMipTailBinds;

		// Bind memory for each mipmap level
		for (deUint32 mipLevelNdx = 0; mipLevelNdx < aspectRequirements.imageMipTailFirstLod; ++mipLevelNdx)
		{
			const deUint32 mipLevelSizeInPixels = getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, mipLevelNdx) / tcu::getPixelSize(m_format);

			if (mipLevelNdx % MEMORY_BLOCK_TYPE_COUNT == MEMORY_BLOCK_NOT_BOUND)
			{
				pixelOffset += mipLevelSizeInPixels;
				continue;
			}

			for (deUint32 pixelNdx = 0u; pixelNdx < mipLevelSizeInPixels; ++pixelNdx)
			{
				residencyReferenceData[pixelOffset + pixelNdx] = MEMORY_BLOCK_BOUND_VALUE;
			}

			pixelOffset += mipLevelSizeInPixels;

			for (deUint32 layerNdx = 0; layerNdx < imageSparseInfo.arrayLayers; ++layerNdx)
			{
				const VkExtent3D		 mipExtent			= mipLevelExtents(imageSparseInfo.extent, mipLevelNdx);
				const tcu::UVec3		 sparseBlocks		= alignedDivide(mipExtent, imageGranularity);
				const deUint32			 numSparseBlocks	= sparseBlocks.x() * sparseBlocks.y() * sparseBlocks.z();
				const VkImageSubresource subresource		= { aspectMask, mipLevelNdx, layerNdx };

				const VkSparseImageMemoryBind imageMemoryBind = makeSparseImageMemoryBind(deviceInterface, getDevice(),
					imageMemoryRequirements.alignment * numSparseBlocks, memoryType, subresource, makeOffset3D(0u, 0u, 0u), mipExtent);

				deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

				imageResidencyMemoryBinds.push_back(imageMemoryBind);
			}
		}

		if (aspectRequirements.imageMipTailFirstLod < imageSparseInfo.mipLevels)
		{
			if (aspectRequirements.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT)
			{
				const VkSparseMemoryBind imageMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
					aspectRequirements.imageMipTailSize, memoryType, aspectRequirements.imageMipTailOffset);

				deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

				imageMipTailBinds.push_back(imageMipTailMemoryBind);
			}
			else
			{
				for (deUint32 layerNdx = 0; layerNdx < imageSparseInfo.arrayLayers; ++layerNdx)
				{
					const VkSparseMemoryBind imageMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
						aspectRequirements.imageMipTailSize, memoryType, aspectRequirements.imageMipTailOffset + layerNdx * aspectRequirements.imageMipTailStride);

					deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

					imageMipTailBinds.push_back(imageMipTailMemoryBind);
				}
			}

			for (deUint32 pixelNdx = pixelOffset; pixelNdx < residencyReferenceData.size(); ++pixelNdx)
			{
				residencyReferenceData[pixelNdx] = MEMORY_BLOCK_BOUND_VALUE;
			}
		}

		// Metadata
		if (metadataAspectIndex != NO_MATCH_FOUND)
		{
			const VkSparseImageMemoryRequirements metadataAspectRequirements = sparseMemoryRequirements[metadataAspectIndex];

			const deUint32 metadataBindCount = (metadataAspectRequirements.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT ? 1u : imageSparseInfo.arrayLayers);
			for (deUint32 bindNdx = 0u; bindNdx < metadataBindCount; ++bindNdx)
			{
				const VkSparseMemoryBind imageMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
					metadataAspectRequirements.imageMipTailSize, memoryType,
					metadataAspectRequirements.imageMipTailOffset + bindNdx * metadataAspectRequirements.imageMipTailStride,
					VK_SPARSE_MEMORY_BIND_METADATA_BIT);

				deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

				imageMipTailBinds.push_back(imageMipTailMemoryBind);
			}
		}

		VkBindSparseInfo bindSparseInfo =
		{
			VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,	//VkStructureType							sType;
			DE_NULL,							//const void*								pNext;
			0u,									//deUint32									waitSemaphoreCount;
			DE_NULL,							//const VkSemaphore*						pWaitSemaphores;
			0u,									//deUint32									bufferBindCount;
			DE_NULL,							//const VkSparseBufferMemoryBindInfo*		pBufferBinds;
			0u,									//deUint32									imageOpaqueBindCount;
			DE_NULL,							//const VkSparseImageOpaqueMemoryBindInfo*	pImageOpaqueBinds;
			0u,									//deUint32									imageBindCount;
			DE_NULL,							//const VkSparseImageMemoryBindInfo*		pImageBinds;
			1u,									//deUint32									signalSemaphoreCount;
			&memoryBindSemaphore.get()			//const VkSemaphore*						pSignalSemaphores;
		};

		VkSparseImageMemoryBindInfo		  imageResidencyBindInfo;
		VkSparseImageOpaqueMemoryBindInfo imageMipTailBindInfo;

		if (imageResidencyMemoryBinds.size() > 0)
		{
			imageResidencyBindInfo.image		= *imageSparse;
			imageResidencyBindInfo.bindCount	= static_cast<deUint32>(imageResidencyMemoryBinds.size());
			imageResidencyBindInfo.pBinds		= &imageResidencyMemoryBinds[0];

			bindSparseInfo.imageBindCount		= 1u;
			bindSparseInfo.pImageBinds			= &imageResidencyBindInfo;
		}

		if (imageMipTailBinds.size() > 0)
		{
			imageMipTailBindInfo.image			= *imageSparse;
			imageMipTailBindInfo.bindCount		= static_cast<deUint32>(imageMipTailBinds.size());
			imageMipTailBindInfo.pBinds			= &imageMipTailBinds[0];

			bindSparseInfo.imageOpaqueBindCount = 1u;
			bindSparseInfo.pImageOpaqueBinds	= &imageMipTailBindInfo;
		}

		// Submit sparse bind commands for execution
		VK_CHECK(deviceInterface.queueBindSparse(sparseQueue.queueHandle, 1u, &bindSparseInfo, DE_NULL));
	}

	// Create image to store texels copied from sparse image
	imageTexelsInfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageTexelsInfo.pNext					= DE_NULL;
	imageTexelsInfo.flags					= 0u;
	imageTexelsInfo.imageType				= imageSparseInfo.imageType;
	imageTexelsInfo.format					= imageSparseInfo.format;
	imageTexelsInfo.extent					= imageSparseInfo.extent;
	imageTexelsInfo.arrayLayers				= imageSparseInfo.arrayLayers;
	imageTexelsInfo.mipLevels				= imageSparseInfo.mipLevels;
	imageTexelsInfo.samples					= imageSparseInfo.samples;
	imageTexelsInfo.tiling					= VK_IMAGE_TILING_OPTIMAL;
	imageTexelsInfo.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imageTexelsInfo.usage					= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | imageOutputUsageFlags();
	imageTexelsInfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imageTexelsInfo.queueFamilyIndexCount	= 0u;
	imageTexelsInfo.pQueueFamilyIndices		= DE_NULL;

	if (m_imageType == IMAGE_TYPE_CUBE || m_imageType == IMAGE_TYPE_CUBE_ARRAY)
	{
		imageTexelsInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	const Unique<VkImage>			imageTexels			(createImage(deviceInterface, getDevice(), &imageTexelsInfo));
	const de::UniquePtr<Allocation>	imageTexelsAlloc	(bindImage(deviceInterface, getDevice(), getAllocator(), *imageTexels, MemoryRequirement::Any));

	// Create image to store residency info copied from sparse image
	imageResidencyInfo			= imageTexelsInfo;
	imageResidencyInfo.format	= mapTextureFormat(m_residencyFormat);

	const Unique<VkImage>			imageResidency		(createImage(deviceInterface, getDevice(), &imageResidencyInfo));
	const de::UniquePtr<Allocation>	imageResidencyAlloc	(bindImage(deviceInterface, getDevice(), getAllocator(), *imageResidency, MemoryRequirement::Any));

	// Create command buffer for compute and transfer oparations
	const Unique<VkCommandPool>	  commandPool(makeCommandPool(deviceInterface, getDevice(), extractQueue.queueFamilyIndex));
	const Unique<VkCommandBuffer> commandBuffer(allocateCommandBuffer(deviceInterface, getDevice(), *commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	std::vector <VkBufferImageCopy> bufferImageSparseCopy(imageSparseInfo.mipLevels);

	{
		deUint32 bufferOffset = 0u;
		for (deUint32 mipLevelNdx = 0u; mipLevelNdx < imageSparseInfo.mipLevels; ++mipLevelNdx)
		{
			bufferImageSparseCopy[mipLevelNdx] = makeBufferImageCopy(mipLevelExtents(imageSparseInfo.extent, mipLevelNdx), imageSparseInfo.arrayLayers, mipLevelNdx, static_cast<VkDeviceSize>(bufferOffset));
			bufferOffset += getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, mipLevelNdx, BUFFER_IMAGE_COPY_OFFSET_GRANULARITY);
		}
	}

	// Start recording commands
	beginCommandBuffer(deviceInterface, *commandBuffer);

	// Create input buffer
	const VkBufferCreateInfo		inputBufferCreateInfo	= makeBufferCreateInfo(imageSparseSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	const Unique<VkBuffer>			inputBuffer				(createBuffer(deviceInterface, getDevice(), &inputBufferCreateInfo));
	const de::UniquePtr<Allocation>	inputBufferAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *inputBuffer, MemoryRequirement::HostVisible));

	// Fill input buffer with reference data
	std::vector<deUint8> referenceData(imageSparseSizeInBytes);

	for (deUint32 mipLevelNdx = 0u; mipLevelNdx < imageSparseInfo.mipLevels; ++mipLevelNdx)
	{
		const deUint32 mipLevelSizeinBytes	= getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, mipLevelNdx);
		const deUint32 bufferOffset			= static_cast<deUint32>(bufferImageSparseCopy[mipLevelNdx].bufferOffset);

		for (deUint32 byteNdx = 0u; byteNdx < mipLevelSizeinBytes; ++byteNdx)
		{
			referenceData[bufferOffset + byteNdx] = (deUint8)(mipLevelNdx + byteNdx);
		}
	}

	deMemcpy(inputBufferAlloc->getHostPtr(), &referenceData[0], imageSparseSizeInBytes);
	flushMappedMemoryRange(deviceInterface, getDevice(), inputBufferAlloc->getMemory(), inputBufferAlloc->getOffset(), imageSparseSizeInBytes);

	{
		// Prepare input buffer for data transfer operation
		const VkBufferMemoryBarrier inputBufferBarrier = makeBufferMemoryBarrier
		(
			VK_ACCESS_HOST_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			*inputBuffer,
			0u,
			imageSparseSizeInBytes
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 1u, &inputBufferBarrier, 0u, DE_NULL);
	}

	const VkImageSubresourceRange fullImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, imageSparseInfo.mipLevels, 0u, imageSparseInfo.arrayLayers);

	{
		// Prepare sparse image for data transfer operation
		const VkImageMemoryBarrier imageSparseTransferDstBarrier = makeImageMemoryBarrier
		(
			0u,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			sparseQueue.queueFamilyIndex != extractQueue.queueFamilyIndex ? sparseQueue.queueFamilyIndex  : VK_QUEUE_FAMILY_IGNORED,
			sparseQueue.queueFamilyIndex != extractQueue.queueFamilyIndex ? extractQueue.queueFamilyIndex : VK_QUEUE_FAMILY_IGNORED,
			*imageSparse,
			fullImageSubresourceRange
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imageSparseTransferDstBarrier);
	}

	// Copy reference data from input buffer to sparse image
	deviceInterface.cmdCopyBufferToImage(*commandBuffer, *inputBuffer, *imageSparse, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<deUint32>(bufferImageSparseCopy.size()), &bufferImageSparseCopy[0]);

	recordCommands(*commandBuffer, imageSparseInfo, *imageSparse, *imageTexels, *imageResidency);

	const VkBufferCreateInfo		bufferTexelsCreateInfo	= makeBufferCreateInfo(imageSparseSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	const Unique<VkBuffer>			bufferTexels			(createBuffer(deviceInterface, getDevice(), &bufferTexelsCreateInfo));
	const de::UniquePtr<Allocation>	bufferTexelsAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *bufferTexels, MemoryRequirement::HostVisible));

	// Copy data from texels image to buffer
	deviceInterface.cmdCopyImageToBuffer(*commandBuffer, *imageTexels, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *bufferTexels, static_cast<deUint32>(bufferImageSparseCopy.size()), &bufferImageSparseCopy[0]);

	const deUint32				imageResidencySizeInBytes = getImageSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_residencyFormat, imageSparseInfo.mipLevels, BUFFER_IMAGE_COPY_OFFSET_GRANULARITY);

	const VkBufferCreateInfo		bufferResidencyCreateInfo	= makeBufferCreateInfo(imageResidencySizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	const Unique<VkBuffer>			bufferResidency				(createBuffer(deviceInterface, getDevice(), &bufferResidencyCreateInfo));
	const de::UniquePtr<Allocation>	bufferResidencyAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *bufferResidency, MemoryRequirement::HostVisible));

	// Copy data from residency image to buffer
	std::vector <VkBufferImageCopy> bufferImageResidencyCopy(imageSparseInfo.mipLevels);

	{
		deUint32 bufferOffset = 0u;
		for (deUint32 mipLevelNdx = 0u; mipLevelNdx < imageSparseInfo.mipLevels; ++mipLevelNdx)
		{
			bufferImageResidencyCopy[mipLevelNdx] = makeBufferImageCopy(mipLevelExtents(imageSparseInfo.extent, mipLevelNdx), imageSparseInfo.arrayLayers, mipLevelNdx, static_cast<VkDeviceSize>(bufferOffset));
			bufferOffset += getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_residencyFormat, mipLevelNdx, BUFFER_IMAGE_COPY_OFFSET_GRANULARITY);
		}
	}

	deviceInterface.cmdCopyImageToBuffer(*commandBuffer, *imageResidency, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *bufferResidency, static_cast<deUint32>(bufferImageResidencyCopy.size()), &bufferImageResidencyCopy[0]);

	{
		VkBufferMemoryBarrier bufferOutputHostReadBarriers[2];

		bufferOutputHostReadBarriers[0] = makeBufferMemoryBarrier
		(
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_HOST_READ_BIT,
			*bufferTexels,
			0u,
			imageSparseSizeInBytes
		);

		bufferOutputHostReadBarriers[1] = makeBufferMemoryBarrier
		(
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_HOST_READ_BIT,
			*bufferResidency,
			0u,
			imageResidencySizeInBytes
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL, 2u, bufferOutputHostReadBarriers, 0u, DE_NULL);
	}

	// End recording commands
	endCommandBuffer(deviceInterface, *commandBuffer);

	const VkPipelineStageFlags stageBits[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	// Submit commands for execution and wait for completion
	submitCommandsAndWait(deviceInterface, getDevice(), extractQueue.queueHandle, *commandBuffer, 1u, &memoryBindSemaphore.get(), stageBits);

	// Wait for sparse queue to become idle
	deviceInterface.queueWaitIdle(sparseQueue.queueHandle);

	// Retrieve data from residency buffer to host memory
	invalidateMappedMemoryRange(deviceInterface, getDevice(), bufferResidencyAlloc->getMemory(), bufferResidencyAlloc->getOffset(), imageResidencySizeInBytes);

	const deUint32* bufferResidencyData = static_cast<const deUint32*>(bufferResidencyAlloc->getHostPtr());

	deUint32 pixelOffsetNotAligned = 0u;
	for (deUint32 mipmapNdx = 0; mipmapNdx < imageSparseInfo.mipLevels; ++mipmapNdx)
	{
		const deUint32 mipLevelSizeInBytes	= getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_residencyFormat, mipmapNdx);
		const deUint32 pixelOffsetAligned	= static_cast<deUint32>(bufferImageResidencyCopy[mipmapNdx].bufferOffset) / tcu::getPixelSize(m_residencyFormat);

		if (deMemCmp(&bufferResidencyData[pixelOffsetAligned], &residencyReferenceData[pixelOffsetNotAligned], mipLevelSizeInBytes) != 0)
			return tcu::TestStatus::fail("Failed");

		pixelOffsetNotAligned += mipLevelSizeInBytes / tcu::getPixelSize(m_residencyFormat);
	}

	// Retrieve data from texels buffer to host memory
	invalidateMappedMemoryRange(deviceInterface, getDevice(), bufferTexelsAlloc->getMemory(), bufferTexelsAlloc->getOffset(), imageSparseSizeInBytes);

	const deUint8* bufferTexelsData = static_cast<const deUint8*>(bufferTexelsAlloc->getHostPtr());

	for (deUint32 mipmapNdx = 0; mipmapNdx < imageSparseInfo.mipLevels; ++mipmapNdx)
	{
		const deUint32 mipLevelSizeInBytes	= getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, mipmapNdx);
		const deUint32 bufferOffset			= static_cast<deUint32>(bufferImageSparseCopy[mipmapNdx].bufferOffset);

		if (mipmapNdx < aspectRequirements.imageMipTailFirstLod)
		{
			if (mipmapNdx % MEMORY_BLOCK_TYPE_COUNT == MEMORY_BLOCK_BOUND)
			{
				if (deMemCmp(&bufferTexelsData[bufferOffset], &referenceData[bufferOffset], mipLevelSizeInBytes) != 0)
					return tcu::TestStatus::fail("Failed");
			}
			else if (getPhysicalDeviceProperties(instance, physicalDevice).sparseProperties.residencyNonResidentStrict)
			{
				std::vector<deUint8> zeroData;
				zeroData.assign(mipLevelSizeInBytes, 0u);

				if (deMemCmp(&bufferTexelsData[bufferOffset], &zeroData[0], mipLevelSizeInBytes) != 0)
					return tcu::TestStatus::fail("Failed");
			}
		}
		else
		{
			if (deMemCmp(&bufferTexelsData[bufferOffset], &referenceData[bufferOffset], mipLevelSizeInBytes) != 0)
				return tcu::TestStatus::fail("Failed");
		}
	}

	return tcu::TestStatus::pass("Passed");
}

} // sparse
} // vkt
