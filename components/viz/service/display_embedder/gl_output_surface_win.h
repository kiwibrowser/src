// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_GL_OUTPUT_SURFACE_WIN_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_GL_OUTPUT_SURFACE_WIN_H_

#include <memory>

#include "base/macros.h"
#include "components/viz/service/display_embedder/gl_output_surface.h"

namespace viz {

class CompositorOverlayCandidateValidatorWin;

class GLOutputSurfaceWin : public GLOutputSurface {
 public:
  GLOutputSurfaceWin(scoped_refptr<VizProcessContextProvider> context_provider,
                     SyntheticBeginFrameSource* synthetic_begin_frame_source,
                     bool use_overlays);
  ~GLOutputSurfaceWin() override;

  // GLOutputSurface implementation:
  OverlayCandidateValidator* GetOverlayCandidateValidator() const override;

 private:
  std::unique_ptr<CompositorOverlayCandidateValidatorWin> overlay_validator_;

  DISALLOW_COPY_AND_ASSIGN(GLOutputSurfaceWin);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_GL_OUTPUT_SURFACE_WIN_H_
