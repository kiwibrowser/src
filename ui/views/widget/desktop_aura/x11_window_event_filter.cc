// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/x11_window_event_filter.h"

#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/hit_test.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/views/widget/widget.h"

namespace {

// These constants are defined in the Extended Window Manager Hints
// standard...and aren't in any header that I can find.
const int k_NET_WM_MOVERESIZE_SIZE_TOPLEFT =     0;
const int k_NET_WM_MOVERESIZE_SIZE_TOP =         1;
const int k_NET_WM_MOVERESIZE_SIZE_TOPRIGHT =    2;
const int k_NET_WM_MOVERESIZE_SIZE_RIGHT =       3;
const int k_NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT = 4;
const int k_NET_WM_MOVERESIZE_SIZE_BOTTOM =      5;
const int k_NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT =  6;
const int k_NET_WM_MOVERESIZE_SIZE_LEFT =        7;
const int k_NET_WM_MOVERESIZE_MOVE =             8;

}  // namespace

namespace views {

X11WindowEventFilter::X11WindowEventFilter(
    DesktopWindowTreeHost* window_tree_host)
    : WindowEventFilter(window_tree_host),
      xdisplay_(gfx::GetXDisplay()),
      xwindow_(window_tree_host->AsWindowTreeHost()->GetAcceleratedWidget()),
      x_root_window_(DefaultRootWindow(xdisplay_)) {}

X11WindowEventFilter::~X11WindowEventFilter() {
}

void X11WindowEventFilter::MaybeDispatchHostWindowDragMovement(
    int hittest,
    ui::MouseEvent* event) {
  if (event->IsLeftMouseButton() && event->native_event()) {
    // Get the |x_root_window_| location out of the native event.
    const gfx::Point x_root_location =
        ui::EventSystemLocationFromNative(event->native_event());
    if (DispatchHostWindowDragMovement(hittest, x_root_location))
      event->StopPropagation();
  }
}

void X11WindowEventFilter::LowerWindow() {
  XLowerWindow(xdisplay_, xwindow_);
}

bool X11WindowEventFilter::DispatchHostWindowDragMovement(
    int hittest,
    const gfx::Point& screen_location) {
  int direction = -1;
  switch (hittest) {
    case HTBOTTOM:
      direction = k_NET_WM_MOVERESIZE_SIZE_BOTTOM;
      break;
    case HTBOTTOMLEFT:
      direction = k_NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT;
      break;
    case HTBOTTOMRIGHT:
      direction = k_NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT;
      break;
    case HTCAPTION:
      direction = k_NET_WM_MOVERESIZE_MOVE;
      break;
    case HTLEFT:
      direction = k_NET_WM_MOVERESIZE_SIZE_LEFT;
      break;
    case HTRIGHT:
      direction = k_NET_WM_MOVERESIZE_SIZE_RIGHT;
      break;
    case HTTOP:
      direction = k_NET_WM_MOVERESIZE_SIZE_TOP;
      break;
    case HTTOPLEFT:
      direction = k_NET_WM_MOVERESIZE_SIZE_TOPLEFT;
      break;
    case HTTOPRIGHT:
      direction = k_NET_WM_MOVERESIZE_SIZE_TOPRIGHT;
      break;
    default:
      return false;
  }

  // We most likely have an implicit grab right here. We need to dump it
  // because what we're about to do is tell the window manager
  // that it's now responsible for moving the window around; it immediately
  // grabs when it receives the event below.
  XUngrabPointer(xdisplay_, x11::CurrentTime);

  XEvent event;
  memset(&event, 0, sizeof(event));
  event.xclient.type = ClientMessage;
  event.xclient.display = xdisplay_;
  event.xclient.window = xwindow_;
  event.xclient.message_type = gfx::GetAtom("_NET_WM_MOVERESIZE");
  event.xclient.format = 32;
  event.xclient.data.l[0] = screen_location.x();
  event.xclient.data.l[1] = screen_location.y();
  event.xclient.data.l[2] = direction;
  event.xclient.data.l[3] = 0;
  event.xclient.data.l[4] = 0;

  XSendEvent(xdisplay_, x_root_window_, x11::False,
             SubstructureRedirectMask | SubstructureNotifyMask, &event);

  return true;
}

}  // namespace views
