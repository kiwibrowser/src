// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_OVERLAY_H_
#define UI_GL_GL_SURFACE_OVERLAY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/overlay_transform.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_image.h"

namespace gfx {
class GpuFence;
}  // namespace gfx

namespace gl {

// For saving the properties of a GLImage overlay plane and scheduling it later.
class GL_EXPORT GLSurfaceOverlay {
 public:
  GLSurfaceOverlay(int z_order,
                   gfx::OverlayTransform transform,
                   GLImage* image,
                   const gfx::Rect& bounds_rect,
                   const gfx::RectF& crop_rect,
                   bool enable_blend,
                   std::unique_ptr<gfx::GpuFence> gpu_fence);
  GLSurfaceOverlay(GLSurfaceOverlay&& other);
  ~GLSurfaceOverlay();

  // Schedule the image as an overlay plane to be shown at swap time for
  // |widget|. The caller needs to ensure that after calling this method the
  // object will stay valid for as long as the associated gpu fence needs to
  // remain valid (typically until after displaying the overlay).
  bool ScheduleOverlayPlane(gfx::AcceleratedWidget widget) const;

  void Flush() const;

 private:
  int z_order_;
  gfx::OverlayTransform transform_;
  scoped_refptr<GLImage> image_;
  gfx::Rect bounds_rect_;
  gfx::RectF crop_rect_;
  bool enable_blend_;
  std::unique_ptr<gfx::GpuFence> gpu_fence_;
};

}  // namespace gl

#endif  // UI_GL_GL_SURFACE_OVERLAY_H_
