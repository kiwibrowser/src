/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2016 The Android Open Source Project
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
 * \brief Win32 Vulkan platform
 *//*--------------------------------------------------------------------*/

// \todo [2016-01-22 pyry] GetVersionEx() used by getOSInfo() is deprecated.
//						   Find a way to get version info without using deprecated APIs.
#pragma warning(disable : 4996)

#include "tcuWin32VulkanPlatform.hpp"
#include "tcuWin32Window.hpp"

#include "tcuFormatUtil.hpp"
#include "tcuFunctionLibrary.hpp"
#include "tcuVector.hpp"

#include "vkWsiPlatform.hpp"

#include "deUniquePtr.hpp"
#include "deMemory.h"

namespace tcu
{
namespace win32
{

using de::MovePtr;
using de::UniquePtr;

DE_STATIC_ASSERT(sizeof(vk::pt::Win32InstanceHandle)	== sizeof(HINSTANCE));
DE_STATIC_ASSERT(sizeof(vk::pt::Win32WindowHandle)		== sizeof(HWND));

class VulkanWindow : public vk::wsi::Win32WindowInterface
{
public:
	VulkanWindow (MovePtr<win32::Window> window)
		: vk::wsi::Win32WindowInterface	(vk::pt::Win32WindowHandle(window->getHandle()))
		, m_window						(window)
	{
	}

	void resize (const UVec2& newSize)
	{
		m_window->setSize((int)newSize.x(), (int)newSize.y());
	}

private:
	UniquePtr<win32::Window>	m_window;
};

class VulkanDisplay : public vk::wsi::Win32DisplayInterface
{
public:
	VulkanDisplay (HINSTANCE instance)
		: vk::wsi::Win32DisplayInterface	(vk::pt::Win32InstanceHandle(instance))
	{
	}

	vk::wsi::Window* createWindow (const Maybe<UVec2>& initialSize) const
	{
		const HINSTANCE	instance	= (HINSTANCE)m_native.internal;
		const deUint32	width		= !initialSize ? 400 : initialSize->x();
		const deUint32	height		= !initialSize ? 300 : initialSize->y();

		return new VulkanWindow(MovePtr<win32::Window>(new win32::Window(instance, (int)width, (int)height)));
	}
};

class VulkanLibrary : public vk::Library
{
public:
	VulkanLibrary (void)
		: m_library	("vulkan-1.dll")
		, m_driver	(m_library)
	{
	}

	const vk::PlatformInterface& getPlatformInterface (void) const
	{
		return m_driver;
	}

private:
	const tcu::DynamicFunctionLibrary	m_library;
	const vk::PlatformDriver			m_driver;
};

VulkanPlatform::VulkanPlatform (HINSTANCE instance)
	: m_instance(instance)
{
}

VulkanPlatform::~VulkanPlatform (void)
{
}

vk::Library* VulkanPlatform::createLibrary (void) const
{
	return new VulkanLibrary();
}

const char* getProductTypeName (WORD productType)
{
	switch (productType)
	{
		case VER_NT_DOMAIN_CONTROLLER:	return "Windows Server (domain controller)";
		case VER_NT_SERVER:				return "Windows Server";
		case VER_NT_WORKSTATION:		return "Windows NT";
		default:						return DE_NULL;
	}
}

static void getOSInfo (std::ostream& dst)
{
	OSVERSIONINFOEX	osInfo;

	deMemset(&osInfo, 0, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = (DWORD)sizeof(osInfo);

	GetVersionEx((OSVERSIONINFO*)&osInfo);

	{
		const char* const	productName	= getProductTypeName(osInfo.wProductType);

		if (productName)
			dst << productName;
		else
			dst << "unknown product " << tcu::toHex(osInfo.wProductType);
	}

	dst << " " << osInfo.dwMajorVersion << "." << osInfo.dwMinorVersion
		<< ", service pack " << osInfo.wServicePackMajor << "." << osInfo.wServicePackMinor
		<< ", build " << osInfo.dwBuildNumber;
}

const char* getProcessorArchitectureName (WORD arch)
{
	switch (arch)
	{
		case PROCESSOR_ARCHITECTURE_AMD64:		return "AMD64";
		case PROCESSOR_ARCHITECTURE_ARM:		return "ARM";
		case PROCESSOR_ARCHITECTURE_IA64:		return "IA64";
		case PROCESSOR_ARCHITECTURE_INTEL:		return "INTEL";
		case PROCESSOR_ARCHITECTURE_UNKNOWN:	return "UNKNOWN";
		default:								return DE_NULL;
	}
}

static void getProcessorInfo (std::ostream& dst)
{
	SYSTEM_INFO	sysInfo;

	deMemset(&sysInfo, 0, sizeof(sysInfo));
	GetSystemInfo(&sysInfo);

	dst << "arch ";
	{
		const char* const	archName	= getProcessorArchitectureName(sysInfo.wProcessorArchitecture);

		if (archName)
			dst << archName;
		else
			dst << tcu::toHex(sysInfo.wProcessorArchitecture);
	}

	dst << ", level " << tcu::toHex(sysInfo.wProcessorLevel) << ", revision " << tcu::toHex(sysInfo.wProcessorRevision);
}

void VulkanPlatform::describePlatform (std::ostream& dst) const
{
	dst << "OS: ";
	getOSInfo(dst);
	dst << "\n";

	dst << "CPU: ";
	getProcessorInfo(dst);
	dst << "\n";
}

void VulkanPlatform::getMemoryLimits (vk::PlatformMemoryLimits& limits) const
{
	limits.totalSystemMemory					= 256*1024*1024;
	limits.totalDeviceLocalMemory				= 128*1024*1024;
	limits.deviceMemoryAllocationGranularity	= 64*1024;
	limits.devicePageSize						= 4096;
	limits.devicePageTableEntrySize				= 8;
	limits.devicePageTableHierarchyLevels		= 3;
}

vk::wsi::Display* VulkanPlatform::createWsiDisplay (vk::wsi::Type wsiType) const
{
	if (wsiType != vk::wsi::TYPE_WIN32)
		TCU_THROW(NotSupportedError, "WSI type not supported");

	return new VulkanDisplay(m_instance);
}

} // win32
} // tcu
