// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/x11_event_sender.h"

#include "ui/aura/window_tree_host.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/x/x11.h"

namespace aura {
namespace test {

void PostEventToWindowTreeHost(const XEvent& xevent, WindowTreeHost* host) {
  XDisplay* xdisplay = gfx::GetXDisplay();
  XID xwindow = host->GetAcceleratedWidget();
  XEvent event = xevent;
  event.xany.display = xdisplay;
  event.xany.window = xwindow;

  switch (event.type) {
    case EnterNotify:
    case LeaveNotify:
    case MotionNotify:
    case KeyPress:
    case KeyRelease:
    case ButtonPress:
    case ButtonRelease: {
      // The fields used below are in the same place for all of events
      // above. Using xmotion from XEvent's unions to avoid repeating
      // the code.
      event.xmotion.root = DefaultRootWindow(event.xany.display);
      event.xmotion.time = x11::CurrentTime;

      gfx::Point point(event.xmotion.x, event.xmotion.y);
      host->ConvertDIPToScreenInPixels(&point);
      event.xmotion.x_root = point.x();
      event.xmotion.y_root = point.y();
      break;
    }
    default:
      break;
  }
  XSendEvent(xdisplay, xwindow, x11::False, 0, &event);
  XFlush(xdisplay);
}

}  // namespace test
}  // namespace aura
