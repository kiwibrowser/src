// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/gl_output_surface_ozone.h"

#include "ui/display/types/display_snapshot.h"

namespace viz {

GLOutputSurfaceOzone::GLOutputSurfaceOzone(
    scoped_refptr<VizProcessContextProvider> context_provider,
    gpu::SurfaceHandle surface_handle,
    SyntheticBeginFrameSource* synthetic_begin_frame_source,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    uint32_t target,
    uint32_t internal_format)
    : GLOutputSurfaceBufferQueue(context_provider,
                                 surface_handle,
                                 synthetic_begin_frame_source,
                                 gpu_memory_buffer_manager,
                                 target,
                                 internal_format,
                                 display::DisplaySnapshot::PrimaryFormat()) {}

GLOutputSurfaceOzone::~GLOutputSurfaceOzone() {}

}  // namespace viz
