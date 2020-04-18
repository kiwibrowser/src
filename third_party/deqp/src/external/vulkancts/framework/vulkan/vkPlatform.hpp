#ifndef _VKPLATFORM_HPP
#define _VKPLATFORM_HPP
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

#include "vkDefs.hpp"

#include <ostream>

namespace tcu
{
class FunctionLibrary;
}

namespace vk
{

class Library
{
public:
										Library					(void) {}
	virtual								~Library				(void) {}

	virtual const PlatformInterface&	getPlatformInterface	(void) const = 0;
};

class PlatformDriver : public PlatformInterface
{
public:
				PlatformDriver	(const tcu::FunctionLibrary& library);
				~PlatformDriver	(void);

#include "vkConcretePlatformInterface.inl"

protected:
	struct Functions
	{
#include "vkPlatformFunctionPointers.inl"
	};

	Functions	m_vk;
};

class InstanceDriver : public InstanceInterface
{
public:
				InstanceDriver	(const PlatformInterface& platformInterface, VkInstance instance);
				~InstanceDriver	(void);

#include "vkConcreteInstanceInterface.inl"

protected:
	struct Functions
	{
#include "vkInstanceFunctionPointers.inl"
	};

	Functions	m_vk;
};

class DeviceDriver : public DeviceInterface
{
public:
				DeviceDriver	(const InstanceInterface& instanceInterface, VkDevice device);
				~DeviceDriver	(void);

#include "vkConcreteDeviceInterface.inl"

protected:
	struct Functions
	{
#include "vkDeviceFunctionPointers.inl"
	};

	Functions	m_vk;
};

// Defined in vkWsiPlatform.hpp
namespace wsi
{
class Display;
} // wsi

struct PlatformMemoryLimits
{
	// System memory properties
	size_t			totalSystemMemory;					//!< #bytes of system memory (heap + HOST_LOCAL) tests must not exceed

	// Device memory properties
	VkDeviceSize	totalDeviceLocalMemory;				//!< #bytes of total DEVICE_LOCAL memory tests must not exceed or 0 if DEVICE_LOCAL counts against system memory
	VkDeviceSize	deviceMemoryAllocationGranularity;	//!< VkDeviceMemory allocation granularity (typically page size)

	// Device memory page table geometry
	// \todo [2016-03-23 pyry] This becomes obsolete if Vulkan API adds a way for driver to expose internal device memory allocations
	VkDeviceSize	devicePageSize;						//!< Page size on device (must be rounded up to nearest POT)
	VkDeviceSize	devicePageTableEntrySize;			//!< Number of bytes per page table size
	size_t			devicePageTableHierarchyLevels;		//!< Number of levels in device page table hierarchy

	PlatformMemoryLimits (void)
		: totalSystemMemory					(0)
		, totalDeviceLocalMemory			(0)
		, deviceMemoryAllocationGranularity	(0)
		, devicePageSize					(0)
		, devicePageTableEntrySize			(0)
		, devicePageTableHierarchyLevels	(0)
	{}
};

/*--------------------------------------------------------------------*//*!
 * \brief Vulkan platform interface
 *//*--------------------------------------------------------------------*/
class Platform
{
public:
							Platform			(void) {}
							~Platform			(void) {}

	virtual Library*		createLibrary		(void) const = 0;

	virtual wsi::Display*	createWsiDisplay	(wsi::Type wsiType) const;

	virtual void			getMemoryLimits		(PlatformMemoryLimits& limits) const = 0;
	virtual void			describePlatform	(std::ostream& dst) const;
};

inline PlatformMemoryLimits getMemoryLimits (const Platform& platform)
{
	PlatformMemoryLimits limits;
	platform.getMemoryLimits(limits);
	return limits;
}

} // vk

#endif // _VKPLATFORM_HPP
