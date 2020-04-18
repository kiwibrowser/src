// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_GL_OUTPUT_SURFACE_MAC_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_GL_OUTPUT_SURFACE_MAC_H_

#include "components/viz/service/display_embedder/compositor_overlay_candidate_validator_mac.h"
#include "components/viz/service/display_embedder/gl_output_surface_buffer_queue.h"

namespace viz {

class GLOutputSurfaceMac : public GLOutputSurfaceBufferQueue {
 public:
  GLOutputSurfaceMac(scoped_refptr<VizProcessContextProvider> context_provider,
                     gpu::SurfaceHandle surface_handle,
                     SyntheticBeginFrameSource* synthetic_begin_frame_source,
                     gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
                     bool allow_overlays);
  ~GLOutputSurfaceMac() override;

 private:
  // GLOutputSurface implementation:
  OverlayCandidateValidator* GetOverlayCandidateValidator() const override;

  std::unique_ptr<CompositorOverlayCandidateValidatorMac> overlay_validator_;

  DISALLOW_COPY_AND_ASSIGN(GLOutputSurfaceMac);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_GL_OUTPUT_SURFACE_MAC_H_
