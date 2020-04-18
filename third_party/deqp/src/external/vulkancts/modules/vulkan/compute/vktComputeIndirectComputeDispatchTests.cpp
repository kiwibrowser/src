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
 * \brief Indirect Compute Dispatch tests
 *//*--------------------------------------------------------------------*/

#include "vktComputeIndirectComputeDispatchTests.hpp"
#include "vktComputeTestsUtil.hpp"

#include <string>
#include <map>
#include <vector>

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkMemUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkQueryUtil.hpp"

#include "tcuVector.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuTestLog.hpp"
#include "tcuRGBA.hpp"
#include "tcuStringTemplate.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"
#include "deStringUtil.hpp"
#include "deArrayUtil.hpp"

#include "gluShaderUtil.hpp"

namespace vkt
{
namespace compute
{
namespace
{

enum
{
	RESULT_BLOCK_BASE_SIZE			= 4 * (int)sizeof(deUint32), // uvec3 + uint
	RESULT_BLOCK_NUM_PASSED_OFFSET	= 3 * (int)sizeof(deUint32),
	INDIRECT_COMMAND_OFFSET			= 3 * (int)sizeof(deUint32),
};

vk::VkDeviceSize getResultBlockAlignedSize (const vk::InstanceInterface&	instance_interface,
											const vk::VkPhysicalDevice		physicalDevice,
											const vk::VkDeviceSize			baseSize)
{
	// TODO getPhysicalDeviceProperties() was added to vkQueryUtil in 41-image-load-store-tests. Use it once it's merged.
	vk::VkPhysicalDeviceProperties deviceProperties;
	instance_interface.getPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vk::VkDeviceSize alignment = deviceProperties.limits.minStorageBufferOffsetAlignment;

	if (alignment == 0 || (baseSize % alignment == 0))
		return baseSize;
	else
		return (baseSize / alignment + 1)*alignment;
}

struct DispatchCommand
{
				DispatchCommand (const deIntptr		offset,
								 const tcu::UVec3&	numWorkGroups)
					: m_offset			(offset)
					, m_numWorkGroups	(numWorkGroups) {}

	deIntptr	m_offset;
	tcu::UVec3	m_numWorkGroups;
};

typedef std::vector<DispatchCommand> DispatchCommandsVec;

struct DispatchCaseDesc
{
								DispatchCaseDesc (const char*					name,
												  const char*					description,
												  const deUintptr				bufferSize,
												  const tcu::UVec3				workGroupSize,
												  const DispatchCommandsVec&	dispatchCommands)
									: m_name				(name)
									, m_description			(description)
									, m_bufferSize			(bufferSize)
									, m_workGroupSize		(workGroupSize)
									, m_dispatchCommands	(dispatchCommands) {}

	const char*					m_name;
	const char*					m_description;
	const deUintptr				m_bufferSize;
	const tcu::UVec3			m_workGroupSize;
	const DispatchCommandsVec	m_dispatchCommands;
};

class IndirectDispatchInstanceBufferUpload : public vkt::TestInstance
{
public:
									IndirectDispatchInstanceBufferUpload	(Context&					context,
																			 const std::string&			name,
																			 const deUintptr			bufferSize,
																			 const tcu::UVec3&			workGroupSize,
																			 const DispatchCommandsVec& dispatchCommands);

	virtual							~IndirectDispatchInstanceBufferUpload	(void) {}

	virtual tcu::TestStatus			iterate									(void);

protected:
	virtual void					fillIndirectBufferData					(const vk::VkCommandBuffer	commandBuffer,
																			 const Buffer&				indirectBuffer);

	deBool							verifyResultBuffer						(const Buffer&				resultBuffer,
																			 const vk::VkDeviceSize		resultBlockSize,
																			 const vk::VkDeviceSize		resultBufferSize) const;

	Context&						m_context;
	const std::string				m_name;

	const vk::DeviceInterface&		m_device_interface;
	const vk::VkDevice				m_device;

	const vk::VkQueue				m_queue;
	const deUint32					m_queueFamilyIndex;

	const deUintptr					m_bufferSize;
	const tcu::UVec3				m_workGroupSize;
	const DispatchCommandsVec		m_dispatchCommands;

	vk::Allocator&					m_allocator;

private:
	IndirectDispatchInstanceBufferUpload (const vkt::TestInstance&);
	IndirectDispatchInstanceBufferUpload& operator= (const vkt::TestInstance&);
};

IndirectDispatchInstanceBufferUpload::IndirectDispatchInstanceBufferUpload (Context&					context,
																			const std::string&			name,
																			const deUintptr				bufferSize,
																			const tcu::UVec3&			workGroupSize,
																			const DispatchCommandsVec&	dispatchCommands)
	: vkt::TestInstance		(context)
	, m_context				(context)
	, m_name				(name)
	, m_device_interface	(context.getDeviceInterface())
	, m_device				(context.getDevice())
	, m_queue				(context.getUniversalQueue())
	, m_queueFamilyIndex	(context.getUniversalQueueFamilyIndex())
	, m_bufferSize			(bufferSize)
	, m_workGroupSize		(workGroupSize)
	, m_dispatchCommands	(dispatchCommands)
	, m_allocator			(context.getDefaultAllocator())
{
}

void IndirectDispatchInstanceBufferUpload::fillIndirectBufferData (const vk::VkCommandBuffer commandBuffer, const Buffer& indirectBuffer)
{
	DE_UNREF(commandBuffer);

	const vk::Allocation& alloc = indirectBuffer.getAllocation();
	deUint8* indirectDataPtr = reinterpret_cast<deUint8*>(alloc.getHostPtr());

	for (DispatchCommandsVec::const_iterator cmdIter = m_dispatchCommands.begin(); cmdIter != m_dispatchCommands.end(); ++cmdIter)
	{
		DE_ASSERT(cmdIter->m_offset >= 0);
		DE_ASSERT(cmdIter->m_offset % sizeof(deUint32) == 0);
		DE_ASSERT(cmdIter->m_offset + INDIRECT_COMMAND_OFFSET <= (deIntptr)m_bufferSize);

		deUint32* const dstPtr = (deUint32*)&indirectDataPtr[cmdIter->m_offset];

		dstPtr[0] = cmdIter->m_numWorkGroups[0];
		dstPtr[1] = cmdIter->m_numWorkGroups[1];
		dstPtr[2] = cmdIter->m_numWorkGroups[2];
	}

	vk::flushMappedMemoryRange(m_device_interface, m_device, alloc.getMemory(), alloc.getOffset(), m_bufferSize);
}

tcu::TestStatus IndirectDispatchInstanceBufferUpload::iterate (void)
{
	tcu::TestContext& testCtx = m_context.getTestContext();

	testCtx.getLog() << tcu::TestLog::Message << "GL_DISPATCH_INDIRECT_BUFFER size = " << m_bufferSize << tcu::TestLog::EndMessage;
	{
		tcu::ScopedLogSection section(testCtx.getLog(), "Commands", "Indirect Dispatch Commands (" + de::toString(m_dispatchCommands.size()) + " in total)");

		for (deUint32 cmdNdx = 0; cmdNdx < m_dispatchCommands.size(); ++cmdNdx)
		{
			testCtx.getLog()
				<< tcu::TestLog::Message
				<< cmdNdx << ": " << "offset = " << m_dispatchCommands[cmdNdx].m_offset << ", numWorkGroups = " << m_dispatchCommands[cmdNdx].m_numWorkGroups
				<< tcu::TestLog::EndMessage;
		}
	}

	// Create result buffer
	const vk::VkDeviceSize resultBlockSize = getResultBlockAlignedSize(m_context.getInstanceInterface(), m_context.getPhysicalDevice(), RESULT_BLOCK_BASE_SIZE);
	const vk::VkDeviceSize resultBufferSize = resultBlockSize * (deUint32)m_dispatchCommands.size();

	Buffer resultBuffer(
		m_device_interface, m_device, m_allocator,
		makeBufferCreateInfo(resultBufferSize, vk::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
		vk::MemoryRequirement::HostVisible);

	{
		const vk::Allocation& alloc = resultBuffer.getAllocation();
		deUint8* resultDataPtr = reinterpret_cast<deUint8*>(alloc.getHostPtr());

		for (deUint32 cmdNdx = 0; cmdNdx < m_dispatchCommands.size(); ++cmdNdx)
		{
			deUint8* const	dstPtr = &resultDataPtr[resultBlockSize*cmdNdx];

			*(deUint32*)(dstPtr + 0 * sizeof(deUint32)) = m_dispatchCommands[cmdNdx].m_numWorkGroups[0];
			*(deUint32*)(dstPtr + 1 * sizeof(deUint32)) = m_dispatchCommands[cmdNdx].m_numWorkGroups[1];
			*(deUint32*)(dstPtr + 2 * sizeof(deUint32)) = m_dispatchCommands[cmdNdx].m_numWorkGroups[2];
			*(deUint32*)(dstPtr + RESULT_BLOCK_NUM_PASSED_OFFSET) = 0;
		}

		vk::flushMappedMemoryRange(m_device_interface, m_device, alloc.getMemory(), alloc.getOffset(), resultBufferSize);
	}

	// Create verify compute shader
	const vk::Unique<vk::VkShaderModule> verifyShader(createShaderModule(
		m_device_interface, m_device, m_context.getBinaryCollection().get("indirect_dispatch_" + m_name + "_verify"), 0u));

	// Create descriptorSetLayout
	vk::DescriptorSetLayoutBuilder layoutBuilder;
	layoutBuilder.addSingleBinding(vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vk::VK_SHADER_STAGE_COMPUTE_BIT);
	vk::Unique<vk::VkDescriptorSetLayout> descriptorSetLayout(layoutBuilder.build(m_device_interface, m_device));

	// Create compute pipeline
	const vk::Unique<vk::VkPipelineLayout> pipelineLayout(makePipelineLayout(m_device_interface, m_device, *descriptorSetLayout));
	const vk::Unique<vk::VkPipeline> computePipeline(makeComputePipeline(m_device_interface, m_device, *pipelineLayout, *verifyShader));

	// Create descriptor pool
	const vk::Unique<vk::VkDescriptorPool> descriptorPool(
		vk::DescriptorPoolBuilder()
		.addType(vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, (deUint32)m_dispatchCommands.size())
		.build(m_device_interface, m_device, vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, static_cast<deUint32>(m_dispatchCommands.size())));

	const vk::VkBufferMemoryBarrier ssboPostBarrier = makeBufferMemoryBarrier(
		vk::VK_ACCESS_SHADER_WRITE_BIT, vk::VK_ACCESS_HOST_READ_BIT, *resultBuffer, 0ull, resultBufferSize);

	// Create command buffer
	const vk::Unique<vk::VkCommandPool> cmdPool(makeCommandPool(m_device_interface, m_device, m_queueFamilyIndex));
	const vk::Unique<vk::VkCommandBuffer> cmdBuffer(allocateCommandBuffer(m_device_interface, m_device, *cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Begin recording commands
	beginCommandBuffer(m_device_interface, *cmdBuffer);

	// Create indirect buffer
	Buffer indirectBuffer(
		m_device_interface, m_device, m_allocator,
		makeBufferCreateInfo(m_bufferSize, vk::VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | vk::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
		vk::MemoryRequirement::HostVisible);
	fillIndirectBufferData(*cmdBuffer, indirectBuffer);

	// Bind compute pipeline
	m_device_interface.cmdBindPipeline(*cmdBuffer, vk::VK_PIPELINE_BIND_POINT_COMPUTE, *computePipeline);

	// Allocate descriptor sets
	typedef de::SharedPtr<vk::Unique<vk::VkDescriptorSet> > SharedVkDescriptorSet;
	std::vector<SharedVkDescriptorSet> descriptorSets(m_dispatchCommands.size());

	vk::VkDeviceSize curOffset = 0;

	// Create descriptor sets
	for (deUint32 cmdNdx = 0; cmdNdx < m_dispatchCommands.size(); ++cmdNdx)
	{
		descriptorSets[cmdNdx] = SharedVkDescriptorSet(new vk::Unique<vk::VkDescriptorSet>(
									makeDescriptorSet(m_device_interface, m_device, *descriptorPool, *descriptorSetLayout)));

		const vk::VkDescriptorBufferInfo resultDescriptorInfo = makeDescriptorBufferInfo(*resultBuffer, curOffset, resultBlockSize);

		vk::DescriptorSetUpdateBuilder descriptorSetBuilder;
		descriptorSetBuilder.writeSingle(**descriptorSets[cmdNdx], vk::DescriptorSetUpdateBuilder::Location::binding(0u), vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultDescriptorInfo);
		descriptorSetBuilder.update(m_device_interface, m_device);

		// Bind descriptor set
		m_device_interface.cmdBindDescriptorSets(*cmdBuffer, vk::VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &(**descriptorSets[cmdNdx]), 0u, DE_NULL);

		// Dispatch indirect compute command
		m_device_interface.cmdDispatchIndirect(*cmdBuffer, *indirectBuffer, m_dispatchCommands[cmdNdx].m_offset);

		curOffset += resultBlockSize;
	}

	// Insert memory barrier
	m_device_interface.cmdPipelineBarrier(*cmdBuffer, vk::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, vk::VK_PIPELINE_STAGE_HOST_BIT, (vk::VkDependencyFlags)0,
										  0, (const vk::VkMemoryBarrier*)DE_NULL,
										  1, &ssboPostBarrier,
										  0, (const vk::VkImageMemoryBarrier*)DE_NULL);

	// End recording commands
	endCommandBuffer(m_device_interface, *cmdBuffer);

	// Wait for command buffer execution finish
	submitCommandsAndWait(m_device_interface, m_device, m_queue, *cmdBuffer);

	// Check if result buffer contains valid values
	if (verifyResultBuffer(resultBuffer, resultBlockSize, resultBufferSize))
		return tcu::TestStatus(QP_TEST_RESULT_PASS, "Pass");
	else
		return tcu::TestStatus(QP_TEST_RESULT_FAIL, "Invalid values in result buffer");
}

deBool IndirectDispatchInstanceBufferUpload::verifyResultBuffer (const Buffer&			resultBuffer,
																 const vk::VkDeviceSize	resultBlockSize,
																 const vk::VkDeviceSize	resultBufferSize) const
{
	deBool allOk = true;
	const vk::Allocation& alloc = resultBuffer.getAllocation();
	vk::invalidateMappedMemoryRange(m_device_interface, m_device, alloc.getMemory(), alloc.getOffset(), resultBufferSize);

	const deUint8* const resultDataPtr = reinterpret_cast<deUint8*>(alloc.getHostPtr());

	for (deUint32 cmdNdx = 0; cmdNdx < m_dispatchCommands.size(); cmdNdx++)
	{
		const DispatchCommand&	cmd = m_dispatchCommands[cmdNdx];
		const deUint8* const	srcPtr = (const deUint8*)resultDataPtr + cmdNdx*resultBlockSize;
		const deUint32			numPassed = *(const deUint32*)(srcPtr + RESULT_BLOCK_NUM_PASSED_OFFSET);
		const deUint32			numInvocationsPerGroup = m_workGroupSize[0] * m_workGroupSize[1] * m_workGroupSize[2];
		const deUint32			numGroups = cmd.m_numWorkGroups[0] * cmd.m_numWorkGroups[1] * cmd.m_numWorkGroups[2];
		const deUint32			expectedCount = numInvocationsPerGroup * numGroups;

		if (numPassed != expectedCount)
		{
			tcu::TestContext& testCtx = m_context.getTestContext();

			testCtx.getLog()
				<< tcu::TestLog::Message
				<< "ERROR: got invalid result for invocation " << cmdNdx
				<< ": got numPassed = " << numPassed << ", expected " << expectedCount
				<< tcu::TestLog::EndMessage;

			allOk = false;
		}
	}

	return allOk;
}

class IndirectDispatchCaseBufferUpload : public vkt::TestCase
{
public:
								IndirectDispatchCaseBufferUpload	(tcu::TestContext&			testCtx,
																	 const DispatchCaseDesc&	caseDesc,
																	 const glu::GLSLVersion		glslVersion);

	virtual						~IndirectDispatchCaseBufferUpload	(void) {}

	virtual void				initPrograms						(vk::SourceCollections&		programCollection) const;
	virtual TestInstance*		createInstance						(Context&					context) const;

protected:
	const deUintptr				m_bufferSize;
	const tcu::UVec3			m_workGroupSize;
	const DispatchCommandsVec	m_dispatchCommands;
	const glu::GLSLVersion		m_glslVersion;

private:
	IndirectDispatchCaseBufferUpload (const vkt::TestCase&);
	IndirectDispatchCaseBufferUpload& operator= (const vkt::TestCase&);
};

IndirectDispatchCaseBufferUpload::IndirectDispatchCaseBufferUpload (tcu::TestContext&		testCtx,
																	const DispatchCaseDesc& caseDesc,
																	const glu::GLSLVersion	glslVersion)
	: vkt::TestCase			(testCtx, caseDesc.m_name, caseDesc.m_description)
	, m_bufferSize			(caseDesc.m_bufferSize)
	, m_workGroupSize		(caseDesc.m_workGroupSize)
	, m_dispatchCommands	(caseDesc.m_dispatchCommands)
	, m_glslVersion			(glslVersion)
{
}

void IndirectDispatchCaseBufferUpload::initPrograms (vk::SourceCollections& programCollection) const
{
	const char* const	versionDecl = glu::getGLSLVersionDeclaration(m_glslVersion);

	std::ostringstream	verifyBuffer;

	verifyBuffer
		<< versionDecl << "\n"
		<< "layout(local_size_x = ${LOCAL_SIZE_X}, local_size_y = ${LOCAL_SIZE_Y}, local_size_z = ${LOCAL_SIZE_Z}) in;\n"
		<< "layout(set = 0, binding = 0, std430) buffer Result\n"
		<< "{\n"
		<< "    uvec3           expectedGroupCount;\n"
		<< "    coherent uint   numPassed;\n"
		<< "} result;\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "    if (all(equal(result.expectedGroupCount, gl_NumWorkGroups)))\n"
		<< "        atomicAdd(result.numPassed, 1u);\n"
		<< "}\n";

	std::map<std::string, std::string> args;

	args["LOCAL_SIZE_X"] = de::toString(m_workGroupSize.x());
	args["LOCAL_SIZE_Y"] = de::toString(m_workGroupSize.y());
	args["LOCAL_SIZE_Z"] = de::toString(m_workGroupSize.z());

	std::string verifyProgramString = tcu::StringTemplate(verifyBuffer.str()).specialize(args);

	programCollection.glslSources.add("indirect_dispatch_" + m_name + "_verify") << glu::ComputeSource(verifyProgramString);
}

TestInstance* IndirectDispatchCaseBufferUpload::createInstance (Context& context) const
{
	return new IndirectDispatchInstanceBufferUpload(context, m_name, m_bufferSize, m_workGroupSize, m_dispatchCommands);
}

class IndirectDispatchInstanceBufferGenerate : public IndirectDispatchInstanceBufferUpload
{
public:
									IndirectDispatchInstanceBufferGenerate	(Context&					context,
																			 const std::string&			name,
																			 const deUintptr			bufferSize,
																			 const tcu::UVec3&			workGroupSize,
																			 const DispatchCommandsVec&	dispatchCommands)
										: IndirectDispatchInstanceBufferUpload(context, name, bufferSize, workGroupSize, dispatchCommands) {}

	virtual							~IndirectDispatchInstanceBufferGenerate	(void) {}

protected:
	virtual void					fillIndirectBufferData					(const vk::VkCommandBuffer	commandBuffer,
																			 const Buffer&				indirectBuffer);

	vk::Move<vk::VkDescriptorPool>	m_descriptorPool;
	vk::Move<vk::VkDescriptorSet>	m_descriptorSet;
	vk::Move<vk::VkPipelineLayout>	m_pipelineLayout;
	vk::Move<vk::VkPipeline>		m_computePipeline;

private:
	IndirectDispatchInstanceBufferGenerate (const vkt::TestInstance&);
	IndirectDispatchInstanceBufferGenerate& operator= (const vkt::TestInstance&);
};

void IndirectDispatchInstanceBufferGenerate::fillIndirectBufferData (const vk::VkCommandBuffer commandBuffer, const Buffer& indirectBuffer)
{
	// Create compute shader that generates data for indirect buffer
	const vk::Unique<vk::VkShaderModule> genIndirectBufferDataShader(createShaderModule(
		m_device_interface, m_device, m_context.getBinaryCollection().get("indirect_dispatch_" + m_name + "_generate"), 0u));

	// Create descriptorSetLayout
	vk::DescriptorSetLayoutBuilder layoutBuilder;
	layoutBuilder.addSingleBinding(vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vk::VK_SHADER_STAGE_COMPUTE_BIT);
	vk::Unique<vk::VkDescriptorSetLayout> descriptorSetLayout(layoutBuilder.build(m_device_interface, m_device));

	// Create compute pipeline
	m_pipelineLayout = makePipelineLayout(m_device_interface, m_device, *descriptorSetLayout);
	m_computePipeline = makeComputePipeline(m_device_interface, m_device, *m_pipelineLayout, *genIndirectBufferDataShader);

	// Create descriptor pool
	m_descriptorPool = vk::DescriptorPoolBuilder()
		.addType(vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(m_device_interface, m_device, vk::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	// Create descriptor set
	m_descriptorSet = makeDescriptorSet(m_device_interface, m_device, *m_descriptorPool, *descriptorSetLayout);

	const vk::VkDescriptorBufferInfo indirectDescriptorInfo = makeDescriptorBufferInfo(*indirectBuffer, 0ull, m_bufferSize);

	vk::DescriptorSetUpdateBuilder	descriptorSetBuilder;
	descriptorSetBuilder.writeSingle(*m_descriptorSet, vk::DescriptorSetUpdateBuilder::Location::binding(0u), vk::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &indirectDescriptorInfo);
	descriptorSetBuilder.update(m_device_interface, m_device);

	const vk::VkBufferMemoryBarrier bufferBarrier = makeBufferMemoryBarrier(
		vk::VK_ACCESS_SHADER_WRITE_BIT, vk::VK_ACCESS_INDIRECT_COMMAND_READ_BIT, *indirectBuffer, 0ull, m_bufferSize);

	// Bind compute pipeline
	m_device_interface.cmdBindPipeline(commandBuffer, vk::VK_PIPELINE_BIND_POINT_COMPUTE, *m_computePipeline);

	// Bind descriptor set
	m_device_interface.cmdBindDescriptorSets(commandBuffer, vk::VK_PIPELINE_BIND_POINT_COMPUTE, *m_pipelineLayout, 0u, 1u, &m_descriptorSet.get(), 0u, DE_NULL);

	// Dispatch compute command
	m_device_interface.cmdDispatch(commandBuffer, 1u, 1u, 1u);

	// Insert memory barrier
	m_device_interface.cmdPipelineBarrier(commandBuffer, vk::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, vk::VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, (vk::VkDependencyFlags)0,
										  0, (const vk::VkMemoryBarrier*)DE_NULL,
										  1, &bufferBarrier,
										  0, (const vk::VkImageMemoryBarrier*)DE_NULL);
}

class IndirectDispatchCaseBufferGenerate : public IndirectDispatchCaseBufferUpload
{
public:
							IndirectDispatchCaseBufferGenerate	(tcu::TestContext&			testCtx,
																 const DispatchCaseDesc&	caseDesc,
																 const glu::GLSLVersion		glslVersion)
								: IndirectDispatchCaseBufferUpload(testCtx, caseDesc, glslVersion) {}

	virtual					~IndirectDispatchCaseBufferGenerate	(void) {}

	virtual void			initPrograms						(vk::SourceCollections&		programCollection) const;
	virtual TestInstance*	createInstance						(Context&					context) const;

private:
	IndirectDispatchCaseBufferGenerate (const vkt::TestCase&);
	IndirectDispatchCaseBufferGenerate& operator= (const vkt::TestCase&);
};

void IndirectDispatchCaseBufferGenerate::initPrograms (vk::SourceCollections& programCollection) const
{
	IndirectDispatchCaseBufferUpload::initPrograms(programCollection);

	const char* const	versionDecl = glu::getGLSLVersionDeclaration(m_glslVersion);

	std::ostringstream computeBuffer;

	// Header
	computeBuffer
		<< versionDecl << "\n"
		<< "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		<< "layout(set = 0, binding = 0, std430) buffer Out\n"
		<< "{\n"
		<< "	highp uint data[];\n"
		<< "};\n"
		<< "void writeCmd (uint offset, uvec3 numWorkGroups)\n"
		<< "{\n"
		<< "	data[offset+0u] = numWorkGroups.x;\n"
		<< "	data[offset+1u] = numWorkGroups.y;\n"
		<< "	data[offset+2u] = numWorkGroups.z;\n"
		<< "}\n"
		<< "void main (void)\n"
		<< "{\n";

	// Dispatch commands
	for (DispatchCommandsVec::const_iterator cmdIter = m_dispatchCommands.begin(); cmdIter != m_dispatchCommands.end(); ++cmdIter)
	{
		const deUint32 offs = (deUint32)(cmdIter->m_offset / sizeof(deUint32));
		DE_ASSERT((size_t)offs * sizeof(deUint32) == (size_t)cmdIter->m_offset);

		computeBuffer
			<< "\twriteCmd(" << offs << "u, uvec3("
			<< cmdIter->m_numWorkGroups.x() << "u, "
			<< cmdIter->m_numWorkGroups.y() << "u, "
			<< cmdIter->m_numWorkGroups.z() << "u));\n";
	}

	// Ending
	computeBuffer << "}\n";

	std::string computeString = computeBuffer.str();

	programCollection.glslSources.add("indirect_dispatch_" + m_name + "_generate") << glu::ComputeSource(computeString);
}

TestInstance* IndirectDispatchCaseBufferGenerate::createInstance (Context& context) const
{
	return new IndirectDispatchInstanceBufferGenerate(context, m_name, m_bufferSize, m_workGroupSize, m_dispatchCommands);
}

DispatchCommandsVec commandsVec (const DispatchCommand& cmd)
{
	DispatchCommandsVec vec;
	vec.push_back(cmd);
	return vec;
}

DispatchCommandsVec commandsVec (const DispatchCommand& cmd0,
								 const DispatchCommand& cmd1,
								 const DispatchCommand& cmd2,
								 const DispatchCommand& cmd3,
								 const DispatchCommand& cmd4)
{
	DispatchCommandsVec vec;
	vec.push_back(cmd0);
	vec.push_back(cmd1);
	vec.push_back(cmd2);
	vec.push_back(cmd3);
	vec.push_back(cmd4);
	return vec;
}

DispatchCommandsVec commandsVec (const DispatchCommand& cmd0,
								 const DispatchCommand& cmd1,
								 const DispatchCommand& cmd2,
								 const DispatchCommand& cmd3,
								 const DispatchCommand& cmd4,
								 const DispatchCommand& cmd5,
								 const DispatchCommand& cmd6)
{
	DispatchCommandsVec vec;
	vec.push_back(cmd0);
	vec.push_back(cmd1);
	vec.push_back(cmd2);
	vec.push_back(cmd3);
	vec.push_back(cmd4);
	vec.push_back(cmd5);
	vec.push_back(cmd6);
	return vec;
}

} // anonymous ns

tcu::TestCaseGroup* createIndirectComputeDispatchTests (tcu::TestContext& testCtx)
{
	static const DispatchCaseDesc s_dispatchCases[] =
	{
		DispatchCaseDesc("single_invocation", "Single invocation only from offset 0", INDIRECT_COMMAND_OFFSET, tcu::UVec3(1, 1, 1),
			commandsVec(DispatchCommand(0, tcu::UVec3(1, 1, 1)))
        ),
		DispatchCaseDesc("multiple_groups", "Multiple groups dispatched from offset 0", INDIRECT_COMMAND_OFFSET, tcu::UVec3(1, 1, 1),
			commandsVec(DispatchCommand(0, tcu::UVec3(2, 3, 5)))
		),
		DispatchCaseDesc("multiple_groups_multiple_invocations", "Multiple groups of size 2x3x1 from offset 0", INDIRECT_COMMAND_OFFSET, tcu::UVec3(2, 3, 1),
			commandsVec(DispatchCommand(0, tcu::UVec3(1, 2, 3)))
		),
		DispatchCaseDesc("small_offset", "Small offset", 16 + INDIRECT_COMMAND_OFFSET, tcu::UVec3(1, 1, 1),
			commandsVec(DispatchCommand(16, tcu::UVec3(1, 1, 1)))
		),
		DispatchCaseDesc("large_offset", "Large offset", (2 << 20), tcu::UVec3(1, 1, 1),
			commandsVec(DispatchCommand((1 << 20) + 12, tcu::UVec3(1, 1, 1)))
		),
		DispatchCaseDesc("large_offset_multiple_invocations", "Large offset, multiple invocations", (2 << 20), tcu::UVec3(2, 3, 1),
			commandsVec(DispatchCommand((1 << 20) + 12, tcu::UVec3(1, 2, 3)))
		),
		DispatchCaseDesc("empty_command", "Empty command", INDIRECT_COMMAND_OFFSET, tcu::UVec3(1, 1, 1),
			commandsVec(DispatchCommand(0, tcu::UVec3(0, 0, 0)))
		),
		DispatchCaseDesc("multi_dispatch", "Dispatch multiple compute commands from single buffer", 1 << 10, tcu::UVec3(3, 1, 2),
			commandsVec(DispatchCommand(0, tcu::UVec3(1, 1, 1)),
						DispatchCommand(INDIRECT_COMMAND_OFFSET, tcu::UVec3(2, 1, 1)),
						DispatchCommand(104, tcu::UVec3(1, 3, 1)),
						DispatchCommand(40, tcu::UVec3(1, 1, 7)),
						DispatchCommand(52, tcu::UVec3(1, 1, 4)))
		),
		DispatchCaseDesc("multi_dispatch_reuse_command", "Dispatch multiple compute commands from single buffer", 1 << 10, tcu::UVec3(3, 1, 2),
			commandsVec(DispatchCommand(0, tcu::UVec3(1, 1, 1)),
						DispatchCommand(0, tcu::UVec3(1, 1, 1)),
						DispatchCommand(0, tcu::UVec3(1, 1, 1)),
						DispatchCommand(104, tcu::UVec3(1, 3, 1)),
						DispatchCommand(104, tcu::UVec3(1, 3, 1)),
						DispatchCommand(52, tcu::UVec3(1, 1, 4)),
						DispatchCommand(52, tcu::UVec3(1, 1, 4)))
		),
	};

	de::MovePtr<tcu::TestCaseGroup> indirectComputeDispatchTests(new tcu::TestCaseGroup(testCtx, "indirect_dispatch", "Indirect dispatch tests"));

	tcu::TestCaseGroup* const	groupBufferUpload = new tcu::TestCaseGroup(testCtx, "upload_buffer", "");
	indirectComputeDispatchTests->addChild(groupBufferUpload);

	for (deUint32 ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_dispatchCases); ndx++)
	{
		groupBufferUpload->addChild(new IndirectDispatchCaseBufferUpload(testCtx, s_dispatchCases[ndx], glu::GLSL_VERSION_310_ES));
	}

	tcu::TestCaseGroup* const	groupBufferGenerate = new tcu::TestCaseGroup(testCtx, "gen_in_compute", "");
	indirectComputeDispatchTests->addChild(groupBufferGenerate);

	for (deUint32 ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_dispatchCases); ndx++)
	{
		groupBufferGenerate->addChild(new IndirectDispatchCaseBufferGenerate(testCtx, s_dispatchCases[ndx], glu::GLSL_VERSION_310_ES));
	}

	return indirectComputeDispatchTests.release();
}

} // compute
} // vkt
