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
 * \brief Synchronization primitive tests with multi queue
 *//*--------------------------------------------------------------------*/

#include "vktSynchronizationOperationMultiQueueTests.hpp"
#include "vkDefs.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkPlatform.hpp"
#include "deUniquePtr.hpp"
#include "tcuTestLog.hpp"
#include "vktSynchronizationUtil.hpp"
#include "vktSynchronizationOperation.hpp"
#include "vktSynchronizationOperationTestData.hpp"
#include "vktSynchronizationOperationResources.hpp"
#include "vktTestGroupUtil.hpp"

namespace vkt
{
namespace synchronization
{
namespace
{
using namespace vk;
using de::MovePtr;
using de::UniquePtr;

enum QueueType
{
	QUEUETYPE_WRITE,
	QUEUETYPE_READ
};

struct QueuePair
{
	QueuePair	(const deUint32 familyWrite, const deUint32 familyRead, const VkQueue write, const VkQueue read)
		: familyIndexWrite	(familyWrite)
		, familyIndexRead	(familyRead)
		, queueWrite		(write)
		, queueRead			(read)
	{}

	deUint32	familyIndexWrite;
	deUint32	familyIndexRead;
	VkQueue		queueWrite;
	VkQueue		queueRead;
};

bool checkQueueFlags (VkQueueFlags availableFlags, const VkQueueFlags neededFlags)
{
	if ((availableFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) != 0)
		availableFlags |= VK_QUEUE_TRANSFER_BIT;

	return (availableFlags & neededFlags) != 0;
}

class MultiQueues
{
	struct QueueData
	{
		VkQueueFlags			flags;
		std::vector<VkQueue>	queue;
	};

public:
	MultiQueues	(const Context& context)
	{
		const InstanceInterface&					instance				= context.getInstanceInterface();
		const VkPhysicalDevice						physicalDevice			= context.getPhysicalDevice();
		const std::vector<VkQueueFamilyProperties>	queueFamilyProperties	= getPhysicalDeviceQueueFamilyProperties(instance, physicalDevice);

		for (deUint32 queuePropertiesNdx = 0; queuePropertiesNdx < queueFamilyProperties.size(); ++queuePropertiesNdx)
		{
			addQueueIndex(queuePropertiesNdx,
						  std::min(2u, queueFamilyProperties[queuePropertiesNdx].queueCount),
						  queueFamilyProperties[queuePropertiesNdx].queueFlags);
		}

		std::vector<VkDeviceQueueCreateInfo>	queueInfos;
		const float								queuePriorities[2] = { 1.0f, 1.0f };	//get max 2 queues from one family

		for (std::map<deUint32, QueueData>::iterator it = m_queues.begin(); it!= m_queues.end(); ++it)
		{
			const VkDeviceQueueCreateInfo queueInfo	=
			{
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,		//VkStructureType			sType;
				DE_NULL,										//const void*				pNext;
				(VkDeviceQueueCreateFlags)0u,					//VkDeviceQueueCreateFlags	flags;
				it->first,										//deUint32					queueFamilyIndex;
				static_cast<deUint32>(it->second.queue.size()),	//deUint32					queueCount;
				&queuePriorities[0]								//const float*				pQueuePriorities;
			};
			queueInfos.push_back(queueInfo);
		}

		{
			const std::vector<std::string>&	deviceExtensions	= context.getDeviceExtensions();
			std::vector<const char*>		charDevExtensions;

			for (size_t ndx = 0; ndx < deviceExtensions.size(); ++ndx)
				charDevExtensions.push_back(deviceExtensions[ndx].c_str());

			const VkDeviceCreateInfo		deviceInfo		=
			{
				VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,			//VkStructureType					sType;
				DE_NULL,										//const void*						pNext;
				0u,												//VkDeviceCreateFlags				flags;
				static_cast<deUint32>(queueInfos.size()),		//deUint32							queueCreateInfoCount;
				&queueInfos[0],									//const VkDeviceQueueCreateInfo*	pQueueCreateInfos;
				0u,												//deUint32							enabledLayerCount;
				DE_NULL,										//const char* const*				ppEnabledLayerNames;
				static_cast<deUint32>(deviceExtensions.size()),	//deUint32							enabledExtensionCount;
				&charDevExtensions[0],							//const char* const*				ppEnabledExtensionNames;
				&context.getDeviceFeatures()					//const VkPhysicalDeviceFeatures*	pEnabledFeatures;
			};

			m_logicalDevice	= createDevice(instance, physicalDevice, &deviceInfo);
			m_deviceDriver	= MovePtr<DeviceDriver>(new DeviceDriver(instance, *m_logicalDevice));
			m_allocator		= MovePtr<Allocator>(new SimpleAllocator(*m_deviceDriver, *m_logicalDevice, getPhysicalDeviceMemoryProperties(instance, physicalDevice)));

			for (std::map<deUint32, QueueData>::iterator it = m_queues.begin(); it != m_queues.end(); ++it)
			for (int queueNdx = 0; queueNdx < static_cast<int>(it->second.queue.size()); ++queueNdx)
				m_deviceDriver->getDeviceQueue(*m_logicalDevice, it->first, queueNdx, &it->second.queue[queueNdx]);
		}
	}

	void addQueueIndex (const deUint32 queueFamilyIndex, const deUint32 count, const VkQueueFlags flags)
	{
		QueueData dataToPush;
		dataToPush.flags = flags;
		dataToPush.queue.resize(count);
		m_queues[queueFamilyIndex] = dataToPush;
	}

	std::vector<QueuePair> getQueuesPairs (const VkQueueFlags flagsWrite, const VkQueueFlags flagsRead)
	{
		std::map<deUint32, QueueData>	queuesWrite;
		std::map<deUint32, QueueData>	queuesRead;
		std::vector<QueuePair>			queuesPairs;

		for (std::map<deUint32, QueueData>::iterator it = m_queues.begin(); it != m_queues.end(); ++it)
		{
			const bool writeQueue	= checkQueueFlags(it->second.flags, flagsWrite);
			const bool readQueue	= checkQueueFlags(it->second.flags, flagsRead);

			if (!(writeQueue || readQueue))
				continue;

			if (writeQueue && readQueue)
			{
				queuesWrite[it->first]	= it->second;
				queuesRead[it->first]	= it->second;
			}
			else if (writeQueue)
				queuesWrite[it->first]	= it->second;
			else if (readQueue)
				queuesRead[it->first]	= it->second;
		}

		for (std::map<deUint32, QueueData>::iterator write = queuesWrite.begin(); write != queuesWrite.end(); ++write)
		for (std::map<deUint32, QueueData>::iterator read  = queuesRead.begin();  read  != queuesRead.end();  ++read)
		{
			const int writeSize	= static_cast<int>(write->second.queue.size());
			const int readSize	= static_cast<int>(read->second.queue.size());

			for (int writeNdx = 0; writeNdx < writeSize; ++writeNdx)
			for (int readNdx  = 0; readNdx  < readSize;  ++readNdx)
			{
				if (write->second.queue[writeNdx] != read->second.queue[readNdx])
				{
					queuesPairs.push_back(QueuePair(write->first, read->first, write->second.queue[writeNdx], read->second.queue[readNdx]));
					writeNdx = readNdx = std::max(writeSize, readSize);	//exit from the loops
				}
			}
		}

		if (queuesPairs.empty())
			TCU_THROW(NotSupportedError, "Queue not found");

		return queuesPairs;
	}

	VkDevice getDevice (void) const
	{
		return *m_logicalDevice;
	}

	const DeviceInterface& getDeviceInterface (void) const
	{
		return *m_deviceDriver;
	}

	Allocator& getAllocator (void)
	{
		return *m_allocator;
	}

private:
	Move<VkDevice>					m_logicalDevice;
	MovePtr<DeviceDriver>			m_deviceDriver;
	MovePtr<Allocator>				m_allocator;
	std::map<deUint32, QueueData>	m_queues;
};

void createBarrierMultiQueue (const DeviceInterface&	vk,
							  const VkCommandBuffer&	cmdBuffer,
							  const SyncInfo&			writeSync,
							  const SyncInfo&			readSync,
							  const Resource&			resource,
							  const deUint32			writeFamily,
							  const deUint32			readFamily,
							  const VkSharingMode		sharingMode,
							  const bool				secondQueue = false)
{
	if (resource.getType() == RESOURCE_TYPE_IMAGE)
	{
		VkImageMemoryBarrier barrier = makeImageMemoryBarrier(writeSync.accessMask, readSync.accessMask,
			writeSync.imageLayout, readSync.imageLayout, resource.getImage().handle, resource.getImage().subresourceRange);

		if (writeFamily != readFamily && VK_SHARING_MODE_EXCLUSIVE == sharingMode)
		{
			barrier.srcQueueFamilyIndex = writeFamily;
			barrier.dstQueueFamilyIndex = readFamily;
			if (secondQueue)
			{
				barrier.oldLayout		= barrier.newLayout;
				barrier.srcAccessMask	= barrier.dstAccessMask;
			}
			vk.cmdPipelineBarrier(cmdBuffer, writeSync.stageMask, readSync.stageMask, (VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 0u, (const VkBufferMemoryBarrier*)DE_NULL, 1u, &barrier);
		}
		else if (!secondQueue)
			vk.cmdPipelineBarrier(cmdBuffer, writeSync.stageMask, readSync.stageMask, (VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 0u, (const VkBufferMemoryBarrier*)DE_NULL, 1u, &barrier);
	}
	else if ((resource.getType() == RESOURCE_TYPE_BUFFER || isIndirectBuffer(resource.getType()))	&&
			 writeFamily != readFamily																&&
			 VK_SHARING_MODE_EXCLUSIVE == sharingMode)
	{
		const VkBufferMemoryBarrier barrier =
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
			DE_NULL,									// const void*		pNext;
			writeSync.accessMask ,						// VkAccessFlags	srcAccessMask;
			readSync.accessMask,						// VkAccessFlags	dstAccessMask;
			writeFamily,								// deUint32			srcQueueFamilyIndex;
			readFamily,									// deUint32			destQueueFamilyIndex;
			resource.getBuffer().handle,				// VkBuffer			buffer;
			resource.getBuffer().offset,				// VkDeviceSize		offset;
			resource.getBuffer().size,					// VkDeviceSize		size;
		};
		vk.cmdPipelineBarrier(cmdBuffer, writeSync.stageMask, readSync.stageMask, (VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, (const VkBufferMemoryBarrier*)&barrier, 0u, (const VkImageMemoryBarrier *)DE_NULL);
	}
}

class BaseTestInstance : public TestInstance
{
public:
	BaseTestInstance (Context& context, const ResourceDescription& resourceDesc, const OperationSupport& writeOp, const OperationSupport& readOp, PipelineCacheData& pipelineCacheData)
		: TestInstance		(context)
		, m_queues			(new MultiQueues(context))
		, m_opContext		(new OperationContext(context, pipelineCacheData, m_queues->getDeviceInterface(), m_queues->getDevice(), m_queues->getAllocator()))
		, m_resourceDesc	(resourceDesc)
		, m_writeOp			(writeOp)
		, m_readOp			(readOp)
	{
	}

protected:
	const UniquePtr<MultiQueues>		m_queues;
	const UniquePtr<OperationContext>	m_opContext;
	const ResourceDescription			m_resourceDesc;
	const OperationSupport&				m_writeOp;
	const OperationSupport&				m_readOp;
};

class SemaphoreTestInstance : public BaseTestInstance
{
public:
	SemaphoreTestInstance (Context& context, const ResourceDescription& resourceDesc, const OperationSupport& writeOp, const OperationSupport& readOp, PipelineCacheData& pipelineCacheData, const VkSharingMode sharingMode)
		: BaseTestInstance	(context, resourceDesc, writeOp, readOp, pipelineCacheData)
		, m_sharingMode		(sharingMode)
	{
	}

	tcu::TestStatus	iterate (void)
	{
		const DeviceInterface&			vk			= m_opContext->getDeviceInterface();
		const VkDevice					device		= m_opContext->getDevice();
		const std::vector<QueuePair>	queuePairs	= m_queues->getQueuesPairs(m_writeOp.getQueueFlags(*m_opContext), m_readOp.getQueueFlags(*m_opContext));

		for (deUint32 pairNdx = 0; pairNdx < static_cast<deUint32>(queuePairs.size()); ++pairNdx)
		{

			const UniquePtr<Resource>		resource		(new Resource(*m_opContext, m_resourceDesc, m_writeOp.getResourceUsageFlags() | m_readOp.getResourceUsageFlags()));
			const UniquePtr<Operation>		writeOp			(m_writeOp.build(*m_opContext, *resource));
			const UniquePtr<Operation>		readOp			(m_readOp.build (*m_opContext, *resource));

			const Move<VkCommandPool>		cmdPool[]		=
			{
				createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queuePairs[pairNdx].familyIndexWrite),
				createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queuePairs[pairNdx].familyIndexRead)
			};
			const Move<VkCommandBuffer>		ptrCmdBuffer[]	=
			{
				makeCommandBuffer(vk, device, *cmdPool[QUEUETYPE_WRITE]),
				makeCommandBuffer(vk, device, *cmdPool[QUEUETYPE_READ])
			};
			const VkCommandBuffer			cmdBuffers[]	=
			{
				*ptrCmdBuffer[QUEUETYPE_WRITE],
				*ptrCmdBuffer[QUEUETYPE_READ]
			};
			const Unique<VkSemaphore>		semaphore		(createSemaphore(vk, device));
			const VkPipelineStageFlags		stageBits[]		= { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
			const VkSubmitInfo				submitInfo[]	=
			{
				{
					VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType			sType;
					DE_NULL,							// const void*				pNext;
					0u,									// deUint32					waitSemaphoreCount;
					DE_NULL,							// const VkSemaphore*		pWaitSemaphores;
					(const VkPipelineStageFlags*)DE_NULL,
					1u,									// deUint32					commandBufferCount;
					&cmdBuffers[QUEUETYPE_WRITE],		// const VkCommandBuffer*	pCommandBuffers;
					1u,									// deUint32					signalSemaphoreCount;
					&semaphore.get(),					// const VkSemaphore*		pSignalSemaphores;
				},
				{
					VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType				sType;
					DE_NULL,							// const void*					pNext;
					1u,									// deUint32						waitSemaphoreCount;
					&semaphore.get(),					// const VkSemaphore*			pWaitSemaphores;
					stageBits,							// const VkPipelineStageFlags*	pWaitDstStageMask;
					1u,									// deUint32						commandBufferCount;
					&cmdBuffers[QUEUETYPE_READ],		// const VkCommandBuffer*		pCommandBuffers;
					0u,									// deUint32						signalSemaphoreCount;
					DE_NULL,							// const VkSemaphore*			pSignalSemaphores;
				}
			};
			const SyncInfo					writeSync		= writeOp->getSyncInfo();
			const SyncInfo					readSync		= readOp->getSyncInfo();

			beginCommandBuffer		(vk, cmdBuffers[QUEUETYPE_WRITE]);
			writeOp->recordCommands	(cmdBuffers[QUEUETYPE_WRITE]);
			createBarrierMultiQueue	(vk, cmdBuffers[QUEUETYPE_WRITE], writeSync, readSync, *resource, queuePairs[pairNdx].familyIndexWrite, queuePairs[pairNdx].familyIndexRead, m_sharingMode);
			endCommandBuffer		(vk, cmdBuffers[QUEUETYPE_WRITE]);

			beginCommandBuffer		(vk, cmdBuffers[QUEUETYPE_READ]);
			createBarrierMultiQueue	(vk, cmdBuffers[QUEUETYPE_READ], writeSync, readSync, *resource, queuePairs[pairNdx].familyIndexWrite, queuePairs[pairNdx].familyIndexRead, m_sharingMode, true);
			readOp->recordCommands	(cmdBuffers[QUEUETYPE_READ]);
			endCommandBuffer		(vk, cmdBuffers[QUEUETYPE_READ]);

			VK_CHECK(vk.queueSubmit(queuePairs[pairNdx].queueWrite, 1u, &submitInfo[QUEUETYPE_WRITE], DE_NULL));
			VK_CHECK(vk.queueSubmit(queuePairs[pairNdx].queueRead, 1u, &submitInfo[QUEUETYPE_READ], DE_NULL));
			VK_CHECK(vk.queueWaitIdle(queuePairs[pairNdx].queueWrite));
			VK_CHECK(vk.queueWaitIdle(queuePairs[pairNdx].queueRead));

			{
				const Data	expected	= writeOp->getData();
				const Data	actual		= readOp->getData();

				if (0 != deMemCmp(expected.data, actual.data, expected.size))
					return tcu::TestStatus::fail("Memory contents don't match");
			}
		}
		return tcu::TestStatus::pass("OK");
	}

private:
	const VkSharingMode	m_sharingMode;
};

class FenceTestInstance : public BaseTestInstance
{
public:
	FenceTestInstance (Context& context, const ResourceDescription& resourceDesc, const OperationSupport& writeOp, const OperationSupport& readOp, PipelineCacheData& pipelineCacheData, const VkSharingMode sharingMode)
		: BaseTestInstance	(context, resourceDesc, writeOp, readOp, pipelineCacheData)
		, m_sharingMode		(sharingMode)
	{
	}

	tcu::TestStatus	iterate (void)
	{
		const DeviceInterface&			vk			= m_opContext->getDeviceInterface();
		const VkDevice					device		= m_opContext->getDevice();
		const std::vector<QueuePair>	queuePairs	= m_queues->getQueuesPairs(m_writeOp.getQueueFlags(*m_opContext), m_readOp.getQueueFlags(*m_opContext));

		for (deUint32 pairNdx = 0; pairNdx < static_cast<deUint32>(queuePairs.size()); ++pairNdx)
		{
			const UniquePtr<Resource>		resource		(new Resource(*m_opContext, m_resourceDesc, m_writeOp.getResourceUsageFlags() | m_readOp.getResourceUsageFlags()));
			const UniquePtr<Operation>		writeOp			(m_writeOp.build(*m_opContext, *resource));
			const UniquePtr<Operation>		readOp			(m_readOp.build(*m_opContext, *resource));
			const Move<VkCommandPool>		cmdPool[]		=
			{
				createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queuePairs[pairNdx].familyIndexWrite),
				createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queuePairs[pairNdx].familyIndexRead)
			};
			const Move<VkCommandBuffer>		ptrCmdBuffer[]	=
			{
				makeCommandBuffer(vk, device, *cmdPool[QUEUETYPE_WRITE]),
				makeCommandBuffer(vk, device, *cmdPool[QUEUETYPE_READ])
			};
			const VkCommandBuffer			cmdBuffers[]	=
			{
				*ptrCmdBuffer[QUEUETYPE_WRITE],
				*ptrCmdBuffer[QUEUETYPE_READ]
			};
			const SyncInfo					writeSync		= writeOp->getSyncInfo();
			const SyncInfo					readSync		= readOp->getSyncInfo();

			beginCommandBuffer		(vk, cmdBuffers[QUEUETYPE_WRITE]);
			writeOp->recordCommands	(cmdBuffers[QUEUETYPE_WRITE]);
			createBarrierMultiQueue	(vk, cmdBuffers[QUEUETYPE_WRITE], writeSync, readSync, *resource, queuePairs[pairNdx].familyIndexWrite, queuePairs[pairNdx].familyIndexRead, m_sharingMode);
			endCommandBuffer		(vk, cmdBuffers[QUEUETYPE_WRITE]);

			submitCommandsAndWait	(vk, device, queuePairs[pairNdx].queueWrite, cmdBuffers[QUEUETYPE_WRITE]);

			beginCommandBuffer		(vk, cmdBuffers[QUEUETYPE_READ]);
			createBarrierMultiQueue	(vk, cmdBuffers[QUEUETYPE_READ], writeSync, readSync, *resource, queuePairs[pairNdx].familyIndexWrite, queuePairs[pairNdx].familyIndexRead, m_sharingMode, true);
			readOp->recordCommands	(cmdBuffers[QUEUETYPE_READ]);
			endCommandBuffer		(vk, cmdBuffers[QUEUETYPE_READ]);

			submitCommandsAndWait	(vk, device, queuePairs[pairNdx].queueRead, cmdBuffers[QUEUETYPE_READ]);

			{
				const Data	expected = writeOp->getData();
				const Data	actual	 = readOp->getData();

				if (0 != deMemCmp(expected.data, actual.data, expected.size))
					return tcu::TestStatus::fail("Memory contents don't match");
			}
		}
		return tcu::TestStatus::pass("OK");
	}

private:
	const VkSharingMode	m_sharingMode;
};

class BaseTestCase : public TestCase
{
public:
	BaseTestCase (tcu::TestContext&			testCtx,
				  const std::string&		name,
				  const std::string&		description,
				  const SyncPrimitive		syncPrimitive,
				  const ResourceDescription	resourceDesc,
				  const OperationName		writeOp,
				  const OperationName		readOp,
				  const VkSharingMode		sharingMode,
				  PipelineCacheData&		pipelineCacheData)
		: TestCase				(testCtx, name, description)
		, m_resourceDesc		(resourceDesc)
		, m_writeOp				(makeOperationSupport(writeOp, resourceDesc))
		, m_readOp				(makeOperationSupport(readOp, resourceDesc))
		, m_syncPrimitive		(syncPrimitive)
		, m_sharingMode			(sharingMode)
		, m_pipelineCacheData	(pipelineCacheData)
	{
	}

	void initPrograms (SourceCollections& programCollection) const
	{
		m_writeOp->initPrograms(programCollection);
		m_readOp->initPrograms(programCollection);
	}

	TestInstance* createInstance (Context& context) const
	{
		switch (m_syncPrimitive)
		{
			case SYNC_PRIMITIVE_FENCE:
				return new FenceTestInstance(context, m_resourceDesc, *m_writeOp, *m_readOp, m_pipelineCacheData, m_sharingMode);
			case SYNC_PRIMITIVE_SEMAPHORE:
				return new SemaphoreTestInstance(context, m_resourceDesc, *m_writeOp, *m_readOp, m_pipelineCacheData, m_sharingMode);
			default:
				DE_ASSERT(0);
				return DE_NULL;
		}
	}

private:
	const ResourceDescription				m_resourceDesc;
	const UniquePtr<OperationSupport>		m_writeOp;
	const UniquePtr<OperationSupport>		m_readOp;
	const SyncPrimitive						m_syncPrimitive;
	const VkSharingMode						m_sharingMode;
	PipelineCacheData&						m_pipelineCacheData;
};

void createTests (tcu::TestCaseGroup* group, PipelineCacheData* pipelineCacheData)
{
	tcu::TestContext& testCtx = group->getTestContext();

	static const struct
	{
		const char*		name;
		SyncPrimitive	syncPrimitive;
		int				numOptions;
	} groups[] =
	{
		{ "fence",		SYNC_PRIMITIVE_FENCE,		1 },
		{ "semaphore",	SYNC_PRIMITIVE_SEMAPHORE,	1 }
	};

	for (int groupNdx = 0; groupNdx < DE_LENGTH_OF_ARRAY(groups); ++groupNdx)
	{
		MovePtr<tcu::TestCaseGroup> synchGroup (new tcu::TestCaseGroup(testCtx, groups[groupNdx].name, ""));

		for (int writeOpNdx = 0; writeOpNdx < DE_LENGTH_OF_ARRAY(s_writeOps); ++writeOpNdx)
		for (int readOpNdx  = 0; readOpNdx  < DE_LENGTH_OF_ARRAY(s_readOps);  ++readOpNdx)
		{
			const OperationName	writeOp		= s_writeOps[writeOpNdx];
			const OperationName	readOp		= s_readOps[readOpNdx];
			const std::string	opGroupName = getOperationName(writeOp) + "_" + getOperationName(readOp);
			bool				empty		= true;

			MovePtr<tcu::TestCaseGroup> opGroup		(new tcu::TestCaseGroup(testCtx, opGroupName.c_str(), ""));

			for (int optionNdx = 0; optionNdx <= groups[groupNdx].numOptions; ++optionNdx)
			for (int resourceNdx = 0; resourceNdx < DE_LENGTH_OF_ARRAY(s_resources); ++resourceNdx)
			{
				const ResourceDescription&	resource	= s_resources[resourceNdx];
				std::string					name		= getResourceName(resource);
				VkSharingMode				sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				// queue family sharing mode used for resource
				if (optionNdx)
				{
					name += "_concurrent";
					sharingMode = VK_SHARING_MODE_CONCURRENT;
				}
				else
					name += "_exclusive";

				if (isResourceSupported(writeOp, resource) && isResourceSupported(readOp, resource))
				{
					opGroup->addChild(new BaseTestCase(testCtx, name, "", groups[groupNdx].syncPrimitive, resource, writeOp, readOp, sharingMode, *pipelineCacheData));
					empty = false;
				}
			}
			if (!empty)
				synchGroup->addChild(opGroup.release());
		}
		group->addChild(synchGroup.release());
	}
}

} // anonymous

tcu::TestCaseGroup* createSynchronizedOperationMultiQueueTests (tcu::TestContext& testCtx, PipelineCacheData& pipelineCacheData)
{
	return createTestGroup(testCtx, "multi_queue", "Synchronization of a memory-modifying operation", createTests, &pipelineCacheData);
}

} // synchronization
} // vkt
