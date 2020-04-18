// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_surface_osmesa_win.h"

#include <memory>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "base/win/windows_version.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/gl_surface_wgl.h"
#include "ui/gl/vsync_provider_win.h"

// From ANGLE's egl/eglext.h.
#if !defined(EGL_D3D11_ELSE_D3D9_DISPLAY_ANGLE)
#define EGL_D3D11_ELSE_D3D9_DISPLAY_ANGLE \
  reinterpret_cast<EGLNativeDisplayType>(-2)
#endif

namespace gl {

// Use BGRA, because StretchDIBits from RGBA causes later PrintWindow or
// BitBlt from the HWND to return all 0s, which causes the snapshot mechanism
// not to work.
GLSurfaceOSMesaWin::GLSurfaceOSMesaWin(gfx::AcceleratedWidget window)
    : GLSurfaceOSMesa(GLSurfaceFormat(GLSurfaceFormat::PIXEL_LAYOUT_BGRA),
                      gfx::Size(1, 1)),
      window_(window),
      device_context_(NULL) {
  DCHECK(window);
}

GLSurfaceOSMesaWin::~GLSurfaceOSMesaWin() {
  Destroy();
}

bool GLSurfaceOSMesaWin::Initialize(GLSurfaceFormat format) {
  if (!GLSurfaceOSMesa::Initialize(format))
    return false;

  device_context_ = GetDC(window_);
  return true;
}

void GLSurfaceOSMesaWin::Destroy() {
  if (window_ && device_context_)
    ReleaseDC(window_, device_context_);

  device_context_ = NULL;

  GLSurfaceOSMesa::Destroy();
}

bool GLSurfaceOSMesaWin::IsOffscreen() {
  return false;
}

gfx::SwapResult GLSurfaceOSMesaWin::SwapBuffers(
    const PresentationCallback& callback) {
  DCHECK(device_context_);

  gfx::Size size = GetSize();
  return PostSubBuffer(0, 0, size.width(), size.height(), callback);
}

bool GLSurfaceOSMesaWin::SupportsPresentationCallback() {
  return true;
}

bool GLSurfaceOSMesaWin::SupportsPostSubBuffer() {
  return true;
}

gfx::SwapResult GLSurfaceOSMesaWin::PostSubBuffer(
    int x,
    int y,
    int width,
    int height,
    const PresentationCallback& callback) {
  DCHECK(device_context_);

  gfx::Size size = GetSize();

  // Note: negating the height below causes GDI to treat the bitmap data as row
  // 0 being at the top.
  BITMAPV4HEADER info = {sizeof(BITMAPV4HEADER)};
  info.bV4Width = size.width();
  info.bV4Height = -size.height();
  info.bV4Planes = 1;
  info.bV4BitCount = 32;
  info.bV4V4Compression = BI_BITFIELDS;
  info.bV4RedMask = 0x00FF0000;
  info.bV4GreenMask = 0x0000FF00;
  info.bV4BlueMask = 0x000000FF;
  info.bV4AlphaMask = 0xFF000000;

  // Copy the back buffer to the window's device context. Do not check whether
  // StretchDIBits succeeds or not. It will fail if the window has been
  // destroyed but it is preferable to allow rendering to silently fail if the
  // window is destroyed. This is because the primary application of this
  // class of GLContext is for testing and we do not want every GL related ui /
  // browser test to become flaky if there is a race condition between GL
  // context destruction and window destruction.
  StretchDIBits(device_context_, x, size.height() - y - height, width, height,
                x, y, width, height, GetHandle(),
                reinterpret_cast<BITMAPINFO*>(&info), DIB_RGB_COLORS, SRCCOPY);

  constexpr int64_t kRefreshIntervalInMicroseconds =
      base::Time::kMicrosecondsPerSecond / 60;
  gfx::PresentationFeedback feedback(
      base::TimeTicks::Now(),
      base::TimeDelta::FromMicroseconds(kRefreshIntervalInMicroseconds),
      0 /* flags */);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback, feedback));
  return gfx::SwapResult::SWAP_ACK;
}

}  // namespace gl
