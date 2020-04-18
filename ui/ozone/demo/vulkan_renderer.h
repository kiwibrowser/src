// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_DEMO_VULKAN_RENDERER_H_
#define UI_OZONE_DEMO_VULKAN_RENDERER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "ui/gfx/swap_result.h"
#include "ui/ozone/demo/renderer_base.h"

namespace gpu {
class VulkanDeviceQueue;
class VulkanImplementation;
class VulkanRenderPass;
class VulkanSurface;
}  // namespace gpu

namespace ui {

class VulkanRenderer : public RendererBase {
 public:
  VulkanRenderer(gpu::VulkanImplementation* vulkan_instance,
                 gfx::AcceleratedWidget widget,
                 const gfx::Size& size);
  ~VulkanRenderer() override;

  // Renderer:
  bool Initialize() override;

 private:
  void RenderFrame();
  void PostRenderFrameTask();

  gpu::VulkanImplementation* const vulkan_implementation_;
  std::unique_ptr<gpu::VulkanDeviceQueue> device_queue_;
  std::unique_ptr<gpu::VulkanSurface> surface_;
  std::unique_ptr<gpu::VulkanRenderPass> render_pass_;

  base::WeakPtrFactory<VulkanRenderer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VulkanRenderer);
};

}  // namespace ui

#endif  // UI_OZONE_DEMO_VULKAN_RENDERER_H_
