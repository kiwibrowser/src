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
 * \file  vktSparseResourcesBase.cpp
 * \brief Sparse Resources Base Instance
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesBase.hpp"
#include "vkMemUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkQueryUtil.hpp"

using namespace vk;

namespace vkt
{
namespace sparse
{
namespace
{

struct QueueFamilyQueuesCount
{
	QueueFamilyQueuesCount() : queueCount(0u) {};

	deUint32 queueCount;
};

static const deUint32 NO_MATCH_FOUND = ~0u;

deUint32 findMatchingQueueFamilyIndex (const std::vector<vk::VkQueueFamilyProperties>&	queueFamilyProperties,
									   const VkQueueFlags								queueFlags,
									   const deUint32									startIndex)
{
	for (deUint32 queueNdx = startIndex; queueNdx < queueFamilyProperties.size(); ++queueNdx)
	{
		if ((queueFamilyProperties[queueNdx].queueFlags & queueFlags) == queueFlags)
			return queueNdx;
	}

	return NO_MATCH_FOUND;
}

} // anonymous

void SparseResourcesBaseInstance::createDeviceSupportingQueues(const QueueRequirementsVec& queueRequirements)
{
	typedef std::map<vk::VkQueueFlags, std::vector<Queue> >		QueuesMap;
	typedef std::map<deUint32, QueueFamilyQueuesCount>			SelectedQueuesMap;
	typedef std::map<deUint32, std::vector<float> >				QueuePrioritiesMap;

	const InstanceInterface&	instance		= m_context.getInstanceInterface();
	const VkPhysicalDevice		physicalDevice	= m_context.getPhysicalDevice();

	deUint32 queueFamilyPropertiesCount = 0u;
	instance.getPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, DE_NULL);

	if(queueFamilyPropertiesCount == 0u)
		TCU_THROW(ResourceError, "Device reports an empty set of queue family properties");

	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	queueFamilyProperties.resize(queueFamilyPropertiesCount);

	instance.getPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, &queueFamilyProperties[0]);

	if (queueFamilyPropertiesCount == 0u)
		TCU_THROW(ResourceError, "Device reports an empty set of queue family properties");

	SelectedQueuesMap	selectedQueueFamilies;
	QueuePrioritiesMap	queuePriorities;

	for (deUint32 queueReqNdx = 0; queueReqNdx < queueRequirements.size(); ++queueReqNdx)
	{
		const QueueRequirements& queueRequirement = queueRequirements[queueReqNdx];

		deUint32 queueFamilyIndex	= 0u;
		deUint32 queuesFoundCount	= 0u;

		do
		{
			queueFamilyIndex = findMatchingQueueFamilyIndex(queueFamilyProperties, queueRequirement.queueFlags, queueFamilyIndex);

			if (queueFamilyIndex == NO_MATCH_FOUND)
				TCU_THROW(NotSupportedError, "No match found for queue requirements");

			const deUint32 queuesPerFamilyCount = deMin32(queueFamilyProperties[queueFamilyIndex].queueCount, queueRequirement.queueCount - queuesFoundCount);

			selectedQueueFamilies[queueFamilyIndex].queueCount = deMax32(queuesPerFamilyCount, selectedQueueFamilies[queueFamilyIndex].queueCount);

			for (deUint32 queueNdx = 0; queueNdx < queuesPerFamilyCount; ++queueNdx)
			{
				Queue queue;
				queue.queueFamilyIndex	= queueFamilyIndex;
				queue.queueIndex		= queueNdx;

				m_queues[queueRequirement.queueFlags].push_back(queue);
			}

			queuesFoundCount += queuesPerFamilyCount;

			++queueFamilyIndex;
		} while (queuesFoundCount < queueRequirement.queueCount);
	}

	std::vector<VkDeviceQueueCreateInfo> queueInfos;

	for (SelectedQueuesMap::iterator queueFamilyIter = selectedQueueFamilies.begin(); queueFamilyIter != selectedQueueFamilies.end(); ++queueFamilyIter)
	{
		for (deUint32 queueNdx = 0; queueNdx < queueFamilyIter->second.queueCount; ++queueNdx)
			queuePriorities[queueFamilyIter->first].push_back(1.0f);

		const VkDeviceQueueCreateInfo queueInfo =
		{
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,		// VkStructureType             sType;
			DE_NULL,										// const void*                 pNext;
			(VkDeviceQueueCreateFlags)0u,					// VkDeviceQueueCreateFlags    flags;
			queueFamilyIter->first,							// uint32_t                    queueFamilyIndex;
			queueFamilyIter->second.queueCount,				// uint32_t                    queueCount;
			&queuePriorities[queueFamilyIter->first][0],	// const float*                pQueuePriorities;
		};

		queueInfos.push_back(queueInfo);
	}

	const VkPhysicalDeviceFeatures	deviceFeatures	= getPhysicalDeviceFeatures(instance, physicalDevice);
	const VkDeviceCreateInfo		deviceInfo		=
	{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,		// VkStructureType                    sType;
		DE_NULL,									// const void*                        pNext;
		(VkDeviceCreateFlags)0,						// VkDeviceCreateFlags                flags;
		static_cast<deUint32>(queueInfos.size()),	// uint32_t                           queueCreateInfoCount;
		&queueInfos[0],								// const VkDeviceQueueCreateInfo*     pQueueCreateInfos;
		0u,											// uint32_t                           enabledLayerCount;
		DE_NULL,									// const char* const*                 ppEnabledLayerNames;
		0u,											// uint32_t                           enabledExtensionCount;
		DE_NULL,									// const char* const*                 ppEnabledExtensionNames;
		&deviceFeatures,							// const VkPhysicalDeviceFeatures*    pEnabledFeatures;
	};

	m_logicalDevice = createDevice(instance, physicalDevice, &deviceInfo);
	m_deviceDriver	= de::MovePtr<DeviceDriver>(new DeviceDriver(instance, *m_logicalDevice));
	m_allocator		= de::MovePtr<Allocator>(new SimpleAllocator(*m_deviceDriver, *m_logicalDevice, getPhysicalDeviceMemoryProperties(instance, physicalDevice)));

	for (QueuesMap::iterator queuesIter = m_queues.begin(); queuesIter != m_queues.end(); ++queuesIter)
	{
		for (deUint32 queueNdx = 0u; queueNdx < queuesIter->second.size(); ++queueNdx)
		{
			Queue& queue = queuesIter->second[queueNdx];

			queue.queueHandle = getDeviceQueue(*m_deviceDriver, *m_logicalDevice, queue.queueFamilyIndex, queue.queueIndex);
		}
	}
}

const Queue& SparseResourcesBaseInstance::getQueue (const VkQueueFlags queueFlags, const deUint32 queueIndex) const
{
	return m_queues.find(queueFlags)->second[queueIndex];
}

} // sparse
} // vkt
