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
 * \file  vktSparseResourcesBufferSparseResidency.cpp
 * \brief Sparse partially resident buffers tests
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesBufferSparseResidency.hpp"
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
	SIZE_OF_UINT_IN_SHADER = 4u,
};

class BufferSparseResidencyCase : public TestCase
{
public:
					BufferSparseResidencyCase	(tcu::TestContext&		testCtx,
												 const std::string&		name,
												 const std::string&		description,
												 const deUint32			bufferSize,
												 const glu::GLSLVersion	glslVersion);

	void			initPrograms				(SourceCollections&		sourceCollections) const;
	TestInstance*	createInstance				(Context&				context) const;

private:
	const deUint32			m_bufferSize;
	const glu::GLSLVersion	m_glslVersion;
};

BufferSparseResidencyCase::BufferSparseResidencyCase (tcu::TestContext&			testCtx,
													  const std::string&		name,
													  const std::string&		description,
													  const deUint32			bufferSize,
													  const glu::GLSLVersion	glslVersion)
	: TestCase			(testCtx, name, description)
	, m_bufferSize		(bufferSize)
	, m_glslVersion		(glslVersion)
{
}

void BufferSparseResidencyCase::initPrograms (SourceCollections& sourceCollections) const
{
	const char* const	versionDecl		= glu::getGLSLVersionDeclaration(m_glslVersion);
	const deUint32		iterationsCount = m_bufferSize / SIZE_OF_UINT_IN_SHADER;

	std::ostringstream src;

	src << versionDecl << "\n"
		<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		<< "layout(set = 0, binding = 0, std430) readonly buffer Input\n"
		<< "{\n"
		<< "	uint data[];\n"
		<< "} sb_in;\n"
		<< "\n"
		<< "layout(set = 0, binding = 1, std430) writeonly buffer Output\n"
		<< "{\n"
		<< "	uint result[];\n"
		<< "} sb_out;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	for(int i=0; i<" << iterationsCount << "; ++i) \n"
		<< "	{\n"
		<< "		sb_out.result[i] = sb_in.data[i];"
		<< "	}\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

class BufferSparseResidencyInstance : public SparseResourcesBaseInstance
{
public:
					BufferSparseResidencyInstance	(Context&			context,
													 const deUint32		bufferSize);

	tcu::TestStatus	iterate							(void);

private:
	const deUint32	m_bufferSize;
};

BufferSparseResidencyInstance::BufferSparseResidencyInstance (Context&			context,
														      const deUint32	bufferSize)
	: SparseResourcesBaseInstance	(context)
	, m_bufferSize					(bufferSize)
{
}

tcu::TestStatus BufferSparseResidencyInstance::iterate (void)
{
	const InstanceInterface&		 instance					= m_context.getInstanceInterface();
	const VkPhysicalDevice			 physicalDevice				= m_context.getPhysicalDevice();
	const VkPhysicalDeviceProperties physicalDeviceProperties	= getPhysicalDeviceProperties(instance, physicalDevice);

	if (!getPhysicalDeviceFeatures(instance, physicalDevice).sparseResidencyBuffer)
		TCU_THROW(NotSupportedError, "Sparse partially resident buffers not supported");

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

	VkBufferCreateInfo bufferCreateInfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		VK_BUFFER_CREATE_SPARSE_BINDING_BIT |
		VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT,	// VkBufferCreateFlags	flags;
		m_bufferSize,							// VkDeviceSize			size;
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,		// VkBufferUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
		0u,										// deUint32				queueFamilyIndexCount;
		DE_NULL									// const deUint32*		pQueueFamilyIndices;
	};

	const deUint32 queueFamilyIndices[] = { sparseQueue.queueFamilyIndex, computeQueue.queueFamilyIndex };

	if (sparseQueue.queueFamilyIndex != computeQueue.queueFamilyIndex)
	{
		bufferCreateInfo.sharingMode			= VK_SHARING_MODE_CONCURRENT;
		bufferCreateInfo.queueFamilyIndexCount	= 2u;
		bufferCreateInfo.pQueueFamilyIndices	= queueFamilyIndices;
	}

	// Create sparse buffer
	const Unique<VkBuffer> sparseBuffer(createBuffer(deviceInterface, getDevice(), &bufferCreateInfo));

	// Create sparse buffer memory bind semaphore
	const Unique<VkSemaphore> bufferMemoryBindSemaphore(createSemaphore(deviceInterface, getDevice()));

	const VkMemoryRequirements bufferMemRequirements = getBufferMemoryRequirements(deviceInterface, getDevice(), *sparseBuffer);

	if (bufferMemRequirements.size > physicalDeviceProperties.limits.sparseAddressSpaceSize)
		TCU_THROW(NotSupportedError, "Required memory size for sparse resources exceeds device limits");

	DE_ASSERT((bufferMemRequirements.size % bufferMemRequirements.alignment) == 0);

	const deUint32				numSparseSlots = static_cast<deUint32>(bufferMemRequirements.size / bufferMemRequirements.alignment);
	std::vector<DeviceMemorySp>	deviceMemUniquePtrVec;

	{
		std::vector<VkSparseMemoryBind>		sparseMemoryBinds;
		const deUint32						memoryType		= findMatchingMemoryType(instance, physicalDevice, bufferMemRequirements, MemoryRequirement::Any);

		if (memoryType == NO_MATCH_FOUND)
			return tcu::TestStatus::fail("No matching memory type found");

		for (deUint32 sparseBindNdx = 0; sparseBindNdx < numSparseSlots; sparseBindNdx += 2)
		{
			const VkSparseMemoryBind sparseMemoryBind = makeSparseMemoryBind(deviceInterface, getDevice(), bufferMemRequirements.alignment, memoryType, bufferMemRequirements.alignment * sparseBindNdx);

			deviceMemUniquePtrVec.push_back(makeVkSharedPtr(Move<VkDeviceMemory>(check<VkDeviceMemory>(sparseMemoryBind.memory), Deleter<VkDeviceMemory>(deviceInterface, getDevice(), DE_NULL))));

			sparseMemoryBinds.push_back(sparseMemoryBind);
		}

		const VkSparseBufferMemoryBindInfo sparseBufferBindInfo = makeSparseBufferMemoryBindInfo(*sparseBuffer, static_cast<deUint32>(sparseMemoryBinds.size()), &sparseMemoryBinds[0]);

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

		VK_CHECK(deviceInterface.queueBindSparse(sparseQueue.queueHandle, 1u, &bindSparseInfo, DE_NULL));
	}

	// Create input buffer
	const VkBufferCreateInfo		inputBufferCreateInfo	= makeBufferCreateInfo(m_bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	const Unique<VkBuffer>			inputBuffer				(createBuffer(deviceInterface, getDevice(), &inputBufferCreateInfo));
	const de::UniquePtr<Allocation>	inputBufferAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *inputBuffer, MemoryRequirement::HostVisible));


	std::vector<deUint8> referenceData;
	referenceData.resize(m_bufferSize);

	for (deUint32 valueNdx = 0; valueNdx < m_bufferSize; ++valueNdx)
	{
		referenceData[valueNdx] = static_cast<deUint8>((valueNdx % bufferMemRequirements.alignment) + 1u);
	}

	deMemcpy(inputBufferAlloc->getHostPtr(), &referenceData[0], m_bufferSize);

	flushMappedMemoryRange(deviceInterface, getDevice(), inputBufferAlloc->getMemory(), inputBufferAlloc->getOffset(), m_bufferSize);

	// Create output buffer
	const VkBufferCreateInfo		outputBufferCreateInfo	= makeBufferCreateInfo(m_bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	const Unique<VkBuffer>			outputBuffer			(createBuffer(deviceInterface, getDevice(), &outputBufferCreateInfo));
	const de::UniquePtr<Allocation>	outputBufferAlloc		(bindBuffer(deviceInterface, getDevice(), getAllocator(), *outputBuffer, MemoryRequirement::HostVisible));

	// Create command buffer for compute and data transfer oparations
	const Unique<VkCommandPool>	  commandPool(makeCommandPool(deviceInterface, getDevice(), computeQueue.queueFamilyIndex));
	const Unique<VkCommandBuffer> commandBuffer(allocateCommandBuffer(deviceInterface, getDevice(), *commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording compute and transfer commands
	beginCommandBuffer(deviceInterface, *commandBuffer);

	// Create descriptor set
	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(deviceInterface, getDevice()));

	// Create compute pipeline
	const Unique<VkShaderModule>	shaderModule(createShaderModule(deviceInterface, getDevice(), m_context.getBinaryCollection().get("comp"), DE_NULL));
	const Unique<VkPipelineLayout>	pipelineLayout(makePipelineLayout(deviceInterface, getDevice(), *descriptorSetLayout));
	const Unique<VkPipeline>		computePipeline(makeComputePipeline(deviceInterface, getDevice(), *pipelineLayout, *shaderModule));

	deviceInterface.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *computePipeline);

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u)
		.build(deviceInterface, getDevice(), VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(deviceInterface, getDevice(), *descriptorPool, *descriptorSetLayout));

	{
		const VkDescriptorBufferInfo inputBufferInfo = makeDescriptorBufferInfo(*inputBuffer, 0ull, m_bufferSize);
		const VkDescriptorBufferInfo sparseBufferInfo = makeDescriptorBufferInfo(*sparseBuffer, 0ull, m_bufferSize);

		DescriptorSetUpdateBuilder()
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &inputBufferInfo)
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &sparseBufferInfo)
			.update(deviceInterface, getDevice());
	}

	deviceInterface.cmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	{
		const VkBufferMemoryBarrier inputBufferBarrier
			= makeBufferMemoryBarrier(	VK_ACCESS_HOST_WRITE_BIT,
										VK_ACCESS_SHADER_READ_BIT,
										*inputBuffer,
										0ull,
										m_bufferSize);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0u, 0u, DE_NULL, 1u, &inputBufferBarrier, 0u, DE_NULL);
	}

	deviceInterface.cmdDispatch(*commandBuffer, 1u, 1u, 1u);

	{
		const VkBufferMemoryBarrier sparseBufferBarrier
			= makeBufferMemoryBarrier(	VK_ACCESS_SHADER_WRITE_BIT,
										VK_ACCESS_TRANSFER_READ_BIT,
										*sparseBuffer,
										0ull,
										m_bufferSize);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 1u, &sparseBufferBarrier, 0u, DE_NULL);
	}

	{
		const VkBufferCopy bufferCopy = makeBufferCopy(0u, 0u, m_bufferSize);

		deviceInterface.cmdCopyBuffer(*commandBuffer, *sparseBuffer, *outputBuffer, 1u, &bufferCopy);
	}

	{
		const VkBufferMemoryBarrier outputBufferBarrier
			= makeBufferMemoryBarrier(	VK_ACCESS_TRANSFER_WRITE_BIT,
										VK_ACCESS_HOST_READ_BIT,
										*outputBuffer,
										0ull,
										m_bufferSize);

		deviceInterface.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL, 1u, &outputBufferBarrier, 0u, DE_NULL);
	}

	// End recording compute and transfer commands
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
	for (deUint32 sparseBindNdx = 0; sparseBindNdx < numSparseSlots; ++sparseBindNdx)
	{
		const deUint32 alignment = static_cast<deUint32>(bufferMemRequirements.alignment);
		const deUint32 offset	 = alignment * sparseBindNdx;
		const deUint32 size		 = sparseBindNdx == (numSparseSlots - 1) ? m_bufferSize % alignment : alignment;

		if (sparseBindNdx % 2u == 0u)
		{
			if (deMemCmp(&referenceData[offset], outputData + offset, size) != 0)
				return tcu::TestStatus::fail("Failed");
		}
		else if (physicalDeviceProperties.sparseProperties.residencyNonResidentStrict)
		{
			deMemset(&referenceData[offset], 0u, size);

			if (deMemCmp(&referenceData[offset], outputData + offset, size) != 0)
				return tcu::TestStatus::fail("Failed");
		}
	}

	return tcu::TestStatus::pass("Passed");
}

TestInstance* BufferSparseResidencyCase::createInstance (Context& context) const
{
	return new BufferSparseResidencyInstance(context, m_bufferSize);
}

} // anonymous ns

void addBufferSparseResidencyTests(tcu::TestCaseGroup* group)
{
	group->addChild(new BufferSparseResidencyCase(group->getTestContext(), "buffer_size_2_10", "", 1 << 10, glu::GLSL_VERSION_440));
	group->addChild(new BufferSparseResidencyCase(group->getTestContext(), "buffer_size_2_12", "", 1 << 12, glu::GLSL_VERSION_440));
	group->addChild(new BufferSparseResidencyCase(group->getTestContext(), "buffer_size_2_16", "", 1 << 16, glu::GLSL_VERSION_440));
	group->addChild(new BufferSparseResidencyCase(group->getTestContext(), "buffer_size_2_17", "", 1 << 17, glu::GLSL_VERSION_440));
	group->addChild(new BufferSparseResidencyCase(group->getTestContext(), "buffer_size_2_20", "", 1 << 20, glu::GLSL_VERSION_440));
	group->addChild(new BufferSparseResidencyCase(group->getTestContext(), "buffer_size_2_24", "", 1 << 24, glu::GLSL_VERSION_440));
}

} // sparse
} // vkt
