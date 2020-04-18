// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_GL_OUTPUT_SURFACE_OZONE_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_GL_OUTPUT_SURFACE_OZONE_H_

#include "components/viz/service/display_embedder/gl_output_surface_buffer_queue.h"

namespace viz {

class GLOutputSurfaceOzone : public GLOutputSurfaceBufferQueue {
 public:
  GLOutputSurfaceOzone(
      scoped_refptr<VizProcessContextProvider> context_provider,
      gpu::SurfaceHandle surface_handle,
      SyntheticBeginFrameSource* synthetic_begin_frame_source,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      uint32_t target,
      uint32_t internal_format);
  ~GLOutputSurfaceOzone() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GLOutputSurfaceOzone);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_GL_OUTPUT_SURFACE_OZONE_H_
