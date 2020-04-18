// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_surface_glx_x11.h"

#include "ui/events/platform/platform_event_source.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"

namespace gl {

GLSurfaceGLXX11::GLSurfaceGLXX11(gfx::AcceleratedWidget window)
    : NativeViewGLSurfaceGLX(window) {}

GLSurfaceGLXX11::~GLSurfaceGLXX11() {
  Destroy();
}

void GLSurfaceGLXX11::RegisterEvents() {
  auto* event_source = ui::PlatformEventSource::GetInstance();
  // Can be null in tests, when we don't care about Exposes.
  if (event_source) {
    XSelectInput(gfx::GetXDisplay(), window(), ExposureMask);
    event_source->AddPlatformEventDispatcher(this);
  }
}

void GLSurfaceGLXX11::UnregisterEvents() {
  auto* event_source = ui::PlatformEventSource::GetInstance();
  if (event_source)
    event_source->RemovePlatformEventDispatcher(this);
}

bool GLSurfaceGLXX11::CanDispatchEvent(const ui::PlatformEvent& event) {
  return CanHandleEvent(event);
}

uint32_t GLSurfaceGLXX11::DispatchEvent(const ui::PlatformEvent& event) {
  ForwardExposeEvent(event);
  return ui::POST_DISPATCH_STOP_PROPAGATION;
}

}  // namespace gl
