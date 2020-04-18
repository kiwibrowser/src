/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 * \brief Android EGL platform.
 *//*--------------------------------------------------------------------*/

#include "tcuAndroidPlatform.hpp"
#include "tcuAndroidUtil.hpp"
#include "gluRenderContext.hpp"
#include "egluNativeDisplay.hpp"
#include "egluNativeWindow.hpp"
#include "egluGLContextFactory.hpp"
#include "egluUtil.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"
#include "tcuFunctionLibrary.hpp"
#include "vkWsiPlatform.hpp"

// Assume no call translation is needed
#include <android/native_window.h>
struct egl_native_pixmap_t;
DE_STATIC_ASSERT(sizeof(eglw::EGLNativeDisplayType) == sizeof(void*));
DE_STATIC_ASSERT(sizeof(eglw::EGLNativePixmapType) == sizeof(struct egl_native_pixmap_t*));
DE_STATIC_ASSERT(sizeof(eglw::EGLNativeWindowType) == sizeof(ANativeWindow*));

namespace tcu
{
namespace Android
{

using namespace eglw;

static const eglu::NativeDisplay::Capability	DISPLAY_CAPABILITIES	= eglu::NativeDisplay::CAPABILITY_GET_DISPLAY_LEGACY;
static const eglu::NativeWindow::Capability		WINDOW_CAPABILITIES		= (eglu::NativeWindow::Capability)(eglu::NativeWindow::CAPABILITY_CREATE_SURFACE_LEGACY |
																										   eglu::NativeWindow::CAPABILITY_SET_SURFACE_SIZE |
																										   eglu::NativeWindow::CAPABILITY_GET_SCREEN_SIZE);

class NativeDisplay : public eglu::NativeDisplay
{
public:
									NativeDisplay			(void) : eglu::NativeDisplay(DISPLAY_CAPABILITIES), m_library("libEGL.so") {}
	virtual							~NativeDisplay			(void) {}

	virtual EGLNativeDisplayType	getLegacyNative			(void)			{ return EGL_DEFAULT_DISPLAY;	}
	virtual const eglw::Library&	getLibrary				(void) const	{ return m_library;				}

private:
	eglw::DefaultLibrary			m_library;
};

class NativeDisplayFactory : public eglu::NativeDisplayFactory
{
public:
									NativeDisplayFactory	(WindowRegistry& windowRegistry);
									~NativeDisplayFactory	(void) {}

	virtual eglu::NativeDisplay*	createDisplay			(const EGLAttrib* attribList) const;
};

class NativeWindow : public eglu::NativeWindow
{
public:
									NativeWindow			(Window* window, int width, int height, int32_t format);
	virtual							~NativeWindow			(void);

	virtual EGLNativeWindowType		getLegacyNative			(void)			{ return m_window->getNativeWindow();	}
	IVec2							getScreenSize			(void) const	{ return m_window->getSize();			}

	void							setSurfaceSize			(IVec2 size);

	virtual void					processEvents			(void);

private:
	Window*							m_window;
	int32_t							m_format;
};

class NativeWindowFactory : public eglu::NativeWindowFactory
{
public:
									NativeWindowFactory		(WindowRegistry& windowRegistry);
									~NativeWindowFactory	(void);

	virtual eglu::NativeWindow*		createWindow			(eglu::NativeDisplay* nativeDisplay, const eglu::WindowParams& params) const;
	virtual eglu::NativeWindow*		createWindow			(eglu::NativeDisplay* nativeDisplay, EGLDisplay display, EGLConfig config, const EGLAttrib* attribList, const eglu::WindowParams& params) const;

private:
	virtual eglu::NativeWindow*		createWindow			(const eglu::WindowParams& params, int32_t format) const;

	WindowRegistry&					m_windowRegistry;
};

// NativeWindow

NativeWindow::NativeWindow (Window* window, int width, int height, int32_t format)
	: eglu::NativeWindow	(WINDOW_CAPABILITIES)
	, m_window				(window)
	, m_format				(format)
{
	// Set up buffers.
	setSurfaceSize(IVec2(width, height));
}

NativeWindow::~NativeWindow (void)
{
	m_window->release();
}

void NativeWindow::processEvents (void)
{
	if (m_window->isPendingDestroy())
		throw eglu::WindowDestroyedError("Window has been destroyed");
}

void NativeWindow::setSurfaceSize (tcu::IVec2 size)
{
	m_window->setBuffersGeometry(size.x() != eglu::WindowParams::SIZE_DONT_CARE ? size.x() : 0,
								 size.y() != eglu::WindowParams::SIZE_DONT_CARE ? size.y() : 0,
								 m_format);
}

// NativeWindowFactory

NativeWindowFactory::NativeWindowFactory (WindowRegistry& windowRegistry)
	: eglu::NativeWindowFactory	("default", "Default display", WINDOW_CAPABILITIES)
	, m_windowRegistry			(windowRegistry)
{
}

NativeWindowFactory::~NativeWindowFactory (void)
{
}

eglu::NativeWindow* NativeWindowFactory::createWindow (eglu::NativeDisplay* nativeDisplay, const eglu::WindowParams& params) const
{
	DE_UNREF(nativeDisplay);
	return createWindow(params, WINDOW_FORMAT_RGBA_8888);
}

eglu::NativeWindow* NativeWindowFactory::createWindow (eglu::NativeDisplay* nativeDisplay, EGLDisplay display, EGLConfig config, const EGLAttrib* attribList, const eglu::WindowParams& params) const
{
	const int32_t format = (int32_t)eglu::getConfigAttribInt(nativeDisplay->getLibrary(), display, config, EGL_NATIVE_VISUAL_ID);
	DE_UNREF(nativeDisplay && attribList);
	return createWindow(params, format);
}

eglu::NativeWindow* NativeWindowFactory::createWindow (const eglu::WindowParams& params, int32_t format) const
{
	Window* window = m_windowRegistry.tryAcquireWindow();

	if (!window)
		throw ResourceError("Native window is not available", DE_NULL, __FILE__, __LINE__);

	return new NativeWindow(window, params.width, params.height, format);
}

// NativeDisplayFactory

NativeDisplayFactory::NativeDisplayFactory (WindowRegistry& windowRegistry)
	: eglu::NativeDisplayFactory("default", "Default display", DISPLAY_CAPABILITIES)
{
	m_nativeWindowRegistry.registerFactory(new NativeWindowFactory(windowRegistry));
}

eglu::NativeDisplay* NativeDisplayFactory::createDisplay (const EGLAttrib* attribList) const
{
	DE_UNREF(attribList);
	return new NativeDisplay();
}

// Vulkan

class VulkanLibrary : public vk::Library
{
public:
	VulkanLibrary (void)
		: m_library	("libvulkan.so")
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

DE_STATIC_ASSERT(sizeof(vk::pt::AndroidNativeWindowPtr) == sizeof(ANativeWindow*));

class VulkanWindow : public vk::wsi::AndroidWindowInterface
{
public:
	VulkanWindow (tcu::Android::Window& window)
		: vk::wsi::AndroidWindowInterface	(vk::pt::AndroidNativeWindowPtr(window.getNativeWindow()))
		, m_window							(window)
	{
	}

	~VulkanWindow (void)
	{
		m_window.release();
	}

private:
	tcu::Android::Window&	m_window;
};

class VulkanDisplay : public vk::wsi::Display
{
public:
	VulkanDisplay (WindowRegistry& windowRegistry)
		: m_windowRegistry(windowRegistry)
	{
	}

	vk::wsi::Window* createWindow (const Maybe<UVec2>& initialSize) const
	{
		Window* const	window	= m_windowRegistry.tryAcquireWindow();

		if (window)
		{
			try
			{
				if (initialSize)
					window->setBuffersGeometry((int)initialSize->x(), (int)initialSize->y(), WINDOW_FORMAT_RGBA_8888);

				return new VulkanWindow(*window);
			}
			catch (...)
			{
				window->release();
				throw;
			}
		}
		else
			TCU_THROW(ResourceError, "Native window is not available");
	}

private:
	WindowRegistry&		m_windowRegistry;
};

static size_t getTotalSystemMemory (ANativeActivity* activity)
{
	const size_t	MiB		= (size_t)(1<<20);

	try
	{
		const size_t	cddRequiredSize	= getCDDRequiredSystemMemory(activity);

		print("Device has at least %.2f MiB total system memory per Android CDD\n", double(cddRequiredSize) / double(MiB));

		return cddRequiredSize;
	}
	catch (const std::exception& e)
	{
		// Use relatively high fallback size to encourage CDD-compliant behavior
		const size_t	fallbackSize	= (sizeof(void*) == sizeof(deUint64)) ? 2048*MiB : 1024*MiB;

		print("WARNING: Failed to determine system memory size required by CDD: %s\n", e.what());
		print("WARNING: Using fall-back size of %.2f MiB\n", double(fallbackSize) / double(MiB));

		return fallbackSize;
	}
}

// Platform

Platform::Platform (NativeActivity& activity)
	: m_activity			(activity)
	, m_totalSystemMemory	(getTotalSystemMemory(activity.getNativeActivity()))
{
	m_nativeDisplayFactoryRegistry.registerFactory(new NativeDisplayFactory(m_windowRegistry));
	m_contextFactoryRegistry.registerFactory(new eglu::GLContextFactory(m_nativeDisplayFactoryRegistry));
}

Platform::~Platform (void)
{
}

bool Platform::processEvents (void)
{
	m_windowRegistry.garbageCollect();
	return true;
}

vk::Library* Platform::createLibrary (void) const
{
	return new VulkanLibrary();
}

void Platform::describePlatform (std::ostream& dst) const
{
	tcu::Android::describePlatform(m_activity.getNativeActivity(), dst);
}

void Platform::getMemoryLimits (vk::PlatformMemoryLimits& limits) const
{
	// Worst-case estimates
	const size_t	MiB				= (size_t)(1<<20);
	const size_t	baseMemUsage	= 400*MiB;
	const double	safeUsageRatio	= 0.25;

	limits.totalSystemMemory					= de::max((size_t)(double(deInt64(m_totalSystemMemory)-deInt64(baseMemUsage)) * safeUsageRatio), 16*MiB);

	// Assume UMA architecture
	limits.totalDeviceLocalMemory				= 0;

	// Reasonable worst-case estimates
	limits.deviceMemoryAllocationGranularity	= 64*1024;
	limits.devicePageSize						= 4096;
	limits.devicePageTableEntrySize				= 8;
	limits.devicePageTableHierarchyLevels		= 3;
}

vk::wsi::Display* Platform::createWsiDisplay (vk::wsi::Type wsiType) const
{
	if (wsiType == vk::wsi::TYPE_ANDROID)
		return new VulkanDisplay(const_cast<WindowRegistry&>(m_windowRegistry));
	else
		TCU_THROW(NotSupportedError, "WSI type not supported on Android");
}

} // Android
} // tcu
