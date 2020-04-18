// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_X_VULKAN_IMPLEMENTATION_X11_H_
#define GPU_VULKAN_X_VULKAN_IMPLEMENTATION_X11_H_

#include <memory>

#include "base/component_export.h"
#include "gpu/vulkan/vulkan_implementation.h"
#include "gpu/vulkan/vulkan_instance.h"
#include "ui/gfx/x/x11_types.h"

namespace gpu {

class COMPONENT_EXPORT(VULKAN_X11) VulkanImplementationX11
    : public VulkanImplementation {
 public:
  VulkanImplementationX11();
  explicit VulkanImplementationX11(XDisplay* x_display);
  ~VulkanImplementationX11() override;

  // VulkanImplementation:
  bool InitializeVulkanInstance() override;
  VkInstance GetVulkanInstance() override;
  std::unique_ptr<VulkanSurface> CreateViewSurface(
      gfx::AcceleratedWidget window) override;
  bool GetPhysicalDevicePresentationSupport(
      VkPhysicalDevice device,
      const std::vector<VkQueueFamilyProperties>& queue_family_properties,
      uint32_t queue_family_index) override;

 private:
  XDisplay* const x_display_;
  VulkanInstance vulkan_instance_;

  DISALLOW_COPY_AND_ASSIGN(VulkanImplementationX11);
};

}  // namespace gpu

#endif  // GPU_VULKAN_X_VULKAN_IMPLEMENTATION_X11_H_
