// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/gpu_output_surface_mac.h"

#include "components/viz/service/display/output_surface_client.h"
#include "components/viz/service/display/output_surface_frame.h"
#include "components/viz/service/display_embedder/compositor_overlay_candidate_validator.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"

namespace content {

GpuOutputSurfaceMac::GpuOutputSurfaceMac(
    scoped_refptr<ui::ContextProviderCommandBuffer> context,
    gpu::SurfaceHandle surface_handle,
    const UpdateVSyncParametersCallback& update_vsync_parameters_callback,
    std::unique_ptr<viz::CompositorOverlayCandidateValidator>
        overlay_candidate_validator,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager)
    : GpuSurfacelessBrowserCompositorOutputSurface(
          std::move(context),
          surface_handle,
          update_vsync_parameters_callback,
          std::move(overlay_candidate_validator),
          GL_TEXTURE_RECTANGLE_ARB,
          GL_RGBA,
          gfx::BufferFormat::RGBA_8888,
          gpu_memory_buffer_manager) {}

GpuOutputSurfaceMac::~GpuOutputSurfaceMac() {}

}  // namespace content
