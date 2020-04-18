// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/gl_output_surface_mac.h"

#include "gpu/GLES2/gl2extchromium.h"

namespace viz {

GLOutputSurfaceMac::GLOutputSurfaceMac(
    scoped_refptr<VizProcessContextProvider> context_provider,
    gpu::SurfaceHandle surface_handle,
    SyntheticBeginFrameSource* synthetic_begin_frame_source,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    bool allow_overlays)
    : GLOutputSurfaceBufferQueue(context_provider,
                                 surface_handle,
                                 synthetic_begin_frame_source,
                                 gpu_memory_buffer_manager,
                                 GL_TEXTURE_RECTANGLE_ARB,
                                 GL_RGBA,
                                 gfx::BufferFormat::RGBA_8888),
      overlay_validator_(
          new CompositorOverlayCandidateValidatorMac(!allow_overlays)) {}

GLOutputSurfaceMac::~GLOutputSurfaceMac() {}

OverlayCandidateValidator* GLOutputSurfaceMac::GetOverlayCandidateValidator()
    const {
  return overlay_validator_.get();
}

}  // namespace viz
