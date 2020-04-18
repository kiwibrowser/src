// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_CPU_SURFACE_PROVIDER_H_
#define CHROME_BROWSER_VR_CPU_SURFACE_PROVIDER_H_

#include "chrome/browser/vr/skia_surface_provider.h"

namespace vr {

// Creates a Skia surface for which drawing commands are executed on the CPU.
class CpuSurfaceProvider : public SkiaSurfaceProvider {
 public:
  CpuSurfaceProvider();
  ~CpuSurfaceProvider() override;

  sk_sp<SkSurface> MakeSurface(const gfx::Size& size) override;
  GLuint FlushSurface(SkSurface* surface, GLuint reuse_texture_id) override;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_CPU_SURFACE_PROVIDER_H_
