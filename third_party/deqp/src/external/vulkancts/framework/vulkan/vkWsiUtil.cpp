/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 * \brief Windowing System Integration (WSI) Utilities.
 *//*--------------------------------------------------------------------*/

#include "vkWsiUtil.hpp"
#include "deArrayUtil.hpp"
#include "deMemory.h"

#include <limits>

namespace vk
{
namespace wsi
{

//! Get canonical WSI name that should be used for example in test case and group names.
const char* getName (Type wsiType)
{
	static const char* const s_names[] =
	{
		"xlib",
		"xcb",
		"wayland",
		"mir",
		"android",
		"win32",
	};
	return de::getSizedArrayElement<TYPE_LAST>(s_names, wsiType);
}

const char* getExtensionName (Type wsiType)
{
	static const char* const s_extNames[] =
	{
		"VK_KHR_xlib_surface",
		"VK_KHR_xcb_surface",
		"VK_KHR_wayland_surface",
		"VK_KHR_mir_surface",
		"VK_KHR_android_surface",
		"VK_KHR_win32_surface",
	};
	return de::getSizedArrayElement<TYPE_LAST>(s_extNames, wsiType);
}

const PlatformProperties& getPlatformProperties (Type wsiType)
{
	// \note These are declared here (rather than queried through vk::Platform for example)
	//		 on purpose. The behavior of a platform is partly defined by the platform spec,
	//		 and partly by WSI extensions, and platform ports should not need to override
	//		 that definition.

	const deUint32	noDisplayLimit	= std::numeric_limits<deUint32>::max();
	const deUint32	noWindowLimit	= std::numeric_limits<deUint32>::max();

	static const PlatformProperties s_properties[] =
	{
		// VK_KHR_xlib_surface
		{
			PlatformProperties::FEATURE_INITIAL_WINDOW_SIZE|PlatformProperties::FEATURE_RESIZE_WINDOW,
			PlatformProperties::SWAPCHAIN_EXTENT_MUST_MATCH_WINDOW_SIZE,
			noDisplayLimit,
			noWindowLimit,
		},
		// VK_KHR_xcb_surface
		{
			PlatformProperties::FEATURE_INITIAL_WINDOW_SIZE|PlatformProperties::FEATURE_RESIZE_WINDOW,
			PlatformProperties::SWAPCHAIN_EXTENT_MUST_MATCH_WINDOW_SIZE,
			noDisplayLimit,
			noWindowLimit,
		},
		// VK_KHR_wayland_surface
		{
			0u,
			PlatformProperties::SWAPCHAIN_EXTENT_SETS_WINDOW_SIZE,
			noDisplayLimit,
			noWindowLimit,
		},
		// VK_KHR_mir_surface
		{
			PlatformProperties::FEATURE_INITIAL_WINDOW_SIZE|PlatformProperties::FEATURE_RESIZE_WINDOW,
			PlatformProperties::SWAPCHAIN_EXTENT_SCALED_TO_WINDOW_SIZE,
			noDisplayLimit,
			noWindowLimit,
		},
		// VK_KHR_android_surface
		{
			PlatformProperties::FEATURE_INITIAL_WINDOW_SIZE,
			PlatformProperties::SWAPCHAIN_EXTENT_SCALED_TO_WINDOW_SIZE,
			1u,
			1u, // Only one window available
		},
		// VK_KHR_win32_surface
		{
			PlatformProperties::FEATURE_INITIAL_WINDOW_SIZE|PlatformProperties::FEATURE_RESIZE_WINDOW,
			PlatformProperties::SWAPCHAIN_EXTENT_MUST_MATCH_WINDOW_SIZE,
			noDisplayLimit,
			noWindowLimit,
		},
	};

	return de::getSizedArrayElement<TYPE_LAST>(s_properties, wsiType);
}

VkResult createSurface (const InstanceInterface&		vki,
						VkInstance						instance,
						Type							wsiType,
						const Display&					nativeDisplay,
						const Window&					nativeWindow,
						const VkAllocationCallbacks*	pAllocator,
						VkSurfaceKHR*					pSurface)
{
	// Update this function if you add more WSI implementations
	DE_STATIC_ASSERT(TYPE_LAST == 6);

	switch (wsiType)
	{
		case TYPE_XLIB:
		{
			const XlibDisplayInterface&			xlibDisplay		= dynamic_cast<const XlibDisplayInterface&>(nativeDisplay);
			const XlibWindowInterface&			xlibWindow		= dynamic_cast<const XlibWindowInterface&>(nativeWindow);
			const VkXlibSurfaceCreateInfoKHR	createInfo		=
			{
				VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
				DE_NULL,
				(VkXlibSurfaceCreateFlagsKHR)0,
				xlibDisplay.getNative(),
				xlibWindow.getNative()
			};

			return vki.createXlibSurfaceKHR(instance, &createInfo, pAllocator, pSurface);
		}

		case TYPE_XCB:
		{
			const XcbDisplayInterface&			xcbDisplay		= dynamic_cast<const XcbDisplayInterface&>(nativeDisplay);
			const XcbWindowInterface&			xcbWindow		= dynamic_cast<const XcbWindowInterface&>(nativeWindow);
			const VkXcbSurfaceCreateInfoKHR		createInfo		=
			{
				VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
				DE_NULL,
				(VkXcbSurfaceCreateFlagsKHR)0,
				xcbDisplay.getNative(),
				xcbWindow.getNative()
			};

			return vki.createXcbSurfaceKHR(instance, &createInfo, pAllocator, pSurface);
		}

		case TYPE_WAYLAND:
		{
			const WaylandDisplayInterface&		waylandDisplay	= dynamic_cast<const WaylandDisplayInterface&>(nativeDisplay);
			const WaylandWindowInterface&		waylandWindow	= dynamic_cast<const WaylandWindowInterface&>(nativeWindow);
			const VkWaylandSurfaceCreateInfoKHR	createInfo		=
			{
				VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
				DE_NULL,
				(VkWaylandSurfaceCreateFlagsKHR)0,
				waylandDisplay.getNative(),
				waylandWindow.getNative()
			};

			return vki.createWaylandSurfaceKHR(instance, &createInfo, pAllocator, pSurface);
		}

		case TYPE_MIR:
		{
			const MirDisplayInterface&			mirDisplay		= dynamic_cast<const MirDisplayInterface&>(nativeDisplay);
			const MirWindowInterface&			mirWindow		= dynamic_cast<const MirWindowInterface&>(nativeWindow);
			const VkMirSurfaceCreateInfoKHR		createInfo		=
			{
				VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR,
				DE_NULL,
				(VkXcbSurfaceCreateFlagsKHR)0,
				mirDisplay.getNative(),
				mirWindow.getNative()
			};

			return vki.createMirSurfaceKHR(instance, &createInfo, pAllocator, pSurface);
		}

		case TYPE_ANDROID:
		{
			const AndroidWindowInterface&		androidWindow	= dynamic_cast<const AndroidWindowInterface&>(nativeWindow);
			const VkAndroidSurfaceCreateInfoKHR	createInfo		=
			{
				VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
				DE_NULL,
				(VkAndroidSurfaceCreateFlagsKHR)0,
				androidWindow.getNative()
			};

			return vki.createAndroidSurfaceKHR(instance, &createInfo, pAllocator, pSurface);
		}

		case TYPE_WIN32:
		{
			const Win32DisplayInterface&		win32Display	= dynamic_cast<const Win32DisplayInterface&>(nativeDisplay);
			const Win32WindowInterface&			win32Window		= dynamic_cast<const Win32WindowInterface&>(nativeWindow);
			const VkWin32SurfaceCreateInfoKHR	createInfo		=
			{
				VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
				DE_NULL,
				(VkWin32SurfaceCreateFlagsKHR)0,
				win32Display.getNative(),
				win32Window.getNative()
			};

			return vki.createWin32SurfaceKHR(instance, &createInfo, pAllocator, pSurface);
		}

		default:
			DE_FATAL("Unknown WSI type");
			return VK_ERROR_SURFACE_LOST_KHR;
	}
}

Move<VkSurfaceKHR> createSurface (const InstanceInterface&		vki,
								  VkInstance					instance,
								  Type							wsiType,
								  const Display&				nativeDisplay,
								  const Window&					nativeWindow,
								  const VkAllocationCallbacks*	pAllocator)
{
	VkSurfaceKHR object = 0;
	VK_CHECK(createSurface(vki, instance, wsiType, nativeDisplay, nativeWindow, pAllocator, &object));
	return Move<VkSurfaceKHR>(check<VkSurfaceKHR>(object), Deleter<VkSurfaceKHR>(vki, instance, pAllocator));
}

VkBool32 getPhysicalDeviceSurfaceSupport (const InstanceInterface&	vki,
										  VkPhysicalDevice			physicalDevice,
										  deUint32					queueFamilyIndex,
										  VkSurfaceKHR				surface)
{
	VkBool32 result = 0;

	VK_CHECK(vki.getPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &result));

	return result;
}

VkSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilities (const InstanceInterface&		vki,
															   VkPhysicalDevice				physicalDevice,
															   VkSurfaceKHR					surface)
{
	VkSurfaceCapabilitiesKHR capabilities;

	deMemset(&capabilities, 0, sizeof(capabilities));

	VK_CHECK(vki.getPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities));

	return capabilities;
}

std::vector<VkSurfaceFormatKHR> getPhysicalDeviceSurfaceFormats (const InstanceInterface&		vki,
																 VkPhysicalDevice				physicalDevice,
																 VkSurfaceKHR					surface)
{
	deUint32	numFormats	= 0;

	VK_CHECK(vki.getPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numFormats, DE_NULL));

	if (numFormats > 0)
	{
		std::vector<VkSurfaceFormatKHR>	formats	(numFormats);

		VK_CHECK(vki.getPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numFormats, &formats[0]));

		return formats;
	}
	else
		return std::vector<VkSurfaceFormatKHR>();
}

std::vector<VkPresentModeKHR> getPhysicalDeviceSurfacePresentModes (const InstanceInterface&		vki,
																	VkPhysicalDevice				physicalDevice,
																	VkSurfaceKHR					surface)
{
	deUint32	numModes	= 0;

	VK_CHECK(vki.getPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &numModes, DE_NULL));

	if (numModes > 0)
	{
		std::vector<VkPresentModeKHR>	modes	(numModes);

		VK_CHECK(vki.getPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &numModes, &modes[0]));

		return modes;
	}
	else
		return std::vector<VkPresentModeKHR>();
}

std::vector<VkImage> getSwapchainImages (const DeviceInterface&			vkd,
										 VkDevice						device,
										 VkSwapchainKHR					swapchain)
{
	deUint32	numImages	= 0;

	VK_CHECK(vkd.getSwapchainImagesKHR(device, swapchain, &numImages, DE_NULL));

	if (numImages > 0)
	{
		std::vector<VkImage>	images	(numImages);

		VK_CHECK(vkd.getSwapchainImagesKHR(device, swapchain, &numImages, &images[0]));

		return images;
	}
	else
		return std::vector<VkImage>();
}

} // wsi
} // vk
