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
 * \brief Synchronization event basic tests
 *//*--------------------------------------------------------------------*/

#include "vktSynchronizationBasicEventTests.hpp"
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
#define SHORT_FENCE_WAIT	1000ull
#define LONG_FENCE_WAIT		~0ull

tcu::TestStatus hostResetSetEventCase (Context& context)
{
	const DeviceInterface&		vk			= context.getDeviceInterface();
	const VkDevice				device		= context.getDevice();
	const VkEventCreateInfo		eventInfo	=
											{
												VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
												DE_NULL,
												0
											};
	VkEvent						event;
	Move<VkEvent>				ptrEvent;

	if (VK_SUCCESS != vk.createEvent(device, &eventInfo, DE_NULL, &event))
		return tcu::TestStatus::fail("Couldn't create event");

	ptrEvent = Move<VkEvent>(check<VkEvent>(event), Deleter<VkEvent>(vk, device, DE_NULL));

	if (VK_EVENT_RESET != vk.getEventStatus(device, event))
		return tcu::TestStatus::fail("Created event should be in unsignaled state");

	if (VK_SUCCESS != vk.setEvent(device, event))
		return tcu::TestStatus::fail("Couldn't set event");

	if (VK_EVENT_SET != vk.getEventStatus(device, event))
		return tcu::TestStatus::fail("Event should be in signaled state after set");

	if (VK_SUCCESS != vk.resetEvent(device, event))
		return tcu::TestStatus::fail("Couldn't reset event");

	if (VK_EVENT_RESET != vk.getEventStatus(device, event))
		return tcu::TestStatus::fail("Event should be in unsignaled state after reset");

	return tcu::TestStatus::pass("Tests set and reset event on host pass");
}

tcu::TestStatus deviceResetSetEventCase (Context& context)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));
	const VkSubmitInfo				submitInfo			=
														{
															VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
															DE_NULL,						// const void*					pNext;
															0u,								// deUint32						waitSemaphoreCount;
															DE_NULL,						// const VkSemaphore*			pWaitSemaphores;
															DE_NULL,						// const VkPipelineStageFlags*	pWaitDstStageMask;
															1u,								// deUint32						commandBufferCount;
															&cmdBuffer.get(),				// const VkCommandBuffer*		pCommandBuffers;
															0u,								// deUint32						signalSemaphoreCount;
															DE_NULL,						// const VkSemaphore*			pSignalSemaphores;
														};
	const Unique<VkEvent>			event				(createEvent(vk, device));

	beginCommandBuffer(vk, *cmdBuffer);
	vk.cmdSetEvent(*cmdBuffer, *event, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	endCommandBuffer(vk, *cmdBuffer);

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, DE_NULL));
	VK_CHECK(vk.queueWaitIdle(queue));

	if (VK_EVENT_SET != vk.getEventStatus(device, *event))
		return tcu::TestStatus::fail("Event should be in signaled state after set");

	beginCommandBuffer(vk, *cmdBuffer);
	vk.cmdResetEvent(*cmdBuffer, *event, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	endCommandBuffer(vk, *cmdBuffer);

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, DE_NULL));
	VK_CHECK(vk.queueWaitIdle(queue));

	if (VK_EVENT_RESET != vk.getEventStatus(device, *event))
		return tcu::TestStatus::fail("Event should be in unsignaled state after set");

	return tcu::TestStatus::pass("Device set and reset event tests pass");
}

tcu::TestStatus deviceWaitForEventCase (Context& context)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const Unique<VkFence>			fence				(createFence(vk, device));
	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));
	const VkSubmitInfo				submitInfo			=
														{
															VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
															DE_NULL,						// const void*					pNext;
															0u,								// deUint32						waitSemaphoreCount;
															DE_NULL,						// const VkSemaphore*			pWaitSemaphores;
															DE_NULL,						// const VkPipelineStageFlags*	pWaitDstStageMask;
															1u,								// deUint32						commandBufferCount;
															&cmdBuffer.get(),				// const VkCommandBuffer*		pCommandBuffers;
															0u,								// deUint32						signalSemaphoreCount;
															DE_NULL,						// const VkSemaphore*			pSignalSemaphores;
														};
	const VkEventCreateInfo			eventInfo			=
														{
															VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
															DE_NULL,
															0
														};
	const Unique<VkEvent>			event				(createEvent(vk, device, &eventInfo, DE_NULL));

	beginCommandBuffer(vk, *cmdBuffer);
	vk.cmdWaitEvents(*cmdBuffer, 1u, &event.get(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0u, DE_NULL, 0u, DE_NULL, 0u, DE_NULL);
	endCommandBuffer(vk, *cmdBuffer);

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
	if (VK_TIMEOUT != vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, SHORT_FENCE_WAIT))
		return tcu::TestStatus::fail("Queue should not end execution");

	if (VK_SUCCESS != vk.setEvent(device, *event))
		return tcu::TestStatus::fail("Couldn't set event");

	if (VK_SUCCESS != vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, LONG_FENCE_WAIT))
		return tcu::TestStatus::fail("Queue should end execution");

	return tcu::TestStatus::pass("Device wait for event tests pass");
}

tcu::TestStatus singleSubmissionCase (Context& context)
{
	enum {SET=0, WAIT, COUNT};
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const Unique<VkFence>			fence				(createFence(vk, device));
	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Move<VkCommandBuffer>		ptrCmdBuffer[COUNT]	= {makeCommandBuffer(vk, device, *cmdPool), makeCommandBuffer(vk, device, *cmdPool)};
	VkCommandBuffer					cmdBuffers[COUNT]	= {*ptrCmdBuffer[SET], *ptrCmdBuffer[WAIT]};
	const VkSubmitInfo				submitInfo			=
														{
															VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
															DE_NULL,						// const void*					pNext;
															0u,								// deUint32						waitSemaphoreCount;
															DE_NULL,						// const VkSemaphore*			pWaitSemaphores;
															DE_NULL,						// const VkPipelineStageFlags*	pWaitDstStageMask;
															2u,								// deUint32						commandBufferCount;
															cmdBuffers,						// const VkCommandBuffer*		pCommandBuffers;
															0u,								// deUint32						signalSemaphoreCount;
															DE_NULL,						// const VkSemaphore*			pSignalSemaphores;
														};
	const Unique<VkEvent>			event				(createEvent(vk, device));

	beginCommandBuffer(vk, cmdBuffers[SET]);
	vk.cmdSetEvent(cmdBuffers[SET], *event, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	endCommandBuffer(vk, cmdBuffers[SET]);

	beginCommandBuffer(vk, cmdBuffers[WAIT]);
	vk.cmdWaitEvents(cmdBuffers[WAIT], 1u, &event.get(),VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0u, DE_NULL, 0u, DE_NULL, 0u, DE_NULL);
	endCommandBuffer(vk, cmdBuffers[WAIT]);

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
	if (VK_SUCCESS != vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, LONG_FENCE_WAIT))
		return tcu::TestStatus::fail("Queue should end execution");

	return tcu::TestStatus::pass("Wait and set even on device single submission tests pass");
}

tcu::TestStatus multiSubmissionCase (Context& context)
{
	enum {SET=0, WAIT, COUNT};
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const Move<VkFence>				ptrFence[COUNT]		=
	{
		createFence(vk, device),
		createFence(vk, device)
	};
	VkFence							fence[COUNT]		= {*ptrFence[SET], *ptrFence[WAIT]};
	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Move<VkCommandBuffer>		ptrCmdBuffer[COUNT]	= {makeCommandBuffer(vk, device, *cmdPool), makeCommandBuffer(vk, device, *cmdPool)};
	VkCommandBuffer					cmdBuffers[COUNT]	= {*ptrCmdBuffer[SET], *ptrCmdBuffer[WAIT]};
	const VkSubmitInfo				submitInfo[COUNT]	=
														{
															{
																VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
																DE_NULL,						// const void*					pNext;
																0u,								// deUint32						waitSemaphoreCount;
																DE_NULL,						// const VkSemaphore*			pWaitSemaphores;
																DE_NULL,						// const VkPipelineStageFlags*	pWaitDstStageMask;
																1u,								// deUint32						commandBufferCount;
																&cmdBuffers[SET],				// const VkCommandBuffer*		pCommandBuffers;
																0u,								// deUint32						signalSemaphoreCount;
																DE_NULL,						// const VkSemaphore*			pSignalSemaphores;
															},
															{
																VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
																DE_NULL,						// const void*					pNext;
																0u,								// deUint32						waitSemaphoreCount;
																DE_NULL,						// const VkSemaphore*			pWaitSemaphores;
																DE_NULL,						// const VkPipelineStageFlags*	pWaitDstStageMask;
																1u,								// deUint32						commandBufferCount;
																&cmdBuffers[WAIT],				// const VkCommandBuffer*		pCommandBuffers;
																0u,								// deUint32						signalSemaphoreCount;
																DE_NULL,						// const VkSemaphore*			pSignalSemaphores;
															}
														};
	const Unique<VkEvent>			event				(createEvent(vk, device));

	beginCommandBuffer(vk, cmdBuffers[SET]);
	vk.cmdSetEvent(cmdBuffers[SET], *event, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	endCommandBuffer(vk, cmdBuffers[SET]);

	beginCommandBuffer(vk, cmdBuffers[WAIT]);
	vk.cmdWaitEvents(cmdBuffers[WAIT], 1u, &event.get(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0u, DE_NULL, 0u, DE_NULL, 0u, DE_NULL);
	endCommandBuffer(vk, cmdBuffers[WAIT]);

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo[SET], fence[SET]));
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo[WAIT], fence[WAIT]));

	if (VK_SUCCESS != vk.waitForFences(device, 2u, fence, DE_TRUE, LONG_FENCE_WAIT))
		return tcu::TestStatus::fail("Queue should end execution");

	return tcu::TestStatus::pass("Wait and set even on device multi submission tests pass");
}

tcu::TestStatus secondaryCommandBufferCase (Context& context)
{
	enum {SET=0, WAIT, COUNT};
	const DeviceInterface&					vk						= context.getDeviceInterface();
	const VkDevice							device					= context.getDevice();
	const VkQueue							queue					= context.getUniversalQueue();
	const deUint32							queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	const Unique<VkFence>					fence					(createFence(vk, device));
	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Move<VkCommandBuffer>				primaryCmdBuffer		(makeCommandBuffer(vk, device, *cmdPool));
	const VkCommandBufferAllocateInfo		cmdBufferInfo			=
																	{
																		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,		// VkStructureType		sType;
																		DE_NULL,											// const void*			pNext;
																		*cmdPool,											// VkCommandPool		commandPool;
																		VK_COMMAND_BUFFER_LEVEL_SECONDARY,					// VkCommandBufferLevel	level;
																		1u,													// deUint32				commandBufferCount;
																	};
	const Move<VkCommandBuffer>				prtCmdBuffers[COUNT]	= {allocateCommandBuffer (vk, device, &cmdBufferInfo), allocateCommandBuffer (vk, device, &cmdBufferInfo)};
	VkCommandBuffer							secondaryCmdBuffers[]	= {*prtCmdBuffers[SET], *prtCmdBuffers[WAIT]};
	const VkSubmitInfo						submitInfo				=
																	{
																		VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
																		DE_NULL,						// const void*					pNext;
																		0u,								// deUint32						waitSemaphoreCount;
																		DE_NULL,						// const VkSemaphore*			pWaitSemaphores;
																		DE_NULL,						// const VkPipelineStageFlags*	pWaitDstStageMask;
																		1u,								// deUint32						commandBufferCount;
																		&primaryCmdBuffer.get(),		// const VkCommandBuffer*		pCommandBuffers;
																		0u,								// deUint32						signalSemaphoreCount;
																		DE_NULL,						// const VkSemaphore*			pSignalSemaphores;
																	};
	const Unique<VkEvent>					event					(createEvent(vk, device));

	const VkCommandBufferInheritanceInfo	secCmdBufInheritInfo	=
																	{
																		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,	//VkStructureType					sType;
																		DE_NULL,											//const void*						pNext;
																		DE_NULL,											//VkRenderPass					renderPass;
																		0u,													//deUint32						subpass;
																		DE_NULL,											//VkFramebuffer					framebuffer;
																		VK_FALSE,											//VkBool32						occlusionQueryEnable;
																		(VkQueryControlFlags)0u,							//VkQueryControlFlags				queryFlags;
																		(VkQueryPipelineStatisticFlags)0u,					//VkQueryPipelineStatisticFlags	pipelineStatistics;
																	};
	const VkCommandBufferBeginInfo			cmdBufferBeginInfo		=
																	{
																		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType                          sType;
																		DE_NULL,										// const void*                              pNext;
																		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,	// VkCommandBufferUsageFlags                flags;
																		&secCmdBufInheritInfo,							// const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
																	};

	VK_CHECK(vk.beginCommandBuffer(secondaryCmdBuffers[SET], &cmdBufferBeginInfo));
	vk.cmdSetEvent(secondaryCmdBuffers[SET], *event, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	endCommandBuffer(vk, secondaryCmdBuffers[SET]);

	VK_CHECK(vk.beginCommandBuffer(secondaryCmdBuffers[WAIT], &cmdBufferBeginInfo));
	vk.cmdWaitEvents(secondaryCmdBuffers[WAIT], 1u, &event.get(),VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0u, DE_NULL, 0u, DE_NULL, 0u, DE_NULL);
	endCommandBuffer(vk, secondaryCmdBuffers[WAIT]);

	beginCommandBuffer(vk, *primaryCmdBuffer);
	vk.cmdExecuteCommands(*primaryCmdBuffer, 2u, secondaryCmdBuffers);
	endCommandBuffer(vk, *primaryCmdBuffer);

	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
	if (VK_SUCCESS != vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, LONG_FENCE_WAIT))
		return tcu::TestStatus::fail("Queue should end execution");

	return tcu::TestStatus::pass("Wait and set even on device using secondary command buffers tests pass");
}

} // anonymous

tcu::TestCaseGroup* createBasicEventTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> basicTests(new tcu::TestCaseGroup(testCtx, "event", "Basic event tests"));
	addFunctionCase(basicTests.get(), "host_set_reset",   "Basic event tests set and reset on host", hostResetSetEventCase);
	addFunctionCase(basicTests.get(), "device_set_reset", "Basic event tests set and reset on device", deviceResetSetEventCase);
	addFunctionCase(basicTests.get(), "host_set_device_wait", "Wait for event on device test", deviceWaitForEventCase);
	addFunctionCase(basicTests.get(), "single_submit_multi_command_buffer", "Wait and set event single submission on device", singleSubmissionCase);
	addFunctionCase(basicTests.get(), "multi_submit_multi_command_buffer", "Wait and set event mutli submission on device", multiSubmissionCase);
	addFunctionCase(basicTests.get(), "multi_secondary_command_buffer", "Event used on secondary command buffer ", secondaryCommandBufferCase);

	return basicTests.release();
}

} // synchronization
} // vkt
