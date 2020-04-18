/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
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
 * \brief Linux Vulkan Platform.
 *//*--------------------------------------------------------------------*/

#include "tcuLnxVulkanPlatform.hpp"
#include "tcuLnxPlatform.hpp"
#include "vkWsiPlatform.hpp"
#include "gluPlatform.hpp"
#include "tcuLnx.hpp"
#include "tcuFunctionLibrary.hpp"
#include "deUniquePtr.hpp"
#include "deMemory.h"

#include <sys/utsname.h>

using de::MovePtr;
using de::UniquePtr;

#if defined (DEQP_SUPPORT_X11)
#	include "tcuLnxX11.hpp"
#	if defined (DEQP_SUPPORT_XCB)
#		include "tcuLnxX11Xcb.hpp"
#	endif // DEQP_SUPPORT_XCB
#endif // DEQP_SUPPORT_X11

#if defined (DEQP_SUPPORT_WAYLAND)
#	include "tcuLnxWayland.hpp"
#endif // DEQP_SUPPORT_WAYLAND

namespace tcu
{
namespace lnx
{

#if defined (DEQP_SUPPORT_X11)

class VulkanWindowXlib : public vk::wsi::XlibWindowInterface
{
public:
	VulkanWindowXlib (MovePtr<x11::XlibWindow> window)
		: vk::wsi::XlibWindowInterface	(vk::pt::XlibWindow(window->getXID()))
		, m_window						(window)
	{
	}

	void resize (const UVec2& newSize)
	{
		m_window->setDimensions((int)newSize.x(), (int)newSize.y());
	}

private:
	UniquePtr<x11::XlibWindow>	m_window;
};

class VulkanDisplayXlib : public vk::wsi::XlibDisplayInterface
{
public:
	VulkanDisplayXlib (MovePtr<x11::DisplayBase> display)
		: vk::wsi::XlibDisplayInterface	(vk::pt::XlibDisplayPtr(((x11::XlibDisplay*)display.get())->getXDisplay()))
		, m_display	(display)
	{
	}

	vk::wsi::Window* createWindow (const Maybe<UVec2>& initialSize) const
	{
		x11::XlibDisplay*	instance	= (x11::XlibDisplay*)(m_display.get());
		const deUint32		height		= !initialSize ? (deUint32)DEFAULT_WINDOW_HEIGHT : initialSize->y();
		const deUint32		width		= !initialSize ? (deUint32)DEFAULT_WINDOW_WIDTH : initialSize->x();
		return new VulkanWindowXlib(MovePtr<x11::XlibWindow>(new x11::XlibWindow(*instance, (int)width, (int)height, instance->getVisual(0))));
	}

private:
	MovePtr<x11::DisplayBase> m_display;
};

#endif // DEQP_SUPPORT_X11

#if defined (DEQP_SUPPORT_XCB)

class VulkanWindowXcb : public vk::wsi::XcbWindowInterface
{
public:
	VulkanWindowXcb (MovePtr<x11::XcbWindow> window)
		: vk::wsi::XcbWindowInterface	(vk::pt::XcbWindow(window->getXID()))
		, m_window						(window)
	{
	}

	void resize (const UVec2& newSize)
	{
		m_window->setDimensions((int)newSize.x(), (int)newSize.y());
	}

private:
	UniquePtr<x11::XcbWindow>	m_window;
};

class VulkanDisplayXcb : public vk::wsi::XcbDisplayInterface
{
public:
	VulkanDisplayXcb (MovePtr<x11::DisplayBase> display)
		: vk::wsi::XcbDisplayInterface	(vk::pt::XcbConnectionPtr(((x11::XcbDisplay*)display.get())->getConnection()))
		, m_display		(display)
	{
	}

	vk::wsi::Window* createWindow (const Maybe<UVec2>& initialSize) const
	{
		x11::XcbDisplay*	instance	= (x11::XcbDisplay*)(m_display.get());
		const deUint32		height		= !initialSize ? (deUint32)DEFAULT_WINDOW_HEIGHT : initialSize->y();
		const deUint32		width		= !initialSize ? (deUint32)DEFAULT_WINDOW_WIDTH : initialSize->x();
		return new VulkanWindowXcb(MovePtr<x11::XcbWindow>(new x11::XcbWindow(*instance, (int)width, (int)height, DE_NULL)));
	}

private:
	MovePtr<x11::DisplayBase> m_display;
};
#endif // DEQP_SUPPORT_XCB

#if defined (DEQP_SUPPORT_WAYLAND)
class VulkanWindowWayland : public vk::wsi::WaylandWindowInterface
{
public:
	VulkanWindowWayland (MovePtr<wayland::Window> window)
		: vk::wsi::WaylandWindowInterface	(vk::pt::WaylandSurfacePtr(window->getSurface()))
		, m_window							(window)
	{
	}

	void resize (const UVec2& newSize)
	{
		m_window->setDimensions((int)newSize.x(), (int)newSize.y());
	}

private:
	UniquePtr<wayland::Window>	m_window;
};

class VulkanDisplayWayland : public vk::wsi::WaylandDisplayInterface
{
public:
	VulkanDisplayWayland (MovePtr<wayland::Display> display)
		: vk::wsi::WaylandDisplayInterface	(vk::pt::WaylandDisplayPtr(display->getDisplay()))
		, m_display		(display)
	{
	}

	vk::wsi::Window* createWindow (const Maybe<UVec2>& initialSize) const
	{
		const deUint32	height		= !initialSize ? (deUint32)DEFAULT_WINDOW_HEIGHT : initialSize->y();
		const deUint32	width		= !initialSize ? (deUint32)DEFAULT_WINDOW_WIDTH : initialSize->x();
		return new VulkanWindowWayland(MovePtr<wayland::Window>(new wayland::Window(*m_display, (int)width, (int)height)));
	}

private:
	MovePtr<wayland::Display> m_display;
};
#endif // DEQP_SUPPORT_WAYLAND

class VulkanLibrary : public vk::Library
{
public:
	VulkanLibrary (void)
		: m_library	("libvulkan.so.1")
		, m_driver	(m_library)
	{
	}

	const vk::PlatformInterface& getPlatformInterface (void) const
	{
		return m_driver;
	}

private:
	const DynamicFunctionLibrary	m_library;
	const vk::PlatformDriver		m_driver;
};

VulkanPlatform::VulkanPlatform (EventState& eventState)
	: m_eventState(eventState)
{
}

vk::wsi::Display* VulkanPlatform::createWsiDisplay (vk::wsi::Type wsiType) const
{
	switch(wsiType)
	{
#if defined (DEQP_SUPPORT_X11)
	case vk::wsi::TYPE_XLIB:
		return new VulkanDisplayXlib(MovePtr<x11::DisplayBase>(new x11::XlibDisplay(m_eventState,"")));
		break;
#endif // DEQP_SUPPORT_X11
#if defined (DEQP_SUPPORT_XCB)
	case vk::wsi::TYPE_XCB:
		return new VulkanDisplayXcb(MovePtr<x11::DisplayBase>(new x11::XcbDisplay(m_eventState,"")));
		break;
#endif // DEQP_SUPPORT_XCB
#if defined (DEQP_SUPPORT_WAYLAND)
	case vk::wsi::TYPE_WAYLAND:
		return new VulkanDisplayWayland(MovePtr<wayland::Display>(new wayland::Display(m_eventState, DE_NULL)));
		break;
#endif // DEQP_SUPPORT_WAYLAND

	default:
		TCU_THROW(NotSupportedError, "WSI type not supported");

	};
}

vk::Library* VulkanPlatform::createLibrary (void) const
{
	return new VulkanLibrary();
}

void VulkanPlatform::describePlatform (std::ostream& dst) const
{
	utsname		sysInfo;
	deMemset(&sysInfo, 0, sizeof(sysInfo));

	if (uname(&sysInfo) != 0)
		throw std::runtime_error("uname() failed");

	dst << "OS: " << sysInfo.sysname << " " << sysInfo.release << " " << sysInfo.version << "\n";
	dst << "CPU: " << sysInfo.machine << "\n";
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

} // linux
} // tcu

