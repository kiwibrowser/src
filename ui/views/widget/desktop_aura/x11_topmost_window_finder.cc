// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/x11_topmost_window_finder.h"

#include <stddef.h>

#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/window.h"
#include "ui/gfx/x/x11.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_x11.h"

namespace views {

X11TopmostWindowFinder::X11TopmostWindowFinder() : toplevel_(x11::None) {}

X11TopmostWindowFinder::~X11TopmostWindowFinder() {
}

aura::Window* X11TopmostWindowFinder::FindLocalProcessWindowAt(
    const gfx::Point& screen_loc_in_pixels,
    const std::set<aura::Window*>& ignore) {
  screen_loc_in_pixels_ = screen_loc_in_pixels;
  ignore_ = ignore;

  std::vector<aura::Window*> local_process_windows =
      DesktopWindowTreeHostX11::GetAllOpenWindows();
  bool found_local_process_window = false;
  for (size_t i = 0; i < local_process_windows.size(); ++i) {
    if (ShouldStopIteratingAtLocalProcessWindow(local_process_windows[i])) {
      found_local_process_window = true;
      break;
    }
  }
  if (!found_local_process_window)
    return NULL;

  ui::EnumerateTopLevelWindows(this);
  return DesktopWindowTreeHostX11::GetContentWindowForXID(toplevel_);
}

XID X11TopmostWindowFinder::FindWindowAt(
    const gfx::Point& screen_loc_in_pixels) {
  screen_loc_in_pixels_ = screen_loc_in_pixels;
  ui::EnumerateTopLevelWindows(this);
  return toplevel_;
}

bool X11TopmostWindowFinder::ShouldStopIterating(XID xid) {
  if (!ui::IsWindowVisible(xid))
    return false;

  aura::Window* window =
      views::DesktopWindowTreeHostX11::GetContentWindowForXID(xid);
  if (window) {
    if (ShouldStopIteratingAtLocalProcessWindow(window)) {
      toplevel_ = xid;
      return true;
    }
    return false;
  }

  if (ui::WindowContainsPoint(xid, screen_loc_in_pixels_)) {
    toplevel_ = xid;
    return true;
  }
  return false;
}

bool X11TopmostWindowFinder::ShouldStopIteratingAtLocalProcessWindow(
    aura::Window* window) {
  if (ignore_.find(window) != ignore_.end())
    return false;

  // Currently |window|->IsVisible() always returns true.
  // TODO(pkotwicz): Fix this. crbug.com/353038
  if (!window->IsVisible())
    return false;

  DesktopWindowTreeHostX11* host =
      DesktopWindowTreeHostX11::GetHostForXID(
          window->GetHost()->GetAcceleratedWidget());
  if (!host->GetX11RootWindowOuterBounds().Contains(screen_loc_in_pixels_))
    return false;

  ::Region shape = host->GetWindowShape();
  if (!shape)
    return true;

  aura::client::ScreenPositionClient* screen_position_client =
      aura::client::GetScreenPositionClient(window->GetRootWindow());
  gfx::Point window_loc(screen_loc_in_pixels_);
  screen_position_client->ConvertPointFromScreen(window, &window_loc);
  return XPointInRegion(shape, window_loc.x(), window_loc.y()) == x11::True;
}

}  // namespace views
