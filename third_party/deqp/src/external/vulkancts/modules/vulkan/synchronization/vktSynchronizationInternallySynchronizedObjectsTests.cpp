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
 * \brief Synchronization internally synchronized objects tests
 *//*--------------------------------------------------------------------*/

#include "vktSynchronizationInternallySynchronizedObjectsTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktSynchronizationUtil.hpp"

#include "vkRef.hpp"
#include "tcuDefs.hpp"
#include "vkTypeUtil.hpp"
#include "vkPlatform.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"

#include "tcuResultCollector.hpp"

#include "deThread.hpp"
#include "deMutex.hpp"
#include "deSharedPtr.hpp"


#include <limits>
#include <iterator>

namespace vkt
{
namespace synchronization
{
namespace
{
using namespace vk;

using std::vector;
using std::string;
using std::map;
using std::exception;
using std::ostringstream;

using tcu::TestStatus;
using tcu::TestContext;
using tcu::ResultCollector;
using tcu::TestException;

using de::UniquePtr;
using de::MovePtr;
using de::SharedPtr;
using de::Mutex;
using de::Thread;
using de::clamp;

enum {EXECUTION_PER_THREAD = 100, BUFFER_ELEMENT_COUNT = 16, BUFFER_SIZE = BUFFER_ELEMENT_COUNT*4 };

class MultiQueues
{
	typedef struct QueueType
	{
		vector<VkQueue>	queues;
		vector<bool>	available;
	} Queues;

public:

	inline void		addQueueFamilyIndex		(const deUint32& queueFamilyIndex, const deUint32& count)
	{
		Queues temp;
		vector<bool>::iterator it;
		it = temp.available.begin();
		temp.available.insert(it, count, false);

		temp.queues.resize(count);
		m_queues[queueFamilyIndex] = temp;
	}

	const deUint32& getQueueFamilyIndex		(const int index)
	{
		map<deUint32,Queues>::iterator it = m_queues.begin();
		advance (it, index);
		return it->first;
	}

	inline size_t	countQueueFamilyIndex	(void)
	{
		return m_queues.size();
	}

	Queues &		getQueues				(const int index)
	{
		map<deUint32,Queues>::iterator it = m_queues.begin();
		advance (it, index);
		return it->second;
	}

	bool			getFreeQueue			(deUint32& returnQueueFamilyIndex, VkQueue& returnQueues, int& returnQueueIndex)
	{
		for (int queueFamilyIndexNdx = 0 ; queueFamilyIndexNdx < static_cast<int>(m_queues.size()); ++queueFamilyIndexNdx)
		{
			Queues& queue = m_queues[getQueueFamilyIndex(queueFamilyIndexNdx)];
			for (int queueNdx = 0; queueNdx < static_cast<int>(queue.queues.size()); ++queueNdx)
			{
				m_mutex.lock();
				if (queue.available[queueNdx])
				{
					queue.available[queueNdx]	= false;
					returnQueueFamilyIndex		= getQueueFamilyIndex(queueFamilyIndexNdx);
					returnQueues				= queue.queues[queueNdx];
					returnQueueIndex			= queueNdx;
					m_mutex.unlock();
					return true;
				}
				m_mutex.unlock();
			}
		}
		return false;
	}

	void			releaseQueue			(const deUint32& queueFamilyIndex, const int& queueIndex)
	{
		m_mutex.lock();
		m_queues[queueFamilyIndex].available[queueIndex] = true;
		m_mutex.unlock();
	}

	inline void		setDevice				(Move<VkDevice> device)
	{
		m_logicalDevice = device;
	}

	inline VkDevice	getDevice				(void)
	{
		return *m_logicalDevice;
	}

	MovePtr<Allocator>		m_allocator;
protected:
	Move<VkDevice>			m_logicalDevice;
	map<deUint32,Queues>	m_queues;
	Mutex					m_mutex;

};

MovePtr<Allocator> createAllocator (const Context& context, const VkDevice& device)
{
	const DeviceInterface&					deviceInterface			= context.getDeviceInterface();
	const InstanceInterface&				instance				= context.getInstanceInterface();
	const VkPhysicalDevice					physicalDevice			= context.getPhysicalDevice();
	const VkPhysicalDeviceMemoryProperties	deviceMemoryProperties	= getPhysicalDeviceMemoryProperties(instance, physicalDevice);

	// Create memory allocator for device
	return MovePtr<Allocator> (new SimpleAllocator(deviceInterface, device, deviceMemoryProperties));
}

bool checkQueueFlags (const VkQueueFlags& availableFlag, const VkQueueFlags& neededFlag)
{
	if (VK_QUEUE_TRANSFER_BIT == neededFlag)
	{
		if ( (availableFlag & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT ||
			 (availableFlag & VK_QUEUE_COMPUTE_BIT)  == VK_QUEUE_COMPUTE_BIT  ||
			 (availableFlag & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT
		   )
			return true;
	}
	else if ((availableFlag & neededFlag) == neededFlag)
	{
		return true;
	}
	return false;
}

MovePtr<MultiQueues> createQueues (const Context& context, const VkQueueFlags& queueFlag)
{
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const InstanceInterface&				instance				= context.getInstanceInterface();
	const VkPhysicalDevice					physicalDevice			= context.getPhysicalDevice();
	MovePtr<MultiQueues>					moveQueues				(new MultiQueues());
	MultiQueues&							queues					= *moveQueues;
	VkDeviceCreateInfo						deviceInfo;
	VkPhysicalDeviceFeatures				deviceFeatures;
	vector<VkQueueFamilyProperties>			queueFamilyProperties;
	vector<float>							queuePriorities;
	vector<VkDeviceQueueCreateInfo>			queueInfos;

	queueFamilyProperties = getPhysicalDeviceQueueFamilyProperties(instance, physicalDevice);

	for (deUint32 queuePropertiesNdx = 0; queuePropertiesNdx < queueFamilyProperties.size(); ++queuePropertiesNdx)
	{
		if (checkQueueFlags(queueFamilyProperties[queuePropertiesNdx].queueFlags, queueFlag))
		{
			queues.addQueueFamilyIndex(queuePropertiesNdx, queueFamilyProperties[queuePropertiesNdx].queueCount);
		}
	}

	if (queues.countQueueFamilyIndex() == 0)
	{
		TCU_THROW(NotSupportedError, "Queue not found");
	}

	{
		vector<float>::iterator it				= queuePriorities.begin();
		unsigned int			maxQueueCount	= 0;
		for (int queueFamilyIndexNdx = 0; queueFamilyIndexNdx < static_cast<int>(queues.countQueueFamilyIndex()); ++queueFamilyIndexNdx)
		{
			if (queues.getQueues(queueFamilyIndexNdx).queues.size() > maxQueueCount)
				maxQueueCount = static_cast<unsigned int>(queues.getQueues(queueFamilyIndexNdx).queues.size());
		}
		queuePriorities.insert(it, maxQueueCount, 1.0);
	}

	for (int queueFamilyIndexNdx = 0; queueFamilyIndexNdx < static_cast<int>(queues.countQueueFamilyIndex()); ++queueFamilyIndexNdx)
	{
		VkDeviceQueueCreateInfo	queueInfo;
		const deUint32			queueCount	= static_cast<deUint32>(queues.getQueues(queueFamilyIndexNdx).queues.size());

		deMemset(&queueInfo, 0, sizeof(queueInfo));

		queueInfo.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pNext				= DE_NULL;
		queueInfo.flags				= (VkDeviceQueueCreateFlags)0u;
		queueInfo.queueFamilyIndex	= queues.getQueueFamilyIndex(queueFamilyIndexNdx);
		queueInfo.queueCount		= queueCount;
		queueInfo.pQueuePriorities	= &queuePriorities[0];

		queueInfos.push_back(queueInfo);
	}

	deMemset(&deviceInfo, 0, sizeof(deviceInfo));
	instance.getPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	deviceInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext					= DE_NULL;
	deviceInfo.enabledExtensionCount	= 0u;
	deviceInfo.ppEnabledExtensionNames	= DE_NULL;
	deviceInfo.enabledLayerCount		= 0u;
	deviceInfo.ppEnabledLayerNames		= DE_NULL;
	deviceInfo.pEnabledFeatures			= &deviceFeatures;
	deviceInfo.queueCreateInfoCount		= static_cast<deUint32>(queues.countQueueFamilyIndex());
	deviceInfo.pQueueCreateInfos		= &queueInfos[0];

	queues.setDevice(createDevice(instance, physicalDevice, &deviceInfo));

	for (deUint32 queueFamilyIndex = 0; queueFamilyIndex < queues.countQueueFamilyIndex(); ++queueFamilyIndex)
	{
		for (deUint32 queueReqNdx = 0; queueReqNdx < queues.getQueues(queueFamilyIndex).queues.size(); ++queueReqNdx)
		{
			vk.getDeviceQueue(queues.getDevice(), queues.getQueueFamilyIndex(queueFamilyIndex), queueReqNdx, &queues.getQueues(queueFamilyIndex).queues[queueReqNdx]);
			queues.getQueues(queueFamilyIndex).available[queueReqNdx]=true;
		}
	}

	queues.m_allocator = createAllocator(context, queues.getDevice());
	return moveQueues;
}

Move<VkRenderPass>	createRenderPass (const Context& context, const VkDevice& device, const VkFormat& colorFormat)
{
	const DeviceInterface&			vk							= context.getDeviceInterface();
	const VkAttachmentDescription	colorAttachmentDescription	=
																{
																	0u,											// VkAttachmentDescriptionFlags	flags;
																	colorFormat,								// VkFormat						format;
																	VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits		samples;
																	VK_ATTACHMENT_LOAD_OP_CLEAR,				// VkAttachmentLoadOp			loadOp;
																	VK_ATTACHMENT_STORE_OP_STORE,				// VkAttachmentStoreOp			storeOp;
																	VK_ATTACHMENT_LOAD_OP_DONT_CARE,			// VkAttachmentLoadOp			stencilLoadOp;
																	VK_ATTACHMENT_STORE_OP_DONT_CARE,			// VkAttachmentStoreOp			stencilStoreOp;
																	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout				initialLayout;
																	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout				finalLayout;
																};
	const VkAttachmentReference		colorAttachmentReference	=
																{
																	0u,											// deUint32			attachment;
																	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
																};
	const VkSubpassDescription		subpassDescription			=
																{
																	0u,									// VkSubpassDescriptionFlags	flags;
																	VK_PIPELINE_BIND_POINT_GRAPHICS,	// VkPipelineBindPoint			pipelineBindPoint;
																	0u,									// deUint32						inputAttachmentCount;
																	DE_NULL,							// const VkAttachmentReference*	pInputAttachments;
																	1u,									// deUint32						colorAttachmentCount;
																	&colorAttachmentReference,			// const VkAttachmentReference*	pColorAttachments;
																	DE_NULL,							// const VkAttachmentReference*	pResolveAttachments;
																	DE_NULL,							// const VkAttachmentReference*	pDepthStencilAttachment;
																	0u,									// deUint32						preserveAttachmentCount;
																	DE_NULL								// const VkAttachmentReference*	pPreserveAttachments;
																};
	const VkRenderPassCreateInfo	renderPassParams			=
																{
																	VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// VkStructureType					sType;
																	DE_NULL,									// const void*						pNext;
																	0u,											// VkRenderPassCreateFlags			flags;
																	1u,											// deUint32							attachmentCount;
																	&colorAttachmentDescription,				// const VkAttachmentDescription*	pAttachments;
																	1u,											// deUint32							subpassCount;
																	&subpassDescription,						// const VkSubpassDescription*		pSubpasses;
																	0u,											// deUint32							dependencyCount;
																	DE_NULL										// const VkSubpassDependency*		pDependencies;
																};
	return createRenderPass(vk, device, &renderPassParams);
}

TestStatus executeComputePipeline (const Context& context, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout,
									const VkDescriptorSetLayout& descriptorSetLayout, MultiQueues& queues, const deUint32& shadersExecutions)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= queues.getDevice();
	deUint32						queueFamilyIndex;
	VkQueue							queue;
	int								queueIndex;
	while(!queues.getFreeQueue(queueFamilyIndex, queue, queueIndex)){}

	{
		const Unique<VkDescriptorPool>	descriptorPool		(DescriptorPoolBuilder()
																.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
																.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));
		Buffer							resultBuffer		(vk, device, *queues.m_allocator, makeBufferCreateInfo(BUFFER_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);
		const VkBufferMemoryBarrier		bufferBarrier		= makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *resultBuffer, 0ull, BUFFER_SIZE);
		const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Unique<VkCommandBuffer>	cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));

		{
			const Allocation& alloc = resultBuffer.getAllocation();
			deMemset(alloc.getHostPtr(), 0, BUFFER_SIZE);
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), BUFFER_SIZE);
		}

		// Start recording commands
		beginCommandBuffer(vk, *cmdBuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

		// Create descriptor set
		const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, descriptorSetLayout));

		const VkDescriptorBufferInfo resultDescriptorInfo = makeDescriptorBufferInfo(*resultBuffer, 0ull, BUFFER_SIZE);

		DescriptorSetUpdateBuilder()
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultDescriptorInfo)
			.update(vk, device);

		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		// Dispatch indirect compute command
		vk.cmdDispatch(*cmdBuffer, shadersExecutions, 1u, 1u);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0,
								 0, (const VkMemoryBarrier*)DE_NULL,
								 1, &bufferBarrier,
								 0, (const VkImageMemoryBarrier*)DE_NULL);

		// End recording commands
		endCommandBuffer(vk, *cmdBuffer);

		// Wait for command buffer execution finish
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
		queues.releaseQueue(queueFamilyIndex, queueIndex);

		{
			const Allocation& resultAlloc = resultBuffer.getAllocation();
			invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), BUFFER_SIZE);

			const deInt32*	ptr = reinterpret_cast<deInt32*>(resultAlloc.getHostPtr());
			for (deInt32 ndx = 0; ndx < BUFFER_ELEMENT_COUNT; ++ndx)
			{
				if (ptr[ndx] != ndx)
				{
					return TestStatus::fail("The data don't match");
				}
			}
		}
		return TestStatus::pass("Passed");
	}
}


TestStatus executeGraphicPipeline (const Context& context, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout,
									const VkDescriptorSetLayout& descriptorSetLayout, MultiQueues& queues, const VkRenderPass& renderPass, const deUint32 shadersExecutions)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= queues.getDevice();
	deUint32						queueFamilyIndex;
	VkQueue							queue;
	int								queueIndex;
	while(!queues.getFreeQueue(queueFamilyIndex, queue, queueIndex)){}

	{
		const Unique<VkDescriptorPool>	descriptorPool				(DescriptorPoolBuilder()
																		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
																		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));
		Move<VkDescriptorSet>			descriptorSet				= makeDescriptorSet(vk, device, *descriptorPool, descriptorSetLayout);
		Buffer							resultBuffer				(vk, device, *queues.m_allocator, makeBufferCreateInfo(BUFFER_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);
		const VkBufferMemoryBarrier		bufferBarrier				= makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *resultBuffer, 0ull, BUFFER_SIZE);
		const VkFormat					colorFormat					= VK_FORMAT_R8G8B8A8_UNORM;
		const VkExtent3D				colorImageExtent			= makeExtent3D(1u, 1u, 1u);
		const VkImageSubresourceRange	colorImageSubresourceRange	= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
		de::MovePtr<Image>				colorAttachmentImage		= de::MovePtr<Image>(new Image(vk, device, *queues.m_allocator,
																		makeImageCreateInfo(VK_IMAGE_TYPE_2D, colorImageExtent, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
																		MemoryRequirement::Any));
		Move<VkImageView>				colorAttachmentView			= makeImageView(vk, device, **colorAttachmentImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorImageSubresourceRange);
		Move<VkFramebuffer>				framebuffer					= makeFramebuffer(vk, device, renderPass, *colorAttachmentView, colorImageExtent.width, colorImageExtent.height, 1u);
		const Unique<VkCommandPool>		cmdPool						(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Unique<VkCommandBuffer>	cmdBuffer					(makeCommandBuffer(vk, device, *cmdPool));
		const VkDescriptorBufferInfo	outputBufferDescriptorInfo	= makeDescriptorBufferInfo(*resultBuffer, 0ull, BUFFER_SIZE);

		DescriptorSetUpdateBuilder()
			.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &outputBufferDescriptorInfo)
			.update		(vk, device);

		{
			const Allocation& alloc = resultBuffer.getAllocation();
			deMemset(alloc.getHostPtr(), 0, BUFFER_SIZE);
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), BUFFER_SIZE);
		}

		// Start recording commands
		beginCommandBuffer(vk, *cmdBuffer);
		// Change color attachment image layout
		{
			const VkImageMemoryBarrier colorAttachmentLayoutBarrier = makeImageMemoryBarrier(
				(VkAccessFlags)0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				**colorAttachmentImage, colorImageSubresourceRange);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0,
				0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentLayoutBarrier);
		}

		{
			const VkRect2D	renderArea	=
										{
											makeOffset2D(0, 0),
											makeExtent2D(1, 1),
										};
			const tcu::Vec4	clearColor	= tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
			beginRenderPass(vk, *cmdBuffer, renderPass, *framebuffer, renderArea, clearColor);
		}

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		vk.cmdDraw(*cmdBuffer, shadersExecutions, 1u, 0u, 0u);
		endRenderPass(vk, *cmdBuffer);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0,
						0, (const VkMemoryBarrier*)DE_NULL,
						1, &bufferBarrier,
						0, (const VkImageMemoryBarrier*)DE_NULL);

		// End recording commands
		endCommandBuffer(vk, *cmdBuffer);

		// Wait for command buffer execution finish
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
		queues.releaseQueue(queueFamilyIndex, queueIndex);

		{
			const Allocation& resultAlloc = resultBuffer.getAllocation();
			invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), BUFFER_SIZE);

			const deInt32*	ptr = reinterpret_cast<deInt32*>(resultAlloc.getHostPtr());
			for (deInt32 ndx = 0; ndx < BUFFER_ELEMENT_COUNT; ++ndx)
			{
				if (ptr[ndx] != ndx)
				{
					return TestStatus::fail("The data don't match");
				}
			}
		}
		return TestStatus::pass("Passed");
	}
}


class ThreadGroupThread : private Thread
{
public:
							ThreadGroupThread	(const Context& context, VkPipelineCache pipelineCache, const VkPipelineLayout& pipelineLayout,
												const VkDescriptorSetLayout& descriptorSetLayout, MultiQueues& queues, const vector<deUint32>& shadersExecutions)
								: m_context				(context)
								, m_pipelineCache		(pipelineCache)
								, m_pipelineLayout		(pipelineLayout)
								, m_descriptorSetLayout	(descriptorSetLayout)
								, m_queues				(queues)
								, m_shadersExecutions	(shadersExecutions)
	{
	}

	virtual					~ThreadGroupThread	(void)
	{
	}

	ResultCollector&		getResultCollector	(void)
	{
		return m_resultCollector;
	}

	using Thread::start;
	using Thread::join;

protected:
	virtual TestStatus		runThread		() = 0;
	const Context&							m_context;
	VkPipelineCache							m_pipelineCache;
	const VkPipelineLayout&					m_pipelineLayout;
	const VkDescriptorSetLayout&			m_descriptorSetLayout;
	MultiQueues&							m_queues;
	const vector<deUint32>&					m_shadersExecutions;

private:
							ThreadGroupThread	(const ThreadGroupThread&);
	ThreadGroupThread&		operator=			(const ThreadGroupThread&);

	void					run					(void)
	{
		try
		{
			TestStatus result = runThread();
			m_resultCollector.addResult(result.getCode(), result.getDescription());
		}
		catch (const TestException& e)
		{
			m_resultCollector.addResult(e.getTestResult(), e.getMessage());
		}
		catch (const exception& e)
		{
			m_resultCollector.addResult(QP_TEST_RESULT_FAIL, e.what());
		}
		catch (...)
		{
			m_resultCollector.addResult(QP_TEST_RESULT_FAIL, "Exception");
		}
	}

	ResultCollector							m_resultCollector;
};

class ThreadGroup
{
	typedef vector<SharedPtr<ThreadGroupThread> >	ThreadVector;
public:
							ThreadGroup			(void)
	{
	}
							~ThreadGroup		(void)
	{
	}

	void					add					(MovePtr<ThreadGroupThread> thread)
	{
		m_threads.push_back(SharedPtr<ThreadGroupThread>(thread.release()));
	}

	TestStatus				run					(void)
	{
		ResultCollector	resultCollector;

		for (ThreadVector::iterator threadIter = m_threads.begin(); threadIter != m_threads.end(); ++threadIter)
			(*threadIter)->start();

		for (ThreadVector::iterator threadIter = m_threads.begin(); threadIter != m_threads.end(); ++threadIter)
		{
			ResultCollector&	threadResult	= (*threadIter)->getResultCollector();
			(*threadIter)->join();
			resultCollector.addResult(threadResult.getResult(), threadResult.getMessage());
		}

		return TestStatus(resultCollector.getResult(), resultCollector.getMessage());
	}

private:
	ThreadVector							m_threads;
};


class CreateComputeThread : public ThreadGroupThread
{
public:
			CreateComputeThread	(const Context& context, VkPipelineCache pipelineCache, vector<VkComputePipelineCreateInfo>& pipelineInfo,
								const VkPipelineLayout& pipelineLayout, const VkDescriptorSetLayout& descriptorSetLayout,
								MultiQueues& queues, const vector<deUint32>& shadersExecutions)
				: ThreadGroupThread		(context, pipelineCache, pipelineLayout, descriptorSetLayout, queues, shadersExecutions)
				, m_pipelineInfo		(pipelineInfo)
	{
	}

	TestStatus	runThread		(void)
	{
		ResultCollector		resultCollector;
		for (int executionNdx = 0; executionNdx < EXECUTION_PER_THREAD; ++executionNdx)
		{
			const int shaderNdx					= executionNdx % (int)m_pipelineInfo.size();
			const DeviceInterface&	vk			= m_context.getDeviceInterface();
			const VkDevice			device		= m_queues.getDevice();
			Move<VkPipeline>		pipeline	= createComputePipeline(vk,device,m_pipelineCache, &m_pipelineInfo[shaderNdx]);

			TestStatus result = executeComputePipeline(m_context, *pipeline, m_pipelineLayout, m_descriptorSetLayout, m_queues, m_shadersExecutions[shaderNdx]);
			resultCollector.addResult(result.getCode(), result.getDescription());
		}
		return TestStatus(resultCollector.getResult(), resultCollector.getMessage());
	}
private:
	vector<VkComputePipelineCreateInfo>&	m_pipelineInfo;
};

class CreateGraphicThread : public ThreadGroupThread
{
public:
			CreateGraphicThread	(const Context& context, VkPipelineCache pipelineCache, vector<VkGraphicsPipelineCreateInfo>& pipelineInfo,
								const VkPipelineLayout& pipelineLayout, const VkDescriptorSetLayout& descriptorSetLayout,
								MultiQueues& queues, const VkRenderPass& renderPass, const vector<deUint32>& shadersExecutions)
				: ThreadGroupThread		(context, pipelineCache, pipelineLayout, descriptorSetLayout, queues, shadersExecutions)
				, m_pipelineInfo		(pipelineInfo)
				, m_renderPass			(renderPass)
	{}

	TestStatus	runThread		(void)
	{
		ResultCollector		resultCollector;
		for (int executionNdx = 0; executionNdx < EXECUTION_PER_THREAD; ++executionNdx)
		{
			const int shaderNdx					= executionNdx % (int)m_pipelineInfo.size();
			const DeviceInterface&	vk			= m_context.getDeviceInterface();
			const VkDevice			device		= m_queues.getDevice();
			Move<VkPipeline>		pipeline	= createGraphicsPipeline(vk,device, m_pipelineCache, &m_pipelineInfo[shaderNdx]);

			TestStatus result = executeGraphicPipeline(m_context, *pipeline, m_pipelineLayout, m_descriptorSetLayout, m_queues, m_renderPass, m_shadersExecutions[shaderNdx]);
			resultCollector.addResult(result.getCode(), result.getDescription());
		}
		return TestStatus(resultCollector.getResult(), resultCollector.getMessage());
	}

private:
	vector<VkGraphicsPipelineCreateInfo>&	m_pipelineInfo;
	const VkRenderPass&						m_renderPass;
};

class PipelineCacheComputeTestInstance  : public TestInstance
{
	typedef vector<SharedPtr<Unique<VkShaderModule> > > ShaderModuleVector;
public:
				PipelineCacheComputeTestInstance	(Context& context, const vector<deUint32>& shadersExecutions)
					: TestInstance			(context)
					, m_shadersExecutions	(shadersExecutions)

	{
	}

	TestStatus	iterate								(void)
	{
		const DeviceInterface&					vk					= m_context.getDeviceInterface();
		MovePtr<MultiQueues>					queues				= createQueues(m_context, VK_QUEUE_COMPUTE_BIT);
		const VkDevice							device				= queues->getDevice();
		ShaderModuleVector						shaderCompModules	= addShaderModules(device);
		Buffer									resultBuffer		(vk, device, *queues->m_allocator, makeBufferCreateInfo(BUFFER_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);
		const Move<VkDescriptorSetLayout>		descriptorSetLayout	(DescriptorSetLayoutBuilder()
																		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
																		.build(vk, device));
		const Move<VkPipelineLayout>			pipelineLayout		(makePipelineLayout(vk, device, *descriptorSetLayout));
		vector<VkPipelineShaderStageCreateInfo>	shaderStageInfos	= addShaderStageInfo(shaderCompModules);
		vector<VkComputePipelineCreateInfo>		pipelineInfo		= addPipelineInfo(*pipelineLayout, shaderStageInfos);
		const VkPipelineCacheCreateInfo			pipelineCacheInfo	=
																	{
																		VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,	// VkStructureType             sType;
																		DE_NULL,										// const void*                 pNext;
																		0u,												// VkPipelineCacheCreateFlags  flags;
																		0u,												// deUintptr                   initialDataSize;
																		DE_NULL,										// const void*                 pInitialData;
																	};
		Move<VkPipelineCache>					pipelineCache		= createPipelineCache(vk, device, &pipelineCacheInfo);
		Move<VkPipeline>						pipeline			= createComputePipeline(vk, device, *pipelineCache, &pipelineInfo[0]);
		const deUint32							numThreads			= clamp(deGetNumAvailableLogicalCores(), 4u, 32u);
		ThreadGroup								threads;

		executeComputePipeline(m_context, *pipeline, *pipelineLayout, *descriptorSetLayout, *queues, m_shadersExecutions[0]);

		for (deUint32 ndx = 0; ndx < numThreads; ++ndx)
			threads.add(MovePtr<ThreadGroupThread>(new CreateComputeThread(
				m_context, *pipelineCache, pipelineInfo, *pipelineLayout, *descriptorSetLayout, *queues, m_shadersExecutions)));

		{
			TestStatus thread_result = threads.run();
			if(thread_result.getCode())
			{
				return thread_result;
			}
		}
		return TestStatus::pass("Passed");
	}

private:
	ShaderModuleVector							addShaderModules					(const VkDevice& device)
	{
		const DeviceInterface&	vk	= m_context.getDeviceInterface();
		ShaderModuleVector		shaderCompModules;
		shaderCompModules.resize(m_shadersExecutions.size());
		for (int shaderNdx = 0; shaderNdx <  static_cast<int>(m_shadersExecutions.size()); ++shaderNdx)
		{
			ostringstream shaderName;
			shaderName<<"compute_"<<shaderNdx;
			shaderCompModules[shaderNdx] = SharedPtr<Unique<VkShaderModule> > (new Unique<VkShaderModule>(createShaderModule(vk, device, m_context.getBinaryCollection().get(shaderName.str()), (VkShaderModuleCreateFlags)0)));
		}
		return shaderCompModules;
	}

	vector<VkPipelineShaderStageCreateInfo>		addShaderStageInfo					(const ShaderModuleVector& shaderCompModules)
	{
		VkPipelineShaderStageCreateInfo			shaderStageInfo;
		vector<VkPipelineShaderStageCreateInfo>	shaderStageInfos;
		shaderStageInfo.sType				=	VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.pNext				=	DE_NULL;
		shaderStageInfo.flags				=	(VkPipelineShaderStageCreateFlags)0;
		shaderStageInfo.stage				=	VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageInfo.pName				=	"main";
		shaderStageInfo.pSpecializationInfo	=	DE_NULL;

		for (int shaderNdx = 0; shaderNdx <  static_cast<int>(m_shadersExecutions.size()); ++shaderNdx)
		{
			shaderStageInfo.module = *(*shaderCompModules[shaderNdx]);
			shaderStageInfos.push_back(shaderStageInfo);
		}
		return shaderStageInfos;
	}

	vector<VkComputePipelineCreateInfo>		addPipelineInfo						(VkPipelineLayout pipelineLayout, const vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos)
	{
		vector<VkComputePipelineCreateInfo> pipelineInfos;
		VkComputePipelineCreateInfo	computePipelineInfo;
									computePipelineInfo.sType				= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
									computePipelineInfo.pNext				= DE_NULL;
									computePipelineInfo.flags				= (VkPipelineCreateFlags)0;
									computePipelineInfo.layout				= pipelineLayout;
									computePipelineInfo.basePipelineHandle	= DE_NULL;
									computePipelineInfo.basePipelineIndex	= 0;

		for (int shaderNdx = 0; shaderNdx < static_cast<int>(m_shadersExecutions.size()); ++shaderNdx)
		{
			computePipelineInfo.stage = shaderStageInfos[shaderNdx];
			pipelineInfos.push_back(computePipelineInfo);
		}
		return pipelineInfos;
	}

	const vector<deUint32>	m_shadersExecutions;
};

class PipelineCacheGraphicTestInstance  : public TestInstance
{
	typedef vector<SharedPtr<Unique<VkShaderModule> > > ShaderModuleVector;
public:
											PipelineCacheGraphicTestInstance	(Context& context, const vector<deUint32>& shadersExecutions)
								: TestInstance			(context)
								, m_shadersExecutions	(shadersExecutions)

	{
	}

	TestStatus								iterate								(void)
	{
		requireFeatures(m_context.getInstanceInterface(), m_context.getPhysicalDevice(), FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS);

		const DeviceInterface&					vk						= m_context.getDeviceInterface();
		MovePtr<MultiQueues>					queues					= createQueues (m_context, VK_QUEUE_GRAPHICS_BIT);
		const VkDevice							device					= queues->getDevice();
		VkFormat								colorFormat				= VK_FORMAT_R8G8B8A8_UNORM;
		Move<VkRenderPass>						renderPass				= createRenderPass(m_context, device, colorFormat);
		const Move<VkDescriptorSetLayout>		descriptorSetLayout		(DescriptorSetLayoutBuilder()
																			.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
																			.build(vk, device));
		ShaderModuleVector						shaderGraphicModules	= addShaderModules(device);
		const Move<VkPipelineLayout>			pipelineLayout			(makePipelineLayout(vk, device, *descriptorSetLayout));
		vector<VkPipelineShaderStageCreateInfo>	shaderStageInfos		= addShaderStageInfo(shaderGraphicModules);
		vector<VkGraphicsPipelineCreateInfo>	pipelineInfo			= addPipelineInfo(*pipelineLayout, shaderStageInfos, *renderPass);
		const VkPipelineCacheCreateInfo			pipelineCacheInfo		=
																		{
																			VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,	// VkStructureType             sType;
																			DE_NULL,										// const void*                 pNext;
																			0u,												// VkPipelineCacheCreateFlags  flags;
																			0u,												// deUintptr                   initialDataSize;
																			DE_NULL,										// const void*                 pInitialData;
																		};
		Move<VkPipelineCache>					pipelineCache			= createPipelineCache(vk, device, &pipelineCacheInfo);
		Move<VkPipeline>						pipeline				= createGraphicsPipeline(vk, device, *pipelineCache, &pipelineInfo[0]);
		const deUint32							numThreads				= clamp(deGetNumAvailableLogicalCores(), 4u, 32u);
		ThreadGroup								threads;

		executeGraphicPipeline(m_context, *pipeline, *pipelineLayout, *descriptorSetLayout, *queues, *renderPass, m_shadersExecutions[0]);

		for (deUint32 ndx = 0; ndx < numThreads; ++ndx)
			threads.add(MovePtr<ThreadGroupThread>(new CreateGraphicThread(
				m_context, *pipelineCache, pipelineInfo, *pipelineLayout, *descriptorSetLayout, *queues, *renderPass, m_shadersExecutions)));

		{
			TestStatus thread_result = threads.run();
			if(thread_result.getCode())
			{
				return thread_result;
			}
		}
		return TestStatus::pass("Passed");
	}

private:
	ShaderModuleVector						addShaderModules					(const VkDevice& device)
	{
		const DeviceInterface&	vk					= m_context.getDeviceInterface();
		ShaderModuleVector		shaderModules;
		shaderModules.resize(m_shadersExecutions.size() + 1);
		for (int shaderNdx = 0; shaderNdx <  static_cast<int>(m_shadersExecutions.size()); ++shaderNdx)
		{
			ostringstream shaderName;
			shaderName<<"vert_"<<shaderNdx;
			shaderModules[shaderNdx] = SharedPtr<Unique<VkShaderModule> > (new Unique<VkShaderModule>(createShaderModule(vk, device, m_context.getBinaryCollection().get(shaderName.str()), (VkShaderModuleCreateFlags)0)));
		}
		shaderModules[m_shadersExecutions.size()] = SharedPtr<Unique<VkShaderModule> > (new Unique<VkShaderModule>(createShaderModule(vk, device, m_context.getBinaryCollection().get("frag"), (VkShaderModuleCreateFlags)0)));
		return shaderModules;
	}

	vector<VkPipelineShaderStageCreateInfo>	addShaderStageInfo					(const ShaderModuleVector& shaderCompModules)
	{
		VkPipelineShaderStageCreateInfo			shaderStageInfo;
		vector<VkPipelineShaderStageCreateInfo>	shaderStageInfos;
		shaderStageInfo.sType				=	VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.pNext				=	DE_NULL;
		shaderStageInfo.flags				=	(VkPipelineShaderStageCreateFlags)0;
		shaderStageInfo.pName				=	"main";
		shaderStageInfo.pSpecializationInfo	=	DE_NULL;

		for (int shaderNdx = 0; shaderNdx <  static_cast<int>(m_shadersExecutions.size()); ++shaderNdx)
		{
			shaderStageInfo.stage	=	VK_SHADER_STAGE_VERTEX_BIT;
			shaderStageInfo.module	= *(*shaderCompModules[shaderNdx]);
			shaderStageInfos.push_back(shaderStageInfo);

			shaderStageInfo.stage	=	VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStageInfo.module	= *(*shaderCompModules[m_shadersExecutions.size()]);
			shaderStageInfos.push_back(shaderStageInfo);
		}
		return shaderStageInfos;
	}

	vector<VkGraphicsPipelineCreateInfo>	addPipelineInfo						(VkPipelineLayout pipelineLayout, const vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos, const VkRenderPass& renderPass)
	{
		VkExtent3D								colorImageExtent	= makeExtent3D(1u, 1u, 1u);
		vector<VkGraphicsPipelineCreateInfo>	pipelineInfo;

		m_vertexInputStateParams.sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		m_vertexInputStateParams.pNext								= DE_NULL;
		m_vertexInputStateParams.flags								= 0u;
		m_vertexInputStateParams.vertexBindingDescriptionCount		= 0u;
		m_vertexInputStateParams.pVertexBindingDescriptions			= DE_NULL;
		m_vertexInputStateParams.vertexAttributeDescriptionCount	= 0u;
		m_vertexInputStateParams.pVertexAttributeDescriptions		= DE_NULL;

		m_inputAssemblyStateParams.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		m_inputAssemblyStateParams.pNext					= DE_NULL;
		m_inputAssemblyStateParams.flags					= 0u;
		m_inputAssemblyStateParams.topology					= VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		m_inputAssemblyStateParams.primitiveRestartEnable	= VK_FALSE;

		m_viewport.x			= 0.0f;
		m_viewport.y			= 0.0f;
		m_viewport.width		= (float)colorImageExtent.width;
		m_viewport.height		= (float)colorImageExtent.height;
		m_viewport.minDepth		= 0.0f;
		m_viewport.maxDepth		= 1.0f;

		//TODO
		m_scissor.offset.x		= 0;
		m_scissor.offset.y		= 0;
		m_scissor.extent.width	= colorImageExtent.width;
		m_scissor.extent.height	= colorImageExtent.height;

		m_viewportStateParams.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		m_viewportStateParams.pNext			= DE_NULL;
		m_viewportStateParams.flags			= 0u;
		m_viewportStateParams.viewportCount	= 1u;
		m_viewportStateParams.pViewports	= &m_viewport;
		m_viewportStateParams.scissorCount	= 1u;
		m_viewportStateParams.pScissors		= &m_scissor;

		m_rasterStateParams.sType					= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		m_rasterStateParams.pNext					= DE_NULL;
		m_rasterStateParams.flags					= 0u;
		m_rasterStateParams.depthClampEnable		= VK_FALSE;
		m_rasterStateParams.rasterizerDiscardEnable	= VK_FALSE;
		m_rasterStateParams.polygonMode				= VK_POLYGON_MODE_FILL;
		m_rasterStateParams.cullMode				= VK_CULL_MODE_NONE;
		m_rasterStateParams.frontFace				= VK_FRONT_FACE_COUNTER_CLOCKWISE;
		m_rasterStateParams.depthBiasEnable			= VK_FALSE;
		m_rasterStateParams.depthBiasConstantFactor	= 0.0f;
		m_rasterStateParams.depthBiasClamp			= 0.0f;
		m_rasterStateParams.depthBiasSlopeFactor	= 0.0f;
		m_rasterStateParams.lineWidth				= 1.0f;

		m_colorBlendAttachmentState.blendEnable			= VK_FALSE;
		m_colorBlendAttachmentState.srcColorBlendFactor	= VK_BLEND_FACTOR_ONE;
		m_colorBlendAttachmentState.dstColorBlendFactor	= VK_BLEND_FACTOR_ZERO;
		m_colorBlendAttachmentState.colorBlendOp		= VK_BLEND_OP_ADD;
		m_colorBlendAttachmentState.srcAlphaBlendFactor	= VK_BLEND_FACTOR_ONE;
		m_colorBlendAttachmentState.dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
		m_colorBlendAttachmentState.alphaBlendOp		= VK_BLEND_OP_ADD;
		m_colorBlendAttachmentState.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT |
														  VK_COLOR_COMPONENT_G_BIT |
														  VK_COLOR_COMPONENT_B_BIT |
														  VK_COLOR_COMPONENT_A_BIT;

		m_colorBlendStateParams.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_colorBlendStateParams.pNext				= DE_NULL;
		m_colorBlendStateParams.flags				= 0u;
		m_colorBlendStateParams.logicOpEnable		= VK_FALSE;
		m_colorBlendStateParams.logicOp				= VK_LOGIC_OP_COPY;
		m_colorBlendStateParams.attachmentCount		= 1u;
		m_colorBlendStateParams.pAttachments		= &m_colorBlendAttachmentState;
		m_colorBlendStateParams.blendConstants[0]	= 0.0f;
		m_colorBlendStateParams.blendConstants[1]	= 0.0f;
		m_colorBlendStateParams.blendConstants[2]	= 0.0f;
		m_colorBlendStateParams.blendConstants[3]	= 0.0f;

		m_multisampleStateParams.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		m_multisampleStateParams.pNext					= DE_NULL;
		m_multisampleStateParams.flags					= 0u;
		m_multisampleStateParams.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
		m_multisampleStateParams.sampleShadingEnable	= VK_FALSE;
		m_multisampleStateParams.minSampleShading		= 0.0f;
		m_multisampleStateParams.pSampleMask			= DE_NULL;
		m_multisampleStateParams.alphaToCoverageEnable	= VK_FALSE;
		m_multisampleStateParams.alphaToOneEnable		= VK_FALSE;

		m_depthStencilStateParams.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		m_depthStencilStateParams.pNext					= DE_NULL;
		m_depthStencilStateParams.flags					= 0u;
		m_depthStencilStateParams.depthTestEnable		= VK_TRUE;
		m_depthStencilStateParams.depthWriteEnable		= VK_TRUE;
		m_depthStencilStateParams.depthCompareOp		= VK_COMPARE_OP_LESS_OR_EQUAL;
		m_depthStencilStateParams.depthBoundsTestEnable	= VK_FALSE;
		m_depthStencilStateParams.stencilTestEnable		= VK_FALSE;
		m_depthStencilStateParams.front.failOp			= VK_STENCIL_OP_KEEP;
		m_depthStencilStateParams.front.passOp			= VK_STENCIL_OP_KEEP;
		m_depthStencilStateParams.front.depthFailOp		= VK_STENCIL_OP_KEEP;
		m_depthStencilStateParams.front.compareOp		= VK_COMPARE_OP_NEVER;
		m_depthStencilStateParams.front.compareMask		= 0u;
		m_depthStencilStateParams.front.writeMask		= 0u;
		m_depthStencilStateParams.front.reference		= 0u;
		m_depthStencilStateParams.back.failOp			= VK_STENCIL_OP_KEEP;
		m_depthStencilStateParams.back.passOp			= VK_STENCIL_OP_KEEP;
		m_depthStencilStateParams.back.depthFailOp		= VK_STENCIL_OP_KEEP;
		m_depthStencilStateParams.back.compareOp		= VK_COMPARE_OP_NEVER;
		m_depthStencilStateParams.back.compareMask		= 0u;
		m_depthStencilStateParams.back.writeMask		= 0u;
		m_depthStencilStateParams.back.reference		= 0u;
		m_depthStencilStateParams.minDepthBounds		= 0.0f;
		m_depthStencilStateParams.maxDepthBounds		= 1.0f;

		VkGraphicsPipelineCreateInfo	graphicsPipelineParams	=
																{
																	VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
																	DE_NULL,											// const void*										pNext;
																	0u,													// VkPipelineCreateFlags							flags;
																	2u,													// deUint32											stageCount;
																	DE_NULL,											// const VkPipelineShaderStageCreateInfo*			pStages;
																	&m_vertexInputStateParams,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
																	&m_inputAssemblyStateParams,						// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
																	DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
																	&m_viewportStateParams,								// const VkPipelineViewportStateCreateInfo*			pViewportState;
																	&m_rasterStateParams,								// const VkPipelineRasterizationStateCreateInfo*	pRasterState;
																	&m_multisampleStateParams,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
																	&m_depthStencilStateParams,							// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
																	&m_colorBlendStateParams,							// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
																	(const VkPipelineDynamicStateCreateInfo*)DE_NULL,	// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
																	pipelineLayout,										// VkPipelineLayout									layout;
																	renderPass,											// VkRenderPass										renderPass;
																	0u,													// deUint32											subpass;
																	DE_NULL,											// VkPipeline										basePipelineHandle;
																	0,													// deInt32											basePipelineIndex;
																};
		for (int shaderNdx = 0; shaderNdx < static_cast<int>(m_shadersExecutions.size()) * 2; shaderNdx+=2)
		{
			graphicsPipelineParams.pStages = &shaderStageInfos[shaderNdx];
			pipelineInfo.push_back(graphicsPipelineParams);
		}
		return pipelineInfo;
	}

	const vector<deUint32>					m_shadersExecutions;
	VkPipelineVertexInputStateCreateInfo	m_vertexInputStateParams;
	VkPipelineInputAssemblyStateCreateInfo	m_inputAssemblyStateParams;
	VkViewport								m_viewport;
	VkRect2D								m_scissor;
	VkPipelineViewportStateCreateInfo		m_viewportStateParams;
	VkPipelineRasterizationStateCreateInfo	m_rasterStateParams;
	VkPipelineColorBlendAttachmentState		m_colorBlendAttachmentState;
	VkPipelineColorBlendStateCreateInfo		m_colorBlendStateParams;
	VkPipelineMultisampleStateCreateInfo	m_multisampleStateParams;
	VkPipelineDepthStencilStateCreateInfo	m_depthStencilStateParams;
};

class PipelineCacheComputeTest : public TestCase
{
public:
							PipelineCacheComputeTest	(TestContext&		testCtx,
														const string&		name,
														const string&		description)
								:TestCase	(testCtx, name, description)
	{
	}

	void					initPrograms				(SourceCollections&	programCollection) const
	{
		ostringstream buffer;
		buffer	<< "layout(set = 0, binding = 0, std430) buffer Output\n"
				<< "{\n"
				<< "	int result[];\n"
				<< "} sb_out;\n";
		{
			ostringstream src;
			src	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "\n"
				<< "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
				<< "\n"
				<< buffer.str()
				<< "void main (void)\n"
				<< "{\n"
				<< "	highp uint ndx = gl_GlobalInvocationID.x;\n"
				<< "	sb_out.result[ndx] = int(ndx);\n"
				<< "}\n";
			programCollection.glslSources.add("compute_0") << glu::ComputeSource(src.str());
		}
		{
			ostringstream src;
			src	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "\n"
				<< "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
				<< "\n"
				<< buffer.str()
				<< "void main (void)\n"
				<< "{\n"
				<< "	for (highp uint ndx = 0u; ndx < "<<BUFFER_ELEMENT_COUNT<<"u; ndx++)\n"
				<< "	{\n"
				<< "		sb_out.result[ndx] = int(ndx);\n"
				<< "	}\n"
				<< "}\n";
			programCollection.glslSources.add("compute_1") << glu::ComputeSource(src.str());
		}
		{
			ostringstream src;
			src	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "\n"
				<< "layout(local_size_x = "<<BUFFER_ELEMENT_COUNT<<", local_size_y = 1, local_size_z = 1) in;\n"
				<< "\n"
				<< buffer.str()
				<< "void main (void)\n"
				<< "{\n"
				<< "	highp uint ndx = gl_LocalInvocationID.x;\n"
				<< "	sb_out.result[ndx] = int(ndx);\n"
				<< "}\n";
			programCollection.glslSources.add("compute_2") << glu::ComputeSource(src.str());
		}
	}

	TestInstance*			createInstance				(Context& context) const
	{
		vector<deUint32>	shadersExecutions;
		shadersExecutions.push_back(16u);	//compute_0
		shadersExecutions.push_back(1u);	//compute_1
		shadersExecutions.push_back(1u);	//compute_2
		return new PipelineCacheComputeTestInstance(context, shadersExecutions);
	}
};

class PipelineCacheGraphicTest : public TestCase
{
public:
							PipelineCacheGraphicTest	(TestContext&		testCtx,
														const string&		name,
														const string&		description)
								:TestCase	(testCtx, name, description)
	{

	}

	void					initPrograms				(SourceCollections&	programCollection) const
	{
		ostringstream buffer;
		buffer	<< "layout(set = 0, binding = 0, std430) buffer Output\n"
				<< "{\n"
				<< "	int result[];\n"
				<< "} sb_out;\n";

		// Vertex
		{
			std::ostringstream src;
			src	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_440) << "\n"
				<< "\n"
				<< buffer.str()
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "   sb_out.result[gl_VertexIndex] = int(gl_VertexIndex);\n"
				<< "}\n";
			programCollection.glslSources.add("vert_0") << glu::VertexSource(src.str());
		}
		// Vertex
		{
			std::ostringstream src;
			src	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_440) << "\n"
				<< "\n"
				<< buffer.str()
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "	for (highp uint ndx = 0u; ndx < "<<BUFFER_ELEMENT_COUNT<<"u; ndx++)\n"
				<< "	{\n"
				<< "		sb_out.result[ndx] = int(ndx);\n"
				<< "	}\n"
				<< "}\n";
			programCollection.glslSources.add("vert_1") << glu::VertexSource(src.str());
		}
		// Vertex
		{
			std::ostringstream src;
			src	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_440) << "\n"
				<< "\n"
				<< buffer.str()
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "	for (int ndx = "<<BUFFER_ELEMENT_COUNT-1<<"; ndx >= 0; ndx--)\n"
				<< "	{\n"
				<< "		sb_out.result[uint(ndx)] = ndx;\n"
				<< "	}\n"
				<< "}\n";
			programCollection.glslSources.add("vert_2") << glu::VertexSource(src.str());
		}
		// Fragment
		{
			std::ostringstream src;
			src	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_440) << "\n"
				<< "\n"
				<< "layout(location = 0) out vec4 o_color;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    o_color = vec4(1.0);\n"
				<< "}\n";
			programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
		}
	}

	TestInstance*			createInstance				(Context& context) const
	{
		vector<deUint32>	shadersExecutions;
		shadersExecutions.push_back(16u);	//vert_0
		shadersExecutions.push_back(1u);	//vert_1
		shadersExecutions.push_back(1u);	//vert_2
		return new PipelineCacheGraphicTestInstance(context, shadersExecutions);
	}
};


} // anonymous

tcu::TestCaseGroup* createInternallySynchronizedObjects (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> tests(new tcu::TestCaseGroup(testCtx, "internally_synchronized_objects", "Internally synchronized objects"));
	tests->addChild(new PipelineCacheComputeTest(testCtx, "pipeline_cache_compute", "Internally synchronized object VkPipelineCache for compute pipeline is tested"));
	tests->addChild(new PipelineCacheGraphicTest(testCtx, "pipeline_cache_graphics", "Internally synchronized object VkPipelineCache for graphics pipeline is tested"));
	return tests.release();
}

} // synchronization
} // vkt
