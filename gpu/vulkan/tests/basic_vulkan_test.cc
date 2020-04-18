// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/tests/basic_vulkan_test.h"

#include "gpu/vulkan/init/vulkan_factory.h"
#include "gpu/vulkan/tests/native_window.h"
#include "gpu/vulkan/vulkan_surface.h"
#include "ui/gfx/geometry/rect.h"

namespace gpu {

BasicVulkanTest::BasicVulkanTest() {}

BasicVulkanTest::~BasicVulkanTest() {}

void BasicVulkanTest::SetUp() {
  const gfx::Rect kDefaultBounds(10, 10, 100, 100);
  window_ = CreateNativeWindow(kDefaultBounds);
  vulkan_implementation_ = CreateVulkanImplementation();
  ASSERT_TRUE(vulkan_implementation_);
  ASSERT_TRUE(vulkan_implementation_->InitializeVulkanInstance());
  device_queue_ = gpu::CreateVulkanDeviceQueue(
      vulkan_implementation_.get(),
      VulkanDeviceQueue::GRAPHICS_QUEUE_FLAG |
          VulkanDeviceQueue::PRESENTATION_SUPPORT_QUEUE_FLAG);
  ASSERT_TRUE(device_queue_);
}

void BasicVulkanTest::TearDown() {
  DestroyNativeWindow(window_);
  window_ = gfx::kNullAcceleratedWidget;
  device_queue_->Destroy();
  vulkan_implementation_.reset();
}

std::unique_ptr<VulkanSurface> BasicVulkanTest::CreateViewSurface(
    gfx::AcceleratedWidget window) {
  return vulkan_implementation_->CreateViewSurface(window);
}

}  // namespace gpu
