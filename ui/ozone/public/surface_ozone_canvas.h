// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PUBLIC_SURFACE_OZONE_CANVAS_H_
#define UI_OZONE_PUBLIC_SURFACE_OZONE_CANVAS_H_

#include <memory>

#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/ozone/ozone_base_export.h"

class SkSurface;

namespace gfx {
class Size;
class VSyncProvider;
}

namespace ui {

// The platform-specific part of an software output. The class is intended
// for use when no EGL/GLES2 acceleration is possible.
// This class owns any bits that the ozone implementation needs freed when
// the software output is destroyed.
class OZONE_BASE_EXPORT SurfaceOzoneCanvas {
 public:
  virtual ~SurfaceOzoneCanvas() {}

  // Returns an SkSurface for drawing on the window.
  virtual sk_sp<SkSurface> GetSurface() = 0;

  // Attempts to resize the canvas to match the viewport size. After
  // resizing, the compositor must call GetCanvas() to get the next
  // canvas - this invalidates any previous canvas from GetCanvas().
  virtual void ResizeCanvas(const gfx::Size& viewport_size) = 0;

  // Present the current canvas. After presenting, the compositor must
  // call GetCanvas() to get the next canvas - this invalidates any
  // previous canvas from GetCanvas().
  //
  // The implementation may assume that any pixels outside the damage
  // rectangle are unchanged since the previous call to PresentCanvas().
  virtual void PresentCanvas(const gfx::Rect& damage) = 0;

  // Returns a gfx::VsyncProvider for this surface. Note that this may be
  // called after we have entered the sandbox so if there are operations (e.g.
  // opening a file descriptor providing vsync events) that must be done
  // outside of the sandbox, they must have been completed in
  // InitializeHardware. Returns an empty scoped_ptr on error.
  virtual std::unique_ptr<gfx::VSyncProvider> CreateVSyncProvider() = 0;
};

}  // namespace ui

#endif  // UI_OZONE_PUBLIC_SURFACE_OZONE_CANVAS_H_
