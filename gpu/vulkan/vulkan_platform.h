// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is used so we only include platform specific headers when
// necessary. Otherwise some headers such as for X11 define extra macros which
// can conflict with other headers.

#ifndef GPU_VULKAN_VULKAN_H_
#define GPU_VULKAN_VULKAN_H_

#ifdef __cplusplus
extern "C" {
#endif

// This section below is taken from <vulkan/vulkan.h>, with an include
// of <X11/Xlib.h> replaced with an include of "ui/gfx/x/x11.h"
#ifdef VK_USE_PLATFORM_XLIB_KHR
#define VK_KHR_xlib_surface 1
#include "ui/gfx/x/x11.h"

#define VK_KHR_XLIB_SURFACE_SPEC_VERSION 6
#define VK_KHR_XLIB_SURFACE_EXTENSION_NAME "VK_KHR_xlib_surface"

typedef VkFlags VkXlibSurfaceCreateFlagsKHR;

typedef struct VkXlibSurfaceCreateInfoKHR {
  VkStructureType sType;
  const void* pNext;
  VkXlibSurfaceCreateFlagsKHR flags;
  Display* dpy;
  Window window;
} VkXlibSurfaceCreateInfoKHR;

typedef VkResult(VKAPI_PTR* PFN_vkCreateXlibSurfaceKHR)(
    VkInstance instance,
    const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSurfaceKHR* pSurface);
typedef VkBool32(VKAPI_PTR* PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    Display* dpy,
    VisualID visualID);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL
vkCreateXlibSurfaceKHR(VkInstance instance,
                       const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
                       const VkAllocationCallbacks* pAllocator,
                       VkSurfaceKHR* pSurface);

VKAPI_ATTR VkBool32 VKAPI_CALL
vkGetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                              uint32_t queueFamilyIndex,
                                              Display* dpy,
                                              VisualID visualID);
#endif
#endif /* VK_USE_PLATFORM_XLIB_KHR */

#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define VK_KHR_android_surface 1
#include <android/native_window.h>

#define VK_KHR_ANDROID_SURFACE_SPEC_VERSION 6
#define VK_KHR_ANDROID_SURFACE_EXTENSION_NAME "VK_KHR_android_surface"

typedef VkFlags VkAndroidSurfaceCreateFlagsKHR;

typedef struct VkAndroidSurfaceCreateInfoKHR {
  VkStructureType sType;
  const void* pNext;
  VkAndroidSurfaceCreateFlagsKHR flags;
  ANativeWindow* window;
} VkAndroidSurfaceCreateInfoKHR;

typedef VkResult(VKAPI_PTR* PFN_vkCreateAndroidSurfaceKHR)(
    VkInstance instance,
    const VkAndroidSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSurfaceKHR* pSurface);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL
vkCreateAndroidSurfaceKHR(VkInstance instance,
                          const VkAndroidSurfaceCreateInfoKHR* pCreateInfo,
                          const VkAllocationCallbacks* pAllocator,
                          VkSurfaceKHR* pSurface);
#endif
#endif /* VK_USE_PLATFORM_ANDROID_KHR */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // GPU_VULKAN_VULKAN_H_
