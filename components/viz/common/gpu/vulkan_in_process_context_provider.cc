// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gpu/vulkan_in_process_context_provider.h"
#include "gpu/vulkan/buildflags.h"
#include "gpu/vulkan/init/vulkan_factory.h"
#include "gpu/vulkan/vulkan_device_queue.h"
#include "gpu/vulkan/vulkan_implementation.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/vk/GrVkInterface.h"

namespace viz {

scoped_refptr<VulkanInProcessContextProvider>
VulkanInProcessContextProvider::Create(
    gpu::VulkanImplementation* vulkan_implementation) {
  scoped_refptr<VulkanInProcessContextProvider> context_provider(
      new VulkanInProcessContextProvider(vulkan_implementation));
  if (!context_provider->Initialize())
    return nullptr;
  return context_provider;
}

GrVkInterface::GetProc make_unified_getter(
    const GrVkInterface::GetInstanceProc& iproc,
    const GrVkInterface::GetDeviceProc& dproc) {
  return [&iproc, &dproc](const char* proc_name, VkInstance instance,
                          VkDevice device) {
    if (device != VK_NULL_HANDLE) {
      return dproc(device, proc_name);
    }
    return iproc(instance, proc_name);
  };
}

bool VulkanInProcessContextProvider::Initialize() {
  DCHECK(!device_queue_);
  std::unique_ptr<gpu::VulkanDeviceQueue> device_queue =
      gpu::CreateVulkanDeviceQueue(
          vulkan_implementation_,
          gpu::VulkanDeviceQueue::GRAPHICS_QUEUE_FLAG |
              gpu::VulkanDeviceQueue::PRESENTATION_SUPPORT_QUEUE_FLAG);
  if (!device_queue) {
    return false;
  }

  device_queue_ = std::move(device_queue);
  const uint32_t feature_flags = kGeometryShader_GrVkFeatureFlag |
                                 kDualSrcBlend_GrVkFeatureFlag |
                                 kSampleRateShading_GrVkFeatureFlag;
  const uint32_t extension_flags =
      kEXT_debug_report_GrVkExtensionFlag | kKHR_surface_GrVkExtensionFlag |
      kKHR_swapchain_GrVkExtensionFlag | kKHR_xcb_surface_GrVkExtensionFlag;
  GrVkBackendContext* backend_context = new GrVkBackendContext();
  backend_context->fInstance = device_queue_->GetVulkanInstance();
  backend_context->fPhysicalDevice = device_queue_->GetVulkanPhysicalDevice();
  backend_context->fDevice = device_queue_->GetVulkanDevice();
  backend_context->fQueue = device_queue_->GetVulkanQueue();
  backend_context->fGraphicsQueueIndex = device_queue_->GetVulkanQueueIndex();
  backend_context->fMinAPIVersion = VK_MAKE_VERSION(1, 0, 8);
  backend_context->fExtensions = extension_flags;
  backend_context->fFeatures = feature_flags;

  auto interface = sk_make_sp<GrVkInterface>(
      make_unified_getter(vkGetInstanceProcAddr, vkGetDeviceProcAddr),
      backend_context->fInstance, backend_context->fDevice,
      backend_context->fExtensions);
  backend_context->fInterface.reset(interface.release());
  backend_context->fOwnsInstanceAndDevice = false;
  backend_context_.reset(backend_context);
  gr_context_ = GrContext::MakeVulkan(backend_context_);
  return true;
}

void VulkanInProcessContextProvider::Destroy() {
  if (gr_context_)
    gr_context_.reset();

  if (device_queue_) {
    device_queue_->Destroy();
    device_queue_.reset();
    backend_context_.reset();
  }
}

GrContext* VulkanInProcessContextProvider::GetGrContext() {
  return gr_context_.get();
}

gpu::VulkanImplementation*
VulkanInProcessContextProvider::GetVulkanImplementation() {
  return vulkan_implementation_;
}

gpu::VulkanDeviceQueue* VulkanInProcessContextProvider::GetDeviceQueue() {
  return device_queue_.get();
}

VulkanInProcessContextProvider::VulkanInProcessContextProvider(
    gpu::VulkanImplementation* vulkan_implementation)
    : vulkan_implementation_(vulkan_implementation) {}

VulkanInProcessContextProvider::~VulkanInProcessContextProvider() {
  Destroy();
}

}  // namespace viz
