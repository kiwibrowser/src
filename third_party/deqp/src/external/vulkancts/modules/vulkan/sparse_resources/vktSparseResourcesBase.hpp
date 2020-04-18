#ifndef _VKTSPARSERESOURCESBASE_HPP
#define _VKTSPARSERESOURCESBASE_HPP
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
 * \file  vktSparseResourcesBase.hpp
 * \brief Sparse Resources Base Instance
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vktTestCase.hpp"
#include "vkRef.hpp"
#include "vkPlatform.hpp"
#include "deUniquePtr.hpp"

#include <map>
#include <vector>

namespace vkt
{
namespace sparse
{

struct Queue
{
	vk::VkQueue	queueHandle;
	deUint32	queueFamilyIndex;
	deUint32	queueIndex;
};

struct QueueRequirements
{
	QueueRequirements(const vk::VkQueueFlags qFlags, const deUint32	qCount)
		: queueFlags(qFlags)
		, queueCount(qCount)
	{}

	vk::VkQueueFlags	queueFlags;
	deUint32			queueCount;
};

class SparseResourcesBaseInstance : public TestInstance
{
public:
	SparseResourcesBaseInstance (Context &context) : TestInstance(context) {}

protected:
	typedef std::vector<QueueRequirements>				QueueRequirementsVec;

	void												createDeviceSupportingQueues	(const QueueRequirementsVec& queueRequirements);
	const Queue&										getQueue						(const vk::VkQueueFlags queueFlags, const deUint32 queueIndex) const;
	const vk::DeviceInterface&							getDeviceInterface				(void) const { return *m_deviceDriver; }
	vk::VkDevice										getDevice						(void) const { return *m_logicalDevice; }
	vk::Allocator&										getAllocator					(void)		 { return *m_allocator; }

private:
	std::map<vk::VkQueueFlags, std::vector<Queue> >		m_queues;
	de::MovePtr<vk::DeviceDriver>						m_deviceDriver;
	vk::Move<vk::VkDevice>								m_logicalDevice;
	de::MovePtr<vk::Allocator>							m_allocator;
};

} // sparse
} // vkt

#endif // _VKTSPARSERESOURCESBASE_HPP
