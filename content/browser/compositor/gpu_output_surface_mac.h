// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_GPU_OUTPUT_SURFACE_MAC_H_
#define CONTENT_BROWSER_COMPOSITOR_GPU_OUTPUT_SURFACE_MAC_H_

#include "content/browser/compositor/gpu_surfaceless_browser_compositor_output_surface.h"

#include "ui/gfx/native_widget_types.h"

namespace content {

class GpuOutputSurfaceMac
    : public GpuSurfacelessBrowserCompositorOutputSurface {
 public:
  GpuOutputSurfaceMac(
      scoped_refptr<ui::ContextProviderCommandBuffer> context,
      gpu::SurfaceHandle surface_handle,
      const UpdateVSyncParametersCallback& update_vsync_parameters_callback,
      std::unique_ptr<viz::CompositorOverlayCandidateValidator>
          overlay_candidate_validator,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager);
  ~GpuOutputSurfaceMac() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GpuOutputSurfaceMac);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_GPU_OUTPUT_SURFACE_MAC_H_
