// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_surface_osmesa_x11.h"

#include <stdint.h>

#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"

namespace gl {

GLSurfaceOSMesaX11::GLSurfaceOSMesaX11(gfx::AcceleratedWidget window)
    : GLSurfaceOSMesa(
          GLSurfaceFormat(GLSurfaceFormat::PIXEL_LAYOUT_BGRA),
          gfx::Size(1, 1)),
      xdisplay_(gfx::GetXDisplay()),
      window_graphics_context_(0),
      window_(window),
      pixmap_graphics_context_(0),
      pixmap_(0) {
  DCHECK(xdisplay_);
  DCHECK(window_);
}

// static
bool GLSurfaceOSMesaX11::InitializeOneOff() {
  static bool initialized = false;
  if (initialized)
    return true;

  if (!gfx::GetXDisplay()) {
    LOG(ERROR) << "XOpenDisplay failed.";
    return false;
  }

  initialized = true;
  return true;
}

bool GLSurfaceOSMesaX11::Initialize(GLSurfaceFormat format) {
  if (!GLSurfaceOSMesa::Initialize(format))
    return false;

  window_graphics_context_ = XCreateGC(xdisplay_, window_, 0, NULL);
  if (!window_graphics_context_) {
    LOG(ERROR) << "XCreateGC failed.";
    Destroy();
    return false;
  }

  return true;
}

void GLSurfaceOSMesaX11::Destroy() {
  if (pixmap_graphics_context_) {
    XFreeGC(xdisplay_, pixmap_graphics_context_);
    pixmap_graphics_context_ = NULL;
  }

  if (pixmap_) {
    XFreePixmap(xdisplay_, pixmap_);
    pixmap_ = 0;
  }

  if (window_graphics_context_) {
    XFreeGC(xdisplay_, window_graphics_context_);
    window_graphics_context_ = NULL;
  }

  XSync(xdisplay_, x11::False);
}

bool GLSurfaceOSMesaX11::Resize(const gfx::Size& new_size,
                                float scale_factor,
                                ColorSpace color_space,
                                bool alpha) {
  if (!GLSurfaceOSMesa::Resize(new_size, scale_factor, color_space, alpha))
    return false;

  XWindowAttributes attributes;
  if (!XGetWindowAttributes(xdisplay_, window_, &attributes)) {
    LOG(ERROR) << "XGetWindowAttributes failed for window " << window_ << ".";
    return false;
  }

  // Destroy the previous pixmap and graphics context.
  if (pixmap_graphics_context_) {
    XFreeGC(xdisplay_, pixmap_graphics_context_);
    pixmap_graphics_context_ = NULL;
  }
  if (pixmap_) {
    XFreePixmap(xdisplay_, pixmap_);
    pixmap_ = 0;
  }

  // Recreate a pixmap to hold the frame.
  pixmap_ = XCreatePixmap(xdisplay_, window_, new_size.width(),
                          new_size.height(), attributes.depth);
  if (!pixmap_) {
    LOG(ERROR) << "XCreatePixmap failed.";
    return false;
  }

  // Recreate a graphics context for the pixmap.
  pixmap_graphics_context_ = XCreateGC(xdisplay_, pixmap_, 0, NULL);
  if (!pixmap_graphics_context_) {
    LOG(ERROR) << "XCreateGC failed";
    return false;
  }

  return true;
}

bool GLSurfaceOSMesaX11::IsOffscreen() {
  return false;
}

gfx::SwapResult GLSurfaceOSMesaX11::SwapBuffers(
    const PresentationCallback& callback) {
  // TODO(penghuang): Provide useful presentation feedback.
  // https://crbug.com/776877
  TRACE_EVENT2("gpu", "GLSurfaceOSMesaX11:RealSwapBuffers", "width",
               GetSize().width(), "height", GetSize().height());

  gfx::Size size = GetSize();

  XWindowAttributes attributes;
  if (!XGetWindowAttributes(xdisplay_, window_, &attributes)) {
    LOG(ERROR) << "XGetWindowAttributes failed for window " << window_ << ".";
    return gfx::SwapResult::SWAP_FAILED;
  }

  // Copy the frame into the pixmap.
  gfx::PutARGBImage(xdisplay_, attributes.visual, attributes.depth, pixmap_,
                    pixmap_graphics_context_,
                    static_cast<const uint8_t*>(GetHandle()), size.width(),
                    size.height());

  // Copy the pixmap to the window.
  XCopyArea(xdisplay_, pixmap_, window_, window_graphics_context_, 0, 0,
            size.width(), size.height(), 0, 0);

  return gfx::SwapResult::SWAP_ACK;
}

bool GLSurfaceOSMesaX11::SupportsPostSubBuffer() {
  return true;
}

gfx::SwapResult GLSurfaceOSMesaX11::PostSubBuffer(
    int x,
    int y,
    int width,
    int height,
    const PresentationCallback& callback) {
  gfx::Size size = GetSize();

  // Move (0,0) from lower-left to upper-left
  y = size.height() - y - height;

  XWindowAttributes attributes;
  if (!XGetWindowAttributes(xdisplay_, window_, &attributes)) {
    LOG(ERROR) << "XGetWindowAttributes failed for window " << window_ << ".";
    return gfx::SwapResult::SWAP_FAILED;
  }

  // Copy the frame into the pixmap.
  gfx::PutARGBImage(xdisplay_, attributes.visual, attributes.depth, pixmap_,
                    pixmap_graphics_context_,
                    static_cast<const uint8_t*>(GetHandle()), size.width(),
                    size.height(), x, y, x, y, width, height);

  // Copy the pixmap to the window.
  XCopyArea(xdisplay_, pixmap_, window_, window_graphics_context_, x, y, width,
            height, x, y);

  constexpr int64_t kRefreshIntervalInMicroseconds =
      base::Time::kMicrosecondsPerSecond / 60;
  callback.Run(gfx::PresentationFeedback(
      base::TimeTicks::Now(),
      base::TimeDelta::FromMicroseconds(kRefreshIntervalInMicroseconds),
      0 /* flags */));

  return gfx::SwapResult::SWAP_ACK;
}

bool GLSurfaceOSMesaX11::SupportsPresentationCallback() {
  return true;
}

GLSurfaceOSMesaX11::~GLSurfaceOSMesaX11() {
  Destroy();
}

}  // namespace gl
