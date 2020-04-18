// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_INIT_VULKAN_FACTORY_H_
#define GPU_VULKAN_INIT_VULKAN_FACTORY_H_

#include <memory>

#include "base/component_export.h"

namespace gpu {

class VulkanImplementation;

COMPONENT_EXPORT(VULKAN_INIT)
std::unique_ptr<VulkanImplementation> CreateVulkanImplementation();

}  // namespace gpu

#endif  // GPU_VULKAN_INIT_VULKAN_FACTORY_H_
