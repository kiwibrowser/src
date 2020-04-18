// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define VK_USE_PLATFORM_XLIB_KHR

#include "gpu/vulkan/x/vulkan_implementation_x11.h"

#include "gpu/vulkan/vulkan_instance.h"
#include "gpu/vulkan/vulkan_surface.h"
#include "ui/gfx/x/x11_types.h"

namespace gpu {

VulkanImplementationX11::VulkanImplementationX11()
    : VulkanImplementationX11(gfx::GetXDisplay()) {}

VulkanImplementationX11::VulkanImplementationX11(XDisplay* x_display)
    : x_display_(x_display) {}

VulkanImplementationX11::~VulkanImplementationX11() {}

bool VulkanImplementationX11::InitializeVulkanInstance() {
  std::vector<const char*> required_extensions;
  required_extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);

  if (!vulkan_instance_.Initialize(required_extensions)) {
    vulkan_instance_.Destroy();
    return false;
  }

  return true;
}

VkInstance VulkanImplementationX11::GetVulkanInstance() {
  return vulkan_instance_.vk_instance();
}

std::unique_ptr<VulkanSurface> VulkanImplementationX11::CreateViewSurface(
    gfx::AcceleratedWidget window) {
  VkSurfaceKHR surface;
  VkXlibSurfaceCreateInfoKHR surface_create_info = {};
  surface_create_info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  surface_create_info.dpy = x_display_;
  surface_create_info.window = window;
  VkResult result = vkCreateXlibSurfaceKHR(
      GetVulkanInstance(), &surface_create_info, nullptr, &surface);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateXlibSurfaceKHR() failed: " << result;
    return nullptr;
  }

  return std::make_unique<VulkanSurface>(GetVulkanInstance(), surface);
}

bool VulkanImplementationX11::GetPhysicalDevicePresentationSupport(
    VkPhysicalDevice device,
    const std::vector<VkQueueFamilyProperties>& queue_family_properties,
    uint32_t queue_family_index) {
  return vkGetPhysicalDeviceXlibPresentationSupportKHR(
      device, queue_family_index, x_display_,
      XVisualIDFromVisual(
          DefaultVisual(x_display_, DefaultScreen(x_display_))));
}

}  // namespace gpu
