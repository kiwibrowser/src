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
 * \brief Synchronization tests for resources shared between instances.
 *//*--------------------------------------------------------------------*/

#include "vktSynchronizationCrossInstanceSharingTests.hpp"

#include "vkDeviceUtil.hpp"
#include "vkPlatform.hpp"

#include "vktTestCaseUtil.hpp"

#include "vktSynchronizationUtil.hpp"
#include "vktSynchronizationOperation.hpp"
#include "vktSynchronizationOperationTestData.hpp"
#include "vktSynchronizationOperationResources.hpp"
#include "vktExternalMemoryUtil.hpp"

#include "tcuResultCollector.hpp"
#include "tcuTestLog.hpp"

using tcu::TestLog;
using namespace vkt::ExternalMemoryUtil;

namespace vkt
{
namespace synchronization
{
namespace
{

struct TestConfig
{
								TestConfig		(const ResourceDescription&						resource_,
												 OperationName									writeOp_,
												 OperationName									readOp_,
												 vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryHandleType_,
												 vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	semaphoreHandleType_,
												 bool											dedicated_)
		: resource				(resource_)
		, writeOp				(writeOp_)
		, readOp				(readOp_)
		, memoryHandleType		(memoryHandleType_)
		, semaphoreHandleType	(semaphoreHandleType_)
		, dedicated				(dedicated_)
	{
	}

	const ResourceDescription							resource;
	const OperationName									writeOp;
	const OperationName									readOp;
	const vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryHandleType;
	const vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	semaphoreHandleType;
	const bool											dedicated;
};

// A helper class to test for extensions upfront and throw not supported to speed up test runtimes compared to failing only
// after creating unnecessary vkInstances.  A common example of this is win32 platforms taking a long time to run _fd tests.
class NotSupportedChecker
{
public:
				NotSupportedChecker	(const Context&			 context,
									 TestConfig				 config,
									 const OperationSupport& writeOp,
									 const OperationSupport& readOp)
	: m_context	(context)
	{
		// Check instance support
		requireInstanceExtension("VK_KHR_get_physical_device_properties2");

		requireInstanceExtension("VK_KHR_external_semaphore_capabilities");
		requireInstanceExtension("VK_KHR_external_memory_capabilities");

		// Check device support
		if (config.dedicated)
			requireDeviceExtension("VK_KHR_dedicated_allocation");

		requireDeviceExtension("VK_KHR_external_semaphore");
		requireDeviceExtension("VK_KHR_external_memory");

		if (config.memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
			|| config.semaphoreHandleType == vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT_KHR
			|| config.semaphoreHandleType == vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR)
		{
			requireDeviceExtension("VK_KHR_external_semaphore_fd");
			requireDeviceExtension("VK_KHR_external_memory_fd");
		}

		if (config.memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
			|| config.memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR
			|| config.semaphoreHandleType == vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
			|| config.semaphoreHandleType == vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR)
		{
			requireDeviceExtension("VK_KHR_external_semaphore_win32");
			requireDeviceExtension("VK_KHR_external_memory_win32");
		}

		TestLog&						log				= context.getTestContext().getLog();
		const vk::InstanceInterface&	vki				= context.getInstanceInterface();
		const vk::VkPhysicalDevice		physicalDevice	= context.getPhysicalDevice();

		// Check resource support
		if (config.resource.type == RESOURCE_TYPE_IMAGE)
		{
			const vk::VkPhysicalDeviceExternalImageFormatInfoKHR	externalInfo		=
			{
				vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO_KHR,
				DE_NULL,
				config.memoryHandleType
			};
			const vk::VkPhysicalDeviceImageFormatInfo2KHR	imageFormatInfo		=
			{
				vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2_KHR,
				&externalInfo,
				config.resource.imageFormat,
				config.resource.imageType,
				vk::VK_IMAGE_TILING_OPTIMAL,
				readOp.getResourceUsageFlags() | writeOp.getResourceUsageFlags(),
				0u
			};
			vk::VkExternalImageFormatPropertiesKHR			externalProperties	=
			{
				vk::VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES_KHR,
				DE_NULL,
				{ 0u, 0u, 0u }
			};
			vk::VkImageFormatProperties2KHR					formatProperties	=
			{
				vk::VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2_KHR,
				&externalProperties,
				{
					{ 0u, 0u, 0u },
					0u,
					0u,
					0u,
					0u,
				}
			};

			{
				const vk::VkResult res = vki.getPhysicalDeviceImageFormatProperties2KHR(physicalDevice, &imageFormatInfo, &formatProperties);

				if (res == vk::VK_ERROR_FORMAT_NOT_SUPPORTED)
					TCU_THROW(NotSupportedError, "Image format not supported");

				VK_CHECK(res); // Check other errors
			}

			log << TestLog::Message << "External image format properties: " << imageFormatInfo << "\n"<< externalProperties << TestLog::EndMessage;

			if ((externalProperties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR) == 0)
				TCU_THROW(NotSupportedError, "Exporting image resource not supported");

			if ((externalProperties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHR) == 0)
				TCU_THROW(NotSupportedError, "Importing image resource not supported");

			if (!config.dedicated && (externalProperties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_KHR) != 0)
			{
				TCU_THROW(NotSupportedError, "Handle requires dedicated allocation, but test uses suballocated memory");
			}
		}
		else
		{
			const vk::VkPhysicalDeviceExternalBufferInfoKHR	info	=
			{
				vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO_KHR,
				DE_NULL,

				0u,
				readOp.getResourceUsageFlags() | writeOp.getResourceUsageFlags(),
				config.memoryHandleType
			};
			vk::VkExternalBufferPropertiesKHR				properties			=
			{
				vk::VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES_KHR,
				DE_NULL,
				{ 0u, 0u, 0u}
			};
			vki.getPhysicalDeviceExternalBufferPropertiesKHR(physicalDevice, &info, &properties);

			log << TestLog::Message << "External buffer properties: " << info << "\n" << properties << TestLog::EndMessage;

			if ((properties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR) == 0
				|| (properties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHR) == 0)
				TCU_THROW(NotSupportedError, "Exporting and importing memory type not supported");

			if (!config.dedicated && (properties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_KHR) != 0)
			{
				TCU_THROW(NotSupportedError, "Handle requires dedicated allocation, but test uses suballocated memory");
			}
		}

		// Check semaphore support
		{
			const vk::VkPhysicalDeviceExternalSemaphoreInfoKHR	info		=
			{
				vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO_KHR,
				DE_NULL,
				config.semaphoreHandleType
			};
			vk::VkExternalSemaphorePropertiesKHR				properties;

			vki.getPhysicalDeviceExternalSemaphorePropertiesKHR(physicalDevice, &info, &properties);

			log << TestLog::Message << info << "\n" << properties << TestLog::EndMessage;

			if ((properties.externalSemaphoreFeatures & vk::VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT_KHR) == 0
				|| (properties.externalSemaphoreFeatures & vk::VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT_KHR) == 0)
				TCU_THROW(NotSupportedError, "Exporting and importing semaphore type not supported");
		}
	}

private:
	void requireDeviceExtension(const char* name) const
	{
		if (!de::contains(m_context.getDeviceExtensions().begin(), m_context.getDeviceExtensions().end(), name))
			TCU_THROW(NotSupportedError, (std::string(name) + " is not supported").c_str());
	}

	void requireInstanceExtension(const char* name) const
	{
		if (!de::contains(m_context.getInstanceExtensions().begin(), m_context.getInstanceExtensions().end(), name))
			TCU_THROW(NotSupportedError, (std::string(name) + " is not supported").c_str());
	}

	const Context& m_context;
};

bool checkQueueFlags (vk::VkQueueFlags availableFlags, const vk::VkQueueFlags neededFlags)
{
	if ((availableFlags & (vk::VK_QUEUE_GRAPHICS_BIT | vk::VK_QUEUE_COMPUTE_BIT)) != 0)
		availableFlags |= vk::VK_QUEUE_TRANSFER_BIT;

	return (availableFlags & neededFlags) != 0;
}

class SimpleAllocation : public vk::Allocation
{
public:
								SimpleAllocation	(const vk::DeviceInterface&	vkd,
													 vk::VkDevice				device,
													 const vk::VkDeviceMemory	memory);
								~SimpleAllocation	(void);

private:
	const vk::DeviceInterface&	m_vkd;
	const vk::VkDevice			m_device;
};

SimpleAllocation::SimpleAllocation (const vk::DeviceInterface&	vkd,
									vk::VkDevice				device,
									const vk::VkDeviceMemory	memory)
	: Allocation	(memory, 0, DE_NULL)
	, m_vkd			(vkd)
	, m_device		(device)
{
}

SimpleAllocation::~SimpleAllocation (void)
{
	m_vkd.freeMemory(m_device, getMemory(), DE_NULL);
}

class DeviceId
{
public:
					DeviceId		(deUint32		vendorId,
									 deUint32		driverVersion,
									 const deUint8	driverUUID[VK_UUID_SIZE],
									 const deUint8	deviceUUID[VK_UUID_SIZE]);

	bool			operator==		(const DeviceId& other) const;
	bool			operator|=		(const DeviceId& other) const;

private:
	const deUint32	m_vendorId;
	const deUint32	m_driverVersion;
	deUint8			m_driverUUID[VK_UUID_SIZE];
	deUint8			m_deviceUUID[VK_UUID_SIZE];
};

DeviceId::DeviceId (deUint32		vendorId,
					deUint32		driverVersion,
					const deUint8	driverUUID[VK_UUID_SIZE],
					const deUint8	deviceUUID[VK_UUID_SIZE])
	: m_vendorId		(vendorId)
	, m_driverVersion	(driverVersion)
{
	deMemcpy(m_driverUUID, driverUUID, sizeof(m_driverUUID));
	deMemcpy(m_deviceUUID, deviceUUID, sizeof(m_deviceUUID));
}

bool DeviceId::operator== (const DeviceId& other) const
{
	if (this == &other)
		return true;

	if (m_vendorId != other.m_vendorId)
		return false;

	if (m_driverVersion != other.m_driverVersion)
		return false;

	if (deMemCmp(m_driverUUID, other.m_driverUUID, sizeof(m_driverUUID)) != 0)
		return false;

	return deMemCmp(m_deviceUUID, other.m_deviceUUID, sizeof(m_deviceUUID)) == 0;
}

DeviceId getDeviceId (const vk::InstanceInterface&	vki,
					  vk::VkPhysicalDevice			physicalDevice)
{
	vk::VkPhysicalDeviceIDPropertiesKHR			propertiesId;
	vk::VkPhysicalDeviceProperties2KHR			properties;

	deMemset(&properties, 0, sizeof(properties));
	deMemset(&propertiesId, 0, sizeof(propertiesId));

	propertiesId.sType	= vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR;

	properties.sType	= vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
	properties.pNext	= &propertiesId;

	vki.getPhysicalDeviceProperties2KHR(physicalDevice, &properties);

	return DeviceId(properties.properties.vendorID, properties.properties.driverVersion, propertiesId.driverUUID, propertiesId.deviceUUID);
}

vk::Move<vk::VkInstance> createInstance (const vk::PlatformInterface& vkp)
{
	try
	{
		std::vector<std::string> extensions;

		extensions.push_back("VK_KHR_get_physical_device_properties2");

		extensions.push_back("VK_KHR_external_semaphore_capabilities");
		extensions.push_back("VK_KHR_external_memory_capabilities");

		return vk::createDefaultInstance(vkp, std::vector<std::string>(), extensions);
	}
	catch (const vk::Error& error)
	{
		if (error.getError() == vk::VK_ERROR_EXTENSION_NOT_PRESENT)
			TCU_THROW(NotSupportedError, "Required external memory extensions not supported by the instance");
		else
			throw;
	}
}

vk::VkPhysicalDevice getPhysicalDevice (const vk::InstanceInterface&	vki,
										vk::VkInstance					instance,
										const tcu::CommandLine&			cmdLine)
{
	return vk::chooseDevice(vki, instance, cmdLine);
}

vk::VkPhysicalDevice getPhysicalDevice (const vk::InstanceInterface& vki, vk::VkInstance instance, const DeviceId& deviceId)
{
	const std::vector<vk::VkPhysicalDevice> devices (vk::enumeratePhysicalDevices(vki, instance));

	for (size_t deviceNdx = 0; deviceNdx < devices.size(); deviceNdx++)
	{
		if (deviceId == getDeviceId(vki, devices[deviceNdx]))
			return devices[deviceNdx];
	}

	TCU_FAIL("No matching device found");

	return (vk::VkPhysicalDevice)0;
}

vk::Move<vk::VkDevice> createDevice (const vk::InstanceInterface&					vki,
									 vk::VkPhysicalDevice							physicalDevice,
									 vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryHandleType,
									 vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	semaphoreHandleType,
									 bool											dedicated,
									 bool										    khrMemReqSupported)
{
	const float										priority				= 0.0f;
	const std::vector<vk::VkQueueFamilyProperties>	queueFamilyProperties	= vk::getPhysicalDeviceQueueFamilyProperties(vki, physicalDevice);
	std::vector<deUint32>							queueFamilyIndices		(queueFamilyProperties.size(), 0xFFFFFFFFu);
	std::vector<const char*>						extensions;

	if (dedicated)
		extensions.push_back("VK_KHR_dedicated_allocation");

	if (khrMemReqSupported)
		extensions.push_back("VK_KHR_get_memory_requirements2");

	extensions.push_back("VK_KHR_external_semaphore");
	extensions.push_back("VK_KHR_external_memory");

	if (memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
		|| semaphoreHandleType == vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT_KHR
		|| semaphoreHandleType == vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR)
	{
		extensions.push_back("VK_KHR_external_semaphore_fd");
		extensions.push_back("VK_KHR_external_memory_fd");
	}

	if (memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
		|| memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR
		|| semaphoreHandleType == vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
		|| semaphoreHandleType == vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR)
	{
		extensions.push_back("VK_KHR_external_semaphore_win32");
		extensions.push_back("VK_KHR_external_memory_win32");
	}

	try
	{
		std::vector<vk::VkDeviceQueueCreateInfo>	queues;

		for (size_t ndx = 0; ndx < queueFamilyProperties.size(); ndx++)
		{
			const vk::VkDeviceQueueCreateInfo	createInfo	=
			{
				vk::VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				DE_NULL,
				0u,

				(deUint32)ndx,
				1u,
				&priority
			};

			queues.push_back(createInfo);
		}

		const vk::VkDeviceCreateInfo		createInfo			=
		{
			vk::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			DE_NULL,
			0u,

			(deUint32)queues.size(),
			&queues[0],

			0u,
			DE_NULL,

			(deUint32)extensions.size(),
			extensions.empty() ? DE_NULL : &extensions[0],
			0u
		};

		return vk::createDevice(vki, physicalDevice, &createInfo);
	}
	catch (const vk::Error& error)
	{
		if (error.getError() == vk::VK_ERROR_EXTENSION_NOT_PRESENT)
			TCU_THROW(NotSupportedError, "Required extensions not supported");
		else
			throw;
	}
}

vk::VkQueue getQueue (const vk::DeviceInterface&	vkd,
					  const vk::VkDevice			device,
					  deUint32						familyIndex)
{
	vk::VkQueue queue;

	vkd.getDeviceQueue(device, familyIndex, 0u, &queue);

	return queue;
}

vk::Move<vk::VkCommandPool> createCommandPool (const vk::DeviceInterface&	vkd,
											   vk::VkDevice					device,
											   deUint32						queueFamilyIndex)
{
	const vk::VkCommandPoolCreateInfo	createInfo			=
	{
		vk::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		DE_NULL,

		0u,
		queueFamilyIndex
	};

	return vk::createCommandPool(vkd, device, &createInfo);
}

vk::Move<vk::VkCommandBuffer> createCommandBuffer (const vk::DeviceInterface&	vkd,
												   vk::VkDevice					device,
												   vk::VkCommandPool			commandPool)
{
	const vk::VkCommandBufferLevel			level			= vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	const vk::VkCommandBufferAllocateInfo	allocateInfo	=
	{
		vk::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		DE_NULL,

		commandPool,
		level,
		1u
	};

	return vk::allocateCommandBuffer(vkd, device, &allocateInfo);
}

de::MovePtr<vk::Allocation> allocateAndBindMemory (const vk::DeviceInterface&					vkd,
												   vk::VkDevice									device,
												   vk::VkBuffer									buffer,
												   vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
												   deUint32&									exportedMemoryTypeIndex,
												   bool											dedicated,
												   bool											getMemReq2Supported)
{
	vk::VkMemoryRequirements memoryRequirements = { 0u, 0u, 0u, };

	if (getMemReq2Supported)
	{
		const vk::VkBufferMemoryRequirementsInfo2KHR	requirementInfo =
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR,
			DE_NULL,
			buffer
		};
		vk::VkMemoryDedicatedRequirementsKHR			dedicatedRequirements =
		{
			vk::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR,
			DE_NULL,
			VK_FALSE,
			VK_FALSE
		};
		vk::VkMemoryRequirements2KHR					requirements =
		{
			vk::VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR,
			&dedicatedRequirements,
			{ 0u, 0u, 0u, }
		};
		vkd.getBufferMemoryRequirements2KHR(device, &requirementInfo, &requirements);

		if (!dedicated && dedicatedRequirements.requiresDedicatedAllocation)
			TCU_THROW(NotSupportedError, "Memory requires dedicated allocation");

		memoryRequirements = requirements.memoryRequirements;
	}
	else
	{
		vkd.getBufferMemoryRequirements(device, buffer, &memoryRequirements);
	}


	vk::Move<vk::VkDeviceMemory> memory = allocateExportableMemory(vkd, device, memoryRequirements, externalType, dedicated ? buffer : (vk::VkBuffer)0, exportedMemoryTypeIndex);
	VK_CHECK(vkd.bindBufferMemory(device, buffer, *memory, 0u));

	return de::MovePtr<vk::Allocation>(new SimpleAllocation(vkd, device, memory.disown()));
}

de::MovePtr<vk::Allocation> allocateAndBindMemory (const vk::DeviceInterface&					vkd,
												   vk::VkDevice									device,
												   vk::VkImage									image,
												   vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
												   deUint32&									exportedMemoryTypeIndex,
												   bool											dedicated,
												   bool											getMemReq2Supported)
{
	vk::VkMemoryRequirements memoryRequirements = { 0u, 0u, 0u, };

	if (getMemReq2Supported)
	{
		const vk::VkImageMemoryRequirementsInfo2KHR	requirementInfo =
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR,
			DE_NULL,
			image
		};
		vk::VkMemoryDedicatedRequirementsKHR			dedicatedRequirements =
		{
			vk::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR,
			DE_NULL,
			VK_FALSE,
			VK_FALSE
		};
		vk::VkMemoryRequirements2KHR					requirements =
		{
			vk::VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR,
			&dedicatedRequirements,
			{ 0u, 0u, 0u, }
		};
		vkd.getImageMemoryRequirements2KHR(device, &requirementInfo, &requirements);

		if (!dedicated && dedicatedRequirements.requiresDedicatedAllocation)
			TCU_THROW(NotSupportedError, "Memory requires dedicated allocation");

		memoryRequirements = requirements.memoryRequirements;
	}
	else
	{
		vkd.getImageMemoryRequirements(device, image, &memoryRequirements);
	}

	vk::Move<vk::VkDeviceMemory> memory = allocateExportableMemory(vkd, device, memoryRequirements, externalType, dedicated ? image : (vk::VkImage)0, exportedMemoryTypeIndex);
	VK_CHECK(vkd.bindImageMemory(device, image, *memory, 0u));

	return de::MovePtr<vk::Allocation>(new SimpleAllocation(vkd, device, memory.disown()));
}

de::MovePtr<Resource> createResource (const vk::DeviceInterface&				vkd,
									  vk::VkDevice								device,
									  const ResourceDescription&				resourceDesc,
									  const std::vector<deUint32>&				queueFamilyIndices,
									  const OperationSupport&					readOp,
									  const OperationSupport&					writeOp,
									  vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
									  deUint32&									exportedMemoryTypeIndex,
									  bool										dedicated,
									  bool										getMemReq2Supported)
{
	if (resourceDesc.type == RESOURCE_TYPE_IMAGE)
	{
		const vk::VkExtent3D				extent					=
		{
			(deUint32)resourceDesc.size.x(),
			de::max(1u, (deUint32)resourceDesc.size.y()),
			de::max(1u, (deUint32)resourceDesc.size.z())
		};
		const vk::VkImageSubresourceRange	subresourceRange		=
		{
			resourceDesc.imageAspect,
			0u,
			1u,
			0u,
			1u
		};
		const vk::VkImageSubresourceLayers	subresourceLayers		=
		{
			resourceDesc.imageAspect,
			0u,
			0u,
			1u
		};
		const vk::VkExternalMemoryImageCreateInfoKHR externalInfo =
		{
			vk::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
			DE_NULL,
			(vk::VkExternalMemoryHandleTypeFlagsKHR)externalType
		};
		const vk::VkImageCreateInfo			createInfo				=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			&externalInfo,
			0u,

			resourceDesc.imageType,
			resourceDesc.imageFormat,
			extent,
			1u,
			1u,
			vk::VK_SAMPLE_COUNT_1_BIT,
			vk::VK_IMAGE_TILING_OPTIMAL,
			readOp.getResourceUsageFlags() | writeOp.getResourceUsageFlags(),
			vk::VK_SHARING_MODE_EXCLUSIVE,

			(deUint32)queueFamilyIndices.size(),
			&queueFamilyIndices[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		vk::Move<vk::VkImage>			image		= vk::createImage(vkd, device, &createInfo);
		de::MovePtr<vk::Allocation>		allocation	= allocateAndBindMemory(vkd, device, *image, externalType, exportedMemoryTypeIndex, dedicated, getMemReq2Supported);

		return de::MovePtr<Resource>(new Resource(image, allocation, extent, resourceDesc.imageType, resourceDesc.imageFormat, subresourceRange, subresourceLayers));
	}
	else
	{
		const vk::VkDeviceSize							offset			= 0u;
		const vk::VkDeviceSize							size			= static_cast<vk::VkDeviceSize>(resourceDesc.size.x());
		const vk::VkBufferUsageFlags					usage			= readOp.getResourceUsageFlags() | writeOp.getResourceUsageFlags();
		const vk:: VkExternalMemoryBufferCreateInfoKHR	externalInfo	=
		{
			vk::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR,
			DE_NULL,
			(vk::VkExternalMemoryHandleTypeFlagsKHR)externalType
		};
		const vk::VkBufferCreateInfo					createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			&externalInfo,
			0u,

			size,
			usage,
			vk::VK_SHARING_MODE_EXCLUSIVE,
			(deUint32)queueFamilyIndices.size(),
			&queueFamilyIndices[0]
		};
		vk::Move<vk::VkBuffer>		buffer		= vk::createBuffer(vkd, device, &createInfo);
		de::MovePtr<vk::Allocation>	allocation	= allocateAndBindMemory(vkd, device, *buffer, externalType, exportedMemoryTypeIndex, dedicated, getMemReq2Supported);

		return de::MovePtr<Resource>(new Resource(resourceDesc.type, buffer, allocation, offset, size));
	}
}

de::MovePtr<vk::Allocation> importAndBindMemory (const vk::DeviceInterface&					vkd,
												 vk::VkDevice								device,
												 vk::VkBuffer								buffer,
												 NativeHandle&								nativeHandle,
												 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
												 deUint32									exportedMemoryTypeIndex,
												 bool										dedicated)
{
	const vk::VkMemoryRequirements	requirements	= vk::getBufferMemoryRequirements(vkd, device, buffer);
	vk::Move<vk::VkDeviceMemory>	memory			= dedicated
													? importDedicatedMemory(vkd, device, buffer, requirements, externalType, exportedMemoryTypeIndex, nativeHandle)
													: importMemory(vkd, device, requirements, externalType, exportedMemoryTypeIndex, nativeHandle);

	VK_CHECK(vkd.bindBufferMemory(device, buffer, *memory, 0u));

	return de::MovePtr<vk::Allocation>(new SimpleAllocation(vkd, device, memory.disown()));
}

de::MovePtr<vk::Allocation> importAndBindMemory (const vk::DeviceInterface&					vkd,
												 vk::VkDevice								device,
												 vk::VkImage								image,
												 NativeHandle&								nativeHandle,
												 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
												 deUint32									exportedMemoryTypeIndex,
												 bool										dedicated)
{
	const vk::VkMemoryRequirements	requirements	= vk::getImageMemoryRequirements(vkd, device, image);
	vk::Move<vk::VkDeviceMemory>	memory			= dedicated
													? importDedicatedMemory(vkd, device, image, requirements, externalType, exportedMemoryTypeIndex, nativeHandle)
													: importMemory(vkd, device, requirements, externalType, exportedMemoryTypeIndex, nativeHandle);
	VK_CHECK(vkd.bindImageMemory(device, image, *memory, 0u));

	return de::MovePtr<vk::Allocation>(new SimpleAllocation(vkd, device, memory.disown()));
}

de::MovePtr<Resource> importResource (const vk::DeviceInterface&				vkd,
									  vk::VkDevice								device,
									  const ResourceDescription&				resourceDesc,
									  const std::vector<deUint32>&				queueFamilyIndices,
									  const OperationSupport&					readOp,
									  const OperationSupport&					writeOp,
									  NativeHandle&								nativeHandle,
									  vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
									  deUint32									exportedMemoryTypeIndex,
									  bool										dedicated)
{
	if (resourceDesc.type == RESOURCE_TYPE_IMAGE)
	{
		const vk::VkExtent3D				extent					=
		{
			(deUint32)resourceDesc.size.x(),
			de::max(1u, (deUint32)resourceDesc.size.y()),
			de::max(1u, (deUint32)resourceDesc.size.z())
		};
		const vk::VkImageSubresourceRange	subresourceRange		=
		{
			resourceDesc.imageAspect,
			0u,
			1u,
			0u,
			1u
		};
		const vk::VkImageSubresourceLayers	subresourceLayers		=
		{
			resourceDesc.imageAspect,
			0u,
			0u,
			1u
		};
		const vk:: VkExternalMemoryImageCreateInfoKHR externalInfo =
		{
			vk::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
			DE_NULL,
			(vk::VkExternalMemoryHandleTypeFlagsKHR)externalType
		};
		const vk::VkImageCreateInfo			createInfo				=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			&externalInfo,
			0u,

			resourceDesc.imageType,
			resourceDesc.imageFormat,
			extent,
			1u,
			1u,
			vk::VK_SAMPLE_COUNT_1_BIT,
			vk::VK_IMAGE_TILING_OPTIMAL,
			readOp.getResourceUsageFlags() | writeOp.getResourceUsageFlags(),
			vk::VK_SHARING_MODE_EXCLUSIVE,

			(deUint32)queueFamilyIndices.size(),
			&queueFamilyIndices[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		vk::Move<vk::VkImage>			image		= vk::createImage(vkd, device, &createInfo);
		de::MovePtr<vk::Allocation>		allocation	= importAndBindMemory(vkd, device, *image, nativeHandle, externalType, exportedMemoryTypeIndex, dedicated);

		return de::MovePtr<Resource>(new Resource(image, allocation, extent, resourceDesc.imageType, resourceDesc.imageFormat, subresourceRange, subresourceLayers));
	}
	else
	{
		const vk::VkDeviceSize							offset			= 0u;
		const vk::VkDeviceSize							size			= static_cast<vk::VkDeviceSize>(resourceDesc.size.x());
		const vk::VkBufferUsageFlags					usage			= readOp.getResourceUsageFlags() | writeOp.getResourceUsageFlags();
		const vk:: VkExternalMemoryBufferCreateInfoKHR	externalInfo	=
		{
			vk::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR,
			DE_NULL,
			(vk::VkExternalMemoryHandleTypeFlagsKHR)externalType
		};
		const vk::VkBufferCreateInfo					createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			&externalInfo,
			0u,

			size,
			usage,
			vk::VK_SHARING_MODE_EXCLUSIVE,
			(deUint32)queueFamilyIndices.size(),
			&queueFamilyIndices[0]
		};
		vk::Move<vk::VkBuffer>		buffer		= vk::createBuffer(vkd, device, &createInfo);
		de::MovePtr<vk::Allocation>	allocation	= importAndBindMemory(vkd, device, *buffer, nativeHandle, externalType, exportedMemoryTypeIndex, dedicated);

		return de::MovePtr<Resource>(new Resource(resourceDesc.type, buffer, allocation, offset, size));
	}
}

void recordWriteBarrier (const vk::DeviceInterface&	vkd,
						 vk::VkCommandBuffer		commandBuffer,
						 const Resource&			resource,
						 const SyncInfo&			writeSync,
						 deUint32					writeQueueFamilyIndex,
						 const SyncInfo&			readSync)
{
	const vk::VkPipelineStageFlags	srcStageMask		= writeSync.stageMask;
	const vk::VkAccessFlags			srcAccessMask		= writeSync.accessMask;

	const vk::VkPipelineStageFlags	dstStageMask		= readSync.stageMask;
	const vk::VkAccessFlags			dstAccessMask		= readSync.accessMask;

	const vk::VkDependencyFlags		dependencyFlags		= 0;

	if (resource.getType() == RESOURCE_TYPE_IMAGE)
	{
		const vk::VkImageMemoryBarrier	barrier				=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			srcAccessMask,
			dstAccessMask,

			writeSync.imageLayout,
			readSync.imageLayout,

			writeQueueFamilyIndex,
			VK_QUEUE_FAMILY_EXTERNAL_KHR,

			resource.getImage().handle,
			resource.getImage().subresourceRange
		};

		vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, 0u, (const vk::VkMemoryBarrier*)DE_NULL, 0u, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1u, (const vk::VkImageMemoryBarrier*)&barrier);
	}
	else
	{
		const vk::VkBufferMemoryBarrier	barrier				=
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,

			srcAccessMask,
			dstAccessMask,

			writeQueueFamilyIndex,
			VK_QUEUE_FAMILY_EXTERNAL_KHR,

			resource.getBuffer().handle,
			0u,
			VK_WHOLE_SIZE
		};

		vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, 0u, (const vk::VkMemoryBarrier*)DE_NULL, 1u, (const vk::VkBufferMemoryBarrier*)&barrier, 0u, (const vk::VkImageMemoryBarrier*)DE_NULL);
	}
}

void recordReadBarrier (const vk::DeviceInterface&	vkd,
						vk::VkCommandBuffer			commandBuffer,
						const Resource&				resource,
						const SyncInfo&				writeSync,
						const SyncInfo&				readSync,
						deUint32					readQueueFamilyIndex)
{
	const vk::VkPipelineStageFlags	srcStageMask		= readSync.stageMask;
	const vk::VkAccessFlags			srcAccessMask		= readSync.accessMask;

	const vk::VkPipelineStageFlags	dstStageMask		= readSync.stageMask;
	const vk::VkAccessFlags			dstAccessMask		= readSync.accessMask;

	const vk::VkDependencyFlags		dependencyFlags		= 0;

	if (resource.getType() == RESOURCE_TYPE_IMAGE)
	{
		const vk::VkImageMemoryBarrier	barrier				=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			srcAccessMask,
			dstAccessMask,

			writeSync.imageLayout,
			readSync.imageLayout,

			VK_QUEUE_FAMILY_EXTERNAL_KHR,
			readQueueFamilyIndex,

			resource.getImage().handle,
			resource.getImage().subresourceRange
		};

		vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, 0u, (const vk::VkMemoryBarrier*)DE_NULL, 0u, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1u, (const vk::VkImageMemoryBarrier*)&barrier);
	}
	else
	{
		const vk::VkBufferMemoryBarrier	barrier				=
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,

			srcAccessMask,
			dstAccessMask,

			VK_QUEUE_FAMILY_EXTERNAL_KHR,
			readQueueFamilyIndex,

			resource.getBuffer().handle,
			0u,
			VK_WHOLE_SIZE
		};

		vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, 0u, (const vk::VkMemoryBarrier*)DE_NULL, 1u, (const vk::VkBufferMemoryBarrier*)&barrier, 0u, (const vk::VkImageMemoryBarrier*)DE_NULL);
	}
}

std::vector<deUint32> getFamilyIndices (const std::vector<vk::VkQueueFamilyProperties>& properties)
{
	std::vector<deUint32> indices (properties.size(), 0);

	for (deUint32 ndx = 0; ndx < properties.size(); ndx++)
		indices[ndx] = ndx;

	return indices;
}

class SharingTestInstance : public TestInstance
{
public:
														SharingTestInstance		(Context&	context,
																				 TestConfig	config);

	virtual tcu::TestStatus								iterate					(void);

private:
	const TestConfig									m_config;
	const de::UniquePtr<OperationSupport>				m_supportWriteOp;
	const de::UniquePtr<OperationSupport>				m_supportReadOp;
	const NotSupportedChecker							m_notSupportedChecker; // Must declare before VkInstance to effectively reduce runtimes!

	const vk::Unique<vk::VkInstance>					m_instanceA;

	const vk::InstanceDriver							m_vkiA;
	const vk::VkPhysicalDevice							m_physicalDeviceA;
	const std::vector<vk::VkQueueFamilyProperties>		m_queueFamiliesA;
	const std::vector<deUint32>							m_queueFamilyIndicesA;

	const bool											m_getMemReq2Supported;

	const vk::Unique<vk::VkDevice>						m_deviceA;
	const vk::DeviceDriver								m_vkdA;

	const vk::Unique<vk::VkInstance>					m_instanceB;
	const vk::InstanceDriver							m_vkiB;
	const vk::VkPhysicalDevice							m_physicalDeviceB;
	const std::vector<vk::VkQueueFamilyProperties>		m_queueFamiliesB;
	const std::vector<deUint32>							m_queueFamilyIndicesB;
	const vk::Unique<vk::VkDevice>						m_deviceB;
	const vk::DeviceDriver								m_vkdB;

	const vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	m_semaphoreHandleType;
	const vk::VkExternalMemoryHandleTypeFlagBitsKHR		m_memoryHandleType;

	// \todo Should this be moved to the group same way as in the other tests?
	PipelineCacheData									m_pipelineCacheData;
	tcu::ResultCollector								m_resultCollector;
	size_t												m_queueANdx;
	size_t												m_queueBNdx;
};

SharingTestInstance::SharingTestInstance (Context&		context,
										  TestConfig	config)
	: TestInstance				(context)
	, m_config					(config)
	, m_supportWriteOp			(makeOperationSupport(config.writeOp, config.resource))
	, m_supportReadOp			(makeOperationSupport(config.readOp, config.resource))
	, m_notSupportedChecker		(context, m_config, *m_supportWriteOp, *m_supportReadOp)

	, m_instanceA				(createInstance(context.getPlatformInterface()))

	, m_vkiA					(context.getPlatformInterface(), *m_instanceA)
	, m_physicalDeviceA			(getPhysicalDevice(m_vkiA, *m_instanceA, context.getTestContext().getCommandLine()))
	, m_queueFamiliesA			(vk::getPhysicalDeviceQueueFamilyProperties(m_vkiA, m_physicalDeviceA))
	, m_queueFamilyIndicesA		(getFamilyIndices(m_queueFamiliesA))
	, m_getMemReq2Supported		(de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_get_memory_requirements2"))
	, m_deviceA					(createDevice(m_vkiA, m_physicalDeviceA, m_config.memoryHandleType, m_config.semaphoreHandleType, m_config.dedicated, m_getMemReq2Supported))
	, m_vkdA					(m_vkiA, *m_deviceA)

	, m_instanceB				(createInstance(context.getPlatformInterface()))

	, m_vkiB					(context.getPlatformInterface(), *m_instanceB)
	, m_physicalDeviceB			(getPhysicalDevice(m_vkiB, *m_instanceB, getDeviceId(m_vkiA, m_physicalDeviceA)))
	, m_queueFamiliesB			(vk::getPhysicalDeviceQueueFamilyProperties(m_vkiB, m_physicalDeviceB))
	, m_queueFamilyIndicesB		(getFamilyIndices(m_queueFamiliesB))
	, m_deviceB					(createDevice(m_vkiB, m_physicalDeviceB, m_config.memoryHandleType, m_config.semaphoreHandleType, m_config.dedicated, m_getMemReq2Supported))
	, m_vkdB					(m_vkiB, *m_deviceB)

	, m_semaphoreHandleType		(m_config.semaphoreHandleType)
	, m_memoryHandleType		(m_config.memoryHandleType)

	, m_resultCollector			(context.getTestContext().getLog())
	, m_queueANdx				(0)
	, m_queueBNdx				(0)
{
}

tcu::TestStatus SharingTestInstance::iterate (void)
{
	TestLog&								log					(m_context.getTestContext().getLog());

	const deUint32							queueFamilyA		= (deUint32)m_queueANdx;
	const deUint32							queueFamilyB		= (deUint32)m_queueBNdx;

	const tcu::ScopedLogSection				queuePairSection	(log,
																	"WriteQueue-" + de::toString(queueFamilyA) + "-ReadQueue-" + de::toString(queueFamilyB),
																	"WriteQueue-" + de::toString(queueFamilyA) + "-ReadQueue-" + de::toString(queueFamilyB));

	const vk::Unique<vk::VkSemaphore>		semaphoreA			(createExportableSemaphore(m_vkdA, *m_deviceA, m_semaphoreHandleType));
	const vk::Unique<vk::VkSemaphore>		semaphoreB			(createSemaphore(m_vkdB, *m_deviceB));

	deUint32								exportedMemoryTypeIndex = ~0U;
	const de::UniquePtr<Resource>			resourceA			(createResource(m_vkdA, *m_deviceA, m_config.resource, m_queueFamilyIndicesA, *m_supportReadOp, *m_supportWriteOp, m_memoryHandleType, exportedMemoryTypeIndex, m_config.dedicated, m_getMemReq2Supported));

	NativeHandle							nativeMemoryHandle;
	getMemoryNative(m_vkdA, *m_deviceA, resourceA->getMemory(), m_memoryHandleType, nativeMemoryHandle);

	const de::UniquePtr<Resource>			resourceB			(importResource(m_vkdB, *m_deviceB, m_config.resource, m_queueFamilyIndicesB, *m_supportReadOp, *m_supportWriteOp, nativeMemoryHandle, m_memoryHandleType, exportedMemoryTypeIndex, m_config.dedicated));

	try
	{
		const vk::VkQueue						queueA				(getQueue(m_vkdA, *m_deviceA, queueFamilyA));
		const vk::Unique<vk::VkCommandPool>		commandPoolA		(createCommandPool(m_vkdA, *m_deviceA, queueFamilyA));
		const vk::Unique<vk::VkCommandBuffer>	commandBufferA		(createCommandBuffer(m_vkdA, *m_deviceA, *commandPoolA));
		vk::SimpleAllocator						allocatorA			(m_vkdA, *m_deviceA, vk::getPhysicalDeviceMemoryProperties(m_vkiA, m_physicalDeviceA));
		const std::vector<std::string>			deviceExtensionsA;
		OperationContext						operationContextA	(m_vkiA, m_vkdA, m_physicalDeviceA, *m_deviceA, allocatorA, deviceExtensionsA, m_context.getBinaryCollection(), m_pipelineCacheData);

		if (!checkQueueFlags(m_queueFamiliesA[m_queueANdx].queueFlags , m_supportWriteOp->getQueueFlags(operationContextA)))
			TCU_THROW(NotSupportedError, "Operation not supported by the source queue");

		const vk::VkQueue						queueB				(getQueue(m_vkdB, *m_deviceB, queueFamilyB));
		const vk::Unique<vk::VkCommandPool>		commandPoolB		(createCommandPool(m_vkdB, *m_deviceB, queueFamilyB));
		const vk::Unique<vk::VkCommandBuffer>	commandBufferB		(createCommandBuffer(m_vkdB, *m_deviceB, *commandPoolB));
		vk::SimpleAllocator						allocatorB			(m_vkdB, *m_deviceB, vk::getPhysicalDeviceMemoryProperties(m_vkiB, m_physicalDeviceB));
		const std::vector<std::string>			deviceExtensionsB;
		OperationContext						operationContextB	(m_vkiB, m_vkdB, m_physicalDeviceB, *m_deviceB, allocatorB, deviceExtensionsB, m_context.getBinaryCollection(), m_pipelineCacheData);

		if (!checkQueueFlags(m_queueFamiliesB[m_queueBNdx].queueFlags , m_supportReadOp->getQueueFlags(operationContextB)))
			TCU_THROW(NotSupportedError, "Operation not supported by the destination queue");

		const de::UniquePtr<Operation>			writeOp				(m_supportWriteOp->build(operationContextA, *resourceA));
		const de::UniquePtr<Operation>			readOp				(m_supportReadOp->build(operationContextB, *resourceB));

		const SyncInfo							writeSync			= writeOp->getSyncInfo();
		const SyncInfo							readSync			= readOp->getSyncInfo();

		beginCommandBuffer(m_vkdA, *commandBufferA);
		writeOp->recordCommands(*commandBufferA);
		recordWriteBarrier(m_vkdA, *commandBufferA, *resourceA, writeSync, queueFamilyA, readSync);
		endCommandBuffer(m_vkdA, *commandBufferA);

		beginCommandBuffer(m_vkdB, *commandBufferB);
		recordReadBarrier(m_vkdB, *commandBufferB, *resourceB, writeSync, readSync, queueFamilyB);
		readOp->recordCommands(*commandBufferB);
		endCommandBuffer(m_vkdB, *commandBufferB);

		{
			const vk::VkCommandBuffer	commandBuffer	= *commandBufferA;
			const vk::VkSemaphore		semaphore		= *semaphoreA;
			const vk::VkSubmitInfo		submitInfo		=
			{
				vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,
				DE_NULL,

				0u,
				DE_NULL,
				DE_NULL,

				1u,
				&commandBuffer,
				1u,
				&semaphore
			};

			VK_CHECK(m_vkdA.queueSubmit(queueA, 1u, &submitInfo, DE_NULL));

			{
				NativeHandle	nativeSemaphoreHandle;

				getSemaphoreNative(m_vkdA, *m_deviceA, *semaphoreA, m_semaphoreHandleType, nativeSemaphoreHandle);
				importSemaphore(m_vkdB, *m_deviceB, *semaphoreB, m_semaphoreHandleType, nativeSemaphoreHandle, 0u);
			}
		}
		{
			const vk::VkCommandBuffer		commandBuffer	= *commandBufferB;
			const vk::VkSemaphore			semaphore		= *semaphoreB;
			const vk::VkPipelineStageFlags	dstStage		= readSync.stageMask;
			const vk::VkSubmitInfo			submitInfo		=
			{
				vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,
				DE_NULL,

				1u,
				&semaphore,
				&dstStage,

				1u,
				&commandBuffer,
				0u,
				DE_NULL,
			};

			VK_CHECK(m_vkdB.queueSubmit(queueB, 1u, &submitInfo, DE_NULL));
		}

		VK_CHECK(m_vkdA.queueWaitIdle(queueA));
		VK_CHECK(m_vkdB.queueWaitIdle(queueB));

		{
			const Data	expected	= writeOp->getData();
			const Data	actual		= readOp->getData();

			DE_ASSERT(expected.size == actual.size);

			if (0 != deMemCmp(expected.data, actual.data, expected.size))
			{
				const size_t		maxBytesLogged	= 256;
				std::ostringstream	expectedData;
				std::ostringstream	actualData;
				size_t				byteNdx			= 0;

				// Find first byte difference
				for (; actual.data[byteNdx] == expected.data[byteNdx]; byteNdx++)
				{
					// Nothing
				}

				log << TestLog::Message << "First different byte at offset: " << byteNdx << TestLog::EndMessage;

				// Log 8 previous bytes before the first incorrect byte
				if (byteNdx > 8)
				{
					expectedData << "... ";
					actualData << "... ";

					byteNdx -= 8;
				}
				else
					byteNdx = 0;

				for (size_t i = 0; i < maxBytesLogged && byteNdx < expected.size; i++, byteNdx++)
				{
					expectedData << (i > 0 ? ", " : "") << (deUint32)expected.data[byteNdx];
					actualData << (i > 0 ? ", " : "") << (deUint32)actual.data[byteNdx];
				}

				if (expected.size > byteNdx)
				{
					expectedData << "...";
					actualData << "...";
				}

				log << TestLog::Message << "Expected data: (" << expectedData.str() << ")" << TestLog::EndMessage;
				log << TestLog::Message << "Actual data: (" << actualData.str() << ")" << TestLog::EndMessage;

				m_resultCollector.fail("Memory contents don't match");
			}
		}
	}
	catch (const tcu::NotSupportedError& error)
	{
		log << TestLog::Message << "Not supported: " << error.getMessage() << TestLog::EndMessage;
	}
	catch (const tcu::TestError& error)
	{
		m_resultCollector.fail(std::string("Exception: ") + error.getMessage());
	}

	// Move to next queue
	{
		m_queueBNdx++;

		if (m_queueBNdx >= m_queueFamiliesB.size())
		{
			m_queueANdx++;

			if (m_queueANdx >= m_queueFamiliesA.size())
			{
				return tcu::TestStatus(m_resultCollector.getResult(), m_resultCollector.getMessage());
			}
			else
			{
				m_queueBNdx = 0;

				return tcu::TestStatus::incomplete();
			}
		}
		else
			return tcu::TestStatus::incomplete();
	}
}

struct Progs
{
	void init (vk::SourceCollections& dst, TestConfig config) const
	{
		const de::UniquePtr<OperationSupport>	readOp	(makeOperationSupport(config.readOp, config.resource));
		const de::UniquePtr<OperationSupport>	writeOp	(makeOperationSupport(config.writeOp, config.resource));

		readOp->initPrograms(dst);
		writeOp->initPrograms(dst);
	}
};

} // anonymous

tcu::TestCaseGroup* createCrossInstanceSharingTest (tcu::TestContext& testCtx)
{
	const struct
	{
		vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryType;
		vk::VkExternalSemaphoreHandleTypeFlagBitsKHR	semaphoreType;
		const char*										nameSuffix;
	} cases[] =
	{
		{
			vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
			vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
			"_fd"
		},
		{
			vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR,
			vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT_KHR,
			"_fence_fd"
		},
		{
			vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR,
			vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR,
			"_win32_kmt"
		},
		{
			vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
			vk::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
			"_win32"
		},
	};
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "cross_instance", ""));

	for (size_t dedicatedNdx = 0; dedicatedNdx < 2; dedicatedNdx++)
	{
		const bool						dedicated		(dedicatedNdx == 1);
		de::MovePtr<tcu::TestCaseGroup>	dedicatedGroup	(new tcu::TestCaseGroup(testCtx, dedicated ? "dedicated" : "suballocated", ""));

		for (size_t writeOpNdx = 0; writeOpNdx < DE_LENGTH_OF_ARRAY(s_writeOps); ++writeOpNdx)
		for (size_t readOpNdx = 0; readOpNdx < DE_LENGTH_OF_ARRAY(s_readOps); ++readOpNdx)
		{
			const OperationName	writeOp		= s_writeOps[writeOpNdx];
			const OperationName	readOp		= s_readOps[readOpNdx];
			const std::string	opGroupName	= getOperationName(writeOp) + "_" + getOperationName(readOp);
			bool				empty		= true;

			de::MovePtr<tcu::TestCaseGroup> opGroup	(new tcu::TestCaseGroup(testCtx, opGroupName.c_str(), ""));

			for (size_t resourceNdx = 0; resourceNdx < DE_LENGTH_OF_ARRAY(s_resources); ++resourceNdx)
			{
				const ResourceDescription&	resource	= s_resources[resourceNdx];

				for (size_t caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); caseNdx++)
				{
					std::string	name= getResourceName(resource) + cases[caseNdx].nameSuffix;

					if (isResourceSupported(writeOp, resource) && isResourceSupported(readOp, resource))
					{
						const TestConfig config (resource, writeOp, readOp, cases[caseNdx].memoryType, cases[caseNdx].semaphoreType, dedicated);

						opGroup->addChild(new InstanceFactory1<SharingTestInstance, TestConfig, Progs>(testCtx, tcu::NODETYPE_SELF_VALIDATE,  name, "", Progs(), config));
						empty = false;
					}
				}
			}

			if (!empty)
				dedicatedGroup->addChild(opGroup.release());
		}

		group->addChild(dedicatedGroup.release());
	}

	return group.release();
}

} // synchronization
} // vkt
