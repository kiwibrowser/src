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
 * \brief Synchronization fence basic tests
 *//*--------------------------------------------------------------------*/

#include "vktSynchronizationBasicFenceTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktSynchronizationUtil.hpp"

#include "vkDefs.hpp"
#include "vkPlatform.hpp"
#include "vkRef.hpp"

namespace vkt
{
namespace synchronization
{
namespace
{
using namespace vk;

static const deUint64	SHORT_FENCE_WAIT	= 1000ull;
static const deUint64	LONG_FENCE_WAIT		= ~0ull;

tcu::TestStatus basicOneFenceCase (Context& context)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));
	const VkFenceCreateInfo			fenceInfo			=
														{
															VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, // VkStructureType		sType;
															DE_NULL,							 // const void*			pNext;
															0u,									 // VkFenceCreateFlags	flags;
														};
	const Unique<VkFence>			fence				(createFence(vk, device, &fenceInfo));
	const VkSubmitInfo				submitInfo			=
														{
															VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType			sType;
															DE_NULL,							// const void*				pNext;
															0u,									// deUint32					waitSemaphoreCount;
															DE_NULL,							// const VkSemaphore*		pWaitSemaphores;
															(const VkPipelineStageFlags*)DE_NULL,
															1u,									// deUint32					commandBufferCount;
															&cmdBuffer.get(),					// const VkCommandBuffer*	pCommandBuffers;
															0u,									// deUint32					signalSemaphoreCount;
															DE_NULL,							// const VkSemaphore*		pSignalSemaphores;
														};

	if (VK_NOT_READY != vk.getFenceStatus(device, *fence))
		return tcu::TestStatus::fail("Created fence should be in unsignaled state");

	if (VK_TIMEOUT != vk.waitForFences(device, 1u, &fence.get(), VK_TRUE, SHORT_FENCE_WAIT))
		return tcu::TestStatus::fail("vkWaitForFences should return VK_TIMEOUT");

	if (VK_NOT_READY != vk.getFenceStatus(device, *fence))
		return tcu::TestStatus::fail("Created fence should be in unsignaled state");

	beginCommandBuffer(vk, *cmdBuffer);
	endCommandBuffer(vk, *cmdBuffer);

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));

	if (VK_SUCCESS != vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, LONG_FENCE_WAIT))
		return tcu::TestStatus::fail("vkWaitForFences should return VK_SUCCESS");

	if (VK_SUCCESS != vk.getFenceStatus(device, *fence))
		return tcu::TestStatus::fail("Fence should be in signaled state");

	if (VK_SUCCESS != vk.resetFences(device, 1u, &fence.get()))
		return tcu::TestStatus::fail("Couldn't reset the fence");

	if (VK_NOT_READY != vk.getFenceStatus(device, *fence))
		return tcu::TestStatus::fail("Fence after reset should be in unsignaled state");

	return tcu::TestStatus::pass("Basic one fence tests passed");
}

tcu::TestStatus basicMultiFenceCase (Context& context)
{
	enum{FISRT_FENCE=0,SECOND_FENCE};
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,  queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));
	const VkFenceCreateInfo			fenceInfo			=
														{
															VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, // VkStructureType		sType;
															DE_NULL,							 // const void*			pNext;
															0u,									 // VkFenceCreateFlags	flags;
														};
	const Move<VkFence>				ptrFence[2]			= { createFence(vk, device, &fenceInfo), createFence(vk, device, &fenceInfo) };
	const VkFence					fence[2]			= { *ptrFence[FISRT_FENCE], *ptrFence[SECOND_FENCE] };
	const VkCommandBufferBeginInfo	info				=
														{
															VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType                          sType;
															DE_NULL,										// const void*                              pNext;
															VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,	// VkCommandBufferUsageFlags                flags;
															DE_NULL,										// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
														};
	const VkSubmitInfo				submitInfo			=
														{
															VK_STRUCTURE_TYPE_SUBMIT_INFO,		// VkStructureType			sType;
															DE_NULL,							// const void*				pNext;
															0u,									// deUint32					waitSemaphoreCount;
															DE_NULL,							// const VkSemaphore*		pWaitSemaphores;
															(const VkPipelineStageFlags*)DE_NULL,
															1u,									// deUint32					commandBufferCount;
															&cmdBuffer.get(),					// const VkCommandBuffer*	pCommandBuffers;
															0u,									// deUint32					signalSemaphoreCount;
															DE_NULL,							// const VkSemaphore*		pSignalSemaphores;
														};


	VK_CHECK(vk.beginCommandBuffer(*cmdBuffer, &info));
	endCommandBuffer(vk, *cmdBuffer);

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, fence[FISRT_FENCE]));

	if (VK_SUCCESS != vk.waitForFences(device, 1u, &fence[FISRT_FENCE], DE_FALSE, LONG_FENCE_WAIT))
		return tcu::TestStatus::fail("vkWaitForFences should return VK_SUCCESS");

	if (VK_SUCCESS != vk.resetFences(device, 1u, &fence[FISRT_FENCE]))
		return tcu::TestStatus::fail("Couldn't reset the fence");

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, fence[FISRT_FENCE]));

	if (VK_TIMEOUT != vk.waitForFences(device, 2u, &fence[FISRT_FENCE], DE_TRUE, SHORT_FENCE_WAIT))
		return tcu::TestStatus::fail("vkWaitForFences should return VK_TIMEOUT");

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, fence[SECOND_FENCE]));

	if (VK_SUCCESS != vk.waitForFences(device, 2u, &fence[FISRT_FENCE], DE_TRUE, LONG_FENCE_WAIT))
		return tcu::TestStatus::fail("vkWaitForFences should return VK_SUCCESS");

	return tcu::TestStatus::pass("Basic multi fence tests passed");
}

tcu::TestStatus emptySubmitCase (Context& context)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const VkFenceCreateInfo			fenceCreateInfo		=
														{
															VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,		// VkStructureType       sType;
															DE_NULL,									// const void*           pNext;
															(VkFenceCreateFlags)0,						// VkFenceCreateFlags    flags;
														};
	const Unique<VkFence>			fence				(createFence(vk, device, &fenceCreateInfo));

	VK_CHECK(vk.queueSubmit(queue, 0u, DE_NULL, *fence));

	if (VK_SUCCESS != vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, LONG_FENCE_WAIT))
		return tcu::TestStatus::fail("vkWaitForFences should return VK_SUCCESS");

	return tcu::TestStatus::pass("OK");
}

} // anonymous

tcu::TestCaseGroup* createBasicFenceTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> basicFenceTests(new tcu::TestCaseGroup(testCtx, "fence", "Basic fence tests"));
	addFunctionCase(basicFenceTests.get(),	 "one",				"Basic one fence tests",							basicOneFenceCase);
	addFunctionCase(basicFenceTests.get(),	 "multi",			"Basic multi fence tests",							basicMultiFenceCase);
	addFunctionCase(basicFenceTests.get(),	 "empty_submit",	"Signal a fence after an empty queue submission",	emptySubmitCase);

	return basicFenceTests.release();
}

} // synchronization
} // vkt
