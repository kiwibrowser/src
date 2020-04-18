// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_GANESH_SURFACE_PROVIDER_H_
#define CHROME_BROWSER_VR_GANESH_SURFACE_PROVIDER_H_

#include "chrome/browser/vr/skia_surface_provider.h"

class GrContext;

namespace vr {

// Creates a Skia surface for which drawing commands are executed on the GPU.
class GaneshSurfaceProvider : public SkiaSurfaceProvider {
 public:
  GaneshSurfaceProvider();
  ~GaneshSurfaceProvider() override;

  sk_sp<SkSurface> MakeSurface(const gfx::Size& size) override;
  GLuint FlushSurface(SkSurface* surface, GLuint reuse_texture_id) override;

 private:
  sk_sp<GrContext> gr_context_;
  GLint main_fbo_ = 0;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_GANESH_SURFACE_PROVIDER_H_
