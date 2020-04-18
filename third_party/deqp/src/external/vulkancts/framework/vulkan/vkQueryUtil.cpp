/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief Vulkan query utilities.
 *//*--------------------------------------------------------------------*/

#include "vkQueryUtil.hpp"
#include "deMemory.h"

namespace vk
{

using std::vector;

vector<VkPhysicalDevice> enumeratePhysicalDevices (const InstanceInterface& vk, VkInstance instance)
{
	deUint32					numDevices	= 0;
	vector<VkPhysicalDevice>	devices;

	VK_CHECK(vk.enumeratePhysicalDevices(instance, &numDevices, DE_NULL));

	if (numDevices > 0)
	{
		devices.resize(numDevices);
		VK_CHECK(vk.enumeratePhysicalDevices(instance, &numDevices, &devices[0]));

		if ((size_t)numDevices != devices.size())
			TCU_FAIL("Returned device count changed between queries");
	}

	return devices;
}

vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties (const InstanceInterface& vk, VkPhysicalDevice physicalDevice)
{
	deUint32						numQueues	= 0;
	vector<VkQueueFamilyProperties>	properties;

	vk.getPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueues, DE_NULL);

	if (numQueues > 0)
	{
		properties.resize(numQueues);
		vk.getPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueues, &properties[0]);

		if ((size_t)numQueues != properties.size())
			TCU_FAIL("Returned queue family count changes between queries");
	}

	return properties;
}

VkPhysicalDeviceFeatures getPhysicalDeviceFeatures (const InstanceInterface& vk, VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceFeatures	features;

	deMemset(&features, 0, sizeof(features));

	vk.getPhysicalDeviceFeatures(physicalDevice, &features);
	return features;
}

VkPhysicalDeviceProperties getPhysicalDeviceProperties (const InstanceInterface& vk, VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties	properties;

	deMemset(&properties, 0, sizeof(properties));

	vk.getPhysicalDeviceProperties(physicalDevice, &properties);
	return properties;
}

VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties (const InstanceInterface& vk, VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceMemoryProperties	properties;

	deMemset(&properties, 0, sizeof(properties));

	vk.getPhysicalDeviceMemoryProperties(physicalDevice, &properties);
	return properties;
}

VkFormatProperties getPhysicalDeviceFormatProperties (const InstanceInterface& vk, VkPhysicalDevice physicalDevice, VkFormat format)
{
	VkFormatProperties	properties;

	deMemset(&properties, 0, sizeof(properties));

	vk.getPhysicalDeviceFormatProperties(physicalDevice, format, &properties);
	return properties;
}

VkImageFormatProperties getPhysicalDeviceImageFormatProperties (const InstanceInterface& vk, VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags)
{
	VkImageFormatProperties	properties;

	deMemset(&properties, 0, sizeof(properties));

	VK_CHECK(vk.getPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, &properties));
	return properties;
}

std::vector<VkSparseImageFormatProperties> getPhysicalDeviceSparseImageFormatProperties(const InstanceInterface& vk, VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling)
{
	deUint32								numProp = 0;
	vector<VkSparseImageFormatProperties>	properties;

	vk.getPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, &numProp, DE_NULL);

	if (numProp > 0)
	{
		properties.resize(numProp);
		vk.getPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, &numProp, &properties[0]);

		if ((size_t)numProp != properties.size())
			TCU_FAIL("Returned sparse image properties count changes between queries");
	}

	return properties;
}

std::vector<VkSparseImageMemoryRequirements> getImageSparseMemoryRequirements(const DeviceInterface& vk, VkDevice device, VkImage image)
{
	deUint32								requirementsCount = 0;
	vector<VkSparseImageMemoryRequirements> requirements;

	vk.getImageSparseMemoryRequirements(device, image, &requirementsCount, DE_NULL);

	if (requirementsCount > 0)
	{
		requirements.resize(requirementsCount);
		vk.getImageSparseMemoryRequirements(device, image, &requirementsCount, &requirements[0]);

		if ((size_t)requirementsCount != requirements.size())
			TCU_FAIL("Returned sparse image memory requirements count changes between queries");
	}

	return requirements;
}

VkMemoryRequirements getBufferMemoryRequirements (const DeviceInterface& vk, VkDevice device, VkBuffer buffer)
{
	VkMemoryRequirements req;
	vk.getBufferMemoryRequirements(device, buffer, &req);
	return req;
}

VkMemoryRequirements getImageMemoryRequirements (const DeviceInterface& vk, VkDevice device, VkImage image)
{
	VkMemoryRequirements req;
	vk.getImageMemoryRequirements(device, image, &req);
	return req;
}

VkMemoryRequirements getImagePlaneMemoryRequirements (const DeviceInterface&	vkd,
													  VkDevice					device,
													  VkImage					image,
													  VkImageAspectFlagBits		planeAspect)
{
	VkImageMemoryRequirementsInfo2KHR		coreInfo;
	VkImagePlaneMemoryRequirementsInfoKHR	planeInfo;
	VkMemoryRequirements2KHR				reqs;

	deMemset(&coreInfo,		0, sizeof(coreInfo));
	deMemset(&planeInfo,	0, sizeof(planeInfo));
	deMemset(&reqs,			0, sizeof(reqs));

	coreInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR;
	coreInfo.pNext			= &planeInfo;
	coreInfo.image			= image;

	planeInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO_KHR;
	planeInfo.planeAspect	= planeAspect;

	reqs.sType				= VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;

	vkd.getImageMemoryRequirements2KHR(device, &coreInfo, &reqs);

	return reqs.memoryRequirements;
}

vector<VkLayerProperties> enumerateInstanceLayerProperties (const PlatformInterface& vkp)
{
	vector<VkLayerProperties>	properties;
	deUint32					numLayers	= 0;

	VK_CHECK(vkp.enumerateInstanceLayerProperties(&numLayers, DE_NULL));

	if (numLayers > 0)
	{
		properties.resize(numLayers);
		VK_CHECK(vkp.enumerateInstanceLayerProperties(&numLayers, &properties[0]));
		TCU_CHECK((size_t)numLayers == properties.size());
	}

	return properties;
}

vector<VkExtensionProperties> enumerateInstanceExtensionProperties (const PlatformInterface& vkp, const char* layerName)
{
	vector<VkExtensionProperties>	properties;
	deUint32						numExtensions	= 0;

	VK_CHECK(vkp.enumerateInstanceExtensionProperties(layerName, &numExtensions, DE_NULL));

	if (numExtensions > 0)
	{
		properties.resize(numExtensions);
		VK_CHECK(vkp.enumerateInstanceExtensionProperties(layerName, &numExtensions, &properties[0]));
		TCU_CHECK((size_t)numExtensions == properties.size());
	}

	return properties;
}

vector<VkLayerProperties> enumerateDeviceLayerProperties (const InstanceInterface& vki, VkPhysicalDevice physicalDevice)
{
	vector<VkLayerProperties>	properties;
	deUint32					numLayers	= 0;

	VK_CHECK(vki.enumerateDeviceLayerProperties(physicalDevice, &numLayers, DE_NULL));

	if (numLayers > 0)
	{
		properties.resize(numLayers);
		VK_CHECK(vki.enumerateDeviceLayerProperties(physicalDevice, &numLayers, &properties[0]));
		TCU_CHECK((size_t)numLayers == properties.size());
	}

	return properties;
}

vector<VkExtensionProperties> enumerateDeviceExtensionProperties (const InstanceInterface& vki, VkPhysicalDevice physicalDevice, const char* layerName)
{
	vector<VkExtensionProperties>	properties;
	deUint32						numExtensions	= 0;

	VK_CHECK(vki.enumerateDeviceExtensionProperties(physicalDevice, layerName, &numExtensions, DE_NULL));

	if (numExtensions > 0)
	{
		properties.resize(numExtensions);
		VK_CHECK(vki.enumerateDeviceExtensionProperties(physicalDevice, layerName, &numExtensions, &properties[0]));
		TCU_CHECK((size_t)numExtensions == properties.size());
	}

	return properties;
}

bool isShaderStageSupported (const VkPhysicalDeviceFeatures& deviceFeatures, VkShaderStageFlagBits stage)
{
	if (stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT || stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		return deviceFeatures.tessellationShader == VK_TRUE;
	else if (stage == VK_SHADER_STAGE_GEOMETRY_BIT)
		return deviceFeatures.geometryShader == VK_TRUE;
	else
		return true;
}

bool isCompatible (const VkExtensionProperties& extensionProperties, const RequiredExtension& required)
{
	if (required.name != extensionProperties.extensionName)
		return false;

	if (required.minVersion && required.minVersion.get() > extensionProperties.specVersion)
		return false;

	if (required.maxVersion && required.maxVersion.get() < extensionProperties.specVersion)
		return false;

	return true;
}

bool isCompatible (const VkLayerProperties& layerProperties, const RequiredLayer& required)
{
	if (required.name != layerProperties.layerName)
		return false;

	if (required.minSpecVersion && required.minSpecVersion.get() > layerProperties.specVersion)
		return false;

	if (required.maxSpecVersion && required.maxSpecVersion.get() < layerProperties.specVersion)
		return false;

	if (required.minImplVersion && required.minImplVersion.get() > layerProperties.implementationVersion)
		return false;

	if (required.maxImplVersion && required.maxImplVersion.get() < layerProperties.implementationVersion)
		return false;

	return true;
}

bool isExtensionSupported (const std::vector<VkExtensionProperties>& extensions, const RequiredExtension& required)
{
	return isExtensionSupported(extensions.begin(), extensions.end(), required);
}

bool isLayerSupported (const std::vector<VkLayerProperties>& layers, const RequiredLayer& required)
{
	return isLayerSupported(layers.begin(), layers.end(), required);
}

VkQueue getDeviceQueue (const DeviceInterface& vkd, VkDevice device, deUint32 queueFamilyIndex, deUint32 queueIndex)
{
	VkQueue queue;

	vkd.getDeviceQueue(device, queueFamilyIndex, queueIndex, &queue);

	return queue;
}

const void* findStructureInChain (const void* first, VkStructureType type)
{
	struct StructureBase
	{
		VkStructureType		sType;
		void*				pNext;
	};

	const StructureBase*	cur		= reinterpret_cast<const StructureBase*>(first);

	while (cur)
	{
		if (cur->sType == type)
			break;
		else
			cur = reinterpret_cast<const StructureBase*>(cur->pNext);
	}

	return cur;
}

void* findStructureInChain (void* first, VkStructureType type)
{
	return const_cast<void*>(findStructureInChain(const_cast<const void*>(first), type));
}

// getStructureType<T> implementations
#include "vkGetStructureTypeImpl.inl"

} // vk
