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
 * \brief Test Case Skeleton Based on Compute Shaders
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmComputeShaderCase.hpp"

#include "deSharedPtr.hpp"
#include "deSTLUtil.hpp"

#include "vkBuilderUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPlatform.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"

namespace
{

using namespace vk;
using std::vector;

typedef vkt::SpirVAssembly::AllocationMp			AllocationMp;
typedef vkt::SpirVAssembly::AllocationSp			AllocationSp;

//typedef Unique<VkBuffer>							BufferHandleUp;
//typedef de::SharedPtr<BufferHandleUp>				BufferHandleSp;

typedef vk::Unique<VkBuffer>						BufferHandleUp;
typedef vk::Unique<VkImage>							ImageHandleUp;
typedef vk::Unique<VkImageView>						ImageViewHandleUp;
typedef vk::Unique<VkSampler>						SamplerHandleUp;
typedef de::SharedPtr<BufferHandleUp>				BufferHandleSp;
typedef de::SharedPtr<ImageHandleUp>				ImageHandleSp;
typedef de::SharedPtr<ImageViewHandleUp>			ImageViewHandleSp;
typedef de::SharedPtr<SamplerHandleUp>				SamplerHandleSp;

/*--------------------------------------------------------------------*//*!
 * \brief Create a buffer, allocate and bind memory for the buffer
 *
 * The memory is created as host visible and passed back as a vk::Allocation
 * instance via outMemory.
 *//*--------------------------------------------------------------------*/
Move<VkBuffer> createBufferAndBindMemory (const DeviceInterface& vkdi, const VkDevice& device, VkDescriptorType dtype, Allocator& allocator, size_t numBytes, AllocationMp* outMemory)
{
	VkBufferUsageFlags			usageBit			= (VkBufferUsageFlags)0;

	switch (dtype)
	{
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:			usageBit = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;	break;
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:			usageBit = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;	break;
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:			usageBit = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;	break;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:			usageBit = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;	break;
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:	usageBit = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;	break;
		default:										DE_FATAL("Not implemented");
	}

	const VkBufferCreateInfo	bufferCreateInfo	=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// sType
		DE_NULL,								// pNext
		0u,										// flags
		numBytes,								// size
		usageBit,								// usage
		VK_SHARING_MODE_EXCLUSIVE,				// sharingMode
		0u,										// queueFamilyCount
		DE_NULL,								// pQueueFamilyIndices
	};

	Move<VkBuffer>				buffer				(createBuffer(vkdi, device, &bufferCreateInfo));
	const VkMemoryRequirements	requirements		= getBufferMemoryRequirements(vkdi, device, *buffer);
	AllocationMp				bufferMemory		= allocator.allocate(requirements, MemoryRequirement::HostVisible);

	VK_CHECK(vkdi.bindBufferMemory(device, *buffer, bufferMemory->getMemory(), bufferMemory->getOffset()));
	*outMemory = bufferMemory;

	return buffer;
}

/*--------------------------------------------------------------------*//*!
 * \brief Create image, allocate and bind memory for the image
 *
 *//*--------------------------------------------------------------------*/
Move<VkImage> createImageAndBindMemory (const DeviceInterface& vkdi, const VkDevice& device, VkDescriptorType dtype, Allocator& allocator, deUint32 queueFamilyIndex, AllocationMp* outMemory)
{
	VkImageUsageFlags			usageBits			= (VkImageUsageFlags)0;

	switch (dtype)
	{
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:			usageBits = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;	break;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:			usageBits = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;	break;
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:	usageBits = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;	break;
		default:										DE_FATAL("Not implemented");
	}

	const VkImageCreateInfo		resourceImageParams	=
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,									//	VkStructureType		sType;
		DE_NULL,																//	const void*			pNext;
		0u,																		//	VkImageCreateFlags	flags;
		VK_IMAGE_TYPE_2D,														//	VkImageType			imageType;
		VK_FORMAT_R32G32B32A32_SFLOAT,											//	VkFormat			format;
		{ 8, 8, 1 },															//  VkExtent3D			extent;
		1u,																		//	deUint32			mipLevels;
		1u,																		//	deUint32			arraySize;
		VK_SAMPLE_COUNT_1_BIT,													//	deUint32			samples;
		VK_IMAGE_TILING_OPTIMAL,												//	VkImageTiling		tiling;
		usageBits,																//  VkImageUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,												//	VkSharingMode		sharingMode;
		1u,																		//	deUint32			queueFamilyCount;
		&queueFamilyIndex,														//	const deUint32*		pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,												//	VkImageLayout		initialLayout;
	};

	// Create image
	Move<VkImage>				image				= createImage(vkdi, device, &resourceImageParams);
	const VkMemoryRequirements	requirements		= getImageMemoryRequirements(vkdi, device, *image);
	de::MovePtr<Allocation>		imageMemory			= allocator.allocate(requirements, MemoryRequirement::Any);

	VK_CHECK(vkdi.bindImageMemory(device, *image, imageMemory->getMemory(), imageMemory->getOffset()));
	*outMemory = imageMemory;

	return image;
}

void copyBufferToImage (const DeviceInterface& vkdi, const VkDevice& device, const VkQueue& queue, VkCommandBuffer cmdBuffer, VkBuffer buffer, VkImage image)
{
	const VkBufferImageCopy			copyRegion			=
	{
		0u,												// VkDeviceSize				bufferOffset;
		0u,												// deUint32					bufferRowLength;
		0u,												// deUint32					bufferImageHeight;
		{
			VK_IMAGE_ASPECT_COLOR_BIT,						// VkImageAspectFlags		aspect;
			0u,												// deUint32					mipLevel;
			0u,												// deUint32					baseArrayLayer;
			1u,												// deUint32					layerCount;
		},												// VkImageSubresourceLayers	imageSubresource;
		{ 0, 0, 0 },									// VkOffset3D				imageOffset;
		{ 8, 8, 1 }										// VkExtent3D				imageExtent;
	};

	// Copy buffer to image
	const VkCommandBufferBeginInfo	cmdBufferBeginInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType							sType;
		DE_NULL,										// const void*								pNext;
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,	// VkCommandBufferUsageFlags				flags;
		DE_NULL											// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
	};

	const VkImageMemoryBarrier		imageBarriers[]		=
	{
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			DE_NULL,									// VkAccessFlags			srcAccessMask;
			VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,					// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,		// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32					dstQueueFamilyIndex;
			image,										// VkImage					image;
			{											// VkImageSubresourceRange	subresourceRange;
				VK_IMAGE_ASPECT_COLOR_BIT,		// VkImageAspectFlags	aspectMask;
				0u,								// deUint32				baseMipLevel;
				1u,								// deUint32				mipLevels;
				0u,								// deUint32				baseArraySlice;
				1u								// deUint32				arraySize;
			}
		},
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags			srcAccessMask;
			VK_ACCESS_SHADER_READ_BIT,					// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,		// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_GENERAL,					// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32					dstQueueFamilyIndex;
			image,										// VkImage					image;
			{											// VkImageSubresourceRange	subresourceRange;
				VK_IMAGE_ASPECT_COLOR_BIT,		// VkImageAspectFlags	aspectMask;
				0u,								// deUint32				baseMipLevel;
				1u,								// deUint32				mipLevels;
				0u,								// deUint32				baseArraySlice;
				1u								// deUint32				arraySize;
			}
		},
	};

	VK_CHECK(vkdi.beginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
	vkdi.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL,
		0u, DE_NULL, 1u, &imageBarriers[0]);
	vkdi.cmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyRegion);
	vkdi.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL,
		0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &imageBarriers[1]);

	VK_CHECK(vkdi.endCommandBuffer(cmdBuffer));

	{
		const VkFenceCreateInfo	fenceParams	=
		{
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,	//	VkStructureType		sType;
			DE_NULL,								//	const void*			pNext;
			0u,										//	VkFenceCreateFlags	flags;
		};

		const Unique<VkFence>	fence		(createFence(vkdi, device, &fenceParams));
		const VkSubmitInfo		submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType				sType;
			DE_NULL,								// const void*					pNext;
			0u,										// deUint32						waitSemaphoreCount;
			DE_NULL,								// const VkSemaphore*			pWaitSemaphores;
			DE_NULL,								// const VkPipelineStageFlags*	pWaitDstStageMask;
			1u,										// deUint32						commandBufferCount;
			&cmdBuffer,								// const VkCommandBuffer*		pCommandBuffers;
			0u,										// deUint32						signalSemaphoreCount;
			DE_NULL									// const VkSemaphore*			pSignalSemaphores;
		};

		VK_CHECK(vkdi.queueSubmit(queue, 1u, &submitInfo, *fence));
		VK_CHECK(vkdi.waitForFences(device, 1u, &fence.get(), DE_TRUE, ~0ull));
	}
}

void setMemory (const DeviceInterface& vkdi, const VkDevice& device, Allocation* destAlloc, size_t numBytes, const void* data)
{
	void* const hostPtr = destAlloc->getHostPtr();

	deMemcpy((deUint8*)hostPtr, data, numBytes);
	flushMappedMemoryRange(vkdi, device, destAlloc->getMemory(), destAlloc->getOffset(), numBytes);
}

void fillMemoryWithValue (const DeviceInterface& vkdi, const VkDevice& device, Allocation* destAlloc, size_t numBytes, deUint8 value)
{
	void* const hostPtr = destAlloc->getHostPtr();

	deMemset((deUint8*)hostPtr, value, numBytes);
	flushMappedMemoryRange(vkdi, device, destAlloc->getMemory(), destAlloc->getOffset(), numBytes);
}

void invalidateMemory (const DeviceInterface& vkdi, const VkDevice& device, Allocation* srcAlloc, size_t numBytes)
{
	invalidateMappedMemoryRange(vkdi, device, srcAlloc->getMemory(), srcAlloc->getOffset(), numBytes);
}

/*--------------------------------------------------------------------*//*!
 * \brief Create a descriptor set layout with the given descriptor types
 *
 * All descriptors are created for compute pipeline.
 *//*--------------------------------------------------------------------*/
Move<VkDescriptorSetLayout> createDescriptorSetLayout (const DeviceInterface& vkdi, const VkDevice& device, const vector<VkDescriptorType>& dtypes)
{
	DescriptorSetLayoutBuilder builder;

	for (size_t bindingNdx = 0; bindingNdx < dtypes.size(); ++bindingNdx)
		builder.addSingleBinding(dtypes[bindingNdx], VK_SHADER_STAGE_COMPUTE_BIT);

	return builder.build(vkdi, device);
}

/*--------------------------------------------------------------------*//*!
 * \brief Create a pipeline layout with one descriptor set
 *//*--------------------------------------------------------------------*/
Move<VkPipelineLayout> createPipelineLayout (const DeviceInterface& vkdi, const VkDevice& device, VkDescriptorSetLayout descriptorSetLayout, const vkt::SpirVAssembly::BufferSp& pushConstants)
{
	VkPipelineLayoutCreateInfo		createInfo	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,	// sType
		DE_NULL,										// pNext
		(VkPipelineLayoutCreateFlags)0,
		1u,												// descriptorSetCount
		&descriptorSetLayout,							// pSetLayouts
		0u,												// pushConstantRangeCount
		DE_NULL,										// pPushConstantRanges
	};

	VkPushConstantRange				range		=
	{
		VK_SHADER_STAGE_COMPUTE_BIT,					// stageFlags
		0,												// offset
		0,												// size
	};

	if (pushConstants != DE_NULL)
	{
		vector<deUint8> pushConstantsBytes;
		pushConstants->getBytes(pushConstantsBytes);

		range.size							= static_cast<deUint32>(pushConstantsBytes.size());
		createInfo.pushConstantRangeCount	= 1;
		createInfo.pPushConstantRanges		= &range;
	}

	return createPipelineLayout(vkdi, device, &createInfo);
}

/*--------------------------------------------------------------------*//*!
 * \brief Create a one-time descriptor pool for one descriptor set that
 * support the given descriptor types.
 *//*--------------------------------------------------------------------*/
inline Move<VkDescriptorPool> createDescriptorPool (const DeviceInterface& vkdi, const VkDevice& device, const vector<VkDescriptorType>& dtypes)
{
	DescriptorPoolBuilder builder;

	for (size_t typeNdx = 0; typeNdx < dtypes.size(); ++typeNdx)
		builder.addType(dtypes[typeNdx], 1);

	return builder.build(vkdi, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, /* maxSets = */ 1);
}

/*--------------------------------------------------------------------*//*!
 * \brief Create a descriptor set
 *
 * The descriptor set's layout contains the given descriptor types,
 * sequentially binded to binding points starting from 0.
 *//*--------------------------------------------------------------------*/
Move<VkDescriptorSet> createDescriptorSet (const DeviceInterface& vkdi, const VkDevice& device, VkDescriptorPool pool, VkDescriptorSetLayout layout, const vector<VkDescriptorType>& dtypes, const vector<VkDescriptorBufferInfo>& descriptorInfos, const vector<VkDescriptorImageInfo>& descriptorImageInfos)
{
	DE_ASSERT(dtypes.size() == descriptorInfos.size() + descriptorImageInfos.size());

	const VkDescriptorSetAllocateInfo	allocInfo	=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		DE_NULL,
		pool,
		1u,
		&layout
	};

	Move<VkDescriptorSet>				descriptorSet	= allocateDescriptorSet(vkdi, device, &allocInfo);
	DescriptorSetUpdateBuilder			builder;

	deUint32							bufferNdx		= 0u;
	deUint32							imageNdx		= 0u;

	for (deUint32 descriptorNdx = 0; descriptorNdx < dtypes.size(); ++descriptorNdx)
	{
		switch (dtypes[descriptorNdx])
		{
			// Write buffer descriptor
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				builder.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(descriptorNdx), dtypes[descriptorNdx], &descriptorInfos[bufferNdx++]);
				break;

			// Write image/sampler descriptor
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				builder.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(descriptorNdx), dtypes[descriptorNdx], &descriptorImageInfos[imageNdx++]);
				break;

			default:
				DE_FATAL("Not implemented");
		}
	}
	builder.update(vkdi, device);

	return descriptorSet;
}

/*--------------------------------------------------------------------*//*!
 * \brief Create a compute pipeline based on the given shader
 *//*--------------------------------------------------------------------*/
Move<VkPipeline> createComputePipeline (const DeviceInterface& vkdi, const VkDevice& device, VkPipelineLayout pipelineLayout, VkShaderModule shader, const char* entryPoint, const vector<deUint32>& specConstants)
{
	const deUint32							numSpecConstants				= (deUint32)specConstants.size();
	vector<VkSpecializationMapEntry>		entries;
	VkSpecializationInfo					specInfo;

	if (numSpecConstants != 0)
	{
		entries.resize(numSpecConstants);

		for (deUint32 ndx = 0; ndx < numSpecConstants; ++ndx)
		{
			entries[ndx].constantID	= ndx;
			entries[ndx].offset		= ndx * (deUint32)sizeof(deUint32);
			entries[ndx].size		= sizeof(deUint32);
		}

		specInfo.mapEntryCount		= numSpecConstants;
		specInfo.pMapEntries		= &entries[0];
		specInfo.dataSize			= numSpecConstants * sizeof(deUint32);
		specInfo.pData				= specConstants.data();
	}

	const VkPipelineShaderStageCreateInfo	pipelineShaderStageCreateInfo	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// sType
		DE_NULL,												// pNext
		(VkPipelineShaderStageCreateFlags)0,					// flags
		VK_SHADER_STAGE_COMPUTE_BIT,							// stage
		shader,													// module
		entryPoint,												// pName
		(numSpecConstants == 0) ? DE_NULL : &specInfo,			// pSpecializationInfo
	};
	const VkComputePipelineCreateInfo		pipelineCreateInfo				=
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,			// sType
		DE_NULL,												// pNext
		(VkPipelineCreateFlags)0,
		pipelineShaderStageCreateInfo,							// cs
		pipelineLayout,											// layout
		(VkPipeline)0,											// basePipelineHandle
		0u,														// basePipelineIndex
	};

	return createComputePipeline(vkdi, device, (VkPipelineCache)0u, &pipelineCreateInfo);
}

} // anonymous

namespace vkt
{
namespace SpirVAssembly
{

/*--------------------------------------------------------------------*//*!
 * \brief Test instance for compute pipeline
 *
 * The compute shader is specified in the format of SPIR-V assembly, which
 * is allowed to access MAX_NUM_INPUT_BUFFERS input storage buffers and
 * MAX_NUM_OUTPUT_BUFFERS output storage buffers maximally. The shader
 * source and input/output data are given in a ComputeShaderSpec object.
 *
 * This instance runs the given compute shader by feeding the data from input
 * buffers and compares the data in the output buffers with the expected.
 *//*--------------------------------------------------------------------*/
class SpvAsmComputeShaderInstance : public TestInstance
{
public:
										SpvAsmComputeShaderInstance	(Context& ctx, const ComputeShaderSpec& spec, const ComputeTestFeatures features);
	tcu::TestStatus						iterate						(void);

private:
	const ComputeShaderSpec&			m_shaderSpec;
	const ComputeTestFeatures			m_features;
};

// ComputeShaderTestCase implementations

SpvAsmComputeShaderCase::SpvAsmComputeShaderCase (tcu::TestContext& testCtx, const char* name, const char* description, const ComputeShaderSpec& spec, const ComputeTestFeatures features)
	: TestCase		(testCtx, name, description)
	, m_shaderSpec	(spec)
	, m_features	(features)
{
}

void SpvAsmComputeShaderCase::initPrograms (SourceCollections& programCollection) const
{
	programCollection.spirvAsmSources.add("compute") << m_shaderSpec.assembly.c_str();
}

TestInstance* SpvAsmComputeShaderCase::createInstance (Context& ctx) const
{
	return new SpvAsmComputeShaderInstance(ctx, m_shaderSpec, m_features);
}

// ComputeShaderTestInstance implementations

SpvAsmComputeShaderInstance::SpvAsmComputeShaderInstance (Context& ctx, const ComputeShaderSpec& spec, const ComputeTestFeatures features)
	: TestInstance		(ctx)
	, m_shaderSpec		(spec)
	, m_features		(features)
{
}

VkImageUsageFlags getMatchingComputeImageUsageFlags (VkDescriptorType dType)
{
	switch (dType)
	{
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:			return VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:			return VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:	return VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		default:										DE_FATAL("Not implemented");
	}
	return (VkImageUsageFlags)0;
}

tcu::TestStatus SpvAsmComputeShaderInstance::iterate (void)
{
	const VkPhysicalDeviceFeatures&		features			= m_context.getDeviceFeatures();
	const vector<std::string>&			extensions			= m_context.getDeviceExtensions();

	for (deUint32 extNdx = 0; extNdx < m_shaderSpec.extensions.size(); ++extNdx)
	{
		const std::string&				ext					= m_shaderSpec.extensions[extNdx];

		if (!de::contains(extensions.begin(), extensions.end(), ext))
		{
			TCU_THROW(NotSupportedError, (std::string("Device extension not supported: ") + ext).c_str());
		}
	}

	if ((m_features == COMPUTE_TEST_USES_INT16 || m_features == COMPUTE_TEST_USES_INT16_INT64) && !features.shaderInt16)
	{
		TCU_THROW(NotSupportedError, "shaderInt16 feature is not supported");
	}

	if ((m_features == COMPUTE_TEST_USES_INT64 || m_features == COMPUTE_TEST_USES_INT16_INT64) && !features.shaderInt64)
	{
		TCU_THROW(NotSupportedError, "shaderInt64 feature is not supported");
	}

	if ((m_features == COMPUTE_TEST_USES_FLOAT64) && !features.shaderFloat64)
	{
		TCU_THROW(NotSupportedError, "shaderFloat64 feature is not supported");
	}

	{
		const InstanceInterface&			vki					= m_context.getInstanceInterface();
		const VkPhysicalDevice				physicalDevice		= m_context.getPhysicalDevice();

		// 16bit storage features
		{
			if (!is16BitStorageFeaturesSupported(vki, physicalDevice, m_context.getInstanceExtensions(), m_shaderSpec.requestedVulkanFeatures.ext16BitStorage))
				TCU_THROW(NotSupportedError, "Requested 16bit storage features not supported");
		}

		// VariablePointers features
		{
			if (!isVariablePointersFeaturesSupported(vki, physicalDevice, m_context.getInstanceExtensions(), m_shaderSpec.requestedVulkanFeatures.extVariablePointers))
				TCU_THROW(NotSupportedError, "Request Variable Pointer feature not supported");
		}
	}

	// defer device and resource creation until after feature checks
	const deUint32						queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	const Unique<VkDevice>				vkDevice			(createDeviceWithExtensions(m_context, queueFamilyIndex, m_context.getDeviceExtensions(), m_shaderSpec.extensions));
	const VkDevice&						device				= *vkDevice;
	const DeviceDriver					vkDeviceInterface	(m_context.getInstanceInterface(), device);
	const DeviceInterface&				vkdi				= vkDeviceInterface;
	const de::UniquePtr<vk::Allocator>	vkAllocator			(createAllocator(m_context.getInstanceInterface(), m_context.getPhysicalDevice(), vkDeviceInterface, device));
	Allocator&							allocator			= *vkAllocator;
	const VkQueue						queue				(getDeviceQueue(vkDeviceInterface, device, queueFamilyIndex, 0));

	vector<AllocationSp>				inputAllocs;
	vector<AllocationSp>				outputAllocs;
	vector<BufferHandleSp>				inputBuffers;
	vector<ImageHandleSp>				inputImages;
	vector<ImageViewHandleSp>			inputImageViews;
	vector<SamplerHandleSp>				inputSamplers;
	vector<BufferHandleSp>				outputBuffers;
	vector<VkDescriptorBufferInfo>		descriptorInfos;
	vector<VkDescriptorImageInfo>		descriptorImageInfos;
	vector<VkDescriptorType>			descriptorTypes;

	DE_ASSERT(!m_shaderSpec.outputs.empty());

	// Create command pool and command buffer

	const Unique<VkCommandPool>			cmdPool				(createCommandPool(vkdi, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	Unique<VkCommandBuffer>				cmdBuffer			(allocateCommandBuffer(vkdi, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Create buffer and image objects, allocate storage, and create view for all input/output buffers and images.

	for (deUint32 inputNdx = 0; inputNdx < m_shaderSpec.inputs.size(); ++inputNdx)
	{
		if (m_shaderSpec.inputTypes.count(inputNdx) != 0)
			descriptorTypes.push_back(m_shaderSpec.inputTypes.at(inputNdx));
		else
			descriptorTypes.push_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		const VkDescriptorType	descType	= descriptorTypes[inputNdx];

		const bool				hasImage	= (descType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)	||
											  (descType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)	||
											  (descType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		const bool				hasSampler	= (descType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)	||
											  (descType == VK_DESCRIPTOR_TYPE_SAMPLER)			||
											  (descType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		// Buffer
		if (!hasImage && !hasSampler)
		{
			const BufferSp&		input			= m_shaderSpec.inputs[inputNdx];
			vector<deUint8>		inputBytes;

			input->getBytes(inputBytes);

			const size_t		numBytes		= inputBytes.size();

			AllocationMp		bufferAlloc;
			BufferHandleUp*		buffer			= new BufferHandleUp(createBufferAndBindMemory(vkdi, device, descType, allocator, numBytes, &bufferAlloc));

			setMemory(vkdi, device, &*bufferAlloc, numBytes, &inputBytes.front());
			inputBuffers.push_back(BufferHandleSp(buffer));
			inputAllocs.push_back(de::SharedPtr<Allocation>(bufferAlloc.release()));
		}
		// Image
		else if (hasImage)
		{
			const BufferSp&		input			= m_shaderSpec.inputs[inputNdx];
			vector<deUint8>		inputBytes;

			input->getBytes(inputBytes);

			const size_t		numBytes		= inputBytes.size();

			AllocationMp		bufferAlloc;
			BufferHandleUp*		buffer			= new BufferHandleUp(createBufferAndBindMemory(vkdi, device, descType, allocator, numBytes, &bufferAlloc));

			AllocationMp		imageAlloc;
			ImageHandleUp*		image			= new ImageHandleUp(createImageAndBindMemory(vkdi, device, descType, allocator, queueFamilyIndex, &imageAlloc));

			setMemory(vkdi, device, &*bufferAlloc, numBytes, &inputBytes.front());

			inputBuffers.push_back(BufferHandleSp(buffer));
			inputAllocs.push_back(de::SharedPtr<Allocation>(bufferAlloc.release()));

			inputImages.push_back(ImageHandleSp(image));
			inputAllocs.push_back(de::SharedPtr<Allocation>(imageAlloc.release()));

			copyBufferToImage(vkdi, device, queue, cmdBuffer.get(), buffer->get(), image->get());
		}
	}

	deUint32							imageNdx			= 0u;
	deUint32							bufferNdx			= 0u;

	for (deUint32 inputNdx = 0; inputNdx < descriptorTypes.size(); ++inputNdx)
	{
		const VkDescriptorType	descType	= descriptorTypes[inputNdx];

		const bool				hasImage	= (descType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)	||
											  (descType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)	||
											  (descType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		const bool				hasSampler	= (descType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)	||
											  (descType == VK_DESCRIPTOR_TYPE_SAMPLER)			||
											  (descType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		// Create image view and sampler
		if (hasImage || hasSampler)
		{
			if (descType != VK_DESCRIPTOR_TYPE_SAMPLER)
			{
				const VkImageViewCreateInfo	imgViewParams	=
				{
					VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	//	VkStructureType			sType;
					DE_NULL,									//	const void*				pNext;
					0u,											//	VkImageViewCreateFlags	flags;
					**inputImages[imageNdx++],					//	VkImage					image;
					VK_IMAGE_VIEW_TYPE_2D,						//	VkImageViewType			viewType;
					VK_FORMAT_R32G32B32A32_SFLOAT,				//	VkFormat				format;
					{
						VK_COMPONENT_SWIZZLE_R,
						VK_COMPONENT_SWIZZLE_G,
						VK_COMPONENT_SWIZZLE_B,
						VK_COMPONENT_SWIZZLE_A
					},											//	VkChannelMapping		channels;
					{
						VK_IMAGE_ASPECT_COLOR_BIT,					//	VkImageAspectFlags		aspectMask;
						0u,											//	deUint32				baseMipLevel;
						1u,											//	deUint32				mipLevels;
						0u,											//	deUint32				baseArrayLayer;
						1u,											//	deUint32				arraySize;
					},											//	VkImageSubresourceRange	subresourceRange;
				};

				Move<VkImageView>			imgView			(createImageView(vkdi, *vkDevice, &imgViewParams));
				inputImageViews.push_back(ImageViewHandleSp(new ImageViewHandleUp(imgView)));
			}

			if (hasSampler)
			{
				const VkSamplerCreateInfo	samplerParams	=
				{
					VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,		// VkStructureType			sType;
					DE_NULL,									// const void*				pNext;
					0,											// VkSamplerCreateFlags		flags;
					VK_FILTER_NEAREST,							// VkFilter					magFilter:
					VK_FILTER_NEAREST,							// VkFilter					minFilter;
					VK_SAMPLER_MIPMAP_MODE_NEAREST,				// VkSamplerMipmapMode		mipmapMode;
					VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeU;
					VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeV;
					VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeW;
					0.0f,										// float					mipLodBias;
					VK_FALSE,									// VkBool32					anistoropyÉnable;
					1.0f,										// float					maxAnisotropy;
					VK_FALSE,									// VkBool32					compareEnable;
					VK_COMPARE_OP_ALWAYS,						// VkCompareOp				compareOp;
					0.0f,										// float					minLod;
					0.0f,										// float					maxLod;
					VK_BORDER_COLOR_INT_OPAQUE_BLACK,			// VkBorderColor			borderColor;
					VK_FALSE									// VkBool32					unnormalizedCoordinates;
				};

				Move<VkSampler>				sampler			(createSampler(vkdi, *vkDevice, &samplerParams));
				inputSamplers.push_back(SamplerHandleSp(new SamplerHandleUp(sampler)));
			}
		}

		// Create descriptor buffer and image infos
		switch (descType)
		{
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				const VkDescriptorBufferInfo bufInfo =
				{
					**inputBuffers[bufferNdx++],				// VkBuffer					buffer;
					0,											// VkDeviceSize				offset;
					VK_WHOLE_SIZE,								// VkDeviceSize				size;
				};
				descriptorInfos.push_back(bufInfo);
				break;
			}

			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			{
				const VkDescriptorImageInfo	imgInfo	=
				{
					DE_NULL,									// VkSampler				sampler;
					**inputImageViews.back(),					// VkImageView				imageView;
					VK_IMAGE_LAYOUT_GENERAL						// VkImageLayout			imageLayout;
				};
				descriptorImageInfos.push_back(imgInfo);
				break;
			}

			case VK_DESCRIPTOR_TYPE_SAMPLER:
			{
				const VkDescriptorImageInfo	imgInfo	=
				{
					**inputSamplers.back(),						// VkSampler				sampler;
					DE_NULL,									// VkImageView				imageView;
					VK_IMAGE_LAYOUT_GENERAL						// VkImageLayout			imageLayout;
				};
				descriptorImageInfos.push_back(imgInfo);
				break;
			}

			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			{

				const VkDescriptorImageInfo	imgInfo	=
				{
					**inputSamplers.back(),						// VkSampler				sampler;
					**inputImageViews.back(),					// VkImageView				imageView;
					VK_IMAGE_LAYOUT_GENERAL						// VkImageLayout			imageLayout;
				};
				descriptorImageInfos.push_back(imgInfo);
				break;
			}

			default:
				DE_FATAL("Not implemented");
		}
	}

	for (deUint32 outputNdx = 0; outputNdx < m_shaderSpec.outputs.size(); ++outputNdx)
	{
		descriptorTypes.push_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		AllocationMp		alloc;
		const BufferSp&		output		= m_shaderSpec.outputs[outputNdx];
		vector<deUint8>		outputBytes;

		output->getBytes(outputBytes);

		const size_t		numBytes	= outputBytes.size();
		BufferHandleUp*		buffer		= new BufferHandleUp(createBufferAndBindMemory(vkdi, device, descriptorTypes.back(), allocator, numBytes, &alloc));

		fillMemoryWithValue(vkdi, device, &*alloc, numBytes, 0xff);
		descriptorInfos.push_back(vk::makeDescriptorBufferInfo(**buffer, 0u, numBytes));
		outputBuffers.push_back(BufferHandleSp(buffer));
		outputAllocs.push_back(de::SharedPtr<Allocation>(alloc.release()));
	}

	// Create layouts and descriptor set.

	Unique<VkDescriptorSetLayout>		descriptorSetLayout	(createDescriptorSetLayout(vkdi, device, descriptorTypes));
	Unique<VkPipelineLayout>			pipelineLayout		(createPipelineLayout(vkdi, device, *descriptorSetLayout, m_shaderSpec.pushConstants));
	Unique<VkDescriptorPool>			descriptorPool		(createDescriptorPool(vkdi, device, descriptorTypes));
	Unique<VkDescriptorSet>				descriptorSet		(createDescriptorSet(vkdi, device, *descriptorPool, *descriptorSetLayout, descriptorTypes, descriptorInfos, descriptorImageInfos));

	// Create compute shader and pipeline.

	const ProgramBinary&				binary				= m_context.getBinaryCollection().get("compute");
	Unique<VkShaderModule>				module				(createShaderModule(vkdi, device, binary, (VkShaderModuleCreateFlags)0u));

	Unique<VkPipeline>					computePipeline		(createComputePipeline(vkdi, device, *pipelineLayout, *module, m_shaderSpec.entryPoint.c_str(), m_shaderSpec.specConstants));

	const VkCommandBufferBeginInfo		cmdBufferBeginInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType
		DE_NULL,										// pNext
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const tcu::IVec3&				numWorkGroups		= m_shaderSpec.numWorkGroups;

	VK_CHECK(vkdi.beginCommandBuffer(*cmdBuffer, &cmdBufferBeginInfo));
	vkdi.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *computePipeline);
	vkdi.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0, 1, &descriptorSet.get(), 0, DE_NULL);
	if (m_shaderSpec.pushConstants != DE_NULL)
	{
		vector<deUint8>	pushConstantsBytes;
		m_shaderSpec.pushConstants->getBytes(pushConstantsBytes);

		const deUint32	size	= static_cast<deUint32>(pushConstantsBytes.size());
		const void*		data	= &pushConstantsBytes.front();

		vkdi.cmdPushConstants(*cmdBuffer, *pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, /* offset = */ 0, /* size = */ size, data);
	}
	vkdi.cmdDispatch(*cmdBuffer, numWorkGroups.x(), numWorkGroups.y(), numWorkGroups.z());
	VK_CHECK(vkdi.endCommandBuffer(*cmdBuffer));

	// Create fence and run.

	const Unique<VkFence>			cmdCompleteFence	(createFence(vkdi, device));
	const deUint64					infiniteTimeout		= ~(deUint64)0u;
	const VkSubmitInfo				submitInfo			=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		DE_NULL,
		0u,
		(const VkSemaphore*)DE_NULL,
		(const VkPipelineStageFlags*)DE_NULL,
		1u,
		&cmdBuffer.get(),
		0u,
		(const VkSemaphore*)DE_NULL,
	};

	VK_CHECK(vkdi.queueSubmit(queue, 1, &submitInfo, *cmdCompleteFence));
	VK_CHECK(vkdi.waitForFences(device, 1, &cmdCompleteFence.get(), 0u, infiniteTimeout)); // \note: timeout is failure

	// Invalidate output memory ranges before checking on host.
	for (size_t outputNdx = 0; outputNdx < m_shaderSpec.outputs.size(); ++outputNdx)
	{
		invalidateMemory(vkdi, device, outputAllocs[outputNdx].get(), m_shaderSpec.outputs[outputNdx]->getByteSize());
	}

	// Check output.
	if (m_shaderSpec.verifyIO)
	{
		if (!(*m_shaderSpec.verifyIO)(m_shaderSpec.inputs, outputAllocs, m_shaderSpec.outputs, m_context.getTestContext().getLog()))
			return tcu::TestStatus(m_shaderSpec.failResult, m_shaderSpec.failMessage);
	}
	else
	{
		for (size_t outputNdx = 0; outputNdx < m_shaderSpec.outputs.size(); ++outputNdx)
		{
			const BufferSp&	expectedOutput = m_shaderSpec.outputs[outputNdx];
			vector<deUint8>	expectedBytes;

			expectedOutput->getBytes(expectedBytes);

			if (deMemCmp(&expectedBytes.front(), outputAllocs[outputNdx]->getHostPtr(), expectedBytes.size()))
				return tcu::TestStatus(m_shaderSpec.failResult, m_shaderSpec.failMessage);
		}
	}

	return tcu::TestStatus::pass("Output match with expected");
}

} // SpirVAssembly
} // vkt
