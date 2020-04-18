// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_GPU_VULKAN_CONTEXT_PROVIDER_H_
#define COMPONENTS_VIZ_COMMON_GPU_VULKAN_CONTEXT_PROVIDER_H_

#include "base/memory/ref_counted.h"
#include "components/viz/common/viz_common_export.h"

class GrContext;

namespace gpu {
class VulkanDeviceQueue;
class VulkanImplementation;
}

namespace viz {

// The VulkanContextProvider groups sharing of vulkan objects synchronously.
class VIZ_COMMON_EXPORT VulkanContextProvider
    : public base::RefCountedThreadSafe<VulkanContextProvider> {
 public:
  virtual gpu::VulkanImplementation* GetVulkanImplementation() = 0;
  virtual gpu::VulkanDeviceQueue* GetDeviceQueue() = 0;
  virtual GrContext* GetGrContext() = 0;

 protected:
  friend class base::RefCountedThreadSafe<VulkanContextProvider>;
  virtual ~VulkanContextProvider() {}
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_GPU_VULKAN_CONTEXT_PROVIDER_H_
