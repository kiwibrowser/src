// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_GL_SURFACE_H_
#define ANDROID_WEBVIEW_BROWSER_AW_GL_SURFACE_H_

#include "base/macros.h"
#include "ui/gl/gl_surface.h"

namespace android_webview {

// This surface is used to represent the underlying surface provided by the App
// inside a hardware draw. Note that offscreen contexts will not be using this
// GLSurface.
class AwGLSurface : public gl::GLSurface {
 public:
  AwGLSurface();

  // Implement GLSurface.
  void Destroy() override;
  bool IsOffscreen() override;
  unsigned int GetBackingFramebufferObject() override;
  gfx::SwapResult SwapBuffers(const PresentationCallback& callback) override;
  gfx::Size GetSize() override;
  void* GetHandle() override;
  void* GetDisplay() override;
  gl::GLSurfaceFormat GetFormat() override;

 protected:
  ~AwGLSurface() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AwGLSurface);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_GL_SURFACE_H_
