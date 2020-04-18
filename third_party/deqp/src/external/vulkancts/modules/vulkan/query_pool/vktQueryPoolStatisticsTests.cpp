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
 * \brief Vulkan Statistics Query Tests
 *//*--------------------------------------------------------------------*/

#include "vktQueryPoolStatisticsTests.hpp"
#include "vktTestCase.hpp"

#include "vktDrawImageObjectUtil.hpp"
#include "vktDrawBufferObjectUtil.hpp"
#include "vktDrawCreateInfoUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkPrograms.hpp"

#include "deMath.h"

#include "tcuTestLog.hpp"
#include "tcuResource.hpp"
#include "tcuImageCompare.hpp"
#include "vkImageUtil.hpp"
#include "tcuCommandLine.hpp"
#include "tcuRGBA.hpp"

namespace vkt
{
namespace QueryPool
{
namespace
{

using namespace vk;
using namespace Draw;

//Test parameters
enum
{
	WIDTH	= 64,
	HEIGHT	= 64
};

std::string inputTypeToGLString (const VkPrimitiveTopology& inputType)
{
	switch (inputType)
	{
		case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
			return "points";
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
			return "lines";
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
			return "lines_adjacency";
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
			return "triangles";
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
			return "triangles_adjacency";
		default:
			DE_ASSERT(DE_FALSE);
			return "error";
	}
}

std::string outputTypeToGLString (const VkPrimitiveTopology& outputType)
{
	switch (outputType)
	{
		case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
			return "points";
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
				return "line_strip";
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
			return "triangle_strip";
		default:
			DE_ASSERT(DE_FALSE);
			return "error";
	}
}

void beginCommandBuffer (const DeviceInterface& vk, const VkCommandBuffer commandBuffer)
{
	const VkCommandBufferBeginInfo info =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,		// VkStructureType							sType;
		DE_NULL,											// const void*								pNext;
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,		// VkCommandBufferUsageFlags				flags;
		DE_NULL,											// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
	};
	VK_CHECK(vk.beginCommandBuffer(commandBuffer, &info));
}

void beginSecondaryCommandBuffer (const DeviceInterface&				vk,
								  const VkCommandBuffer					commandBuffer,
								  const VkQueryPipelineStatisticFlags	queryFlags,
								  const VkRenderPass					renderPass = (VkRenderPass)0u,
								  const VkFramebuffer					framebuffer = (VkFramebuffer)0u,
								  const VkCommandBufferUsageFlags		bufferUsageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
{
	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		DE_NULL,
		renderPass,					// renderPass
		0u,							// subpass
		framebuffer,				// framebuffer
		VK_FALSE,					// occlusionQueryEnable
		(VkQueryControlFlags)0u,	// queryFlags
		queryFlags,					// pipelineStatistics
	};

	const VkCommandBufferBeginInfo			info					=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType							sType;
		DE_NULL,										// const void*								pNext;
		bufferUsageFlags,								// VkCommandBufferUsageFlags				flags;
		&secCmdBufInheritInfo,							// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
	};
	VK_CHECK(vk.beginCommandBuffer(commandBuffer, &info));
}

void submitCommandsAndWait (const DeviceInterface&	vk,
							const VkDevice			device,
							const VkQueue			queue,
							const VkCommandBuffer	commandBuffer)
{
	const VkFenceCreateInfo	fenceInfo	=
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		(VkFenceCreateFlags)0,					// VkFenceCreateFlags	flags;
	};
	const Unique<VkFence>	fence		(createFence(vk, device, &fenceInfo));

	const VkSubmitInfo		submitInfo	=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
		DE_NULL,						// const void*					pNext;
		0u,								// uint32_t						waitSemaphoreCount;
		DE_NULL,						// const VkSemaphore*			pWaitSemaphores;
		DE_NULL,						// const VkPipelineStageFlags*	pWaitDstStageMask;
		1u,								// uint32_t						commandBufferCount;
		&commandBuffer,					// const VkCommandBuffer*		pCommandBuffers;
		0u,								// uint32_t						signalSemaphoreCount;
		DE_NULL,						// const VkSemaphore*			pSignalSemaphores;
	};
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
	VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, ~0ull));
}

Move<VkQueryPool> makeQueryPool (const DeviceInterface& vk, const VkDevice device, VkQueryPipelineStatisticFlags statisticFlags)
{
	const VkQueryPoolCreateInfo queryPoolCreateInfo =
	{
		VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,	// VkStructureType					sType
		DE_NULL,									// const void*						pNext
		(VkQueryPoolCreateFlags)0,					// VkQueryPoolCreateFlags			flags
		VK_QUERY_TYPE_PIPELINE_STATISTICS ,			// VkQueryType						queryType
		1u,											// deUint32							entryCount
		statisticFlags,								// VkQueryPipelineStatisticFlags	pipelineStatistics
	};
	return createQueryPool(vk, device, &queryPoolCreateInfo);
}

Move<VkPipelineLayout> makePipelineLayout (const DeviceInterface& vk, const VkDevice device, const VkDescriptorSetLayout* descriptorSetLayout)
{
	const VkPipelineLayoutCreateInfo pipelineLayoutParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,	// VkStructureType					sType;
		DE_NULL,										// const void*						pNext;
		0u,												// VkPipelineLayoutCreateFlags		flags;
		1u,												// deUint32							setLayoutCount;
		descriptorSetLayout,							// const VkDescriptorSetLayout*		pSetLayouts;
		0u,												// deUint32							pushConstantRangeCount;
		DE_NULL,										// const VkPushConstantRange*		pPushConstantRanges;
	};
	return (createPipelineLayout(vk, device, &pipelineLayoutParams));
}

void clearBuffer (const DeviceInterface& vk, const VkDevice device, const de::SharedPtr<Buffer> buffer, const VkDeviceSize bufferSizeBytes)
{
	const std::vector<deUint8>	data			((size_t)bufferSizeBytes, 0u);
	const Allocation&			allocation		= buffer->getBoundMemory();
	void*						allocationData	= allocation.getHostPtr();
	invalidateMappedMemoryRange(vk, device, allocation.getMemory(), allocation.getOffset(), bufferSizeBytes);
	deMemcpy(allocationData, &data[0], (size_t)bufferSizeBytes);
}

class StatisticQueryTestInstance : public TestInstance
{
public:
					StatisticQueryTestInstance	(Context& context);
protected:
	virtual void	checkExtensions				(void);
};

StatisticQueryTestInstance::StatisticQueryTestInstance (Context& context)
	: TestInstance	(context)
{
}

void StatisticQueryTestInstance::checkExtensions (void)
{
	if (!m_context.getDeviceFeatures().pipelineStatisticsQuery)
		throw tcu::NotSupportedError("Pipeline statistics queries are not supported");
}

class ComputeInvocationsTestInstance : public StatisticQueryTestInstance
{
public:
	struct ParametersCompute
	{
		tcu::UVec3	localSize;
		tcu::UVec3	groupSize;
		std::string	shaderName;
	};
							ComputeInvocationsTestInstance		(Context& context, const std::vector<ParametersCompute>& parameters);
	tcu::TestStatus			iterate								(void);
protected:
	virtual tcu::TestStatus	executeTest							(const VkCommandPool&			cmdPool,
																 const VkPipelineLayout			pipelineLayout,
																 const VkDescriptorSet&			descriptorSet,
																 const de::SharedPtr<Buffer>	buffer,
																 const VkDeviceSize				bufferSizeBytes);
	deUint32				getComputeExecution					(const ParametersCompute& parm) const
		{
			return parm.localSize.x() * parm.localSize.y() *parm.localSize.z() * parm.groupSize.x() * parm.groupSize.y() * parm.groupSize.z();
		}
	const std::vector<ParametersCompute>&	m_parameters;
};

ComputeInvocationsTestInstance::ComputeInvocationsTestInstance (Context& context, const std::vector<ParametersCompute>& parameters)
	: StatisticQueryTestInstance	(context)
	, m_parameters					(parameters)
{
}

tcu::TestStatus	ComputeInvocationsTestInstance::iterate (void)
{
	checkExtensions();
	const DeviceInterface&				vk						= m_context.getDeviceInterface();
	const VkDevice						device					= m_context.getDevice();
	deUint32							maxSize					= 0u;

	for(size_t parametersNdx = 0; parametersNdx < m_parameters.size(); ++parametersNdx)
		maxSize = deMaxu32(maxSize, getComputeExecution(m_parameters[parametersNdx]));

	const VkDeviceSize					bufferSizeBytes			= static_cast<VkDeviceSize>(deAlignSize(static_cast<size_t>(sizeof(deUint32) * maxSize),
																								static_cast<size_t>(m_context.getDeviceProperties().limits.nonCoherentAtomSize)));
	de::SharedPtr<Buffer>				buffer					= Buffer::createAndAlloc(vk, device, BufferCreateInfo(bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
																							 m_context.getDefaultAllocator(), MemoryRequirement::HostVisible);

	const Unique<VkDescriptorSetLayout>	descriptorSetLayout		(DescriptorSetLayoutBuilder()
			.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.build(vk, device));

	const Unique<VkPipelineLayout>		pipelineLayout			(makePipelineLayout(vk, device, &(*descriptorSetLayout)));

	const Unique<VkDescriptorPool>		descriptorPool			(DescriptorPoolBuilder()
			.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const VkDescriptorSetAllocateInfo allocateParams		=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,	// VkStructureType				sType;
		DE_NULL,										// const void*					pNext;
		*descriptorPool,								// VkDescriptorPool				descriptorPool;
		1u,												// deUint32						setLayoutCount;
		&(*descriptorSetLayout),						// const VkDescriptorSetLayout*	pSetLayouts;
	};

	const Unique<VkDescriptorSet>		descriptorSet		(allocateDescriptorSet(vk, device, &allocateParams));
	const VkDescriptorBufferInfo		descriptorInfo		=
	{
		buffer->object(),	//VkBuffer		buffer;
		0ull,				//VkDeviceSize	offset;
		bufferSizeBytes,	//VkDeviceSize	range;
	};

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &descriptorInfo)
		.update(vk, device);

	const CmdPoolCreateInfo			cmdPoolCreateInfo	(m_context.getUniversalQueueFamilyIndex());
	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, &cmdPoolCreateInfo));

	return executeTest (*cmdPool, *pipelineLayout, *descriptorSet, buffer, bufferSizeBytes);
}

tcu::TestStatus ComputeInvocationsTestInstance::executeTest (const VkCommandPool&			cmdPool,
															 const VkPipelineLayout			pipelineLayout,
															 const VkDescriptorSet&			descriptorSet,
															 const de::SharedPtr<Buffer>	buffer,
															 const VkDeviceSize				bufferSizeBytes)
{
	const DeviceInterface&				vk						= m_context.getDeviceInterface();
	const VkDevice						device					= m_context.getDevice();
	const VkQueue						queue					= m_context.getUniversalQueue();
	const VkBufferMemoryBarrier			computeFinishBarrier	=
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
		DE_NULL,									// const void*		pNext;
		VK_ACCESS_SHADER_WRITE_BIT,					// VkAccessFlags	srcAccessMask;
		VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags	dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			destQueueFamilyIndex;
		buffer->object(),							// VkBuffer			buffer;
		0ull,										// VkDeviceSize		offset;
		bufferSizeBytes,							// VkDeviceSize		size;
	};

	for(size_t parametersNdx = 0u; parametersNdx < m_parameters.size(); ++parametersNdx)
	{
		clearBuffer(vk, device, buffer, bufferSizeBytes);
		const Unique<VkShaderModule>			shaderModule				(createShaderModule(vk, device,
																			m_context.getBinaryCollection().get(m_parameters[parametersNdx].shaderName), (VkShaderModuleCreateFlags)0u));

		const VkPipelineShaderStageCreateInfo	pipelineShaderStageParams	=
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0u,					// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_COMPUTE_BIT,							// VkShaderStageFlagBits				stage;
			*shaderModule,											// VkShaderModule						module;
			"main",													// const char*							pName;
			DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
		};

		const VkComputePipelineCreateInfo		pipelineCreateInfo			=
		{
			VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,	// VkStructureType					sType;
			DE_NULL,										// const void*						pNext;
			(VkPipelineCreateFlags)0u,						// VkPipelineCreateFlags			flags;
			pipelineShaderStageParams,						// VkPipelineShaderStageCreateInfo	stage;
			pipelineLayout,									// VkPipelineLayout					layout;
			DE_NULL,										// VkPipeline						basePipelineHandle;
			0,												// deInt32							basePipelineIndex;
		};
		const Unique<VkPipeline> pipeline(createComputePipeline(vk, device, DE_NULL , &pipelineCreateInfo));

		const Unique<VkCommandBuffer>	cmdBuffer			(allocateCommandBuffer(vk, device, cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const Unique<VkQueryPool>		queryPool			(makeQueryPool(vk, device, VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT));

		beginCommandBuffer(vk, *cmdBuffer);
			vk.cmdResetQueryPool(*cmdBuffer, *queryPool, 0u, 1u);

			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);

			vk.cmdBeginQuery(*cmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
			vk.cmdDispatch(*cmdBuffer, m_parameters[parametersNdx].groupSize.x(), m_parameters[parametersNdx].groupSize.y(), m_parameters[parametersNdx].groupSize.z());
			vk.cmdEndQuery(*cmdBuffer, *queryPool, 0u);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
				(VkDependencyFlags)0u, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, &computeFinishBarrier, 0u, (const VkImageMemoryBarrier*)DE_NULL);
		VK_CHECK(vk.endCommandBuffer(*cmdBuffer));

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Compute shader invocations: " << getComputeExecution(m_parameters[parametersNdx]) << tcu::TestLog::EndMessage;

		// Wait for completion
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);

		// Validate the results
		const Allocation& bufferAllocation = buffer->getBoundMemory();
		invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);

		{
			deUint64 data = 0u;
			VK_CHECK(vk.getQueryPoolResults(device, *queryPool, 0u, 1u, sizeof(deUint64), &data, 0u, VK_QUERY_RESULT_64_BIT));
			if (getComputeExecution(m_parameters[parametersNdx]) != data)
				return tcu::TestStatus::fail("QueryPoolResults incorrect");
		}

		const deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());
		for (deUint32 ndx = 0u; ndx < getComputeExecution(m_parameters[parametersNdx]); ++ndx)
		{
			if (bufferPtr[ndx] != ndx)
				return tcu::TestStatus::fail("Compute shader didn't write data to the buffer");
		}
	}
	return tcu::TestStatus::pass("Pass");
}

class ComputeInvocationsSecondaryTestInstance : public ComputeInvocationsTestInstance
{
public:
							ComputeInvocationsSecondaryTestInstance	(Context& context, const std::vector<ParametersCompute>& parameters);
protected:
	tcu::TestStatus			executeTest								(const VkCommandPool&			cmdPool,
																	 const VkPipelineLayout			pipelineLayout,
																	 const VkDescriptorSet&			descriptorSet,
																	 const de::SharedPtr<Buffer>	buffer,
																	 const VkDeviceSize				bufferSizeBytes);
	virtual tcu::TestStatus	checkResult								(const de::SharedPtr<Buffer>	buffer,
																	 const VkDeviceSize				bufferSizeBytes,
																	 const VkQueryPool				queryPool);
};

ComputeInvocationsSecondaryTestInstance::ComputeInvocationsSecondaryTestInstance	(Context& context, const std::vector<ParametersCompute>& parameters)
	: ComputeInvocationsTestInstance	(context, parameters)
{
}

tcu::TestStatus ComputeInvocationsSecondaryTestInstance::executeTest (const VkCommandPool&			cmdPool,
																	  const VkPipelineLayout		pipelineLayout,
																	  const VkDescriptorSet&		descriptorSet,
																	  const de::SharedPtr<Buffer>	buffer,
																	  const VkDeviceSize			bufferSizeBytes)
{
	typedef de::SharedPtr<Unique<VkShaderModule> >	VkShaderModuleSp;
	typedef de::SharedPtr<Unique<VkPipeline> >		VkPipelineSp;

	const DeviceInterface&					vk							= m_context.getDeviceInterface();
	const VkDevice							device						= m_context.getDevice();
	const VkQueue							queue						= m_context.getUniversalQueue();

	const VkBufferMemoryBarrier				computeShaderWriteBarrier	=
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
		DE_NULL,									// const void*		pNext;
		VK_ACCESS_SHADER_WRITE_BIT,					// VkAccessFlags	srcAccessMask;
		VK_ACCESS_SHADER_WRITE_BIT,					// VkAccessFlags	dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			destQueueFamilyIndex;
		buffer->object(),							// VkBuffer			buffer;
		0ull,										// VkDeviceSize		offset;
		bufferSizeBytes,							// VkDeviceSize		size;
	};

	const VkBufferMemoryBarrier				computeFinishBarrier		=
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
		DE_NULL,									// const void*		pNext;
		VK_ACCESS_SHADER_WRITE_BIT,					// VkAccessFlags	srcAccessMask;
		VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags	dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			destQueueFamilyIndex;
		buffer->object(),							// VkBuffer			buffer;
		0ull,										// VkDeviceSize		offset;
		bufferSizeBytes,							// VkDeviceSize		size;
	};

	std::vector<VkShaderModuleSp>			shaderModule;
	std::vector<VkPipelineSp>				pipeline;
	for(size_t parametersNdx = 0; parametersNdx < m_parameters.size(); ++parametersNdx)
	{
		shaderModule.push_back(VkShaderModuleSp(new Unique<VkShaderModule>(createShaderModule(vk, device, m_context.getBinaryCollection().get(m_parameters[parametersNdx].shaderName), (VkShaderModuleCreateFlags)0u))));
		const VkPipelineShaderStageCreateInfo	pipelineShaderStageParams	=
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			0u,														// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_COMPUTE_BIT,							// VkShaderStageFlagBits				stage;
			shaderModule.back().get()->get(),						// VkShaderModule						module;
			"main",													// const char*							pName;
			DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
		};

		const VkComputePipelineCreateInfo		pipelineCreateInfo			=
		{
			VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,	// VkStructureType					sType;
			DE_NULL,										// const void*						pNext;
			0u,												// VkPipelineCreateFlags			flags;
			pipelineShaderStageParams,						// VkPipelineShaderStageCreateInfo	stage;
			pipelineLayout,									// VkPipelineLayout					layout;
			DE_NULL,										// VkPipeline						basePipelineHandle;
			0,												// deInt32							basePipelineIndex;
		};
		pipeline.push_back(VkPipelineSp(new Unique<VkPipeline>(createComputePipeline(vk, device, DE_NULL , &pipelineCreateInfo))));
	}

	const Unique<VkCommandBuffer>				primaryCmdBuffer			(allocateCommandBuffer(vk, device, cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const Unique<VkCommandBuffer>				secondaryCmdBuffer			(allocateCommandBuffer(vk, device, cmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY));

	const Unique<VkQueryPool>					queryPool					(makeQueryPool(vk, device, VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT));

	clearBuffer(vk, device, buffer, bufferSizeBytes);
	beginSecondaryCommandBuffer(vk, *secondaryCmdBuffer, VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT);
		vk.cmdBindDescriptorSets(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);
		vk.cmdResetQueryPool(*secondaryCmdBuffer, *queryPool, 0u, 1u);
		vk.cmdBeginQuery(*secondaryCmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
		for(size_t parametersNdx = 0; parametersNdx < m_parameters.size(); ++parametersNdx)
		{
				vk.cmdBindPipeline(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline[parametersNdx].get()->get());
				vk.cmdDispatch(*secondaryCmdBuffer, m_parameters[parametersNdx].groupSize.x(), m_parameters[parametersNdx].groupSize.y(), m_parameters[parametersNdx].groupSize.z());

				vk.cmdPipelineBarrier(*secondaryCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					(VkDependencyFlags)0u, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, &computeShaderWriteBarrier, 0u, (const VkImageMemoryBarrier*)DE_NULL);
		}
		vk.cmdEndQuery(*secondaryCmdBuffer, *queryPool, 0u);
	VK_CHECK(vk.endCommandBuffer(*secondaryCmdBuffer));

	beginCommandBuffer(vk, *primaryCmdBuffer);
		vk.cmdExecuteCommands(*primaryCmdBuffer, 1u, &secondaryCmdBuffer.get());

		vk.cmdPipelineBarrier(*primaryCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
			(VkDependencyFlags)0u, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, &computeFinishBarrier, 0u, (const VkImageMemoryBarrier*)DE_NULL);

	VK_CHECK(vk.endCommandBuffer(*primaryCmdBuffer));

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *primaryCmdBuffer);
	return checkResult(buffer, bufferSizeBytes, *queryPool);
}

tcu::TestStatus ComputeInvocationsSecondaryTestInstance::checkResult (const de::SharedPtr<Buffer> buffer, const VkDeviceSize bufferSizeBytes, const VkQueryPool queryPool)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	{
		deUint64 result		= 0u;
		deUint64 expected	= 0u;
		for(size_t parametersNdx = 0; parametersNdx < m_parameters.size(); ++parametersNdx)
			expected += getComputeExecution(m_parameters[parametersNdx]);
		VK_CHECK(vk.getQueryPoolResults(device, queryPool, 0u, 1u, sizeof(deUint64), &result, 0u, VK_QUERY_RESULT_64_BIT));
		if (expected != result)
			return tcu::TestStatus::fail("QueryPoolResults incorrect");
	}

	{
		// Validate the results
		const Allocation&	bufferAllocation	= buffer->getBoundMemory();
		invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);
		const deUint32*		bufferPtr			= static_cast<deUint32*>(bufferAllocation.getHostPtr());
		deUint32			minSize				= ~0u;
		for(size_t parametersNdx = 0; parametersNdx < m_parameters.size(); ++parametersNdx)
			minSize = deMinu32(minSize, getComputeExecution(m_parameters[parametersNdx]));
		for (deUint32 ndx = 0u; ndx < minSize; ++ndx)
		{
			if (bufferPtr[ndx] != ndx * m_parameters.size())
				return tcu::TestStatus::fail("Compute shader didn't write data to the buffer");
		}
	}
	return tcu::TestStatus::pass("Pass");
}

class ComputeInvocationsSecondaryInheritedTestInstance : public ComputeInvocationsSecondaryTestInstance
{
public:
					ComputeInvocationsSecondaryInheritedTestInstance	(Context& context, const std::vector<ParametersCompute>& parameters);
protected:
	virtual void	checkExtensions							(void);
	tcu::TestStatus	executeTest								(const VkCommandPool&			cmdPool,
															 const VkPipelineLayout			pipelineLayout,
															 const VkDescriptorSet&			descriptorSet,
															 const de::SharedPtr<Buffer>	buffer,
															 const VkDeviceSize				bufferSizeBytes);
};

ComputeInvocationsSecondaryInheritedTestInstance::ComputeInvocationsSecondaryInheritedTestInstance	(Context& context, const std::vector<ParametersCompute>& parameters)
	: ComputeInvocationsSecondaryTestInstance	(context, parameters)
{
}

void ComputeInvocationsSecondaryInheritedTestInstance::checkExtensions (void)
{
	StatisticQueryTestInstance::checkExtensions();
	if (!m_context.getDeviceFeatures().inheritedQueries)
		throw tcu::NotSupportedError("Inherited queries are not supported");
}

tcu::TestStatus ComputeInvocationsSecondaryInheritedTestInstance::executeTest (const VkCommandPool&			cmdPool,
																			  const VkPipelineLayout		pipelineLayout,
																			  const VkDescriptorSet&		descriptorSet,
																			  const de::SharedPtr<Buffer>	buffer,
																			  const VkDeviceSize			bufferSizeBytes)
{
	typedef de::SharedPtr<Unique<VkShaderModule> >	VkShaderModuleSp;
	typedef de::SharedPtr<Unique<VkPipeline> >		VkPipelineSp;

	const DeviceInterface&						vk								= m_context.getDeviceInterface();
	const VkDevice								device							= m_context.getDevice();
	const VkQueue								queue							= m_context.getUniversalQueue();

	const VkBufferMemoryBarrier					computeShaderWriteBarrier		=
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
		DE_NULL,									// const void*		pNext;
		VK_ACCESS_SHADER_WRITE_BIT,					// VkAccessFlags	srcAccessMask;
		VK_ACCESS_SHADER_WRITE_BIT,					// VkAccessFlags	dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			destQueueFamilyIndex;
		buffer->object(),							// VkBuffer			buffer;
		0ull,										// VkDeviceSize		offset;
		bufferSizeBytes,							// VkDeviceSize		size;
	};

	const VkBufferMemoryBarrier					computeFinishBarrier			=
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
		DE_NULL,									// const void*		pNext;
		VK_ACCESS_SHADER_WRITE_BIT,					// VkAccessFlags	srcAccessMask;
		VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags	dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			destQueueFamilyIndex;
		buffer->object(),							// VkBuffer			buffer;
		0ull,										// VkDeviceSize		offset;
		bufferSizeBytes,							// VkDeviceSize		size;
	};

	std::vector<VkShaderModuleSp>				shaderModule;
	std::vector<VkPipelineSp>					pipeline;
	for(size_t parametersNdx = 0u; parametersNdx < m_parameters.size(); ++parametersNdx)
	{
		shaderModule.push_back(VkShaderModuleSp(new Unique<VkShaderModule>(createShaderModule(vk, device, m_context.getBinaryCollection().get(m_parameters[parametersNdx].shaderName), (VkShaderModuleCreateFlags)0u))));
		const VkPipelineShaderStageCreateInfo	pipelineShaderStageParams		=
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType					sType;
			DE_NULL,												// const void*						pNext;
			0u,														// VkPipelineShaderStageCreateFlags	flags;
			VK_SHADER_STAGE_COMPUTE_BIT,							// VkShaderStageFlagBits			stage;
			shaderModule.back().get()->get(),						// VkShaderModule					module;
			"main",													// const char*						pName;
			DE_NULL,												// const VkSpecializationInfo*		pSpecializationInfo;
		};

		const VkComputePipelineCreateInfo		pipelineCreateInfo				=
		{
			VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,	// VkStructureType					sType;
			DE_NULL,										// const void*						pNext;
			0u,												// VkPipelineCreateFlags			flags;
			pipelineShaderStageParams,						// VkPipelineShaderStageCreateInfo	stage;
			pipelineLayout,									// VkPipelineLayout					layout;
			DE_NULL,										// VkPipeline						basePipelineHandle;
			0,												// deInt32							basePipelineIndex;
		};
		pipeline.push_back(VkPipelineSp(new Unique<VkPipeline>(createComputePipeline(vk, device, DE_NULL , &pipelineCreateInfo))));
	}

	const Unique<VkCommandBuffer>				primaryCmdBuffer			(allocateCommandBuffer(vk, device, cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const Unique<VkCommandBuffer>				secondaryCmdBuffer			(allocateCommandBuffer(vk, device, cmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY));

	const Unique<VkQueryPool>					queryPool					(makeQueryPool(vk, device, VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT));

	clearBuffer(vk, device, buffer, bufferSizeBytes);
	beginSecondaryCommandBuffer(vk, *secondaryCmdBuffer, VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT);
		vk.cmdBindDescriptorSets(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);
		for(size_t parametersNdx = 1; parametersNdx < m_parameters.size(); ++parametersNdx)
		{
				vk.cmdBindPipeline(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline[parametersNdx].get()->get());
				vk.cmdDispatch(*secondaryCmdBuffer, m_parameters[parametersNdx].groupSize.x(), m_parameters[parametersNdx].groupSize.y(), m_parameters[parametersNdx].groupSize.z());

				vk.cmdPipelineBarrier(*secondaryCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					(VkDependencyFlags)0u, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, &computeShaderWriteBarrier, 0u, (const VkImageMemoryBarrier*)DE_NULL);
		}
	VK_CHECK(vk.endCommandBuffer(*secondaryCmdBuffer));

	beginCommandBuffer(vk, *primaryCmdBuffer);
		vk.cmdResetQueryPool(*primaryCmdBuffer, *queryPool, 0u, 1u);
		vk.cmdBindDescriptorSets(*primaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);
		vk.cmdBindPipeline(*primaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline[0].get()->get());

		vk.cmdBeginQuery(*primaryCmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
		vk.cmdDispatch(*primaryCmdBuffer, m_parameters[0].groupSize.x(), m_parameters[0].groupSize.y(), m_parameters[0].groupSize.z());

		vk.cmdPipelineBarrier(*primaryCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				(VkDependencyFlags)0u, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, &computeShaderWriteBarrier, 0u, (const VkImageMemoryBarrier*)DE_NULL);

		vk.cmdExecuteCommands(*primaryCmdBuffer, 1u, &secondaryCmdBuffer.get());

		vk.cmdPipelineBarrier(*primaryCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
			(VkDependencyFlags)0u, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, &computeFinishBarrier, 0u, (const VkImageMemoryBarrier*)DE_NULL);

		vk.cmdEndQuery(*primaryCmdBuffer, *queryPool, 0u);
	VK_CHECK(vk.endCommandBuffer(*primaryCmdBuffer));

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *primaryCmdBuffer);
	return checkResult(buffer, bufferSizeBytes, *queryPool);
}

class GraphicBasicTestInstance : public StatisticQueryTestInstance
{
public:
	struct VertexData
	{
		VertexData (const tcu::Vec4 position_, const tcu::Vec4 color_)
			: position	(position_)
			, color		(color_)
		{}
		tcu::Vec4	position;
		tcu::Vec4	color;
	};
	struct  ParametersGraphic
	{
			ParametersGraphic (const VkQueryPipelineStatisticFlags queryStatisticFlags_, const VkPrimitiveTopology primitiveTopology_)
			: queryStatisticFlags	(queryStatisticFlags_)
			, primitiveTopology		(primitiveTopology_)
		{}
		VkQueryPipelineStatisticFlags	queryStatisticFlags;
		VkPrimitiveTopology				primitiveTopology;
	};
											GraphicBasicTestInstance			(vkt::Context&					context,
																				 const std::vector<VertexData>&	data,
																				 const ParametersGraphic&		parametersGraphic);
	tcu::TestStatus							iterate								(void);
protected:
	de::SharedPtr<Buffer>					creatAndFillVertexBuffer			(void);
	virtual void							createPipeline						(void) = 0;
	void									creatColorAttachmentAndRenderPass	(void);
	bool									checkImage							(void);
	virtual tcu::TestStatus					executeTest							(void) = 0;
	virtual tcu::TestStatus					checkResult							(VkQueryPool queryPool) = 0;
	virtual void							draw								(VkCommandBuffer cmdBuffer) = 0;

	const VkFormat						m_colorAttachmentFormat;
	de::SharedPtr<Image>				m_colorAttachmentImage;
	de::SharedPtr<Image>				m_depthImage;
	Move<VkImageView>					m_attachmentView;
	Move<VkImageView>					m_depthiew;
	Move<VkRenderPass>					m_renderPass;
	Move<VkFramebuffer>					m_framebuffer;
	Move<VkPipeline>					m_pipeline;
	Move<VkPipelineLayout>				m_pipelineLayout;
	const std::vector<VertexData>&		m_data;
	const ParametersGraphic&			m_parametersGraphic;
};

GraphicBasicTestInstance::GraphicBasicTestInstance (vkt::Context&					context,
													const std::vector<VertexData>&	data,
													const ParametersGraphic&		parametersGraphic)
	: StatisticQueryTestInstance	(context)
	, m_colorAttachmentFormat		(VK_FORMAT_R8G8B8A8_UNORM)
	, m_data						(data)
	, m_parametersGraphic			(parametersGraphic)
{
}

tcu::TestStatus GraphicBasicTestInstance::iterate (void)
{
	checkExtensions();
	creatColorAttachmentAndRenderPass();
	createPipeline();
	return executeTest();
}

de::SharedPtr<Buffer> GraphicBasicTestInstance::creatAndFillVertexBuffer (void)
{
	const DeviceInterface&		vk				= m_context.getDeviceInterface();
	const VkDevice				device			= m_context.getDevice();

	const VkDeviceSize			dataSize		= static_cast<VkDeviceSize>(deAlignSize(static_cast<size_t>( m_data.size() * sizeof(VertexData)),
		static_cast<size_t>(m_context.getDeviceProperties().limits.nonCoherentAtomSize)));

	de::SharedPtr<Buffer>		vertexBuffer	= Buffer::createAndAlloc(vk, device, BufferCreateInfo(dataSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), m_context.getDefaultAllocator(), MemoryRequirement::HostVisible);

	deUint8*					ptr				= reinterpret_cast<deUint8*>(vertexBuffer->getBoundMemory().getHostPtr());
	deMemcpy(ptr, &m_data[0], static_cast<size_t>( m_data.size() * sizeof(VertexData)));

	flushMappedMemoryRange(vk, device, vertexBuffer->getBoundMemory().getMemory(), vertexBuffer->getBoundMemory().getOffset(), dataSize);
	return vertexBuffer;
}

void GraphicBasicTestInstance::creatColorAttachmentAndRenderPass (void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	{
		VkExtent3D					imageExtent				=
		{
			WIDTH,	// width;
			HEIGHT,	// height;
			1u		// depth;
		};

		const ImageCreateInfo		colorImageCreateInfo	(VK_IMAGE_TYPE_2D, m_colorAttachmentFormat, imageExtent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
															VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		m_colorAttachmentImage	= Image::createAndAlloc(vk, device, colorImageCreateInfo, m_context.getDefaultAllocator(), m_context.getUniversalQueueFamilyIndex());

		const ImageViewCreateInfo	attachmentViewInfo		(m_colorAttachmentImage->object(), VK_IMAGE_VIEW_TYPE_2D, m_colorAttachmentFormat);
		m_attachmentView			= createImageView(vk, device, &attachmentViewInfo);

		ImageCreateInfo				depthImageCreateInfo	(vk::VK_IMAGE_TYPE_2D, VK_FORMAT_D16_UNORM, imageExtent, 1, 1, vk::VK_SAMPLE_COUNT_1_BIT, vk::VK_IMAGE_TILING_OPTIMAL,
															 vk::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		m_depthImage				= Image::createAndAlloc(vk, device, depthImageCreateInfo, m_context.getDefaultAllocator(), m_context.getUniversalQueueFamilyIndex());

		// Construct a depth  view from depth image
		const ImageViewCreateInfo	depthViewInfo			(m_depthImage->object(), vk::VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_D16_UNORM);
		m_depthiew				= vk::createImageView(vk, device, &depthViewInfo);
	}

	{
		// Renderpass and Framebuffer
		RenderPassCreateInfo		renderPassCreateInfo;
		renderPassCreateInfo.addAttachment(AttachmentDescription(m_colorAttachmentFormat,						// format
																	VK_SAMPLE_COUNT_1_BIT,						// samples
																	VK_ATTACHMENT_LOAD_OP_CLEAR,				// loadOp
																	VK_ATTACHMENT_STORE_OP_STORE ,				// storeOp
																	VK_ATTACHMENT_LOAD_OP_DONT_CARE,			// stencilLoadOp
																	VK_ATTACHMENT_STORE_OP_STORE ,				// stencilLoadOp
																	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// initialLauout
																	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));	// finalLayout

		renderPassCreateInfo.addAttachment(AttachmentDescription(VK_FORMAT_D16_UNORM,										// format
																 vk::VK_SAMPLE_COUNT_1_BIT,									// samples
																 vk::VK_ATTACHMENT_LOAD_OP_CLEAR,							// loadOp
																 vk::VK_ATTACHMENT_STORE_OP_DONT_CARE,						// storeOp
																 vk::VK_ATTACHMENT_LOAD_OP_DONT_CARE,						// stencilLoadOp
																 vk::VK_ATTACHMENT_STORE_OP_DONT_CARE,						// stencilLoadOp
																 vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,		// initialLauout
																 vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));	// finalLayout

		const VkAttachmentReference	colorAttachmentReference =
		{
			0u,											// attachment
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// layout
		};

		const VkAttachmentReference depthAttachmentReference =
		{
			1u,															// attachment
			vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL		// layout
		};

		const VkSubpassDescription	subpass =
		{
			(VkSubpassDescriptionFlags) 0,		//VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,	//VkPipelineBindPoint			pipelineBindPoint;
			0u,									//deUint32						inputAttachmentCount;
			DE_NULL,							//const VkAttachmentReference*	pInputAttachments;
			1u,									//deUint32						colorAttachmentCount;
			&colorAttachmentReference,			//const VkAttachmentReference*	pColorAttachments;
			DE_NULL,							//const VkAttachmentReference*	pResolveAttachments;
			&depthAttachmentReference,			//const VkAttachmentReference*	pDepthStencilAttachment;
			0u,									//deUint32						preserveAttachmentCount;
			DE_NULL,							//const deUint32*				pPreserveAttachments;
		};

		renderPassCreateInfo.addSubpass(subpass);
		m_renderPass = createRenderPass(vk, device, &renderPassCreateInfo);

		std::vector<vk::VkImageView> attachments(2);
		attachments[0] = *m_attachmentView;
		attachments[1] = *m_depthiew;

		FramebufferCreateInfo		framebufferCreateInfo(*m_renderPass, attachments, WIDTH, HEIGHT, 1);
		m_framebuffer = createFramebuffer(vk, device, &framebufferCreateInfo);
	}
}

bool GraphicBasicTestInstance::checkImage (void)
{
	const VkQueue						queue			= m_context.getUniversalQueue();
	const VkOffset3D					zeroOffset		= { 0, 0, 0 };
	const tcu::ConstPixelBufferAccess	renderedFrame	= m_colorAttachmentImage->readSurface(queue, m_context.getDefaultAllocator(),
															VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, VK_IMAGE_ASPECT_COLOR_BIT);
	int									colorNdx		= 0;
	tcu::Texture2D						referenceFrame	(mapVkFormat(m_colorAttachmentFormat), WIDTH, HEIGHT);
	referenceFrame.allocLevel(0);

	for (int y = 0; y < HEIGHT/2; ++y)
	for (int x = 0; x < WIDTH/2; ++x)
			referenceFrame.getLevel(0).setPixel(m_data[colorNdx].color, x, y);

	colorNdx += 4;
	for (int y =  HEIGHT/2; y < HEIGHT; ++y)
	for (int x = 0; x < WIDTH/2; ++x)
			referenceFrame.getLevel(0).setPixel(m_data[colorNdx].color, x, y);

	colorNdx += 4;
	for (int y = 0; y < HEIGHT/2; ++y)
	for (int x =  WIDTH/2; x < WIDTH; ++x)
			referenceFrame.getLevel(0).setPixel(m_data[colorNdx].color, x, y);

	colorNdx += 4;
	for (int y =  HEIGHT/2; y < HEIGHT; ++y)
	for (int x =  WIDTH/2; x < WIDTH; ++x)
			referenceFrame.getLevel(0).setPixel(m_data[colorNdx].color, x, y);

	return tcu::floatThresholdCompare(m_context.getTestContext().getLog(), "Result", "Image comparison result", referenceFrame.getLevel(0), renderedFrame, tcu::Vec4(0.01f), tcu::COMPARE_LOG_ON_ERROR);
}

class VertexShaderTestInstance : public GraphicBasicTestInstance
{
public:
							VertexShaderTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic);
protected:
	virtual void			createPipeline				(void);
	virtual tcu::TestStatus	executeTest					(void);
	virtual tcu::TestStatus	checkResult					(VkQueryPool queryPool);
	void					draw						(VkCommandBuffer cmdBuffer);
};

VertexShaderTestInstance::VertexShaderTestInstance (vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic)
	: GraphicBasicTestInstance	(context, data, parametersGraphic)
{
}

void VertexShaderTestInstance::createPipeline (void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	// Pipeline
	Unique<VkShaderModule> vs(createShaderModule(vk, device, m_context.getBinaryCollection().get("vertex"), 0));
	Unique<VkShaderModule> fs(createShaderModule(vk, device, m_context.getBinaryCollection().get("fragment"), 0));

	const PipelineCreateInfo::ColorBlendState::Attachment attachmentState;

	const PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	m_pipelineLayout = createPipelineLayout(vk, device, &pipelineLayoutCreateInfo);

	const VkVertexInputBindingDescription vertexInputBindingDescription		=
	{
		0,											// binding;
		static_cast<deUint32>(sizeof(VertexData)),	// stride;
		VK_VERTEX_INPUT_RATE_VERTEX				// inputRate
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
	{
		{
			0u,
			0u,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			0u
		},	// VertexElementData::position
		{
			1u,
			0u,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			static_cast<deUint32>(sizeof(tcu::Vec4))
		},	// VertexElementData::color
	};

	const VkPipelineVertexInputStateCreateInfo vf_info			=
	{																	// sType;
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// pNext;
		NULL,															// flags;
		0u,																// vertexBindingDescriptionCount;
		1u,																// pVertexBindingDescriptions;
		&vertexInputBindingDescription,									// vertexAttributeDescriptionCount;
		2u,																// pVertexAttributeDescriptions;
		vertexInputAttributeDescriptions
	};

	PipelineCreateInfo pipelineCreateInfo(*m_pipelineLayout, *m_renderPass, 0, (VkPipelineCreateFlags)0);
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*vs, "main", VK_SHADER_STAGE_VERTEX_BIT));
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*fs, "main", VK_SHADER_STAGE_FRAGMENT_BIT));
	pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState());
	pipelineCreateInfo.addState(PipelineCreateInfo::InputAssemblerState(m_parametersGraphic.primitiveTopology));
	pipelineCreateInfo.addState(PipelineCreateInfo::ColorBlendState(1, &attachmentState));

	const VkViewport	viewport	=
	{
		0.0f,		// float x;
		0.0f,		// float y;
		WIDTH,	// float width;
		HEIGHT,	// float height;
		0.0f,	// float minDepth;
		1.0f	// float maxDepth;
	};

	const VkRect2D		scissor		=
	{
		{
			0,		// deInt32 x
			0,		// deInt32 y
		},		// VkOffset2D	offset;
		{
			WIDTH,	// deInt32 width;
			HEIGHT,	// deInt32 height
		},		// VkExtent2D	extent;
	};
	pipelineCreateInfo.addState(PipelineCreateInfo::ViewportState(1u, std::vector<VkViewport>(1, viewport), std::vector<VkRect2D>(1, scissor)));
	pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState());
	pipelineCreateInfo.addState(PipelineCreateInfo::RasterizerState());
	pipelineCreateInfo.addState(PipelineCreateInfo::MultiSampleState());
	pipelineCreateInfo.addState(vf_info);
	m_pipeline = createGraphicsPipeline(vk, device, DE_NULL, &pipelineCreateInfo);
}

tcu::TestStatus VertexShaderTestInstance::executeTest (void)
{
	const DeviceInterface&					vk						= m_context.getDeviceInterface();
	const VkDevice							device					= m_context.getDevice();
	const VkQueue							queue					= m_context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();

	const CmdPoolCreateInfo					cmdPoolCreateInfo		(queueFamilyIndex);
	const Move<VkCommandPool>				cmdPool					= createCommandPool(vk, device, &cmdPoolCreateInfo);
	const Unique<VkQueryPool>				queryPool				(makeQueryPool(vk, device, m_parametersGraphic.queryStatisticFlags));

	const VkDeviceSize						vertexBufferOffset		= 0u;
	const de::SharedPtr<Buffer>				vertexBufferSp			= creatAndFillVertexBuffer();
	const VkBuffer							vertexBuffer			= vertexBufferSp->object();

	const Unique<VkCommandBuffer>			cmdBuffer				(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	beginCommandBuffer(vk, *cmdBuffer);
	{
		const VkRect2D				renderArea				= { { 0, 0 }, { WIDTH, HEIGHT } };
		std::vector<VkClearValue>	renderPassClearValues	(2);
		deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(VkClearValue));
		const RenderPassBeginInfo	renderPassBegin			(*m_renderPass, *m_framebuffer, renderArea, renderPassClearValues);

		initialTransitionColor2DImage(vk, *cmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		initialTransitionDepth2DImage(vk, *cmdBuffer, m_depthImage->object(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

		vk.cmdResetQueryPool(*cmdBuffer, *queryPool, 0u, 1u);

		vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

		vk.cmdBeginQuery(*cmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
		vk.cmdBindVertexBuffers(*cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		draw(*cmdBuffer);
		vk.cmdEndQuery(*cmdBuffer, *queryPool, 0u);

		vk.cmdEndRenderPass(*cmdBuffer);

		transition2DImage(vk, *cmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
						  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vk.endCommandBuffer(*cmdBuffer);

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	return checkResult (*queryPool);
}

tcu::TestStatus VertexShaderTestInstance::checkResult (VkQueryPool queryPool)
{
	const DeviceInterface&	vk			= m_context.getDeviceInterface();
	const VkDevice			device		= m_context.getDevice();
	deUint64				result		= 0u;
	deUint64				expectedMin	= 0u;
	deUint64				expectedMax	= 0u;
	switch(m_parametersGraphic.queryStatisticFlags)
	{
		case VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT:
			expectedMin = 16u;
			expectedMax = expectedMin + 3u;
			break;
		case VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT:
			expectedMin =	m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST					? 15u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY			?  8u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY		? 14u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY		?  6u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY	?  8u :
							16u;
			expectedMax =	m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY		? 12u : 19u;
			break;
		case VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT:
			expectedMin =	m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST						? 16u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST						?  8u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP						? 15u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST					?  5u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP					?  8u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN						? 14u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY			?  4u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY		? 13u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY		?  2u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY	?  6u :
							0u;
			expectedMax = expectedMin + 3u;
			break;
		case VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT:
			expectedMin =	m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST						?     9u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST						?   192u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP						?   448u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST					?  2016u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP					?  4096u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN						? 10208u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY			?   128u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY		?   416u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY		?   992u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY	?  3072u :
							0u;
			expectedMax = 4u * expectedMin;
			break;
		default:
			DE_ASSERT(0);
			break;
	}

	VK_CHECK(vk.getQueryPoolResults(device, queryPool, 0u, 1u, sizeof(deUint64), &result, 0u, VK_QUERY_RESULT_64_BIT));
	if (result < expectedMin || result > expectedMax)
		return tcu::TestStatus::fail("QueryPoolResults incorrect");

	if (m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP && !checkImage())
		return tcu::TestStatus::fail("Result image doesn't match expected image.");

	return tcu::TestStatus::pass("Pass");
}

void VertexShaderTestInstance::draw (VkCommandBuffer cmdBuffer)
{
	const DeviceInterface& vk = m_context.getDeviceInterface();
	switch(m_parametersGraphic.primitiveTopology)
	{
		case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
			vk.cmdDraw(cmdBuffer, 16u, 1u, 0u, 0u);
			break;
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
			vk.cmdDraw(cmdBuffer, 4u, 1u, 0u,  0u);
			vk.cmdDraw(cmdBuffer, 4u, 1u, 4u,  1u);
			vk.cmdDraw(cmdBuffer, 4u, 1u, 8u,  2u);
			vk.cmdDraw(cmdBuffer, 4u, 1u, 12u, 3u);
			break;
		default:
			DE_ASSERT(0);
			break;
	}
}

class VertexShaderSecondaryTestInstance : public VertexShaderTestInstance
{
public:
							VertexShaderSecondaryTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic);
protected:
	virtual tcu::TestStatus	executeTest							(void);
};

VertexShaderSecondaryTestInstance::VertexShaderSecondaryTestInstance (vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic)
	: VertexShaderTestInstance	(context, data, parametersGraphic)
{
}

tcu::TestStatus VertexShaderSecondaryTestInstance::executeTest (void)
{
	const DeviceInterface&					vk						= m_context.getDeviceInterface();
	const VkDevice							device					= m_context.getDevice();
	const VkQueue							queue					= m_context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();

	const CmdPoolCreateInfo					cmdPoolCreateInfo		(queueFamilyIndex);
	const Move<VkCommandPool>				cmdPool					= createCommandPool(vk, device, &cmdPoolCreateInfo);
	const Unique<VkQueryPool>				queryPool				(makeQueryPool(vk, device, m_parametersGraphic.queryStatisticFlags));

	const VkDeviceSize						vertexBufferOffset		= 0u;
	const de::SharedPtr<Buffer>				vertexBufferSp			= creatAndFillVertexBuffer();
	const VkBuffer							vertexBuffer			= vertexBufferSp->object();

	const Unique<VkCommandBuffer>			primaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const Unique<VkCommandBuffer>			secondaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY));

	beginSecondaryCommandBuffer(vk, *secondaryCmdBuffer, m_parametersGraphic.queryStatisticFlags, *m_renderPass, *m_framebuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
		vk.cmdBindPipeline(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		vk.cmdBeginQuery(*secondaryCmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
		vk.cmdBindVertexBuffers(*secondaryCmdBuffer, 0u, 1u, &vertexBuffer, &vertexBufferOffset);
		draw(*secondaryCmdBuffer);
		vk.cmdEndQuery(*secondaryCmdBuffer, *queryPool, 0u);
	vk.endCommandBuffer(*secondaryCmdBuffer);

	beginCommandBuffer(vk, *primaryCmdBuffer);
	{
		const VkRect2D				renderArea				= { { 0, 0 }, { WIDTH, HEIGHT } };
		std::vector<VkClearValue>	renderPassClearValues	(2);
		deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(VkClearValue));
		const RenderPassBeginInfo	renderPassBegin			(*m_renderPass, *m_framebuffer, renderArea, renderPassClearValues);

		initialTransitionColor2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									  vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		initialTransitionDepth2DImage(vk, *primaryCmdBuffer, m_depthImage->object(), vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									  vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, vk::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | vk::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

		vk.cmdResetQueryPool(*primaryCmdBuffer, *queryPool, 0u, 1u);

		vk.cmdBeginRenderPass(*primaryCmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		vk.cmdExecuteCommands(*primaryCmdBuffer, 1u, &secondaryCmdBuffer.get());
		vk.cmdEndRenderPass(*primaryCmdBuffer);
		transition2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
						  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vk.endCommandBuffer(*primaryCmdBuffer);

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *primaryCmdBuffer);
	return checkResult (*queryPool);
}

class VertexShaderSecondaryInheritedTestInstance : public VertexShaderTestInstance
{
public:
							VertexShaderSecondaryInheritedTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic);
protected:
	void					checkExtensions						(void);
	virtual tcu::TestStatus	executeTest							(void);
};

VertexShaderSecondaryInheritedTestInstance::VertexShaderSecondaryInheritedTestInstance (vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic)
	: VertexShaderTestInstance	(context, data, parametersGraphic)
{
}

void VertexShaderSecondaryInheritedTestInstance::checkExtensions (void)
{
	StatisticQueryTestInstance::checkExtensions();
	if (!m_context.getDeviceFeatures().inheritedQueries)
		throw tcu::NotSupportedError("Inherited queries are not supported");
}

tcu::TestStatus VertexShaderSecondaryInheritedTestInstance::executeTest (void)
{
	const DeviceInterface&					vk						= m_context.getDeviceInterface();
	const VkDevice							device					= m_context.getDevice();
	const VkQueue							queue					= m_context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();

	const CmdPoolCreateInfo					cmdPoolCreateInfo		(queueFamilyIndex);
	const Move<VkCommandPool>				cmdPool					= createCommandPool(vk, device, &cmdPoolCreateInfo);
	const Unique<VkQueryPool>				queryPool				(makeQueryPool(vk, device, m_parametersGraphic.queryStatisticFlags));

	const VkDeviceSize						vertexBufferOffset		= 0u;
	const de::SharedPtr<Buffer>				vertexBufferSp			= creatAndFillVertexBuffer();
	const VkBuffer							vertexBuffer			= vertexBufferSp->object();

	const Unique<VkCommandBuffer>			primaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const Unique<VkCommandBuffer>			secondaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY));

	beginSecondaryCommandBuffer(vk, *secondaryCmdBuffer, m_parametersGraphic.queryStatisticFlags, *m_renderPass, *m_framebuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
		vk.cmdBindPipeline(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		vk.cmdBindVertexBuffers(*secondaryCmdBuffer, 0u, 1u, &vertexBuffer, &vertexBufferOffset);
		draw(*secondaryCmdBuffer);
	vk.endCommandBuffer(*secondaryCmdBuffer);

	beginCommandBuffer(vk, *primaryCmdBuffer);
	{
		const VkRect2D				renderArea				= { { 0, 0 }, { WIDTH, HEIGHT } };
		std::vector<VkClearValue>	renderPassClearValues	(2);
		deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(VkClearValue));
		const RenderPassBeginInfo	renderPassBegin			(*m_renderPass, *m_framebuffer, renderArea, renderPassClearValues);

		initialTransitionColor2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		initialTransitionDepth2DImage(vk, *primaryCmdBuffer, m_depthImage->object(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

		vk.cmdResetQueryPool(*primaryCmdBuffer, *queryPool, 0u, 1u);
		vk.cmdBeginQuery(*primaryCmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);

		vk.cmdBeginRenderPass(*primaryCmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		vk.cmdExecuteCommands(*primaryCmdBuffer, 1u, &secondaryCmdBuffer.get());
		vk.cmdEndRenderPass(*primaryCmdBuffer);
		vk.cmdEndQuery(*primaryCmdBuffer, *queryPool, 0u);
		transition2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
						  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vk.endCommandBuffer(*primaryCmdBuffer);

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *primaryCmdBuffer);
	return checkResult (*queryPool);
}

class GeometryShaderTestInstance : public GraphicBasicTestInstance
{
public:
							GeometryShaderTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic);
protected:
	virtual void			checkExtensions				(void);
	virtual void			createPipeline				(void);
	virtual tcu::TestStatus	executeTest					(void);
	tcu::TestStatus			checkResult					(VkQueryPool queryPool);
	void					draw						(VkCommandBuffer cmdBuffer);
};

GeometryShaderTestInstance::GeometryShaderTestInstance (vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic)
	: GraphicBasicTestInstance	(context, data, parametersGraphic)
{
}

void GeometryShaderTestInstance::checkExtensions (void)
{
	StatisticQueryTestInstance::checkExtensions();
	if (!m_context.getDeviceFeatures().geometryShader)
		throw tcu::NotSupportedError("Geometry shader are not supported");
}

void GeometryShaderTestInstance::createPipeline (void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	// Pipeline
	Unique<VkShaderModule> vs(createShaderModule(vk, device, m_context.getBinaryCollection().get("vertex"), (VkShaderModuleCreateFlags)0));
	Unique<VkShaderModule> gs(createShaderModule(vk, device, m_context.getBinaryCollection().get("geometry"), (VkShaderModuleCreateFlags)0));
	Unique<VkShaderModule> fs(createShaderModule(vk, device, m_context.getBinaryCollection().get("fragment"), (VkShaderModuleCreateFlags)0));

	const PipelineCreateInfo::ColorBlendState::Attachment attachmentState;

	const PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	m_pipelineLayout = createPipelineLayout(vk, device, &pipelineLayoutCreateInfo);

	const VkVertexInputBindingDescription vertexInputBindingDescription		=
	{
		0u,											// binding;
		static_cast<deUint32>(sizeof(VertexData)),	// stride;
		VK_VERTEX_INPUT_RATE_VERTEX					// inputRate
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
	{
		{
			0u,
			0u,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			0u
		},	// VertexElementData::position
		{
			1u,
			0u,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			static_cast<deUint32>(sizeof(tcu::Vec4))
		},	// VertexElementData::color
	};

	const VkPipelineVertexInputStateCreateInfo vf_info			=
	{																	// sType;
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// pNext;
		NULL,															// flags;
		0u,																// vertexBindingDescriptionCount;
		1,																// pVertexBindingDescriptions;
		&vertexInputBindingDescription,									// vertexAttributeDescriptionCount;
		2,																// pVertexAttributeDescriptions;
		vertexInputAttributeDescriptions
	};

	PipelineCreateInfo pipelineCreateInfo(*m_pipelineLayout, *m_renderPass, 0, (VkPipelineCreateFlags)0);
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*vs, "main", VK_SHADER_STAGE_VERTEX_BIT));
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*gs, "main", VK_SHADER_STAGE_GEOMETRY_BIT));
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*fs, "main", VK_SHADER_STAGE_FRAGMENT_BIT));
	pipelineCreateInfo.addState(PipelineCreateInfo::InputAssemblerState(m_parametersGraphic.primitiveTopology));
	pipelineCreateInfo.addState(PipelineCreateInfo::ColorBlendState(1, &attachmentState));

	const VkViewport	viewport	=
	{
		0.0f,		// float x;
		0.0f,		// float y;
		WIDTH,	// float width;
		HEIGHT,	// float height;
		0.0f,	// float minDepth;
		1.0f	// float maxDepth;
	};

	const VkRect2D		scissor		=
	{
		{
			0,		// deInt32 x
			0,		// deInt32 y
		},		// VkOffset2D	offset;
		{
			WIDTH,	// deInt32 width;
			HEIGHT,	// deInt32 height
		},		// VkExtent2D	extent;
	};
	pipelineCreateInfo.addState(PipelineCreateInfo::ViewportState(1, std::vector<VkViewport>(1, viewport), std::vector<VkRect2D>(1, scissor)));

	if (m_context.getDeviceFeatures().depthBounds)
		pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState(true, true, VK_COMPARE_OP_GREATER_OR_EQUAL, true));
	else
		pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState());

	pipelineCreateInfo.addState(PipelineCreateInfo::RasterizerState(false));
	pipelineCreateInfo.addState(PipelineCreateInfo::MultiSampleState());
	pipelineCreateInfo.addState(vf_info);
	m_pipeline = createGraphicsPipeline(vk, device, DE_NULL, &pipelineCreateInfo);
}

tcu::TestStatus GeometryShaderTestInstance::executeTest (void)
{
	const DeviceInterface&					vk						= m_context.getDeviceInterface();
	const VkDevice							device					= m_context.getDevice();
	const VkQueue							queue					= m_context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();

	const CmdPoolCreateInfo					cmdPoolCreateInfo		(queueFamilyIndex);
	const Move<VkCommandPool>				cmdPool					= createCommandPool(vk, device, &cmdPoolCreateInfo);
	const Unique<VkQueryPool>				queryPool				(makeQueryPool(vk, device, m_parametersGraphic.queryStatisticFlags));

	const VkDeviceSize						vertexBufferOffset		= 0u;
	const de::SharedPtr<Buffer>				vertexBufferSp			= creatAndFillVertexBuffer();
	const VkBuffer							vertexBuffer			= vertexBufferSp->object();

	const Unique<VkCommandBuffer>			cmdBuffer				(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	beginCommandBuffer(vk, *cmdBuffer);
	{
		const VkRect2D				renderArea				= { { 0, 0 }, { WIDTH, HEIGHT } };
		std::vector<VkClearValue>	renderPassClearValues	(2);
		deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(VkClearValue));
		const RenderPassBeginInfo	renderPassBegin			(*m_renderPass, *m_framebuffer, renderArea, renderPassClearValues);

		initialTransitionColor2DImage(vk, *cmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		initialTransitionDepth2DImage(vk, *cmdBuffer, m_depthImage->object(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

		vk.cmdResetQueryPool(*cmdBuffer, *queryPool, 0u, 1u);

		vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

		vk.cmdBeginQuery(*cmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
		vk.cmdBindVertexBuffers(*cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

		draw(*cmdBuffer);

		vk.cmdEndQuery(*cmdBuffer, *queryPool, 0u);

		vk.cmdEndRenderPass(*cmdBuffer);

		transition2DImage(vk, *cmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
						  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vk.endCommandBuffer(*cmdBuffer);

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	return checkResult(*queryPool);
}

tcu::TestStatus GeometryShaderTestInstance::checkResult (VkQueryPool queryPool)
{
	const DeviceInterface&	vk			= m_context.getDeviceInterface();
	const VkDevice			device		= m_context.getDevice();
	deUint64				result		= 0u;
	deUint64				expectedMin	= 0u;
	deUint64				expectedMax	= 0u;

	switch(m_parametersGraphic.queryStatisticFlags)
	{
		case VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT:
			expectedMin =	m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST						? 16u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST						? 8u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP						? 15u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST					? 4u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP					? 4u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN						? 14u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY			? 4u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY		? 13u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY		? 2u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY	? 6u :
							0u;
			break;
		case VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT:
		case VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT:
		case VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT:
			expectedMin =	m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST						? 112u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST						? 32u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP						? 60u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST					? 8u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP					? 8u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN						? 28u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY			? 16u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY		? 52u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY		? 4u :
							m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY	? 12u :
							0u;
			break;
		default:
			DE_ASSERT(0);
		break;
	}

	expectedMax = expectedMin + 1;

	VK_CHECK(vk.getQueryPoolResults(device, queryPool, 0u, 1u, sizeof(deUint64), &result, 0u, VK_QUERY_RESULT_64_BIT));
	if (result < expectedMin || result > expectedMax)
		return tcu::TestStatus::fail("QueryPoolResults incorrect");

	if ( (m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST || m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ) && !checkImage())
		return tcu::TestStatus::fail("Result image doesn't match expected image.");

	return tcu::TestStatus::pass("Pass");
}

void GeometryShaderTestInstance::draw (VkCommandBuffer cmdBuffer)
{
	const DeviceInterface&	vk			= m_context.getDeviceInterface();
	if (m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ||
		m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
	{
		vk.cmdDraw(cmdBuffer, 3u, 1u,  0u,  1u);
		vk.cmdDraw(cmdBuffer, 3u, 1u,  4u,  1u);
		vk.cmdDraw(cmdBuffer, 3u, 1u,  8u,  2u);
		vk.cmdDraw(cmdBuffer, 3u, 1u, 12u,  3u);
	}
	else
	{
		vk.cmdDraw(cmdBuffer, 16u, 1u, 0u,  0u);
	}
}

class GeometryShaderSecondaryTestInstance : public GeometryShaderTestInstance
{
public:
							GeometryShaderSecondaryTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic);
protected:
	virtual tcu::TestStatus	executeTest							(void);
};

GeometryShaderSecondaryTestInstance::GeometryShaderSecondaryTestInstance (vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic)
	: GeometryShaderTestInstance	(context, data, parametersGraphic)
{
}

tcu::TestStatus GeometryShaderSecondaryTestInstance::executeTest (void)
{
	const DeviceInterface&					vk						= m_context.getDeviceInterface();
	const VkDevice							device					= m_context.getDevice();
	const VkQueue							queue					= m_context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();

	const CmdPoolCreateInfo					cmdPoolCreateInfo		(queueFamilyIndex);
	const Move<VkCommandPool>				cmdPool					= createCommandPool(vk, device, &cmdPoolCreateInfo);
	const Unique<VkQueryPool>				queryPool				(makeQueryPool(vk, device, m_parametersGraphic.queryStatisticFlags));

	const VkDeviceSize						vertexBufferOffset		= 0;
	const de::SharedPtr<Buffer>				vertexBufferSp			= creatAndFillVertexBuffer();
	const VkBuffer							vertexBuffer			= vertexBufferSp->object();

	const Unique<VkCommandBuffer>			primaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const Unique<VkCommandBuffer>			secondaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY));

	beginSecondaryCommandBuffer(vk, *secondaryCmdBuffer, m_parametersGraphic.queryStatisticFlags, *m_renderPass, *m_framebuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
		vk.cmdBindPipeline(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		vk.cmdBeginQuery(*secondaryCmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
		vk.cmdBindVertexBuffers(*secondaryCmdBuffer, 0u, 1u, &vertexBuffer, &vertexBufferOffset);
		draw(*secondaryCmdBuffer);
		vk.cmdEndQuery(*secondaryCmdBuffer, *queryPool, 0u);
	vk.endCommandBuffer(*secondaryCmdBuffer);

	beginCommandBuffer(vk, *primaryCmdBuffer);
	{
		const VkRect2D				renderArea				= { { 0, 0 }, { WIDTH, HEIGHT } };
		std::vector<VkClearValue>	renderPassClearValues	(2);
		deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(VkClearValue));
		const RenderPassBeginInfo	renderPassBegin			(*m_renderPass, *m_framebuffer, renderArea, renderPassClearValues);

		initialTransitionColor2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		initialTransitionDepth2DImage(vk, *primaryCmdBuffer, m_depthImage->object(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

		vk.cmdResetQueryPool(*primaryCmdBuffer, *queryPool, 0u, 1u);
		vk.cmdBeginRenderPass(*primaryCmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		vk.cmdExecuteCommands(*primaryCmdBuffer, 1u, &secondaryCmdBuffer.get());
		vk.cmdEndRenderPass(*primaryCmdBuffer);

		transition2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
						  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vk.endCommandBuffer(*primaryCmdBuffer);

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *primaryCmdBuffer);
	return checkResult(*queryPool);
}

class GeometryShaderSecondaryInheritedTestInstance : public GeometryShaderTestInstance
{
public:
							GeometryShaderSecondaryInheritedTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic);
protected:
	void					checkExtensions						(void);
	virtual tcu::TestStatus	executeTest							(void);
};

GeometryShaderSecondaryInheritedTestInstance::GeometryShaderSecondaryInheritedTestInstance (vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic)
	: GeometryShaderTestInstance	(context, data, parametersGraphic)
{
}

void GeometryShaderSecondaryInheritedTestInstance::checkExtensions (void)
{
	GeometryShaderTestInstance::checkExtensions();
	if (!m_context.getDeviceFeatures().inheritedQueries)
		throw tcu::NotSupportedError("Inherited queries are not supported");
}

tcu::TestStatus GeometryShaderSecondaryInheritedTestInstance::executeTest (void)
{
	const DeviceInterface&					vk						= m_context.getDeviceInterface();
	const VkDevice							device					= m_context.getDevice();
	const VkQueue							queue					= m_context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();

	const CmdPoolCreateInfo					cmdPoolCreateInfo		(queueFamilyIndex);
	const Move<VkCommandPool>				cmdPool					= createCommandPool(vk, device, &cmdPoolCreateInfo);
	const Unique<VkQueryPool>				queryPool				(makeQueryPool(vk, device, m_parametersGraphic.queryStatisticFlags));

	const VkDeviceSize						vertexBufferOffset		= 0u;
	const de::SharedPtr<Buffer>				vertexBufferSp			= creatAndFillVertexBuffer();
	const VkBuffer							vertexBuffer			= vertexBufferSp->object();

	const Unique<VkCommandBuffer>			primaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const Unique<VkCommandBuffer>			secondaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY));

	beginSecondaryCommandBuffer(vk, *secondaryCmdBuffer, m_parametersGraphic.queryStatisticFlags, *m_renderPass, *m_framebuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
		vk.cmdBindPipeline(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		vk.cmdBindVertexBuffers(*secondaryCmdBuffer, 0u, 1u, &vertexBuffer, &vertexBufferOffset);
		draw(*secondaryCmdBuffer);
	vk.endCommandBuffer(*secondaryCmdBuffer);

	beginCommandBuffer(vk, *primaryCmdBuffer);
	{
		const VkRect2D				renderArea				= { { 0, 0 }, { WIDTH, HEIGHT } };
		std::vector<VkClearValue>	renderPassClearValues	(2);
		deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(VkClearValue));
		const RenderPassBeginInfo	renderPassBegin			(*m_renderPass, *m_framebuffer, renderArea, renderPassClearValues);

		initialTransitionColor2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		initialTransitionDepth2DImage(vk, *primaryCmdBuffer, m_depthImage->object(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

		vk.cmdResetQueryPool(*primaryCmdBuffer, *queryPool, 0u, 1u);
		vk.cmdBeginQuery(*primaryCmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
		vk.cmdBeginRenderPass(*primaryCmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		vk.cmdExecuteCommands(*primaryCmdBuffer, 1u, &secondaryCmdBuffer.get());
		vk.cmdEndRenderPass(*primaryCmdBuffer);
		vk.cmdEndQuery(*primaryCmdBuffer, *queryPool, 0u);

		transition2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
						  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vk.endCommandBuffer(*primaryCmdBuffer);

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *primaryCmdBuffer);
	return checkResult(*queryPool);
}

class TessellationShaderTestInstance : public GraphicBasicTestInstance
{
public:
							TessellationShaderTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic);
protected:
	virtual	void			checkExtensions				(void);
	virtual void			createPipeline				(void);
	virtual tcu::TestStatus	executeTest					(void);
	virtual tcu::TestStatus	checkResult					(VkQueryPool queryPool);
	void					draw						(VkCommandBuffer cmdBuffer);
};

TessellationShaderTestInstance::TessellationShaderTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic)
	: GraphicBasicTestInstance	(context, data, parametersGraphic)
{
}

void TessellationShaderTestInstance::checkExtensions (void)
{
	StatisticQueryTestInstance::checkExtensions();
	if (!m_context.getDeviceFeatures().tessellationShader)
		throw tcu::NotSupportedError("Tessellation shader are not supported");
}


void TessellationShaderTestInstance::createPipeline (void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	// Pipeline
	Unique<VkShaderModule> vs(createShaderModule(vk, device, m_context.getBinaryCollection().get("vertex"), (VkShaderModuleCreateFlags)0));
	Unique<VkShaderModule> tc(createShaderModule(vk, device, m_context.getBinaryCollection().get("tessellation_control"), (VkShaderModuleCreateFlags)0));
	Unique<VkShaderModule> te(createShaderModule(vk, device, m_context.getBinaryCollection().get("tessellation_evaluation"), (VkShaderModuleCreateFlags)0));
	Unique<VkShaderModule> fs(createShaderModule(vk, device, m_context.getBinaryCollection().get("fragment"), (VkShaderModuleCreateFlags)0));

	const PipelineCreateInfo::ColorBlendState::Attachment attachmentState;

	const PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	m_pipelineLayout = createPipelineLayout(vk, device, &pipelineLayoutCreateInfo);

	const VkVertexInputBindingDescription vertexInputBindingDescription		=
	{
		0u,											// binding;
		static_cast<deUint32>(sizeof(VertexData)),	// stride;
		VK_VERTEX_INPUT_RATE_VERTEX					// inputRate
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
	{
		{
			0u,
			0u,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			0u
		},	// VertexElementData::position
		{
			1u,
			0u,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			static_cast<deUint32>(sizeof(tcu::Vec4))
		},	// VertexElementData::color
	};

	const VkPipelineVertexInputStateCreateInfo vf_info			=
	{																	// sType;
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// pNext;
		NULL,															// flags;
		0u,																// vertexBindingDescriptionCount;
		1u,																// pVertexBindingDescriptions;
		&vertexInputBindingDescription,									// vertexAttributeDescriptionCount;
		2u,																// pVertexAttributeDescriptions;
		vertexInputAttributeDescriptions
	};

	PipelineCreateInfo pipelineCreateInfo(*m_pipelineLayout, *m_renderPass, 0, (VkPipelineCreateFlags)0);
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*vs, "main", VK_SHADER_STAGE_VERTEX_BIT));
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*tc, "main", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT));
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*te, "main", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT));
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*fs, "main", VK_SHADER_STAGE_FRAGMENT_BIT));
	pipelineCreateInfo.addState	(PipelineCreateInfo::TessellationState(4));
	pipelineCreateInfo.addState(PipelineCreateInfo::InputAssemblerState(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST));
	pipelineCreateInfo.addState(PipelineCreateInfo::ColorBlendState(1, &attachmentState));

	const VkViewport	viewport	=
	{
		0.0f,		// float x;
		0.0f,		// float y;
		WIDTH,	// float width;
		HEIGHT,	// float height;
		0.0f,	// float minDepth;
		1.0f	// float maxDepth;
	};

	const VkRect2D		scissor		=
	{
		{
			0,		// deInt32 x
			0,		// deInt32 y
		},		// VkOffset2D	offset;
		{
			WIDTH,	// deInt32 width;
			HEIGHT,	// deInt32 height
		},		// VkExtent2D	extent;
	};
	pipelineCreateInfo.addState(PipelineCreateInfo::ViewportState(1, std::vector<VkViewport>(1, viewport), std::vector<VkRect2D>(1, scissor)));
	pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState());
	pipelineCreateInfo.addState(PipelineCreateInfo::RasterizerState());
	pipelineCreateInfo.addState(PipelineCreateInfo::MultiSampleState());
	pipelineCreateInfo.addState(vf_info);
	m_pipeline = createGraphicsPipeline(vk, device, DE_NULL, &pipelineCreateInfo);
}

tcu::TestStatus	TessellationShaderTestInstance::executeTest (void)
{
	const DeviceInterface&					vk						= m_context.getDeviceInterface();
	const VkDevice							device					= m_context.getDevice();
	const VkQueue							queue					= m_context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();

	const CmdPoolCreateInfo					cmdPoolCreateInfo		(queueFamilyIndex);
	const Move<VkCommandPool>				cmdPool					= createCommandPool(vk, device, &cmdPoolCreateInfo);
	const Unique<VkQueryPool>				queryPool				(makeQueryPool(vk, device, m_parametersGraphic.queryStatisticFlags));

	const VkDeviceSize						vertexBufferOffset		= 0u;
	const de::SharedPtr<Buffer>				vertexBufferSp			= creatAndFillVertexBuffer();
	const VkBuffer							vertexBuffer			= vertexBufferSp->object();

	const Unique<VkCommandBuffer>			cmdBuffer				(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	beginCommandBuffer(vk, *cmdBuffer);
	{
		const VkRect2D				renderArea				= { { 0, 0 }, { WIDTH, HEIGHT } };
		std::vector<VkClearValue>	renderPassClearValues	(2);
		deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(VkClearValue));
		const RenderPassBeginInfo	renderPassBegin			(*m_renderPass, *m_framebuffer, renderArea, renderPassClearValues);

		initialTransitionColor2DImage(vk, *cmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		initialTransitionDepth2DImage(vk, *cmdBuffer, m_depthImage->object(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

		vk.cmdResetQueryPool(*cmdBuffer, *queryPool, 0u, 1u);

		vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

		vk.cmdBeginQuery(*cmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
		vk.cmdBindVertexBuffers(*cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

		draw(*cmdBuffer);

		vk.cmdEndQuery(*cmdBuffer, *queryPool, 0u);

		vk.cmdEndRenderPass(*cmdBuffer);

		transition2DImage(vk, *cmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
						  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vk.endCommandBuffer(*cmdBuffer);

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	return checkResult (*queryPool);
}

tcu::TestStatus TessellationShaderTestInstance::checkResult (VkQueryPool queryPool)
{
	const DeviceInterface&	vk			= m_context.getDeviceInterface();
	const VkDevice			device		= m_context.getDevice();
	deUint64				result		= 0u;
	deUint64				expectedMin	= 0u;
	deUint64				expectedMax	= 0u;
	switch(m_parametersGraphic.queryStatisticFlags)
	{
		case VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT:
			expectedMin = 4u;
			expectedMax = expectedMin * 4u;
			break;
		case VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT:
			expectedMin = 100u;
			expectedMax = expectedMin * 4u;
			break;
		default:
			DE_ASSERT(0);
			break;
	}
	VK_CHECK(vk.getQueryPoolResults(device, queryPool, 0u, 1u, sizeof(deUint64), &result, 0u, VK_QUERY_RESULT_64_BIT));
	if (result < expectedMin || result > expectedMax)
		return tcu::TestStatus::fail("QueryPoolResults incorrect");

	if (!checkImage())
		return tcu::TestStatus::fail("Result image doesn't match expected image.");

	return tcu::TestStatus::pass("Pass");
}

void TessellationShaderTestInstance::draw (VkCommandBuffer cmdBuffer)
{
	const DeviceInterface& vk = m_context.getDeviceInterface();
	vk.cmdDraw(cmdBuffer, static_cast<deUint32>(m_data.size()), 1u, 0u, 0u);
}

class TessellationShaderSecondrayTestInstance : public TessellationShaderTestInstance
{
public:
							TessellationShaderSecondrayTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic);
protected:
	virtual tcu::TestStatus	executeTest								(void);
};

TessellationShaderSecondrayTestInstance::TessellationShaderSecondrayTestInstance (vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic)
	: TessellationShaderTestInstance	(context, data, parametersGraphic)
{
}

tcu::TestStatus	TessellationShaderSecondrayTestInstance::executeTest (void)
{
	const DeviceInterface&					vk						= m_context.getDeviceInterface();
	const VkDevice							device					= m_context.getDevice();
	const VkQueue							queue					= m_context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();

	const CmdPoolCreateInfo					cmdPoolCreateInfo		(queueFamilyIndex);
	const Move<VkCommandPool>				cmdPool					= createCommandPool(vk, device, &cmdPoolCreateInfo);
	const Unique<VkQueryPool>				queryPool				(makeQueryPool(vk, device, m_parametersGraphic.queryStatisticFlags));

	const VkDeviceSize						vertexBufferOffset		= 0u;
	const de::SharedPtr<Buffer>				vertexBufferSp			= creatAndFillVertexBuffer();
	const VkBuffer							vertexBuffer			= vertexBufferSp->object();

	const Unique<VkCommandBuffer>			primaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const Unique<VkCommandBuffer>			secondaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY));

	beginSecondaryCommandBuffer(vk, *secondaryCmdBuffer, m_parametersGraphic.queryStatisticFlags, *m_renderPass, *m_framebuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
		vk.cmdBindPipeline(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		vk.cmdBeginQuery(*secondaryCmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);
		vk.cmdBindVertexBuffers(*secondaryCmdBuffer, 0u, 1u, &vertexBuffer, &vertexBufferOffset);
		draw(*secondaryCmdBuffer);
		vk.cmdEndQuery(*secondaryCmdBuffer, *queryPool, 0u);
	vk.endCommandBuffer(*secondaryCmdBuffer);

	beginCommandBuffer(vk, *primaryCmdBuffer);
	{
		const VkRect2D				renderArea				= { { 0, 0 }, { WIDTH, HEIGHT } };
		std::vector<VkClearValue>	renderPassClearValues	(2);
		deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(VkClearValue));
		const RenderPassBeginInfo	renderPassBegin			(*m_renderPass, *m_framebuffer, renderArea, renderPassClearValues);

		initialTransitionColor2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		initialTransitionDepth2DImage(vk, *primaryCmdBuffer, m_depthImage->object(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

		vk.cmdBindVertexBuffers(*primaryCmdBuffer, 0u, 1u, &vertexBuffer, &vertexBufferOffset);
		vk.cmdBindPipeline(*primaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		vk.cmdResetQueryPool(*primaryCmdBuffer, *queryPool, 0u, 1u);

		vk.cmdBeginRenderPass(*primaryCmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		vk.cmdExecuteCommands(*primaryCmdBuffer, 1u, &secondaryCmdBuffer.get());
		vk.cmdEndRenderPass(*primaryCmdBuffer);

		transition2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
						  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vk.endCommandBuffer(*primaryCmdBuffer);

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *primaryCmdBuffer);
	return checkResult (*queryPool);
}

class TessellationShaderSecondrayInheritedTestInstance : public TessellationShaderTestInstance
{
public:
							TessellationShaderSecondrayInheritedTestInstance	(vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic);
protected:
	virtual void			checkExtensions							(void);
	virtual tcu::TestStatus	executeTest								(void);
};

TessellationShaderSecondrayInheritedTestInstance::TessellationShaderSecondrayInheritedTestInstance (vkt::Context& context, const std::vector<VertexData>& data, const ParametersGraphic& parametersGraphic)
	: TessellationShaderTestInstance	(context, data, parametersGraphic)
{
}

void TessellationShaderSecondrayInheritedTestInstance::checkExtensions (void)
{
	TessellationShaderTestInstance::checkExtensions();
	if (!m_context.getDeviceFeatures().inheritedQueries)
		throw tcu::NotSupportedError("Inherited queries are not supported");
}

tcu::TestStatus	TessellationShaderSecondrayInheritedTestInstance::executeTest (void)
{
	const DeviceInterface&					vk						= m_context.getDeviceInterface();
	const VkDevice							device					= m_context.getDevice();
	const VkQueue							queue					= m_context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();

	const CmdPoolCreateInfo					cmdPoolCreateInfo		(queueFamilyIndex);
	const Move<VkCommandPool>				cmdPool					= createCommandPool(vk, device, &cmdPoolCreateInfo);
	const Unique<VkQueryPool>				queryPool				(makeQueryPool(vk, device, m_parametersGraphic.queryStatisticFlags));

	const VkDeviceSize						vertexBufferOffset		= 0u;
	const de::SharedPtr<Buffer>				vertexBufferSp			= creatAndFillVertexBuffer();
	const VkBuffer							vertexBuffer			= vertexBufferSp->object();

	const Unique<VkCommandBuffer>			primaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const Unique<VkCommandBuffer>			secondaryCmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY));

	beginSecondaryCommandBuffer(vk, *secondaryCmdBuffer, m_parametersGraphic.queryStatisticFlags, *m_renderPass, *m_framebuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
		vk.cmdBindPipeline(*secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		vk.cmdBindVertexBuffers(*secondaryCmdBuffer, 0u, 1u, &vertexBuffer, &vertexBufferOffset);
		draw(*secondaryCmdBuffer);
	vk.endCommandBuffer(*secondaryCmdBuffer);

	beginCommandBuffer(vk, *primaryCmdBuffer);
	{
		const VkRect2D				renderArea				= { { 0, 0 }, { WIDTH, HEIGHT } };
		std::vector<VkClearValue>	renderPassClearValues	(2);
		deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(VkClearValue));
		const RenderPassBeginInfo	renderPassBegin			(*m_renderPass, *m_framebuffer, renderArea, renderPassClearValues);

		initialTransitionColor2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		initialTransitionDepth2DImage(vk, *primaryCmdBuffer, m_depthImage->object(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

		vk.cmdBindVertexBuffers(*primaryCmdBuffer, 0u, 1u, &vertexBuffer, &vertexBufferOffset);
		vk.cmdBindPipeline(*primaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		vk.cmdResetQueryPool(*primaryCmdBuffer, *queryPool, 0u, 1u);
		vk.cmdBeginQuery(*primaryCmdBuffer, *queryPool, 0u, (VkQueryControlFlags)0u);

		vk.cmdBeginRenderPass(*primaryCmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		vk.cmdExecuteCommands(*primaryCmdBuffer, 1u, &secondaryCmdBuffer.get());
		vk.cmdEndRenderPass(*primaryCmdBuffer);
		vk.cmdEndQuery(*primaryCmdBuffer, *queryPool, 0u);

		transition2DImage(vk, *primaryCmdBuffer, m_colorAttachmentImage->object(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
						  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vk.endCommandBuffer(*primaryCmdBuffer);

	// Wait for completion
	submitCommandsAndWait(vk, device, queue, *primaryCmdBuffer);
	return checkResult (*queryPool);
}

template<class Instance>
class QueryPoolStatisticsTest : public TestCase
{
public:
	QueryPoolStatisticsTest (tcu::TestContext &context, const char *name, const char *description)
		: TestCase			(context, name, description)
	{
		const tcu::UVec3	localSize[]		=
		{
			tcu::UVec3	(2u,			2u,	2u),
			tcu::UVec3	(1u,			1u,	1u),
			tcu::UVec3	(WIDTH/(7u*3u),	7u,	3u),
		};

		const tcu::UVec3	groupSize[]		=
		{
			tcu::UVec3	(2u,			2u,	2u),
			tcu::UVec3	(WIDTH/(7u*3u),	7u,	3u),
			tcu::UVec3	(1u,			1u,	1u),
		};

		DE_ASSERT(DE_LENGTH_OF_ARRAY(localSize) == DE_LENGTH_OF_ARRAY(groupSize));

		for(int shaderNdx = 0; shaderNdx < DE_LENGTH_OF_ARRAY(localSize); ++shaderNdx)
		{
			std::ostringstream	shaderName;
			shaderName<< "compute_" << shaderNdx;
			const ComputeInvocationsTestInstance::ParametersCompute	prameters	=
			{
				localSize[shaderNdx],
				groupSize[shaderNdx],
				shaderName.str(),
			};
			m_parameters.push_back(prameters);
		}
	}

	vkt::TestInstance* createInstance (vkt::Context& context) const
	{
		return new Instance(context, m_parameters);
	}

	void initPrograms(SourceCollections& sourceCollections) const
	{
		std::ostringstream	source;
		source	<< "layout(binding = 0) writeonly buffer Output {\n"
				<< "	uint values[];\n"
				<< "} sb_out;\n\n"
				<< "void main (void) {\n"
				<< "	uvec3 indexUvec3 = uvec3 (gl_GlobalInvocationID.x,\n"
				<< "	                          gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x,\n"
				<< "	                          gl_GlobalInvocationID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_WorkGroupSize.x * gl_WorkGroupSize.y);\n"
				<< "	uint index = indexUvec3.x + indexUvec3.y + indexUvec3.z;\n"
				<< "	sb_out.values[index] += index;\n"
				<< "}\n";

		for(size_t shaderNdx = 0; shaderNdx < m_parameters.size(); ++shaderNdx)
		{
			std::ostringstream	src;
			src	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450)<<"\n"
				<< "layout (local_size_x = " << m_parameters[shaderNdx].localSize.x() << ", local_size_y = " << m_parameters[shaderNdx].localSize.y() << ", local_size_z = " << m_parameters[shaderNdx].localSize.z() << ") in;\n"
				<< source.str();
			sourceCollections.glslSources.add(m_parameters[shaderNdx].shaderName) << glu::ComputeSource(src.str());
		}
	}
private:
	std::vector<ComputeInvocationsTestInstance::ParametersCompute>	m_parameters;
};

template<class Instance>
class QueryPoolGraphicStatisticsTest : public TestCase
{
public:
	QueryPoolGraphicStatisticsTest (tcu::TestContext &context, const char *name, const char *description, const GraphicBasicTestInstance::ParametersGraphic parametersGraphic)
		: TestCase				(context, name, description)
		, m_parametersGraphic	(parametersGraphic)
	{
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4(-1.0f,-1.0f, 1.0f, 1.0f), tcu::RGBA::red().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4(-1.0f, 0.0f, 1.0f, 1.0f), tcu::RGBA::red().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 0.0f,-1.0f, 1.0f, 1.0f), tcu::RGBA::red().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 0.0f, 0.0f, 1.0f, 1.0f), tcu::RGBA::red().toVec()));

		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4(-1.0f, 0.0f, 1.0f, 1.0f), tcu::RGBA::green().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4(-1.0f, 1.0f, 1.0f, 1.0f), tcu::RGBA::green().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 0.0f, 0.0f, 1.0f, 1.0f), tcu::RGBA::green().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 0.0f, 1.0f, 1.0f, 1.0f), tcu::RGBA::green().toVec()));

		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 0.0f,-1.0f, 1.0f, 1.0f), tcu::RGBA::blue().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 0.0f, 0.0f, 1.0f, 1.0f), tcu::RGBA::blue().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 1.0f,-1.0f, 1.0f, 1.0f), tcu::RGBA::blue().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 1.0f, 0.0f, 1.0f, 1.0f), tcu::RGBA::blue().toVec()));

		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 0.0f, 0.0f, 1.0f, 1.0f), tcu::RGBA::gray().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 0.0f, 1.0f, 1.0f, 1.0f), tcu::RGBA::gray().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 1.0f, 0.0f, 1.0f, 1.0f), tcu::RGBA::gray().toVec()));
		m_data.push_back(GraphicBasicTestInstance::VertexData(tcu::Vec4( 1.0f, 1.0f, 1.0f, 1.0f), tcu::RGBA::gray().toVec()));
	}

	vkt::TestInstance* createInstance (vkt::Context& context) const
	{
		return new Instance(context, m_data, m_parametersGraphic);
	}

	void initPrograms(SourceCollections& sourceCollections) const
	{
		{ // Vertex Shader
			std::ostringstream	source;
			source	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450)<<"\n"
					<< "layout(location = 0) in highp vec4 in_position;\n"
					<< "layout(location = 1) in vec4 in_color;\n"
					<< "layout(location = 0) out vec4 out_color;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = in_position;\n"
					<< "	out_color = in_color;\n"
					<< "}\n";
			sourceCollections.glslSources.add("vertex") << glu::VertexSource(source.str());
		}

		if (m_parametersGraphic.queryStatisticFlags & (VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT|
									VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT))
		{// Tessellation control & evaluation
			std::ostringstream source_tc;
			source_tc	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
						<< "#extension GL_EXT_tessellation_shader : require\n"
						<< "layout(vertices = 4) out;\n"
						<< "layout(location = 0) in vec4 in_color[];\n"
						<< "layout(location = 0) out vec4 out_color[];\n"
						<< "\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	if( gl_InvocationID == 0 )\n"
						<< "	{\n"
						<< "		gl_TessLevelInner[0] = 4.0f;\n"
						<< "		gl_TessLevelInner[1] = 4.0f;\n"
						<< "		gl_TessLevelOuter[0] = 4.0f;\n"
						<< "		gl_TessLevelOuter[1] = 4.0f;\n"
						<< "		gl_TessLevelOuter[2] = 4.0f;\n"
						<< "		gl_TessLevelOuter[3] = 4.0f;\n"
						<< "	}\n"
						<< "	out_color[gl_InvocationID] = in_color[gl_InvocationID];\n"
						<< "	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
						<< "}\n";
			sourceCollections.glslSources.add("tessellation_control") << glu::TessellationControlSource(source_tc.str());

			std::ostringstream source_te;
			source_te	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
						<< "#extension GL_EXT_tessellation_shader : require\n"
						<< "layout( quads, equal_spacing, ccw ) in;\n"
						<< "layout(location = 0) in vec4 in_color[];\n"
						<< "layout(location = 0) out vec4 out_color;\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	const float u = gl_TessCoord.x;\n"
						<< "	const float v = gl_TessCoord.y;\n"
						<< "	const float w = gl_TessCoord.z;\n"
						<< "	gl_Position = (1 - u) * (1 - v) * gl_in[0].gl_Position +(1 - u) * v * gl_in[1].gl_Position + u * (1 - v) * gl_in[2].gl_Position + u * v * gl_in[3].gl_Position;\n"
						<< "	out_color = in_color[0];\n"
						<< "}\n";
			sourceCollections.glslSources.add("tessellation_evaluation") << glu::TessellationEvaluationSource(source_te.str());
		}

		if(m_parametersGraphic.queryStatisticFlags & (VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
									VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT))
		{ // Geometry Shader
			std::ostringstream	source;
			source	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450)<<"\n"
					<< "layout("<<inputTypeToGLString(m_parametersGraphic.primitiveTopology)<<") in;\n"
					<< "layout("<<outputTypeToGLString (m_parametersGraphic.primitiveTopology)<<", max_vertices = 16) out;\n"
					<< "layout(location = 0) in vec4 in_color[];\n"
					<< "layout(location = 0) out vec4 out_color;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	out_color = in_color[0];\n"
					<< "	gl_Position = gl_in[0].gl_Position;\n"
					<< "	EmitVertex();\n"
					<< "	EndPrimitive();\n"
					<< "\n"
					<< "	out_color = in_color[0];\n"
					<< "	gl_Position = vec4(1.0, 1.0, 1.0, 1.0);\n"
					<< "	EmitVertex();\n"
					<< "	out_color = in_color[0];\n"
					<< "	gl_Position = vec4(-1.0, -1.0, 1.0, 1.0);\n"
					<< "	EmitVertex();\n"
					<< "	EndPrimitive();\n"
					<< "\n";
			if (m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ||
				m_parametersGraphic.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			{
				source	<< "\n"
						<< "	out_color = in_color[0];\n"
						<< "	gl_Position = gl_in[0].gl_Position;\n"
						<< "	EmitVertex();\n"
						<< "	out_color = in_color[0];\n"
						<< "	gl_Position = gl_in[1].gl_Position;\n"
						<< "	EmitVertex();\n"
						<< "	out_color = in_color[0];\n"
						<< "	gl_Position = gl_in[2].gl_Position;\n"
						<< "	EmitVertex();\n"
						<< "	out_color = in_color[0];\n"
						<< "	gl_Position = vec4(gl_in[2].gl_Position.x, gl_in[1].gl_Position.y, 1.0, 1.0);\n"
						<< "	EmitVertex();\n"
						<< "	EndPrimitive();\n";
			}
			else
			{
				source	<< "	out_color = in_color[0];\n"
						<< "	gl_Position =  vec4(1.0, 1.0, 1.0, 1.0);\n"
						<< "	EmitVertex();\n"
						<< "	out_color = in_color[0];\n"
						<< "	gl_Position = vec4(1.0, -1.0, 1.0, 1.0);\n"
						<< "	EmitVertex();\n"
						<< "	out_color = in_color[0];\n"
						<< "	gl_Position = vec4(-1.0, 1.0, 1.0, 1.0);\n"
						<< "	EmitVertex();\n"
						<< "	out_color = in_color[0];\n"
						<< "	gl_Position = vec4(-1.0, -1.0, 1.0, 1.0);\n"
						<< "	EmitVertex();\n"
						<< "	EndPrimitive();\n";
			}
			source	<< "}\n";
			sourceCollections.glslSources.add("geometry") << glu::GeometrySource(source.str());
		}

		{ // Fragment Shader
			std::ostringstream	source;
			source	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450)<<"\n"
					<< "layout(location = 0) in vec4 in_color;\n"
					<< "layout(location = 0) out vec4 out_color;\n"
					<< "void main()\n"
					<<"{\n"
					<< "	out_color = in_color;\n"
					<< "}\n";
			sourceCollections.glslSources.add("fragment") << glu::FragmentSource(source.str());
		}
	}
private:
	std::vector<GraphicBasicTestInstance::VertexData>	m_data;
	const GraphicBasicTestInstance::ParametersGraphic	m_parametersGraphic;
};
} //anonymous

QueryPoolStatisticsTests::QueryPoolStatisticsTests (tcu::TestContext &testCtx)
	: TestCaseGroup(testCtx, "statistics_query", "Tests for statistics queries")
{
}

void QueryPoolStatisticsTests::init (void)
{
	std::string topology_name [VK_PRIMITIVE_TOPOLOGY_LAST] =
	{
		"point_list",
		"line_list",
		"line_strip",
		"triangle_list",
		"triangle_strip",
		"triangle_fan",
		"line_list_with_adjacency",
		"line_strip_with_adjacency",
		"triangle_list_with_adjacency",
		"triangle_strip_with_adjacency",
		"patch_list"
	};

	de::MovePtr<TestCaseGroup>	computeShaderInvocationsGroup		(new TestCaseGroup(m_testCtx, "compute_shader_invocations",			"Query pipeline statistic compute shader invocations"));
	de::MovePtr<TestCaseGroup>	inputAssemblyVertices				(new TestCaseGroup(m_testCtx, "input_assembly_vertices",			"Query pipeline statistic input assembly vertices"));
	de::MovePtr<TestCaseGroup>	inputAssemblyPrimitives				(new TestCaseGroup(m_testCtx, "input_assembly_primitives",			"Query pipeline statistic input assembly primitives"));
	de::MovePtr<TestCaseGroup>	vertexShaderInvocations				(new TestCaseGroup(m_testCtx, "vertex_shader_invocations",			"Query pipeline statistic vertex shader invocation"));
	de::MovePtr<TestCaseGroup>	fragmentShaderInvocations			(new TestCaseGroup(m_testCtx, "fragment_shader_invocations",		"Query pipeline statistic fragment shader invocation"));
	de::MovePtr<TestCaseGroup>	geometryShaderInvocations			(new TestCaseGroup(m_testCtx, "geometry_shader_invocations",		"Query pipeline statistic geometry shader invocation"));
	de::MovePtr<TestCaseGroup>	geometryShaderPrimitives			(new TestCaseGroup(m_testCtx, "geometry_shader_primitives",			"Query pipeline statistic geometry shader primitives"));
	de::MovePtr<TestCaseGroup>	clippingInvocations					(new TestCaseGroup(m_testCtx, "clipping_invocations",				"Query pipeline statistic clipping invocations"));
	de::MovePtr<TestCaseGroup>	clippingPrimitives					(new TestCaseGroup(m_testCtx, "clipping_primitives",				"Query pipeline statistic clipping primitives"));
	de::MovePtr<TestCaseGroup>	tesControlPatches					(new TestCaseGroup(m_testCtx, "tes_control_patches",				"Query pipeline statistic tessellation control shader patches"));
	de::MovePtr<TestCaseGroup>	tesEvaluationShaderInvocations		(new TestCaseGroup(m_testCtx, "tes_evaluation_shader_invocations",	"Query pipeline statistic tessellation evaluation shader invocations"));

	computeShaderInvocationsGroup->addChild(new QueryPoolStatisticsTest<ComputeInvocationsTestInstance>						(m_testCtx, "primary",				""));
	computeShaderInvocationsGroup->addChild(new QueryPoolStatisticsTest<ComputeInvocationsSecondaryTestInstance>			(m_testCtx, "secondary",			""));
	computeShaderInvocationsGroup->addChild(new QueryPoolStatisticsTest<ComputeInvocationsSecondaryInheritedTestInstance>	(m_testCtx, "secondary_inherited",	""));

	//VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT
	inputAssemblyVertices->addChild(new QueryPoolGraphicStatisticsTest<VertexShaderTestInstance>					(m_testCtx,	"primary",				"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)));
	inputAssemblyVertices->addChild(new QueryPoolGraphicStatisticsTest<VertexShaderSecondaryTestInstance>			(m_testCtx,	"secondary",			"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)));
	inputAssemblyVertices->addChild(new QueryPoolGraphicStatisticsTest<VertexShaderSecondaryInheritedTestInstance>	(m_testCtx,	"secondary_inherited",	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)));

	//VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT
	{
		de::MovePtr<TestCaseGroup>	primary				(new TestCaseGroup(m_testCtx, "primary",			""));
		de::MovePtr<TestCaseGroup>	secondary			(new TestCaseGroup(m_testCtx, "secondary",			""));
		de::MovePtr<TestCaseGroup>	secondaryInherited	(new TestCaseGroup(m_testCtx, "secondary_inherited",""));
		for (int topologyNdx = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; topologyNdx < VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; ++topologyNdx)
		{
			primary->addChild			(new QueryPoolGraphicStatisticsTest<VertexShaderTestInstance>					(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondary->addChild			(new QueryPoolGraphicStatisticsTest<VertexShaderSecondaryTestInstance>			(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondaryInherited->addChild(new QueryPoolGraphicStatisticsTest<VertexShaderSecondaryInheritedTestInstance>	(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT, (VkPrimitiveTopology)topologyNdx)));
		}
		inputAssemblyPrimitives->addChild(primary.release());
		inputAssemblyPrimitives->addChild(secondary.release());
		inputAssemblyPrimitives->addChild(secondaryInherited.release());
	}

	//VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT
	{
		de::MovePtr<TestCaseGroup>	primary				(new TestCaseGroup(m_testCtx, "primary",			""));
		de::MovePtr<TestCaseGroup>	secondary			(new TestCaseGroup(m_testCtx, "secondary",			""));
		de::MovePtr<TestCaseGroup>	secondaryInherited	(new TestCaseGroup(m_testCtx, "secondary_inherited",""));
		for (int topologyNdx = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; topologyNdx < VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; ++topologyNdx)
		{
			primary->addChild			(new QueryPoolGraphicStatisticsTest<VertexShaderTestInstance>					(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondary->addChild			(new QueryPoolGraphicStatisticsTest<VertexShaderSecondaryTestInstance>			(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondaryInherited->addChild(new QueryPoolGraphicStatisticsTest<VertexShaderSecondaryInheritedTestInstance>	(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
		}
		vertexShaderInvocations->addChild(primary.release());
		vertexShaderInvocations->addChild(secondary.release());
		vertexShaderInvocations->addChild(secondaryInherited.release());
	}

	//VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT
	{
		de::MovePtr<TestCaseGroup>	primary				(new TestCaseGroup(m_testCtx, "primary",			""));
		de::MovePtr<TestCaseGroup>	secondary			(new TestCaseGroup(m_testCtx, "secondary",			""));
		de::MovePtr<TestCaseGroup>	secondaryInherited	(new TestCaseGroup(m_testCtx, "secondary_inherited",""));
		for (int topologyNdx = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; topologyNdx < VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; ++topologyNdx)
		{
			primary->addChild			(new QueryPoolGraphicStatisticsTest<VertexShaderTestInstance>					(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondary->addChild			(new QueryPoolGraphicStatisticsTest<VertexShaderSecondaryTestInstance>			(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondaryInherited->addChild(new QueryPoolGraphicStatisticsTest<VertexShaderSecondaryInheritedTestInstance>	(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
		}
		fragmentShaderInvocations->addChild(primary.release());
		fragmentShaderInvocations->addChild(secondary.release());
		fragmentShaderInvocations->addChild(secondaryInherited.release());
	}

	//VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT
	{
		de::MovePtr<TestCaseGroup>	primary				(new TestCaseGroup(m_testCtx, "primary",			""));
		de::MovePtr<TestCaseGroup>	secondary			(new TestCaseGroup(m_testCtx, "secondary",			""));
		de::MovePtr<TestCaseGroup>	secondaryInherited	(new TestCaseGroup(m_testCtx, "secondary_inherited",""));
		for (int topologyNdx = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; topologyNdx < VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; ++topologyNdx)
		{
			primary->addChild			(new QueryPoolGraphicStatisticsTest<GeometryShaderTestInstance>						(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondary->addChild			(new QueryPoolGraphicStatisticsTest<GeometryShaderSecondaryTestInstance>			(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondaryInherited->addChild(new QueryPoolGraphicStatisticsTest<GeometryShaderSecondaryInheritedTestInstance>	(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
		}
		geometryShaderInvocations->addChild(primary.release());
		geometryShaderInvocations->addChild(secondary.release());
		geometryShaderInvocations->addChild(secondaryInherited.release());
	}

	//VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT
	{
		de::MovePtr<TestCaseGroup>	primary				(new TestCaseGroup(m_testCtx, "primary",			""));
		de::MovePtr<TestCaseGroup>	secondary			(new TestCaseGroup(m_testCtx, "secondary",			""));
		de::MovePtr<TestCaseGroup>	secondaryInherited	(new TestCaseGroup(m_testCtx, "secondary_inherited",""));
		for (int topologyNdx = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; topologyNdx < VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; ++topologyNdx)
		{
			primary->addChild			(new QueryPoolGraphicStatisticsTest<GeometryShaderTestInstance>						(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondary->addChild			(new QueryPoolGraphicStatisticsTest<GeometryShaderSecondaryTestInstance>			(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondaryInherited->addChild(new QueryPoolGraphicStatisticsTest<GeometryShaderSecondaryInheritedTestInstance>	(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT, (VkPrimitiveTopology)topologyNdx)));
		}
		geometryShaderPrimitives->addChild(primary.release());
		geometryShaderPrimitives->addChild(secondary.release());
		geometryShaderPrimitives->addChild(secondaryInherited.release());
	}

	//VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT
	{
		de::MovePtr<TestCaseGroup>	primary				(new TestCaseGroup(m_testCtx, "primary",			""));
		de::MovePtr<TestCaseGroup>	secondary			(new TestCaseGroup(m_testCtx, "secondary",			""));
		de::MovePtr<TestCaseGroup>	secondaryInherited	(new TestCaseGroup(m_testCtx, "secondary_inherited",""));
		for (int topologyNdx = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; topologyNdx < VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; ++topologyNdx)
		{
			primary->addChild			(new QueryPoolGraphicStatisticsTest<GeometryShaderTestInstance>						(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondary->addChild			(new QueryPoolGraphicStatisticsTest<GeometryShaderSecondaryTestInstance>			(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondaryInherited->addChild(new QueryPoolGraphicStatisticsTest<GeometryShaderSecondaryInheritedTestInstance>	(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT, (VkPrimitiveTopology)topologyNdx)));
		}
		clippingInvocations->addChild(primary.release());
		clippingInvocations->addChild(secondary.release());
		clippingInvocations->addChild(secondaryInherited.release());
	}

	//VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT
	{
		de::MovePtr<TestCaseGroup>	primary				(new TestCaseGroup(m_testCtx, "primary",			""));
		de::MovePtr<TestCaseGroup>	secondary			(new TestCaseGroup(m_testCtx, "secondary",			""));
		de::MovePtr<TestCaseGroup>	secondaryInherited	(new TestCaseGroup(m_testCtx, "secondary_inherited",""));
		for (int topologyNdx = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; topologyNdx < VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; ++topologyNdx)
		{
			primary->addChild			(new QueryPoolGraphicStatisticsTest<GeometryShaderTestInstance>						(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondary->addChild			(new QueryPoolGraphicStatisticsTest<GeometryShaderSecondaryTestInstance>			(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT, (VkPrimitiveTopology)topologyNdx)));
			secondaryInherited->addChild(new QueryPoolGraphicStatisticsTest<GeometryShaderSecondaryInheritedTestInstance>	(m_testCtx,	topology_name[topologyNdx].c_str(),	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT, (VkPrimitiveTopology)topologyNdx)));
		}
		clippingPrimitives->addChild(primary.release());
		clippingPrimitives->addChild(secondary.release());
		clippingPrimitives->addChild(secondaryInherited.release());
	}

	//VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT
	tesControlPatches->addChild(new QueryPoolGraphicStatisticsTest<TessellationShaderTestInstance>					(m_testCtx,	"tes_control_patches",						"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)));
	tesControlPatches->addChild(new QueryPoolGraphicStatisticsTest<TessellationShaderSecondrayTestInstance>			(m_testCtx,	"tes_control_patches_secondary",			"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)));
	tesControlPatches->addChild(new QueryPoolGraphicStatisticsTest<TessellationShaderSecondrayInheritedTestInstance>(m_testCtx,	"tes_control_patches_secondary_inherited",	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)));

	//VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT
	tesEvaluationShaderInvocations->addChild(new QueryPoolGraphicStatisticsTest<TessellationShaderTestInstance>					 (m_testCtx,	"tes_evaluation_shader_invocations",						"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)));
	tesEvaluationShaderInvocations->addChild(new QueryPoolGraphicStatisticsTest<TessellationShaderSecondrayTestInstance>		 (m_testCtx,	"tes_evaluation_shader_invocations_secondary",				"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)));
	tesEvaluationShaderInvocations->addChild(new QueryPoolGraphicStatisticsTest<TessellationShaderSecondrayInheritedTestInstance>(m_testCtx,	"tes_evaluation_shader_invocations_secondary_inherited",	"",	GraphicBasicTestInstance::ParametersGraphic(VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)));

	addChild(computeShaderInvocationsGroup.release());
	addChild(inputAssemblyVertices.release());
	addChild(inputAssemblyPrimitives.release());
	addChild(vertexShaderInvocations.release());
	addChild(fragmentShaderInvocations.release());
	addChild(geometryShaderInvocations.release());
	addChild(geometryShaderPrimitives.release());
	addChild(clippingInvocations.release());
	addChild(clippingPrimitives.release());
	addChild(tesControlPatches.release());
	addChild(tesEvaluationShaderInvocations.release());
}

} //QueryPool
} //vkt
