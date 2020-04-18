/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 Google Inc.
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
 * \brief Utilities for Vulkan SPIR-V assembly tests
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmUtils.hpp"

#include "deMemory.h"
#include "deSTLUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"

namespace vkt
{
namespace SpirVAssembly
{

using namespace vk;

namespace
{

VkPhysicalDeviceFeatures filterDefaultDeviceFeatures (const VkPhysicalDeviceFeatures& deviceFeatures)
{
	VkPhysicalDeviceFeatures enabledDeviceFeatures = deviceFeatures;

	// Disable robustness by default, as it has an impact on performance on some HW.
	enabledDeviceFeatures.robustBufferAccess = false;

	return enabledDeviceFeatures;
}

VkPhysicalDevice16BitStorageFeaturesKHR	querySupported16BitStorageFeatures (const InstanceInterface& vki, VkPhysicalDevice device, const std::vector<std::string>& instanceExtensions)
{
	VkPhysicalDevice16BitStorageFeaturesKHR	extensionFeatures	=
	{
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR,	// sType
		DE_NULL,														// pNext
		false,															// storageUniformBufferBlock16
		false,															// storageUniform16
		false,															// storagePushConstant16
		false,															// storageInputOutput16
	};
	VkPhysicalDeviceFeatures2KHR			features;

	deMemset(&features, 0, sizeof(features));
	features.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
	features.pNext	= &extensionFeatures;

	// Call the getter only if supported. Otherwise above "zero" defaults are used
	if (de::contains(instanceExtensions.begin(), instanceExtensions.end(), "VK_KHR_get_physical_device_properties2"))
	{
		vki.getPhysicalDeviceFeatures2KHR(device, &features);
	}

	return extensionFeatures;
}

VkPhysicalDeviceVariablePointerFeaturesKHR querySupportedVariablePointersFeatures (const InstanceInterface& vki, VkPhysicalDevice device, const std::vector<std::string>& instanceExtensions)
{
	VkPhysicalDeviceVariablePointerFeaturesKHR extensionFeatures	=
	{
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES_KHR,	// sType
		DE_NULL,															// pNext
		false,																// variablePointersStorageBuffer
		false,																// variablePointers
	};

	VkPhysicalDeviceFeatures2KHR	features;
	deMemset(&features, 0, sizeof(features));
	features.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
	features.pNext	= &extensionFeatures;

	// Call the getter only if supported. Otherwise above "zero" defaults are used
	if (de::contains(instanceExtensions.begin(), instanceExtensions.end(), "VK_KHR_get_physical_device_properties2"))
	{
		vki.getPhysicalDeviceFeatures2KHR(device, &features);
	}

	return extensionFeatures;
}

} // anonymous

bool is16BitStorageFeaturesSupported (const InstanceInterface& vki, VkPhysicalDevice device, const std::vector<std::string>& instanceExtensions, Extension16BitStorageFeatures toCheck)
{
	VkPhysicalDevice16BitStorageFeaturesKHR extensionFeatures	= querySupported16BitStorageFeatures(vki, device, instanceExtensions);

	if ((toCheck & EXT16BITSTORAGEFEATURES_UNIFORM_BUFFER_BLOCK) != 0 && extensionFeatures.storageBuffer16BitAccess == VK_FALSE)
		return false;

	if ((toCheck & EXT16BITSTORAGEFEATURES_UNIFORM) != 0 && extensionFeatures.uniformAndStorageBuffer16BitAccess == VK_FALSE)
		return false;

	if ((toCheck & EXT16BITSTORAGEFEATURES_PUSH_CONSTANT) != 0 && extensionFeatures.storagePushConstant16 == VK_FALSE)
		return false;

	if ((toCheck & EXT16BITSTORAGEFEATURES_INPUT_OUTPUT) != 0 && extensionFeatures.storageInputOutput16 == VK_FALSE)
		return false;

	return true;
}

bool isVariablePointersFeaturesSupported (const InstanceInterface& vki, VkPhysicalDevice device, const std::vector<std::string>& instanceExtensions, ExtensionVariablePointersFeatures toCheck)
{
	VkPhysicalDeviceVariablePointerFeaturesKHR extensionFeatures = querySupportedVariablePointersFeatures(vki, device, instanceExtensions);

	if ((toCheck & EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS_STORAGEBUFFER) != 0 && extensionFeatures.variablePointersStorageBuffer == VK_FALSE)
		return false;

	if ((toCheck & EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS) != 0 && extensionFeatures.variablePointers == VK_FALSE)
		return false;

	return true;
}

Move<VkDevice> createDeviceWithExtensions (Context&							context,
										   const deUint32					queueFamilyIndex,
										   const std::vector<std::string>&	supportedExtensions,
										   const std::vector<std::string>&	requiredExtensions)
{
	const InstanceInterface&					vki							= context.getInstanceInterface();
	const VkPhysicalDevice						physicalDevice				= context.getPhysicalDevice();
	std::vector<const char*>					extensions					(requiredExtensions.size());
	void*										pExtension					= DE_NULL;
	const VkPhysicalDeviceFeatures				deviceFeatures				= getPhysicalDeviceFeatures(vki, physicalDevice);
	VkPhysicalDevice16BitStorageFeaturesKHR		ext16BitStorageFeatures;
	VkPhysicalDeviceVariablePointerFeaturesKHR	extVariablePointerFeatures;

	for (deUint32 extNdx = 0; extNdx < requiredExtensions.size(); ++extNdx)
	{
		const std::string&	ext = requiredExtensions[extNdx];

		// Check that all required extensions are supported first.
		if (!de::contains(supportedExtensions.begin(), supportedExtensions.end(), ext))
		{
			TCU_THROW(NotSupportedError, (std::string("Device extension not supported: ") + ext).c_str());
		}

		// Currently don't support enabling multiple extensions at the same time.
		if (ext == "VK_KHR_16bit_storage")
		{
			// For the 16bit storage extension, we have four features to test. Requesting all features supported.
			// Note that we don't throw NotImplemented errors here if a specific feature is not supported;
			// that should be done when actually trying to use that specific feature.
			ext16BitStorageFeatures	= querySupported16BitStorageFeatures(vki, physicalDevice, context.getInstanceExtensions());
			pExtension = &ext16BitStorageFeatures;
		}
		else if (ext == "VK_KHR_variable_pointers")
		{
			// For the VariablePointers extension, we have two features to test. Requesting all features supported.
			extVariablePointerFeatures	= querySupportedVariablePointersFeatures(vki, physicalDevice, context.getInstanceExtensions());
			pExtension = &extVariablePointerFeatures;
		}

		extensions[extNdx] = ext.c_str();
	}

	const float						queuePriorities[]	= { 1.0f };
	const VkDeviceQueueCreateInfo	queueInfos[]		=
	{
		{
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			DE_NULL,
			(VkDeviceQueueCreateFlags)0,
			queueFamilyIndex,
			DE_LENGTH_OF_ARRAY(queuePriorities),
			&queuePriorities[0]
		}
	};
	const VkPhysicalDeviceFeatures	features			= filterDefaultDeviceFeatures(deviceFeatures);
	const VkDeviceCreateInfo		deviceParams		=
	{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		pExtension,
		(VkDeviceCreateFlags)0,
		DE_LENGTH_OF_ARRAY(queueInfos),
		&queueInfos[0],
		0u,
		DE_NULL,
		(deUint32)extensions.size(),
		extensions.empty() ? DE_NULL : &extensions[0],
		&features
	};

	return vk::createDevice(vki, physicalDevice, &deviceParams);
}

Allocator* createAllocator (const InstanceInterface& instanceInterface, const VkPhysicalDevice physicalDevice, const DeviceInterface& deviceInterface, const VkDevice device)
{
	const VkPhysicalDeviceMemoryProperties memoryProperties = getPhysicalDeviceMemoryProperties(instanceInterface, physicalDevice);

	// \todo [2015-07-24 jarkko] support allocator selection/configuration from command line (or compile time)
	return new SimpleAllocator(deviceInterface, device, memoryProperties);
}

} // SpirVAssembly
} // vkt
