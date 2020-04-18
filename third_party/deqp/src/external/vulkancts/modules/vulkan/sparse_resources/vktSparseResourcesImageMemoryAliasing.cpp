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
 * \file  vktSparseResourcesImageMemoryAliasing.cpp
 * \brief Sparse image memory aliasing tests
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesImageMemoryAliasing.hpp"
#include "vktSparseResourcesTestsUtil.hpp"
#include "vktSparseResourcesBase.hpp"
#include "vktTestCaseUtil.hpp"

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkRefUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"
#include "tcuTexture.hpp"

#include <deMath.h>
#include <string>
#include <vector>

using namespace vk;

namespace vkt
{
namespace sparse
{
namespace
{

enum ShaderParameters
{
	MODULO_DIVISOR = 128
};

const std::string getCoordStr  (const ImageType		imageType,
								const std::string&	x,
								const std::string&	y,
								const std::string&	z)
{
	switch (imageType)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_BUFFER:
			return x;

		case IMAGE_TYPE_1D_ARRAY:
		case IMAGE_TYPE_2D:
			return "ivec2(" + x + "," + y + ")";

		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_3D:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
			return "ivec3(" + x + "," + y + "," + z + ")";

		default:
			DE_ASSERT(false);
			return "";
	}
}

tcu::UVec3 alignedDivide (const VkExtent3D& extent, const VkExtent3D& divisor)
{
	tcu::UVec3 result;

	result.x() = extent.width  / divisor.width  + ((extent.width  % divisor.width)  ? 1u : 0u);
	result.y() = extent.height / divisor.height + ((extent.height % divisor.height) ? 1u : 0u);
	result.z() = extent.depth  / divisor.depth  + ((extent.depth  % divisor.depth)  ? 1u : 0u);

	return result;
}

class ImageSparseMemoryAliasingCase : public TestCase
{
public:
					ImageSparseMemoryAliasingCase	(tcu::TestContext&			testCtx,
													 const std::string&			name,
													 const std::string&			description,
													 const ImageType			imageType,
													 const tcu::UVec3&			imageSize,
													 const tcu::TextureFormat&	format,
													 const glu::GLSLVersion		glslVersion);

	void			initPrograms					(SourceCollections&			sourceCollections) const;
	TestInstance*	createInstance					(Context&					context) const;


private:
	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const tcu::TextureFormat	m_format;
	const glu::GLSLVersion		m_glslVersion;
};

ImageSparseMemoryAliasingCase::ImageSparseMemoryAliasingCase (tcu::TestContext&			testCtx,
															  const std::string&		name,
															  const std::string&		description,
															  const ImageType			imageType,
															  const tcu::UVec3&			imageSize,
															  const tcu::TextureFormat&	format,
															  const glu::GLSLVersion	glslVersion)
	: TestCase				(testCtx, name, description)
	, m_imageType			(imageType)
	, m_imageSize			(imageSize)
	, m_format				(format)
	, m_glslVersion			(glslVersion)
{
}

class ImageSparseMemoryAliasingInstance : public SparseResourcesBaseInstance
{
public:
					ImageSparseMemoryAliasingInstance	(Context&								context,
														 const ImageType						imageType,
														 const tcu::UVec3&						imageSize,
														 const tcu::TextureFormat&				format);

	tcu::TestStatus	iterate								(void);

private:
	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const tcu::TextureFormat	m_format;
};

ImageSparseMemoryAliasingInstance::ImageSparseMemoryAliasingInstance (Context&					context,
																	  const ImageType			imageType,
																	  const tcu::UVec3&			imageSize,
																	  const tcu::TextureFormat&	format)
	: SparseResourcesBaseInstance	(context)
	, m_imageType					(imageType)
	, m_imageSize					(imageSize)
	, m_format						(format)
{
}

tcu::TestStatus ImageSparseMemoryAliasingInstance::iterate (void)
{
	const InstanceInterface&			instance				= m_context.getInstanceInterface();
	const VkPhysicalDevice				physicalDevice			= m_context.getPhysicalDevice();
	const tcu::UVec3					maxWorkGroupSize		= tcu::UVec3(128u, 128u, 64u);
	const tcu::UVec3					maxWorkGroupCount		= tcu::UVec3(65535u, 65535u, 65535u);
	const deUint32						maxWorkGroupInvocations	= 128u;
	VkImageCreateInfo					imageSparseInfo;
	VkSparseImageMemoryRequirements		aspectRequirements;
	std::vector<DeviceMemorySp>			deviceMemUniquePtrVec;

	// Check if image size does not exceed device limits
	if (!isImageSizeSupported(instance, physicalDevice, m_imageType, m_imageSize))
		TCU_THROW(NotSupportedError, "Image size not supported for device");

	// Check if sparse memory aliasing is supported
	if (!getPhysicalDeviceFeatures(instance, physicalDevice).sparseResidencyAliased)
		TCU_THROW(NotSupportedError, "Sparse memory aliasing not supported");

	// Check if device supports sparse operations for image type
	if (!checkSparseSupportForImageType(instance, physicalDevice, m_imageType))
		TCU_THROW(NotSupportedError, "Sparse residency for image type is not supported");

	imageSparseInfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageSparseInfo.pNext					= DE_NULL;
	imageSparseInfo.flags					= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT |
											  VK_IMAGE_CREATE_SPARSE_ALIASED_BIT   |
											  VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
	imageSparseInfo.imageType				= mapImageType(m_imageType);
	imageSparseInfo.format					= mapTextureFormat(m_format);
	imageSparseInfo.extent					= makeExtent3D(getLayerSize(m_imageType, m_imageSize));
	imageSparseInfo.arrayLayers				= getNumLayers(m_imageType, m_imageSize);
	imageSparseInfo.samples					= VK_SAMPLE_COUNT_1_BIT;
	imageSparseInfo.tiling					= VK_IMAGE_TILING_OPTIMAL;
	imageSparseInfo.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imageSparseInfo.usage					= VK_IMAGE_USAGE_TRANSFER_DST_BIT |
											  VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
											  VK_IMAGE_USAGE_STORAGE_BIT;
	imageSparseInfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	imageSparseInfo.queueFamilyIndexCount	= 0u;
	imageSparseInfo.pQueueFamilyIndices		= DE_NULL;

	if (m_imageType == IMAGE_TYPE_CUBE || m_imageType == IMAGE_TYPE_CUBE_ARRAY)
		imageSparseInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

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
		// Create logical device supporting both sparse and compute queues
		QueueRequirementsVec queueRequirements;
		queueRequirements.push_back(QueueRequirements(VK_QUEUE_SPARSE_BINDING_BIT, 1u));
		queueRequirements.push_back(QueueRequirements(VK_QUEUE_COMPUTE_BIT, 1u));

		createDeviceSupportingQueues(queueRequirements);
	}

	const DeviceInterface&	deviceInterface	= getDeviceInterface();
	const Queue&			sparseQueue		= getQueue(VK_QUEUE_SPARSE_BINDING_BIT, 0);
	const Queue&			computeQueue	= getQueue(VK_QUEUE_COMPUTE_BIT, 0);

	// Create sparse image
	const Unique<VkImage> imageRead(createImage(deviceInterface, getDevice(), &imageSparseInfo));
	const Unique<VkImage> imageWrite(createImage(deviceInterface, getDevice(), &imageSparseInfo));

	// Create semaphores to synchronize sparse binding operations with other operations on the sparse images
	const Unique<VkSemaphore> memoryBindSemaphoreTransfer(createSemaphore(deviceInterface, getDevice()));
	const Unique<VkSemaphore> memoryBindSemaphoreCompute(createSemaphore(deviceInterface, getDevice()));

	const VkSemaphore imageMemoryBindSemaphores[] = { memoryBindSemaphoreTransfer.get(), memoryBindSemaphoreCompute.get() };

	{
		std::vector<VkSparseImageMemoryBind> imageResidencyMemoryBinds;
		std::vector<VkSparseMemoryBind>		 imageReadMipTailBinds;
		std::vector<VkSparseMemoryBind>		 imageWriteMipTailBinds;

		// Get sparse image general memory requirements
		const VkMemoryRequirements imageMemoryRequirements = getImageMemoryRequirements(deviceInterface, getDevice(), *imageRead);

		// Check if required image memory size does not exceed device limits
		if (imageMemoryRequirements.size > getPhysicalDeviceProperties(instance, physicalDevice).limits.sparseAddressSpaceSize)
			TCU_THROW(NotSupportedError, "Required memory size for sparse resource exceeds device limits");

		DE_ASSERT((imageMemoryRequirements.size % imageMemoryRequirements.alignment) == 0);

		// Get sparse image sparse memory requirements
		const std::vector<VkSparseImageMemoryRequirements> sparseMemoryRequirements = getImageSparseMemoryRequirements(deviceInterface, getDevice(), *imageRead);

		DE_ASSERT(sparseMemoryRequirements.size() != 0);

		const deUint32 colorAspectIndex = getSparseAspectRequirementsIndex(sparseMemoryRequirements, VK_IMAGE_ASPECT_COLOR_BIT);

		if (colorAspectIndex == NO_MATCH_FOUND)
			TCU_THROW(NotSupportedError, "Not supported image aspect - the test supports currently only VK_IMAGE_ASPECT_COLOR_BIT");

		aspectRequirements = sparseMemoryRequirements[colorAspectIndex];

		const VkImageAspectFlags	aspectMask			= aspectRequirements.formatProperties.aspectMask;
		const VkExtent3D			imageGranularity	= aspectRequirements.formatProperties.imageGranularity;

		DE_ASSERT((aspectRequirements.imageMipTailSize % imageMemoryRequirements.alignment) == 0);

		const deUint32 memoryType = findMatchingMemoryType(instance, physicalDevice, imageMemoryRequirements, MemoryRequirement::Any);

		if (memoryType == NO_MATCH_FOUND)
			return tcu::TestStatus::fail("No matching memory type found");

		// Bind memory for each layer
		for (deUint32 layerNdx = 0; layerNdx < imageSparseInfo.arrayLayers; ++layerNdx)
		{
			for (deUint32 mipLevelNdx = 0; mipLevelNdx < aspectRequirements.imageMipTailFirstLod; ++mipLevelNdx)
			{
				const VkExtent3D			mipExtent		= mipLevelExtents(imageSparseInfo.extent, mipLevelNdx);
				const tcu::UVec3			sparseBlocks	= alignedDivide(mipExtent, imageGranularity);
				const deUint32				numSparseBlocks = sparseBlocks.x() * sparseBlocks.y() * sparseBlocks.z();
				const VkImageSubresource	subresource		= { aspectMask, mipLevelNdx, layerNdx };

				const VkSparseImageMemoryBind imageMemoryBind = makeSparseImageMemoryBind(deviceInterface, getDevice(),
					imageMemoryRequirements.alignment * numSparseBlocks, memoryType, subresource, makeOffset3D(0u, 0u, 0u), mipExtent);

				deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

				imageResidencyMemoryBinds.push_back(imageMemoryBind);
			}

			if (!(aspectRequirements.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) && aspectRequirements.imageMipTailFirstLod < imageSparseInfo.mipLevels)
			{
				const VkSparseMemoryBind imageReadMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
					aspectRequirements.imageMipTailSize, memoryType, aspectRequirements.imageMipTailOffset + layerNdx * aspectRequirements.imageMipTailStride);

				deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageReadMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

				imageReadMipTailBinds.push_back(imageReadMipTailMemoryBind);

				const VkSparseMemoryBind imageWriteMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
					aspectRequirements.imageMipTailSize, memoryType, aspectRequirements.imageMipTailOffset + layerNdx * aspectRequirements.imageMipTailStride);

				deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageWriteMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

				imageWriteMipTailBinds.push_back(imageWriteMipTailMemoryBind);
			}
		}

		if ((aspectRequirements.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) && aspectRequirements.imageMipTailFirstLod < imageSparseInfo.mipLevels)
		{
			const VkSparseMemoryBind imageReadMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
				aspectRequirements.imageMipTailSize, memoryType, aspectRequirements.imageMipTailOffset);

			deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageReadMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

			imageReadMipTailBinds.push_back(imageReadMipTailMemoryBind);

			const VkSparseMemoryBind imageWriteMipTailMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(),
				aspectRequirements.imageMipTailSize, memoryType, aspectRequirements.imageMipTailOffset);

			deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(imageWriteMipTailMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

			imageWriteMipTailBinds.push_back(imageWriteMipTailMemoryBind);
		}

		VkBindSparseInfo bindSparseInfo =
		{
			VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,		//VkStructureType							sType;
			DE_NULL,								//const void*								pNext;
			0u,										//deUint32									waitSemaphoreCount;
			DE_NULL,								//const VkSemaphore*						pWaitSemaphores;
			0u,										//deUint32									bufferBindCount;
			DE_NULL,								//const VkSparseBufferMemoryBindInfo*		pBufferBinds;
			0u,										//deUint32									imageOpaqueBindCount;
			DE_NULL,								//const VkSparseImageOpaqueMemoryBindInfo*	pImageOpaqueBinds;
			0u,										//deUint32									imageBindCount;
			DE_NULL,								//const VkSparseImageMemoryBindInfo*		pImageBinds;
			2u,										//deUint32									signalSemaphoreCount;
			imageMemoryBindSemaphores				//const VkSemaphore*						pSignalSemaphores;
		};

		VkSparseImageMemoryBindInfo		  imageResidencyBindInfo[2];
		VkSparseImageOpaqueMemoryBindInfo imageMipTailBindInfo[2];

		if (imageResidencyMemoryBinds.size() > 0)
		{
			imageResidencyBindInfo[0].image		= *imageRead;
			imageResidencyBindInfo[0].bindCount = static_cast<deUint32>(imageResidencyMemoryBinds.size());
			imageResidencyBindInfo[0].pBinds	= &imageResidencyMemoryBinds[0];

			imageResidencyBindInfo[1].image		= *imageWrite;
			imageResidencyBindInfo[1].bindCount = static_cast<deUint32>(imageResidencyMemoryBinds.size());
			imageResidencyBindInfo[1].pBinds	= &imageResidencyMemoryBinds[0];

			bindSparseInfo.imageBindCount		= 2u;
			bindSparseInfo.pImageBinds			= imageResidencyBindInfo;
		}

		if (imageReadMipTailBinds.size() > 0)
		{
			imageMipTailBindInfo[0].image		= *imageRead;
			imageMipTailBindInfo[0].bindCount	= static_cast<deUint32>(imageReadMipTailBinds.size());
			imageMipTailBindInfo[0].pBinds		= &imageReadMipTailBinds[0];

			imageMipTailBindInfo[1].image		= *imageWrite;
			imageMipTailBindInfo[1].bindCount	= static_cast<deUint32>(imageWriteMipTailBinds.size());
			imageMipTailBindInfo[1].pBinds		= &imageWriteMipTailBinds[0];

			bindSparseInfo.imageOpaqueBindCount = 2u;
			bindSparseInfo.pImageOpaqueBinds	= imageMipTailBindInfo;
		}

		// Submit sparse bind commands for execution
		VK_CHECK(deviceInterface.queueBindSparse(sparseQueue.queueHandle, 1u, &bindSparseInfo, DE_NULL));
	}

	// Create command buffer for compute and transfer oparations
	const Unique<VkCommandPool>	  commandPool  (makeCommandPool(deviceInterface, getDevice(), computeQueue.queueFamilyIndex));
	const Unique<VkCommandBuffer> commandBuffer(allocateCommandBuffer(deviceInterface, getDevice(), *commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	std::vector<VkBufferImageCopy> bufferImageCopy(imageSparseInfo.mipLevels);

	{
		deUint32 bufferOffset = 0u;
		for (deUint32 mipLevelNdx = 0u; mipLevelNdx < imageSparseInfo.mipLevels; ++mipLevelNdx)
		{
			bufferImageCopy[mipLevelNdx] = makeBufferImageCopy(mipLevelExtents(imageSparseInfo.extent, mipLevelNdx), imageSparseInfo.arrayLayers, mipLevelNdx, bufferOffset);
			bufferOffset += getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, mipLevelNdx, BUFFER_IMAGE_COPY_OFFSET_GRANULARITY);
		}
	}

	// Start recording commands
	beginCommandBuffer(deviceInterface, *commandBuffer);

	const deUint32					imageSizeInBytes		= getImageSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, imageSparseInfo.mipLevels, BUFFER_IMAGE_COPY_OFFSET_GRANULARITY);
	const VkBufferCreateInfo		inputBufferCreateInfo	= makeBufferCreateInfo(imageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	const Unique<VkBuffer>			inputBuffer				(createBuffer(deviceInterface, getDevice(), &inputBufferCreateInfo));
	const de::UniquePtr<Allocation>	inputBufferAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *inputBuffer, MemoryRequirement::HostVisible));

	std::vector<deUint8> referenceData(imageSizeInBytes);

	for (deUint32 mipLevelNdx = 0u; mipLevelNdx < imageSparseInfo.mipLevels; ++mipLevelNdx)
	{
		const deUint32 mipLevelSizeInBytes	= getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, mipLevelNdx);
		const deUint32 bufferOffset			= static_cast<deUint32>(bufferImageCopy[mipLevelNdx].bufferOffset);

		deMemset(&referenceData[bufferOffset], mipLevelNdx + 1u, mipLevelSizeInBytes);
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
			sparseQueue.queueFamilyIndex != computeQueue.queueFamilyIndex ? sparseQueue.queueFamilyIndex  : VK_QUEUE_FAMILY_IGNORED,
			sparseQueue.queueFamilyIndex != computeQueue.queueFamilyIndex ? computeQueue.queueFamilyIndex : VK_QUEUE_FAMILY_IGNORED,
			*imageRead,
			makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, imageSparseInfo.mipLevels, 0u, imageSparseInfo.arrayLayers)
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imageSparseTransferDstBarrier);
	}

	deviceInterface.cmdCopyBufferToImage(*commandBuffer, *inputBuffer, *imageRead, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<deUint32>(bufferImageCopy.size()), &bufferImageCopy[0]);

	{
		const VkImageMemoryBarrier imageSparseTransferSrcBarrier = makeImageMemoryBarrier
		(
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*imageRead,
			makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, imageSparseInfo.mipLevels, 0u, imageSparseInfo.arrayLayers)
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imageSparseTransferSrcBarrier);
	}

	{
		const VkImageMemoryBarrier imageSparseShaderStorageBarrier = makeImageMemoryBarrier
		(
			0u,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			*imageWrite,
			makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, imageSparseInfo.mipLevels, 0u, imageSparseInfo.arrayLayers)
		);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imageSparseShaderStorageBarrier);
	}

	// Create descriptor set layout
	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(deviceInterface, getDevice()));

	Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(deviceInterface, getDevice(), *descriptorSetLayout));

	Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imageSparseInfo.mipLevels)
		.build(deviceInterface, getDevice(), VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, imageSparseInfo.mipLevels));

	typedef de::SharedPtr< Unique<VkImageView> >		SharedVkImageView;
	std::vector<SharedVkImageView>						imageViews;
	imageViews.resize(imageSparseInfo.mipLevels);

	typedef de::SharedPtr< Unique<VkDescriptorSet> >	SharedVkDescriptorSet;
	std::vector<SharedVkDescriptorSet>					descriptorSets;
	descriptorSets.resize(imageSparseInfo.mipLevels);

	typedef de::SharedPtr< Unique<VkPipeline> >			SharedVkPipeline;
	std::vector<SharedVkPipeline>						computePipelines;
	computePipelines.resize(imageSparseInfo.mipLevels);

	for (deUint32 mipLevelNdx = 0u; mipLevelNdx < imageSparseInfo.mipLevels; ++mipLevelNdx)
	{
		std::ostringstream name;
		name << "comp" << mipLevelNdx;

		// Create and bind compute pipeline
		Unique<VkShaderModule> shaderModule(createShaderModule(deviceInterface, getDevice(), m_context.getBinaryCollection().get(name.str()), DE_NULL));

		computePipelines[mipLevelNdx]	= makeVkSharedPtr(makeComputePipeline(deviceInterface, getDevice(), *pipelineLayout, *shaderModule));
		VkPipeline computePipeline		= **computePipelines[mipLevelNdx];

		deviceInterface.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

		// Create and bind descriptor set
		descriptorSets[mipLevelNdx]		= makeVkSharedPtr(makeDescriptorSet(deviceInterface, getDevice(), *descriptorPool, *descriptorSetLayout));
		VkDescriptorSet descriptorSet	= **descriptorSets[mipLevelNdx];

		// Select which mipmap level to bind
		const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, mipLevelNdx, 1u, 0u, imageSparseInfo.arrayLayers);

		imageViews[mipLevelNdx] = makeVkSharedPtr(makeImageView(deviceInterface, getDevice(), *imageWrite, mapImageViewType(m_imageType), imageSparseInfo.format, subresourceRange));
		VkImageView imageView	= **imageViews[mipLevelNdx];

		const VkDescriptorImageInfo sparseImageInfo = makeDescriptorImageInfo(DE_NULL, imageView, VK_IMAGE_LAYOUT_GENERAL);

		DescriptorSetUpdateBuilder()
			.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &sparseImageInfo)
			.update(deviceInterface, getDevice());

		deviceInterface.cmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);

		const tcu::UVec3	gridSize			= getShaderGridSize(m_imageType, m_imageSize, mipLevelNdx);
		const deUint32		xWorkGroupSize		= std::min(std::min(gridSize.x(), maxWorkGroupSize.x()), maxWorkGroupInvocations);
		const deUint32		yWorkGroupSize		= std::min(std::min(gridSize.y(), maxWorkGroupSize.y()), maxWorkGroupInvocations / xWorkGroupSize);
		const deUint32		zWorkGroupSize		= std::min(std::min(gridSize.z(), maxWorkGroupSize.z()), maxWorkGroupInvocations / (xWorkGroupSize * yWorkGroupSize));

		const deUint32		xWorkGroupCount		= gridSize.x() / xWorkGroupSize + (gridSize.x() % xWorkGroupSize ? 1u : 0u);
		const deUint32		yWorkGroupCount		= gridSize.y() / yWorkGroupSize + (gridSize.y() % yWorkGroupSize ? 1u : 0u);
		const deUint32		zWorkGroupCount		= gridSize.z() / zWorkGroupSize + (gridSize.z() % zWorkGroupSize ? 1u : 0u);

		if (maxWorkGroupCount.x() < xWorkGroupCount ||
			maxWorkGroupCount.y() < yWorkGroupCount ||
			maxWorkGroupCount.z() < zWorkGroupCount)
			TCU_THROW(NotSupportedError, "Image size is not supported");

		deviceInterface.cmdDispatch(*commandBuffer, xWorkGroupCount, yWorkGroupCount, zWorkGroupCount);
	}

	{
		const VkMemoryBarrier memoryBarrier = makeMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 1u, &memoryBarrier, 0u, DE_NULL, 0u, DE_NULL);
	}

	const VkBufferCreateInfo		outputBufferCreateInfo	= makeBufferCreateInfo(imageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	const Unique<VkBuffer>			outputBuffer			(createBuffer(deviceInterface, getDevice(), &outputBufferCreateInfo));
	const de::UniquePtr<Allocation>	outputBufferAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *outputBuffer, MemoryRequirement::HostVisible));

	deviceInterface.cmdCopyImageToBuffer(*commandBuffer, *imageRead, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *outputBuffer, static_cast<deUint32>(bufferImageCopy.size()), &bufferImageCopy[0]);

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

	const VkPipelineStageFlags stageBits[] = { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

	// Submit commands for execution and wait for completion
	submitCommandsAndWait(deviceInterface, getDevice(), computeQueue.queueHandle, *commandBuffer, 2u, imageMemoryBindSemaphores, stageBits);

	// Retrieve data from buffer to host memory
	invalidateMappedMemoryRange(deviceInterface, getDevice(), outputBufferAlloc->getMemory(), outputBufferAlloc->getOffset(), imageSizeInBytes);

	const deUint8* outputData = static_cast<const deUint8*>(outputBufferAlloc->getHostPtr());

	// Wait for sparse queue to become idle
	deviceInterface.queueWaitIdle(sparseQueue.queueHandle);

	for (deUint32 mipLevelNdx = 0; mipLevelNdx < aspectRequirements.imageMipTailFirstLod; ++mipLevelNdx)
	{
		const tcu::UVec3				  gridSize		= getShaderGridSize(m_imageType, m_imageSize, mipLevelNdx);
		const deUint32					  bufferOffset	= static_cast<deUint32>(bufferImageCopy[mipLevelNdx].bufferOffset);
		const tcu::ConstPixelBufferAccess pixelBuffer	= tcu::ConstPixelBufferAccess(m_format, gridSize.x(), gridSize.y(), gridSize.z(), outputData + bufferOffset);

		for (deUint32 offsetZ = 0u; offsetZ < gridSize.z(); ++offsetZ)
		for (deUint32 offsetY = 0u; offsetY < gridSize.y(); ++offsetY)
		for (deUint32 offsetX = 0u; offsetX < gridSize.x(); ++offsetX)
		{
			const deUint32 index			= offsetX + (offsetY + offsetZ * gridSize.y()) * gridSize.x();
			const tcu::UVec4 referenceValue = tcu::UVec4(index % MODULO_DIVISOR, index % MODULO_DIVISOR, index % MODULO_DIVISOR, 1u);
			const tcu::UVec4 outputValue	= pixelBuffer.getPixelUint(offsetX, offsetY, offsetZ);

			if (deMemCmp(&outputValue, &referenceValue, sizeof(deUint32) * getNumUsedChannels(m_format.order)) != 0)
				return tcu::TestStatus::fail("Failed");
		}
	}

	for (deUint32 mipLevelNdx = aspectRequirements.imageMipTailFirstLod; mipLevelNdx < imageSparseInfo.mipLevels; ++mipLevelNdx)
	{
		const deUint32 mipLevelSizeInBytes	= getImageMipLevelSizeInBytes(imageSparseInfo.extent, imageSparseInfo.arrayLayers, m_format, mipLevelNdx);
		const deUint32 bufferOffset			= static_cast<deUint32>(bufferImageCopy[mipLevelNdx].bufferOffset);

		if (deMemCmp(outputData + bufferOffset, &referenceData[bufferOffset], mipLevelSizeInBytes) != 0)
			return tcu::TestStatus::fail("Failed");
	}

	return tcu::TestStatus::pass("Passed");
}

void ImageSparseMemoryAliasingCase::initPrograms(SourceCollections&	sourceCollections) const
{
	const char* const	versionDecl				= glu::getGLSLVersionDeclaration(m_glslVersion);
	const std::string	imageTypeStr			= getShaderImageType(m_format, m_imageType);
	const std::string	formatQualifierStr		= getShaderImageFormatQualifier(m_format);
	const std::string	formatDataStr			= getShaderImageDataType(m_format);
	const deUint32		maxWorkGroupInvocations = 128u;
	const tcu::UVec3	maxWorkGroupSize		= tcu::UVec3(128u, 128u, 64u);

	const tcu::UVec3	layerSize				= getLayerSize(m_imageType, m_imageSize);
	const deUint32		widestEdge				= std::max(std::max(layerSize.x(), layerSize.y()), layerSize.z());
	const deUint32		mipLevels				= static_cast<deUint32>(deFloatLog2(static_cast<float>(widestEdge))) + 1u;

	for (deUint32 mipLevelNdx = 0; mipLevelNdx < mipLevels; ++mipLevelNdx)
	{
		// Create compute program
		const tcu::UVec3	gridSize		= getShaderGridSize(m_imageType, m_imageSize, mipLevelNdx);
		const deUint32		xWorkGroupSize  = std::min(std::min(gridSize.x(), maxWorkGroupSize.x()), maxWorkGroupInvocations);
		const deUint32		yWorkGroupSize  = std::min(std::min(gridSize.y(), maxWorkGroupSize.y()), maxWorkGroupInvocations / xWorkGroupSize);
		const deUint32		zWorkGroupSize  = std::min(std::min(gridSize.z(), maxWorkGroupSize.z()), maxWorkGroupInvocations / (xWorkGroupSize * yWorkGroupSize));

		std::ostringstream src;

		src << versionDecl << "\n"
			<< "layout (local_size_x = " << xWorkGroupSize << ", local_size_y = " << yWorkGroupSize << ", local_size_z = " << zWorkGroupSize << ") in; \n"
			<< "layout (binding = 0, " << formatQualifierStr << ") writeonly uniform highp " << imageTypeStr << " u_image;\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "	if( gl_GlobalInvocationID.x < " << gridSize.x() << " ) \n"
			<< "	if( gl_GlobalInvocationID.y < " << gridSize.y() << " ) \n"
			<< "	if( gl_GlobalInvocationID.z < " << gridSize.z() << " ) \n"
			<< "	{\n"
			<< "		int index = int(gl_GlobalInvocationID.x + (gl_GlobalInvocationID.y + gl_GlobalInvocationID.z*" << gridSize.y() << ")*" << gridSize.x() << ");\n"
			<< "		imageStore(u_image, " << getCoordStr(m_imageType, "gl_GlobalInvocationID.x", "gl_GlobalInvocationID.y", "gl_GlobalInvocationID.z") << ","
			<< formatDataStr << "( index % " << MODULO_DIVISOR << ", index % " << MODULO_DIVISOR << ", index % " << MODULO_DIVISOR << ", 1 )); \n"
			<< "	}\n"
			<< "}\n";

		std::ostringstream name;
		name << "comp" << mipLevelNdx;
		sourceCollections.glslSources.add(name.str()) << glu::ComputeSource(src.str());
	}
}

TestInstance* ImageSparseMemoryAliasingCase::createInstance (Context& context) const
{
	return new ImageSparseMemoryAliasingInstance(context, m_imageType, m_imageSize, m_format);
}

} // anonymous ns

tcu::TestCaseGroup* createImageSparseMemoryAliasingTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "image_sparse_memory_aliasing", "Sparse Image Memory Aliasing"));

	static const deUint32 sizeCountPerImageType = 4u;

	struct ImageParameters
	{
		ImageType	imageType;
		tcu::UVec3	imageSizes[sizeCountPerImageType];
	};

	static const ImageParameters imageParametersArray[] =
	{
		{ IMAGE_TYPE_2D,		{ tcu::UVec3(512u, 256u, 1u),	tcu::UVec3(128u, 128u, 1u),	tcu::UVec3(503u, 137u, 1u),	tcu::UVec3(11u, 37u, 1u) } },
		{ IMAGE_TYPE_2D_ARRAY,	{ tcu::UVec3(512u, 256u, 6u),	tcu::UVec3(128u, 128u, 8u),	tcu::UVec3(503u, 137u, 3u),	tcu::UVec3(11u, 37u, 3u) } },
		{ IMAGE_TYPE_CUBE,		{ tcu::UVec3(256u, 256u, 1u),	tcu::UVec3(128u, 128u, 1u),	tcu::UVec3(137u, 137u, 1u),	tcu::UVec3(11u, 11u, 1u) } },
		{ IMAGE_TYPE_CUBE_ARRAY,{ tcu::UVec3(256u, 256u, 6u),	tcu::UVec3(128u, 128u, 8u),	tcu::UVec3(137u, 137u, 3u),	tcu::UVec3(11u, 11u, 3u) } },
		{ IMAGE_TYPE_3D,		{ tcu::UVec3(256u, 256u, 16u),	tcu::UVec3(128u, 128u, 8u),	tcu::UVec3(503u, 137u, 3u),	tcu::UVec3(11u, 37u, 3u) } }
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

				formatGroup->addChild(new ImageSparseMemoryAliasingCase(testCtx, stream.str(), "", imageType, imageSize, format, glu::GLSL_VERSION_440));
			}
			imageTypeGroup->addChild(formatGroup.release());
		}
		testGroup->addChild(imageTypeGroup.release());
	}

	return testGroup.release();
}

} // sparse
} // vkt
