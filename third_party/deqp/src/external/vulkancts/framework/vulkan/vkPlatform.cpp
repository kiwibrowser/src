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
 * \brief Vulkan platform abstraction.
 *//*--------------------------------------------------------------------*/

#include "vkPlatform.hpp"
#include "tcuFunctionLibrary.hpp"

namespace vk
{

PlatformDriver::PlatformDriver (const tcu::FunctionLibrary& library)
{
	m_vk.getInstanceProcAddr	= (GetInstanceProcAddrFunc)library.getFunction("vkGetInstanceProcAddr");

#define GET_PROC_ADDR(NAME) m_vk.getInstanceProcAddr(DE_NULL, NAME)
#include "vkInitPlatformFunctionPointers.inl"
#undef GET_PROC_ADDR
}

PlatformDriver::~PlatformDriver (void)
{
}

InstanceDriver::InstanceDriver (const PlatformInterface& platformInterface, VkInstance instance)
{
#define GET_PROC_ADDR(NAME) platformInterface.getInstanceProcAddr(instance, NAME)
#include "vkInitInstanceFunctionPointers.inl"
#undef GET_PROC_ADDR
}

InstanceDriver::~InstanceDriver (void)
{
}

DeviceDriver::DeviceDriver (const InstanceInterface& instanceInterface, VkDevice device)
{
#define GET_PROC_ADDR(NAME) instanceInterface.getDeviceProcAddr(device, NAME)
#include "vkInitDeviceFunctionPointers.inl"
#undef GET_PROC_ADDR
}

DeviceDriver::~DeviceDriver (void)
{
}

#include "vkPlatformDriverImpl.inl"
#include "vkInstanceDriverImpl.inl"
#include "vkDeviceDriverImpl.inl"

wsi::Display* Platform::createWsiDisplay (wsi::Type) const
{
	TCU_THROW(NotSupportedError, "WSI not supported");
}

void Platform::describePlatform (std::ostream& dst) const
{
	dst << "vk::Platform::describePlatform() not implemented";
}

} // vk
