// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_implementation.h"

#include "base/bind.h"
#include "gpu/vulkan/vulkan_device_queue.h"

namespace gpu {

VulkanImplementation::VulkanImplementation() {}

VulkanImplementation::~VulkanImplementation() {}

std::unique_ptr<VulkanDeviceQueue> CreateVulkanDeviceQueue(
    VulkanImplementation* vulkan_implementation,
    uint32_t option) {
  auto device_queue = std::make_unique<VulkanDeviceQueue>(
      vulkan_implementation->GetVulkanInstance());
  auto callback = base::BindRepeating(
      &VulkanImplementation::GetPhysicalDevicePresentationSupport,
      base::Unretained(vulkan_implementation));
  if (!device_queue->Initialize(option, callback)) {
    device_queue->Destroy();
    return nullptr;
  }

  return device_queue;
}

}  // namespace gpu
