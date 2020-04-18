// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/x11/gl_surface_egl_ozone_x11.h"

#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/gl/egl_util.h"

namespace ui {

GLSurfaceEGLOzoneX11::GLSurfaceEGLOzoneX11(EGLNativeWindowType window)
    : NativeViewGLSurfaceEGL(window, nullptr) {}

EGLConfig GLSurfaceEGLOzoneX11::GetConfig() {
  // Try matching the window depth with an alpha channel, because we're worried
  // the destination alpha width could constrain blending precision.
  const int kBufferSizeOffset = 1;
  const int kAlphaSizeOffset = 3;
  EGLint config_attribs[] = {EGL_BUFFER_SIZE,
                             ~0,  // To be replaced.
                             EGL_ALPHA_SIZE,
                             8,
                             EGL_BLUE_SIZE,
                             8,
                             EGL_GREEN_SIZE,
                             8,
                             EGL_RED_SIZE,
                             8,
                             EGL_RENDERABLE_TYPE,
                             EGL_OPENGL_ES2_BIT,
                             EGL_SURFACE_TYPE,
                             EGL_WINDOW_BIT,
                             EGL_NONE};

  // Get the depth of XWindow for surface.
  XWindowAttributes win_attribs;
  if (XGetWindowAttributes(gfx::GetXDisplay(), window_, &win_attribs)) {
    config_attribs[kBufferSizeOffset] = win_attribs.depth;
  }

  EGLDisplay display = GetDisplay();

  EGLConfig config;
  EGLint num_configs;
  if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs)) {
    LOG(ERROR) << "eglChooseConfig failed with error "
               << GetLastEGLErrorString();
    return nullptr;
  }

  if (num_configs > 0) {
    EGLint config_depth;
    if (!eglGetConfigAttrib(display, config, EGL_BUFFER_SIZE, &config_depth)) {
      LOG(ERROR) << "eglGetConfigAttrib failed with error "
                 << GetLastEGLErrorString();
      return nullptr;
    }
    if (config_depth == config_attribs[kBufferSizeOffset]) {
      return config;
    }
  }

  // Try without an alpha channel.
  config_attribs[kAlphaSizeOffset] = 0;
  if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs)) {
    LOG(ERROR) << "eglChooseConfig failed with error "
               << GetLastEGLErrorString();
    return nullptr;
  }

  if (num_configs == 0) {
    LOG(ERROR) << "No suitable EGL configs found.";
    return nullptr;
  }
  return config;
}

bool GLSurfaceEGLOzoneX11::Resize(const gfx::Size& size,
                                  float scale_factor,
                                  ColorSpace color_space,
                                  bool has_alpha) {
  if (size == GetSize())
    return true;

  size_ = size;

  eglWaitGL();
  XResizeWindow(gfx::GetXDisplay(), window_, size.width(), size.height());
  eglWaitNative(EGL_CORE_NATIVE_ENGINE);

  return true;
}

GLSurfaceEGLOzoneX11::~GLSurfaceEGLOzoneX11() {
  Destroy();
}

}  // namespace ui
