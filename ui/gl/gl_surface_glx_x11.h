// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_GLX_X11_H_
#define UI_GL_GL_SURFACE_GLX_X11_H_

#include "base/macros.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_surface_glx.h"

namespace gl {

// X11 specific implementation of GLX surface. Registers as a
// PlatformEventDispatcher to handle XEvents.
class GL_EXPORT GLSurfaceGLXX11 : public NativeViewGLSurfaceGLX,
                                  public ui::PlatformEventDispatcher {
 public:
  explicit GLSurfaceGLXX11(gfx::AcceleratedWidget window);

 protected:
  ~GLSurfaceGLXX11() override;

  // NativeViewGLSurfaceGLX:
  void RegisterEvents() override;
  void UnregisterEvents() override;

  // PlatformEventDispatcher:
  bool CanDispatchEvent(const ui::PlatformEvent& event) override;
  uint32_t DispatchEvent(const ui::PlatformEvent& event) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GLSurfaceGLXX11);
};

}  // namespace gl

#endif  // UI_GL_GL_SURFACE_GLX_X11_H_
