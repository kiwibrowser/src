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
 * \brief Instance and device initialization utilities.
 *//*--------------------------------------------------------------------*/

#include "vkDeviceUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkApiVersion.hpp"

#include "tcuCommandLine.hpp"

#include "qpInfo.h"

namespace vk
{

using std::vector;
using std::string;

Move<VkInstance> createDefaultInstance (const PlatformInterface&		vkPlatform,
										const vector<string>&			enabledLayers,
										const vector<string>&			enabledExtensions,
										const VkAllocationCallbacks*	pAllocator)
{
	vector<const char*>		layerNamePtrs		(enabledLayers.size());
	vector<const char*>		extensionNamePtrs	(enabledExtensions.size());
	const deUint32			apiVersion			= pack(ApiVersion(1, 0, 0));

	const struct VkApplicationInfo		appInfo			=
	{
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		DE_NULL,
		"deqp",									// pAppName
		qpGetReleaseId(),						// appVersion
		"deqp",									// pEngineName
		qpGetReleaseId(),						// engineVersion
		apiVersion								// apiVersion
	};
	const struct VkInstanceCreateInfo	instanceInfo	=
	{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		DE_NULL,
		(VkInstanceCreateFlags)0,
		&appInfo,
		(deUint32)layerNamePtrs.size(),
		layerNamePtrs.empty() ? DE_NULL : &layerNamePtrs[0],
		(deUint32)extensionNamePtrs.size(),
		extensionNamePtrs.empty() ? DE_NULL : &extensionNamePtrs[0],
	};

	for (size_t ndx = 0; ndx < enabledLayers.size(); ++ndx)
		layerNamePtrs[ndx] = enabledLayers[ndx].c_str();

	for (size_t ndx = 0; ndx < enabledExtensions.size(); ++ndx)
		extensionNamePtrs[ndx] = enabledExtensions[ndx].c_str();

	return createInstance(vkPlatform, &instanceInfo, pAllocator);
}

Move<VkInstance> createDefaultInstance (const PlatformInterface& vkPlatform)
{
	return createDefaultInstance(vkPlatform, vector<string>(), vector<string>(), DE_NULL);
}

VkPhysicalDevice chooseDevice (const InstanceInterface& vkInstance, VkInstance instance, const tcu::CommandLine& cmdLine)
{
	const vector<VkPhysicalDevice>	devices	= enumeratePhysicalDevices(vkInstance, instance);

	if (devices.empty())
		TCU_THROW(NotSupportedError, "No Vulkan devices available");

	if (!de::inBounds(cmdLine.getVKDeviceId(), 1, (int)devices.size()+1))
		TCU_THROW(InternalError, "Invalid --deqp-vk-device-id");

	return devices[(size_t)(cmdLine.getVKDeviceId()-1)];
}

} // vk
