/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Compute Shader Tests
 *//*--------------------------------------------------------------------*/

#include "vktComputeBasicComputeShaderTests.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktComputeTestsUtil.hpp"

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
#include "deRandom.hpp"

#include <vector>

using namespace vk;

namespace vkt
{
namespace compute
{
namespace
{

template<typename T, int size>
T multiplyComponents (const tcu::Vector<T, size>& v)
{
	T accum = 1;
	for (int i = 0; i < size; ++i)
		accum *= v[i];
	return accum;
}

template<typename T>
inline T squared (const T& a)
{
	return a * a;
}

inline VkImageCreateInfo make2DImageCreateInfo (const tcu::IVec2& imageSize, const VkImageUsageFlags usage)
{
	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,				// VkStructureType			sType;
		DE_NULL,											// const void*				pNext;
		0u,													// VkImageCreateFlags		flags;
		VK_IMAGE_TYPE_2D,									// VkImageType				imageType;
		VK_FORMAT_R32_UINT,									// VkFormat					format;
		vk::makeExtent3D(imageSize.x(), imageSize.y(), 1),	// VkExtent3D				extent;
		1u,													// deUint32					mipLevels;
		1u,													// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,							// VkImageTiling			tiling;
		usage,												// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,							// VkSharingMode			sharingMode;
		0u,													// deUint32					queueFamilyIndexCount;
		DE_NULL,											// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			initialLayout;
	};
	return imageParams;
}

inline VkBufferImageCopy makeBufferImageCopy(const tcu::IVec2& imageSize)
{
	return compute::makeBufferImageCopy(vk::makeExtent3D(imageSize.x(), imageSize.y(), 1), 1u);
}

enum BufferType
{
	BUFFER_TYPE_UNIFORM,
	BUFFER_TYPE_SSBO,
};

class SharedVarTest : public vkt::TestCase
{
public:
						SharedVarTest	(tcu::TestContext&		testCtx,
										 const std::string&		name,
										 const std::string&		description,
										 const tcu::IVec3&		localSize,
										 const tcu::IVec3&		workSize);

	void				initPrograms	(SourceCollections&		sourceCollections) const;
	TestInstance*		createInstance	(Context&				context) const;

private:
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class SharedVarTestInstance : public vkt::TestInstance
{
public:
									SharedVarTestInstance	(Context&			context,
															 const tcu::IVec3&	localSize,
															 const tcu::IVec3&	workSize);

	tcu::TestStatus					iterate					(void);

private:
	const tcu::IVec3				m_localSize;
	const tcu::IVec3				m_workSize;
};

SharedVarTest::SharedVarTest (tcu::TestContext&		testCtx,
							  const std::string&	name,
							  const std::string&	description,
							  const tcu::IVec3&		localSize,
							  const tcu::IVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

void SharedVarTest::initPrograms (SourceCollections& sourceCollections) const
{
	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);
	const int numValues = workGroupSize * workGroupCount;

	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) writeonly buffer Output {\n"
		<< "    uint values[" << numValues << "];\n"
		<< "} sb_out;\n\n"
		<< "shared uint offsets[" << workGroupSize << "];\n\n"
		<< "void main (void) {\n"
		<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
		<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		<< "    uint globalOffs = localSize*globalNdx;\n"
		<< "    uint localOffs  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_LocalInvocationID.z + gl_WorkGroupSize.x*gl_LocalInvocationID.y + gl_LocalInvocationID.x;\n"
		<< "\n"
		<< "    offsets[localSize-localOffs-1u] = globalOffs + localOffs*localOffs;\n"
		<< "    memoryBarrierShared();\n"
		<< "    barrier();\n"
		<< "    sb_out.values[globalOffs + localOffs] = offsets[localOffs];\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* SharedVarTest::createInstance (Context& context) const
{
	return new SharedVarTestInstance(context, m_localSize, m_workSize);
}

SharedVarTestInstance::SharedVarTestInstance (Context& context, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus SharedVarTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);

	// Create a buffer and host-visible memory for it

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * workGroupSize * workGroupCount;
	const Buffer buffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo descriptorInfo = makeDescriptorBufferInfo(*buffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &descriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0u));
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

	const VkBufferMemoryBarrier computeFinishBarrier = makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *buffer, 0ull, bufferSizeBytes);

	const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &computeFinishBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& bufferAllocation = buffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());

	for (int groupNdx = 0; groupNdx < workGroupCount; ++groupNdx)
	{
		const int globalOffset = groupNdx * workGroupSize;
		for (int localOffset = 0; localOffset < workGroupSize; ++localOffset)
		{
			const deUint32 res = bufferPtr[globalOffset + localOffset];
			const deUint32 ref = globalOffset + squared(workGroupSize - localOffset - 1);

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Output.values[" << (globalOffset + localOffset) << "]";
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class SharedVarAtomicOpTest : public vkt::TestCase
{
public:
						SharedVarAtomicOpTest	(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 const tcu::IVec3&	localSize,
												 const tcu::IVec3&	workSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context&			context) const;

private:
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class SharedVarAtomicOpTestInstance : public vkt::TestInstance
{
public:
									SharedVarAtomicOpTestInstance	(Context&			context,
																	 const tcu::IVec3&	localSize,
																	 const tcu::IVec3&	workSize);

	tcu::TestStatus					iterate							(void);

private:
	const tcu::IVec3				m_localSize;
	const tcu::IVec3				m_workSize;
};

SharedVarAtomicOpTest::SharedVarAtomicOpTest (tcu::TestContext&		testCtx,
											  const std::string&	name,
											  const std::string&	description,
											  const tcu::IVec3&		localSize,
											  const tcu::IVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

void SharedVarAtomicOpTest::initPrograms (SourceCollections& sourceCollections) const
{
	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);
	const int numValues = workGroupSize * workGroupCount;

	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) writeonly buffer Output {\n"
		<< "    uint values[" << numValues << "];\n"
		<< "} sb_out;\n\n"
		<< "shared uint count;\n\n"
		<< "void main (void) {\n"
		<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
		<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		<< "    uint globalOffs = localSize*globalNdx;\n"
		<< "\n"
		<< "    count = 0u;\n"
		<< "    memoryBarrierShared();\n"
		<< "    barrier();\n"
		<< "    uint oldVal = atomicAdd(count, 1u);\n"
		<< "    sb_out.values[globalOffs+oldVal] = oldVal+1u;\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* SharedVarAtomicOpTest::createInstance (Context& context) const
{
	return new SharedVarAtomicOpTestInstance(context, m_localSize, m_workSize);
}

SharedVarAtomicOpTestInstance::SharedVarAtomicOpTestInstance (Context& context, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus SharedVarAtomicOpTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);

	// Create a buffer and host-visible memory for it

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * workGroupSize * workGroupCount;
	const Buffer buffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo descriptorInfo = makeDescriptorBufferInfo(*buffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &descriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0u));
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

	const VkBufferMemoryBarrier computeFinishBarrier = makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *buffer, 0ull, bufferSizeBytes);

	const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1u, &computeFinishBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& bufferAllocation = buffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());

	for (int groupNdx = 0; groupNdx < workGroupCount; ++groupNdx)
	{
		const int globalOffset = groupNdx * workGroupSize;
		for (int localOffset = 0; localOffset < workGroupSize; ++localOffset)
		{
			const deUint32 res = bufferPtr[globalOffset + localOffset];
			const deUint32 ref = localOffset + 1;

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Output.values[" << (globalOffset + localOffset) << "]";
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class SSBOLocalBarrierTest : public vkt::TestCase
{
public:
						SSBOLocalBarrierTest	(tcu::TestContext&	testCtx,
												 const std::string& name,
												 const std::string&	description,
												 const tcu::IVec3&	localSize,
												 const tcu::IVec3&	workSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context&			context) const;

private:
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class SSBOLocalBarrierTestInstance : public vkt::TestInstance
{
public:
									SSBOLocalBarrierTestInstance	(Context&			context,
																	 const tcu::IVec3&	localSize,
																	 const tcu::IVec3&	workSize);

	tcu::TestStatus					iterate							(void);

private:
	const tcu::IVec3				m_localSize;
	const tcu::IVec3				m_workSize;
};

SSBOLocalBarrierTest::SSBOLocalBarrierTest (tcu::TestContext&	testCtx,
											const std::string&	name,
											const std::string&	description,
											const tcu::IVec3&	localSize,
											const tcu::IVec3&	workSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

void SSBOLocalBarrierTest::initPrograms (SourceCollections& sourceCollections) const
{
	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);
	const int numValues = workGroupSize * workGroupCount;

	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) coherent buffer Output {\n"
		<< "    uint values[" << numValues << "];\n"
		<< "} sb_out;\n\n"
		<< "void main (void) {\n"
		<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
		<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		<< "    uint globalOffs = localSize*globalNdx;\n"
		<< "    uint localOffs  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_LocalInvocationID.z + gl_WorkGroupSize.x*gl_LocalInvocationID.y + gl_LocalInvocationID.x;\n"
		<< "\n"
		<< "    sb_out.values[globalOffs + localOffs] = globalOffs;\n"
		<< "    memoryBarrierBuffer();\n"
		<< "    barrier();\n"
		<< "    sb_out.values[globalOffs + ((localOffs+1u)%localSize)] += localOffs;\n"		// += so we read and write
		<< "    memoryBarrierBuffer();\n"
		<< "    barrier();\n"
		<< "    sb_out.values[globalOffs + ((localOffs+2u)%localSize)] += localOffs;\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* SSBOLocalBarrierTest::createInstance (Context& context) const
{
	return new SSBOLocalBarrierTestInstance(context, m_localSize, m_workSize);
}

SSBOLocalBarrierTestInstance::SSBOLocalBarrierTestInstance (Context& context, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus SSBOLocalBarrierTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);

	// Create a buffer and host-visible memory for it

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * workGroupSize * workGroupCount;
	const Buffer buffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo descriptorInfo = makeDescriptorBufferInfo(*buffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &descriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0u));
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

	const VkBufferMemoryBarrier computeFinishBarrier = makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *buffer, 0ull, bufferSizeBytes);

	const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &computeFinishBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& bufferAllocation = buffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());

	for (int groupNdx = 0; groupNdx < workGroupCount; ++groupNdx)
	{
		const int globalOffset = groupNdx * workGroupSize;
		for (int localOffset = 0; localOffset < workGroupSize; ++localOffset)
		{
			const deUint32	res		= bufferPtr[globalOffset + localOffset];
			const int		offs0	= localOffset - 1 < 0 ? ((localOffset + workGroupSize - 1) % workGroupSize) : ((localOffset - 1) % workGroupSize);
			const int		offs1	= localOffset - 2 < 0 ? ((localOffset + workGroupSize - 2) % workGroupSize) : ((localOffset - 2) % workGroupSize);
			const deUint32	ref		= static_cast<deUint32>(globalOffset + offs0 + offs1);

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Output.values[" << (globalOffset + localOffset) << "]";
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class CopyImageToSSBOTest : public vkt::TestCase
{
public:
						CopyImageToSSBOTest		(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 const tcu::IVec2&	localSize,
												 const tcu::IVec2&	imageSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context&			context) const;

private:
	const tcu::IVec2	m_localSize;
	const tcu::IVec2	m_imageSize;
};

class CopyImageToSSBOTestInstance : public vkt::TestInstance
{
public:
									CopyImageToSSBOTestInstance		(Context&			context,
																	 const tcu::IVec2&	localSize,
																	 const tcu::IVec2&	imageSize);

	tcu::TestStatus					iterate							(void);

private:
	const tcu::IVec2				m_localSize;
	const tcu::IVec2				m_imageSize;
};

CopyImageToSSBOTest::CopyImageToSSBOTest (tcu::TestContext&		testCtx,
										  const std::string&	name,
										  const std::string&	description,
										  const tcu::IVec2&		localSize,
										  const tcu::IVec2&		imageSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
	DE_ASSERT(m_imageSize.x() % m_localSize.x() == 0);
	DE_ASSERT(m_imageSize.y() % m_localSize.y() == 0);
}

void CopyImageToSSBOTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ") in;\n"
		<< "layout(binding = 1, r32ui) readonly uniform highp uimage2D u_srcImg;\n"
		<< "layout(binding = 0) writeonly buffer Output {\n"
		<< "    uint values[" << (m_imageSize.x() * m_imageSize.y()) << "];\n"
		<< "} sb_out;\n\n"
		<< "void main (void) {\n"
		<< "    uint stride = gl_NumWorkGroups.x*gl_WorkGroupSize.x;\n"
		<< "    uint value  = imageLoad(u_srcImg, ivec2(gl_GlobalInvocationID.xy)).x;\n"
		<< "    sb_out.values[gl_GlobalInvocationID.y*stride + gl_GlobalInvocationID.x] = value;\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* CopyImageToSSBOTest::createInstance (Context& context) const
{
	return new CopyImageToSSBOTestInstance(context, m_localSize, m_imageSize);
}

CopyImageToSSBOTestInstance::CopyImageToSSBOTestInstance (Context& context, const tcu::IVec2& localSize, const tcu::IVec2& imageSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
}

tcu::TestStatus CopyImageToSSBOTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Create an image

	const VkImageCreateInfo imageParams = make2DImageCreateInfo(m_imageSize, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	const Image image(vk, device, allocator, imageParams, MemoryRequirement::Any);

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Unique<VkImageView> imageView(makeImageView(vk, device, *image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_UINT, subresourceRange));

	// Staging buffer (source data for image)

	const deUint32 imageArea = multiplyComponents(m_imageSize);
	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * imageArea;

	const Buffer stagingBuffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT), MemoryRequirement::HostVisible);

	// Populate the staging buffer with test data
	{
		de::Random rnd(0xab2c7);
		const Allocation& stagingBufferAllocation = stagingBuffer.getAllocation();
		deUint32* bufferPtr = static_cast<deUint32*>(stagingBufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < imageArea; ++i)
			*bufferPtr++ = rnd.getUint32();

		flushMappedMemoryRange(vk, device, stagingBufferAllocation.getMemory(), stagingBufferAllocation.getOffset(), bufferSizeBytes);
	}

	// Create a buffer to store shader output

	const Buffer outputBuffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	// Set the bindings

	const VkDescriptorImageInfo imageDescriptorInfo = makeDescriptorImageInfo(DE_NULL, *imageView, VK_IMAGE_LAYOUT_GENERAL);
	const VkDescriptorBufferInfo bufferDescriptorInfo = makeDescriptorBufferInfo(*outputBuffer, 0ull, bufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageDescriptorInfo)
		.update(vk, device);

	// Perform the computation
	{
		const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0u));
		const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
		const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

		const VkBufferMemoryBarrier stagingBufferPostHostWriteBarrier = makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, *stagingBuffer, 0ull, bufferSizeBytes);

		const VkImageMemoryBarrier imagePreCopyBarrier = makeImageMemoryBarrier(
			0u, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			*image, subresourceRange);

		const VkImageMemoryBarrier imagePostCopyBarrier = makeImageMemoryBarrier(
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
			*image, subresourceRange);

		const VkBufferMemoryBarrier computeFinishBarrier = makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *outputBuffer, 0ull, bufferSizeBytes);

		const VkBufferImageCopy copyParams = makeBufferImageCopy(m_imageSize);
		const tcu::IVec2 workSize = m_imageSize / m_localSize;

		// Prepare the command buffer

		const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
		const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

		// Start recording commands

		beginCommandBuffer(vk, *cmdBuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &stagingBufferPostHostWriteBarrier, 1, &imagePreCopyBarrier);
		vk.cmdCopyBufferToImage(*cmdBuffer, *stagingBuffer, *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyParams);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &imagePostCopyBarrier);

		vk.cmdDispatch(*cmdBuffer, workSize.x(), workSize.y(), 1u);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &computeFinishBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

		endCommandBuffer(vk, *cmdBuffer);

		// Wait for completion

		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32* refBufferPtr = static_cast<deUint32*>(stagingBuffer.getAllocation().getHostPtr());

	for (deUint32 ndx = 0; ndx < imageArea; ++ndx)
	{
		const deUint32 res = *(bufferPtr + ndx);
		const deUint32 ref = *(refBufferPtr + ndx);

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for Output.values[" << ndx << "]";
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class CopySSBOToImageTest : public vkt::TestCase
{
public:
						CopySSBOToImageTest	(tcu::TestContext&	testCtx,
											 const std::string&	name,
											 const std::string&	description,
											 const tcu::IVec2&	localSize,
											 const tcu::IVec2&	imageSize);

	void				initPrograms		(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance		(Context&			context) const;

private:
	const tcu::IVec2	m_localSize;
	const tcu::IVec2	m_imageSize;
};

class CopySSBOToImageTestInstance : public vkt::TestInstance
{
public:
									CopySSBOToImageTestInstance	(Context&			context,
																 const tcu::IVec2&	localSize,
																 const tcu::IVec2&	imageSize);

	tcu::TestStatus					iterate						(void);

private:
	const tcu::IVec2				m_localSize;
	const tcu::IVec2				m_imageSize;
};

CopySSBOToImageTest::CopySSBOToImageTest (tcu::TestContext&		testCtx,
										  const std::string&	name,
										  const std::string&	description,
										  const tcu::IVec2&		localSize,
										  const tcu::IVec2&		imageSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
	DE_ASSERT(m_imageSize.x() % m_localSize.x() == 0);
	DE_ASSERT(m_imageSize.y() % m_localSize.y() == 0);
}

void CopySSBOToImageTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ") in;\n"
		<< "layout(binding = 1, r32ui) writeonly uniform highp uimage2D u_dstImg;\n"
		<< "layout(binding = 0) readonly buffer Input {\n"
		<< "    uint values[" << (m_imageSize.x() * m_imageSize.y()) << "];\n"
		<< "} sb_in;\n\n"
		<< "void main (void) {\n"
		<< "    uint stride = gl_NumWorkGroups.x*gl_WorkGroupSize.x;\n"
		<< "    uint value  = sb_in.values[gl_GlobalInvocationID.y*stride + gl_GlobalInvocationID.x];\n"
		<< "    imageStore(u_dstImg, ivec2(gl_GlobalInvocationID.xy), uvec4(value, 0, 0, 0));\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* CopySSBOToImageTest::createInstance (Context& context) const
{
	return new CopySSBOToImageTestInstance(context, m_localSize, m_imageSize);
}

CopySSBOToImageTestInstance::CopySSBOToImageTestInstance (Context& context, const tcu::IVec2& localSize, const tcu::IVec2& imageSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
}

tcu::TestStatus CopySSBOToImageTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Create an image

	const VkImageCreateInfo imageParams = make2DImageCreateInfo(m_imageSize, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	const Image image(vk, device, allocator, imageParams, MemoryRequirement::Any);

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Unique<VkImageView> imageView(makeImageView(vk, device, *image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_UINT, subresourceRange));

	// Create an input buffer (data to be read in the shader)

	const deUint32 imageArea = multiplyComponents(m_imageSize);
	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * imageArea;

	const Buffer inputBuffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Populate the buffer with test data
	{
		de::Random rnd(0x77238ac2);
		const Allocation& inputBufferAllocation = inputBuffer.getAllocation();
		deUint32* bufferPtr = static_cast<deUint32*>(inputBufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < imageArea; ++i)
			*bufferPtr++ = rnd.getUint32();

		flushMappedMemoryRange(vk, device, inputBufferAllocation.getMemory(), inputBufferAllocation.getOffset(), bufferSizeBytes);
	}

	// Create a buffer to store shader output (copied from image data)

	const Buffer outputBuffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	// Set the bindings

	const VkDescriptorImageInfo imageDescriptorInfo = makeDescriptorImageInfo(DE_NULL, *imageView, VK_IMAGE_LAYOUT_GENERAL);
	const VkDescriptorBufferInfo bufferDescriptorInfo = makeDescriptorBufferInfo(*inputBuffer, 0ull, bufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageDescriptorInfo)
		.update(vk, device);

	// Perform the computation
	{
		const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0u));
		const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
		const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

		const VkBufferMemoryBarrier inputBufferPostHostWriteBarrier = makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, *inputBuffer, 0ull, bufferSizeBytes);

		const VkImageMemoryBarrier imageLayoutBarrier = makeImageMemoryBarrier(
			0u, 0u,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			*image, subresourceRange);

		const VkImageMemoryBarrier imagePreCopyBarrier = makeImageMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*image, subresourceRange);

		const VkBufferMemoryBarrier outputBufferPostCopyBarrier = makeBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *outputBuffer, 0ull, bufferSizeBytes);

		const VkBufferImageCopy copyParams = makeBufferImageCopy(m_imageSize);
		const tcu::IVec2 workSize = m_imageSize / m_localSize;

		// Prepare the command buffer

		const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
		const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

		// Start recording commands

		beginCommandBuffer(vk, *cmdBuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &inputBufferPostHostWriteBarrier, 1, &imageLayoutBarrier);
		vk.cmdDispatch(*cmdBuffer, workSize.x(), workSize.y(), 1u);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &imagePreCopyBarrier);
		vk.cmdCopyImageToBuffer(*cmdBuffer, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *outputBuffer, 1u, &copyParams);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &outputBufferPostCopyBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

		endCommandBuffer(vk, *cmdBuffer);

		// Wait for completion

		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32* refBufferPtr = static_cast<deUint32*>(inputBuffer.getAllocation().getHostPtr());

	for (deUint32 ndx = 0; ndx < imageArea; ++ndx)
	{
		const deUint32 res = *(bufferPtr + ndx);
		const deUint32 ref = *(refBufferPtr + ndx);

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for pixel " << ndx;
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class BufferToBufferInvertTest : public vkt::TestCase
{
public:
	void								initPrograms				(SourceCollections&	sourceCollections) const;
	TestInstance*						createInstance				(Context&			context) const;

	static BufferToBufferInvertTest*	UBOToSSBOInvertCase			(tcu::TestContext&	testCtx,
																	 const std::string& name,
																	 const std::string& description,
																	 const deUint32		numValues,
																	 const tcu::IVec3&	localSize,
																	 const tcu::IVec3&	workSize);

	static BufferToBufferInvertTest*	CopyInvertSSBOCase			(tcu::TestContext&	testCtx,
																	 const std::string& name,
																	 const std::string& description,
																	 const deUint32		numValues,
																	 const tcu::IVec3&	localSize,
																	 const tcu::IVec3&	workSize);

private:
										BufferToBufferInvertTest	(tcu::TestContext&	testCtx,
																	 const std::string& name,
																	 const std::string& description,
																	 const deUint32		numValues,
																	 const tcu::IVec3&	localSize,
																	 const tcu::IVec3&	workSize,
																	 const BufferType	bufferType);

	const BufferType					m_bufferType;
	const deUint32						m_numValues;
	const tcu::IVec3					m_localSize;
	const tcu::IVec3					m_workSize;
};

class BufferToBufferInvertTestInstance : public vkt::TestInstance
{
public:
									BufferToBufferInvertTestInstance	(Context&			context,
																		 const deUint32		numValues,
																		 const tcu::IVec3&	localSize,
																		 const tcu::IVec3&	workSize,
																		 const BufferType	bufferType);

	tcu::TestStatus					iterate								(void);

private:
	const BufferType				m_bufferType;
	const deUint32					m_numValues;
	const tcu::IVec3				m_localSize;
	const tcu::IVec3				m_workSize;
};

BufferToBufferInvertTest::BufferToBufferInvertTest (tcu::TestContext&	testCtx,
													const std::string&	name,
													const std::string&	description,
													const deUint32		numValues,
													const tcu::IVec3&	localSize,
													const tcu::IVec3&	workSize,
													const BufferType	bufferType)
	: TestCase		(testCtx, name, description)
	, m_bufferType	(bufferType)
	, m_numValues	(numValues)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
	DE_ASSERT(m_numValues % (multiplyComponents(m_workSize) * multiplyComponents(m_localSize)) == 0);
	DE_ASSERT(m_bufferType == BUFFER_TYPE_UNIFORM || m_bufferType == BUFFER_TYPE_SSBO);
}

BufferToBufferInvertTest* BufferToBufferInvertTest::UBOToSSBOInvertCase (tcu::TestContext&	testCtx,
																		 const std::string&	name,
																		 const std::string&	description,
																		 const deUint32		numValues,
																		 const tcu::IVec3&	localSize,
																		 const tcu::IVec3&	workSize)
{
	return new BufferToBufferInvertTest(testCtx, name, description, numValues, localSize, workSize, BUFFER_TYPE_UNIFORM);
}

BufferToBufferInvertTest* BufferToBufferInvertTest::CopyInvertSSBOCase (tcu::TestContext&	testCtx,
																		const std::string&	name,
																		const std::string&	description,
																		const deUint32		numValues,
																		const tcu::IVec3&	localSize,
																		const tcu::IVec3&	workSize)
{
	return new BufferToBufferInvertTest(testCtx, name, description, numValues, localSize, workSize, BUFFER_TYPE_SSBO);
}

void BufferToBufferInvertTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	if (m_bufferType == BUFFER_TYPE_UNIFORM)
	{
		src << "#version 310 es\n"
			<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
			<< "layout(binding = 0) readonly uniform Input {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} ub_in;\n"
			<< "layout(binding = 1, std140) writeonly buffer Output {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} sb_out;\n"
			<< "void main (void) {\n"
			<< "    uvec3 size           = gl_NumWorkGroups * gl_WorkGroupSize;\n"
			<< "    uint numValuesPerInv = uint(ub_in.values.length()) / (size.x*size.y*size.z);\n"
			<< "    uint groupNdx        = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
			<< "    uint offset          = numValuesPerInv*groupNdx;\n"
			<< "\n"
			<< "    for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
			<< "        sb_out.values[offset + ndx] = ~ub_in.values[offset + ndx];\n"
			<< "}\n";
	}
	else if (m_bufferType == BUFFER_TYPE_SSBO)
	{
		src << "#version 310 es\n"
			<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
			<< "layout(binding = 0, std140) readonly buffer Input {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} sb_in;\n"
			<< "layout (binding = 1, std140) writeonly buffer Output {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} sb_out;\n"
			<< "void main (void) {\n"
			<< "    uvec3 size           = gl_NumWorkGroups * gl_WorkGroupSize;\n"
			<< "    uint numValuesPerInv = uint(sb_in.values.length()) / (size.x*size.y*size.z);\n"
			<< "    uint groupNdx        = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
			<< "    uint offset          = numValuesPerInv*groupNdx;\n"
			<< "\n"
			<< "    for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
			<< "        sb_out.values[offset + ndx] = ~sb_in.values[offset + ndx];\n"
			<< "}\n";
	}

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* BufferToBufferInvertTest::createInstance (Context& context) const
{
	return new BufferToBufferInvertTestInstance(context, m_numValues, m_localSize, m_workSize, m_bufferType);
}

BufferToBufferInvertTestInstance::BufferToBufferInvertTestInstance (Context&			context,
																	const deUint32		numValues,
																	const tcu::IVec3&	localSize,
																	const tcu::IVec3&	workSize,
																	const BufferType	bufferType)
	: TestInstance	(context)
	, m_bufferType	(bufferType)
	, m_numValues	(numValues)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus BufferToBufferInvertTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Customize the test based on buffer type

	const VkBufferUsageFlags inputBufferUsageFlags		= (m_bufferType == BUFFER_TYPE_UNIFORM ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	const VkDescriptorType inputBufferDescriptorType	= (m_bufferType == BUFFER_TYPE_UNIFORM ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	const deUint32 randomSeed							= (m_bufferType == BUFFER_TYPE_UNIFORM ? 0x111223f : 0x124fef);

	// Create an input buffer

	const VkDeviceSize bufferSizeBytes = sizeof(tcu::UVec4) * m_numValues;
	const Buffer inputBuffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, inputBufferUsageFlags), MemoryRequirement::HostVisible);

	// Fill the input buffer with data
	{
		de::Random rnd(randomSeed);
		const Allocation& inputBufferAllocation = inputBuffer.getAllocation();
		tcu::UVec4* bufferPtr = static_cast<tcu::UVec4*>(inputBufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < m_numValues; ++i)
			bufferPtr[i].x() = rnd.getUint32();

		flushMappedMemoryRange(vk, device, inputBufferAllocation.getMemory(), inputBufferAllocation.getOffset(), bufferSizeBytes);
	}

	// Create an output buffer

	const Buffer outputBuffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(inputBufferDescriptorType, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(inputBufferDescriptorType)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo inputBufferDescriptorInfo = makeDescriptorBufferInfo(*inputBuffer, 0ull, bufferSizeBytes);
	const VkDescriptorBufferInfo outputBufferDescriptorInfo = makeDescriptorBufferInfo(*outputBuffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), inputBufferDescriptorType, &inputBufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &outputBufferDescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0u));
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

	const VkBufferMemoryBarrier hostWriteBarrier = makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, *inputBuffer, 0ull, bufferSizeBytes);

	const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *outputBuffer, 0ull, bufferSizeBytes);

	const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &hostWriteBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &shaderWriteBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), bufferSizeBytes);

	const tcu::UVec4* bufferPtr = static_cast<tcu::UVec4*>(outputBufferAllocation.getHostPtr());
	const tcu::UVec4* refBufferPtr = static_cast<tcu::UVec4*>(inputBuffer.getAllocation().getHostPtr());

	for (deUint32 ndx = 0; ndx < m_numValues; ++ndx)
	{
		const deUint32 res = bufferPtr[ndx].x();
		const deUint32 ref = ~refBufferPtr[ndx].x();

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for Output.values[" << ndx << "]";
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class InvertSSBOInPlaceTest : public vkt::TestCase
{
public:
						InvertSSBOInPlaceTest	(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 const deUint32		numValues,
												 const bool			sized,
												 const tcu::IVec3&	localSize,
												 const tcu::IVec3&	workSize);


	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context&			context) const;

private:
	const deUint32		m_numValues;
	const bool			m_sized;
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class InvertSSBOInPlaceTestInstance : public vkt::TestInstance
{
public:
									InvertSSBOInPlaceTestInstance	(Context&			context,
																	 const deUint32		numValues,
																	 const tcu::IVec3&	localSize,
																	 const tcu::IVec3&	workSize);

	tcu::TestStatus					iterate							(void);

private:
	const deUint32					m_numValues;
	const tcu::IVec3				m_localSize;
	const tcu::IVec3				m_workSize;
};

InvertSSBOInPlaceTest::InvertSSBOInPlaceTest (tcu::TestContext&		testCtx,
											  const std::string&	name,
											  const std::string&	description,
											  const deUint32		numValues,
											  const bool			sized,
											  const tcu::IVec3&		localSize,
											  const tcu::IVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_numValues	(numValues)
	, m_sized		(sized)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
	DE_ASSERT(m_numValues % (multiplyComponents(m_workSize) * multiplyComponents(m_localSize)) == 0);
}

void InvertSSBOInPlaceTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) buffer InOut {\n"
		<< "    uint values[" << (m_sized ? de::toString(m_numValues) : "") << "];\n"
		<< "} sb_inout;\n"
		<< "void main (void) {\n"
		<< "    uvec3 size           = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "    uint numValuesPerInv = uint(sb_inout.values.length()) / (size.x*size.y*size.z);\n"
		<< "    uint groupNdx        = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
		<< "    uint offset          = numValuesPerInv*groupNdx;\n"
		<< "\n"
		<< "    for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
		<< "        sb_inout.values[offset + ndx] = ~sb_inout.values[offset + ndx];\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* InvertSSBOInPlaceTest::createInstance (Context& context) const
{
	return new InvertSSBOInPlaceTestInstance(context, m_numValues, m_localSize, m_workSize);
}

InvertSSBOInPlaceTestInstance::InvertSSBOInPlaceTestInstance (Context&			context,
															  const deUint32	numValues,
															  const tcu::IVec3&	localSize,
															  const tcu::IVec3&	workSize)
	: TestInstance	(context)
	, m_numValues	(numValues)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus InvertSSBOInPlaceTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Create an input/output buffer

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * m_numValues;
	const Buffer buffer(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Fill the buffer with data

	typedef std::vector<deUint32> data_vector_t;
	data_vector_t inputData(m_numValues);

	{
		de::Random rnd(0x82ce7f);
		const Allocation& bufferAllocation = buffer.getAllocation();
		deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < m_numValues; ++i)
			inputData[i] = *bufferPtr++ = rnd.getUint32();

		flushMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);
	}

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo bufferDescriptorInfo = makeDescriptorBufferInfo(*buffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferDescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0u));
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

	const VkBufferMemoryBarrier hostWriteBarrier = makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, *buffer, 0ull, bufferSizeBytes);

	const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *buffer, 0ull, bufferSizeBytes);

	const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &hostWriteBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &shaderWriteBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& bufferAllocation = buffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());

	for (deUint32 ndx = 0; ndx < m_numValues; ++ndx)
	{
		const deUint32 res = bufferPtr[ndx];
		const deUint32 ref = ~inputData[ndx];

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for InOut.values[" << ndx << "]";
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class WriteToMultipleSSBOTest : public vkt::TestCase
{
public:
						WriteToMultipleSSBOTest	(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 const deUint32		numValues,
												 const bool			sized,
												 const tcu::IVec3&	localSize,
												 const tcu::IVec3&	workSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context&			context) const;

private:
	const deUint32		m_numValues;
	const bool			m_sized;
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class WriteToMultipleSSBOTestInstance : public vkt::TestInstance
{
public:
									WriteToMultipleSSBOTestInstance	(Context&			context,
																	 const deUint32		numValues,
																	 const tcu::IVec3&	localSize,
																	 const tcu::IVec3&	workSize);

	tcu::TestStatus					iterate							(void);

private:
	const deUint32					m_numValues;
	const tcu::IVec3				m_localSize;
	const tcu::IVec3				m_workSize;
};

WriteToMultipleSSBOTest::WriteToMultipleSSBOTest (tcu::TestContext&		testCtx,
												  const std::string&	name,
												  const std::string&	description,
												  const deUint32		numValues,
												  const bool			sized,
												  const tcu::IVec3&		localSize,
												  const tcu::IVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_numValues	(numValues)
	, m_sized		(sized)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
	DE_ASSERT(m_numValues % (multiplyComponents(m_workSize) * multiplyComponents(m_localSize)) == 0);
}

void WriteToMultipleSSBOTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) writeonly buffer Out0 {\n"
		<< "    uint values[" << (m_sized ? de::toString(m_numValues) : "") << "];\n"
		<< "} sb_out0;\n"
		<< "layout(binding = 1) writeonly buffer Out1 {\n"
		<< "    uint values[" << (m_sized ? de::toString(m_numValues) : "") << "];\n"
		<< "} sb_out1;\n"
		<< "void main (void) {\n"
		<< "    uvec3 size      = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "    uint groupNdx   = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
		<< "\n"
		<< "    {\n"
		<< "        uint numValuesPerInv = uint(sb_out0.values.length()) / (size.x*size.y*size.z);\n"
		<< "        uint offset          = numValuesPerInv*groupNdx;\n"
		<< "\n"
		<< "        for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
		<< "            sb_out0.values[offset + ndx] = offset + ndx;\n"
		<< "    }\n"
		<< "    {\n"
		<< "        uint numValuesPerInv = uint(sb_out1.values.length()) / (size.x*size.y*size.z);\n"
		<< "        uint offset          = numValuesPerInv*groupNdx;\n"
		<< "\n"
		<< "        for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
		<< "            sb_out1.values[offset + ndx] = uint(sb_out1.values.length()) - offset - ndx;\n"
		<< "    }\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* WriteToMultipleSSBOTest::createInstance (Context& context) const
{
	return new WriteToMultipleSSBOTestInstance(context, m_numValues, m_localSize, m_workSize);
}

WriteToMultipleSSBOTestInstance::WriteToMultipleSSBOTestInstance (Context&			context,
																  const deUint32	numValues,
																  const tcu::IVec3&	localSize,
																  const tcu::IVec3&	workSize)
	: TestInstance	(context)
	, m_numValues	(numValues)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus WriteToMultipleSSBOTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Create two output buffers

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * m_numValues;
	const Buffer buffer0(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);
	const Buffer buffer1(vk, device, allocator, makeBufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo buffer0DescriptorInfo = makeDescriptorBufferInfo(*buffer0, 0ull, bufferSizeBytes);
	const VkDescriptorBufferInfo buffer1DescriptorInfo = makeDescriptorBufferInfo(*buffer1, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &buffer0DescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &buffer1DescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0u));
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

	const VkBufferMemoryBarrier shaderWriteBarriers[] =
	{
		makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *buffer0, 0ull, bufferSizeBytes),
		makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *buffer1, 0ull, bufferSizeBytes)
	};

	const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, DE_LENGTH_OF_ARRAY(shaderWriteBarriers), shaderWriteBarriers, 0, (const VkImageMemoryBarrier*)DE_NULL);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results
	{
		const Allocation& buffer0Allocation = buffer0.getAllocation();
		invalidateMappedMemoryRange(vk, device, buffer0Allocation.getMemory(), buffer0Allocation.getOffset(), bufferSizeBytes);
		const deUint32* buffer0Ptr = static_cast<deUint32*>(buffer0Allocation.getHostPtr());

		for (deUint32 ndx = 0; ndx < m_numValues; ++ndx)
		{
			const deUint32 res = buffer0Ptr[ndx];
			const deUint32 ref = ndx;

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Out0.values[" << ndx << "] res=" << res << " ref=" << ref;
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	{
		const Allocation& buffer1Allocation = buffer1.getAllocation();
		invalidateMappedMemoryRange(vk, device, buffer1Allocation.getMemory(), buffer1Allocation.getOffset(), bufferSizeBytes);
		const deUint32* buffer1Ptr = static_cast<deUint32*>(buffer1Allocation.getHostPtr());

		for (deUint32 ndx = 0; ndx < m_numValues; ++ndx)
		{
			const deUint32 res = buffer1Ptr[ndx];
			const deUint32 ref = m_numValues - ndx;

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Out1.values[" << ndx << "] res=" << res << " ref=" << ref;
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class SSBOBarrierTest : public vkt::TestCase
{
public:
						SSBOBarrierTest		(tcu::TestContext&	testCtx,
											 const std::string&	name,
											 const std::string&	description,
											 const tcu::IVec3&	workSize);

	void				initPrograms		(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance		(Context&			context) const;

private:
	const tcu::IVec3	m_workSize;
};

class SSBOBarrierTestInstance : public vkt::TestInstance
{
public:
									SSBOBarrierTestInstance		(Context&			context,
																 const tcu::IVec3&	workSize);

	tcu::TestStatus					iterate						(void);

private:
	const tcu::IVec3				m_workSize;
};

SSBOBarrierTest::SSBOBarrierTest (tcu::TestContext&		testCtx,
								  const std::string&	name,
								  const std::string&	description,
								  const tcu::IVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_workSize	(workSize)
{
}

void SSBOBarrierTest::initPrograms (SourceCollections& sourceCollections) const
{
	sourceCollections.glslSources.add("comp0") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"layout(binding = 2) readonly uniform Constants {\n"
		"    uint u_baseVal;\n"
		"};\n"
		"layout(binding = 1) writeonly buffer Output {\n"
		"    uint values[];\n"
		"};\n"
		"void main (void) {\n"
		"    uint offset = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		"    values[offset] = u_baseVal + offset;\n"
		"}\n");

	sourceCollections.glslSources.add("comp1") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"layout(binding = 1) readonly buffer Input {\n"
		"    uint values[];\n"
		"};\n"
		"layout(binding = 0) coherent buffer Output {\n"
		"    uint sum;\n"
		"};\n"
		"void main (void) {\n"
		"    uint offset = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		"    uint value  = values[offset];\n"
		"    atomicAdd(sum, value);\n"
		"}\n");
}

TestInstance* SSBOBarrierTest::createInstance (Context& context) const
{
	return new SSBOBarrierTestInstance(context, m_workSize);
}

SSBOBarrierTestInstance::SSBOBarrierTestInstance (Context& context, const tcu::IVec3& workSize)
	: TestInstance	(context)
	, m_workSize	(workSize)
{
}

tcu::TestStatus SSBOBarrierTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Create a work buffer used by both shaders

	const int workGroupCount = multiplyComponents(m_workSize);
	const VkDeviceSize workBufferSizeBytes = sizeof(deUint32) * workGroupCount;
	const Buffer workBuffer(vk, device, allocator, makeBufferCreateInfo(workBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::Any);

	// Create an output buffer

	const VkDeviceSize outputBufferSizeBytes = sizeof(deUint32);
	const Buffer outputBuffer(vk, device, allocator, makeBufferCreateInfo(outputBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Initialize atomic counter value to zero
	{
		const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
		deUint32* outputBufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
		*outputBufferPtr = 0;
		flushMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), outputBufferSizeBytes);
	}

	// Create a uniform buffer (to pass uniform constants)

	const VkDeviceSize uniformBufferSizeBytes = sizeof(deUint32);
	const Buffer uniformBuffer(vk, device, allocator, makeBufferCreateInfo(uniformBufferSizeBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Set the constants in the uniform buffer

	const deUint32	baseValue = 127;
	{
		const Allocation& uniformBufferAllocation = uniformBuffer.getAllocation();
		deUint32* uniformBufferPtr = static_cast<deUint32*>(uniformBufferAllocation.getHostPtr());
		uniformBufferPtr[0] = baseValue;

		flushMappedMemoryRange(vk, device, uniformBufferAllocation.getMemory(), uniformBufferAllocation.getOffset(), uniformBufferSizeBytes);
	}

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u)
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo workBufferDescriptorInfo = makeDescriptorBufferInfo(*workBuffer, 0ull, workBufferSizeBytes);
	const VkDescriptorBufferInfo outputBufferDescriptorInfo = makeDescriptorBufferInfo(*outputBuffer, 0ull, outputBufferSizeBytes);
	const VkDescriptorBufferInfo uniformBufferDescriptorInfo = makeDescriptorBufferInfo(*uniformBuffer, 0ull, uniformBufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &outputBufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &workBufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(2u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uniformBufferDescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkShaderModule> shaderModule0(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp0"), 0));
	const Unique<VkShaderModule> shaderModule1(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp1"), 0));

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline> pipeline0(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule0));
	const Unique<VkPipeline> pipeline1(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule1));

	const VkBufferMemoryBarrier writeUniformConstantsBarrier = makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT, *uniformBuffer, 0ull, uniformBufferSizeBytes);

	const VkBufferMemoryBarrier betweenShadersBarrier = makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, *workBuffer, 0ull, workBufferSizeBytes);

	const VkBufferMemoryBarrier afterComputeBarrier = makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *outputBuffer, 0ull, outputBufferSizeBytes);

	const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline0);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &writeUniformConstantsBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &betweenShadersBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

	// Switch to the second shader program
	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline1);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &afterComputeBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), outputBufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32	res = *bufferPtr;
	deUint32		ref = 0;

	for (int ndx = 0; ndx < workGroupCount; ++ndx)
		ref += baseValue + ndx;

	if (res != ref)
	{
		std::ostringstream msg;
		msg << "ERROR: comparison failed, expected " << ref << ", got " << res;
		return tcu::TestStatus::fail(msg.str());
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class ImageAtomicOpTest : public vkt::TestCase
{
public:
						ImageAtomicOpTest		(tcu::TestContext&	testCtx,
												 const std::string& name,
												 const std::string& description,
												 const deUint32		localSize,
												 const tcu::IVec2&	imageSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context&			context) const;

private:
	const deUint32		m_localSize;
	const tcu::IVec2	m_imageSize;
};

class ImageAtomicOpTestInstance : public vkt::TestInstance
{
public:
									ImageAtomicOpTestInstance		(Context&			context,
																	 const deUint32		localSize,
																	 const tcu::IVec2&	imageSize);

	tcu::TestStatus					iterate							(void);

private:
	const deUint32					m_localSize;
	const tcu::IVec2				m_imageSize;
};

ImageAtomicOpTest::ImageAtomicOpTest (tcu::TestContext&		testCtx,
									  const std::string&	name,
									  const std::string&	description,
									  const deUint32		localSize,
									  const tcu::IVec2&		imageSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
}

void ImageAtomicOpTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "#extension GL_OES_shader_image_atomic : require\n"
		<< "layout (local_size_x = " << m_localSize << ") in;\n"
		<< "layout(binding = 1, r32ui) coherent uniform highp uimage2D u_dstImg;\n"
		<< "layout(binding = 0) readonly buffer Input {\n"
		<< "    uint values[" << (multiplyComponents(m_imageSize) * m_localSize) << "];\n"
		<< "} sb_in;\n\n"
		<< "void main (void) {\n"
		<< "    uint stride = gl_NumWorkGroups.x*gl_WorkGroupSize.x;\n"
		<< "    uint value  = sb_in.values[gl_GlobalInvocationID.y*stride + gl_GlobalInvocationID.x];\n"
		<< "\n"
		<< "    if (gl_LocalInvocationIndex == 0u)\n"
		<< "        imageStore(u_dstImg, ivec2(gl_WorkGroupID.xy), uvec4(0));\n"
		<< "    memoryBarrierImage();\n"
		<< "    barrier();\n"
		<< "    imageAtomicAdd(u_dstImg, ivec2(gl_WorkGroupID.xy), value);\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* ImageAtomicOpTest::createInstance (Context& context) const
{
	return new ImageAtomicOpTestInstance(context, m_localSize, m_imageSize);
}

ImageAtomicOpTestInstance::ImageAtomicOpTestInstance (Context& context, const deUint32 localSize, const tcu::IVec2& imageSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
}

tcu::TestStatus ImageAtomicOpTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Create an image

	const VkImageCreateInfo imageParams = make2DImageCreateInfo(m_imageSize, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	const Image image(vk, device, allocator, imageParams, MemoryRequirement::Any);

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Unique<VkImageView> imageView(makeImageView(vk, device, *image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_UINT, subresourceRange));

	// Input buffer

	const deUint32 numInputValues = multiplyComponents(m_imageSize) * m_localSize;
	const VkDeviceSize inputBufferSizeBytes = sizeof(deUint32) * numInputValues;

	const Buffer inputBuffer(vk, device, allocator, makeBufferCreateInfo(inputBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Populate the input buffer with test data
	{
		de::Random rnd(0x77238ac2);
		const Allocation& inputBufferAllocation = inputBuffer.getAllocation();
		deUint32* bufferPtr = static_cast<deUint32*>(inputBufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < numInputValues; ++i)
			*bufferPtr++ = rnd.getUint32();

		flushMappedMemoryRange(vk, device, inputBufferAllocation.getMemory(), inputBufferAllocation.getOffset(), inputBufferSizeBytes);
	}

	// Create a buffer to store shader output (copied from image data)

	const deUint32 imageArea = multiplyComponents(m_imageSize);
	const VkDeviceSize outputBufferSizeBytes = sizeof(deUint32) * imageArea;
	const Buffer outputBuffer(vk, device, allocator, makeBufferCreateInfo(outputBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	// Set the bindings

	const VkDescriptorImageInfo imageDescriptorInfo = makeDescriptorImageInfo(DE_NULL, *imageView, VK_IMAGE_LAYOUT_GENERAL);
	const VkDescriptorBufferInfo bufferDescriptorInfo = makeDescriptorBufferInfo(*inputBuffer, 0ull, inputBufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageDescriptorInfo)
		.update(vk, device);

	// Perform the computation
	{
		const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0u));
		const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
		const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

		const VkBufferMemoryBarrier inputBufferPostHostWriteBarrier = makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, *inputBuffer, 0ull, inputBufferSizeBytes);

		const VkImageMemoryBarrier imageLayoutBarrier = makeImageMemoryBarrier(
			(VkAccessFlags)0, VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			*image, subresourceRange);

		const VkImageMemoryBarrier imagePreCopyBarrier = makeImageMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*image, subresourceRange);

		const VkBufferMemoryBarrier outputBufferPostCopyBarrier = makeBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *outputBuffer, 0ull, outputBufferSizeBytes);

		const VkBufferImageCopy copyParams = makeBufferImageCopy(m_imageSize);

		// Prepare the command buffer

		const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
		const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

		// Start recording commands

		beginCommandBuffer(vk, *cmdBuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &inputBufferPostHostWriteBarrier, 1, &imageLayoutBarrier);
		vk.cmdDispatch(*cmdBuffer, m_imageSize.x(), m_imageSize.y(), 1u);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &imagePreCopyBarrier);
		vk.cmdCopyImageToBuffer(*cmdBuffer, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *outputBuffer, 1u, &copyParams);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &outputBufferPostCopyBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

		endCommandBuffer(vk, *cmdBuffer);

		// Wait for completion

		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), outputBufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32* refBufferPtr = static_cast<deUint32*>(inputBuffer.getAllocation().getHostPtr());

	for (deUint32 pixelNdx = 0; pixelNdx < imageArea; ++pixelNdx)
	{
		const deUint32	res = bufferPtr[pixelNdx];
		deUint32		ref = 0;

		for (deUint32 offs = 0; offs < m_localSize; ++offs)
			ref += refBufferPtr[pixelNdx * m_localSize + offs];

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for pixel " << pixelNdx;
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class ImageBarrierTest : public vkt::TestCase
{
public:
						ImageBarrierTest	(tcu::TestContext&	testCtx,
											const std::string&	name,
											const std::string&	description,
											const tcu::IVec2&	imageSize);

	void				initPrograms		(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance		(Context&			context) const;

private:
	const tcu::IVec2	m_imageSize;
};

class ImageBarrierTestInstance : public vkt::TestInstance
{
public:
									ImageBarrierTestInstance	(Context&			context,
																 const tcu::IVec2&	imageSize);

	tcu::TestStatus					iterate						(void);

private:
	const tcu::IVec2				m_imageSize;
};

ImageBarrierTest::ImageBarrierTest (tcu::TestContext&	testCtx,
									const std::string&	name,
									const std::string&	description,
									const tcu::IVec2&	imageSize)
	: TestCase		(testCtx, name, description)
	, m_imageSize	(imageSize)
{
}

void ImageBarrierTest::initPrograms (SourceCollections& sourceCollections) const
{
	sourceCollections.glslSources.add("comp0") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"layout(binding = 2) readonly uniform Constants {\n"
		"    uint u_baseVal;\n"
		"};\n"
		"layout(binding = 1, r32ui) writeonly uniform highp uimage2D u_img;\n"
		"void main (void) {\n"
		"    uint offset = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		"    imageStore(u_img, ivec2(gl_WorkGroupID.xy), uvec4(offset + u_baseVal, 0, 0, 0));\n"
		"}\n");

	sourceCollections.glslSources.add("comp1") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"layout(binding = 1, r32ui) readonly uniform highp uimage2D u_img;\n"
		"layout(binding = 0) coherent buffer Output {\n"
		"    uint sum;\n"
		"};\n"
		"void main (void) {\n"
		"    uint value = imageLoad(u_img, ivec2(gl_WorkGroupID.xy)).x;\n"
		"    atomicAdd(sum, value);\n"
		"}\n");
}

TestInstance* ImageBarrierTest::createInstance (Context& context) const
{
	return new ImageBarrierTestInstance(context, m_imageSize);
}

ImageBarrierTestInstance::ImageBarrierTestInstance (Context& context, const tcu::IVec2& imageSize)
	: TestInstance	(context)
	, m_imageSize	(imageSize)
{
}

tcu::TestStatus ImageBarrierTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Create an image used by both shaders

	const VkImageCreateInfo imageParams = make2DImageCreateInfo(m_imageSize, VK_IMAGE_USAGE_STORAGE_BIT);
	const Image image(vk, device, allocator, imageParams, MemoryRequirement::Any);

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Unique<VkImageView> imageView(makeImageView(vk, device, *image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_UINT, subresourceRange));

	// Create an output buffer

	const VkDeviceSize outputBufferSizeBytes = sizeof(deUint32);
	const Buffer outputBuffer(vk, device, allocator, makeBufferCreateInfo(outputBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Initialize atomic counter value to zero
	{
		const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
		deUint32* outputBufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
		*outputBufferPtr = 0;
		flushMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), outputBufferSizeBytes);
	}

	// Create a uniform buffer (to pass uniform constants)

	const VkDeviceSize uniformBufferSizeBytes = sizeof(deUint32);
	const Buffer uniformBuffer(vk, device, allocator, makeBufferCreateInfo(uniformBufferSizeBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Set the constants in the uniform buffer

	const deUint32	baseValue = 127;
	{
		const Allocation& uniformBufferAllocation = uniformBuffer.getAllocation();
		deUint32* uniformBufferPtr = static_cast<deUint32*>(uniformBufferAllocation.getHostPtr());
		uniformBufferPtr[0] = baseValue;

		flushMappedMemoryRange(vk, device, uniformBufferAllocation.getMemory(), uniformBufferAllocation.getOffset(), uniformBufferSizeBytes);
	}

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorImageInfo imageDescriptorInfo = makeDescriptorImageInfo(DE_NULL, *imageView, VK_IMAGE_LAYOUT_GENERAL);
	const VkDescriptorBufferInfo outputBufferDescriptorInfo = makeDescriptorBufferInfo(*outputBuffer, 0ull, outputBufferSizeBytes);
	const VkDescriptorBufferInfo uniformBufferDescriptorInfo = makeDescriptorBufferInfo(*uniformBuffer, 0ull, uniformBufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &outputBufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(2u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uniformBufferDescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkShaderModule>	shaderModule0(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp0"), 0));
	const Unique<VkShaderModule>	shaderModule1(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp1"), 0));

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkPipeline> pipeline0(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule0));
	const Unique<VkPipeline> pipeline1(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule1));

	const VkBufferMemoryBarrier writeUniformConstantsBarrier = makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT, *uniformBuffer, 0ull, uniformBufferSizeBytes);

	const VkImageMemoryBarrier imageLayoutBarrier = makeImageMemoryBarrier(
		0u, 0u,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		*image, subresourceRange);

	const VkImageMemoryBarrier imageBarrierBetweenShaders = makeImageMemoryBarrier(
		VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		*image, subresourceRange);

	const VkBufferMemoryBarrier afterComputeBarrier = makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *outputBuffer, 0ull, outputBufferSizeBytes);

	const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline0);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &writeUniformConstantsBarrier, 1, &imageLayoutBarrier);

	vk.cmdDispatch(*cmdBuffer, m_imageSize.x(), m_imageSize.y(), 1u);
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &imageBarrierBetweenShaders);

	// Switch to the second shader program
	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline1);

	vk.cmdDispatch(*cmdBuffer, m_imageSize.x(), m_imageSize.y(), 1u);
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &afterComputeBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), outputBufferSizeBytes);

	const int		numValues = multiplyComponents(m_imageSize);
	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32	res = *bufferPtr;
	deUint32		ref = 0;

	for (int ndx = 0; ndx < numValues; ++ndx)
		ref += baseValue + ndx;

	if (res != ref)
	{
		std::ostringstream msg;
		msg << "ERROR: comparison failed, expected " << ref << ", got " << res;
		return tcu::TestStatus::fail(msg.str());
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

namespace EmptyShaderTest
{

void createProgram (SourceCollections& dst)
{
	dst.glslSources.add("comp") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"void main (void) {}\n"
	);
}

tcu::TestStatus createTest (Context& context)
{
	const DeviceInterface&	vk					= context.getDeviceInterface();
	const VkDevice			device				= context.getDevice();
	const VkQueue			queue				= context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= context.getUniversalQueueFamilyIndex();

	const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, context.getBinaryCollection().get("comp"), 0u));

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

	const Unique<VkCommandPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);

	const tcu::IVec3 workGroups(1, 1, 1);
	vk.cmdDispatch(*cmdBuffer, workGroups.x(), workGroups.y(), workGroups.z());

	endCommandBuffer(vk, *cmdBuffer);

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	return tcu::TestStatus::pass("Compute succeeded");
}

} // EmptyShaderTest ns
} // anonymous

tcu::TestCaseGroup* createBasicComputeShaderTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> basicComputeTests(new tcu::TestCaseGroup(testCtx, "basic", "Basic compute tests"));

	addFunctionCaseWithPrograms(basicComputeTests.get(), "empty_shader", "Shader that does nothing", EmptyShaderTest::createProgram, EmptyShaderTest::createTest);

	basicComputeTests->addChild(BufferToBufferInvertTest::UBOToSSBOInvertCase(testCtx,	"ubo_to_ssbo_single_invocation",	"Copy from UBO to SSBO, inverting bits",	256,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::UBOToSSBOInvertCase(testCtx,	"ubo_to_ssbo_single_group",			"Copy from UBO to SSBO, inverting bits",	1024,	tcu::IVec3(2,1,4),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::UBOToSSBOInvertCase(testCtx,	"ubo_to_ssbo_multiple_invocations",	"Copy from UBO to SSBO, inverting bits",	1024,	tcu::IVec3(1,1,1),	tcu::IVec3(2,4,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::UBOToSSBOInvertCase(testCtx,	"ubo_to_ssbo_multiple_groups",		"Copy from UBO to SSBO, inverting bits",	1024,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	basicComputeTests->addChild(BufferToBufferInvertTest::CopyInvertSSBOCase(testCtx,	"copy_ssbo_single_invocation",		"Copy between SSBOs, inverting bits",	256,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::CopyInvertSSBOCase(testCtx,	"copy_ssbo_multiple_invocations",	"Copy between SSBOs, inverting bits",	1024,	tcu::IVec3(1,1,1),	tcu::IVec3(2,4,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::CopyInvertSSBOCase(testCtx,	"copy_ssbo_multiple_groups",		"Copy between SSBOs, inverting bits",	1024,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	basicComputeTests->addChild(new InvertSSBOInPlaceTest(testCtx,	"ssbo_rw_single_invocation",			"Read and write same SSBO",		256,	true,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new InvertSSBOInPlaceTest(testCtx,	"ssbo_rw_multiple_groups",				"Read and write same SSBO",		1024,	true,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));
	basicComputeTests->addChild(new InvertSSBOInPlaceTest(testCtx,	"ssbo_unsized_arr_single_invocation",	"Read and write same SSBO",		256,	false,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new InvertSSBOInPlaceTest(testCtx,	"ssbo_unsized_arr_multiple_groups",		"Read and write same SSBO",		1024,	false,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	basicComputeTests->addChild(new WriteToMultipleSSBOTest(testCtx,	"write_multiple_arr_single_invocation",			"Write to multiple SSBOs",	256,	true,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new WriteToMultipleSSBOTest(testCtx,	"write_multiple_arr_multiple_groups",			"Write to multiple SSBOs",	1024,	true,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));
	basicComputeTests->addChild(new WriteToMultipleSSBOTest(testCtx,	"write_multiple_unsized_arr_single_invocation",	"Write to multiple SSBOs",	256,	false,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new WriteToMultipleSSBOTest(testCtx,	"write_multiple_unsized_arr_multiple_groups",	"Write to multiple SSBOs",	1024,	false,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	basicComputeTests->addChild(new SSBOLocalBarrierTest(testCtx,	"ssbo_local_barrier_single_invocation",	"SSBO local barrier usage",	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new SSBOLocalBarrierTest(testCtx,	"ssbo_local_barrier_single_group",		"SSBO local barrier usage",	tcu::IVec3(3,2,5),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new SSBOLocalBarrierTest(testCtx,	"ssbo_local_barrier_multiple_groups",	"SSBO local barrier usage",	tcu::IVec3(3,4,1),	tcu::IVec3(2,7,3)));

	basicComputeTests->addChild(new SSBOBarrierTest(testCtx,	"ssbo_cmd_barrier_single",		"SSBO memory barrier usage",	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new SSBOBarrierTest(testCtx,	"ssbo_cmd_barrier_multiple",	"SSBO memory barrier usage",	tcu::IVec3(11,5,7)));

	basicComputeTests->addChild(new SharedVarTest(testCtx,	"shared_var_single_invocation",		"Basic shared variable usage",	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new SharedVarTest(testCtx,	"shared_var_single_group",			"Basic shared variable usage",	tcu::IVec3(3,2,5),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new SharedVarTest(testCtx,	"shared_var_multiple_invocations",	"Basic shared variable usage",	tcu::IVec3(1,1,1),	tcu::IVec3(2,5,4)));
	basicComputeTests->addChild(new SharedVarTest(testCtx,	"shared_var_multiple_groups",		"Basic shared variable usage",	tcu::IVec3(3,4,1),	tcu::IVec3(2,7,3)));

	basicComputeTests->addChild(new SharedVarAtomicOpTest(testCtx,	"shared_atomic_op_single_invocation",		"Atomic operation with shared var",		tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new SharedVarAtomicOpTest(testCtx,	"shared_atomic_op_single_group",			"Atomic operation with shared var",		tcu::IVec3(3,2,5),	tcu::IVec3(1,1,1)));
	basicComputeTests->addChild(new SharedVarAtomicOpTest(testCtx,	"shared_atomic_op_multiple_invocations",	"Atomic operation with shared var",		tcu::IVec3(1,1,1),	tcu::IVec3(2,5,4)));
	basicComputeTests->addChild(new SharedVarAtomicOpTest(testCtx,	"shared_atomic_op_multiple_groups",			"Atomic operation with shared var",		tcu::IVec3(3,4,1),	tcu::IVec3(2,7,3)));

	basicComputeTests->addChild(new CopyImageToSSBOTest(testCtx,	"copy_image_to_ssbo_small",	"Image to SSBO copy",	tcu::IVec2(1,1),	tcu::IVec2(64,64)));
	basicComputeTests->addChild(new CopyImageToSSBOTest(testCtx,	"copy_image_to_ssbo_large",	"Image to SSBO copy",	tcu::IVec2(2,4),	tcu::IVec2(512,512)));

	basicComputeTests->addChild(new CopySSBOToImageTest(testCtx,	"copy_ssbo_to_image_small",	"SSBO to image copy",	tcu::IVec2(1, 1),	tcu::IVec2(64, 64)));
	basicComputeTests->addChild(new CopySSBOToImageTest(testCtx,	"copy_ssbo_to_image_large",	"SSBO to image copy",	tcu::IVec2(2, 4),	tcu::IVec2(512, 512)));

	basicComputeTests->addChild(new ImageAtomicOpTest(testCtx,	"image_atomic_op_local_size_1",	"Atomic operation with image",	1,	tcu::IVec2(64,64)));
	basicComputeTests->addChild(new ImageAtomicOpTest(testCtx,	"image_atomic_op_local_size_8",	"Atomic operation with image",	8,	tcu::IVec2(64,64)));

	basicComputeTests->addChild(new ImageBarrierTest(testCtx,	"image_barrier_single",		"Image barrier",	tcu::IVec2(1,1)));
	basicComputeTests->addChild(new ImageBarrierTest(testCtx,	"image_barrier_multiple",	"Image barrier",	tcu::IVec2(64,64)));

	return basicComputeTests.release();
}

} // compute
} // vkt
