// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SURFACE_MANAGER_H_
#define MEDIA_BASE_SURFACE_MANAGER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "media/base/media_export.h"
#include "ui/gfx/geometry/size.h"

namespace media {

using SurfaceCreatedCB = base::Callback<void(int)>;
using RequestSurfaceCB = base::Callback<void(bool, const SurfaceCreatedCB&)>;

class MEDIA_EXPORT SurfaceManager {
 public:
  enum { kNoSurfaceID = -1 };

  SurfaceManager() {}
  virtual ~SurfaceManager() {}

  // Create a fullscreen surface. The id will be returned with
  // |surface_created_cb|. If this is called more than once before the first
  // |surface_created_cb| is called, the surface will be delivered to the last
  // caller. If this is called after the fullscreen surface is created, the
  // existing surface will be returned. The client should ensure that the
  // previous consumer is no longer using the surface.
  virtual void CreateFullscreenSurface(
      const gfx::Size& video_natural_size,
      const SurfaceCreatedCB& surface_created_cb) = 0;

  // Call this when the natural size of the fullscreen video changes. The
  // surface will be resized to match the aspect ratio.
  virtual void NaturalSizeChanged(const gfx::Size& size) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SurfaceManager);
};

}  // namespace media

#endif  // MEDIA_BASE_SURFACE_MANAGER_H_
