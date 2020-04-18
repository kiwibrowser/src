// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_surface_overlay.h"

#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/gpu_fence.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/overlay_transform.h"
#include "ui/gl/gl_image.h"

namespace gl {

GLSurfaceOverlay::GLSurfaceOverlay(int z_order,
                                   gfx::OverlayTransform transform,
                                   GLImage* image,
                                   const gfx::Rect& bounds_rect,
                                   const gfx::RectF& crop_rect,
                                   bool enable_blend,
                                   std::unique_ptr<gfx::GpuFence> gpu_fence)
    : z_order_(z_order),
      transform_(transform),
      image_(image),
      bounds_rect_(bounds_rect),
      crop_rect_(crop_rect),
      enable_blend_(enable_blend),
      gpu_fence_(std::move(gpu_fence)) {}

GLSurfaceOverlay::GLSurfaceOverlay(GLSurfaceOverlay&& other) = default;

GLSurfaceOverlay::~GLSurfaceOverlay() {}

bool GLSurfaceOverlay::ScheduleOverlayPlane(
    gfx::AcceleratedWidget widget) const {
  return image_->ScheduleOverlayPlane(widget, z_order_, transform_,
                                      bounds_rect_, crop_rect_, enable_blend_,
                                      gpu_fence_.get());
}

void GLSurfaceOverlay::Flush() const {
  return image_->Flush();
}

}  // namespace gl
