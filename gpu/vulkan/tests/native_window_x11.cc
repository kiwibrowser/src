// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/tests/native_window.h"

#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"

namespace gpu {

gfx::AcceleratedWidget CreateNativeWindow(const gfx::Rect& bounds) {
  XDisplay* display = gfx::GetXDisplay();
  XSetWindowAttributes swa;
  swa.event_mask = StructureNotifyMask | ExposureMask;
  swa.override_redirect = x11::True;
  XID window = XCreateWindow(
      display, XRootWindow(display, DefaultScreen(display)),  // parent
      bounds.x(), bounds.y(), bounds.width(), bounds.height(),
      0,               // border width
      CopyFromParent,  // depth
      InputOutput,
      CopyFromParent,  // visual
      CWEventMask | CWOverrideRedirect, &swa);
  XMapWindow(display, window);

  while (1) {
    XEvent event;
    XNextEvent(display, &event);
    if (event.type == MapNotify && event.xmap.window == window)
      break;
  }

  return window;
}

void DestroyNativeWindow(gfx::AcceleratedWidget window) {
  XDestroyWindow(gfx::GetXDisplay(), window);
}

}  // namespace gpu
