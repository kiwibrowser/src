// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_VISUAL_PICKER_GLX_H_
#define UI_GL_GL_VISUAL_PICKER_GLX_H_

#include <vector>

#include "base/macros.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/gl/gl_export.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace gl {

// Picks the best X11 visuals to use for GL.  This class is adapted from GTK's
// pick_better_visual_for_gl.  Tries to find visuals that
// 1. Support GL
// 2. Support double buffer
// 3. Have an alpha channel only if we want one
class GL_EXPORT GLVisualPickerGLX {
 public:
  static GLVisualPickerGLX* GetInstance();

  ~GLVisualPickerGLX();

  const XVisualInfo& system_visual() { return system_visual_; };

  const XVisualInfo& rgba_visual() { return rgba_visual_; };

 private:
  friend struct base::DefaultSingletonTraits<GLVisualPickerGLX>;

  XVisualInfo PickBestGlVisual(const std::vector<XVisualInfo>& visuals,
                               bool want_alpha) const;

  XVisualInfo PickBestSystemVisual(
      const std::vector<XVisualInfo>& visuals) const;

  XVisualInfo PickBestRgbaVisual(const std::vector<XVisualInfo>& visuals) const;

  XDisplay* display_;

  bool has_glx_visual_rating_;
  bool has_glx_multisample_;

  XVisualInfo system_visual_;
  XVisualInfo rgba_visual_;

  GLVisualPickerGLX();

  DISALLOW_COPY_AND_ASSIGN(GLVisualPickerGLX);
};

}  // namespace gl

#endif  // UI_GL_GL_VISUAL_PICKER_GLX_H_
