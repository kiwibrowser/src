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
 * \file  vktSparseResourcesBufferSparseBinding.cpp
 * \brief Buffer Sparse Binding tests
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesBufferSparseBinding.hpp"
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

class BufferSparseBindingCase : public TestCase
{
public:
					BufferSparseBindingCase	(tcu::TestContext&	testCtx,
											 const std::string&	name,
											 const std::string&	description,
											 const deUint32		bufferSize);

	TestInstance*	createInstance			(Context&			context) const;

private:
	const deUint32	m_bufferSize;
};

BufferSparseBindingCase::BufferSparseBindingCase (tcu::TestContext&		testCtx,
												  const std::string&	name,
												  const std::string&	description,
												  const deUint32		bufferSize)
	: TestCase			(testCtx, name, description)
	, m_bufferSize		(bufferSize)
{
}

class BufferSparseBindingInstance : public SparseResourcesBaseInstance
{
public:
					BufferSparseBindingInstance (Context&		context,
												 const deUint32	bufferSize);

	tcu::TestStatus	iterate						(void);

private:
	const deUint32	m_bufferSize;
};

BufferSparseBindingInstance::BufferSparseBindingInstance (Context&			context,
														  const deUint32	bufferSize)

	: SparseResourcesBaseInstance	(context)
	, m_bufferSize					(bufferSize)
{
}

tcu::TestStatus BufferSparseBindingInstance::iterate (void)
{
	const InstanceInterface&	instance		= m_context.getInstanceInterface();
	const VkPhysicalDevice		physicalDevice	= m_context.getPhysicalDevice();

	if (!getPhysicalDeviceFeatures(instance, physicalDevice).sparseBinding)
		TCU_THROW(NotSupportedError, "Sparse binding not supported");

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

	VkBufferCreateInfo bufferCreateInfo;

	bufferCreateInfo.sType					= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;	// VkStructureType		sType;
	bufferCreateInfo.pNext					= DE_NULL;								// const void*			pNext;
	bufferCreateInfo.flags					= VK_BUFFER_CREATE_SPARSE_BINDING_BIT;	// VkBufferCreateFlags	flags;
	bufferCreateInfo.size					= m_bufferSize;							// VkDeviceSize			size;
	bufferCreateInfo.usage					= VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
											  VK_BUFFER_USAGE_TRANSFER_DST_BIT;		// VkBufferUsageFlags	usage;
	bufferCreateInfo.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;			// VkSharingMode		sharingMode;
	bufferCreateInfo.queueFamilyIndexCount	= 0u;									// deUint32				queueFamilyIndexCount;
	bufferCreateInfo.pQueueFamilyIndices	= DE_NULL;								// const deUint32*		pQueueFamilyIndices;

	const deUint32 queueFamilyIndices[] = { sparseQueue.queueFamilyIndex, computeQueue.queueFamilyIndex };

	if (sparseQueue.queueFamilyIndex != computeQueue.queueFamilyIndex)
	{
		bufferCreateInfo.sharingMode			= VK_SHARING_MODE_CONCURRENT;	// VkSharingMode		sharingMode;
		bufferCreateInfo.queueFamilyIndexCount	= 2u;							// deUint32				queueFamilyIndexCount;
		bufferCreateInfo.pQueueFamilyIndices	= queueFamilyIndices;			// const deUint32*		pQueueFamilyIndices;
	}

	// Create sparse buffer
	const Unique<VkBuffer> sparseBuffer(createBuffer(deviceInterface, getDevice(), &bufferCreateInfo));

	// Create sparse buffer memory bind semaphore
	const Unique<VkSemaphore> bufferMemoryBindSemaphore(createSemaphore(deviceInterface, getDevice()));

	const VkMemoryRequirements bufferMemRequirement = getBufferMemoryRequirements(deviceInterface, getDevice(), *sparseBuffer);

	if (bufferMemRequirement.size > getPhysicalDeviceProperties(instance, physicalDevice).limits.sparseAddressSpaceSize)
		TCU_THROW(NotSupportedError, "Required memory size for sparse resources exceeds device limits");

	DE_ASSERT((bufferMemRequirement.size % bufferMemRequirement.alignment) == 0);

	Move<VkDeviceMemory> sparseMemoryAllocation;

	{
		std::vector<VkSparseMemoryBind>	sparseMemoryBinds;
		const deUint32					numSparseBinds = static_cast<deUint32>(bufferMemRequirement.size / bufferMemRequirement.alignment);
		const deUint32					memoryType	   = findMatchingMemoryType(instance, physicalDevice, bufferMemRequirement, MemoryRequirement::Any);

		if (memoryType == NO_MATCH_FOUND)
			return tcu::TestStatus::fail("No matching memory type found");

		{
			const VkMemoryAllocateInfo allocateInfo =
			{
				VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,			// VkStructureType    sType;
				DE_NULL,										// const void*        pNext;
				bufferMemRequirement.size,						// VkDeviceSize       allocationSize;
				memoryType,										// uint32_t           memoryTypeIndex;
			};

			sparseMemoryAllocation = allocateMemory(deviceInterface, getDevice(), &allocateInfo);
		}

		for (deUint32 sparseBindNdx = 0; sparseBindNdx < numSparseBinds; ++sparseBindNdx)
		{
			const VkSparseMemoryBind sparseMemoryBind =
			{
				bufferMemRequirement.alignment * sparseBindNdx,			// VkDeviceSize               resourceOffset;
				bufferMemRequirement.alignment,							// VkDeviceSize               size;
				*sparseMemoryAllocation,								// VkDeviceMemory             memory;
				bufferMemRequirement.alignment * sparseBindNdx,			// VkDeviceSize               memoryOffset;
				(VkSparseMemoryBindFlags)0,								// VkSparseMemoryBindFlags    flags;
			};

			sparseMemoryBinds.push_back(sparseMemoryBind);
		}

		const VkSparseBufferMemoryBindInfo sparseBufferBindInfo = makeSparseBufferMemoryBindInfo(*sparseBuffer, numSparseBinds, &sparseMemoryBinds[0]);

		const VkBindSparseInfo bindSparseInfo =
		{
			VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,			//VkStructureType							sType;
			DE_NULL,									//const void*								pNext;
			0u,											//deUint32									waitSemaphoreCount;
			DE_NULL,									//const VkSemaphore*						pWaitSemaphores;
			1u,											//deUint32									bufferBindCount;
			&sparseBufferBindInfo,						//const VkSparseBufferMemoryBindInfo*		pBufferBinds;
			0u,											//deUint32									imageOpaqueBindCount;
			DE_NULL,									//const VkSparseImageOpaqueMemoryBindInfo*	pImageOpaqueBinds;
			0u,											//deUint32									imageBindCount;
			DE_NULL,									//const VkSparseImageMemoryBindInfo*		pImageBinds;
			1u,											//deUint32									signalSemaphoreCount;
			&bufferMemoryBindSemaphore.get()			//const VkSemaphore*						pSignalSemaphores;
		};

		// Submit sparse bind commands for execution
		VK_CHECK(deviceInterface.queueBindSparse(sparseQueue.queueHandle, 1u, &bindSparseInfo, DE_NULL));
	}

	// Create command buffer for transfer oparations
	const Unique<VkCommandPool>		commandPool(makeCommandPool(deviceInterface, getDevice(), computeQueue.queueFamilyIndex));
	const Unique<VkCommandBuffer>	commandBuffer(allocateCommandBuffer(deviceInterface, getDevice(), *commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording transfer commands
	beginCommandBuffer(deviceInterface, *commandBuffer);

	const VkBufferCreateInfo		inputBufferCreateInfo = makeBufferCreateInfo(m_bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	const Unique<VkBuffer>			inputBuffer				(createBuffer(deviceInterface, getDevice(), &inputBufferCreateInfo));
	const de::UniquePtr<Allocation>	inputBufferAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *inputBuffer, MemoryRequirement::HostVisible));

	std::vector<deUint8> referenceData;
	referenceData.resize(m_bufferSize);

	for (deUint32 valueNdx = 0; valueNdx < m_bufferSize; ++valueNdx)
	{
		referenceData[valueNdx] = static_cast<deUint8>((valueNdx % bufferMemRequirement.alignment) + 1u);
	}

	deMemcpy(inputBufferAlloc->getHostPtr(), &referenceData[0], m_bufferSize);

	flushMappedMemoryRange(deviceInterface, getDevice(), inputBufferAlloc->getMemory(), inputBufferAlloc->getOffset(), m_bufferSize);

	{
		const VkBufferMemoryBarrier inputBufferBarrier
			= makeBufferMemoryBarrier(	VK_ACCESS_HOST_WRITE_BIT,
										VK_ACCESS_TRANSFER_READ_BIT,
										*inputBuffer,
										0u,
										m_bufferSize);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 1u, &inputBufferBarrier, 0u, DE_NULL);
	}

	{
		const VkBufferCopy bufferCopy = makeBufferCopy(0u, 0u, m_bufferSize);

		deviceInterface.cmdCopyBuffer(*commandBuffer, *inputBuffer, *sparseBuffer, 1u, &bufferCopy);
	}

	{
		const VkBufferMemoryBarrier sparseBufferBarrier
			= makeBufferMemoryBarrier(	VK_ACCESS_TRANSFER_WRITE_BIT,
										VK_ACCESS_TRANSFER_READ_BIT,
										*sparseBuffer,
										0u,
										m_bufferSize);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 1u, &sparseBufferBarrier, 0u, DE_NULL);
	}

	const VkBufferCreateInfo		outputBufferCreateInfo	= makeBufferCreateInfo(m_bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	const Unique<VkBuffer>			outputBuffer			(createBuffer(deviceInterface, getDevice(), &outputBufferCreateInfo));
	const de::UniquePtr<Allocation>	outputBufferAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *outputBuffer, MemoryRequirement::HostVisible));

	{
		const VkBufferCopy bufferCopy = makeBufferCopy(0u, 0u, m_bufferSize);

		deviceInterface.cmdCopyBuffer(*commandBuffer, *sparseBuffer, *outputBuffer, 1u, &bufferCopy);
	}

	{
		const VkBufferMemoryBarrier outputBufferBarrier
			= makeBufferMemoryBarrier(	VK_ACCESS_TRANSFER_WRITE_BIT,
										VK_ACCESS_HOST_READ_BIT,
										*outputBuffer,
										0u,
										m_bufferSize);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL, 1u, &outputBufferBarrier, 0u, DE_NULL);
	}

	// End recording transfer commands
	endCommandBuffer(deviceInterface, *commandBuffer);

	const VkPipelineStageFlags waitStageBits[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

	// Submit transfer commands for execution and wait for completion
	submitCommandsAndWait(deviceInterface, getDevice(), computeQueue.queueHandle, *commandBuffer, 1u, &bufferMemoryBindSemaphore.get(), waitStageBits);

	// Retrieve data from output buffer to host memory
	invalidateMappedMemoryRange(deviceInterface, getDevice(), outputBufferAlloc->getMemory(), outputBufferAlloc->getOffset(), m_bufferSize);

	const deUint8* outputData = static_cast<const deUint8*>(outputBufferAlloc->getHostPtr());

	// Wait for sparse queue to become idle
	deviceInterface.queueWaitIdle(sparseQueue.queueHandle);

	// Compare output data with reference data
	if (deMemCmp(&referenceData[0], outputData, m_bufferSize) != 0)
		return tcu::TestStatus::fail("Failed");
	else
		return tcu::TestStatus::pass("Passed");
}

TestInstance* BufferSparseBindingCase::createInstance (Context& context) const
{
	return new BufferSparseBindingInstance(context, m_bufferSize);
}

} // anonymous ns

void addBufferSparseBindingTests (tcu::TestCaseGroup* group)
{
	group->addChild(new BufferSparseBindingCase(group->getTestContext(), "buffer_size_2_10", "", 1 << 10));
	group->addChild(new BufferSparseBindingCase(group->getTestContext(), "buffer_size_2_12", "", 1 << 12));
	group->addChild(new BufferSparseBindingCase(group->getTestContext(), "buffer_size_2_16", "", 1 << 16));
	group->addChild(new BufferSparseBindingCase(group->getTestContext(), "buffer_size_2_17", "", 1 << 17));
	group->addChild(new BufferSparseBindingCase(group->getTestContext(), "buffer_size_2_20", "", 1 << 20));
	group->addChild(new BufferSparseBindingCase(group->getTestContext(), "buffer_size_2_24", "", 1 << 24));
}

} // sparse
} // vkt
