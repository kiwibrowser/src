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
 * \brief Synchronization primitive tests with single queue
 *//*--------------------------------------------------------------------*/

#include "vktSynchronizationOperationSingleQueueTests.hpp"
#include "vkDefs.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"
#include "deUniquePtr.hpp"
#include "tcuTestLog.hpp"
#include "vktSynchronizationUtil.hpp"
#include "vktSynchronizationOperation.hpp"
#include "vktSynchronizationOperationTestData.hpp"
#include "vktSynchronizationOperationResources.hpp"

namespace vkt
{
namespace synchronization
{
namespace
{
using namespace vk;

class BaseTestInstance : public TestInstance
{
public:
	BaseTestInstance (Context& context, const ResourceDescription& resourceDesc, const OperationSupport& writeOp, const OperationSupport& readOp, PipelineCacheData& pipelineCacheData)
		: TestInstance	(context)
		, m_opContext	(context, pipelineCacheData)
		, m_resource	(new Resource(m_opContext, resourceDesc, writeOp.getResourceUsageFlags() | readOp.getResourceUsageFlags()))
		, m_writeOp		(writeOp.build(m_opContext, *m_resource))
		, m_readOp		(readOp.build(m_opContext, *m_resource))
	{
	}

protected:
	OperationContext					m_opContext;
	const de::UniquePtr<Resource>		m_resource;
	const de::UniquePtr<Operation>		m_writeOp;
	const de::UniquePtr<Operation>		m_readOp;
};

class EventTestInstance : public BaseTestInstance
{
public:
	EventTestInstance (Context& context, const ResourceDescription& resourceDesc, const OperationSupport& writeOp, const OperationSupport& readOp, PipelineCacheData& pipelineCacheData)
		: BaseTestInstance		(context, resourceDesc, writeOp, readOp, pipelineCacheData)
	{
	}

	tcu::TestStatus iterate (void)
	{
		const DeviceInterface&			vk					= m_context.getDeviceInterface();
		const VkDevice					device				= m_context.getDevice();
		const VkQueue					queue				= m_context.getUniversalQueue();
		const deUint32					queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
		const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Unique<VkCommandBuffer>	cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));
		const Unique<VkEvent>			event				(createEvent(vk, device));
		const SyncInfo					writeSync			= m_writeOp->getSyncInfo();
		const SyncInfo					readSync			= m_readOp->getSyncInfo();

		beginCommandBuffer(vk, *cmdBuffer);

		m_writeOp->recordCommands(*cmdBuffer);
		vk.cmdSetEvent(*cmdBuffer, *event, writeSync.stageMask);

		if (m_resource->getType() == RESOURCE_TYPE_BUFFER || isIndirectBuffer(m_resource->getType()))
		{
			const VkBufferMemoryBarrier barrier = makeBufferMemoryBarrier(writeSync.accessMask, readSync.accessMask,
				m_resource->getBuffer().handle, m_resource->getBuffer().offset, m_resource->getBuffer().size);
			vk.cmdWaitEvents(*cmdBuffer, 1u, &event.get(), writeSync.stageMask, readSync.stageMask, 0u, DE_NULL, 1u, &barrier, 0u, DE_NULL);
		}
		else if (m_resource->getType() == RESOURCE_TYPE_IMAGE)
		{
			const VkImageMemoryBarrier barrier =  makeImageMemoryBarrier(writeSync.accessMask, readSync.accessMask,
				writeSync.imageLayout, readSync.imageLayout, m_resource->getImage().handle, m_resource->getImage().subresourceRange);
			vk.cmdWaitEvents(*cmdBuffer, 1u, &event.get(), writeSync.stageMask, readSync.stageMask, 0u, DE_NULL, 0u, DE_NULL, 1u, &barrier);
		}

		m_readOp->recordCommands(*cmdBuffer);

		endCommandBuffer(vk, *cmdBuffer);
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);

		{
			const Data	expected = m_writeOp->getData();
			const Data	actual	 = m_readOp->getData();

			if (0 != deMemCmp(expected.data, actual.data, expected.size))
				return tcu::TestStatus::fail("Memory contents don't match");
		}

		return tcu::TestStatus::pass("OK");
	}
};

class BarrierTestInstance : public BaseTestInstance
{
public:
	BarrierTestInstance	(Context& context, const ResourceDescription& resourceDesc, const OperationSupport& writeOp, const OperationSupport& readOp, PipelineCacheData& pipelineCacheData)
		: BaseTestInstance		(context, resourceDesc, writeOp, readOp, pipelineCacheData)
	{
	}

	tcu::TestStatus iterate (void)
	{
		const DeviceInterface&			vk					= m_context.getDeviceInterface();
		const VkDevice					device				= m_context.getDevice();
		const VkQueue					queue				= m_context.getUniversalQueue();
		const deUint32					queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
		const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Move<VkCommandBuffer>		cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));
		const SyncInfo					writeSync			= m_writeOp->getSyncInfo();
		const SyncInfo					readSync			= m_readOp->getSyncInfo();

		beginCommandBuffer(vk, *cmdBuffer);

		m_writeOp->recordCommands(*cmdBuffer);

		if (m_resource->getType() == RESOURCE_TYPE_BUFFER || isIndirectBuffer(m_resource->getType()))
		{
			const VkBufferMemoryBarrier barrier = makeBufferMemoryBarrier(writeSync.accessMask, readSync.accessMask,
				m_resource->getBuffer().handle, m_resource->getBuffer().offset, m_resource->getBuffer().size);
			vk.cmdPipelineBarrier(*cmdBuffer,  writeSync.stageMask, readSync.stageMask, (VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 1u, &barrier, 0u, (const VkImageMemoryBarrier*)DE_NULL);
		}
		else if (m_resource->getType() == RESOURCE_TYPE_IMAGE)
		{
			const VkImageMemoryBarrier barrier =  makeImageMemoryBarrier(writeSync.accessMask, readSync.accessMask,
				writeSync.imageLayout, readSync.imageLayout, m_resource->getImage().handle, m_resource->getImage().subresourceRange);
			vk.cmdPipelineBarrier(*cmdBuffer,  writeSync.stageMask, readSync.stageMask, (VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 0u, (const VkBufferMemoryBarrier*)DE_NULL, 1u, &barrier);
		}

		m_readOp->recordCommands(*cmdBuffer);

		endCommandBuffer(vk, *cmdBuffer);

		submitCommandsAndWait(vk, device, queue, *cmdBuffer);

		{
			const Data	expected = m_writeOp->getData();
			const Data	actual	 = m_readOp->getData();

			if (0 != deMemCmp(expected.data, actual.data, expected.size))
				return tcu::TestStatus::fail("Memory contents don't match");
		}

		return tcu::TestStatus::pass("OK");
	}
};

class SemaphoreTestInstance : public BaseTestInstance
{
public:
	SemaphoreTestInstance (Context& context, const ResourceDescription& resourceDesc, const OperationSupport& writeOp, const OperationSupport& readOp, PipelineCacheData& pipelineCacheData)
		: BaseTestInstance	(context, resourceDesc, writeOp, readOp, pipelineCacheData)
	{
	}

	tcu::TestStatus	iterate (void)
	{
		enum {WRITE=0, READ, COUNT};
		const DeviceInterface&			vk					= m_context.getDeviceInterface();
		const VkDevice					device				= m_context.getDevice();
		const VkQueue					queue				= m_context.getUniversalQueue();
		const deUint32					queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
		const Unique<VkSemaphore>		semaphore			(createSemaphore (vk, device));
		const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Move<VkCommandBuffer>		ptrCmdBuffer[COUNT]	= {makeCommandBuffer(vk, device, *cmdPool), makeCommandBuffer(vk, device, *cmdPool)};
		VkCommandBuffer					cmdBuffers[COUNT]	= {*ptrCmdBuffer[WRITE], *ptrCmdBuffer[READ]};
		const VkPipelineStageFlags		stageBits[]			= { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
		const VkSubmitInfo				submitInfo[2]		=
															{
																{
																	VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType			sType;
																	DE_NULL,							// const void*				pNext;
																	0u,									// deUint32					waitSemaphoreCount;
																	DE_NULL,							// const VkSemaphore*		pWaitSemaphores;
																	(const VkPipelineStageFlags*)DE_NULL,
																	1u,									// deUint32					commandBufferCount;
																	&cmdBuffers[WRITE],					// const VkCommandBuffer*	pCommandBuffers;
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
																	&cmdBuffers[READ],					// const VkCommandBuffer*		pCommandBuffers;
																	0u,									// deUint32						signalSemaphoreCount;
																	DE_NULL,							// const VkSemaphore*			pSignalSemaphores;
																}
															};
		const SyncInfo					writeSync			= m_writeOp->getSyncInfo();
		const SyncInfo					readSync			= m_readOp->getSyncInfo();

		beginCommandBuffer(vk, cmdBuffers[WRITE]);

		m_writeOp->recordCommands(cmdBuffers[WRITE]);

		if (m_resource->getType() == RESOURCE_TYPE_IMAGE)
		{
			const VkImageMemoryBarrier barrier =  makeImageMemoryBarrier(writeSync.accessMask, readSync.accessMask,
				writeSync.imageLayout, readSync.imageLayout, m_resource->getImage().handle, m_resource->getImage().subresourceRange);
			vk.cmdPipelineBarrier(cmdBuffers[WRITE],  writeSync.stageMask, readSync.stageMask, (VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 0u, (const VkBufferMemoryBarrier*)DE_NULL, 1u, &barrier);
		}

		endCommandBuffer(vk, cmdBuffers[WRITE]);

		beginCommandBuffer(vk, cmdBuffers[READ]);

		m_readOp->recordCommands(cmdBuffers[READ]);

		endCommandBuffer(vk, cmdBuffers[READ]);

		VK_CHECK(vk.queueSubmit(queue, 2u, submitInfo, DE_NULL));
		VK_CHECK(vk.queueWaitIdle(queue));

		{
			const Data	expected = m_writeOp->getData();
			const Data	actual	 = m_readOp->getData();

			if (0 != deMemCmp(expected.data, actual.data, expected.size))
				return tcu::TestStatus::fail("Memory contents don't match");
		}

		return tcu::TestStatus::pass("OK");
	}
};

class FenceTestInstance : public BaseTestInstance
{
public:
	FenceTestInstance (Context& context, const ResourceDescription& resourceDesc, const OperationSupport& writeOp, const OperationSupport& readOp, PipelineCacheData& pipelineCacheData)
		: BaseTestInstance	(context, resourceDesc, writeOp, readOp, pipelineCacheData)
	{
	}

	tcu::TestStatus	iterate (void)
	{
		enum {WRITE=0, READ, COUNT};
		const DeviceInterface&			vk					= m_context.getDeviceInterface();
		const VkDevice					device				= m_context.getDevice();
		const VkQueue					queue				= m_context.getUniversalQueue();
		const deUint32					queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
		const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
		const Move<VkCommandBuffer>		ptrCmdBuffer[COUNT]	= {makeCommandBuffer(vk, device, *cmdPool), makeCommandBuffer(vk, device, *cmdPool)};
		VkCommandBuffer					cmdBuffers[COUNT]	= {*ptrCmdBuffer[WRITE], *ptrCmdBuffer[READ]};
		const SyncInfo					writeSync			= m_writeOp->getSyncInfo();
		const SyncInfo					readSync			= m_readOp->getSyncInfo();

		beginCommandBuffer(vk, cmdBuffers[WRITE]);

		m_writeOp->recordCommands(cmdBuffers[WRITE]);

		if (m_resource->getType() == RESOURCE_TYPE_IMAGE)
		{
			const VkImageMemoryBarrier barrier =  makeImageMemoryBarrier(writeSync.accessMask, readSync.accessMask,
				writeSync.imageLayout, readSync.imageLayout, m_resource->getImage().handle, m_resource->getImage().subresourceRange);
			vk.cmdPipelineBarrier(cmdBuffers[WRITE],  writeSync.stageMask, readSync.stageMask, (VkDependencyFlags)0, 0u, (const VkMemoryBarrier*)DE_NULL, 0u, (const VkBufferMemoryBarrier*)DE_NULL, 1u, &barrier);
		}

		endCommandBuffer(vk, cmdBuffers[WRITE]);

		submitCommandsAndWait(vk, device, queue, cmdBuffers[WRITE]);

		beginCommandBuffer(vk, cmdBuffers[READ]);

		m_readOp->recordCommands(cmdBuffers[READ]);

		endCommandBuffer(vk, cmdBuffers[READ]);

		submitCommandsAndWait(vk, device, queue, cmdBuffers[READ]);

		{
			const Data	expected = m_writeOp->getData();
			const Data	actual	 = m_readOp->getData();

			if (0 != deMemCmp(expected.data, actual.data, expected.size))
				return tcu::TestStatus::fail("Memory contents don't match");
		}

		return tcu::TestStatus::pass("OK");
	}
};

class SyncTestCase : public TestCase
{
public:
	SyncTestCase	(tcu::TestContext&			testCtx,
					 const std::string&			name,
					 const std::string&			description,
					 const SyncPrimitive		syncPrimitive,
					 const ResourceDescription	resourceDesc,
					 const OperationName		writeOp,
					 const OperationName		readOp,
					 PipelineCacheData&			pipelineCacheData)
		: TestCase				(testCtx, name, description)
		, m_resourceDesc		(resourceDesc)
		, m_writeOp				(makeOperationSupport(writeOp, resourceDesc))
		, m_readOp				(makeOperationSupport(readOp, resourceDesc))
		, m_syncPrimitive		(syncPrimitive)
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
				return new FenceTestInstance(context, m_resourceDesc, *m_writeOp, *m_readOp, m_pipelineCacheData);
			case SYNC_PRIMITIVE_SEMAPHORE:
				return new SemaphoreTestInstance(context, m_resourceDesc, *m_writeOp, *m_readOp, m_pipelineCacheData);
			case SYNC_PRIMITIVE_BARRIER:
				return new BarrierTestInstance(context, m_resourceDesc, *m_writeOp, *m_readOp, m_pipelineCacheData);
			case SYNC_PRIMITIVE_EVENT:
				return new EventTestInstance(context, m_resourceDesc, *m_writeOp, *m_readOp, m_pipelineCacheData);
		}

		DE_ASSERT(0);
		return DE_NULL;
	}

private:
	const ResourceDescription				m_resourceDesc;
	const de::UniquePtr<OperationSupport>	m_writeOp;
	const de::UniquePtr<OperationSupport>	m_readOp;
	const SyncPrimitive						m_syncPrimitive;
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
		{ "fence",		SYNC_PRIMITIVE_FENCE,		0, },
		{ "semaphore",	SYNC_PRIMITIVE_SEMAPHORE,	0, },
		{ "barrier",	SYNC_PRIMITIVE_BARRIER,		1, },
		{ "event",		SYNC_PRIMITIVE_EVENT,		1, },
	};

	for (int groupNdx = 0; groupNdx < DE_LENGTH_OF_ARRAY(groups); ++groupNdx)
	{
		de::MovePtr<tcu::TestCaseGroup> synchGroup (new tcu::TestCaseGroup(testCtx, groups[groupNdx].name, ""));

		for (int writeOpNdx = 0; writeOpNdx < DE_LENGTH_OF_ARRAY(s_writeOps); ++writeOpNdx)
		for (int readOpNdx = 0; readOpNdx < DE_LENGTH_OF_ARRAY(s_readOps); ++readOpNdx)
		{
			const OperationName	writeOp		= s_writeOps[writeOpNdx];
			const OperationName	readOp		= s_readOps[readOpNdx];
			const std::string	opGroupName = getOperationName(writeOp) + "_" + getOperationName(readOp);
			bool				empty		= true;

			de::MovePtr<tcu::TestCaseGroup> opGroup	(new tcu::TestCaseGroup(testCtx, opGroupName.c_str(), ""));

			for (int resourceNdx = 0; resourceNdx < DE_LENGTH_OF_ARRAY(s_resources); ++resourceNdx)
			{
				const ResourceDescription&	resource	= s_resources[resourceNdx];
				std::string					name		= getResourceName(resource);

				if (isResourceSupported(writeOp, resource) && isResourceSupported(readOp, resource))
				{
					opGroup->addChild(new SyncTestCase(testCtx, name, "", groups[groupNdx].syncPrimitive, resource, writeOp, readOp, *pipelineCacheData));
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

tcu::TestCaseGroup* createSynchronizedOperationSingleQueueTests (tcu::TestContext& testCtx, PipelineCacheData& pipelineCacheData)
{
	return createTestGroup(testCtx, "single_queue", "Synchronization of a memory-modifying operation", createTests, &pipelineCacheData);
}

} // synchronization
} // vkt
