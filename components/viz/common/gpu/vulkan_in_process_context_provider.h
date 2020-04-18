// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_GPU_VULKAN_IN_PROCESS_CONTEXT_PROVIDER_H_
#define COMPONENTS_VIZ_COMMON_GPU_VULKAN_IN_PROCESS_CONTEXT_PROVIDER_H_

#include <memory>

#include "components/viz/common/gpu/vulkan_context_provider.h"
#include "components/viz/common/viz_common_export.h"
#include "gpu/vulkan/buildflags.h"
#if BUILDFLAG(ENABLE_VULKAN)
#include "third_party/skia/include/gpu/vk/GrVkBackendContext.h"
#endif

namespace gpu {
class VulkanImplementation;
class VulkanDeviceQueue;
}

namespace viz {

class VIZ_COMMON_EXPORT VulkanInProcessContextProvider
    : public VulkanContextProvider {
 public:
  static scoped_refptr<VulkanInProcessContextProvider> Create(
      gpu::VulkanImplementation* vulkan_implementation);

  bool Initialize();
  void Destroy();
  GrContext* GetGrContext() override;

  // VulkanContextProvider implementation
  gpu::VulkanImplementation* GetVulkanImplementation() override;
  gpu::VulkanDeviceQueue* GetDeviceQueue() override;

 protected:
  explicit VulkanInProcessContextProvider(
      gpu::VulkanImplementation* vulkan_implementation);
  ~VulkanInProcessContextProvider() override;

 private:
#if BUILDFLAG(ENABLE_VULKAN)
  sk_sp<GrContext> gr_context_;
  gpu::VulkanImplementation* vulkan_implementation_;
  std::unique_ptr<gpu::VulkanDeviceQueue> device_queue_;
  sk_sp<GrVkBackendContext> backend_context_;
#endif

  DISALLOW_COPY_AND_ASSIGN(VulkanInProcessContextProvider);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_GPU_VULKAN_IN_PROCESS_CONTEXT_PROVIDER_H_
